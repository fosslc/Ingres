/*
**	Copyright (c) 1983, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<si.h>
# include	<st.h>
# include	<stdarg.h>

/*
**	STprintf - C sprintf
**
**	Like the function 'sprintf' of the C language,
**	but return buf.
**
**	History:
**		2-15-83   - Written (jen)
**		3-8-83	  made p1 not an register var so it would
**			  compile under unix and typed the routine (cfr)
**	29-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Use xCL_017_UNSIGNED_IOBUF_PTR.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-jun-95 (emmag)
**	    Desktop porting changes.  SIdofrmt takes an extra flag on NT.
**	02-may-95 (morayf)
**	    adding "bconnell" change of 27-nov-92 for odt_us5 to
**	    initialise FILE structure since SCO putc macro is apparently
**	    different than most others.
**	30-may-95 (tutto01, canor01)
**	    Changed interface to SIdofrmt.
**	04-jan-95 (wadag01)
**	    Removed # define CHANGE_TO_XPG4_PLUS added by morayf for sos_us5.
**	    Instead "-a ods30" was added to CCMACH in mkdefault.sh.
**	    This option causes on SCO to include ods_30_compat/stdio.h instead
**	    of the default stdio.h (workaround for __ptr syntax errors on SCO).
**	28-oct-1997 (canor01, by kosma01)
**	    Redo variable-arg function definitions for STprintf(), matching
**	    prototype and removing NT dependency.
**	19-feb-1998 (toumi01)
**	    For Linux (lnx_us5) for FILE structure:
**	    define FILE_PTR as _IO_write_ptr, else define as _ptr and
**	    define FILE_WRITE_END as _IO_write_end, else define as _wend.
**	    For Linux (lnx_us5) FILE struture init, set write-end rather
**	    than (non-existant) count field.
**	04-jan-1999 (popri01)
**	    Another variation in FILE structure member names - this
**	    time on Unixware (usl_us5), due to UNIX 95 compliance
**	    on Unixware 7.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	06-oct-1999 (toumi01)
**	    Change Linux config string from lnx_us5 to int_lnx.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-apr-1999 (hanch04)
**	    Make STprintf generic for all platforms.
*/

char *
STprintf( char *buffer, const char *fmt, ...)
{
	va_list	ap;

	va_start( ap, fmt );

	SIdofrmt(CMCPYCHAR, buffer, fmt, ap);

	va_end( ap );

	return (buffer);
}
