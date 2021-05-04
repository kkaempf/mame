// license:BSD-3-Clause
// copyright-holders:rfka01, K. Loy
/***************************************************************************

    Siemens PG 631 SKELETON
	
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

class pg631_state : public driver_device
{
public:
	pg631_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_rom(*this, "maincpu")
		// , m_terminal(*this, "terminal")
		{ }

	void pg631(machine_config &config);

private:
	
	void io_map(address_map &map);
	void mem_map(address_map &map);

	void machine_reset() override;
	void machine_start() override;
	required_device<cpu_device> m_maincpu;
	required_region_ptr<u8> m_rom;
	
};

void pg631_state::mem_map(address_map &map)
{
	map.unmap_value_high();
	map(0x0000, 0x7fff).rom().region("maincpu", 0);
	// map(0xd100, 0xd103).rw(m_pit, FUNC(pit8253_device::read), FUNC(pit8253_device::write));
	map(0xe000, 0xe7ff).ram(); // RAM on CPU board
	map(0xe800, 0xefff).ram(); // RAM on video board
	
}

void pg631_state::io_map(address_map &map)
{
	map.unmap_value_high();
}

/* Input ports */
static INPUT_PORTS_START( pg631 )
INPUT_PORTS_END

void pg631_state::machine_reset()
{

}

void pg631_state::machine_start()
{
}

void pg631_state::pg631(machine_config &config)
{
	/* basic machine hardware */
	I8085A(config, m_maincpu, XTAL(2'000'000));
	m_maincpu->set_addrmap(AS_PROGRAM, &pg631_state::mem_map);
	m_maincpu->set_addrmap(AS_IO, &pg631_state::io_map);


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
	
	ROM_REGION (0x800, "crt", 0 )
	ROM_LOAD( "chargen.bin",  0x0000, 0x0800, CRC(5cbf0e9f) SHA1(030ed2a5c15cecacfa992cd73fe886de9c8d8412) )
	

ROM_END

/* Driver */

//    YEAR  NAME   PARENT  COMPAT  MACHINE  INPUT  CLASS        INIT        COMPANY    FULLNAME  FLAGS
COMP( 198?, pg631, 0,      0,      pg631,   pg631, pg631_state, empty_init, "Siemens", "PG631",  MACHINE_IS_SKELETON )
