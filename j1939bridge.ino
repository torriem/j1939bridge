/*
	A simple CAN bridge with j1939 decoding for the Teensy 4.0.
	It requires two CAN ports, so won't work on the Teensy 3.6.

	Requires the following library for Teensy 4.0:

	https://github.com/tonton81/FlexCAN_T4
*/

#include <FlexCAN_T4.h>

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can1;

void can0_got_frame_teensy(const CAN_message_t &frame) {
	Serial.println("0->1");
	Can1.write(frame);
}

void can1_got_frame_teensy(const CAN_message_t &frame) {
	Serial.println("1->0");
	Can0.write(frame);
}


void setup()
{
	delay(5000);
	Serial.begin(115200);
	Serial.println("j1939 CAN bridge.");

	//Teensy FlexCAN_T4 setup
	Can0.begin();
	Can0.setBaudRate(250000);
	Can0.setMaxMB(16);
	Can0.enableFIFO();
	Can0.enableFIFOInterrupt();
	Can0.onReceive(can0_got_frame_teensy);
	//Can0.enableMBInterrupts(FIFO);
	//Can0.enableMBInterrupts();
	Can0.mailboxStatus();

	Can1.begin();
	Can1.setBaudRate(250000);
	Can1.setMaxMB(16);
	Can1.enableFIFO();
	Can1.enableFIFOInterrupt();
	Can1.onReceive(can1_got_frame_teensy);
	//Can1.enableMBInterrupts(FIFO);
	//Can1.enableMBInterrupts();
	Can1.mailboxStatus();
}

void loop() 
{
	//process collected frames
	Can0.events();
	Can1.events();

	/*
	if ( Can0.read(msg) ) {
		Can1.write(msg);
	}
	else if ( Can1.read(msg) ) {
		Can0.write(msg);
	}
	*/

}

