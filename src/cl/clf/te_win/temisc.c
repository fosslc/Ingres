/*
**  Copyright (c) 1983, 2003 Ingres Corporation
*/

# include       <compat.h>
# include       <cv.h>
# include       <ex.h>
# include       <te.h>
# include       <si.h>
# include       <cm.h>
# include       "telocal.h"

#ifdef i64_win
#pragma optimize("", off)
#endif


/*
**  History:
**
**      20-mar-1991 (fraser)
**          General cleanup.  Removed the following routines:
**              TEgetfd, TEgetfp
**          Moved TEunbuf to tetest.c
**		14-sep-94 (leighb)
**			Replaced with TFD version.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	12-dec-2001 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent CBF from SEGV'ing upon
**	    startup.
**	26-jun-2003 (somsa01)
**	    Corrected NO_OPTIM for i64_win.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
*/

/*TEflush
**
**      Flush buffers to terminal
**
**      Flush the write buffer to the terminal.  Normally called when
**      the buffer is full or when the application decides that the
**      screen should be updated.
**
*/
STATUS
TEflush(void)
{
	return(OK);
/*	return(SIflush(stdout));	*/
}

/*TEput
**
**      Put a character to the terminal.
**
**      char    c;
**      TEput(c);
*/
VOID
TEput(
char  c)
{
        if      ((TEok2write == OK) || (c == BELL))
        {
                putchar( c );
        }
}

/* position cursor to line and col (0 relative) */

VOID
TEposcur(
i4 line,
i4 col)
{
	COORD   coordChar;
	
	coordChar.X = col;
	coordChar.Y = line;
	SetConsoleCursorPosition(hTEconsole, coordChar);
}

/* Store termcap values in TExlate table */

static  char    TEnewcolor(char i, char a);

void
TEtermcap(
i2      rows,   /* # of rows on the screen */
i2      cols,   /* # of cols on the screen */
char *  ea,     /* string to turn off all display attributes */
char *  bo,     /* bold */
char *  us,     /* underline */
char *  zb,     /* bold underline */
char *  bl,     /* blink */
char *  zc,     /* bold blink */
char *  ze,     /* underline blink */
char *  zh,     /* bold underline blink */
char *  rv,     /* reverse */
char *  zd,     /* bold reverse */
char *  zf,     /* underline reverse */
char *  zk,     /* reverse bold underline */
char *  zg,     /* blink reverse */
char *  zj,     /* blink reverse bold */
char *  zi,     /* underline blink reverse */
char *  za,     /* blink reverse bold underline */
char *  ya,     /* color 0 */
char *  yb,     /* color 1 */
char *  yc,     /* color 2 */
char *  yd,     /* color 3 */
char *  ye,     /* color 4 */
char *  yf,     /* color 5 */
char *  yg,     /* color 6 */
char *  yh)     /* color 7 */
{
register i2      i;
         char    a;
         i4      l;
         char ** p;

        i = ((TErows != rows) || (TEcols != cols)) ? 1 : 0;
        TErows = rows;
        TEcols = cols;

        if      ((ea != NULL) && (*ea != '\0'))
        {
                return;			/* Indicate no direct screen writes */
        }

        for     (p = &bo, i = 0; ++i < 16; ++p)
        {
                if      (*p)
                {
                        if      (CVahxl(*p, &l) == OK)
                                TExlate[i] = l;
                        **p = EOS;
                }
        }
        memcpy(&TExlate[16], &TExlate[0], 16 * 7);
        for     (p = &ya, i = -1; ++i < 8; ++p)
        {
                if      (*p)
                {
			if	(strlen(*p) > 2)
				*(*p+2) = '\0';
                        if      (CVahxl(*p, &l) == OK)
                                TExlate[i * 16] = l;
                        **p = EOS;
                }
        }
        for     (i = 16; i < 128; i += 16)
        {
                if      (TExlate[i] != TExlate[0])
                {
                        for     (a = 0; ++a < 16; )
                                TExlate[i+a] = TEnewcolor(TExlate[i], a);
                }
        }
        memcpy(&TExlate[128], &TExlate[0], 128);
}

static
char
TEnewcolor(
char    i,
char    a)
{
        register unsigned char  j;

        if      (a & _RVVID)
        {
                i &= 0x77;
                j  = i;
                i  = i << 4;
                i += j >> 4;
        }
        if      (a & _CHGINT)
        {
                i = (i & 0x08) ? i & 0x77 : i | 0x08;
        }
        if      (a & _BLINK)
                i |= 0x80;
        return(i);
}
