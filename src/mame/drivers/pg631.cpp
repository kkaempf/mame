// license:BSD-3-Clause
// copyright-holders:rfka01, K. Loy, kkaempf
/***************************************************************************

    Siemens PG 631 SKELETON
	
	The PG 631 is a programming device for the Siemens S5 series of programmable logic controllers.
	Later devices in this series are the PG 675 and PG 685 which include floppy or hard disk drives running
	DOS or PCP/M-86
	
	Dual oscillator 4.915MHz and 2.457MHz, this one feeds the 8085's CLK input (Pin 2)
	
	PG631 Memory Map V0.2
	=======================
	K.Loy, 18.04.2021

	Firmware 32 KByte
	16x 2716 EPROM         +-------- IC No. U21, 24, ...
                        ##
	0000...07FF  2716  CPU_21.BIN
	0800...0FFF  2716  CPU_24.BIN
	1000...17FF  2716  CPU_26.BIN
	1800...1FFF  2716  CPU_29.BIN
	2000...27FF  2716  CRT_08.BIN
	2800...2FFF  2716  CRT_09.BIN
	3000...37FF  2716  CRT_10.BIN
	3800...3FFF  2716  CRT_11.BIN
	4000...47FF  2716  CPU_36.BIN
	4800...4FFF  2716  CPU_39.BIN
	5000...57FF  2716  CPU_44.BIN
	5800...5FFF  2716  CPU_51.BIN
	6000...67FF  2716  CRT_12.BIN
	6800...6FFF  2716  CRT_13.BIN
	7000...77FF  2716  CRT_14.BIN
	7800...7FFF  2716  CRT_30.BIN
 
        C000...DFFF  RAM (optional)
 
	E000...E3FF  2114 1KRAM, U28/U23 (CPU board)
	E400...E7FF  2114 1KRAM, U25/U20 (CPU board)
	E800...EBFF  2114 1KRAM, U5/U7 (CRT board)
	EC00...EFFF  2114 1KRAM, U4/U6 (CRT board)
 
	F000...F7FF  2114 2kRAM, video RAM 
  
	F800...F8FF  8155 RAM, 256Byte
	F900         8155 Control
	F901         8155 Port A
	F901         8155 Port B
 
	FA00...FAFF  open bus, reading returns the low address
	FB00...FB7F  open bus, reading returns the low address
	FB80...FBFF  read returns FF
 
	FC00...FCFF  open bus, reading returns the low address
	FD00...FDFF  open bus, reading returns the low address
 
	FE00...      peripheral ICs
				 address decoder 74LS138, U43
	FE00         Y0: NC or unknown
	FE02...FE03  Y1: 8279 keyboard controller
	FE04...FE05  Y2: 8251 UART
	FE06...FE07  Y3: 8259 interrupr controller
	FE08...FE09  Y4: NC
	FE0A...FE0B  Y5: NC
	FE0C...FE0D  Y6: NC
	FE0E...FE0F  Y7: NC
 
	FE10...FE7F  open bus, reading returns the low address
	FE80...FEFF  read returns FF
	FF00...FEFF  open bus, reading returns the low address
 
	FFFE         6845 CRT-controller, address register
	FFFF         6845 CRT-controller, data register
	
	A 4702 takes care of baud rate generation, it has four inputs for the dividing factor, those inputs are set by a ROM which is set by the 8155 PIO
	

****************************************************************************/

#include "emu.h"
#include "cpu/i8085/i8085.h"
#include "machine/i8155.h"
#include "machine/i8279.h"
#include "screen.h"
#include "emupal.h"

class pg631_state : public driver_device
{
public:
	pg631_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_rom(*this, "maincpu")
		// , m_ramio(*this, "ramio")
                , m_video(*this, "video")
		, m_chargen(*this, "chargen")
                , m_keyboard(*this, "X%u", 0U)
		{ }

	void pg631(machine_config &config);
        uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
private:
	
	void io_map(address_map &map);
	void mem_map(address_map &map);

	void machine_reset() override;
	void machine_start() override;
	required_device<cpu_device> m_maincpu;
	required_region_ptr<u8> m_rom;
	// required_device<i8155_device> m_ramio;
	optional_shared_ptr<u8> m_video;
        required_region_ptr<u8> m_chargen;

	required_ioport_array<6> m_keyboard;
	void scanlines_w(u8 data);
	u8 kbd_r();
	u8 m_digit;
};

static const gfx_layout pg631_charlayout =
{
        8, 11,      /* 8 x 11 characters */
        128,        /* 128 characters */
        1,          /* 1 bits per pixel */
        { 0 },      /* no bitplanes */
        /* x offsets */
        { 0, 1, 2, 3, 4, 5, 6, 7 },
        /* y offsets */
        {  0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8 },
        8*16        /* every char takes 16 bytes in chargen rom */
};

static GFXDECODE_START(gfx_pg631)
        GFXDECODE_ENTRY( "chargen", 0, pg631_charlayout, 0, 1 )
GFXDECODE_END


void pg631_state::mem_map(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x7fff).rom().region("maincpu", 0);
	map(0xc000, 0xdfff).ram(); // RAM extension
	map(0xe000, 0xe7ff).ram(); // RAM on CPU board
	map(0xe800, 0xefff).ram(); // RAM on video board
        map(0xf000, 0xf7ff).ram().share(m_video); // 2k Video RAM
	map(0xf800, 0xf8ff).ram(); // 256 bytes RAM
	// map(0xf900, 0xf903).mirror(0xf9).rw(m_ramio, FUNC(i8155_device::memory_r), FUNC(i8155_device::memory_w));
	map(0xfe02, 0xfe03).rw("i8279", FUNC(i8279_device::read), FUNC(i8279_device::write));
}

void pg631_state::io_map(address_map &map)
{
	map.unmap_value_high();
}

uint32_t pg631_state::screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
    static int visible_set = 0;
	uint8_t y,ra,chr,gfx;
	uint16_t sy=0,ma=0,x;
    if (visible_set == 0) {
        screen.set_visible_area(0, 80*8-1, 0, 24*11-1); /* charater 8 x 11 */
        visible_set = 1;
    }
        /* line counter */
	for (y = 0; y < 24; y++)
        {
                for (ra = 0; ra < 11; ra++) /* raster line */
		{
			uint16_t *p = &bitmap.pix(sy++);

			for (x = ma; x < ma + 80; x++) /* memory address, iterate per line */
			{
                            uint8_t inverted;
				chr = m_video[x];
                            inverted = ((chr & 0x80) == 0x80)?0x00:0x01;
                                gfx = m_chargen[((chr & 0x7f)<<4) | ra]; /* 16 bytes per char in chargen rom */

                                /* Display a scanline of a character (8 pixels) */

				*p++ = BIT(gfx, 7) ^ inverted;
				*p++ = BIT(gfx, 6) ^ inverted;
				*p++ = BIT(gfx, 5) ^ inverted;
				*p++ = BIT(gfx, 4) ^ inverted;
				*p++ = BIT(gfx, 3) ^ inverted;
				*p++ = BIT(gfx, 2) ^ inverted;
				*p++ = BIT(gfx, 1) ^ inverted;
				*p++ = BIT(gfx, 0) ^ inverted;
			}
		}
		ma += 80;
	}
	return 0;
}


void pg631_state::scanlines_w(u8 data)
{
	m_digit = data & 7;
}

u8 pg631_state::kbd_r()
{
	uint8_t data = 0xff;

	if ((m_digit & 7) < 6)
		data = m_keyboard[m_digit & 3]->read();

	return data;
}

/* Input ports */
static INPUT_PORTS_START( pg631 )

/* The  PG 631 has no regular alphanumerical keyboard, just the programming keys in its front plate.

      [  |  ]    [  ^  ]  [ (X) ]     [  x| ]  [ |   ]     [     ]
      [  /  ]    [  | /]  [  |  ]     [     ]  [ |-->]     [     ]
      [  |  ]    [ <-- ]  [  v_ ]     [ ->| ]  [ | | ]     [     ]
Key   1/1        2/1      3/1         4/1      5/1         6/1


      [ |   ]    [  U  ]  [  N  ]     [     ]  [     ]     [     ]  [     ]  [     ]
      [ |-> ]    [     ]  [     ]     [  P  ]  [  E  ]     [  7  ]  [  8  ]  [  9  ]
      [ |   ]    [ -][-]  [-]|[-]     [     ]  [     ]     [     ]  [     ]  [     ]
Key   1/2        2/2      3/2         4/2      5/2         6/2      7/2      8/2

      [ =<-|]    [  O. ]  [  O  ]     [     ]  [     ]     [     ]  [     ]  [     ]
      [-   -]    [\   /]  [   | ]     [  B  ]  [  A  ]     [  4  ]  [  5  ]  [  6  ]
      [|->= ]    [ ]-[ ]  [ <-- ]     [     ]  [     ]     [     ]  [     ]  [     ]
Key   1/3        2/3      3/3         4/3      5/3         6/3      7/3      8/3

      [  ^  ]    [  U( ]  [  )  ]     [  BE ]  [     ]     [     ]  [     ]  [     ]
      [ |=| ]    [--|--]  [--|--]     [     ]  [  M  ]     [  1  ]  [  2  ]  [  3  ]
      [  v  ]    [ -|  ]  [  |- ]     [ BEB ]  [     ]     [     ]  [     ]  [     ]
Key   1/4        2/4      3/4         4/4      5/4         6/4      7/4      8/4

      [  -1 ]    [  =  ]  [     ]     [  SV ]  [     ]     [     ]  [     ]  [     ]
      [     ]    [     ]  [  S  ]     [     ]  [  T  ]     [  0  ]  [  .  ]  [     ]
      [  +1 ]    [ -()-]  [     ]     [  SI ]  [     ]     [     ]  [     ]  [     ]
Key   1/5        2/5      3/5         4/5      5/5         6/5      7/5      8/5

      [     ]    [  T  ]  [     ]     [ *** ]  [     ]     [   / ]  [ / \ ]  [     ]
      [SHIFT]    [     ]  [  R  ]     [     ]  [  Z  ]     [ /// ]  [  |  ]  [SHIFT]
      [     ]    [  L  ]  [     ]     [  ZR ]  [     ]     [ /   ]  [ \ / ]  [     ]
Key   1/6        2/6      3/6         4/6      5/6         6/6      7/6      8/6

Key no.   Key colour   German                              English

Function keys: 
1/1       orange       Aufruf der Grundmaske			   Access start mask
                       Funktionsanwah1                     select function
					   Abbruch der laufenden Funktion      interrupt currently running function

1/2       orange       Suchlauf                            scan

7/6       red          Übernahmetaste                      Enter key

6/6       orange       Löschen des Eingabefeldes           clear input field

5/1       red          Signalzustandsanzeige               signal status indicator

2/1       orange       Eingabe oder Korrektur              finish entry or correction and switch to output
					   abschließen und umschalten
					   auf Ausgabe
					   Shift: Korrektur einleiten          Shift: begin correction
					   
3/1       brown        Einfügen einer Anweisung            insert command
                       Shift: Löschen einer Anweisung      Shift: delete command

4/1       orange       Eingabe einleiten oder Einfügen     start input or start inserting a network
					   eines Netzwerkes einleiten
					   Shift: Löschen eines Netzwerkes     Shift: delete a network
					   
Positioning keys: 
1/3       yellow       Anwahl Programmende                 select end of program 
                       Shift: Anwahl Programmanfang        Shift: select beginning of program

1/4       yellow       Anwahl nächstes Netzwerk            select next network 
                       Shift: Anwahl vorhergehendes        Shift: select previous network
					   Netzwerk

1/5       yellow       Anwahl nächste Anweisung            select next command
                       Shift: Anwahl vorhergehende         Shift: select previous command
					   Anweisung

Keys for entering a program: 
AWL mode: Anweisungsliste = command list - KOP mode: Kontaktplan = ladder diagram

2/2       grey         AWL: Verknüpfung nach UND           AND link
                       KOP: Reihenschaltung eines          connect a contact in series
					        Kontaktes
							
2/3       grey         AWL: Verknüpfung nach ODER          OR link
                       KOP: einzelner Parallelkontakt      single parallel contact
					   
3/2       grey         AWL: Abfrage auf "Null"             check for zero
                       KOP: Kennzeichnung von Öffnern      mark normally open switches

3/3       grey         AWL: ODER-Verknüpfung von UND-      OR link of AND functions
                            Funktionen
					   KOP: Rückführung zum Beginn der     return to the beginning of a parallel branch
                            Parallelverzweigung
							
2/4       grey         AWL: UND-Verknüpfung von Klammer-   AND link of bracketed expressions
                            ausdrücken
					   KOP: Beginn einer Parallel-         beginning of a parallel branch
					        verzweigung
							
3/4       grey         AWL: Klammer zu                     close bracket
                       KOP: Abschluss einer Parallel-      end of a parallel branch
					        verzweigung
							
2/5       grey         AWL: Zuweisung                      allocation
                       KOP: Relais/Schütz                  relay/connector
					   
3/5       grey         AWL: Setzen                         set
                       KOP: Relais/Schütz speichernd       switch on and save relay/connector
					        einschalten
							
3/6       grey	       AWL: Rücksetzen                     reset
                       KOP: Relais/Schütz speichernd       switch off and save relay/connector
					        ausschalten
							
2/6       grey         Laden                               load
                       Shift: Transferieren                Shift: tranfer
					   
4/5       grey         Starten einer Zeit als Impuls       starting a timer as an impulse
                       Shift: Starten einer Zeit als       Shift: starting a timer as a prolongued impulse
					   verlängerter Impuls
					   
4/6		  grey         Zählen rückwärts                    count backwards
                       Shift: Segmentende                  Shift: end of segment

4/4       grey         Bausteinende bedingt                conditional end of block
                       Shift: Bausteinende absolut         Shift: absolute end of block
					   
5/2       light grey   Operandenkennzeichen für Eingang    operand marker for input

5/3       light grey   Operandenkennzeichen für Ausgang    operand marker for output

5/4       light grey   Operandenkennzeichen für Merker     operand marker for reminder

5/5       light grey   Operandenkennzeichen für Zeit       operand marker for timer

5/6       light grey   Operandenkennzeichen für Zähler     operand marker for counter

4/2       light grey   Operandenkennzeichen für            operand marker for peripheral
                       Peripherie

4/5       light grey   Operandenkennzeichen für Byte       operand marker for byte (only
                       (nur in Verbindung mit P, A         in connection with P, A or M
					   oder M)
					   Shift: NOP eingeben                 Shift: enter NOP
					   
Digit keys:
Die Ziffern-Tasten werden zur Vorgabe der Parameter und der Adressen sowie zur Anwahl der Funktionen über Masken verwendet.					   
The digit keys are used to enter parameters and addresses, as well as selecting functions in forms.



					   
Blank keys: 
6/1       brown
8/6       brown


       
-*/


/* X0: Brk     Enter CmdIns  NetIns SigState  ? */
	PORT_START("X0")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Break") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(UCHAR_MAMEKEY(ENTER))
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Cmd Ins") PORT_CODE(KEYCODE_INSERT) PORT_CHAR(UCHAR_MAMEKEY(INSERT))
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Net Ins") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Sig State") PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N/A") PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT(0xC0, IP_ACTIVE_LOW, IPT_UNUSED)

/* X1: Srch    U     N       P      E         7   8   9 */
	PORT_START("X1")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Search") PORT_CODE(KEYCODE_B) PORT_CHAR('S')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E) PORT_CHAR('P')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7) PORT_CHAR('7')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8) PORT_CHAR('8')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9) PORT_CHAR('9')

/* X2: GtoEnd  Or    OrAnd   B      A         4   5   6 */
	PORT_START("X2")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Goto End") PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Or") PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Or And") PORT_CHAR('#')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4) PORT_CHAR('4')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5) PORT_CHAR('5')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6) PORT_CHAR('6')

/* X3: ?       (     )       BEB/BE M         1   2   3 */
	PORT_START("X3")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("?") PORT_CHAR('?')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("(") PORT_CHAR('(')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(")") PORT_CHAR(')')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("BEB BE") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1) PORT_CHAR('1')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2) PORT_CHAR('2')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3) PORT_CHAR('3')

/* X4: +/-     <>/=  S       SV/SI  T         0   .   ? */
	PORT_START("X4")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("+/-") PORT_CODE(KEYCODE_PLUS_PAD) PORT_CHAR('+')
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("<>/=") PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("SI/SV") PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP) PORT_CHAR('.')
	PORT_BIT(0x80, IP_ACTIVE_LOW, IPT_UNUSED)

/* X5: LShft   T/L   R       ZR / *   Z         Opz Del RShift */
	PORT_START("X5")
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Left Shift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("T/L") PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("ZR/*") PORT_CODE(KEYCODE_ASTERISK) PORT_CHAR('*')
	PORT_BIT(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Op Cntr") PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_DEL) PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD) PORT_NAME("Right Shift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_2)

INPUT_PORTS_END

void pg631_state::machine_reset()
{
		// m_ramio->reset();
}

void pg631_state::machine_start()
{
}

void pg631_state::pg631(machine_config &config)
{
	/* basic machine hardware */
	I8085A(config, m_maincpu, XTAL(2'457'600));
	m_maincpu->set_addrmap(AS_PROGRAM, &pg631_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &pg631_state::io_map);

	// I8155(config, m_ramio, 6.144_MHz_XTAL / 2); // Basic RAM (A16)
	// m_ramio->out_to_callback().set_inputline(m_maincpu, I8085_TRAP_LINE);

    screen_device &screen(SCREEN(config, "screen", SCREEN_TYPE_RASTER));
    /* pixel clock   11.45664 (45.8304 / 4 = 11.4576 MHz)
     * total pixel clocks per line incl. blanking, 612 (18.720 kHz per line)
     * index of first pixed after horiz blank, 0
     * Index of first pixel in horzontal blanking period after visible pixels. 480
     * Total lines per frame, including vertical blanking period. 264   312 (60 Hz per screen)
     * Index of first visible line after vertical blanking period ends. 0
     * Index of first line in vertical blanking period after visible lines. 300
     *
     * max 80 x 25, 6 x 12 chars = 480 pixels per line, 300 lines per screen
     *
     *         screen_device &set_raw(u32 pixclock, u16 htotal, u16 hbend, u16 hbstart, u16 vtotal, u16 vbend, u16 vbstart)
     *
     */
    screen.set_raw(45.8304_MHz_XTAL/4, 800, 0, 666, 320, 0, 300);
    GFXDECODE(config, "gfxdecode", "palette", gfx_pg631);
    screen.set_screen_update(FUNC(pg631_state::screen_update));
    screen.set_palette("palette");
    PALETTE(config, "palette", palette_device::MONOCHROME);

	i8279_device &kbdc(I8279(config, "i8279", 4.9152_MHz_XTAL / 2)); // based on divider
	kbdc.out_sl_callback().set(FUNC(pg631_state::scanlines_w));    // scan SL lines
	kbdc.in_rl_callback().set(FUNC(pg631_state::kbd_r));           // kbd RL lines
	kbdc.in_shift_callback().set_constant(1);                       // Shift key
	kbdc.in_ctrl_callback().set_constant(1);
}

/* ROM definition */
ROM_START( pg631 )
	ROM_REGION( 0x8000, "maincpu", 0 ) 
	ROM_LOAD( "cpu_21.bin",   0x0000, 0x0800, CRC(b21b5b18) SHA1(263e4044a67ac0f8b158bbe6ef29275d640d294c) )
	ROM_LOAD( "cpu_24.bin",   0x0800, 0x0800, CRC(5522e262) SHA1(599a2dd1c894c6d1df9c5fdbd60514665eff187c) )
	ROM_LOAD( "cpu_26.bin",   0x1000, 0x0800, CRC(fda5a315) SHA1(7879a757c81b0f7c39f3fad5f7c74046d329686c) )
	ROM_LOAD( "cpu_29.bin",   0x1800, 0x0800, CRC(1fdcc96a) SHA1(426c42004846a0131e02046d11999f2560b45d8a) )
	ROM_LOAD( "crt_08.bin",   0x2000, 0x0800, CRC(fba0c206) SHA1(bd454ce5a3d6b028feafa506c5b3d64629a4a5a0) )
	ROM_LOAD( "crt_09.bin",   0x2800, 0x0800, CRC(dd5a7b45) SHA1(6e2394c571f3af4ede6a05e801945b3b424b0453) )
	ROM_LOAD( "crt_10.bin",   0x3000, 0x0800, CRC(1a6df4bd) SHA1(05819e21d089a131c56c39a3f3774b1b0956c969) )
	ROM_LOAD( "crt_11.bin",   0x3800, 0x0800, CRC(602a730d) SHA1(32cb168d519a6a422cfbc0b221497eca6ca5ef06) )
	ROM_LOAD( "cpu_36.bin",   0x4000, 0x0800, CRC(5d5d2b13) SHA1(9414c27be23f0d311edb0e1876467910671cdc4d) )
	ROM_LOAD( "cpu_39.bin",   0x4800, 0x0800, CRC(1de24a48) SHA1(fe636707c816de5d00c9deab04abde2cfef0a546) )
	ROM_LOAD( "cpu_44.bin",   0x5000, 0x0800, CRC(2ddf292e) SHA1(9b6247ac6696856333419c8e97e6d14e7724e6d3) )
	ROM_LOAD( "cpu_51.bin",   0x5800, 0x0800, CRC(9e7b24a8) SHA1(8f1b799053948566bb7a072ce6848d7f72f82296) )
	ROM_LOAD( "crt_12.bin",   0x6000, 0x0800, CRC(ffed89b9) SHA1(af57fcfdc9c5fbc806949aa6e7871cc7a0dd0850) )
	ROM_LOAD( "crt_13.bin",   0x6800, 0x0800, CRC(7a4b973a) SHA1(557c910ab7e6f28b8c5e8e21fa7c3d09bba50803) )
	ROM_LOAD( "crt_14.bin",   0x7000, 0x0800, CRC(2af3390e) SHA1(936e72e485356192697af977fad901bf9d2d71e6) )
	ROM_LOAD( "crt_30.bin",   0x7800, 0x0800, CRC(123ac690) SHA1(9059c4c7aa541dc01426c8f16706b14bede2e295) )
	
	ROM_REGION (0x800, "chargen", 0 )
	ROM_LOAD( "chargen.bin",  0x0000, 0x0800, CRC(5cbf0e9f) SHA1(030ed2a5c15cecacfa992cd73fe886de9c8d8412) )
	

ROM_END

/* Driver */

//    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT  CLASS        INIT        COMPANY    FULLNAME  FLAGS
COMP( 198?, pg631, 0,      0,      pg631,   pg631, pg631_state, empty_init, "Siemens", "Programmiergerät PG 631",  MACHINE_IS_SKELETON )
