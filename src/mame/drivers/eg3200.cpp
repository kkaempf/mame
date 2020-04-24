// license:BSD-3-Clause
// based-on: trs80.cpp
// copyright-holders:kkaempf
/***************************************************************************

EG3200 Eaca Computer Ltd 1982 
Video Genie III - german marketing name

https://www.old-computers.com/museum/computer.asp?c=130

Enhanced 'TRS-80'

- z80A (1.77 and 4.0 MHz clock)
- 64k ram (32 x 4116)
- Simple boot rom, no basic
  3 x 2716/2732 EPROM
- HD46505 crtc (compatible with Motorola 6845)
- 2716 eprom character
- 2k (4 x 2114) video ram 
  -> can do 64x16 (TRS-80 M1 compatible) or 80x24 (cp/m 2.2 compatible)
- Boots from disk
- 8250 serial
- 1771 + 1791 disk controller
- parallel port
- hardware clock (MSMS 832) https://www.datasheetarchive.com/?q=MSMS832

Banked memory (switched via port 0xFA)
 - Bank 0  0x0000 - 0xffff
   64k RAM
 - Bank 1  0x0000 - 0x7ff (2k boot rom)
   ROM/EPROM (up to 12k, 2k boot rom)
 - Bank 2  0x3c00 - 0x3fff
   video memory 0 (1k, 64x16, TRS-80 M1 compatible)
 - Bank 3  0x4000 - 0x43ff / 0x4400 - 0x47ff
   video memory 1 (additional 1k for 80x24 video mode)
   video memory 2 (programmable character generator)
 - Bank 4  0x37eX disk, 0x3800 - 0x3bff keyboard
   keyboard and disk control
   0x37e0
     bit 0: drive 0
     bit 1: drive 1
     bit 2: drive 2
     bit 3: drive 3
     bit 4: side select (0 = front, 1 = back)
     bit 6: interrupt (read)
     bit 7: rtc (read, 25ms)
   0x37ec
     fdc status / command
     bit 0: density select (1 = DD/1791, 0 = SD/1771)
     bit 1: 
     bit 2: 
     bit 3: must be 1 when selecting densite
     bit 4: must be 1 when selecting densite
     bit 5: must be 1 when selecting densite
     bit 6: must be 1 when selecting densite
     bit 7: must be 1 when selecting densite
   0x37ed
     track register
   0x37ee
     sector register
     bit 6: size (1 = 8", 0 = 5.25")
     bit 7: must be 1 when writing size
   0x37ef
     data register
   0x3801, 0x3802, 0x3804, 0x3808, 0x3810, 0x3820, 0x3840, 0x3880 - TRS-80 compatible
   0x38A0: F1 .. F8
   0x38C0: 0 .. 7 (numeric block)
   0x38E0: 8 9 00 lock , - . 
 - Loudspeaker in keyboard unit
   0x3860 - each access results in a 'pitch'
 
 Banks 1+2+4 will give a TRS-80 M1 memory layout

I/O ports

FD: printer port (output = data, input = status)
- bit 7 : busy (active high)
- bit 6 : out of paper (active high)
- bit 5 : unit select (active low)
- bit 4 : held high 

FA: memory bank switching, inverted (bit set to 0 => bank enabled)
- bit 0 : bank 1
- bit 1 : bank 2
- bit 2 : bank 3
- bit 3 : bank 4

F7: CRTC control
F6: CRTC address

F5: video mode
 - bit 0 : set - inverse video
           reset - TRS-80 compatible

EF: master reset
EE: modem status
ED: line status
EC: modem control
EB: UART line control
- bit 7: 0 - ports E8:data, E9:interrupt enable
         1 - ports E8/E9: divisor latch
EA: interrupt identification (read only)
E9: UART divisor msb
E8: UART divisor lsb
- divisor = 3.072.000 / (baud rate * 16)
E9: interupt enable
E8: receiver buffer (read), transmit buffer (write)

E1: RTC write
- bit 6 : rd
- bit 7 : wr

E0: RTC addr/data
- bit 0: d0
- bit 1: d1
- bit 2: d2
- bit 3: d3
- bit 4: a0 (write only)
- bit 5: a1 (write only)
- bit 6: a2 (write only)
- bit 7: a3 (write only)

***************************************************************************/

#include "emu.h"
#include "includes/eg3200.h"

#include "screen.h"
#include "speaker.h"

#include "formats/trs80_dsk.h"
#include "formats/dmk_dsk.h"


static void eg3200_floppies(device_slot_interface &device)
{
	device.option_add("sssd", FLOPPY_525_QD);
}

void eg3200_state::eg3200_io(address_map &map)
{
    logerror("eg3200_state::eg3200_io\n");
	map.global_mask(0xff);
	map(0xf6, 0xf6).w(FUNC(eg3200_state::crtc_addr));
	map(0xf7, 0xf7).w(FUNC(eg3200_state::crtc_ctrl));
	map(0xfa, 0xfa).w(FUNC(eg3200_state::port_bank_w));
        map(0xfd, 0xfd).rw(FUNC(eg3200_state::printer_r), FUNC(eg3200_state::printer_w));
}

void eg3200_state::eg3200_mem(address_map &map)
{
    logerror("eg3200_state::eg3200_mem()\n");
	map(0x0000, 0x07ff).bankr("bankr_rom").bankw("bankw_rom");
	map(0x0800, 0x36ff).ram();
	map(0x3700, 0x38ff).m(m_bank_dk, FUNC(address_map_bank_device::amap8));
	map(0x3900, 0x3bff).ram();
	map(0x3c00, 0x3fff).bankrw("bank_video0");
	map(0x4000, 0x43ff).bankrw("bank_video1");
	map(0x4400, 0xffff).ram();
}

/*
 * dk disk keyboard bank
 * bank 0: disk + keyboard
 * bank 1: ram
 * stride: 0x500 
 */
void eg3200_state::eg3200_bank_dk(address_map &map)
{
    logerror("eg3200_state::eg3200_bank_dk\n");
    /* mapped to 0x3700 */
        map(0x000, 0x0df).noprw();
	map(0x0e0, 0x0e3).rw(FUNC(eg3200_state::irq_status_r), FUNC(eg3200_state::motor_w));
        map(0x0e4, 0x0e7).noprw();
	map(0x0e8, 0x0eb).rw(FUNC(eg3200_state::printer_r), FUNC(eg3200_state::printer_w));
	map(0x0ec, 0x0ec).r(m_fdc, FUNC(fd1793_device::status_r));
	map(0x0ec, 0x0ec).w(FUNC(eg3200_state::dk_37ec_w)); //w(m_fdc, FUNC(fd1793_device::cmd_w));
	map(0x0ed, 0x0ed).rw(m_fdc, FUNC(fd1793_device::track_r), FUNC(fd1793_device::track_w));
	map(0x0ee, 0x0ee).rw(m_fdc, FUNC(fd1793_device::sector_r), FUNC(fd1793_device::sector_w));
	map(0x0ef, 0x0ef).rw(m_fdc, FUNC(fd1793_device::data_r), FUNC(fd1793_device::data_w));
    /* 0x3800 - 0x38ff - keyboard */
	map(0x100, 0x1ff).r(FUNC(eg3200_state::keyboard_r));
	map(0x100, 0x1ff).nopw();
    /* bank 1 */
        map(0x200, 0x3ff).ram();
}

static INPUT_PORTS_START( eg3200 )
	PORT_START("LINE0")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("@") PORT_CODE(KEYCODE_OPENBRACE)  PORT_CHAR('@')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)          PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)          PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)          PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)          PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)          PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)          PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)          PORT_CHAR('g') PORT_CHAR('G')

	PORT_START("LINE1")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)          PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)          PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)          PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)          PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)          PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)          PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)          PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)          PORT_CHAR('o') PORT_CHAR('O')

	PORT_START("LINE2")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)          PORT_CHAR('p') PORT_CHAR('P')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)          PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)          PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)          PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)          PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)          PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)          PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)          PORT_CHAR('w') PORT_CHAR('W')

	PORT_START("LINE3")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)          PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)          PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)          PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT(0xF8, 0x00, IPT_UNUSED) // these bits were tested and do nothing useful

	PORT_START("LINE4")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD)          PORT_CHAR('0')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD)          PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD)          PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD)          PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD)          PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD)          PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD)          PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD)          PORT_CHAR('7') PORT_CHAR('\'')

	PORT_START("LINE5")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD)          PORT_CHAR('8') PORT_CHAR('(')
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD)          PORT_CHAR('9') PORT_CHAR(')')
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME(": *") PORT_CODE(KEYCODE_MINUS)        PORT_CHAR(':') PORT_CHAR('*')
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("; +") PORT_CODE(KEYCODE_COLON)        PORT_CHAR(';') PORT_CHAR('+')
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME(", <") PORT_CODE(KEYCODE_COMMA)        PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("- =") PORT_CODE(KEYCODE_EQUALS)       PORT_CHAR('-') PORT_CHAR('=')
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME(". >") PORT_CODE(KEYCODE_STOP) PORT_CODE(KEYCODE_DEL_PAD)    PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("/ ?") PORT_CODE(KEYCODE_SLASH)        PORT_CHAR('/') PORT_CHAR('?')

	PORT_START("LINE6")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Clear") PORT_CODE(KEYCODE_HOME)       PORT_CHAR(UCHAR_MAMEKEY(F8))
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_END)        PORT_CHAR(UCHAR_MAMEKEY(F9))
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP)         PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)        PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	/* backspace do the same as cursor left */
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CODE(KEYCODE_BACKSPACE)   PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)      PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT(0x80, 0x00, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)      PORT_CHAR(' ')

	PORT_START("LINE7")
	PORT_BIT(0x01, 0x00, IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, 0x00, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT)
	// These keys are only on a Model 4. These bits do nothing on Model 3.
	PORT_BIT(0x04, 0x00, IPT_KEYBOARD) PORT_NAME("CTL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_SHIFT_2)
	PORT_BIT(0x08, 0x00, IPT_KEYBOARD) PORT_NAME("Caps") PORT_CODE(KEYCODE_CAPSLOCK) // When activated, lowercase entry is possible
	PORT_BIT(0x10, 0x00, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1) // prints tic character
	PORT_BIT(0x20, 0x00, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
	PORT_BIT(0x40, 0x00, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
	PORT_BIT(0x80, 0x00, IPT_UNUSED)
INPUT_PORTS_END


/**************************** F4 CHARACTER DISPLAYER ***********************************************************/
static const gfx_layout eg3200_charlayout =
{
	8, 8,           /* 8 x 8 characters */
	256,            /* 256 characters */
	1,          /* 1 bits per pixel */
	{ 0 },          /* no bitplanes */
	/* x offsets */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	/* y offsets */
	{  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8        /* every char takes 8 bytes */
};

static GFXDECODE_START(gfx_eg3200)
	GFXDECODE_ENTRY( "chargen", 0, eg3200_charlayout, 0, 1 )
GFXDECODE_END


FLOPPY_FORMATS_MEMBER( eg3200_state::floppy_formats )
	FLOPPY_TRS80_FORMAT,
	FLOPPY_DMK_FORMAT
FLOPPY_FORMATS_END

void eg3200_state::eg3200(machine_config &config)
{
	/* basic machine hardware */
	Z80(config, m_maincpu, 4_MHz_XTAL);
	m_maincpu->set_addrmap(AS_PROGRAM, &eg3200_state::eg3200_mem);
	m_maincpu->set_addrmap(AS_IO, &eg3200_state::eg3200_io);
	m_maincpu->set_periodic_int(FUNC(eg3200_state::rtc_interrupt), attotime::from_hz(4_MHz_XTAL / 100000)); /* 40Hz, 25 ms */

        RAM(config, RAM_TAG).set_default_size("64K");

	ADDRESS_MAP_BANK(config, "bank_dk").set_map(&eg3200_state::eg3200_bank_dk).set_options(ENDIANNESS_LITTLE, 8, 11, 0x200); /* 0x000 - 0x3ff -> 11 bits */

	/* video hardware */
	screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
	screen.set_raw(12.672_MHz_XTAL, 800, 0, 640, 264, 0, 240); // FIXME: these are Model 4 80-column parameters
	screen.set_screen_update(FUNC(eg3200_state::screen_update_eg3200));
	screen.set_palette("palette");

	GFXDECODE(config, "gfxdecode", "palette", gfx_eg3200);
	PALETTE(config, "palette", palette_device::MONOCHROME);

	QUICKLOAD(config, "quickload", "cmd", attotime::from_seconds(1)).set_load_callback(FUNC(eg3200_state::quickload_cb));

	FD1793(config, m_fdc, 4_MHz_XTAL / 4);
	m_fdc->intrq_wr_callback().set(FUNC(eg3200_state::intrq_w));
	m_fdc->drq_wr_callback().set(FUNC(eg3200_state::drq_w));

	// Internal drives
	FLOPPY_CONNECTOR(config, "fdc:0", eg3200_floppies, "sssd", eg3200_state::floppy_formats).enable_sound(true);
	FLOPPY_CONNECTOR(config, "fdc:1", eg3200_floppies, "sssd", eg3200_state::floppy_formats).enable_sound(true);

	CENTRONICS(config, m_centronics, centronics_devices, "printer");
	m_centronics->busy_handler().set(m_cent_status_in, FUNC(input_buffer_device::write_bit7));
	m_centronics->perror_handler().set(m_cent_status_in, FUNC(input_buffer_device::write_bit6));
	m_centronics->select_handler().set(m_cent_status_in, FUNC(input_buffer_device::write_bit5));
	m_centronics->fault_handler().set(m_cent_status_in, FUNC(input_buffer_device::write_bit4));

	INPUT_BUFFER(config, m_cent_status_in);

	OUTPUT_LATCH(config, m_cent_data_out);
	m_centronics->set_output_latch(*m_cent_data_out);

	RS232_PORT(config, "rs232", default_rs232_devices, nullptr);
}


ROM_START(eg3200)
	ROM_REGION(0x3000, "bios",0)
	ROM_LOAD("eg3200_system.z27",  0x0000, 0x0800, CRC(ef4fbd20) SHA1(5a6ad3e0a80b8c5eee7b235f6ecaba07bfca8267))

	ROM_REGION(0x0800, "chargen",0)
	ROM_LOAD("eg3200_video.z23", 0x0000, 0x0800, CRC(74aeca3b) SHA1(60071dea1177202fa727dc12c828fe097f0c7952))
ROM_END

ROM_START(genie3)
	ROM_REGION(0x3000, "bios",0)
	ROM_LOAD("eg3200_system.z27",  0x0000, 0x0800, CRC(ef4fbd20) SHA1(5a6ad3e0a80b8c5eee7b235f6ecaba07bfca8267))

	ROM_REGION(0x0800, "chargen",0)
	ROM_LOAD("eg3200_video.z23", 0x0000, 0x0800, CRC(74aeca3b) SHA1(60071dea1177202fa727dc12c828fe097f0c7952))
ROM_END


void eg3200_state::init_eg3200()
{
}

//    YEAR  NAME         PARENT    COMPAT    MACHINE   INPUT     CLASS          INIT             COMPANY               FULLNAME                FLAGS
COMP( 1982, eg3200,      0,        0,        eg3200,   eg3200,   eg3200_state,  init_eg3200,     "EACA Computers Ltd", "EG 3200",              MACHINE_NO_SOUND_HW )
COMP( 1982, genie3,      eg3200,   0,        eg3200,   eg3200,   eg3200_state,  init_eg3200,     "TCS Computer GmbH",  "Video Genie III",      MACHINE_NO_SOUND_HW )
