/*
** Copyright (c) 1986, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<si.h>
#include	<ex.h>
#include	<lo.h>
#include	<er.h>
# include	<gl.h>
# include	<iicommon.h>
#include	<fe.h>

/*{
** Name:    FEcopyright() -	Display RTI Copyright Notice Utility.
**
** Description:
**	Displays a copyright notice on the screen using the input FE name
**	and original date of copyright.  If the Forms System is active,
**	the copyright is displayed at the bottom of the screen.  Otherwise,
**	it is simply printed out on the standard error output.
**
** Input:
**	cr_fename	{char *}  The name of the product for which the
**				copyright notice applies.
**	cr_fedate	{char *}  The orginal year of copyright (a string.)
**
** History:
**	Revision 6.0
**	04-apr-1987 (peter) Changed FEmsg_routine to IIugmfa_msg_function.
**	12-nov-1986 (yamamoto) Modified for STrindex.
**
**	Revision 40.10  86/04/25  12:17:44  peter
**	Add flush to STDERR message.
**
**	Revision 40.9  86/04/17  10:21:44  wong
**	Output "\r\n" in non-forms case only.
**
**	Revision 40.6  86/02/27  21:48:33  wong
**	Generalized to use forms message routine pointer
**	when Forms System is active.
**
**	8-22-1985 (scl) - If the year of creation and the current
**			  year are the same then just display one
**
**	3-12-1985 (rlk) - First written.
**	06-nov-1989 (marian)
**		Replace "Relational Technology Inc." with "Ingres Corporation".
**	27-feb-90 (russ)
**		If odt_us5, $II_SYSTEM/ingres/odt exists, and II_ODT_COPYRIGHT
**		is not set, do not display copyright.
**	14-mar-91 (rudyw)
**		Correct the syntax of a return statement within odt_us5 section.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		Changed _Copyright to __Copyright to avoid conflict with
**		Metaware C compiler.
**	10/11/92 (dkh) - Removed readonly qualification on references
**			 declaration to Reldate.  Alpha cross compiler
**			 does not like the qualification for globalrefs.
**	15-mar-93 (fraser)
**		Cast arguments to (*IIugmfa_msg_function)().
**		For future reference, IIugmfa_msg_function is set by the
**		function IIugfrs_setting (in ugfrs.c).  Routines in
**		frame!runtime call IIugfrs_setting to set 
**		IIugmfa_msg_function to FTmessage.
**	18-nov-94 (stephenb)
**		Change copyright message to Computer Associates.
**	07-dec-95 (harpa06)
**		If an application has a copyright year later than the value of 
**		Reldate, the copyright message will read "<later year>, 
**		<value of Reldate>." Fixed it so that the copyright message 
**		will always show the 2 years in ascending order.
**	18-nov-96 (mcgem01)
**		Copyright notice should reference OpenIngres not INGRES.
**	22-nov-96 (hanch04)
**		Put back INGRES, too many diffs.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	27-arp-1999 (mcgem01)
**		Remove hard-coded reference to Ingres.
**	10-may-1999 (walro03)
**		Remove obsolete version string odt_us5.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	1-nov-2001 (stephenb)
**	    Change copyright notice to comply with new standard.
**	12-Jul-2006 (kschendel)
**	    Output \n only, not \r\n, platform lib will translate
**	    if necessary.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

GLOBALREF char	Reldate[];

GLOBALREF i4	(*IIugmfa_msg_function)();	/* Forms Message Routine Ptr */

VOID
FEcopyright (cr_fename, cr_fedate)
char	*cr_fename;
char	*cr_fedate;
{
    register char	*cur_year;
    char		cp_date[16];	  

    static const char __Copyright[] =		 
	ERx("%s %s Copyright %s Ingres Corporation");

    /*
    ** Modify Reldate from the format dd-mmm-yyyy to simply yyyy.
    */

    if ((cur_year = STrchr(Reldate, '-')) == NULL)
	EXsignal(EXFEBUG, 1, ERx("FEcopyright"));
    else
	++cur_year;

    /*
    **    only display current year (new copyright standards)
    */
    STcopy(cur_year, cp_date);
	
    if (IIugmfa_msg_function != NULL)
	(*IIugmfa_msg_function)(__Copyright, (bool) FALSE, (bool) FALSE, 
					SystemDBMSName, cr_fename, cp_date);
    else
    {
	SIfprintf(stdout, __Copyright, SystemDBMSName, cr_fename, cp_date);
	SIfprintf(stdout, ERx("\n"));
	SIflush(stdout);
    }
}
