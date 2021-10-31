// use twelite mwx c++ template library
#include <TWELITE>
#include <NWK_SIMPLE>
#include <MONOSTICK>
#include <STG_STD>
#include "ADC_COMMON_LIB/adc_twelite_types.hpp"

/*** Config part */
// application ID
const uint32_t DEFAULT_APP_ID = 0x1234abcd;
// channel
const uint8_t DEFAULT_CHANNEL = 13;
// option bits
uint32_t OPT_BITS = 0;

/*** application defs */
constexpr uint8_t FS3000_TYPE_ID = 10;
constexpr size_t FS3000_FLOAT_DATA_SIZE = 1;
constexpr size_t FS3000_BYTE_DATA_SIZE = floatSizeToByteSize(FS3000_FLOAT_DATA_SIZE);

std::array<float, FS3000_FLOAT_DATA_SIZE> fs3000FloatData;
smplbuf_u8<FS3000_BYTE_DATA_SIZE> fs3000ByteData;

/*** setup procedure (run once at cold boot) */
void setup() {
	/*** SETUP section */
	auto&& brd = the_twelite.board.use<MONOSTICK>();
	auto&& set = the_twelite.settings.use<STG_STD>();
	auto&& nwk = the_twelite.network.use<NWK_SIMPLE>();

	// settings: configure items
	set << SETTINGS::appname("PARENT");
	set << SETTINGS::appid_default(DEFAULT_APP_ID); // set default appID
	set << SETTINGS::ch_default(DEFAULT_CHANNEL); // set default channel
	set << SETTINGS::lid_default(0x00); // set default LID
	set.hide_items(E_STGSTD_SETID::OPT_DWORD2, E_STGSTD_SETID::OPT_DWORD3, E_STGSTD_SETID::OPT_DWORD4, E_STGSTD_SETID::ENC_KEY_STRING, E_STGSTD_SETID::ENC_MODE);
	set.reload(); // load from EEPROM.
	OPT_BITS = set.u32opt1(); // this value is not used in this example.

	// the twelite main class
	the_twelite
		<< set                    // apply settings (appid, ch, power)
		<< TWENET::rx_when_idle() // open receive circuit (if not set, it can't listen packts from others)
		;

	// Register Network
	nwk << set;							// apply settings (LID and retry)
	nwk << NWK_SIMPLE::logical_id(0x00) // set Logical ID. (0x00 means parent device)
		;

	// configure hardware
	brd.set_led_red(LED_TIMER::ON_RX, 200); // RED (on receiving)
	brd.set_led_yellow(LED_TIMER::BLINK, 500); // YELLOW (blinking)

	/*** BEGIN section */
	the_twelite.begin(); // start twelite!

	/*** INIT message */
	Serial << "--- MONOSTICK_Parent act ---" << mwx::crlf;
}

/*** loop procedure (called every event) */
void loop() {
}

void on_rx_packet(packet_rx& rx, bool_t &handled) {
	SensorData FS3000SensorData(FS3000_TYPE_ID, FS3000_FLOAT_DATA_SIZE);
	
	FS3000SensorData.getFloatData(rx.get_payload(), fs3000ByteData);
	Serial << fs3000ByteData[0];
	Serial << mwx::crlf;
	Serial.flush();

}
