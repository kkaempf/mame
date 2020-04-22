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
		, m_region_maincpu(*this, "maincpu")
		, m_p_chargen(*this, "chargen")
		, m_p_videoram(*this, "videoram")
		, m_bankdev(*this, "bankdev")
		, m_centronics(*this, "centronics")
		, m_cent_data_out(*this, "cent_data_out")
		, m_cent_status_in(*this, "cent_status_in")
		, m_fdc(*this, "fdc")
		, m_floppy0(*this, "fdc:0")
		, m_floppy1(*this, "fdc:1")
		, m_io_config(*this, "CONFIG")
		, m_io_keyboard(*this, "LINE%u", 0)
		, m_mainram(*this, RAM_TAG)
		, m_bank(*this, "banked_mem")
		, m_32kbanks(*this, "bank%u", 0U)
		, m_16kbank(*this, "16kbank")
		, m_vidbank(*this, "vidbank")
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
	DECLARE_WRITE8_MEMBER(dsksel_w);
	DECLARE_WRITE8_MEMBER(crtc_addr);
	DECLARE_WRITE8_MEMBER(crtc_ctrl);
	DECLARE_READ8_MEMBER(irq_status_r);
	DECLARE_READ8_MEMBER(keyboard_r);
	DECLARE_READ8_MEMBER(wd179x_r);

	INTERRUPT_GEN_MEMBER(rtc_interrupt);
	INTERRUPT_GEN_MEMBER(fdc_interrupt);
	DECLARE_WRITE_LINE_MEMBER(intrq_w);
	DECLARE_WRITE_LINE_MEMBER(drq_w);
	DECLARE_QUICKLOAD_LOAD_MEMBER(quickload_cb);
	uint32_t screen_update_eg3200(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	void eg3200_io(address_map &map);
	void eg3200_mem(address_map &map);
	void eg3200_banked_mem(address_map &map);

	uint8_t m_mode;
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
	bool m_wait;
	bool m_drq_off;
	bool m_intrq_off;
	floppy_image_device *m_floppy;
	required_device<cpu_device> m_maincpu;
	required_memory_region m_region_maincpu;
	required_region_ptr<u8> m_p_chargen;
	optional_shared_ptr<u8> m_p_videoram;
	optional_device<address_map_bank_device> m_bankdev;
	optional_device<centronics_device> m_centronics;
	optional_device<output_latch_device> m_cent_data_out;
	optional_device<input_buffer_device> m_cent_status_in;
	optional_device<fd1793_device> m_fdc;
	optional_device<floppy_connector> m_floppy0;
	optional_device<floppy_connector> m_floppy1;
	optional_ioport m_io_config;
	required_ioport_array<8> m_io_keyboard;
	optional_device<ram_device>                 m_mainram;
	optional_device<address_map_bank_device>    m_bank;
	optional_memory_bank_array<2>               m_32kbanks;
	optional_memory_bank                        m_16kbank;
	optional_memory_bank                        m_vidbank;
};

#endif // MAME_INCLUDES_EG3200_H
