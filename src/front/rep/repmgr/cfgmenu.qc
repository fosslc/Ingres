/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/
# include <compat.h>
# include <st.h>
# include <er.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <erfe.h>
# include <uf.h>
# include <ug.h>
# include "errm.h"

/**
** Name:	cfgmenu.qc - configuration menu
**
** Description:
**	Defines
**		cfg_menu	- configuration menu
**
** History:
**	16-dec-96 (joea)
**		Created based on cfgmenu.osq in replicator library.
**	11-dec-97 (joea)
**		Remove BuildServer menuitem.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

GLOBALREF
##char	RMdb_display[];


static bool	form_initd;
static const
##char	*form_name = ERx("rmconfig_menu"),
##	*tf_name = ERx("iitf"),
##	*fcmd = ERx("cmd"),
##	*fexpl = ERx("explanation");


/*{
** Name:	cfg_menu	- configuration menu
**
** Description:
**	Configure Replicator
**
** Inputs:
**	none
**
** Outputs:
**	none
**
** Returns:
**	0	- normal return
**	1	- exit from RepMgr
*/
STATUS
cfg_menu()
##{
##	char	menu_choice[81];
	STATUS	retval = 0;

	if (!form_initd)
	{
		if (IIUFgtfGetForm(IIUFlcfLocateForm(), form_name) != OK)
		{
			IIUGerr(E_RM0006_Get_form_failed, UG_ERR_ERROR, 1,
				form_name);
			return (FAIL);
		}
		form_initd = TRUE;
	}
##	inittable form_name tf_name read

##	loadtable form_name tf_name (fcmd = ERget(F_RM000D_Databases),
##		fexpl = ERget(F_RM000E_Databases_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM000F_CDDS),
##		fexpl = ERget(F_RM0010_CDDS_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM0011_Tables),
##		fexpl = ERget(F_RM0012_Tables_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM0015_MoveConfig),
##		fexpl = ERget(F_RM0016_MoveConfig_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM0017_Activate),
##		fexpl = ERget(F_RM0018_Activate_expl))
##	loadtable form_name tf_name (fcmd = ERget(F_RM0019_MailList),
##		fexpl = ERget(F_RM001A_MailList_expl))
##	scroll form_name tf_name to 1

##	display form_name read
##	initialize (db = RMdb_display)
##	activate menuitem ERget(FE_Select)
##		(expl = ERget(F_FE0108_ExplSelect)), FRSkey4
##	{
##		getrow form_name tf_name (menu_choice = fcmd)
		if (STequal(menu_choice, ERget(F_RM000D_Databases)))
			db_summ();
		else if (STequal(menu_choice, ERget(F_RM000F_CDDS)))
			cdds_summ();
		else if (STequal(menu_choice, ERget(F_RM0011_Tables)))
			tbl_summ();
		else if (STequal(menu_choice, ERget(F_RM0015_MoveConfig)))
			move_config();
		else if (STequal(menu_choice, ERget(F_RM0017_Activate)))
			chgrec_activate();
		else if (STequal(menu_choice, ERget(F_RM0019_MailList)))
			mail_list();
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
		FEhelp(ERx("rmcfgmen.hlp"), ERget(F_RM001B_Config_Rep));
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
