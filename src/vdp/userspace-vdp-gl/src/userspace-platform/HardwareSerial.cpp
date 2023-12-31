#include "HardwareSerial.h"
#include <stdio.h>
#include <mutex>
#include "vdplib.h"
#include "debug.h"

HardwareSerial Serial2;

static int debuglevel = DBG_INFO;
static int serialReady = 0;
static int portfd = -1;

extern "C" void z80_send_to_vdp(uint8_t b)
{
	if (! serialReady ) {
		portfd = vdplib_startup();
		if (portfd >= 0) {
			DBG_I("VDP Port Initialized\n");
			serialReady = 1;
		} else {
			DBG_E("Unable to initialize VDP Port\n");
		}
	}

	if (serialReady) {
		DBG_N("Sending 0x%02x (%d) (%c) to VDP...\n", b, b, b);
		vdplib_send(portfd, b);
	}
	Serial2.writeToInQueue(b);
}

extern "C" bool z80_recv_from_vdp(uint8_t *out)
{
	return Serial2.readFromOutQueue(out);
}

int HardwareSerial::available() {
	std::unique_lock<std::mutex> lock(m_lock_in);
	return m_buf_in.size();
}
int HardwareSerial::read() {
	std::unique_lock<std::mutex> lock(m_lock_in);
	uint8_t v = m_buf_in.front();
	m_buf_in.pop_front();
	return v;
}
bool HardwareSerial::readFromOutQueue(uint8_t *out) {
	std::unique_lock<std::mutex> lock(m_lock_out);
	if (m_buf_out.size()) {
		*out = m_buf_out.front();
		m_buf_out.pop_front();
		return true;
	} else {
		return false;
	}
}
size_t HardwareSerial::write(uint8_t c) {
	std::unique_lock<std::mutex> lock(m_lock_out);
	m_buf_out.push_back(c);
	return 1;
}
void HardwareSerial::writeToInQueue(uint8_t c) {
	std::unique_lock<std::mutex> lock(m_lock_in);
	m_buf_in.push_back(c);
}
