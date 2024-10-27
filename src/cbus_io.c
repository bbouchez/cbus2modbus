/*
cbus_io.c
cbus2modbus
CBUS communication processing to update local Modbus images
Development : Benoit BOUCHEZ - M8718

+ Event I/O
The driver listens to events from external producers and can produce events for consumers
Consumed events are visible as digital inputs. When an event is received, its state is
transferred to a PLC digital input if it is declared in the PLC configuration.

Produced events are seen as digital outputs. when a digital output is changed in the PLC,
an event is produced.

To avoid CBUS overflow, events are not refreshed for each PLC cycle. Each event is associated
with a freshness counter, which is reset when an event is received or transmitted.
If freshness counter reaches 0, a request is sent by the PLC to update its image in case
an event has been missed (PLC disconnected / stopped when event is generated)
Same method is used for produced events :  each time a digital output changes, its freshness
counter is reloaded. If freshness counter reaches 0, the even is produced generated again
to make sure consumers are updated even if they have lost connection for some reason

+ Generic CBUS access for consist control
A queue is provided between the PLC runtime and the driver. The runtime can send CBUS messaages
via a Function Block, which are queued to avoid runtime blocking in case cansocket does not
return immediately.
The CBUS driver thread sends the queued message each time its thread is reactivated

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include "CBUS_OPC.h"
#include "cbus_io.h"
#include "SocketCBUS.h"

//! CBUS CAN message for the queue from PLC to driver
typedef struct {
    unsigned int ID;
    unsigned int DLC;
    unsigned char Data[8];
} TCBUSMsg;

typedef struct {
	uint8_t CurrentOutput;		// Output state set by PLC
	uint8_t LastOutput;			// Last state sent to CBUS
	uint32_t LastRefresh;
	uint32_t CBUSDeviceNumber;		// 0 = entry not used
	uint32_t CBUSEventNumber;
} TCBUS_OUTPUT_CTRL;

typedef struct {
	uint8_t CurrentInput;
	uint32_t LastRefresh;
	uint32_t CBUSDeviceNumber;		// 0 = entry not used
	uint32_t CBUSEventNumber;
} TCBUS_INPUT_CTRL;

#define REFRESH_OUTPUT_TIMEOUT	30000		// 30 seconds
#define REFRESH_INPUT_TIMEOUT	30000		// 30 seconds

//! Precomputed CBUS ID. Value is from 0 to 2047
static unsigned int CBUS_ID = 0x2FF;
static int CBUS_MAJOR_PRIORITY = 0;   // value is 0 (max) to 2 (3 is not allowed)
static int CBUS_MINOR_PRIORITY = 0;   // value is 0 (max) to 3

uint8_t CANSocketReady = 0;     // False until cansocket is opened successfully

// Boolean I/O images for the PLC. These images are sampled at PLC level for the current PLC cycle (they do not change during a PLC cycle)
uint8_t CBUS_PLC_BoolInput[NUM_CBUS_BOOL_INPUTS];
// Asynchronous inputs from CBUS (updated dynamically when a CBUS message is received: they may change in the middle of a PLC cycle)
TCBUS_INPUT_CTRL CBUS_InCtrl[NUM_CBUS_BOOL_INPUTS];

pthread_mutex_t IntermediateInputBufferLock;

// We do no need intermediate buffers for output. When PLC writes an output, it is sent by the background thread to the CBUS
// It does not matter if they change in the middle of a PLC cycle as there is not timing relationship ensure between each signal
TCBUS_OUTPUT_CTRL CBUS_OutCtrl[NUM_CBUS_BOOL_OUTPUTS];
uint8_t CBUS_PLC_BoolOutput[NUM_CBUS_BOOL_OUTPUTS];

const char* TokenDelimiter = " ,\r\n";

unsigned int VerbosityLevel = 0;

//! Read I/O configuration file to associate PLC I/Os to CBUS events
// Each line in the file corresponds to a PLC boolean input. The two values are
// - input number
// - node number
// - event number
int ReadCBUSInputsConfig (void)
{
    FILE* ConfigFile;
    char Buffer [256];
    char* Token;
    int InputNumber, NN, EN;  // Node Number, Event Number

    if (VerbosityLevel > 0)
        fprintf (stdout, "Reading CBUS input configuration file...\n");

    ConfigFile = fopen ("cbus_inputs.dat", "rt");
    if (ConfigFile!=0)
    {
        while (fgets(Buffer, 256, ConfigFile))
        {
            // Ignore line if starting by a # -> this is a comment
            if (Buffer[0]!='#')
            {
                // Avoid to create an entry if there is an error in the configuration file
                InputNumber = -1;
                EN = -1;
                NN = -1;

                // Get first part of the string (input number)
                Token = strtok (Buffer, TokenDelimiter);
                if (Token)
                    InputNumber = atoi (Token);

                // Get second part of string (node number)
                // Use NULL as we need to continue with the current strtok
                // Using buffer as first parameter would initiate a new tokenization
                Token = strtok (NULL, TokenDelimiter);
                if (Token)
                    NN = atoi (Token);

                // Get last part of the string (event number)
                Token = strtok (NULL, TokenDelimiter);
                if (Token)
                    EN = atoi (Token);

                if ((InputNumber<NUM_CBUS_BOOL_INPUTS)&&(InputNumber>=0))
                {
                    if ((NN>0)&&(NN<65535)&&(EN>=0)&&(EN<65535))
                    {
                        if (VerbosityLevel > 0)
                            fprintf (stdout, "Input:%d NN:%d EN:%d\n", InputNumber, NN, EN);

                        CBUS_InCtrl[InputNumber].CBUSDeviceNumber = NN;
                        CBUS_InCtrl[InputNumber].CBUSEventNumber = EN;
                    }
                }
            }
        }
        fclose (ConfigFile);
        return 0;
    }
    return -1;      // Missing input configuration file
}  // ReadCBUSInputsConfig
// ------------------------------------------------------------

int ReadCBUSOutputsConfig (void)
{
    FILE* ConfigFile;
    char Buffer [256];
    char* Token;
    int OutputNumber, NN, EN;  // Node Number, Event Number

    ConfigFile = fopen ("cbus_outputs.dat", "rt");
    if (ConfigFile!=0)
    {
        if (VerbosityLevel > 0)
            fprintf (stdout, "Reading CBUS output configuration file...\n");

        while (fgets(Buffer, 256, ConfigFile))
        {
            // Avoid to create an entry if there is an error in the configuration file
            OutputNumber = -1;
            EN = -1;
            NN = -1;

            // Ignore line if starting by a # -> this is a comment
            if (Buffer[0]!='#')
            {
                // Get first part of the string (input number)
                Token = strtok (Buffer, TokenDelimiter);
                if (Token)
                    OutputNumber = atoi (Token);

                // Get second part of string (node number)
                Token = strtok (NULL, TokenDelimiter);
                if (Token)
                    NN = atoi (Token);

                // Get last part of the string (event number)
                Token= strtok (NULL, TokenDelimiter);
                if (Token)
                    EN = atoi (Token);

                //printf ("%d %d %d\n", OutputNumber, NN, EN);

                if ((OutputNumber<NUM_CBUS_BOOL_INPUTS)&&(OutputNumber>=0))
                {
                    if ((NN>0)&&(NN<65535)&&(EN>=0)&&(EN<65535))
                    {
                        if (VerbosityLevel > 0)
                            fprintf (stdout, "Output:%d NN:%d EN:%d\n", OutputNumber, NN, EN);

                        CBUS_OutCtrl[OutputNumber].CBUSDeviceNumber = NN;
                        CBUS_OutCtrl[OutputNumber].CBUSEventNumber = EN;
                    }
                }
            }
        }
        fclose (ConfigFile);
        return 0;
    }
    return -1;      // Missing output configuration file
}  // ReadCBUSOutputsConfig
// ------------------------------------------------------------

void setCBUS_ID (unsigned int id)
{
    CBUS_ID=id+(CBUS_MAJOR_PRIORITY<<9)+(CBUS_MINOR_PRIORITY<<7);
}  // setCBUS_ID
// ------------------------------------------------------------

//! Called by CBUS driver thread to process incoming CBUS messages and generate CBUS message from PLC outputs
void ProcessCBUS_IO (void)
{
    uint16_t NN;  // CBUS node number
    uint16_t EN;  // CBUS event number
    uint8_t ReceivedCANMsg[8];
    uint8_t SendCANMsg[8];
    unsigned int ReceivedCANSize;
    unsigned int ReceivedCANID;
    int OutputCounter;
    int InputCounter;
    uint8_t OutSnapshot;

	if (CANSocketReady == 0) return;		// cansocket connection is not opened : nothing can be done

	// Check if we have received anything in socketcan
	ReceivedCANSize = getNextCBUSMessage (&ReceivedCANID, &ReceivedCANMsg[0]);
	if (ReceivedCANSize != 0xFFFFFFFF)
	{  // if we have received a first message, start looping until we have received all messages from the socket
	    // Protect intermediate buffer in case PLC tries to update its inputs while we copy received CBUS events images
	    //pthread_mutex_lock (&IntermediateInputBufferLock);	// Lock only after we have checked if there are CBUS messages waiting to avoid useless mutex calls

	    do
	    {
		switch (ReceivedCANMsg[0])
		{
			case OPC_ACON : case OPC_ARON :  // CBUS event accessory ON either from response after request or "normal" event
				NN=(ReceivedCANMsg[1]<<8)+ReceivedCANMsg[2];
				EN=(ReceivedCANMsg[3]<<8)+ReceivedCANMsg[4];
				if (VerbosityLevel > 1)
                    fprintf (stdout, "Received ACON / ARON NN:%d - EN:%d\n", NN, EN);

				// Search in the input table if this event is associated with a PLC input
				// If event is found, set the PLC input
				for (InputCounter=0; InputCounter<NUM_CBUS_BOOL_INPUTS; InputCounter++)
				{
					if (CBUS_InCtrl[InputCounter].CBUSDeviceNumber!=0)  // Input is defined
					{
						if ((CBUS_InCtrl[InputCounter].CBUSDeviceNumber==NN)&&(CBUS_InCtrl[InputCounter].CBUSEventNumber==EN))
						{
							CBUS_InCtrl[InputCounter].CurrentInput = 1;
							CBUS_InCtrl[InputCounter].LastRefresh = 0;	// Reset timeout
						}
					}
				}
				break;
			case OPC_ACOF : case OPC_AROF :  // CBUS event accessory OFF either from response after request or "normal" event
				NN=(ReceivedCANMsg[1]<<8)+ReceivedCANMsg[2];
				EN=(ReceivedCANMsg[3]<<8)+ReceivedCANMsg[4];

				if (VerbosityLevel > 1)
                    fprintf (stdout, "Received ACOF / AROF NN:%d - EN:%d\n", NN, EN);

				// Search in the input table if this event is associated with a PLC input
				// If event is found, clear the PLC input
				//printf ("ACOF NN: %d - EN: %d\n", NN, EN);
				for (InputCounter=0; InputCounter<NUM_CBUS_BOOL_INPUTS; InputCounter++)
				{
					if (CBUS_InCtrl[InputCounter].CBUSDeviceNumber!=0)  // Input is defined
					{
						if ((CBUS_InCtrl[InputCounter].CBUSDeviceNumber==NN)&&(CBUS_InCtrl[InputCounter].CBUSEventNumber==EN))
						{
							CBUS_InCtrl[InputCounter].CurrentInput = 0;
							CBUS_InCtrl[InputCounter].LastRefresh = 0;	// Reset timeout
						}
					}
				}
				break;
		}

		// Get next CAN message from socket
		ReceivedCANSize = getNextCBUSMessage (&ReceivedCANID, &ReceivedCANMsg[0]);
	    } while (ReceivedCANSize != 0xFFFFFFFF);

	    // Free access to intermediate buffer (PLC can now read updated inputs)
	    //pthread_mutex_unlock (&IntermediateInputBufferLock);
	}

	// Scan all PLC outputs
	// if an output is associated with an event, check if output state has changed since last call to this function
	// or if output has not been refreshed since maximum refresh time
	// if output has changed or if timeout occurs, generate a OPC_ACOF or OPC_ACON depending on the output state
	// and clear refresh timer
	for (OutputCounter=0; OutputCounter<NUM_CBUS_BOOL_OUTPUTS; OutputCounter++)
	{
		if (CBUS_OutCtrl[OutputCounter].CBUSDeviceNumber!=0)  // PLC output is associated with an event
		{
			CBUS_OutCtrl[OutputCounter].LastRefresh++;
			OutSnapshot = CBUS_OutCtrl[OutputCounter].CurrentOutput;  // Make sure output will not be changed by PLC while we process it
			if ((CBUS_OutCtrl[OutputCounter].LastRefresh >= REFRESH_OUTPUT_TIMEOUT)||(CBUS_OutCtrl[OutputCounter].LastOutput!=OutSnapshot))
			{  // Output state has changed or timeout occured
                if (VerbosityLevel > 1)
                    fprintf (stdout, "Updating output %d\n", OutputCounter);

				if (OutSnapshot == 0)
				{
					SendCANMsg[0]=OPC_ACOF;
				}
				else
				{
					SendCANMsg[0]=OPC_ACON;
				}
				SendCANMsg[1] = CBUS_OutCtrl[OutputCounter].CBUSDeviceNumber>>8;
				SendCANMsg[2] = CBUS_OutCtrl[OutputCounter].CBUSDeviceNumber&0xFF;
				SendCANMsg[3] = CBUS_OutCtrl[OutputCounter].CBUSEventNumber>>8;
				SendCANMsg[4] = CBUS_OutCtrl[OutputCounter].CBUSEventNumber&0xFF;
				sendCBUSRaw (CBUS_ID, 5, &SendCANMsg[0]);

				CBUS_OutCtrl[OutputCounter].LastOutput = OutSnapshot;
				CBUS_OutCtrl[OutputCounter].LastRefresh = 0;
			}
		}
	}

	// Check timeout on inputs. If we have not received an event for an input for a "long" timeout
	// send a CBUS status request for the event. This allows the CBUS PLC to get a correct image of
	// all inputs even if it connects to CBUS after events have been already exchanged
	for (InputCounter=0; InputCounter<NUM_CBUS_BOOL_INPUTS; InputCounter++)
	{
		if (CBUS_InCtrl[InputCounter].CBUSDeviceNumber!=0)  // PLC input is associated with an event
		{
			CBUS_InCtrl[InputCounter].LastRefresh++;
			if (CBUS_InCtrl[InputCounter].LastRefresh >= REFRESH_OUTPUT_TIMEOUT)
			{
				//printf ("Ask refresh of input %d\n", InputCounter);

				SendCANMsg[0]=OPC_AREQ;
				SendCANMsg[1] = CBUS_InCtrl[InputCounter].CBUSDeviceNumber>>8;
				SendCANMsg[2] = CBUS_InCtrl[InputCounter].CBUSDeviceNumber&0xFF;
				SendCANMsg[3] = CBUS_InCtrl[InputCounter].CBUSEventNumber>>8;
				SendCANMsg[4] = CBUS_InCtrl[InputCounter].CBUSEventNumber&0xFF;
				sendCBUSRaw (CBUS_ID, 5, &SendCANMsg[0]);

				CBUS_InCtrl[InputCounter].LastRefresh = 0;
			}
		}
	}
}  // ProcessCBUS_IO
/* ------------------------------------------------- */

int startCBUSDriver (char* InterfaceName)
{
    int SockErr;
    int InputCounter;
    int OutputCounter;
    uint8_t SendCANMsg[8];
    int RetVal;

    // Read I/O configuration file to associate events with PLC I/Os
	RetVal = ReadCBUSInputsConfig();
	if (RetVal != 0)
        return RetVal;       // No input configuration file or corrupted file
	RetVal = ReadCBUSOutputsConfig();
	if (RetVal != 0)
        return RetVal;       // No output configuration file or corrupted file

	SockErr=createCBUSSocket(InterfaceName);
	if (SockErr!=0)
	{
        return 0x10000000+SockErr;
	}

	CANSocketReady=1;

	// Preload refresh timer for all outputs
	for (OutputCounter=0; OutputCounter<NUM_CBUS_BOOL_OUTPUTS; OutputCounter++)
	{
		CBUS_OutCtrl[OutputCounter].LastRefresh = OutputCounter*500;
	}

	// Send AREQ for all inputs to get the latest images
	// DO NOT SEND updates for outputs when PLC starts, as we want outputs to keep their state
	for (InputCounter=0; InputCounter<NUM_CBUS_BOOL_INPUTS; InputCounter++)
	{
		if (CBUS_InCtrl[InputCounter].CBUSDeviceNumber!=0)
		{
			CBUS_InCtrl[InputCounter].LastRefresh = InputCounter*500;	// Preload the timer to spread inputs refresh requests

			SendCANMsg[0]=OPC_AREQ;
			SendCANMsg[1] = CBUS_InCtrl[InputCounter].CBUSDeviceNumber>>8;
			SendCANMsg[2] = CBUS_InCtrl[InputCounter].CBUSDeviceNumber&0xFF;
			SendCANMsg[3] = CBUS_InCtrl[InputCounter].CBUSEventNumber>>8;
			SendCANMsg[4] = CBUS_InCtrl[InputCounter].CBUSEventNumber&0xFF;
			sendCBUSRaw (CBUS_ID, 5, &SendCANMsg[0]);
		}
		usleep (10000);   // 10 ms between each request
	}

	return 0;
}  // startCBUSDriver
/* ------------------------------------------------- */

void closeCBUSDriver (void)
{
    CANSocketReady=0;
    closeCBUSSocket();
}  // closeCBUSDriver
/* ------------------------------------------------- */

void acquireCBUSPLCInputs (void)
{
  int InputCounter;

  // Copy CBUS boolean data to PLC input
  for (InputCounter=0; InputCounter<NUM_CBUS_BOOL_INPUTS; InputCounter++)
  {
    CBUS_PLC_BoolInput[InputCounter] = CBUS_InCtrl[InputCounter].CurrentInput;
  }
}  // acquireCBUSPLCInputs
/* ------------------------------------------------- */

void updateCBUSPLCOutputs (void)
{
  int OutputCounter;

  // Copy CBUS boolean data to PLC input
  for (OutputCounter=0; OutputCounter<NUM_CBUS_BOOL_OUTPUTS; OutputCounter++)
  {
    CBUS_OutCtrl[OutputCounter].CurrentOutput = CBUS_PLC_BoolOutput[OutputCounter];
  }
}  // updateCBUSPLCOutputs
/* ------------------------------------------------- */

