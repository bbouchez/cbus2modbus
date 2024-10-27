# cbus2modbus
Raspberry Pi Modbus gateway for MERG CBUS 

cbus2modbus is a CBUS gateway created specifically for ClassicLadder2 project. It translates CBUS messages into Modbus variables which can be accessed via Modbus/TCP.

The current version requires the Raspberry Pi to be equipped with a CAN interface declared as *can0* (MERG CANPiCAP Kit 86 or any other equivalent hardware interface).
Please refer to the interface documentation in order to declare it at hardware level.

In order to activate *can0* interface, the following command must be entered in the terminal window :

sudo ip link set can0 up type can bitrate 125000 restart-ms 100

**Usage**
cbus2modbus acts a Modbus/TCP server providing 128 boolean inputs and 128 boolean outputs ("coils") to the PLC. Each input and output is associated with a CBUS event and node number.

cbus2modbus requires two configuration files called cbus_inputs.dat and cbus_outputs.dat in the same directory than the executable.

cbus_inputs.dat associates incoming ACON/ACOF events with boolean inputs going to the PLC. Each line in the file is made of 3 parts, separated by a whitespace :
- PLC input number (0 to 127 for %I0 to %I127)
- CBUS node number (number of the node sending the event, 0 to 65535)
- CBUS event number for the node (0 to 65335)

For example, the following line 
0 300 9

will set PLC input to 1 when Node 300 sends ACON event 9. PLC input will be reset when Node 300 sends ACOF event 9.

cbus_outputs.dat does the same job as cbus_inputs.dat, but for the PLC outputs ("coils"). When the PLC sets an output, cbus2modbus will send a ACON event with the provided Node Number and Event Number. When the output is reset, cbus2modbus will send a ACOF event.

For example, the following line 
0 300 1

will send ACON event 1 to Node 300 when %Q0 is set in the PLC, and will send ACOF event 1 to Node 300 when %Q0 is reset in the PLC.

You can put comments in the cbus_inputs.dat and cbus_outputs files if needed, by starting the line with #.

**Command line parameters**
By default, cbus2modbus does not require any arguments when launched.

It is possible to use --verbose argument for testing purpose.
--verbose 1 will show minimal information at startup (display of configuration files being read)
--verbose 2 will display dynamic information about received and transmitted CBUS events

**How to compile**
cbus2modbus has been written using Code::Blocks IDE. If you want to recompile the application, you will need to open the project file (cbus2modbus.cbp) and launch compiler withing the IDE. In the future, I plan to provide a makefile too.

Note that cbus2modbus uses the BEB SDK to be compiled : https://github.com/bbouchez/BEBSDK

You will need to clone it on your machine before you can compile the application within Code::Blocks.
IMPORTANT : you will probably need to relocate the SDK files in cbus2modbus project tree as my machine uses the following path to the SDK : /home/benoit/SDK. Any other path to the SDK will lead to missing files error when Code::Blocks will try to compile the application.

cbus2modbus also uses libmodbus-dev. If needed, execute the following command before compiling the application :

sudo apt install libmodbus-dev