/*
**	Copyright (c) 1983, 1998 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<cm.h>
# include	<st.h>
# include	<stdarg.h>
# include	"silocal.h"
# include	<siwinpr.h>

/**
** Name:	siprintf.c -	Formatted Print Compatibility Module.
**
** Description:
**	This file contains the definitions of the routines used to output
**	formatted strings through the SI module.  ('SIdofrmt()' is used by
**	the string formatting routines of ST ('STprintf()' and 'STzprintf()',
**	as well.)
**
**	SIprintf()	formatted print to standard output
**	SIfprintf()	formatted print to stream file
**
**	19-jun-95 (emmag)
**	    Desktop porting changes.   SI is quite different on NT.
**	30-may-95 (tutto01, canor01)
**	    Changed call to SIdofrmt to allow caller to specify method of
**	    writing characters.  This changed addressed reentrancy issues.
**	21-dec-95 (tsale01)
**	    Corrected changes made by tutto01 and canor01 to work with
**	    double byte correctly.
**	    While we are at it, fix feature where only half of a double byte
**	    character is printed when printing using precision of wrong size.
**	    Now SIdofrmt will only print complete characters. Any byte position
**	    left over (only ever 1) will be space filled.
**	16-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**	    Added 02-sep-93 (kchin)
**	    Changed type of var_args in SIdofrmt() to long.
**	19-mar-1996 (canor01)
**	    For Windows NT only: remove thread-local storage definition.
**	24-jun-96 (somsa01)
**	    For Windows NT only; removed linebuf variable, since it is not used,
**	    and changed printbuf[512] to printbuf[2048].
**	07-may-96 (emmag)
**	    Remove wn.h which is no longer required on NT.
**	09-dec-1996 (donc)
**	    Trigger OpenROAD Trace windows via a call to GetOpenRoadStyle()
**	    for NT only. Default is to work as before. 
**	27-jan-1998 (canor01)
**	    Clean up compile errors. Restore NT-specific comments only.
**	16-apr-1998 (wonst02)
**	    Do not call SIprintf from within SIfprintf--the arg list gets hosed.
**	28-apr-1999 (somsa01)
**	    Fixed calls to SIdofrmt().
**	28-Sep-2007 (drivi01)
**	    Increased printbuf in SIprintf to MAXBUF. ipsetp attempts to copy
**	    buffers that are two times larger than printbuf.
*/

static 
void  JJprintf( char *str );

#define MAXBUF		8192

VOID 
SIprintf( char    *fmt, ... )
{
	VOID SIprint_ortrace();
	va_list ap;
	char	printbuf[MAXBUF+1];     /* arbitrary long-enough length */

	va_start( ap, fmt );
	if (GetOpenRoadStyle())
	{
            SIprint_ortrace(fmt, ap);
	}
	else
	{
	    SIdofrmt(CMCPYCHAR, (void *)&printbuf, fmt, ap);
	    if (SIchar_mode)
		printf ("%s", printbuf);
	    else
		JJprintf (printbuf);
	}
	va_end( ap );
}

VOID
SIfprintf( FILE     *ioptr, char     *fmt, ... )
{
	va_list ap;

	va_start( ap, fmt );
	if (ioptr == NULL)
	{
	    if (GetOpenRoadStyle())
	    {
		VOID SIprint_ortrace();

            	SIprint_ortrace(fmt, ap);
	    }
	    else
	    {
		char	printbuf[2048];     /* arbitrary long-enough length */

	    	SIdofrmt(CMCPYCHAR, (void *)&printbuf, fmt, ap);
	    	if (SIchar_mode)
		    printf ("%s", printbuf);
	    	else
		    JJprintf (printbuf);
	    }
	}
	else
	{
		SIdofrmt(PUTC, ioptr, fmt, ap);
	}
	va_end( ap );
}

static
void
JJprintf( char *str )
{
	ATOM atom;
	HWND hWnd;
	i2 len=strlen(str);
	if (!len) 
		return;
	atom = GlobalAddAtom(str);
	hWnd = FindWindow(WCLASS, WTITLE);
	if (hWnd) 
		SendMessage(hWnd, WM_IIPRINTF, (WPARAM)len+1, (LPARAM)atom);
	return;
}

static char tmpbuf[256]; /* only used by non-threaded front ends */

char *
SIgetPrompt(void)
{
	ATOM atom=NULL;
	HWND hWnd;
	hWnd = FindWindow(WCLASS, WTITLE);
	if (hWnd) 
		atom = (ATOM) SendMessage(hWnd, WM_IIGETPROMPT, 0, 0);
	GlobalGetAtomName(atom, tmpbuf, 256);
	GlobalDeleteAtom(atom);
	return(tmpbuf);
}
