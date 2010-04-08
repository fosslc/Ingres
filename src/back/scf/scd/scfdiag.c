/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <evset.h>
#include    <gl.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <tm.h>
#include    <st.h>
#include    <cv.h>
#include    <cs.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <scf.h>
#include    <gca.h>
#include    <qu.h>

#include    <dmccb.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <psfparse.h>

#include    <sc.h>
#include    <sc0m.h>
#include    <scc.h>
#include    <sce.h>
#include    <scs.h>
#include    <scd.h>

#include    <scfcontrol.h>

/**
**  Name: scfdiag - SCF analysis routines
**
**  Description:
**      This module contains code for getting at SCF information in core files
**      The following routines are supplied:-
**
**      ScfdiagQuery - Display current query
**
**  History:
**	22-feb-1996 - Initial port to Open Ingres
**	13-Mch-1996 (prida01)
**	    Add scd_diag as link to the cs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-dec-2003 (horda03) Bug 111657/INGSRV2677
**          ACCVIO occurs on AXP.OSF because display of
**          ics_dbname, ics_dbowner and ics_rusername are incorrect.
**          Also, need to ignore CS internal threads too.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
*/

/*{
**  Name: ScfDiagQuery - Display current query
**
**  Description:
**      Display the current query from the SCF control block
**
**  Inputs:
**      void (*output)(PTR format,...)  Routine to call with output
**      void (*error)(PTR  format,...)   Routine to call with errors
**
**  History:
**	15-Mch-1996 (prida01)
**	    If we have a query then dump as much as possible.
**      28-dec-2003 (horda03) Bug 111657/INGSRV2677
**          Ignore CS internal threads.
**          Use the base address of the char arrays, not the
**          structure they are stored in. Causes a SIGSEGV on
**          AXP.OSF.
**	16-Jun-2006 (kschendel)
**	    NULL is pointer, not null char, fix annoying compiler warnings.
*/

ScfDiagQuery(output,error,scd)
void (*output)();
void (*error)();
VOID  *scd;
{
    SCD_SCB *scd_scb = (SCD_SCB *)scd;
    char buffer[80];


    if ((scd_scb!=NULL) && (scd_scb->scb_sscb.sscb_stype == SCS_SNORMAL) &&
	(scd_scb->cs_scb.cs_thread_type != CS_INTRNL_THREAD))
    {

        if (scd_scb->scb_sscb.sscb_ics.ics_qbuf[0] != '\0')
        {
            (*output)("%s\n\n",scd_scb->scb_sscb.sscb_ics.ics_qbuf);
        }
	else
		return;

	if (scd_scb->scb_sscb.sscb_ics.ics_dbname.db_db_name[0] != '\0')
        {
	    (*output)("Database          : %20s\n",
            	scd_scb->scb_sscb.sscb_ics.ics_dbname.db_db_name);
	}
	if (scd_scb->scb_sscb.sscb_ics.ics_dbowner.db_own_name[0] != '\0')
        {
	    (*output)("Owner             : %20s\n",
            	scd_scb->scb_sscb.sscb_ics.ics_dbowner.db_own_name);
	}
	if (scd_scb->scb_sscb.sscb_ics.ics_iusername.db_own_name[0] != '\0')
	{
	    (*output)("Session User      : %20s\n",
			scd_scb->scb_sscb.sscb_ics.ics_iusername.db_own_name);
	}
	if (scd_scb->scb_sscb.sscb_ics.ics_rusername.db_own_name[0] != '\0')
	{
	    (*output)("Real User         : %20s\n",
			scd_scb->scb_sscb.sscb_ics.ics_rusername.db_own_name);
	}
	if ((scd_scb->scb_sscb.sscb_ics.ics_eusername) &&
	   (scd_scb->scb_sscb.sscb_ics.ics_eusername->db_own_name[0] != '\0'))
	{
	    (*output)("Effective Username: %20s\n",
			scd_scb->scb_sscb.sscb_ics.ics_eusername->db_own_name);
	}
	if (scd_scb->scb_sscb.sscb_ics.ics_sessid[0] != '\0')
	{
	    (*output)("Session Identifier: %20s\n", 
			scd_scb->scb_sscb.sscb_ics.ics_sessid);
	}
	if (scd_scb->scb_sscb.sscb_ics.ics_tz_name[0] != '\0')
	{
	    (*output)("Front End TZ      : %s\n",
			scd_scb->scb_sscb.sscb_ics.ics_tz_name);
	}
	(*output)("Real User Stat 0x%x Initial User Stat 0x%x \n",
		  scd_scb->scb_sscb.sscb_ics.ics_rustat,
		  scd_scb->scb_sscb.sscb_ics.ics_iustat);
	(*output)("Default User Priv 0x%x Max User Pri 0x%x \n",
		  scd_scb->scb_sscb.sscb_ics.ics_defustat,
		  scd_scb->scb_sscb.sscb_ics.ics_maxustat);
	(*output)("Req  User Stat 0x%x Real User Id 0x%x \n",
		  scd_scb->scb_sscb.sscb_ics.ics_requstat,
		  scd_scb->scb_sscb.sscb_ics.ics_ruid);
	(*output)("Query row  limit %d Query I/O  limit %d Query CPU  limit %d\n",
		scd_scb->scb_sscb.sscb_ics.ics_qrow_limit,
		scd_scb->scb_sscb.sscb_ics.ics_qdio_limit,
		scd_scb->scb_sscb.sscb_ics.ics_qcpu_limit);
	(*output)("Query page limit %d Query cost limit %d Idle time limit %d\n",
		scd_scb->scb_sscb.sscb_ics.ics_qpage_limit,
		scd_scb->scb_sscb.sscb_ics.ics_qcost_limit,
		scd_scb->scb_sscb.sscb_ics.ics_idle_limit);
	(*output)("Connect time limit %d Priority limit %d Curr Pri limit %d\n",
		scd_scb->scb_sscb.sscb_ics.ics_connect_limit,
		scd_scb->scb_sscb.sscb_ics.ics_priority_limit,
		scd_scb->scb_sscb.sscb_ics.ics_cur_priority);
	(*output)("Curr Idle limit %d Curr Conn Limit %d \n",
		scd_scb->scb_sscb.sscb_ics.ics_cur_idle_limit,
		scd_scb->scb_sscb.sscb_ics.ics_cur_connect_limit);
	TMstr(&scd_scb->scb_sscb.sscb_connect_time,buffer);
	(*output)("Time session started %s\n",
		buffer);

    }
}
/*{
**  Name: scd_diag - link from cs to rest of backend
**
**  Description:
**      Display the current query from the SCF control block
**
**  Inputs:
**      void *diaglink  	contains relevant information to obtain diags.
**
**  History:
**	13-Mch-1996 (prida01)
**	    Created.
*/

scd_diag(diag_link)
void *diag_link;
{
    DIAG_LINK *link = (DIAG_LINK *)diag_link;
    CL_ERR_DESC	sys_err;
    STATUS	status;
    PTR		file;
    SCD_SCB 	*scd_scb = (SCD_SCB *)link->scd;
    PSQ_CB	*ps_ccb  = scd_scb->scb_sscb.sscb_psccb;
    QSF_RCB	*qs_ccb  = scd_scb->scb_sscb.sscb_qsccb;
    i4		tmp_err;

    switch (link->type)
    {
	case DMF_OPEN_TABLES:
		dmf_diag_dump_tables(link->output,link->error);
		break;
	case SCF_CURR_QUERY:
		ScfDiagQuery(link->output,link->error,link->scd);
		break;
	case QSF_SELF_DIAGNOSTICS:
	case DMF_SELF_DIAGNOSTICS:
	case PSF_SELF_DIAGNOSTICS:
		/* Close current file to flush buffer */
		NMgtAt("II_DBMS_LOG",&file);
		status = TRset_file(TR_F_CLOSE, file, STlength(file), &sys_err);
		if ((status = TRset_file(TR_F_OPEN, link->filename, 
				STlength(link->filename), &sys_err)) != OK)
    		{
        		return (FAIL);
    		}
		switch (link->type)
		{
			case DMF_SELF_DIAGNOSTICS:
					dmf_diag_dmp_pool(1);
					break;
			case QSF_SELF_DIAGNOSTICS:
					qsd_lru_dump();
					qsf_call(QSD_OBJ_DUMP,qs_ccb);
					qsf_call(QSD_OBQ_DUMP,qs_ccb);		
					qsf_call(QSD_BO_DUMP,qs_ccb);		
					qsf_call(QSD_BOQ_DUMP,qs_ccb);		
					break;
			case PSF_SELF_DIAGNOSTICS:
			{
    					tmp_err = ps_ccb->psq_error.err_code;
					psq_call(PSQ_PRMDUMP,ps_ccb,
						scd_scb->scb_sscb.sscb_psscb);
					psq_call(PSQ_SESDUMP,ps_ccb,
						scd_scb->scb_sscb.sscb_psscb);
					ps_ccb->psq_error.err_code = tmp_err;
					break;
			}
		}
		if ((status = TRset_file(TR_F_CLOSE, link->filename, 
				STlength(link->filename), &sys_err)) != OK)
    		{
        		return (FAIL);
    		}
		if (file != NULL || *file != EOS)
		{
		    status=TRset_file(TR_F_APPEND,file,STlength(file),&sys_err);
		}

		break;

		

	default:break;
    }
	
}
