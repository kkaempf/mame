// license:BSD-3-Clause
// based-on: trs80.h
// copyright-holders:kkaempf
//*****************************************************************************

#ifndef MAME_INCLUDES_EG3200_H
#define MAME_INCLUDES_EG3200_H

#pragma once

#include "bus/centronics/ctronics.h"
#include "cpu/z80/z80.h"
#include "machine/ram.h"
#include "machine/bankdev.h"
#include "imagedev/floppy.h"
#include "imagedev/snapquik.h"
#include "bus/rs232/rs232.h"
#include "machine/buffer.h"
#include "machine/wd_fdc.h"
#include "emupal.h"

class eg3200_state : public driver_device
{
public:
	eg3200_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_mainram(*this, RAM_TAG)
		, m_p_chargen(*this, "chargen")
		, m_bank_dk(*this, "bank_dk")
		, m_centronics(*this, "centronics")
		, m_cent_data_out(*this, "cent_data_out")
		, m_cent_status_in(*this, "cent_status_in")
		, m_fdc(*this, "fdc")
		, m_floppy0(*this, "fdc:0")
		, m_floppy1(*this, "fdc:1")
		, m_floppy2(*this, "fdc:2")
		, m_floppy3(*this, "fdc:3")
		, m_io_keyboard(*this, "LINE%u", 0)
        { }

	void eg3200(machine_config &config);

	void init_eg3200();

protected:
	virtual void machine_start() override;
	virtual void machine_reset() override;

private:
	static void floppy_formats(format_registration &fr);
	uint8_t printer_r();
	void printer_w(uint8_t data);
	void port_bank_w(uint8_t data);
	void crtc_addr(uint8_t data);
	void crtc_ctrl(uint8_t data);
	uint8_t irq_status_r();
	void motor_w(uint8_t data);
	void dk_37ec_w(uint8_t data);
	void dk_37ee_w(uint8_t data);
	void video_invert(uint8_t data);
	uint8_t keyboard_r(offs_t offset);
	uint8_t rtc_r();
	void rtc_w(uint8_t data);
	void rtc_rdwr_w(uint8_t data);

	INTERRUPT_GEN_MEMBER(rtc_interrupt);

	DECLARE_WRITE_LINE_MEMBER(intrq_w);
	DECLARE_QUICKLOAD_LOAD_MEMBER(quickload_cb);
	uint32_t screen_update_eg3200(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void eg3200_io(address_map &map);
	void eg3200_mem(address_map &map);
	void eg3200_bank_dk(address_map &map);
	void rtc_clock();
        uint8_t m_int_counter; /* count 40 25msec interrupts for 1Hz RTC */
	uint8_t m_vidmode; /* video mode  0: 16x64, 1: 24x80, 2: 25x80 */
        uint8_t m_vidinv; /* 0 - trs-80 block graphics, 1 - invers characters */
        uint8_t m_cursor_msb; /* cursor position */
        uint8_t m_cursor_lsb;
        uint8_t m_rtc_mode; /* port 0xE1, read/write bits */
        uint8_t m_rtc_addr; /* addr written to 0xE0 */
        uint8_t m_rtc_data; /* data written to 0xE0 */
	uint8_t m_irq;
	uint8_t m_mask;
	uint8_t m_nmi_mask;
	uint8_t m_motor; /* last value written to 0x37e1 */
	bool m_reg_load;
	uint8_t m_nmi_data;
	uint16_t m_start_address;
	uint8_t m_crtc_reg;
	uint8_t m_size_store;
	uint16_t m_timeout;
	std::unique_ptr<uint8_t[]> m_mem_video0;
	std::unique_ptr<uint8_t[]> m_mem_video1;
	std::unique_ptr<uint8_t[]> m_rtc_regs; // rtc registers
	floppy_image_device *m_floppy;
	required_device<cpu_device> m_maincpu;
	required_device<ram_device> m_mainram;
	required_region_ptr<u8> m_p_chargen;
	required_device<address_map_bank_device> m_bank_dk;
	optional_device<centronics_device> m_centronics;
	optional_device<output_latch_device> m_cent_data_out;
	optional_device<input_buffer_device> m_cent_status_in;
	optional_device<fd1793_device> m_fdc;
	optional_device<floppy_connector> m_floppy0;
	optional_device<floppy_connector> m_floppy1;
	optional_device<floppy_connector> m_floppy2;
	optional_device<floppy_connector> m_floppy3;
	required_ioport_array<11> m_io_keyboard;
};

#endif // MAME_INCLUDES_EG3200_H
