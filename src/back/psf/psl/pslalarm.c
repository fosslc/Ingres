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
#include    <psyaudit.h>

/*
** Name: PSLALARM.C:	this file contains functions used in semantic actions
**			for CREATE/DROP SECURITY_ALARM statements. 
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_AL<number>_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
** Description:
**	this file contains functions used by the SQL grammar in
**	parsing of the CREATE/DROP SECURITY_ALARM statements
**
**	Routines:
**		psl_al1_create_alarm - Final processing of create alarm
**		psl_al2_tbl_obj_spec - Handle table object spec
**		psl_al3_db_obj_spec  - Handle database object spec
**		psl_al4_drop_alarm   - Handle drop security alarms.
**
** History:
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 psl_al1_create_alarm(
	PSS_SESBLK *cb,
	PSY_CB *psy_cb,
	PSQ_CB *psq_cb,
	i4 subject_type,
	i4 object_type);
i4 psl_al2_tbl_obj_spec(
	PSS_SESBLK *cb,
	PSY_CB *psy_cb,
	PSQ_CB *psq_cb,
	PSS_OBJ_NAME *obj_spec);
i4 psl_al3_db_obj_spec(
	PSS_SESBLK *cb,
	PSY_CB *psy_cb,
	PSQ_CB *psq_cb,
	char *objname,
	bool is_delim);
i4 psl_al4_drop_alarm(
	PSS_SESBLK *cb,
	PSY_CB *psy_cb,
	PSQ_CB *psq_cb,
	i4 object_type);

/*
** Name: psl_al1_create_alarm
**
** Description:
**	Handles the final stage of the CREATE SECURITY_ALARM statement,
**	including  generating the query text and  cleaning up
**
** Inputs:
**	cb,psy_cb,psq_cb   - Regular control blocks
**
** 	subject_type	   - Type of grantee (public, group, role, user)
**
**      object_type	   - Type of object (table, database)
**
** History:
**	6-dec-93 (robf)
**          Created
**	21-dec-93 (robf)
**          Lookup dbevent name/owner from id
**	09-jan-95 (carly01)
**		b66236 fixed E_PS04D1 alarm error message.
**	
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psq_tout(), psq_tmulti_out().
**	17-Jan-2001 (jenjo02)
**	    Supress calls to psy_secaudit() unless C2SECURE.
**	31-May-2002 (jenjo02)
**	    Test for C2SECURE misplaced, defeating privileges check.
**	15-Apr-2003 (bonro01)
**	    Added include <psyaudit.h> for prototype of psy_secaudit()
*/
DB_STATUS
psl_al1_create_alarm(
	PSS_SESBLK  *cb,
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	i4	    subject_type,
	i4	    object_type
)
{
    DB_STATUS	    status=E_DB_OK;
    PTR             piece;
    u_char	    ident_placeholder[DB_MAX_DELIMID + 1];
    char	    text[1024];
    char	    *str;
    i4	    err_code;
    i4		    str_len;
    i4  	    len;
    bool	    comma = FALSE;
    i4		    num_tbl;
    bool	    got_alarm_name=TRUE;
    int		    num_usr;
    PSY_TBL	    *psy_tbl;
    char	    *obj_name;
    i4		    obj_name_len;
    char	    *objtype;
    DB_SECALARM     *alarm= &psy_cb->psy_tuple.psy_alarm;
    PSY_USR 	    *psy_usr;
    u_char	    *subj_name;
    i4		    subj_name_len;
    u_char          delim_obj_name[DB_MAX_DELIMID];
    u_char          delim_subj_name[DB_MAX_DELIMID];
    DB_ERROR	    err_blk;

    /*
    ** Do various semantic checks by object type
    */
    if(object_type==DBOB_TABLE)
    {
	num_tbl=0;
        for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next)
	{
		num_tbl++;
	}
	/*
	** Make sure we have appropriate numbers of tables.
	*/
	if(!num_tbl)
	{
		(VOID) psf_error(E_PS04D2_ALARM_NO_OBJECT_NAMES, 
			0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 1,
			sizeof("TABLE")-1, "TABLE");
		return (E_DB_ERROR);
	}
	if(num_tbl>1)
	{
		i4 maxobj=1;
		(VOID) psf_error(E_PS04D3_ALARM_TOO_MANY_OBJS, 
			0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 2,
			sizeof("TABLE")-1, "TABLE",
			sizeof(maxobj),&maxobj);
		return (E_DB_ERROR);
	}
	/*
	** Check for valid operations
	*/
        if (psy_cb->psy_opmap & (DB_CONNECT|DB_DISCONNECT))
	{
		if (psy_cb->psy_opmap & DB_CONNECT)
			objtype="CONNECT";
		else
			objtype="DISCONNECT";

		(VOID) psf_error(E_PS04D5_ALARM_OBJ_OPER_MISMATCH, 
			0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 2,
			STlength(objtype),objtype,
			sizeof("TABLE")-1, "TABLE");
		return (E_DB_ERROR);
	}
	if(!(psy_cb->psy_opmap & (DB_RETRIEVE|DB_APPEND|DB_REPLACE|DB_DELETE)))
		psy_cb->psy_opmap |= (DB_RETRIEVE|DB_APPEND|DB_REPLACE|DB_DELETE);
	psy_cb->psy_opctl|=psy_cb->psy_opmap;
    }
    else if(object_type==DBOB_DATABASE)
    {
	/*
	** Check for valid operations
	*/
        if (psy_cb->psy_opmap & (DB_RETRIEVE|DB_APPEND|DB_REPLACE|DB_DELETE))
	{
		if (psy_cb->psy_opmap & DB_RETRIEVE)
			objtype="SELECT";
		else if (psy_cb->psy_opmap & DB_APPEND)
			objtype="INSERT";
		else if (psy_cb->psy_opmap & DB_REPLACE)
			objtype="UPDATE";
		else
			objtype="DELETE";

		(VOID) psf_error(E_PS04D5_ALARM_OBJ_OPER_MISMATCH, 
			0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 2,
			STlength(objtype),objtype,
			sizeof("DATABASE/INSTALLATION")-1, 
			"DATABASE/INSTALLATION");
		return (E_DB_ERROR);
	}
        if(!(psy_cb->psy_opmap &(DB_CONNECT|DB_DISCONNECT)))
		psy_cb->psy_opmap |= (DB_CONNECT|DB_DISCONNECT);
	psy_cb->psy_opctl|=psy_cb->psy_opmap;
    }

    if(STskipblank((char*)&alarm->dba_alarmname,
    		sizeof(alarm->dba_alarmname)))
	got_alarm_name=TRUE;
    else
	got_alarm_name=FALSE;

    alarm->dba_popset=psy_cb->psy_opmap;
    alarm->dba_popctl=psy_cb->psy_opctl;
    /*
    ** Check the number of subjects. For unnamed alarms we allow a
    ** list of subjects (grandfathered) which is split up ala
    ** permits, but for named alarms only a single subject may be
    ** specified
    */
    if(got_alarm_name)
    {
	psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;
	num_usr=0;

	if (psy_usr != (PSY_USR *) &psy_cb->psy_usrq)
	{
	  do {
	      psy_usr = (PSY_USR *) psy_usr->queue.q_next;
	      num_usr++;
	  } while (psy_usr != (PSY_USR *) &psy_cb->psy_usrq);
	}
	if(num_usr>1)
	{
	    i4  maxsubj=1;
	    (VOID) psf_error(E_PS04D4_ALARM_TOO_MANY_SUBJS, 
			0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 1,
			sizeof(maxsubj), &maxsubj);
	    return (E_DB_ERROR);
	}
    }
    /*
    ** Now generate the query text 
    */
    /* set identifier type; default is PUBLIC */
    if (subject_type==DBGR_DEFAULT)
    	psy_cb->psy_gtype=DBGR_PUBLIC;
    else
    	psy_cb->psy_gtype=subject_type;

    alarm->dba_objtype=object_type;
    /*
    ** The "create security_alarm" statement requires query text to be
    ** stored in the iiqrytext relation. 
    */

    /* Generate query text */

    /* Fill in an identifier placeholder */
    (VOID) MEfill(DB_MAX_DELIMID, '?', (PTR) ident_placeholder);
    ident_placeholder[DB_MAX_DELIMID] = EOS;

    status = psq_topen(&cb->pss_tchain, &cb->pss_memleft, 
        &psq_cb->psq_error);
    if (status != E_DB_OK)
        return (status);

    /* Text is a buffer where templates are built */
    text[0] = EOS;

    STcat(text, "create security_alarm ");

    /* Append optional alarm name  */
    if(got_alarm_name)
    {
	i4 alen;

	STcat(text,"\"");
        len=STlength(text);
	alen = psf_trmwhite( sizeof(alarm->dba_alarmname),
                         (char*)&alarm->dba_alarmname);
        STncpy(text+len, (char*)&alarm->dba_alarmname, alen );
	text[len + alen] = '\0';
        STcat(text,"\" ");
    }
    /* Object type */
    if(object_type==DBOB_TABLE)
    {
    	STcat(text,"on table ");
        len=STlength(text);

	psy_tbl= (PSY_TBL *) psy_cb->psy_tblq.q_next;
        obj_name = (char *) &psy_tbl->psy_tabnm;
	STmove(obj_name,' ',sizeof(alarm->dba_objname),
		(char*)&alarm->dba_objname);
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid,alarm->dba_objid);
        obj_name_len = 
	    (i4) psf_trmwhite(sizeof(psy_tbl->psy_tabnm), (char *) obj_name);
        if (~psy_tbl->psy_mask & PSY_REGID_OBJ_NAME)
        {
	    status = psl_norm_id_2_delim_id((u_char**)&obj_name, &obj_name_len,
	        delim_obj_name, &psy_cb->psy_error);
	    if(status!=E_DB_OK)
		return status;
        }
	len=STlength(text);
	STncpy(text+len, obj_name, obj_name_len);
	text[len + obj_name_len] = '\0';
        STcat(text," ");
    }
    else if(object_type==DBOB_DATABASE)
    {
        if(!(alarm->dba_flags&DBA_ALL_DBS))
        {
           /*
           ** Database name 
           */
	   i4 alen;
           STcat(text,"on database ");
	   if(psy_cb->psy_flags&PSY_DELIM_OBJ)
		STcat(text,"\"");
           len=STlength(text);
	   alen = psf_trmwhite(
    		    sizeof(alarm->dba_objname),
    		     (char*)&alarm->dba_objname);
           STncpy(text+len, (char*)&alarm->dba_objname, alen );
	   text[len + alen] = '\0';
	   if(psy_cb->psy_flags&PSY_DELIM_OBJ)
		STcat(text,"\"");
           STcat(text," ");
        }
        else
        {
    	    /*
    	    ** Current installation is same as "all databases"
    	    */
    	    STcat(text,"on current installation ");
        }
    }

    /* Generate IF cond_list */
    if (psy_cb->psy_alflag & PSY_ALMSUCCESS &&
    	psy_cb->psy_alflag & PSY_ALMFAILURE)
    {
    	str = " if success, failure";
    }
    else if (psy_cb->psy_alflag & PSY_ALMFAILURE)
    {
    	str = " if failure";
    }
    else
    {
    	str = " if success";
    }

    STcat(text, str);

        /* Generate WHEN priv_list  */
    comma=FALSE;
    if (psy_cb->psy_opmap & DB_RETRIEVE)
    {
    	STcat(text, " when select");
    	comma = TRUE;
    }
    if (psy_cb->psy_opmap & DB_APPEND)
    {
    	STcat(text, ((comma) ? ", insert" : " when insert"));
    	comma = TRUE;
    }
    if (psy_cb->psy_opmap & DB_DELETE)
    {
    	STcat(text, ((comma) ? ", delete" : " when delete"));
    	comma = TRUE;
    }
    if (psy_cb->psy_opmap & DB_REPLACE)
    {
    	STcat(text, ((comma) ? ", update" : " when update"));
    	comma = TRUE;
    }
    if (psy_cb->psy_opmap & DB_CONNECT)
    {
    	STcat(text, ((comma) ? ", connect" : " when connect"));
    	comma = TRUE;
    }
    if (psy_cb->psy_opmap & DB_DISCONNECT)
    {
    	STcat(text, ((comma) ? ", disconnect" : " when disconnect"));
    	comma = TRUE;
    }

    /* add subject type/name */

    if (subject_type == DBGR_USER || 
        subject_type == DBGR_APLID || 
        subject_type == DBGR_GROUP )
    {
        psl_grantee_type_to_str(subject_type, &str, &str_len,TRUE);
        if (!str)
        {
             /* this is very bad */
             (VOID) psf_error(E_PS0002_INTERNAL_ERROR, 0L, PSF_INTERR,
    	     &err_code, &psq_cb->psq_error, 0);
             return (E_DB_ERROR);
        }
        STcat(text, str);

    	psy_cb->psy_noncol_grantee_off = STlength(text);
    	/* This is a placeholder for the real subject name */
	if(!got_alarm_name)
	{
	    /*
	    ** Deferred until PSY operations
	    */
	    STcat(text,(char*)ident_placeholder);
	    STcat(text," ");
	}
	else
	{
	    /*
	    ** Only one subject allowed, so assign now
	    */
	    psy_usr = (PSY_USR *) psy_cb->psy_usrq.q_next;
	    subj_name = (u_char *) &psy_usr->psy_usrnm;
	    subj_name_len = 
		(i4) psf_trmwhite(sizeof(psy_usr->psy_usrnm), 
		    (char *) subj_name);
	    if (~psy_usr->psy_usr_flags & PSY_REGID_USRSPEC)
	    {
		status = psl_norm_id_2_delim_id(&subj_name, 
		    &subj_name_len, delim_subj_name, 
		    &psy_cb->psy_error);
		if (DB_FAILURE_MACRO(status))
		    return status;
	    }
            len = STlength(text);
	    STncpy(text+len, (char *)subj_name, subj_name_len);
	    text[len + subj_name_len] = '\0';
	    STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm, alarm->dba_subjname);
    	    psy_cb->psy_noncol_grantee_off = 0;
	}
	alarm->dba_subjtype=subject_type;
    }
    else
    {
    	STcat(text," by public ");
    	psy_cb->psy_noncol_grantee_off = 0;
	MEfill(sizeof(alarm->dba_subjname),' ',
		(PTR)&alarm->dba_subjname);
	alarm->dba_subjtype=DBGR_PUBLIC;
    }

    /* Event info */
    /*
    ** Store the query text template.
    */
    if(psy_cb->psy_tuple.psy_alarm.dba_flags&DBA_DBEVENT)
    {
	DB_IIEVENT evtuple;
	i4	   alen;
	/*
	** Get name/owner info for event
	*/
	status=psy_evget_by_id(cb,  &alarm->dba_eventid,
			&evtuple, &err_blk);
	if(status!=E_DB_OK)
	{
		(VOID) psf_error(E_US247A_9338_MISSING_ALM_EVENT, 
				0L, PSF_INTERR,
				&err_blk.err_code, &err_blk, 3,
		sizeof(alarm->dba_eventid.db_tab_base), &alarm->dba_eventid.db_tab_base,
		sizeof(alarm->dba_eventid.db_tab_index), &alarm->dba_eventid.db_tab_index,
		sizeof(alarm->dba_alarmname), &alarm->dba_alarmname);
		return E_DB_ERROR;
	}
    	STcat(text," raise dbevent \"");
        len=STlength(text);
    	alen = 	psf_trmwhite( sizeof(evtuple.dbe_owner),
    		     (char*)&evtuple.dbe_owner);
        STncpy(text+len, (char*)&evtuple.dbe_owner, alen );
	text[ len + alen ] = '\0';
        STcat(text,"\".\"");
        len=STlength(text);
    	alen = psf_trmwhite( sizeof(evtuple.dbe_name),
    		     (char*)&evtuple.dbe_name);
        STncpy(text+len, (char*)&evtuple.dbe_name, alen );
        STcat(text,"\" ");

    	/* Optional event text */
    	if(alarm->dba_flags&DBA_DBEVTEXT)
    	{
    	    i4  evtextlen;

    	    STcat(text,"'");
    	    len=STlength(text);
    	    /*
    	    ** Find length without trailing spaces
    	    */
    	    evtextlen=psf_trmwhite(
    	    	sizeof(alarm->dba_eventtext),
    	    	 (char*)&alarm->dba_eventtext);
    	    /*
    	    ** Copy over excluding trailing spaces
    	    */
    	    STncpy(text+len, (char*)&alarm->dba_eventtext, evtextlen);
	    text[ len + evtextlen ] = '\0';
    	    STcat(text,"'");
    	}
    }

    status = psq_tadd(cb->pss_tchain, (u_char *) text,
    		  STlength(text), &piece, &psq_cb->psq_error);
    if (status != E_DB_OK)
        return (status);

    /* Determine size of the template */
    status = psq_tqrylen(cb->pss_tchain, &psy_cb->psy_noncol_qlen);
    if (status != E_DB_OK)
        return (status);

    status = psq_tout(cb, (PSS_USRRANGE*) NULL, cb->pss_tchain,
    		  &cb->pss_tstream, &psq_cb->psq_error);
    if (status != E_DB_OK)
        return (status);

    /*
    ** Copy the object id for the query text to the control block.
    */
    STRUCT_ASSIGN_MACRO(cb->pss_tstream.psf_mstream, psy_cb->psy_qrytext);

    return E_DB_OK;
}

/*
** Name: psl_al2_tbl_obj_spec
**
** Description:
**	Handles the ON TABLE obj_spec clause of the CREATE SECURITY_ALARM
**	statement.
**
** Inputs:
**	cb,psy_cb,psq_cb   - Regular control blocks
**
** 	obj_spec	   - Object spec
** 
** History:
**	6-dec-93 (robf)
**          Created
**	19-Jun-2010 (kiria01) b123951
**	    Add extra parameter to psl0_rngent for WITH support.
*/
DB_STATUS
psl_al2_tbl_obj_spec(
	PSS_SESBLK  *cb,
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	PSS_OBJ_NAME*obj_spec
)
{
	i4		err_code;
	PSY_TBL		*psy_tbl;
	DB_STATUS	status=E_DB_OK;
	bool		found = FALSE;
	PSS_RNGTAB	*resrange;
	i4		rngvar_info;
        char 		*objtype;


	if (obj_spec->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA)
	{
	    status = psl_orngent(&cb->pss_auxrng, -1, 
		obj_spec->pss_orig_obj_name,
		&obj_spec->pss_owner, &obj_spec->pss_obj_name, cb, 
		TRUE, &resrange,
		psq_cb->psq_mode, &psq_cb->psq_error, &rngvar_info);
	}
	else
	{
	    status = psl0_rngent(&cb->pss_auxrng, -1, 
		obj_spec->pss_orig_obj_name,
		&obj_spec->pss_obj_name, cb, TRUE, &resrange,
		psq_cb->psq_mode, &psq_cb->psq_error,
		PSS_USRTBL | PSS_DBATBL | PSS_INGTBL, &rngvar_info, 0, NULL);
	}

	if (status != E_DB_OK)
	{
	    return (status);
	}
	else if (resrange == (PSS_RNGTAB *) NULL)
	{
	    /* table doesn't exist */
	    (VOID) psf_error(2117L, 0L, PSF_USERERR,&err_code,
		&psq_cb->psq_error, 1, STlength(obj_spec->pss_orig_obj_name),
		obj_spec->pss_orig_obj_name);
	    return E_DB_ERROR;
	}
	else
	{
	    i4	    mask;
	    
	    /* Check if table is specified for the first time */
	    for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
		 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
		 psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next
		)
	    {
		/*
		** compare components of DB_TAB_ID rather than using more
		** expensive MEcmp()
		*/
		if (resrange->pss_tabid.db_tab_base ==
		    psy_tbl->psy_tabid.db_tab_base
		    &&
		    resrange->pss_tabid.db_tab_index ==
		    psy_tbl->psy_tabid.db_tab_index
		   )
		{
		    found = TRUE;

		    /*
		    ** if the same object is mentioned more than once by its 
		    ** name (as opposed to by synonym name) remember whether 
		    ** its schema and/or object name may be expressed as regular
		    ** identifiers
		    */
		    if (~rngvar_info & PSS_BY_SYNONYM)
		    {
			if (   ~psy_tbl->psy_mask & PSY_REGID_SCHEMA_NAME
			    && obj_spec->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA
			    && obj_spec->pss_schema_id_type == PSS_ID_REG
			   )
			{
			    psy_tbl->psy_mask |= PSY_REGID_SCHEMA_NAME;
			}

			if (   ~psy_tbl->psy_mask & PSY_REGID_OBJ_NAME
			    && obj_spec->pss_obj_id_type == PSS_ID_REG)
			{
			    psy_tbl->psy_mask |= PSY_REGID_OBJ_NAME;
			}
		    }

		    break;
		}
	    }

	    if (!found)
	    {
		mask = resrange->pss_tabdesc->tbl_status_mask;

		if (mask & (DMT_IDX|DMT_VIEW))
		{
		    /*
		    ** let user know if name supplied by the user was
		    ** resolved to a synonym
		    */
		    if (rngvar_info & PSS_BY_SYNONYM)
		    {
			i4	    len;
			char    *op;

		        op = "CREATE SECURITY_ALARM";
		        len = sizeof("CREATE SECURITY_ALARM") - 1;
			
			psl_syn_info_msg(cb, resrange, obj_spec, rngvar_info,
			    len, op, &psq_cb->psq_error);
		    }
		    if(mask&DMT_VIEW)
		    {
			/*66236 - switched "an index" and "a view"*/
			objtype="a view";
		    }
		    else
		    {
			/*66236 - switched from "an index" and "a view"*/
			objtype="an index";
		    }
		    /* CREATE SECURITY_ALARM on  an index/view is not allowed */

		    /*66236 - added 2nd objtype reference to
		     *psf_error(E_PS04D1) call.  *num_parms (the 6th parm)
		     *is 3, indicating the number of parm (pair(s) that
		     *follow. The message was intended to include objtype
		     *twice. The message was being created
		     *without both references to objtype.
		     */

		    (VOID) psf_error(E_PS04D1_ALARM_INVALID_OBJECT, 
			0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 3,
			psf_trmwhite(sizeof(DB_TAB_NAME),
			    (char *) &resrange->pss_tabname),
			&resrange->pss_tabname,
			STlength(objtype), objtype,
			STlength(objtype), objtype);
		     return E_DB_ERROR;
		}
		else
		{
		    /*
		    ** The following now applies to
		    ** GRANT and CREATE SECURITY_ALARM:
		    ** DBA can run the query against extended catalogs.
		    ** User may run the query against catalogs if he has a
		    ** catalog update privilege.
		    ** Any user may run the query against his table.
		    ** If !psy_permit_ok(), we definitely need to report an
		    ** error.
		    **
		    */

		    if ( !psy_permit_ok(mask, cb, &resrange->pss_ownname) )
		    {
			if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
			{
			    /*
			    ** Must audit CREATE SECURITY_ALARM failure.
			    */
			    DB_STATUS       local_status;
			    DB_ERROR         e_error;

			    local_status = psy_secaudit(FALSE, cb,
				&resrange->pss_tabdesc->tbl_name.db_tab_name[0],
				&resrange->pss_tabdesc->tbl_owner,
				sizeof(DB_TAB_NAME), SXF_E_TABLE,
				I_SX202D_ALARM_CREATE, SXF_A_FAIL | SXF_A_CREATE,
				&e_error);
			}

			/*
			** let user know if name supplied by the user was
			** resolved to a synonym
			*/
			if (rngvar_info & PSS_BY_SYNONYM)
			{
			    psl_syn_info_msg(cb, resrange, obj_spec, rngvar_info,
				sizeof("CREATE SECURITY_ALARM") - 1,
				"CREATE SECURITY_ALARM", &psq_cb->psq_error);
			}

			(VOID) psf_error(E_PS04D1_ALARM_INVALID_OBJECT, 
			     0L, PSF_USERERR, &err_code,
			    &psq_cb->psq_error, 3,
			    psf_trmwhite(sizeof(DB_TAB_NAME),
				(char *) &resrange->pss_tabname),
			    &resrange->pss_tabname,
			    sizeof("not owned by you")-1,"not owned by you",
			    sizeof("objects not owned by you")-1,"objects not owned by you"
			    );
				
			return (E_DB_ERROR);
		    }

		    /* Allocate memory for a table entry */
		    status = psf_malloc(cb, &cb->pss_ostream,
			(i4) (sizeof(PSY_TBL) +
			    (psy_cb->psy_u_numcols + psy_cb->psy_i_numcols +
			      psy_cb->psy_r_numcols) * sizeof(i4)), 
			(PTR *) &psy_tbl, &psq_cb->psq_error);
		    if (status != E_DB_OK)
		    {
			return (status);
		    }

		    /*
		    ** Copy table id, table name, and owner name in table entry.
		    */
		    STRUCT_ASSIGN_MACRO(resrange->pss_tabid,
					psy_tbl->psy_tabid);
		    STRUCT_ASSIGN_MACRO(resrange->pss_tabname,
					psy_tbl->psy_tabnm);
		    STRUCT_ASSIGN_MACRO(resrange->pss_ownname,
					psy_tbl->psy_owner);

		    /* remember the object type */
		    psy_tbl->psy_mask = PSY_OBJ_IS_TABLE;

		    /*
		    ** if the object was specified using its real name (as 
		    ** opposed to a name of a synonym defined for it), remember
		    ** whether the schema and/or object name may be expressed 
		    ** as regular identifiers
		    */
		    if (~rngvar_info & PSS_BY_SYNONYM)
		    {
			if (   obj_spec->pss_objspec_flags & PSS_OBJSPEC_EXPL_SCHEMA
			    && obj_spec->pss_schema_id_type == PSS_ID_REG
			   )
			{
			    psy_tbl->psy_mask |= PSY_REGID_SCHEMA_NAME;
			}

			if (obj_spec->pss_obj_id_type == PSS_ID_REG)
			{
			    psy_tbl->psy_mask |= PSY_REGID_OBJ_NAME;
			}
		    }

		    /*
		    ** Attach element to the list.
		    */
		    (VOID) QUinsert((QUEUE *) psy_tbl, 
			(QUEUE *) &psy_cb->psy_tblq);

		}	    /* table is not an index */
	    }	    /* if (!found) */
	}	/* resrange != NULL */
	return status;
}

/*
** Name: psl_al1_db_obj_spec
**
** Description:
**	Handles the ON DATABASE/CURRENT INSTALLATION clause of the 
**      CREATE SECURITY_ALARM statement.
**
** Inputs:
**	cb,psy_cb,psq_cb   - Regular control blocks
**
**	dbname		   - database name
**
**	is_delim	   - TRUE if specified as a delimited identifier.
** 
** History:
**	6-dec-93 (robf)
**          Created
**      6-mar-95 (inkdo01)
**          Fix bug 67313 by blanking object name for CURRENT INSTALLATION
**
*/
DB_STATUS
psl_al3_db_obj_spec(
	PSS_SESBLK  *cb,
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	char	    *objname,
	bool	    is_delim
)
{
    DB_SECALARM *alarm= &psy_cb->psy_tuple.psy_alarm;


    /*
    ** Save database name in alarm tuple 
    */
    if(objname)
    {
	    STmove(objname,' ',sizeof(alarm->dba_objname), (char*)&alarm->dba_objname);
	    if(!is_delim)
		    psy_cb->psy_flags |= PSY_DELIM_OBJ;
    }
    else
    {
	/*
	** Mark as all databases
	*/
	alarm->dba_flags|=DBA_ALL_DBS;
        MEfill(sizeof(psy_cb->psy_tuple.psy_alarm.dba_objname), ' ',
            (PTR)&psy_cb->psy_tuple.psy_alarm.dba_objname);
                                      /* blank out the object name */
    }
    return E_DB_OK;
}

/*
** Name: psl_al4_drop_alarm
**
** Description:
**	Handles the final stage of the DROp SECURITY_ALARM statement
**
** Inputs:
**	cb,psy_cb,psq_cb   - Regular control blocks
**
**      object_type	   - Type of object (table, database)
** 
** History:
**	6-dec-93 (robf)
**          Created
*/
DB_STATUS
psl_al4_drop_alarm(
	PSS_SESBLK  *cb,
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	i4	    object_type
)
{
    i4	    err_code;
    i4		    num_tbl;
    PSY_TBL	    *psy_tbl;
    DB_SECALARM     *alarm= &psy_cb->psy_tuple.psy_alarm;
    char	    *obj_name;

    /*
    ** Do various semantic checks by object type
    */
    if(object_type==DBOB_TABLE)
    {
	num_tbl=0;
        for (psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
	 psy_tbl != (PSY_TBL *) &psy_cb->psy_tblq;
	 psy_tbl = (PSY_TBL *) psy_tbl->queue.q_next)
	{
		num_tbl++;
	}
	/*
	** Make sure we have appropriate numbers of tables.
	*/
	if(!num_tbl)
	{
		(VOID) psf_error(E_PS04D2_ALARM_NO_OBJECT_NAMES, 
			0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 1,
			sizeof("TABLE")-1, "TABLE");
		return (E_DB_ERROR);
	}
	if(num_tbl>1)
	{
		i4 maxobj=1;
		(VOID) psf_error(E_PS04D3_ALARM_TOO_MANY_OBJS, 
			0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 2,
			sizeof("TABLE")-1, "TABLE",
			sizeof(maxobj),&maxobj);
		return (E_DB_ERROR);
	}
	/*
	** Save table info in tuple for psy
	*/
        psy_tbl = (PSY_TBL *) psy_cb->psy_tblq.q_next;
        obj_name = (char *) &psy_tbl->psy_tabnm;
	STmove(obj_name,' ',sizeof(alarm->dba_objname),
		(char*)&alarm->dba_objname);
	STRUCT_ASSIGN_MACRO(psy_tbl->psy_tabid,alarm->dba_objid);

    }
    /*
    ** Build values into tuple
    */
    alarm->dba_objtype=object_type;

    return E_DB_OK;
}
