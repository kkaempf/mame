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

/*
 * FA: memory bank switching, inverted (bit set to 0 => bank enabled)
 * - bit 0 : bank 1
 * - bit 1 : bank 2
 * - bit 2 : bank 3
 * - bit 3 : bank 4
 */

WRITE8_MEMBER( eg3200_state::port_bank_w )
{
}

/*
 * F7: CRTC control
 * F6: CRTC address
 */

WRITE8_MEMBER( eg3200_state::crtc_ctrl )
{
    logerror("crtc_ctrl %02x\n", data);
}

WRITE8_MEMBER( eg3200_state::crtc_addr )
{
    logerror("crtc_addr %02x\n", data);
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

/* Selection of drive and parameters - d6..d5 not emulated.
 A write also causes the selected drive motor to turn on for about 3 seconds.
 When the motor turns off, the drive is deselected.
   0x37e0
     bit 0: drive 0
     bit 1: drive 1
     bit 2: drive 2
     bit 3: drive 3
     bit 4: side select (0 = front, 1 = back)
     bit 6: -
     bit 7: -
 */
/*
 M4:
    d7 1=MFM, 0=FM
    d6 1=Wait
    d5 1=Write Precompensation enabled
    d4 0=Side 0, 1=Side 1
    d3 1=select drive 3
    d2 1=select drive 2
    d1 1=select drive 1
    d0 1=select drive 0
 */

WRITE8_MEMBER( eg3200_state::dsksel_w )
{
	if (BIT(data, 6))
	{
		if (m_drq_off && m_intrq_off)
		{
			m_maincpu->set_input_line(Z80_INPUT_LINE_WAIT, ASSERT_LINE);
			m_wait = true;
		}
	}
	else
	{
		m_maincpu->set_input_line(Z80_INPUT_LINE_WAIT, CLEAR_LINE);
		m_wait = false;
	}

	m_floppy = nullptr;

	if (BIT(data, 0)) m_floppy = m_floppy0->get_device();
	if (BIT(data, 1)) m_floppy = m_floppy1->get_device();

	m_fdc->set_floppy(m_floppy);

	if (m_floppy)
	{
		m_floppy->mon_w(0);
		m_floppy->ss_w(BIT(data, 4));
		m_timeout = 1600;
	}

	m_fdc->dden_w(!BIT(data, 7));
}

/*************************************
 *
 *      Interrupt handlers.
 *
 *************************************/

INTERRUPT_GEN_MEMBER(eg3200_state::rtc_interrupt)
{
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
 *                                   *
 *      Memory handlers              *
 *                                   *
 *************************************/

READ8_MEMBER( eg3200_state::wd179x_r )
{
	uint8_t data = 0xff;
	if (BIT(m_io_config->read(), 7))
		data = m_fdc->status_r();

	return data;
}

/*************************************
 *      Keyboard                     *
 *************************************/
READ8_MEMBER( eg3200_state::keyboard_r )
{
	u8 i, result = 0;

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
	m_mode = 0; /* vide mode 1: 32, 0: 64 chars per line */
	m_reg_load = 1;
	m_nmi_data = 0;
	m_timeout = 1;
	m_wait = 0;
/*
	m_bank->set_stride(0x00000);
	m_bank->space(0).unmap_readwrite(0x08000, 0x0ffff);
	m_16kbank->configure_entries(0, 8, m_mainram->pointer() + 0x00000, 0x00000);
	m_vidbank->configure_entries(0, 2, &m_p_videoram[0], 0x0400);
*/
 }

void eg3200_state::machine_reset()
{
	m_size_store = 0xff;
	m_drq_off = true;
	m_intrq_off = true;
        m_mode = 0;
	address_space &mem = m_maincpu->space(AS_PROGRAM);

	port_bank_w(mem, 0, 1);    // enable rom
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
