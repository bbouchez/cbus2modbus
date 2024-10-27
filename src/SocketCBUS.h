/* SocketCBUS.h
CBUS Library for CAN Socket
Development : Benoit BOUCHEZ (BEB)

Copyright 2021 Benoit BOUCHEZ (M8718)
Creative Commons Attribution-NonCommercial-ShareaLIKE 4.0 International License
 License summary:
   You are free to:
     Share, copy and redistribute the material in any medium or format
     Adapt, remix, transform, and build upon the material
   The licensor cannot revoke these freedoms as long as you follow the license terms.
   Attribution : You must give appropriate credit, provide a link to the license,
                  and indicate if changes were made. You may do so in any reasonable manner,
                  but not in any way that suggests the licensor endorses you or your use.
   NonCommercial : You may not use the material for commercial purposes. **(see note below)
   ShareAlike : If you remix, transform, or build upon the material, you must distribute
                 your contributions under the same license as the original.
   No additional restrictions : You may not apply legal terms or technological measures that
                                 legally restrict others from doing anything the license permits.
  ** For commercial use, please contact the original copyright holder(s) to agree licensing terms

This software is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE

*/

#ifndef __SOCKETCBUS_H__
#define __SOCKETCBUS_H__

// CBUS Error codes
#define CBUS_ERR_SOCKET_ERROR		-1		// Can not create the socket
#define CBUS_ERR_BIND_ERROR			-2		// Can not bind the socket to requested interface

//! \return 0 if socket has been created correctly, negative values are errors (see CBUS_ERROR_CODES)
int createCBUSSocket (char* ifname);

//! Release all resources allocated to CBUS socket
void closeCBUSSocket (void);

//! Get next CBUS message in system reception queue
// Function is non blocking and returns -1 if no CAN message has been received (as DLC can be 0)
unsigned int getNextCBUSMessage (unsigned int* CANID, unsigned char* CANData);

//! Send a message on the CAN bus
void sendCBUSRaw (unsigned int ID, unsigned char DLC, unsigned char* Data);


#endif
