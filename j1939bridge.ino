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
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_256> Can0;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_256> Can1;
#endif

static inline void print_hex(uint8_t *data, int len) {
	char temp[10];
	for (int b=0;b < len; b++) {
		sprintf(temp, "%.2x",data[b]);
		Serial.print(temp);
	}
	Serial.println("");
}

bool j1939PeerToPeer(long PGN)
{
	// Check the PGN 
	if(PGN > 0 && PGN <= 0xEFFF)
		return true;

	if(PGN > 0x10000 && PGN <= 0x1EFFF)
		return true;

	return false;

}

void j1939Decode(long ID, unsigned long* PGN, byte* priority, byte* src_addr, byte *dest_addr)
{
	/* decode j1939 fields from 29-bit CAN id */
	*src_addr = 255;
	*dest_addr = 255;

	long _priority = ID & 0x1C000000;
	*priority = (int)(_priority >> 26);

	*PGN = ID & 0x00FFFF00;
	*PGN = *PGN >> 8;

	ID = ID & 0x000000FF;
	*src_addr = (int)ID;

	if(j1939PeerToPeer(*PGN) == true)
	{
		*dest_addr = (int)(*PGN & 0xFF);
		*PGN = *PGN & 0x01FF00;
	}
}

void got_frame(CANFrame &frame, uint8_t which_interface) 
{
	unsigned long PGN;
	byte priority;
	byte srcaddr;
	byte destaddr;

	j1939Decode(frame.get_id(), &PGN, &priority, &srcaddr, &destaddr);

	//could filter out what we want to look at here on any of these
	//variables.
  
	if(which_interface)
		Serial.print("1->0,");
	else
		Serial.print("0->1,");


	Serial.print(PGN);
	Serial.print(",");
	Serial.print(priority);
	Serial.print(",");
	Serial.print(srcaddr);
	Serial.print(",");
	Serial.print(destaddr);
	Serial.print(",");
  Serial.print(frame.get_length());
  Serial.print(",");

  //for(int i=0; i < frame.get_length(); i++)
  //{
  // Serial.print(frame.get_data()->bytes[i],HEX);
  //}

  //Serial.println();
  
	print_hex(frame.get_data()->bytes, frame.get_length());
 

	//example of a decode
	if (PGN == 65267) { //vehicle position message
		//latitude
    uint32_t latitude = frame.get_data()->uint32[0];
    uint32_t longitude = frame.get_data()->uint32[1];
    
		Serial.print((double)(latitude / 10000000.0 - 210.0), 7);
		Serial.print(",");
		//longitude
		Serial.println((double)(longitude / 10000000.0 - 210.0),7);
	}
	//if (PGN == 1792) {
	//	frame.get_data()->bytes[1] = 0x99;
	//}
 
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
  //Serial.println("on 0");	
	//process frame
	got_frame(frame, 0);
  //Can1.write(orig_frame);
}

void can1_got_frame_teensy(const CAN_message_t &orig_frame) {
	//copy frame in case we want to modify it.
	CANFrame frame = orig_frame;
  //Serial.println("on 1");
	
	//process frame
	got_frame(frame, 1);
  //Can0.write(orig_frame);
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
	Serial.println("j1939 CAN bridge sniffer.");

#ifdef TEENSY
	//Teensy FlexCAN_T4 setup
	Can0.begin();
	Can0.setBaudRate(250000);
	//Can0.setMaxMB(1);
	Can0.enableFIFO();
	Can0.enableFIFOInterrupt();
	Can0.onReceive(can0_got_frame_teensy);
	Can0.enableMBInterrupts(FIFO);
	Can0.enableMBInterrupts();

	Can1.begin();
	Can1.setBaudRate(250000);
	//Can1.setMaxMB(1);
	Can1.enableFIFO();
	Can1.enableFIFOInterrupt();
	Can1.onReceive(can1_got_frame_teensy);
	Can1.enableMBInterrupts(FIFO);
	Can1.enableMBInterrupts();

  Can0.mailboxStatus();
  Can1.mailboxStatus();
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
