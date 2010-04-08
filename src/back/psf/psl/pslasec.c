/*
**Copyright (c) 2004 Ingres Corporation
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
** Name: PSLASEC.C:	this file contains functions used in semantic actions
**			for ALTER SECURITY_AUDIT
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_AS<number>_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
** Description:
**	this file contains functions used by both SQL and QUEL grammar in
**	parsing of the ALTER SECURITY_AUDIT statement
**
**	Routines:
**	    psl_as1_nonkeyword 	   	 - actions for nonkeyword
**	    psl_as2_with_nonkw_eq_sconst - actions for WITH nkw=sconst
**	    psq_as3_alter_secaud	 - actions for end of statement
*/

/*
** Forward/external declarations
*/

GLOBALREF PSF_SERVBLK *Psf_srvblk;

/*
** Name: psl_as1_nonkeyword
**
** Description: 
**	Handles the nonkeyword action of ALTER SECURITY_AUDIT
**
** Inputs:
**	psy_cb	
**	
**	word	The nonkeyword
**
** Outputs:
**	None
**
** History
**	29-jul-93 (robf)
**		First created
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp(),
**	    psl_rptqry_tblids(), psl_cons_text(), psl_gen_alter_text(),
**	    psq_tmulti_out(), psq_1rptqry_out(), psq_tout().
**	17-Jan-2001 (jenjo02)
**	    Short-circuit calls to psy_secaudit() if not C2SECURE.
**      15-Apr-2003 (bonro01)
**          Added include <psyaudit.h> for prototype of psy_secaudit()
[@history_template@]...
*/
DB_STATUS
psl_as1_nonkeyword(
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	char 	    *word
)
{
	DB_STATUS   status;
	i4	    err_code;
        DB_DATA_VALUE	db_data;

	if (STcasecmp(word, "SUSPEND" ) == 0)
	{
	    /* Indicate SUSPEND was specified */
	    psy_cb->psy_auflag |= PSY_AUSUSPEND;
	}
	else if (STcasecmp(word, "RESUME" ) == 0)
	{
	    /* Indicate RESUME was specified */
	    psy_cb->psy_auflag |= PSY_AURESUME;
	}
	else if (STcasecmp(word, "STOP" ) == 0)
	{
	    /* Indicate STOP was specified */
	    psy_cb->psy_auflag |= PSY_AUSTOP;
	}
	else if (STcasecmp(word, "RESTART" ) == 0)
	{
	    /* Indicate RESTART was specified */
	    psy_cb->psy_auflag |= PSY_AURESTART;
	}
	else
	{
	    (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 2,
		(i4) sizeof("ALTER SECURITY_AUDIT")-1, 
			"ALTER SECURITY_AUDIT",
		(i4) STtrmwhite(word), word);
	    return (E_DB_ERROR);    /* non-zero return means error */
	}
	return E_DB_OK;
}


/*
** Name: psl_as2_with_nonkw_eq_sconst
**
** Description: 
**	Handles the WITH nonkw EQUAL sconst action of ALTER SECURITY_AUDIT
**
** Inputs:
**	psy_cb	
**	
**	word1	The nonkeyword
**
**	word2	The name/sconst
**
** Outputs:
**	None
**
** History
**	29-jul-93 (robf)
**		First created
**
*/
DB_STATUS
psl_as2_with_nonkw_eq_sconst(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *word2
)
{
	i4	    err_code;
	DB_STATUS   status;

	if (STcasecmp(word1, "AUDIT_LOG" ) == 0)
	{
	    if (psy_cb->psy_auflag & PSY_AUSETLOG)
	   {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("ALTER SECURITY_AUDIT")-1, 
			"ALTER SECURITY_AUDIT",
		    (i4)sizeof("AUDIT_LOG")-1, "AUDIT_LOG");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }

	   /* Indicate AUDIT_LOG was specified */
	   psy_cb->psy_auflag |= PSY_AUSETLOG;

	   status = psf_malloc(cb, &cb->pss_ostream, (i4) STlength(word2)+1, 
		(PTR*)&psy_cb->psy_aufile,  &psq_cb->psq_error);
	   if (DB_FAILURE_MACRO(status))
		    return (status);
	   STcopy(word2, psy_cb->psy_aufile);	
	}
	else
        {
            (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
                &psq_cb->psq_error, 2,
                (i4) sizeof("ALTER SECURITY_AUDIT")-1, 
			"ALTER SECURITY_AUDIT",
                (i4) STtrmwhite(word1), word1);
	    return (E_DB_ERROR);    /* non-zero return means error */
        }
	return E_DB_OK;
}

/*
** Name: psl_as3_alter_secaud
**
** Description: 
**	This completes processing the alter_secaud statement
**
** Inputs:
**	psy_cb	
**
** Outputs:
**	None
**
** History
**	29-jul-93 (robf)
**		First created
**      30-jan-95 (newca01)
**              Added check to make sure some parameter was entered 
**              following "alter security_audit".
**	09-Sep-2008 (jonj)
**	    4th param of psf_error() is i4 *, not DB_ERROR *.
*/
DB_STATUS
psl_as3_alter_secaud(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb
)
{
	DB_STATUS status=E_DB_OK;
	DB_ERROR  err;
	i4	  err_code;

        PSY_CB    *psy_cb; 
	i4    lineno; 

	psy_cb = (PSY_CB *)cb->pss_object; 

	if ((psy_cb->psy_auflag & ( PSY_AUSETLOG | PSY_AUSUSPEND | 
	     PSY_AURESUME | PSY_AUSTOP | PSY_AURESTART ) ) == 0)
	{ 
		/* error - no parameters entered  */
		lineno = cb->pss_lineno; 
		(VOID)  psf_error(4120L, 0L, PSF_USERERR, 
			 &err_code, &psq_cb->psq_error, 2, 
			 (i4) sizeof(lineno), &lineno, 
			 (i4) sizeof("EOF")-1, 
			 "EOF"); 
	 	status=E_DB_ERROR; 
		return(status);
	}

        /* Make sure user is authorized */

        if (cb->pss_ustat & DU_UALTER_AUDIT)
	    return(E_DB_OK);

	/* User not authorized */

	/*
	** Audit the failure to ALTER SECURITY AUDITing
	*/

	if ( Psf_srvblk->psf_capabilities & PSF_C_C2SECURE )
	    status = psy_secaudit(FALSE, cb,
			    ERx("SECURITY_AUDIT"), (DB_OWN_NAME *)NULL,
			    sizeof(ERx("SECURITY_AUDIT"))-1, 
			    SXF_E_SECURITY,
			    I_SX272D_ALTER_SECAUDIT, 
			    SXF_A_FAIL | SXF_A_ALTER,
			    &err);

        (VOID) psf_error(E_US18D3_6355_NOT_AUTH, 0L,
                         PSF_USERERR, &err_code, &psq_cb->psq_error, 1,
                         sizeof("ALTER SECURITY_AUDIT")-1,
                         "ALTER SECURITY_AUDIT");

	return(E_DB_ERROR);
}
