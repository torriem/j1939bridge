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
//#define Serial SerialUSB
#endif

#ifdef TEENSY
//Union for parsing CAN bus data messages. Warning:
//Invokes type punning, but this works here because
//CAN bus data is always little-endian (or should be)
//and the ARM processor on these boards is also little
//endian.
typedef union {
    uint64_t uint64;
    uint32_t uint32[2]; 
    uint16_t uint16[4];
    uint8_t  uint8[8];
    int64_t int64;
    int32_t int32[2]; 
    int16_t int16[4];
    int8_t  int8[8];

    //deprecated names used by older code
    uint64_t value;
    struct {
        uint32_t low;
        uint32_t high;
    };
    struct {
        uint16_t s0;
        uint16_t s1;
        uint16_t s2;
        uint16_t s3;
    };
    uint8_t bytes[8];
    uint8_t byte[8]; //alternate name so you can omit the s if you feel it makes more sense
} BytesUnion;

FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> Can0;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can1;
#endif

static inline void print_hex(uint8_t *data, int len) {
	char temp[4];
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

void got_frame(uint32_t id, uint8_t extended, uint8_t length, 
               BytesUnion *data, uint8_t which_interface) {
	unsigned long PGN;
	byte priority;
	byte srcaddr;
	byte destaddr;

	j1939Decode(id, &PGN, &priority, &srcaddr, &destaddr);

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
	print_hex(data->bytes, length);

	//example of a decode
	if (PGN == 65267) { //vehicle position message
		//latitude
		Serial.print(data->uint32[0] / 10000000.0 - 210.0);
		Serial.print(",");
		//longitude
		Serial.println(data->uint32[1] / 10000000.0 - 210.0);
	}
}

#ifdef TEENSY
void can0_got_frame_teensy(const CAN_message_t &orig_frame) {
	//copy frame in case we want to modify it.
	CAN_message_t frame = orig_frame;
	
	//process frame
	got_frame(frame.id, frame.flags.extended, frame.len,
	          (BytesUnion *)frame.buf, 0);

	//send to other interface (bridge)
	Can1.write(frame);

}

void can1_got_frame_teensy(const CAN_message_t &orig_frame) {
	//copy frame in case we want to modify it.
	CAN_message_t frame = orig_frame;
	
	//process frame
	got_frame(frame.id, frame.flags.extended, frame.len,
	          (BytesUnion *)frame.buf, 1);

	//send to other interface (bridge)
	Can0.write(frame);

}

#else
void can0_got_frame_due(CAN_FRAME *frame) {
	//process frame
	got_frame(frame->id, frame->extended, frame->length, &(frame->data), 0);
	
	//send to other interface (bridge)
	Can1.sendFrame(*frame);
}

void can1_got_frame_due(CAN_FRAME *frame) {
	//process frame
	got_frame(frame->id, frame->extended, frame->length, &(frame->data), 1);

	//send to other interface (bridge)
	Can0.sendFrame(*frame);
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
	Can0.enableFIFO();
	Can0.enableFIFOInterrupt();
	Can0.onReceive(can0_got_frame_teensy);
	Can0.enableMBInterrupts(FIFO);
	Can0.enableMBInterrupts();

	Can1.begin();
	Can1.setBaudRate(250000);
	Can1.enableFIFO();
	Can1.enableFIFOInterrupt();
	Can1.onReceive(can1_got_frame_teensy);
	Can1.enableMBInterrupts(FIFO);
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

