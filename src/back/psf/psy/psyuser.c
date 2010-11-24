/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <me.h>
#include    <st.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmacb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <psyaudit.h>
#include    <scf.h>
#include    <dudbms.h>

/**
**
**  Name: PSYUSER.C - Functions to manipulate the iiuser catalog.
**
**  Description:
**      This file contains functions necessary to create, alter and drop
**	users and user profiles:
**
**          psy_user  - Routes requests to manipulate iiuser/iiprofile
**          psy_cuser - Creates users
**          psy_auser - Changes users attributes
**          psy_kuser - Drops users
**          psy_cprofile - Creates profile
**          psy_aprofile - Changes profile attributes
**          psy_kprofile - Drops profiles
**
**  User profiles have many of the same properties as users, so are processed
**  in a similar way to user profiles. Note, however, that the attributes
**  are not identical. Down the road they could diverge even more, so 
**  are stored in seperate tables, and processed by QEF/RDF differently.
**
**  History:
**	04-sep-89 (ralph)
**	    written
**	30-oct-89 (ralph)
**	    call qeu_audit() only if failure due to authorization check
**	16-jan-90 (ralph)
**	    add support for user passwords
**      11-apr-90 (jennifer)
**          bug 20746 - Fix initialization of access and msgid.
**	08-aug-90 (ralph)
**	    add support for oldpassword (as in ALTER USER OLDPASSWORD = 'xxx')
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	03-aug-92 (barbara)
**	    Initialize RDF_CB by calling pst_rdfcb_init before call to RDF
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	09-dec-1992 (pholman)
**	    C2: change obsolete qeu_audit to be psy_secaudit.
**	1-jun-1993 (robf)
**	    Don't access du_reserve any more, its gone
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	19-oct-93 (robf)
**          Add support for all default privileges
**	18-nov-93 (stephenb)
**	    Include psyaudit.h for prototyping.
**	14-feb-94 (robf)
**          Add (PTR) casts to rdf_call to stop compiler warnings
**	15-mar-94 (robf)
**          Add support for users  with external passwords
**	13-jun-96 (nick)
**	    LINT directed changes.
**	17-jul-96 (cwaldman)
**	    When creating a user or profile, set du_flagsmask flags 
**	    DU_UPRIV, DU_UALLEVENTS, DU_UDEFEVENTS, DU_UQRYTEXT and
**	    DU_UNOQRYTEXT as appropriate.
**	10-Jan-2001 (jenjo02)
**	    Supply session id to SCF instead of DB_NOSESSION.
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_user(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_cuser(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_auser(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_kuser(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
static i4 psy_cprofile(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
static i4 psy_aprofile(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
static i4 psy_kprofile(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psy_user - Dispatch user qrymod routines
**
**  INTERNAL PSF call format: status = psy_user(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_USER,&psy_cb, sess_cb);
**
** Description:
**	This procedure checks the psy_usflag field of the PSY_CB
**	to determine which qrymod processing routine to call:
**		PSY_CUSER results in call to psy_cuser()
**		PSY_AUSER results in call to psy_auser()
**		PSY_KUSER results in call to psy_kuser()
**
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usflag			user operation
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	04-sep-89 (ralph)
**          written
**	30-oct-89 (ralph)
**	    call qeu_audit() only if failure due to authorization check
**      11-apr-90 (jennifer)
**          bug 20746 - Fix initialization of access and msgid.
**	09-dec-1992 (pholman)
**	    C2: change obsolete qeu_audit to be psy_secaudit.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	17-jun-93 (andre)
**	    changed interface of psy_secaudit() to accept PSS_SESBLK
**	12-jul-93 (robf)
**	    check if user passwords are allowed
**	2-feb-94 (robf)
**          Don't check for passwords allowed with PROFILE statements,
**          profiles don't have passwords.
**	26-Oct-2004 (schka24)
**	    Restore security check accidently deleted with B1.
*/
DB_STATUS
psy_user(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, local_status;
    DB_ERROR		e_error;
    i4		err_code;
    char		dbname[DB_DB_MAXNAME];
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[1];
    i4			access = 0;    		        
    i4		msgid;
    PSY_USR		*psy_usr = (PSY_USR *)psy_cb->psy_usrq.q_next;
    bool		sensitive = FALSE;
    bool		leave_loop = TRUE;

    /* This code is called for SQL only */

    /* Make sure session is authorized and connected to iidbdb */

    do
    {
	/* Ensure we're authorized to issue CREATE/ALTER/DROP USER */

	if (((psy_cb->psy_usflag & ~(PSY_AUSER | PSY_USRPASS | PSY_USROLDPASS))
	    ||
	    (!(psy_cb->psy_usflag & PSY_USRPASS))
	    ||
	    (psy_cb->psy_usflag & (PSY_USREXPDATE|PSY_USRDEFPRIV))
	    ||
	    (STskipblank
	     ((char *)&psy_cb->psy_apass,sizeof(psy_cb->psy_apass)) == NULL)
	   ) &&  (!(sess_cb->pss_ustat & DU_UMAINTAIN_USER)))
	    sensitive = TRUE;

	/* Now ALTER_AUDIT clauses - SECURITY_AUDIT */
	if((psy_cb->psy_usflag & PSY_USRSECAUDIT) &&
	  !(sess_cb->pss_ustat & DU_UALTER_AUDIT))
	    sensitive = TRUE;

	/* Alter priv with SECURITY, and don't have SECURITY, is sensitive */
	if ( (psy_cb->psy_usflag & (PSY_USRPRIVS|PSY_USRAPRIVS|PSY_USRDPRIVS))
	  && (psy_cb->psy_usprivs & DU_USECURITY)
	  && ! (sess_cb->pss_ustat & DU_USECURITY) )
	    sensitive = TRUE;

	if(((psy_cb->psy_usflag & (PSY_APROFILE|PSY_DPROFILE|PSY_CPROFILE)))
		&& !(sess_cb->pss_ustat & DU_UMAINTAIN_USER))
	{
	    /*
	    ** Creating profiles  is a privileged operation
	    */
	    sensitive=TRUE;
	}
	if (sensitive)
	{
	    /* User is not authorized to perform the requested function */
	    if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	    {
		if ((psy_cb->psy_usflag & (PSY_CUSER | PSY_AUSER | PSY_KUSER))
					== PSY_CUSER)
		{	
		    access = SXF_A_FAIL | SXF_A_CREATE;
		    msgid = I_SX200C_USER_CREATE;
		}
		else if ((psy_cb->psy_usflag & (PSY_CUSER | PSY_AUSER | PSY_KUSER))
					== PSY_AUSER)
		{
		    access = SXF_A_FAIL | SXF_A_ALTER;
		    msgid = I_SX2023_USER_ALTER;
		}
		else if ((psy_cb->psy_usflag & (PSY_CUSER | PSY_AUSER | PSY_KUSER))
					== PSY_KUSER)
		{
		    access = SXF_A_FAIL | SXF_A_DROP;
		    msgid = I_SX200D_USER_DROP;
		}
		else if ((psy_cb->psy_usflag & (PSY_CPROFILE | PSY_APROFILE | PSY_DPROFILE))
					== PSY_CPROFILE)
		{	
		    access = SXF_A_FAIL | SXF_A_CREATE;
		    msgid =  I_SX272E_CREATE_PROFILE;
		}
		else if ((psy_cb->psy_usflag & (PSY_CPROFILE | PSY_APROFILE | PSY_DPROFILE))
					== PSY_APROFILE)
		{
		    access = SXF_A_FAIL | SXF_A_ALTER;
		    msgid = I_SX272F_ALTER_PROFILE;
		}
		else if ((psy_cb->psy_usflag & (PSY_CPROFILE | PSY_APROFILE | PSY_DPROFILE))
					== PSY_DPROFILE)
		{
		    access = SXF_A_FAIL | SXF_A_DROP;
		    msgid =  I_SX2730_DROP_PROFILE;
		}

		local_status = psy_secaudit(FALSE, sess_cb,
			    (char *)&psy_usr->psy_usrnm,
			    (DB_OWN_NAME *)NULL,
			    sizeof(DB_OWN_NAME), SXF_E_SECURITY,
			    msgid, access,
			    &e_error);
	    }

	    if (psy_cb->psy_usflag & PSY_AUSER)
	    {
		err_code = E_US18D5_6357_FORM_NOT_AUTH;
		(VOID) psf_error(E_US18D5_6357_FORM_NOT_AUTH, 0L,
			     PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			     sizeof("ALTER USER/PROFILE")-1,
			     "ALTER USER/PROFILE");
	    }
	    else
	    {
		err_code = E_US18D3_6355_NOT_AUTH;
		(VOID) psf_error(E_US18D3_6355_NOT_AUTH, 0L,
			     PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			     sizeof("CREATE/DROP USER/PROFILE")-1,
			     "CREATE/DROP USER/PROFILE");
	    }
	    status   = E_DB_ERROR;
	    break;

	}

	/* Ensure we're connected to the iidbdb database */

	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type	= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_PSF_ID;
	scf_cb.scf_session	= sess_cb->pss_sessid;
	scf_cb.scf_ptr_union.scf_sci     = (SCI_LIST *) sci_list;
	scf_cb.scf_len_union.scf_ilength = 1;
	sci_list[0].sci_code    = SCI_DBNAME;
	sci_list[0].sci_length  = sizeof(dbname);
	sci_list[0].sci_aresult = (char *) dbname;
	sci_list[0].sci_rlength = NULL;

	status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	{
	    err_code = scf_cb.scf_error.err_code;
	    (VOID) psf_error(E_PS0D13_SCU_INFO_ERR, 0L,
			 PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
	    return(status);
	}

	if (MEcmp((PTR)dbname, (PTR)DB_DBDB_NAME, sizeof(dbname)))
	{
	    /* Session not connected to iidbdb */
	    err_code = E_US18D4_6356_NOT_DBDB;
	    (VOID) psf_error(E_US18D4_6356_NOT_DBDB, 0L,
			     PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			     sizeof("CREATE/ALTER/DROP USER/PROFILE")-1,
			     "CREATE/ALTER/DROP USER/PROFILE");
	    status   = E_DB_ERROR;
	    break;
	}

	/*
	** Check if passwords are allowed.
	*/
	if((sess_cb->pss_ses_flag & PSS_PASSWORD_NONE)
	   && !(psy_cb->psy_usflag & (
			PSY_KUSER|
			PSY_DPROFILE|PSY_APROFILE|PSY_CPROFILE))
    	   && STskipblank((char *)&psy_cb->psy_apass,
                           (i4)sizeof(psy_cb->psy_apass)) != NULL)
	{
	    err_code=E_US18E7_6375;
            (VOID) psf_error(E_US18E7_6375, 0L,
                             PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
                             sizeof("CREATE/ALTER USER WITH PASSWORD")-1,
                             "CREATE/ALTER USER WITH PASSWORD");
	    status   = E_DB_ERROR;
	    break;
	}
	break;
	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    /* Select the proper routine to process this request */

    if (status == E_DB_OK)
    switch (psy_cb->psy_usflag & (PSY_CUSER | PSY_AUSER | PSY_KUSER|
	    		PSY_CPROFILE | PSY_APROFILE | PSY_DPROFILE))
    {
	case PSY_CUSER:
	    status = psy_cuser(psy_cb, sess_cb);
	    break;

	case PSY_AUSER:
	    status = psy_auser(psy_cb, sess_cb);
	    break;

	case PSY_KUSER:
	    status = psy_kuser(psy_cb, sess_cb);
	    break;

	case PSY_CPROFILE:
	    status = psy_cprofile(psy_cb, sess_cb);
	    break;

	case PSY_APROFILE:
	    status = psy_aprofile(psy_cb, sess_cb);
	    break;

	case PSY_DPROFILE:
	    status = psy_kprofile(psy_cb, sess_cb);
	    break;

	default:
	    status = E_DB_SEVERE;
	    err_code = E_PS0D46_INVALID_USER_OP;
	    (VOID) psf_error(E_PS0D46_INVALID_USER_OP, 0L,
		    PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    break;
    }

    return    (status);
}

/*{
** Name: psy_cuser- Create user
**
**  INTERNAL PSF call format: status = psy_cuser(&psy_cb, sess_cb);
**
** Description:
**	This procedure creates a user.  If the
**	user already exists, the statement is aborted.
**	If the user does not exist, it is added to iiuser.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usrq			user list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in iiuser.
**
** History:
**	04-sep-89 (ralph)
**          written
**	16-jan-90 (ralph)
**	    add support for user passwords
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	28-feb-94 (robf)
**          Set flag when specifying a group so QEF knows its been 
**	    specified.
**	17-jul-96 (cwaldman)
**	    Set du_flagsmask flags DU_UPRIV, DU_UALLEVENTS, DU_UDEFEVENTS,
**	    DU_UQRYTEXT and DU_UNOQRYTEXT as appropriate when creating a
**	    user. These flags will later be used to accurately merge user
**	    and profile specification.
**	29-July-2003 (guphs01)
**	    Fixed handling of noprivileges case for create user.
**	19-July-2005 (toumi01)
**	    Support WITH PASSWORD=X'<encrypted>'. 
*/
DB_STATUS
psy_cuser(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    i4		err_code;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DU_USER		ustuple;
    register DU_USER	*ustup = &ustuple;
    PSY_USR		*psy_usr;
    bool                encrypt;
    SCF_CB              scf_cb;
    DB_DATA_VALUE	db_data;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_USER;
    rdf_rb->rdr_qrytuple    = (PTR) ustup;
    rdf_rb->rdr_qtuple_count = 1;

    ustup->du_gid	= 0;
    ustup->du_mid	= 0;
    ustup->du_flagsmask  = 0;

    /* 
    ** User profile. Note: This assumes the "default profile" is all
    ** spaces, and is initialized as so in PSL
    */
    ustup->du_flagsmask|= DU_UHASPROFILE;

    MECOPY_CONST_MACRO((PTR)&psy_cb->psy_usprofile, 
	sizeof(psy_cb->psy_usprofile), 
	(PTR)&ustup->du_profile);
	   
    /* Default privileges */
    if(psy_cb->psy_usflag& PSY_USRDEFPRIV)
    {
	ustup->du_defpriv = psy_cb->psy_usdefprivs;
	ustup->du_flagsmask|= DU_UDEFPRIV;
	/* Add default=all indicator */
	if(psy_cb->psy_usflag & PSY_USRDEFALL)
		ustup->du_flagsmask|=DU_UDEFALL;
    }
    else
    {
	/*
	** If no default specified use all privileges
	*/
	if (psy_cb->psy_usflag & PSY_USRPRIVS)
		ustup->du_defpriv = (i4)psy_cb->psy_usprivs;
	else
		ustup->du_defpriv = 0;
    }

    /*
    ** Expiration date, may be empty
    */
    if(psy_cb->psy_usflag& PSY_USREXPDATE)
    {
	/*
	** Date already formatted earlier
	*/
	MECOPY_CONST_MACRO((PTR)&psy_cb->psy_date, sizeof(DB_DATE), 
			(PTR)&ustup->du_expdate);
	ustup->du_flagsmask|= DU_UEXPDATE;
    }
    else
    {
	/*
	** Initialize to the empty date
	*/
	db_data.db_datatype  = DB_DTE_TYPE;
	db_data.db_prec	 = 0;
	db_data.db_length    = DB_DTE_LEN;
	db_data.db_data	 = (PTR)&ustup->du_expdate;
	status = adc_getempty(sess_cb->pss_adfcb, &db_data);
	if(status)
		return status;
    }

    if (psy_cb->psy_usflag & PSY_USRPRIVS) 
    { 
      ustup->du_status = (i4) psy_cb->psy_usprivs;
      ustup->du_flagsmask |= DU_UPRIV; 

      if (psy_cb->psy_usflag & PSY_UNOPRIVS)
      {
	ustup->du_flagsmask |= DU_UNOPRIV; 

	if ((psy_cb->psy_usflag& PSY_USRPROFILE) &&
	    !(psy_cb->psy_usflag & PSY_UNODEFPRIV))
	{
	  /* If create user has PSY_UNOPRIVS specified 
	  ** and we have provided a profile for this user 
	  ** but we have not set the nodefault_privileges
	  ** specified, We should return an error. profile
	  ** may have default_privileges, which will be  
	  ** inherited by the user with noprivileges.
	  */
	  {
	    err_code = E_US1968_6504_UNOPRIV_WDEFAULT;
	    (VOID) psf_error( E_US1968_6504_UNOPRIV_WDEFAULT, 0L,
	    PSF_USERERR, &err_code, &psy_cb->psy_error, 0);
	    status = E_DB_ERROR;
	    return(status);
	  }	
	}
      }
    }
    else
      ustup->du_status = 0;

    /*
    ** Save security audit options
    */
    if (psy_cb->psy_usflag & PSY_USRSECAUDIT)
    {
	if(psy_cb->psy_ussecaudit & PSY_USAU_ALL_EVENTS)
		{ ustup->du_status |= DU_UAUDIT;
		  ustup->du_flagsmask |= DU_UALLEVENTS; }
	else ustup->du_flagsmask |= DU_UDEFEVENTS;

	if (psy_cb->psy_ussecaudit & PSY_USAU_QRYTEXT)
		{ ustup->du_status |= DU_UAUDIT_QRYTEXT;
		  ustup->du_flagsmask |= DU_UQRYTEXT; }
	else ustup->du_flagsmask |= DU_UNOQRYTEXT;
    }

    if (psy_cb->psy_usflag & PSY_USREXTPASS)
	     ustup->du_flagsmask |= DU_UEXTPASS;

    if (psy_cb->psy_usflag & PSY_USRDEFGRP)
    {
	MEcopy((PTR)&psy_cb->psy_usgroup,
	       sizeof(ustup->du_group),
	       (PTR)&ustup->du_group);
	ustup->du_flagsmask|= DU_UGROUP;
	
    }
    else
	MEfill(sizeof(ustup->du_group),
	       (u_char)' ',
	       (PTR)&ustup->du_group);

    encrypt = (STskipblank((char *)&psy_cb->psy_apass,
                           (i4)sizeof(psy_cb->psy_apass))
                            != NULL);
    if (encrypt)
    {
        scf_cb.scf_length       = sizeof (SCF_CB);
        scf_cb.scf_type = SCF_CB_TYPE;
        scf_cb.scf_facility     = DB_PSF_ID;
        scf_cb.scf_session      = sess_cb->pss_sessid;
        scf_cb.scf_nbr_union.scf_xpasskey =
            (PTR)&ustup->du_name;
        scf_cb.scf_ptr_union.scf_xpassword =
            (PTR)&ustup->du_pass;
        scf_cb.scf_len_union.scf_xpwdlen =
             sizeof(ustup->du_pass);
    }
    else
        STRUCT_ASSIGN_MACRO(psy_cb->psy_apass, ustup->du_pass);

    status = E_DB_OK;

    for (psy_usr  = (PSY_USR *)  psy_cb->psy_usrq.q_next;
	 psy_usr != (PSY_USR *) &psy_cb->psy_usrq;
	 psy_usr  = (PSY_USR *)  psy_usr->queue.q_next
	)
    {
	MEcopy((PTR)&psy_usr->psy_usrnm,
	       sizeof(ustup->du_name),
	       (PTR)&ustup->du_name);

	/* Encrypt the password if nonblank and not already hex value */

        if (encrypt)
        {
            STRUCT_ASSIGN_MACRO(psy_cb->psy_apass, ustup->du_pass);
	    if(!(psy_cb->psy_usflag & PSY_HEXPASS))
	    {
		status = scf_call(SCU_XENCODE, &scf_cb);
		if (status != E_DB_OK)
		{
		    err_code = scf_cb.scf_error.err_code;
		    (VOID) psf_error(E_PS0D43_XENCODE_ERROR, 0L,
			PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
		    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
		    return(status);
		}
	    }
        }

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
	    &psy_cb->psy_error);

    return    (status);
}

/*{
** Name: psy_auser - Alter user
**
**  INTERNAL PSF call format: status = psy_auser(&psy_cb, sess_cb);
**
** Description:
**	This procedure alters an iiuser tuple.  If the
**	user does not exist, the statement is aborted.
**	If the user does exist, the associated
**	iiuser tuple is replaced.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usrq			user list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Replaces tuples in iiuser.
**
** History:
**	04-sep-89 (ralph)
**          written
**      16-jan-90 (ralph)
**          add support for user passwords
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
**	08-aug-90 (ralph)
**	    add support for oldpassword (as in ALTER USER OLDPASSWORD = 'xxx')
**	12-apr-95 (forky01)
**	    Apply default privs when only privs specified to allow backward
**	    compatibility to fix Secure 2.0 code.
**	28-jul-2003 (gupsh01)
**	    Added check for case when noprivileges have been added and
**	    nodefault_privileges is not specified, with a profile in alter
**	    user statement. 
*/
DB_STATUS
psy_auser(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    i4		err_code;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    struct
    {
	DU_USER	    ustuple;
	DU_USER	    ustuple2;
    }			usparam;
    register DU_USER	*ustup  = &usparam.ustuple;
    register DU_USER	*ustup2 = &usparam.ustuple2;
    PSY_USR		*psy_usr;
    bool                encrypt;
    bool                encrypt2;
    SCF_CB              scf_cb;
    SCF_CB              scf_cb2;
    DB_DATA_VALUE	db_data;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_REPLACE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_USER;
    rdf_rb->rdr_qrytuple    = (PTR) ustup;
    rdf_rb->rdr_qtuple_count = 1;

    ustup->du_gid	= 0;
    ustup->du_mid	= 0;
    ustup->du_flagsmask  = 0;

    /* User profile */
    if(psy_cb->psy_usflag& PSY_USRPROFILE)
    {
	    MECOPY_CONST_MACRO((PTR)&psy_cb->psy_usprofile, 
		sizeof(psy_cb->psy_usprofile), 
	    	(PTR)&ustup->du_profile);
	   
	    ustup->du_flagsmask|= DU_UHASPROFILE;
    }
    else
    {
	MEfill(sizeof(ustup->du_profile),
	       (u_char)' ',
	       (PTR)&ustup->du_profile);
    }

    if ((psy_cb->psy_usflag& PSY_USRPROFILE) &&
	(psy_cb->psy_usflag& PSY_USRPRIVS) &&
	(psy_cb->psy_usflag& PSY_UNOPRIVS) &&
        !(psy_cb->psy_usflag & PSY_UNODEFPRIV))
    {
  	/* If alter user has PSY_UNOPRIVS specified and
  	** we have provided a profile for this user
  	** but we have not set the nodefault_privileges
  	** specified, We should return an error. profile
  	** may have default_privileges, which will be
  	** inherited by the user with noprivileges.
  	*/
  	  err_code = E_US1968_6504_UNOPRIV_WDEFAULT;
  	  (VOID) psf_error( E_US1968_6504_UNOPRIV_WDEFAULT, 0L,
  	     PSF_USERERR, &err_code, &psy_cb->psy_error, 0);
  	  status = E_DB_ERROR;

	  return(status);
    }

    if(psy_cb->psy_usflag& PSY_USRDEFPRIV)
    {
	ustup->du_defpriv = psy_cb->psy_usdefprivs;
	ustup->du_flagsmask|= DU_UDEFPRIV;
	/* Add default=all indicator */
	if(psy_cb->psy_usflag & PSY_USRDEFALL)
		ustup->du_flagsmask|=DU_UDEFALL;

	/* set value of nodefault_privileges flag */
	if (psy_cb->psy_usflag & PSY_UNODEFPRIV)
	  ustup->du_flagsmask|= DU_UNODEFPRIV;
    }
    else
    {
	/*
	** If no default specified use all privileges
	*/
	if (psy_cb->psy_usflag & PSY_USRPRIVS)
		ustup->du_defpriv = (i4)psy_cb->psy_usprivs;
	else
		ustup->du_defpriv = 0;
    }

    /*
    ** Expiration date, may be empty
    */
    if(psy_cb->psy_usflag& PSY_USREXPDATE)
    {
	/*
	** Date already formatted earlier
	*/
	MECOPY_CONST_MACRO((PTR)&psy_cb->psy_date, sizeof(DB_DATE), 
			(PTR)&ustup->du_expdate);
	ustup->du_flagsmask|= DU_UEXPDATE;
    }
    else
    {
	/*
	** Initialize to the empty date
	*/
	db_data.db_datatype  = DB_DTE_TYPE;
	db_data.db_prec	 = 0;
	db_data.db_length    = DB_DTE_LEN;
	db_data.db_data	 = (PTR)&ustup->du_expdate;
	status = adc_getempty(sess_cb->pss_adfcb, &db_data);
	if(status)
		return status;
    }
    /*
    ** Check if adding, deleting, or setting privileges
    */
    ustup->du_status = (i4) psy_cb->psy_usprivs;

	
    if (psy_cb->psy_usflag & PSY_USRAPRIVS)
	ustup->du_flagsmask |=  DU_UAPRIV;
    else if (psy_cb->psy_usflag & PSY_USRDPRIVS)
	ustup->du_flagsmask |=  DU_UDPRIV;
    else if (psy_cb->psy_usflag & PSY_USRPRIVS)
	ustup->du_flagsmask |=  DU_UPRIV;
    else
	ustup->du_status = 0;
    /*
    ** Check if updating security audit options
    */
    if (psy_cb->psy_usflag & PSY_USRSECAUDIT)
    {
	if(psy_cb->psy_ussecaudit & PSY_USAU_ALL_EVENTS)
		ustup->du_flagsmask |= DU_UALLEVENTS;
	else
		ustup->du_flagsmask |= DU_UDEFEVENTS;
	if (psy_cb->psy_ussecaudit & PSY_USAU_QRYTEXT)
		ustup->du_flagsmask |= DU_UQRYTEXT;
	else
		ustup->du_flagsmask |= DU_UNOQRYTEXT;
    }

    if (psy_cb->psy_usflag & PSY_USRDEFGRP)
    {
	MEcopy((PTR)&psy_cb->psy_usgroup,
	       sizeof(ustup->du_group),
	       (PTR)&ustup->du_group);
	ustup->du_flagsmask|= DU_UGROUP;
    }
    else
    {
	MEfill(sizeof(ustup->du_group),
	       (u_char)' ',
	       (PTR)&ustup->du_group);
    }

    encrypt = (STskipblank((char *)&psy_cb->psy_apass,
                           (i4)sizeof(psy_cb->psy_apass))
                            != NULL);
    if (encrypt)
    {
        scf_cb.scf_length       = sizeof (SCF_CB);
        scf_cb.scf_type		= SCF_CB_TYPE;
        scf_cb.scf_facility     = DB_PSF_ID;
        scf_cb.scf_session      = sess_cb->pss_sessid;
        scf_cb.scf_nbr_union.scf_xpasskey =
            (PTR)&ustup->du_name;
        scf_cb.scf_ptr_union.scf_xpassword =
            (PTR)&ustup->du_pass;
        scf_cb.scf_len_union.scf_xpwdlen =
             sizeof(ustup->du_pass);
    }
    else
        STRUCT_ASSIGN_MACRO(psy_cb->psy_apass, ustup->du_pass);

    if (psy_cb->psy_usflag & PSY_USRPASS)
    {
	ustup->du_flagsmask|= DU_UPASS;
        if (psy_cb->psy_usflag & PSY_USREXTPASS)
	     ustup->du_flagsmask |= DU_UEXTPASS;
    }
    else
    {
	MEfill(sizeof(ustup->du_pass),
	       (u_char)' ',
	       (PTR)&ustup->du_pass);
    }

    encrypt2 = (STskipblank((char *)&psy_cb->psy_bpass,
                           (i4)sizeof(psy_cb->psy_bpass))
                            != NULL);
    if (encrypt2)
    {
        scf_cb2.scf_length      = sizeof (SCF_CB);
        scf_cb2.scf_type	= SCF_CB_TYPE;
        scf_cb2.scf_facility    = DB_PSF_ID;
        scf_cb2.scf_session     = sess_cb->pss_sessid;
        scf_cb2.scf_nbr_union.scf_xpasskey =
            (PTR)&ustup->du_name;
        scf_cb2.scf_ptr_union.scf_xpassword =
            (PTR)&ustup2->du_pass;
        scf_cb2.scf_len_union.scf_xpwdlen =
             sizeof(ustup2->du_pass);
    }
    else
        STRUCT_ASSIGN_MACRO(psy_cb->psy_bpass, ustup2->du_pass);

    if (psy_cb->psy_usflag & PSY_USROLDPASS)
	ustup->du_flagsmask|= DU_UOLDPASS;
    else
    {
	MEfill(sizeof(ustup2->du_pass),
	       (u_char)' ',
	       (PTR)&ustup2->du_pass);
    }

    status = E_DB_OK;

    for (psy_usr  = (PSY_USR *)  psy_cb->psy_usrq.q_next;
	 psy_usr != (PSY_USR *) &psy_cb->psy_usrq;
	 psy_usr  = (PSY_USR *)  psy_usr->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm,
			       ustup->du_name); */
	MEcopy((PTR)&psy_usr->psy_usrnm,
	       sizeof(ustup->du_name),
	       (PTR)&ustup->du_name);

	/* Encrypt the password if nonblank and not already hex value */

        if (encrypt)
        {
            STRUCT_ASSIGN_MACRO(psy_cb->psy_apass, ustup->du_pass);
	    if(!(psy_cb->psy_usflag & PSY_HEXPASS))
	    {
		status = scf_call(SCU_XENCODE, &scf_cb);
		if (status != E_DB_OK)
		{
		    err_code = scf_cb.scf_error.err_code;
		    (VOID) psf_error(E_PS0D43_XENCODE_ERROR, 0L,
			PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
		    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
		    return(status);
		}
            }
        }

        if (encrypt2)
        {
            STRUCT_ASSIGN_MACRO(psy_cb->psy_bpass, ustup2->du_pass);
            status = scf_call(SCU_XENCODE, &scf_cb2);
            if (status != E_DB_OK)
            {
                err_code = scf_cb2.scf_error.err_code;
                (VOID) psf_error(E_PS0D43_XENCODE_ERROR, 0L,
                                PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
                status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
                return(status);
            }
        }

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
	    &psy_cb->psy_error);

    return    (status);
} 

/*{
** Name: psy_kuser - Destroy user
**
**  INTERNAL PSF call format: status = psy_kuser(&psy_cb, sess_cb);
**
** Description:
**	This procedure deletes a user.  If the
**	user does not exist, the statement is aborted.
**	If the user does exist, the associated
**	iiuser tuple is deleted.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usrq			user list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_INFO			Function completed with warning(s).
**	    E_DB_WARN			One ore more roles were rejected.
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removed tuples from iiuser.
**
** History:
**	04-sep-89 (ralph)
**          written
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**      22-may-90 (teg)
**          init rdr_instr to RDF_NO_INSTR
*/
DB_STATUS
psy_kuser(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DU_USER		ustuple;
    register DU_USER	*ustup = &ustuple;
    PSY_USR		*psy_usr;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_types_mask  = RDR_USER;
    rdf_rb->rdr_qrytuple    = (PTR) ustup;
    rdf_rb->rdr_qtuple_count = 1;

    ustup->du_gid	= 0;
    ustup->du_mid	= 0;
    ustup->du_status 	= 0;
    ustup->du_flagsmask	= 0;


    MEfill(sizeof(ustup->du_group),
	   (u_char)' ',
	   (PTR)&ustup->du_group);

    status = E_DB_OK;

    for (psy_usr  = (PSY_USR *)  psy_cb->psy_usrq.q_next;
	 psy_usr != (PSY_USR *) &psy_cb->psy_usrq;
	 psy_usr  = (PSY_USR *)  psy_usr->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm,
			       ustup->du_name); */
	MEcopy((PTR)&psy_usr->psy_usrnm,
	       sizeof(ustup->du_name),
	       (PTR)&ustup->du_name);

	stat = rdf_call(RDF_UPDATE, (PTR)&rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			     &psy_cb->psy_error);

    return    (status);
} 

/*{
** Name: psy_cprofile- Create profile
**
** Description:
**	This procedure creates a profile.  If the
**	profile already exists, the statement is aborted.
**	If the profile does not exist, it is added to iiprofile.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usrq			profile list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in iiuser.
**
** History:
**	26-aug-93 (robf)
**	   Created from Secure 2.0
**	17-jul-96 (cwaldman)
**	    Set du_flagsmask flags DU_UPRIV, DU_UALLEVENTS, DU_UDEFEVENTS,
**	    DU_UQRYTEXT and DU_UNOQRYTEXT as appropriate when creating a
**	    profile. These flags will later be used to accurately merge
**	    user and profile specification.
*/
static DB_STATUS
psy_cprofile(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DU_PROFILE		protuple;
    register DU_PROFILE	*protup = &protuple;
    PSY_USR		*psy_usr;
    DB_DATA_VALUE	db_data;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_APPEND;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_2types_mask = RDR2_PROFILE;
    rdf_rb->rdr_qrytuple    = (PTR) protup;
    rdf_rb->rdr_qtuple_count = 1;

    protup->du_flagsmask  = 0;

    /* Default privileges */
    if(psy_cb->psy_usflag& PSY_USRDEFPRIV)
    {
	protup->du_defpriv = psy_cb->psy_usdefprivs;
	protup->du_flagsmask|= DU_UDEFPRIV;

	/* Add default=all indicator */
	if(psy_cb->psy_usflag & PSY_USRDEFALL)
		protup->du_flagsmask|=DU_UDEFALL;
    }
    else
    {
	/*
	** If no default specified use all privileges
	*/
	if (psy_cb->psy_usflag & PSY_USRPRIVS)
		protup->du_defpriv = (i4)psy_cb->psy_usprivs;
	else
		protup->du_defpriv = 0;
    }

    /*
    ** Expiration date, may be empty
    */
    if(psy_cb->psy_usflag& PSY_USREXPDATE)
    {
	/*
	** Date already formatted earlier
	*/
	MECOPY_CONST_MACRO((PTR)&psy_cb->psy_date, sizeof(DB_DATE), 
			(PTR)&protup->du_expdate);
	protup->du_flagsmask|= DU_UEXPDATE;
    }
    else
    {
	/*
	** Initialize to the empty date
	*/
	db_data.db_datatype  = DB_DTE_TYPE;
	db_data.db_prec	 = 0;
	db_data.db_length    = DB_DTE_LEN;
	db_data.db_data	 = (PTR)&protup->du_expdate;
	status = adc_getempty(sess_cb->pss_adfcb, &db_data);
	if(status)
		return status;
    }

    if (psy_cb->psy_usflag & PSY_USRPRIVS)
	{ protup->du_status = (i4) psy_cb->psy_usprivs;
	  protup->du_flagsmask |= DU_UPRIV; }
    else
	protup->du_status = 0;

    /*
    ** Save security audit options
    */
    if (psy_cb->psy_usflag & PSY_USRSECAUDIT)
    {
	if(psy_cb->psy_ussecaudit & PSY_USAU_ALL_EVENTS)
		{ protup->du_status |= DU_UAUDIT;
		  protup->du_flagsmask |= DU_UALLEVENTS; }
	else protup->du_flagsmask |= DU_UDEFEVENTS;

	if (psy_cb->psy_ussecaudit & PSY_USAU_QRYTEXT)
		{ protup->du_status |= DU_UAUDIT_QRYTEXT;
		  protup->du_flagsmask |= DU_UQRYTEXT; }
	else protup->du_flagsmask |= DU_UNOQRYTEXT;
    }

    if (psy_cb->psy_usflag & PSY_USRDEFGRP)
	MEcopy((PTR)&psy_cb->psy_usgroup,
	       sizeof(protup->du_group),
	       (PTR)&protup->du_group);
    else
	MEfill(sizeof(protup->du_group),
	       (u_char)' ',
	       (PTR)&protup->du_group);


    status = E_DB_OK;

    for (psy_usr  = (PSY_USR *)  psy_cb->psy_usrq.q_next;
	 psy_usr != (PSY_USR *) &psy_cb->psy_usrq;
	 psy_usr  = (PSY_USR *)  psy_usr->queue.q_next
	)
    {
	MEcopy((PTR)&psy_usr->psy_usrnm,
	       sizeof(protup->du_name),
	       (PTR)&protup->du_name);

	stat = rdf_call(RDF_UPDATE, (PTR)&rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
	    &psy_cb->psy_error);

    return    (status);
}

/*{
** Name: psy_aprofile - Alter profile
**
** Description:
**	This procedure alters an iiprofile tuple.  If the
**	user does not exist, the statement is aborted.
**	If the user does exist, the associated
**	iiprofile tuple is replaced.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usrq			user list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Replaces tuples in iiuser.
**
** History:
**	27-aug-93 (robf)
**          Written
*/
static DB_STATUS
psy_aprofile(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    struct
    {
	DU_PROFILE	    protuple;
	DU_PROFILE	    protuple2;
    }			usparam;
    register DU_PROFILE	*protup  = &usparam.protuple;
    PSY_USR		*psy_usr;
    DB_DATA_VALUE	db_data;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_REPLACE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_2types_mask = RDR2_PROFILE;
    rdf_rb->rdr_qrytuple    = (PTR) protup;
    rdf_rb->rdr_qtuple_count = 1;

    protup->du_flagsmask  = 0;

    if(psy_cb->psy_usflag& PSY_USRDEFPRIV)
    {
	protup->du_defpriv = psy_cb->psy_usdefprivs;
	protup->du_flagsmask|= DU_UDEFPRIV;
	/* Add default=all indicator */
	if(psy_cb->psy_usflag & PSY_USRDEFALL)
		protup->du_flagsmask|=DU_UDEFALL;

	/* if user has specified nodefault_privileges we
	** store this flag to negotiate the final status
	** of default privileges.
	*/
	if (psy_cb->psy_usflag & PSY_UNODEFPRIV)
  	  protup->du_flagsmask|= DU_UNODEFPRIV;
    }
    else
	protup->du_defpriv = 0;

    /*
    ** Expiration date, may be empty
    */
    if(psy_cb->psy_usflag& PSY_USREXPDATE)
    {
	/*
	** Date already formatted earlier
	*/
	MECOPY_CONST_MACRO((PTR)&psy_cb->psy_date, sizeof(DB_DATE), 
			(PTR)&protup->du_expdate);
	protup->du_flagsmask|= DU_UEXPDATE;
    }
    else
    {
	/*
	** Initialize to the empty date
	*/
	db_data.db_datatype  = DB_DTE_TYPE;
	db_data.db_prec	 = 0;
	db_data.db_length    = DB_DTE_LEN;
	db_data.db_data	 = (PTR)&protup->du_expdate;
	status = adc_getempty(sess_cb->pss_adfcb, &db_data);
	if(status)
		return status;
    }
    /*
    ** Check if adding, deleting, or setting privileges
    */
    protup->du_status = (i4) psy_cb->psy_usprivs;
    if (psy_cb->psy_usflag & PSY_USRAPRIVS)
	protup->du_flagsmask |=  DU_UAPRIV;
    else if (psy_cb->psy_usflag & PSY_USRDPRIVS)
	protup->du_flagsmask |=  DU_UDPRIV;
    else if (psy_cb->psy_usflag & PSY_USRPRIVS)
	protup->du_flagsmask |=  DU_UPRIV;
    else
	protup->du_status = 0;
    /*
    ** Check if updating security audit options
    */
    if (psy_cb->psy_usflag & PSY_USRSECAUDIT)
    {
	if(psy_cb->psy_ussecaudit & PSY_USAU_ALL_EVENTS)
		protup->du_flagsmask |= DU_UALLEVENTS;
	else
		protup->du_flagsmask |= DU_UDEFEVENTS;
	if (psy_cb->psy_ussecaudit & PSY_USAU_QRYTEXT)
		protup->du_flagsmask |= DU_UQRYTEXT;
	else
		protup->du_flagsmask |= DU_UNOQRYTEXT;
    }

    if (psy_cb->psy_usflag & PSY_USRDEFGRP)
    {
	MEcopy((PTR)&psy_cb->psy_usgroup,
	       sizeof(protup->du_group),
	       (PTR)&protup->du_group);
	protup->du_flagsmask|= DU_UGROUP;
    }
    else
    {
	MEfill(sizeof(protup->du_group),
	       (u_char)' ',
	       (PTR)&protup->du_group);
    }



    status = E_DB_OK;

    for (psy_usr  = (PSY_USR *)  psy_cb->psy_usrq.q_next;
	 psy_usr != (PSY_USR *) &psy_cb->psy_usrq;
	 psy_usr  = (PSY_USR *)  psy_usr->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm,
			       protup->du_name); */
	MEcopy((PTR)&psy_usr->psy_usrnm,
	       sizeof(protup->du_name),
	       (PTR)&protup->du_name);

	stat = rdf_call(RDF_UPDATE, (PTR)&rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
	    &psy_cb->psy_error);

    return    (status);
} 

/*{
** Name: psy_kuser - Destroy profile
**
** Description:
**	This procedure deletes a profile.  If the
**	profile does not exist, the statement is aborted.
**	If the user does exist, the associated
**	iiprofile tuple is deleted.
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_usrq			user list
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_INFO			Function completed with warning(s).
**	    E_DB_WARN			One ore more roles were rejected.
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removed tuples from iiuser.
**
** History:
**	27-aug-93 (robf)
**          written
*/
static DB_STATUS
psy_kprofile(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status, stat;
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DU_PROFILE		protuple;
    register DU_PROFILE	*protup = &protuple;
    PSY_USR		*psy_usr;

    /* This code is called for SQL only */

    /*
    ** Fill in the part of RDF request block that will be constant.
    */
    pst_rdfcb_init(&rdf_cb, sess_cb);
    rdf_rb->rdr_update_op   = RDR_DELETE;
    rdf_rb->rdr_status	    = DB_SQL;
    rdf_rb->rdr_2types_mask = RDR2_PROFILE;
    rdf_rb->rdr_qrytuple    = (PTR) protup;
    rdf_rb->rdr_qtuple_count = 1;

    protup->du_status 	= 0;
    protup->du_flagsmask	= 0;
    /*
    ** Pass on RESTRICT/CASCADE processsing. RESTRICT is the default,
    */
    if (psy_cb->psy_usflag & PSY_USRCASCADE)
	protup->du_flagsmask|= DU_UCASCADE;


    MEfill(sizeof(protup->du_group),
	   (u_char)' ',
	   (PTR)&protup->du_group);

    status = E_DB_OK;

    for (psy_usr  = (PSY_USR *)  psy_cb->psy_usrq.q_next;
	 psy_usr != (PSY_USR *) &psy_cb->psy_usrq;
	 psy_usr  = (PSY_USR *)  psy_usr->queue.q_next
	)
    {
	/* STRUCT_ASSIGN_MACRO(psy_usr->psy_usrnm,
			       protup->du_name); */
	MEcopy((PTR)&psy_usr->psy_usrnm,
	       sizeof(protup->du_name),
	       (PTR)&protup->du_name);

	stat = rdf_call(RDF_UPDATE, (PTR) &rdf_cb);
	status = (stat > status) ? stat : status;

	if (DB_FAILURE_MACRO(stat))
	    break;
    }

    if (DB_FAILURE_MACRO(status))
	(VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
			     &psy_cb->psy_error);

    return    (status);
} 
