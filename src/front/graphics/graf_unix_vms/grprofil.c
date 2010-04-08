/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<gropt.h>
# include	<ooclass.h>
# include	<oodefine.h>
# include	<oosymbol.h>
# include	<uigdata.h>

/**
** Name:	grprofil.qc -	Visual Graphics Options Profile Module.
**
** Description:
**	Contains routines the read and write the graphics option profile
**	for a particular user.	This file defines:
**
**	GRpr_set(opt)	set graphics option profile.
**	GRpr_fetch(opt) fetch graphics option profile.
**
** History:	
 * Revision 61.1  88/03/25  23:25:28  sylviap
 * *** empty log message ***
 * 
 * Revision 61.1  88/03/25  21:05:06  sylviap
 * *** empty log message ***
 * 
Revision 60.13  88/02/08  13:06:44  bobm
add parameter to OOsnd

Revision 60.12  87/08/05  19:15:25  bobm
ER conversion

Revision 60.11	87/07/07  13:28:35  bobm
User -> IIUIuser

Revision 60.10	87/06/22  09:26:59  peterk
eliminated unnecessary trailing argument, 0, on calls to OOsnd.

Revision 60.9  87/06/16	 13:22:17  peterk
renamed from a .qc to a .c file - file doesn't use EQUEL.

Revision 60.8  87/05/18	 10:32:49  bobm
grsymbol -> oosysmbol

Revision 60.7  87/05/18	 10:07:48  peterk
6.0 OO changes

**		Revision 40.1.5.1  86/05/21  16:24:47  peterk
**		Branch taken by peterk.
**
**		Revision 40.1  86/04/08	 19:00:36  peterk
**		change include of types.qh to use shared header GRCLASS_QH
**
**		Revision 40.0  86/03/04	 12:42:25  wong
**		Initial revision.  (Moved from "vig/catfetch.c".)
**
**	27-jan-93 ( pghosh)
**		Previous change was rejected as there was no comment. As the 
**		file does not have any proper way of putting comments, I did
**		not put any comment. The last change was:
**		Modified _flushAll to ii_flushAll.
**	28-Oct-93 (donc)
**		Cast refernces to the OOsndxxx calls to OOID. These functions
**		have been declared now as returning PTR.
**    25-Oct-2005 (hanje04)
**        Add prototype for def_profile() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**
**/

/* local prototypes */
static def_profile();

STATUS
GRpr_set (opt)
GR_OPT *opt;
{
	OOID	gopt;
	if (opt->id < 0) {
	    if ((gopt = (OOID)OOsnd(OC_GROPT, _newDb, TRUE, opt)) != nil)
	    {
		opt->id = gopt;
		return OK;
	    }
	}
	else {
	    gopt = (OOID)OOsnd(opt->id, _init, opt);
	    if ((OOID)OOsnd(gopt, ii_flushAll))
		return OK;
	}
	return FAIL;
}

VOID
GRpr_fetch (opt)
GR_OPT *opt;
{
	OOID	p;
	p = (OOID)OOsnd(OC_GROPT, _withName, IIuiUser, IIuiUser);
	if (p == nil)
	    def_profile(opt);
	else
	    (OOID)OOsnd(p, _vig, opt);
}

static
def_profile(opt)
GR_OPT *opt;
{
	opt->id = -1;
	opt->dt_trunc = 0;
	opt->lv_main = 5;
	opt->lv_edit = 2;
	opt->lv_lyr = 1;
	opt->lv_plot = 5;
	opt->o_flags = EO_TPLOT|EO_CONF|EO_EXPL;
	opt->plot_type = ERx("");
	opt->plot_loc = ERx("");
	opt->fc_char[0] = 'g';
	opt->fc_char[1] = '\0';
}
