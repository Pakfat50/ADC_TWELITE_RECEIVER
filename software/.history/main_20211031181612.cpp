// use twelite mwx c++ template library
#include "ADC_EXT_LIB/fs3000_twelite.h"
#include "ADC_COMMON_LIB/adc_twelite_types.hpp"
#include <TWELITE>
#include <STG_STD>
#include <NWK_SIMPLE>
#include <BRD_APPTWELITE>

constexpr uint8_t FS3000_TYPE_ID = 10;
constexpr size_t FS3000_DATA_SIZE = 1;

/*** Config part */
// application ID
const uint32_t DEFAULT_APP_ID = 0x1234abcd;
// channel
const uint8_t DEFAULT_CHANNEL = 15;
// option bits
uint32_t OPT_BITS = 0;
// logical id
uint8_t LID = 0;

/*** function prototype */
MWX_APIRET transmit();
void receive();

/* Timer 周期*/
const uint16_t TIMER0_HZ = 5; //5Hz timer

/* コンパイル時設定 */
#define DEBUG_MODE

/* プロトタイプ */
float fs3000Data;
FS3000 *fs;

void setup() {
	/*** SETUP section */	
	// init vars

	// load board and settings
	auto&& set = the_twelite.settings.use<STG_STD>();
	auto&& brd = the_twelite.board.use<BRD_APPTWELITE>();
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();
	
	// settings: configure items
	set << SETTINGS::appname("BRD_APPTWELITE");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.
	LID = set.u8devid(); // logical ID

	// the twelite main class
	the_twelite
		<< set                      // apply settings (appid, ch, power)
		<< TWENET::rx_when_idle(0);  // open receive circuit (if not set, it can't listen packts from others)

	if (brd.get_M1()) { LID = 0; }

	// Register Network
	nwk << set // apply settings (LID and retry)
			;

	// if M1 pin is set, force parent device (LID=0)
	nwk << NWK_SIMPLE::logical_id(LID); // write logical id again.
	
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- PingPong sample (press 't' to transmit) ---" << mwx::crlf;

 	Wire.begin();
	delay(500);

	fs = new FS3000();
	if (fs->begin() == false) //Begin communication over I2C
	{
		//TBD 初期化時エラー処理
	}

	/*** INIT message */
	Serial 	<< "--- BRD_APPTWELITE ---" << mwx::crlf;
	Serial	<< format("-- app:x%08x/ch:%d/lid:%d"
					, the_twelite.get_appid()
					, the_twelite.get_channel()
					, nwk.get_config().u8Lid
				)
			<< mwx::crlf;
	Serial 	<< format("-- pw:%d/retry:%d/opt:x%08x"
					, the_twelite.get_tx_power()
					, nwk.get_config().u8RetryDefault
					, OPT_BITS
			)
			<< mwx::crlf;
	Timer0.begin(TIMER0_HZ); // 10Hz Timer 
}

void loop() {

	if (Timer0.available()) { //再送に即座に対応できるように、loopはタイマー割り込みで記述し、delayは極力使わない。

		fs3000Data = fs->readMetersPerSecond();
		#ifdef DEBUG_MODE
		Serial.print(millis());
		Serial.print("\t");
		Serial.println(fs3000Data);
		#endif

		transmit();
	}
}

/*** transmit a packet */
MWX_APIRET transmit() {
	SensorData<FS3000_DATA_SIZE> FS3000SensorData(FS3000_TYPE_ID);
	uint8_t payloadByte[FS3000SensorData.byteSize];

	if (auto&& pkt = the_twelite.network.use<NWK_SIMPLE>().prepare_tx_packet()) {
		// set tx packet behavior
		pkt << tx_addr(LID == 0 ? 0xFE : 0x00)  // 0..0xFF (LID 0:parent, FE:child w/ no id, FF:LID broad cast), 0x8XXXXXXX (long address)
			<< tx_retry(0x1) // set retry (0x1 send two times in total)
			<< tx_packet_delay(0,10,5); // send packet w/ delay (send first packet with randomized delay from 100 to 200ms, repeat every 20ms)
		
		if(FS3000SensorData.getByteData(&fs3000Data, payloadByte)){
			for (size_t i=0; i<FS3000SensorData.byteSize; i++){
				Serial.println(payloadByte[i],HEX);
				pack_bytes(pkt.get_payload(), payloadByte[i]); // adc values
			}
			// do transmit 
			return pkt.transmit();
		}
	}
	return MWX_APIRET(false, 0);
}
