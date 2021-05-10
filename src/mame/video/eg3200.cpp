// license:BSD-3-Clause
// copyright-holders:Juergen Buchmueller, Robbbert, kkaempf
//***************************************************************************

#include "emu.h"
#include "includes/eg3200.h"
#include "screen.h"

/* Bit assignment for "m_vidmode"
    d1 7 or 8 bit video (1=requires 7-bit, 0=don't care)
    d0 80/64 or 40/32 characters per line (1=32) */


/* 7 or 8-bit video, 32/64 characters per line = eg3200
   384 horiz * 192 vert pixels */
uint32_t eg3200_state::screen_update_eg3200(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	uint8_t y,ra,chr,gfx,gfxbit;
	uint16_t sy=0,ma=0,x;
	uint8_t cols;
        uint8_t rows;

        if (m_vidmode == 1) {
            rows = 24;
            cols = 80;
        }
        else if (m_vidmode == 2) {
            rows = 25;
            cols = 80;
        }
        else {
            rows = 16;
            cols = 64;
        }
	if (m_vidmode != m_size_store)
	{
		m_size_store = m_vidmode;
		screen.set_visible_area(0, cols*6-1, 0, rows*12-1); /* charater 6 x 12 */
	}
        /* line counter */
	for (y = 0; y < rows; y++)
        {       /* scanline counter, 16 * 12 = 192 */
		for (ra = 0; ra < 12; ra++)
		{
			uint16_t *p = &bitmap.pix(sy++);
                        /* charater column counter, 6 pixels per char, 6 * 64 = 384, 6 * 80 = 480 */
			for (x = ma; x < ma + cols; x++)
			{
                                if (x < 0x400)
				        chr = m_mem_video0[x];
                                else
				        chr = m_mem_video1[x-0x400];

                                if ((chr & 0x80) && (m_vidinv = 0)) /* block 'graphics' */
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
                                        uint8_t inverted = ((chr & 0x80) == 0x80)?0x01:0x00;
                                        chr = chr & 0x7f;
                                        if (ra > 0 && ra < 10)
                                                gfx = m_p_chargen[(chr<<4) | (ra-1)];
					else
						gfx = inverted;

					/* Display a scanline of a character (6 pixels) */
					*p++ = BIT(gfx, 7) ^ inverted;
					*p++ = BIT(gfx, 6) ^ inverted;
					*p++ = BIT(gfx, 5) ^ inverted;
					*p++ = BIT(gfx, 4) ^ inverted;
					*p++ = BIT(gfx, 3) ^ inverted;
					*p++ = BIT(gfx, 2) ^ inverted;
				}
			}
		}
		ma += cols;
	}
	return 0;
}

