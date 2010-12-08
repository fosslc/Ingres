/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <dudbms.h>
# include    <cm.h>
# include    <cs.h>
# include    <st.h>
# include    <tm.h>
# include    <ulf.h>
# include    <adf.h>
# include    <scf.h>
# include    <dmf.h>
# include    <dmucb.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <gwf.h>
# include    <gwfint.h>
# include    "gwfsxa.h"

/*
** Name: GWSXAUTIL.C	- Utility/support routines for SXA gateway
**
** Description:
**	This file contains the routines which support the SXA gateway
**
** History:
**	14-sep-92 (robf)
**	    Created
**	20-nov-92 (robf)
**	    Added handling for objectowner attribute.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (robf)
**	    Replace gwsxa_security_user by gwsxa_priv_user to check
**	    appropriate privileges.
**	    Add handling for sessionid attribute
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Delete local scf_call() declaration.
*/
GLOBALREF	GW_FACILITY	*Gwf_facility;
/*
** gwsxa_priv_user - Checks whether current user has appropriate priv.
**
** Description:
**	This does an SCF info call to determine the REAL
**	priv status. That is, people with -u (DB_ADMIN) priv
**	can't get fake access to security tables by pretending to
**	be someone else.
**
** Returns:
**	E_DB_OK		No problem, user has privilege
**	E_DB_WARN	User does not have privilege
**	E_DB_ERROR	Something went wrong
**
** History:
**	7-jul-93 (robf)
**	    Created
**      15-sep-93 (smc)
**          Made gwsxa_scf_session pass CS_SID.
*/
DB_STATUS
gwsxa_priv_user(i4 privmask)
{
        SCF_CB      scf_cb;
        SCF_SCI     sci_list;
        DB_STATUS   status;
	i4	    ustat;

        scf_cb.scf_length = sizeof(SCF_CB);
        scf_cb.scf_type = SCF_CB_TYPE;
        scf_cb.scf_facility = DB_GWF_ID;
        scf_cb.scf_session = DB_NOSESSION;
        scf_cb.scf_len_union.scf_ilength = 1;
        /* may cause lint message */
        scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) &sci_list;
        sci_list.sci_code = SCI_USTAT;
        sci_list.sci_length = sizeof(ustat);
        sci_list.sci_aresult = (PTR) &ustat;
        sci_list.sci_rlength = NULL;
        status = scf_call(SCU_INFORMATION, &scf_cb);
        if (status != E_DB_OK)
        {
	    gwsxa_error((GWX_RCB*)NULL,E_GW4056_SXA_CANT_GET_USER_PRIVS,
			SXA_ERR_INTERNAL,0);
            return(status);
        }
	/*
	**	Now check the SECURITY bit in the status
	*/
	if ( (ustat & privmask)== privmask)
	{
		/*
		**	has all required privs
		*/
		return E_DB_OK;
	}
	else
	{
		return E_DB_WARN;
	}
}

/*
**	gwsxachkrcb - Check the RCB to make sure it makes sense
**
** Description:
**	Checks a GWX_RCB pointer to see if it makes sense
**
** Returns
**	E_DB_OK		If valid
**	E_DB_ERROR 	If Invalid
**
** History:
**	14-sep-92 (robf)
**	    Created
*/
DB_STATUS
gwsxachkrcb( GWX_RCB *gwx_rcb, char *routine)
{
	if ( gwx_rcb == (GWX_RCB*)0)
	{
		/*
		**	NULL pointer, error
		*/
		/* E_GW4050 The SXA Gateway routine %0c was passed a NULL RCB. This is an
		internal error*/
		gwsxa_error(gwx_rcb,E_GW4050_SXA_NULL_RCB,
				SXA_ERR_INTERNAL, 1, 
			STlength(routine),routine);
		return ( E_DB_ERROR);
	}

	if ( gwx_rcb->xrcb_gw_id != DMGW_SEC_AUD)
	{
		/*
		**	Not our gateway, internal error
		*/
		/* E_GW4051 The SXA Gateway routine %0c was passed an RCB referring to another gateway (%1d) */
		 gwsxa_error(gwx_rcb,E_GW4051_SXA_BAD_GW_ID,
				SXA_ERR_INTERNAL, 2,
				STlength(routine), routine,
				sizeof(gwx_rcb->xrcb_gw_id),
				(PTR)&(gwx_rcb->xrcb_gw_id));
		return ( E_DB_ERROR);
	}
	return ( E_DB_OK );
}

/*
**	This is a list of the known attributes which can be registered to
**	GWF-SXA
*/
static struct SXA_ATTR{
	char        *attname;
	SXA_ATT_ID  attid;
} attlist[] = {
	"audittime",	SXA_ATT_AUDITTIME,
	"user_name",	SXA_ATT_USER_NAME,
	"real_name",	SXA_ATT_REAL_NAME,
	"userprivileges",SXA_ATT_USERPRIVILEGES,
	"objprivileges", SXA_ATT_OBJPRIVILEGES,
	"database",	SXA_ATT_DATABASE,
	"auditstatus",	SXA_ATT_AUDITSTATUS,
	"auditevent",	SXA_ATT_AUDITEVENT,
	"objecttype",	SXA_ATT_OBJECTTYPE,
	"objectname",	SXA_ATT_OBJECTNAME,
	"objectowner",  SXA_ATT_OBJECTOWNER,
	"description",	SXA_ATT_DESCRIPTION,
	"detailinfo",   SXA_ATT_DETAILINFO,
	"detailnum",    SXA_ATT_DETAILNUM,
	"sessionid",	SXA_ATT_SESSID,
	"querytext_sequence",	SXA_ATT_QRYTEXTSEQ,
	"*unknown*",	SXA_ATT_INVALID
};
/*
** Name: gwsxa_find_attbyname - Returns the attribute id
**		for an attribute name.
**
** Description:
**	This searches the list of known attributes checking for the
**	one required. The list is small, and the search is only rarely
**	done (when REGISTER currently) so we do a linear search of the
**	table
**
** Returns:
**	SXA_ATT_INVALID if none, otherwise the id
**
** History:
**	14-sep-92 (robf)
**	    Created
**	13-dec-94 (forky01)
**	    Fix bug #65975 - Use STbcompare instead of STcompare to allow
**	    case insensitive check on column names used in security audit
**	    log file.
*/
SXA_ATT_ID  
gwsxa_find_attbyname(char *attname)
{
	i4 i;

	for( i=0; attlist[i].attid!=SXA_ATT_INVALID; i++)
	{
		/*
		** Case insensitive compare to allow FIPS mode to work
		*/
		if ( STbcompare((char *)attlist[i].attname,0,attname,
				0,TRUE)==0 )
		{
			/*
			**	Found so stop looking
			*/
			break;
		}
	}
	return attlist[i].attid;
}

/*
** Name: gwsxa_find_attbyid - Returns the attribute name
**		for an attribute id.
**
** Description:
**	This searches the list of known attributes checking for the
**	one required. The list is small, and the search is only rarely
**	done so we do a linear search of the table
**
** Returns:
**	name of the attribute id, the "unknown" string otherwise.
**
** History:
**	14-sep-92 (robf)
**	    Created
*/
char*
gwsxa_find_attbyid(SXA_ATT_ID attid)
{
	i4 i;

	for( i=0; attlist[i].attid!=SXA_ATT_INVALID; i++)
	{
		if( attlist[i].attid==attid)
		{
			/*
			**	Found so stop looking
			*/
			break;
		}
	}
	return attlist[i].attname;
}
/*
** Name: gwsxa_scf_session - Return the session SCF session 
**       identifier.
**
** Returns:
**	E_DB_OK - No error
**	Other   - Something went wrong
**
** History:
**	14-sep-92 (robf)
**	    Created
*/
DB_STATUS 
gwsxa_scf_session(CS_SID *where)
{
        SCF_CB      scf_cb;
        SCF_SCI     sci_list;
        DB_STATUS   status;

        scf_cb.scf_length = sizeof(SCF_CB);
        scf_cb.scf_type = SCF_CB_TYPE;
        scf_cb.scf_facility = DB_SCF_ID; /* Get the SCF session id */
        scf_cb.scf_session = DB_NOSESSION;
        scf_cb.scf_len_union.scf_ilength = 1;
        /* may cause lint message */
        scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) &sci_list;
        sci_list.sci_code = SCI_SCB;
        sci_list.sci_length = sizeof(where);
        sci_list.sci_aresult = (PTR)where;
        sci_list.sci_rlength = NULL;
        status = scf_call(SCU_INFORMATION, &scf_cb);
        if (status != E_DB_OK)
        {
	    gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	    gwf_error(E_GW0302_SCU_INFORMATION_ERROR, GWF_INTERR, 0);
        }
        return(status);
}

/*
** Name: gwsxa_zapblank - trim blanks off a (possibly non-terminated)
**	 string. 
** 
** Description:
**	This routine walks through a (possibly non-NULL-terminated) string
**	up to maxlen bytes looking for a space. It replaces the space with
**      a NULL value. The use of this function is primarily to convert
**	a blank-padded string to a regular NULL-terminated string for 
**	error reporting.
**
** Returns:
**	Nothing.
**
** History:
**	14-sep-92 (robf)
**	    Created
*/
VOID
gwsxa_zapblank(char *str,i4 maxlen)
{
	i4 i;

	/*
	**	If passed an empty string then just return
	*/
	if (!str)
		return;

	for( i=0; i<maxlen; i++)
	{
		if(CMspace(str))
			break;
		CMnext(str);
	}
	if( i!=maxlen)
		*str= EOS;
}



/*
** Name: gwsxa_rightTrimBlank - trim blanks off tails of a (possibly non-terminated)
**	 string. 
** 
** Description:
**	This routine walks through a (possibly non-NULL-terminated) string
**	up to maxlen bytes looking for last non-space character. It then
**      steps over that character and replaces the ensuing space with a NULL.
**
**      This function differs from gwsxa_zapblank() above in that it will
**      allow embeded spaces to remain within the string.
**
** Returns:
**	Nothing.
**
** History:
**	04-Dec-2008 (coomi01) b121323
**	    Created
*/
VOID
gwsxa_rightTrimBlank(char *str, i4 maxlen)
{
    char *notBlankPtr;
    char *currPtr;
    char *endPtr;

    /*
    **  If passed an empty string then just return
    */
    if (!str || (maxlen<=0) )
	return;

    /*
    ** Init the pointers 
    */
    currPtr  = str;
    endPtr   = &str[maxlen];
    notBlankPtr = NULL;

    /* 
    ** Careful, The string maybe dbyte
    */
    while ( currPtr < endPtr )
    {
	if ( (*currPtr != EOS) && !CMspace(currPtr) )
	{
	    /* Save non-space position */
	    notBlankPtr = currPtr;
	}

	/*
	** Step forward
	*/
	CMnext(currPtr);
    }

    if ( NULL != notBlankPtr )
    {
	/* 
	** Step foward, over last non-blank
	*/
	CMnext(notBlankPtr);

	/* 
	** But do not write beyound buffer end
	*/
	if (notBlankPtr < endPtr)
	{
	    *notBlankPtr = EOS;
	}
    }
    else
    {
	/* 
	** String entirely composed of spaces 
	*/
	*str = EOS;
    }
}
