/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <nm.h>
# include <lo.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <erfe.h>
# include <uf.h>
# include <ug.h>
# include "errm.h"
# include "collisn.h"

/**
** Name:	reports.qc - reports menu
**
** Description:
**	Defines
**		reports		- reports menu
**
** History:
**	16-dec-96 (joea)
**		Created based on reports.osq in replicator library.
**	14-jan-97 (joea)
**		Change in fast message numbers to close gap.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

GLOBALREF i2	RMlocal_db_no;
GLOBALREF
##char	RMdb_display[];


static bool	form_initd;
static const
##char	*form_name = ERx("rmreport_menu"),
##	*tf_name = ERx("iitf"),
##	*fcmd = ERx("cmd"),
##	*fexpl = ERx("explanation");


/*{
** Name:	reports	- reports menu
**
** Description:
**	Resolver and Other Reports
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	OK	- normal return
**	1	- exit from RepMgr
*/
STATUS
reports()
##{
	STATUS	err;
##	char	msg[1024];
##	char	choice[81];
	STATUS	retval = OK;
	char	*p;
	char	file_name[MAX_LOC+1];
	LOCATION	loc;

	if (!form_initd)
	{
		if (IIUFgtfGetForm(IIUFlcfLocateForm(), form_name) != OK)
		{
			IIUGerr(E_RM0006_Get_form_failed, UG_ERR_ERROR, 1,
				form_name);
			return (retval);
		}
		form_initd = TRUE;
	}

##	inittable form_name tf_name read
##	loadtable form_name tf_name (fcmd = ERget(F_RM0097_Collision),
##		fexpl = ERget(F_RM0098_Collision_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM0099_Integrity),
##		fexpl = ERget(F_RM009A_Integrity_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM009B_DistribConfig),
##		fexpl = ERget(F_RM009C_DistribConfig_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM000D_Databases),
##		fexpl = ERget(F_RM009D_Databases_rpt_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM000F_CDDS),
##		fexpl = ERget(F_RM009E_Cdds_rpt_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM0011_Tables),
##		fexpl = ERget(F_RM009F_Tables_rpt_expl))
##	scroll form_name tf_name to 1

##	display form_name read
##	initialize (db = RMdb_display)
##	activate menuitem ERget(FE_Select)
##		(expl = ERget(F_FE0108_ExplSelect)), FRSkey4
##	{
		NMloc(TEMP, PATH, NULL, &loc);

##		getrow form_name tf_name (choice = fcmd)
		if (STequal(choice, ERget(F_RM0097_Collision)))
		{
			LOfstfile(ERx("rpcollsn.rpt"), &loc);
			LOtos(&loc, &p);
			STcopy(p, file_name);
			IIUGmsg(ERget(F_RM00A1_Compar_distrib_queue), FALSE,
				0);
			if (queue_collision(RMlocal_db_no, file_name) != OK)
			{
				IIUGerr(E_RM0069_Error_collision_rpt,
					UG_ERR_ERROR, 0);
##				resume
			}
		}
		else if (STequal(choice, ERget(F_RM0099_Integrity)))
		{
			LOfstfile(ERx("rptblint.rpt"), &loc);
			LOtos(&loc, &p);
			STcopy(p, file_name);
			err = tbl_integ(file_name);
			if (err == 1)		/* end/cancel */
			{
##				resume
			}
			else if (err)		/* error */
			{
				IIUGerr(E_RM006A_Error_tbl_integ_rpt,
					UG_ERR_ERROR, 0);
##				resume
			}
		}
		else if (STequal(choice, ERget(F_RM009B_DistribConfig)))
		{
			LOfstfile(ERx("rpckdcfg.rpt"), &loc);
			LOtos(&loc, &p);
			STcopy(p, file_name);
			if (check_distrib_config(file_name) != OK)
			{
				IIUGerr(E_RM006B_Error_cfg_check_rpt,
					UG_ERR_ERROR, 0);
##				resume
			}
		}
		else if (STequal(choice, ERget(F_RM000D_Databases)))
		{
			LOfstfile(ERx("rpdbs.rpt"), &loc);
			LOtos(&loc, &p);
			STcopy(p, file_name);
			if (databases_report(file_name) != OK)
			{
				IIUGerr(E_RM006C_Error_dbs_rpt, UG_ERR_ERROR,
					0);
##				resume
			}
		}
		else if (STequal(choice, ERget(F_RM000F_CDDS)))
		{
			LOfstfile(ERx("rppaths.rpt"), &loc);
			LOtos(&loc, &p);
			STcopy(p, file_name);
			if (paths_report(file_name) != OK)
			{
				IIUGerr(E_RM006D_Error_paths_rpt, UG_ERR_ERROR,
					0);
##				resume
			}
		}
		else if (STequal(choice, ERget(F_RM0011_Tables)))
		{
			LOfstfile(ERx("rptables.rpt"), &loc);
			LOtos(&loc, &p);
			STcopy(p, file_name);
			if (tables_report(file_name) != OK)
			{
				IIUGerr(E_RM006E_Error_tables_rpt,
					UG_ERR_ERROR, 0);
##				resume
			}
		}
		file_display(file_name);
##	}

##	activate FRSkey7 (expl = ERget(F_FE0104_ExplFind))
##	{
		FEtabfnd(form_name, tf_name);
##	}

##	activate FRSkey5 (expl = ERget(F_FE0105_ExplTop))
##	{
##		scroll form_name tf_name to 1
##	}

##	activate FRSkey6 (expl = ERget(F_FE0106_ExplBottom))
##	{
##		scroll form_name tf_name to end
##	}

##	activate menuitem ERget(FE_Help)
##		(expl = ERget(F_FE0100_ExplFrameHelp)), FRSkey1
##	{
		FEhelp(ERx("rmreport.hlp"),
			ERget(F_RM00A0_Resolver_other_rpts));
##	}

##	activate menuitem ERget(FE_End) (expl = ERget(F_FE0102_ExplEnd)),
##		FRSkey3
##	{
##		breakdisplay
##	}

##	activate menuitem ERget(FE_Quit) (expl = ERget(F_RM0008_Quit_expl)),
##		FRSkey2
##	{
		retval = 1;
##		breakdisplay
##	}

	return (retval);
##}
