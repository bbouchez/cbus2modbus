/*
cbus_io.h
cbus2modbus
CBUS communication processing to update local Modbus images
Development : Benoit BOUCHEZ - M8718
*/

#ifndef __CBUS_IO_H__
#define __CBUS_IO_H__

#include <stdint.h>

#define NUM_CBUS_BOOL_INPUTS	128
#define NUM_CBUS_BOOL_OUTPUTS	128

#ifdef __cplusplus
extern "C" {
#endif

extern uint8_t CBUS_PLC_BoolInput[NUM_CBUS_BOOL_INPUTS];
extern uint8_t CBUS_PLC_BoolOutput[NUM_CBUS_BOOL_OUTPUTS];

extern unsigned int VerbosityLevel;

//! Starts CBUS communication driver
int startCBUSDriver (char* InterfaceName);

//! Terminates CBUS communication
void closeCBUSDriver (void);

//! Transform incoming CBUS messages into PLC inputs
void acquireCBUSPLCInputs (void);
//! Transform PLC outputs into CBUS messages
void updateCBUSPLCOutputs (void);

void ProcessCBUS_IO (void);

#ifdef __cplusplus
}
#endif

#endif
