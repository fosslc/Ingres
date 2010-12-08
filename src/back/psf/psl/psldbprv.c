/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <erclf.h>
#include    <gl.h>
#include    <me.h>
#include    <ci.h>
#include    <cv.h>
#include    <qu.h>
#include    <st.h>
#include    <cm.h>
#include    <er.h>
#include    <tm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <pc.h>
#include    <cs.h>
#include    <cui.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <qefcb.h>
#include    <qeuqcb.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <psftrmwh.h>
#include    <psldef.h>
#include    <cui.h>

/*
** Name: PSLDBPRV.C:	this file contains functions used in semantic actions
**			for GRANT/REVOKE DBPRIV and ROLE
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_DB<number>_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
** Description:
**	this file contains functions used by both SQL and QUEL grammar in
**	parsing of the GRANT/REVOKE DBPRIV/ROLE statements
**
**	Routines:
**	    psl_db1_dbp_nonkeyword 	   - actions for dbpriv keyword
**
** History:
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 psl_dp1_dbp_nonkeyword(
	PSS_SESBLK *cb,
	PSY_CB *psy_cb,
	PSQ_CB *psq_cb,
	char *nonkw);

/*
** Forward/external declarations
*/

GLOBALREF PSF_SERVBLK *Psf_srvblk;
static struct {
	char *name;
	i4 ctl_priv;
	i4 fl_priv;
	bool	need_value;
} dbprivlist[] = {
{"QUERY_IO_LIMIT", 	DBPR_QDIO_LIMIT, 	DBPR_QDIO_LIMIT, 	TRUE},
{"QUERY_ROW_LIMIT", 	DBPR_QROW_LIMIT, 	DBPR_QROW_LIMIT,	TRUE},
{"QUERY_CPU_LIMIT", 	DBPR_QCPU_LIMIT, 	DBPR_QCPU_LIMIT,	TRUE},
{"QUERY_PAGE_LIMIT", 	DBPR_QPAGE_LIMIT, 	DBPR_QPAGE_LIMIT,	TRUE},
{"QUERY_COST_LIMIT", 	DBPR_QCOST_LIMIT, 	DBPR_QCOST_LIMIT,	TRUE},
{"CREATE_TABLE",  	DBPR_TAB_CREATE, 	DBPR_TAB_CREATE, 	FALSE},
{"CREATE_PROCEDURE", 	DBPR_PROC_CREATE, 	DBPR_PROC_CREATE, 	FALSE},
{"CREATE_SEQUENCE", 	DBPR_SEQ_CREATE, 	DBPR_SEQ_CREATE, 	FALSE},
{"LOCKMODE", 		DBPR_LOCKMODE, 		DBPR_LOCKMODE, 		FALSE},
{"ACCESS",  		DBPR_ACCESS,		DBPR_ACCESS, 		FALSE},
{"UPDATE_SYSCAT", 	DBPR_UPDATE_SYSCAT, 	DBPR_UPDATE_SYSCAT,	FALSE},
{"SELECT_SYSCAT", 	DBPR_QUERY_SYSCAT, 	DBPR_QUERY_SYSCAT,	FALSE},
{"TABLE_STATISTICS", 	DBPR_TABLE_STATS, 	DBPR_TABLE_STATS,	FALSE},
{"CONNECT_TIME_LIMIT", 	DBPR_CONNECT_LIMIT, 	DBPR_CONNECT_LIMIT,	TRUE},
{"IDLE_TIME_LIMIT", 	DBPR_IDLE_LIMIT, 	DBPR_IDLE_LIMIT,	TRUE},
{"SESSION_PRIORITY", 	DBPR_PRIORITY_LIMIT, 	DBPR_PRIORITY_LIMIT,	TRUE},
{"DB_ADMIN", 		DBPR_DB_ADMIN, 		DBPR_DB_ADMIN,		FALSE},
{"TIMEOUT_ABORT",	DBPR_TIMEOUT_ABORT,     DBPR_TIMEOUT_ABORT,	FALSE},
{"NOQUERY_IO_LIMIT", 	DBPR_QDIO_LIMIT, 	0,			FALSE},
{"NOQUERY_ROW_LIMIT", 	DBPR_QROW_LIMIT, 	0,			FALSE},
{"NOQUERY_CPU_LIMIT", 	DBPR_QCPU_LIMIT, 	0,			FALSE},
{"NOQUERY_PAGE_LIMIT", 	DBPR_QPAGE_LIMIT, 	0,			FALSE},
{"NOQUERY_COST_LIMIT", 	DBPR_QCOST_LIMIT, 	0,			FALSE},
{"NOCREATE_TABLE",  	DBPR_TAB_CREATE, 	0,			FALSE},
{"NOCREATE_PROCEDURE", 	DBPR_PROC_CREATE, 	0,			FALSE},
{"NOCREATE_SEQUENCE", 	DBPR_SEQ_CREATE, 	0,			FALSE},
{"NOLOCKMODE", 		DBPR_LOCKMODE, 		0,			FALSE},
{"NOACCESS",  		DBPR_ACCESS, 		0,			FALSE},
{"NOUPDATE_SYSCAT", 	DBPR_UPDATE_SYSCAT, 	0,			FALSE},
{"NOSELECT_SYSCAT", 	DBPR_QUERY_SYSCAT, 	0,			FALSE},
{"NOTABLE_STATISTICS", 	DBPR_TABLE_STATS, 	0,			FALSE},
{"NOCONNECT_TIME_LIMIT",DBPR_CONNECT_LIMIT, 	0,			FALSE},
{"NOIDLE_TIME_LIMIT", 	DBPR_IDLE_LIMIT, 	0,			FALSE},
{"NOSESSION_PRIORITY", 	DBPR_PRIORITY_LIMIT, 	0,			FALSE},
{"NODB_ADMIN", 		DBPR_DB_ADMIN, 		0,			FALSE},
{"NOTIMEOUT_ABORT",	DBPR_TIMEOUT_ABORT,     0,			FALSE},
{NULL, 0, 0,  FALSE}
};

/* 
** Name: psl_db1_dbp_nonkeyword - process dbpriv nonkeyword
**
** Description:
**	This routine checks a dbpriv nonkeyword in a GRANT/REVOKE
**	statement, either setting dbpriv flags if its a valid
**	database grant, or saving as a role name otherwise. Later
**	processing will detect which is more appropriate
**
** Inputs:
**	cb, psy_cb,psq_cb - Standard control blocks
**
**	nonkw - the nonkeyword to be processed
**
** Outputs:
**	none
**
** History:
**	4-mar-94 (robf)
**         Created
**	14-jul-94 (robf)
**         Correct regression when statements like
**	       GRANT LOCKMODE, NOLOCKMODE ON DATABASE ...
**         were incorrectly accepted.
**	10-nov-1998 (sarjo01)
**	   Added TIMEOUT_ABORT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-sep-2000 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout().
**	3-may-02 (inkdo01)
**	    Added [NO]CREATE_SEQUENCE for sequence implementation.
*/
DB_STATUS
psl_dp1_dbp_nonkeyword(
	PSS_SESBLK  *cb,
	PSY_CB 	    *psy_cb,
	PSQ_CB      *psq_cb,
	char	    *nonkw)

{
    i4	    err_code;
    i4	    err_num = 0;
    i4	    	    priv;
    PSY_OBJ 	    *psy_obj;
    DB_STATUS	    status=E_DB_OK;

    for(priv=0; dbprivlist[priv].name; priv++)
    {
	if (STcasecmp(nonkw, dbprivlist[priv].name)==0)
	{
	    if(psq_cb->psq_mode == PSQ_GRANT && dbprivlist[priv].need_value)
	    {
		/* Need a value  */
		err_num = 6285L;
	    }
	    else if ((psy_cb->psy_ctl_dbpriv & dbprivlist[priv].ctl_priv) &&
		 (((psy_cb->psy_fl_dbpriv & dbprivlist[priv].ctl_priv) &&
			 !dbprivlist[priv].fl_priv)
		     ||
		    (!(psy_cb->psy_fl_dbpriv & dbprivlist[priv].ctl_priv) &&
			 dbprivlist[priv].fl_priv)
		  )
		  )
	    {
			err_num = 6289L;
	    }
	    else
	    {
		psy_cb->psy_ctl_dbpriv |=  dbprivlist[priv].ctl_priv;
		psy_cb->psy_fl_dbpriv  |=  dbprivlist[priv].fl_priv;
	    }
	    break;
	}
    }
    /*
    ** Check for errors in specified name
    */
    if (err_num != 0)
    {
	    (VOID) psf_error(err_num, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1, (i4) STtrmwhite(nonkw), nonkw);
	    return (E_DB_ERROR);
    }
    if ( dbprivlist[priv].name==NULL)
    {
	/*
	** Not a known dbpriv, so we assume its a 
	** role name for now, save on object list
	*/
	/* Allocate memory for an object entry */
	status = psf_malloc(cb, &cb->pss_ostream, (i4) sizeof(PSY_OBJ), 
		(PTR *) &psy_obj, &psq_cb->psq_error);
	if (status != E_DB_OK)
	{
		return (status);
	}
	(VOID) QUinsert((QUEUE *)psy_obj, (QUEUE *) &psy_cb->psy_objq);

	STmove(nonkw, ' ', sizeof(DB_NAME), (char *) &psy_obj->psy_objnm);
	/* Mark we saved a role name */
	psy_cb->psy_flags |= PSY_GR_REV_ROLE;
    }
    else
    {
	/* Mark we saved a dbpriv */
	psy_cb->psy_flags |= PSY_GR_REV_DBPRIV;
    }
    return E_DB_OK;
}

