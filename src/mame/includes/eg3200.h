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
		, m_io_keyboard(*this, "LINE%u", 0)
        { }

	void eg3200(machine_config &config);

	void init_eg3200();

protected:
	virtual void machine_start() override;
	virtual void machine_reset() override;

private:
	DECLARE_FLOPPY_FORMATS(floppy_formats);
	DECLARE_READ8_MEMBER(printer_r);
	DECLARE_WRITE8_MEMBER(printer_w);
	DECLARE_WRITE8_MEMBER(port_bank_w);
	DECLARE_WRITE8_MEMBER(crtc_addr);
	DECLARE_WRITE8_MEMBER(crtc_ctrl);
	DECLARE_READ8_MEMBER(irq_status_r);
	DECLARE_WRITE8_MEMBER(motor_w);
        DECLARE_WRITE8_MEMBER(dk_37ec_w);
	DECLARE_READ8_MEMBER(keyboard_r);

	INTERRUPT_GEN_MEMBER(rtc_interrupt);
	INTERRUPT_GEN_MEMBER(fdc_interrupt);
	DECLARE_WRITE_LINE_MEMBER(intrq_w);
	DECLARE_QUICKLOAD_LOAD_MEMBER(quickload_cb);
	uint32_t screen_update_eg3200(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void eg3200_io(address_map &map);
	void eg3200_mem(address_map &map);
	void eg3200_bank_dk(address_map &map);

	uint8_t m_vidmode; /* video mode  0: 16x64, 1: 24x80 */
	uint8_t m_irq;
	uint8_t m_mask;
	uint8_t m_nmi_mask;
	uint8_t m_port_ec;
	bool m_reg_load;
	uint8_t m_nmi_data;
	uint16_t m_start_address;
	uint8_t m_crtc_reg;
	uint8_t m_size_store;
	uint16_t m_timeout;
	std::unique_ptr<uint8_t[]> m_mem_video0;
	std::unique_ptr<uint8_t[]> m_mem_video1;
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
	required_ioport_array<11> m_io_keyboard;
};

#endif // MAME_INCLUDES_EG3200_H
