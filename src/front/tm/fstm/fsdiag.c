
/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>		/* 6-x_PC_80x86 */
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ui.h>
#include	<adf.h>
#include	<fedml.h>
#include	<fstm.h>
#include	<fmt.h>
#include	<frame.h>
#include	<frsctblk.h>
#include	<ermf.h>
#include        <stdarg.h>


/**
** Name:	FSdiag
**
** Description:
**
** History:
**	31-jul-89 (sylviap)	
**		Took out of fstm.c and created fsdiag.c.
**		This is necessary so ReportWriter could use the FS routines
**		for scrollable output.
**	25-jan-90 (teresal)
**		Modified to use location buffer for LOfroms call.
**      03-oct-1996 (rodjo04)
**              Changed "ermf.h" to <ermf.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      18-03-2003 (wanfr01)
**          Bug 108728, INGCBT 431
**          Corrected syntax for variable length arguments to prevent
**          stack corruption.
**	27-mar-2004 (drivi01)
**	   Changed the name of the globaldef debug to fs_debug,
**	   to minimize possible conflicts of varaibles.
**/


GLOBALDEF ADF_CB	*FS_adfscb;
GLOBALDEF char		*fs_debug=NULL;

VOID
FSdiag (char *str, ...)
{
	static FILE	*fp = NULL;
        i4	a1, a2, a3, a4, a5;
        va_list ap;

	LOCATION	loc;
	char		locbuf[MAX_LOC+1];

	if ( fs_debug == NULL || *fs_debug == EOS )
		return;

	va_start (ap, str);
# ifdef FT3270
	FTdiag(str, a1, a2, a3, a4, a5);
# else
	if ( fp == NULL )
	{
		STcopy(fs_debug, locbuf);
		LOfroms(PATH&FILENAME, locbuf, &loc);
		if (SIopen(&loc, ERx("w"), &fp) != OK)
		{
		    IIUGerr(E_MF0023_Couldn_t_open_debug_f, UG_ERR_ERROR, 0);
		    return;
		}
		SIfprintf(fp, ERx("Start of Debug file\n"));
		SIflush(fp);
	}

	SIdofrmt(PUTC, fp, str, ap);
	va_end( ap );
	SIflush(fp);
# endif /* FT3270 */
}
