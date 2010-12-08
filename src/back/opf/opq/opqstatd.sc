/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <cs.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <ddb.h>
# include    <ulm.h>
# include    <ulf.h>
# include    <adf.h>
# include    <dmf.h>
# include    <dmtcb.h>
# include    <scf.h>
# include    <qsf.h>
# include    <qefrcb.h>
# include    <rdf.h>
# include    <psfparse.h>
# include    <qefnode.h>
# include    <qefact.h>
# include    <qefqp.h>
# include    <me.h>
# include    <cm.h>
# include    <si.h>
# include    <st.h>
# include    <pc.h>
# include    <lo.h>
# include    <ex.h>
# include    <er.h>
# include    <cv.h>
# include    <generr.h>
/* beginning of optimizer header files */
# include    <opglobal.h>
# include    <opdstrib.h>
# include    <opfcb.h>
# include    <ophisto.h>
# include    <opq.h>

exec sql include sqlca;
exec sql include sqlda;

/*
**
**  Name: OPQSTATD.SC - program to print statistics in system catalogs
**
**  Description:
**      Utility to print statistics used by the optimizer
**      FIXME - optimal size of element in iihistogram relation is
**              (tuple_2008+tid_4)/8 = 251 = 12+tid_4+data_235 
**              so choose the SQL datatype to store binary data
**              of varchar 232 (with a 2 byte size), when equel varchar
**              is ready, use i4 for now.  Need a DB_DATA_VALUE interface
**              so null termination is done.
**
**
**      History:
**	7/9/82 (kooi)
**	    written (original version).
**      15-jan-86 (seputis)
**          initial creation from 5.0 statdump
**	12-Aug-88 (anton)
**	    added MEadvise
**	15-sep-88 (stec)
**	    Use standard cats to get relation, attribute names.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	    Add output statistics to a file feature.
**	08-jul-89 (stec)
**	    Merged into terminator path (changed ERlookup call).
**	10-jul-89 (stec)
**	    Added user length, scale to column info displayed.
**	13-sep-89 (stec)
**	    Add initialization for samplecrtd field.
**	    In usage message display '-zn#' instead of '-zf#' (float prec).
**	12-dec-89 (stec)
**	    Add generic error support. Changed utilid to be a global ref.
**	    Moved exception handler to opqutils.sc. Improved error handling
**	    and added error handling for serialization errors (TXs will be
**	    automatically restarted. Made code case sensitive with respect to
**	    database object names. Will count rows when count not available.
**	    Improved exception handling.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	05-apr-90 (stec)
**	    Removed automatic initialization of range static variable for
**	    hp3_us5 (HP 9000/320) C compiler.
**	10-apr-90 (stec)
**	    Allocates and initializes relation and attribute arrays.
**	16-may-90 (stec)
**	    Included cv.h, moved FUNC_EXTERNs to opq.h.
**	27-jun-90 (stec)
**	    Changed call to opq_error() for E_OP0927 in process().
**	09-jun-90 (stec)
**	    Removed automatic initialization of range static variable for
**	    mac_us5 C compiler.
**	24-jul-90 (stec)
**	    Added checks in the code to treat the RMS gateway pretty much
**	    the same way as Ingres.
**	02-aug-90 (stec)
**	    Modified att_list() to adjust case of the argv value.
**	06-nov-90 (stec)
**	    Changed code in att_list() to process correctly 
**	    attribute lists on the command line.
**	10-jan-91 (stec)
**	    Changed main().
**	23-jan-91 (stec)
**	    Modified code in process(), main().
**	28-feb-91 (stec)
**	    Modified code using TEXT6, TEXT7 strings. The output version
**	    of these strings, i.e., one with an "O" at the end will be used
**	    for formatting a format string, such that thw width and precision
**	    will be specified for floats like the null count and the normalized
**	    cell count. This addresses some issues raised by the bug report
**	    36152, which was marked NOT A BUG. To see the changes search this 
**	    modile for strings TEXT6, TEXT7.
**	06-apr-92 (rganski)
**	    Changed definition of rellist to (OPQ_RLIST **) and definition
**	    of attlist to (OPQ_ALIST **). These variables are pointers to 
**	    pointers, but they were not declared that way.
**	09-nov-92 (rganski)
**	    Added prototypes.
**	    Moved r_rel_list_from_rel_rel() to opqutils.sc, since it is useed 
**	    by both this opqstatd.sc and opqoptdb.sc. Added extra parameter to 
**	    indicate whether the caller is statdump.
**	    Commented out text after #endif's.
**	16-nov-92 (dianeh)
**	    Added CUFLIB to the NEEDLIBS line.
**	18-jan-93 (rganski)
**	    Added bound_length parameter (set to 0) to calls to setattinfo().
**	    Removed global variables count[] and range[], which are now local
**	    variables (pointers only) in process().
**	    Cosmetic changes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (dianeh)
**	    Added #include <st.h> for STtrmwhite (IISTtrmwhite).
**	26-aug-93 (dianeh)
**	    Back out previous change -- didn't need it afterall.
**	15-sep-93 (swm)
**	    Moved cs.h include above other header files which need its
**	    definition of CS_SID.
**	9-jan-96 (inkdo01)
**	    Added support of per-cell repetition factors.
**	06-mar-96 (nanpr01)
**	    Added order by column_sequence so that it retrives the columns
**	    in order.
**	13-sep-96 (nick)
**	    We are an ingres tool so tell EX so.
**	23-sep-1996 (canor01)
**	    Move global data definitions to opqdata.sc.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**	22-mar-2005 (thaju02)
**	    Changes for support of "+w" SQL option.
**    05-apr-05 (mutma03)
**        Added -xrtablename option for statdump to exclude table for
**        statistics.
**	27-june-05 (inkdo01)
**	    Several instances of "select 1 from ..." were changed to 
**	    "select first 1 1 from ..." to make 'em go faster. Also 
**	    changed a few queries on the iistats and iihistograms OJ views 
**	    to queries on the underlying catalog tables (minus the OJ)
**	    to make them go immensely faster. Fixes bug 114736.
**      28-sep-2009 (joea)
**          Add restriction on attver_dropped to select from iihistogram in
**          process().
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
static void stat_opq_exit(void);
void opx_error(
	i4 errcode);
void stat_usage(void);
static bool att_list(
	OPQ_GLOBAL *g,
	OPQ_ALIST *attlst[],
	OPQ_RLIST *relp,
	char **argv);
static void process(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST *attrp,
	FILE *outf);
int main(
	int argc,
	char *argv[]);

/*
**    UNIX mkming hints.
**
NEEDLIBS =	OPFLIB SHQLIB ULFLIB COMPATLIB 
**
NEEDLIBSW = 	SHEMBEDLIB SHADFLIB
**
PROGRAM =	statdump
**
OWNER =		INGUSER
**
*/

/*
** Definition of all global variables owned by this file.
*/
GLOBALREF	OPQ_RLIST	**rellist;
/* pointer to an array of pointers to relation descriptors - one for each 
** relation to be processed by STATDUMP
*/
GLOBALREF	OPQ_ALIST	**attlist;
/* pointer to an array of pointers to attribute descriptors - used within 
** the context of one relation, i.e. one attribute descriptor for each 
** attribute of the relation currently being processed
*/
GLOBALREF	bool		quiet;
/* -zq Quiet mode.  Print out only the information contained in the 
** iistatistics table and not the histogram information contained in the
** iihistogram table
*/
GLOBALREF	bool		supress_nostats;
/* -zqq Supress mode.  Do not print out warning messages for tables with
** no statistics.
*/
GLOBALREF	bool		deleteflag;
/* -zdl Delete statistics from the system catalogs.  When this flag is
** included, the statistics for the specified tables and columns (if any
** are specified) are deleted rather than displayed
*/

GLOBALREF	IISQLDA		_sqlda;		
/* SQLDA area for all dynamically built queries 
** (needed for a routine in OPQUTILS)
*/
GLOBALREF	OPQ_GLOBAL	opq_global;
/* global data struct
*/
GLOBALREF	i4		opq_retry;
/* serialization error retries count.
*/
GLOBALREF	i4		opq_excstat;
/* additional info about exception processed.
*/
# ifdef xDEBUG
GLOBALREF	bool		opq_xdebug;
/* debugging only
*/
# endif /* xDEBUG */
GLOBALREF       VOID (*opq_exit)(VOID);
GLOBALREF       VOID (*usage)(VOID);



/*{
** Name: stat_opq_exit() -	exit handler for optimizer utilities
**
** Description:
**      This is the exit handling routine. Current transaction is rolled back
**	open database it closed and so is the open file. Exception handler
**	is deleted.
**
** Inputs:
**	None.
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
**	None.
**	    
** History:
**      25-Nov-86 (seputis)
**          Initial creation.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	12-feb-92 (rog)
**	    Changed call to PCexit() to FAIL instead of OK.
**	9-june-99 (inkdo01)
**	    Change to support ifid and ofid for optdb.
*/
static
VOID
stat_opq_exit(VOID)
{
    if (opq_global.opq_env.dbopen)
    {
	/* Rollback will close all open cursors. */
	exec sql rollback;
	exec sql disconnect;
	opq_global.opq_env.dbopen = FALSE;
    }

    if (opq_global.opq_env.ofid)
    {
	(VOID)SIclose(opq_global.opq_env.ofid);
	opq_global.opq_env.ofid = (FILE *)NULL;
    }

    (VOID) EXdelete();
    PCexit((i4)FAIL);
    /*NOTREACHED*/
}

/*{
** Name: opx_error	- error reporting routine
**
** Description:
**      This routine is an error reporting routine which matches the name 
**      of the dbms optimizer routine, so that opf.olb can be used without 
**      bringing in references to SCF etc.
**
** Inputs:
**	errcode		    error code to be reported
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
**	Stops program execution after rolling back work and disconnecting
**	from the DBMS session, closes file if it's open.
**
** History:
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
*/
VOID
opx_error(
i4	    errcode)
{
    opq_error(E_DB_ERROR, errcode, (i4)0);

    (*opq_exit)();
}

/*{
** Name: stat_usage	- print message describing syntax of STATDUMP command
**
** Description:
**      print message about STATDUMP usage
**
** Inputs:
**	none
**
** Outputs:
**	none
**
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**	    Prints message to user about statdump syntax.
**
** History:
**      4-dec-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	13-sep-89 (stec)
**	    In usage message display '-zn#' instead of '-zf#' (float prec).
**	12-feb-92 (rog)
**	    Changed call to PCexit() to FAIL instead of OPQ_ESTATUS.
**	28-may-01 (inkdo01)
**	    Add -zcpk option (to display base table structure composite hist.)
**    05-apr-05 (mutma03)
**        Added -xrtablename option to exclude table
*/
VOID
stat_usage(VOID)
{
    SIprintf("usage: statdump [-zf filename]\n");
    SIprintf("	  OR\n");
    SIprintf("       statdump [-zcpk] [-zq] [-zdl] [-zn#]\n");
    SIprintf("       [-zc] [-o filename]\n");
    SIprintf("       [ingres flags] dbname\n");
    SIprintf("             {-rtablename {-acolumnname}} |\n");
    SIprintf("             {-xrtablename}\n");

    (VOID) EXdelete();
    PCexit((i4)FAIL);
}

exec sql whenever sqlerror call opq_sqlerror;
exec sql whenever sqlwarning call opq_sqlerror;

/*
 * {
** Name: att_list	- init attribute descriptors for a table
**
** Description:
**      Routine will read in and initialize attribute descriptors for a table
**
** Inputs:
**	g		ptr to global data struct
**	  opq_utilid	name of utility from which this proc is called from
**        opq_dbname    name of data base
**        opq_adfcb	ptr to ADF control block
**	attlst		ptr to array of ptrs to attribute descriptors
**	relp		ptr to relation descriptor to be
**                      analyzed
**      argv            input parameter names which may contain
**                      attribute names (if not then all
**                      attribute for table will be selected
**
** Outputs:
**      attlist         ptr to list of attributes descriptors
**                      for the table
**	Returns:
**	    TRUE if successful
**	Exceptions:
**	    none
**
** Side Effects:
**	    Allocates dynamic memory
**
** History:
**	16-sep-88 (stec)
**	    Modified to use sql and standard catalogs.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	10-jul-89 (stec)
**	    Added user length to column info displayed.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	02-aug-90 (stec)
**	    Adjust, as necessary, the case of the relation name specified
**	    on the command line with the "-r" flag, otherwise comparisons are
**	    not going to work.
**	06-nov-90 (stec)
**	    Changed code to initialize argv_temp local var. correctly. Incorrect
**	    initialization resulted in not processing of command line attribute
**	    lists for all listed relations with the exception of the first one.
**	09-nov-92 (rganski)
**	    Replaced code for translating data types with calls to new function
**	    opq_translatetype(), which is in opqutils.sc. Removed local
**	    variables dtype, which were only used in the replaced code.
**	20-sep-93 (rganski)
**	    Delimited ID support. Compare table name arguments with
**	    relp->delimname, which is as the user specified it, instead of
**	    relp->relname, which may be normalized and case adjusted. This
**	    makes argv_temp and case translation unnecessary.
**	    Changed all calls to opq_error to use relp->relname, for
**	    consistency. Got rid of relname local variable.
**	    Normalize attribute names in command line for catalog lookups,
**	    unnormalize attribute names found in catalog but not in command
**	    line.
**	06-mar-96 (nanpr01)
**	    Added order by column_sequence so that it retrives the columns
**	    in order.
**	2-nov-00 (inkdo01)
**	    Changes to support display of composite histograms.
**	27-apr-01 (inkdo01)
**	    Support of statdump -zcpk option to explicitly request display 
**	    of composite histogram on base table key structure.
**	07-sep-2001 (thaju02)
**	    If non-histogrammable column (eg. BLOB) is found in non-column
**	    specific search, skip over it. If found when explicitly requested
**	    with "-a" flag, give E_OP0985 error. (B105719)
**	21-Mar-2005 (schka24)
**	    Table name as specified on the command line is in argname.
**	27-june-05 (inkdo01)
**	    Changed iistats query to look for histogrammed columns to
**	    use the underlying catalog tables (sans OJ) to make it go
**	    way faster - composite histos are processed separately anyway.
*/
static bool
att_list(
OPQ_GLOBAL	    *g,
OPQ_ALIST	    *attlst[],
OPQ_RLIST	    *relp,
char		    **argv)
{
    i4		ai;		/* number of attribute descriptors defined
				** so far */
    bool	overrun;	/* TRUE if more attributes defined than
				** space is available */
    OPQ_ALIST	*ap;            /* descriptor of current attribute being
                                ** analyzed */
    i4		typeflags;	/* datatype attributes indicator to determine
				** histogrammability of column */
    bool	dummy;

    ai = 0;

    {
	i4	   parmno;              /* current argv parameter number
                                        ** being analyzed */
	bool	   att_from_input;	/* TRUE - if attributes specs are
				        ** obtained from the input file
                                        ** FALSE - no attributes specified
                                        ** so that all attributes need to
                                        ** be read implicitly from sys catalogs
                                        */
	bool	   comppkey = FALSE;

	att_from_input = FALSE;

	for( parmno = 0;
	    argv[parmno] && !CMalpha(&argv[parmno][0]);
	    argv[parmno]? parmno++: 0)  /* increment parmno only if the end
					** of the list has not been reached
					*/
	{   /* go till dbname or null */

	    if (isrelation(argv[parmno], &dummy))
	    {
		if (STcompare((char *)&relp->argname,
			      (char *)&argv[parmno][2]))
		    continue;
		if (g->opq_comppkey)
		    comppkey = TRUE;

	        /* target relation found, so search the list of parameters
                ** immediately following this to get attributes associated
                ** with this relation
                */
		while (comppkey || isattribute(argv[++parmno]))
		{   /* for each attribute */
		    i4	    	attcount;   /* number of tuples associated
                                            ** with attribute 
                                            */
		    DB_ATT_STR	atnameptr;  /* Attribute name being analyzed */
		    bool	ishistogrammable = TRUE;

		    att_from_input = TRUE;  /* at least one attribute specified
					    */
		    if (ai >= g->opq_dbcaps.max_cols)
		    {
			opq_error( (DB_STATUS)E_DB_ERROR,
			    (i4)E_OP0913_ATTRIBUTES, (i4)8, 
                            (i4)0, (PTR)g->opq_utilid,
			    (i4)0, (PTR)g->opq_dbname, 
			    (i4)0, (PTR)&relp->relname, 
			    (i4)sizeof(g->opq_dbcaps.max_cols),
				(PTR)&g->opq_dbcaps.max_cols);
			/* %s: database %s, table %s, more than %d 
			**       columns defined
			*/
			return (FALSE);
		    }

		    if (comppkey)
		    {
			/* -zpk option says look for composite, too. */
			exec sql begin declare section;
			    i4	    es_num;
			    char    es_nulls[8+1];
			    char    *es_atnameptr;
			    char    *es_rname;
			    char    *es_rowner;
			exec sql end declare section;

			comppkey = FALSE;	/* reset for rest of atts */

			es_rname = (char *)&relp->relname;
			es_rowner = (char *)&relp->ownname;
			es_atnameptr = (char *)&"IICOMPOSITE";
			es_num = 0;

			exec sql repeated select first 1 1
	        	    into :es_num
	        	    from iistats s
			    where s.table_name  = :es_rname and
				s.table_owner = :es_rowner and
				s.column_name = :es_atnameptr;

			if (es_num != 1) 
			    continue;		/* no composite here */

			relp->comphist = TRUE;

			/* Make fake attr descriptor for the composite histogram. */
			ap = (OPQ_ALIST *) getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));
			attlst[ai++] = ap;
			attlst[ai] = NULL;

			STcopy("IICOMPOSITE", (char *)&ap->attname);
			/* Unnormalize attribute name, put result in ap->delimname
			*/
			opq_idunorm((char *)&ap->attname, (char *)&ap->delimname);  

			ap->hist_dt.db_datatype = DB_CHA_TYPE;
			ap->attr_dt.db_length = 0;
			ap->attrno.db_att_id = -1;
			STcopy("character", (char *) &ap->typename);
			ap->nullable ='N'; 
			ap->scale = 0;
			ap->userlen = 0;
			ap->withstats = (bool)TRUE;
			ap->comphist = (bool)TRUE;

			/* Translate data type into Ingres internal type */
			es_nulls[0] = 'N';
			opq_translatetype(g->opq_adfcb, "character", es_nulls, ap);
			continue;
	    	    }

		    /* Must have a REAL attribute parm. Process it here. */
		    ap = (OPQ_ALIST *)getspace((char *)g->opq_utilid,
			(PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));
		    attcount = 0;	    /* number of tuples read */

  		    {
			exec sql begin declare section;
			    char	*es_attname;
			    char        es_type[DB_TYPE_MAXLEN+1];
			    char        es_nulls[8+1];
			    i4          es_len;
			    i4          es_scale;
			    i4          es_attrno;
			    i2		es_ingtype;
			    char        *es_atnameptr;
			    char	*es_rname;
			    char	*es_rowner;
			exec sql end declare section;
			char argname[DB_GW1_MAXNAME+3];

			/* Copy attribute name as found on command line */
			STcopy((char *)&argv[parmno][2], &argname[0]);
			es_rname = (char *)&relp->relname;
			es_rowner = (char *)&relp->ownname;
                        es_atnameptr = (char *)&atnameptr;
			es_attname = (char *)&ap->attname;

			/* Perform necessary translations on column name */
			opq_idxlate(g, argname, (char *)&atnameptr,
				    (char *) &ap->delimname);

			/* In STAR, for now, iistats and iihistograms are
			** treated as user tables and do not get sent across
			** privileged connection; all other catalogs get
			** rerouted, to avoid deadlocks we must not mix the two
			** catalogs with other catalogs.
			*/
			exec sql repeated select c.column_name,
			    c.column_datatype, c.column_length, c.column_scale,
			    c.column_nulls, c.column_sequence, 
			    c.column_internal_ingtype
			    into :es_attname, :es_type, :es_len,
				 :es_scale, :es_nulls, :es_attrno, 
				 :es_ingtype
			    from iicolumns c
			    where c.table_name  = :es_rname and
				  c.table_owner = :es_rowner and
				  c.column_name = :es_atnameptr
			    order by c.column_sequence;

			exec sql begin;

			    if (adi_dtinfo(g->opq_adfcb, es_ingtype,
				&typeflags) || (typeflags & AD_NOHISTOGRAM))
			    {
				(VOID) STtrmwhite((char *)&ap->attname);
				(VOID) STtrmwhite((char *)&es_type);
				opq_error((DB_STATUS)E_DB_ERROR,
                                        (i4)E_OP0985_NOHIST, (i4)4,
                                        (i4)0, (PTR)&ap->attname,
                                        (i4)0, (PTR)&es_type);
				ishistogrammable = FALSE;
			    }
			    else
			    {
			    /*
			    ** Trim trailing white space, convert
			    ** type to lower case.
			    */
			    (VOID) STtrmwhite(es_attname);
			    (VOID) STtrmwhite(es_type);
			    CVlower((char *)es_type);

                            ap->attr_dt.db_length = es_len;
			    ap->attrno.db_att_id = es_attrno;
			    STcopy((char *) es_type, (char *) &ap->typename);
			    ap->nullable = es_nulls[0];
			    ap->scale = (i2)es_scale;
			    ap->userlen = es_len;
			    ap->withstats = (bool)FALSE;/*assume no stats */
			    ap->comphist = (bool)FALSE;

			    /* Translate data type into Ingres internal type */
			    opq_translatetype(g->opq_adfcb, es_type,
					      es_nulls, ap);

			    attcount++;
			    }
			exec sql end;
  		    }

		    if (attcount > 1)
		    {
			opq_error( (DB_STATUS)E_DB_ERROR,
			    (i4)E_OP0914_ATTRTUPLES, (i4)8,
                            (i4)0, (PTR)g->opq_utilid,
			    (i4)0, (PTR)g->opq_dbname, 
			    (i4)0, (PTR)&relp->relname, 
			    (i4)0, (PTR)&atnameptr);
			/* statdump: database %s, table %s, column %s
			**       inconsistent sys catalog
			*/
			(*opq_exit)();
		    }
		    else if (attcount == 0)
		    {
			if (ishistogrammable == TRUE)
			{
			    opq_error( (DB_STATUS)E_DB_ERROR,
				(i4)W_OP0925_NOSTATS, (i4)6, 
				(i4)0, (PTR)g->opq_dbname, 
				(i4)0, (PTR)&relp->relname, 
				(i4)0, (PTR)&atnameptr);
			    /* statdump: database %s, table %s, column %s
			    **       statistics not found
			    */
			    attlst[ai] = NULL; /* deallocate slot for attribute */
			}
		    }
		    else
		    {
			setattinfo(g, attlst, &ai, (OPS_DTLENGTH)0);

# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}

		if (att_from_input)
		{
		    attlst[ai] = NULL;
		}

		/* Determine which columns have stats. */
		{
		    exec sql begin declare section;
			i4	es_num;
			char    *es_atnameptr;
			char    *es_rname;
			char    *es_rowner;
		    exec sql end declare section;

		    register OPQ_ALIST   **app;

		    es_rname = (char *)&relp->relname;
		    es_rowner = (char *)&relp->ownname;

		    for (app = &attlst[0]; *app; app++)
		    {
			if ((*app)->comphist)
			{
			    relp->attcount++;	/* count interesting cols */
			    continue;	/* already know this has stats */
			}
			es_atnameptr = (char *)&(*app)->attname;

			exec sql repeated select first 1 1
			    into :es_num
			    from iistats s
			    where s.table_name  = :es_rname and
				  s.table_owner = :es_rowner and
				  s.column_name = :es_atnameptr;

			exec sql begin;
			    (*app)->withstats = (bool)TRUE;
			    relp->attcount++;	/* count interesting cols */
			exec sql end;
		    }
		}

		if (att_from_input)
		{
		    return (TRUE);  /* successfully read cols from input */
		}
	    }
	}
    }

    /* relation name not in arg list or no atts specified for it */
    overrun = FALSE;
    {
	exec sql begin declare section;
	    char	*es1_atname;
	    char        es1_type[DB_TYPE_MAXLEN + 1];
	    char        es1_nulls[8+1];
	    i4          es1_len;
	    i4          es1_scale;
	    i4          es1_atno;
	    char	*es1_rname;
	    char	*es1_rowner;
	    i2		es1_ingtype;
	exec sql end declare section;

        DB_ATT_STR	tempname;

        es1_atname = (char *)&tempname;
	es1_rname = (char *)&relp->relname;
	es1_rowner = (char *)&relp->ownname;

	/* In STAR, for now, iistats and iihistograms are treated as user
	** tables and do not get sent across privileged connection; all other
	** catalogs get rerouted, to avoid deadlocks we must not mix the two
	** catalogs with other catalogs.
	*/
	exec sql repeated select c.column_name, c.column_datatype,
	    c.column_length, c.column_scale, c.column_nulls, c.column_sequence,
	    c.column_internal_ingtype
	    into :es1_atname, :es1_type, :es1_len, :es1_scale,
	    :es1_nulls, :es1_atno, :es1_ingtype
	    from iicolumns c
	    where c.table_name  = :es1_rname and
		  c.table_owner = :es1_rowner
	    order by c.column_sequence;

	exec sql begin;

	    if (!overrun)
	    {
		if (ai >= g->opq_dbcaps.max_cols)
		{
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0913_ATTRIBUTES, (i4)8, 
                        (i4)0, (PTR)g->opq_utilid,
			(i4)0, (PTR)g->opq_dbname, 
			(i4)0, (PTR)&relp->relname, 
			(i4)sizeof(g->opq_dbcaps.max_cols),
			    (PTR)&g->opq_dbcaps.max_cols);
		    /* %s: database %s, table %s, more than %d 
		    **       columns defined
		    */
		    overrun = TRUE;
		}
		else if ( adi_dtinfo(g->opq_adfcb, es1_ingtype, &typeflags)
				== E_DB_OK && !(typeflags & AD_NOHISTOGRAM) )
		{
		    ap	= (OPQ_ALIST *) getspace((char *)g->opq_utilid,
			(PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

		    /*
		    ** Trim trailing white space, convert
		    ** type to lower case.
		    */
		    (VOID) STtrmwhite(es1_atname);
		    (VOID) STtrmwhite(es1_type);
		    CVlower((char *)es1_type);

		    STcopy(es1_atname, (char *)&ap->attname);
		    /* Unnormalize attribute name, put result in ap->delimname
		    */
		    opq_idunorm((char *)&ap->attname, (char *)&ap->delimname);  

		    ap->attr_dt.db_length = es1_len;
		    ap->attrno.db_att_id = es1_atno;
		    STcopy((char *) es1_type, (char *) &ap->typename);
		    ap->nullable = es1_nulls[0];
		    ap->scale = (i2)es1_scale;
		    ap->userlen = es1_len;
		    ap->withstats = (bool)FALSE;    /* assume no stats */
		    ap->comphist = (bool)FALSE;

		    /* Translate data type into Ingres internal type */
		    opq_translatetype(g->opq_adfcb, es1_type, es1_nulls, ap);

		    setattinfo(g, attlst, &ai, (OPS_DTLENGTH)0);

# ifdef xDEBUG
		    if (opq_xdebug)
			SIprintf("\n\t\tattribute '%s'\t(%s)\n",
			    &ap->attname, &relp->relname);
# endif /* xDEBUG */

		}
	    }

	exec sql end;
    }

    if (overrun)
	return (FALSE);
    else
    {
	attlst[ai] = NULL;

	/* Determine which columns have stats. */
	{
	    exec sql begin declare section;
		i4	es_num;
		char    es_nulls[8+1];
		char    *es_atnameptr;
		char    *es_rname;
		char    *es_rowner;
	    exec sql end declare section;

	    register OPQ_ALIST   **app;

	    es_rname = (char *)&relp->relname;
	    es_rowner = (char *)&relp->ownname;

	    for (app = &attlst[0]; *app; app++)
	    {
		es_atnameptr = (char *)&(*app)->attname;

		exec sql repeated select first 1 1
		    into :es_num
		    from iistatistics s, iirelation r, iiattribute a
		    where s.stabbase = r.reltid and
			s.stabindex = r.reltidx and
			s.satno = a.attid and
			r.reltid = a.attrelid and
			r.reltidx = a.attrelidx and
			r.relid = :es_rname and
			r.relowner = :es_rowner and
			a.attname = :es_atnameptr;

		exec sql begin;
		    (*app)->withstats = (bool)TRUE;
		    relp->attcount++;	/* count interesting cols */
		exec sql end;
	    }

	    /* One last check for a composite histogram. */
	    es_atnameptr = (char *)&"IICOMPOSITE";
	    es_num = 0;

	    exec sql repeated select first 1 1
	        into :es_num
	        from iistats s
		where s.table_name  = :es_rname and
		  s.table_owner = :es_rowner and
		  s.column_name = :es_atnameptr;

	    if (es_num == 1) 
	    {
		relp->comphist = TRUE;

		/* Make fake attr descriptor for the composite histogram. */
		ap = (OPQ_ALIST *) getspace((char *)g->opq_utilid,
			(PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));
		attlst[ai] = ap;
		attlst[ai+1] = NULL;

		STcopy("IICOMPOSITE", (char *)&ap->attname);
		/* Unnormalize attribute name, put result in ap->delimname
		*/
		opq_idunorm((char *)&ap->attname, (char *)&ap->delimname);  

		ap->hist_dt.db_datatype = DB_CHA_TYPE;
		ap->attr_dt.db_length = 0;
		ap->attrno.db_att_id = -1;
		STcopy("character", (char *) &ap->typename);
		ap->nullable ='N'; 
		ap->scale = 0;
		ap->userlen = 0;
		ap->withstats = (bool)TRUE;
		ap->comphist = (bool)TRUE;

		/* Translate data type into Ingres internal type */
		es_nulls[0] = 'N';
		opq_translatetype(g->opq_adfcb, "character", es_nulls, ap);
	    }
	}

	return (TRUE);
    }
}

/*{
** Name: process	- dump statistics on a particular attribute
**
** Description:
**      Routine will dump statistics on the selected attribute
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_utilid	    name of utility
**	  opq_dbname	    name of the database being processed
**	  opq_dbcaps	    database capabilities struct
**	    cat_updateable  mask showing which catalogs are updateable
**	  opq_adfcb	    ptr to ADF control block
**	relp		    ptr to relation descriptor to be processed
**	attrp		    ptr to attribute descriptor to be processed
**	outf		    output file descriptor
**
** Outputs:
**	NONE
**
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    modifies catalogs.
**
** History:
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	10-jul-89 (stec)
**	    Added user length, scale to column info displayed.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	27-jun-90 (stec)
**	    Changed call to opq_error() for E_OP0927 to pass sequence
**	    number as parameter.
**	23-jan-91 (stec)
**	    Implement correct initialization of "complete" flag; this is done
**	    in connection with the "zw" flag.
**	14-feb-91 (stec)
**	    When retrieving statistics directly from iistatistics relation
**	    make sure time is expressed as GMT time, so that it is consistent
**	    with data from iistats.
**	18-jan-93 (rganski)
**	    Changed global variables count[] and range[] to local variables
**	    here. count is now OPN_PERCENT * instead of an array and range is
**	    OPH_CELLS instead of an array of OPQ_MAXELEMENT; since boundary 
**	    values can now be up to 1950 bytes long, it is inefficient to
**	    statically allocate maximum-size histograms. Now the entire
**	    histogram, including both counts and boundary values, is
**	    dynamically and contiguously allocated, and the histogram is simply
**	    read into it. Counts and boundary values are then read from this
**	    buffer directly.
**	    Moved statistics-printing code to opq_print_stats() in opqutils.sc.
**	26-mar-93 (vijay)
**	    AIX compiler does not like LHS typecasting.
**	15-apr-93 (ed)
**	    Bug 49864 - use symbolic value for default domain OPH_DDOMAIN.
**	26-apr-93 (rganski)
**	    Character Histogram Enhancements project:
**	    added reading of character set statistics when column is of
**	    character type.
**	9-jan-96 (inkdo01)
**	    Added support of per-cell repetition factors.
**	20-jul-00 (inkdo01)
**	    Added OP0992 message to warn that actual row count has diverged from
**	    original histogram row count.
**	2-nov-00 (inkdo01)
**	    Changes to support display of composite histograms.
**	15-apr-05 (inkdo01)
**	    Supported for 0 cell histograms (all null column).
**	27-june-05 (inkdo01)
**	    Changed iihistograms query to look for histogrammed columns to
**	    use the underlying catalog tables (sans OJ) to make it go
**	    way faster. Separate query is required to search for 
**	    composite histograms. Fixes bug 114736.
**	27-sep-05 (inkdo01)
**	    Made same change for same reason for composite histograms.
**	30-Nov-07 (kibro01) b119508
**	    When selecting composite histograms, ensure that hatno is -1,
**	    in case other histograms exist for this table.
**	15-Feb-08 (wanfr01) 
**	    Bug 119927
**	    Print iistatistic information if no histogram exists
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
*/
static VOID
process(
OPQ_GLOBAL	*g,
OPQ_RLIST	*relp,
OPQ_ALIST       *attrp,
FILE		*outf)
{
    exec sql begin declare section;
    	i2      attrno;
        i2      cellnm;
  	char	*rname;
  	char	*rowner;
	char	*attname;
	i4	reltid;
	i4	reltidx;
    exec sql end declare section;

    rname   = (char *)&relp->relname;
    rowner  = (char *)&relp->ownname;
    attname = (char *)&attrp->attname;
    attrno  = attrp->attrno.db_att_id;
    reltid  = relp->reltid;
    reltidx = relp->reltidx;

    if (deleteflag)
    {
	if (g->opq_dbcaps.cat_updateable & OPQ_STATS_UPD)
	{
	    exec sql repeated delete from iistats
		    where table_name	= :rname
		    and   table_owner	= :rowner
		    and   column_name	= :attname;
	}
	else
	{
	    exec sql repeated delete from iistatistics
		    where stabbase	= :reltid
		    and   stabindex	= :reltidx
		    and   satno		= :attrno;
	}

	if (g->opq_dbcaps.cat_updateable & OPQ_HISTS_UPD)
	{
	    exec sql repeated delete from iihistograms
		    where table_name	= :rname
		    and   table_owner	= :rowner
		    and   column_name	= :attname;
	}
	else
	{
	    exec sql repeated delete from iihistogram
		    where htabbase	= :reltid
		    and   htabindex	= :reltidx
		    and   hatno		= :attrno;
	}

	opq_error( (DB_STATUS)E_DB_OK,
	    (i4)I_OP092A_STATSDELETED, (i4)6, 
	    (i4)0, (PTR)g->opq_dbname, 
	    (i4)0, (PTR)&relp->relname,
	    (i4)0, (PTR)&attrp->attname);
	/* Statistics for database '%s', table '%s', column '%s' deleted */

	return;
    }
    else
    {
	/* Read and print statistics */

	/* Read information from iistatistics */

	exec sql begin declare section;
	    f8		nunique;	/* number of unique values in domain */
	    f8          reptfact;       /* times a value repeats in domain */
	    f8          nullcount;      /* percentage of 1.0 which is NULL */
	    char        isunique[8 + 1];/* TRUE if all values are unique */
	    i1          iscomplete;     /* TRUE if attribute is complete,
					** i.e., has coverage of all values 
					** of its domain.
					*/
	    i2          domain;         /* domain id */
	    char        date[DATE_SIZE + 1];
					/* date statistics were entered 
					** - FIXME - should get this with a
					** DB_DATA_VALUE
					*/
	    char	version[DB_STAT_VERSION_LENGTH+1];
	                                /* Statistics version number */
	    i2		histlength;	/* Length of histogram boundary values
					*/
	exec sql end declare section;

	i4	tuple_count = 0;	/* number of statistics tuples */
	char	*oversion;		/* pointer to version. NULL indicates
					** no version.
					*/
	f8	origcount;

	if (g->opq_dbcaps.cat_updateable & OPQ_STATS_UPD)
	{
	    domain = OPH_DDOMAIN;
	    iscomplete = OPH_DCOMPLETE;

	    exec sql repeated select num_unique, rept_factor,
		    num_cells, has_unique, pct_nulls, create_date,
	            stat_version, hist_data_length
		    into :nunique, :reptfact, :cellnm, :isunique,
		    :nullcount, :date, :version, :histlength
		    from iistats
		    where table_name    = :rname
		    and   table_owner   = :rowner
		    and   column_name   = :attname;
	    exec sql begin;
		    tuple_count++;
	    exec sql end;
	}
	else
	{
	    /*
	    ** When iistats catalog is not updateable we assume that it is
	    ** a view, in this case, we further assume that it is a view on
	    ** iistatistics catalog (INGRES, RMS) and we get data directly
	    ** from the underlying table. To make output consistent with data
	    ** from iistats, we need to ensure that time is expressed as GMT.
	    ** We will use date_gmt() function, which is only available in
	    ** INGRES and RMS servers. We expect that other servers will have
	    ** iistats relation defined as updateable (not a view) and the
	    ** TRUE statement block (above) will be executed rather than this
	    ** one.
	    */

	    exec sql begin declare section;
		i1	es_sunique;	/* TRUE if all values are unique */
	    exec sql end declare section;

	    exec sql repeated select snunique, sreptfact, snumcells, sunique,
		    snull, date_gmt(sdate), scomplete, sdomain, sversion,
		    shistlength
		    into :nunique, :reptfact, :cellnm, :es_sunique,
		    :nullcount, :date, :iscomplete, :domain, :version,
		    :histlength
		    from iistatistics
		    where stabbase	= :reltid
		    and   stabindex	= :reltidx
		    and   satno		= :attrno;
	    exec sql begin;
		    tuple_count++;
	    exec sql end;

	    if (es_sunique == 0)
		isunique[0] = 'N';
	    else if (es_sunique == 1)
		isunique[0] = 'Y';
	    else
		isunique[0] = '?';

	    isunique[1] = '\0';	    /* construct null terminated string */
	}

	if (tuple_count == 0)
	    return;			/* no statistics available */

	if (tuple_count != 1)
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP0926_DUPLICATES, (i4)8, 
		(i4)0, (PTR)g->opq_dbname, 
		(i4)0, (PTR)&relp->relname,
		(i4)0, (PTR)&attrp->attname,
		(i4)sizeof(tuple_count), (PTR)&tuple_count);
	    /* duplicate statistics tuples: database %s, table %s, column %s
	    **      ... %d duplicates */
	    (*opq_exit)();
	}

	attrp->hist_dt.db_length = histlength;	/* Adjust histogram value
						** length according to what was
						** in iistatistics */
	if (attrp->comphist)
	{
	    STRUCT_ASSIGN_MACRO(attrp->hist_dt, attrp->attr_dt);
						/* Init type from hist. */
	    attrp->userlen = histlength;
	}

	if (!STcompare(version, DB_NO_STAT_VERSION))
	    /* There is no version */
	    oversion = NULL;
	else
	    /* There is a version */
	    oversion = version;

	/* Check for histogram whose row count is out of whack with the
	** current table row count. */
	origcount = nunique * reptfact;		/* count when hist built */
	if (origcount * 1.25 < (f8)relp->ntups || 
			origcount * .8 > (f8)relp->ntups)
	{
	    /* When histogram was built, row count was 20% greater or less
	    ** than now. Warn that value distribution may not be accurate. */
	    opq_error( (DB_STATUS) E_DB_WARN,
		(i4)W_OP0992_ROWCNT_INCONS, (i4)4,
		(i4)0, (PTR)&attrp->attname,
		(i4)0, (PTR)&relp->relname);
	}
	

	if (quiet || cellnm <= 0)
	{
	    /* Quiet mode (-zq) or 0-cell. Print out iistatistics 
	    ** information only */
	    opq_print_stats(g, relp, attrp, oversion, relp->ntups, relp->pages,
			    relp->overflow, date, nunique, reptfact, isunique,
			    iscomplete, domain, cellnm, nullcount,
			    (OPH_COUNTS)NULL, (OPH_COUNTS)NULL, 
			    (OPH_CELLS)NULL, outf, FALSE, quiet);
	    return;
	}
	else
	{
	    /* Read histogram from iihistograms and print all statistics */
	    
	    exec sql begin declare section;
	    i2	sequence;		/* Sequence number of histogram tuple
					** to be read */
	    DB_DATA_VALUE es2_hist;	/* Read histogram into this structure
					** as SQL char without null terminator
					*/
	    exec sql end declare section;
	    
	    OPH_CELLS	histogram = NULL; /* Pointer to entire histogram */
	    OPH_COUNTS	count;		/* Pointer to cell counts */
	    OPH_COUNTS	cellrepf = NULL; /* Pointer to cell rep. factors */
	    i4	*intrepf;
	    OPH_CELLS	range;		/* Pointer to cell boundary values */
	    OPO_TUPLES	num_tups, nt1;  /* Number of histogram tuples in this
					** histogram */
	    i4     total, tot1;	/* Total size of the entire histogram
					** including cell counts, boundary
					** values, and character set statistics
					** (when present) */
	    i4		hist_count;	/* Number of iihistogram tuples read */
	    i4		i;
	    bool	norepfs = TRUE; /* rep factor array indicator */
	    
	    total = cellnm * (sizeof(OPN_PERCENT) + attrp->hist_dt.db_length);

	    /* If column is of character type and there is a version, add
	    ** length of character set statistics to total length */
	    if ((attrp->hist_dt.db_datatype == DB_CHA_TYPE ||
		 attrp->hist_dt.db_datatype == DB_BYTE_TYPE) && oversion != NULL)
		total += histlength * (sizeof(i4) + sizeof(f4));
	    tot1 = total + cellnm * sizeof(OPN_PERCENT);
			/* size including rep. factor array (if present) */
	    
	    num_tups = (total + DB_OPTLENGTH - 1) / DB_OPTLENGTH;
	    nt1 = (tot1 + DB_OPTLENGTH - 1) / DB_OPTLENGTH;
			/* num_tups including rep factor array (if present) */
	    
	    /* Allocate contiguous memory for entire histogram. This will be
	    ** filled with counts followed by boundary values. Must be a
	    ** multiple of histogram tuple size.
	    ** Possible presence of repetition factor array requires alloc.
	    ** of enough memory to contain additional hist tuples, too. 
	    ** Whereas cell counts and boundary values are processed in
	    ** place in this big buffer, the rep factors are MEcopy'ed
	    ** to a separately allocated (and aligned) location after the
	    ** histogram tuples are read.
	    */
	    (VOID) getspace((char *)g->opq_utilid, (PTR *)&histogram, 
			    (u_i4)(nt1 * DB_OPTLENGTH));

	    /* Allocate repetition factor array. This is done separately
	    ** to assure proper alignment.
	    */
	    (VOID) getspace((char *)g->opq_utilid, (PTR *)&cellrepf,  
			    (u_i4)(cellnm * sizeof(OPN_PERCENT)));
	    intrepf = (i4 *)cellrepf;	
	    
	    /* Point count and range at their respective first elements */
	    count = (OPH_COUNTS) histogram;
	    range = (OPH_CELLS)  histogram + cellnm * sizeof(OPN_PERCENT);
	    
	    /* Initialize buffer for selecting histogram tuples into */
	    es2_hist.db_datatype= DB_BYTE_TYPE;		/* SQL char type */
	    es2_hist.db_length 	= sizeof(DB_OPTDATA);	/* Size of tuple */
	    es2_hist.db_prec	= 0;			/* Just in case */
	    es2_hist.db_data	= (PTR) histogram;	/* Start selecting into
							** start of histogram
							** buffer */
	    
	    /* Read all histogram tuples into histogram buffer */
	    for (sequence = 1; sequence <= nt1; sequence++)
	    {
		hist_count = 0;
		
		exec sql repeated 
		    select	h.histogram
		    into	:es2_hist
		    from	iihistogram h, 
				iirelation r, iiattribute a
		    where	h.htabbase = r.reltid and
				h.htabindex = r.reltidx and
				r.reltid = a.attrelid and
				r.reltidx = a.attrelidx and
				h.hatno = a.attid and
				r.relid = :rname and
				r.relowner = :rowner and
				a.attname = :attname and
                                a.attver_dropped = 0 and
				h.hsequence+1 = :sequence;
		
		exec sql begin;
		hist_count++;
		exec sql end;
		if (hist_count == 0 && 
			STcompare((char *)attname, (char *)"IICOMPOSITE") == 0)
		{
		    /* Retry with just iirelation & iihistogram to find 
		    ** composite histogram, checking that the hatno is -1
		    ** (kibro01) b119508. */
		    exec sql repeated
			select	h.histogram
			into	:es2_hist
			from	iihistogram h, 
				iirelation r
			where	h.htabbase = r.reltid and
				h.htabindex = r.reltidx and
				h.hatno = -1 and
				r.relid = :rname and
				r.relowner = :rowner and
				h.hsequence+1 = :sequence;

		    exec sql begin;
		    hist_count++;
		    exec sql end;
		}


		if (hist_count == 0)
		{
		    /* No histogram due to SIR 114323.  Print iistatistics
		    ** info and exit
		    */

		    opq_print_stats(g, relp, attrp, oversion, relp->ntups, relp->pages,
			    relp->overflow, date, nunique, reptfact, isunique,
			    iscomplete, domain, cellnm, nullcount,
			    (OPH_COUNTS)NULL, (OPH_COUNTS)NULL, 
			    (OPH_CELLS)NULL, outf, FALSE, TRUE);
		    return;
		}

		if (hist_count != 1)
		{
		    if (num_tups != nt1 && sequence == num_tups+1) break;
				/* old histogram with NO repfact array */
		    opq_error( (DB_STATUS)E_DB_ERROR,
			      (i4)E_OP0927_DUPLICATES, (i4)8, 
			      (i4)0, (PTR)g->opq_dbname, 
			      (i4)0, (PTR)&relp->relname,
			      (i4)0, (PTR)&attrp->attname,
			      (i4)sizeof(hist_count), (char *)&hist_count,
			      (i4)sizeof(sequence), (char *)&sequence);
		    /* duplicate histogram tuples: database %s, table %s, 
		    ** column %s... %d duplicates */
		    (*opq_exit)();
		}
		
		/* Advance to next tuple area in histogram buffer */
 		es2_hist.db_data = (OPH_CELLS) es2_hist.db_data + DB_OPTLENGTH;
	    }
	    
	    /* Copy/prepare repetition factor array (if present) */
	    if (sequence == nt1+1)
	    {
		MEcopy ((PTR)(histogram+total), tot1-total,
			(PTR)cellrepf);	/* copy to aligned array */
		for (i = 0; i < cellnm && norepfs; i++)
		   if (intrepf[i] != 0) norepfs = FALSE;
	    }
	    if (norepfs)	
	       for (i = 0; i < cellnm; i++) cellrepf[i] = reptfact;
			/* no array in histogram - default to all reptfact */

	    /* Print the statistics */
	    opq_print_stats(g, relp, attrp, oversion, relp->ntups, relp->pages,
			    relp->overflow, date, nunique, reptfact, isunique,
			    iscomplete, domain, cellnm, nullcount, count,
			    cellrepf, range, outf, FALSE, quiet);
	}
    }

    return;
}

/*
** Name: main	- entry point to statdump
**
** Description:
**      Entry point the statdump utility.
**
** Inputs:
**	argc	    argument count
**	argv	    array of ptr to arguments
**
** Outputs:
**	none.

**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-jan-87 (seputis)
**          initial creation from statdump
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	    Add output statistics to a file feature.
**	20-sep-89 (stec)
**	    Recognize new unadvertised "-zqq" flag resulting in supression
**	    of warning messages when tables with no statistics are found.
**	10-apr-90 (stec)
**	    Allocates and initializes relation and attribute arrays.
**	10-jan-91 (stec)
**	    Changed length of buffer for ERreport() to ER_MAX_LEN.
**	    Added IIUGinit() call to initialize CM attribute table (requested
**	    by stevet).
**	23-jan-91 (stec)
**	    Modified code to pass an additional parm to badarglist().
**	12-feb-92 (rog)
**	    Changed call to PCexit() to FAIL instead of OPQ_ESTATUS.
**	14-dec-92 (rganski)
**	    Changed application flag from DBA_VERIFYDB (-A5) to DBA_OPTIMIZEDB.
**	    Changed to use macro DBA_OPTIMIZEDB instead of hard-coded constant.
**	    Added some explanatory comments to existing code.
**	    Pass new bound_length parameter to badarglist(), for new optimizedb
**	    "-length" flag.
**	20-sep-93 (rganski)
**	    Delimited ID support. Use relp->delimname instead of relp->relname
**	    in set nostatistics statement.
**	2-nov-93 (robf)
**          Verify we are authorized to run statdump
**	10-nov-93 (robf)
**          Save password (if applicable) so user doesn't get prompted
**	    twice.
**	25-apr-94 (rganski)
**	    b60272: Use SIfopen() with length = SI_MAX_TXT_REC for output file,
**	    to allow writing long histogram lines.
**	13-sep-96 (nick)
**	    Call EXsetclient() else we don't catch user interrupts.
**	9-june-99 (inkdo01)
**	    Change to support ifid and ofid for optdb.
**	2-nov-00 (inkdo01)
**	    Changes to support display of composite histograms.
**	17-july-01 (inkdo01)
**	    Changes to support "-xr" command line option.
**	22-mar-2005 (thaju02)
**	    Changes for support of "+w" SQL option.
**	01-nov-2005 (thaju02)
**	    Add statdump param to i_rel_list_from_input().
**	22-dec-06 (hayke02)
**	    Add &junk for the nosetstatistics argument in the call to
**	    badarglist(). This change implements SIR 117405.
*/
int
main(
int    argc,
char   *argv[])
{
    exec sql begin declare section;
	char	*ifs[MAXIFLAGS];	/* flags to ingres */
	char	*user;
        char	appl_flag[20];
	char	*waitopt = NULL;
    exec sql end declare section;

    char	*argvp;
    EX_CONTEXT	excontext;
    i4		parmno;
    char	**dbnameptr;
    char	*outfnm;	    /* output file name ptr */
    FILE	*outf;
    f8		samplepct;	    /* percentage of rows to sample
				    ** (not used)
				    */
    bool	sortsample;	    /* TRUE if sort of sample tids
				    ** requested (not used)
				    */

    (void)EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* Initialize CM attribute table. */
    if (IIUGinit() != OK)
    {
	PCexit((i4)FAIL);
    }

    /* initialize exit handler */
    opq_exit = stat_opq_exit;
    usage = stat_usage;

    /* zero fill the global struct */
    MEfill (sizeof(OPQ_GLOBAL), 0, (PTR)&opq_global);

    opq_global.opq_adfcb = &opq_global.opq_adfscb;
    opq_global.opq_utilid = "statdump";

    opq_retry = OPQ_NO_RETRIES;
    if (EXdeclare(opq_exhandler, &excontext) != OK)
    {
	if (opq_excstat == OPQ_INTR || opq_excstat == OPQ_DEFAULT)
	{
	    IIbreak();
	}
	else
	{
	    /* serialization error has occurred */
	    opq_error((DB_STATUS)E_DB_WARN,
		(i4)W_OP0970_SERIALIZATION, (i4)0);
	}
	(*opq_exit)();	    /* should not return - exit after
			    ** closing database
			    */
    }

    /*
    ** Initialize ADF for internal use and initialize ADF
    ** parameters for ingres, so that results are consistent.
    ** Needs to be initialized now, so that we can read floating
    ** point number for sampling percentage. Later we will modify
    ** precision according to value read.
    */
    adfinit(&opq_global);

    /* read info from command file if necessary */
    fileinput(opq_global.opq_utilid, &argv, &argc);

    {
	bool		junk;		    	/* uninteresing options */
	char		*cjunk;			/* uninteresting char * arg */
	OPS_DTLENGTH	bound_length = 0;	/* No -length parameter in
						** statdump. */

	/* validate the parameter list */
	if (badarglist(&opq_global, argc, argv, &parmno, ifs,
	    &dbnameptr, &junk, &junk, &junk, &junk, (i4 *)&junk,
	    (i4 *)&junk, &quiet, &deleteflag, &user, &cjunk, &outfnm, &junk,
	    &samplepct, &supress_nostats, &junk, &junk,
	    &bound_length, &waitopt, &junk))
	{
	    /* print usage and exit */
	    (*usage)();
	}
    }

    /* parmno will contain the offset of the database name */
    while ((argvp = argv[parmno]) && CMalpha(argvp))	/* a database name */
    {
	exec sql begin declare section;
	    char	*dbname;
	exec sql end declare section;

	OPQ_RLIST	**ex_rellist = NULL;
	bool		retval;
	bool		excl = FALSE;

	dbname = argvp;			/* get the database name, note that 
                                        ** dbnameptr will place the database 
                                        ** name in the ifs array, so that the
                                        ** following ingres command will pick
                                        ** it up */

	opq_global.opq_dbname = dbname;

	parmno++;	/* set ptr to beginning of relation parameters */

	/*
	** Save user password if applicable
	*/
	exec sql set_sql (savepassword=1);
	/*
	** Options to Ingres and gateways differ, we have to log in twice.
	** First time we do not pass any options that the user might have
	** specified. After connecting we check iidbcapabilities to see if
	** it is an Ingres database, then we log out and reconnect with 
	** user specified options.
	*/
	
	if (!user)
	{
	    exec sql connect :dbname options = :waitopt;
	}
	else
	{
	    exec sql connect :dbname identified by :user
			options = :waitopt;
	}

	opq_global.opq_env.dbopen = TRUE;

	/* See if Ingres dbms */
	opq_dbmsinfo (&opq_global);

	exec sql disconnect;

	opq_global.opq_env.dbopen = FALSE;
        /*
	** Check if we  are allowed to run statdump
	*/
	if(opq_global.opq_no_tblstats==TRUE)
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP0984_NO_PRIV_TABLE_STATS, (i4)0);
	    (*opq_exit)();
	}

	if (opq_global.opq_dbms.dbms_type & (OPQ_INGRES | OPQ_STAR | OPQ_RMSGW))
	{
	    /* Ingres dbms */

	    /* Convert application flag DBA_OPTIMIZEDB to '-A' followed by
	    ** numeral
	    */
	    {
		char	tmp_buf[15];

		CVla((i4)DBA_OPTIMIZEDB, tmp_buf);
		(VOID)STpolycat(2, "-A", tmp_buf, appl_flag);
	    }
	    
	    /* Connect */
	    if (!user)
	    {
		exec sql connect :dbname options = :appl_flag, '+Y',
			:ifs[0], :ifs[1], :ifs[2], :ifs[3], :ifs[4],
			:ifs[5], :ifs[6], :ifs[7], :ifs[8], :ifs[9],
			:waitopt;
	    }
	    else
	    {
		exec sql connect :dbname identified by :user
			options = :appl_flag, '+Y',
			:ifs[0], :ifs[1], :ifs[2], :ifs[3], :ifs[4],
			:ifs[5], :ifs[6], :ifs[7], :ifs[8], :ifs[9],
			:waitopt;
	    }
	}
	else
	{
	    /* Gateway dbms */
	    if (!user)
	    {
		exec sql connect :dbname options =
			    :ifs[0], :ifs[1], :ifs[2], :ifs[3], :ifs[4],
			    :ifs[5], :ifs[6], :ifs[7], :ifs[8], :ifs[9],
			    :waitopt;
	    }
	    else
	    {
		exec sql connect :dbname identified by :user options =
			    :ifs[0], :ifs[1], :ifs[2], :ifs[3], :ifs[4],
			    :ifs[5], :ifs[6], :ifs[7], :ifs[8], :ifs[9],
			    :waitopt;
	    }
	}

	opq_global.opq_env.dbopen = TRUE;

	/*
	** Reinitialize ADF precision for floating point numbers.
	*/
	adfreset(&opq_global);

	/* See if statistics tables are updateable */
	opq_upd (&opq_global);

	/* Find out the owner and dba */
        opq_owner(&opq_global);

	/* Find the max number of relations to be considered. */
	opq_mxrel(&opq_global, argc, argv);

	/*
	** Allocate relation and attribute arrays,
	** memory will have been cleared.
	*/
	{
	    u_i4 size;

	    size = (opq_global.opq_maxrel + 1) * sizeof(OPQ_RLIST);
	    rellist = (OPQ_RLIST **)getspace((char *)opq_global.opq_utilid,
		(PTR *)&rellist, size);

	    size = (opq_global.opq_dbcaps.max_cols + 1) * sizeof(OPQ_ALIST);
	    attlist = (OPQ_ALIST **)getspace((char *)opq_global.opq_utilid,
		(PTR *)&attlist, size);
	}

        /*
	** Initialize rellist with the info on relations in 
        ** the database, which will be analyzed for statistics.
	*/
	if (isrelation(argv[parmno], &excl))
	{
	    if (excl)
	    {
		u_i4 size;
		/* First is a "-xr". Either they all are (and we need another
		** "rellist", or there's an error coming up. */
		size = (opq_global.opq_maxrel + 1) * sizeof(OPQ_RLIST);
		ex_rellist = (OPQ_RLIST **)getspace((char *)opq_global.opq_utilid,
		    (PTR *)&ex_rellist, size);

		retval = i_rel_list_from_input(&opq_global, ex_rellist, &argv[parmno], (bool)TRUE);
		retval = r_rel_list_from_rel_rel(&opq_global, rellist, ex_rellist, 
								(bool)TRUE);
	    }
	    else
	    {
		/* fetch ONLY relation names specified since -r is specified */
		retval = i_rel_list_from_input(&opq_global, rellist, &argv[parmno], (bool)TRUE);
	    }
	}
	else
	{
	    /* fetch all relations in database since -r was NOT specified */
	    retval = r_rel_list_from_rel_rel(&opq_global, rellist, NULL, (bool)TRUE);
	}

	if (retval)
	{
	    OPQ_RLIST	    **relp;	/* used to traverse the list of
                                        ** relation descriptors */

	    outf = (FILE *) NULL;

	    if (!deleteflag)
	    {
		if (outfnm)
		{
		    /* open the output file */
		    LOCATION	inf_loc;
		    STATUS	status;

		    if (((status = LOfroms(PATH & FILENAME,
			    outfnm, &inf_loc)) != OK)
			||
			((status = SIfopen(&inf_loc, "w", SI_TXT,
					  (i4)SI_MAX_TXT_REC, &outf)) != OK)
			|| 
			(outf == (FILE *) NULL)
		       )
		    {
			/* report error opening file */
			opq_error( (DB_STATUS)E_DB_ERROR,
			    (i4)E_OP0937_OPEN_ERROR, (i4)6,
			    (i4)0, (PTR)opq_global.opq_utilid,
			    (i4)0, (PTR)outfnm, 
			    (i4)sizeof(status), (PTR)&status);
		    }		    

		    opq_global.opq_env.ofid = outf;
		}
	    }

	    for (relp = rellist; *relp; relp++)
	    {
		opq_retry = OPQ_SER_RETRIES;
		(VOID) EXdelete();
		if (EXdeclare(opq_exhandler, &excontext) != OK)
		{
		    if (opq_excstat == OPQ_INTR ||
			opq_excstat == OPQ_DEFAULT
		       )
		    {
			IIbreak();
			(*opq_exit)();	/* should not return - exit 
					** after closing database.
					*/
		    }
		    else
		    {
			/* serialization error has occurred */
			opq_error((DB_STATUS)E_DB_WARN,
			 (i4)W_OP0970_SERIALIZATION, (i4)0);
		    }
		}

		if ((*relp)->withstats == (bool)FALSE && !quiet
		    && !supress_nostats)
		{
		    opq_error((DB_STATUS)E_DB_OK,
			(i4)I_OP093E_NO_STATS, (i4)8,
			(i4)0, (PTR)opq_global.opq_utilid,
			(i4)0, (PTR)opq_global.opq_dbname,
			(i4)0, (PTR)&(*relp)->relname, 
			(i4)0, (PTR)&(*relp)->ownname);
		    continue;
		}

		if (att_list(&opq_global, attlist, *relp, &argv[parmno]))
		{
		    OPQ_ALIST	**attrp;

		    for (attrp = attlist; *attrp; attrp++)
		    {
			if ((*attrp)->withstats == (bool)FALSE)
			{
			    continue;
			}

			process(&opq_global, *relp, *attrp, outf);
		    }
		}

		/* If we are deleting statistics and we deleted all of them
		** for a relation, tell the relation relation entry for this 
		** relation so the optimizer doesn't make unnecessary probes
		** into the zopt relations 
		*/
		if (deleteflag)
		{
		    exec sql begin declare section;
			i4	es_num;
			char	*es_rname;
			char	*es_rowner;
		    exec sql end declare section;

		    bool        nostats;

		    nostats = TRUE;

                    es_rname = (char *)&(*relp)->relname;
                    es_rowner= (char *)&(*relp)->ownname;

		    exec sql select first 1 1 into :es_num
			from iistats
			where   table_name  = :es_rname and
				table_owner = :es_rowner;
		    exec sql begin;
			nostats = FALSE;
		    exec sql end;

		    /*
		    ** If no rows retrieved then no statistics exist,
		    ** reset catalog info. In case of 6.0 Ingres we
		    ** will use the SET command, else we will reset
		    ** info in standard catalogs if they are updateable.
		    */
		    if (nostats)
		    {
			if (opq_global.opq_lang.ing_sqlvsn > OPQ_SQL600)
			{
			    exec sql begin declare section;
				char stmt[sizeof("set nostatistics ") +
				    DB_GW1_MAXNAME+2];
			    exec sql end declare section;

			    (VOID) STprintf(stmt, "set nostatistics %s",
				&(*relp)->delimname);

			    exec sql execute immediate :stmt;
			}
			else if (opq_global.opq_dbcaps.cat_updateable & OPQ_TABLS_UPD)
			{
			    exec sql begin declare section;
				char	es_no[8 + 1];
			    exec sql end declare section;

			    es_no[0] = EOS;
			    STcopy("N", es_no);

			    exec sql repeated update iitables
				set table_stats = :es_no
				where   table_name  = :es_rname and
					table_owner = :es_rowner;

# ifdef FUTURE_USE
			    exec sql repeated update iiphysical_tables
				set table_stats = :es_no
				where   table_name  = :es_rname and
					table_owner = :es_rowner;
# endif /* FUTURE_USE */
			}
		    }

		    /*
		    ** To save the work done so far, we commit, i.e.,
		    ** data will be committed after processing each
		    ** relation; this is reasonable, since number of
		    ** rows affected is not very large, and delete is
		    ** a fast operation in this case.
		    */
		    exec sql commit;
		}
	    }

	    if (outf)
	    {
		/* close the output file */
		STATUS	status;

		status = SIclose(outf);
		if (status != OK)
		{   /* report error closing file */
		    char	buf[ER_MAX_LEN];

		    if ((ERreport(status, buf)) != OK)
		    {
			buf[0] = '\0';
		    }

		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0938_CLOSE_ERROR, (i4)8,
			(i4)0, (PTR)opq_global.opq_utilid,
			(i4)0, (PTR)outfnm, 
			(i4)sizeof(status), (PTR)&status,
			(i4)0, (PTR)buf);
		    /* %s: cannot close output file:%s, os status:%d\n\n
		    ** outfnm, status
		    */
		}		    

		opq_global.opq_env.ofid = (FILE *)NULL;
	    }
	}

	while (argv[parmno] && argv[parmno][0] == '-')
	    parmno++;				/* skip to next db */

	exec sql disconnect;

	opq_global.opq_env.dbopen = FALSE;
    }

    (VOID) EXdelete();
    PCexit((i4)OK);
}
