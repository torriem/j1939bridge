/*
	A simple CAN bridge with j1939 decoding.

	Requires the following libraries installed into Arduino:

	https://github.com/collin80/can_common
	https://github.com/collin80/due_can
 */

#include "variant.h" 
#include <due_can.h>

//if needing higher speed output use the native USB port
//#define Serial SerialUSB

static inline void print_hex(uint8_t *data, int len) {
	char temp[4];
	for (int b=0;b < len; b++) {
		sprintf(temp, "%.2x",data[b]);
		Serial.print(temp);
	}
	Serial.println("");
}

bool j1939PeerToPeer(long lPGN)
{
	// Check the PGN 
	if(lPGN > 0 && lPGN <= 0xEFFF)
		return true;

	if(lPGN > 0x10000 && lPGN <= 0x1EFFF)
		return true;

	return false;

}

void j1939Decode(long lID, unsigned long* lPGN, byte* nPriority, byte* nSrcAddr, byte *nDestAddr)
{
	/* decode j1939 fields from 29-bit CAN id */
	byte nRetCode = 1;

	*nSrcAddr = 255;
	*nDestAddr = 255;

	long lPriority = lID & 0x1C000000;
	*nPriority = (int)(lPriority >> 26);

	*lPGN = lID & 0x00FFFF00;
	*lPGN = *lPGN >> 8;

	lID = lID & 0x000000FF;
	*nSrcAddr = (int)lID;

	if(j1939PeerToPeer(*lPGN) == true)
	{
		*nDestAddr = (int)(*lPGN & 0xFF);
		*lPGN = *lPGN & 0x01FF00;
	}
}

void got_frame(CAN_FRAME *frame, int which) {
	unsigned long PGN;
	byte priority;
	byte srcaddr;
	byte destaddr;

	j1939Decode(frame->id, &PGN, &priority, &srcaddr, &destaddr);

	//could filter out what we want to look at here on any of these
	//variables.
	Serial.print(PGN);
	Serial.print(",");
	Serial.print(priority);
	Serial.print(",");
	Serial.print(srcaddr);
	Serial.print(",");
	Serial.print(destaddr);
	Serial.print(",");
	print_hex(frame->data.bytes, frame->length);

	//example of a decode
	if (PGN == 65267) { //vehicle position message
		//latitude
		Serial.print(frame->data.uint32[0] / 10000000.0 - 210.0);
		Serial.print(",");
		//longitude
		Serial.println(frame->data.uint32[1] / 10000000.0 - 210.0);
	}

	if (which == 0) {
		//transmit it out Can1
		Can1.sendFrame(*frame);
	} else {
		//transmit it out Can0
		Can0.sendFrame(*frame);
	}
}

void can0_got_frame(CAN_FRAME *frame) {
	got_frame(frame,0);
}

void can1_got_frame(CAN_FRAME *frame) {
	got_frame(frame,1);
}

void setup()
{
	Serial.begin(115200);
	Can0.begin(CAN_BPS_250K);
	Can1.begin(CAN_BPS_250K);

	for (int filter=0;filter <3; filter ++) {
		Can0.setRXFilter(0,0,true);
		Can1.setRXFilter(0,0,true);
	}

	Can0.attachCANInterrupt(can0_got_frame);
	Can1.attachCANInterrupt(can1_got_frame);
}

void loop() 
{
}

