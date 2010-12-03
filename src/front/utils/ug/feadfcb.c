/*
** Copyright (c) 1989, 2010 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<nm.h>
#include	<me.h>
#include	<st.h>
#include	<cm.h>
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<fedml.h>
#include 	<adf.h>
#include	<ug.h>
#include	<tm.h>
#include	<tmtz.h>
#include	<pm.h>
#include	<aduucol.h>

/**
** Name:	feadfcb.c -	Front-End Utility ADF Control Block Access.
**
** Description:
**	Contains the routine that allocates an ADF control block for the
**	front-ends, and returns a reference to the allocated ADF control block.
**	Defines:
**
** 	FEadfcb()	return an ADF_CB.
**
** History:
**	Revision 6.4  89/11/27  wong
**	Set default arithmetic exception handling to "ERR".
**
**	Revision 6.0  87/02/04  daver
**	Initial revision.
**	11/09/87 (dkh) - Put in fix for "now".
**	 13-mar-1996 (angusm)
**		format MULTINATIONAL4 to give MULTINATIONAL behaviour
**		but with 4-digit year.
**	18-dec-1996(angusm)
**	Add IIUGlocal_decimal() to overwrite decimal point value
**	with value from II_APP_DECIMAL (bugs 75776, 79824).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	27-jul-2001 (stephenb)
**	    Add inittialization of ucollation for ADF_CB.
**      27-Nov-2001 (hweho01)
**          Fixed the mismatch in ADF_CB structure initialization, 
**	31-Jan-2003 (hanch04)
**	    If II_INSTALLATION is not set, then it's a new install.
**	    Don't print out TIMEZONE error.
**	03-Jan-2003 (hanch04)
**	    Redo last change.  II_INSTALLATION will be set in an upgrade
**	    installation, 
**	    but II_CONFIG will be set to $II_SYSTEM/ingres/install and
**	    the zoneinfo directory is not there.  Only display error if
**	    the $II_CONFIG/zoneinfo is there and the TM functions failed.
**	04-Mar-2003 (hanje04)
**	    Change all 'size' variables used in ME calls to SIZE_TYPE to 
**	    prevent problems on som 64bit platforms.
**	03-sep-2003 (abbjo03)
**	    Add missing parameters in call to adg_startup().
**	26-Jun-2007 (kibro01) b118511, b118230
**	    Include <pm.h> to avoid SIGSEGV on LP64 platforms 
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**      27-Mar-2009 (hanal04) Bug 121857
**          Too many arguements passed to adg_startup(). Found whilst
**          building with xDEBUG defined in adf.h
**	22-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**	aug-2009 (stephenb)
**		Prototyping front-ends
**      01-Dec-2009 (hanal04) Bug 122938
**          Add initialisation of adf_max_decprec to cb declaration.
**      07-Dec-2009 (maspa05) Bug 122806
**          Apply changes to thread specific ADF_CB rather than static default
**          in FEset_* functions 
**      08-Jan-2009 (maspa05) Bug 123127
**          Moved declaration of IILQasGetThrAdfcb to libq.h
**      15-Jan-2009 (maspa05) Bug 123141
**          Removed FEset_* functions and made FEapply_* equivalents external
**          FEset_* were just wrappers calling FEapply_* with the current
**          ADF_CB as returned by IILQasGetThrAdfcb - which is in libq. 
**          However there are front-ends (repserv) that use FEadfcb but not
**          link libq
**	20-Jan-2010 (hanje04)
**         SIR 123296
**         Add LSB option, writable files are stored under ADMIN, logs under
**         LOG and read-only under FILES location.
**      30-Nov-2010 (hanal04) Bug 124758
**         Add initialisation of adf_max_namelen to cb declaration.
**/
GLOBALREF DB_STATUS     (*IIugefa_err_function)();

DB_STATUS	IIUGnfNowFunc();
DB_STATUS	IIUGlocal_decimal();

/*{
** Name:	FEadfcb() -	 return an ADF_CB.
**
** Description:
**	FEadfcb returns a pointer to the ADF_CB to be used by frontends.
**	This ADF_CB is a single global structure. 
**
**	This routine MUST be called before calling afe routines, 
**	although it can be used directly in an afe routine call:
**		afe_xxx(FEadfcb(),...); is legal.
**
**	This routine can be called as often as one wishes: in each afe 
**	call, or just called once, by saving the pointer returned:
**		ADF_CB 	*cb;
**		cb = FEadfcb(); 
**		afe_xxx(cb,...);
**	is also legal, and may be more effecient.
**
** Returns:
**	{ADF_CB *}  A reference to the allocated ADF control block.
**
** History:
**	02-feb-1987 (daver)
**		Written.
**	09-apr-1987 (peter)
**		Added call to iiuglcd_langcode.
**	23-apr-1987	(daver)
**		Loaded ADF_CB with FEdml, and Multinational information.
**	05/29/87 (dkh) - Added third argument in call to adg_startup() to
**			 reflect reality.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	26-oct-1987 (peter)
**		Add support for II_NULLSTRING.
**	11/09/87 (dkh) - Put in fix for "now".
**	11/27/89 (jhw) - Default arithmetic exception handling is now "ERR".
**	22-Jan-90 (GordonW)
**		'NMgtAt()' output value for II_NULL_STRING should be tested 
**		for both NULL and whether it is empty.
**	27-nov-90 (jrb)
**	    Set adf_timezone field of adf session cb with value from TMzone.
**	2-apr-92 (seg)
**	    Fixed incorrect number of initializers in ADF_CB structure.
**	9-jul-1992 (mgw)
**		Took out TMzone() call and added new ADF_CB member,
**		adf_tzcb in place of timezone. Also added new International
**		Date Formats YMD, DMY, and MDY
**	31-jul-92 (edf)
**		Fixed bug 42445 by appending a blank to II_MONEY_FORMAT
**		if it is either 'L:' or 'R:'. This caused connection
**		failure in LIBQ when using I/Net.
**	03-aug-92 (edf)
**		Reversed the fix for bug 42445. The fix could cause false
**		support of blank as a valid currency symbol.
**	11-nov-1992 (mgw)
**		Don't try to report errors from TMtz_init() call. Can't
**		report errors via IIUGerr() until after adg_init() call
**		and Timezone errors will be reported in IIcc1_connect_params()
**		anyway.
**	10-sep-93 (kchin)
**		Changed structure initialization of ADF_CB, for its last
**		4 elements, namely a PTR and 3 i4's, the initialization
**		should be NULL, and 3 0's respectively.  Use of 0L for i4
**		element will result in compilation error on 64-bit platform.
**	20-oct-93 (donc)
**		Since adf_lo_proto and adf_strtrunc_opt were added ADF_CB,
**		I've decided to initialize them to 0 and ignore truncation 
**		explicitly
**	28-oct-93 (donc)
**		Prior change (20-oct-93) states adf_lo_proto was initialized,
**		the real attribute name is adf_lo_context. I also changed 
**		the initialization of adf_collation and adf_tzcb to reflect
**		that they are declared as PTR.
**	 7-jan-94 (swm)
**		Bug #58635
**		Added PTR casts for assignments to tabi_l_reserved and
**		tabi_owner which have changed type to PTR.
**	13-mar-1996 (angusm)
**		Support new setting for II_DATE_FORMAT = "MULTINATIONAL4"
**	20-mar-96 (nick)
**	    Add support for II_DATE_CENTURY_BOUNDARY.
**	25-Mar-98 (kitch01)
**		Bug 69663. Add extra check for II_MONEY_FORMAT to ensure it
**		has been set correctly to {L|T}:symbol.
**	06-Jul-1999 (shero03)
**	    Support II_MONEY_FORMAT=NONE. Sir 92541
**      17-Jul-2002 (horda03) Bug 108121
**              Above change indicates that Timezone problems will be reported
**              elsewhere from TMtz_init. Well that isn't the case. What is
**              happening is if session can't access the Timezone file for
**              any reason, then the sessiondefaults to GMT. As this is done
**              with the user knowing, this can lead to incorrect dates being
**              entered.
**              Now if we have a problem initialising the Timezone infomation,
**              report the problem once the adg_init() call has completed.
**	 5-sep-03 (hayke02)
**	    Modify the above change so that we skip the error reporting for
**	    TM_PMFILE_OPNERR (E_CL1F0B) which is expected during ingbuild
**	    startup. This change fixes bug 110850.
**	26-sep-03 (hayke02)
**	    Addition to above change - also check for a non-zero tmtz_status.
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
**	03-Oct-2005 (toumi01) BUG 115098
**	    If FE startup fails sanity check our setup. Otherwise we may exit
**	    silently from EX without any indication of the cause of failure.
**	18-Jun-2007 (kibro01) b118511, b118230
**	    When we get the timezone from config.dat, also look up the
**	    date_type_alias field so it is set for all FE tools.
**	10-Jul-2007 (kibro01) b118702
**	    Always use INGRESDATE as default, and check the new setting
**	    in 'ii.$.config.*.date_type_alias'
**	14-Sep-2007 (gupsh01) 
**	    For UTF8 installation for front end programs unicode 
**	    collation is not initialized.
**	18-Oct-2007 (kibro01) b119318
**	    Use a common function in adu to determine valid date formats
**	7-Nov-2007 (kibro01) b119432
**	    Separate out setting of money and date formats to allow them to
**	    be set through the standard set statements in the front-end.
**	05-Nov-2007 (kiria01) 
**	    Initialise the default substitution character for unmappable
**	    Unicode data that gets through to the tm and provide a means of
**	    altering the default with II_UNICODE_SUBS environment varaiable.
**	15-Sep-2008 (jonj)
**	    SIR 120874: Add ad_dberror inside adf_errcb in ADF_CB static.
**	05-Feb-2009 (wanfr01)
**	    Bug 121617
**	    Do not blindly copy 4 bytes in IIUGnfNowFunc because you might
**	    be copying the value into a long rather than an i4.
*/

static bool	done = FALSE;

static ADF_CB  cb = {
	NULL,					/* adf_srv_cb */
	DB_US_DFMT,				/* adf_dfmt */
	{ERx("$"),ERx(""),DB_LEAD_MONY,2},	/* adf_mfmt */
	{ TRUE, '.' },				/* adf_decimal */
	{					/* adf_outarg */
	  -1,					/* .ad_c0width */
	  -1,					/* .ad_t0width */
	  -1,					/* .ad_i1width */
	  -1,					/* .ad_i2width */
	  -1,					/* .ad_i4width */
	  -1,					/* .ad_f4width */
	  -1,					/* .ad_f8width */
	  -1,					/* .ad_f4prec */
	  -1,					/* .ad_f8prec */
	  -1,					/* .ad_i8width */
	  -1,					/* .ad_2_rsvd */
	  NULLCHAR,				/* .ad_f4style */
	  NULLCHAR,				/* .ad_f8style */
	  NULLCHAR,				/* .ad_3_rsvd */
	  NULLCHAR 				/* .ad_4_rsvd */
	},					/* adf_outarg */
	{					/* adf_errcb */
	  E_AD0000_OK,				/* .ad_errcode */
	  0,					/* .ad_errclass */
	  0,					/* .ad_usererr */
	  { 0 },				/* .ad_sqlstate */
	  0,					/* .ad_ebuflen */
	  0,					/* .ad_emsglen */
	  NULL,					/* .ad_errmsgp */
	  { 0, 0, NULL, 0 }			/* .ad_dberror */
	},
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },	/* adf_warncb */
	ADX_ERR_MATHEX,				/* adf_exmathopt */
	DB_SQL,					/* adf_qlang */
	{ 0, ERx("") },				/* adf_nullstr */
	NULL,					/* adf_constants */
	0,					/* adf_slang */
	DB_GW4_MAXSTRING,			/* adf_maxstring */
	NULL,					/* adf_collation */
	0,					/* adf_rms_context */
	NULL,					/* adf_tzcb */
	0,					/* adf_proto_level */
        CL_MAX_DECPREC,                         /* adf_max_decprec */
        DB_GW1_MAXNAME,				/* adf_max_namelen */
	0,					/* adf_lo_context */
	ADF_IGN_STRTRUNC,			/* adf_strtrunc_opt */
	TM_DEF_YEAR_CUTOFF,			/* adf_year_cutoff */
	NULL,					/* adf_ucollation */
	AD_UNISUB_OFF,				/* adf_unisub_status */
	{'$',0},				/* adf_unisub_char */
	AD_UNINORM_NFD,				/* adf_uninorm_flag */
	NULL,					/* adf_autohash */
};

static ADF_TAB_DBMSINFO	*dbinfo;

static char	ebuffer[DB_ERR_SIZE];

void
FEapply_date_format(ADF_CB *cb, char *date_format)
{
    i4 date_fmt;
    CVlower(date_format);

    /* Use common function for date formats (kibro01) b119318 */
    date_fmt = adu_date_format(date_format);
    if (date_fmt == -1)
    {
    	IIUGerr(E_UG0020_BadDateFormat, 0, 1, (PTR)date_format);
	/* default is "us" */
    } else
    {
    	cb->adf_dfmt = date_fmt;
    }
}

void
FEapply_money_format(ADF_CB *cb, char *money_format)
{
    CMtolower(money_format, money_format);
    if ((*money_format == 'n') && STlength(money_format) == 4)
    {	
    	if ( ((*(money_format+1) == 'o') || (*(money_format+1) == 'O')) &&
             ((*(money_format+2) == 'n') || (*(money_format+2) == 'N')) &&
             ((*(money_format+3) == 'e') || (*(money_format+3) == 'E')) )
       {
           cb->adf_mfmt.db_mny_lort = DB_NONE_MONY;
       }
       else
       {
	IIUGerr(E_UG0021_BadMnyFormat, 0, 1, (PTR)money_format);
       }		
    }   	
    else if ((*money_format != 'l' && *money_format != 't' && *money_format != 'n') ||
    	 *(money_format+1) != ':' /* {l|t}:$$$ */ ||
         STlength(money_format) > DB_MAXMONY + 2 || STlength(money_format) < 3)
    {
	IIUGerr(E_UG0021_BadMnyFormat, 0, 1, (PTR)money_format);
	/* default is "l:$"; leading currency symbol of "$" */
    }
    else
    {
    	if ( *money_format == 'l')
	{
    	   cb->adf_mfmt.db_mny_lort = DB_LEAD_MONY;
	}		      
	else if ( *money_format == 't' )
	{ 
    	   cb->adf_mfmt.db_mny_lort = DB_TRAIL_MONY;
	}
       STcopy( money_format + 2, cb->adf_mfmt.db_mny_sym );
    }
}

void
FEapply_money_prec(ADF_CB *cb, char *money_prec)
{
    if (*money_prec == '0')
    {
    	cb->adf_mfmt.db_mny_prec = 0;
    }
    else if (*money_prec == '1')
    {
   	cb->adf_mfmt.db_mny_prec = 1;
    }
    else if (*money_prec != '2')
    {
    	IIUGerr(E_UG0022_BadMnyPrec, 0, 1, (PTR)money_prec);
	/* default is 2 */
    }
}

void
FEapply_decimal(ADF_CB *cb, char *decimal)
{
    if (*(decimal+1) != EOS || (*decimal != '.' && *decimal != ','))
    {
	IIUGerr(E_UG0023_BadDecimal, 0, 1, (PTR)decimal);
	/* default is `.' */
    }
    else
    {
    	cb->adf_decimal.db_decimal = decimal[0];
    }
}

void
FEapply_null(ADF_CB *cb, char *null)
{
    if ( (cb->adf_nullstr.nlst_string = STalloc(null)) == NULL )
    {
	EXsignal(EXFEBUG, 1, ERx("FEadfcb(alloc2)"));
    }
    cb->adf_nullstr.nlst_length = STlength(null);	    
}

ADF_CB *
FEadfcb ()
{
    if (!done)
    {
	char		*cp;
	DB_STATUS	rval;
	PTR		srv_cb_ptr;
	SIZE_TYPE	srv_size;
	SIZE_TYPE	dbinfo_size;
	PTR		dbinfoptr;
        STATUS          tmtz_status;
        STATUS          date_status;
	char		date_type_alias[100];
        char            col[] = "udefault";
    	STATUS          status = OK;
    	CL_ERR_DESC     sys_err;
	ADUUCETAB	*ucode_ctbl; /* unicode collation information */
        PTR         	ucode_cvtbl;


	
    	done = TRUE;

	/* first, get the size of the ADF's server control block */
	srv_size = adg_srv_size();

	/* allocate enough memory for it */
	if ((srv_cb_ptr = MEreqmem(0, srv_size, FALSE, (STATUS *)NULL)) == NULL)
	{
    	    EXsignal(EXFEBUG, 1, ERx("FEadfcb(alloc)"));
	}

	/*
	**  Fix up code so that "now" works properly for frontends.
	**  Only allocating enough for one (1) dbmsinfo() request.
	*/
	dbinfo_size = sizeof(ADF_TAB_DBMSINFO);
	if ((dbinfoptr = MEreqmem(0, dbinfo_size, TRUE, (STATUS*)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("FEadfcb"));
	}

	dbinfo = (ADF_TAB_DBMSINFO *) dbinfoptr;

	dbinfo->tdbi_next = NULL;
	dbinfo->tdbi_prev = NULL;
	dbinfo->tdbi_length = dbinfo_size;
	dbinfo->tdbi_type = ADTDBI_TYPE;
	dbinfo->tdbi_s_reserved = -1;
	dbinfo->tdbi_l_reserved = (PTR)-1;
	dbinfo->tdbi_owner = (PTR)-1;
	dbinfo->tdbi_ascii_id = ADTDBI_ASCII_ID;
	dbinfo->tdbi_numreqs = 1;

	/*
	**  Now define request.
	*/
	dbinfo->tdbi_reqs[0].dbi_func = IIUGnfNowFunc;
	dbinfo->tdbi_reqs[0].dbi_num_inputs = 0;
	dbinfo->tdbi_reqs[0].dbi_lenspec.adi_lncompute = ADI_FIXED;
	STcopy("_BINTIM", dbinfo->tdbi_reqs[0].dbi_reqname);
	dbinfo->tdbi_reqs[0].dbi_reqcode = 1;
	dbinfo->tdbi_reqs[0].dbi_lenspec.adi_fixedsize = 4;
	dbinfo->tdbi_reqs[0].dbi_dtr = DB_INT_TYPE;

	/*
	** set timezone in ADF cb  - Any errors must be reported
        ** after adg_init() called.
	*/

	if ( (tmtz_status = TMtz_init(&cb.adf_tzcb)) == OK)
        {
	   tmtz_status = TMtz_year_cutoff(&cb.adf_year_cutoff);
        }

	/* Always use INGRESDATE by default, and check 
	** 'ii.<node>.config.date_alias' (kibro01) b118702
	*/
	cb.adf_date_type_alias = AD_DATE_TYPE_ALIAS_INGRES;
	STprintf(date_type_alias,ERx("ii.%s.config.date_alias"),PMhost());
	date_status = PMget(date_type_alias, &cp);
	if (date_status == OK && cp != NULL && STlength(cp))
	{
	    if (STbcompare(cp, 0, ERx("ansidate"), 0, 1) == 0)
		cb.adf_date_type_alias = AD_DATE_TYPE_ALIAS_ANSI;
	    else if (STbcompare(cp, 0, ERx("ingresdate"), 0, 1) == 0)
		cb.adf_date_type_alias = AD_DATE_TYPE_ALIAS_INGRES;
	    else
		cb.adf_date_type_alias = AD_DATE_TYPE_ALIAS_NONE;
	}

	/* set timezone table to NULL in ADF cb, adg_init() will fill it */
	/* cb.adf_tz_table = NULL; - not needed, done in declaration above */

	/* Start up ADF */
    	rval = adg_startup(srv_cb_ptr, srv_size, dbinfo, 0);
	if (DB_FAILURE_MACRO(rval))
	{
	    /*
	    ** Before bailing out, try to be helpful. Since the environment
	    ** is likely not set up correctly, write hard coded messages
	    ** since error message fetch will likely fail in this situation.
	    */
	    char	*tempchar;
	    i4		dummy;
	    LOCATION	temploc;
	    NMgtAt("II_SYSTEM", &tempchar);
	    if (tempchar && *tempchar)
	    {
		LOfroms(PATH, tempchar, &temploc);
		if (LOexist(&temploc))
		{
		    tempchar = ERx("FEadfcb: II_SYSTEM DOES NOT EXIST\n");
		    SIwrite(STlength(tempchar), tempchar, &dummy, stderr);
		}
		else
		{
		    NMloc(ADMIN, FILENAME, ERx("config.dat"), &temploc);
		    if (LOexist(&temploc))
		    {
			tempchar = ERx("FEadfcb: II_SYSTEM IS NOT SET CORRECTLY\n");
			SIwrite(STlength(tempchar), tempchar, &dummy, stderr);
		    }
		}
	    }
	    else
	    {
		tempchar = ERx("FEadfcb: II_SYSTEM IS NOT SET\n");
		SIwrite(STlength(tempchar), tempchar, &dummy, stderr);
	    }
    	    EXsignal(EXFEBUG, 1, ERx("FEadfcb(start)"));
	}

	/* put the pointer to ADF's server control block in the ADF_CB */
	cb.adf_srv_cb = srv_cb_ptr;

	/* set up the error message buffer */
    	cb.adf_errcb.ad_ebuflen = DB_ERR_SIZE;
    	cb.adf_errcb.ad_errmsgp = ebuffer;

	/* initialize the ADF_CB */
    	rval = adg_init(&cb);
	if (DB_FAILURE_MACRO(rval))
	{
    	    EXsignal(EXFEBUG, 1, ERx("FEadfcb(init)"));
	}

	/* find out which natural language we should speak */
	cb.adf_slang = iiuglcd_langcode();

	/* Always QUEL; for SQL explictly change this with 'IIAFdsDmlSet()'. */
	cb.adf_qlang = DB_QUEL;

	/* lets get the multinational info */

	/* 
	** Defaults for all of these are initially set statically, above.
	*/
	NMgtAt(ERx("II_DATE_FORMAT"), &cp);
    	if ( cp != NULL && *cp != EOS )
    	{
	    FEapply_date_format(&cb,cp);
    	}

	NMgtAt(ERx("II_MONEY_FORMAT"), &cp);
    	if ( cp != NULL && *cp != EOS )
    	{
	    FEapply_money_format(&cb,cp);
    	}

	NMgtAt(ERx("II_MONEY_PREC"), &cp);
    	if ( cp != NULL && *cp != EOS )
    	{
	    FEapply_money_prec(&cb,cp);
    	}

	NMgtAt(ERx("II_DECIMAL"), &cp);
    	if ( cp != NULL && *cp != EOS )
    	{
	    FEapply_decimal(&cb,cp);
    	}

	NMgtAt(ERx("II_UNICODE_SUBS"), &cp);
	if ( !cp )
	{
	    cb.adf_unisub_status = AD_UNISUB_OFF;
	}
	else
	{
	    /* As with SET unicode_substitution take the first char */
	    cb.adf_unisub_status = AD_UNISUB_ON;
	    *cb.adf_unisub_char = cp[0];
	}

	NMgtAt(ERx("II_NULL_STRING"), &cp);
	if ( cp != NULL && *cp != EOS )
	{
	    FEapply_null(&cb,cp);
	}

	/* If there was a Timezone error other than the expected ingbuild
	** error TM_PMFILE_OPNERR, then lets report it now. */

        if (tmtz_status && (tmtz_status != TM_PMFILE_OPNERR))
        {
	    LOCATION    loc_root;
	    STATUS status = OK;
	    status = NMloc(FILES, PATH, ERx("zoneinfo"), &loc_root);
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
            status = LOfaddpath(&loc_root, ERx("lp64"), &loc_root);
#endif
#if defined(conf_BUILD_ARCH_64_32) && defined(BUILD_ARCH32)
    {
        /*
        ** Reverse hybrid support must be available in ALL
        ** 32bit binaries
        */
        char    *rhbsup;
        NMgtAt("II_LP32_ENABLED", &rhbsup);
        if ( (rhbsup && *rhbsup) &&
       ( !(STbcompare(rhbsup, 0, "ON", 0, TRUE)) ||
         !(STbcompare(rhbsup, 0, "TRUE", 0, TRUE))))
            status = LOfaddpath(&loc_root, "lp32", &loc_root);
    }
#endif /* ! LP64 */

            if ( status == OK )
            {
		status = LOexist(&loc_root);
            }
            if ( status == OK )
	    {
                   IIUGerr(tmtz_status, 0, 0);

                   /* As this may disappear from the screen quickly,
                   ** let's pause to allow the users to reflect on the
                   ** fact that the timezone info is (probably) wrong.
                   */
                   if (!IIugefa_err_function) PCsleep(5000);
	    }
        }

	/* Initialize unicode collation for UTF8 installations */
	if (CMischarset_utf8())
	{
          if (status = aduucolinit(col, MEreqmem, 
			&ucode_ctbl, &ucode_cvtbl, &sys_err))
          {
    	    EXsignal(EXFEBUG, 1, ERx("FEadfcb(alloc)"));
          }

          cb.adf_ucollation = (PTR) ucode_ctbl;

	  /* Set unicode normalization to NFC just in case 
	  ** Front end will need it.
	  */
          if ((status = adg_setnfc(&cb)) != E_DB_OK)
    	    EXsignal(EXFEBUG, 1, ERx("FEadfcb(alloc)"));
	}
	/* lets not do this again! */
    }

    return &cb;
}


DB_STATUS
IIUGnfNowFunc(dbi, db1, dbr, err)
ADF_DBMSINFO	*dbi;
DB_DATA_VALUE	*db1;
DB_DATA_VALUE	*dbr;
DB_ERROR	*err;
{
	SYSTIME	time;
	i8 val8;

	/*
	**  To get things going, this routine assumes everything
	**  that is passed in is OK.  Inputs "db1" and "err" are
	**  ignored for now.
	*/
	TMnow(&time);
	switch (dbr->db_length)
	{
	    case 8:
 		val8 = time.TM_secs;
		I8ASSIGN_MACRO(val8, *(i8 *)dbr->db_data);
		break;
	    case 4:
		I4ASSIGN_MACRO(time.TM_secs, *(i4 *)dbr->db_data);
		break;
	    default:
		return(E_DB_ERROR);
	}
	return(E_DB_OK);
}
DB_STATUS
IIUGlocal_decimal(ADF_CB *a, PTR cp1)
{
	DB_STATUS	status = E_DB_OK;
	PTR		cp;

	/* Determine the decimal point character for source input */
	NMgtAt(ERx("II_APP_DECIMAL"), &cp);
    	if ( cp != NULL && *cp != EOS )
    	{
	    if (*(cp+1) != EOS || (*cp != '.' && *cp != ','))
	    {
		status = E_DB_ERROR;
		*cp1 = *cp;
		/* default is `.' */
	    }
	    else
	    {
	    	a->adf_decimal.db_decimal = cp[0];
		status = E_DB_OK;
	    }
    	}
return(status);
}

