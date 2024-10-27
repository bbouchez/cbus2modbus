/* CBUS_OPC.h
CBUS Operation Codes include file
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

#ifndef __CBUS_OPC_H__
#define __CBUS_OPC_H__

#define OPC_ACK 0x00     //  General affirmative acknowledge
#define OPC_NAK 0x01     //  General negative acknowledge
#define OPC_HLT 0x02     //  CAN bus not available / busy
#define OPC_BON 0x03     //  CAN bus available
#define OPC_TOF 0x04     //  DCC track is off
#define OPC_TON 0x05     //  DCC track is on
#define OPC_ESTOP 0x06   //  Emergency stop all
#define OPC_ARST 0x07    //  System reset
#define OPC_RTOF 0x08    //  Request track off
#define OPC_RTON 0x09    //  Request track on
#define OPC_RESTP 0x0A   //  Request emergency stop all
#define OPC_RSTAT 0x0C   //  Query status of command station
#define OPC_QNN 0x0D     //  Query node status
#define OPC_RQNP 0x10    //  Request node parameters
#define OPC_RQMN 0x11    //  Request module name
#define OPC_KLOC 0x21    //  Release engine
#define OPC_QLOC 0x22    //  Query engine
#define OPC_DKEEP 0x23   //  Session keepalive from CAB
#define OPC_DBG1 0x30    //  Debug. For development only
#define OPC_EXTC 0x3F    //  Extended OPC with no added bytes
#define OPC_RLOC 0x40    //  Request engine session
#define OPC_QCON 0x41    //  Query consist
#define OPC_SNN 0x42     //  Set node number
#define OPC_ALOC 0x43    //  Allocate loc to assignment or activity
#define OPC_STMOD 0x44   //  Set cab session mode
#define OPC_PCON 0x45    //  Set loco into consist
#define OPC_KCON 0x46    //  Remove loco from consist
#define OPC_DSPD 0x47    //  Set engine speed / direction
#define OPC_DFLG 0x48    //  Set engine session flags
#define OPC_DFNON 0x49   //  Set engine function ON
#define OPC_DFNOF 0x4A   //  Set engine function OFF
#define OPC_SSTAT 0x4C   //  Service mode status
#define OPC_RQNN 0x50    //  Request node number
#define OPC_NNREL 0x51   //  Node number release
#define OPC_NNACK 0x52   //  Node number acknowledge
#define OPC_NNLRN 0x53   //  Set node into learn mode
#define OPC_NNULN 0x54   //  Release node from learn mode
#define OPC_NNCLR 0x55   //  Clear all events from a node
#define OPC_NNEVN 0x56   //  Read number of events available
#define OPC_NERD 0x57    //  Read back all events in a node
#define OPC_RQEVN 0x58   //  Read number of stored events in a node
#define OPC_WRACK 0x59   //  Write acknowledge
#define OPC_RQDAT 0x5A   //  Request node data event
#define OPC_RQDDS 0x5B   //  Request device data (short)
#define OPC_BOOTM 0x5C   //  Put node into bootloader mode
#define OPC_ENUM 0x5D    //  Force self enumeration of CAN_ID
#define OPC_EXTC1 0x5F   //  Extended OPC with one added byte
#define OPC_DFUN 0x60    //  Set engine functions (DCC format)
#define OPC_GLOC 0x61    //  Get engine session
#define OPC_ERR 0x63     //  Command station error report
#define OPC_CMDERR 0x6F  //  Error message during configuration
#define OPC_EVNLF 0x70   //  Event space left
#define OPC_NVRD 0x71    //  Request read of node variable
#define OPC_NENRD 0x72   //  Request read of events by index
#define OPC_RQNPN 0x73   //  Request read of node parameter by index
#define OPC_NUMEV 0x74   //  Number of events stored in node
#define OPC_CANID 0x75   //  For a specific CAN_ID
#define OPC_EXTC2 0x7F   //  Extended OPC with two added bytes
#define OPC_RDCC3 0x80   //  Request 3 byt DCC packet
#define OPC_WCVO 0x82    //  Write CV in OPS mode (byte)
#define OPC_WCVB 0x83    //  Write CV in OPS mode (bit)
#define OPC_QCVS 0x84    //  Request read CV (service mode)
#define OPC_PCVS 0x85    //  Report CV (service mode)
#define OPC_ACON 0x90    //  Accessory ON event (long)
#define OPC_ACOF 0x91    //  Accessory OFF event (long)
#define OPC_AREQ 0x92    //  Accessory status request (long)
#define OPC_ARON 0x93    //  Accessory response ON (long)
#define OPC_AROF 0x94    //  Accessory response OFF (long)
#define OPC_EVULN 0x95   //  Unlearn an event in learn mode
#define OPC_NVSET 0x96   //  Set a node variable
#define OPC_NVANS 0x97   //  Node variable value response
#define OPC_ASON 0x98    //  Accessory ON event (short)
#define OPC_ASOF 0x99    //  Accessory OFF event (short)
#define OPC_ASRQ 0x9A    //  Accessory status request (short)
#define OPC_PARAN 0x9B   //  Parameter readback by index
#define OPC_REVAL 0x9C   //  Request read of event variable
#define OPC_ARSON 0x9D   //  Accessory response ON (short)
#define OPC_ARSOF 0x9E   //  Accessory response OFF (short)
#define OPC_EXTC3 0x9F   //  Extended OPC with three added bytes
#define OPC_RDCC1 0xA0   //  Request 4 byte DCC packet
#define OPC_WCVS 0xA2    //  Write CV in service mode
#define OPC_ACON1 0xB0   //  Accessory ON event with one added byte (long)
#define OPC_ACOF1 0xB1   //  Accessory OFF event with one added byte (long)
#define OPC_REQEV 0xB2   //  Read event variable in learn mode
#define OPC_ARON1 0xB3   //  Accessory response event ON with one added byte (long)
#define OPC_AROF1 0xB4   //  Accessory response event OFF with one added byte (long)
#define OPC_NEVAL 0xB5   //  Read of EV value response
#define OPC_PNN 0xB6     //  Response to query node
#define OPC_ASON1 0xB8   //  Accessory ON event with one added byte (short)
#define OPC_ASOF1 0xB9   //  Accessory OFF event with one added byte (short)
#define OPC_ARSON1 0xBD  //  Accessory response ON with one added byte (short)
#define OPC_ARSOF1 0xBE  //  Accessory response OFF with one added byte (short)
#define OPC_EXTC4 0xBF   //  Extended OPC with four added bytes
#define OPC_RDCC5 0xC0   //  Request 5 byte DCC packet
#define OPC_WCVOA 0xC1   //  Write CV in OPS mode by address
#define OPC_FCLK 0xCF    //  Fast clock
#define OPC_ACON2 0xD0   //  Accessory ON event with two added byte (long)
#define OPC_ACOF2 0xD1   //  Accessory OFF event with two added byte (long)
#define OPC_EVLRN 0xD2   //  Teach event in learn mode
#define OPC_EVANS 0xD3   //  Response to request for EV value in learn mode
#define OPC_ARON2 0xD4   //  Accessory response event ON with two added byte (long)
#define OPC_AROF2 0xD5   //  Accessory response event OFF with two added byte (long)
#define OPC_ASON2 0xD8   //  Accessory ON event with two added byte (short)
#define OPC_ASOF2 0xD9   //  Accessory OFF event with two added byte (short)
#define OPC_ARSON2 0xDD  //  Accessory response ON with two added byte (short)
#define OPC_ARSOF2 0xDE  //  Accessory response OFF with two added byte (short)
#define OPC_EXTC5 0xDF   //  Extended OPC with five added bytes
#define OPC_RDCC6 0xE0   //  Request 6 byte DCC packet
#define OPC_PLOC 0xE1    //  Engine report from command station
#define OPC_NAME 0xE2    //  Response to request for node name
#define OPC_STAT 0xE3    //  Command station status report
#define OPC_PARAMS 0xEF  //  Response to request for node parameters (in setup)
#define OPC_ACON3 0xF0   //  Accessory ON event with three added byte (long)
#define OPC_ACOF3 0xF1   //  Accessory OFF event with three added byte (long)
#define OPC_ENRSP 0xF2   //  Response to request to read node events
#define OPC_ARON3 0xF3   //  Accessory response event ON with three added byte (long)
#define OPC_AROF3 0xF4   //  Accessory response event OFF with three added byte (long)
#define OPC_EVLRNI 0xF5  //  Teach event in learn mode using event indexing
#define OPC_ACDAT 0xF6   //  Accessory node data event 5 data bytes (long)
#define OPC_ARDAT 0xF7   //  Accessory node data response 5 data bytes (long)
#define OPC_ASON3 0xF8   //  Accessory ON event with three added byte (short)
#define OPC_ASOF3 0xF9   //  Accessory OFF event with three added byte (short)
#define OPC_DDES 0xFA    //  Accessory node data event 5 data bytes (short)
#define OPC_DDRS 0xFB    //  Accessory node data response 5 data bytes (short)
#define OPC_ARSON3 0xFD  //  Accessory response ON with three added byte (short)
#define OPC_ARSOF3 0xFE  //  Accessory response OFF with three added byte (short)
#define OPC_EXTC6 0xBF   //  Extended OPC with six added bytes

#endif // __CBUS_OPC_H__
