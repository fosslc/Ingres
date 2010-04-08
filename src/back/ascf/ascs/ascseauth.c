/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cm.h>
#include    <cv.h>
#include    <ex.h>
#include    <lo.h>
#include    <pc.h>
#include    <st.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <me.h>
#include    <tr.h>
#include    <er.h>
#include    <cs.h>
#include    <lk.h>
#include    <gcm.h>
#include    <gcn.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <usererror.h>

#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <opfcb.h>
#include    <ddb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <tm.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <lg.h>

#include    <duf.h>
#include    <dudbms.h>
#include    <copy.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <sc0a.h>

#include    <scserver.h>

#include    <urs.h>
#include    <ascs.h>

#include    <cui.h>

/**
**
**  Name: SCSEAUTH.C - External authentication services
**
**  Description:
**      This file contains external authentication services.
**
**	    ascs_check_extern_password    - Check an external password
**
**  History:
**      15-mar-94 (robf)
**          Created.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	16-jul-96 (stephenb)
**	    Add effective user to parameter list and pass to authentication
**	    server (fix primarily for ICE authentication, but useful 
**	    information for the user anyway).
**	03-Aug-1998 (fanra01)
**	    Add casting to parameters to remove compiler warnings on unix.
**      15-Jan-1999 (fanra01)
**          Removed scs_trimwhite and renamed scs_check_extern_password.
**      12-Feb-99 (fanra01)
**          Replaced previously deleted scs_trimwhite and renamed to
**          ascs_trimwhite.
**      23-Feb-99 (fanra01)
**          Hand integration of change 439795 by nanpr01.
**          Changed the sc0m_deallocate to pass pointer to a pointer.
**      23-jun-99 (hweho01)
**          Added *CS_find_scb() function prototype. Without the 
**          explicit declaration, the default int return value   
**          for a function will truncate a 64-bit address
**          on ris_u64 platform.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Removed CS_find_scb() prototype, now correctly typed
**	    in cs.h.
**	10-Mar-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
*/
/*
**  Forward and/or External function references.
*/

/*
** Definition of all global variables owned by this file.
*/

GLOBALREF SC_MAIN_CB          *Sc_main_cb; /* Central structure for SCF */

FUNC_EXTERN       i4      gcm_put_int();
FUNC_EXTERN       i4      gcm_put_str();
FUNC_EXTERN       i4      gcm_get_long();
FUNC_EXTERN       i4      gcm_get_int();

static void	 cep_complete(SCD_SCB*);
/*}
** Name: SCS_REBUFF - Structure to hold external request info
**
** Description:
**	This structure is used to hold external request info.
**	
** History:
**	9-mar-94  (robf)
**        Created
*/
typedef struct _SCS_REBUFF SCS_REBUFF;

struct _SCS_REBUFF
{
	SCS_ROLE	*scrl_next;
	SCS_ROLE	*scrl_prev;
	i4		scrl_length;
	i2		scrl_type;
#define			SCR_REBUFF_TYPE	0x422	/* Internal ID for structure*/
      	i2		scrl_s_reserved;
	i4		scrl_l_reserved;
	i4		scrl_owner;
	i4		scrl_tag;
#define                 SCRE_TAG         CV_C_CONST_MACRO('s','c','r','e')
#define   RQ_BUFF_SIZE 4096
	char		work_buff[RQ_BUFF_SIZE];
	char		gcmvalue[4+GL_MAXNAME+GL_MAXNAME+GL_MAXNAME+sizeof(DB_PASSWORD)+1];
} ;

/*
** Name: ascs_check_external_password 
**
** Description:
**	Check the external password for a role
**
** Inputs:
**	scb 		- SCB
**
** 	authname 	- Role/User name being checked
**
**	password 	- Role/User password
**
**	auth_role	- TRUE if authenticating a role
**
** Returns:
**	E_DB_OK - Access is allowed
**
**	E_DB_WARN - Access is denied
**
**	E_DB_ERROR - Failure determining access
**
** History:
**	9-mar-94 (robf)
**		Created
**	11-mar-94 (robf)
**           Distinguish between access disallowed and some other
**	     error. Add SCF activity in case auth. server take a while
**	15-mar-94 (robf)
**	     Rework to allow for user or role authentication, add
**	     parameter auth_role. Make routine external, 
**	     name scs_check_external_password, and move to scseauth.c
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	16-jul-96 (stephenb)
**	    Add effective user to parameter list and pass to authentication
**	    server (fix primarily for ICE authentication, but useful 
**	    information for the user anyway).
**	03-Aug-1998 (fanra01)
**	    Add casting to remove compiler warning on unix.
**      23-Feb-99 (fanra01)
**          Hand integration of change 439795 by nanpr01.
**          Changed the sc0m_deallocate to pass pointer to a pointer.
*/
DB_STATUS
ascs_check_external_password (
	SCD_SCB *scb, 
	DB_OWN_NAME *authname, 
	DB_OWN_NAME *e_authname,
	DB_PASSWORD *password,
	bool	    auth_role
)
{
    DB_STATUS 	status=E_DB_WARN;
    STATUS    	cl_stat;
    STATUS    	local_status;
    char 	pmname[64];
    char 	*pmvalue;
    char  	target[64];
    GCA_FS_PARMS fs_parms; 
    SCS_REBUFF   *re_buff=NULL;
    char	 *work_buff;
    i4		 resume=0;
    CS_SID	 sid;
    PTR		 save_data_area;
    char	 *q;
    char	 *gcmvalue;
    i4	 	 error_index;
    i4	 error_status;
    char	 *act_string;
    DB_OWN_NAME  *username;
    char	 blank_string[32] = "";

    for(;;)
    {
	
	if(auth_role)
		act_string="Validating external role password";
	else
		act_string="Validating external user password";

	MEcopy((PTR)act_string, STlength(act_string),
				scb->scb_sscb.sscb_ics.ics_act1);
	scb->scb_sscb.sscb_ics.ics_l_act1 = STlength(act_string);
	/*
	** Find the authentication mechanism for this role
	*/
	if(auth_role)
	{
	    STprintf(pmname, "ii.$.secure.role_auth.%*s",
		ascs_trimwhite(sizeof(*authname),
			(char*)authname), 
		(char*)authname);
	}
	else
	{
	    STprintf(pmname, "ii.$.secure.user_auth.%*s",
		ascs_trimwhite(sizeof(*authname),
			(char*)authname), 
		(char*)authname);
	}
	cl_stat=PMget(pmname, &pmvalue);
	if(cl_stat!=OK)
	{
	        sc0e_put(E_SC035A_EXTPWD_NO_AUTH_MECH, 0, 1,
		         ascs_trimwhite(sizeof(*authname),
			      (char*)authname),
			(PTR)authname,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0 );
		status=E_DB_ERROR;
		break;
	}
	/*
	** Now we know the target mechanism, set up target destination
	**
	** Legal values are either 'value' which maps to value/authsvr
	** or @value which is sent untranslated. 
	*/
	if(*pmvalue!='@')
		STprintf(target,"%s/authsvr",pmvalue);
	else
		STprintf(target,"%s", pmvalue+1);
	/*
	** Allocate buffer for processing the request
	*/
        status = sc0m_allocate(SCU_MZERO_MASK,
		    (i4)sizeof(SCS_REBUFF),
		    DB_SCF_ID,
		    (PTR)SCS_MEM,
		    SCRE_TAG,
		    (PTR*)&re_buff);
        if(status!=E_DB_OK)
        {
		/* sc0m_allocate puts error codes in return status */
		sc0e_0_put(status, 0);
		status=E_DB_ERROR;
		break;
    	}
	work_buff= re_buff->work_buff;
	gcmvalue = re_buff->gcmvalue;

	CSget_sid(&sid);

	save_data_area=work_buff;
	/*
	** Build the fastselect request
	*/
	fs_parms.gca_user_name=NULL;
	fs_parms.gca_password=NULL;
	fs_parms.gca_account_name=NULL;
	fs_parms.gca_completion=cep_complete;
	fs_parms.gca_closure=NULL;
	fs_parms.gca_peer_protocol=GCA_PROTOCOL_LEVEL_61;
	fs_parms.gca_partner_name=target;
	fs_parms.gca_modifiers=GCA_RQ_GCM;
	fs_parms.gca_buffer=work_buff;
	fs_parms.gca_b_length=RQ_BUFF_SIZE;
	fs_parms.gca_message_type=GCM_SET;

	q=save_data_area;
	q += sizeof(i4);	/* Past error_status */
	q += sizeof(i4);	/* Past error_index */
	q += sizeof(i4);	/* Past future[0] */
	q += sizeof(i4);	/* Past future[1] */
	q += gcm_put_int(q, -1);/* Client perms */
	q += gcm_put_int(q, 1); /* Row count */
	q += gcm_put_int(q, 1); /* Element count */
	if(auth_role)
	    q += gcm_put_str(q, "exp.scf.auth_server.role_authenticate"); /* Class id */
	else
	    q += gcm_put_str(q, "exp.scf.auth_server.user_authenticate"); /* Class id */
	q += gcm_put_str(q, "0"); 		  /* Instance */
	/*
	** Now build the value.
	** This consists of:
	** Role:
	**  flag 	 - 4 byte integer, currently 0
	**  real user 	 - 32 byte blank-padded
	**  effective user - 32 byte blank-padded
	**  role         - 32 byte blank-padded
	**  password     - 24 byte blank-padded
	** User:
	**  flag 	 - 4 byte integer, currently 1
	**  real user 	 - 32 byte blank-padded
	**  effective user - 32 byte blank-padded
	**  password     - 24 byte blank-padded
	** These are all padded into a single GCM value.
	** Note: This assumes no NULL values in any of the fields.
	*/
	if(auth_role)
	{
	    STprintf(gcmvalue,"%-4.4s%32.32s%32.32s%32.32s%-24.24s",
		"0",
		(char*)&scb->scb_sscb.sscb_ics.ics_rusername,
		e_authname ? (char*)e_authname : blank_string,
		(char*)authname,
		(char*)password);
	}
	else
	{
	    STprintf(gcmvalue,"%-4.4s%32.32s%32.32s%-24.24s",
		"1",
		(char*)authname,
		e_authname ? (char*)e_authname : blank_string,
		(char*)password);
	}
	q += gcm_put_str(q, gcmvalue );	  	  /* Value */
	fs_parms.gca_msg_length=q-save_data_area;
	break;
    }

    /*
    ** Now do the fastselect
    */
    resume=0;
    if(status==E_DB_OK)
    do 
    {

	cl_stat=IIGCa_call(GCA_FASTSELECT,
			(GCA_PARMLIST *)&fs_parms,
			GCA_ASYNC_FLAG | GCA_ALT_EXIT | resume,
			(PTR)CS_find_scb(sid),
			(i4) -1,
			&local_status);
        if (cl_stat != OK)
        {
	    sc0e_0_put(cl_stat, 0);
	    sc0e_0_put(E_SC0357_EXTPWD_GCA_FASTSELECT, 0);
	    status=E_DB_ERROR;
	    break;
        }
        else
        {
            /* wait for completion routine to wake us */
            cl_stat = CSsuspend(CS_BIO_MASK, 0, 0);
            if (cl_stat != OK)
            {
	        sc0e_0_put(cl_stat, 0);
	        sc0e_0_put(E_SC0356_EXTPWD_GCA_CSSUSPEND, 0);
	        status=E_DB_ERROR;
		resume=0;
		break;
            }
            else                /* completion handler called */
            {
                switch ( cl_stat = fs_parms.gca_status )
                {
                case OK:

                    resume = 0;
                    break;

                case E_GCFFFE_INCOMPLETE:

                    resume = GCA_RESUME;
                    break;

                case E_GC0032_NO_PEER:
		case E_GC0138_GCN_NO_SERVER:
		case E_GC0139_GCN_NO_DBMS:

                    resume = 0;

		    sc0e_put(E_SC0355_EXTPWD_GCA_NOPEER, 0, 2,
				 ascs_trimwhite(sizeof(*authname),
				      (char*)authname),
				(PTR)authname,
				STlength(target),
				(PTR)target,
		     		0, (PTR)0,
		     		0, (PTR)0,
		     		0, (PTR)0,
		     		0, (PTR)0 );
		    status=E_DB_ERROR;
                    break;

                default:

                    resume = 0;

	       	    sc0e_0_put(cl_stat, 0);
		    sc0e_0_put(E_SC0354_EXTPWD_GCA_COMPLETION, 0);
		    status=E_DB_ERROR;
                    break;
                }
            }
        }
    } while( resume != 0 );
    /*
    ** Check if select worked OK, if not skip to the end
    */
    if(status==E_DB_OK)
    {
	/*
	** Got result of fastselect, so unpack and see what happened
	*/
	q=save_data_area;
	q+=gcm_get_int(q, &error_status);
	q+=gcm_get_int(q, &error_index);
	if(error_status!=0)
	{
	   if(error_index==-1)
	   {
	       	sc0e_0_put(error_status, 0);
		sc0e_0_put(E_SC0359_EXTPWD_GCM_ERROR, 0);
	        status=E_DB_ERROR;
	   }
	   else
	   {
		/* Access is denied */
		status=E_DB_WARN;
	    }
	}
	else
	{
		/* Access is allowed */
		status=E_DB_OK;
	}
    }

    /* Reset activity */
    scb->scb_sscb.sscb_ics.ics_l_act1 = 0;

    /*
    ** Free buffer if allocated
    */
    if(re_buff)
    {
	if(sc0m_deallocate(0, (PTR*) &re_buff)!=E_DB_OK)
	{
	    sc0e_0_put(status, 0);
	}
    }
    return status;
}

/*
** Name: cep_complete
**
** Description:
**	Completion routine for the cep fastselect, simply resumes the
**	requesting thread.
**
** Inputs:
**	sid - requesting session id
**
** Outputs:
**	none
**
** Side Effects:
**      The CSsuspended thread is resumed.
**
** History:
**	9-mar-94 (robf)
**          Created
*/
static VOID
cep_complete(SCD_SCB *scb)
{
	CSresume((CS_SID)scb->cs_scb.cs_self);
	return;
}

/*{
** Name: ascs_trimwhite	- Trim trailing white space.
**
** Description:
**	Utility to trim white space off of trace and error names.
**	It is assumed that no nested blanks are contained within text.
**
** Inputs:
**      len		Max length to be scanned for white space
**	buf		Buf holding char data (may not be null-terminated)
**
** Outputs:
**	Returns:
**	    Length of non-white part of the string
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	7-dec-93 (robf)
**	    Written for alarm events
*/
i4
ascs_trimwhite(
i4	len,
char	*buf )
{
    register char   *ep;    /* pointer to end of buffer (one beyond) */
    register char   *p;
    register char   *save_p;

    if (!buf)
    {
       return (0);
    }

    for (save_p = p = buf, ep = p + len; (p < ep) && (*p != EOS); )
    {
        if (!CMwhite(p))
            save_p = CMnext(p);
        else
            CMnext(p);
    }

    return ((i4)(save_p - buf));
} /* scs_trimwhite */
