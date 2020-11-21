/*
	A simple CAN bridge with j1939 decoding for the Arduino
	Due or the Teensy 4.0 (requires two CAN ports, so won't
	work on the Teensy 3.6)

	Requires the following libraries installed into Arduino:

	https://github.com/collin80/can_common
	https://github.com/collin80/due_can

	Requires the following library for Teensy 4.0:

	https://github.com/tonton81/FlexCAN_T4
*/

#ifdef ARDUINO_TEENSY40
#define TEENSY 1
#endif

#ifdef TEENSY
#  include <FlexCAN_T4.h>
#else //Due
#  include "variant.h" 
#  include <due_can.h>
//if needing higher speed output use the native USB port
#define Serial SerialUSB
#endif

#include "canframe.h"

#ifdef TEENSY
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can1;
#endif

void got_frame(CANFrame &frame, uint8_t which_interface) 
{
	if (which_interface == 1) {
		//send to other interface (bridge)
#ifdef TEENSY
		Can0.write(frame);
#else
		Can0.sendFrame(frame);
#endif

	} else {
		//send to other interface (bridge)
#ifdef TEENSY
		Can1.write(frame);
#else
		Can1.sendFrame(frame);
#endif
	}
}

#ifdef TEENSY
void can0_got_frame_teensy(const CAN_message_t &orig_frame) {
	//copy frame in case we want to modify it.
	CANFrame frame = orig_frame;
	
	//process frame
	got_frame(frame, 0);
}

void can1_got_frame_teensy(const CAN_message_t &orig_frame) {
	//copy frame in case we want to modify it.
	CANFrame frame = orig_frame;
	
	//process frame
	got_frame(frame, 1);
}

#else
void can0_got_frame_due(CAN_FRAME *orig_frame) {
	//copy frame in case we want to modify it.
	CANFrame frame = static_cast<CANFrame>(*orig_frame);

	//process frame
	got_frame(frame, 0);
}

void can1_got_frame_due(CAN_FRAME *orig_frame) {
	//copy frame in case we want to modify it.
	CANFrame frame = static_cast<CANFrame>(*orig_frame);

	//process frame
	got_frame(frame, 1);
}
#endif

void setup()
{
	delay(5000);
	Serial.begin(115200);
	Serial.println("j1939 CAN bridge.");

#ifdef TEENSY
	//Teensy FlexCAN_T4 setup
	Can0.begin();
	Can0.setBaudRate(250000);
	Can0.setMaxMB(16);
	//Can0.enableFIFO();
	//Can0.enableFIFOInterrupt();
	Can0.onReceive(can0_got_frame_teensy);
	//Can0.enableMBInterrupts(FIFO);
	Can0.enableMBInterrupts();

	Can1.begin();
	Can1.setBaudRate(250000);
	Can1.setMaxMB(16);
	//Can1.enableFIFO();
	//Can1.enableFIFOInterrupt();
	Can1.onReceive(can1_got_frame_teensy);
	//Can1.enableMBInterrupts(FIFO);
	Can1.enableMBInterrupts();
#else
	Can0.begin(CAN_BPS_250K);
	Can1.begin(CAN_BPS_250K);

	for (int filter=0;filter <3; filter ++) {
		Can0.setRXFilter(0,0,true);
		Can1.setRXFilter(0,0,true);
	}

	Can0.attachCANInterrupt(can0_got_frame_due);
	Can1.attachCANInterrupt(can1_got_frame_due);
#endif
}

void loop() 
{
#ifdef TEENSY
	//process collected frames
	Can0.events();
	Can1.events();
#endif
}

