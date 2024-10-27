#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "modbus.h"
#include "CThread.h"
#include <signal.h>
#include <string.h>
#include "SystemSleep.h"
#include "cbus_io.h"
#ifdef __TARGET_LINUX__
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

//! Number of input bits
//#define DIRECT_INPUTS_NUMBER        16
//! Number of output bits
//#define COILS_NUMBER                16
//! Number of input registers (ADC)
//#define INPUT_REGISTERS_NUMBER      0
//! Number of output registers (DAC)
//#define OUTPUT_REGISTERS_NUMBER     0

modbus_t* ctx = 0;
int ModbusListenSocket = -1;
modbus_mapping_t* mb_mapping = 0;
CThread* ModbusThread = 0;
unsigned char BreakRequest=0;

void* ModbusThreadFunc (CThread *Control)
{
    uint8_t ModbusQuery[1500];
    int rc=0;

    modbus_tcp_accept (ctx, &ModbusListenSocket);

    while ((Control->ShouldStop==false)&&(rc!=-1))
    {
        // modbus_receive will return -1 if socket is closed
        rc = modbus_receive (ctx, &ModbusQuery[0]);
		if (rc>0)
		{
			modbus_reply (ctx, &ModbusQuery[0], rc, mb_mapping);
        }
    }

    Control->IsStopped=true;
	pthread_exit(NULL);
	return 0;
}  // ModbusThreadFunc
// --------------------------------

void Terminate (void)
{
    closeCBUSDriver();

    if (ModbusThread)
    {
        if (ModbusListenSocket!=-1)
        {
#ifdef __TARGET_LINUX__
            close (ModbusListenSocket);
#endif
#ifdef __TARGET_WIN__
			closesocket(ModbusListenSocket);
#endif
            ModbusListenSocket = -1;
            ModbusThread->StopThread(500);
            delete ModbusThread;
            ModbusThread = 0;
        }
    }

    if (ModbusListenSocket!=-1)
    {
#ifdef __TARGET_LINUX__
        close (ModbusListenSocket);
#endif
#ifdef __TARGET_WIN__
		closesocket (ModbusListenSocket);
#endif
        ModbusListenSocket = -1;
    }

    if (mb_mapping!=0)
    {
        modbus_mapping_free(mb_mapping);
        mb_mapping = 0;
    }

    if (ctx!=0)
    {
        modbus_close (ctx);
        modbus_free (ctx);
        ctx = 0;
    }

#ifdef __TARGET_WIN__
	CloseNetwork();
#endif
}  // Terminate
// --------------------------------

void sig_handler (int signo)
{
    if (signo == SIGINT)
    {
        printf ("Termination requested by user!\n");
        BreakRequest = 1;
        Terminate();
    }
}  // sig_handler
// --------------------------------

//! Exchange Modbus data with the CBUS handler
void UpdateModbusData (void)
{
    //int RegisterNumber;
    int InputNumber;
    int CoilNumber;

    acquireCBUSPLCInputs();
    for (InputNumber=0; InputNumber<NUM_CBUS_BOOL_INPUTS; InputNumber++)
    {
        mb_mapping->tab_input_bits[InputNumber] = CBUS_PLC_BoolInput[InputNumber];
    }

    for (CoilNumber=0; CoilNumber<NUM_CBUS_BOOL_OUTPUTS; CoilNumber++)
    {
        CBUS_PLC_BoolOutput[CoilNumber] = mb_mapping->tab_bits[CoilNumber];
    }
    updateCBUSPLCOutputs();
}  // UpdateModbusData
// --------------------------------

//! Parse options on command line
void ParseCLIParameters (int argc, char* argv[])
{
    int ParmCount;
    int TestInt;

    if (argc<2) return;

    for (ParmCount = 1; ParmCount<argc; ParmCount++)
    {
        if (strcmp(argv[ParmCount], "--verbose") == 0)
        {
            // Make sure we have one argument following (verbosity level)
            if (ParmCount >= (argc - 1))
            {
                fprintf (stderr, "Missing or invalid value for parameter --verbose\n");
                return;
            }

            TestInt = atoi (argv[ParmCount + 1]);
            if (TestInt<0) TestInt = 0;
            VerbosityLevel = TestInt;

            ParmCount += 1;     // Jump over the argument value
        }
    }
}  // ParseCLIParameters
// --------------------------------

int main(int argc, char* argv[])
{
    int MaxPrio;
    int CBUSResult;

	fprintf (stdout, "cbus2modbus : MERG CBUS to Modbus gateway - V0.1\n");
	fprintf (stdout, "(c) Benoit BOUCHEZ - 2024\n");

	ParseCLIParameters(argc, argv);

    signal (SIGINT, sig_handler);       // Make sure we terminate application gracefully

    CBUSResult = startCBUSDriver("can0");
    if (CBUSResult != 0)
    {
        if (CBUSResult == -1)
            fprintf (stdout, "Missing or corrupted PLC inputs configuration file\n");
        else if (CBUSResult == -2)
            fprintf (stdout, "Missing or corrupted PLC outputs configuration file\n");
        else
            fprintf (stdout, "Can not create can0 communication socket");
        fprintf (stdout, "Exiting cbus2modbus\n");
    }

   	// Use port 1502 as 502 requires root to be opened (not possible from CodeBlocks)
   	fprintf (stdout, "Using TCP port 1502 for Modbus/TCP server\n");
	ctx = modbus_new_tcp (NULL, 1502);		// Problem on Windows : if we set NULL for IP, libmodbus crashes... but apparently, the IP address does not matter (we can connect from 127.0.0.1 using 192.168.0.36 here)
	if (ctx == NULL)
	{
        fprintf (stderr, "Error : Unable to allocate modbus context\n");
        Terminate();
        return -1;
	}

	ModbusListenSocket = modbus_tcp_listen (ctx, 1);    // Maximum one connection
	if (ModbusListenSocket==-1)
	{
        fprintf (stderr, "%s\n", strerror(errno));
        fprintf (stderr, "Error : Can not create TCP socket for Modbus\n");
        Terminate ();
        return -1;
	}

	mb_mapping = modbus_mapping_new (NUM_CBUS_BOOL_OUTPUTS, NUM_CBUS_BOOL_INPUTS, 0, 0);
    if (mb_mapping==0)
    {
        fprintf (stderr, "Error : Unable to allocate Modbus mapping\n");
        Terminate();
        return -1;
    }

    // Start Modbus thread
    MaxPrio = sched_get_priority_max(SCHED_FIFO);
    ModbusThread = new CThread ((ThreadFuncType*)ModbusThreadFunc, MaxPrio, 0);
    if (ModbusThread==0)
    {
        fprintf (stderr, "Error : can not create Modbus communication thread\n");
    }

    while (BreakRequest==0)
    {
        // Modbus thread will exist each time the client closes the connexion
        // So we have to restart it each time until we want to close the program
        if (ModbusThread->IsStopped)
        {
            delete ModbusThread;      // Remove terminated thread from memory

            // Respawn the thread to accept a new communication
            fprintf (stdout, "Modbus thread respawned...\n");
            ModbusThread = new CThread ((ThreadFuncType*)ModbusThreadFunc, MaxPrio, 0);
            // TODO : display a warning message if Modbus thread can not be respawned successfully
        }

        ProcessCBUS_IO ();
        UpdateModbusData();

        SystemSleepMillis(1);
    }

    Terminate ();
    fprintf (stdout, "Done!\n");

    return 0;
}
