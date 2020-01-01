j1939sniff
==========

This is a simple example of using the  due_can  library and two
CAN transceivers--one on the Due's CAN0 pins and the other on the
CAN1 pins--in a bridge configuration. All packets that are seen
by the one CAN interface are echoed to the other in both directions.
Code decodes j1939 src address and PGN fields, and one could modify
the code to filter out or modify specific messages as they traverse
the bridge.

This code only runs on the Arduino Due.

Requires CAN transceiver chips attached to the CAN0 TX and RX and CAN1 
TX/RX pins of the Due.

Capturing lots of data to the Serial port for analysis will require
the use of the native USB port, which can go much faster than the
normal UART on the programming USB port.

