/* SocketCBUS.c
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

#include "SocketCBUS.h"
#include <linux/can.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

static int CANSocket = -1;

int createCBUSSocket (char* ifname)
{
    struct sockaddr_can addr;
    struct ifreq ifr;

    // Just in case...
    closeCBUSSocket ();

    // Try to create the CAN socket
    CANSocket=socket (PF_CAN, SOCK_RAW, CAN_RAW);
    if (CANSocket == -1)
    {
        return CBUS_ERR_SOCKET_ERROR;
    }

    // Find interface index based on the required name
    strcpy (ifr.ifr_name, ifname);
    ioctl (CANSocket, SIOCGIFINDEX, &ifr);

    // Bind socket to CAN interface
    memset (&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;       // 0 = all CAN interfaces
    if (bind(CANSocket, (struct sockaddr*)&addr, sizeof(addr))<0)
    {
        return CBUS_ERR_BIND_ERROR;
    }

    // Make the socket non blocking
    int Flags = fcntl (CANSocket, F_GETFL, 0);
    fcntl (CANSocket, F_SETFL, Flags | O_NONBLOCK);

    return 0;
}  // createCBUSSocket
// ------------------------------------------------------------

void closeCBUSSocket (void)
{
    if (CANSocket != -1)
    {
	close (CANSocket);
	CANSocket = -1;
    }
}  // closeCBUSSocket
// ------------------------------------------------------------

unsigned int getNextCBUSMessage (unsigned int* CANID, unsigned char* CANData)
{
    int nbytes;
    struct can_frame frame;
    int len;

    nbytes = read (CANSocket, &frame, sizeof(struct can_frame));
    if (nbytes == -1) return 0xFFFFFFFF;

    len = frame.can_dlc & 0xF;
    *CANID = frame.can_id;
    memcpy (CANData, &frame.data[0], len);

    return frame.can_dlc;
}  // getNextCBUSMessage
// ------------------------------------------------------------

void sendCBUSRaw (unsigned int ID, unsigned char DLC, unsigned char* Data)
{
	struct can_frame frame;

	frame.can_id = ID;
	frame.can_dlc = DLC;

    if (DLC>0)
    {
    	memcpy (&frame.data[0], Data, DLC);
    }

	write (CANSocket, &frame, sizeof (struct can_frame));
}  // sendCBUSRaw
// ------------------------------------------------------------

