/*
** Copyright (c) 1986, 2005, 2010 Ingres Corporation
**
**
*/
# include    <compat.h>
# include    <gl.h>
# include    <cs.h>
# include    <mh.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <ddb.h>
# include    <dudbms.h>
# include    <ulm.h>
# include    <ulf.h>
# include    <adf.h>
# include    <adulcol.h>
# include    <aduucol.h>
# include    <dmf.h>
# include    <dmtcb.h>
# include    <scf.h>
# include    <qsf.h>
# include    <qefrcb.h>
# include    <rdf.h>
# include    <cui.h>
# include    <psfparse.h>
# include    <qefnode.h>
# include    <qefact.h>
# include    <qefqp.h>
# include    <intl.h>
# include    <cm.h>
# include    <si.h>
# include    <pc.h>
# include    <lo.h>
# include    <ex.h>
# include    <er.h>
# include    <cv.h>
# include    <st.h>
# include    <me.h>
# include    <generr.h>
# include    <usererror.h>
/* beginning of optimizer header files */
# include    <opglobal.h>
# include    <opdstrib.h>
# include    <opfcb.h>
# include    <ophisto.h>
# include    <opq.h>
# include    <math.h> 
# include    <stdarg.h>
# include    <gwf.h>

exec sql include sqlca;
exec sql include sqlda;
exec sql declare s statement;
exec sql declare c1 cursor for s;

/*
**
**  Name: OPQUTILS.C - optimizedb and statdump utility routines
**
**  Description:
**      Routines in common with both optimizedb and statdump
**
**
**  History:
**      15-jan-87 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	08-jul-89 (stec)
**	    Merged into terminator path (changed ERlookup call).
**	    Changed code to reflect new interface to adc_lenchk.
**	24-jul-89 (stec)
**	    Changed badarglist to accept Ingres flags which start with a "+".
**	13-sep-89 (stec)
**	    Changed badarglist to accept -zn# rather than -zf# (FRC approved).
**	20-sep-89 (stec)
**	    Recognize -zqq flag.
**	12-dec-89 (stec)
**	    Improve opq_sqlerror routine.
**	12-dec-89 (stec)
**	    Remove FROM clause from the query with dbmsinfo function.
**	    Add generic error handling. Added global definition for utilid.
**	    Moved exception handler to opqutils.sc. Improved error handling
**	    and added error handling for serialization errors (TXs will be
**	    automatically restarted. Made code case sensitive with respect to
**	    database object names. Will count rows when count not available.
**	    Improved exception handling.
**	22-jan-90 (stec)
**	    Modified dbms_info.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	03-apr-90 (stec)
**	    Changed output format for floats to 'n' from 'e'. Changed opq_error
**	    to cast parms to ERlookup() to appropriate types, also removed code
**	    stripping off error id.
**	    Removed automatic initialization of convbuf static variable for
**	    hp3_us5 (HP 9000/320) C compiler.
**	16-may-90 (stec)
**	    Included cv.h, moved FUNC_EXTERNs to opq.h.
**	06-jun-90 (stec)
**	    Fixed bug 20159 in opq_upd().
**	21-jun-90 (stec)
**	    Modified opq_error() to pass correct language code to ERlookup()
**	    and display more info in case of ER failure.
**	27-jun-90 (stec)
**	    Modified opq_error() - added a NL char at the beginning of
**	    hardcoded error message.
**	09-jun-90 (stec)
**	    Removed automatic initialization of convbuf static variable for
**	    mac_us5 C compiler.
**	24-jul-90 (stec)
**	    Modified opq_dbmsinfo() for the RMS gateway.
**	06-nov-90 (stec)
**	    Added minor fix for ASCII C compiler in prelem().
**	08-nov-1990 (stec)
**          Made a code change in opq_error() to get rid of a compiler warning.
**	10-jan-91 (stec)
**	    Changed opq_exhandler() and fileinput().
**	22-jan-91 (stec)
**	    Modified badarglist().
**	20-mar-91 (stec)
**	    Modified badarglist().
**	22-mar-91 (stec)
**	    Modified opq_mxrel().
**	20-jun-91 (rog)
**	    Fixed bug 38065 in fileinput().
**	06-apr-92 (rganski)
**	    Merged change from seg:
**	    Can't use "errno" -- name reserved to C runtime library.  Renamed
**	    to "errnum".
**	31-aug-92 (rganski)
**	    Added comment delimiters after "# endif"s to stop HP
**	    compiler warnings.
**	24-oct-92 (andre)
**	    instead of ERlookup() we will be calling ERslookup()
**	09-nov-92 (rganski)
**	    Added prototypes.
**	    Added opq_adferror(), to replace a lot of common inline code 
**	    for handling ADF errors.
**	    Added opq_translatetype(), to replace a lot of other common inline 
**	    code for translating data types.
**	    Minor cosmetic changes.
**	14-dec-92 (rganski)
**	    Added include of dudbms.h and st.h.
**	18-jan-93 (rganski)
**	    Added opq_print_stats(), which is shared by optimizedb and
**	    statdump. Also added opq_current_date(), which is used by
**	    opq_print_stats().
**	    Moved global variable opq_date here from opqoptdb.sc.
**	    Changed global convbuf to length of OPQ_IO_VALUE_LENGTH, since
**	    boundary values can be much longer now. Alignment is not required.
**	29-apr-93 (rganski)
**	    Changed charnunique and chardensity from local to global variables.
**	21-jun-93 (rganski)
**	    Added include of me.h, for prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	20-sep-93 (rganski)
**	    Added delimited identifier support.
**	18-oct-93 (rganski)
**	    Local collation sequence support: added opq_collation(), include of
**	    adulcol.h.
**      10-apr-95 (nick)(cross-integ angusm)
**      Altered difference between floating point width and precision
**      to eight as this prevents large negative numbers which were
**      overflowing on IEEE machines to print ok.  #56481
**	9-jan-96 (inkdo01)
**	    Added support of per-cell repetition factors to print_stats.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to opqdata.sc.
**      08-jun-1999 (chash01))
**          fetch row_width from iitables for RMS GW. USing it to calculate 
**          number of pages.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      08-jun-1999 (chash01))
**          fetch row_width from iitables for RMS GW. USing it to calculate 
**          number of pages.
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	22-Sep-2000 (hanch04)
**	    Change long to i4.
**	19-apr-2001 (mcgem01)
**	    Update the usage message to incldue some recently added flags
**	    including -zcpk and -zdn
**      18-May-2001 ( huazh01 )
**	    (hanje04) X-Integ for 451660
**          For function opq_print_stats(), reduce the total number of digits
**          after the decimal point when cell_reps is greater than 9,999,999
**          This fix bug 104907.
**	09-May-2002 (xu$we01)
**	    Fixed the compilation warnings by moving the included header file
**	    <math.h> under <cs.h>.
**	18-sep-2002 (abbjo03)
**	    Replace call to log10 by MHlog10 and include of math.h by mh.h.
**      20-Aug-2002 (gilta01) Bug 106456 INGSRV 1615
**          Changed variable lang to an i4 in opq_error(), to match 
**          ERlangcode param.
**	21-Mar-2005 (schka24)
**	    Treat -r"foo bar" as the id "foo bar" even if the dquotes don't
**	    make it through on the command line.  Arrange for delim_name to
**	    always be quoted if quotes are needed; relname will never have
**	    quotes added.  argname will be the name exactly as given on the
**	    command line.
**	22-mar-2005 (thaju02)
**	    Changes for support of "+w" SQL option.
**	27-june-05 (inkdo01)
**	    2 instances of a "select distinct 1 ..." were changed to "select
**	    first 1 1 ..." because it'll go wayyy faster.
**      28-jun-2005 (huazh01)
**          On routine r_rel_list_from_rel_rel, change es_tabtype from
**          char[2] to char[9]. On a distributed database, column 
**          'table_type' on 'iitables' is char(8), not char(1). 
**          This fixes bug 109850, INGSTR54.
**	11-May-2009 (kschendel)
**	    Compiler warning fixes.
**      01-Feb-2010 (maspa05) b123140
**        Disallow multiple -r flags with the same table name
**      11-Feb-2010 (maspa05) b123140
**        Missing parameter in STxcompare in the above change
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
void opq_error(
	i4 dbstatus,
	i4 errnum,
	i4 argcount,
	...);
void opq_sqlerror(void);
void opq_adferror(
	ADF_CB *adfcb,
	i4 errnum,
	i4 p1,
	PTR p2,
	i4 p3,
	PTR p4);
i4 opq_exhandler(
	EX_ARGS *ex_args);
void adfinit(
	OPQ_GLOBAL *g);
void adfreset(
	OPQ_GLOBAL *g);
void nostackspace(
	char *utilid,
	i4 status,
	u_i4 size);
PTR getspace(
	char *utilid,
	PTR *spaceptr,
	SIZE_TYPE size);
void fileinput(
	char *utilid,
	char ***argv,
	i4 *argc);
bool isrelation(
	char *relptr,
	bool *excl);
bool isattribute(
	char *attptr);
static bool good_type(
	DB_DT_ID typeid,
	ADF_CB *opq_adfscbp);
OPS_DTLENGTH align(
	OPS_DTLENGTH realsize);
void setattinfo(
	OPQ_GLOBAL *g,
	OPQ_ALIST **attlist,
	i4 *index,
	OPS_DTLENGTH bound_length);
bool badarglist(
	OPQ_GLOBAL *g,
	i4 argc,
	char **argv,
	i4 *dbindex,
	char *ifs[],
	char ***dbnameptr,
	bool *verbose,
	bool *histoprint,
	bool *key_attsplus,
	bool *minmaxonly,
	i4 *uniquecells,
	i4 *regcells,
	bool *quiet,
	bool *deleteflag,
	char **userptr,
	char **ifilenm,
	char **ofilenm,
	bool *pgupdate,
	f8 *samplepct,
	bool *supress_nostats,
	bool *comp_flag,
	bool *nosetstatistics,
	OPS_DTLENGTH *bound_length,
	char **waitopt,
	bool *nosample);
void prelem(
	OPQ_GLOBAL *g,
	PTR dataptr,
	DB_DATA_VALUE *datatype,
	FILE *outf);
static char *opq_current_date(
	OPQ_GLOBAL *g,
	char *out);
void opq_print_stats(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST *attrp,
	char *version,
	i4 row_count,
	i4 pages,
	i4 overflow,
	char *date,
	f8 nunique,
	f8 reptfact,
	char *unique_flag,
	i1 iscomplete,
	i2 domain,
	i2 cellnm,
	OPO_TUPLES nullcount,
	OPH_COUNTS cell_count,
	OPH_COUNTS cell_repf,
	OPH_CELLS intervals,
	FILE *outf,
	bool verbose,
	bool quiet);
void opq_cntrows(
	OPQ_RLIST *rp);
void opq_phys(
	OPQ_RLIST *rp);
void opq_idxlate(
	OPQ_GLOBAL *g,
	char *src,
	char *xlatename,
	char *delimname);
void opq_idunorm(
	char *src,
	char *dst);
bool i_rel_list_from_input(
	OPQ_GLOBAL *g,
	OPQ_RLIST **rellist,
	char **argv,
	bool statdump);
void opq_mxrel(
	OPQ_GLOBAL *g,
	int argc,
	char *argv[]);
void opq_collation(
	OPQ_GLOBAL *g);
void opq_ucollation(
	OPQ_GLOBAL *g);
void opq_owner(
	OPQ_GLOBAL *g);
void opq_upd(
	OPQ_GLOBAL *g);
void opq_dbmsinfo(
	OPQ_GLOBAL *g);
void opq_translatetype(
	ADF_CB *adfcb,
	char *es_type,
	char *es_nulls,
	OPQ_ALIST *attrp);
bool r_rel_list_from_rel_rel(
	OPQ_GLOBAL *g,
	OPQ_RLIST *rellst[],
	OPQ_RLIST *ex_rellst[],
	bool statdump);
void opt_usage(void);

/*
**  Global variables
*/

static OPQ_ERRCB	opq_errcb;
						/* error control block */
static	i4		charnunique[DB_MAX_HIST_LENGTH];
                                              /* Array of number of unique
					      ** characters per character
					      ** position for character
					      ** attributes
					      */
static	f4		chardensity[DB_MAX_HIST_LENGTH];
                                              /* Array of number of character
					      ** set densities per character
					      ** position for character
					      ** attributes
					      */

static OPQ_IO_VALUE	opq_convbuf;
						/* conversion buffer for
						** prelem routine.
						*/

GLOBALREF OPQ_GLOBAL	    opq_global;		/* global data struct */
GLOBALREF ER_ARGUMENT	    er_args[OPQ_MERROR];/* array of arg structs for
						** ERslookup.
						*/ 
GLOBALREF i4		    opq_retry;		/* serialization error
						** retries count.
						*/
GLOBALREF i4		    opq_excstat; 	/* additional info about 
						** exception processed.
						*/
GLOBALREF IISQLDA	    _sqlda;		/* SQLDA area for all
						** dynamically built queries.
						*/
GLOBALREF OPQ_DBUF	    opq_date;		/* buffer for date 
						*/
GLOBALREF OPQ_RLIST         **rellist;
GLOBALREF bool              sample;
GLOBALREF bool              nosetstats;
GLOBALREF OPQ_DBUF          opq_sbuf;         /* buffer for dynamic SQL queries
                                              ** (statements)
                                              */

# ifdef xDEBUG
GLOBALREF i4		    opq_xdebug;		/* debugging only */
# endif /* xDEBUG */
GLOBALREF       VOID (*opq_exit)(VOID);
GLOBALREF       VOID (*usage)(VOID);


/*{
** Name: opq_error() -  format an error message and translate 
**			to proper DU_STATUS for return.
**
** Description:
**        This routine formats an error message using ERslookup().  
**
** Inputs:
**	dbstatus	    status
**	errnum		    error number.
**	argcount	    argument count
**	p1 .. p20	    10  or less argument len-value pairs
**
** Outputs:
**      opq_errcb
**	    .errbuf			Buffer containing null-terminated
**					formatted error	message.
**	Returns:
**	    None.
**
** Side Effects:
**	    An informational or warning message may be printed out.
**
** History:
**      25-nov-86 (seputis)
**          Initial creation.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	03-apr-90 (stec)
**	    Added casts for parms to ERlookup(), removed code stripping
**	    off error id, but the part following the first 8 chars will
**	    be stripped.
**	21-jun-90 (stec)
**	    Modified code so that correct laguage code is passed to ERlookup()
**	    and display more info in case of ER failure.
**	27-jun-90 (stec)
**	    Added a NL char at the beginning of hardcoded error message.
**	08-nov-1990 (stec)
**          Change an argument to ERlookup() to get rid of a compiler warning.
**	06-apr-92 (rganski)
**	    Merged a change from seg:
**	    Can't use "errno" -- name reserved to C runtime library.  Renamed
**	    to "errnum".
**	24-oct-92 (andre)
**	    instead of ERlookup() we will be calling ERslookup()
**	09-mar-93 (walt)
**	    Removed declaration of STindex() from opq_error.  It's declared
**	    now in <st.h>, and the redeclaration here causes compile errors.
**	23-jul-93 (rganski)
**	    Changed variable lang to a long, to match ERlangcode param.
**      20-Aug-2002 (gilta01) Bug 106456 INGSRV 1615
**          Changed variable lang to an i4, to match ERlangcode param.
**      29-apr-02 (wanfr01)
**          Bug 106456, INGSRV 1615
**          Changed function to correctly use variable length arguments
*/
VOID
opq_error(DB_STATUS dbstatus, i4 errnum, i4 argcount, ...)
{
    i4		tmp_status;	    /* ERslookup status */
    CL_SYS_ERR	sys_err;
    i4		msglen;
    i4		buf_len;
    i4		er_flag;
    i4	lang;
    va_list 	ap;
    int 	i;

# ifdef E_OP0921_ERROR
    /* Check if the call was done correctly */
    if ((argcount % 2) == 1 || argcount > OPQ_MERROR)
    {
	errnum = E_OP0921_ERROR;
        dbstatus = E_DB_SEVERE;
	argcount = 0;
    }
# endif /* E_OP0921_ERROR */
    buf_len	    = sizeof(opq_errcb.errbuf)-1;   /* save a position for 
						    ** the NULL */
    opq_errcb.status = dbstatus;
    opq_errcb.error.err_code = errnum;

    /* Initialize the argument array */
    va_start( ap, argcount );
    for( i = 0; i < (argcount/2) ; i++ )
     {
       er_args[i].er_size = (int) va_arg( ap, int );
       er_args[i].er_value = (PTR) va_arg( ap, PTR );
     }
     va_end( ap );

    /* Error code, no timestamp. */
    er_flag = 0;

    /* Obtain the language code */
    tmp_status = ERlangcode((char *)NULL, &lang);

    if (tmp_status != OK)
    {
	/* We do not want to call ERslookup to output an error msg
	** since we just failed in obtaining a parm value for
	** this call; we will use a default lang code value of -1
	** instead - this is not advertised in the CL spec is clearly
	** used all over ER code.
	*/
	msglen = 0;

	SIprintf(
	    "Error occurred when attempting to display message for %d (0x%x) error code.\n",
	    errnum, errnum);
	SIprintf(
	    "Status %d (0x%x) was returned when trying to identify default language.\n",
	    tmp_status, tmp_status);

	opq_errcb.status =
	dbstatus = E_DB_SEVERE;
	opq_errcb.error.err_code = E_OP0922_BAD_ERLOOKUP;
    }
    else
    {
	/* Format the database utility error message */

	tmp_status  = ERslookup((i4)errnum, (CL_ERR_DESC *)NULL,
		(i4)er_flag, (char *)NULL, (char *)&opq_errcb.errbuf[0],
		(i4)buf_len, (i4)lang, (i4 *)&msglen, (CL_ERR_DESC *)&sys_err,
		(i4)10, (ER_ARGUMENT *)&er_args[0]);

	if (tmp_status != OK)
	{
	    ER_ARGUMENT		loc_param[2];

	    loc_param[0].er_size   = sizeof(errnum);
	    loc_param[0].er_value  = (PTR)&errnum;
	    loc_param[1].er_size   = sizeof(errnum);
	    loc_param[1].er_value  = (PTR)&errnum;

	    SIprintf(
		"Error occurred when attempting to display message for %d (0x%x) error code.\n",
		errnum, errnum);
	    SIprintf(
		"Status %d (0x%x) was returned when trying to look up an error message.\n",
		tmp_status, tmp_status);

	    tmp_status = ERslookup((i4)E_OP0922_BAD_ERLOOKUP,
		(CL_ERR_DESC *)NULL, (i4)er_flag, (char *)NULL,
		(char *)&opq_errcb.errbuf[0], (i4)buf_len, (i4)lang,
		(i4 *)&msglen, (CL_ERR_DESC *)&sys_err,
		(i4)2, (ER_ARGUMENT *)&loc_param[0]);

	    if (tmp_status != OK)
	    {
		msglen = 0;
		SIprintf("\nCannot look up error messages (status %d (0x%x))\n",
		    tmp_status, tmp_status);
	    }
	    opq_errcb.status =
	    dbstatus = E_DB_SEVERE;
	    opq_errcb.error.err_code = E_OP0922_BAD_ERLOOKUP;
	}
    }

    opq_errcb.errbuf[msglen] = EOS;

    if (er_flag == 0 && msglen != 0)
    {
	/*
	** We have a message of the following format:
	**  'E_OPxxxx[A] text' and we want to print out
	**  'E_OPxxxx text', therefore the [A] part has 
	** to be stripped off (we want only first 8 chars
	** from the msg code).
	*/
	char *p = (char *)&opq_errcb.errbuf[0], *s;

	/* Remove white space in front, if any. */
	while (CMwhite(p))
	    CMnext(p);

	/* p points now to first non-white char. */
	s = p;

	/*
	** Find first non-printable char, it is followed
	** by the text to be displayed.
	*/
	while (CMprint(p))
	    CMnext(p);

	/* p is now pointing at the end of the message code (i.e., first
	** char after); check if the msg code is longer than 8 chars.
	** If not, there is nothing to do, else move text.
	*/
	if (p - s > 8)
	{
	    i4	    len;

	    /* make s point at the spot where the text is to be moved. */
	    s += 8;

	    len = p - s;

	    MEcopy((PTR)p,
		((char *)&opq_errcb.errbuf[0] + msglen - p), (PTR)s);

	    msglen -= len;

	    opq_errcb.errbuf[msglen] = EOS;
	}
    }

    if (opq_global.opq_adfscb.adf_errcb.ad_errcode != E_AD0000_OK)
    {   /* attach ADF error message to utility message */
	i4	    adflen;	    /* length of ADF error message */
	opq_errcb.errbuf[msglen++]    = '\n';
	buf_len	    -= msglen;
	adflen = opq_global.opq_adfscb.adf_errcb.ad_emsglen;
	if (adflen > buf_len)
	    adflen = buf_len;
        STncpy( (char *)&opq_errcb.errbuf[msglen],
		(char *)opq_global.opq_adfscb.adf_errcb.ad_errmsgp, adflen);
	opq_errcb.errbuf[msglen + adflen ] = '\0';
    }

    SIprintf("%s\n", (char *)&opq_errcb.errbuf[0]);
    (VOID)SIflush(stdout);

}

/*{
** Name: opq_sqlerror  - SQL error reporting routine
**
** Description:
**      This routine processes contents of SQLCA when an error
**	or warning has occurred.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
**	Returns:
**	    None.
**	Exceptions:
**	    None.
**
** Side Effects:
**	    Reads contents of sqlca.
**
** History:
**	12-02-88 (stec)
**	    Created.
**	12-dec-89 (stec)
**	    Change code to interpret sqlca correctly.
*/
VOID
opq_sqlerror(VOID)
{
    if (sqlca.sqlcode < 0)
    {
	/*
	** It is an error. Display dbms specific error message.
	** If the DBMS sent several error messages back, currently
	** only the first one will be displayed due to limitations
	** in the inquire_ingres mechanism.
	*/

	exec sql begin declare section;
	    char  error_msg[ER_MAX_LEN + 1];
	exec sql end declare section;

	exec sql inquire_ingres(:error_msg = errortext);

	SIprintf("%s\n", error_msg);
	(VOID)SIflush(stdout);

	/* If not a serialization error, discontinue execution */
	if (sqlca.sqlcode == -GE_SERIALIZATION
	    ||
	    sqlca.sqlcode == -4700
	    ||
	    sqlca.sqlcode == -4706
	    ||
	    sqlca.sqlcode == -4708
	   )
	{
	    if (opq_retry-- > 0)
	    {
		EXsignal((i4)EX_JNP_LJMP, (i4)0);
	    }
	    else
	    {
		/* serialization error - too many retries */
		opq_error((DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0971_TOO_MANY_RETRIES, (i4)0);
		(*opq_exit)();
	    }
	}
	else
	{
	    (*opq_exit)();
	}
    }
    else if (sqlca.sqlwarn.sqlwarn0 == 'W')
    {
	/* 
	** Some of these should be set only when sqlcode
	** is set. For the sake of completeness all cases
	** will be handled here. More than one field can
	** be set at one time.
	*/

	if (sqlca.sqlwarn.sqlwarn1 == 'W')
	{
	    /* truncation on char string assignment */
	    opq_error((DB_STATUS)E_DB_WARN,
		(i4)W_OP0961_SQLWARN1, (i4)0);
	}

	if (sqlca.sqlwarn.sqlwarn2 == 'W')
	{
	    /* Set when NULL values are eliminated from
	    ** aggregates.
	    */
	    opq_error((DB_STATUS)E_DB_WARN,
		(i4)W_OP0962_SQLWARN2, (i4)0);
	}

	if (sqlca.sqlwarn.sqlwarn3 == 'W')
	{
	    /* mismatching number of result columns
	    ** and result host variables in a FETCH
	    ** or SELECT.
	    */
	    opq_error((DB_STATUS)E_DB_WARN,
		(i4)W_OP0963_SQLWARN3, (i4)0);
	}

	if (sqlca.sqlwarn.sqlwarn4 == 'W')
	{
	    /* Set when preparing an update or
	    ** delete statement with a WHERE clause.
	    */
	    opq_error((DB_STATUS)E_DB_WARN,
		(i4)W_OP0964_SQLWARN4, (i4)0);
	}

	if (sqlca.sqlwarn.sqlwarn5 == 'W')
	{
	    /* Currently unused */
	    opq_error((DB_STATUS)E_DB_WARN,
		(i4)W_OP0965_SQLWARN5, (i4)0);
	}

	if (sqlca.sqlwarn.sqlwarn6 == 'W')
	{
	    /* Set when abnormal termination of
	    ** a transaction occurred.
	    */
	    opq_error((DB_STATUS)E_DB_WARN,
		(i4)W_OP0966_SQLWARN6, (i4)0);
	}

	if (sqlca.sqlwarn.sqlwarn7 == 'W')
	{
	    /* Currently unused */
	    opq_error((DB_STATUS)E_DB_WARN,
		(i4)W_OP0967_SQLWARN7, (i4)0);
	}

	/* No error, just a warning - continue */
	return;
    }
}

/*
 * {
** Name: opq_adferror() -  ADF error reporting routine
**
** Description:
**        This routine handles any ADF error returned to optimizedb or 
**        statdump. This routine determines the error code, and then, by
**	  examining the error number, determines the number of arguments
**	  and the order in which to pass them to opq_error().
**	  It calls opq_error() with the error code and the rest of 
**        the arguments and then aborts via opq_exit().
**
**	  The caller of this routine must put the correct arguments, in the 
**	  correct order, in p1 through p4. See eropf.msg for the correct 
**	  arguments and ordering. Do not pass the error code, which is 
**	  determined here. If there are less than 4 arguments, the caller 
**	  must pad the arguments to with 0s cast to i4s and PTRs.
**	  If you are passing an error number which does not appear in the
**	  switch statement below, and it does not follow the default behavior 
**	  (pass error code followed by all 4 arguments), then add it to the
**	  switch statement in the appropriate spot.
**	  
** Inputs:
**	adfcb			ptr to ADF control block
**	  adf_errcb		ADF error control block
**	     ad_usererr 	user error code: set by adf function
**	     ad_errcode 	ADF error code: set by adf function
**	errnum			ADF error number
**	p1 .. p4		len-argument pairs for passing to opq_error.
**
** Outputs:
**	None.
**	
**	Returns:
**	    None.
**
** Side Effects:
**	    An informational or warning message will be printed out.
**	    Exits the program.
**
** History:
**      09-nov-92 (rganski)
**          Initial creation.
*/
VOID
opq_adferror(
ADF_CB		*adfcb,
i4		errnum,
i4		p1,
PTR		p2,
i4		p3,
PTR		p4)
{
    i4 adf_errcode;
    
    if (adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	adf_errcode = adfcb->adf_errcb.ad_usererr;
    else
	adf_errcode = adfcb->adf_errcb.ad_errcode;
    
    switch(errnum)
    {
    case E_OP0936_ADI_TYID:
    case E_OP093A_ADC_LENCHK:
	/* pass error code followed by 2 arguments */
	opq_error((DB_STATUS)E_DB_ERROR, errnum, (i4)4,
		  (i4)sizeof(adf_errcode), (PTR)&adf_errcode, p1, p2);
	break;
    case E_OP091D_HISTOGRAM:
    case E_OP092B_COMPARE:
	/* pass 2 arguments followed by error code */	
	opq_error((DB_STATUS)E_DB_ERROR, errnum, (i4)4, p1, p2,
		  (i4)sizeof(adf_errcode), (PTR)&adf_errcode);
	break;
    default:
	/* pass error code followed by 4 arguments.
	** Includes E_OP0929_ADC_HDEC, E_OP0980_ADI_PM_ENCODE,
	** E_OP0981_ADI_FICOERCE, E_OP0982_ADF_FUNC, E_OP0983_ADC_COMPARE.
	*/
	opq_error((DB_STATUS)E_DB_ERROR, errnum, (i4)6,
		  (i4)sizeof(adf_errcode), (PTR)&adf_errcode, p1, p2, p3,
		  p4);
	break;
    }

    (*opq_exit)();
} 

/*
 * {
** Name: opq_exhandler() - Exception handler for statdump, optimizedb.
**
** Description:
**      This is the exception handler for OPF utilities. If the exception 
**	is generated by the user (interrupt), a message is displayed and the 
**	program is exits after doing some cleanup. If the exception is generated
**	by the program to indicate serialization error, no action is taken and
**	EXDECLARE value is returned,transaction will be automatically restarted.
**	In case of any other exception, appropriate information will be logged
**	and the program will exit.
**
** Inputs:
**      ex_args                         EX_ARGS struct.
**	    .ex_argnum			Tag for the exception that occurred.
**
** Outputs:
**	none.
**
**	Returns:
**	    The signal is resignaled if it is not recognized.
**	Exceptions:
**	    none
**
** Side Effects:
**	Exception handler sets opq_excstat to indicate the type of exception
**	that occurred.
**
** History:
**      25-nov-86 (seputis)
**          Initial creation.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	19-dec-89 (stec)
**	    Removed calls to IIbreak and opq_exit. No database calls
**	    can be issued safely from here because tha stack needs to
**	    be unwound. Moved it from opqoptd.sc and opqstatd.sc here
**	    since the code is identical and can be shared.
**	10-jan-91 (stec)
**	    Changed length of buffer for EXsys_report() to EX_SYS_MAX_REP.
**	    Changed return type to be STATUS as per CL spec, changed definition 
**	    of status to be returned.
*/
STATUS
opq_exhandler(
EX_ARGS		*ex_args)
{
    STATUS  stat; 

    switch (EXmatch(ex_args->exarg_num, (i4)2, (EX)EXINTR, (EX)EX_JNP_LJMP))
    {
	case 1:
	    {
		opq_excstat = OPQ_INTR;
		opq_error( (DB_STATUS)E_DB_OK,
		    (i4)I_OP0900_INTERRUPT, (i4)0);
		stat = EXDECLARE;
		break;
	    }
	case 2:
	    {
		opq_excstat = OPQ_SERIAL;
		stat = EXDECLARE;
		break;
	    }
	default:
	    {
		char buf[EX_MAX_SYS_REP];

		opq_excstat = OPQ_DEFAULT;
		
		if (EXsys_report(ex_args, buf) == (bool)TRUE)
		{
		    /* A System/Hardware exception occurred */
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP095B_SYSHWEXC, (i4)2,
			(i4)0, (PTR)buf);
		}
		else
		{
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0901_UNKNOWN_EXCEPTION, (i4)2,
			(i4)sizeof(ex_args->exarg_num),
			    (PTR)&ex_args->exarg_num);
		}

		stat = EXDECLARE;
		break;
	    }
    }

    return(stat);
}

/*{
** Name: adfinit	- initialize ADF
**
** Description:
**      Initializes the ADF for use.
**
** Inputs:
**	g				ptr to global data struct
**	    .opq_utilid                 name of utility calling this routine
**	    .opq_adfcb			ptr to ADF ctrl block.
**	    .opq_prec			precision for floating point nos.
**
** Outputs:
**	g				ptr to global data struct
**	    .opq_adfscbp                initialized ADF ctrl block.
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Returns:
**	None.
**
** Side Effects:
**	    none
**
** History:
**      25-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	03-apr-90 (stec)
**	    Force output format for floating point numbers to 'n'.
**	09-nov-92 (rganski)
**	    Added initialization of timezone control block, for date
**	    conversions. 
**      10-apr-95 (nick)(cross-int angusm)
**      Altered difference between floating point width and precision
**      to eight as this prevents large negative numbers which were
**      overflowing on IEEE machines to print ok.  #56481
*/
VOID
adfinit(
OPQ_GLOBAL  *g)
{
    ADF_CB     *feadfcb;	/* get front end control
                                ** block if possible
				*/
    if (feadfcb = FEadfcb())
    {
	STRUCT_ASSIGN_MACRO(*feadfcb, g->opq_adfscb); /* copy fields in the
					** front end ADF control block */
        /* allocate an error message buffer if it is not available */
	if (!g->opq_adfcb->adf_errcb.ad_errmsgp)
	{
	    g->opq_adfcb->adf_errcb.ad_ebuflen = sizeof(opq_errcb.adfbuf);
	    g->opq_adfcb->adf_errcb.ad_errmsgp = (char *)&opq_errcb.adfbuf[0];
	}

	if (g->opq_prec)
	{
	    /* To ensure proper handling of floating point numbers */
	    g->opq_adfcb->adf_outarg.ad_f4prec =  g->opq_prec;
	    g->opq_adfcb->adf_outarg.ad_f8prec =  g->opq_prec;
	    g->opq_adfcb->adf_outarg.ad_f4width = g->opq_prec + 8;
	    g->opq_adfcb->adf_outarg.ad_f8width = g->opq_prec + 8;
	}

	g->opq_adfcb->adf_outarg.ad_f4style = 'n';
	g->opq_adfcb->adf_outarg.ad_f8style = 'n';

	/* Initialize timezone control block for date conversions */
	if (TMtz_init((PTR)&g->opq_adfcb->adf_tzcb) == OK)
	    return;
    }
# ifdef	xDEBUG
    if (opq_xdebug)
	SIprintf("Could not get an ADF control block from front ends\n");
# endif /* xDEBUG */
    opq_error( (DB_STATUS)E_DB_ERROR,
	(i4)E_OP0902_ADFSTARTUP, (i4)2,
	(i4)0, (PTR)g->opq_utilid);
    /* %s: cannot initialize ADF */
}

/*{
** Name: adfreset	- reinitialize ADF control block
**
** Description:
**      Reinitializes the ADF control block for floating point numbers.
**
** Inputs:
**	g				ptr to global data struct
**	    .opq_adfcb	    		ptr to ADF ctrl block.
**	    .opq_prec			precision for floating point nos.
**
** Outputs:
**	g				ptr to global data struct
**        opq_adfcb                   ptr to ADF ctrl block.
**	    adf_outarg			if prec > 0 following fields modified
**		ad_f4prec		=  prec
**		ad_f8prec		=  prec 
**		ad_f4width		= prec + 8
**		ad_f8width		= prec + 8
**		ad_f4style		= 'n'
**		ad_f8style		= 'n'
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-mar-89 (stec)
**          initial creation
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values.
**	03-apr-90 (stec)
**	    Changed output format to 'n' from 'e'. 
**      10-apr-95 (nick)(cross-int angusm)
**      Altered difference between floating point width and precision
**      to eight as this prevents large negative numbers which were
**      overflowing on IEEE machines to print ok.  #56481
*/
VOID
adfreset(
OPQ_GLOBAL  *g)
{
    if (g->opq_prec > 0)
    {
	/* To ensure proper handling of floating point numbers */
	g->opq_adfcb->adf_outarg.ad_f4prec =  g->opq_prec;
	g->opq_adfcb->adf_outarg.ad_f8prec =  g->opq_prec;
	g->opq_adfcb->adf_outarg.ad_f4width = g->opq_prec + 8;
	g->opq_adfcb->adf_outarg.ad_f8width = g->opq_prec + 8;
	g->opq_adfcb->adf_outarg.ad_f4style = 'n';
	g->opq_adfcb->adf_outarg.ad_f8style = 'n';
    }

    return;
}

/*{
** Name: nostackspace	- error exit routine when no memory exists
**
** Description:
**      Error exit routine when no memory exists.
**
** Inputs:
**      utilid          name of utility calling this routine
**	status		error status
**	size		size of dyn. memory to be allocated
**
** Outputs:
**	None.
**
**	Returns:
**	    Does not return
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	10-apr-90 (stec)
**	    Added status and size parameters to make the message
**	    more descriptive.
*/
VOID
nostackspace(
char		*utilid,
STATUS		status,
u_i4	size)
{
    i4	stat = status;

    opq_error( (DB_STATUS)E_DB_ERROR,
	(i4)E_OP091A_STACK, (i4)6,
	(i4)0, (PTR)utilid,
	(i4)sizeof(stat), (PTR)&stat,
	(i4)sizeof(size), (PTR)&size);
    (*opq_exit)();
}

/*{
** Name: getspace - allocate space if necessary
**
** Description:
**      This routine will check if space has already been allocated and
**      allocate if not. If space has already been allocated, checks size of
**	allocated space: if >= requested size, returns same spaceptr; otherwise
**	frees old memory and allocates requested amount.
**	If new space is allocated, it is cleared to '\0'.
**
**	NOTE: spaceptr must be initialized to NULL when requesting brand new
**	      memory!
**
** Inputs:
**      utilid                          name of utility calling this routine
**      spaceptr                        NULL if memory is to be allocated
**      size                            size in bytes of memory required
**
** Outputs:
**      spaceptr                        ptr to allocated memory
**
**	Returns:
**	    PTR to allocated memory of at least 'size' length,
**	Exceptions:
**	    none.
**
** Side Effects:
**	    Frees, allocates and clears memory if necessary.
**
** History:
**      16-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	18-jan-93 (rganski)
**	    Changed so that if *spaceptr is not null, checks size of allocated
**	    memory: if >= requested size, returns same spaceptr; otherwise
**	    frees old memory and allocates requested amount.
**	21-jun-93 (rganski)
**	    Check status returned by MEfree.
*/
PTR
getspace(
char		*utilid,
PTR     	*spaceptr,
SIZE_TYPE 	size)
{
    STATUS  status;

    if (*spaceptr != NULL)
    {
	SIZE_TYPE old_size;
	
	status = MEsize(*spaceptr, &old_size);
	if (status != OK)
	    /* Abort with memory error */
	    nostackspace(utilid, status, size);	
	if ( old_size >= size )
	    /* Buffer is large enough already */
	    return(*spaceptr);
	else
	{
	    /* Need to allocate a larger buffer */
	    status = MEfree(*spaceptr);
	    if (status != OK)
		/* Abort with memory error */
		nostackspace(utilid, status, size);	
	}
    }
    
    /* Allocate new buffer */

    *spaceptr = MEreqmem((u_i4)OPQ_MTAG, size, (bool)TRUE, &status);
    if (status != (STATUS)OK)
	/* Abort with memory error */
	nostackspace(utilid, status, size);	

    return (*spaceptr);
}

/*{
** Name: fileinput	- get input files
**
** Description:
**      This routine will make input from the command line, transparent
**      from input from a file name.  This will be done by reading
**      the input file and building an argv,argc parameter from the
**      file specification.
**
**
** Inputs:
**      utilid                          ptr to name of utility calling this
**                                      routine
**      argv                            ptr to command line parameters
**                                      which may point to a file name
**                                      containing input specifications
**      argc                            ptr to number of command line parameters
**
** Outputs:
**      argv                            ptr to command line parameters
**                                      which may have been created from
**                                      an input file specification
**      argc                            ptr to number of command line parameters
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-nov-86 (seputis)
**          initial creation.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	07-feb-90 (stec)
**	    Added SIclose to close the file.
**	10-jan-91 (stec)
**	    Changed length of ERreport() buffer to ER_MAX_LEN.
**	20-jun-91 (rog)
**	    When reading arguments from a file, make any white-space
**	    be an argument separator, not just newlines.  Otherwise,
**	    "-o filename" will not work.  (Bug 38065)
**	20-sep-93 (rganski)
**	    Delimited ID support. Make -i and -o special cases which have white
**	    space as argument separators; otherwise newline is argument
**	    separator. 
**	3-june-99 (inkdo01)
**	    Allow arbitrarily many file contained arguments - and allocate
**	    arg space more accurately (instead of 4000 bytes each!).
**  20-Jan-05 (nansa02)
**      Added STtrmwhite to the input parameters from a file. (Bug 113775)
**
*/
VOID
fileinput(
char               *utilid,
char               ***argv,
i4                 *argc)
{
    FILE	*inf;
    STATUS	stat;

    if (*argc != 3 || (STcompare("-zf", (*argv)[1])))
	return;			/* get input from command line if a file
                                ** specification is not given i.e.
                                ** -zf <filename> will redirect input
                                ** from the file name */

    /* file specification found - create argv and argc to simulate a 
    ** "long" command input line */
    {
	/* open the file containing the specifications */
	LOCATION	inf_loc;

	inf = NULL;

	if (
	    ((stat =LOfroms(PATH & FILENAME, (*argv)[2], &inf_loc)) != OK)
	    ||
	    ((stat = SIopen(&inf_loc, "r", &inf)) != OK)
	    || 
	    (inf == NULL)
	   )
	{   /* report error dereferencing or opening file */
	    char	buf[ER_MAX_LEN];

	    if ((ERreport(stat, buf)) != OK)
	    {
		buf[0] = '\0';
	    }

	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP0904_OPENFILE, (i4)8,
                (i4)0, (PTR)utilid,
		(i4)0, (PTR)((*argv)[2]), 
                (i4)sizeof(stat), (PTR)&stat,
		(i4)0, (PTR)buf);
	    /* %s: cannot open input file:%s, os status:%d\n\n
	    ** (*argv)[2]), stat
            */
	    (*usage)();
	    (*opq_exit)();
	}

	opq_global.opq_env.ifid = inf;	    
    }

    {
	char		buf[MAXBUFLEN];	    /* buffer used to read lines from 
					    ** the command file */
	char		**targv;            /* ptr to current parameter to be
                                            ** copied from temp buffer */
	i4		targc;              /* number of parameters which have
                                            ** been copied */
	SIZE_TYPE	len;		    /* size of dyn.mem. to be allocated.
					    */
	i4		argcount = MAXARGLEN;

	/* get room for maxarglen args */
	len = MAXARGLEN * sizeof(char *);
	targv = (char **)NULL;
	targv = (char **)getspace((char *)utilid, (PTR *)&targv, len);

	targc		= 1;
	buf[MAXBUFLEN-1]= 0;

	while ((stat = SIgetrec((PTR)&buf[0], (i4)(MAXBUFLEN-1), inf))
		== OK)
	{
	    char *cp = &buf[0];

	    do
	    {

		if (targc >= argcount)
		{
		    i4	i;
		    char **targv1 = NULL;

		    /* Allocate another vector, twice the size of the prev. */
		    targv1 = (char **)getspace((char *)utilid, 
			(PTR *)&targv1, (i4)(argcount * 2 * sizeof(char *)));

		    /* Copy previous into current. */
		    for (i = 0; i < argcount; i++)
			targv1[i] = targv[i];
		    targv = targv1;
		    argcount += argcount;
		}

		{
		    char *tp = (char *) NULL;
		    i4	arglen = STlength(cp);

		    tp = (char *)getspace((char *)utilid, (PTR *)&tp, arglen);

		    targv[targc++] = tp;

		    /* Look for "-i" or "-o", which can have 2 arguments on the
		    ** same line */
		    if (*cp == '-' && (*(cp+1) == 'i' || *(cp+1) == 'o'))
		    {
			while (*cp && !CMwhite(cp))
			    CMcpyinc(cp, tp);
			*tp = EOS;

			while (*cp && CMwhite(cp))
			    CMnext(cp);
		    }
		    else
		    {
		    i4 len = STtrmwhite(cp);
			STcopy(cp, tp);
			cp = &cp[len];
		    }
		}
	    }
	    while (*cp);
	}

	if (stat != ENDFILE)
	{
	    char	buf[ER_MAX_LEN];

	    if ((ERreport(stat, buf)) != OK)
	    {
		buf[0] = '\0';
	    }

	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0919_READ_ERR, (i4)4,
		(i4)sizeof(stat), (PTR)&stat,
		(i4)0, (PTR)buf);
	    (*opq_exit)();
	}

	*argv = targv;
	*argc = targc;
    }

    /* Close input file */
    stat = SIclose(inf);
    opq_global.opq_env.ifid = (FILE *)NULL;
    if (stat != OK)
    {   /* report error closing file */
	char	buf[ER_MAX_LEN];

	if ((ERreport(stat, buf)) != OK)
	{
	    buf[0] = '\0';
	}

	opq_error( (DB_STATUS)E_DB_ERROR,
	    (i4)E_OP0938_CLOSE_ERROR, (i4)8,
	    (i4)0, (PTR)utilid,
	    (i4)0, (PTR)((*argv)[2]), 
	    (i4)sizeof(stat), (PTR)&stat,
	    (i4)0, (PTR)buf);
	(*opq_exit)();
    }		    

    return;
}

/*{
** Name: isrelation	- check for relation name parameter
**
** Description:
**      Syntactic check for relation name parameter which must be of the form:
**      
**                    -r<relation name> or -xr<relation name>
**
** Inputs:
**      relptr            ptr to possible relation name
**	excl		  ptr to bool flag
**
** Outputs:
**	excl		  TRUE - -xr<relation name>
**			  FALSE - -r<relation name>
**
**	Returns:
**	    TRUE if this is a relation name
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	29-dec-89 (stec)
**	    Make the code case insensitive.
**	16-july-01 (inkdo01)
**	    Add support for -xr command line option.
*/
bool
isrelation(
char	*relptr,
bool	*excl)
{
    char    tmp[2+1];

    *excl = FALSE;
    if (relptr)
    {
	/* Create a copy of the first 2 chars */
	tmp[0] = *relptr;
	tmp[1] = *(relptr + 1);
	if (tmp[1] == 'x')
	{
	    /* This is a "-xr" option. */
	    *excl = TRUE;
	    tmp[1] = *(relptr + 2);
	}
	tmp[2] = '\0';

	CVlower(tmp);	
    }

    return (relptr
	    &&
	    (!MEcmp((PTR)tmp, (PTR)"-r", 2) ||
	    !MEcmp((PTR)tmp, (PTR)"-xr", 3))
	    );
}

/*{
** Name: isattribute	- check for attribute name parameter
**
** Description:
**      Syntactic check for attribute name parameter which must be of the form:
**      
**                    -a<attribute name>
**
** Inputs:
**      attptr               ptr to possible attribute name
**
** Outputs:
**	None
**
**	Returns:
**	    TRUE if this is a attribute name
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	29-dec-89 (stec)
**	    Make the code case insensitive.
*/
bool
isattribute(
char    *attptr)
{
    char    tmp[2+1];

    if (attptr)
    {
	/* Create a copy of the first 2 chars */
	tmp[0] = *attptr;
	tmp[1] = *(attptr + 1);
	tmp[2] = '\0';

	CVlower(tmp);	
    }

    return (attptr
	    &&
	    !MEcmp((PTR)tmp, (PTR)"-a", 2)
	    );
}

/*{
** Name: good_type	- consistency check on type
**
** Description:
**      Consistency check on the data type of the attribute found in the
**      attribute tuple
**
** Inputs:
**      typeid		    ADF type id
**	opq_adfscbp	    ptr to ADF ctrl block             
**
** Outputs:
**	None.
**
**	Returns:
**	    TRUE - if typeid is a valid ADF code
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
*/
static bool
good_type(
DB_DT_ID    typeid,
ADF_CB      *opq_adfscbp)
{
    ADI_DT_NAME	    adi_name;
    DB_STATUS	    status;

    status = adi_tyname(opq_adfscbp, typeid, &adi_name);
    return (status == E_DB_OK);
}

/*{
** Name: align	- find "aligned" size
**
** Description:
**      Routine used to get a size which will be "aligned".  If all allocation
**      are done with this routine then all elements will be aligned.
**
** Inputs:
**      realsize              size of object to be aligned
**
** Outputs:
**	None
**
**	Returns:
**	    "aligned" size of object
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      1-dec-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
*/
OPS_DTLENGTH
align(
OPS_DTLENGTH    realsize)
{
    i4	    diff;

    if( diff = (realsize % sizeof(ALIGN_RESTRICT)))
	return( realsize + (sizeof(ALIGN_RESTRICT)-diff));
 
    return(realsize);
}

/*{
** Name: setattinfo	- initialize the attribute descriptor
**
** Description:
**      Routine will validate the attribute descriptor
**
** Inputs:
**	g				ptr to global data struct
**        opq_utilid                    ptr to name of utility calling routine
**	  opq_opq_adfcb			ptr to ADF ctrl block
**      attlist                         ptr to array of ptrs to attribute
**                                      descriptors
**      index                           ptr to index in attlist[] of the
**					descriptor to validate
**	bound_length			histogram boundary value length, as
**					supplied by user in -length flag; if
**					not provided, 0.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	10-jul-89 (stec)
**	    Changed code to reflect new interface to adc_lenchk.
**	    Changed the way precision is computed for DECIMAL.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	18-jan-93 (rganski)
**	    Added bound_length parameter.
**	    Initialize ap->hist_dt.db_length to bound_length before call to
**	    adc_hg_dtln; if user supplied "-length" parameter, the boundary
**	    value length for character types will be same as this parameter or
**	    column size, whichever is smaller. Changed limit check for boundary
**	    value length to DB_MAX_HIST_LENGTH, which is 1950.
**	    Part of Character Histogram Enhancements project. See project spec
**	    for more details.
**	26-apr-93 (rganski)
**	    Character Histograms project: removed limitation of OPH_NH_LENGTH
**	    when bound_length parameter not provided.
**	31-jan-94 (rganski)
**	    Replaced formula for computing decimal type length with
**	    DB_PREC_TO_LEN_MACRO, as suggested by jrb comment.
*/
VOID
setattinfo(
OPQ_GLOBAL	*g,
OPQ_ALIST       **attlist,
i4              *index,
OPS_DTLENGTH	bound_length)
{
    OPQ_ALIST	*ap;

    ap = attlist[*index];

    if (!good_type(ap->attr_dt.db_datatype, g->opq_adfcb))
    {
	opq_error( (DB_STATUS)E_DB_ERROR,
	    (i4)E_OP0916_TYPE, (i4)6, 
            (i4)0, (PTR)g->opq_utilid,
	    (i4)sizeof(ap->attr_dt.db_datatype),
		(PTR)&ap->attr_dt.db_datatype, 
	    (i4)0, (PTR)&ap->attname);
	/* %s: bad datatype id =%d in iiattribute.attfrmt, column name %s
	*/
	attlist[*index] = NULL;		/* deallocate attribute since we
                                        ** cannot process it */
	return;
    }

    /* compute precision */
    {
	DB_DT_ID	dtype;

	dtype = ap->attr_dt.db_datatype;

	if (dtype < 0)
	    dtype = -dtype;

	/* In iicolumns standard catalog, the column_length field is the
	** precision for DECIMAL, but the user-specified length for all
	** other types.
	*/
	if (dtype == DB_DEC_TYPE)    
	{
	    ap->attr_dt.db_prec = DB_PS_ENCODE_MACRO(ap->attr_dt.db_length,
		ap->scale);
	    ap->attr_dt.db_length = DB_PREC_TO_LEN_MACRO(ap->attr_dt.db_length);
	}
	else
	{
	    ap->attr_dt.db_prec = 0;
	}
    }

    /* column length comes now from standard catalogs and it does not
    ** reflect nullability, for money and date datatypes length is zero,
    ** we need to find the actual length to allocate buffers properly.
    */
    {
	DB_STATUS	status; 
	DB_DATA_VALUE	rdv;

	status = adc_lenchk(g->opq_adfcb, TRUE, &ap->attr_dt, &rdv);

	if (DB_FAILURE_MACRO(status))
	{
	    i4 adf_errcode;

	    if (g->opq_adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		adf_errcode = g->opq_adfcb->adf_errcb.ad_usererr;
	    else
		adf_errcode = g->opq_adfcb->adf_errcb.ad_errcode;

	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP093A_ADC_LENCHK, (i4)4,
		(i4)sizeof(adf_errcode), (PTR)&adf_errcode,
		(i4)0, (PTR)&ap->attname);

	    attlist[*index] = NULL;	    /* deallocate attribute since we
					    ** cannot process it */
	    return;
	}

	STRUCT_ASSIGN_MACRO(rdv, ap->attr_dt);
    }

    {
	DB_STATUS	    hgstatus;

	/* call ADF to get the histogram datatypes */
	ap->hist_dt.db_length = bound_length;	/* either 0 or user-supplied
						** value. */
	hgstatus = adc_hg_dtln(g->opq_adfcb, &ap->attr_dt, &ap->hist_dt);

	if (DB_FAILURE_MACRO(hgstatus))
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP0917_HTYPE, (i4)12, 
		(i4)0, (PTR)g->opq_utilid,
		(i4)sizeof(ap->attr_dt.db_datatype), 
		    (PTR)&ap->attr_dt.db_datatype, 
		(i4)sizeof(ap->attr_dt.db_prec), 
		    (char *)&ap->attr_dt.db_prec, 
		(i4)sizeof(ap->attr_dt.db_length), 
		    (PTR)&ap->attr_dt.db_length, 
		(i4)0, (PTR)&ap->attname, 
		(i4)0, (PTR)g->opq_adfcb->adf_errcb.ad_errmsgp);
	    /* %s: histogram type=%d prec=%d len=%d in iiattribute,
            **    column name %s, error->%s
	    */
	    attlist[*index] = NULL;	    /* deallocate attribute since we
					    ** cannot process it */
	    return;
	}

	if (ap->hist_dt.db_length > DB_MAX_HIST_LENGTH)
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP0917_HTYPE, (i4)12, 
		(i4)0, (PTR)g->opq_utilid,
		(i4)sizeof(ap->attr_dt.db_datatype), 
		    (PTR)&ap->attr_dt.db_datatype, 
		(i4)sizeof(ap->attr_dt.db_prec), 
		    (char *)&ap->attr_dt.db_prec, 
		(i4)sizeof(ap->attr_dt.db_length), 
		    (PTR)&ap->attr_dt.db_length, 
		(i4)0, (PTR)&ap->attname, 
		(i4)0, (PTR)g->opq_adfcb->adf_errcb.ad_errmsgp);
	    /* %s: histogram type=%d prec=%d len=%d in iiattribute, 
	    **	column name %s, error->%s
	    */
	    attlist[*index] = NULL;	    /* deallocate attribute since we
					    ** cannot process it */
	    return;
	}
    }

    (*index)++;
}

/*{
** Name: badarglist	- check user argument list
**
** Description:
**      This routine will check the validity of the user argument list.
**      It will return the offset in the parameter list of the database name
**      (which should immediately preceed all relation and attribute names)
**
** Inputs:
**	g		    ptr to global data struct
**        opq_utilid	    name of utility calling this routine
**	  opq_adfcb	    ptr to ADF ctrl block
**	  opq_cats	    "consider catalogs" flag
**	  opq_prec	    precision for floating pt nos.
**      argc                number of user parameters
**      argv                ptrs to user parameters
**      dbindex             offset in argv array of the database
**                          name parameter
**      ifs                 ingres flag values are returned, with
**                          the last element reserved for the
**                          database name
**      dbnameptr           returns first available ptr in ifs
**                          to be used for the database name
**      verbose             TRUE if optimizedb -zv option selected
**      histoprint          TRUE if optimizedb -zh option selected
**      key_attsplus        TRUE if optimizedb -zk option selected
**      minmaxonly          TRUE if optimizedb -zx option selected
**      uniquecells         optimizedb -zu option value if supplied
**      regcells            optimizedb -zr option value if supplied
**      quiet               TRUE if statdump -zq option selected
**      deleteflag          TRUE if statdump -zdl option selected
**	userptr		    ptr to user name
**	ifilenm		    ptr to file name
**	ofilenm		    ptr to file name
**	pgupdate	    ptr to page/row update flag
**	samplepct	    ptr to sample percentage (of total rows)
**      supress_nostats     ptr to "supress messages for tables
**			    with no stats" flag.
**	comp_flag	    ptr to complete flag boolean.
**	bound_length	    ptr to optimizedb length flag value.
**	nosample	    ptr to "nosample" flag boolean.
**
** Outputs:
**	g		    ptr to global data struct
**	  opq_cats	    initialized "consider catalogs" flag
**	  opq_prec	    initialized precision for floating pt nos.
**	  opq_lvrepf	    initialized "leave rep factor" flag
**      dbindex             offset in argv array of the database
**                          name parameter
**                                      
**      ifs                 ingres flag values are returned, with
**                          the last element reserved for the
**                          database name
**      dbnameptr           returns first available ptr in ifs
**                          to be used for the database name
**      verbose             TRUE if optimizedb -zv option selected
**      histoprint          TRUE if optimizedb -zh option selected
**      key_attsplus        TRUE if optimizedb -zk option selected
**      minmaxonly          TRUE if optimizedb -zx option selected
**      uniquecells         optimizedb -zu option value if supplied
**      regcells            optimizedb -zr option value if supplied
**      quiet               TRUE if statdump -zq option selected
**      deleteflag          TRUE if statdump -zdl option selected
**	userptr		    ptr to user name
**	ifilenm		    ptr to input file name (optdb)
**	ofilenm		    ptr to output file name (optdb and statdump)
**	pgupdate	    ptr to page/row update flag
**	samplepct	    ptr to sample percentage (of total rows)
**      supress_nostats     TRUE if statdump -zqq option selected.
**	comp_flag	    TRUE if optimizedb zw option selected.
**	bound_length	    ptr to optimizedb length flag value. 
**			    0 if not supplied.
**	nosample	    TRUE if optimizedb -zns option selected
**
**	Returns:
**	    TRUE - if some error is detected in the definition of user
**                 parameters
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	24-jul-89 (stec)
**	    Changed badarglist to accept Ingres flags which start with a "+".
**	20-sep-89 (stec)
**	    Recognize -zqq flag (supress output messages for tables with
**	    no statistics).
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	22-jan-91 (stec)
**	    Implement the new "zw" flag.
**	20-mar-91 (stec)
**	    Initialized opq_prec to OPQ_INIPREC.
**	14-dec-92 (rganski)
**	    Added new "length" flag. Represented by new bound_length param.
**	31-jan-94 (rganski)
**	    Put parentheses around equals+1 cast to PTR: can't do arithmetic on
**	    PTR (want to do arithmetic on the char * before the cast).
**	29-apr-98 (inkdo01)
**	    Set opq_lvrepf flag, based on -z?
**	9-june-99 (inkdo01)
**	    Minor fiddle to support "-l" flag for locking DB. Also, support of
**	    both input and output filenames (for optdb).
**	27-oct-99 (hayke02)
**	    The test for valid -zs values has been changed so that 100 or
**	    100.0 will now not result in E_OP094E. This change fixes bug
**	    38890.
**	10-jul-00 (inkdo01)
**	    Support new option -zdn for new distinct value estimation
**	    technique.
**	30-oct-00 (inkdo01)
**	    Support -zcpk option requesting composite histogram on primary
**	    key structure.
**	26-apr-01 (inkdo01)
**	    Add -zcpk as statdump option, too.
**	28-may-01 (inkdo01)
**	    Add -zfq and -znt as performance improvement options.
**	22-mar-2005 (thaju02)
**	    Changes for support of "+w" SQL option.
**	15-july-05 (inkdo01)
**	    Added -zns for "nosample" flag to override new table 
**	    cardinality-based sampling default.
**	21-oct-05 (inkdo01)
**	    Added -zna for "noAFVs" flag to override new introduction
**	    of even more exact cells in inexact histograms.
**	22-dec-06 (hayke02)
**	    Add nosetstatistics boolean for new "-zy" optimizedb flag which
**	    switches off 'set statistics' if it has already been run. This
**	    change implements SIR 117405.
**	26-jul-10 (toumi01) BUG 124136
**	    Add opq_encstats flag to request statistics for encrypted
**	    columns. The default behavior is to skip them.
*/
bool
badarglist(
OPQ_GLOBAL	*g,
i4		argc,
char            **argv,
i4              *dbindex,
char            *ifs[],
char            ***dbnameptr,
bool	        *verbose,
bool	        *histoprint,
bool  	        *key_attsplus,
bool	        *minmaxonly,
i4         *uniquecells,
i4         *regcells,
bool            *quiet,
bool            *deleteflag,
char		**userptr,
char            **ifilenm,
char            **ofilenm,
bool		*pgupdate,
f8		*samplepct,
bool		*supress_nostats,
bool		*comp_flag,
bool		*nosetstatistics,
OPS_DTLENGTH    *bound_length,
char            **waitopt,
bool		*nosample)
{
    i4		parmno;         /* current argv parameter being analyzed */
    i4		ifcnt;		/* number of ingres flags defined */
    bool        statdump;       /* TRUE - if statdump called this routine */
    bool        optimizedb;     /* TRUE - if optimizedb called this routine */

    for (ifcnt = 0; ifcnt < MAXIFLAGS; ifcnt++)
	ifs[ifcnt] = NULL;

    if (argc <= 1)
	return (TRUE);

    ifcnt = 0;

    optimizedb = !STcompare(g->opq_utilid, "optimizedb");
    statdump = !STcompare(g->opq_utilid, "statdump");

    /* These parms may have not been initialized by the caller */
    *userptr	= 0;
    *ifilenm	= 0;
    *ofilenm	= 0;
    *pgupdate	= FALSE;
    g->opq_cats = FALSE;
    g->opq_prec = OPQ_INIPREC;
    g->opq_lvrepf = FALSE;
    g->opq_newrepfest = FALSE;
    g->opq_encstats = FALSE;
    g->opq_newquery = FALSE;
    g->opq_comppkey = FALSE;
    g->opq_nott = FALSE;
    *samplepct	= 0.0;
    *comp_flag	= OPH_DCOMPLETE;
    *nosetstatistics = FALSE;
    g->opq_noAFVs = FALSE;
    *nosample = FALSE;
    g->opq_hexout = FALSE;

    for ( parmno = 1;
	  argv[parmno] && (argv[parmno][0] == '-' || argv[parmno][0] == '+');
	  parmno++)
    {
	char	temp[OPQ_PRMLEN + 1];

	/* Create a copy of the command line arg for parsing */
	STncpy(temp, &argv[parmno][0], OPQ_PRMLEN);
	temp[ OPQ_PRMLEN ] = '\0';

	CVlower(temp);	

	if (optimizedb && !STcompare(temp, "-zv"))
	{
	    *verbose = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-zh"))
	{
	    *histoprint	= TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-zk"))
	{
	    *key_attsplus = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-zlr"))
	{
	    g->opq_lvrepf = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-zdn"))
	{
	    g->opq_newrepfest = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-ze"))
	{
	    g->opq_encstats = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-zfq"))
	{
	    g->opq_newquery = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-znt"))
	{
	    g->opq_nott = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-zna"))
	{
	    g->opq_noAFVs = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-zns"))
	{
	    *nosample = TRUE;
	}

# ifdef NOTUSED
	/* too complicated to explain in users manual */
	else if (optimizedb && !STcompare(temp, "-zp"))
	{
	    *keyatts = TRUE;
	}
# endif /* NOTUSED */

	else if (optimizedb && !STcompare(temp, "-zx"))
	{
	    *minmaxonly	= TRUE;
	}
	else if (optimizedb && !MEcmp((PTR)temp, (PTR)"-zu", 3))
	{
	    i4		tempcount;

	    if (CVal(&argv[parmno][3], &tempcount) 
		||
		tempcount < MINCELLS 
		|| 
		tempcount * 2 >= MAXCELLS
		)
	    {
		i4	    maxcells;
		i4     mincells;

		maxcells = MAXCELLS/2;
		mincells = 0;	/* fix bug #2765 */
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0907_UNIQUECELLS, (i4)8,
		    (i4)0, (PTR)g->opq_utilid,
		    (i4)sizeof(mincells), (PTR)&mincells, 
		    (i4)sizeof(maxcells), (PTR)&maxcells, 
		    (i4)sizeof(tempcount), (PTR)&tempcount);
		(*opq_exit)();
		/* optimizedb: bad uniquecells value 1 <= # <= %d :%d
		** MAXCELLS/2, uniquecells */
	    }
		*uniquecells = 2 * tempcount; /* two cells needed per value */
	}
	else if (optimizedb && !MEcmp((PTR)temp, (PTR)"-zr", 3))
	{
	    i4	    tempcells;

	    if (CVal(&argv[parmno][3], &tempcells) 
		|| 
		tempcells <= MINCELLS	/* fix bug #2765 */
		||
		tempcells >= MAXCELLS)
	    {
		i4	    maxcells;
		i4	    mincells;

		maxcells = MAXCELLS;
		mincells = MINCELLS;
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0908_HISTOCELLS, (i4)8, 
		    (i4)0, (PTR)g->opq_utilid,
		    (i4)sizeof(mincells), (PTR)&mincells, 
		    (i4)sizeof(maxcells), (PTR)&maxcells, 
		    (i4)sizeof(tempcells), (PTR)&tempcells);
		(*opq_exit)();
		/* optimizedb: bad histogram cell count need 1<=#<=%d,
		** got %d MAXCELLS, tempcells
		*/
	    }

	    *regcells = tempcells;
	}
	else if (optimizedb && !STcompare(temp, "-i"))
	{
	    /* Next arg must be input file name */
	    parmno++;

	    if (parmno > argc)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0931_NO_FILE, (i4)0);
		(*opq_exit)();
	    }

	    *ifilenm = &argv[parmno][0];
	}
	else if (optimizedb && !STcompare(temp, "-o"))
	{
	    /* Next arg must be output file name */
	    parmno++;

	    if (parmno > argc)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0931_NO_FILE, (i4)0);
		(*opq_exit)();
	    }

	    *ofilenm = &argv[parmno][0];
	}
	else if (optimizedb && !STcompare(temp, "-zp"))
	{
	    *pgupdate = TRUE;
	}
	else if (optimizedb && !MEcmp((PTR)temp, (PTR)"-zs", 3))
	{
	    f8	    val;
	    i4	    pos = 3;

	    if (temp[pos] == 's')
	    {
		pos++;
	    }

	    /* must be a number greater than 0 less than 100 */
	    if (CVaf(&argv[parmno][pos], g->opq_adfcb->adf_decimal.db_decimal,
		&val) || val <= 0 || val > 100)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP094E_INV_SAMPLE, (i4)0);
		(*opq_exit)();
	    }

	    *samplepct = val;
	}
	else if (optimizedb && !STcompare(temp, "-zw"))
	{
	    *comp_flag = TRUE;
	}
	else if (optimizedb && !STcompare(temp, "-zy"))
	{
	    *nosetstatistics = TRUE;
	}
	else if (optimizedb && !MEcmp((PTR)temp, (PTR)"-le", 3))
	{
	    /* "-length= flag" */
	    char 	*equals;	/* pointer to '=' */

	    /* look for '=' */
	    equals = STindex(temp, "=", 0);
	    /* check for '=' and spelling of flag */
	    if (equals == NULL
		||
		(equals - temp) > 7
		||
		STncasecmp(temp, "-length", 7))
	    {
		opq_error((DB_STATUS)E_DB_ERROR, (i4)E_OP090E_PARAMETER,
			  (i4)4, (i4)0, (PTR)g->opq_utilid,
			  (i4)0, (PTR)temp);
		(*opq_exit)();
	    }

	    /* convert and check value supplied */
	    if (CVal(equals+1, bound_length) 
		||
		*bound_length < MINLENGTH 
		|| 
		*bound_length > MAXLENGTH
		)
	    {
		i4	    minlength;
		i4	    maxlength;

		minlength = MINLENGTH;
		maxlength = MAXLENGTH;
		opq_error((DB_STATUS)E_DB_ERROR, (i4)E_OP0960_BAD_LENGTH, 
			  (i4)8, (i4)0, (PTR)g->opq_utilid,
			  (i4)0, (PTR)(equals+1),
			  (i4)sizeof(minlength), (PTR)&minlength,
			  (i4)sizeof(maxlength), (PTR)&maxlength);
		(*opq_exit)();
	    }
	}
	else if (statdump && !STcompare(temp, "-zq"))
	{
	    *quiet = TRUE;
	}
	else if (statdump && !STcompare(temp, "-zqq"))
	{
	    *supress_nostats = TRUE;
	}
	else if (statdump && !STcompare(temp, "-zdl"))
	{
	    *deleteflag = TRUE;
	}
	else if (statdump && !STcompare(temp, "-o"))
	{
	    /* Next arg must be file name */
	    parmno++;

	    if (parmno > argc)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0931_NO_FILE, (i4)0);
		(*opq_exit)();
	    }

	    *ofilenm = &argv[parmno][0];
	}
	else if (!STcompare(temp, "-zc"))
	{
	    g->opq_cats = TRUE;
	}
	else if (!STcompare(temp, "-zcpk"))
	{
	    g->opq_comppkey = TRUE;
	}
	else if (!STcompare(temp, "-zhex"))
	{
	    g->opq_hexout = TRUE;
	}
	else if (!MEcmp((PTR)temp, (PTR)"-zn", 3))
	{
	    i4	    val;

	    /* must be a number in 0 .. 30 range */
	    if (CVal(&argv[parmno][3], &val) || val < 0 || val > 30)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0932_BAD_PREC, (i4)0);
		(*opq_exit)();
	    }

	    g->opq_prec = val;
	}
	else
	{
	    /* treat any unrecognized flag as an ingres flag */

	    /* -uusername flag needs special treatment */
	    if (argv[parmno][0] == '-' && argv[parmno][1] == 'u')
	    {
		if (argv[parmno][2] == EOS)
		{
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0933_BAD_USER, (i4)0);
		    (*opq_exit)();
		}

		*userptr = &argv[parmno][2];
		continue;
	    }

            /* +w specified */
            if ( (argv[parmno][0] == '+') && (argv[parmno][1] == 'w') )
            {
                *waitopt = argv[parmno];
                continue;
            }

	    if (ifcnt >= (MAXIFLAGS-1) )
	    {   /* save one location for the DB name */
		i4	    maxiflags;

		maxiflags = MAXIFLAGS;
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0909_INGRESFLAGS, (i4)4, 
                    (i4)0, (PTR)g->opq_utilid,
                    (i4)sizeof(maxiflags), (PTR)&maxiflags);
		(*opq_exit)();
		/* utility: more than %d ingres flags specified
		*/
	    }
	    ifs[ifcnt++] = argv[parmno];
	}
    }

    if (!argv[parmno])
	return (TRUE);		    /* no database name */

    /* If -i was specified, ignore -length flag, if any */
    if (*ifilenm)
	*bound_length = 0;

    *dbindex	= parmno;
    *dbnameptr	= &ifs[ifcnt];	    /* where we will store pointer to
				    ** the database name */

    {
	char	    *argvp;	    /* ptr to database name */
	bool	    excl;

	while ((argvp = argv[parmno]) && CMalpha(argvp))
				    /* for each database name - right now
                                    ** there will be only one pass thru this
                                    ** loop */
	{
	    if (*dbindex != parmno)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP090A_DATABASE, (i4)4, 
                    (i4)0, (PTR)g->opq_utilid,
                    (i4)0, (PTR)argvp);
		return (TRUE);	    /* only allow one dbname for 
				    ** now just to make a simpler interface */
	    }
	    if (STlength(argvp) > sizeof(DB_DB_NAME))
	    {
		i4	    db_db_name;
		db_db_name = sizeof(DB_DB_NAME);
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP090B_DBLENGTH, (i4)6, 
                    (i4)0, (PTR)g->opq_utilid,
                    (i4)sizeof(db_db_name), (PTR)&db_db_name, 
                    (i4)0, (PTR)argvp);
		return (TRUE);
	    }

	    parmno++;

	    while (isrelation(argvp = argv[parmno], &excl))
	    {	/* for each relation name */
		/* Could be -xr"delim" so allow 3 extra chars */
		if (STlength(argvp) - STlength("-r") > NAMELEN+3)
		{
		    i4	    namelen1;
		    namelen1 = NAMELEN;
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP090C_RELLENGTH, (i4)6, 
			(i4)0, (PTR)g->opq_utilid,
			(i4)sizeof(namelen1), (PTR)&namelen1, 
                        (i4)0, (PTR)argvp);
		    return (TRUE);
		}
		parmno++;

		while (isattribute(argvp = argv[parmno]))
		{   /* for each attribute */
		    /* Could be -a"delim" so allow 2 extra */
		    if (STlength(argvp) - STlength("-a") > NAMELEN+2)
		    {
			i4	    namelen2;
			namelen2 = NAMELEN;
			opq_error( (DB_STATUS)E_DB_ERROR,
			    (i4)E_OP090D_ATTLENGTH, (i4)6, 
			    (i4)0, (PTR)g->opq_utilid,
			    (i4)sizeof(namelen2), (PTR)&namelen2, 
			    (i4)0, (PTR)argvp);
			return (TRUE);
		    }
		    parmno++;
		}
	    }
	}
	if (parmno != argc)
	{
	    if (argvp)
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP090E_PARAMETER, (i4)4, 
                    (i4)0, (PTR)g->opq_utilid,
                    (i4)0, (PTR)argvp);
	    return(TRUE);
	}
    }
    return(FALSE);			/* parameters are all valid */
}

/*{
** Name: prelem	- print histogram element
**
** Description:
**      Print user readable form of histogram element
**
** Inputs:
**	g		    ptr to global data struct
**        opq_utilid	    name of utility calling this routine
**	  opq_adfcb	    ptr to ADF ctrl block
**      dataptr             ptr to data value
**      datatype            type of data in dataptr
**	outf		    output file descriptor
**
** Outputs:
**	None.
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    print the value of the histogram element
**
** History:
**      4-dec-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	06-nov-90 (stec)
**	    Added minor fix for ASCII C compiler requested by Eurotech.
**	06-apr-01 (inkdo01)
**	    Changed adc_tmlen parms from *i2 to *i4.
**	9-nov-05 (inkdo01)
**	    Optionally support hex output as directed by "-zhex" option.
**	    Note, this can't be reloaded with optimizedb -i <filename>.
**      09-Jul-2010 (coomi01) b124051
**          Additionally use hex strings for byte data types.
*/
VOID
prelem(
OPQ_GLOBAL	   *g,
PTR		   dataptr,
DB_DATA_VALUE      *datatype,
FILE		   *outf)
{
    i4	default_width;
    i4  worst_width;

    /*
    ** Swtch on hex output if data type is byte, or the user has requested it.
    */
    if ((DB_BYTE_TYPE == datatype->db_datatype) || g->opq_hexout)
    {
	/* Call adu_hex() to generate the hex string, then printf it. */
	DB_STATUS	status;
	DB_DATA_VALUE	hexout, hexin;
	PTR		outptr;
	i2		datalen;

	STRUCT_ASSIGN_MACRO(*datatype, hexin);
	hexin.db_data = dataptr;
	hexout.db_datatype = DB_VCH_TYPE;
	hexout.db_length = OPQ_IO_VALUE_LENGTH;
	hexout.db_data = (PTR)&opq_convbuf;

	status = adu_hex(g->opq_adfcb, &hexin, &hexout);
	if (DB_FAILURE_MACRO(status))
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP091F_DATATYPE, (i4)8,
		(i4)0, (PTR)g->opq_utilid,
		(i4)sizeof(datatype->db_datatype), 
		    (PTR)&datatype->db_datatype, 
		(i4)sizeof(datatype->db_prec), 
		    (char *)&datatype->db_prec,
		(i4)sizeof(datatype->db_length), 
		    (PTR)&datatype->db_length);
	    /* %c: error displaying type %d, prec %d, length %d
            */
	    return;
	}

	datalen = ((DB_TEXT_STRING *)hexout.db_data)->db_t_count;
	outptr = (PTR)&((DB_TEXT_STRING *)hexout.db_data)->db_t_text;
					/* skip varchar length */
	outptr[datalen] = 0;		/* null terminate */
	if (outf)
	    SIfprintf(outf, TEXT8, outptr);
	else SIprintf(TEXT8, outptr);

	return;
    }

    {
	DB_STATUS   tmlen_status;

	tmlen_status = adc_tmlen(g->opq_adfcb, datatype,
	    &default_width, &worst_width);
	if (DB_FAILURE_MACRO(tmlen_status))
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP091F_DATATYPE, (i4)8,
		(i4)0, (PTR)g->opq_utilid,
		(i4)sizeof(datatype->db_datatype), 
		    (PTR)&datatype->db_datatype, 
		(i4)sizeof(datatype->db_prec), 
		    (char *)&datatype->db_prec,
		(i4)sizeof(datatype->db_length), 
		    (PTR)&datatype->db_length);
	    /* %c: error displaying type %d, prec %d, length %d
            */
	    return;
	}
    }

    {
	DB_STATUS           tmcvt_status;
	i4		    outlen;
	char                *charbuf;
        DB_DATA_VALUE       dest_dt;

        datatype->db_data = dataptr;
	dest_dt.db_length = default_width;
        charbuf = (char *)(dest_dt.db_data = (PTR)&opq_convbuf);
	dest_dt.db_prec = 0;

	/* use the term. monitor conversion routines to display histo results
	*/
	tmcvt_status = adc_tmcvt(g->opq_adfcb, datatype, &dest_dt, &outlen);
	if (DB_SUCCESS_MACRO(tmcvt_status))
	{
	    charbuf[outlen] = 0;	/* null terminate buf data */

	    if (outf)
		SIfprintf(outf, TEXT8, charbuf);
	    else
		SIprintf(TEXT8, charbuf);
	}
	else
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP091F_DATATYPE, (i4)8,
		(i4)0, (PTR)g->opq_utilid,
		(i4)sizeof(datatype->db_datatype), 
		    (PTR)&datatype->db_datatype, 
		(i4)sizeof(datatype->db_prec), 
		    (char *)&datatype->db_prec,
		(i4)sizeof(datatype->db_length), 
		    (PTR)&datatype->db_length);
	    /* utility: error displaying type %d, prec %d, length %d
            */
	    return;
	}
    }
}

/*{
** Name: opq_current_date - Get current local date and time
**
** Description:
**      Returns pointer to string representation of current local date and
**      time.
**
** Inputs:
**	g		    Ptr to global data structure
**        opq_utilid	    Name of utility calling this routine
**	  opq_adfcb	    Ptr to ADF ctrl block
**	out		    Buffer for writing date and time.
**			    Must be at least DATE_SIZE + 1.
**
** Outputs:
**	out		    Contains current date and time.
**
**	Returns:
**	    char * pointer to out, filled with current date and time.
**	Exceptions:
**	    none
**
** Side Effects:
**	    None
**
** History:
**      18-jan-93 (rganski)
**          Initial creation from part of insert_histo().
*/
static
char *
opq_current_date(
OPQ_GLOBAL	*g,
char		*out)
{
    SYSTIME		stime;
    struct {			/* DB_TEXT_STRING must be aligned */
	DB_TEXT_STRING  string;
	char	    data[4];
    } time;
    DB_TEXT_STRING	*timestr = &time.string;
    DB_DATA_VALUE	dv1, dv2, rdv;
    DB_STATUS		stat;
    bool		error;
    i4			outlen;
    
    timestr->db_t_count = sizeof("now") - 1;
    STcopy("now", (char *)timestr->db_t_text);
    
    dv1.db_length	= (i4)(DB_CNTSIZE + sizeof("now") - 1);
    dv1.db_data		= (PTR)timestr;
    dv1.db_datatype	= (DB_DT_ID)DB_LTXT_TYPE;
    dv1.db_prec		= 0;
    
    dv2.db_length	= 0;
    dv2.db_data		= (PTR)0;
    dv2.db_datatype	= DB_DTE_TYPE;
    dv2.db_prec		= 0;
    
    error = (bool)TRUE;
    
    stat = adc_lenchk(g->opq_adfcb, TRUE, &dv2, &rdv);
    
    if (DB_SUCCESS_MACRO(stat))
    {
	STRUCT_ASSIGN_MACRO(rdv, dv2);
	
	if (opq_date.length < (i4)rdv.db_length)
	{
	    /* allocate new buffer */
	    opq_date.bufptr = (PTR) NULL;
	    (VOID) getspace((char *)g->opq_utilid,
			    (PTR *)&opq_date.bufptr, (u_i4)rdv.db_length);
	    
	    opq_date.length = (i4)rdv.db_length;
	}
	
	dv2.db_data = opq_date.bufptr;
	
	stat = adc_cvinto(g->opq_adfcb, &dv1, &dv2);
	
	if (DB_SUCCESS_MACRO(stat))
	{
	    dv1.db_length = DATE_SIZE;
	    dv1.db_data = (PTR)out;
	    
	    stat = adc_tmcvt(g->opq_adfcb, &dv2, &dv1, &outlen);
	    
	    if (DB_SUCCESS_MACRO(stat))
	    {
		out[outlen] = '\0';
		error = (bool)FALSE;
	    }
	}
    }
    
    if (error == (bool)TRUE)
    {
	TMnow(&stime);
	TMstr(&stime, out);
    }
    
    return(out);
}

/*{
** Name: opq_print_stats - print histogram for an attribute
**
** Description:
**      Prints histogram for an attribute in user-readable form.
**
** Inputs:
**	g		    Ptr to global data structure
**	  opq_adfcb	    Ptr to ADF ctrl block
**      relp                Ptr to relation descriptor
**      attrp               Ptr to attribute descriptor upon
**                          Which the histogram is built
**	version		    Ptr to statistics version for these statistics.
**			    If NULL, no version exists.
**      row_count           Number of tuples in relation
**	pages		    Number of prime pages in relation
**	overflow	    Number of overflow pages in relation
**	date		    Date and time statistics were generated.
**			    If NULL, defaults to current local date and time.
**      nunique             Number of unique values in the column
**	reptfact	    Repetition factor for this column.
**	unique_flag	    A character ('Y' or 'N') specifying whether the
**			    values in this column are unique.
**	iscomplete	    Boolean value specifying whether column is complete
**			    for its domain.
**	domain		    Domain of the column.
**      cellnm		    Number of cells in the histogram.
**	nullcount	    Percentage of nulls in the column.
**      cell_count          Array of cell counts associated
**                          with histogram cell intervals
**      cell_repf           Array of repetition factors associated
**                          with histogram cell intervals
**      intervals           Array of boundary values delimiting histogram cells
**	outf		    Output file descriptor. NULL if standard output.
**	verbose		    TRUE if "-zv" flag was used (optimizedb only).
**	quiet		    TRUE if "-zq" flag was used (statdump only).
**
** Outputs:
**	None.
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Prints the statistics for the attribute.
**
** History:
**      18-jan-93 (rganski)
**          Initial creation from parts of optimizedb and statdump.
**	    Added printing of new version and value length fields when version
**	    exists (version is not null).
**	26-apr-93 (rganski)
**	    Character Histogram Enhancements project:
**	    Added printing of character set statistics when column is of
**	    character type and there is a version.
**	29-apr-93 (rganski)
**	    Replaced non-standard local declarations of charnunique and
**	    chardensity with global declarations.
**	9-jan-96 (inkdo01)
**	    Added support of per-cell repetition factors to print_stats.
**      18-May-2001 ( huazh01 )
**          Add variable extra_len. This will be used when it needs to 
**          to reduce the total number  of digits after the decimal point
**	15-apr-05 (inkdo01)
**	    Don't print cells when there aren't any (as in all null hist).
**	17-oct-05 (inkdo01)
**	    Add precision to display of repetition factor to get rid of those
**	    annoying OP0991 warnings.
**      27-jun-2007 (huazh01)
**          Adjust the precision to display of a column's repetition factor 
**          if it is over 10 million. This allows such reptfact to be 
**          displayed correctly in the output. 
**          bug 118339.
**	2-Apr-2008 (kibro01) b120056
**	    Display warning if precision is not sufficient to differentiate
**	    between two histogram cells (and thus cause E_OP0945 on import).
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
**	7-Nov-2008 (kibro01) b120056
**	    Do not display warning about precision between cells when using
**	    then hex form of statdump.
**      20-Feb-2009 (huazh01)
**          Adjust the display precision if the null_count is over 10 
**          million. (b121643)
**      08-July-2010 (coomi01) b124029
**          When looking for monotone problems, compare histogram keys
**          directly.
**      09-Jul-2010 (coomi01) b124051
**          Detect byte datatypes, and then use hex strings to export values.
*/
VOID
opq_print_stats(
OPQ_GLOBAL	*g,
OPQ_RLIST	*relp,
OPQ_ALIST       *attrp,
char		*version,
i4		row_count,
i4		pages,
i4		overflow,
char        	*date,
f8		nunique,
f8          	reptfact,
char           	*unique_flag,
i1          	iscomplete,
i2          	domain,
i2		cellnm,
OPO_TUPLES    	nullcount,
OPH_COUNTS	cell_count,
OPH_COUNTS	cell_repf,
OPH_CELLS	intervals,
FILE		*outf,
bool		verbose,
bool		quiet)
{
    /*
    ** Flag hex production by either command line, or use of a Byte data type
    */
    bool hexOut = g->opq_hexout || (DB_BYTE_TYPE == attrp->hist_dt.db_datatype);
    char histval_copy[DB_MAXSTRING+1]={0};

    /* Print information from iistatistics */
    {
	char		*nullable;
	char		*text1ptr;	/* Pointer to verion of first line */
	char		*text5ptr;	/* Pointer to verion of fifth line */
	char		*text6ptr;	/* Pointer to verion of sixth line */
	char		current_date[DATE_SIZE + 1];
	char		text5o_fmt[sizeof(TEXT5O) + 10];
	char		text6o_fmt[sizeof(TEXT6O1) + 10];
	bool		composite = (relp->reltidx != 0 || g->opq_comppkey);
        i4              length, null_length; 

        length = null_length = 0;

	if (attrp->nullable == 'N' || composite)
	    nullable = "not_nullable";
	else
	    nullable = "nullable";

	if (version == NULL)
	{
	    /* No version: use old format */
	    text1ptr = TEXT1;
	    text5ptr = TEXT5;
	    text6ptr = TEXT6O;
	}
	else
	{
	    /* Version is present: use new formats with version and value
	    ** length fields */
	    text1ptr = TEXT1O;
	    text5ptr = TEXT5O;
	    text6ptr = TEXT6O1;
	}

	if (outf)
	{
	    SIfprintf(outf, text1ptr, g->opq_dbname, version);
	    SIfprintf(outf, TEXT2, (char *)&relp->relname, 
		      row_count, pages, overflow);
	    if (composite)
	        SIfprintf(outf, TEXT3D, attrp->userlen,
		      (i4)0, nullable);
	    else SIfprintf(outf, TEXT3, (char *)&attrp->attname,
		      (char *)&attrp->typename, attrp->userlen,
		      attrp->scale, nullable);
	}
	else
	{
	    SIprintf(text1ptr, g->opq_dbname, version);
	    SIprintf(TEXT2, (char *)&relp->relname,
		     row_count, pages, overflow);
	    if (composite)
	        SIprintf(TEXT3D, attrp->userlen,
		      (i4)0, nullable);
	    else SIprintf(TEXT3, (char *)&attrp->attname,
		     (char *)&attrp->typename, attrp->userlen,
		     attrp->scale, nullable);
	}


	if (date == (char *) NULL)
	    /* Use current date and time */
	    date = opq_current_date(g, current_date);

        length = (i4)MHlog10(reptfact) + 1;
        null_length = (i4)MHlog10(nullcount) + 1; 

	if (sizeof(OPO_TUPLES) == sizeof(f8))
	{
            if (length >= g->opq_adfcb->adf_outarg.ad_f8prec)
                STprintf(text5o_fmt, text5ptr,
                     g->opq_adfcb->adf_outarg.ad_f8width,
                     g->opq_adfcb->adf_outarg.ad_f8width - length - 1 );          
            else
	        STprintf(text5o_fmt, text5ptr,
		     g->opq_adfcb->adf_outarg.ad_f8width,
		     g->opq_adfcb->adf_outarg.ad_f8prec);

            if (null_length >= g->opq_adfcb->adf_outarg.ad_f8prec)
                STprintf(text6o_fmt, text6ptr,
                     g->opq_adfcb->adf_outarg.ad_f8width,
                     g->opq_adfcb->adf_outarg.ad_f8width - null_length - 1);
            else
	        STprintf(text6o_fmt, text6ptr,
		     g->opq_adfcb->adf_outarg.ad_f8width,
		     g->opq_adfcb->adf_outarg.ad_f8prec);
	}
	else
	{
            if (length >= g->opq_adfcb->adf_outarg.ad_f4prec)
                STprintf(text5o_fmt, text5ptr,
                     g->opq_adfcb->adf_outarg.ad_f4width,
                     g->opq_adfcb->adf_outarg.ad_f4width - length - 1 );
            else
	        STprintf(text5o_fmt, text5ptr,
		     g->opq_adfcb->adf_outarg.ad_f4width,
		     g->opq_adfcb->adf_outarg.ad_f4prec);

            if (null_length >= g->opq_adfcb->adf_outarg.ad_f4prec)
                STprintf(text6o_fmt, text6ptr,
                     g->opq_adfcb->adf_outarg.ad_f4width,
                     g->opq_adfcb->adf_outarg.ad_f4width - null_length - 1);
            else
                STprintf(text6o_fmt, text6ptr,
		     g->opq_adfcb->adf_outarg.ad_f4width,
		     g->opq_adfcb->adf_outarg.ad_f4prec);
	}

	/* Helping hint: the "g->opq_adfcb->adf_decimal.db_decimal" refs
	** in print lists are to define the decimal character to display.
	*/

	if (outf)
	{
	    SIfprintf(outf, TEXT4O, date, nunique,
		      g->opq_adfcb->adf_decimal.db_decimal); 
	    SIfprintf(outf, text5o_fmt, reptfact,
		      g->opq_adfcb->adf_decimal.db_decimal,
		      unique_flag[0], iscomplete);
	    SIfprintf(outf, text6o_fmt, domain, cellnm,
		      nullcount, g->opq_adfcb->adf_decimal.db_decimal,
		      attrp->hist_dt.db_length);
	}
	else
	{
	    SIprintf(TEXT4O, date, nunique,
		     g->opq_adfcb->adf_decimal.db_decimal); 
	    SIprintf(text5o_fmt, reptfact, g->opq_adfcb->adf_decimal.db_decimal,
		     unique_flag[0], iscomplete);
	    SIprintf(text6o_fmt, domain, cellnm,
		     nullcount, g->opq_adfcb->adf_decimal.db_decimal,
		     attrp->hist_dt.db_length);
	}

	if (verbose)
	{
	    /* -zv (verbose) flag was specified. Only first 6 lines are
	    ** printed. */
	    SIprintf("\n");
	    return;
	}

    }

    if (quiet || cellnm <= 0)
    {
	/* -zq flag (quiet mode) was specified or no cells (all null);
	** don't print histogram. */
	SIprintf("\n");
	return;
    }
    else
    {
	/* Print out each histogram cell: count, then boundary value */

	i4		cell;		/* Number of the histogram cell 
					** currently being dumped */
	OPH_CELLS	curr_value;	/* Pointer to boundary value of
					** histogram cell being dumped */
	OPS_DTLENGTH	value_length;	/* Length of a boundary value */

	char		text7o_fmt[sizeof(TEXT7O1) + 10];
	
	i4             extra_len = 0;


	if (sizeof(OPN_PERCENT) == sizeof(f8))
	{
	    /*
	    ** Create a output formatter incorporating hex values if required.
	    */
	    STprintf(text7o_fmt, hexOut ? TEXT7O1_hex : TEXT7O1 ,
		     g->opq_adfcb->adf_outarg.ad_f8width,
		     g->opq_adfcb->adf_outarg.ad_f8prec, 
		     g->opq_adfcb->adf_outarg.ad_f8width,
		     g->opq_adfcb->adf_outarg.ad_f8prec);
	}
	else
	{
	    STprintf(text7o_fmt, hexOut ? TEXT7O1_hex : TEXT7O1,
		     g->opq_adfcb->adf_outarg.ad_f4width,
		     g->opq_adfcb->adf_outarg.ad_f4prec, 
		     g->opq_adfcb->adf_outarg.ad_f4width,
		     g->opq_adfcb->adf_outarg.ad_f4prec);
	}

	curr_value = intervals;
	value_length = attrp->hist_dt.db_length;

	for (cell = 0; cell < cellnm; cell++)
	{

	    extra_len = 0;
	   
	    if ( ( sizeof(OPN_PERCENT) != sizeof(f8) ) &&
		(cell_repf[cell] >= 10000000  ))
	    {
		extra_len = ( (i4)MHlog10( (f8)cell_repf[cell] ) ) - 6;

		STprintf(text7o_fmt, hexOut ? TEXT7O1_hex : TEXT7O1,
			 g->opq_adfcb->adf_outarg.ad_f4width,
			 g->opq_adfcb->adf_outarg.ad_f4prec,
			 g->opq_adfcb->adf_outarg.ad_f4width ,
			 g->opq_adfcb->adf_outarg.ad_f4prec - extra_len );
	    }


	    /* Print cell number, count, and value for cell */
	    if (outf)
	    {
		SIfprintf(outf, text7o_fmt, cell, cell_count[cell],
			g->opq_adfcb->adf_decimal.db_decimal,
			cell_repf[cell], g->opq_adfcb->adf_decimal.db_decimal);
	    }
	    else
	    {
		SIprintf(text7o_fmt, cell, cell_count[cell],
			g->opq_adfcb->adf_decimal.db_decimal,
			cell_repf[cell], g->opq_adfcb->adf_decimal.db_decimal);
	    }
	    prelem(g, (PTR)curr_value, &attrp->hist_dt, outf);

	    /*
	    ** Compare the histogram value with its' predecessor
	    */
	    if (!g->opq_hexout &&
	        MEcmp(curr_value, curr_value+value_length, value_length) == 0)
	    {
	        opq_error((DB_STATUS)E_DB_WARN,
		    (i4)W_OP0976_PREC_WARNING, (i4)6,
			0, cell-1,
			0, cell,
			STlen(histval_copy), histval_copy);
	    }

	    /*
	    ** Copy entire key to histval_copy
	    */
	    MEcopy(opq_convbuf.buf.value, value_length, histval_copy);


	    curr_value += value_length;	/* Point to next boundary value */
	}		

	/* If column is of character type and there is a version, print
	** character set statistics */
	if ((attrp->hist_dt.db_datatype == DB_CHA_TYPE ||
	     attrp->hist_dt.db_datatype == DB_BYTE_TYPE) && version != NULL)
	{
	    char	text11_fmt[sizeof(TEXT11O) + 10]; /* format string for
							** floating points */
	    OPH_CELLS	offset;				/* Location in buffer */
	    i4		i;				/* Loop counter */
	    
	    /* charnunique and chardensity are global variables */

	    /* Copy the statistics to their respective arrays; this is
	    ** necessary because the numbers may not be aligned in the
	    ** histogram buffer */
	    offset = intervals + cellnm * value_length;	/* charnunique */
	    MEcopy((PTR)(offset), value_length * sizeof(i4),
		   (PTR)charnunique);
	    offset += value_length * sizeof(i4);
	    MEcopy((PTR)(offset), value_length * sizeof(f4),
		   (PTR)chardensity);

	    /* Choose width and precision for printing of densities.
	    ** Width is 2 more than prec, to allow for 1 or 0 and '.' */
	    if (sizeof(OPN_PERCENT) == sizeof(f8))
	    {
		STprintf(text11_fmt, TEXT11O,
			 g->opq_adfcb->adf_outarg.ad_f8prec + 2,
			 g->opq_adfcb->adf_outarg.ad_f8prec);
	    }
	    else
	    {
		STprintf(text11_fmt, TEXT11O,
			 g->opq_adfcb->adf_outarg.ad_f4prec + 2,
			 g->opq_adfcb->adf_outarg.ad_f4prec);
	    }

	    /* Print character set statistics */
	    if (outf)
	    {
		SIfprintf(outf, TEXT9O);
		for (i = 0; i < value_length; i++)
		    SIfprintf(outf, TEXT12O, charnunique[i]);
		SIfprintf(outf, TEXT10O);
		for (i = 0; i < value_length; i++)
		    SIfprintf(outf, text11_fmt, chardensity[i],
			      g->opq_adfcb->adf_decimal.db_decimal); 
	    }
	    else
	    {
		SIprintf(TEXT9O);
		for (i = 0; i < value_length; i++)
		    SIprintf(TEXT12O, charnunique[i]);
		SIprintf(TEXT10O);
		for (i = 0; i < value_length; i++)
		    SIprintf(text11_fmt, chardensity[i],
			      g->opq_adfcb->adf_decimal.db_decimal); 
	    }
	}

	/* Print final newline */
	if (outf)
	{
	    SIfprintf(outf, "\n");
	}
	else
	{
	    SIprintf("\n");
	}
    }

    return;

}

exec sql whenever sqlerror call opq_sqlerror;
exec sql whenever sqlwarning call opq_sqlerror;

/*
** Name: opq_cntrows - count rows in a relation.
**
** Description:
**      Counts rows in a relation when the count is not available
**	from system catalogs.
**
** Inputs:
**	rp		    ptr to relation descriptor
**
** Outputs:
**	rp
**	    ntups	    updated row count
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	Updates relation descriptor.
**
** History:
**      12-dec-89 (stec)
**	    written.
**	20-sep-93 (rganski)
**	    Delimited ID support. Use rp->delimname instead of rp->relname in
**	    select statement.
**      02-Jan-1997 (merja01)
**          Change long to i4 to fix Bug 79066 E_LQ000A Conversion failure
**          on axp.osf
*/
VOID
opq_cntrows(
OPQ_RLIST   *rp)
{
# define OPQ_CQRY1	"select count(*) from %s"
    exec sql begin declare section;
	char    stmt[sizeof(OPQ_CQRY1) + DB_GW1_MAXNAME + 2];
    exec sql end declare section;

    IISQLDA	*sqlda = &_sqlda;
    i4	nrows = -1; /* indicate no rows */

    /* 
    ** Warn user about row count missing from catalogs, this
    ** action is time consuming, therefore user needs to know.
    */

    opq_error( (DB_STATUS)E_DB_OK,
	(i4)I_OP0958_COUNTING_ROWS, (i4)2,
	(i4)0, (PTR)&rp->relname);

    STprintf(stmt, OPQ_CQRY1,
	&rp->delimname[0]);
    sqlda->sqln = 1;
    /* FIXME (schka24) This is really doing it the hard way.
    ** Use execute immediate instead.
    */
    exec sql prepare s from :stmt;
    exec sql describe s into :sqlda;

    /* describe location for the count(*) */
    sqlda->sqlvar[0].sqldata = (char *)&nrows;
    sqlda->sqlvar[0].sqllen = sizeof(i4);

    /* retrieve data for the constructed query. */
    exec sql open c1 for readonly;

    while (sqlca.sqlcode == GE_OK)
    {
	exec sql fetch c1 using descriptor :sqlda;
    }

    exec sql close c1;

    rp->ntups = nrows;
}

/*
** Name: opq_phys - find physical characteristics of a table.
**
** Description:
**      Table type is retrieved from iitables for both objects. If type is
**	'T', then table are updateable. If not statistical table must
**	be updated directly, i.e., standard catalogs cannot be used.
**
** Inputs:
**      utilid		    name of utility calling this routine
**	rp		    ptr to relation descriptor
**
** Outputs:
**	None.
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	Updates relation descriptor.
**
** History:
**      12-mar-89 (stec)
**	    written.
*/
VOID
opq_phys(
OPQ_RLIST   *rp)
{
    exec sql begin declare section;
	char        *es_relname;
	char        *es_ownname;
	i4	    es_nrows;
	i4	    es_pages;
	i4	    es_ovflow;
    exec sql end declare section;

    es_relname = (char *)&rp->relname;
    es_ownname = (char *)&rp->ownname;

    exec sql repeated select num_rows,
	     number_pages, overflow_pages
	into :es_nrows, :es_pages, :es_ovflow
	from iiphysical_tables
	where table_owner = :es_ownname and
	      table_name  = :es_relname;

    if (sqlca.sqlcode == GE_OK)
    {
	/* There should be only 1 row. */
	rp->pages    = es_pages;
	rp->overflow = es_ovflow;
	rp->ntups    = es_nrows;
    }
}

/*{
** Name: opq_idxlate
**
** Description:
**      Perform necessary translations on identifier as specified on command
**      line.
**
**	The identifier is first run through ID translation exactly as-is,
**	using the case translation flags that were set up from the database
**	characteristics.  If the ID translator says that the ID is an
**	invalid regular identifier, we'll put double-quotes around it
**	and try again.  (This is to deal with -r"foo bar" where the OS
**	shell eats up the double quotes.)
**
**	If successful ID translation tells us that the ID is a delimited
**	ID, an unnormalized (ie double quoted) copy is returned into
**	the "delimname" result.  Otherwise an exact copy is returned
**	into "delimname".  (Actually it would be OK to always unnormalize
**	the ID, but we'll try to avoid introducing dquotes unnecessarily!)
**
** Inputs:
**	g			Ptr to global data struct
**	  opq_dbcaps		Database capabilities
**	     cui_flags		Indicates case translation semantics of database
**	src			Source identifier
**
** Outputs:
**	xlatename		Where to put translated normalized name
**	delimname		Where to put translated name, delimited if
**				double-quotes are needed
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
* History:
**      20-sep-93 (rganski)
**          Initial creation for delimited ID support.
**	15-Mar-2005 (schka24)
**	    Add unnormalized output.
**	24-Jun-2005 (schka24)
**	    Treat $-identifiers as delimited ones, even though idxlate
**	    won't tell us that they need to be delimited.
*/
VOID
opq_idxlate(
OPQ_GLOBAL	*g,
char		*src,
char		*xlatename,
char		*delimname)
{
    u_i4		norm_len;
    u_i4		retmode;
    DB_ERROR		err_blk;
    DB_STATUS	    	status;

    norm_len = DB_GW1_MAXNAME;
    status = cui_idxlate((u_char *)src, (u_i4 *)NULL, (u_char *)xlatename,
			 &norm_len, g->opq_dbcaps.cui_flags, &retmode,
			 &err_blk);
	
    if (DB_FAILURE_MACRO(status))
    {
	/* Error, but maybe the ID would be valid if we double-quoted it.
	** Try again as a delimited ID.  This is for the case where the
	** user thought (s)he specified double-quotes on the command line,
	** but the OS shell cleverly stripped the quotes away...
	*/
	if (retmode == CUI_ID_REG
	  && (err_blk.err_code == E_US1A20_ID_START
		|| err_blk.err_code == E_US1A14_ID_ANSI_START
		|| err_blk.err_code == E_US1A22_ID_BODY
		|| err_blk.err_code == E_US1A25_ID_TOO_LONG
		|| err_blk.err_code == E_US1A13_ID_ANSI_BODY
		|| err_blk.err_code == E_US1A12_ID_ANSI_END) )
	{
	    /* Try it again with implied delimiters */
	    norm_len = DB_GW1_MAXNAME;
	    status = cui_idxlate((u_char *)src, (u_i4 *)NULL,
			(u_char *)xlatename, &norm_len,
			g->opq_dbcaps.cui_flags | CUI_ID_NORM | CUI_ID_DLM,
			&retmode,
			&err_blk);
	}
    }
    if (DB_FAILURE_MACRO(status))
    {
	/* Error occurred. The error messages used by cui_idxlate
	** require 2 params: the first is the string "Identifier", the
	** second is the source identifier.
	*/
	opq_error((DB_STATUS)E_DB_ERROR, (i4)err_blk.err_code, (i4)4,
		  (i4)0, (PTR)"Identifier", (i4)0, (PTR)src);
	(*opq_exit)();
    }
	
    /* Null terminate the translated name.
    */
    *(xlatename + norm_len) = EOS;

    /* Un-normalize the ID so that caller has a delimited version if the
    ** ID requires delimiters -- regardless of whether we found the
    ** delimiters on the command line, or invented them.
    ** We need to check one special case:  for whatever crazed reason,
    ** idxlate doesn't tell us that an ID needs to be delimited if the
    ** leading character is a $!  (The comment says something about allowing
    ** $ingres through, whatever that means.)  $-names won't be allowed in
    ** the SQL we generate without delimiters, so add delimiters for them
    ** too.
    */
    if (retmode != CUI_ID_DLM && xlatename[0] != '$')
	STcopy(xlatename, delimname);
    else
	opq_idunorm(xlatename, delimname);
}

/*{
** Name: opq_idunorm
**
** Description:
**      Unnormalize identifier read from catalog: wrap delimiters around it,
**      and escape embedded delimiters/quotes. The resulting identifier can be
**      used in the select and from clauses of a select statement, and other
**      statements where a delimited identifier is appropriate.
**
** Inputs:
**	src			Source identifier, normalized.
**
** Outputs:
**      dst			Destination buffer for unnormalized identifier
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
* History:
**      20-sep-93 (rganski)
**          Initial creation for delimited ID support.
*/
VOID
opq_idunorm(
char		*src,
char		*dst)
{
    u_i4		unorm_len = DB_MAX_DELIMID; 
    DB_ERROR		err_blk;
    DB_STATUS	    	status;
	
    status = cui_idunorm((u_char *)src, (u_i4 *)NULL, (u_char *)dst,
			 &unorm_len, (u_i4)CUI_ID_DLM, &err_blk);
	
    if (DB_FAILURE_MACRO(status))
    {
	/* Error occurred. The error messages used by cui_idunorm
	** require 2 params: the first is the string "Identifier", the
	** second is the source identifier.
	*/
	opq_error((DB_STATUS)E_DB_ERROR, (i4)err_blk.err_code, (i4)4,
		  (i4)0, (PTR)"Identifier", (i4)0, (PTR)src);
	(*opq_exit)();
    }
	
    /* Null terminate the unnormalized name.
    */
    *(dst + unorm_len) = EOS;
}

/*{
** Name: i_rel_list_from_input	- read relation specs from argument list
**
** Description:
**      Create relation descriptors only for those relations specified
**      in the input list (as opposed to the entire database), or
**	identify tables to be excluded.
**
** Inputs:
**	g		ptr to global data struct
**        opq_utilid	name of utility calling this routine
**        opq_dbname    ptr to the database name
**        opq_owner     the database owner
**        opq_dba       the DBA
**	  opq_ dbcaps	structure showing where physical table info
**			is to be gotten from.
**	rellist		ptr to array of ptr to relation descriptors
**      argv            ptr to base of array of ptr
**                      to user table names (plus respective
**                      attribute names)
**	checkstats	flag indicating request to check id stats present
**
** Outputs:
**      rellist		array of ptrs to relations descriptors
**                      which need statistics gathered
**
**	Returns:
**	    TRUE - if relation descriptors initialized successfully
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-nov-86 (seputis)
**          initial creation
**	16-sep-88 (stec)
**	    Use SQL, convert to standard catalogs, use new OPQ_RLIST.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	13-sep-89 (stec)
**	    Add initialization for samplecrtd field.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	20-sep-93 (rganski)
**	    Added normalization for delimited IDs.
**	    Moved initialization of relation name and owner name outside of for
**	    loop.
**      08-jun-99 (chash01)
**          fetch row_width fromiitables forRMS GW. USing it to calculate 
**          number of pages.
**	5-apr-00 (inkdo01)
**	    Support for composite histograms.
**	16-july-01 (inkdo01)
**	    Add support for "-xr" command line option.
**	10-aug-05 (inkdo01)
**	    Preset statset flag using table_stats view column.
**	25-Aug-2005 (hweho01)
**	   Change es_tstat from char[2] to char[9], avoid "W_OP0961 Truncation
**         occurred" msg.
**	11-oct-05 (inkdo01)
**	    Remove es_tstat and defective changes to statset.
**	01-nov-2005 (thaju02) (B115475)
**	    Only query iistats/iihistograms if statdump.
**	    Call opq_cntrows() if !OPQ_T_SRC, see r_rel_list_from_rel_rel.
**	22-dec-06 (hayke02)
**	    Reinstate es_tstat and switch off 'set statistics' if the new "-zy"
**	    flag is set, and es_tstat/iitables.table_stats is 'Y' (indicating
**	    that 'set statistics' has already been run). This change implements
**	    implements SIR 117405.
**	15-feb-08 (wanfr01)
**	    Bug 119927
**	    SIR 114323 will result in iistatistic rows without corresponding
**	    iihistogram rows.  Set 'withstats' if no histogram exists.
**      20-Mar-08 (coomi01) : b120138
**          Change relational object enquiry to allow synonyms for tables to be
**        used.
**    02-sep-08 (hayke02)
**        Repair change for SIR 117405 so that we init and set statset
**        correctly.
**      01-Feb-2010 (maspa05) b123140
**        Disallow multiple -r flags with the same table name
**      11-Feb-2010 (maspa05) b123140
**        Missing parameter in STxcompare in above change
**      20-Jul-2010 (hanal04) Bug 124100
**        Do not try to select from iisynonym when we are connected to a
**        STAR DB. It does not exist.
*/
bool
i_rel_list_from_input(
OPQ_GLOBAL	*g,
OPQ_RLIST	**rellist,
char		**argv,
bool		statdump)
{
    i4		ri;		/* number of relations processed so far	*/
    i4		parmno;         /* number of parameter being analyzed */
    OPQ_RLIST	*rp;
    bool	prevexrel, exrel;

    ri = 0;
    parmno = 0;

    /* for each relation in the argument list */
    while (isrelation(argv[parmno], &exrel))
    {
	exec sql begin declare section;
	    char        *es_owner;
	    char        *es_relname;
	    char        *es_ownname;
	    i4		es_nrows;
	    char	es_type[8 + 1];
	    i4		es_reltid;
	    char	es_tstat[8 + 1];
	    i4		es_xreltid;
	    i4		es_pages;
	    i4		es_ovflow;
            i4          es_relwid;
	exec sql end declare section;

	i4	    cnt;	/* no. of tuples found for a table. */
	bool	    first;
        i4          rl_i;       /* counter to check for duplicate tables */

	/* Check for combination of "-r" and "-xr"s. */
	if (parmno == 0)
	    prevexrel = exrel;
	else if (exrel != prevexrel)
	{
	    /* There's a mixture - send out error. */
            opq_error((DB_STATUS)E_DB_ERROR,
                (i4)E_OP0993_XRBADMIX, (i4)4,
                (i4)0, (PTR)g->opq_utilid,
                (i4)0, "-r and -xr");
            /* utility: mixture of -r and -xr parameters. */
            opq_exit();
        }
	prevexrel = exrel;

	if (ri >= g->opq_maxrel)
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP090F_TABLES, (i4)6, 
                (i4)0, (PTR)g->opq_utilid,
		(i4)sizeof(g->opq_maxrel), (PTR)&g->opq_maxrel, 
		(i4)0, (PTR)g->opq_dbname);
	    /* utility: more than %d tables for database  '%s'
	    */
	    return(FALSE);
	}

	rp = (OPQ_RLIST *) getspace( (char *)g->opq_utilid, (PTR *)&rellist[ri],
	    sizeof(OPQ_RLIST));

	STcopy((char *)&argv[parmno][STlength("-r") + exrel], (char *)&rp->argname);

        /* check this argument against current list for duplicates */

        for (rl_i=0;rl_i<ri && 
               STxcompare((char *)&rp->argname, 0, 
                          (char *)&rellist[rl_i]->argname,0,TRUE,TRUE);rl_i++);

        
        /* duplicate -r's found */

        if (rl_i < ri )
        {
                opq_error( (DB_STATUS) E_DB_ERROR,
                           (i4) E_OP0994_DUP_RELS, (i4) 4,
                           (i4)0, (char *)g->opq_utilid,
                           (i4)0, (char *)&rp->argname);
                return(FALSE);
        }
                
	/* Perform necessary translations on table name */
	/* Also places a delimited-if-necessary version in delimname */
	opq_idxlate(g, (char *)&rp->argname, (char *)&rp->relname,
			(char *) &rp->delimname);

	es_relname = (char *)&rp->relname;
	es_ownname = (char *)&rp->ownname;

	/* See if table exists. Try by owner first, then DBA */
	for (cnt = 0, first = TRUE, es_owner = (char *)&g->opq_owner;;
	     first = FALSE, es_owner = (char *)&g->opq_dba)
	{
            if(g->opq_dbms.dbms_type == OPQ_STAR)
            {   
                exec sql repeated select t.table_owner, t.num_rows, t.table_type,
		         t.table_reltid, t.table_reltidx, t.number_pages,
		         t.table_stats, t.overflow_pages, t.row_width
                    into :es_ownname, :es_nrows, :es_type,
		         :es_reltid, :es_xreltid, :es_pages,
		         :es_tstat, :es_ovflow, :es_relwid
		    from  iitables t;
    
	        exec sql begin;
		    /* Trim trailing white space and save table info */
		    (VOID) STtrmwhite((char *)&rp->relname);
		    rp->samplename[0] = EOS;
		    (VOID) STtrmwhite((char *)&rp->ownname);
		    if (!exrel)
		    {	/* No need for this if we're excluding the table. */
		        rp->nsample = (i4)0;
		        rp->reltid  = es_reltid;
		        rp->reltidx = es_xreltid;
		        rp->pages   = es_pages;
		        rp->overflow= es_ovflow;
		        rp->ntups = es_nrows;
                        rp->relwid = es_relwid;
		        rp->attcount = 0;	    /* count of interesting attributes*/
                        rp->statset = (bool)FALSE;
		        if ((es_tstat[0] == 'Y')	/* need to "set statistics" */
			    &&
			    (nosetstats))
			       rp->statset = (bool)TRUE;
		        rp->sampleok = (bool)TRUE;  /* assume sampling ok */
		        rp->physinfo = (bool)TRUE;  /* assume phys info retrieved */
		        rp->withstats = (bool)FALSE;/* assume no stats present */
		        rp->samplecrtd = (bool)FALSE;/* assume sample tbl not created */
		        rp->comphist = (bool)FALSE; /* assume no composite histogram */
		        rp->index = FALSE;	    /* assume base table - not ix */
		    }

		    cnt++;
	            exec sql end;

            }
            else
            {
                exec sql repeated select t.table_owner, t.num_rows, t.table_type,
		         t.table_reltid, t.table_reltidx, t.number_pages,
		         t.table_stats, t.overflow_pages, t.row_width
                    into :es_ownname, :es_nrows, :es_type,
		         :es_reltid, :es_xreltid, :es_pages,
		         :es_tstat, :es_ovflow, :es_relwid
		    from  (iitables t left join iisynonyms s on t.table_name = s.table_name)
		    where (t.table_owner = :es_owner) 
		    and   ((t.table_name = :es_relname) or (s.synonym_name = :es_relname));
    
	        exec sql begin;
		    /* Trim trailing white space and save table info */
		    (VOID) STtrmwhite((char *)&rp->relname);
		    rp->samplename[0] = EOS;
		    (VOID) STtrmwhite((char *)&rp->ownname);
		    if (!exrel)
		    {	/* No need for this if we're excluding the table. */
		        rp->nsample = (i4)0;
		        rp->reltid  = es_reltid;
		        rp->reltidx = es_xreltid;
		        rp->pages   = es_pages;
		        rp->overflow= es_ovflow;
		        rp->ntups = es_nrows;
                        rp->relwid = es_relwid;
		        rp->attcount = 0;	    /* count of interesting attributes*/
                        rp->statset = (bool)FALSE;
		        if ((es_tstat[0] == 'Y')	/* need to "set statistics" */
			    &&
			    (nosetstats))
			       rp->statset = (bool)TRUE;
		        rp->sampleok = (bool)TRUE;  /* assume sampling ok */
		        rp->physinfo = (bool)TRUE;  /* assume phys info retrieved */
		        rp->withstats = (bool)FALSE;/* assume no stats present */
		        rp->samplecrtd = (bool)FALSE;/* assume sample tbl not created */
		        rp->comphist = (bool)FALSE; /* assume no composite histogram */
		        rp->index = FALSE;	    /* assume base table - not ix */
		    }

		    cnt++;
	            exec sql end;
            }


	    if (cnt > 0 || first == FALSE)
	    {
		break;
	    }
	}

	if (cnt == 0)
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP0911_NOTABLE, (i4)6, 
		(i4)0, (PTR)g->opq_utilid,
		(i4)0, (PTR)g->opq_dbname, 
		(i4)0, (PTR)&rp->relname);
	    /* utility: database %s, table %s cannot be found
	    */
	}
	else if (es_type[0] != 'T' && es_type[0] != 'I')
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP0930_NOT_BASE, (i4)6, 
		(i4)0, (PTR)g->opq_utilid,
		(i4)0, (PTR)g->opq_dbname, 
		(i4)0, (PTR)&rp->relname);
	    /* utility: database %s, table %s is not a base table
	    ** and will be ignored.
	    */
	}
	else
	{
	    if (es_type[0] == 'I' || es_type[0] == 'T' && g->opq_comppkey)
	    {
		/* Index or comppkey - must be a composite histogram. */
		if (g->opq_utilid[0] == 'o' && isattribute(argv[parmno+1]))
		{
		    /* Can't have "-a"s after an index or composite
		    ** pkey specification. */
		    opq_error(E_DB_ERROR,
			E_OP0969_OPTIX_NOATTS, 4,
			0, (PTR)g->opq_dbname,
			0, (PTR)&rp->relname);
		}
		if (es_type[0] == 'I') rp->index = TRUE;
	    }
	    if (g->opq_dbcaps.phys_tbl_info_src != (i4)OPQ_T_SRC)
	    {
		opq_phys(rp);

		/*
		** Check whether number of rows is meaningful,
		** count if necessary
		*/
		if (!exrel && rp->ntups <= 0)
		{
		    opq_cntrows(rp);
		}

		if (rp->ntups <= 0)
		{
		    rp->physinfo = (bool)FALSE;
		}
	    }

	    /* relation is valid, so allocate
	    ** position by incrementing index.
	    */
	    ri++;
	}

# ifdef xDEBUG
	if (opq_xdebug)
	    SIprintf("\nAdded relation '%s'\n", &rp->relname);
# endif /* xDEBUG */

	/* Skip over atts - but send error if "-xr" and "-a". */
	while (isattribute(argv[++parmno]))
	 if (exrel)
	 {
	    /* There's a mixture - send out error. */
            opq_error((DB_STATUS)E_DB_ERROR,
                (i4)E_OP0993_XRBADMIX, (i4)4,
                (i4)0, (PTR)g->opq_utilid,
                (i4)0, "-a and -xr");
            /* utility: mixture of -a and -xr parameters. */
            opq_exit();
         }
    }

    /* Mark the end of list */
    rellist[ri] = NULL;

    /* Determine which tables have stats. */
    {
	register OPQ_RLIST   **rpp;

	for (rpp = &rellist[0]; statdump && *rpp; rpp++)
	{
	    exec sql begin declare section;
		i4	es_num;
		char    *es_relname;
		char    *es_ownname;
	    exec sql end declare section;

	    es_relname = (char *)&(*rpp)->relname;
	    es_ownname = (char *)&(*rpp)->ownname;

	    exec sql repeated select first 1 1
		into :es_num
		from iistats s
		where	s.table_owner = :es_ownname and
			s.table_name  = :es_relname;

	    exec sql begin;
		(*rpp)->withstats = (bool)TRUE;
	    exec sql end;
	}
    }

    return (TRUE);			/* successful */
}

/*{
** Name: opq_mxrel - calculate the maximum number of relations 
**		      to be considered.
**
** Description:
**      Calculates the maximum number of relations for this execution.
**	If relations are specified on the command line, they are counted,
**	if they are not specified then count of "interesting" relations 
**	is retrieved from the database.
**
** Inputs:
**	g		ptr to global data struct
**	  opq_cats	TRUE if cats to be considered
**        opq_owner     the database owner
**        opq_dba       the DBA
**	  opq_dbcaps	structure showing where physical table info
**			is to be gotten from.
**	argc		argument count
**      argv            ptr to base of array of ptr
**                      to user table names (plus respective
**                      attribute names)
**
** Outputs:
**	g		ptr to global data struct
**	  opq_maxrel	maximum number of relations to be considered
**
**	Returns:
**	    none.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-apr-90 (stec)
**          initial creation
**	22-mar-91 (stec)
**	    Add OPQ_MARGIN to the number of relations to prevent
**	    problems when tables are being created concurrently.
**	17-july-01 (inkdo01)
**	    Add support for "-xr" command line option.
**	28-may-02 (inkdo01)
**	    Add indexes to potential relation count (since they're the target
**	    of composite histograms).
**	30-Nov-2010 (kschendel)
**	    Exclude new spatial catalogs, they don't start with ii.
*/
VOID
opq_mxrel(
OPQ_GLOBAL  *g,
int	    argc,
char	    *argv[])
{
    exec sql begin declare section;
	i4	rel_cnt = 0;
	char	*es_ownr;
	char	*es_dba;
	char	*es_notin1, *es_notin2;
    exec sql end declare section;
    i4	    parmno;
    i4	    relc = 0;
    bool    exrel = FALSE;

    for (parmno = 0; parmno < argc; parmno++)
    {
	if (isrelation(argv[parmno], &exrel))
	    relc++;
    }

    if ((rel_cnt = relc) == 0 || exrel)
    {
	/* no relations have been specified on the command line. */
	es_dba = (char *)&g->opq_dba;
	es_ownr = (char *)&g->opq_owner;
	es_notin1 = es_notin2 = " ";
	if (!g->opq_cats)
	{
	    es_notin1 = "S";
	    es_notin2 = "G";
	    /* i.e. leaving only 'U' to be selected */
	}

	exec sql select count(*)
	    into :rel_cnt
	from iitables
  	where table_owner in (:es_ownr, :es_dba) and
	      system_use not in (:es_notin1, :es_notin2) and
  	      table_type  in ('T', 'I');
    }

    g->opq_maxrel = relc + rel_cnt + OPQ_MARGIN;
}

/*{
** Name: opq_collation	- get collation sequence for database
**
** Description:
**      Retrieve name of local collation sequence, if any, used by the current
**      database. If there is a collation sequence, initialize the ADF control
**      block's collation sequence field.
**
** Inputs:
**	g			ptr to global data struct
**
** Outputs:
**	g			ptr to global data struct
**	  .opq_adfcb		ptr to ADF ctrl block.
**	     .adf_collation	ptr to collation descriptor, or NULL
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-oct-93 (rganski)
**          Initial creation, for local collation sequence support.
**          b48831.
*/
VOID
opq_collation(
OPQ_GLOBAL *g)
{
    exec sql begin declare section;    
	char     *es_collation;
    exec sql end declare section;    

    DB_COLLATION_STR    collation_name;	/* Name of collation sequence */
    STATUS	status;		/* Returned from aducolinit() */
    ADULTABLE	*tbl;		/* Collation table */
    CL_SYS_ERR  sys_err;	/* OS-specific error information */

    es_collation = (char *)&collation_name;

    exec sql select dbmsinfo('collation')
	into :es_collation;

    if (STlength(es_collation) == 0)
    {
	/* No local collation sequence */
	g->opq_adfcb->adf_collation = NULL;
    }
    else
    {
	/* There is a local collation sequence. Get collation table from ADF
	** and initialize the ADF control block to point to it. */
	if (status = aducolinit(es_collation,
			       MEreqmem,
			       &tbl, &sys_err)
	    != OK)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		      (i4)E_DM00A0_UNKNOWN_COLLATION, (i4)0);
	}
	else
	{
	    g->opq_adfcb->adf_collation = (PTR)tbl;
	}
    }
}

/*{
** Name: opq_ucollation	- get Unicode collation
**
** Description:
**      Retrieve default Unicode collation sequence In response to
**	request to build histogram for NCHAR/NVARCHAR column(s).
**
** Inputs:
**	g			ptr to global data struct
**
** Outputs:
**	g			ptr to global data struct
**	  .opq_adfcb		ptr to ADF ctrl block.
**	     .adf_ucollation	ptr to Unicode collation table
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-dec-04 (inkdo01)
**          Written for Unicode histograms.
**	21-feb-05 (inkdo01)
**	    Init Unicode normalization flag while we're here.
*/
VOID
opq_ucollation(
OPQ_GLOBAL *g)
{
    exec sql begin declare section;    
	char     *es_collation;
    exec sql end declare section;    

    DB_COLLATION_STR	collation_name;	/* Name of collation sequence */
    STATUS	status;		/* Returned from aducolinit() */
    ADUUCETAB	*utbl;		/* Collation table */
    PTR		uvtbl;		/* var chunk of Unicode table */
    CL_SYS_ERR  sys_err;	/* OS-specific error information */

    es_collation = (char *)&collation_name;

    exec sql select dbmsinfo('ucollation')
	into :es_collation;

    if (STlength(es_collation) == 0)
    {
	/* No local Unicode collation sequence */
	g->opq_adfcb->adf_ucollation = NULL;
    }
    else
    {
	/* There is a Unicode collation sequence. Get collation table from ADF
	** and initialize the ADF control block to point to it. */
	if (status = aduucolinit(es_collation,
			       MEreqmem,
			       &utbl, &uvtbl, &sys_err)
	    != OK)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		      (i4)E_DM00A0_UNKNOWN_COLLATION, (i4)0);
	}
	else
	{
	    g->opq_adfcb->adf_ucollation = (PTR)utbl;
	}
    }
    g->opq_adfcb->adf_uninorm_flag = 0;
}

/*{
** Name: opq_owner	- get owner and dba names
**
** Description:
**      Return the owner name, and "$ingres" if the owner name is the dba.
**
** Inputs:
**	g		ptr to global data struct
**        opq_owner	owner name
**        opq_dba       dba name
**
** Outputs:
**	g		ptr to global data struct
**        opq_owner	initialized owner name
**        opq_dba       initialized dba name
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-jul-87 (seputis)
**          initial creation
**	12-feb-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	12-dec-89 (stec)
**	    Remove FROM clause from the query with dbmsinfo function.
**	    Split the select query into two, since this is required by
**	    Common SQL in case of dbmsinfo() function.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	18-oct-93 (rganski)
**	    Use case-insensitive comparison when comparing user name and dba
**	    name; otherwise comparison will fail when case semantics of iidbdb
**	    and the target database differ.
**	    FIXME: this is a temporary workaround until a solution is found for
**	    the case differences between iidbdb and target databases.
*/
VOID
opq_owner(
OPQ_GLOBAL *g)
{
    exec sql begin declare section;    
	char     *es_owner;
	char     *es_dba;
    exec sql end declare section;    

    es_owner = (char *)&g->opq_owner;
    es_dba = (char *)&g->opq_dba;

    /*
    ** We have to issue two separate queries since
    ** Common SQL requires this.
    */
    exec sql select dbmsinfo('username')
	into :es_owner;

    exec sql select dbmsinfo('dba')
	into :es_dba;

    if (!STbcompare(es_dba, 0, es_owner, 0, TRUE ))
    {
	if (g->opq_dbms.dbms_type & (OPQ_INGRES | OPQ_STAR | OPQ_RMSGW))
	{
	    /* 
	    ** If a dba user, add catalogs to the set
	    ** of tables under consideration.
	    */
	    if (g->opq_dbcaps.tblname_case == (i4)OPQ_LOWER)
	    {
		STcopy("$ingres", (char *)&g->opq_dba);
	    }
	    else
	    {
		STcopy("$INGRES", (char *)&g->opq_dba);
	    }
	}
	else
	{
	    /*
	    ** In case of non-Ingres DBMSs/GWs we do not
	    ** what to assume regarding catalog ownership.
	    ** Let's assume they are owned by DBA.
	    */
	    STcopy(es_dba, (char *)&g->opq_dba);
	}
    }
    else
    {
	/*
	** If a non-dba user, process his own tables only .
	*/
	g->opq_dba[0] = EOS;
    }
}

/*
** Name: opq_upd - check if iistats, iihistograms, iitables are updateable.
**
** Description:
**      Table type is retrieved from iitables for both objects. If type is
**	'T', then table are updateable. If not statistical table must
**	be updated directly, i.e., standard catalogs cannot be used.
**
** Inputs:
**	g			ptr to global data struct
**	  opq_dbcaps		database capabilities
**	    cat_updateable	catalogs updateable flag
**
** Outputs:
**	g			ptr to global data struct
**	  opq_dbcaps		database capabilities
**	    cat_updateable	catalogs updateable flag
**				(TRUE if iistats, iihistograms,
**				iitables are updateable)
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-dec-88 (stec)
**	    written.
**	28-dec-89 (stec)
**	    Modified. Uses appropraite case for object names.
**	    Sets updateability as a mask rather than boolean.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	06-jun-90 (stec)
**	    Fixed bug 20159. Problem was in table names returned from
**	    the query followed by white space. Comparison, therefore,
**	    did not work and STAR tables were marked as not updateable,
**	    which is wrong.
*/
VOID
opq_upd(
OPQ_GLOBAL *g)
{
    exec sql begin declare section;
	char	es_type[8 + 1];
	char	es_tblname[DB_TAB_MAXNAME + 1];
	char	iistats[DB_GW1_MAXNAME + 1];
	char	iihistograms[DB_GW1_MAXNAME + 1];
	char	iitables[DB_GW1_MAXNAME + 1];
    exec sql end declare section;

    g->opq_dbcaps.cat_updateable = OPQ_CATNOTUPD;

    if (g->opq_dbcaps.tblname_case == (i4)OPQ_LOWER)
    {
	STcopy("iistats",	iistats);
	STcopy("iihistograms",	iihistograms);
	STcopy("iitables",	iitables);
    }
    else
    {
	STcopy("IISTATS",	iistats);
	STcopy("IIHISTOGRAMS",	iihistograms);
	STcopy("IITABLES",	iitables);
    }

    /*
    ** Select distinct is used because certain
    ** dbms types return multiple entries.
    */
    exec sql select distinct table_name, table_type
	into :es_tblname, :es_type
	from iitables
  	where table_name in (:iistats, :iihistograms, :iitables);

    exec sql begin;

	if (es_type[0] != 'T' &&
	    es_type[0] != 'V'
	   )
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0934_BAD_SCAT_TYPE, (i4)2, 
		(i4)0, (PTR)&es_type[0]);
	    (*opq_exit)();
	}
	
	/* trim white space in table name so that comparisons work */
	(VOID)STtrmwhite(es_tblname);

	if (es_type[0] == 'T')
	{
	    if (STcompare(es_tblname, iistats) == 0)
		g->opq_dbcaps.cat_updateable |= OPQ_STATS_UPD;
	    else if (STcompare(es_tblname, iihistograms) == 0)
		g->opq_dbcaps.cat_updateable |= OPQ_HISTS_UPD;
	    else if (STcompare(es_tblname, iitables) == 0)
		g->opq_dbcaps.cat_updateable |= OPQ_TABLS_UPD;
	}

    exec sql end;

    return;
}

/*
** Name: opq_dbmsinfo - check if Ingres dbms and if Ingres SQL can be used.
**
** Description:
**      'INGRES/SQL_LEVEL', 'DBMS_TYPE', and 'STANDARD_CATALOG_LEVEL' tuples
**	are retrieved from the iidbcapabilities catalog and related flags are
**	checked for value. 
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_dbms	    OPQ_DBMS struct
**	  opq_lang	    OPQ_LANG struct
**	  opq_dbcaps	    struct showing where phys.table info is to
**			    be gotten from.
**
** Outputs:
**	g		    ptr to global data struct
**	  opq_dbms	    OPQ_DBMS struct
**	  opq_lang	    OPQ_LANG struct
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      12-dec-88 (stec)
**	    written.
**	22-jan-90 (stec)
**	    Added a lookup on INGRES_UDT.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	13-apr-90 (stec)
**	    Added a read for MAX_COLUMNS.
**	    This routine should be changed to issue one read, and while reading,
**	    look at interesting rows only.
**	24-jul-90 (stec)
**	    Added code for the RMS gateway.
**	    Changed the code to operate on cursor rather than have a separate
**	    statement for each capability of interest. This should improve
**	    performance.
**	14-dec-92 (rganski)
**	    Added read of STANDARD_CATALOG_LEVEL.
**	    If database type is INGRES, STANDARD_CATALOG_LEVEL is checked; if
**	    it does not match DU_DB6_CUR_STDCAT_LEVEL, abort with error.
**	    Changed check of dbms type to switch statement.
**	20-sep-93 (rganski)
**	    Delimited ID support. Added read and storage of DB_DELIMITED_CASE,
**	    and setting of new field cui_flags in opq_dbcaps.
**	2-nov-93 (robf)
**          Check TABLE_STATISTICS privilege.
**  21-Jan-05 (nansa02)
**       Added the CUI_ID_NORM bit to DB_DELIMITED_CASE for proper normalization in cui_idxlate.
**       (Bug 113786)  
**	28-Jan-2005 (schka24)
**	    Back out above, we're only setting proper case conversion here.
*/
VOID
opq_dbmsinfo (
OPQ_GLOBAL  *g)
{
# define OPQ_CQRY2 "select cap_capability, cap_value from iidbcapabilities"

    exec sql begin declare section;
	char	es_cap[DB_CAP_MAXLEN + 1];
	char	es_val[DB_CAPVAL_MAXLEN + 1];
	char    stmt[sizeof(OPQ_CQRY2)+ 1];
	char    es_tblstats[9];
    exec sql end declare section;

    IISQLDA	*sqlda = &_sqlda;
    bool	distributed_y = FALSE;

    g->opq_dbms.dbms_type = OPQ_DBMSUNKNOWN;
    g->opq_lang.ing_sqlvsn = OPQ_NOTSUPPORTED;
    g->opq_lang.com_sqlvsn = OPQ_NOTSUPPORTED;
    g->opq_dbcaps.phys_tbl_info_src = OPQ_U_SRC;
    g->opq_dbcaps.tblname_case = OPQ_NOCASE;
    g->opq_dbcaps.delim_case = OPQ_NOCASE;
    g->opq_dbcaps.cui_flags = 0;
    g->opq_dbcaps.udts_avail = OPQ_NOUDTS;
    g->opq_dbcaps.max_cols = DB_MAX_COLS;
    g->opq_dbcaps.std_cat_level = OPQ_NOTSUPPORTED;
    
    STprintf(stmt, OPQ_CQRY2);

    sqlda->sqln = 2;
    exec sql prepare s from :stmt;
    exec sql describe s into :sqlda;

    /* describe location for the output data */
    sqlda->sqlvar[0].sqldata = (char *)es_cap;
    sqlda->sqlvar[0].sqllen = sizeof(es_cap);
    sqlda->sqlvar[1].sqldata = (char *)es_val;
    sqlda->sqlvar[1].sqllen = sizeof(es_val);

    /* retrieve data for the constructed query. */
    exec sql open c1 for readonly;

    while (sqlca.sqlcode == GE_OK)
    {
	exec sql fetch c1 using descriptor :sqlda;

	if (sqlca.sqlcode == GE_NO_MORE_DATA)
	    break;

	(VOID)STtrmwhite(es_cap);
	(VOID)STtrmwhite(es_val);

	if (!STcompare(es_cap, "DBMS_TYPE"))
	{
	    if (!STcompare(es_val, "INGRES"))
	    {
		g->opq_dbms.dbms_type = OPQ_INGRES;
	    }
	    else if (!STcompare(es_val, "STAR"))
	    {
		g->opq_dbms.dbms_type = OPQ_STAR;
	    }
	    else if (!STcompare(es_val, "RMS"))
	    {
		g->opq_dbms.dbms_type = OPQ_RMSGW;
	    }
	    else
	    {
		g->opq_dbms.dbms_type = OPQ_GENDBMS;
	    }
	}
	else if (!STcompare(es_cap, "COMMON/SQL_LEVEL"))
	{
	    i4	    vsn;
	    STATUS  s;

	    if ((s = CVal(es_val, &vsn)) != OK)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0935_LVSN_CERR, (i4)4,
		    (i4)sizeof(STATUS), (PTR)&s,
		    (i4)0, (PTR)&es_val[0]);
		(*opq_exit)();
	    }

	    g->opq_lang.com_sqlvsn = vsn;	
	}	    
	else if (!STcompare(es_cap, "INGRES/SQL_LEVEL"))
	{
	    i4	    vsn;
	    STATUS  s;

	    if ((s = CVal(es_val, &vsn)) != OK)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0935_LVSN_CERR, (i4)4,
		    (i4)sizeof(STATUS), (PTR)&s,
		    (i4)0, (PTR)&es_val[0]);
		(*opq_exit)();
	    }

	    g->opq_lang.ing_sqlvsn = vsn;	
	}
	else if (!STcompare(es_cap, "PHYSICAL_SOURCE"))
	{
	    if (!STcompare(es_val, "T"))
	    {
		g->opq_dbcaps.phys_tbl_info_src = OPQ_T_SRC;
	    }
	    else if (!STcompare(es_val, "P"))
	    {
		g->opq_dbcaps.phys_tbl_info_src = OPQ_P_SRC;
	    }
	}
	else if (!STcompare(es_cap, "DB_NAME_CASE"))
	{
	    /* Set proper regular ID case translation */
	    if (!STcompare(es_val, "UPPER"))
	    {
		g->opq_dbcaps.tblname_case = OPQ_UPPER;
		g->opq_dbcaps.cui_flags |= CUI_ID_REG_U;
	    }
	    else if (!STcompare(es_val, "LOWER"))
	    {
		g->opq_dbcaps.tblname_case = OPQ_LOWER;
		g->opq_dbcaps.cui_flags |= CUI_ID_REG_L;
	    }
	    else if (!STcompare(es_val, "MIXED"))
	    {
		g->opq_dbcaps.tblname_case = OPQ_MIXED;
		g->opq_dbcaps.cui_flags |= CUI_ID_REG_M;
	    }
	}
	else if (!STcompare(es_cap, "DB_DELIMITED_CASE"))
	{
	    /* Set proper delimited ID case translation */
	    if (!STcompare(es_val, "UPPER"))
	    {
		g->opq_dbcaps.delim_case = OPQ_UPPER;
		g->opq_dbcaps.cui_flags |= CUI_ID_DLM_U;
	    }
	    else if (!STcompare(es_val, "LOWER"))
	    {
		g->opq_dbcaps.delim_case = OPQ_LOWER;
		g->opq_dbcaps.cui_flags |= CUI_ID_DLM_L;
	    }
	    else if (!STcompare(es_val, "MIXED"))
	    {
		g->opq_dbcaps.delim_case = OPQ_MIXED;
		g->opq_dbcaps.cui_flags |= CUI_ID_DLM_M;
	    }
	}
	else if (!STcompare(es_cap, "INGRES_UDT"))
	{
	    if (es_val[0] == 'Y')
	    {
		g->opq_dbcaps.udts_avail = OPQ_UDTS;
	    }
	    else if (es_val[0] == 'N')
	    {
		g->opq_dbcaps.udts_avail = OPQ_NOUDTS;
	    }
	}
	else if (!STcompare(es_cap, "MAX_COLUMNS"))
	{
	    STATUS	s;
	    i4	mc;

	    s = CVan((char *)&es_val[0], &mc);
	    if (s != OK)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0974_BAD_MAX_COLS, (i4)4,
		    (i4)sizeof(STATUS), (PTR)&s,
		    (i4)0, (PTR)&es_val[0]);
		(*opq_exit)();
	    }

	    g->opq_dbcaps.max_cols = mc;
	}
	else if (!STcompare(es_cap, "DISTRIBUTED"))
	{
	    if (!STcompare(es_val, "Y"))
	    {
		distributed_y  = TRUE;
	    }
	}
	else if (!STcompare(es_cap, "STANDARD_CATALOG_LEVEL"))
	{
	    i4	    vsn;
	    STATUS  s;

	    if ((s = CVal(es_val, &vsn)) != OK)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP092E_CVSN_CERR, (i4)4,
		    (i4)sizeof(STATUS), (PTR)&s,
		    (i4)0, (PTR)&es_val[0]);
		(*opq_exit)();
	    }

	    g->opq_dbcaps.std_cat_level = vsn;	
	}

    } /* end of fetch loop */

    exec sql close c1;

    /*
    ** Following checks test if values expected to be
    ** initialized based on the iidbcapabilities data
    ** have, indeed, been initialized; if not, then 
    ** default values are set. If dbms type has not been
    ** read, then catalog is inconsistent. For STAR,
    ** additional consistency check is done here.
    */
    switch (g->opq_dbms.dbms_type)
    {
    case OPQ_DBMSUNKNOWN:
	opq_error((DB_STATUS)E_DB_ERROR, (i4)E_OP0957_NO_CAPABILITY,
		  (i4)2, (i4)0, (PTR)"DBMS_TYPE");
	(*opq_exit)();
    case OPQ_STAR:
	if (distributed_y == FALSE)
	{
	    opq_error( (DB_STATUS)E_DB_ERROR, (i4)E_OP0959_CAP_ERR,
		      (i4)0);
	    (*opq_exit)();
	}
	break;
    case OPQ_INGRES:
	/* if STANDARD_CATALOG_LEVEL does not match DU_DB6_CUR_STDCAT_LEVEL,
	** abort with error. 
	*/
	{
	    i4	    vsn;

	    (VOID) CVal(DU_DB6_CUR_STDCAT_LEVEL, &vsn);
	    if (g->opq_dbcaps.std_cat_level != vsn)
	    {
		opq_error((DB_STATUS)E_DB_ERROR, (i4)E_OP092F_STD_CAT,
			  (i4)4, (i4)0, (PTR)g->opq_utilid,
			  (i4)0, (PTR)g->opq_dbname);
		(*opq_exit)();
	    }
	}
	break;
    default:
	break;
    }
	    
    if (g->opq_dbcaps.phys_tbl_info_src == OPQ_U_SRC)
    {
	/* set default */
        g->opq_dbcaps.phys_tbl_info_src = OPQ_T_SRC;
    }

    if (g->opq_dbcaps.tblname_case == OPQ_NOCASE)
    {
	/* set default */
	g->opq_dbcaps.tblname_case = OPQ_LOWER;
	g->opq_dbcaps.cui_flags |= CUI_ID_REG_L;
    }

    if (g->opq_dbcaps.delim_case == OPQ_NOCASE)
    {
	/* set default */
	g->opq_dbcaps.delim_case = OPQ_LOWER;
	g->opq_dbcaps.cui_flags |= CUI_ID_DLM_L;
    }

    /*
    ** Check table statistics, note default is to allow unless
    ** explicitly denied
    */
    g->opq_no_tblstats=FALSE;

    if(g->opq_dbms.dbms_type==OPQ_INGRES)
    {
	/*
	** Only INGRES DBMS supports table_statistics currently
	*/
    	exec sql select dbmsinfo('table_statistics')
		into :es_tblstats;

	if(*es_tblstats=='N' || *es_tblstats=='n')
		g->opq_no_tblstats=TRUE;
    }
    return;
}

/*
** Name: opq_translatetype - translate a data type into Ingres internal type
**
** Description:
**	Translates a data type from a system catalog to an Ingres internal 
**	data type, by calling adi_tyid. Sets sign appropriately, depending 
**	on whether the column is nullable.
**
** Inputs:
**	adfcb			pointer to ADF control block
**	es_type			type description from iicolumns
**	es_nulls		indicator of whether column is nullable (Y/N)
**
** Outputs:
**	attrp			attribute descriptor for column in question
**	   attr_dt.db_datatype	set to internal data type, translated from 
**				es_type    
**
**	Returns:
**	    VOID
**	Exceptions:
**	    If return status from adi_tyid indicates an error, prints error 
**	    message and aborts the program.
**
** Side Effects:
**	    none
**
** History:
**      09-nov-92 (rganski)
**	    Initial creation.
*/
VOID
opq_translatetype(
ADF_CB		*adfcb,
char		*es_type,
char		*es_nulls,
OPQ_ALIST 	*attrp)
{
    DB_STATUS   status;
    ADI_DT_NAME typename;
    DB_DT_ID 	datatype;

    STmove(es_type, EOS, DB_TYPE_MAXLEN, typename.adi_dtname);
    
    status = adi_tyid(adfcb, &typename, &datatype);
    
    if (DB_FAILURE_MACRO(status))
	opq_adferror(adfcb, (i4)E_OP0936_ADI_TYID, (i4)0, 
		     (PTR)&typename, (i4)0, (PTR)0);   
    
    if (es_nulls[0] == 'Y')
	attrp->attr_dt.db_datatype = -datatype;
    else
	attrp->attr_dt.db_datatype = datatype;

    return;
}

/*{
** Name: r_rel_list_from_rel_rel	- get list of all relation in DB
**
** Description:
**      This routine is called if the user has not specified any tables, which
**      indicates that all tables in the database need to have statistics
**	gathered or dumped.
**
**	This routine is called from opqoptdb.sc and opqstatd.sc
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_utilid	    name of utility this proc is called from
**        opq_dbname        ptr to database name
**        opq_owner         table owner name
**	  opq_dba	    dba name
**	  opq_cats	    "consider catalogs" flag
**	  opq_dbcaps	    struct showing where phys.table info is to be
**			    read from.
**	rellst		    ptr to array of ptrs to relation descriptors
**	ex_rellist	    ptr to array of ptrs to "-xr" tables
**	statdump	    Indicates whether the caller is statdump; i.e.
**			    we are only interested in tables with statistics.
**
** Outputs:
**      rellst          ptr to array of relation descriptors
**                      of all relations in the database (less those in 
**			-xr parameters)
**	Returns:
**	    TRUE - if all table descriptors successfully read
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-nov-86 (seputis)
**          initial creation
**	15-sep-88 (stec)
**	    Use sql and access standard catalogs.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	13-sep-89 (stec)
**	    Add initialization for samplecrtd field.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	09-nov-92 (rganski)
**	    Moved this function here from opqoptdb.sc and opqstatd.sc,
**	    since it is shared by both. Added parameter statdump to
**	    indicate whether we are only interested in tables with statistics.
**	    Added prototype, changed from static.
**	    Removed redundant predicates from WHERE clause of SELECT which
**	    determines if there are statistics for the tables; transitivity
**	    makes these predicates unnecessary.
**	20-sep-93 (rganski)
**	    Delimited ID support. Call opq_idunorm() to unnormalize table
**	    names, store in relp->delimname.
**      08-jun-99 (schang)
**          add code to retrieve tuple width so that an estimate of page number
**          can be calculated for RMS GW.
**      28-jun-2005 (huazh01)
**          change es_tabtype from char[2] to char[9]. On a distributed 
**          database, column 'table_type' on 'iitables' is not char(1) but
**          char(8). This fixes 109850, INGSTR54.
**	10-aug-05 (inkdo01)
**	    Preset statset flag using table_stats view column.
**	25-Aug-2005 (hweho01)
**	   Change es_tstat from char[2] to char[9], avoid "W_OP0961 Truncation
**         occurred" msg.
**	11-oct-05 (inkdo01)
**	    Remove es_tstat and defective changes to statset.
**	22-dec-06 (hayke02)
**	    Reinstate es_tstat and switch off 'set statistics' if the new "-zy"
**	    flag is set, and es_tstat/iitables.table_stats is 'Y' (indicating
**	    that 'set statistics' has already been run). This change implements
**	    implements SIR 117405.
**	12-feb-07 (kibro01) b117368
**	    IMA tables should never be listed for statistics, either in statdump
**	    or in optimizedb - they don't contain meaningfully-static data.
**	22-May-07 (kibro01) b118377
**	    Join between iitables and iirelation allowed multiple rows through
**	    which enabled a reoccurrence of a bug like b107881
**	15-feb-08 (wanfr01)
**	    Bug 119927
**	    SIR 114323 will result in iistatistic rows without corresponding
**	    iihistogram rows.  Set 'withstats' if no histogram exists.
**    02-sep-08 (hayke02)
**        Repair change for SIR 117405 so that we init and set statset
**        correctly.
**      10-Jun-2010 (hanal04) Bug 123898
**          Do not try to access iirelation. It does not exist in a Star
**          database. Use iitables.table_subtype <> 'I' to eliminate
**          gateway tables from the table list.
**      19-Aug-2010 (hanal04) Bug 124276
**          Fix for Bug 123898 excluded all Gateway tables. Rework fix
**          for Bug 123898 and 117368 to use different query text
**          if we are a STAR DB and use a cursor instead of a SELECT loop.
**	30-Nov-2010 (kschendel)
**	    Exclude new spatial catalogs, they don't start with ii.
**	    Fix remotecmdlock exclusion to work properly in an ANSI
**	    (uppercase name) installation.
*/
bool
r_rel_list_from_rel_rel(
OPQ_GLOBAL	*g,
OPQ_RLIST       *rellst[],
OPQ_RLIST       *ex_rellst[],
bool		statdump)
{
    exec sql begin declare section;
	char	*es_relname;
	i4	es_nrows;
	char	*es_ownr;
	char	*es_ownname;
	char    *es_dba;
	i4	es_reltid;
	i4	es_xreltid;
	i4	es_pages;
	i4	es_ovflow;
        i4      es_relwid;        /* schang: tuple width */
	char	es_tabtype[9];
	char	es_tstat[8 + 1];
        char    stmtbuf[2048];
        char    gwid[5];
    exec sql end declare section;

    i4		ri = 0;
    i4		i;
    bool	overrun = FALSE;
    bool	exclude = (ex_rellst != NULL);
    DB_TAB_STR	trelname;
    DB_OWN_STR  townname;

    es_ownr = (char *)&g->opq_owner;
    es_dba = (char *)&g->opq_dba;
    es_ownname = (char *)&townname;
    es_relname = (char *)&trelname;


    /* In STAR, for now, iistats and iihistograms are treated as user
    ** tables and do not get sent across privileged connection; all other
    ** catalogs get rerouted, to avoid deadlocks we must not mix the two
    ** catalogs with other catalogs.
    */
    /* b117368 (kibro01)
    ** Any table which is a registered IMA gateway should not be selected.
    ** Also, remotecmdlock is always held in an exclusive lock despite
    ** being created as a normal table.  If it is selected here the
    ** optimizedb will wait for the permanent lock and will never complete.
    ** See rmcmd/rmcmd.sc for info on remotecmdlock
    ** b118377 (kibro01)
    ** Adjust above change to ensure the link between iirelation and iitables
    ** cannot produce spurious extra rows, leading to reoccurrence of a bug
    ** with the same symptoms as b107881.
    */
    if(g->opq_dbms.dbms_type == OPQ_STAR)
    {
        /* ESQL precompiler does not like lines beyond a certain length   */
        /* Parameters in this query must match those in the non-star case */
        STprintf(stmtbuf, "select t.table_name, t.table_owner, t.num_rows, ");
        STpolycat(4, stmtbuf, "t.table_reltid, t.table_reltidx, ",
                              "t.number_pages, t.overflow_pages, ",
                              "t.row_width, t.table_type, t.table_stats ",
                              stmtbuf);
        STcat(stmtbuf, "from iitables t where t.table_owner in (?, ?) ");
	if (!g->opq_cats)
	{
	    STcat(stmtbuf, "and t.system_use = 'U' ");
	}
        STpolycat(3, stmtbuf, "and t.table_type in ('T', 'I') ",
			      "and t.table_subtype <> 'I' and ",
                              stmtbuf);
	/* Not sure if all Star LDB's can do "lowercase()" ?? */
        STpolycat(4, stmtbuf, "(t.table_name <> 'remotecmdlock' or ",
                              "t.table_owner <> '$ingres') order by ",
                              "t.table_name",
                              stmtbuf);
    }
    else
    {
        /* ESQL precompiler does not like lines beyond a certain length   */
        /* Parameters in this query must match those in the star case     */
        STprintf(gwid, "%d ", GW_IMA);
        STprintf(stmtbuf, "select t.table_name, t.table_owner, t.num_rows, ");
        STpolycat(4, stmtbuf, "t.table_reltid, t.table_reltidx, ",
                              "t.number_pages, t.overflow_pages, ",
                              "t.row_width, t.table_type, t.table_stats ",
                              stmtbuf);
        STpolycat(3, stmtbuf, "from iitables t, iirelation i where ",
                              "t.table_owner in (?, ?) ",
                              stmtbuf);
	if (!g->opq_cats)
	{
	    STcat(stmtbuf, "and t.system_use = 'U' ");
	}
	STpolycat(5, stmtbuf, "and t.table_type in ('T', 'I') ",
			      "and t.table_reltid = i.reltid and ",
                              "t.table_name = i.relid and i.relgwid <> ",
                              gwid,
                              stmtbuf);
        STpolycat(4, stmtbuf, "and (lowercase(t.table_name) <> 'remotecmdlock' or ",
                              "lowercase(t.table_owner) <> '$ingres') order by ",
                              "t.table_name",
                              stmtbuf);

    }

    EXEC SQL prepare stmt from :stmtbuf;
    EXEC SQL declare cs cursor for stmt;
    EXEC SQL open cs for readonly using :es_ownr, :es_dba;

    while(1)
    {
        EXEC SQL fetch cs into :es_relname, :es_ownname, :es_nrows,
             :es_reltid, :es_xreltid, :es_pages,
             :es_ovflow, :es_relwid, :es_tabtype,
             :es_tstat;
        if(sqlca.sqlcode)
            break; 

	/* First check for "-xr" list and skip the OPQ_RLIST build
	** if current table is in exclusion list. */
	if (ex_rellst != NULL)
	{
	    exclude = FALSE;
	    for (i = 0, exclude = FALSE; !exclude && ex_rellst[i]; i++)
	    {
		OPQ_RLIST   *rp;
		DB_TAB_STR	tabname;
		DB_OWN_STR	ownname;
		rp = ex_rellst[i];
		tabname[DB_TAB_MAXNAME] = 0;
		ownname[DB_OWN_MAXNAME] = 0;
		MEcopy(es_relname, sizeof(DB_TAB_NAME), (char *)&tabname);
		MEcopy(es_ownname, sizeof(DB_OWN_NAME), (char *)&ownname);
		(VOID) STtrmwhite((char *) &tabname);
		(VOID) STtrmwhite((char *) &ownname);
		if (STcasecmp((char *)&ownname, (char *)&rp->ownname))
			continue;
		if (STcasecmp((char *)&tabname, (char *)&rp->relname))
			continue;
		exclude = TRUE;		/* match - exit loop & skip table */
	    }
	}

	if (!exclude && !overrun && (statdump || es_tabtype[0] != 'I'))
	{
	    OPQ_RLIST   *rp;

	    if (ri >= g->opq_maxrel)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP090F_TABLES, (i4)6, 
		    (i4)0, (PTR)g->opq_utilid,
		    (i4)sizeof(g->opq_maxrel), (PTR)&g->opq_maxrel, 
		    (i4)0, (PTR)g->opq_dbname);
		/* %s: more than %d tables for database  '%s'
		*/
		overrun = TRUE;
		break;
	    }
	    else
	    {
		rp = (OPQ_RLIST *) getspace((char *)g->opq_utilid,
		    (PTR *)&rellst[ri], sizeof(OPQ_RLIST));

		/* Trim white space and save table info */
		(VOID) STtrmwhite((char *) &trelname);
		(VOID) STtrmwhite((char *) &townname);
		STcopy((char *) &trelname, (char *)&rp->relname);
		/* Unnormalize table name, put result in rp->delimname */
		opq_idunorm((char *)&rp->relname, (char *)&rp->delimname);
		STcopy((char *) &rp->delimname, (char *)&rp->argname);
		rp->samplename[0] = EOS;
		STcopy((char *) &townname, (char *)&rp->ownname);
		rp->nsample = (i4)0;
		rp->reltid  = es_reltid;
		rp->reltidx = es_xreltid;
		rp->pages   = es_pages;
		rp->overflow= es_ovflow;
                rp->relwid  = es_relwid;
		rp->ntups   = es_nrows;
		rp->attcount= 0;          /* count of interesting attributes */
                rp->statset = (bool)FALSE;
		if ((es_tstat[0] == 'Y')	/* need to "set statistics" */
		    &&
		    (nosetstats))
                        rp->statset = (bool)TRUE; 
		rp->sampleok= (bool)TRUE;   /* assume sampling ok */
		rp->physinfo= (bool)TRUE;   /* assume phys info retrieved */
		rp->withstats = (bool)FALSE; /* assume stats not present */
		rp->samplecrtd = (bool)FALSE;/* assume sample tbl not created */
		rp->comphist = (bool)FALSE; /* assume no composite histogram */

		ri++;

# ifdef xDEBUG
		if (opq_xdebug)
		    SIprintf("\nAdded relation '%s'\n", &rp->relname);
# endif /* xDEBUG */
	    }
	}
    }

    EXEC SQL close cs;

    /* Indicate the end of list */
    if (!overrun)
	rellst[ri] = NULL;

    /* For statdump, determine which tables have stats. */
    if (statdump)
    {
	register OPQ_RLIST   **rpp;

	for (rpp = &rellst[0]; *rpp; rpp++)
	{
	    exec sql begin declare section;
		i4	es_num;
	    exec sql end declare section;

	    es_relname = (char *)&(*rpp)->relname;
	    es_ownname = (char *)&(*rpp)->ownname;

	    exec sql repeated select first 1 1
		into :es_num
		from iistats s
		where	s.table_owner = :es_ownname and
			s.table_name  = :es_relname;

	    exec sql begin;
		(*rpp)->withstats = (bool)TRUE;
	    exec sql end;
	}
    }

    /* Finish filling physical table info if necessary */
    if (g->opq_dbcaps.phys_tbl_info_src != (i4)OPQ_T_SRC)
    {
	register OPQ_RLIST   **rpp;

	for (rpp = &rellst[0]; *rpp; rpp++)
	{
	    if (!statdump ||
		(*rpp)->withstats == (bool)TRUE)
		opq_phys(*rpp);

	    /*
	    ** Check whether number of rows is meaningful,
	    ** count if necessary
	    */
	    if ((*rpp)->ntups <= 0)
	    {
		opq_cntrows(*rpp);
	    }

	    if ((*rpp)->ntups <= 0)
	    {
		(*rpp)->physinfo = (bool)FALSE;
	    }
	}
    }

    return(!overrun);
}

/*{
** Name: opt_usage	- print message describing syntax of OPTIMIZEDB command
**
** Description:
**      print message about OPTIMIZEDB usage
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**	    Prints message to user about optimizedb syntax.
**
** History:
**      4-dec-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	13-sep-89 (stec)
**	    In usage message display '-zn#' instead of '-zf#' (float prec).
**	22-jan-91 (stec)
**	    Added "zw" flag.
**	12-feb-92(rog)
**	    Change OPQ_ESTATUS to FAIL.
**	21-dec-99 (inkdo01)
**	    Update with new command line options (-zlr -o filename).
**	28-may-01 (inkdo01)
**	    Add more new options - -zcpk, -zdn, -zfq, -znt.
**	17-july-01 (inkdo01)
**	    Add "-xr"
**	26-july-10 (toumi01) BUG 124136
**	    Add "-ze"
*/
VOID
opt_usage(VOID)
{
    if (opq_global.opq_dbms.dbms_type & OPQ_RMSGW)
    {
        SIprintf("usage:     optimizedb [-zf filename]\n");
        SIprintf("       OR\n");
        SIprintf("           optimizedb [-zv] [-zh] [-zk] [-zx]\n");
        SIprintf("           [-zu#] [-zr#] [-zn#]\n");
        SIprintf("           [-zc] [-zp] [-zw] [-i filename]\n");
        SIprintf("           [ingres flags] dbname/RMS\n");
        SIprintf("	         {-rtablename {-acolumnname}} |\n");
        SIprintf("	         {-xrtablename}\n");
    }
    else
    {
        SIprintf("usage:     optimizedb [-zf filename]\n");
        SIprintf("       OR\n");
        SIprintf("           optimizedb [-i filename] [-o filename]\n");
        SIprintf("           [-zv] [-zfq] [-zh] [-zk] [-zlr] [-zx]\n");
        SIprintf("           [-zdn] [-zcpk] [-zu#] [-zr#] [-zn#] [-zs#]\n");
        SIprintf("           [-znt] [-zc] [-zp] [-zw] [-ze]\n");
        SIprintf("           [ingres flags] dbname\n");
        SIprintf("	         {-rtablename {-acolumnname}} |\n");
        SIprintf("	         {-xrtablename}\n");
    }

    (VOID) EXdelete();
    PCexit((i4)FAIL);
}
