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
** Name: PSLUSER.C:	this file contains functions used in semantic actions
**			for CREATE/ALTER USER/PROFILE/ROLE statements. 
**			SET SESSION also uses this file since many of the 
**			attributes (e.g. privileges) are the same in both
**			cases.
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_US<number>_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
** Description:
**	this file contains functions used by both SQL and QUEL grammar in
**	parsing of the CREATE/ALTER USER statement
**
**	Routines:
**	    psl_us1_with_nonkeyword 	   - actions for WITH nonkeyword
**	    psl_us2_with_nonkw_eq_nmsconst - actions for WITH kw=sconst
**	    psl_us3_usrpriv		   - actions for a user priv
**	    psl_us4_usr_priv_or_nonkw      - actions for user (def) priv
** 	    psl_us5_usr_priv_def	   - actions for  add/drop priv
**
** 	    psl_us9_set_nonkw_eq_nonkw     - actions for SET SESSION WITH ...
**	    psl_us10_set_priv		   - actions for SET SESSION PRIV
**	    psl_us11_set_nonkw_roll_svpt   - SET SESSION O_E ROLLBACK SAVEPOINT
**	    psl_us12_set_nonkw_roll_nonkw  - SET SESSION O_E ROLLBACK nonkeyword
**	    psl_us14_set_nonkw_eq_int      - SET SESSION WITH PRIORITY=nnn
**	    psl_us15_set_nonkw             - SET SESSION WITH non_keyword
*/

/*
** Forward/external declarations
*/

GLOBALREF PSF_SERVBLK *Psf_srvblk;
/*
** privlist is a list of known user/profile privileges, and their legal contexts
*/
static struct {
	i4 usrpriv;
	i4 priv;
# define	PSL_UCREATEDB		0x01
# define	PSL_UTRACE		0x02
# define	PSL_USECURITY   	0x04
# define	PSL_UOPERATOR   	0x08
# define	PSL_USYSADMIN   	0x010
# define	PSL_UAUDIT		0x020
# define	PSL_UWRITEUP		0x040
# define	PSL_UWRITEDOWN  	0x080
# define	PSL_UWRITEFIXED 	0x0100
# define	PSL_UINSERTUP		0x0200
# define	PSL_UINSERTDOWN		0x0400
# define	PSL_UMONITOR    	0x0800
# define	PSL_UAUDITOR		0x02000
# define	PSL_UALTER_AUDIT 	0x04000
# define	PSL_UDOWNGRADE		0x08000
# define	PSL_UALL_EVENTS 	0x010000
# define	PSL_UDEF_EVENTS 	0x020000
# define	PSL_UQRYTEXT		0x040000
# define	PSL_UMAINTAIN_USER 	0x080000
# define	PSL_UALL_PRIVS 		0x0100000

	char    *privname;
	i4	flags;
# define	PSL_PRV_OK	0x01	/* Normal privilege */
# define	PSL_PRV_B1	0x02	/* B1-MAC only */
# define	PSL_PRV_OBS	0x04	/* Obsolete */
# define	PSL_PRV_AUDIT   0x010   /* Audit privilege */
} privlist[] = {
	DU_UCREATEDB,	 PSL_UCREATEDB,	"CREATEDB",	PSL_PRV_OK,
	DU_UTRACE,	 PSL_UTRACE, 	"TRACE",	PSL_PRV_OK,
	DU_USECURITY,	 PSL_USECURITY,	"SECURITY",	PSL_PRV_OK,
	DU_UOPERATOR,	 PSL_UOPERATOR,	"OPERATOR",	PSL_PRV_OK,
	DU_USYSADMIN,	 PSL_USYSADMIN,	"MAINTAIN_LOCATIONS",	PSL_PRV_OK,
	DU_UAUDIT,	 PSL_UAUDIT,	"AUDIT_ALL",	PSL_PRV_OBS,
	DU_UWRITEUP,	 PSL_UWRITEUP,	"WRITE_UP",	PSL_PRV_B1,
	DU_UWRITEDOWN,	 PSL_UWRITEDOWN,"WRITE_DOWN",	PSL_PRV_B1,
	DU_UWRITEFIXED,	 PSL_UWRITEFIXED,"WRITE_FIXED",	PSL_PRV_B1,
	DU_UINSERTUP,	 PSL_UINSERTUP,	"INSERT_UP",	PSL_PRV_B1,
	DU_UINSERTDOWN,	 PSL_UINSERTDOWN,"INSERT_DOWN",	PSL_PRV_B1,
	DU_UMONITOR,	 PSL_UMONITOR,	"MONITOR",	PSL_PRV_OK,
	DU_UAUDITOR,	 PSL_UAUDITOR,		"AUDITOR",	PSL_PRV_OK,
	DU_UALTER_AUDIT, PSL_UALTER_AUDIT,	"MAINTAIN_AUDIT",PSL_PRV_OK,
	DU_UMAINTAIN_USER, PSL_UMAINTAIN_USER,	"MAINTAIN_USERS",PSL_PRV_OK,
	DU_UDOWNGRADE,	 PSL_UDOWNGRADE,	"DOWNGRADE",	PSL_PRV_OBS,
	DU_UALL_PRIVS,	 PSL_UALL_PRIVS,	"ALL",	PSL_PRV_OBS,
	/*
	** Security audit types
	*/
	PSY_USAU_QRYTEXT,    PSL_UQRYTEXT,	"QUERY_TEXT", 	PSL_PRV_AUDIT,
	PSY_USAU_ALL_EVENTS, PSL_UALL_EVENTS,	"ALL_EVENTS",   PSL_PRV_AUDIT,
	PSY_USAU_DEF_EVENTS, PSL_UDEF_EVENTS,	"DEFAULT_EVENTS",PSL_PRV_AUDIT,
	0, 0,		NULL
};

/*
** Very commonly user text in messages
*/
static char CRTALTUSRPRO[] = ERx("CREATE/ALTER USER/PROFILE/ROLE");
/*
** Name: psl_us1_with_nonkeyword
**
** Description: 
**	Handles the WITH nonkeyword action of CREATE/ALTER USER
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
**	25-jun-93 (robf)
**	     First created
**	 8-oct-93 (robf)
**           Updated include files (no more dbms.h)
**	15-mar-94 (robf)
**           Allow EXTERNAL_PASSWORD for CREATE/ALTER USER
**	17-mar-94 (robf)
**           Correct checks for roles to == not &.
**      10-mar-95 (sarjo01)
**           Bug no. 67097 (67264): for case of ALTER USER query, if
**           NOPROFILE keyword specified, assure that user profile status
**           bit is turned off
**	11-apr-95 (forky01)
**	     Bug 67979: ALTER/CREATE USER WITH NOPROFILE was incorrectly
**	     ignoring the remove of the existing profile.  In the case of
**	     ALTER, the above fix kept the QEF code from clearing out the
**	     old profile name due to the profile bit turned off.  Back out
**	     fix by (sarjo01)10-mar-95.
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
**	29-aug-2003 (gupsh01)
**	    Added correctly handling -noprivileges, and -nodefault_privileges 
**	    flags for the user.
[@history_template@]...
*/
DB_STATUS
psl_us1_with_nonkeyword(
	PSY_CB	    *psy_cb,
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char 	    *word
)
{
	DB_STATUS   status;
	i4	    err_code;
        DB_DATA_VALUE	db_data;

	if (STcasecmp(word, "NOGROUP") == 0)
	{
	    /* Ensure GROUP was not previously specified */
	    if (psy_cb->psy_usflag & PSY_USRDEFGRP)
	    {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4) sizeof("[NO]GROUP")-1, "[NO]GROUP");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }
           if( psq_cb->psq_mode == PSY_CAPLID ||
	       psq_cb->psq_mode == PSY_AAPLID)
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("[NO]GROUP")-1, "[NO]GROUP");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	    
	    /* Indicate GROUP was specified */
	    psy_cb->psy_usflag |= PSY_USRDEFGRP;
	}

	else if (STcasecmp(word, "NOPRIVILEGES") == 0)
	{
	    /* Ensure PRIVILEGES was not previously specified */
	    if (psy_cb->psy_usflag & (PSY_USRPRIVS|PSY_USRAPRIVS|PSY_USRDPRIVS))
	    {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4) sizeof("[NO]PRIVILEGES")-1, "[NO]PRIVILEGES");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }
		
	    /* set the noprivileges flags */	
	    psy_cb->psy_usflag |= PSY_UNOPRIVS;
	    /* Indicate PRIVILEGES was specified */
	    psy_cb->psy_usflag |= PSY_USRPRIVS;
	}
	else if (STcasecmp(word, "NOSECURITY_AUDIT") == 0)
	{
	    /* Ensure SECURITY_AUDIT was not previously specified */
	    if (psy_cb->psy_usflag & PSY_USRSECAUDIT)
	    {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4) sizeof("[NO]SECURITY_AUDIT")-1, "[NO]SECURITY_AUDIT");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }

	    /* Indicate SECURITY_AUDIT was specified */
	    psy_cb->psy_usflag |= PSY_USRSECAUDIT;
	}
        else if (STcasecmp(word, "NODEFAULT_PRIVILEGES") == 0)
        {
            /* Ensure DEFAULT_PRIVILEGES was not previously pecified */
           if (psy_cb->psy_usflag & PSY_USRDEFPRIV)
           {
                (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
                    (i4)sizeof("[NO]DEFAULT_PRIVILEGES")-1, 
			"[NO]DEFAULT_PRIVILEGES");
                return (E_DB_ERROR);    /* non-zero return means error */
            }
	    if( psq_cb->psq_mode == PSY_CAPLID ||
	  	psq_cb->psq_mode == PSY_AAPLID)
	    {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("[NO]DEFAULT_PRIVILEGES")-1, 
				"[NO]DEFAULT_PRIVILEGES");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }

	   /* Indicate that users have specified -nodefault_privileges */ 
            psy_cb->psy_usflag |= PSY_UNODEFPRIV;
           /* Indicate DEFAULT_PRIVILEGES was specified */
            psy_cb->psy_usflag |= PSY_USRDEFPRIV;
            psy_cb->psy_usdefprivs=0;
        }
        else if (STcasecmp(word, "NOPASSWORD") == 0)
        {
	   /*
	   ** PASSWORD can't be specified with CREATE/ALTER PROFILE 
	   */
	   if(psy_cb->psy_usflag & (PSY_CPROFILE|PSY_APROFILE))
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER PROFILE")-1, "CREATE/ALTER PROFILE",
		    (i4)sizeof("[NO]PASSWORD")-1, "[NO]PASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }

           /* Ensure PASSWORD was not previously pecified */
           if (psy_cb->psy_usflag & PSY_USRPASS)
           {
                (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
                    (i4)sizeof("[NO]PASSWORD")-1, "[NO]PASSWORD");
                return (E_DB_ERROR);    /* non-zero return means error */
            }

           /* Indicate PASSWORD was specified */
            psy_cb->psy_usflag |= PSY_USRPASS;
        }
        else if (STcasecmp(word, "EXTERNAL_PASSWORD") == 0)
        {
	   /*
	   ** EXTERNAL_PASSWORD can't be specified with CREATE/ALTER 
	   ** 				PROFILE  yet
	   */
	   if(psy_cb->psy_usflag & 
			(PSY_CPROFILE|PSY_APROFILE))
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER PROFILE")-1, 
				"CREATE/ALTER PROFILE",
		    (i4)sizeof("EXTERNAL_PASSWORD")-1, 
				"EXTERNAL_PASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }

           /* Ensure PASSWORD was not previously pecified */
           if (psy_cb->psy_usflag & PSY_USRPASS)
           {
                (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
                    (i4)sizeof("[NO]PASSWORD")-1, "[NO]PASSWORD");
                return (E_DB_ERROR);    /* non-zero return means error */
            }

           /* Indicate EXTERNAL_PASSWORD was specified */
            psy_cb->psy_usflag |= (PSY_USRPASS|PSY_USREXTPASS);
        }
	else if (STcasecmp(word, "NOPROFILE")==0)
	{
	   /*
	   ** WITH NOPROFILE is illegal with CREATE/ALTER PROFILE/ROLE
	   */
	   if(psy_cb->psy_usflag & (PSY_CPROFILE|PSY_APROFILE))
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4)sizeof("[NO]PROFILE")-1, "[NO]PROFILE");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	   if( psq_cb->psq_mode == PSY_CAPLID ||
	  	psq_cb->psq_mode == PSY_AAPLID)
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("[NO]PROFILE")-1, "[NO]PROFILE");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	    /* Ensure PROFILE not previously specified */
	    if (psy_cb->psy_usflag & PSY_USRPROFILE)
	   {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4)sizeof("PROFILE")-1, 
				"PROFILE");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }
	    /*
	    ** Mark noprofile as having profile to reset to default profile 
	    */
            psy_cb->psy_usflag |= PSY_USRPROFILE;
	}
	else if (STcasecmp(word, "NOEXPIRE_DATE")==0)
	{
	    /* Ensure EXPIRE_DATE not previously specified */
            if (psy_cb->psy_usflag & PSY_USREXPDATE)
           {
                (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
                    (i4)sizeof("[NO]EXPIRE_DATE")-1, 
				"[NO]EXPIRE_DATE");
                return (E_DB_ERROR);    /* non-zero return means error */
            }
	    if( psq_cb->psq_mode == PSY_CAPLID ||
	  	psq_cb->psq_mode == PSY_AAPLID)
	    {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("[NO]EXPIRE_DATE")-1, "[NO]EXPIRE_DATE");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }

            /* Indicate EXPIRE_DATE was specified */
            psy_cb->psy_usflag |= PSY_USREXPDATE;
	    /*
	    ** Initialize to the empty date
	    */
	    db_data.db_datatype  = DB_DTE_TYPE;
	    db_data.db_prec	 = 0;
	    db_data.db_length    = DB_DTE_LEN;
	    db_data.db_data	 = (PTR)&psy_cb->psy_date;
	    status = adc_getempty(cb->pss_adfcb, &db_data);
	    if(status)
		return status;
	}
	else if (STcasecmp(word, "NOEXPIRE_DATE")==0)
	{
	    /* Ensure EXPIRE_DATE not previously specified */
            if (psy_cb->psy_usflag & PSY_USREXPDATE)
           {
                (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
                    (i4)sizeof("[NO]EXPIRE_DATE")-1, 
				"[NO]EXPIRE_DATE");
                return (E_DB_ERROR);    /* non-zero return means error */
            }
	    if( psq_cb->psq_mode == PSY_CAPLID ||
	  	psq_cb->psq_mode == PSY_AAPLID)
	    {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("[NO]EXPIRE_DATE")-1, "[NO]EXPIRE_DATE");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }

           /* Indicate EXPIRE_DATE was specified */
            psy_cb->psy_usflag |= PSY_USREXPDATE;
	    /*
	    ** Initialize to the empty date
	    */
	    db_data.db_datatype  = DB_DTE_TYPE;
	    db_data.db_prec	 = 0;
	    db_data.db_length    = DB_DTE_LEN;
	    db_data.db_data	 = (PTR)&psy_cb->psy_date;
	    status = adc_getempty(cb->pss_adfcb, &db_data);
	    if(status)
		return status;
	}
	else
	{
	    (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 2,
		sizeof(CRTALTUSRPRO)-1,
		CRTALTUSRPRO,
		(i4) STtrmwhite(word), word);
	    return (E_DB_ERROR);    /* non-zero return means error */
	}
	return E_DB_OK;
}


/*
** Name: psl_us2_with_nonkw_eq_nmsconst
**
** Description: 
**	Handles the WITH nonkw EQUAL nmsconst action of CREATE/ALTER USER
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
**	25-jun-93 (robf)
**		First created
**	19-oct-93 (robf)
**	    Add support for DEFAULT_PRIVILEGES=ALL
**	19-nov-93 (robf)
**          Allow both OLDPASSWORD and OLD_PASSWORD
**	19-apr-94 (robf)
**          Blank pad group name on saving.
**
*/
DB_STATUS
psl_us2_with_nonkw_eq_nmsconst(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *word2
)
{
	i4	    err_code;
	DB_STATUS   status;
	ADF_CB		    adf_cb;
	ADF_CB		    *adf_scb = (ADF_CB*) cb->pss_adfcb;
	DB_DATA_VALUE	    tdv;
	DB_DATA_VALUE	    fdv;
	DB_DATA_VALUE	    date_now;
	DB_DATE		    dn_now;
	DB_DATE		    dn_add;

	if (STcasecmp(word1, "PASSWORD") == 0)
	{
	   /*
	   ** PASSWORD can't be specified with CREATE/ALTER PROFILE 
	   */
	   if(psy_cb->psy_usflag & (PSY_CPROFILE|PSY_APROFILE))
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER PROFILE")-1, "CREATE/ALTER PROFILE",
		    (i4)sizeof("[NO]PASSWORD")-1, "[NO]PASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	    /* Ensure PASSWORD was not previously pecified */
	   if (psy_cb->psy_usflag & PSY_USRPASS)
	   {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4)sizeof("[NO]PASSWORD")-1, "[NO]PASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }

	    /* Indicate PASSWORD was specified */
	    psy_cb->psy_usflag |= PSY_USRPASS;
	    STmove(word2, ' ', (i2) sizeof(DB_OWN_NAME),
		   (char *) &psy_cb->psy_apass);
	}
	else if (STcasecmp(word1, "PROFILE") == 0)
	{
	   /*
	   ** PROFILE can't be specified with CREATE/ALTER PROFILE/ROLE
	   */
	   if(psy_cb->psy_usflag & (PSY_CPROFILE|PSY_APROFILE))
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER PROFILE")-1, "CREATE/ALTER PROFILE",
		    (i4)sizeof("[NO]PROFILE")-1, "[NO]PROFILE");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	    if( psq_cb->psq_mode == PSY_CAPLID ||
	  	psq_cb->psq_mode == PSY_AAPLID)
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("[NO]PROFILE")-1, "[NO]PROFILE");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	   /* Ensure PROFILE was not previously pecified */
	   if (psy_cb->psy_usflag & PSY_USRPROFILE)
	   {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4)sizeof("[NO]PROFILE")-1, "[NO]PROFILE");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }

	    /* Indicate PROFILE was specified */
	    psy_cb->psy_usflag |= PSY_USRPROFILE;
	    STmove(word2, ' ', (i2) sizeof(psy_cb->psy_usprofile),
		   (char *) &psy_cb->psy_usprofile);
	}
	else if ((STcasecmp(word1, "OLDPASSWORD") == 0) ||
	        (STcasecmp(word1, "OLD_PASSWORD") == 0))
	{
	   /*
	   ** OLDPASSWORD can't be specified with CREATE/ALTER PROFILE/ROLE
	   */
	   if(psy_cb->psy_usflag & (PSY_CPROFILE|PSY_APROFILE))
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER PROFILE")-1, "CREATE/ALTER PROFILE",
		    (i4)sizeof("OLD_PASSWORD")-1, "OLD_PASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	   if( psq_cb->psq_mode == PSY_CAPLID ||
	  	psq_cb->psq_mode == PSY_AAPLID)
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("OLDPASSWORD")-1, "OLDPASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	   /* Ensure OLDPASSWORD was not previously pecified */
	   if (psy_cb->psy_usflag & PSY_USROLDPASS)
	   {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4)sizeof("OLDPASSWORD")-1, "OLDPASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }

	    /* Indicate OLDPASSWORD was specified */
	    psy_cb->psy_usflag |= PSY_USROLDPASS;
	    /* psy_cb->psy_usflag |= PSY_USROPASSCTL;*/
	    STmove(word2, ' ', (i2) sizeof(DB_OWN_NAME),
		   (char *) &psy_cb->psy_bpass);

	}
	else if (STcasecmp(word1, "EXPIRE_DATE")==0)
	{
	    i4  result;
	    /* Ensure EXPIRE_DATE not previously specified */
            if (psy_cb->psy_usflag & PSY_USREXPDATE)
           {
                (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
                    (i4)sizeof("[NO]EXPIRE_DATE")-1, 
				"[NO]EXPIRE_DATE");
                return (E_DB_ERROR);    /* non-zero return means error */
            }
	    if( psq_cb->psq_mode == PSY_CAPLID ||
	  	psq_cb->psq_mode == PSY_AAPLID)
	    {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("[NO]EXPIRE_DATE")-1, "[NO]EXPIRE_DATE");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }

           /* Indicate EXPIRE_DATE was specified */
           psy_cb->psy_usflag |= PSY_USREXPDATE;

	   /*
	   ** Convert the data string to a real INGRES date.
	   */
	
   	   /* Copy the session ADF block into local one */
	   STRUCT_ASSIGN_MACRO(*adf_scb, adf_cb);

	   adf_cb.adf_errcb.ad_ebuflen = 0;
	   adf_cb.adf_errcb.ad_errmsgp = 0;
	   /*
	   ** Use ADF to convert the string into a date
	   */
	   fdv.db_prec		    = 0;
	   fdv.db_length	    = STlength(word2);
	   fdv.db_datatype	    = DB_CHA_TYPE;
	   fdv.db_data		    = (PTR) word2;

	   tdv.db_prec		    = 0;
	   tdv.db_length	    = DB_DTE_LEN;
	   tdv.db_datatype	    = DB_DTE_TYPE;
	   tdv.db_data		    = (PTR) &psy_cb->psy_date;
	    
	   if (adc_cvinto(&adf_cb, &fdv, &tdv) != E_DB_OK)
	   {
	    	(VOID) psf_error(4302, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 1,
			STlength(word2), word2);
	    	return (E_DB_ERROR);
	   }
	   /*
	   ** Check for date interval, convert to absolute if
	   ** necessary. We do NOT do this for profiles, since the resolution
	   ** occurs at user creation time
	   */
	   if(!(psy_cb->psy_usflag & (PSY_CPROFILE|PSY_APROFILE)))
	   {
	       status=adc_date_chk(&adf_cb, &tdv, &result);
	       if(status!=E_DB_OK)
		    return E_DB_ERROR;
		
	       if(result==2)
	       {
	    	    /*
		    ** Interval, so normalize as offset from current date
		    */
	           /* fdv alreadyset  up as a CHA */
	           fdv.db_length	= 3;
	           fdv.db_data	=(PTR)"now";

	           date_now.db_prec	    = 0;
	           date_now.db_length	    = DB_DTE_LEN;
	           date_now.db_datatype	    = DB_DTE_TYPE;
	           date_now.db_data		    = (PTR) &dn_now;
	    
	           if (adc_cvinto(&adf_cb, &fdv, &date_now) != E_DB_OK)
	           {
	    		(VOID) psf_error(4302, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 1,
			STlength(word2), word2);
	    		return (E_DB_ERROR);
	           }
	           /*
	           ** Add 'now' and current to get final value
	           */
	           fdv.db_data 	= (PTR) &dn_add;
	           fdv.db_length    = DB_DTE_LEN;
	           fdv.db_datatype  = DB_DTE_TYPE;
	           status=adc_date_add(&adf_cb, &date_now, &tdv, &fdv);
	           if(status!=E_DB_OK)
	           {
			return status;
	           }
	           /*
	           ** Copy added date to output date
	           */
	           MECOPY_CONST_MACRO((PTR)&dn_add, sizeof(DB_DATE), 
			(PTR) &psy_cb->psy_date);
	       }
	    }
	}
	else if (STcasecmp(word1, "DEFAULT_GROUP") == 0
	     || STcasecmp(word1, "GROUP") == 0)
	{
	   /*
	   ** Roles don't have default groups
	   */
	   if( psq_cb->psq_mode == PSY_CAPLID ||
	  	psq_cb->psq_mode == PSY_AAPLID)
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		    (i4)sizeof("[NO]GROUP")-1, "[NO]GROUP");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	    /* Ensure GROUP was not previously specified */
	    if (psy_cb->psy_usflag & PSY_USRDEFGRP)
	    {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4) sizeof("[NO]GROUP")-1, "[NO]GROUP");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }
	    
	    /* Indicate GROUP was specified */
	   psy_cb->psy_usflag |= PSY_USRDEFGRP;
	   /*
	   ** Blank pad group name on moving
	   */
	   STmove(word2, ' ', (i2) sizeof(DB_OWN_NAME),
		(char*) &psy_cb->psy_usgroup);
	}
	else if (STcasecmp(word1, "DEFAULT_PRIVILEGES") == 0)
	{
	       if( psq_cb->psq_mode == PSY_CAPLID ||
		   psq_cb->psq_mode == PSY_AAPLID)
	       {
		    (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		        &psq_cb->psq_error, 2,
		        (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		        (i4)sizeof("[NO]DEFAULT_PRIVILEGES")-1, 
				"[NO]DEFAULT_PRIVILEGES");
		    return (E_DB_ERROR);    /* non-zero return means error */
	       }
		/* Ensure DEFAULT_PRIVILEGES was not previously specified */
		if (psy_cb->psy_usflag & PSY_USRDEFPRIV)
		{
			(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
			    &psq_cb->psq_error, 2,
			    sizeof(CRTALTUSRPRO)-1,
			    CRTALTUSRPRO,
			    (i4) sizeof("[NO]DEFAULT_PRIVILEGES")-1, 
					"[NO]DEFAULT_PRIVILEGES");
			    return (E_DB_ERROR);    /* non-zero return means error */
		}
		psy_cb->psy_usflag |= PSY_USRDEFPRIV;

		/* Only legal value is ALL */
		if (STcasecmp(word2,"ALL")==0)
		{
			/* Mark as default all privs */
			psy_cb->psy_usflag |= PSY_USRDEFALL;
		}
		else
		{
		    (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
			sizeof(CRTALTUSRPRO)-1,
			CRTALTUSRPRO,
			(i4) STtrmwhite(word2), word2);
		    return (E_DB_ERROR);    /* non-zero return means error */
		}
	}
	else
        {
            (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
                &psq_cb->psq_error, 2,
		sizeof(CRTALTUSRPRO)-1,
		CRTALTUSRPRO,
                (i4) STtrmwhite(word1), word1);
	    return (E_DB_ERROR);    /* non-zero return means error */
        }
    return E_DB_OK;
}

/*
** Name: psl_us2_with_nonkw_eq_hexconst
**
** Description: 
**	Handles the WITH nonkw EQUAL hexconst action of CREATE/ALTER USER/ROLE
**
** Inputs:
**	psy_cb	
**	
**	word1	The nonkeyword
**
**	length	The hexconst length
**
**	word2	The hexconst
**
** Outputs:
**	None
**
** History
**	19-jul-2005 (toumi01)
**		Created to support WITH PASSWORD=X'<encrypted>'.
**
*/
DB_STATUS
psl_us2_with_nonkw_eq_hexconst(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	u_i2	    length,
	char	    *word2
)
{
	i4	    err_code;
	DB_STATUS   status;

	if (STcasecmp(word1, "PASSWORD") == 0)
	{
	   /*
	   ** PASSWORD can't be specified with CREATE/ALTER PROFILE 
	   */
	   if(psy_cb->psy_usflag & (PSY_CPROFILE|PSY_APROFILE))
	   {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4)sizeof("CREATE/ALTER PROFILE")-1, "CREATE/ALTER PROFILE",
		    (i4)sizeof("[NO]PASSWORD")-1, "[NO]PASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }
	    /* Ensure PASSWORD was not previously pecified */
	   if (psy_cb->psy_usflag & PSY_USRPASS)
	   {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    (i4)sizeof("[NO]PASSWORD")-1, "[NO]PASSWORD");
		return (E_DB_ERROR);    /* non-zero return means error */
	    }
	    /* Make sure HEX PASSWORD is the correct length */
	   if (length != DB_PASSWORD_LENGTH)
	   {
		(VOID) psf_error(6374L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 1,
		    (i4)sizeof("CREATE/ALTER USER/ROLE")-1,
		    "CREATE/ALTER USER/ROLE");
		return (E_DB_ERROR);    /* non-zero return means error */
	   }

	    /* Indicate HEX PASSWORD was specified */
	    psy_cb->psy_usflag |= (PSY_USRPASS|PSY_HEXPASS);
	    MECOPY_CONST_MACRO((PTR)word2, DB_PASSWORD_LENGTH,
			(PTR) &psy_cb->psy_apass);
	}
	else
        {
            (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
                &psq_cb->psq_error, 2,
		sizeof(CRTALTUSRPRO)-1,
		CRTALTUSRPRO,
                (i4) STtrmwhite(word1), word1);
	    return (E_DB_ERROR);    /* non-zero return means error */
        }
    return E_DB_OK;
}

/*
** Name: psl_us3_usrpriv
**
** Description: 
**	Handles the usrpriv action of CREATE/ALTER USER
**
** Inputs:
**	psy_cb	
**	
**	word	The privilege specified
**
** Outputs:
**	priv_ptr - bit map of privileges selected
**
**	None
**
** History
**	25-jun-93 (robf)
**		First created
**
*/
DB_STATUS
psl_us3_usrpriv(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	char	    *word,
	i4     *privptr
)
{
	i4	    err_code;
	i4	    *priv_var;
	i4	    p;

	priv_var=privptr;
	*priv_var=0;

	for( p=0; privlist[p].privname; p++)
	{
		if (STcasecmp(word, privlist[p].privname) == 0)
		{
		    if((privlist[p].flags &PSL_PRV_OK) ||
		       (privlist[p].flags &PSL_PRV_B1) ||
		       (privlist[p].flags &PSL_PRV_AUDIT) 
		       )
		    {
			/*
			** This privilege is OK. We allow  B1-specific 
			** privileges to be granted currently, but later 
			** may wish to disallow/warn on non-B1 systems
			*/
			*priv_var = privlist[p].priv;
		    }
		    else if(privlist[p].flags &PSL_PRV_OBS)
		    {
			/*
			** This privilege is obsolete and usage should be
			** discontinued in a future release. 
			** We warn but continue.
			*/
			(VOID) psf_error(W_PS03A8_OBSOLETE_PRIV, 
				0L, PSF_USERERR, &err_code,
				&psq_cb->psq_error, 1,
				(i4) STtrmwhite(word), word);
			*priv_var |= privlist[p].priv;
		    }
		    break;
		}
	}

	if(!privlist[p].privname || !*priv_var)
	{ /*
	  ** Not found 
	  */
	  (VOID) psf_error(6288, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 1,
		(i4) STtrmwhite(word), word);
	  return (E_DB_ERROR);    /* non-zero return means error */
	}
	return E_DB_OK;
}

/*
** Name: psl_us4_usr_priv_or_nonkw
**
** Description: 
**	Handles the usrpriv_or_nonkw action of CREATE/ALTER USER/PROFILE/ROLE
**
** Inputs:
**	psy_cb	
**	
**	word	The operation specified
**
**	privs  	list of privileges
**
** Outputs:
**	None
**
** History
**	25-jun-93 (robf)
**	     First created
**	18-apr-94 (robf)
**           Don't merge security audit events with privileges yet,
**	     this is done in psy....
**
*/
DB_STATUS
psl_us4_usr_priv_or_nonkw(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word,
	i4	    privs
)
{
	i4	    err_code;
	i4	    p;
	/*
	** Allowed clauses are DEFAULT_PRIVILEGES, SECURITY_AUDIT
	*/
	if (STcasecmp(word, "SECURITY_AUDIT") == 0)
	{
		/* Ensure SECURITY_AUDIT was not previously specified */
		if (psy_cb->psy_usflag & PSY_USRSECAUDIT)
		{
		    (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
		        sizeof(CRTALTUSRPRO)-1,
		        CRTALTUSRPRO,
			(i4) sizeof("[NO]SECURITY_AUDT")-1, "[NO]SECURITY_AUDT");
		    return (E_DB_ERROR);    /* non-zero return means error */
		}
		/* Indicate SECURITY_AUDIT was specified */
		psy_cb->psy_usflag |= PSY_USRSECAUDIT;
		/*
		** Loop round all privs, checking they are  allowed and
		** mapping into psy_usprivs
		*/
		for( p=0; privlist[p].privname; p++)
		{
			if(privs & privlist[p].priv )
			{
				if(!(privlist[p].flags & PSL_PRV_AUDIT))
				{
					/*
					** Must specify audit privileges !
					*/
					(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
					    &psq_cb->psq_error, 2,
					    sizeof(CRTALTUSRPRO)-1,
					    CRTALTUSRPRO,
					    STlength(privlist[p].privname), 
						    privlist[p].privname);
					return (E_DB_ERROR);   
				}
				psy_cb->psy_ussecaudit|=privlist[p].usrpriv;
			}
		}
	}
	else if (STcasecmp(word, "DEFAULT_PRIVILEGES") == 0)
	{
	       if( psq_cb->psq_mode == PSY_CAPLID ||
		   psq_cb->psq_mode == PSY_AAPLID)
	       {
		    (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		        &psq_cb->psq_error, 2,
		        (i4)sizeof("CREATE/ALTER ROLE")-1, "CREATE/ALTER ROLE",
		        (i4)sizeof("[NO]DEFAULT_PRIVILEGES")-1, 
				"[NO]DEFAULT_PRIVILEGES");
	    	    return (E_DB_ERROR);    /* non-zero return means error */
	       }
		/* Ensure DEFAULT_PRIVILEGES was not previously specified */
		if (psy_cb->psy_usflag & PSY_USRDEFPRIV)
		{
			(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
			    &psq_cb->psq_error, 2,
			    sizeof(CRTALTUSRPRO)-1,
			    CRTALTUSRPRO,
			    (i4) sizeof("[NO]DEFAULT_PRIVILEGES")-1, 
					"[NO]DEFAULT_PRIVILEGES");
			    return (E_DB_ERROR);    /* non-zero return means error */
		}
		psy_cb->psy_usflag |= PSY_USRDEFPRIV;
		/*
		** Loop round all privs, checking they are  allowed and
		** mapping into psy_usprivs
		*/
		for( p=0; privlist[p].privname; p++)
		{
			if(privs & privlist[p].priv )
			{
				if(privlist[p].flags & PSL_PRV_AUDIT)
				{
					/*
					** Can't specify audit priv with privs
					*/
					(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
					    &psq_cb->psq_error, 2,
					    sizeof(CRTALTUSRPRO)-1,
					    CRTALTUSRPRO,
					    STlength(privlist[p].privname), 
						    privlist[p].privname);
					return (E_DB_ERROR);   
				}
				psy_cb->psy_usdefprivs|=privlist[p].usrpriv;
			}
		}
	}
	else
	{
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    sizeof(CRTALTUSRPRO)-1,
		    CRTALTUSRPRO,
		    STlength(word), word);
		return (E_DB_ERROR);    /* non-zero return means error */
	}
	return E_DB_OK;
}

/*
** Name: psl_us5_usr_priv_def
**
** Description: 
**	Handles [ADD/DROP] PRIVILEGES clauses of CREATE/ALTER USER
**
** Inputs:
**	psy_cb	
**	
**	word	The operation specified
**
**	mode	Add/Drop/Set  privileges
**
**	privs  	list of privileges
**
** Outputs:
**	None
**
** History
**	25-jun-93 (robf)
**		First created
**
*/
DB_STATUS
psl_us5_usr_priv_def(
	PSY_CB      *psy_cb, 
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	i4	    mode,
	i4	    privs
)
{
	i4	    err_code;
	i4	    p;
	/* Ensure PRIVILEGES was not previously specified */
	if (psy_cb->psy_usflag & (PSY_USRPRIVS|PSY_USRAPRIVS|PSY_USRDPRIVS))
	{
	    (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 2,
		(i4) sizeof("CREATE/ALTER USER/ROLE")-1, 
				"CREATE/ALTER USER/ROLE",
		(i4) sizeof("[NO]PRIVILEGES")-1, "[NO]PRIVILEGES");
	    return (E_DB_ERROR);    /* non-zero return means error */
	}
	/* Indicate mode was specified */
	psy_cb->psy_usflag |= mode;
	/*
	** Loop round all privs, checking they are  allowed and
	** mapping into psy_usprivs
	*/
	for( p=0; privlist[p].privname; p++)
	{
		if(privs & privlist[p].priv )
		{
			if(privlist[p].flags & PSL_PRV_AUDIT)
			{
				/*
				** Can't specify audit priv with privs
				*/
				(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
				    &psq_cb->psq_error, 2,
				    (i4)sizeof("CREATE/ALTER USER/ROLE")-1, 
					"CREATE/ALTER USER/ROLE",
				    STlength(privlist[p].privname), 
					    privlist[p].privname);
				return (E_DB_ERROR);   
			}
			psy_cb->psy_usprivs|=privlist[p].usrpriv;
		}
	}
	return E_DB_OK;
}

/*
** Name: psl_us9_set_nonkw_eq_nonkw
**
** Description: 
**	This is used to process SET SESSION WITH nonkeyword = nonkeyword
**
** Inputs:
**	scb
**
**	word1	First word
**
**	word2	Second word
**
**	isnkw	TRUE if nonkeyword, FALSE if a string
**
** Outputs:
**	None
**
** History
**	30-jul-93 (robf)
**	   First created
**	28-oct-93 (robf)
**         Add "priority" clause
**	9-nov-93 (robf)
**         Add "description" clause
**      28-nov-95 (stial01)
**         Add "xa_xid" clause
**	12-jul-1998 (rigka01)
**	   Bug 90913 - change size parm in psf_malloc calls from
**	   STlength(word2+1) to STlength(word2)+1 
**	20-Apr-2004 (jenjo02)
**	    Added SET SESSION WITH ON_LOGFULL = ABORT | COMMIT | NOTIFY
**	13-Jul-2005 (thaju02)
**	    Added SET SESSION WITH ON_USER_ERROR = ROLLBACK TRANSACTION.
*/
DB_STATUS
psl_us9_set_nonkw_eq_nonkw(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *word2,
	bool	    isnkw
)
{
	DB_STATUS status;
	i4   err_code;

	if (STcasecmp(word1, "priority")==0)
	{
		if(psq_cb->psq_ret_flag & PSQ_SET_PRIORITY)
		{
			/*
			** Can only have once
			*/
		        (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    		&psq_cb->psq_error, 2,
			    (i4) sizeof("SET SESSION")-1, "SET SESSION",
			    (i4) sizeof("PRIORITY")-1, "PRIORITY");
		        return (E_DB_ERROR);  /* non-zero return means error */
		}
		/*
		** Check seconds word, should be MAXIMUM, MINIMUM or INITIAL
		*/
		if(STcasecmp(word2, "maximum")==0)
		{
			psq_cb->psq_ret_flag|=PSQ_SET_PRIO_MAX;
		}
		else  if (STcasecmp(word2, "minimum")==0)
		{
			psq_cb->psq_ret_flag|=PSQ_SET_PRIO_MIN;
		}
		else  if (STcasecmp(word2, "initial")==0)
		{
			psq_cb->psq_ret_flag|=PSQ_SET_PRIO_INITIAL;
		}
		else
		{
		    (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
		        (i4) sizeof("SET SESSION")-1, "SET SESSION",
		        (i4)STlength(word2), word2);
		    return (E_DB_ERROR);    /* non-zero return means error */
		}
		psq_cb->psq_ret_flag |= PSQ_SET_PRIORITY;
	}
	else if (STcasecmp(word1, "description")==0)
	{
		if(psq_cb->psq_ret_flag & PSQ_SET_DESCRIPTION)
		{
			/*
			** Can only have once
			*/
		        (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    		&psq_cb->psq_error, 2,
			    (i4) sizeof("SET SESSION")-1, "SET SESSION",
			    (i4) sizeof("DESCRIPTION")-1, "DESCRIPTION");
		        return (E_DB_ERROR);  /* non-zero return means error */
		}
		/*
		** Check seconds word, should be NONE
		*/
		if(isnkw==TRUE)
		{
			if(STcasecmp(word2, "none")==0)
			{
	    	            status = psf_malloc(cb, &cb->pss_ostream,
				STlength(word2)+1,
				(PTR *) &psq_cb->psq_desc,
				&psq_cb->psq_error);
		            if (DB_FAILURE_MACRO(status))
				return(status);
			    /* Empty string */
			    psq_cb->psq_desc[0]=0;
			}
			else
			{
		    	    (VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
				&psq_cb->psq_error, 2,
		        	(i4) sizeof("SET SESSION")-1, "SET SESSION",
		        	(i4)STlength(word2), word2);
		    		return (E_DB_ERROR);    /* non-zero return means error */
			}
		}
		else
		{
			/*
			** Save session description
			*/
	    	        status = psf_malloc(cb, &cb->pss_ostream,
				STlength(word2)+1,
				(PTR *) &psq_cb->psq_desc,
				&psq_cb->psq_error);
		        if (DB_FAILURE_MACRO(status))
				return(status);
			STcopy(word2, psq_cb->psq_desc);
		}
		psq_cb->psq_ret_flag |= PSQ_SET_DESCRIPTION;
	}
	else if (STcasecmp(word1, "xa_xid")==0)
	{
	    if(psq_cb->psq_ret_flag & PSQ_SET_XA_XID)
	    {
		/*
		** Can only have once
		*/
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
		    (i4) sizeof("SET SESSION")-1, "SET SESSION",
		    (i4) sizeof("XA_XID")-1, "XA_XID");
		return (E_DB_ERROR);  /* non-zero return means error */
	    }
	    /*
	    ** Save distributed transaction id
	    */
	    status = psf_malloc(cb, &cb->pss_ostream,
		    STlength(word2)+1,
		    (PTR *) &psq_cb->psq_distranid,
		    &psq_cb->psq_error);
	    if (DB_FAILURE_MACRO(status))
		    return(status);
	    STcopy(word2, psq_cb->psq_distranid);
	    psq_cb->psq_ret_flag |= PSQ_SET_XA_XID;
	}
	else if (STcasecmp(word1, "on_logfull")==0)
	{
	    if (cb->pss_distrib & DB_3_DDB_SESS)
	    {
		i4	err_code;

		(VOID) psf_error(6350L, 0L, PSF_USERERR,
		    &err_code, &psq_cb->psq_error, 1,
		    (i4) sizeof("SET SESSION WITH ON_LOGFULL")-1, 
			    "SET SESSION WITH ON_LOGFULL");
		return (E_DB_ERROR);
	    }

	    if (psq_cb->psq_ret_flag & (PSQ_SET_ON_LOGFULL_ABORT |
					PSQ_SET_ON_LOGFULL_COMMIT |
					PSQ_SET_ON_LOGFULL_NOTIFY))
	    {
		/*
		** Can only have once
		*/
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
		    (i4) sizeof("SET SESSION")-1, "SET SESSION",
		    (i4) sizeof("ON_LOGFULL")-1, "ON_LOGFULL");
		return (E_DB_ERROR);  /* non-zero return means error */
	    }
	    /*
	    ** Check second word, should be ABORT, COMMIT, or NOTIFY
	    */
	    if ( STcasecmp(word2, "abort") == 0 )
		psq_cb->psq_ret_flag |= PSQ_SET_ON_LOGFULL_ABORT;
	    else if ( STcasecmp(word2, "commit") == 0 )
		psq_cb->psq_ret_flag |= PSQ_SET_ON_LOGFULL_COMMIT;
	    else if ( STcasecmp(word2, "notify") == 0 )
		psq_cb->psq_ret_flag |= PSQ_SET_ON_LOGFULL_NOTIFY;
	    else
	    {
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4) sizeof("SET SESSION")-1, "SET SESSION",
		    (i4)STlength(word2), word2);
		return (E_DB_ERROR);    /* non-zero return means error */
	    }
	}
	else if (STcasecmp(word1, "on_user_error")==0)
        {
	    if (cb->pss_distrib & DB_3_DDB_SESS)
	    {
		(VOID) psf_error(6350L, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 1,
			(i4) sizeof("SET SESSION WITH ON_USER_ERROR")-1,
			"SET SESSION WITH ON_USER_ERROR");
		return (E_DB_ERROR);
	    }

	    if(psq_cb->psq_ret_flag & (PSQ_ON_USRERR_ROLLBACK | 
				       PSQ_ON_USRERR_NOROLL))
	    {
		/* Can only have once */
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
			(i4) sizeof("SET SESSION")-1, "SET SESSION",
			(i4) sizeof("ON_USER_ERROR")-1, "ON_USER_ERROR");
		return (E_DB_ERROR);  
	    }

	    if (STcasecmp(word2, "norollback") == 0)
	    {
		psq_cb->psq_ret_flag &= ~PSQ_ON_USRERR_ROLLBACK;
		psq_cb->psq_ret_flag |= PSQ_ON_USRERR_NOROLL;
	    }
	    else
	    {
		(VOID) psf_error(E_PS0F87_SET_ON_USER_ERROR, 0L,
			PSF_USERERR, &err_code, &psq_cb->psq_error,
			1, (i4) STlength(word2), word2);
		return (E_DB_ERROR);    /* non-zero return means error */
	    }
	}
	else
	{
		/*
		** Invalid option
		*/
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
		    (i4) sizeof("SET SESSION")-1, "SET SESSION",
		    (i4)STlength(word1), word1);
		return (E_DB_ERROR);    /* non-zero return means error */
	}
        return (E_DB_OK);               /* If we emerge down here, must
                                        ** have been ok (function is a 
                                        ** big series of if .. then .. else
                                        ** .. if ... Errors explcitly 
                                        ** return with status).        */
}

/*
** Name: psl_us10_set_priv
**
** Description: 
**	Handles [ADD/DROP] PRIVILEGES clauses of SET SESSION statement
**
** Inputs:
**	psq_cb	
**	
**	word	The operation specified
**
**	mode	Add/Drop/Set  privileges
**
**	privs  	list of privileges
**
** Outputs:
**	None
**
** History
**	30-jul-93 (robf)
**		First created
**
*/
DB_STATUS
psl_us10_set_priv(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	i4	    mode,
	i4	    privs
)
{
	i4	    err_code;
	i4	    p;
	/* Ensure PRIVILEGES was not previously specified */
	if (psq_cb->psq_ret_flag & (PSQ_SET_PRIV))
	{
	    (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 2,
		(i4) sizeof("SET SESSION")-1, "SET SESSION",
		(i4) sizeof("[NO]PRIVILEGES")-1, "[NO]PRIVILEGES");
	    return (E_DB_ERROR);    /* non-zero return means error */
	}
	/* Indicate mode was specified */
	psq_cb->psq_ret_flag |= (mode|PSQ_SET_PRIV);
	/*
	** Loop round all privs, checking they are  allowed and
	** mapping into psy_usprivs
	*/
	for( p=0; privlist[p].privname; p++)
	{
		if(privs & privlist[p].priv )
		{
			if(privlist[p].flags & PSL_PRV_AUDIT)
			{
				/*
				** Can't specify audit priv with privs
				*/
				(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
				    &psq_cb->psq_error, 2,
				    (i4)sizeof("SET SESSION")-1, "SET SESSION",
				    STlength(privlist[p].privname), 
					    privlist[p].privname);
				return (E_DB_ERROR);   
			}
			psq_cb->psq_privs|=privlist[p].usrpriv;
		}
	}
	return E_DB_OK;
}

/*
** Name: psl_us11_set_nonkw_roll_svpt
**
** Description: 
**	Handles SET SESSION WITH ON_ERROR ROLLBACK SAVEPOINT ...
**
** Inputs:
**	psq_cb	
**	
**	word1	Should be ON_ERROR
**
**	svpt	Savepoint name
**
** Outputs:
**	None
**
** History
**	30-jul-93 (robf)
**		First created, code modularized out of pslsgram.yi
**	14-feb-94 (robf)
**          Disallow SET SESSION WITH ON_ERROR in STAR, currently
**	    non-supported functionality. (This already happened in
**          the grammer, but got moved here now we allow some SET SESSION
**          functionality it STAR)
**	26-Feb-2001 (jenjo02)
**	    Set session_id in QEF_RCB;
**
*/
DB_STATUS
psl_us11_set_nonkw_roll_svpt(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *svpt
)
{
	DB_STATUS		status;
	QEF_RCB			*qef_rcb;
	i4			err_code;

	/*
	** Only valid with SET SESSION WITH ON_ERROR =
	*/
        if (STcasecmp(word1, "on_error"))
	{
            _VOID_ psf_error(E_PS0F83_SET_ONERROR_STMT, 0L, PSF_USERERR,
                &err_code, &psq_cb->psq_error, 1, 
			(i4) STlength(word1), word1);
            return (E_DB_ERROR);
	}
	/* "set session with on_error" is not allowed in distributed yet */
	if (cb->pss_distrib & DB_3_DDB_SESS)
	{
	    i4	err_code;

	    (VOID) psf_error(6350L, 0L, PSF_USERERR,
		&err_code, &psq_cb->psq_error, 1,
		(i4) sizeof("SET SESSION WITH ON_ERROR")-1, 
			"SET SESSION WITH ON_ERROR");
	    return (E_DB_ERROR);
	}
	/*
	** Can only specify this once
	*/
	if(psq_cb->psq_ret_flag & PSQ_SET_ON_ERROR)
	{
	    (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		&psq_cb->psq_error, 2,
		(i4) sizeof("SET SESSION")-1, "SET SESSION",
		(i4) sizeof("ON_ERROR")-1, "ON_ERROR");

	    return E_DB_ERROR;
	}

	psq_cb->psq_ret_flag|= PSQ_SET_ON_ERROR;


	/* Allocate control block for QEC_ALTER call for set session */

	status = psf_malloc(cb, &cb->pss_ostream, sizeof(QEF_RCB), (PTR *) &qef_rcb,
	    &psq_cb->psq_error);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	status = psf_mroot(cb, &cb->pss_ostream, (PTR) qef_rcb, &psq_cb->psq_error);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	cb->pss_object = (PTR) qef_rcb;

	qef_rcb->qef_length 	= sizeof(QEF_RCB);
	qef_rcb->qef_type 	= QEFRCB_CB;
	qef_rcb->qef_owner 	= (PTR)DB_PSF_ID;
	qef_rcb->qef_ascii_id 	= QEFRCB_ASCII_ID;
	qef_rcb->qef_eflag 	= QEF_INTERNAL;
	qef_rcb->qef_sess_id    = cb->pss_sessid;
	/* 
	** Allocate memory for alter control block.
	*/
	status = psf_malloc(cb, &cb->pss_ostream, (i4) sizeof(QEF_ALT), 
	    (PTR *) &qef_rcb->qef_alt, &psq_cb->psq_error);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	qef_rcb->qef_asize = 1;
	qef_rcb->qef_alt->alt_code = 0;
	/*
	** Rollback to savepoint on error option.
	*/
	/*
	** Rollback to user savepoint on error option.
	** This option is not currently supported but the processing is
	** included here for future enhancements.
	**
	** We will return an error below.
	*/

	/*
	** Allocate memory for savepoint name.
	*/
	status = psf_malloc(cb, &cb->pss_ostream, (i4) sizeof(DB_SP_NAME), 
	    (PTR *) &qef_rcb->qef_spoint, &psq_cb->psq_error);
	if (DB_FAILURE_MACRO(status))
	    return (status);

	/*
	** Fill in savepoint name.	
	*/
	STmove(svpt, ' ', sizeof(DB_SP_NAME), (char *) qef_rcb->qef_spoint);

	/*
	** Fill in on_error type in qef_rcb control block.
	*/
	qef_rcb->qef_alt->alt_code = QEC_ONERROR_STATE;
	qef_rcb->qef_alt->alt_value.alt_i4 = QEF_SVPT_ROLLBACK;

	/*
	** Even though we have now done all the work, return an error since
	** this option is not yet supported.
        **
        ** Allow special secret savepoint name for internal testing of option.
        */
        if (STcasecmp(svpt, "ii_sp_test"))
        {
            _VOID_ psf_error(E_PS0F83_SET_ONERROR_STMT, 0L, PSF_USERERR,
                &err_code, &psq_cb->psq_error, 1, 
			(i4) STlength(svpt), svpt);
            return (E_DB_ERROR);
        }
	return E_DB_OK;
}


/*
** Name: psl_us12_nonkw_roll_nonkw
**
** Description: 
**	Handles SET SESSION WITH ON_ERROR = ROLLBACK STATEMENT|TRANSACTION
**
** Inputs:
**	psq_cb	
**	
**	word1	Should be ON_ERROR
**
**	word2	Should be TRANSACTION or SAVEPOINT
**
**
** Outputs:
**	None
**
** History
**	30-jul-93 (robf)
**		First created, code modularized out of pslsgram.yi
**	14-feb-94 (robf)
**          Disallow SET SESSION WITH ON_ERROR in STAR, currently
**	    non-supported functionality. (This already happened in
**          the grammer, but got moved here now we allow some SET SESSION
**          functionality it STAR)
**	26-Feb-2001 (jenjo02)
**	    Set session_id in QEF_RCB;
**	13-Jul-2005 (thaju02)
**	    Added SET SESSION WITH ON_USER_ERROR = ROLLBACK TRANSACTION.
**
**
*/
DB_STATUS
psl_us12_set_nonkw_roll_nonkw(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	char	    *word2
)
{
	QEF_RCB			*qef_rcb;
	i4			err_code;
	DB_STATUS		status;

	if (STcasecmp(word1, "on_user_error") == 0)
	{
	    if (cb->pss_distrib & DB_3_DDB_SESS)
	    {
		(VOID) psf_error(6350L, 0L, PSF_USERERR,
			&err_code, &psq_cb->psq_error, 1,
			(i4) (sizeof("SET SESSION WITH ON_SYNTAX_ERROR")-1),
			"SET SESSION WITH ON_SYNTAX_ERROR");
		return(E_DB_ERROR);
	    }
	    /* Can only specify this once */
	    if(psq_cb->psq_ret_flag & (PSQ_ON_USRERR_NOROLL |
                                   PSQ_ON_USRERR_ROLLBACK))
	    {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
			(i4) sizeof("SET SESSION")-1, "SET SESSION",
			(i4) sizeof("ON_USER_ERROR")-1, "ON_USER_ERROR");
		return(E_DB_ERROR);
	    }
	    if (!STcasecmp(word2, "transaction"))
	    {
		psq_cb->psq_ret_flag &= ~PSQ_ON_USRERR_NOROLL;
		psq_cb->psq_ret_flag |= PSQ_ON_USRERR_ROLLBACK;
	    }
	    else
	    {
		(VOID) psf_error(E_PS0F87_SET_ON_USER_ERROR, 0L,
			PSF_USERERR, &err_code, &psq_cb->psq_error,
			1, (i4) STlength(word2), word2);
		return(E_DB_ERROR);
	    }
	}
	else
	{
	    /*
	    ** Rollback statement on error option.
	    */
	    /*
	    ** Only valid with SET SESSION WITH ON_ERROR =
	    */
	    if (STcasecmp(word1, "on_error"))
	    {
		_VOID_ psf_error(E_PS0F83_SET_ONERROR_STMT, 0L, PSF_USERERR,
		    &err_code, &psq_cb->psq_error, 1, 
			    (i4) STlength(word1), word1);
		return (E_DB_ERROR);
	    }
	    /* "set session with on_error" is not allowed in distributed yet */
	    if (cb->pss_distrib & DB_3_DDB_SESS)
	    {
		i4	err_code;

		(VOID) psf_error(6350L, 0L, PSF_USERERR,
		    &err_code, &psq_cb->psq_error, 1,
		    (i4) sizeof("SET SESSION WITH ON_ERROR")-1, 
			    "SET SESSION WITH ON_ERROR");
		return (E_DB_ERROR);
	    }
	    /*
	    ** Can only specify this once
	    */
	    if(psq_cb->psq_ret_flag & PSQ_SET_ON_ERROR)
	    {
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    &psq_cb->psq_error, 2,
		    (i4) sizeof("SET SESSION")-1, "SET SESSION",
		    (i4) sizeof("ON_ERROR")-1, "ON_ERROR");

		return E_DB_ERROR;
	    }

	    psq_cb->psq_ret_flag|= PSQ_SET_ON_ERROR;


	    status = psf_malloc(cb, &cb->pss_ostream, sizeof(QEF_RCB), (PTR *) &qef_rcb,
		&psq_cb->psq_error);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    status = psf_mroot(cb, &cb->pss_ostream, (PTR) qef_rcb, &psq_cb->psq_error);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    cb->pss_object = (PTR) qef_rcb;

	    qef_rcb->qef_length 	= sizeof(QEF_RCB);
	    qef_rcb->qef_type 	= QEFRCB_CB;
	    qef_rcb->qef_owner 	= (PTR)DB_PSF_ID;
	    qef_rcb->qef_ascii_id 	= QEFRCB_ASCII_ID;
	    qef_rcb->qef_eflag 	= QEF_INTERNAL;
	    qef_rcb->qef_sess_id    = cb->pss_sessid;
	    /* 
	    ** Allocate memory for alter control block.
	    */
	    status = psf_malloc(cb, &cb->pss_ostream, (i4) sizeof(QEF_ALT), 
		(PTR *) &qef_rcb->qef_alt, &psq_cb->psq_error);
	    if (DB_FAILURE_MACRO(status))
		return (status);

	    qef_rcb->qef_asize = 1;
	    qef_rcb->qef_alt->alt_code = 0;

	    /*
	    ** Validate rollback type - must be STATEMENT or TRANSACTION.
	    ** Fill in on_error type in qef_rcb control block.
	    */
	    if (!STcasecmp(word2, "statement"))
	    {
		qef_rcb->qef_alt->alt_code = QEC_ONERROR_STATE;
		qef_rcb->qef_alt->alt_value.alt_i4 = QEF_STMT_ROLLBACK;
	    }
	    else if (!STcasecmp(word2, "transaction"))
	    {
		qef_rcb->qef_alt->alt_code = QEC_ONERROR_STATE;
		qef_rcb->qef_alt->alt_value.alt_i4 = QEF_XACT_ROLLBACK;
	    }
	    else
	    {
		_VOID_ psf_error(E_PS0F83_SET_ONERROR_STMT, 0L, PSF_USERERR, 
		    &err_code, &psq_cb->psq_error, 1, (i4) 
			    STlength(word2), word2);
		return (E_DB_ERROR);    /* non-zero return means error */
	    }
	}
	return E_DB_OK;
}


/*
** Name: psl_us14_set_nonkw_eq_int
**
** Description: 
**	Handles SET SESSION WITH PRIORITY = nnnn;
**
** Inputs:
**	psq_cb	
**	
**	word1	Should be PRIORITY
**
**	word2	new priority
**
**
** Outputs:
**	None
**
** History
**	22-oct-93 (robf)
**	    Created
**
*/
DB_STATUS
psl_us14_set_nonkw_eq_int(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word1,
	i4	    word2
)
{
	QEF_RCB			*qef_rcb;
	i4			err_code;
	DB_STATUS		status;

	/*
	** Only valid with SET SESSION WITH PRIORITY =
	*/
        if (STcasecmp(word1, "priority")==0)
	{
		if(psq_cb->psq_ret_flag & PSQ_SET_PRIORITY)
		{
			/*
			** Can only have once
			*/
		        (VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
		    		&psq_cb->psq_error, 2,
			    (i4) sizeof("SET SESSION")-1, "SET SESSION",
			    (i4) sizeof("PRIORITY")-1, "PRIORITY");
		        return (E_DB_ERROR);    /* non-zero return means error */
		}
		psq_cb->psq_ret_flag |= PSQ_SET_PRIORITY;
		psq_cb->psq_priority = word2;
	}
	else
	{
		/*
		** Invalid option
		*/
		(VOID) psf_error(6353L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
		    (i4) sizeof("SET SESSION")-1, "SET SESSION",
		    (i4) sizeof("PRIORITY")-1, "PRIORITY");
		return (E_DB_ERROR);    /* non-zero return means error */
	}
	return E_DB_OK;
}

/*
** Name: psl_set_us15_set_nonkw
**
** Description: 
**	Handles SET SESSION WITH NOPRIVILEGES/NODESCRIPTION
**
** Inputs:
**	psq_cb	
**	
**	word	the nonkeyword
**
** Outputs:
**	None
**
** History
**	15-nov-93 (robf)
**	    Created
**
*/
DB_STATUS
psl_us15_set_nonkw(
	PSQ_CB	    *psq_cb,
	PSS_SESBLK  *cb,
	char	    *word
)
{
	i4			err_code;
	DB_STATUS		status;

	if (!STcasecmp( word, "noprivileges"))
	{
		status=psl_us10_set_priv(psq_cb, cb, PSQ_SET_NOPRIV, 0);
		if(DB_FAILURE_MACRO(status))
			return status;
	}
	else if (!STcasecmp( word, "nodescription"))
	{
	    if(psq_cb->psq_ret_flag & PSQ_SET_DESCRIPTION)
	    {
		/*
		** Can only have once
		*/
		(VOID) psf_error(6351L, 0L, PSF_USERERR, &err_code,
			&psq_cb->psq_error, 2,
		    (i4) sizeof("SET SESSION")-1, "SET SESSION",
		    (i4) sizeof("DESCRIPTION")-1, "DESCRIPTION");
		return (E_DB_ERROR);  /* non-zero return means error */
	    }
	    status = psf_malloc(cb, &cb->pss_ostream,1,
				(PTR *) &psq_cb->psq_desc,
				&psq_cb->psq_error);
	    if (DB_FAILURE_MACRO(status))
		return(status);
	    /* Empty string */
	    psq_cb->psq_desc[0]=0;

	    psq_cb->psq_ret_flag |= PSQ_SET_DESCRIPTION;
	}
	else
	{
	    _VOID_ psf_error(E_PS0F82_SET_SESSION_STMT, 0L, PSF_USERERR, 
		&err_code, &psq_cb->psq_error, 1, (i4) 
		STlength(word), word);
	    return (E_DB_ERROR);    /* non-zero return means error */
	}
	return E_DB_OK;
}
