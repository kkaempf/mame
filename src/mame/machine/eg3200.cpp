// license:BSD-3-Clause
// copyright-holders:kkaempf
//**************************************************************************************************************************

#include "emu.h"
#include "includes/eg3200.h"


/*
 * FD: printer port (output = data, input = status)
 * - bit 7 : busy (active high)
 * - bit 6 : out of paper (active high)
 * - bit 5 : unit select (active low)
 * - bit 4 : held high 
 */

WRITE8_MEMBER( eg3200_state::printer_w )
{
	m_cent_data_out->write(data);
	m_centronics->write_strobe(0);
	m_centronics->write_strobe(1);
}

READ8_MEMBER( eg3200_state::printer_r )
{
	return m_cent_status_in->read();
}


/* set density, write cmd */
WRITE8_MEMBER( eg3200_state::dk_37ec_w)
{
    if ((data & 0xF8) == 0xF8) {
	// switch fm/mfm
	m_fdc->dden_w(BIT(data, 0));
//        logerror("0x37ec w %s-density\n", (BIT(data, 0))?"double":"single");
    }
    else {
//        logerror("eg3200_state::cmd_w %02x\n", data);
        m_fdc->cmd_w(data);
    }
}

WRITE8_MEMBER( eg3200_state::motor_w )
{
//    logerror("eg3200_state::motor_w %02x\n", data);
	m_floppy = nullptr;

	if (BIT(data, 0)) m_floppy = m_floppy0->get_device();
	if (BIT(data, 1)) m_floppy = m_floppy1->get_device();
//	if (BIT(data, 2)) m_floppy = m_floppy2->get_device();
//	if (BIT(data, 3)) m_floppy = m_floppy3->get_device();

	m_fdc->set_floppy(m_floppy);

	if (m_floppy)
	{
//            logerror("\tmotor_w %02x\n", data);
		m_floppy->mon_w(1);
		m_floppy->ss_w(BIT(data, 4)); /* side select */
		m_timeout = 200;
	}
}

/*
 * FA: memory bank switching, inverted (bit set to 0 => bank enabled)
 * bank 0 - 64k memory
 * - bit 0 : bank 1 - ROM 0x0000 - 0x2fff
 * - bit 1 : bank 2 - 16x64 Video 0x3c00 - 0x3fff
 * - bit 2 : bank 3 - 80x25 Video 0x4000 - 0x43ff (+ 0x4400 - 0x47ff optinal)
 * - bit 3 : bank 4 - Disk 0x37e0 - 0x37ef + Keyboard 0x3800 - 0x3bff
 */

WRITE8_MEMBER( eg3200_state::port_bank_w )
{
    /* swap in ram */
    logerror("port_bank_w(%02x) rom %d, video0 %d, video1 %d, dk %d\n", data, BIT(data, 0), BIT(data, 1), BIT(data, 2), BIT(data, 3));

    membank("bankr_rom")->set_entry(BIT(data, 0));
    membank("bank_video0")->set_entry(BIT(data, 1));
    membank("bank_video1")->set_entry(BIT(data, 2));
    m_bank_dk->set_bank(BIT(data, 3));
}

/*
 * F7: CRTC control
 * F6: CRTC address
 */

WRITE8_MEMBER( eg3200_state::crtc_ctrl )
{
    if (m_crtc_reg == 1) /* horiz displayed */
            m_vidmode = (data == 80) ? 1 : 0; /* poor mans 6845 emulation */
    logerror("crtc %02x / %u\n", data, data);
}

WRITE8_MEMBER( eg3200_state::crtc_addr )
{
    const char *regs[] = {
        "Horiz total",
        "Horiz displayed",
        "HSync pos",
        "HSync width",
        "Vert total",
        "Vert adjust",
        "Vert displayed",
        "VSync pos",
        "Interlace",
        "Max scan line addr",
        "Cursor start",
        "Cursor end",
        "Start addr (H)",
        "Start addr (L)",
        "Cursor (H)",
        "Cursor (L)",
        "Light pen (H)",
        "Light pen (L)"
    };
    m_crtc_reg = data; 
    if (data < 18) {
            logerror("crtc %s: ", regs[data]);
    }
    else {
            logerror("crtc reg %d ", data);
    }
}


/* F5: video mode
 * - bit 0 : set - inverse video
 *           reset - TRS-80 compatible

WRITE8_MEMBER( eg3200_state::port_f5_w )
{
}
 */

/* UART
 * EF: master reset
 * EE: modem status
 * ED: line status
 * EC: modem control
 * EB: UART line control
 * - bit 7: 0 - ports E8:data, E9:interrupt enable
 *          1 - ports E8/E9: divisor latch
 * EA: interrupt identification (read only)
 * E9: UART divisor msb
 * E8: UART divisor lsb
 * - divisor = 3.072.000 / (baud rate * 16)
 * E9: interupt enable
 * E8: receiver buffer (read), transmit buffer (write)
 */

/* RTC
 * E1: RTC write
 * - bit 6 : rd
 * - bit 7 : wr
 * 
 * E0: RTC addr/data
 * - bit 0: d0
 * - bit 1: d1
 * - bit 2: d2
 * - bit 3: d3
 * - bit 4: a0 (write only)
 * - bit 5: a1 (write only)
 * - bit 6: a2 (write only)
 * - bit 7: a3 (write only)
 */

/* Read interrupt status
   0x37e0
     bit 0: -
     bit 1: -
     bit 2: -
     bit 3: -
     bit 4: -
     bit 6: interrupt (read)
     bit 7: rtc (read, 25ms)
 */
READ8_MEMBER( eg3200_state::irq_status_r )
{
	u8 result = m_irq;
	m_maincpu->set_input_line(0, CLEAR_LINE);
	m_irq = 0;
	return result;
}

/*************************************
 *
 *      Interrupt handlers.
 *
 *************************************/

#include <sys/time.h>

INTERRUPT_GEN_MEMBER(eg3200_state::rtc_interrupt)
{
/*
    static suseconds_t last_tv = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    logerror("\trtc_interrupt %ld:%03ld (%ld)\n", tv.tv_sec, tv.tv_usec, tv.tv_usec-last_tv);
    last_tv = tv.tv_usec;
 */
	if (m_timeout)
	{
		m_timeout--;
		if (m_timeout == 0)
			if (m_floppy)
				m_floppy->mon_w(1);  // motor off
	}
}

// The floppy sector has been read. Enable CPU and NMI.
WRITE_LINE_MEMBER(eg3200_state::intrq_w)
{
	m_intrq_off = state ? false : true;
	if (state)
	{
		m_maincpu->set_input_line(Z80_INPUT_LINE_WAIT, CLEAR_LINE);
		m_wait = false;
		if (BIT(m_nmi_mask, 7))
		{
			m_nmi_data |= 0x80;
			//m_maincpu->pulse_input_line(INPUT_LINE_NMI, attotime::zero);
			m_maincpu->set_input_line(INPUT_LINE_NMI, HOLD_LINE);
		}
	}
}

// The next byte from floppy is available. Enable CPU so it can get the byte.
WRITE_LINE_MEMBER(eg3200_state::drq_w)
{
	m_drq_off = state ? false : true;
	if (state)
	{
		m_maincpu->set_input_line(Z80_INPUT_LINE_WAIT, CLEAR_LINE);
		m_wait = false;
	}
}


/*************************************
 *      Keyboard                     *
 *************************************/
READ8_MEMBER( eg3200_state::keyboard_r )
{
	u8 i, result = 0;
if (offset == 0x30)
            return (u8)0x04; /* "0": 0x01, "1": 0x02, "2": 0x04 */
	for (i = 0; i < 8; i++)
		if (BIT(offset, i))
			result |= m_io_keyboard[i]->read();

	return result;
}


/*************************************
 *  Machine              *
 *************************************/

void eg3200_state::machine_start()
{
    logerror("eg3200_state::machine_start()\n");
	m_vidmode = 0;
	m_reg_load = 1;
	m_nmi_data = 0;
	m_timeout = 1;
	m_wait = 0;
        m_mem_video0 = std::make_unique<uint8_t[]>(0x0400);
        m_mem_video1 = std::make_unique<uint8_t[]>(0x0400);
	uint8_t *rom = memregion("bios")->base();
	uint8_t *ram = m_mainram->pointer();

	membank("bankr_rom")->configure_entry(0, &rom[0]);
	membank("bankr_rom")->configure_entry(1, &ram[0]);
	membank("bankw_rom")->configure_entry(0, &ram[0]);
	membank("bank_video0")->configure_entry(0, m_mem_video0.get());
	membank("bank_video0")->configure_entry(1, &ram[0x3c00]);
	membank("bank_video1")->configure_entry(0, m_mem_video1.get());
	membank("bank_video1")->configure_entry(1, &ram[0x4000]);
    logerror("eg3200_state::machine_start() done m_mem_video0 %p, m_mem_video1 %p\n", m_mem_video0.get(), m_mem_video1.get());
 }

void eg3200_state::machine_reset()
{
    logerror("eg3200_state::machine_reset()\n");
	m_size_store = 0xff;
	m_drq_off = true;
	m_intrq_off = true;
        m_vidmode = 0;

        /* rom, video0, and dskkbd */
        membank("bankr_rom")->set_entry(0);
        membank("bankw_rom")->set_entry(0);
        membank("bank_video0")->set_entry(0);
        membank("bank_video1")->set_entry(1);
        m_bank_dk->set_bank(0);

    logerror("eg3200_state::machine_reset() done\n");
}


/***************************************************************************
    PARAMETERS
***************************************************************************/

#define LOG 1

#define CMD_TYPE_OBJECT_CODE                            0x01
#define CMD_TYPE_TRANSFER_ADDRESS                       0x02
#define CMD_TYPE_END_OF_PARTITIONED_DATA_SET_MEMBER     0x04
#define CMD_TYPE_LOAD_MODULE_HEADER                     0x05
#define CMD_TYPE_PARTITIONED_DATA_SET_HEADER            0x06
#define CMD_TYPE_PATCH_NAME_HEADER                      0x07
#define CMD_TYPE_ISAM_DIRECTORY_ENTRY                   0x08
#define CMD_TYPE_END_OF_ISAM_DIRECTORY_ENTRY            0x0a
#define CMD_TYPE_PDS_DIRECTORY_ENTRY                    0x0c
#define CMD_TYPE_END_OF_PDS_DIRECTORY_ENTRY             0x0e
#define CMD_TYPE_YANKED_LOAD_BLOCK                      0x10
#define CMD_TYPE_COPYRIGHT_BLOCK                        0x1f

/***************************************************************************
    IMPLEMENTATION
***************************************************************************/

QUICKLOAD_LOAD_MEMBER(eg3200_state::quickload_cb)
{
	address_space &program = m_maincpu->space(AS_PROGRAM);

	uint8_t type, length;
	uint8_t data[0x100];
	uint8_t addr[2];
	void *ptr;

	while (!image.image_feof())
	{
		image.fread( &type, 1);
		image.fread( &length, 1);

		length -= 2;
		int block_length = length ? length : 256;

		switch (type)
		{
		case CMD_TYPE_OBJECT_CODE:
			{
			image.fread( &addr, 2);
			uint16_t address = (addr[1] << 8) | addr[0];
			if (LOG) logerror("/CMD object code block: address %04x length %u\n", address, block_length);
			ptr = program.get_write_ptr(address);
			image.fread( ptr, block_length);
			}
			break;

		case CMD_TYPE_TRANSFER_ADDRESS:
			{
			image.fread( &addr, 2);
			uint16_t address = (addr[1] << 8) | addr[0];
			if (LOG) logerror("/CMD transfer address %04x\n", address);
			m_maincpu->set_state_int(Z80_PC, address);
			}
			break;

		case CMD_TYPE_LOAD_MODULE_HEADER:
			image.fread( &data, block_length);
			if (LOG) logerror("/CMD load module header '%s'\n", data);
			break;

		case CMD_TYPE_COPYRIGHT_BLOCK:
			image.fread( &data, block_length);
			if (LOG) logerror("/CMD copyright block '%s'\n", data);
			break;

		default:
			image.fread( &data, block_length);
			logerror("/CMD unsupported block type %u!\n", type);
		}
	}

	return image_init_result::PASS;
}
