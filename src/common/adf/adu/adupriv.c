/*
**  Copyright (c) 2004 Ingres Corporation
*/
/*
NO_OPTIM = su4_u42 su4_cmw
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <cv.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adftrace.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <adffiids.h>

/**
** Name:  ADUPRIV.C -    Routines for managing privilges.
**
** Description:
**	  This file defines routines for managing privileges.
**
**	This file defines:
**
**	    adu_sess_priv()	    - handles the session_priv() call
**	    adu_iitblstat()	    - handles the iitblstat() call
**
**  Function prototypes defined in ADUINT.H file.
**
**  History:
**	8-sep-93 (robf)
**	    Created.
**	13-oct-93 (robf)
**          Add st.h
**	22-oct-93 (robf)
**          Add adu_iitblstat()
**	14-feb-94 (robf)
**          Add NO_OPTIM hint for su4_u42/cmw ACC compilers
**	8-jul-94 (robf)
**          Removed include of dbdbms.h/dudbms.h as privilege masks are
**	    now in iicommon.h. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* Forward/static declarations */
static struct {
	i4 privmask;
	char	*name;
} privlist[] = {
	DU_UAUDITOR,	 "AUDITOR",	
	DU_UAUDIT,	 "AUDIT_ALL",	
	DU_UCREATEDB,	 "CREATEDB",	
	DU_UINSERTDOWN,	 "INSERT_DOWN",	
	DU_UINSERTUP,	 "INSERT_UP",	
	DU_UMONITOR,	 "MONITOR",	
	DU_UOPERATOR,	 "OPERATOR",	
	DU_USECURITY,	 "SECURITY",	
	DU_UTRACE,	 "TRACE",
	DU_UWRITEDOWN,	 "WRITE_DOWN",
	DU_UWRITEFIXED,	 "WRITE_FIXED",	
	DU_UWRITEUP,	 "WRITE_UP",	
	DU_UALTER_AUDIT, "MAINTAIN_AUDIT",
	DU_UMAINTAIN_USER, "MAINTAIN_USERS",
	DU_USYSADMIN,	 "MAINTAIN_LOCATIONS",	
	DU_UAUDIT_QRYTEXT, "AUDIT_QUERYTEXT",	
	0, 	NULL
};

/*{
** Name: adu_sesspriv -	Internal routine for returning session
**				privileges
**
** Description:
**	This function takes a privilege name and returns Y, N or R
**	depending on whether or not the session has, can get, or doesn't
**	have the privilege.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	privname			DB_DATA_VALUE describing the 
**					privilege data value.
**	    .db_data			Ptr to a internal  struct.
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-sep-93 (robf)
**	    Created.
**	10-mar-94 (robf)
**          Make sure tmpstr is NULL-terminated otherwise CVla could fail
**      19-sep-1995 (whitfield) bug 71502
**          ANSI does not guarantee that string literals can be written to.
**	14-oct-2007 (gupsh01)
**	    Pass in the type of source string to the adu_movestring 
**	    routine.
*/
/*ARGSUSED*/
DB_STATUS
adu_sesspriv(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*str_dv,
DB_DATA_VALUE	*rdv)
{
    DB_STATUS status	=E_DB_OK;
    i4  		dvlen;
    char 		*dvaddr;
    DB_DATA_VALUE	usn_dv;
    DB_DATA_VALUE	tusn_dv;
    char		tmpstr[GL_MAXNAME+1];
    DEFINE_DB_TEXT_STRING(maxstr,"MAX_PRIV_MASK")
    DEFINE_DB_TEXT_STRING(curstr,"CURRENT_PRIV_MASK")
    i4		maxpriv;
    i4		curpriv;
    i4			cmp;
    char		*result;
    DB_STATUS		dbstat;
    i4			p;

    /*
    ** Get address values for where values go.
    */
    if ((dbstat = adu_3straddr(adf_scb, str_dv, &dvaddr)) != E_DB_OK)
	return (dbstat);
    /*
    ** Total string length of this variable.
    */
    switch (str_dv->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
        dvlen = str_dv->db_length;
        break;

      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VCH_TYPE:
        dvlen = str_dv->db_length - DB_CNTSIZE;
        break;
      default:
	/*
	** non-text type, its not found
	*/
	return adu_error(adf_scb, E_AD1097_BAD_DT_FOR_SESSPRIV, 0);
    }
    for(;;)
    {
	bool	found;
	/*
	** First try to find the request privilege
	*/
	found=FALSE;
	for(p=0; privlist[p].name!=NULL; p++)
	{
		cmp=STbcompare(privlist[p].name,0, dvaddr, dvlen, TRUE);
		if(cmp==0)
		{
			found=TRUE;
			break;
		}
	}
	if(!found)
	{
		/*
		** Unknown privilege, so we don't have it
		*/
		result="N";
		break;
	}
	/*
	** Output is CHAR for DBMSINFO
	*/
	tusn_dv.db_datatype = DB_CHA_TYPE;
	tusn_dv.db_prec	   = 0;
	tusn_dv.db_length   = sizeof(tmpstr);
	tusn_dv.db_data     = tmpstr;

	/*
	** Get current privs
	*/
	usn_dv.db_datatype = DB_VCH_TYPE;
	usn_dv.db_prec	   = 0;
	usn_dv.db_length   = sizeof(curstr);
	usn_dv.db_data     =  (PTR)&curstr;


	if ( adu_dbmsinfo(adf_scb, &usn_dv, &tusn_dv)!=OK)
		return adu_error(adf_scb, E_AD1096_SESSION_PRIV, 0);

	/* Make sure NULL terminated for CVla() */
	tmpstr[sizeof(tmpstr)-1]='\0';
	/*
	** Convert value to integer
	*/
	if(CVal(tmpstr, &curpriv)!=OK)
	{
		return adu_error(adf_scb, E_AD1096_SESSION_PRIV, 0);
	}
	if(privlist[p].privmask & curpriv)
	{
		/*
		** Current priv is set, so we are done
		*/
		result="Y";
		break;
	}

	/*
	** Request maximum privs label
	*/
	usn_dv.db_length   = sizeof(maxstr);
	usn_dv.db_data     = (PTR)&maxstr;



	if ( adu_dbmsinfo(adf_scb, &usn_dv, &tusn_dv)!=OK)
		return adu_error(adf_scb, E_AD1096_SESSION_PRIV, 0);

	/* Make sure NULL terminated for CVla() */
	tmpstr[sizeof(tmpstr)-1]='\0';
	/*
	** Convert value to integer
	*/
	if(CVal(tmpstr, &maxpriv)!=OK)
	{
		return adu_error(adf_scb, E_AD1096_SESSION_PRIV, 0);
	}
	if(privlist[p].privmask & maxpriv)
		result="R";
	else
		result="N";
	break;
    }
    /*
    ** Convert result back into final value
    */
    if ((dbstat = adu_3straddr(adf_scb, rdv, &dvaddr)) != E_DB_OK)
	return (dbstat);

    switch (rdv->db_datatype)
    {
      case DB_CHA_TYPE:
      case DB_CHR_TYPE:
      case DB_TXT_TYPE:
      case DB_LTXT_TYPE:
      case DB_VCH_TYPE:
	dbstat=adu_movestring(adf_scb, (u_char*)result, 1, str_dv->db_datatype, rdv);
        break;

      default:
	/*
	** non-text type, error
	*/
        return(adu_error(adf_scb, E_AD1096_SESSION_PRIV, 0));
    }
    return status;
}

/*{
** Name: adu_iitblstat -	Internal routine for mapping table statistics
**
** Description:
**	This function returns either its input or 0 depending on whether
**	the current session has TABLE_STATISTICS privileges or not.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	statvalue			DB_DATA_VALUE describing the 
**					statistics data value.
**	    .db_data			Ptr to a internal  struct.
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-oct-93 (robf)
**	    Created.
**	26-oct-93 (robf)
**          Correct typo in i4 assignment
**      19-sep-1995 (whitfield) bug 71502
**          ANSI does not guarantee that string literals can be written to.
**	29-feb-96 (morayf)
**	    ANSI does not guarantee that char arrays will be aligned to a
**	    word boundary. Thus assigning a u_i2 value to such an address
**	    can (and on the Pyramid NILE does) bus error.
**	    Solution: since we now use grown-up ANSI C, let's make use of
**	    its ability to initialise structure contents at compile time.
**	    Since we are using our own DB_TEXT_STRING-like type, let's
**	    put a new macro which allocates just enough memory and copies
**	    in the required string to the proper char array field, and put
**	    this macro (DEFINE_DB_TEXT_STRING) in iicommon.h too.
**	    Also eliminate literal size assignments: the compiler knows
**	    how big the string is, and the VARCHAR size is just this size
**	    plus the size of the count field (ie. a u_i2 = 2 bytes).
**	    Only thing which could break this is a non-aligned structure
**	    or an access to the string which assumes it is 2 bytes offset
**	    from the start of the structure (this would be naughty).
**      23-may-2007 (smeke01) b118342/b118344
**	    Work with i8.
*/
/*ARGSUSED*/
DB_STATUS
adu_iitblstat(
ADF_CB		*adf_scb,
DB_DATA_VALUE	*idv,
DB_DATA_VALUE	*rdv)
{
    DB_STATUS status	=E_DB_OK;
    DB_DATA_VALUE	usn_dv;
    DB_DATA_VALUE	tusn_dv;
    char		tmpstr[GL_MAXNAME+1];
    DEFINE_DB_TEXT_STRING(tblstatstr,"TABLE_STATISTICS")
    i4		result;
    i8		i8temp;

    for(;;)
    {
	tusn_dv.db_datatype = DB_CHA_TYPE;
	tusn_dv.db_prec	   = 0;
	tusn_dv.db_prec	   = 0;
	tusn_dv.db_length   = sizeof(tmpstr);
	tusn_dv.db_data     = tmpstr;
	/*
        ** Get table_stats status
	*/
	usn_dv.db_datatype = DB_VCH_TYPE;
	usn_dv.db_prec	   = 0;
	usn_dv.db_length   = sizeof(tblstatstr);
	usn_dv.db_data     = (PTR)&tblstatstr;


	if ( adu_dbmsinfo(adf_scb, &usn_dv, &tusn_dv)!=OK)
		return adu_error(adf_scb, E_AD1096_SESSION_PRIV, 0);

	 /*
	 ** Check if set to 'Y', meaning session has table_stats privilege
	 */
	 if(*tmpstr=='Y')
	 {
	    switch (idv->db_length)
	    {
		case 1:
		    result =  I1_CHECK_MACRO( *(i1 *) idv->db_data);
		    break;
		case 2: 
		    result =  *(i2 *) idv->db_data;
		    break;
		case 4:
		    result =  *(i4 *) idv->db_data;
		    break;
		case 8:
		    i8temp  = *(i8 *)idv->db_data;

		    /* limit to i4 values */
		    if ( i8temp > MAXI4 || i8temp < MINI4LL )
			return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "tblstat result overflow"));

		    result = (i4) i8temp;
		    break;
		default:
		    return (adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0, "tblstat result length"));
	    }
	 }
	 else
		result=0;
	 break;
    }
    MECOPY_CONST_MACRO((PTR)&result, sizeof(i4), (PTR)rdv->db_data);

    return status;
}
