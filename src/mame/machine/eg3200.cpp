// license:BSD-3-Clause
// copyright-holders:kkaempf
//**************************************************************************************************************************

#include "emu.h"
#include "includes/eg3200.h"

#define IRQ_RTC      0x80    /* RTC interrupt */
#define IRQ_FDC      0x40    /* FDC interrupt */
#define RTC_REG_COUNT 13     /* R0 .. R12 */

/* mode bits when writing to 0xE1 */
/* writing: set E1 to NO_MODE, write addr/data to E0, write WRITE_MODE, write NO_MODE */
/* -> high-to-low transition of WRITE_MODE triggers the actual write */
#define RTC_NO_MODE 0x00
#define RTC_READ_MODE 0x40
#define RTC_WRITE_MODE 0x80

/* E0: RTC addr/data
 *          read   write
 * - bit 0  D0     D0
 * - bit 1  D1     D1
 * - bit 2  D2     D2
 * - bit 3  D3     D3
 * - bit 4  -      A0
 * - bit 5  -      A1
 * - bit 6  -      A2
 * - bit 7  -      A3
 * 
 * R0:  S1   0..9
 * R1:  S10  0..5
 * R2:  MI1  0..9
 * R3:  MI10 0..5
 * R4:  H1   0..9
 * R5:  H10  0..1/2, D2 0:AM, 1:PM; D3 0: 12hr, 1: 24hr
 * R6:  W    0..6
 * R7:  D1   0..9
 * R8:  D10  0..3, D2 0: Feb has 28 days, 1: Feb has 29 days
 * R9:  MO1  0..9
 * R10: MO10 0..1
 * R11: Y1   0..9
 * R12: Y10  0..9
 */

/* 1Hz interrupt to increase clock */

void eg3200_state::rtc_clock()
{
//    logerror("%s\n", __func__);
    if (++m_rtc_regs[0] < 10)
        return;
    m_rtc_regs[0] = 0;
    if (++m_rtc_regs[1] < 6)
        return;
    m_rtc_regs[1] = 0;
    /* overflow to minute */
    if (++m_rtc_regs[2] < 10)
        return;
    m_rtc_regs[2] = 0;
    if (++m_rtc_regs[3] < 6)
        return;
    m_rtc_regs[3] = 0;
    /* overflow to hour */
    uint8_t h1 = m_rtc_regs[4];
    uint8_t val = m_rtc_regs[5];
    uint8_t h10 = (val & 0x03);
    uint8_t hour = h10 * 10 + h1 + 1;
    if (BIT(val, 3)) { /* 24hrs */
        if (hour < 24) {
            m_rtc_regs[4] = (hour % 10);
            m_rtc_regs[5] = (hour / 10) | (val & 0xc0);
            return;
        }
        /* fallthru: overflow */
    }
    else { /* 12hrs */
        if (hour < 13) {
            m_rtc_regs[4] = (hour % 10);
            m_rtc_regs[5] = (hour / 10) | (val & 0xc0);
            return;
        }
        else {
            val ^= 0x04; /* flip AM/PM */
            if (BIT(val, 2)) { /* flipped to PM -> set hour to 1 */
                m_rtc_regs[4] = 1;
                m_rtc_regs[5] = val & 0xc0;
                return;
            }
            /* fallthru: PM->AM overflow */
        }
    }
    /* overflow to weekday */
    if (++m_rtc_regs[6] > 6) {
        m_rtc_regs[6] = 0;
    }
    /* overflow to day */
    uint8_t m1 = m_rtc_regs[9];
    uint8_t m10 = m_rtc_regs[10];
    uint8_t month = m10 * 10 + m1;
    uint8_t d1 = m_rtc_regs[7];
    val = m_rtc_regs[8];
    uint8_t d10 = (val & 0x03);
    uint8_t day = d10 * 10 + d1 + 1;
    uint8_t max_day = 0;
    switch (month) {
        case 0: /* Jan - 31 */
        case 2: /* Mar */
        case 4: /* May */
        case 6: /* Jul */
        case 7: /* Aug */
        case 9: /* Okt */
        case 11: /* Dec */
            max_day = 31;
            break;
        case 1: /* Feb - 28/29 */
            if (BIT(val, 2))
                max_day = 29;
            else
                max_day = 28;
            break;
        case 3: /* Apr */
        case 5: /* Jun */
        case 8: /* Sep */
        case 10: /* Nov */
            max_day = 31;
            break;
        default:
            break;
    }
    if (day <= max_day) {
        m_rtc_regs[7] = day % 10;
        m_rtc_regs[8] = (day / 10) | (val & 0x40);
        return;
    }
    /* overflow to month */
    if (max_day == 29) { /* Feb, leap year */
        val = 0; /* reset leap flag */
    }
    /* set day to 1 */
    m_rtc_regs[7] = 1;
    m_rtc_regs[8] = (val & 0x40);
    if (month++ < 13) {
        m_rtc_regs[9] = month % 10;
        m_rtc_regs[10] = month / 10;
        return;
    }
    /* overflow to year */
    if (++m_rtc_regs[11] < 10)
        return;
    m_rtc_regs[11] = 0;
    if (++m_rtc_regs[12] < 10)
        return;
    m_rtc_regs[12] = 0;
    return;
}

void eg3200_state::rtc_w(uint8_t data)
{
    m_rtc_addr = (data >> 4) & 0x0f; /* address in upper nibble */
    m_rtc_data = data & 0x0f; /* data in lower nibble */
}

uint8_t eg3200_state::rtc_r()
{
    if (m_rtc_mode == RTC_READ_MODE) {
        if (m_rtc_addr < RTC_REG_COUNT) {
            return m_rtc_regs[m_rtc_addr];
        }
    }
    return 0;
}

/* E1: RTC rdwr
 * - bit 7: wr
 * - bit 6: rd
 * - bit 5..0: n/c
 */

void eg3200_state::rtc_rdwr_w(uint8_t data)
{
//    logerror("%s %02x\n", __func__, data);
    data &= (RTC_READ_MODE|RTC_WRITE_MODE);
    if (data == RTC_NO_MODE) {
        if (m_rtc_mode == RTC_WRITE_MODE) {
            /* high->low transition */
            if (m_rtc_addr < RTC_REG_COUNT) {
//                logerror("%s reg[%d] <- %d\n", __func__, m_rtc_addr, m_rtc_data);
                m_rtc_regs[m_rtc_addr] = m_rtc_data & 0x0f;
            }
        }
    }
    m_rtc_mode = data;
}

/*
 * FD: printer port (output = data, input = status)
 * - bit 7 : busy (active high)
 * - bit 6 : out of paper (active high)
 * - bit 5 : unit select (active low)
 * - bit 4 : held high 
 */

void eg3200_state::printer_w(uint8_t data)
{
	m_cent_data_out->write(data);
	m_centronics->write_strobe(0);
	m_centronics->write_strobe(1);
}

uint8_t eg3200_state::printer_r()
{
//	return m_cent_status_in->read();
    return 0x30; // not busy, not out of paper, not select, high
}


/* set density, write cmd */
void eg3200_state::dk_37ec_w(uint8_t data)
{
    if ((data & 0xF8) == 0xF8) {
	// switch fm/mfm
	m_fdc->dden_w(BIT(data, 0)?0:1);
    }
    else {
        m_fdc->cmd_w(data);
    }
}

/* set size, set sector */
void eg3200_state::dk_37ee_w(uint8_t data)
{
    if ((data & 0x80) == 0x80) {
	// switch 5.25" / 8"
	return; // nop - must switch 1793 clock freq
    }
    else {
        m_fdc->sector_w(data);
    }
}

void eg3200_state::motor_w(uint8_t data)
{
    logerror("eg3200_state::motor_w %02x\n", data);
	m_floppy = nullptr;

	if (BIT(data, 0)) m_floppy = m_floppy0->get_device();
	if (BIT(data, 1)) m_floppy = m_floppy1->get_device();
	if (BIT(data, 2)) m_floppy = m_floppy2->get_device();
	if (BIT(data, 3)) m_floppy = m_floppy3->get_device();

	m_fdc->set_floppy(m_floppy);

	if (m_floppy)
	{
//            if (data != m_motor) {
//                logerror("\tmotor_w %02x (%s)\n", data & 0x0f, (data & 0x10)?"back":"front");
//            }
		m_floppy->mon_w(0);
		m_floppy->ss_w(BIT(data, 4)); /* side select */
		m_timeout = 200;
	}
    m_motor = data;
}

/*
 * FA: memory bank switching, inverted (bit set to 0 => bank enabled)
 * bank 0 - 64k memory
 * - bit 0 : bank 1 - ROM 0x0000 - 0x0fff
 * - bit 1 : bank 2 - 16x64 Video 0x3c00 - 0x3fff
 * - bit 2 : bank 3 - 80x25 Video 0x4000 - 0x43ff (+ 0x4400 - 0x47ff optinal)
 * - bit 3 : bank 4 - Disk 0x37e0 - 0x37ef + Keyboard 0x3800 - 0x3bff
 * ...
 * - bit 7 : reset (not implemented)
 */

void eg3200_state::port_bank_w(uint8_t data)
{
    /* swap in ram */
    logerror("port_bank_w(%02x) rom %d, video0 %d, video1 %d, dk %d\n", data, BIT(data, 0), BIT(data, 1), BIT(data, 2), BIT(data, 3));

    membank("bankr_rom")->set_entry(BIT(data, 0));
    membank("bankw_rom")->set_entry(BIT(data, 0));
    membank("bank_video0")->set_entry(BIT(data, 1));
    membank("bank_video1")->set_entry(BIT(data, 2));
    m_bank_fdc->set_bank(BIT(data, 3));
    m_bank_keyboard->set_bank(BIT(data, 3));
}

/*
 * F5: What does 'high bit 1' mean in the video ram ?
 * 0 - trs-80 compatible 'block' graphics
 * 1 - inverted character bits
 */
void eg3200_state::video_invert(uint8_t data)
{
    m_vidinv = data;
}

/*
 * F7: CRTC control
 * F6: CRTC address
 */

void eg3200_state::crtc_ctrl(uint8_t data)
{
    switch (m_crtc_reg) {
    case 1: /* horiz displayed */
        m_vidmode = (data == 80) ? 1 : 0; /* poor mans 6845 emulation */
        break;
    case 6: /* vert displayed */
        if (m_vidmode > 0)
            m_vidmode = (data == 25) ? 2 : 1;
        else
            m_vidmode = 0;
        break;
    case 14: /* cursor lsb */
        m_cursor_lsb = data;
        break;
    case 15: /* cursor msb */
        m_cursor_msb = data;
        break;
    }
//    logerror("crtc %02x / %u\n", data, data);
}

void eg3200_state::crtc_addr(uint8_t data)
{
    /*
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

    if (data < 18) {
            logerror("crtc %s: ", regs[data]);
    }
    else {
            logerror("crtc reg %d ", data);
    }
    */
    m_crtc_reg = data; 
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
uint8_t eg3200_state::irq_status_r()
{
//    logerror("%s\n", __func__);
/* Whenever an interrupt occurs, 37E0 is read to see what devices require service.
    d7 = RTC
    d6 = FDC
    All interrupting devices are serviced in a single interrupt. There is a mask byte,
    which is dealt with by the DOS. We take the opportunity to reset the cpu INT line. */
//    logerror("irq_status_r %02x\n", m_irq);
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

#include <time.h>
#include <sys/time.h>

INTERRUPT_GEN_MEMBER(eg3200_state::rtc_interrupt)
{
//    logerror("%s\n", __func__);
	m_irq |= IRQ_RTC;
    if (m_int_counter++ == 40) {
        rtc_clock();
        m_int_counter = 0;
    }
	m_maincpu->set_input_line(0, HOLD_LINE);
/*
    static suseconds_t last_tv = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    logerror("\trtc_interrupt %ld:%03ld (%ld)\n", tv.tv_sec, tv.tv_usec, tv.tv_usec-last_tv);
    last_tv = tv.tv_usec;

	if (m_timeout)
	{
		m_timeout--;
		if (m_timeout == 0)
			if (m_floppy) {
                            logerror("%s timeout\n", __func__);
				m_floppy->mon_w(1);  // motor off
                        }
	}
 */
}

// The floppy sector has been read. Enable CPU and NMI.
WRITE_LINE_MEMBER(eg3200_state::intrq_w)
{
//    logerror("\tintrq_w %02x\n", state);
	if (state)
	{
		m_irq |= IRQ_FDC;
		m_maincpu->set_input_line(0, HOLD_LINE);
	}
	else
		m_irq &= ~IRQ_FDC;
}

/*************************************
 *      Keyboard                     *
 *************************************/
uint8_t eg3200_state::keyboard_r(offs_t offset)
{
	u8 i, result = 0;

        if ((offset & 0x00e0) < 0x00a0) {
    /* 0x3800 .. 0x389f -> check 8 addr lines */
            for (i = 0; i < 8; i++) {
                if (BIT(offset, i)) {
                    result |= m_io_keyboard[i]->read();
//            logerror("\tline%d -> %02x\n", i, result);
                }
            }
        }
        else {
    /* 0x38a0 .. 0x38ff -> check 5 addr lines */
            for (i = 0; i < 5; i++) {
                if (BIT(offset, i)) {
                    result |= m_io_keyboard[i]->read();
//            logerror("\tline%d -> %02x\n", i, result);
                }
            }
            switch (offset & 0x00e0) {
                /* 0x38a0, 0x38c0, 0x38e0 -> check separately */
            case 0x00a0:
                result |= m_io_keyboard[8]->read();
                break;
            case 0x00c0:
                result |= m_io_keyboard[9]->read();
                break;
            case 0x00e0:
                result |= m_io_keyboard[10]->read();
                break;
            default:
                break;
            }
        }
    if (result != 0) {
        logerror("keyboard_r(38%02x) => %02x\n", offset, result);
    }
	return result;
}


/*************************************
 *  Machine              *
 *************************************/

void eg3200_state::machine_start()
{
	m_vidmode = 0;
        m_vidinv = 0;
	m_reg_load = 1;
	m_nmi_data = 0;
	m_timeout = 1;
        m_mem_video0 = std::make_unique<uint8_t[]>(0x0400);
        m_mem_video1 = std::make_unique<uint8_t[]>(0x0400);
        m_rtc_regs = std::make_unique<uint8_t[]>(RTC_REG_COUNT);
	uint8_t *rom = memregion("bios")->base();
	uint8_t *ram = m_mainram->pointer();

	membank("bankr_rom")->configure_entry(0, &rom[0]);
	membank("bankr_rom")->configure_entry(1, &ram[0]);
	membank("bankw_rom")->configure_entry(0, &ram[0]);
	membank("bankw_rom")->configure_entry(1, &ram[0]);
	membank("bank_video0")->configure_entry(0, m_mem_video0.get());
	membank("bank_video0")->configure_entry(1, &ram[0x3c00]);
	membank("bank_video1")->configure_entry(0, m_mem_video1.get());
	membank("bank_video1")->configure_entry(1, &ram[0x4000]);
}


void eg3200_state::machine_reset()
{
    uint8_t wday;
    struct tm *lt;
    time_t t;
	m_size_store = 0xff;
        m_irq = 0;
        m_int_counter = 0;
        m_vidmode = 0;
        m_vidinv = 0;
        m_rtc_mode = RTC_READ_MODE;
    time(&t);
    lt = localtime(&t);
    if (lt) {
        m_rtc_regs[0] = lt->tm_sec % 10;
        m_rtc_regs[1] = lt->tm_sec / 10;
        m_rtc_regs[2] = lt->tm_min % 10;
        m_rtc_regs[3] = lt->tm_min / 10;
        m_rtc_regs[4] = lt->tm_hour % 10;
        m_rtc_regs[5] = (lt->tm_hour / 10) | 0x08; /* 24hr format */
        /* Tue, 29-Jun-86, 12:34:56 */
        wday = lt->tm_wday; /* 0: sunday */
        if (wday == 0)
          wday = 7;
        m_rtc_regs[6] = wday - 1; /* 0: Mon */
        m_rtc_regs[7] = lt->tm_mday % 10;
        m_rtc_regs[8] = lt->tm_mday / 10;
        m_rtc_regs[9] = (lt->tm_mon + 1) % 10;
        m_rtc_regs[10] = (lt->tm_mon + 1) / 10;
        m_rtc_regs[11] = (lt->tm_year - 100) % 10;
        m_rtc_regs[12] = (lt->tm_year - 100) / 10;
    }
        /* rom, video0, and dskkbd */
        membank("bankr_rom")->set_entry(0);
        membank("bankw_rom")->set_entry(0);
        membank("bank_video0")->set_entry(0);
        membank("bank_video1")->set_entry(1);
        m_bank_fdc->set_bank(0);
        m_bank_keyboard->set_bank(0);
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
