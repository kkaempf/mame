// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller, Robbbert, kkaempf
//***************************************************************************

#include "emu.h"
#include "includes/eg3200.h"
#include "screen.h"

/* Bit assignment for "m_mode"
    d1 7 or 8 bit video (1=requires 7-bit, 0=don't care)
    d0 80/64 or 40/32 characters per line (1=32) */


/* 7 or 8-bit video, 32/64 characters per line = eg3200
   384 horiz * 192 vert pixels */
uint32_t eg3200_state::screen_update_eg3200(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	uint8_t y,ra,chr,gfx,gfxbit;
	uint16_t sy=0,ma=0,x;
	uint8_t cols = BIT(m_mode, 0) ? 32 : 64;
	uint8_t skip = BIT(m_mode, 0) ? 2 : 1;

	if (m_mode != m_size_store)
	{
		m_size_store = m_mode;
		screen.set_visible_area(0, cols*6-1, 0, 16*12-1);
	}
#if 0
    unsigned char *ptr = m_p_videoram;
    unsigned char *lptr = m_p_videoram;
    int size = 0x400;
    int count = 0;
    long start = 0;
    char hexdump[512];
    char *hptr = hexdump;

    while (size-- > 0)
    {
	if ((count % 64) == 0) {
            hptr = hexdump;
	    sprintf(hptr, "  %08lx:", start);
            hptr += 10;
        }
	sprintf(hptr, " %02x", *ptr++); hptr += 3;
	count++;
	start++;
	if (size == 0)
	{
	    while ((count % 64) != 0)
	    {
		sprintf(hptr, "   "); hptr += 3;
		count++;
	    }
	}
	if ((count % 64) == 0)
	{
	    sprintf(hptr, " "); hptr += 1;
	    while (lptr < ptr)
	    {
	        unsigned char c = ((*lptr&0x7f) < 32)?'.':(*lptr & 0x7f);
		sprintf(hptr, "%c", c); hptr += 1;
		lptr++;
	    }
	    logerror("%s\n", hexdump); hptr = hexdump;
	}
    }
    if ((count % 64) != 0) {
        logerror("%s\n", hexdump);
    }
#endif
        /* line counter */
	for (y = 0; y < 16; y++)
        {       /* scanline counter, 16 * 12 = 192 */
		for (ra = 0; ra < 12; ra++)
		{
			uint16_t *p = &bitmap.pix16(sy++);
                        /* charater column counter, 6 pixels per char, 6 * 64 = 384*/
			for (x = ma; x < ma + 64; x+=skip)
			{
				chr = m_p_videoram[x];
				if (chr & 0x80) /* block 'graphics' */
				{
					gfxbit = (ra & 0x0c)>>1;
					/* Display one line of a lores character (6 pixels) */
					*p++ = BIT(chr, gfxbit);
					*p++ = BIT(chr, gfxbit);
					*p++ = BIT(chr, gfxbit);
					gfxbit++;
					*p++ = BIT(chr, gfxbit);
					*p++ = BIT(chr, gfxbit);
					*p++ = BIT(chr, gfxbit);
				}
				else /* character */
				{
					if (BIT(m_mode, 1) & (chr < 32)) chr+=64;
                                        unsigned int addr = 0;
					// if  , ; g j p q y   lower the descender
					if ((chr==0x2c)||(chr==0x3b)||(chr==0x67)||(chr==0x6a)||(chr==0x70)||(chr==0x71)||(chr==0x79))
					{
						if ((ra < 9) && (ra > 0)) {
                                                        addr = (chr<<4) | (ra-1);
							gfx = m_p_chargen[addr];
                                                }
						else
							gfx = 0;
					}
					else
					{
						if (ra < 8) {
                                                        addr = (chr<<4) | ra;
							gfx = m_p_chargen[addr];
                                                }
						else
							gfx = 0;
					}
					if (chr == 0x30) {
					  logerror("line %d, column %d, chr %c, ra %d, addr %04x, gfx %02x\n", y, x, chr, ra, addr, gfx);
					}
					/* Display a scanline of a character (6 pixels) */
					*p++ = BIT(gfx, 7);
					*p++ = BIT(gfx, 6);
					*p++ = BIT(gfx, 5);
					*p++ = BIT(gfx, 4);
					*p++ = BIT(gfx, 3);
					*p++ = BIT(gfx, 2);
				}
			}
		}
		ma+=64;
	}
	return 0;
}

