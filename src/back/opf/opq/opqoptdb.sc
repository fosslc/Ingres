/*
**Copyright (c) 1986, 2005, 2010 Ingres Corporation
*/
# include    <compat.h>
# include    <gl.h>
# include    <cs.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <ddb.h>
# include    <dudbms.h>
# include    <ulm.h>
# include    <ulf.h>
# include    <adf.h>
# include    <add.h>
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
# include    <mh.h>
# include    <cm.h>
# include    <si.h>
# include    <pc.h>
# include    <lo.h>
# include    <ex.h>
# include    <er.h>
# include    <st.h>
# include    <tm.h>
# include    <cv.h>
# include    <bt.h>
# include    <generr.h>
# include    <clfloat.h>
/* beginning of optimizer header files */
# include    <opglobal.h>
# include    <opdstrib.h>
# include    <opfcb.h>
# include    <opgcb.h>
# include    <opscb.h>
# include    <ophisto.h>
# include    <opboolfact.h>
# include    <oplouter.h>
# include    <opeqclass.h>
# include    <opcotree.h>
# include    <opvariable.h>
# include    <opattr.h>
# include    <openum.h>
# include    <opagg.h>
# include    <opmflat.h>
# include    <opcsubqry.h>
# include    <opsubquery.h>
# include    <opcstate.h>
# include    <opstate.h>
# include    <opxlint.h>
# include    <opq.h>
# include    <adulcol.h>
# include    <time.h>

# if defined(i64_win)
# pragma optimize("", off)
# endif

exec sql include sqlca;
exec sql include sqlda;
exec sql declare s statement;
exec sql declare c1 cursor for s;

# define MAXCOLS	(IISQ_MAX_COLS - 1)/2
# define MAXEXIT	5
# define MAXFREQ	2000
# define MAXCOMPOSITE	256
# define OPQ_DEFSAMPLE	1000000
# define OPQ_I8COUNT	i8
# define OPQ_MINEXCELL	1.0

# define NO_COLLID_SUP_IN_STAR

/*
**
**  Name: OPQOPTDB.SC - program to generate statistics for optimizer
**
**  Description:
**      Utility to generate statistics for the optimizer
**      FIXME - optimal size of element in iihistogram relation is
**              (tuple_2008+tid_4)/8 = 251 = 12+tid_4+data_235 
**              so choose the SQL datatype to store binary data
**              of varchar 232 (with a 2 byte size), when equel varchar
**              is ready, use i4 for now.  Need a DB_DATA_VALUE interface
**              so null termination is done.
**      FIXME - change so that one scan of relation is needed, use index
**              info to determine uniqueness in this case, and defaults
**              for other statistics that require sorting
**
**
**
**  History:
**      17-nov-86 (seputis)
**          initial creation from OPTIMIZEDB.C
**	7/6/82 (kooi)
**	    written (original version).
**	11-17-88 (stec)
**	    Fix minmax statistics bug.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	    Add create statistics from a file data feature.
**	08-jul-89 (stec)
**	    Merged into terminator path (changed ERlookup call).
**	    Changed code to reflect changes in adc_lenchk interface.
**	10-jul-89 (stec)
**	    Added user length, scale to column info displayed.
**	31-jul-89 (stec)
**	    When updating number of rows for subject table in iitables
**	    update the same for indexes on the table.
**	    Improve tid generation when sampling small tables.
**	13-sep-89 (stec)
**	    Fix opq_sample to process 1 row table correctly (bug 7933).
**	    Fix bug 7735; continue creating stats on table columns even
**	    if a column with all values NULL is encountered.
**	    Fix bug 7668 - preserve uniqness of column values when sampling,
**	    by adding tid values from the original table to rows in the sample
**	    table and when creating stats on the sample table removing
**	    col_on_which_stats_are_created, tid_value_from_orig_tbl duplicates.
**	    In usage message display '-zn#' instead of '-zf#' (float prec).
**	20-sep-89 (stec)
**	    Change interface to badarglist (add 1 argument for "-zqq" flag).
**	24-oct-89 (stec)
**	    Fix bug 8469. When sample stats are created, repetition factor
**	    is off. The solution for now, even though it is not an ideal one,
**	    will be to scale both the repetition factor and unique values up.
**	01-nov-89 (stec)
**	    Treat TIDs as unsigned i4's rather that signed.
**	12-dec-89 (stec)
**	    Fix a porting problem: DB_TEXT_STRING should be aligned.
**	    Add generic error handling. Changed utilid to global reference.
**	    Added cleanup for temporary sampling tables (will occur on error
**	    exit and interrupts).
**	    Moved exception handler to opqutils.sc. Improved error handling
**	    and added error handling for serialization errors (TXs will be
**	    automatically restarted. Made code case sensitive with respect to
**	    database object names. Will count rows when count not available.
**	    Improved exception handling.
**	05-jan-89 (stec)
**	    Correct Common SQL problem; pass an initialized char string
**	    rather than use char(date('now')).
**	29-jan-90 (stec)
**	    Improve error handling for UDTs.
**	06-feb-90 (stec)
**	    Change handling of the unique flag when "-i" option specified.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values.
**	    Changed the way statistics flag is set: for INGRES and gateways
**	    it's set after stats for each column get created, for STAR only
**	    after the first and last column. Created opq_setstats().
**	    To minimize logging added commit after each insert statement
**	    in the opq_sample() routine creating table with sample data.
**	    Added a mechanism in process() which causes two adjacent cells
**	    with identical upper and lower bounds to collapse into a single
**	    cell.
**	    Changed exception handling code in main() to use defines rather
**	    than hardcoded constants.
**	    Put all values related to float handling by histdec() into
**	    a struct.
**	    Improved error handling of the read_process().
**	30-mar-90 (stec)
**	    Change code in process() to collapse cells in a slightly
**	    different way.
**	    Removed automatic initialization of exacthist, inexacthist,
**	    prev_full_val static variables for hp3_us5 (HP 9000/320) compiler.
**	10-apr-90 (stec)
**	    Allocates and initializes relation and attribute arrays.
**	16-may-90 (stec)
**	    Included cv.h, moved most FUNC_EXTERNs to opq.h.
**	    Fixed bug 21556 in insert_histo().
**	28-jun-90 (stec)
**	    Modified insert_histo() to start sequence numbering for
**	    iihistograms for STAR with 1.
**	06-jul-90 (stec)
**	    Modified insert_histo(), read_process() and main() to solve the 
**	    problem of 2PC feature not being supported in a VAXcluster
**	    environment at this time. This problem exists only for Ingres/STAR.
**	    See modified procedures for specific explanation of the problem.
**	09-jun-90 (stec)
**	    Removed automatic initialization of exacthist, inexacthist,
**	    prev_full_val static variables for mac_us5 C compiler.
**	24-jul-90 (stec)
**	    Added checks in the code to treat the RMS gateway pretty much
**	    the same way as Ingres, except for sampling.
**	02-aug-90 (stec)
**	    Modified att_list() to adjust case of the argv value.
**	06-nov-90 (stec)
**	    Changed code in att_list() to process correctly 
**	    attribute lists on the command line.
**	10-jan-91 (stec)
**	    Added include for <clfloat.h>. Modified main(), read_process(),
**	    inithistdec().
**	22-jan-91 (stec)
**	    Modified insert_histo(), usage(), main(), read_process(),
**	    opq_singres().
**	13-jan-91 (stec)
**	    Removed inithistdec(), histdec(), global variable opq_hdec,
**	    modified main(), minmax(), process(), read_process().
**	28-feb-91 (stec)
**	    Modified read_process().
**	28-feb-91 (stec)
**	    Modified code using TEXT6, TEXT7 strings. The output version
**	    of these strings, i.e., one with an "O" at the end will be used
**	    for formatting a format string, such that thw width and precision
**	    will be specified for floats like the null count and the normalized
**	    cell count. This addresses some issues raised by the bug report
**	    36152, which was marked NOT A BUG. To see the changes search this 
**	    modile for strings TEXT6, TEXT7.
**	18-mar-91 (stec)
**	    Modified insert_histo() and all routines which call it to reflect
**	    presence of a new argument.
**	26-jun-91 (seputis)
**	    - Add warning when table does not exist when statistics are being 
**	      read in
**	    - increased line size to fix diffs in opfutils tests
**	06-apr-92 (rganski)
**	    Changed definition of rellist to (OPQ_RLIST **) and definition
**	    of attlist to (OPQ_ALIST **). These variables are pointers to 
**	    pointers, but they were not declared that way.
**	    Added explicit coercion for some assignment type mismatches.
**	    Merged seg's changes: can't do arithmetic on PTR variables.
**	09-jul-92 (rganski)
**	    Changed forward declaration of opq_bufalloc from
**	    FUNC_EXTERN to static, since it is a static function.
**	09-nov-92 (rganski)
**	    Added prototypes.
**	    Replaced a lot of common inline code for handling ADF errors
**	    with calls to new function opq_adferror(), which is in
**	    opqutils.sc.
**	    Moved r_rel_list_from_rel_rel() to opqutils.sc, since it is useed 
**	    by both this opqstatd.sc and opqoptdb.sc. Added extra parameter to 
**	    indicate whether the caller is statdump.
**	    Added new functions compare_values(), new_unique(),
**	    decrement_value(), and add_exact_cell(), in an attempt to
**	    modularize process().
**	    Streamlined main loop in process().
**	    Removed FUNC_EXTERN of opn_diff(), since I removed the call to it 
**	    (see header for add_exact_cell()).
**	    Commented out text after #endif's.
**	16-nov-92 (dianeh)
**	    Added CUFLIB to the NEEDLIBS line.
**	14-dec-92 (rganski)
**	    Added GLOBALDEF for bound_length, the value of the new -length
**	    command line flag. Initialized to 0.
**	    Added bound_length parameter to calls to setattinfo().
**	    Part of Character Histogram Enhancements project.
**	    Cosmetic changes.
**	18-jan-93 (rganski)
**	    Character Histogram Enhancements project, phase IIIb:
**	    Replaced static allocation of exacthist and inexacthist with
**	    dynamic allocation in process() and read_process().
**	    Since boundary values can now be up to 1950 bytes long, it is
**	    inefficient to statically allocate maximum-size histograms. Only as
**	    much memory as is required for the histogram is allocated.
**	    Changed type of exacthist and inexacthist from array of
**	    OPQ_MAXELEMENT to OPH_CELLS. Changed all references to these arrays
**	    and their elements to pointer dereferences, using pointer
**	    arithmetic.
**	    Added new version parameter to insert_histo() and calls thereto.
**	    Moved global variable opq_date to opqutils.sc. Moved typedef of
**	    OPQ_DBUF to opq.h.
**	    Removed LINE_SIZE: this is replaced by OPQ_LINE_SIZE in opq.h,
**	    which is much larger, to accomodate long boundary values.
**	    Added static global variables opq_line (buffer for input lines) and
**	    opq_value_buf (buffer for converting values) to replace local
**	    variables in read_process to conserve stack space.
**	    Added allocation, initialization, and storage of character set
**	    statistics arrays.
**	26-apr-93 (rganski)
**	    Character Histogram Enhancements project, phase IV:
**	    Added boundary value truncation and collection of character set
**	    statistics.
**	    Added include of opxlint.h and other header files it requires for
**	    inclusion of OPF prototypes. Removed FUNC_EXTERNs which are now in
**	    opxlint.h.
**	    Added prototypes for read_char_stats() and record_chars().
**	    Added include of bt.h.
**	28-apr-93 (rganski)
**	    Changed charnunique and chardensity from local to global variables.
**	24-may-93 (rganski)
**	    Removed row_count parameter from prototypes of
**	    inexact_histo_length() and truncate_histo().
**	21-jun-93 (rganski)
**	    Changed "cell_number" to "max_cell" throughout this file.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	20-sep-93 (rganski)
**	    Delimited ID support. In general, use delimited names except when
**	    name is used in a catalog lookup, in which case use normalized
**	    name.
**      10-mar-95 (sarjo01)
**          Bug no. 67058: for HASH and ISAM tables, do not set relprim
**          with value from statdump file, but rather leave original value
**      10-apr-1995 (nick)(cross-int by angusm)
**      When making sanity checks on the repetition factor having loaded
**      stats from a file, use the tuple count passed to us rather than
**      rel->ntups if we are updating the page and tuple counts with
**      -zp as well. #44193
**	11-apr-1995 (inkdo01)
**	bug 68015: min/max stats on mix of char/int cols could SEGV
**	wrong var was being used in function min_max().
**      11-apr-1995 (nick)(cross-int angusm)
**        Preserve C datatypes when calculating min/max statistics until
**        actually inserting into the histogram cell ( the full statistics
**        run already did this ).
**	01-Nov-95 (allmi01)
**	    Added NO_OPTIM ming hint for dgi_us5 to correct a compiler abort if this      
**	    module is optimized at level 2 optimization.
**	20-nov-95 (nick)
**	    Alter truncate_histo() to correct erronous use of internal 
**	    datatype length when deciding whether to truncate ( by comparing 
**	    against the old character histogram size of OPH_NH_LENGTH ). #71702
**	8-jan-96 (inkdo01)
**	    Add computation of per-cell repetition factors.
**	19-jan-1996 (nick)
**	    Lack of precision in an OPH_DOMAIN value meant that we would
**	    scale the unique values in a sample to the total tuple count
**	    and hence mark the relation as unique. #70073
**	18-Apr-1996 (allmi01)
**	    Added NO_OPTIM for dgi_us5 to correct SIGSEGV in optimzedb for 
**	    1104 patch.
**	13-sep-96 (nick)
**	    Tell EX we are an Ingres tool.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to opqdata.sc.
**	27-mar-1997 (hayke02)
**	    In the function truncate_histo(), we now ensure that after
**	    histogram truncation, charnunique for the last character position
**	    is at least 2; this ensures that there will be a diff of 1 between
**	    a boundary value and the decremented boundary value (e.g. between
**	    the first two boundary values in the histogram). This change is
**	    an extension of the fix for bug 59298 and fixes bugs 81133 and
**	    81210.
**	10-oct-1996 (canor01)
**	    Add SystemProductName to E_OP094D and E_OP0950.
**	19-aug-98 (hayke02)
**	    Double the size of the inexactcount and uniquepercell arrays
**	    to cope with the possiblility of a cellcount greater than
**	    MAXCELLS + 1. This can arise when a high inexact cell count
**	    is set by the user using the -zr# flag and exact cells are
**	    added. This change fixes bug 92234.
**      21-apr-1999 (hanch04)
**          replace STindex with STchr
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	07-sep-99 (sarjo01)
**	    Bug 98665: when inserting an exact value with a high rep count
**	    into an inexact histogram, make sure check to previous value
**	    before inserting the lower bound, it may already be in the
**	    histogram.
**      07-Feb-2000 (hanal04) Bug 100372 INGSRV 1107
**         Prevent an infinite loop of opt_opq_exit(), opq_sqlerror() calls
**         by disabling the jump to opq_sqlerror() on sqlerror.
**      01-Jan-00 (sarjo01)
**          Bug 99120: Added new parameter tbl so collation sequence
**          can be used by opn_diff where applicable.
**      28-aug-200 : 07-jun-1999 (schang) roll in  changes for rms gateway
**	29-aug-2000 (somsa01)
**	    Amended last change due to compile problems.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Sep-2000 (hanch04)
**	    replace nat and longnat with i4
**      29-Jun-2001 (huazh01 ) Bug 105184
**	   (hanje04) X-Integ of 451658
**         Introduce variable dup_crrval_count to keep the accuracy of 
**         crrval_count, which won't be incremented correctly once it reaches
**         16777216. Use dup_currval_count to update currval_count if it is 
**         greater than 16777216.
**	17-sep-2001 (somsa01)
**	    Due to last cross-integration (change 452926), changed longnat's
**	    to i4's.
**	20-dec-2001 (somsa01)
**	    Added NO_OPTIM for i64_win to prevent E_OP03A2_HISTINCREASING
**	    error messages percolating into the errlog.log after an
**	    optimizedb operation.
**      31-Jan-2003 (hanal04) Bug 109519 INGSRV 2075
**         Prevent duplicate temporary table names in opq_sample.
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**	28-Jan-2005 (schka24)
**	    Space needed for dynamic queries was figured a bit too closely
**	    in some cases (no room for delimited-ID quotes).  Also, fix
**	    -zp for partitioned tables, need to set something for the
**	    partitions as well -- the master reltups/relpages is irrelevant.
**	21-Mar-2005 (schka24)
**	    More of above, add argname which is relation name as specified on
**	    the command line.  (delimname may be translated and delimited.)
**	    Remove dead code for random sampling via tids, not called.
**	22-mar-2005 (thaju02)
**	    Changes for support of "+w" SQL option.
**	17-Jun-2005 (toumi01)
**	    Set ap->attr_dt.db_collID to avoid E_OP03A2 (histogram not
**	    monitonically increasing) error in query with where clause
**	    reference to histogrammed Unicode column with case insensitive
**	    collation.
**      08-Sep-2005 (hweho01)
**          1) Avoid error US0836 "'column_collid' not found" when running     
**             optimizedb over a star db with parms "-zk -zv -zc",    
**             make separate sets of queries for star database excluding    
**             column_collid, because it is not supported by star catalog 
**             currently. The separate query sets are ifdefed with         
**             NO_COLLID_SUP_IN_STAR, so they can be removed later easily. 
**          2) Added commit statement for the star query, avoid 
**             error E_US1722.
**    25-Oct-2005 (hanje04)
**        Add prototype for update_row_and_page_stats() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	21-july-06 (dougi)
**	    Changes to use i8's to accumulate counts that might otherwise
**	    encounter the problem of adding one ("+= 1.0") to an f4 that gets
**	    stuck at 16M because of mantissa precision.
**      28-Nov-2006 (hanal04) Bug 117147
**          Ensure we establish the ing_type when OPQ_STAR. We were
**          checking uninitialised datatypes and silently failing for
**          all columns. The result, no errors, no stats.
**          OPQ_STAR does not support "declare global temp . . .". Modified
**          related routines to create a non-journaled table for
**          sampling and drop it when we are done.
**          iirelation does not exist in a STAR DB so don't try selecting from
**          it if OPQ_STAR.
**	29-Nov-2006 (kschendel)
**	    Raise default sampling (back?) to 1 million rows.
**	22-dec-06 (hayke02)
**	    Add nosetstats bool for -zy optimizedb flag to switch off 'set
**	    statistics'. This change implements SIR 117405.
**      07-feb-2008 (horda03) Bug 119853
**          Division by 0 exception causes optimizedb to fail with
**          E_OP095B.
**      19-Feb-2008 (hanal04) Bug 119877
**          Correct getspace() size calculation for inexacthist to prevent
**          spurious failures.
**	03-Oct-2008 (kiria01) b120966
**	    Added DB_BYTE_TYPE support to allow for Unicode CE entries
**	    to be treated as raw collation data alongside DB_CHA_TYPE.
**      11-dec-2008 (stial01)
**          Redefine tidp as string and then blank pad to DB_ATT_NAME size.
**	17-Mar-2009 (toumi01) b121809
**	    To make them utf8-safe, handle composite histograms as byte
**	    rather than as char. Otherwise interpretation of hex strings as
**	    2-byte utf8 characters can cause incorrect and possibly invalid
**	    (not monotonically increasing) histograms.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
void opx_error(
	i4 errcode);
static bool doatt(
	OPQ_ALIST *attlst[],
	register i4 attno,
	i4 indx,
	i4 attkdom);
static bool att_list(
	OPQ_GLOBAL *g,
	OPQ_ALIST *attlst[],
	OPQ_RLIST *relp,
	char **argv);
static bool hist_convert(
	char *tbuf,
	OPQ_ALIST *attrp,
	ADF_CB *adfcb);
static bool opq_scaleunique(
	OPO_TUPLES samplesize,
	OPH_DOMAIN sampleunique,
	OPO_TUPLES totalsize,
	OPH_DOMAIN *totalunique);
static OPO_TUPLES opq_jackknife_est(
	OPO_TUPLES samptups,
	OPO_TUPLES sampunique,
	OPO_TUPLES actualtups);
static void opq_setstats(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp);
static void write_histo_tuple(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST *attrp,
	i4 sequence,
	PTR histdata);
static void insert_iihistogram(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST *attrp,
	OPH_CELLS intervalp,
	OPN_PERCENT cell_count[],
	i4 max_cell,
	char *version);
static void insert_histo(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST *attrp,
	OPO_TUPLES nunique,
	OPH_CELLS intervalp,
	OPN_PERCENT cell_count[],
	i4 max_cell,
	i4 numtups,
	i4 pages,
	i4 overflow,
	OPO_TUPLES null_count,
	bool normalize,
	bool exact_histo,
	char unique_flag,
	bool complete,
	f8 irptfactor,
	char *version,
	bool fromfile);
static void add_default_char_stats(
	OPS_DTLENGTH histlength,
	i4 max_cell,
	OPH_CELLS intervalp);
static void minmax(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST *attlst[]);
static i4 compare_values(
	ADF_CB *adfcb,
	DB_DATA_VALUE *datavalue1,
	DB_DATA_VALUE *datavalue2,
	DB_ATT_STR *columnname);
static bool new_unique(
	i4 cellinexact,
	OPQ_ALIST *attrp,
	DB_DATA_VALUE *prev_dt,
	ADF_CB *adfcb);
static void decrement_value(
	ADF_CB *adfcb,
	OPQ_ALIST *attrp,
	OPQ_RLIST *relp,
	OPH_CELLS result);
static i4 add_exact_cell(
	OPQ_ALIST *attrp,
	OPQ_RLIST *relp,
	DB_DATA_VALUE *prev_hdt,
	i4 cellexact,
	i4 totalcells,
	ADF_CB *adfcb);
static i4 update_hcarray(
	DB_DATA_VALUE *newval,
	OPQ_MAXELEMENT *prevval,
	i4 valcount,
	i4 prevcount,
	i4 nunique,
	i4 *hccount);
static OPS_DTLENGTH exact_histo_length(
	i4 max_cell,
	OPN_PERCENT cell_count[],
	OPH_CELLS intervalp,
	OPS_DTLENGTH oldhistlength);
static OPS_DTLENGTH inexact_histo_length(
	ADF_CB *adfcb,
	OPQ_RLIST *relp,
	OPQ_ALIST *attrp,
	i4 max_cell,
	OPN_PERCENT cell_count[],
	OPH_CELLS intervalp,
	OPS_DTLENGTH oldhistlength);
static void truncate_histo(
	OPQ_ALIST *attrp,
	OPQ_RLIST *relp,
	i4 *max_cellp,
	OPN_PERCENT cell_count[],
	OPH_CELLS intervalp,
	bool exact_histo,
	ADF_CB *adfcb);
static void add_char_stats(
	OPS_DTLENGTH histlength,
	i4 max_cell,
	OPH_CELLS intervalp,
	OPQ_BMCHARSET charsets);
static void read_char_stats(
	OPQ_GLOBAL *g,
	FILE *inf,
	OPH_CELLS statbuf,
	OPS_DTLENGTH histlength);
static void record_chars(
	OPQ_ALIST *attrp,
	OPQ_BMCHARSET charsets);
static bool comp_selbuild(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST **attrpp,
	char **selexpr,
	bool *use_hex);
static void exinex_insert(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST *attrp,
	OPQ_MAXELEMENT *prev_val,
	OPQ_MAXELEMENT *prev1_val,
	OPO_TUPLES *rowest,
	i4 *cellinexact,
	i4 *cellinexact1,
	OPO_TUPLES currval_count);
static void exinex_insert_rest(
	ADF_CB *adfcb,
	OPQ_RLIST *relp,
	OPQ_ALIST *attrp,
	i4 hccount,
	i4 *cellcount);
static i4 process_oldq(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST **attrpp,
	bool sample);
static i4 process_newq(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	OPQ_ALIST **attrpp,
	bool sample,
	bool tempt);
static i4 read_process(
	OPQ_GLOBAL *g,
	FILE *inf,
	char **argv);
static void opq_sample(
	OPQ_GLOBAL *g,
	char *utilid,
	OPQ_RLIST *relp,
	OPQ_ALIST *attlst[],
	f8 samplepct);
static bool opq_singres(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp);
static i4 opq_initcollate(
	ADULTABLE *tbl);
int main(
	i4 argc,
	char *argv[]);
static void opt_opq_exit(void);
static void opq_bufalloc(
	PTR utilid,
	PTR *mptr,
	i4 len,
	OPQ_DBUF *buf);
static i4 update_row_and_page_stats(
	OPQ_GLOBAL *g,
	OPQ_RLIST *relp,
	i4 rows,
	i4 pages,
	i4 overflow);
static bool collatable_type(
	i4 datatype);

/*
**    UNIX mkming hints.
**
NEEDLIBS =	OPFLIB SHQLIB ULFLIB COMPATLIB 
**
NEEDLIBSW =	SHADFLIB SHEMBEDLIB
**
PROGRAM =	optimizedb
**
OWNER =		INGUSER
**
NO_OPTIM =	dgi_us5
**
*/

/*
** Definition of all global variables owned by this file.
*/
GLOBALREF	i4		uniquecells;	
/* -zu# flag - If there are not too many unique values for a column, then it is
** worth allowing more cells in order to produce an exact histogram.  This
** number, therefore, specifies the number of unique values that the histogram
** is automatically extended to accomodate.
** - seems like a good default, will handle up to 50 unique values (even more 
** if integer domain and some of the values are adjacent)
*/
GLOBALREF	i4		regcells;	
/* - the histogram can contain no more than this number of cells.  Larger
** numbers require more processing time by the query optimizer but, because
** they are more accurate, generally result in more efficient query execution
** plans
** - if there are too many unique values for a histo with uniquecells then do 
** a regular histo with 15 cells
*/
GLOBALREF	OPQ_RLIST	**rellist;
/* pointer to an array of pointers to relation descriptors - one for each
** relation to be processed by OPTIMIZEDB
*/
GLOBALREF	OPQ_ALIST	**attlist;
/* pointer to an array of pointers to attribute descriptors - used within 
** the context of one relation, i.e. one attribute descriptor for each 
** attribute of the relation currently being processed
*/
GLOBALREF	bool		verbose;
/* -zv Verbose. Print information about each column as it is being processed
*/
GLOBALREF	bool		histoprint;
/* -zh Histograms. Print the histogram that was generated for each column
** This flag also implies the -zv flag
*/
GLOBALREF	bool		keyatts;
/* Not used 
*/
GLOBALREF	bool		key_attsplus;
/* -zk In addition to any columns specified for the table, statistics for 
** columns that are keys on the table or are indexed are also generated
*/
GLOBALREF	bool		minmaxonly;

GLOBALREF	bool		opq_cflag;
/* complete flag. 
*/
GLOBALREF	bool		nosetstats;
/* -zy no 'set statistics'
*/
GLOBALREF	OPS_DTLENGTH	bound_length;	
/* The -length command line flag. If not supplied by the user, remains 0; this
** tells optimizedb to figure out the best length for the boundary values.
** Range is 1 to 1950. 
*/
GLOBALREF	bool		sample;
/* boolean indicating if sampling requested.
*/

GLOBALREF bool      pgupdate;   /* TRUE if page and row count
                            ** in iirelation (or equiv.)
                            ** has been requested.
                            */

GLOBALREF	IISQLDA		_sqlda;		/* SQLDA area for all dyna-
						** mically built queries.
						*/

GLOBALREF	 OPQ_GLOBAL	opq_global;	/* global data struct */

GLOBALREF	 i4		opq_retry;	/* serialization error retries
						** count.
						*/

GLOBALREF	 i4		opq_excstat;	/* additional info about
						** exception processed.
						*/

# ifdef xDEBUG
GLOBALREF	 bool		opq_xdebug;	/* debugging only */
# endif /* xDEBUG */
GLOBALREF	VOID (*opq_exit)(VOID);
GLOBALREF	VOID (*usage)(VOID);
FUNC_EXTERN VOID	opt_usage(VOID);

/* define following variables statically so that 
** large stack areas are not used. 
*/
static OPN_PERCENT      exactcount[MAXCELLS+1];
					      /* used to store tuple counts
					      ** for the "exact histogram" i.e.
                                              ** if number of cells greater than
                                              ** number of values in domain */
static OPN_PERCENT      inexactcount[MAXCELLS+MAXCELLS+1];
					      /* used to store tuple counts
                                              ** counts for the inexact 
					      ** histogram */
static OPN_PERCENT      uniquepercell[MAXCELLS+MAXCELLS+1];
					      /* used to store unique value per
                                              ** cell counts for the inexact 
					      ** histogram */

static OPH_CELLS	exacthist;	      /* store histogram values for the
					      ** exact histogram; corresponds
                                              ** to the exactcount array 
					      */
static OPH_CELLS	inexacthist;	      /* store histogram values 
                                              ** for the inexact histogram;
                                              ** corresponds to the 
					      ** inexactcount array
					      */
static OPQ_BUFTYPE      prev_full_val;
					      /* buffer used to store previous
                                              ** element so that duplicates can
                                              ** be checked for */

GLOBALREF OPQ_DBUF	opq_sbuf;             /* buffer for dynamic SQL queries
					      ** (statements)
					      */
static 	OPQ_DBUF	opq_rbuf	      = {0, 0};
					      /* buffer for dynamic SQL queries
					      ** (results)
					      */
static 	OPQ_DBUF	opq_rbuf1	      = {0, 0};
					      /* another buffer for dynamic SQL
					      ** queries (converted to hist)
					      */
static 	OPQ_DBUF	opq_rbuf2	      = {0, 0};
					      /* Toss holder for -zfq queries
					      ** that need hex sorter
					      */
static  OPQ_DBUF	opq_sbuf1	      = {0, 0};
					      /* buffer for constructing comp.
					      ** histogram select expr.
					      */
static  OPQ_DBUF	opq_cbuf1	      = {0, 0};
					      /* buffer for constructing comp.
					      ** histogram select expr.
					      */
static  OPQ_DBUF	opq_cbuf2	      = {0, 0};
					      /* buffer for constructing comp.
					      ** histogram select expr.
					      */
static	char		opq_line[OPQ_LINE_SIZE];
					      /* Buffer for reading lines from
					      ** input file (-i)
					      */
static	OPQ_IO_VALUE	opq_value_buf;
					      /* Buffer for converting boundary
					      ** values to and from printable
					      ** format
					      */
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
static	f4		*freqvec;	      /* ptr to array of counters
					      ** indicating number of values
					      ** having "i" number of dups
					      ** (used for enhanced distinct
					      ** value estimation technique)
					      */
static	i4		maxfreq;	      /* highest duplicate count */
static	i4		exinexcells;	      /* target number of exact
					      ** cells in an inexact histogram
					      ** (10% of regcells). */
typedef struct _RCARRAY {
    i4		nrows;			/* no. rows with this value */
    i4		pnrows;			/* no. rows with values preceding
					** this one in current cell */
    i4		puperc;			/* no. unique values preceding 
					** this one in current cell */
    char	*valp;			/* value for this entry */
    char	*pvalp;			/* value for previous entry */
    bool	exactcell;		/* If TRUE, there's already an 
					** exact cell for this value */
} RCARRAY;
static RCARRAY		*hcarray;	      /* ptr to array of high row
					      ** count descriptors */

static  ADULTABLE	*collationtbl;

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

exec sql whenever sqlerror call opq_sqlerror;
exec sql whenever sqlwarning call opq_sqlerror;

/*{
** Name: doatt	- test if statistics required on this attribute
**
** Description:
**      Checks whether attribute descriptor has already been allocated, or if
**      it is required.
**
** Inputs:
**      attlst          list of attribute descriptors already
**                      initialized
**      attno           dmf attribute number which needs
**                      to be tested to see if stats required
**      indx            number of attributes defined so far
**      attkdom         non-zero if this attribute is part
**                      of a key
**
** Outputs:
**	none
**
**	Returns:
**	    TRUE - if attribute descriptor is required
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-nov-86 (seputis)
**	    Initial creation.
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
*/
static bool
doatt(
OPQ_ALIST          *attlst[],
register i4        attno,
i4                 indx,
i4                 attkdom)
{
    bool    ret_val = TRUE;

    if (keyatts || key_attsplus)
    {
	if (ret_val = (attkdom != 0))
	{
	    register OPQ_ALIST	**atpp, **limatpp;

	    for (atpp = &attlst[0], limatpp = &attlst[indx];
		 atpp < limatpp; atpp++)
	    {
		if ((*atpp)->attrno.db_att_id == attno)
		{
		    return((bool)FALSE);
		}
	    }
	}
    }

    return(ret_val);
}

/*{
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
**      attlst          ptr to list of attributes descriptors
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
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	10-jul-89 (stec)
**	    Added user length to column info.
**	29-jan-90 (stec)
**	    Improve error handling for UDTs. When attributes are specified
**	    on the command line and the datatype happens to be a UDT, do not
**	    output an error message when the attcount equals 0.
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
**	    variable dtype, which was only used in the replaced code.
**	18-jan-93 (rganski)
**	    Added bound_length parameter to setattinfo() call. This allows user
**	    to control boundary value lengths with new -length flag.
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
**	1-nov-99 (inkdo01)
**	    If non-histogrammable column (e.g. BLOB) is found in non-column
**	    specific search, skip over it. If found when explicitly requested
**	    with "-a" flag, give E_OP0985 error.
**	5-apr-00 (inkdo01)
**	    Added support for composite histograms.
**	25-apr-01 (inkdo01)
**	    Key columns of composite histos not being read in sequence.
**	24-dec-04 (inkdo01)
**	    Init collation ID.
**      27-Nov-2006 (hanal04) Bug 117147
**          Select column_ingdatatype into :es1_ingtype before using the
**          value held in :es1_ingtype.
**	26-july-10 (toumi01) BUG 124136
**	    By default skip encrypted columns, unless they are named with
**	    the -a flag or unless -ze is specified. (NOTE: for STAR the -a
**	    flag must be used else encrypted columns WILL be included, a
**	    limitation caused by lack of STAR iicolumns support for the
**	    the attribute "column_encrypted".)
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
    DB_ATT_NAME tidp_name_lower;
    DB_ATT_NAME tidp_name_upper;

    STmove("tidp", ' ', sizeof(DB_ATT_NAME), tidp_name_lower.db_att_name);
    STmove("TIDP", ' ', sizeof(DB_ATT_NAME), tidp_name_upper.db_att_name);

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

	parmno = -1;
	att_from_input = FALSE;

	while (argv[++parmno] && !CMalpha(&argv[parmno][0]))
	{   /* go till dbname or null */

	    if (isrelation(argv[parmno], &dummy))
	    {
		if (STcompare((char *)&relp->argname,
			      (char *)&argv[parmno][2])) 
		    continue;

		/*
		** target relation found, so search the list of parameters
                ** immediately following this to get attributes associated
                ** with this relation
                */
		while (isattribute(argv[++parmno]))
		{   /* for each attribute */
		    i4		attcount;   /* number of tuples associated
                                            ** with attribute 
                                            */
		    DB_ATT_STR	atnameptr;  /* Attribute name being analyzed */

		    bool isudt = FALSE;	    /* on if column datatype is UDT */
		    bool ishistogrammable = TRUE; /* OFF if column datatype is 
					    ** not histogrammable */
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

		    ap = (OPQ_ALIST *)getspace((char *)g->opq_utilid,
			(PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));
		    attcount = 0;	    /* number of tuples read */

		    {
			exec sql begin declare section;
			    char	*es_atname;
			    char        es_type[DB_TYPE_MAXLEN+1];
			    char        es_nulls[8+1];
			    i2		es_ingtype;
			    i4          es_len;
			    i4          es_scale;
			    i4          es_collID;
			    i4          es_attrno;
			    char        *es_atnptr;
			    char	*es_rname;
			    char	*es_rowner;
			exec sql end declare section;
			char argname[DB_GW1_MAXNAME+3];
                        
			/* Copy attribute name as found on command line */
			STcopy((char *)&argv[parmno][2], &argname[0]);
			es_rname = (char *)&relp->relname;
			es_rowner = (char *)&relp->ownname;
                        es_atnptr = (char *)&atnameptr;
			es_atname = (char *)&ap->attname;

			/* Perform necessary translations on column name */
			/* Also un-delimit ID if delimited */
			opq_idxlate(g, argname, (char *)&atnameptr,
				    (char *) &ap->delimname);

			if (g->opq_lang.ing_sqlvsn >= OPQ_SQL603 &&
			    g->opq_dbcaps.udts_avail == OPQ_UDTS)
			{
			    /* UDTs may be present */

# ifdef NO_COLLID_SUP_IN_STAR
                          /*   
                          **  Beginning of the special handling for Star DB
                          **  This section should be removed once 
                          **  column_collid is supported by Star database.
                          */
                          if (g->opq_dbms.dbms_type & OPQ_STAR)
                           {
			    exec sql repeated select column_name,
				column_datatype, column_length, column_scale,
				column_nulls, column_sequence,
				column_internal_ingtype
				into :es_atname, :es_type, :es_len, :es_scale,
				:es_nulls, :es_attrno, :es_ingtype
				from iicolumns
				where table_name  = :es_rname and
				      table_owner = :es_rowner and
				      column_name = :es_atnptr;

			    exec sql begin;

				if (es_ingtype < 0)
				    es_ingtype = - es_ingtype;

				if (es_ingtype >= ADD_LOW_USER  &&
				    es_ingtype <= ADD_HIGH_USER
				   )
				{
				    /* We have an UDT */
				    isudt = TRUE; 
				    (VOID) STtrmwhite((char *)&ap->attname);
				    opq_error((DB_STATUS)E_DB_WARN,
					(i4)W_OP095D_UDT, (i4)4, 
					(i4)0, (PTR)&ap->attname,
					(i4)0, (PTR)&relp->relname);
				}
				/* Make sure we can build histos for this type. */
				else if (adi_dtinfo(g->opq_adfcb, es_ingtype,
					&typeflags) || (typeflags & AD_NOHISTOGRAM))
				{
				    /* We have a non-histograammable datatype. */
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
				    isudt = FALSE;
 
				    /*
				    ** Trim trailing white space, convert
				    ** type to lower case.
				    */
				    (VOID) STtrmwhite(es_atname);
				    (VOID) STtrmwhite(es_type);
				    CVlower((char *)es_type);

				    ap->attr_dt.db_length = es_len;
				    ap->attr_dt.db_collID = 0;
				    ap->attrno.db_att_id = es_attrno;
				    STcopy((char *) es_type,
					(char *) &ap->typename);
				    ap->nullable = es_nulls[0];
				    ap->scale = (i2)es_scale;
				    ap->collID = (i2) 0 ;
				    ap->userlen = es_len;
				    /*assume stats present */
				    ap->withstats = (bool)TRUE;

				    /* Translate data type into Ingres 
				    ** internal type
				    */
				    opq_translatetype(g->opq_adfcb, es_type,
						      es_nulls, ap);

				    attcount++;

				}

			    exec sql end;

                           }  
                          else  /* End of the special handling for Star DB */
# endif  /* ifdef NO_COLLID_SUP_IN_STAR   */
                           {
                                
			    exec sql repeated select column_name,
				column_datatype, column_length, column_scale,
				column_collid, column_nulls, column_sequence,
				column_internal_ingtype
				into :es_atname, :es_type, :es_len, :es_scale,
				:es_collID, :es_nulls, :es_attrno, :es_ingtype
				from iicolumns
				where table_name  = :es_rname and
				      table_owner = :es_rowner and
				      column_name = :es_atnptr;

			    exec sql begin;

				if (es_ingtype < 0)
				    es_ingtype = - es_ingtype;

				if (es_ingtype >= ADD_LOW_USER  &&
				    es_ingtype <= ADD_HIGH_USER
				   )
				{
				    /* We have an UDT */
				    isudt = TRUE; 
				    (VOID) STtrmwhite((char *)&ap->attname);
				    opq_error((DB_STATUS)E_DB_WARN,
					(i4)W_OP095D_UDT, (i4)4, 
					(i4)0, (PTR)&ap->attname,
					(i4)0, (PTR)&relp->relname);
				}
				/* Make sure we can build histos for this type. */
				else if (adi_dtinfo(g->opq_adfcb, es_ingtype,
					&typeflags) || (typeflags & AD_NOHISTOGRAM))
				{
				    /* We have a non-histograammable datatype. */
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
				    isudt = FALSE;
 
				    /*
				    ** Trim trailing white space, convert
				    ** type to lower case.
				    */
				    (VOID) STtrmwhite(es_atname);
				    (VOID) STtrmwhite(es_type);
				    CVlower((char *)es_type);

				    ap->attr_dt.db_length = es_len;
				    ap->attr_dt.db_collID = (i2)es_collID;
				    ap->attrno.db_att_id = es_attrno;
				    STcopy((char *) es_type,
					(char *) &ap->typename);
				    ap->nullable = es_nulls[0];
				    ap->scale = (i2)es_scale;
				    ap->collID = (i2)es_collID;
				    ap->userlen = es_len;
				    /*assume stats present */
				    ap->withstats = (bool)TRUE;

				    /* Translate data type into Ingres 
				    ** internal type
				    */
				    opq_translatetype(g->opq_adfcb, es_type,
						      es_nulls, ap);

				    attcount++;

				}

			    exec sql end;
                          }

			}
			else
			{
			    /* UDTs not available */

# ifdef NO_COLLID_SUP_IN_STAR
                         /*   
                         **  Beginning of the special handling for Star DB
                         **  This section should be removed once 
                         **  column_collid is supported by Star database.
                         */
                         if (g->opq_dbms.dbms_type & OPQ_STAR)
                          {
			    exec sql repeated select
				column_name, column_datatype,
				column_length, column_scale,
				column_nulls, column_sequence
				into :es_atname, :es_type, :es_len,
				:es_scale, :es_nulls, :es_attrno
				from iicolumns
				where table_name  = :es_rname and
				      table_owner = :es_rowner and
				      column_name = :es_atnptr;

			    exec sql begin;

				/* Make sure we can build histos for this type. */
				if (adi_dtinfo(g->opq_adfcb, es_ingtype,
					&typeflags) || (typeflags & AD_NOHISTOGRAM))
				{
				    /* We have a non-histograammable datatype. */
				    (VOID) STtrmwhite((char *)&ap->attname);
				    (VOID) STtrmwhite((char *)&es_type);
				    opq_error((DB_STATUS)E_DB_ERROR,
					(i4)E_OP0985_NOHIST, (i4)4,
					(i4)0, (PTR)&ap->attname,
					(i4)0, (PTR)&es_type);
				    ishistogrammable = FALSE;
				}

				/*
				** Trim trailing white space, convert
				** type to lower case.
				*/
				(VOID) STtrmwhite(es_atname);
				(VOID) STtrmwhite(es_type);
				CVlower((char *)es_type);

				ap->attr_dt.db_length = es_len;
				ap->attr_dt.db_collID = (i2) 0 ;
				ap->attrno.db_att_id = es_attrno;
				STcopy((char *) es_type, (char *)&ap->typename);
				ap->nullable = es_nulls[0];
				ap->scale = (i2)es_scale;
				ap->collID = (i2) 0 ;
				ap->userlen = es_len;
				ap->withstats = (bool)TRUE;/*assume stats */

				/* Translate data type into
				** Ingres internal type 
				*/
			        opq_translatetype(g->opq_adfcb, es_type,
						  es_nulls, ap);

				attcount++;

			    exec sql end;
			}   
                       else  /* End of the special handling for Star DB */
# endif  /* ifdef NO_COLLID_SUP_IN_STAR   */
                        { 
			    exec sql repeated select
				column_name, column_datatype,
				column_length, column_scale, column_collid,
				column_nulls, column_sequence
				into :es_atname, :es_type, :es_len,
				:es_scale, :es_collID, :es_nulls, :es_attrno
				from iicolumns
				where table_name  = :es_rname and
				      table_owner = :es_rowner and
				      column_name = :es_atnptr;

			    exec sql begin;

				/* Make sure we can build histos for this type. */
				if (adi_dtinfo(g->opq_adfcb, es_ingtype,
					&typeflags) || (typeflags & AD_NOHISTOGRAM))
				{
				    /* We have a non-histograammable datatype. */
				    (VOID) STtrmwhite((char *)&ap->attname);
				    (VOID) STtrmwhite((char *)&es_type);
				    opq_error((DB_STATUS)E_DB_ERROR,
					(i4)E_OP0985_NOHIST, (i4)4,
					(i4)0, (PTR)&ap->attname,
					(i4)0, (PTR)&es_type);
				    ishistogrammable = FALSE;
				}

				/*
				** Trim trailing white space, convert
				** type to lower case.
				*/
				(VOID) STtrmwhite(es_atname);
				(VOID) STtrmwhite(es_type);
				CVlower((char *)es_type);

				ap->attr_dt.db_length = es_len;
				ap->attr_dt.db_collID = (i2)es_collID;
				ap->attrno.db_att_id = es_attrno;
				STcopy((char *) es_type, (char *)&ap->typename);
				ap->nullable = es_nulls[0];
				ap->scale = (i2)es_scale;
				ap->collID = (i2)es_collID;
				ap->userlen = es_len;
				ap->withstats = (bool)TRUE;/*assume stats */

				/* Translate data type into
				** Ingres internal type 
				*/
			        opq_translatetype(g->opq_adfcb, es_type,
						  es_nulls, ap);

				attcount++;

			    exec sql end;
			}
                       }
		    }

		    if (attcount > 1)
		    {
			opq_error( (DB_STATUS)E_DB_ERROR,
			    (i4)E_OP0914_ATTRTUPLES, (i4)8,
                            (i4)0, (PTR)g->opq_utilid,
			    (i4)0, (PTR)g->opq_dbname, 
			    (i4)0, (PTR)&relp->delimname, 
			    (i4)0, (PTR)&atnameptr);
			/* %s: database %s, table %s, column %s
			**       inconsistent sys catalog
			*/
			(*opq_exit)();
		    }
		    else if (attcount == 0)
		    {
			if (isudt == FALSE && ishistogrammable == TRUE)
			{
			    opq_error( (DB_STATUS)E_DB_ERROR,
				(i4)E_OP0915_NOATTR, (i4)6, 
				(i4)0, (PTR)g->opq_dbname, 
				(i4)0, (PTR)&relp->relname, 
				(i4)0, (PTR)&atnameptr);
			    /* optimizedb: database %s, table %s, column %s
			    **       not found
			    */
			    attlst[ai] = NULL; /* deallocate slot for attr */
			}
		    }
		    else
		    {
			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */

# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}

		if (att_from_input && !key_attsplus)
		{
		    attlst[ai] = NULL;
		    return (TRUE);	/* successfully read attributes
                                        ** from input specification */
		}

		parmno--;		/* relook at the parameter which just
                                        ** failed to see if it is another
                                        ** relation */
	    }
	}
    }

    /*
    ** Relation name not in arg list or no atts 
    ** specified for it or key_attsplus flag o.
    ** This includes composite histograms defined
    ** on indexes.
    */

    overrun = FALSE;

    {
	exec sql begin declare section;
	    char	*es1_atname;
	    char	es1_type[DB_TYPE_MAXLEN+1];
	    char	es1_nulls[8+1];
	    i2		es1_ingtype;
	    i4          es1_len;
	    i4          es1_scale;
	    i4          es1_collID;
	    i4          es1_atno;
	    i4          es1_keysq;
	    char	*es1_rname;
	    char	*es1_rowner;
	    char	es1_encrypted[2];	/* Y/N */
	exec sql end declare section;

        DB_ATT_STR	tempname;

        es1_atname = (char *) &tempname;
	tempname[DB_ATT_MAXNAME] = 0;
	es1_rname = (char *) &relp->relname;
	es1_rowner = (char *) &relp->ownname;

	if (g->opq_lang.ing_sqlvsn >= OPQ_SQL603 &&
	    g->opq_dbcaps.udts_avail == OPQ_UDTS)
	{
	    /* UDTs may be present */

# ifdef NO_COLLID_SUP_IN_STAR
          /*   
          **  Beginning of the special handling for Star DB
          **  This section should be removed once 
          **  column_collid and column_encrypted are supported by Star database.
          */
          if (g->opq_dbms.dbms_type & OPQ_STAR)
           {
	    exec sql repeated select column_name, column_datatype,
		column_length, column_scale, column_nulls,
		column_sequence, key_sequence, column_internal_ingtype
		into :es1_atname, :es1_type, :es1_len, :es1_scale,
		     :es1_nulls, :es1_atno, :es1_keysq, 
		     :es1_ingtype
		from iicolumns
		where table_name  = :es1_rname and
		      table_owner = :es1_rowner
		order by key_sequence;

	    exec sql begin;

		if (es1_ingtype < 0)
		    es1_ingtype = - es1_ingtype;

		if (es1_ingtype >= ADD_LOW_USER  &&
		    es1_ingtype <= ADD_HIGH_USER
		   )
		{
		    /* We have an UDT */
		    (VOID) STtrmwhite((char *)&tempname);
		    opq_error((DB_STATUS)E_DB_WARN,
			(i4)W_OP095D_UDT, (i4)4, 
			(i4)0, (PTR)&tempname,
			(i4)0, (PTR)&relp->relname);
		}				
		else if ((relp->index || relp->comphist || g->opq_comppkey) && 
			(es1_keysq == 0 ||
			MEcmp(tidp_name_lower.db_att_name,
			    (PTR)es1_atname, sizeof(DB_ATT_NAME)) == 0 ||
			MEcmp(tidp_name_upper.db_att_name,
			    (PTR)es1_atname, sizeof(DB_ATT_NAME)) == 0))
		    continue;		/* skip non-keys in indexes/pkeys */
		else if (adi_dtinfo(g->opq_adfcb, es1_ingtype, &typeflags)
			== E_DB_OK && !(typeflags & AD_NOHISTOGRAM) &&
			!overrun && doatt(attlst, es1_atno, ai, es1_keysq))
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
		    else
		    {
			ap  = (OPQ_ALIST *) getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

			/*
			** Trim trailing white space, convert
			** type to lower case.
			*/
			(VOID) STtrmwhite(es1_atname);
			(VOID) STtrmwhite(es1_type);
			CVlower((char *)es1_type);

			STcopy(es1_atname, (char *)&ap->attname);
			/* Unnormalize attribute name, put result in
			** ap->delimname */
			opq_idunorm((char *)&ap->attname,
				    (char *)&ap->delimname);

			ap->attr_dt.db_length = es1_len;
			ap->attr_dt.db_collID = (i2) 0 ;
			ap->attrno.db_att_id = es1_atno;
			STcopy((char *) es1_type, (char *) &ap->typename);
			ap->nullable = es1_nulls[0];
			ap->scale = (i2)es1_scale;
			ap->collID = (i2) 0 ;
			ap->userlen = es1_len;
			ap->withstats = (bool)TRUE; /* assume stats present */

			/* Translate data type into Ingres internal type */
			opq_translatetype(g->opq_adfcb, es1_type, es1_nulls,ap);

			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */
			
# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}
	    exec sql end;
	    exec sql commit;
           }    
          else   /* End of the special handling for Star DB */
# endif  /* ifdef NO_COLLID_SUP_IN_STAR */
           {
	    exec sql repeated select column_name, column_datatype,
		column_length, column_scale, column_collid, column_nulls,
		column_sequence, key_sequence, column_internal_ingtype,
		column_encrypted
		into :es1_atname, :es1_type, :es1_len, :es1_scale,
		     :es1_collID, :es1_nulls, :es1_atno, :es1_keysq, 
		     :es1_ingtype, :es1_encrypted
		from iicolumns
		where table_name  = :es1_rname and
		      table_owner = :es1_rowner
		order by key_sequence;

	    exec sql begin;

		if (es1_ingtype < 0)
		    es1_ingtype = - es1_ingtype;

		if (es1_ingtype >= ADD_LOW_USER  &&
		    es1_ingtype <= ADD_HIGH_USER
		   )
		{
		    /* We have an UDT */
		    (VOID) STtrmwhite((char *)&tempname);
		    opq_error((DB_STATUS)E_DB_WARN,
			(i4)W_OP095D_UDT, (i4)4, 
			(i4)0, (PTR)&tempname,
			(i4)0, (PTR)&relp->relname);
		}				
		else if ((relp->index || relp->comphist || g->opq_comppkey) && 
			(es1_keysq == 0 ||
			MEcmp(tidp_name_lower.db_att_name,
			    (PTR)es1_atname, sizeof(DB_ATT_NAME)) == 0 ||
			MEcmp(tidp_name_upper.db_att_name,
			    (PTR)es1_atname, sizeof(DB_ATT_NAME)) == 0))
		    continue;		/* skip non-keys in indexes/pkeys */
		else if (adi_dtinfo(g->opq_adfcb, es1_ingtype, &typeflags)
			== E_DB_OK && !(typeflags & AD_NOHISTOGRAM) &&
			!overrun && doatt(attlst, es1_atno, ai, es1_keysq) &&
			(opq_global.opq_encstats || es1_encrypted[0] == 'N'))
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
		    else
		    {
			ap  = (OPQ_ALIST *) getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

			/*
			** Trim trailing white space, convert
			** type to lower case.
			*/
			(VOID) STtrmwhite(es1_atname);
			(VOID) STtrmwhite(es1_type);
			CVlower((char *)es1_type);

			STcopy(es1_atname, (char *)&ap->attname);
			/* Unnormalize attribute name, put result in
			** ap->delimname */
			opq_idunorm((char *)&ap->attname,
				    (char *)&ap->delimname);

			ap->attr_dt.db_length = es1_len;
			ap->attr_dt.db_collID = (i2)es1_collID;
			ap->attrno.db_att_id = es1_atno;
			STcopy((char *) es1_type, (char *) &ap->typename);
			ap->nullable = es1_nulls[0];
			ap->scale = (i2)es1_scale;
			ap->collID = (i2)es1_collID;
			ap->userlen = es1_len;
			ap->withstats = (bool)TRUE; /* assume stats present */

			/* Translate data type into Ingres internal type */
			opq_translatetype(g->opq_adfcb, es1_type, es1_nulls,ap);

			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */
			
# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}
	    exec sql end;
            if (opq_global.opq_dbms.dbms_type & OPQ_STAR)
             {
              exec sql commit;
             }
           }
	}
	else
	{
	    /* UDTs not available */

# ifdef NO_COLLID_SUP_IN_STAR
         /*   
         **  Beginning of the special handling for Star DB
         **  This section should be removed once 
         **  column_collid and column_encrypted are supported by Star database.
         */
         if (g->opq_dbms.dbms_type & OPQ_STAR)  
          {
	    exec sql repeated select column_name, column_datatype,
		column_length, column_scale, column_nulls,
		column_sequence, key_sequence, column_ingdatatype
		into :es1_atname, :es1_type, :es1_len,
		:es1_scale, :es1_nulls, :es1_atno, :es1_keysq, :es1_ingtype
		from iicolumns
		where table_name  = :es1_rname and
		      table_owner = :es1_rowner
		order by key_sequence;

	    exec sql begin;
		if ((relp->index || relp->comphist || g->opq_comppkey) && 
			(es1_keysq == 0 ||
			MEcmp(tidp_name_lower.db_att_name, 
			    (PTR)es1_atname, sizeof(DB_ATT_NAME)) == 0 ||
			MEcmp(tidp_name_upper.db_att_name,
			    (PTR)es1_atname, sizeof(DB_ATT_NAME)) == 0))
		    continue;		/* skip non-keys in indexes/pkeys */
		if (adi_dtinfo(g->opq_adfcb, es1_ingtype, &typeflags)
			== E_DB_OK && !(typeflags & AD_NOHISTOGRAM) &&
			!overrun && doatt(attlst, es1_atno, ai, es1_keysq))
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
		    else
		    {
			ap  = (OPQ_ALIST *) getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

			/*
			** Trim trailing white space, convert
			** type to lower case.
			*/
			(VOID) STtrmwhite(es1_atname);
			(VOID) STtrmwhite(es1_type);
			CVlower((char *)es1_type);

			STcopy(es1_atname, (char *)&ap->attname);
			/* Unnormalize attribute name, put result in
			** ap->delimname */
			opq_idunorm((char *)&ap->attname,
				    (char *)&ap->delimname); 

			ap->attr_dt.db_length = es1_len;
			ap->attr_dt.db_collID = (i2) 0 ;
			ap->attrno.db_att_id = es1_atno;
			STcopy((char *) es1_type, (char *) &ap->typename);
			ap->nullable = es1_nulls[0];
			ap->scale = (i2)es1_scale;
			ap->collID = (i2) 0 ;
			ap->userlen = es1_len;
			ap->withstats = (bool)TRUE; /* assume stats present */

			/* Translate data type into Ingres internal type */
			opq_translatetype(g->opq_adfcb, es1_type,es1_nulls, ap);

			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */

# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}
	    exec sql end;
	    exec sql commit;
          }  
         else  /* End of the special handling for Star DB */
# endif  /*  ifdef NO_COLLID_SUP_IN_STAR  */
          {
	    exec sql repeated select column_name, column_datatype,
		column_length, column_scale, column_collid, column_nulls,
		column_sequence, key_sequence, column_encrypted
		into :es1_atname, :es1_type, :es1_len,
		:es1_scale, :es1_collID, :es1_nulls, :es1_atno, :es1_keysq,
		:es1_encrypted
		from iicolumns
		where table_name  = :es1_rname and
		      table_owner = :es1_rowner
		order by key_sequence;

	    exec sql begin;
		if ((relp->index || relp->comphist || g->opq_comppkey) && 
			(es1_keysq == 0 ||
			MEcmp(tidp_name_lower.db_att_name, 
			    (PTR)es1_atname, sizeof(DB_ATT_NAME)) == 0 ||
			MEcmp(tidp_name_upper.db_att_name,
			    (PTR)es1_atname, sizeof(DB_ATT_NAME)) == 0))
		    continue;		/* skip non-keys in indexes/pkeys */
		if (adi_dtinfo(g->opq_adfcb, es1_ingtype, &typeflags)
			== E_DB_OK && !(typeflags & AD_NOHISTOGRAM) &&
			!overrun && doatt(attlst, es1_atno, ai, es1_keysq) &&
			(opq_global.opq_encstats || es1_encrypted[0] == 'N'))
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
		    else
		    {
			ap  = (OPQ_ALIST *) getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

			/*
			** Trim trailing white space, convert
			** type to lower case.
			*/
			(VOID) STtrmwhite(es1_atname);
			(VOID) STtrmwhite(es1_type);
			CVlower((char *)es1_type);

			STcopy(es1_atname, (char *)&ap->attname);
			/* Unnormalize attribute name, put result in
			** ap->delimname */
			opq_idunorm((char *)&ap->attname,
				    (char *)&ap->delimname); 

			ap->attr_dt.db_length = es1_len;
			ap->attr_dt.db_collID = (i2)es1_collID;
			ap->attrno.db_att_id = es1_atno;
			STcopy((char *) es1_type, (char *) &ap->typename);
			ap->nullable = es1_nulls[0];
			ap->scale = (i2)es1_scale;
			ap->collID = (i2)es1_collID;
			ap->userlen = es1_len;
			ap->withstats = (bool)TRUE; /* assume stats present */

			/* Translate data type into Ingres internal type */
			opq_translatetype(g->opq_adfcb, es1_type,es1_nulls, ap);

			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */

# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}
	    exec sql end;
            if (opq_global.opq_dbms.dbms_type & OPQ_STAR)
             {
              exec sql commit;
             }
          }
        }
          /*  UDTs may not available */
    }

    if (overrun)
	return (FALSE);
    else
    {
	if (!key_attsplus && !keyatts || relp->index || relp->comphist ||
						g->opq_comppkey)
	{
	    if ((relp->index || relp->comphist || g->opq_comppkey) && ai <= 1)
	    {
		/* 1 column index - these are useless and disallowed. */
		opq_error(E_DB_WARN, W_OP096A_OPTIX_1ATT, 4,
		    0, (PTR)g->opq_dbname,
		    0, (PTR)&relp->relname);
		return(FALSE);
	    }
	    attlst[ai] = NULL;
	    return (TRUE);
	}
    }

    /* get any atts that are indexed */
    {
	exec sql begin declare section;
	    char	*es2_atname;
	    char        es2_type[DB_TYPE_MAXLEN + 1];
	    char        es2_nulls[8+1];
	    i2		es2_ingtype;
	    i4          es2_len;
	    i4          es2_scale;
	    i4          es2_collID;
	    i4          es2_atno;
	    char	*es2_rname;
	    char	*es2_rowner;
	exec sql end declare section;

	DB_ATT_STR    itempname;

	itempname[DB_ATT_MAXNAME] = 0;
        es2_atname = (char *) &itempname;
	es2_rname = (char *) &relp->relname;
	es2_rowner = (char *) &relp->ownname;

	if (g->opq_lang.ing_sqlvsn >= OPQ_SQL603 &&
	    g->opq_dbcaps.udts_avail == OPQ_UDTS)
	{
	    /* UDTs may be present */

# ifdef NO_COLLID_SUP_IN_STAR
         /*   
         **  Beginning of the special handling for Star DB
         **  This section should be removed once 
         **  column_collid is supported by Star database.
         */
         if (g->opq_dbms.dbms_type & OPQ_STAR)
          {
	    exec sql repeated select distinct c.column_name, c.column_datatype,
		c.column_length, c.column_scale, 
		c.column_nulls, c.column_sequence, c.column_internal_ingtype
		into :es2_atname, :es2_type, :es2_len, :es2_scale,
		:es2_nulls, :es2_atno, :es2_ingtype
		from iicolumns c, iiindexes i, iiindex_columns ic
		where   i.base_name   = :es2_rname and
			i.base_owner  = :es2_rowner and
			c.table_name  = :es2_rname and
			c.table_owner = :es2_rowner and
			i.index_name  = ic.index_name and
			i.index_owner = ic.index_owner and
			c.column_name = ic.column_name and
			c.table_name  = i.base_name and
			c.table_owner = i.base_owner;

	    exec sql begin;

		if (es2_ingtype < 0)
		    es2_ingtype = -es2_ingtype;

		if (es2_ingtype >= ADD_LOW_USER  &&
		    es2_ingtype <= ADD_HIGH_USER
		   )
		{
		    /* We have an UDT */
		    (VOID) STtrmwhite((char *)&itempname);
		    opq_error((DB_STATUS)E_DB_WARN,
			(i4)W_OP095D_UDT, (i4)4, 
			(i4)0, (PTR)&itempname,
			(i4)0, (PTR)&relp->relname);
		}				
		else if (!overrun && doatt(attlst, es2_atno, ai, TRUE))
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
		    else
		    {
			ap = (OPQ_ALIST *)getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

			/*
			** Trim trailing white space, convert
			** type to lower case.
			*/
			(VOID) STtrmwhite(es2_atname);
			(VOID) STtrmwhite(es2_type);
			CVlower((char *)es2_type);

			STcopy(es2_atname, (char *)&ap->attname);
			/* Unnormalize attribute name, put result in
			** ap->delimname */
			opq_idunorm((char *)&ap->attname,
				    (char *)&ap->delimname); 

			ap->attr_dt.db_length = es2_len;
			ap->attr_dt.db_collID = (i2) 0;
			ap->attrno.db_att_id = es2_atno;
			STcopy((char *) es2_type, (char *) &ap->typename);
			ap->nullable = es2_nulls[0];
			ap->scale = (i2)es2_scale;
			ap->collID = (i2) 0 ;
			ap->userlen = es2_len;
			ap->withstats = (bool)TRUE; /* assume stats present */

			/* Translate data type into Ingres internal type */
			opq_translatetype(g->opq_adfcb, es2_type,es2_nulls, ap);

			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */

# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}
	    exec sql end;
          }   
         else   /* End of the special handling for Star DB */
# endif   /* ifdef  NO_COLLID_SUP_IN_STAR  */
          {
	    exec sql repeated select distinct c.column_name, c.column_datatype,
		c.column_length, c.column_scale, c.column_collid, 
		c.column_nulls, c.column_sequence, c.column_internal_ingtype
		into :es2_atname, :es2_type, :es2_len, :es2_scale,
		:es2_collID, :es2_nulls, :es2_atno, :es2_ingtype
		from iicolumns c, iiindexes i, iiindex_columns ic
		where   i.base_name   = :es2_rname and
			i.base_owner  = :es2_rowner and
			c.table_name  = :es2_rname and
			c.table_owner = :es2_rowner and
			i.index_name  = ic.index_name and
			i.index_owner = ic.index_owner and
			c.column_name = ic.column_name and
			c.table_name  = i.base_name and
			c.table_owner = i.base_owner;

	    exec sql begin;

		if (es2_ingtype < 0)
		    es2_ingtype = -es2_ingtype;

		if (es2_ingtype >= ADD_LOW_USER  &&
		    es2_ingtype <= ADD_HIGH_USER
		   )
		{
		    /* We have an UDT */
		    (VOID) STtrmwhite((char *)&itempname);
		    opq_error((DB_STATUS)E_DB_WARN,
			(i4)W_OP095D_UDT, (i4)4, 
			(i4)0, (PTR)&itempname,
			(i4)0, (PTR)&relp->relname);
		}				
		else if (!overrun && doatt(attlst, es2_atno, ai, TRUE))
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
		    else
		    {
			ap = (OPQ_ALIST *)getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

			/*
			** Trim trailing white space, convert
			** type to lower case.
			*/
			(VOID) STtrmwhite(es2_atname);
			(VOID) STtrmwhite(es2_type);
			CVlower((char *)es2_type);

			STcopy(es2_atname, (char *)&ap->attname);
			/* Unnormalize attribute name, put result in
			** ap->delimname */
			opq_idunorm((char *)&ap->attname,
				    (char *)&ap->delimname); 

			ap->attr_dt.db_length = es2_len;
			ap->attr_dt.db_collID = (i2)es2_collID;
			ap->attrno.db_att_id = es2_atno;
			STcopy((char *) es2_type, (char *) &ap->typename);
			ap->nullable = es2_nulls[0];
			ap->scale = (i2)es2_scale;
			ap->collID = (i2)es2_collID;
			ap->userlen = es2_len;
			ap->withstats = (bool)TRUE; /* assume stats present */

			/* Translate data type into Ingres internal type */
			opq_translatetype(g->opq_adfcb, es2_type,es2_nulls, ap);

			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */

# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}
	    exec sql end;
          }

	}
	else
	{
	    /* UDTs not available */
 
# ifdef NO_COLLID_SUP_IN_STAR
          /*   
          **  Beginning of the special handling for Star DB
          **  This section should be removed once 
          **  column_collid is supported by Star database.
          */
          if (g->opq_dbms.dbms_type & OPQ_STAR)
           {
	    exec sql repeated select distinct c.column_name, c.column_datatype,
		c.column_length, c.column_scale,
		c.column_nulls, c.column_sequence
		into :es2_atname, :es2_type, :es2_len, :es2_scale,
		:es2_nulls, :es2_atno
		from iicolumns c, iiindexes i, iiindex_columns ic
		where   i.base_name   = :es2_rname and
			i.base_owner  = :es2_rowner and
			c.table_name  = :es2_rname and
			c.table_owner = :es2_rowner and
			i.index_name  = ic.index_name and
			i.index_owner = ic.index_owner and
			c.column_name = ic.column_name and
			c.table_name  = i.base_name and
			c.table_owner = i.base_owner;

	    exec sql begin;

		if (!overrun && doatt(attlst, es2_atno, ai, TRUE))
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
		    else
		    {
			ap = (OPQ_ALIST *)getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

			/*
			** Trim trailing white space, convert
			** type to lower case.
			*/
			(VOID) STtrmwhite(es2_atname);
			(VOID) STtrmwhite(es2_type);
			CVlower((char *)es2_type);

			STcopy(es2_atname, (char *)&ap->attname);
			/* Unnormalize attribute name, put result in
			** ap->delimname */
			opq_idunorm((char *)&ap->attname,
				    (char *)&ap->delimname); 

			ap->attr_dt.db_length = es2_len;
			ap->attr_dt.db_collID = (i2) 0 ;
			ap->attrno.db_att_id = es2_atno;
			STcopy((char *) es2_type, (char *) &ap->typename);
			ap->nullable = es2_nulls[0];
			ap->scale = (i2)es2_scale;
			ap->collID = (i2) 0 ;
			ap->userlen = es2_len;
			ap->withstats = (bool)TRUE; /* assume stats present */

			/* Translate data type into Ingres internal type */
			opq_translatetype(g->opq_adfcb, es2_type,es2_nulls, ap);

			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */

# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}
	    exec sql end;
           }  
        else   /* End of the special handling for Star DB */
# endif  /* ifdef NO_COLLID_SUP_IN_STAR  */
           {
	    exec sql repeated select distinct c.column_name, c.column_datatype,
		c.column_length, c.column_scale, c.column_collid,
		c.column_nulls, c.column_sequence
		into :es2_atname, :es2_type, :es2_len, :es2_scale,
		:es2_collID, :es2_nulls, :es2_atno
		from iicolumns c, iiindexes i, iiindex_columns ic
		where   i.base_name   = :es2_rname and
			i.base_owner  = :es2_rowner and
			c.table_name  = :es2_rname and
			c.table_owner = :es2_rowner and
			i.index_name  = ic.index_name and
			i.index_owner = ic.index_owner and
			c.column_name = ic.column_name and
			c.table_name  = i.base_name and
			c.table_owner = i.base_owner;

	    exec sql begin;

		if (!overrun && doatt(attlst, es2_atno, ai, TRUE))
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
		    else
		    {
			ap = (OPQ_ALIST *)getspace((char *)g->opq_utilid,
			    (PTR *)&attlst[ai], (u_i4)sizeof(OPQ_ALIST));

			/*
			** Trim trailing white space, convert
			** type to lower case.
			*/
			(VOID) STtrmwhite(es2_atname);
			(VOID) STtrmwhite(es2_type);
			CVlower((char *)es2_type);

			STcopy(es2_atname, (char *)&ap->attname);
			/* Unnormalize attribute name, put result in
			** ap->delimname */
			opq_idunorm((char *)&ap->attname,
				    (char *)&ap->delimname); 

			ap->attr_dt.db_length = es2_len;
			ap->attr_dt.db_collID = (i2)es2_collID;
			ap->attrno.db_att_id = es2_atno;
			STcopy((char *) es2_type, (char *) &ap->typename);
			ap->nullable = es2_nulls[0];
			ap->scale = (i2)es2_scale;
			ap->collID = (i2)es2_collID;
			ap->userlen = es2_len;
			ap->withstats = (bool)TRUE; /* assume stats present */

			/* Translate data type into Ingres internal type */
			opq_translatetype(g->opq_adfcb, es2_type,es2_nulls, ap);

			setattinfo(g, attlst, &ai, bound_length);
			relp->attcount++; /* count of interesting attributes */

# ifdef xDEBUG
			if (opq_xdebug)
			    SIprintf("\n\t\tattribute '%s'\t(%s)\n",
				&ap->attname, &relp->relname);
# endif /* xDEBUG */

		    }
		}
	    exec sql end;
         }
	}
    }

    if (!overrun)
	attlst[ai] = NULL;

    return(!overrun);
}

/*{
** Name: hist_convert	- convert to histogram value
**
** Description:
**      Convert a data value to a histogram value or increment the NULL counter
**
** Inputs:
**      tbuf		    ptr to value to convert
**	attrp		    ptr to attribute descriptor
**	adfcb		    ptr to ADF control block
**
** Outputs:
**      tbuf		    histogram value returned
**      attrp		    ptr to attribute descriptor
**	    attr_dt
**		db_data	
**	    hist_dt
**		db_data	    ptr to histogram value
**
**	Returns:
**	    TRUE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      4-dec-86 (seputis)
**          initial creation
**	17-nov-88 (stec)
**	    Fix minmax statistics bug. Input and output db_data  
**	    pointers in DB_DATA_VALUEs passed to adc_helem need 
**	    to be initialized correctly (translation is done in place).
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	9-nov-05 (inkdo01)
**	    DON'T assign attr_dt addr to hist_dt. They aren't shared.
**	1-oct-08 (toumi01) (bug 120966)
**	    Composite histograms are hex bytes; don't treat as UTF8 chars.
**	17-Mar-2009 (toumi01) b121809
**	    Remove "composite" argument to hist_convert; we now handle
**	    composite histograms as byte and handle byte data correctly
**	    in adc_1helem_rti so we don't have to force the issue.
*/
static bool
hist_convert(
char            *tbuf,
OPQ_ALIST       *attrp,
ADF_CB		*adfcb)
{
    DB_STATUS	status;

    status = adc_helem(adfcb, &attrp->attr_dt, &attrp->hist_dt);

    if (DB_FAILURE_MACRO(status))
	if (adfcb->adf_errcb.ad_errcode == E_AD1050_NULL_HISTOGRAM)
	    return((bool)TRUE);
	else
	    opq_adferror(adfcb, (i4)E_OP091D_HISTOGRAM,
			 (i4)0, (PTR)&attrp->attname, (i4)0, (PTR)0);
	    /* optimizedb: error creating histogram value in %s, code = %d
	    */

    return((bool)FALSE);
}

/*{
** Name: opq_scaleunique	- calculate unique values based on a sample
**
** Description:
**      This routine will estimate the number of unique values in a relation
**      when sampling is used on a subset of the relation.  A ball and cells
**      solution is used, in which iteration will converge on the estimated 
**      unique number.  FIXME - there is probably a faster way to get this
**	estimate but the statistical formulas used here have worked well in
**      other parts of the optimizer, and this is not meant to be a performance 
**      critical routine. 
**
** Inputs:
**      samplesize              number of tuples in the sample
**      sampleunique		number of unique values in the sample
**      totalsize		total number of tuples in the relation
**	totalunique		ptr to a float to be filled in with estimated
**				number of unique values in the whole relation
**
** Outputs:
**	totalunique		a float filled in with estimated number of
**				unique values in the whole relation
**
**	Returns:
**	    status		TRUE if error, else FALSE
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      5-feb-90 (seputis)
**          initial creation
**	07-feb-90 (stec)
**	    integrated and tested.
**	19-jan-1996 (nick)
**	    ensure upper limit for binary chop is greater than the row
**	    count. #70073
**    4-jul-2006 (bolke01) Bug 116331
**          Corrected an error; the tempunique value should be samplesize
**          not totalsize. If totalsize is used, the opn_eprime function
**          ALWAYS returns the tuple count that is passed in.
**          Assume that the table is unique if the 
**	       samplesize == sampleestimate
**	    After correcting the tempunique initialization 
**          the value for the uniqueness is incorrectly set to approx 50%
**          and the code does not set the unique flag correctly.
*/
static bool
opq_scaleunique(
OPO_TUPLES      samplesize,
OPH_DOMAIN      sampleunique,
OPO_TUPLES	totalsize,
OPH_DOMAIN	*totalunique)
{
    OPH_DOMAIN	    lowerunique;    /* lower bound on scaled estimate */
    OPH_DOMAIN	    upperunique;    /* upper bound on scaled estimate */
    OPH_DOMAIN	    tempunique;
    OPH_DOMAIN	    tempinc;
    f8		    lnexp;	    /* MHln(FMIN) for calls to opn_eprime() */

    if ((sampleunique < 1.0)
	||
	(samplesize < sampleunique)
	||
	(totalsize < samplesize))
    {
	/* make sure that the input data is consistent with sampling */
	return((bool)TRUE);
    }

    if (totalsize == samplesize)
    {
	*totalunique = sampleunique;
	return((bool)FALSE);
    }

    *totalunique = (OPH_DOMAIN)0.0;

    tempunique = samplesize;
    lowerunique = 1.0;
    tempinc = 1.0;

    do
    {
    	upperunique = totalsize + tempinc;
	tempinc += tempinc;
    } while (upperunique == tempunique);

    lnexp = MHln(FMIN);		/* for calls to opn_eprime() */

    for(;;)
    {
	/*
	** Binary search for correct estimate of unique values.
	**
	** Our approach is to reverse the question we are trying to
	** solve, i.e., assume certain number of unique values for
	** the whole relation (this is actually what we are trying to
	** determine) and compute the estimate of unique values in the
	** sample (we already have this). Based on the difference
	** between the estimated and actual values we make adjustments
	** and repeat the calculation until the delta between the
	** estimate and the actual value is small enough.
	*/

	OPH_DOMAIN	sampleestimate;

	sampleestimate = opn_eprime(
	    samplesize,		    /* number of balls */
	    totalsize/tempunique,   /* number of balls per cell */
	    tempunique,		    /* estimate no. of unique values (cells) */
	    lnexp);

	if (sampleestimate > sampleunique)
	{
	    /*
	    ** If the estimate is too high then lower the total unique
	    ** value estimate to get closer to correct value.
	    */
	    if (upperunique > tempunique)
		upperunique = tempunique;
	    else
		break;
	}
	else if (sampleestimate < sampleunique)
	{
	    /*
	    ** If the estimate is too low then raise the total unique
	    ** value estimate to get closer to correct value.
	    */
	    if (tempunique > lowerunique)
		lowerunique = tempunique;
	    else
		break;

	    /* 
            ** cannot return more unique values than relation 
            ** Set tempunique to the tuple count
	    */
	    if (lowerunique > totalsize)
            {
                tempunique = totalsize;
		break;
            }
	}
	else
	{
	    /* 
            ** got lucky, return exact match 
            ** If the sample count and unique values are also equal
            ** then we need to set the uniqueness of the table column to 
            ** be 100% unique since with fix for alway setting the 
            ** uniqueness to the tuple count (116331) in place the
            ** estimated tuple count is not correctly set.  
            */
	    if (samplesize == sampleestimate)
            {
                tempunique = totalsize;
	    }
	    break;
	}

	if (upperunique <= lowerunique)
	    break;

	/* new estimate for tuple count */
	tempunique = (upperunique - lowerunique) / 2.0 + lowerunique;
    }

    *totalunique = tempunique;
    return((bool)FALSE);
}

/*{
** Name: opq_jackknife_est - calculate unique values based on a sample
**
** Description:
**      This routine estimates the number of distinct values in a relation
**      when sampling is used on a subset of the relation.  Unlike the 
**	default heuristic technique based on the ever popular balls and 
**	cells algorithm, this routine uses a technique from the recent
**	literature specifically aimed at solving this problem. 
**
** Inputs:
**      samptups                number of tuples in the sample
**      sampunique		number of unique values in the sample
**      actualtups		total number of tuples in the relation
**
** Outputs:
**
**	Returns:
**	    dsj			estimated distinct values in whole 
**				relation using hybrid smoothed jackknife 
**				technique
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-sep-00 (inkdo01)
**          Written.
*/
static OPO_TUPLES
opq_jackknife_est(
OPO_TUPLES      samptups,
OPO_TUPLES      sampunique,
OPO_TUPLES	actualtups)

{
    f8		dsj, d0, chi, gx, hx, x1, x2;
    i4		i, nb;


    /* Use hybrid smoothed jackknife estimator from "Sampling-Based
    ** Estimation of the Number of Distinct Values of an Attribute" (Haas
    ** et al - VLDB'95) abd "A Block Sampling Approach to Distinct Value
    ** Estimation" (Brutlag & Richardson - tech. report no. 355, Univ. of
    ** Washington).
    **
    ** This technique uses a value count frequency vector (i'th entry is 
    ** number of values which occur i times in the sample) computed as the
    ** histograms are built. Since this vector reflects variations in value
    ** counts in the sample population, it offers amore sound basis for 
    ** estimation than the original, purely heuristic technique.
    **
    ** Tests show that the new technique gives better estimates, in particular
    ** for smaller, skewed samples. 
    **
    ** The computations in this technique are highly complex. In this
    ** routine, the computations are broken into discrete subcomputations.
    ** The final result is built at the end. Those with unhealthy levels of
    ** curiosity may refer to the above-cited papers for more details.
    */

    /* Compute the D0 value of the estimate. */

    if (freqvec[0] == 0.0) freqvec[0] = 1.0;	/* fudge */

    x1 = 1.0 - (actualtups - samptups + 1.0) * freqvec[0] 
		/ actualtups / samptups;
    d0 = (sampunique - freqvec[0]/samptups) / x1;
    nb = (i4) (actualtups/d0 + 0.5);

    /* Compute the g(x) value of the estimate. */

    for (i = 1, gx = 0.0; i < (i4)samptups; i++)
     gx += 1.0 / (actualtups - ((f8)(nb+i)) - samptups + 1.0);

    /* Compute the h(x) value of the estimate. */

    for (i = nb-1, hx = 1.0; i >= 0; i--)
     hx *= (actualtups - samptups - (f8)i)
		/ (actualtups - (f8)i);

    /* Compute the chi value of the estimate. */

    for (i = 0, x1 = 0.0; i < maxfreq; i++)
     x1 += ((f8)i) * ((f8)(i+1)) * freqvec[i];
    chi = d0 / actualtups - 1.0;
    x2 = (actualtups - 1.0) / actualtups * d0 / samptups
		/ (samptups - 1.0);
    chi += x2 * x1;

    /* Compute the final unique value estimate. */

    dsj = (sampunique + actualtups * gx * hx * chi)
		/ (1.0 - (actualtups - ((f8)nb) - samptups + 1.0)
		* freqvec[0] / (samptups * actualtups));

    return(dsj);
}



/*{
** Name: opq_setstats	- sets statistics flag.
**
** Description:
**	A flag is set in a catalog to indicate that stats exist.
**
** Inputs:
**	g	    ptr to global data struct
**	  opq_lang  language info
**      relp        ptr to relation descriptor
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Updates catalogs.
**
** History:
**      22-mar-90 (stec)
**          initial creation
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	20-sep-93 (rganski)
**	    Delimited ID support. Use relp->delimname instead of relp->relname
**	    in set statistics statement.
**  21-Jan-05 (nansa02)
**      Table name with delimited identifiers should be enclosed within '"'.
**      (Bug 113786)
**	28-Jan-2005 (schka24)
**	    Back out double delimiting, doesn't work real well...
**	10-aug-05 (inkdo01)
**	    Only do "set statistics" if stats flag isn't already on (saves
**	    locking iirelation).
**	11-oct-05 (inkdo01)
**	    Above was wrong - OPF depends on "set statistics" to determine
**	    when to reload stats.
*/
static VOID
opq_setstats(
OPQ_GLOBAL	*g,
OPQ_RLIST       *relp)
{
    if (g->opq_lang.ing_sqlvsn >= OPQ_SQL600)
    {
	exec sql begin declare section;
	    char    *stmt;
	exec sql end declare section;

	opq_bufalloc((PTR)g->opq_utilid, (PTR *)&stmt,
	    (i4)(sizeof("set statistics ") + DB_GW1_MAXNAME+2), &opq_sbuf);

	(VOID) STprintf(stmt, "set statistics %s",
	    &relp->delimname);

	exec sql execute immediate :stmt;
    }
    else if (g->opq_dbcaps.cat_updateable & OPQ_TABLS_UPD)
    {
	exec sql begin declare section;
	    char	es_yes[8 + 1];
	    char    *rname;
	    char    *rowner;
	exec sql end declare section;

	es_yes[0] = EOS;
	STcopy("Y", es_yes);
	rname   = (char *)&relp->relname;
	rowner  = (char *)&relp->ownname;

	exec sql repeated update iitables
	    set table_stats = :es_yes
	    where   table_name  = :rname and
		    table_owner = :rowner;

# ifdef FUTURE_USE
	exec sql repeated update iiphysical_tables
	    set table_stats = :es_yes
	    where   table_name  = :rname and
		    table_owner = :rowner;
# endif /* FUTURE_USE */
    }

    /* Indicate that stats are set */
    relp->statset = (bool)TRUE;
}

/*{
** Name: write_histo_tuple - insert a single histogram tuple in iihistogram
**
** Description:
**	This routine inserts as much of the histogram for an attribute as fits
**	into a single iihistogram tuple into the system catalog iihistogram or
**	iihistograms, using a repeated insert statement.
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_dbcaps	    database capabilities struct
**	    cat_updateable  mask showing which standard catalogs
**			    are updateable
**      relp                ptr to relation descriptor
**      attrp               ptr to attribute descriptor upon
**                          which the histogram is built
**	sequence	    sequence number for this tuple
**	histdata	    ptr to histogram data for this tuple
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Updates the iihistogram or iihistograms catalog.
**
** History:
**	18-jan-93 (rganski)
**	    Initial creation from portion of insert_histo().
*/
static VOID
write_histo_tuple(
OPQ_GLOBAL	*g,
OPQ_RLIST       *relp,
OPQ_ALIST       *attrp,
i4	      	sequence,
PTR		histdata)
{
    exec sql begin declare section;
    i2		  	es2_sequence;	/* sequence number of tuple */
    DB_DATA_VALUE	es2_hist;	/* Pointer to histogram data */
    exec sql end declare section;

    es2_sequence = 		sequence;
    es2_hist.db_datatype = 	DB_BYTE_TYPE;	/* SQL char type */
    es2_hist.db_length = 	DB_OPTLENGTH;	/* size of histogram tuple */
    es2_hist.db_prec = 		0;
    es2_hist.db_data = 		histdata;

    if (g->opq_dbcaps.cat_updateable & OPQ_HISTS_UPD)
    {
	exec sql begin declare section;
	char    	*rname;
	char    	*rowner;
	char		*atrname;
	exec sql end declare section;

	rname   = (char *)&relp->relname;
	rowner  = (char *)&relp->ownname;
	atrname = (char *)&attrp->attname;

	exec sql repeated insert into iihistograms
	    (table_name, table_owner, column_name, text_sequence, text_segment)
	    values (:rname, :rowner, :atrname, :es2_sequence, :es2_hist); 
    }
    else
    {
	exec sql begin declare section;
	i4		reltid;
	i4		reltidx;
	i2		atrno;
	exec sql end declare section;

	reltid  = relp->reltid;
	reltidx = relp->reltidx;
	atrno   = attrp->attrno.db_att_id;

	exec sql repeated insert into iihistogram
	    (htabbase, htabindex, hatno, hsequence, histogram)
	    values (:reltid, :reltidx, :atrno, :es2_sequence, :es2_hist); 
    }
}

/*{
** Name: insert_iihistogram - insert histogram for attribute into iihistogram
**
** Description:
**	This routine inserts the histogram for an attribute into the system
**	catalog iihistogram. The histogram itself is inserted as a stream of
**	binary data, first the counts, then the boundary values. The stream is
**	divided into chunks of 228 bytes and inserted into the "histogram"
**	column, which is defined as char(228). The stream is padded with 0s if
**	it is not an exact multiple of 228. These chunks are accompanied by the
**	table and attribute identifiers, and a sequence number for each tuple.
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_utilid	    name of utility from which this proc is called
**        opq_dbname        name of database
**	  opq_dbcaps	    database capabilities struct
**	    cat_updateable  mask showing which standard catalogs
**			    are updateable
**	  opq_adfcb	    ptr to ADF control block
**      relp                ptr to relation descriptor
**      attrp               ptr to attribute descriptor upon
**                          which the histogram is built
**      nunique             number of unique values in the column
**      intervalp           array of interval values delimiting
**                          histogram cells
**      cell_count          array of cell counts associated
**                          with histogram cell intervals
**      max_cell         maximum cell number in histogram
**      numtups             number of tuples in relation
**	pages		    number of prime pages
**	overflow	    number of overflow pages
**	null_count	    null count
**	normalize	    flag indicating whether normalization is to be done
**	exact_histo	    a boolean indicating if exact histogram is being
**			    stored.
**	unique_flag	    a character specifying the value of the unique
**			    flag to be stored, ignore if 0.
**	complete	    boolean value for the complete flag to be stored.
**	irptfactor	    repeat factor read in from a text file. When
**			    expected to be generated by this routine, this arg
**			    should be set to zero.
**	version		    Statistics version, as stored in
**			    iistatistics.sversion. If NULL, there is no
**			    version, and character set statistics are not
**			    stored.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Updates the iihistogram or iihistograms catalog.
**
** History:
**	18-jan-93 (rganski)
**	    Initial creation from portion of insert_histo().
**	    Changed algorithm for inserting histogram data into tuples, since
**	    now the boundary values are now contiguous.
**	    Added allocation, initialization, and storage of character set
**	    statistics arrays. For now, they are initialized to values which
**	    give same results as before: 11 for charnunique, and 1.0 for
**	    chardensity.
**	    Added version param; char set statistics are only added if there is
**	    a version ID.
**	26-apr-93 (rganski)
**	    Moved creation of character set statistics arrays to process() and
**	    read_process().
**	8-jan-96 (inkdo01)
**	    Added support of repetiton factor per cell values.
*/
static VOID
insert_iihistogram(
OPQ_GLOBAL	   *g,
OPQ_RLIST          *relp,
OPQ_ALIST          *attrp,
OPH_CELLS	   intervalp,
OPN_PERCENT	   cell_count[],
i4                 max_cell,
char		   *version)
{
    i4			sequence;	/* sequence number for the iihistogram
					** tuple */
    OPS_DTLENGTH	histlength;	/* length of a histogram element */
    i4		count_size;	/* size in bytes of array containing
					** the counts of tuples in histogram
					** cells */
    i4	    	total_size;	/* size in bytes of all histogram
					** data i.e. counts and cell boundaries
					*/ 
    i4	    	written;	/* size of all histogram data 
					** written to iihistogram relation
					** so far */

    /* Sequence numbers in standard catalogs start at 1, in Ingres at 0.
    */
    if (g->opq_dbms.dbms_type & (OPQ_INGRES | OPQ_RMSGW))
	/* Ingres case */
	sequence = 0;
    else if (g->opq_dbms.dbms_type & OPQ_STAR)
	/* STAR case */
	sequence = 1;
    else
	/* Gateway case */
	sequence = 1;

    histlength = attrp->hist_dt.db_length;

    count_size = (max_cell + 1) * sizeof(OPN_PERCENT);
    total_size = count_size + (max_cell + 1) * (histlength + 
	sizeof(OPN_PERCENT));	/* add boundaries AND per-cell rep factors */

    /* If there is a version ID and attribute is a character type, add size of
    ** character set statistics arrays to total_size */
    if (version != NULL && (attrp->hist_dt.db_datatype == DB_CHA_TYPE ||
			    attrp->hist_dt.db_datatype == DB_BYTE_TYPE))
	total_size += histlength * (sizeof(i4) + sizeof(f4));

    for (written = 0; written < total_size; written += DB_OPTLENGTH)
    {	                        
	/* for each iihistogram tuple */
	PTR		histdata;	/* ptr to data to write */

	/* Write counts first. See if there are enough counts left to fill
	** a whole tuple.
	*/
	if ((count_size - written) >= (i4)DB_OPTLENGTH)
	{
	    /* Write a histogram tuple full of counts */
	    histdata = (PTR)&cell_count[written / 4];
	}
	else /* not enough counts left to fill tuple */
	{
	    DB_OPTDATA	tempbuf;	/* Tuple-sized buffer for copying
					** histogram data */
	    i4	    	data_offset; 	/* offset in tempbuf of next 
					** available byte */

	    data_offset = 0;
	    histdata = (PTR)tempbuf;	/* save base of tuple to write */

	    if (written < count_size)
	    {
		/* Part of the count array and part of the value array 
		** need to be written in one tuple. Copy remainder of
		** counts to tempbuf. 
		*/
		/* number of bytes remaining in count array */
		data_offset = count_size - written;

		/* copy remainder of counts to beginning of tempbuf */
		MEcopy((PTR)&cell_count[written/4], data_offset,
		       (PTR)tempbuf);
	    }

	    if (total_size - written < (i4)DB_OPTLENGTH)
	    {
		/* Remaining data will not fill up tuple.
		** Copy remaining data and pad with zeros.
		*/
		MEcopy((PTR)&intervalp[written + data_offset - count_size],
		       total_size - written - data_offset,
		       (PTR)(tempbuf + data_offset));
		MEfill(written + DB_OPTLENGTH - total_size,
		       0, 
		       &tempbuf[total_size - written]);
	    }
	    else
		/* There is at least enough remaining data to fill tuple */
		if (data_offset == 0)
		    /* We are writing only boundary values now and there
		    ** are at least one tuple's worth, so we can just point
		    ** histdata at the current point in the value array.
		    */
		    histdata = (PTR)&intervalp[written - count_size];
		else
		    /* We are adding after count values: copy boundary
		    ** values up to end of tempbuf
		    */
		    MEcopy(
			(PTR)&intervalp[written + data_offset - count_size],
			DB_OPTLENGTH - data_offset,
			(PTR)(tempbuf + data_offset));
	} /* not enough counts... */

	write_histo_tuple(g, relp, attrp, sequence, histdata);

	sequence++;

    } /* for */

    return;
}

/*{
** Name: insert_histo	- insert histogram element into system relations
**
** Description:
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_utilid	    name of utility from which this proc is called
**        opq_dbname        name of database
**	  opq_dbcaps	    database capabilities struct
**	    cat_updateable  mask showing which standard catalogs
**			    are updateable
**	  opq_adfcb	    ptr to ADF control block
**      relp                ptr to relation descriptor
**      attrp               ptr to attribute descriptor upon
**                          which the histogram is built
**      nunique             number of unique values in the column
**      intervalp           array of interval values delimiting
**                          histogram cells
**      cell_count          array of cell counts associated
**                          with histogram cell intervals
**      max_cell         maximum cell number in histogram
**      numtups             number of tuples in relation
**	pages		    number of prime pages
**	overflow	    number of overflow pages
**	null_count	    null count
**	normalize	    flag indicating whether normalization is to be done
**	exact_histo	    a boolean indicating if exact histogram is being
**			    stored.
**	unique_flag	    a character specifying the value of the unique
**			    flag to be stored, ignore if 0.
**	complete	    boolean value for the complete flag to be stored.
**	irptfactor	    repeat factor read in from a text file. When
**			    expected to be generated by this routine, this arg
**			    should be set to zero.
**	version		    Statistics version, to be stored in
**			    iistatistics.sversion. If NULL, there is no
**			    version, and blanks will be copied to sversion.
**	fromfile	    flag indicating whether stats originated in a 
**			    statdump file (-i <filename>). This determines 
**			    whether "set statistics" is done here or deferred.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Updates the iihistograms and iistats catalogs.
**
** History:
**      3-dec-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	10-jul-89 (stec)
**	    Changed code to reflect new interface to adc_lenchk.
**	10-jul-89 (stec)
**	    Added user length, scale to column info displayed.
**	24-oct-89 (stec)
**	    Fix bug 8469. When sample stats are created, repetition factor
**	    is off. The solution for now, even though it is not an ideal one,
**	    will be to scale both the repetition factor and unique values
**	    up when the created histogram is inexact. In connection with that
**	    a new parameter is added to the argument list (exact_histo).
**	    The way we do it is : find how much bigger is original relation
**	    compared to sample relation. Then find square root of this number
**	    and use this as scaling factor. This is justified because following
**	    equation should hold at all times:
**		number of tuples = repetition factor * number of unique values
**	    so
**				      ________		  _______
**		nts * nto/nts = rf * Vnto/nts	*   nu * Vnto/nts
**			^		^		    ^
**			|		|		    |
**		scaling factor squared	|		    |
**				    scaling factor (sf)	    sf
**
**	    where
**		 _____
**		V	 - square root
**		nts	 - number of tuples in sample relation
**		nto	 - number of tuple in sampled relation
**		rf	 - repetition factor
**		nu	 - number of unique values
**	    [Don't get stuck reading this stuff - it was replaced by ye olde
**	    "ball and cell" technique at a later date (inkdo01 8/1/96).]
**
**	12-dec-89 (stec)
**	    Fix a porting problem: DB_TEXT_STRING should be aligned.
**	05-jan-89 (stec)
**	    Correct Common SQL problem; pass an initialized char string
**	    rather than use char(date('now')).
**	06-feb-90 (stec)
**	    Change handling of the unique flag when "-i" option specified.
**	    When read in from a file, store as specified by the user, if
**	    doesn't conform to the rules issue a warning.
**	    Changed way scaling is done (described in comment above). We
**	    will now use OPF code to estimate it for us (see new proc
**	    opq_scaleunique above).
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	    Changed the way statistics flag is set: for INGRES and gateways
**	    it's set after stats for each column get created, for STAR only
**	    after the first and last column. Moved some code to opq_setstats().
**	16-may-90 (stec)
**	    Fixed bug 21556 by displaying relp->ntups when sampling, numtups
**	    otherwise.
**	28-jun-90 (stec)
**	    Modified code to start sequence numbering for iihistograms  for
**	    STAR with 1.
**	06-jul-90 (stec)
**	    In case of STAR we will commit before calling opq_setstats().
**	    Update queries for iistats and iihistograms get routed over a
**	    different LDB server association than update queries for "set
**	    statistics". This would, at this time, cause a problem in a
**	    VAXcluster environment, in which 2PC feature in not supported.
**	22-jan-91 (stec)
**	    Added casting to avoid porting problems. Changed handling of the
**	    complete flag.
**	18-mar-91 (stec)
**	    Added irptfactor argument to be used when statistics are read in
**	    from a file (see description above).
**	09-nov-92 (rganski)
**	    Added new columns to insert statements for iistats and
**	    iistatistics: for iistats, added values for new columns
**	    column_domain, is_complete, stat_version, and hist_data_length; for
**	    iistatistics, added values for sversion and shistlength. This is
**	    part of the Character Histogram Enhancements project.
**	18-jan-93 (rganski)
**	    Changed type of intervalp to OPH_CELLS. Changed to use pointer
**	    arithmetic to dereference intervalp, instead of array reference.
**	    Moved declaration and initialization of histlen so that more code
**	    could use it.
**	    Changed algorithm for inserting histogram data into tuples, since
**	    now the boundary values are now contiguous, and moved this code to
**	    separate new function insert_iihistogram().
**	    Added new version parameter, which gets inserted into
**	    iistatistics.sversion.
**	    Moved statistics-printing code to opq_print_stats() in opqutils.sc.
**	15-apr-93 (ed)
**	    Bug 49864 - use symbolic value for default domain OPH_DOMAIN.
**	8-jan-96 (inkdo01)
**	    Added support for per-cell repetition factor computation.
**	29-apr-98 (inkdo01)
**	    Added support of "-zlr" to reuse existing repetition factor.
**	9-june-99 (inkdo01)
**	    Added support of "-o <filename>" option to write stats to file, but
**	    NOT the catalog.
**	14-jul-00 (inkdo01)
**	    Recompute per-cell rept facts for exact histos, too.
**	13-sep-00 (inkdo01)
**	    Added support of "-zdn" parameter, which uses new (and improved) 
**	    algorithm for estimating number of distinct values from a sample.
**	15-apr-05 (inkdo01)
**	    Add support for 0-cell histograms (column is all null).
**	11-oct-05 (inkdo01)
**	    Non-Star tables have "set statistics" deferred 'til end of relp
**	    loop in main() to minimize locking interference.
**      29-Nov-2006 (kschendel)
**          Delete in the same order we insert, ie histogram then stats.
**          No big deal since we now force serializable, but might help
**          avoid deadlock a bit better especially when no stats existed.
**	17-oct-05 (inkdo01)
**	    Add "fromfile" parm so stat stats is NOT deferrred for 
**	    "-i <filename>".
**	26-oct-05 (inkdo01)
**	    Set "sample" more precisely to avoid -zfq cases that create 
**	    temp tables.
**      08-jan-2008 (huazh01)
**          avoid division-by-zero while normalizing the histogram.
**          bug 119712.
*/
static VOID
insert_histo(
OPQ_GLOBAL	   *g,
OPQ_RLIST          *relp,
OPQ_ALIST          *attrp,
OPO_TUPLES	   nunique,
OPH_CELLS	   intervalp,
OPN_PERCENT	   cell_count[],
i4                 max_cell,
i4            numtups,
i4            pages,
i4            overflow,
OPO_TUPLES	   null_count,
bool		   normalize,
bool		   exact_histo,
char		   unique_flag,
bool		   complete,
f8		   irptfactor,
char		   *version,
bool		   fromfile)
{
    exec sql begin declare section;
	char    *rname;
	char    *rowner;
	char	*atrname;
	i2	atrno;
	i4	reltid;
	i4	reltidx;
	f8	statrepf;
    exec sql end declare section;

    OPO_TUPLES          reptfact;	/* number of tuples on average that
                                        ** have the same attribute value */
    bool                unique;		/* TRUE if 90% of values in relation
                                        ** are unique */
    char		charunique[2];  /* Y/N - indicates whether unique */
    i4	                domain;
    bool		sample;		/* true when sampling */
    OPS_DTLENGTH	histlen;        /* length of a histogram element */
    i4			i;

    histlen = attrp->hist_dt.db_length;

    sample = g->opq_sample;

    /* normalize the histogram with respect to the number of tuples 
    ** so that histogram integrates to 1; i.e., change pure tuple counts
    ** into proportions of relation cardinality 
    */
    if (normalize)
    {
	for (i = 1; i <= max_cell; i++)
	{
	    if (cell_count[i] == 0.0) uniquepercell[i] = 0.0;
	     else if (exact_histo == (bool)TRUE) uniquepercell[i] = 
							cell_count[i];
	     else if (uniquepercell[i] > 0.0) 
                  uniquepercell[i] = cell_count[i] / uniquepercell[i];
			/* compute per-cell rep factors while we're at it */
	    cell_count[i] /= numtups;
	}
    }

    reptfact = ((OPO_TUPLES) numtups) / nunique;

    if (sample == (bool)TRUE)
    {
	OPO_TUPLES	rf1 = reptfact;

	if (exact_histo == (bool)TRUE)
	{
	    /*
	    ** Exact histogram case.
	    ** We assume that number of unique values is
	    ** correct and use number of rows in the sampled
	    ** relation to calculate the repetition factor.
	    */
	    reptfact = ((OPO_TUPLES) relp->ntups) / nunique;
	}
	else if (opq_global.opq_newrepfest)
	{
	    /*
	    ** New distinct value estimation algorithm is to be used.
	    */
	    nunique = opq_jackknife_est((OPO_TUPLES)numtups, 
		nunique,
		(OPO_TUPLES)relp->ntups);
	    reptfact = (OPO_TUPLES) relp->ntups / nunique;
	}
	else
	{
	    /*
	    ** Inexact histogram case.
	    ** We need to scale the repetition factor
	    ** and number of unique values using the old heuristic.
	    */
	    DB_STATUS	stat;
	    OPH_DOMAIN	totalunique;

	    /*
	    ** We want to use numtups here since this is actual
	    ** no. of rows retrieved; nsample doesn't take into
	    ** account the fact that there may be duplicates among
	    ** generated TIDs, so uniqueness characteristics of
	    ** the data would be skewed, if we used it.
	    */
	    stat = opq_scaleunique(
		(OPO_TUPLES)numtups,
		(OPH_DOMAIN)nunique,
		(OPO_TUPLES)relp->ntups,
		(OPH_DOMAIN *)&totalunique);

	    if (stat == TRUE)
	    {
		opq_error( (DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0972_SCALE_ERR, (i4)6, 
		    (i4)sizeof(numtups), (PTR)&numtups,
		    (i4)sizeof(nunique), (PTR)&nunique,
		    (i4)sizeof(relp->ntups), (PTR)&relp->ntups);
		(*opq_exit)();
	    }

	    nunique  = (OPO_TUPLES)totalunique;
	    reptfact = ((OPO_TUPLES) relp->ntups) / nunique;

	}

	/* Finally, recompute per-cell rept. factors. */
	rf1 = reptfact/rf1;
	for (i = 1; i <= max_cell; i++)
	 uniquepercell[i] *= rf1;	/* fiddle per-cell stats, too */
    }

    /* See if we are dealing with statistics read from a text file. */
    if (irptfactor != 0.0)
    {
	f8  min_er, lo, hi;
	
	/*
	** Calculate min and max acceptable values
	** for the repetition factor.
	*/
	min_er = reptfact;
	if (reptfact > nunique)
	{
	    min_er = nunique;
    	}
        if (pgupdate)
            lo = hi = numtups;
    	else
	    lo = hi = relp->ntups;
	lo -= min_er;
	hi += min_er;
	/* Normalize */
	lo /= nunique;
	hi /= nunique;

	/*
	** Check if repetition factor read from file is
	** within acceptable range.
	*/
	if (irptfactor < lo || irptfactor > hi)
	{
	    /* Warn user that he is making things inconsistent. */
	    opq_error( (DB_STATUS)E_DB_WARN,
		(i4)W_OP0991_INV_RPTFACTOR, (i4)6, 
		(i4)sizeof(irptfactor), (PTR)&irptfactor,
		(i4)0, (PTR)&attrp->attname,
		(i4)0, (PTR)&relp->relname);
	}

	/*
	** We want to store the repetition factor value
	** from the text file, not the calculated one.
	*/
	reptfact = irptfactor;
    }

    /*
    ** Now that the per-cell repetition factors have been computed into 
    ** the static array, copy it to the end of the histogram cell data for 
    ** insertion into the catalog.
    */
    {
	u_i4	extra;
	
	if (attrp->hist_dt.db_datatype == DB_CHA_TYPE ||
	    attrp->hist_dt.db_datatype == DB_BYTE_TYPE)
		extra = histlen * (sizeof(i4) + sizeof(f4));
	 else extra = 0;

	if (max_cell >= 0)
	    MEcopy ((PTR)&uniquepercell, (max_cell+1) * sizeof(OPN_PERCENT),
		(PTR)(intervalp + (max_cell+1) * histlen + extra));
    }
	
    /*
    ** The correct value of unique flag needs to be computed
    ** differently for sample case and non-sample case.
    */
    {
	i4	rcount;

	if (sample == (bool)TRUE)
	    rcount = relp->ntups;
	else
	    rcount = numtups;	

	if (nunique >= OPH_UFACTOR * (OPO_TUPLES) rcount)
	{
	    unique = TRUE;	    	/* if number of unique values is
					** at least 90% of the number of tuples
					** then consider the relation to be
					** unique */
	    STcopy("Y", charunique);
	}
	else
	{
	    unique = FALSE;
	    STcopy("N", charunique);
	}
    }

    if (unique_flag != 0)
    {
	/*
	** We are reading from input and the value
	** specified by the user should be stored.
	*/
	if (unique_flag != charunique[0])
	{
	    /* Warn user that he is making things inconsistent. */
	    opq_error( (DB_STATUS)E_DB_WARN,
		(i4)W_OP095F_WARN_UNIQUE, (i4)6, 
		(i4)1, (PTR)&unique_flag,
		(i4)0, (PTR)&attrp->attname,
		(i4)0, (PTR)&relp->relname);

	    /*
	    ** Now replace the computed value with
	    ** user specified value.
	    */

	    charunique[0] = unique_flag;

	    if (unique_flag == 'Y')
		unique = TRUE;
	    else
		unique = FALSE;
	}
    }

    domain	= OPH_DDOMAIN;

    rname   = (char *)&relp->relname;
    rowner  = (char *)&relp->ownname;
    atrname = (char *)&attrp->attname;
    atrno   = attrp->attrno.db_att_id;
    reltid  = relp->reltid;
    reltidx = relp->reltidx;

    if (g->opq_lvrepf)
    {
	/* If "-zlr", read existing rep factor first (if there) and reset
	** reptfact with it. */

	exec sql whenever sqlwarning continue;		/* must tolerate 0 rows */
	if (g->opq_dbcaps.cat_updateable & OPQ_STATS_UPD)
	{
	    exec sql repeated select rept_factor into :statrepf from iistats
		    where table_name	= :rname
		    and   table_owner	= :rowner
		    and   column_name	= :atrname;
	}
	else
	{
	    exec sql repeated select sreptfact into :statrepf from iistatistics
		    where stabbase	= :reltid
		    and   stabindex	= :reltidx
		    and   satno		= :atrno;
	}
	exec sql whenever sqlwarning call opq_sqlerror;	/* revert to default */
	if (sqlca.sqlerrd[2]) 
	{
	    reptfact = statrepf;	/* if a row existed, use it 
					** to override computed repfactor */
	    nunique = relp->ntups / reptfact;	/* rep fact & nunique must 
					** be consistent */
	}
    }

    /* print out info if requested */
    if (verbose || histoprint || opq_global.opq_env.ofid)
    {
	FILE		*outf = opq_global.opq_env.ofid;
	i4		rcount;
	bool		verbose_param = TRUE;

	/* Set row count */
	if (sample == (bool)TRUE)
	    rcount = relp->ntups;
	else
	    rcount = numtups;	

	if (histoprint || outf)
	    verbose_param = FALSE;	/* Print all statistics if -zh or -o */

	opq_print_stats(g, relp, attrp, version, rcount, pages, overflow, 
			(char *)NULL, nunique, reptfact, charunique, opq_cflag,
			(i2)domain, max_cell + 1, null_count, cell_count,
			uniquepercell, intervalp, outf, verbose_param, 
			FALSE);
    }

    /* If "-o" option, we don't update the catalogs. */
    if (opq_global.opq_env.ofid) return;

    /* remove old statistics before storing new ones */
    {
	/*
	** If standard catalogs describing statistics are 
	** not updateable, base tables need to be modified
	*/
	if (g->opq_dbcaps.cat_updateable & OPQ_HISTS_UPD)
	{
	    exec sql repeated delete from iihistograms
		    where table_name	= :rname
		    and   table_owner	= :rowner
		    and   column_name	= :atrname;
	}
	else
	{
	    exec sql repeated delete from iihistogram
		    where htabbase	= :reltid
		    and   htabindex	= :reltidx
		    and   hatno		= :atrno;
	}

	if (g->opq_dbcaps.cat_updateable & OPQ_STATS_UPD)
	{
	    exec sql repeated delete from iistats
		    where table_name	= :rname
		    and   table_owner	= :rowner
		    and   column_name	= :atrname;
	}
	else
	{
	    exec sql repeated delete from iistatistics
		    where stabbase	= :reltid
		    and   stabindex	= :reltidx
		    and   satno		= :atrno;
	}
    }

    /* Insert histogram into iihistogram */
    if (max_cell > 0)
        insert_iihistogram(g, relp, attrp, intervalp, cell_count, max_cell,
		       version);
    
    /* insert statistics info into iistatistics */
    {
	exec sql begin declare section;
            f4      es3_nunique;
            f4      es3_reptfact;
            i2      es3_max_cell;
            i2      es3_domain;
            i1      es3_complete;
            i1      es3_unique;
	    char    es3_charunique[8+1];
            f4      es3_null_count;
	    char    es3_charcomplete[8+1];
	    char    es3_version[DB_STAT_VERSION_LENGTH+1];
	    i2	    es3_histlength;
	exec sql end declare section;

	es3_nunique = nunique;
	es3_reptfact = reptfact;
	es3_max_cell = max_cell + 1;
	es3_domain = domain;
	es3_unique = unique;
	if (version == NULL)
	    STcopy(DB_NO_STAT_VERSION, es3_version);
	else
	    STcopy(version, es3_version);
	es3_histlength = (i2) histlen;

	/* Set complete flag to a numerical value; only 0 and 1 are allowed. */
	if (complete == TRUE)
	    es3_complete = 1;
	else
	    es3_complete = 0;

	es3_null_count = null_count;

	if (es3_null_count > 1.0)
	    es3_null_count = 1.0;

	if (es3_null_count < 0.0)
	    es3_null_count = 0.0;

	if (g->opq_dbcaps.cat_updateable & OPQ_STATS_UPD)
	{
	    exec sql begin declare section;
		char	es3_date[DATE_SIZE+1];
	    exec sql end declare section;

	    SYSTIME	stime;
	    i4		zone;

	    /* We need to store creation time expressed in GMT. */

	    TMnow(&stime);
	    TMzone(&zone);
	    zone *= 60;		    /* local time zone in secs from GMT */
	    stime.TM_secs += zone;  /* corresponding GMT */
	    TMstr(&stime, es3_date);
	    (VOID)STcat((char *)es3_date, (char *)" GMT");
	 
	    if (unique)
		STcopy("Y", es3_charunique);
	    else
		STcopy("N", es3_charunique);

	    if (complete)
		STcopy("Y", es3_charcomplete);
	    else
		STcopy("N", es3_charcomplete);

	    exec sql repeated insert into iistats
		(table_name, table_owner, column_name, create_date,
		 num_unique, rept_factor, has_unique, pct_nulls, num_cells,
		 column_domain, is_complete, stat_version, hist_data_length)
		values (:rname, :rowner, :atrname, :es3_date,
			:es3_nunique, :es3_reptfact, :es3_charunique,
			:es3_null_count, :es3_max_cell, :es3_domain,
			:es3_charcomplete, :es3_version, :es3_histlength);
	}
	else
	{
	    /*
	    ** We are assumming this is the INGRES case, we need to
	    ** insert actual local time. The iistats view definition
	    ** converts it to GMT by using date_gmt function.
	    */
	    exec sql repeated insert into iistatistics
		(stabbase, stabindex, snunique, sreptfact, snull, satno,
		 snumcells, sdomain, scomplete, sunique, sdate, sversion,
		 shistlength)
		values (:reltid, :reltidx, :es3_nunique, :es3_reptfact,
			:es3_null_count, :atrno, :es3_max_cell, :es3_domain,
			:es3_complete, :es3_unique, date('now'), :es3_version,
			:es3_histlength);
	}
    }

    /*
    ** Indicate that statistics on relation exist,
    ** if this has not been done yet - just for Star here, though. Rest 
    ** are done from main().
    */
    if (g->opq_dbms.dbms_type & OPQ_STAR)
    {
	if (relp->statset == (bool)FALSE)
	{
	    /* We must commit in case of Ingres/STAR because we have
	    ** just updated iistats and iihistograms, and these queries
	    ** get routed over non-privileged association; set statistics
	    ** will result in an update of iitables, which gets routed over
	    ** the privileged association. If we did not commit, 2PC on
	    ** a VAXcluster would return an error.
	    */
	    exec sql commit;

	    opq_setstats(g, relp);
	}
    }
    else if (fromfile)
	opq_setstats(g, relp);
}

/*{
** Name: add_default_char_stats - Add default character set statistics to
** 				  histogram
**
** Description:
**	Stores default character set statistics in the character statistics
**	arrays, which are in the histogram buffer after the boundary values;
**	these arrays have already been allocated. Default character set
**	statistics are used when character set information is not available.
**
**	See Character Histogram Enhancements project spec for details.
**
**	Called by: minmax()
**	
** Inputs:
**	histlength	Length of a histogram boundary value
**      max_cell	Maximum cell number in histogram
**      intervalp       Array of boundary values delimiting histogram cells.
**      		Character set statistics arrays will immediately follow
**      		the boundary values in this buffer.
**
** Outputs:
**      intervalp	Character set statistics arrays are stored immediately
**      		after the boundary values in this buffer.
**
** Returns:
**	None.
**	   
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**      24-may-93 (rganski)
**          Initial creation for Character Histogram Enhancements project.
*/
static VOID
add_default_char_stats(
OPS_DTLENGTH	histlength,
i4		max_cell,
OPH_CELLS	intervalp)
{
    i4		i;		/* Loop counter */
    OPH_CELLS	offset;		/* Offset into intervalp at which
				** character set statistics are to be
				** located */
    /* charnunique and chardensity are global arrays */
    
    for (i = 0; i < histlength; i++)
    {
	charnunique[i] = DB_DEFAULT_CHARNUNIQUE;
	chardensity[i] = DB_DEFAULT_CHARDENSITY;
    }

    /* Copy default character set statistics to histogram buffer */
    offset = intervalp + (max_cell + 1) * histlength;
    MEcopy((PTR)charnunique, histlength * sizeof(i4), (PTR)offset);
    offset += histlength * sizeof(i4);
    MEcopy((PTR)chardensity, histlength * sizeof(f4), (PTR)offset);
	
    return;
}

/*{
** Name: minmax	- minimal statistics only
**
** Description:
**      Gather minimal statistics on table only.  Determine the minimum and
**      maximum values for each column.  Because minimum and maximum values for
**      columns from the same table can be determined by a single scan
**      through the table, this flag provides a quick way to generate
**      a minimal set of statistics.
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_utilid	    name of utility from which this proc is called
**        opq_owner         table owner
**        opq_dbname        database name
**	  opq_dbcaps	    databse capabilities struct
**	    cat_updateable  mask showing which standard catalogs are updateable
**	  opq_adfcb	    ptr to ADF control block
**      relp		    ptr to relation descriptor for table to gather
**			    statistics on
**      attlst		    ptr to array of ptrs to attribute descriptors for
**			    relation referred to by reldescp
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Updates the iistatistics catalog with info about columns in the
**          table
**
** History:
**      30-nov-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	10-jul-89 (stec)
**	    Changed code to reflect new interface to adc_lenchk.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	06-apr-92 (rganski)
**	    Merged change from seg: can't do arithmetic on PTR variables.
**	18-jan-93 (rganski)
**	    Changed range[] to OPH_CELLS, and allocated dynamically. Data is
** 	    written to it contiguously.
**	    Added space for character set statistics, if column is of character
**	    type.
**	20-sep-93 (rganski)
**	    Delimited ID support. Use relp->delimname instead of relp->relname
**	    and ap->delimname instead of ap->attname in aggregate select
**	    statement.
**	31-jan-94 (rganski)
**	    b54824. Compute the attribute precision and length properly for
**	    decimal datatype.
**	11-apr-1995 (inkdo01)
**	bug 68015: min/max stats on mix of char/int cols could SEGV
**	wrong var was being used in function min_max().
**      11-apr-95 (nick) (cross-int angusm)
**      Preserve C datatypes when calculating min/max statistics until
**      actually inserting into the histogram cell ( the full statistics
**      run already did this ).  We have to do this as the query we
**      create here returns the datatype of max(C) as a CHAR ( indeed,
**      all dynamic SQL on C datatypes describe CHAR unless we explicitly
**      request otherwise by putting DB_DBV_TYPE in the SQLDA vector and
**      pointing the element's data to a fully qualified DB_DATA_VALUE ).
**      We were using the datatype returned by the describe to populate
**      the DB_DATA_VALUE rather than the type calculated and passed
**      in the attlst vector.  This meant we didn't convert to CHAR when
**      inserting into the histogram cell.
**      01-Jan-1997  (merja01)
**      Change long to i4 to address BUG 79066 E_LQ000A conversion failure
**      on axp.osf
**	25-sep-1998 (hayke02)
**	    Allocate space in range (the minmax histogram boundary values
**	    pointer) for the per-cell repetition factor values (in the case
**	    of a minmax histogram, this will be 2 * sizeof(OPN_PERCENT)).
**	    This change fixes bug 90917.
**      280aug-2000 (chash01) 07-jun-1999 (schang)
**	    For RMS-Gateway handling:  because the system catalogs are not
**	    automatically kept in synch with the current row count in a 
**	    base table of imported origin, if requested then immediately
**	    update the standard catalogs so that the row count of a base table 
**	    will be in synch with the row count used to generate statistics
**	    for columns of that base table.
**	 7-jul-04 (hayke02)
**	    Add "session." to the table name if we are sampling. This change
**	    fixes problem INGSRV 2898, bug 112617.
**	15-apr-05 (inkdo01)
**	    Add support for 0-cell histograms (column is all null).
**	20-jan-06 (dougi)
**	    Separate allocation for hist_dt data area.
**	6-dec-2006 (dougi)
**	    Inhibit SET STATISTICS for empty tables.
*/
static VOID
minmax(
OPQ_GLOBAL	*g,
OPQ_RLIST       *relp,
OPQ_ALIST       *attlst[])
{
    char	*stmt;
    PTR		resbuf;
    i4	nrows;
    IISQLDA	*sqlda = &_sqlda;
    i4		maxcols;
    bool	sample;
# ifdef FUTURE_CODE
    i4		maxlen;
# endif /* FUTURE_CODE */
    char        sestbl[9];

    if (g->opq_dbms.dbms_type & OPQ_STAR)
    {
        sestbl[0] = 0x0;
    }
    else
    {
        STcopy("session.", sestbl);
    }

    if (attlst[0] == (OPQ_ALIST *)NULL)
    {
	opq_error( (DB_STATUS)E_DB_ERROR,
	    (i4)E_OP0918_NOCOLUMNS, (i4)4, 
	    (i4)0, (PTR)g->opq_dbname, 
	    (i4)0, (PTR)&relp->relname);
	/* optimizedb: no columns for database '%s', table '%s'\n
        */
	return;
    }

# ifdef FUTURE_CODE
    /* The code below should get maximums from iidbcapabilities, but it is not
    ** there yet, so we have to make certain assumptions.
    */
    if (g->opq_dbms.dbms_type & (OPQ_INGRES | OPQ_STAR))
    {
	maxlen = DB_MAXTUP;
    }
    else
    {
	maxlen = DB_GW3_MAXTUP; 
    }
# endif /* FUTURE_CODE */

    maxcols = g->opq_dbcaps.max_cols;

    /* Check if the column number in the generated query will not exceed
    ** maximum.
    */
    if ((1 + 2 * relp->attcount) > maxcols)
    {
	opq_error( (DB_STATUS)E_DB_ERROR,
	    (i4)E_OP0939_TOO_MANY_COLS, (i4)4,
	    (i4)0, (PTR)g->opq_dbname, 
	    (i4)0, (PTR)&relp->relname);
	return;
    }

    if (relp->samplename[0] != EOS)
    {
	sample = (bool)TRUE;

	if (relp->sampleok == (bool)FALSE)
	{
	    /* user has already been warned */
	    return;
	}
    }
    else
    {
	sample = (bool)FALSE;
    }

    /*
    ** Build a query that will retrieve min, max info for columns and
    ** the row count. This query will be executed using dynamic SQL.
    ** In the best case all info will be retrieved in one pass, in the
    ** worst case we will get one aggregate at a time; in an extreme
    ** case it may not be possible to get it back at all. Also, in 
    ** Ingres, OPF modifies the query target list and this makes things 
    ** more complicated. Until successful PREPARE can (sort of guarantee)
    ** that the result tuple will not be too long (in Ingres requires 
    ** invoking OPF, which is not the case right now), we have to determine
    ** if query is executable using trial and error method.
    */

    /* allocate space for query buffer */
    {
	register i4  len;

	len = sizeof("select count(*), ") - 1;
	len += relp->attcount * (sizeof("min()") + DB_ATT_MAXNAME - 1
			    + 4 /* ", " and possible quotes */
			    + sizeof("max()") + DB_ATT_MAXNAME - 1
			    + 4 /* ", " and possible quotes*/);

	len += sizeof("from ") - 1;
	len += sizeof(sestbl) + DB_TAB_MAXNAME+2; /* relation name */

	opq_bufalloc((PTR)g->opq_utilid, (PTR *)&stmt, (i4)len, &opq_sbuf);
    }

    /* Fill in the statement buffer */
    {
	register OPQ_ALIST  **attlstp;	/* used to traverse the list of
					** attribute descriptors */
	register OPQ_ALIST  *attrp;	/* ptr to current attribute descriptor
					** to be analyzed */
	register bool	firstelem;

	(VOID) STcopy("select count(*), ", stmt);

	for (attlstp = &attlst[0], firstelem = TRUE; *attlstp; attlstp++)
	{
	    char    elem[sizeof("min(%s), max(%s)") + 2 * DB_ATT_MAXNAME +4];

	    if (!firstelem)
	    {
		(VOID) STcat(stmt, ", ");
	    }
	    else
	    {
		firstelem = FALSE;
	    }

	    (VOID) STprintf(elem, "min(%s), max(%s)", &(*attlstp)->delimname,
		&(*attlstp)->delimname);

	    (VOID) STcat(stmt, elem);
	}

	{
	    char   elem[sizeof(" from %s") + sizeof(sestbl) + DB_ATT_MAXNAME+2];
	    char   *name;

	    if (sample == (bool)TRUE)
	    {
		name = (char *)&relp->samplename;
	    }
	    else
	    {
		name = (char *)&relp->delimname;
	    }

	    if (sample == (bool)TRUE)
		(VOID) STprintf(elem, " from %s%s", sestbl, name);
	    else
		(VOID) STprintf(elem, " from %s", name);
	    (VOID) STcat(stmt, elem);
	}

	/* retrieve min/max statistics from relation */
	{
	    register IISQLVAR *sqlv, *lsqlv;

	    sqlda->sqln = IISQ_MAX_COLS;
	    exec sql prepare s from :stmt;
	    exec sql describe s into :sqlda;

	    /* compute result buffer length,
	    ** do not allocate buffer for count(*);
	    ** allocate result buffer.
	    */
	    {
		register i4  len = 0;

		for (sqlv = &sqlda->sqlvar[1],
		     lsqlv = &sqlda->sqlvar[sqlda->sqld];
		     sqlv < lsqlv; sqlv++)
		{
		    DB_STATUS	    s; 
		    DB_DATA_VALUE   dv, rdv;
		    DB_DT_ID	    dtype;

		    dv.db_data = (PTR)0;
		    dv.db_datatype = sqlv->sqltype;

		    dtype = sqlv->sqltype;
		    if (dtype < 0)
			dtype = -dtype;
		    if (dtype == DB_DEC_TYPE)
		    {
			/* sqlv->sqllen contains precision in high byte and
			** scale in low byte. Use this to assign the precision
			** and length */
			dv.db_prec = sqlv->sqllen;
			dv.db_length = DB_P_DECODE_MACRO(sqlv->sqllen);
			dv.db_length = DB_PREC_TO_LEN_MACRO(dv.db_length);
		    }
		    else
		    {
			dv.db_length = sqlv->sqllen;
			dv.db_prec = 0;
		    }

		    s = adc_lenchk(g->opq_adfcb, TRUE, &dv, &rdv);

		    if (DB_FAILURE_MACRO(s))
			opq_adferror(g->opq_adfcb, (i4)E_OP093A_ADC_LENCHK,
				     (i4)sqlv->sqlname.sqlnamel, 
				     (PTR)sqlv->sqlname.sqlnamec, (i4)0,
				     (PTR)0);

		    sqlv->sqllen = (short) rdv.db_length;
		    len += sqlv->sqllen;

		    /* take alignment into account */
		    len = align(len);
		}

		/* allocate result buffer */
		opq_bufalloc((PTR)g->opq_utilid, (PTR *)&resbuf,
		    (i4)len, &opq_rbuf);
	    }

	    /* describe location for the count(*) */
	    sqlv = &sqlda->sqlvar[0];
	    sqlv->sqldata = (char *)&nrows;
	    sqlv->sqllen = sizeof(i4);

	    /* describe locations for min, max pairs for
	    ** each attribute.
	    */
	    {
		register PTR p = resbuf;

		for (sqlv = &sqlda->sqlvar[1],
		     lsqlv = &sqlda->sqlvar[sqlda->sqld],
		     attlstp = &attlst[0];
		     (sqlv < lsqlv) && (attrp = *attlstp);
		     attlstp++)
		{

            /*
            ** max and min are always nullable datatypes so the
            ** length calculated is correct - however, we want
            ** to keep knowledge of 'c' datatypes rather than
            ** 'char' returned by the describe above otherwise we won't
            ** convert to 'char' in hist_convert() and erronously
            ** place a 'c' datatype in the char(8) which is a
            ** histogram cell
            */
            if (abs(attrp->attr_dt.db_datatype == DB_CHR_TYPE))
            {
            if (attrp->attr_dt.db_datatype > 0)
                attrp->attr_dt.db_datatype =
                -attrp->attr_dt.db_datatype;
            }
            else
            {
	    		attrp->attr_dt.db_datatype = sqlv->sqltype; 
			}
	    	attrp->attr_dt.db_length = sqlv->sqllen; 

		    STRUCT_ASSIGN_MACRO(attrp->attr_dt, attrp->attr1_dt);

		    attrp->attr_dt.db_data = p;
		    sqlv->sqldata = (char *)&attrp->attr_dt;
		    sqlv->sqltype = DB_DBV_TYPE;
 		    p = (char *)p + align(sqlv->sqllen);
		    sqlv->sqllen = sizeof(DB_DATA_VALUE);
		    sqlv++;

		    attrp->attr1_dt.db_data = p;
		    sqlv->sqldata = (char *)&attrp->attr1_dt;
		    sqlv->sqltype = DB_DBV_TYPE;
 		    p = (char *)p + align(sqlv->sqllen);
		    sqlv->sqllen = sizeof(DB_DATA_VALUE);
		    sqlv++;
		}
	    }

	    /*
	    ** Retrieve data for the constructed query.
	    ** We disable automatic error checking to be able
	    ** to catch errors, in some cases E_OP0200 tuple
	    ** too wide will be returned by Ingres.
	    */
	    exec sql whenever sqlerror continue;
	    exec sql whenever sqlwarning continue;

	    exec sql open c1 for readonly;
	    if (sqlca.sqlcode != GE_OK)
	    {
		if (g->opq_dbms.dbms_type & 
			(OPQ_INGRES | OPQ_STAR | OPQ_RMSGW) &&
		    (
		     (sqlca.sqlcode == -GE_SYSTEM_LIMIT
		      &&
		      sqlca.sqlerrd[0] == -E_OP0200_TUPLEOVERFLOW
		     )
		     ||
		     sqlca.sqlcode == -E_OP0200_TUPLEOVERFLOW
		    )
		   )
		{
		    /* issue a warning */
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP095A_TUPLE_OVFLOW, (i4)2,
			(i4)0, (PTR)&relp->relname);
		    return;
		}
		else
		{
		    opq_sqlerror();
		}
	    }

	    exec sql whenever sqlerror call opq_sqlerror;
	    exec sql whenever sqlwarning call opq_sqlerror;

	    while (sqlca.sqlcode == GE_OK)
	    {
		exec sql fetch c1 using descriptor :sqlda;
	    }

	    exec sql close c1;
	}

	/* check the count(*), if zero, then there are no rows in relation,
	** so it doesn't make sense to create statistics for it.
	*/
	if (nrows == 0)
	{
	    opq_error( (DB_STATUS)E_DB_WARN,
		(i4)I_OP091C_NOROWS, (i4)4, 
		(i4)0, (PTR)&relp->relname,
		(i4)0, (PTR)g->opq_dbname);
	    /* optimizedb: table '%0c' in database '%1c' contains no rows. */
	    relp->statset = TRUE;   /* don't SET STATISTICS on empty table */
	    return;
	}

	/* update tuple count */
	if (sample == (bool)FALSE)
	{
	    relp->ntups = nrows;
	}
	else
	{
	    relp->nsample = nrows;
	}

	/* create minmax statistics for relation */
	{
	    OPQ_ALIST		**attlstp;	/* Used to traverse the list of
						** attribute descriptors */
	    OPH_CELLS		range = NULL;	/* Pointer to minmax histogram
						** boundary values */
	    register OPQ_ALIST 	*attp;		/* Ptr to current attribute
						** descriptor to be analyzed */
	    register PTR 	p = resbuf;

	    /* for each attribute, generate a histogram */
	    for (attlstp = &attlst[0]; (attp = *attlstp); attlstp++)
	    {
		OPN_PERCENT	hist_count[2]; /* a count of elements in a
                                               ** histogram cell */
		DB_DATA_VALUE	tdv;
		DB_STATUS	s;
		i4		attlen;
		bool		allnull;

    		hist_count[0] = 0.0;
		hist_count[1] = nrows;    /* indicates all tuples fall
					  ** in this interval (it will
					  ** normalized to integrate to
                                          ** 1.0 inside insert_histo )*/

		/* Allocate space for range and hist_dt buffer.
		** If character type, include space for character set
		** statistics.
		*/
	        {
		    u_i4	extra;	/* Extra space required, if any, for
					** character set statistics. */

		    if (attp->hist_dt.db_datatype == DB_CHA_TYPE ||
			attp->hist_dt.db_datatype == DB_BYTE_TYPE)
			extra = attp->hist_dt.db_length * (sizeof(i4) +
							  sizeof(f4));
		    else
			extra = 0;

		    (VOID) getspace((char *)g->opq_utilid, (PTR *)&range, 
			    (u_i4)(extra + 2 *
			    (attp->hist_dt.db_length + sizeof(OPN_PERCENT))));
		    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&attp->hist_dt.db_data,
		     attp->hist_dt.db_length, &opq_rbuf1);
		}

		attlen = align(attp->attr_dt.db_length);

		if (hist_convert(p, attp, g->opq_adfcb)
		    ||
 		    hist_convert((char *)p + attlen, attp, g->opq_adfcb)
		    )
		{
		    /* It used to flag all null columns as a warning and
		    ** refused to create a histogram. All nulls is still
		    ** useful information, so now we flag 'em and build
		    ** the histogram anyway. */
		    allnull = TRUE;
		}
		else allnull = FALSE;

		attp->hist_dt.db_data = (PTR) p;

		STRUCT_ASSIGN_MACRO(attp->hist_dt, tdv);
		tdv.db_data = (PTR)range;

		s = adc_hdec(g->opq_adfcb, &attp->hist_dt, &tdv);

		if (DB_FAILURE_MACRO(s))
		    opq_adferror(g->opq_adfcb, (i4)E_OP0929_ADC_HDEC, 
				 (i4)0, (PTR)&attp->attname, (i4)0,
				 (PTR)&relp->relname);

 		p = (char *)p + attlen;

		MEcopy((PTR)p, attp->hist_dt.db_length,
		       (PTR)(range + attp->hist_dt.db_length));

 		p = (char *)p + attlen;

		{
		    OPO_TUPLES	nunique;
		    OPO_TUPLES	null_count = (allnull) ? 1.0 : 0.0;
		    i4	max_cell = (allnull) ? -1 : 1; /* max cell number */

		    nunique = relp->ntups / 2.0;

		    if (nunique < 1.0)
			nunique = 1.0;

		    /* If the attribute is of character type:
		    ** 1) Store default character set statistics in the
		    ** character statistics arrays, which are in the histogram
		    ** buffer after the boundary values; these arrays have
		    ** already been allocated. Default statistics are used
		    ** because we have not scanned the table for character set
		    ** statistics.
		    ** 2) If the -length flag has not been used, determine the
		    ** necessary length for character histogram boundary values
		    ** and truncate them accordingly.
		    */
/* bug 68015: should be attp->...., not attrp->...) */
  
		    if (attp->hist_dt.db_datatype == DB_CHA_TYPE ||
			attp->hist_dt.db_datatype == DB_BYTE_TYPE)
		    {
			/* adc_hg_dtln() makes histogram DB_CHA_TYPE or
			** DB_BYTE_TYPE for all character types.*/

			add_default_char_stats(attp->hist_dt.db_length,
					       max_cell, range);
			if (bound_length <= 0)
			    /* Note that for purposes of truncation, the
			    ** histogram is considered inexact, but for
			    ** purposes of reptf and nunique (insert_histo), it
			    ** is considered exact */
			    truncate_histo(attp, relp, &max_cell, hist_count,
					   range,
					   (bool)FALSE /* inexact histo */,
					   g->opq_adfcb);
		    }
		    
		    /* Insert the statistics into the system catalogs */
		    insert_histo(g, relp, attp, nunique,
			range, hist_count, max_cell, 
			(i4)nrows, (i4)relp->pages,
			(i4)relp->overflow, null_count,
			(bool) TRUE, (bool)TRUE, (char)0, opq_cflag, (f8)0.0,
			(char *) DU_DB6_CUR_STDCAT_LEVEL, (bool) FALSE);
		    /*
		    ** 05-dec-97 (chash01)
		    ** For RMS-Gateway databases:  if requested then immediately
                    ** update the standard catalogs so that the row count of a
                    ** base table will be in synch with the row count used to
                    ** generate statistics for columns of that base table.
		    */
		    if ((pgupdate) 
			&&
			(g->opq_dbms.dbms_type & OPQ_RMSGW)
			)
		    {
			(VOID) update_row_and_page_stats(g, relp,
			    (i4)nrows, (i4)relp->pages,
			    (i4)relp->overflow);
		    }
		}
	    }
	}
    }
}

/* {
** Name: compare_values - Compare two data values
**
** Description:
**      This routine calls adc_compare to compare the two data values.
**	It returns zero, a negative number, or a positive number, depending on
**	whether the data values are equal, the first is less than the second, 
**	or the first is greater than the second, respectively.
**
** Inputs:
**	adfcb		Ptr to ADF control block, for passing to adc_compare().
**	datavalue1	Ptr to first data value.
**	datavalue2	Ptr to second data value.
**	columnname	Ptr to name of column from which one of the values is
**			derived. For printing error message if necessary.
**
** Outputs:
**	None.
**
**	Returns:
**	    0		If data values are equal.
**	    < 0		If first data value is less than second.
**	    > 0		If first data value is greater than second.
**	   
**	Exceptions:
**	    If an exception occurs on the call to adc_compare(), an error 
**	    message is printed and the program aborts.
**
** Side Effects:
**	    none
**
** History:
**      09-nov-92 (rganski)
**          Initial creation.
*/
static i4
compare_values(
ADF_CB		*adfcb,
DB_DATA_VALUE	*datavalue1,
DB_DATA_VALUE	*datavalue2,
DB_ATT_STR	*columnname)
{
    DB_STATUS	status;
    i4		adc_cmp_result;
    
    status = adc_compare(adfcb, datavalue1, datavalue2, &adc_cmp_result);
    
    if (DB_FAILURE_MACRO(status))
	opq_adferror(adfcb, (i4)E_OP092B_COMPARE, (i4)0,
		     (PTR)columnname, (i4)0, (PTR)0);
    /* optimizedb: data comparison failed, column %s, 
    ** code = %d
    */

    return(adc_cmp_result);
}

/* {
** Name: new_unique - See if new value from table is unique
**
** Description:
**      This routine calls compare_values to see if the new value read from the 
**      table in process() is different from the previous value. If it is 
**      different, it is copied over the previous value (thus becoming the new 
**      "previous" value) and this function returns TRUE. Otherwise, FALSE is 
**      returned. 
**
** 	We use the full value (not truncated as chars and DB_TXT_TYPEs
**	are if they have len > 8) so that the optimizer can estimate 
**	sizes of joins correctly.  (The problem was that after 
**	truncating to 8, we might get a larger number of duplicates
**	that would make the optimizer think that the join on these 
**	atts would be large).
**
** Inputs:
**	cellinexact	Number of cells in inexact histogram so far. If this is 
**			zero, we assume a new unique value.
**	attrp		Pointer to attribute descriptor
**	    attr_dt	New data value
**	    attname	Name of column we are looking at.
**	prev_dt		Previous data value
**	adfcb		Ptr to ADF control block, for passing to adc_compare().
**
** Outputs:
**	prev_dt		Gets new data value if it is different
**
**	Returns:
**	    TRUE	If new data value is different from previous 
**	    		data value 
**	    FALSE	If new data value is same as previous 
**	    		data value 
**	   
**	Exceptions:
**	    If an exception occurs on the call to adc_compare(), an error 
**	    message is printed and the program aborts.
**
** Side Effects:
**	    none
**
** History:
**      09-nov-92 (rganski)
**          Initial creation.
**	    The code was basically taken from process() in an effort to 
**	    modularize the code a little.
*/
static bool
new_unique(
i4		cellinexact,
OPQ_ALIST	*attrp,
DB_DATA_VALUE	*prev_dt,
ADF_CB		*adfcb)
{
    if (cellinexact == 0 ||
       (compare_values(adfcb, &attrp->attr_dt, prev_dt, &attrp->attname) != 0))
    {
	/* We have a new unique value */
	MEcopy((PTR)attrp->attr_dt.db_data,
	       attrp->attr_dt.db_length,
	       (PTR)prev_dt->db_data);
	return(TRUE);
    }
    else
	return(FALSE);
}

/* {
** Name: decrement_value - Get immmediately preceding histogram value
**
** Description:
**      This routine calls adc_hdec to decrement the current histogram value, 
**      yielding the value immediately preceding it in magnitude. This can be 
**      used as the lower boundary value of a histogram cell.
**
** Inputs:
**	adfcb		Ptr to ADF control block, for passing to adc_compare().
**	attrp		Pointer to attribute descriptor for current column.
**	    attr_dt		New data value.
**	    hist_dt		New histogram value.
**	    attname		Name of current column.
**	relp		Pointer to relation descriptor for current table.
**	    relname		Name of current table.
**
** Outputs:
**	result		Pointer to decremented value.
**
**	Returns:
**	    None.
**	   
**	Exceptions:
**	    If an exception occurs on the call to adc_hdec(), an error 
**	    message is printed and the program aborts.
**
** Side Effects:
**	    none
**
** History:
**      09-nov-92 (rganski)
**          Initial creation.
**	    The code was basically taken from process() in an effort to 
**	    modularize the code a little.
**	9-nov-05 (inkdo01)
**	    DON'T copy attr_dt addr to hist_dt - they're not shared.
*/
static VOID
decrement_value(
ADF_CB		*adfcb,
OPQ_ALIST	*attrp,
OPQ_RLIST	*relp,
OPH_CELLS	result)
{
    DB_STATUS		tstat;
    DB_DATA_VALUE	tdv;
    
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, tdv);
    tdv.db_data = (PTR)result;
    
    tstat = adc_hdec(adfcb, &attrp->hist_dt, &tdv);
    
    if (DB_FAILURE_MACRO(tstat))
	opq_adferror(adfcb, (i4)E_OP0929_ADC_HDEC, (i4)0,
		     (PTR)&attrp->attname, (i4)0, (PTR)&relp->relname);
    
    return;
}

/*{
** Name: add_exact_cell	 -	Add new exact histogram cell
**
** Description:
**      This routine creates a new exact histogram cell, since the current 
**      histogram value differs from the previous one. The first step is to 
**      make sure that we have a proper lower boundary value for the current 
**      value. We do this by calling adc_hdec with the current value, then 
**      comparing the result with the previous histogram value. If they are 
**      the same, we can use the previous histogram cell as the 
**      lower boundary, and just add one new cell. If not, we add an empty 
**      cell, to serve as the lower boundary, and then we add the new cell with 
**      the current histogram value.
**
**	If at any point we exceed the total number of allowed exact histogram 
**	cells, we return without adding any more cells.
**	
** Inputs:
**	attrp		Pointer to attribute descriptor for current column.
**	    attr_dt		New data value.
**	    hist_dt		New histogram value.
**	    attname		Name of current column.
**	relp		Pointer to relation descriptor for current table.
**	    relname		Name of current table.
**	prev_hdt	Previous histogram value.
**	cellexact	Maximum cell number in exact histogram so far. If this
**			is -1, there are no cells in the histogram yet.
**	totalcells	Maximum number of exact histogram cells.
**	adfcb		Ptr to ADF control block, for passing to adc_compare().
**
** Outputs:
**	None, but see Side Effects below.
**
**	Returns:
**	    New exact cell count (cellexact). 
**	   
**	Exceptions:
**	    If an exception occurs on the call to adc_hdec() or
**	    compare_values(), an error message is printed and the program
**	    aborts. 
**
** Side Effects:
**	    Adds new cells to global exact histogram (exacthist and exactcount).
**
** History:
**      09-nov-92 (rganski)
**          Initial creation.
**	    This was mainly copied from process() in an attempt at modularity.
**	    Got rid of the call to opn_diff(); instead, always call adc_hdec() 
**	    and compare the result with the previous cell.
*/
static i4
add_exact_cell(
OPQ_ALIST	*attrp,
OPQ_RLIST	*relp,
DB_DATA_VALUE	*prev_hdt,
i4		cellexact,
i4		totalcells,
ADF_CB		*adfcb)
{
    OPH_CELLS	new_cell;		/* pointer to exact histogram cell to
					** be added */
    /* exacthist and exactcount are global variables */
    
    cellexact++;
    if (cellexact >= totalcells)  
	return(cellexact);		/* too many exact cells */
	
    /* compute location of new cell */
    new_cell = exacthist + cellexact * attrp->hist_dt.db_length;

    /* See if the previous cell can serve as lower boundary value for the 
    ** current value; if not, add an empty cell to serve as the lower boundary.
    */
    {
	DB_DATA_VALUE	tdv;
	
	/* decrement current value and place in new_cell */
	decrement_value(adfcb, attrp, relp, new_cell);

	STRUCT_ASSIGN_MACRO(attrp->hist_dt, tdv);
	tdv.db_data = (PTR)new_cell;
	
	/* Now see if we need an empty cell as lower boundary */
	if (cellexact == 0 ||
	    compare_values(adfcb, &tdv, prev_hdt, &attrp->attname) > 0)
	{
	    /* This is the first cell or the decremented value is greater than
	    ** the previous histogram value, so use decremented value in empty
	    ** cell (count == 0) to serve as the lower boundary.
	    */
	    exactcount[cellexact] = 0.0;
	    cellexact++;
	    if (cellexact >= totalcells)
		return(cellexact);	/* too many exact cells */
	    new_cell += attrp->hist_dt.db_length;
	}
    }    

    /* Add the new non-empty histogram cell */
    exactcount[cellexact] = 1.0;
    MEcopy((PTR)attrp->hist_dt.db_data, attrp->hist_dt.db_length, 
	   (PTR)new_cell);
    
    return(cellexact);
}

/* {
** Name: update_hcarray	- adds new entry to the "high row count" descriptor
**	array
**
** Description:
**      This function checks the array of high count values to determine if
**	the most recently processed distinct value occurs frequently enough
**	to be added to the array (possibly replacing another entry). 
**
** Inputs:
**	newval		Ptr to DB_DATA_VALUE for newest value.
**	prevval		Ptr to preceding value.
**	valcount	Count of rows containing the value.
**	prevcount	Count of rows preceding current value in this cell.
**	nunique		Count of unique values in this cell.
**	hccount		Ptr to count of occupied entries in hcarray.
**
** Outputs:
**	result		Updated hcarray.
**
**	Returns:
**	    Index to array entry assigned to new value (or -1).
**	   
** Side Effects:
**	    none
**
** History:
**      17-oct-05 (inkdo01)
**	    Written to support fixed numbers of exact cells in inexact
**	    histograms.
*/
static i4
update_hcarray(
	DB_DATA_VALUE		*newval,
	OPQ_MAXELEMENT		*prevval,
	i4			valcount,
	i4			prevcount,
	i4			nunique,
	i4			*hccount)
{

    i4		i, j, currcount;


    /* Check first if this is the 1st entry in the array - no search
    ** needed for that. */
    if (*hccount <= 0)
    {
	hcarray[0].nrows = valcount;
	hcarray[0].pnrows = prevcount;
	hcarray[0].puperc = nunique - 1.0;
	MEcopy ((PTR)newval->db_data, newval->db_length,
			(PTR)hcarray[0].valp);
	MEcopy ((PTR)prevval, newval->db_length,
			(PTR)hcarray[0].pvalp);
	(*hccount)++;
	return(0);
    }

    /* If array is full, see if the new entry has a count too small
    ** to fit. */
    if (*hccount >= exinexcells && 
		valcount <= hcarray[exinexcells-1].nrows)
	return(-1);
	
    /* Locate position of new value in array (by row count) & insert. */
    currcount = (exinexcells <= *hccount) ? exinexcells : ++(*hccount);

    for (i = 0; i < currcount; i++)
    {
	if (valcount <= hcarray[i].nrows)
	    continue;

	/* Found the array entry to replace - move the rest down one
	** entry, possibly losing one off the end. */
	for (j = currcount; j > i; j--)
	{
	    if (j >= currcount)
		continue;		/* drop the last entry */
	    hcarray[j].nrows = hcarray[j-1].nrows;
	    hcarray[j].pnrows = hcarray[j-1].pnrows;
	    hcarray[j].puperc = hcarray[j-1].puperc;
	    MEcopy((PTR)hcarray[j-1].valp, newval->db_length,
				(PTR)hcarray[j].valp);
	    MEcopy((PTR)hcarray[j-1].pvalp, newval->db_length,
				(PTR)hcarray[j].pvalp);
	    hcarray[j].exactcell = hcarray[j-1].exactcell;
	}
	hcarray[i].nrows = valcount;
	hcarray[i].pnrows = prevcount;
	hcarray[i].puperc = nunique - 1.0;
	MEcopy ((PTR)newval->db_data, newval->db_length,
			(PTR)hcarray[i].valp);
	MEcopy ((PTR)prevval, newval->db_length,
			(PTR)hcarray[i].pvalp);
	return(i);
    }
    return -1;
}

/*{
** Name: exact_histo_length - compute value length for exact character histogram
**
** Description:
**      This routine determines the necessary length for the boundary values of
**      an exact histogram for a column of character type and length greater
**      than 8. For columns of length 8 or less, the length of the histogram
**      values will simply be the column length, and this function is not called.
**      For columns longer than 8 characters, the necessary length for the
**      boundary values will be the length of the longest common prefix + 1.
**      This requires comparing each adjacent pair of boundary values
**      and finding the maximum character position at which they diverge.
**	If there is only one value (2 cells) in the histogram, the length will
**	be 8.
**
**      See Character Histogram Enhancements project spec for more details.
**	
**	Called by: truncate_histo()
**
** Inputs:
**      max_cell	Maximum cell number in histogram
**      cell_count	Array of cell counts associated with histogram cell
**      		intervals
**      intervalp       Array of boundary values delimiting histogram cells
**	oldhistlength	Length of untruncated histogram values.
**
** Outputs:
**	None.
**	
** Returns:
**	OPS_DTLENGTH: the necessary boundary value length for this exact
**	histogram.
**	   
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**      26-apr-93 (rganski)
**          Initial creation for Character Histogram Enhancements project.
*/
static OPS_DTLENGTH
exact_histo_length(
i4		max_cell,
OPN_PERCENT	cell_count[],
OPH_CELLS	intervalp,
OPS_DTLENGTH	oldhistlength)
{
    OPS_DTLENGTH        maxlength;	/* maximum necessary length for
					** boundary values */
    i4			cell;		/* Current cell */

    /* First check to see if there are two successive cells with non-zero
    ** counts. If so, this means that there are at least two data values in the
    ** column that are immediately adjacent; therefore no truncation should
    ** occur. Cell 0 always has a zero count, and no two adjacent cells have
    ** zero counts; this means that if there is an even-numbered cell with a
    ** non-zero count, there are two adjacent non-zero cells.
    */
    for (cell = 2; cell <= max_cell; cell += 2)
	if (cell_count[cell] != 0.0)
	    return(oldhistlength);	/* found 2 adjacent non-zero cells */

    /* Compare successive boundary values, finding maximum character position
    ** at which they diverge. Compare only "real" boundary values, i.e.
    ** boundary values which actually occur in the data rather than boundary
    ** values with 0 counts which were created to serve as delimiters. These
    ** boundary values will all be in the odd cells, since there are no
    ** adjacent real values.
    */
    maxlength = 0;
    for (cell = 3; cell <= max_cell; cell += 2)
    {
	/* Compare current cell's boundary value with previous "real" value */

	OPH_CELLS	value1, value2;	/* Boundary values being compared */
	i4		pos;		/* Position of characters being
					** compared within both values */

	value1	= intervalp + (cell - 2) * oldhistlength; /* Lower value */
	value2	= intervalp + cell * oldhistlength;	  /* Higher value */
	
	/* Compare values one character at a time */
	for (pos = 0; pos < oldhistlength; pos++)
	{
	    if (value1[pos] != value2[pos])	/* Found point of divergence */
	    {
		if (pos + 1 > maxlength)
		    maxlength = pos + 1;
		break;
	    }
	}
	
	if (maxlength >= oldhistlength)
	    return(oldhistlength);		/* No truncation required */
    }
    return(maxlength);
}

/*{
** Name: inexact_histo_length - compute value length for inexact char histogram
**
** Description:
**      This routine determines the necessary length for the boundary values of
**      an inexact histogram for a column of character type and length greater
**      than 8. For columns of length 8 or less, the length of the histogram
**      values will simply be the column length, and this function is not
**      called.
**
**	For columns longer than 8 characters, the length of the boundary values
**	is determined by computing the diff between each adjacent pair of
**	boundary values. opn_diff(), which is used to compute the diff, returns
**	the length to which the diff was computed (it stops when enough
**	accuracy was achieved for computing range selectivities). The necessary
**	length for the entire histogram is the maximum necessary length for the
**	individual diffs.
**
**      See Character Histogram Enhancements project spec for more details.
**	
**	Called by: truncate_histo()
**
**	NOTE: this routine assumes that the global arrays charnunique and
**	chardensity contain the character set statistics for this histogram.
**	These arrays are filled by add_char_stats() and
**	add_default_char_stats(), which are called before this routine.
**
** Inputs:
**	attrp		Pointer to attribute descriptor for current column.
**	    attr_dt		Data type information for column.
**	    hist_dt		Data type information for histogram (including
**	    			length)
**      max_cell	Maximum cell number in histogram
**      cell_count	Array of cell counts associated with histogram cell
**      		intervals
**      intervalp       Array of boundary values delimiting histogram cells
**	oldhistlength	Length of untruncated histogram values.
**
** Outputs:
**	None.
**	
** Returns:
**	OPS_DTLENGTH: the necessary boundary value length for this inexact
**	histogram.
**	   
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**      26-apr-93 (rganski)
**          Initial creation for Character Histogram Enhancements project.
**	24-may-93 (rganski)
**	    Changed att.opz_histogram.oph_charnunique and oph_chardensity to
**	    point at globals charnunique and chardensity instead of at the
**	    statistics in the histogram buffer; otherwise alignment problems
**	    occur. Changed the celltups parameter sent to opn_diff from
**	    row_count * cell_count to just cell_count, because at this point
**	    cell_count is not yet normalized; this makes the row_count
**	    parameter of this function and truncate_histo() obsolete, so
**	    removed those.
**	21-jun-93 (rganski)
**	    Bug fix: changed loop termination condition from "< max_cell" to
**	    "<= max_cell".
**	28-apr-05 (inkdo01)
**	    Minor heuristic to reduce very large histlengths for big char/
**	    varchar/Unicode columns with exact cells in inexact histos.
**	10-Oct-08 (kibro01) b121031
**	    If we find out we need fewer that OPN_NH_LENGTH characters, increase
**	    back to OPN_NH_LENGTH anyway, like we do above in truncate_histo().
**	    Otherwise we decrement the wrong character.
*/
static OPS_DTLENGTH
inexact_histo_length(
ADF_CB		*adfcb,
OPQ_RLIST	*relp,
OPQ_ALIST	*attrp,
i4		max_cell,
OPN_PERCENT	cell_count[],
OPH_CELLS	intervalp,
OPS_DTLENGTH	oldhistlength)
{
    OPS_DTLENGTH        maxlength;	/* maximum necessary length for
					** boundary values */
    i4			cell;		/* Current cell */
    OPH_CELLS		lower, upper;	/* Lower/upper boundary value of 
					** current cell */ 
    OPZ_ATTS		att;		/* Attribute information for
					** call to opn_diff(), which only uses
					** the character set statistics in it
					*/
    DB_DATA_VALUE	vall, valu;	/* For resetting boundary values
					** of exact cells */
    DB_STATUS		stat;
    i4			exinex = 0;

    /* Initialize att to point at character set statistics. charnunique and
    ** chardensity are global arrays which must contain the character set
    ** statistics for this attribute */
    att.opz_histogram.oph_charnunique = (i4 *)charnunique;
    att.opz_histogram.oph_chardensity = (OPH_COUNTS)chardensity;

    /* Call opn_diff() for each adjacent pair of boundary values. We are not
    ** interested in the actual diff, but in the length to which the diff was
    ** computed (difflength), which opn_diff() changes: opn_diff() stops its
    ** diff computation when it deems that sufficient accuracy has been
    ** achieved, given the number of tuples in the cell; it changes difflength
    ** to the length at which the diff computation stopped.
    ** We will use the maximum of these lengths as the necessary length for
    ** values in this histogram.
    */
    maxlength = 0;
    lower = intervalp;
    for (cell = 1; cell <= max_cell; cell ++, lower += oldhistlength)
    {
	i4		difflength;	/* Length to which diff went for this
					** cell */

	difflength = 0;			/* Compute diff to necessary length */

	opn_diff((PTR)(lower + oldhistlength), (PTR)lower, &difflength,
		 (OPO_TUPLES)cell_count[cell], &att, &(attrp->hist_dt), (PTR)collationtbl);
	if (cell_count[cell-1] == 0.0)
	{
	    exinex = cell;
	    continue;
	}

	/* opn_diff() changed difflength */
	if (difflength >= oldhistlength)
	    return(oldhistlength);	/* Reached maximum length possible */
	if (difflength > maxlength)
	    maxlength = difflength;	/* New maximum length */
    }

    if (exinex <= 1)
	return(maxlength);

    /* Even if we don't need to go beyond a few characters to get a difference
    ** we must duplicate the check in truncate_histo which forces us back
    ** up to OPN_NH_LENGTH, since otherwise we decrement the wrong bit of
    ** the string (kibro01) b121031
    */
    if (maxlength < OPH_NH_LENGTH)
	maxlength = OPH_NH_LENGTH;

    /* There are exact cells in the inexact histogram. These were 
    ** decremented using the original column width. The following loop 
    ** locates these cells and rebuilds the marker value using the 
    ** truncated length. */

    STRUCT_ASSIGN_MACRO(attrp->hist_dt, vall);
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, valu);
    vall.db_length = valu.db_length = maxlength;
    lower = intervalp;

    for (cell = 1; cell <= max_cell; cell++, lower += oldhistlength)
    {
	upper = lower + oldhistlength;
	vall.db_data = (PTR)lower;
	valu.db_data = (PTR)upper;
	if (cell_count[cell-1] != 0.0 || cell == 1)
	    continue;			/* skip unless marker cell and 
					** not 1st (it's decr'd later) */

	stat = adc_hdec(adfcb, &valu, &vall);

	if (DB_FAILURE_MACRO(stat))
	    opq_adferror(adfcb, (i4)E_OP0929_ADC_HDEC,
		(i4)0, (PTR)&attrp->attname, (i4)0,
		(PTR)&relp->relname);
    }

    return(maxlength);
}

/*{
** Name: truncate_histo	 -	truncate character histogram boundary values
**
** Description:
**      This routine determines the necessary length for the boundary values of
**      a character-type histogram and truncates the boundary values to
**      that length if necessary. It has been determined by the caller that the
**      -length flag has not been used and that the column is of character
**      type.
**
**      For columns of length 8 or less, the length of the histogram values
**      will be the column length (simply return).
**
**      For columns longer than 8 characters:
**
**      For exact histograms, the length of the boundary values
**      will be the length of the longest common prefix + 1, or 8, whichever is
**      greater. This requires comparing each adjacent pair of boundary values
**      and finding the maximum character position at which they diverge.
**	If there is only one value (2 cells) in the histogram, the length will
**	be 8.
**
**	For inexact histograms, the length of the boundary values is determined
**	by computing the diff between each adjacent pair of boundary values.
**	opn_diff(), which is used to compute the diff, returns the length to
**	which the diff was computed (it stops when enough accuracy was achieved
**	for computing range selectivities). The necessary length for the entire
**	histogram is the maximum necessary length for the individual diffs.
**
**      See Character Histogram Enhancements project spec for more details.
**	
**	Called by: process()
**	
**	NOTE: this routine assumes that the global arrays charnunique and
**	chardensity contain the character set statistics for this histogram.
**	These arrays are filled by add_char_stats() and
**	add_default_char_stats(), which are called before this routine.
**
** Inputs:
**	attrp		Pointer to attribute descriptor for current column.
**	    attr_dt		Data type information for column.
**	    hist_dt		Data type information for histogram (including
**	    			length)
**	relp		Pointer to relation descriptor for current table.
**      max_cellp	Address of maximum cell number in histogram
**      cell_count	Array of cell counts associated with histogram cell
**      		intervals
**      intervalp       Array of boundary values delimiting histogram cells
**	exact_histo	Is histogram exact?
**	adfcb		Ptr to ADF control block, for passing to adc_compare().
**
** Outputs:
**	attrp
**	    hist_dt
**	    	db_length	This is modified to new histogram length.
**      max_cellp		The maximum cell number will change if
**      			truncation resulted in coalescing of cells.
**      cell_count		These will be shifted if coalescing occurs.
**      intervalp		The boundary values in this buffer will be
**      			shifted if there is a new histogram length.
**      			This effectively truncates the values.
** Returns:
**	None.
**	   
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**      26-apr-93 (rganski)
**          Initial creation for Character Histogram Enhancements project.
**	24-may-93 (rganski)
**	    Removed row_count parameter, which is no longer needed by
**	    inexact_histo_length().
**	21-jun-93 (rganski)
**	    Re-compute first boundary value after truncation, since truncation
**	    wipes out original decrementation.
**	    Changed "cell_numberp" and "old_cell_number" to "max_cellp" and
**	    "old_max_cell".
**	20-nov-95 (nick)
**	    The values in attrp->attr_dt.db_length are the internal sizes
**	    ( i.e. nullability and byte counts included etc. ) and hence
**	    are the wrong values to use in comparing against OPH_NH_LENGTH.
**	    Since we can't be here if -length was specified and we know we 
**	    are working with character datatypes, we should return immediately
**	    if the already built histogram is <= OPH_NH_LENGTH as this implies
**	    the external user data length is <= OPH_NH_LENGTH i.e. use
**	    hist_dt.db_length and not attr_dt.db_length in the decision. #71702
**	27-mar-1997 (hayke02)
**	    We now ensure that after histogram truncation, charnunique for the
**	    last character position is at least 2; this ensures that there
**	    will be a diff of 1 between a boundary value and the decremented
**	    boundary value (e.g. between the first two boundary values in the
**	    histogram). This change is an extension of the fix for bug 59298
**	    and fixes bugs 81133 and 81210.
**	5-Mar-2008 (kibro01) b120041
**	    It is not valid to say that there are no coalesce cells when
**	    truncating - it is possible that an exact cell added into an
**	    inexact histogram will have used a boundary value decremented from
**	    its own, not using the opn_diff call to determine how long it
**	    should have been.  Allow these cells to be coalesced.
**	7-Jul-2009 (kibro01) b122269
**	    Firstly when reducing a histogram use a temporary buffer to avoid
**	    a chance of overlaps, and secondly use the new location as the 
**	    value for the next comparison, or else you may [randomly, of course]
**	    end up with a badly-corrupted histogram with only a handful of
**	    cells.
*/
static VOID
truncate_histo(
OPQ_ALIST	*attrp,
OPQ_RLIST	*relp,
i4		*max_cellp,
OPN_PERCENT	cell_count[],
OPH_CELLS	intervalp,
bool		exact_histo,
ADF_CB		*adfcb)
{
    OPS_DTLENGTH        oldhistlength;	/* length of boundary values before
					** truncation */
    OPS_DTLENGTH        maxlength;	/* length of boundary values after
					** truncation */
    i4			old_max_cell;/* Max cell number before truncation */
    i4			cell;		/* Counter: current cell */
    OPH_CELLS		oldloc, newloc;	/* Old and new location of memory being
					** copied within histogram buffer */
    OPH_CELLS		offset;		/* Offset into intervalp at which
					** character set statistics are to be
					** located */
    OPH_CELLS		lastloc;	/* Offset to previous cell */
    char		dummy_buffer[DB_MAX_HIST_LENGTH];

    oldhistlength = attrp->hist_dt.db_length;
    old_max_cell = *max_cellp;

    /* 
    ** If column length is 8 or less, do not truncate. This is for
    ** backwards compatibility.
    */ 
    if (attrp->hist_dt.db_length <= OPH_NH_LENGTH)	/* old maximum of 8 */
	return;

    /* See if the histogram is exact or inexact, and compute the boundary value
    ** length accordingly. exacthist and inexacthist are global variables
    ** pointing to the exact and inexact histograms for the current column.
    */
    if (exact_histo)
	maxlength = exact_histo_length(*max_cellp, cell_count, intervalp,
				       oldhistlength);
    else
	maxlength = inexact_histo_length(adfcb, relp, attrp, *max_cellp, 
					cell_count, intervalp, oldhistlength);
    if (maxlength >= oldhistlength)
	return;				/* No truncation necessary */

    /* If maximum length is less than 8, truncate to 8 */
    if (maxlength < OPH_NH_LENGTH)
	maxlength = OPH_NH_LENGTH;

    /* Truncate boundary values to maxlength. This is done by shifting values
    ** to their new places in the same buffer used to hold pre-truncated values.
    ** If the histogram is exact, some cells may have to be coalesced.
    */
    attrp->hist_dt.db_length = maxlength;
    if (exact_histo)
    {
	/* The histogram is exact, which means that in addition to shifting the
	** values, the lower boundary value for each cell must be re-computed
	** based on the new length, since it may now be the same as the
	** previous value due to truncation; if it is the same as the previous
	** boundary value, the cell is coalesced with the previous one. The
	** routine add_exact_cell() does all this, so we use it.
	** If coalescing occurs, this also means decrementing the number of
	** cells and shifting the cell counts as well.
	*/
	DB_DATA_VALUE	prev_hdt;	/* Holds data value of previous cell */ 
	i4		newcell;	/* Counter for new cells - may be less
					** than cell because of coalescing */

	STRUCT_ASSIGN_MACRO(attrp->hist_dt, prev_hdt);
	prev_hdt.db_data = intervalp;

	for (cell = 1, newcell = -1; cell <= old_max_cell; cell+= 2)
	{
	    OPN_PERCENT	savecount;	/* For saving current cell's count */
	    
	    /* Point at old location of current non-zero cell */
	    attrp->attr_dt.db_data = attrp->hist_dt.db_data =
		intervalp + oldhistlength * cell;
	    /* Copy count for current cell; add_exact_cell overwrites it */
	    savecount = cell_count[cell];
	    
	    newcell = add_exact_cell(attrp, relp, &prev_hdt, newcell,
				     old_max_cell + 1, adfcb);
	    
	    /* Copy count to new cell's location (may be lower due to
	    ** coalescing */
	    cell_count[newcell] = savecount;
	    
	    prev_hdt.db_data = intervalp + maxlength * (newcell);
	}

	*max_cellp = newcell;	/* Number of cells may be lower because
					** of coalescing */
    }
    else
    {
	/* The histogram is inexact, so only shifting of values and
	** re-computing of first boundary value is required.
	** No coalescing is necessary because all boundary values are guaranteed
	** unique after truncation: opn_diff(), which is used to determine
	** truncation length, does not stop a diff computation until after a
	** common prefix.
	** (kibro01) b120041 - The above is not strictly true if exact cells
	** are entered into an inexact histogram.  If two cells are identical
	** after truncation, coalesce them (were the above true this would
	** simply never happen).
	*/
	DB_STATUS	adc_status;	/* For call to adc_hdec */
	DB_DATA_VALUE ocell,ncell;
	i4 old_cell;

	oldloc = intervalp + oldhistlength;
	newloc = intervalp + maxlength;

	lastloc = intervalp;

	STRUCT_ASSIGN_MACRO(attrp->hist_dt,ocell);
	STRUCT_ASSIGN_MACRO(attrp->hist_dt,ncell);
	ocell.db_length = maxlength;
	ncell.db_length = maxlength;

	for (old_cell = 1, cell = 1; cell <= old_max_cell; cell++)
	{
	    ocell.db_data = lastloc;
	    ncell.db_data = oldloc;

	    if (compare_values(adfcb, &ncell, &ocell, &attrp->attname) > 0)
	    {
	        MEcopy((PTR)oldloc, maxlength, (PTR)dummy_buffer);
	        MEcopy((PTR)dummy_buffer, maxlength, (PTR)newloc);
	        lastloc = newloc;
	        oldloc += oldhistlength;
	        newloc += maxlength;
		/* We've coalesced a cell due to truncation */
		if (cell != old_cell)
		{
		    cell_count[old_cell] = cell_count[cell];
		    uniquepercell[old_cell] = uniquepercell[cell];
		}
		old_cell++;
	    } else
	    {
	        oldloc += oldhistlength;
		cell_count[old_cell]+=cell_count[cell];
		uniquepercell[old_cell]+=uniquepercell[cell];
	    }
	}
	*max_cellp = (old_cell - 1);

	/* Call adc_hdec() to re-compute the first boundary value; it was
	** decremented before, but the truncation has wiped out the original
	** decrementation. Decrementation is done in place. */
	attrp->hist_dt.db_data = (PTR)intervalp;
	adc_status = adc_hdec(adfcb, &attrp->hist_dt, &attrp->hist_dt);
	if (DB_FAILURE_MACRO(adc_status))
	    opq_adferror(adfcb, (i4)E_OP0929_ADC_HDEC, (i4)0,
			 (PTR)&attrp->attname, (i4)0, (PTR)&relp->relname);
    }
    
    /* Shift character set statistics also */
    oldloc = intervalp + (old_max_cell + 1) * oldhistlength;
    newloc = intervalp + (*max_cellp + 1) * maxlength;
    MEcopy((PTR)(oldloc), maxlength * sizeof(i4), (PTR)(newloc));
    oldloc += oldhistlength * sizeof(i4);
    newloc += maxlength * sizeof(i4);
    MEcopy((PTR)(oldloc), maxlength * sizeof(f4), (PTR)(newloc));
    /*
    ** Bugs 81133 and 81210 - Make sure the last charnunique is at least
    ** 2 after histogram truncation (extension of fix for bug 59298).
    */
    if (charnunique[maxlength-1] < 2)
    {
	charnunique[maxlength-1] = 2;
	offset = intervalp + (*max_cellp + 1) * maxlength;
	MEcopy((PTR)charnunique, (maxlength * sizeof(i4)), (PTR)offset);
    }

    return;
}

/*{
** Name: add_char_stats - Add character set statistics to histogram
**
** Description:
**	Computes character set statistics from the character bitmaps and stores
**	them in the character statistics arrays, which are in the histogram
**	buffer after the boundary values; these arrays have already been
**	allocated.
**
**	See Character Histogram Enhancements project spec for details.
**
**	Called by: process()
**	
** Inputs:
**	histlength	Length of a histogram boundary value
**      max_cell	Maximum cell number in histogram
**      intervalp       Array of boundary values delimiting histogram cells.
**      		Character set statistics arrays will immediately follow
**      		the boundary values in this buffer.
**	charsets	Pointer to array of character set bitmaps.
**
** Outputs:
**      intervalp	Character set statistics arrays are stored immediately
**      		after the boundary values in this buffer.
**
** Returns:
**	None.
**	   
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**      26-apr-93 (rganski)
**          Initial creation for Character Histogram Enhancements project, from
**          part of insert_iihistogram(). Changed to MEcopy() method to avoid
**          alignment problems (FIXME: is there a more space-efficient way to
**          do this, without allocating separate arrays?).
**	25-apr-94 (rganski)
**	    b59298: make sure the charnunique for the last character position
**	    is at least 2; this ensures that there will be a diff of 1 between
**	    a boundary value and the decremented boundary value (e.g. between
**	    the first two boundary values in the histogram).
**	28-may-03 (inkdo01)
**	    Tweak computation of char stats for alt collations to ignore chars
**	    > 0x80 (they distort stats and cause later probs) - b109507.
**      08-feb-2008 (horda03) Bug 119853
**          If the difference between the min/max char is 0, set difference to
**          1 to prevent Division By 0 exception.
*/
static VOID
add_char_stats(
OPS_DTLENGTH	histlength,
i4		max_cell,
OPH_CELLS	intervalp,
OPQ_BMCHARSET	charsets)
{
    i4			i;		/* Loop counter */
    i4			bitrange = OPQ_BMCHARSETBITS;
    OPQ_BMCHARSET	charset = charsets;
    OPH_CELLS		offset;		/* Offset into intervalp at which
					** character set statistics are to be
					** located */
    /* charnunique and chardensity are global arrays */
    if ((PTR)collationtbl != NULL)
	bitrange /= 2;		/* eliminate chars > 0x80 */

    for (i = 0; i < histlength; i++)
    {
	i4	maxdiff;	/* Difference between minimum and maximum
				** characters in character position i */

	/* Count number of unique chars in character position i */
	charnunique[i] = (i4) BTcount((char *)charset, bitrange);

	/* Compute character set density for position i */
	if (charnunique[i] == 1)
	    chardensity[i] = 1.0;
	else
	{
	    /* Compute difference between minimum and maximum characters */
	    maxdiff = BThigh((char *)charset, bitrange) -
		      BTnext((i4)-1, (char *)charset, bitrange);

            if (!maxdiff) maxdiff = 1; /* prevent division by 0 */

	    chardensity[i] = (f4)(charnunique[i] - 1) / (f4)maxdiff;
	}

	charset += OPQ_BMCHARSETBYTES;	/* Go to next character set bitmap */
    }

    /* Make sure the last charnunique is at least 2  - b59298 */
    if (charnunique[histlength-1] < 2)
	charnunique[histlength-1] = 2;

    /* Copy character set statistics to histogram buffer */
    offset = intervalp + (max_cell + 1) * histlength;
    MEcopy((PTR)charnunique, histlength * sizeof(i4), (PTR)offset);
    offset += histlength * sizeof(i4);
    MEcopy((PTR)chardensity, histlength * sizeof(f4), (PTR)offset);
	
    return;
}

/*{
** Name: read_char_stats - Read character set statistics from file
**
** Description:
**	Reads character set statistics from input file and stores
**	them in the character statistics arrays, which are in the histogram
**	buffer after the boundary values; these arrays have already been
**	allocated.
**
**	See Character Histogram Enhancements project spec for details.
**
**	Called by: read_process()
**	
** Inputs:
**	inf		Input file descriptor
**      statbuf       Buffer in which to store character set statistics in
**      		numeric form; number of unique characters per character
**      		position, immediately followed by character set density
**      		per character position.
**	histlength	Length of histogram boundary values
**
** Outputs:
**      statbuf		Character set statistics arrays are stored immediately
**      		after the boundary values in this buffer.
**
** Returns:
**	None.
**	   
** Exceptions:
**	None.
**
** Side Effects:
**	Advances position in input file.
**
** History:
**      26-apr-93 (rganski)
**          Initial creation for Character Histogram Enhancements project.
**	29-apr-93 (rganski)
**	    Replaced scanning code with default character set statistics;
**	    FIXME: replace fscanf() with alternative.
**	24-may-93 (rganski)
**	    Replaced fscanf() code with combination of SIgetrec() and
**	    opq_scanf(), with provisions for lines longer than OPQ_LINE_SIZE
**	    (7870 bytes). Only the character set densities line can be longer
**	    than OPQ_LINE_SIZE.
**	19-Apr-07 (kibro01) b118074
**	    Statistics from a blank table produce nothing after the first
**	    density value, and thus crashed doing STlength (not checking if
**	    the string continues further and doing STchr on it).
*/
static VOID
read_char_stats(
OPQ_GLOBAL	*g,
FILE	    	*inf,
OPH_CELLS	statbuf,
OPS_DTLENGTH	histlength)
{
    DB_STATUS	stat;
    i4		acnt;
    i4		nunique;	/* Read each nunique into this */
    f8		density;	/* Read each density into this; opq_scanf
				** requires f8 for floats */
    OPN_PERCENT	odensity;	/* Used for copying density to statbuf */
    i4		numstats;	/* Counter for number of stats read */
    char	*linep;		/* Pointer for scanning opq_line, which is a
				** global variable */
    bool	got_whole_line;
    
    /* Read unique chars line */
    stat = SIgetrec(opq_line, sizeof(opq_line), inf);
    if (stat != OK)
    {
	if (stat != ENDFILE)
	{
	    char	buf[ER_MAX_LEN];
	    
	    if ((ERreport(stat, buf)) != OK)
	    {
		buf[0] = '\0';
	    }
	    
	    opq_error((DB_STATUS)E_DB_ERROR,
		      (i4)E_OP0940_READ_ERR, (i4)4,
		      (i4)sizeof(stat), (PTR)&stat,
		      (i4)0, (PTR)buf);
	    (*opq_exit)();
	}
	return;
    }
    linep = opq_line;

    /* Read first nunique */
    acnt = opq_scanf(linep, TEXT9I, g->opq_adfcb->adf_decimal.db_decimal,
		     (PTR)&nunique, (PTR)0, (PTR)0, (PTR)0, (PTR)0);
    if (acnt != 1)
    {
	/* "unique chars: %d" not found where it should be */
	opq_error((DB_STATUS)E_DB_ERROR,
		  (i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
		  (i4)0, (PTR)TEXT9I);
	(*opq_exit)();
    }

    /* Copy first nunique to histogram buffer and advance line pointer to
    ** first nunique */
    MEcopy((PTR)&nunique, sizeof(i4), (PTR)statbuf);
    statbuf += sizeof(i4);
    linep = STchr(linep, ':');
    linep++;
    linep = STskipblank(linep, STlength(linep));

    /* Read the rest of the nuniques and copy them to histogram buffer also. */
    for (numstats = 1; numstats < histlength; numstats++)
    {
	/* Advance line ptr past previous nunique */
	linep = STchr(linep, ' ');
	if (linep == NULL)
	    break;
	linep = STskipblank(linep, STlength(linep));

	acnt = opq_scanf(linep, TEXT12I,
			 g->opq_adfcb->adf_decimal.db_decimal, (PTR)&nunique,
			 (PTR)0, (PTR)0, (PTR)0, (PTR)0);
	if (acnt != 1)
	    break;

	MEcopy((PTR)&nunique, sizeof(i4), (PTR)statbuf);
	statbuf += sizeof(i4);
    }

    if (numstats < histlength)
    {
	/* Not enough nuniques were found */
	opq_error((DB_STATUS)E_DB_ERROR,
		  (i4)E_OP0975_STAT_COUNT, (i4)4,
		  (i4)sizeof(numstats), (PTR)&numstats,
		  (i4)sizeof(histlength), (PTR)&histlength); 
	(*opq_exit)();
    }
    
    /* Read char set densities
    ** Note: char set densities line can be very long, so it may take more than
    ** one call to SIgetrec() */

    stat = SIgetrec(opq_line, sizeof(opq_line), inf);
    if (stat != OK)
    {
	if (stat != ENDFILE)
	{
	    char	buf[ER_MAX_LEN];
	    
	    if ((ERreport(stat, buf)) != OK)
	    {
		buf[0] = '\0';
	    }
	    
	    opq_error((DB_STATUS)E_DB_ERROR,
		      (i4)E_OP0940_READ_ERR, (i4)4,
		      (i4)sizeof(stat), (PTR)&stat,
		      (i4)0, (PTR)buf);
	    (*opq_exit)();
	}
	return;
    }
    linep = opq_line;

    numstats = 0;

    /* Read first density */
    acnt = opq_scanf(linep, TEXT10I, g->opq_adfcb->adf_decimal.db_decimal,
		     (PTR)&density, (PTR)0, (PTR)0, (PTR)0, (PTR)0);
    if (acnt != 1)
    {
	/* "char set densities: %f" not found where it should be */
	opq_error((DB_STATUS)E_DB_ERROR,
		  (i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
		  (i4)0, (PTR)TEXT10I);
	(*opq_exit)();
    }

    /* Convert first density to OPN_PERCENT, copy to histogram buffer and
    ** advance line pointer past first density */
    odensity = (OPN_PERCENT)density;
    MEcopy((PTR)&odensity, sizeof(OPN_PERCENT), (PTR)statbuf);
    statbuf += sizeof(OPN_PERCENT);
    numstats++;
    linep = STchr(linep, ':');
    linep++;
    linep = STskipblank(linep, STlength(linep));
    linep = STchr(linep, ' ');
    if (linep == NULL)
    {
	/* This occurs when no statistics actually exist for the table.
	** (kibro01) b118074
	*/
	return;
    }
    linep = STskipblank(linep, STlength(linep));

    got_whole_line = FALSE;	/* Start with pessimistic assumption */

    while (!got_whole_line)
    {
	/* See if we got the whole line. If not, have to push back the end of
	** the line */
	if (STrchr(linep, '\n') == NULL)
	{
	    /* Didn't get the whole line. Call SIungetc() to push back any
	    ** partial density at end of line, until last blank is reached */

	    char	*linep2;

	    linep2 = linep + STlength(linep);
	    while (*(--linep2) != ' ')
		SIungetc(*linep2, inf);
	    *linep2 = '\0';
	}
	else
	    got_whole_line = TRUE;
	
	/* Read the densities in the line and copy them to histogram buffer */
	for(;;)
	{
	    acnt = opq_scanf(linep, TEXT11I,
			 g->opq_adfcb->adf_decimal.db_decimal, (PTR)&density,
			 (PTR)0, (PTR)0, (PTR)0, (PTR)0);
	    if (acnt != 1)
		break;

	    /* Convert to OPN_PERCENT and copy to statbuf */
	    odensity = (OPN_PERCENT)density;
	    MEcopy((PTR)&odensity, sizeof(OPN_PERCENT), (PTR)statbuf);
	    statbuf += sizeof(OPN_PERCENT);
	    numstats++;

	    /* Advance line ptr past copied density */
	    linep = STchr(linep, ' ');
	    if (linep == NULL)
		break;
	    linep = STskipblank(linep, STlength(linep));
	}

	if (!got_whole_line)
	{
	    stat = SIgetrec(opq_line, sizeof(opq_line), inf);
	    if (stat != OK)
	    {
		if (stat != ENDFILE)
		{
		    char	buf[ER_MAX_LEN];
		    
		    if ((ERreport(stat, buf)) != OK)
		    {
			buf[0] = '\0';
		    }
	    
		    opq_error((DB_STATUS)E_DB_ERROR,
			      (i4)E_OP0940_READ_ERR, (i4)4,
			      (i4)sizeof(stat), (PTR)&stat,
			      (i4)0, (PTR)buf);
		    (*opq_exit)();
		}
		return;
	    }
	    linep = opq_line;
	}
    }

    if (numstats < histlength)
    {
	/* Not enough densities were found */
	opq_error((DB_STATUS)E_DB_ERROR,
		  (i4)E_OP0975_STAT_COUNT, (i4)4,
		  (i4)sizeof(numstats), (PTR)&numstats,
		  (i4)sizeof(histlength), (PTR)&histlength); 
	(*opq_exit)();
    }

    return;
}

/*{
** Name: record_chars - Record characters present in a character data value
**
** Description:
**	Records which ASCII characters are present in a data value from a
**	character-type columns in an array of character set bitmaps, one bitmap
**	per character position.
**	E.g. if the data value is 'FOX', the following bits will be set: bit 70
**	(ASCII 'F') of bitmap 0, bit 79 (ASCII 'O') of bitmap 1, and bit 88
**	of (you guessed it) bitmap 2.
**	The data value is actually the histogram value corresponding to a data
**	value derived from the column.
**	Used for generating character set statistics.
**	See Character Histogram Enhancements project spec for details.
**
**	Called by: process()
**	
** Inputs:
**	attrp		Pointer to attribute descriptor
**	   hist_dt
**	     db_data	Pointer to new histogram value
**	     db_length	Length of new histogram value
**	charsets	Pointer to array of character set bitmaps
**
** Outputs:
**	charsets	Bits corresponding to characters in histogram value are
**			set.
**
** Returns:
**	None.
**	   
** Exceptions:
**	None.
**
** Side Effects:
**	None.
**
** History:
**      26-apr-93 (rganski)
**          Initial creation for Character Histogram Enhancements project.
**	23-aug-93 (rganski)
**	    Coerced value[i] to unsigned char before coercing to nat. Otherwise
**	    high ascii characters are considered negative, which corrupts
**	    memory below charsets.
*/
static VOID
record_chars(
OPQ_ALIST	*attrp,
OPQ_BMCHARSET	charsets)
{
    i4  		i;	/* Loop counter */
    i4  		length = attrp->hist_dt.db_length;
    char		*value = (char *)attrp->hist_dt.db_data;
    OPQ_BMCHARSET	charset = charsets;

    for (i = 0; i < length; i++)
    {
	BTset((i4)(unsigned char)value[i], (char *)charset);
	charset += OPQ_BMCHARSETBYTES;	/* Go to next character set bitmap,
					** corresponding to next character in
					** value */
    }

    return;
}

/*{
** Name: comp_selbuild	- build select expression for composite histogram
**
** Description:
**      This routine loops over attr descriptors for composite histogram,
**	building the select expression which will return the values to be
**	histogrammed. It will look something like:
**	    "unhex(char(hex(col1))+char(hex(char(col2)))+ifnull(x'00'+col3, x'010000..00')
**	    + ...)"
**	where the "char" coercion is used for varying length string columns
**	and the "ifnull" convention is used for nullable columns. The intent
**	is to create a fixed length string in which the values of the constituent
**	columns are at the same offset for every row retrieved.
**
** Inputs:
**	g		    ptr to global data struct
**      relp                ptr to relation descriptor to be analyzed
**      attrpp              ptr to ptrs to attribute descriptors in relation
**                          to be analyzed. 1st in list is current attr. Ptr
**			    vector is passed in case we're building composite
**	selexpr		    ptr to resulting select expression
**
** Outputs:
**	Returns:
**	    bool	    TRUE	if no error
**			    FALSE	if something didn't work
**	Exceptions:
**	    none
**
** History:
**	6-apr-00 (inkdo01)
**	    Written to support composite histograms.
**	25-apr-01 (inkdo01)
**	    Minor fix for var length columns.
**      13-feb-03 (chash01) x-integrate change#461908
**          Two problems.  We should use STprintf, not sprintf; and
**          &attrp->delimname is a structure pointer, not a string address,
**          cast it with (char *). 
**	6-apr-05 (toumi01)
**	    Fix up composite histogram handling:
**	    - add support for NVCHR
**	    - fix string building for handling nulls
**		- make code reentrant (we were using static mem)
**		- refresh buffer for each field (we were corrupting static mem)
**		- the buffer was too small by half
**		- check max length before string handling to avoid mem overflow
**		- move string to end of stack for alignment efficiency
**	7-Nov-08 (kibro01) b121063
**	    Mark that we need to avoid collation if this composite isn't
**	    all made up of character fields.
*		
*/
static bool
comp_selbuild(
OPQ_GLOBAL	*g,
OPQ_RLIST       *relp,
OPQ_ALIST       **attrpp,
char		**selexpr,
bool		*use_hex)
{
    OPQ_ALIST		*attrp;
    DB_DATA_VALUE	compdt;
    char		*wrktxt1, *wrktxt2;
    char		*coltxt1, *coltxt2;
    i4			datlen, i;
    bool		firstcol = TRUE;
    char		nullstr[2*MAXCOMPOSITE+5]; /* '01[00*MAXCOMPOSITE]'\0 */

    /* Start by initializing the data type descriptor for the 
    ** composite "column". */
    compdt.db_datatype = DB_BYTE_TYPE;
    compdt.db_length = 0;
    compdt.db_prec = 0;
    compdt.db_data = (PTR) NULL;

    /* Allocate string buffer space. */
    opq_bufalloc((PTR)g->opq_utilid, (PTR *)selexpr, (i4)1000, &opq_sbuf1);
    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&wrktxt1, (i4)1000, &opq_cbuf1);
    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&wrktxt2, (i4)1000, &opq_cbuf2);

    /* Now loop over constituent attr descriptors, building select
    ** expression and accumulating composite column length as we go. */

    for (i = 0; (attrp = attrpp[i]); i++)
    {
	if (!collatable_type(attrp->attr_dt.db_datatype))
	    *use_hex = TRUE;
    }
    /* But not if there's no collation anyway */
    if (g->opq_adfcb->adf_collation == NULL)
	*use_hex = FALSE;

    for (i = 0; (attrp = attrpp[i]); i++)
    {
	coltxt1 = wrktxt1;

	switch (abs(attrp->attr_dt.db_datatype)) {
	  case DB_VCH_TYPE:
	  case DB_VBYTE_TYPE:
	  case DB_VBIT_TYPE:
	  case DB_TXT_TYPE:
	  case DB_NVCHR_TYPE:
	    STprintf(coltxt1, "byte(hex(char(%s)))", (char *)&attrp->delimname);
			/* varying strings need "char" coercion */
	    datlen = attrp->attr_dt.db_length - 2;
			/* remove length */
	    break;
	  default:
	    STprintf(coltxt1, "byte(hex(%s))", (char *)&attrp->delimname);
			/* the rest are trivial */
	    datlen = attrp->attr_dt.db_length;
	    break;
	}

	/* roll length into composite total and fail if too big */
	compdt.db_length += datlen;
	if (compdt.db_length > MAXCOMPOSITE) return(FALSE);

	/* If the column is nullable, the expression becomes:
	**   "byte(ifnull('00'+char(hex(col)), '010000...00'))". That is, 
	** we stick a null indicator byte in front, and the nulls
	** are all zeroes, of the appropriate length. */
	if (attrp->attr_dt.db_datatype < 0)
	{
	    MEfill(2*datlen+1, '0', (PTR)nullstr);
	    				/* start with	000[00xdatlen-1]??  */
	    nullstr[0] = '\'';		/* begin quote	'00[00xdatlen-1]??  */
	    nullstr[2] = '1';		/* null ind	'01[00xdatlen-1]??  */
	    nullstr[2*datlen+1] = '\'';	/* end quote	'01[00xdatlen-1]'?  */
	    nullstr[2*datlen+2] = 0;	/* term string	'01[00xdatlen-1]'\0 */
	    coltxt1 = wrktxt2;
	    coltxt2 = wrktxt1;

	    sprintf(coltxt1, "byte(ifnull('00'+%s, %s))", 
				coltxt2, nullstr);
	}

	/* add string to eventual select expression. */
	if (firstcol) sprintf(*selexpr, "unhex(%s", coltxt1);
	else sprintf(*selexpr, "%s + %s", *selexpr, coltxt1);
	firstcol = FALSE;
	}

    /* Add the terminating paren, reset the first attr descriptor and
    ** discard the remaining attr descriptors. */
    sprintf(*selexpr, "%s)", *selexpr);
    attrp = attrpp[0];
    STRUCT_ASSIGN_MACRO(compdt, attrp->attr_dt);
    STRUCT_ASSIGN_MACRO(compdt, attrp->hist_dt);
    attrp->userlen = attrp->attr_dt.db_length;
    attrpp[1] = (OPQ_ALIST *)NULL;
    return(TRUE);
}

/*{
** Name: exinex_insert	- insert exact cell into inexact histogram
**
** Description:
**      This routine will insert an exact cell into an inexact histogram 
**	for a value whose count is out of proportion with the rest of the
**	counts. This allows OPF to be more accurate in its use of histograms
**	to evaluate where clause selectivity.
**
** Inputs:
**	g		    ptr to global data struct
**      relp                ptr to relation descriptor to be analyzed
**      attrpp              ptr to ptrs to attribute descriptors in relation
**                          to be analyzed. 1st in list is current attr. Ptr
**			    vector is passed in case we're building composite
**	sample		    boolean indicating if this histogram is sampled
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** History:
**      29-may-01 (inkdo01)
**	    Split from process() to allow check for exact cell at end of hist.
**	21-oct-02 (inkdo01)
**	    Minor fix for histos whose first valued cell is the minimum value for the
**	    data type (e.g. an empty "c" string is x'00').
**	30-mar-05 (inkdo01)
**	    Small hack to keep histlength values smallish. If cells are of
**	    large char/varchar/Unicode, the new cell with the decrement
**	    makes the diff between values not start 'til the last byte and
**	    that is what controls cell length.
**	28-apr-05 (inkdo01)
**	    Above hack led to consistency errors (OP03A2, possibly OP0393)
**	    and has been reworked and placed in inexact_histo_length().
*/
static VOID
exinex_insert(
OPQ_GLOBAL	*g,
OPQ_RLIST       *relp,
OPQ_ALIST       *attrp,
OPQ_MAXELEMENT	*prev_val,
OPQ_MAXELEMENT	*prev1_val,
OPO_TUPLES	*rowest,
i4		*cellinexact,
i4		*cellinexact1,
OPO_TUPLES	currval_count)

{
    DB_DATA_VALUE	tdvs, tdvd, tdvp;
    DB_STATUS		tstat;
    i4			boundcount = 0, cmp_res;
    float		boundpercell = 0.0;
    OPS_DTLENGTH	histlength = attrp->hist_dt.db_length;
    bool                newlow = TRUE;

    /* Many rows had previous value. Insert boundary to
    ** make this exact cell in inexact histogram to 
    ** improve "=" selectivity estimates on it. 
    **
    ** First, copy previous value to histogram and
    ** decrement it (as lower bound). Then copy same
    ** value to next histogram cell. Move counts to 
    ** new cell and zero count in lower bound. */

    MEcopy((PTR)prev_val, histlength,
	(PTR)(inexacthist + (*cellinexact1) * histlength));
    MEcopy((PTR)prev_val, histlength,
	(PTR)(inexacthist + ((*cellinexact1)+1) * histlength));
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, tdvs);
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, tdvd);
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, tdvp);
    tdvs.db_data = (PTR)(inexacthist + ((*cellinexact1)+1) *
	histlength);
    tdvd.db_data = (PTR)(inexacthist + (*cellinexact1) *
	histlength);
    tdvp.db_data = (PTR)prev1_val;

    tstat = adc_hdec(g->opq_adfcb, &tdvs, &tdvd);
				/* decrement from srce to dest */
    if (DB_FAILURE_MACRO(tstat))
	opq_adferror(g->opq_adfcb, (i4)E_OP0929_ADC_HDEC,
		  (i4)0, (PTR)&attrp->attname, (i4)0,
		  (PTR)&relp->relname);
    *rowest -= inexactcount[(*cellinexact1)];
				/* adjust remaining row count */

    /* if decremented current value is the same as last
    value, use the slot for current value */
    if (*cellinexact1 > 0 &&
	MEcmp((PTR)(inexacthist + ((*cellinexact1)-1) * histlength),
	    (PTR)(inexacthist + (*cellinexact1) * histlength),
                                        histlength) == 0 )
    {
	newlow = FALSE;
	MEcopy((PTR)prev_val, histlength,
	    (PTR)(inexacthist + (*cellinexact1) * histlength));
    }
    /* Now see if current cell accounts for other values,
    ** too. If so, add yet another cell to separate them 
    ** from the multiply occurring one. */
    cmp_res = compare_values(g->opq_adfcb,
			&tdvd, &tdvp, &attrp->attname);
    if (cmp_res >= 0)
    {
	if (inexactcount[(*cellinexact1)] > currval_count &&
	    (MEcmp((PTR)prev1_val, (PTR)prev_val, histlength) != 0))
	 if (cmp_res > 0)
	 {
	    /* Prev val is diff from lower bound,
	    ** insert yet another cell. */
	    MEcopy((PTR)tdvs.db_data, histlength,
		(PTR)(inexacthist+((*cellinexact1)+2)*histlength));
	    MEcopy((PTR)tdvd.db_data, histlength,
		(PTR)tdvs.db_data);
	    MEcopy((PTR)prev1_val, histlength,
		(PTR)tdvd.db_data);
				/* copy earlier value in front */
	    uniquepercell[(*cellinexact1)]--;
	    inexactcount[(*cellinexact1)] -= currval_count;
				/* fiddle counts */
	    (*cellinexact1)++;
	 }
	 else
	 /* curr cell has mult vals, but previous
	 ** val is identical to lower boundary */
	 {
	    boundcount = inexactcount[(*cellinexact1)] - currval_count;
	    boundpercell = uniquepercell[(*cellinexact1)]-1.0;
	 }
    }
    else
    {
	newlow = FALSE;
	MEcopy((PTR)prev_val, histlength,
	    (PTR)(inexacthist + (*cellinexact1) * histlength));
    }

    if (newlow == TRUE)
	(*cellinexact1)++;

    uniquepercell[(*cellinexact1)] = 1.0;  /* exact cell */
    /* copy count to new cell */
    if (MEcmp((PTR)prev1_val, (PTR)prev_val, histlength) != 0 ||
		*cellinexact1 == 1)
	inexactcount[(*cellinexact1)] = currval_count;
    else inexactcount[(*cellinexact1)] = inexactcount[(*cellinexact1)-1];

    if (newlow == TRUE)
    {
	uniquepercell[(*cellinexact1)-1] = boundpercell;
	inexactcount[(*cellinexact1)-1] = boundcount;
    }

    /* And finally, initialize new inexact cell */
    (*cellinexact1)++;
    (*cellinexact)++;
    inexactcount[(*cellinexact1)] = 0.0;
    uniquepercell[(*cellinexact1)] = 0.0;
}


/*{
** Name: exinex_insert_rest - insert remaining entries from hcarray as
**	exact cells in the inexact histogram.
**
** Description:
**      This routine will traverse the hcarray[] array of frequently 
**	occurring values and add those that aren't already exact cells
**	as exact cells in the inexact histogram.
**
** Inputs:
**	adfcb		    ptr to ADF_CB
**      relp                ptr to relation descriptor to be analyzed
**      attrp		    ptr to attribute descriptor of column being
**			    processed. relp, attrp are only here for error
**			    handling
**	hccount		    number of hcarray[] entries occupied
**	cellcount	    ptr to current number of cells in inexact histo.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** History:
**      20-oct-05 (inkdo01)
**	    Written to augment exact cells in inexact histogram.
**      25-jun-2007 (huazh01)
**          insert one less cell if newval is the same as its previous
**          value. Otherwise, histogram cells may become '-2, -1, -2'
**          and cause E_OP03A2 error. Also set inexactcount[] and/or 
**          uniquepercell[] to 0 if its new value is less than 0.
**          Bug 118370.
**	2-Jan-2008 (kibro01) b119583
**	    In case of collation sequences where decrementing a string value
**	    may not produce the next lowest value, check >= instead of = for
**	    reducing back into the previous cell.
**	23-Feb-2009 (kibro01) b121491
**	    Use >= 0 instead of == 0 when checking cmpres results when
**	    working out how many parts of the (prev value - marker - new val)
**	    set to insert, so that if either the prev value or the marker
**	    (the "reduced-by-one" value below an exact histogram cell) is
**	    equal to or lower than the previous histogram cell's upper bound,
**	    we reduce the number of insertions accordingly.
*/
static VOID
exinex_insert_rest(
ADF_CB		*adfcb,
OPQ_RLIST	*relp,
OPQ_ALIST	*attrp,
i4		hccount,
i4		*cellcount)

{
    DB_DATA_VALUE	newdv, prevdv, prev1dv, currdv, markerdv;
					/* for performing comparisons */
    char		*val1p, *val2p;
    OPQ_MAXELEMENT	markerval;
    RCARRAY		temp;		/* for the sort */

    i4			i, j, k;	/* for loop indexes */
    i4			cmpres, newcount, histlength;
    bool		neweqcurr, p1ismarker, p1EqNew;
    DB_STATUS		status;


    /* Init some dv's & other stuff, then loop over hcarray[] looking
    ** for values not already represented by exact cells (hcarray[i].
    ** exactcell == FALSE). */

    histlength = attrp->hist_dt.db_length;
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, newdv);
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, prevdv);
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, prev1dv);
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, currdv);
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, markerdv);
    markerdv.db_data = (PTR)&markerval;

    /* hcarray[] must first be sorted on the values. Otherwise, the 
    ** count adjustments don't work. The sort is descending so the 
    ** insertions are done from the end of the cell to the front. That
    ** allows the hcarray[] count fields to keep the cell counts in sync. */
    for (i = 0; i < hccount-1; i++)
     for (j = 1; j < hccount-i; j++)
    {
	/* Bubbly, bubbly, bubbly ... */
	prevdv.db_data = hcarray[j-1].valp;
	newdv.db_data = hcarray[j].valp;
	cmpres = compare_values(adfcb, &prevdv, &newdv, &attrp->attname);
					/* compare [j-1], [j] */
	if (cmpres < 0)
	{
	    /* if [j-1] < [j], swap 'em. */
	    STRUCT_ASSIGN_MACRO(hcarray[j-1], temp);
	    STRUCT_ASSIGN_MACRO(hcarray[j], hcarray[j-1]);
	    STRUCT_ASSIGN_MACRO(temp, hcarray[j]);
	}
    }

    for (i = 0; i < hccount; i++)
    {
	if (hcarray[i].exactcell)
	    continue;			/* value is already in exact cell */

	/* Make copy of value and decrement to get corresponding 
	** marker value. */
	newdv.db_data = hcarray[i].valp;
	prev1dv.db_data = hcarray[i].pvalp;
	status = adc_hdec(adfcb, &newdv, &markerdv);
					/* decrement from newval to marker */
	if (DB_FAILURE_MACRO(status))
	    opq_adferror(adfcb, (i4)E_OP0929_ADC_HDEC,
		  (i4)0, (PTR)&attrp->attname, (i4)0,
		  (PTR)&relp->relname);

	/* If marker is same as newval's previous, we insert 1 less cell. */
	cmpres = compare_values(adfcb, &prev1dv, &markerdv, &attrp->attname);

	/* If marker is same OR HIGHER than previous, we should insert 1 fewer
	** cell, since there could be collation involved (i.e. decrementing
	** 'O' could give you 'N', when 'N~' could be higher than N but lower
	** than O).  It is not possible to guarantee that decrementing using
	** collation would work, since we could have multiple characters as
	** the lower value, so just use this simpler technique to avoid an
	** error (kibro01) b119583
	*/

	if (cmpres >= 0)
	    p1ismarker = TRUE;
	else p1ismarker = FALSE;

        /* bug: 118370
        ** if newval is same as its previous value, insert 1 less cell 
        */ 
        cmpres = compare_values(adfcb, &prev1dv, &newdv, &attrp->attname);
        if (cmpres == 0)
            p1EqNew = TRUE;
        else
            p1EqNew = FALSE;

	/* Loop over inexact histogram, looking for cell following
	** value to be inserted. */
	for (j = 1; j <= *cellcount; j++)
	{
	    currdv.db_data = inexacthist + j*histlength;
					/* current cell value */
	    cmpres = compare_values(adfcb, &currdv, &newdv, &attrp->attname);
	    if (cmpres < 0)
		continue;		/* current cell < new value */

	    /* Found insertion point - determine which and how many cells
	    ** must be inserted. */
	    prevdv.db_data = inexacthist + (j-1)*histlength;
					/* previous cell value */
	    if (cmpres == 0)
	    {
		/* New value is already in a cell - if marker value is
		** the same as the previous cell (shouldn't really happen,
		** though it might if exact cell arose naturally and not
		** because of exinex_insert() being called), it's already
		** an exact cell. Otherwise we just need to insert cells
		** for the marker value and (possibly) newval's previous
		** value. */
		cmpres = compare_values(adfcb, &prevdv, &markerdv,
							&attrp->attname);

		/* Adjust check to be less than or equal to marker value
		** since collated items (or floats) can end up skipping
		** values in the decrement (kibro01) b121491
		*/
		if (cmpres >= 0)
		    break;		/* skip to next hcarray[] entry */

		neweqcurr = TRUE;
		cmpres = compare_values(adfcb, &prevdv, &prev1dv, 
							&attrp->attname);
		if (cmpres == 0 || p1ismarker || p1EqNew)
		    newcount = 1;	/* if newval's previous is same
					** as previous cell OR marker,
                                        ** or if newval is the same as
                                        ** its previous value,
					** we just insert marker */
		else newcount = 2;
	    }
	    else
	    {
		/* New value is less than current cell. If previous cell
		** is the same as the marker value, just insert newval.
		** Otherwise, insert cells for newval, the marker value
		** and (possibly) newval's previous value. */
		neweqcurr = FALSE;
		cmpres = compare_values(adfcb, &prevdv, &markerdv, 
							&attrp->attname);
		if (cmpres >= 0)
		    newcount = 1;
		else
		{
		    cmpres = compare_values(adfcb, &prevdv, &prev1dv, 
							&attrp->attname);
		    /* Adjust check to be less than or equal to marker value
		    ** since collated items (or floats) can end up skipping
		    ** values in the decrement (kibro01) b121491
		    */
		    if (cmpres >= 0 || p1ismarker || p1EqNew)
			newcount = 2;	/* if newval's previous is same
					** as previous cell OR marker, 
                                        ** or if newval is the same as 
                                        ** it previous value, we
					** only insert marker & newval */
		    else newcount = 3;
		}
	    }

	    /* Cells following point of insertion must now be moved down
	    ** to make room. */
	    for (k = (*cellcount); k >= j; k--)
	    {
		inexactcount[k + newcount] = inexactcount[k];
		uniquepercell[k + newcount] = uniquepercell[k];
		val1p = inexacthist + (k * histlength);
		val2p = inexacthist + ((k + newcount) * histlength);
		MEcopy((PTR)val1p, histlength, (PTR)val2p);
	    }

	    /* Finally do the insertions & adjust existing counts around
	    ** the new cells. */
 
	    if (newcount == 1)
	    {
		if (neweqcurr)
		{
		    /* Previous cell is newval's previous. Cell[j] becomes
		    ** marker (cell[j+1] is already newval). */
		    if (p1ismarker)
		    {
			inexactcount[j] = hcarray[i].pnrows;
			uniquepercell[j] = hcarray[i].puperc;
		    }
		    else
		    {
			inexactcount[j] = 0.0;
			uniquepercell[j] = 0.0;
		    }
		    MEcopy((PTR)&markerval, histlength, 
				(PTR)currdv.db_data);
		}
		else
		{
		    /* Previous cell is the marker. Cell[j] becomes newval
		    ** and cell[j+1]'s count is adjusted accordingly. */
		    inexactcount[j] = hcarray[i].nrows;
		    uniquepercell[j] = 1;
		    inexactcount[j+1] -= hcarray[i].nrows;
                    if (inexactcount[j+1] < 0.0) inexactcount[j+1] = 0.0;

		    uniquepercell[j+1]--;
                    if (uniquepercell[j+1] < 0.0) uniquepercell[j+1] = 0.0;

		    MEcopy((PTR)hcarray[i].valp, histlength,
				(PTR)currdv.db_data);
		}
	    }

	    else if (newcount == 2)
	    {
		if (neweqcurr)
		{
		    /* Current cell is same as newval. Cell[j] becomes
		    ** newval's previous, cell[j+1] becomes marker and j,
		    ** j+2 counts are adjusted accordingly. */
		    inexactcount[j] -= hcarray[i].nrows;
                    if (inexactcount[j] < 0.0) inexactcount[j] = 0.0;

		    uniquepercell[j]--;
                    if (uniquepercell[j] < 0.0) uniquepercell[j] = 0.0;

		    MEcopy((PTR)hcarray[i].pvalp, histlength, 
				(PTR)currdv.db_data);
		    inexactcount[j+1] = 0.0;
		    uniquepercell[j+1] = 0.0;
		    val1p = inexacthist + ((j+1) * histlength);
		    MEcopy((PTR)&markerval, histlength, (PTR)val1p);
		    inexactcount[j+2] = hcarray[i].nrows;
		    uniquepercell[j+2] = 1.0;
		}
		else
		{
		    /* Previous cell is newval's previous value. Cell[j]
		    ** becomes marker, cell[j+1] becomes newval, and j+1
		    ** and j+2 counts are adjusted accordingly. */
		    if (p1ismarker)
		    {
			inexactcount[j] = hcarray[i].pnrows;
			uniquepercell[j] = hcarray[i].puperc;
		    }
		    else
		    {
			inexactcount[j] = 0.0;
			uniquepercell[j] = 0.0;
		    }
		    MEcopy((PTR)&markerval, histlength, (PTR)currdv.db_data);
		    inexactcount[j+1] = hcarray[i].nrows;
		    uniquepercell[j+1] = 1.0;
		    val1p = inexacthist + ((j+1) * histlength);
		    MEcopy((PTR)newdv.db_data, histlength, (PTR)val1p);
		    inexactcount[j+2] -= (hcarray[i].nrows + inexactcount[j]);
                    if (inexactcount[j+2] < 0.0) inexactcount[j+2] = 0.0;

		    uniquepercell[j+2] -= (1.0 + uniquepercell[j]);
                    if (uniquepercell[j+2] < 0.0) uniquepercell[j+2] = 0.0;
		}
	    }

	    else if (newcount == 3)
	    {
		/* Current cell differs from newval, previous differs from 
		** marker value. Cell[j] becomes newval's previous value,
		** cell[j+1] becomes the marker value and cell[j+2] becomes
		** newval. Counts are adjusted in cell[j], cell[j+2] and
		** cell[j+3]. */
		inexactcount[j] = hcarray[i].pnrows;
		uniquepercell[j] = hcarray[i].puperc;
		MEcopy((PTR)hcarray[i].pvalp, histlength,
				(PTR)currdv.db_data);
		inexactcount[j+1] = 0.0;
		uniquepercell[j+1] = 0.0;
		val1p = inexacthist + ((j+1) * histlength);
		MEcopy((PTR)&markerval, histlength, (PTR)val1p);
		inexactcount[j+2] = hcarray[i].nrows;
		uniquepercell[j+2] = 1.0;
		val1p = inexacthist + ((j+2) * histlength);
		MEcopy((PTR)newdv.db_data, histlength, (PTR)val1p);

		inexactcount[j+3] -= (hcarray[i].nrows + hcarray[i].pnrows);
                if (inexactcount[j+3] < 0.0) inexactcount[j+3] = 0.0; 

		uniquepercell[j+3] -= (hcarray[i].puperc + 1.0);
                if (uniquepercell[j+3] < 0.0) uniquepercell[j+3] = 0.0;
	    }

	    (*cellcount) += newcount;
	    break;	/* when we get here, break from the j-loop */
	}

    }
}


/*{
** Name: process_oldq	- create full statistics on the attribute using old
**			row at a time query
**
** Description:
**      This routine will build an exact histogram (based on a limited number
**      of unique values), and an inexact histogram (based on the average
**      tuple count per cell).  The exact histogram is used if it can be
**      completed successfully. This function uses the old simple select
**	that is more efficient for data with low repetition.
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_utilid	    name of utility from which this procedure is called
**        opq-owner         name of database owner
**        opq_dbname        name of database
**	  opq_dbcaps	    database capabilities struct
**	    cat_updateable  mask showing which standard cats are updateable
**	  opq_adfcb	    ptr to ADF control block
**      relp                ptr to relation descriptor to be analyzed
**      attrpp              ptr to ptrs to attribute descriptors in relation
**                          to be analyzed. 1st in list is current attr. Ptr
**			    vector is passed in case we're building composite
**	sample		    boolean indicating if this histogram is sampled
**
** Outputs:
**	Returns:
**	    DB_STATUS	    E_DB_OK	if no error
**			    E_DB_WARN	if no rows
**			    E_DB_ERROR	if sampling attempted when not allowed
**	Exceptions:
**	    none
**
** Side Effects:
**	    update the iihistogram relation with the histogram of the attribute
**
** History:
**      4-dec-86 (seputis)
**          initial creation
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	13-sep-89 (stec)
**	    Return E_DB_OK status when all values are NULL, since
**	    the caller should continue creating stats for the
**	    remaining columns rather than move on to the next table.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	    Added a mechanism in process() which causes two adjacent cells
**	    with identical upper and lower bound to collapse into a single cell.
**	30-mar-90 (stec)
**	    Change code in process() to collapse cells in a slightly
**	    different way. A new cell is to be created only when the newly
**	    computed low boundary is greater than the previous histogram
**	    value, in other words cells will be collapsed when upper and
**	    lower boundaries of adjacent cells are equal or the upper is
**	    greater than newly computed lower. 
**	09-nov-92 (rganski)
**	    Modularization effort. Created routines to do some of the work of 
**	    this routine, and replaced the inline code with calls to these 
**	    smaller routines: compare_values(), new_unique(), decrement_value(),
**	    add_exact_cell(). 
**	    Streamlined some of the logic in the main loop to avoid redundant
**	    and unnecessary calls; the largest change is that most of the 
**	    comparisons, etc., are skipped when the new value is the same as 
**	    the previous one.
**	    Removed redundant call to adc_compare().
**	    Removed local variable not_scattered, since opn_diff() is not
**	    called any more (see add_exact_cell()).
**	18-jan-93 (rganski)
**	    Added dynamic allocation of exacthist and inexacthist. Since
**	    boundary values can now be up to 1950 bytes long, it is inefficient
**	    to statically allocate maximum-size histograms.
**	    Added space for character set statistics, if column is of character
**	    type.
**	26-apr-93 (rganski)
**	    Character Histograms project:
**	    Added calls to add_char_stats(), which computes and stores
**	    character set statistics for character histograms, and to
**	    truncate_histo(), which determines necessary length 
**	    of character histogram boundary values, and truncates them
**	    accordingly. Truncation is only done when the -length flag has not
**	    been used.
**	    Reorganized call to insert_histo() to avoid duplication.
**	    Added recording of character set statistics using array of
**	    character set bitmaps, one bitmap per character position in the
**	    column (if it is of character type, of course). Recording is done
**	    in record_chars(). The bitmaps are turned into character set
**	    statistics by add_char_stats().
**	30-apr-1993 (robf)
**	    When allocating the SQLDA buffer, make sure its big enough to
**	    both tuple data and histogram (possible for histogram to be 
**	    bigger than tuple, causing overflow. Symptom is an AV)
**	23-jul-93 (rganski)
**	    Increased number of cells required for default inexact histogram
**	    when requesting memory, from 15 to 16, which is the actual number
**	    of cells. This was causing MEfree() errors on VMS.
**	20-sep-93 (rganski)
**	    Delimited ID support. Use relp->delimname instead of relp->relname
**	    and ap->delimname instead of ap->attname in select statement.
**	8-jan-96 (inkdo01)
**	    Added support of per-cell repetition factor computation (just
**	    per-cell unique value counts in process()).
**	23-dec-97 (inkdo01)
**	    Added code to insert exact cells into inexact histograms for values
**	    which account for a large proportion of rows.
**	8-jan-98 (inkdo01)
**	    Changed the logic which switches inexact cells to give more
**	    accurate n-tile boundaries in the face of skewed distributions.
**	18-jan-99 (hayke02)
**	    Fix up to exact cell insertion code so that different data values
**	    with the same histogram value (which can happen with date + time
**	    data, eg. 19.10.1998 13:44:02 and 19.10.1998 13:44:07 both have
**	    the histogram value 35012868) are handled correctly. This change
**	    fixes bug 94737.
**      07-sep-99 (sarjo01)
**          Bug 98665: when inserting an exact value with a high rep count
**          into an inexact histogram, make sure check to previous value
**          before inserting the lower bound, it may already be in the
**          histogram.
**	08-feb-2000 (hayke02)
**	    Extend the fix for bug 98665 to check that the 'new' previous
**	    value (prev1_val) is not greater than the decremented exact
**	    value. This can occur if we insert an exact float cell with a
**	    value of 0.0 - the decremented value will be -1e-14 which can be
**	    less than the previous inexact histogram cell value. This
**	    change fixes bug 100032.
**	04-oct-2000 (hayke02)
**	    Modify fix for 100032 so that cmp_res < 0 test is outside of the
**	    test for inexactcount[cellienxact1] > currval_count. This
**	    prevents the same effect as bug 100032 - decremented float value
**	    used for exact histogram cell insertion less than previous 
**	    float value leading to E_OP03A2 - when inexactcount[cellienxact1]
**	    = currval_count and the check for cmp_res < 0 is therefore
**	    bypassed. This change fixes bug 102829.
**	6-apr-00 (inkdo01)
**	    Changes to support composite histograms.
**	05-jul-00 (inkdo01)
**	    Change queries for sampled histograms to reference global temp 
**	    table.
**      28-aug-2000 (chash01) 05-dec-97 (chash01)
**          For RMS-Gateway databases:  if requested then immediately update 
**          the standard catalogs so that the row count of a base table will 
**          be in synch with the row count used to generate statistics for 
**          columns of that base table.
**	10-jul-00 (inkdo01)
**	    Changes to setup for enhanced distinct value estimation technique.
**	12-Mar-2001 (thaju02)
**	    Initialize prev_val with the minimum histogram element for
**	    the pertinent datatype. (B104224)
**	23-may-2001 (hayke02)
**	    Extend above fix to prev1_val. This prevents a SIGBUS when
**	    attempting to add an f8 exact histogram cell and compare_values()
**	    is called for the first histogram value with tdvp.db_data
**	    (prev1_val) filled with garbage. This change fixes bug 104737.
**	29-may-01 (inkdo01)
**	    Replicated to support old and new (aggregate) queries under control
**	    of "-zfq" flag.
**      29-jun-2001 (huazh01)
**	   (hanje04) X-Integ of 451658
**          introduce variable dup_currval_count. Due to overflow error, 
**          currval_count will not be incremented correctly once it reaches 
**          16777216. In such case, use the value of dup_currval_count 
**          insead of the value of currval_count . This fix b105184
**	19-oct-01 (inkdo01)
**	    Minor fix to 23-may cross to accomodate composite histos. too.
**	07-jul-04 (hayke02)
**	    Add another regcells to the size of inexacthist to allow space for
**	    the insertion of > regcells/2 exact histogram cells. This change
**	    fixes problem INGSRV 2687, bug 111758.
**	21-Jan-05 (nansa02)
**	    Table name with delimited identifiers should be enclosed within '"'.
**	    (Bug 113786)
**	28-Jan-2005 (schka24)
**	    We're already using delimname, back out above change.
**	15-apr-05 (inkdo01)
**	    Add support for 0-cell histograms (column is all null).
**	17-oct-05 (inkdo01)
**	    Add support for minimum number of exact cells in inexact histo.
**	9-nov-05 (inkdo01)
**	    Tidy a few things pertaining to Unicode histograms.
**	9-may-06 (dougi)
**	    Tiny change to add one more to first cell count.
**	28-sep-2006 (dougi)
**	    No histogram truncation for composite histos.
**	6-dec-2006 (dougi)
**	    Inhibit SET STATISTICS for empty tables.
**      08-aug-2008 (huazh01)
**          don't update inexactcount[] and exactcount[] if a column
**          has all NULL values. if all null, 'cellexact' and
**          'cellinexact1' is -1, updating those arrays using either
**          'cellinexact1' / 'cellexact' could SEGV. bug 120754.
**	10-Nov-2008 (kibro01) b121063
**	    Don't use collation on a char field which is actually a
**	    composite index of non-char fields.  Sort on its hex value
**	    as well to ensure the DBMS doesn't collate.
**	17-Sep-2009 (wanfr01) b122605
**	    Exact histogram cell count needs to be incremented for unique
**	    values which map to the came histogram cell
*/
static DB_STATUS
process_oldq(
OPQ_GLOBAL	*g,
OPQ_RLIST       *relp,
OPQ_ALIST       **attrpp,
bool		sample)
{
    OPQ_ALIST		*attrp = *attrpp;     /* attribute descriptor ptr */
    OPO_TUPLES		averagepercell;	      /* average number of tuples per
                                              ** histogram cell */
    OPO_TUPLES		rowest;		      /* estimate of rows in table/
					      ** sample */
    OPS_DTLENGTH        histlength;           /* length of a histogram
                                              ** element */
    OPQ_MAXELEMENT      prev_val, prev1_val;  /* histogram element of
					      ** cell boundary */
    i4             row_count;	      /* number of rows in the
					      ** relation, which may be
                                              ** different from what was
                                              ** retrieved from the iirelation
					      */
    i4		totalcells;           /* max number of cells for
                                              ** histogram if values are
                                              ** unique */
    i4                  cellexact;	      /* number of cells defined in
                                              ** exact histogram so far */
    bool                toomany;              /* TRUE if too many 
					      ** cells are required
					      ** for the exact histogram so an
                                              ** inexact histogram is used */
    i4                  cellinexact;	      /* number of cells valued in
					      ** inexact histogram */
    i4			cellinexact1;	      /* cells occupied in inexact
					      ** histogram */
    OPQ_I8COUNT         nunique;              /* number of unique values found
                                              ** in the attribute so far */
    OPQ_I8COUNT		null_count;	      /* number of NULLs in column */
    OPO_TUPLES		f4null_count;	      /* ratio of NULLs in column */
    OPO_TUPLES		currval_count;	      /* count of rows with current
					      ** value, so far */
    IISQLDA		*sqlda = &_sqlda;     /* ptr to SQLDA */
    bool		nulls_only = (bool)TRUE; /* assume all values are NULL,
						 ** until proven otherwise. */
    OPH_CELLS		intervalp;	/* ptr to final histogram cells */
    OPH_CELLS		exinexvalp;	/* ptr to array of values of exact
					** cells in inexact histogram */
    OPN_PERCENT		*cell_count;	/* ptr to final histogram cell counts */
    i4			max_cell;	/* Max cell number in final histogram */
    bool		exact_histo;	/* Is final histogram exact? */
    OPQ_BMCHARSET	charsets;	/* Pointer to array of character set
					** bitmaps */
    DB_DATA_VALUE	fromdv, hmindv;
    DB_DATA_VALUE	prev_hdt;	    /* used to describe the histo data 
					    ** value previously scanned */
    DB_STATUS		tstat;
    char		*selexpr = (char *)&attrp->delimname;  
					      /* ptr to column name
					      ** or composite expression which
					      ** goes in select statement */
    OPQ_I8COUNT		d_exactcount, d_inexactcount, d_currval_count; 
					/* f8 versions of cell counters to
					** prevent mantissa overflow at 16M */
    i4			hccount, hcindex, ecrows;
    i4			i;
    char        sestbl[9];
    bool	use_hex = FALSE;
    PTR		save_collation = g->opq_adfcb->adf_collation;

    if (g->opq_dbms.dbms_type & OPQ_STAR)
    {
        sestbl[0] = 0x0;
    }
    else
    {
        STcopy("session.", sestbl);
    }


# ifdef	x3B5
    /*
    ** 3B5 requires f4s to be initialized or will cause
    ** core dump upon access (e.g. in debugger).
    */

    {
	i4	    i3b5;
	for (i3b5 = 0; i3b5 < MAXCELLS + 1; i3b5++)
	{
		exactcount[i3b5] = inexactcount[i3b5] = 0.0;
	}
    }
# endif	/* x3B5 */

    null_count = 0;
    totalcells = uniquecells;
    attrp->attr_dt.db_collID = attrp->collID;

    /*
    ** initialize prev_val to the minimum histogram
    ** element for the appropriate datatype.
    */
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, fromdv);
    STRUCT_ASSIGN_MACRO(attrp->hist_dt, hmindv);
    hmindv.db_data = (PTR)&prev_val;
    tstat = adc_hmin(g->opq_adfcb, &fromdv, &hmindv);
    if (DB_FAILURE_MACRO(tstat))
	opq_adferror(g->opq_adfcb, (i4)E_OP0789_ADC_HMIN,
		(i4)0, (PTR)&attrp->attname, 
		(i4)0, (PTR)&relp->relname);

    histlength = attrp->hist_dt.db_length;	/* boundary value length */

    MEcopy((PTR)&prev_val, histlength, (PTR)&prev1_val);

    if (sample)
    {
	i4	i;

	if (relp->sampleok == (bool)FALSE)
	{
	    /*
	    ** Sampling on non-Ingres systems not allowed,
	    ** user has already been warned.
	    */
	    return E_DB_ERROR;
	}
	/* Init. the frequency count array for enhanced distinct 
	** value estimation (but only if were using the "new" technique). */
	if (freqvec) for (i = 0; i < MAXFREQ; i++) freqvec[i] = (f4)0.0;
    }


    /* uniquecells should be greater than regcells - check just in case */
    if (totalcells < regcells)
	totalcells = regcells;

    totalcells++;

    /* If building composite histogram, this function builds select
    ** expression which concatenates all the columns. */
    if (relp->index || g->opq_comppkey)
    {
	if (!comp_selbuild(g, relp, attrpp, &selexpr, &use_hex)) return(FALSE);
	histlength = attrp->hist_dt.db_length;	/* bound. len for composite */
	if (use_hex)
	    g->opq_adfcb->adf_collation = NULL;
    }

    /*
    ** note that ntups may be wrong because reltups entry in relation
    ** relation is wrong, but it is a reasonable guess at this point
    */

    /* Allocate exacthist and inexacthist.
    ** If character type, include space for character set
    ** statistics, and allocate character set bitmaps.
    */
    {
	DB_DT_ID	type;	/* Histogram data type */
	u_i4	extra;	/* Extra space required, if any, for character
				** set statistics. */

	type = attrp->hist_dt.db_datatype;
	if (type < (DB_DT_ID)0)
	    type = -type;

	charsets = NULL;
	if (type == DB_CHA_TYPE || type == DB_BYTE_TYPE)
	{
	    /* Extra space needed for histogram buffers */
	    extra = histlength * (sizeof(i4) + sizeof(f4));
	    /* Allocate character set bitmaps */
	    (VOID) getspace((char *)g->opq_utilid, (PTR *)&charsets, 
		(u_i4)(histlength * OPQ_BMCHARSETBYTES)); 
	}
	else
	    extra = 0;

	(VOID) getspace((char *)g->opq_utilid, (PTR *)&exacthist, 
			(u_i4)((histlength + sizeof(OPN_PERCENT)) * 
			totalcells + extra)); 
	(VOID) getspace((char *)g->opq_utilid, (PTR *)&inexacthist,
			(u_i4)((histlength + sizeof(OPN_PERCENT)) *
			(regcells * 3 + exinexcells + 1) + extra)); 
				/* added per-cell unique count array, and
				** space for exact cells in inexact histos */
	exinexvalp = NULL;	/* so getspace() gets fresh buffer */
	(VOID) getspace((char *)g->opq_utilid, (PTR *)&exinexvalp,
			(u_i4)(histlength * exinexcells * 2));
				/* And space for value/previous value array 
				** for exact cells in inexact histogram */
    }

    /* Init. array of counts for exact cells in inexact histograms. */
    {
	i4	i;

	for (i = 0; i < exinexcells; i++)
	{
	    hcarray[i].nrows = 0;
	    hcarray[i].pnrows = 0;
	    hcarray[i].puperc = 0;
	    hcarray[i].valp = exinexvalp;
	    hcarray[i].pvalp = &exinexvalp[histlength];
	    hcarray[i].exactcell = FALSE;
	    exinexvalp = &exinexvalp[2 * histlength];
	}
    }

    /* Estimate number of tuples per cell */
    if (sample == (bool)TRUE)
    {
	averagepercell = (OPO_TUPLES)relp->nsample / (OPO_TUPLES)regcells;
	rowest = (OPO_TUPLES)relp->nsample;
    }
    else
    {
	if (relp->ntups > (OPO_TUPLES)0)
	{
	    averagepercell = (OPO_TUPLES)relp->ntups / (OPO_TUPLES)regcells;
	    rowest = (OPO_TUPLES)relp->ntups;
	}
	else
	{
	    /* When row info missing */
	    averagepercell = 1.0;
	    rowest = 0.0;
	}
    }

    if (averagepercell < 1.0)
	averagepercell = 1.0;

    /*
    **	    try to make a hist where every unique value has its own cell
    **	       and if there is not enough room then go to next retrieve to
    **	       do the normal histogram stuff
    **
    **	    As an example, suppose the values of an attribute are
    **			    1,2,2,3,4,6,20,21
    **		    then the histogram will be in count-value pairs:
    **			    (0,0) (1,1) (2,2) (1,3) (1,4) (0,5)
    **			    (1,6) (0,19), (1,20) (1,21)
    */

    {
	exec sql begin declare section;
	    char	*stmt;		    /* temp buffer to store text
					    ** of target list */
	exec sql end declare section;

	DB_DATA_VALUE	prev_dt;	    /* used to describe the data value
					    ** previously scanned */
    
	/* Initialize prev_dt */
	STRUCT_ASSIGN_MACRO( attrp->attr_dt, prev_dt);
	prev_dt.db_data = (PTR)&prev_full_val;

	/* Initialize prev_hdt */
	STRUCT_ASSIGN_MACRO( attrp->hist_dt, prev_hdt);
	prev_hdt.db_data = (PTR)&prev_val;

	if (relp->index || g->opq_comppkey)
	    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&stmt,
	    (i4)1000, &opq_sbuf);
	else
	{
	   /* 4*(...) because there are 4 items, +2 for quotes if required */
	    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&stmt,
	        (i4)(sizeof("select %s from %s%s order by hex(%s)")) +
	        4 * (DB_ATT_MAXNAME+2), &opq_sbuf);
	}

	if (sample)
	{
		STprintf(stmt, "select %s from %s%s order by %s%s%s",
		    selexpr,
		    sestbl,
		    relp->samplename,
		    use_hex?"hex(":"",
		    use_hex?selexpr:"1",
		    use_hex?")":"");
		sqlda->sqln = 1;
	}
	else
	{
		STprintf(stmt, "select %s from %s order by %s%s%s",
		    selexpr,
		    relp->delimname,
		    use_hex?"hex(":"",
		    use_hex?selexpr:"1",
		    use_hex?")":"");
		sqlda->sqln = 1;
	}

# ifdef xDEBUG
	{
	    long	timeval;
	    struct tm *timeval1;
	    time(&timeval);
	    timeval1 = localtime(&timeval);
	    SIprintf("Start of value query for %s, %s \n", selexpr, asctime(timeval1));
	}
# endif /* (xDEBUG) */
	exec sql prepare s from :stmt;

	exec sql describe s into :sqlda;

	/* allocate buffer, describe location for the column. */
	{
	    sqlda->sqlvar[0].sqltype = DB_DBV_TYPE;
	    sqlda->sqlvar[0].sqllen  = sizeof(DB_DATA_VALUE);
	    sqlda->sqlvar[0].sqldata = (char *)&attrp->attr_dt;

	    /*
	    ** Allocate one for query result and one for converted histogram
	    ** value (Unicode ones expand and get messed up if converted in
	    ** place).
	    */
	    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&attrp->attr_dt.db_data,
		     attrp->attr_dt.db_length, &opq_rbuf);
	    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&attrp->hist_dt.db_data,
		     attrp->hist_dt.db_length, &opq_rbuf1);

	}

	/* retrieve data for the constructed query. */
	exec sql open c1 for readonly;

	toomany	= FALSE;
	cellexact = -1;
	cellinexact = 0;
	cellinexact1 = 0;
	nunique	= 0;
	currval_count = 0.0;
	maxfreq = -1;
	row_count = 1;
	hccount = 0;
	d_exactcount = 0;
	d_inexactcount = 0;
	d_currval_count = 0;

	while (sqlca.sqlcode == GE_OK)
	{
	    exec sql fetch c1 using descriptor :sqlda;

	    if (sqlca.sqlcode != GE_OK)
		break;
	    else
	    {
		/* See if new value is a new unique value */
		if (new_unique(cellinexact, attrp, &prev_dt, g->opq_adfcb))
		{
		    nunique += 1;
		    if (freqvec && d_currval_count > 0)
		    {
			/* Enhanced distinct value estimation requires
			** vector of counts, the "ith" entry of which is
			** the number of values in the sample with "i"
			** duplicates. */
			if (d_currval_count >= MAXFREQ)
			    freqvec[MAXFREQ-1] += 1.0;
			else freqvec[((i4)d_currval_count)-1] += 1.0;

			if (d_currval_count > maxfreq) 
			    maxfreq = (d_currval_count < MAXFREQ) ? 
						d_currval_count : MAXFREQ;
		    }
		    
		    /* create the histogram element in place */
		    if (hist_convert((char *)attrp->attr_dt.db_data,
				     attrp, g->opq_adfcb))
			null_count++;	/* increment NULL count if found */
		    else
		    {
			i4	cmp_result = 0;
			
			/* indicate that a non-NULL value is present */
			nulls_only = (bool)FALSE;
			
			if (cellexact > -1)
			    /* See if the new histogram value is different from 
			    ** the previous one (we have to check, because 
			    ** histogram values can be the same even if the 
			    ** data values are different) 
			    */
			    cmp_result = compare_values(g->opq_adfcb,
						&attrp->hist_dt, &prev_hdt,
						&attrp->attname);
			
			/* Could verify that cmp_result isn't < 0 here. That
			** would be a big boo boo. */

			/* If column is of character type, record characters
			** present in new unique value in the character set bit
			** maps, for character set statistics */
			if ((attrp->hist_dt.db_datatype == DB_CHA_TYPE ||
			     attrp->hist_dt.db_datatype == DB_BYTE_TYPE) &&
			    (cellexact == -1 || cmp_result != 0))
			{
			    record_chars(attrp, charsets);
			}

			if (!toomany) 	/* Still room in exact histogram? */
			{
			    if (cellexact == -1 || cmp_result != 0)
			    {
				/* We either have no cells yet or the current
				** histogram value differs from the previous 
				** one. 
				*/
				if (cellexact > -1)
				{
				    exactcount[cellexact] = 
						(OPN_PERCENT)d_exactcount;
				}
				if (cmp_result != 0)
				    d_exactcount = 1;  /* new cell for this value */
				else 
				    d_exactcount = 0;  /* count row later */
				cellexact = add_exact_cell(attrp,relp, &prev_hdt,
						          cellexact, totalcells,
							  g->opq_adfcb);
				if (cellexact >= totalcells)
				    toomany = TRUE;
			    }
			    else
				exactcount[cellexact] += 1.0;
			}
			/*
			** build the inexact histo here (adjacent values 
			** may be put in the same histogram cell).
			*/
			if (cellinexact == 0)
			{
			    /* Decrement current value and store it in first
			    ** inexact histogram cell.
			    */
			    decrement_value(g->opq_adfcb, attrp, relp,
					    inexacthist);
			    
			    cellinexact++;
			    cellinexact1++;
			    inexactcount[cellinexact1] = 0.0;	/* init cells */
			    d_inexactcount = 0;
			    uniquepercell[cellinexact1] = 0.0;
			}
			else
			{
			    inexactcount[cellinexact1] = 
						(OPN_PERCENT)d_inexactcount;
			    if (cmp_result != 0 && d_currval_count >= 2)
				hcindex = update_hcarray(&prev_hdt, &prev1_val,
					(i4)d_currval_count, 
					(i4)(d_inexactcount - d_currval_count),
					uniquepercell[cellinexact1],
					&hccount);
			    if (cmp_result != 0 && averagepercell > 1.0 && 
				d_currval_count > averagepercell*OPQ_MINEXCELL)
			    {
				exinex_insert(g, relp, attrp, &prev_val, 
				    &prev1_val, &rowest, &cellinexact, 
				    &cellinexact1, (OPO_TUPLES)d_currval_count);
					/* add exact cell to inexact histo. */
				d_inexactcount = 0;
				if (hcindex >= 0)
				    hcarray[hcindex].exactcell = TRUE;
			    }
			    else if ((cmp_result != 0) &&
				(cellinexact < regcells) &&
				((rowest <= 1.0 && 
				(cellinexact + 0.001 < row_count/averagepercell))
				 || (rowest/(regcells-cellinexact+1) <= 
						inexactcount[cellinexact1])))
				/* this cell is full - note the "rowest" formula 
				** dynamically adjusts n-tiles for more balanced
				** cells with skewed data distributions */
			    {
				/* create new inexact cell */
				MEcopy((PTR)&prev_val, histlength, 
				  (PTR)(inexacthist + cellinexact1 * histlength));
			    
				rowest -= inexactcount[cellinexact1];
						/* adjust remaining row count */
				cellinexact++;
				cellinexact1++;
				inexactcount[cellinexact1] = 0.0;
				d_inexactcount = 0;
				uniquepercell[cellinexact1] = 0.0;
			    }
			}
	
			/* Include this value in the current exact histogram 
			   cell if it belongs there */
                           
			if (cmp_result == 0)
			    d_exactcount += 1;	
			d_inexactcount += 1;
			uniquepercell[cellinexact1] += 1.0;
			d_currval_count = 1;
			
			MEcopy((PTR)&prev_val, 
				histlength, (PTR)&prev1_val);
			MEcopy((PTR)attrp->hist_dt.db_data,
			        histlength, (PTR)&prev_val);
					/* save last 2 distinct values */
		    }
		}
		else
		{
		    /* new value is same as previous one */
		    if (null_count != 0)
			null_count++;	/* Nulls are always last */
		    else
		    {
			if (!toomany)
			    d_exactcount += 1;
			d_inexactcount += 1;
			d_currval_count += 1;

		    }
		}

		row_count++;
	    }
	} /* while */

	exec sql close c1;
    }
# ifdef xDEBUG
    {
	long	timeval;
	struct tm *timeval1;
	time(&timeval);
	timeval1 = localtime(&timeval);
	SIprintf("End of value query for %s, %s \n", selexpr, asctime(timeval1));
    }
# endif /* (xDEBUG) */
	
    currval_count = d_currval_count;
    if (nulls_only == (bool)FALSE)
    {
       if (toomany)
	  inexactcount[cellinexact1] = (OPN_PERCENT)d_inexactcount;
       else exactcount[cellexact] = (OPN_PERCENT)d_exactcount;
    }

    if (toomany && d_currval_count >= 2)
	hcindex = update_hcarray(&prev_hdt, &prev1_val,
				(i4)d_currval_count,
				(i4)(d_inexactcount - d_currval_count),
				uniquepercell[cellinexact1],
				&hccount);
    else hcindex = -1;

    /* Check for last value requiring exact cell in inexact histogram. */
    if (toomany && ( hcindex > -1 || averagepercell > 1.0 && 
			currval_count > averagepercell*OPQ_MINEXCELL))
    {
	exinex_insert(g, relp, attrp, &prev_val, &prev1_val,
	    &rowest, &cellinexact, &cellinexact1, currval_count);
					/* add exact cell to inexact histo. */
	if (inexactcount[cellinexact1] == 0.0)
	    cellinexact1--;		/* drop empty cell on end */
	if (hcindex >= 0)
	    hcarray[hcindex].exactcell = TRUE;
    }

    /* If inexact histogram, add remaining entries from hcarray[] as 
    ** exact cells. */
    if (toomany && !(g->opq_noAFVs) && hccount > 0)
	exinex_insert_rest(g->opq_adfcb, relp, attrp, hccount, 
							&cellinexact1);

    /* normalize null_count */
    if (row_count > 1)
	f4null_count = ((OPO_TUPLES)null_count) / (row_count - 1);
    else f4null_count = 0.0;

    /* Now insert histo data into catalogs OR inform user
    ** that there is no data on which to create stats.
    */
    if (row_count == 1)
    {
	opq_error( (DB_STATUS)E_DB_WARN,
	    (i4)I_OP091C_NOROWS, (i4)4, 
	    (i4)0, (PTR)&relp->relname,
	    (i4)0, (PTR)g->opq_dbname);
	/* optimizedb: table '%0c' in database '%1c' contains no rows. */
	relp->statset = TRUE;	/* don't SET STATISTICS on empty table */
	g->opq_adfcb->adf_collation = save_collation;
	return (E_DB_WARN);
    }

    /* Nulls only histograms (with no cells) are now supported. It used 
    ** to display warning here - now we just let it fall through. */

    if (toomany)
    {
	/* cannot use exact histogram since too many unique values exist. */
	if (cellinexact1 > 0)
	    MEcopy((PTR)&prev_val, histlength,
		(PTR)(inexacthist + cellinexact1 * histlength));
	intervalp = inexacthist;
	cell_count = inexactcount;
	max_cell = cellinexact1;
	exact_histo = FALSE;
    }
    else
    {   /*
	** Exact histogram can be used since 
	** small number of unique values used.
	*/
	if (cellexact >= 0)
	    MEcopy((PTR)&prev_val, histlength,
		(PTR)(exacthist + cellexact * histlength));
	intervalp = exacthist;
	cell_count = exactcount;
	max_cell = cellexact;
	exact_histo = TRUE;
    }

    /* If the attribute is of character type:
    ** 1) Compute character set statistics from the character bitmaps and store
    ** them in the character statistics arrays, which are in the histogram
    ** buffer after the boundary values; these arrays have already been
    ** allocated.
    ** 2) If the -length flag has not been used, determine the necessary
    ** length for character histogram boundary values and truncate them
    ** accordingly, though NOT for composite histograms.
    */
    /* adc_hg_dtln() makes histogram DB_CHA_TYPE or DB_BYTE_TYPE for all
    ** character types.*/
    if (attrp->hist_dt.db_datatype == DB_CHA_TYPE ||
	attrp->hist_dt.db_datatype == DB_BYTE_TYPE)
    {
	add_char_stats(attrp->hist_dt.db_length, max_cell, intervalp, charsets);
	if (bound_length <= 0 && !(relp->index || g->opq_comppkey))
	    truncate_histo(attrp, relp, &max_cell, cell_count, intervalp,
			   exact_histo, g->opq_adfcb);
    }

    /* If this is a composite histogram, change attrid to -1. */
    if (relp->index || g->opq_comppkey) attrp->attrno.db_att_id = -1;

    /* Add up row count for exact cells. */
    for (i = 0, ecrows = 0; i < hccount; i++)
	ecrows += hcarray[i].nrows;

    /* Insert the statistics into the system catalogs */
    insert_histo(g, relp, attrp, (OPO_TUPLES)nunique, intervalp, cell_count,
		 max_cell, (i4)(row_count - 1), (i4)relp->pages,
		 (i4)relp->overflow, f4null_count, (bool)TRUE, exact_histo,
		 (char)0, opq_cflag, (f8)0.0, (char *) DU_DB6_CUR_STDCAT_LEVEL,
		 (bool)FALSE);
    /*
    ** 05-dec-97 (chash01)
    ** For RMS-Gateway databases:  if requested then immediately update the
    ** standard catalogs so that the row count of a base table will be in synch
    ** with the row count used to generate statistics for columns of that base
    ** table.
    */
    if ((pgupdate)
	&&
	(g->opq_dbms.dbms_type & OPQ_RMSGW)
	)
    {
        (VOID) update_row_and_page_stats(g, relp,
	    (i4)(row_count - 1), (i4)relp->pages,
	    (i4)relp->overflow);
    }

    g->opq_adfcb->adf_collation = save_collation;

    return (E_DB_OK);
}

/*{
** Name: process_newq	- create full statistics on the attribute using new 
**			aggregate query
**
** Description:
**      This routine will build an exact histogram (based on a limited number
**      of unique values), and an inexact histogram (based on the average
**      tuple count per cell).  The exact histogram is used if it can be
**      completed successfully. This function uses the new aggregate query
**	that is much faster for highly replicated data.
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_utilid	    name of utility from which this procedure is called
**        opq-owner         name of database owner
**        opq_dbname        name of database
**	  opq_dbcaps	    database capabilities struct
**	    cat_updateable  mask showing which standard cats are updateable
**	  opq_adfcb	    ptr to ADF control block
**      relp                ptr to relation descriptor to be analyzed
**      attrpp              ptr to ptrs to attribute descriptors in relation
**                          to be analyzed. 1st in list is current attr. Ptr
**			    vector is passed in case we're building composite
**	sample		    boolean indicating if this histogram is sampled
**	tempt		    boolean indicating if data is to be read from 
**			    pre-instantiated session temp table
**
** Outputs:
**	Returns:
**	    DB_STATUS	    E_DB_OK	if no error
**			    E_DB_WARN	if no rows
**			    E_DB_ERROR	if sampling attempted when not allowed
**	Exceptions:
**	    none
**
** Side Effects:
**	    update the iihistogram relation with the histogram of the attribute
**
** History:
**	22-may-01 (inkdo01)
**	    Cloned from process_oldq() (used to be simply process()).
**	22-may-01 (inkdo01)
**	    Changes to use new query (aggregating on server side) to speed 
**	    histogram creation. Whole function replicated to permit use of 
**	    both new aggregate and old queries under control of "-zfq" flag.
**	21-Jan-05 (nansa02)
**	    Table name with delimited identifiers should be enclosed within '"'.
**	    (Bug 113786)
**	28-Jan-2005 (schka24)
**	    No, we're already using delimname.
**	15-apr-05 (inkdo01)
**	    Add support for 0-cell histograms (column is all null).
**	17-oct-05 (inkdo01)
**	    Add support for minimum number of exact cells in inexact histo.
**	19-oct-05 (inkdo01)
**	    Also dumped all the comments at the top that were just copied
**	    from the old process() function. They just obscured what's really
**	    been done with this function.
**	9-nov-05 (inkdo01)
**	    Tidy a few things pertaining to Unicode histograms.
**	28-sep-2006 (dougi)
**	    No histogram truncation for composite histos.
**	6-dec-2006 (dougi)
**	    Inhibit SET STATISTICS for empty tables.
**	10-Nov-2008 (kibro01) b121063
**	    Don't use collation on a char field which is actually a
**	    composite index of non-char fields.  Sort on its hex value
**	    as well to ensure the DBMS doesn't collate.
**	5-Jan-2010 (kschendel) b123107
**	    Above accidently broke -zfq for non-sampling, producing
**	    E_US0845 table 'group' does not exist or is not owned by you.
**	    Refer to delim-name if not sampling.  Clarify SQL slightly.
*/
static DB_STATUS
process_newq(
OPQ_GLOBAL	*g,
OPQ_RLIST       *relp,
OPQ_ALIST       **attrpp,
bool		sample,
bool		tempt)
{
    OPQ_ALIST		*attrp = *attrpp;     /* attribute descriptor ptr */
    OPO_TUPLES		averagepercell;	      /* average number of tuples per
                                              ** histogram cell */
    OPO_TUPLES		rowest;		      /* estimate of rows in table/
					      ** sample */
    OPS_DTLENGTH        histlength;           /* length of a histogram
                                              ** element */
    OPQ_MAXELEMENT      prev_val, prev1_val;  /* histogram element of
					      ** cell boundary */
    i4             row_count;	      /* number of rows in the
					      ** relation, which may be
                                              ** different from what was
                                              ** retrieved from the iirelation
					      */
    i4		totalcells;           /* max number of cells for
                                              ** histogram if values are
                                              ** unique */
    i4                  cellexact;	      /* number of cells defined in
                                              ** exact histogram so far */
    bool                toomany;              /* TRUE if too many 
					      ** cells are required
					      ** for the exact histogram so an
                                              ** inexact histogram is used */
    i4                  cellinexact;	      /* number of cells valued in
					      ** inexact histogram */
    i4			cellinexact1;	      /* cells occupied in inexact
					      ** histogram */
    OPQ_I8COUNT         nunique;              /* number of unique values found
                                              ** in the attribute so far */
    OPQ_I8COUNT		null_count;	      /* number of NULLs in column */
    OPO_TUPLES		f4null_count;	      /* ratio of NULLs in column */
    OPO_TUPLES		currval_count;	      /* count of rows with current
					      ** value, so far */
    i4			value_count;	      /* count (from FETCH) of current
					      ** column value */
    IISQLDA		*sqlda = &_sqlda;     /* ptr to SQLDA */
    bool		nulls_only = (bool)TRUE; /* assume all values are NULL,
						 ** until proven otherwise. */
    OPH_CELLS		intervalp;	/* ptr to final histogram cells */
    OPH_CELLS		exinexvalp;	/* ptr to array of values of exact
					** cells in inexact histogram */
    OPN_PERCENT		*cell_count;	/* ptr to final histogram cell counts */
    i4			max_cell;	/* Max cell number in final histogram */
    bool		exact_histo;	/* Is final histogram exact? */
    OPQ_BMCHARSET	charsets;	/* Pointer to array of character set
					** bitmaps */
    char		*selexpr = (char *)&attrp->delimname;  
					      /* ptr to column name
					      ** or composite expression which
					      ** goes in select statement */
    char		*toss;		/* Pointer to ignored use-hex sorter */
    DB_DATA_VALUE	prev_hdt;	    /* used to describe the histo data 
					    ** value previously scanned */
    i4			hccount, hcindex;
    char        sestbl[9];
    bool	use_hex = FALSE;

    if (!tempt || (g->opq_dbms.dbms_type & OPQ_STAR))
    {
        sestbl[0] = '\0';
    }
    else
    {
        STcopy("session.", sestbl);
    }


# ifdef	x3B5
    /*
    ** 3B5 requires f4s to be initialized or will cause
    ** core dump upon access (e.g. in debugger).
    */

    {
	i4	    i3b5;
	for (i3b5 = 0; i3b5 < MAXCELLS + 1; i3b5++)
	{
		exactcount[i3b5] = inexactcount[i3b5] = 0.0;
	}
    }
# endif	/* x3B5 */

    attrp->attr_dt.db_collID = attrp->collID;
    null_count = 0;
    totalcells = uniquecells;

    if (sample)
    {
	i4	i;

	if (relp->sampleok == (bool)FALSE)
	{
	    /*
	    ** Sampling on non-Ingres systems not allowed,
	    ** user has already been warned.
	    */
	    return E_DB_ERROR;
	}
	/* Init. the frequency count array for enhanced distinct 
	** value estimation (but only if were using the "new" technique). */
	if (freqvec) for (i = 0; i < MAXFREQ; i++) freqvec[i] = (f4)0.0;
    }

    /* uniquecells should be greater than regcells - check just in case */
    if (totalcells < regcells)
	totalcells = regcells;

    totalcells++;

    /* If building composite histogram, this function builds select
    ** expression which concatenates all the columns. */
    if (relp->index || g->opq_comppkey)
     if (!comp_selbuild(g, relp, attrpp, &selexpr, &use_hex)) return(FALSE);

    /*
    ** note that ntups may be wrong because reltups entry in relation
    ** relation is wrong, but it is a reasonable guess at this point
    */

    histlength = attrp->hist_dt.db_length;	/* boundary value length */

    /* Allocate exacthist and inexacthist.
    ** If character type, include space for character set
    ** statistics, and allocate character set bitmaps.
    */
    {
	DB_DT_ID	type;	/* Histogram data type */
	u_i4	extra;	/* Extra space required, if any, for character
				** set statistics. */

	type = attrp->hist_dt.db_datatype;
	if (type < (DB_DT_ID)0)
	    type = -type;

	charsets = NULL;
	if (type == DB_CHA_TYPE ||
	    type == DB_BYTE_TYPE)
	{
	    /* Extra space needed for histogram buffers */
	    extra = histlength * (sizeof(i4) + sizeof(f4));
	    /* Allocate character set bitmaps */
	    (VOID) getspace((char *)g->opq_utilid, (PTR *)&charsets, 
		(u_i4)(histlength * OPQ_BMCHARSETBYTES)); 
	}
	else
	    extra = 0;

	(VOID) getspace((char *)g->opq_utilid, (PTR *)&exacthist, 
			(u_i4)((histlength + sizeof(OPN_PERCENT)) * 
			totalcells + extra)); 
	(VOID) getspace((char *)g->opq_utilid, (PTR *)&inexacthist,
			(u_i4)((histlength + sizeof(OPN_PERCENT)) *
			(regcells * 3 + exinexcells + 1) + extra)); 
				/* added per-cell unique count array, and
				** space for exact cells in inexact histos */
	exinexvalp = NULL;	/* so getspace() gets fresh buffer */
	(VOID) getspace((char *)g->opq_utilid, (PTR *)&exinexvalp,
			(u_i4)(histlength * exinexcells * 2));
				/* And space for value array for exact cells
				** in ineact histogram */
    }

    /* Init. array of counts for exact cells in inexact histograms. */
    {
	i4	i;

	for (i = 0; i < exinexcells; i++)
	{
	    hcarray[i].nrows = 0;
	    hcarray[i].pnrows = 0;
	    hcarray[i].puperc = 0;
	    hcarray[i].valp = exinexvalp;
	    hcarray[i].pvalp = &exinexvalp[histlength];
	    hcarray[i].exactcell = FALSE;
	    exinexvalp = &exinexvalp[2 * histlength];
	}
    }

    /* Estimate number of tuples per cell */
    if (sample == (bool)TRUE)
    {
	averagepercell = (OPO_TUPLES)relp->nsample / (OPO_TUPLES)regcells;
	rowest = (OPO_TUPLES)relp->nsample;
    }
    else
    {
	if (relp->ntups > (OPO_TUPLES)0)
	{
	    averagepercell = (OPO_TUPLES)relp->ntups / (OPO_TUPLES)regcells;
	    rowest = (OPO_TUPLES)relp->ntups;
	}
	else
	{
	    /* When row info missing */
	    averagepercell = 1.0;
	    rowest = 0.0;
	}
    }

    if (averagepercell < 1.0)
	averagepercell = 1.0;

    /*
    **	    try to make a hist where every unique value has its own cell
    **	       and if there is not enough room then go to next retrieve to
    **	       do the normal histogram stuff
    **
    **	    As an example, suppose the values of an attribute are
    **			    1,2,2,3,4,6,20,21
    **		    then the histogram will be in count-value pairs:
    **			    (0,0) (1,1) (2,2) (1,3) (1,4) (0,5)
    **			    (1,6) (0,19), (1,20) (1,21)
    */

    {
	exec sql begin declare section;
	    char	*stmt;		    /* temp buffer to store text
					    ** of target list */
	exec sql end declare section;

	DB_DATA_VALUE	prev_dt;	    /* used to describe the data value
					    ** previously scanned */
    
	/* Initialize prev_dt */
	STRUCT_ASSIGN_MACRO( attrp->attr_dt, prev_dt);
	prev_dt.db_data = (PTR)&prev_full_val;

	/* Initialize prev_hdt */
	STRUCT_ASSIGN_MACRO( attrp->hist_dt, prev_hdt);
	prev_hdt.db_data = (PTR)&prev_val;

	if (relp->index || g->opq_comppkey)
	    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&stmt,
	    (i4)1000, &opq_sbuf);
	else opq_bufalloc((PTR)g->opq_utilid, (PTR *)&stmt,
	    (i4)(sizeof("select %s, count(*) from %s group by 1 order by 1") +
	    2 * DB_ATT_MAXNAME+4), &opq_sbuf);

	/* Build "select" statement to produce distinct column values and counts.
	** The select is simply "select column, count(*) from table group by 1 
	** order by 1". For composite histograms, "column" is replaced by the
	** column value concatenation expression. For sampled histograms, "table"
	** is replaced by "session.table". This aggregating query takes advantage
	** of the 2.6 non-sorting aggregation, saving backend overhead as well as
	** optimizedb overhead in processing the individual rows. */
	{
	    char    *name;

	    name = (char *)&relp->delimname;
	    if (sample)
		name = (char *)&relp->samplename;
	    if (use_hex)
	    {
		STprintf(stmt, 
		"select %s, count(*), hex(%s) from %s%s group by 1,3 order by 3",
		    selexpr,
		    selexpr,
		    sestbl,
		    name);
		sqlda->sqln = 3;
	    }
	    else
	    {
		STprintf(stmt, 
		    "select %s, count(*) from %s%s group by 1 order by 1",
		    selexpr,
		    sestbl,
		    name);
		sqlda->sqln = 2;
	    }
	}
# ifdef xDEBUG
	{
	    long	timeval;
	    struct tm *timeval1;
	    time(&timeval);
	    timeval1 = localtime(&timeval);
	    SIprintf("Start of value query for %s, %s \n", selexpr, asctime(timeval1));
	}
# endif /* (xDEBUG) */
	exec sql prepare s from :stmt;

	exec sql describe s into :sqlda;

	/* allocate buffer, describe location for the column. */
	{
	    sqlda->sqlvar[0].sqltype = DB_DBV_TYPE;
	    sqlda->sqlvar[0].sqllen  = sizeof(DB_DATA_VALUE);
	    sqlda->sqlvar[0].sqldata = (char *)&attrp->attr_dt;

	    /*
	    ** Allocate one for query result and one for converted histogram
	    ** value (Unicode ones expand and get messed up if converted in
	    ** place).
	    */
	    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&attrp->attr_dt.db_data,
		     attrp->attr_dt.db_length, &opq_rbuf);
	    opq_bufalloc((PTR)g->opq_utilid, (PTR *)&attrp->hist_dt.db_data,
		     attrp->hist_dt.db_length, &opq_rbuf1);

	    /* type is int from describe */
	    sqlda->sqlvar[1].sqllen  = sizeof(i4);
	    sqlda->sqlvar[1].sqldata = (char *)&value_count;
	    if (use_hex)
	    {
		/* Use-hex query returns hex version of value to avoid
		** sorting with improper collation.  Set up a receiver for
		** that column too, just to make sure libq doesn't decide
		** to take exception, but we'll just toss it.
		*/
		opq_bufalloc((PTR)g->opq_utilid, (PTR *) &toss,
			sqlda->sqlvar[2].sqllen, &opq_rbuf2);
		sqlda->sqlvar[2].sqldata = (char *)toss;
	    }
	}

	/* retrieve data for the constructed query. */
	exec sql open c1 for readonly;

	toomany	= FALSE;
	cellexact = -1;
	cellinexact = 0;
	cellinexact1 = 0;
	nunique	= 0.0;
	currval_count = 0.0;
	maxfreq = -1;
	row_count = 1;
	hccount = 0;

	while (sqlca.sqlcode == GE_OK)
	{
	    exec sql fetch c1 using descriptor :sqlda;

	    if (sqlca.sqlcode != GE_OK)
		break;
	    else
	    {
		/* In 2.6 optimizedb with new aggregate query, every row 
		** represents a new column value. */
		{
		    nunique += 1.0;
		    if (freqvec && currval_count > 0.0)
		    {
			/* Enhanced distinct value estimation requires
			** vector of counts, the "ith" entry of which is
			** the number of values in the sample with "i"
			** duplicates. */
			if (value_count >= MAXFREQ)
			    freqvec[MAXFREQ-1] += 1.0;
			else freqvec[value_count-1] += 1.0;

			if (value_count > maxfreq) 
			    maxfreq = (value_count < MAXFREQ) ? 
						value_count : MAXFREQ;
		    }
		    
		    /* create the histogram element in place */
		    if (hist_convert((char *)attrp->attr_dt.db_data,
				     attrp, g->opq_adfcb))
			null_count = value_count; 
						/* NULL count if found */
		    else
		    {
			i4	cmp_result = 0;
			
			/* indicate that a non-NULL value is present */
			nulls_only = (bool)FALSE;
			
			if (cellexact > -1)
			    /* See if the new histogram value is different from 
			    ** the previous one (we have to check, because 
			    ** histogram values can be the same even if the 
			    ** data values are different) 
			    */
			    cmp_result = compare_values(g->opq_adfcb,
						&attrp->hist_dt, &prev_hdt,
						&attrp->attname);
			
			/* If column is of character type, record characters
			** present in new unique value in the character set bit
			** maps, for character set statistics */
			if ((attrp->hist_dt.db_datatype == DB_CHA_TYPE ||
			     attrp->hist_dt.db_datatype == DB_BYTE_TYPE) &&
			    (cellexact == -1 || cmp_result != 0))
			{
			    record_chars(attrp, charsets);
			}

			if (!toomany) 	/* Still room in exact histogram? */
			{
			    if (cellexact == -1 || cmp_result != 0)
			    {
				/* We either have no cells yet or the current
				** histogram value differs from the previous 
				** one. 
				*/
				cellexact = add_exact_cell(attrp,relp, &prev_hdt,
						          cellexact, totalcells,
							  g->opq_adfcb);
				if (cellexact >= totalcells)
				    toomany = TRUE;
				else 
				    exactcount[cellexact] = (OPO_TUPLES)value_count;
			    }
			    else
				exactcount[cellexact] += (OPO_TUPLES)value_count;
			}
			/*
			** build the inexact histo here (adjacent values 
			** may be put in the same histogram cell).
			*/
			if (cellinexact == 0)
			{
			    /* Decrement current value and store it in first
			    ** inexact histogram cell.
			    */
			    decrement_value(g->opq_adfcb, attrp, relp,
					    inexacthist);
			    
			    cellinexact++;
			    cellinexact1++;
			    inexactcount[cellinexact1] = 0.0;	/* init cells */
			    uniquepercell[cellinexact1] = 0.0;
			}
			else
			{
			    if (cmp_result != 0 && currval_count >= 2.0)
				hcindex = update_hcarray(&prev_hdt, &prev1_val,
					currval_count, 
					inexactcount[cellinexact1] - 
								currval_count,
					uniquepercell[cellinexact1],
					&hccount);
			    if (cmp_result != 0 && averagepercell > 1.0 && 
				currval_count > averagepercell*OPQ_MINEXCELL)
			    {
			        exinex_insert(g, relp, attrp, &prev_val,
				    &prev1_val, &rowest, &cellinexact,
				    &cellinexact1, currval_count);
					/* add exact cell to inexact histo. */
				if (hcindex >= 0)
				    hcarray[hcindex].exactcell = TRUE;
			    }
			    else if ((cmp_result != 0) &&
			      (cellinexact < regcells) &&
			      ((rowest <= 1.0 && 
			      (cellinexact + 0.001 < row_count/averagepercell))
				 || (rowest/(regcells-cellinexact+1) <= 
						inexactcount[cellinexact1])))
				/* this cell is full - note the "rowest" formula 
				** dynamically adjusts n-tiles for more balanced
				** cells with skewed data distributions */
			    {
				/* create new inexact cell */
				MEcopy((PTR)&prev_val, histlength, 
				  (PTR)(inexacthist + cellinexact1 * histlength));
			    
				rowest -= inexactcount[cellinexact1];
						/* adjust remaining row count */
				cellinexact++;
				cellinexact1++;
				inexactcount[cellinexact1] = 0.0;
				uniquepercell[cellinexact1] = 0.0;
			    }
			}
			
			inexactcount[cellinexact1] += (OPO_TUPLES)value_count;
			uniquepercell[cellinexact1] += 1.0;
			currval_count = (OPO_TUPLES)value_count;
			
			MEcopy((PTR)&prev_val, 
				histlength, (PTR)&prev1_val);
			MEcopy((PTR)attrp->hist_dt.db_data,
			        histlength, (PTR)&prev_val);
					/* save last 2 distinct values */
		    }
		}

		row_count += value_count;	/* overall row count */
	    }
	} /* while */

	exec sql close c1;
    }
# ifdef xDEBUG
    {
	long	timeval;
	struct tm *timeval1;
	time(&timeval);
	timeval1 = localtime(&timeval);
	SIprintf("End of value query for %s, %s \n", selexpr, asctime(timeval1));
    }
# endif  /* (xDEBUG) */

    if (toomany && currval_count >= 2.0)
	hcindex = update_hcarray(&prev_hdt, &prev1_val,
				currval_count,
				inexactcount[cellinexact1] - currval_count,
				uniquepercell[cellinexact1],
				&hccount);
    else hcindex = -1;
	
    /* Check for last value requiring exact cell in inexact histogram. */
    if (toomany && (hcindex > -1 || averagepercell > 1.0 && 
			currval_count > averagepercell*OPQ_MINEXCELL))
    {
	exinex_insert(g, relp, attrp, &prev_val, &prev1_val,
	    &rowest, &cellinexact, &cellinexact1, currval_count);
					/* add exact cell to inexact histo. */
	if (inexactcount[cellinexact1] == 0.0)
	    cellinexact1--;		/* drop empty cell on end */
	if (hcindex >= 0)
	    hcarray[hcindex].exactcell = TRUE;
    }

    /* If inexact histogram, add remaining entries from hcarray[] as 
    ** exact cells. */
    if (toomany && !(g->opq_noAFVs) && hccount > 0)
	exinex_insert_rest(g->opq_adfcb, relp, attrp, hccount, 
							&cellinexact1);

    /* normalize null_count */
    if (row_count > 1)
	f4null_count = ((OPO_TUPLES)null_count) / (row_count - 1);
    else f4null_count = 0.0;

    /* Now insert histo data into catalogs OR inform user
    ** that there is no data on which to create stats.
    */
    if (row_count == 1)
    {
	opq_error( (DB_STATUS)E_DB_WARN,
	    (i4)I_OP091C_NOROWS, (i4)4, 
	    (i4)0, (PTR)&relp->relname,
	    (i4)0, (PTR)g->opq_dbname);
	/* optimizedb: table '%0c' in database '%1c' contains no rows. */
	relp->statset = TRUE;	/* don't SET STATISTICS on empty table */
	return (E_DB_WARN);
    }

    /* Nulls only histograms (with no cells) are now supported. It used 
    ** to display warning here - now we just let it fall through. */

    if (toomany)
    {
	/* cannot use exact histogram since too many unique values exist. */
	if (cellinexact1 > 0)
	    MEcopy((PTR)&prev_val, histlength,
		(PTR)(inexacthist + cellinexact1 * histlength));
	intervalp = inexacthist;
	cell_count = inexactcount;
	max_cell = cellinexact1;
	exact_histo = FALSE;
    }
    else
    {   /*
	** Exact histogram can be used since 
	** small number of unique values used.
	*/
	if (cellexact >= 0)
	    MEcopy((PTR)&prev_val, histlength,
		(PTR)(exacthist + cellexact * histlength));
	intervalp = exacthist;
	cell_count = exactcount;
	max_cell = cellexact;
	exact_histo = TRUE;
    }

    /* If the attribute is of character type:
    ** 1) Compute character set statistics from the character bitmaps and store
    ** them in the character statistics arrays, which are in the histogram
    ** buffer after the boundary values; these arrays have already been
    ** allocated.
    ** 2) If the -length flag has not been used, determine the necessary
    ** length for character histogram boundary values and truncate them
    ** accordingly, though NOT for composite histograms.
    */
    /* adc_hg_dtln() makes histogram DB_CHA_TYPE for all character types.*/
    if (attrp->hist_dt.db_datatype == DB_CHA_TYPE ||
	attrp->hist_dt.db_datatype == DB_BYTE_TYPE)
    {
	add_char_stats(attrp->hist_dt.db_length, max_cell, intervalp, charsets);
	if (bound_length <= 0 && !(relp->index || g->opq_comppkey))
	    truncate_histo(attrp, relp, &max_cell, cell_count, intervalp,
			   exact_histo, g->opq_adfcb);
    }

    /* If this is a composite histogram, change attrid to -1. */
    if (relp->index || g->opq_comppkey) attrp->attrno.db_att_id = -1;

    /* Insert the statistics into the system catalogs */
    insert_histo(g, relp, attrp, nunique, intervalp, cell_count,
		 max_cell, (i4)(row_count - 1), (i4)relp->pages,
		 (i4)relp->overflow, f4null_count, (bool)TRUE, exact_histo,
		 (char)0, opq_cflag, (f8)0.0, (char *) DU_DB6_CUR_STDCAT_LEVEL,
		 (bool)FALSE);
    /*
    ** 05-dec-97 (chash01)
    ** For RMS-Gateway databases:  if requested then immediately update the
    ** standard catalogs so that the row count of a base table will be in synch
    ** with the row count used to generate statistics for columns of that base
    ** table.
    */
    if ((pgupdate)
	&&
	(g->opq_dbms.dbms_type & OPQ_RMSGW)
	)
    {
        (VOID) update_row_and_page_stats(g, relp,
	    (i4)(row_count - 1), (i4)relp->pages,
	    (i4)relp->overflow);
    }

    return (E_DB_OK);
}

/*{
** Name: read_process - Process input statistics from a file.
**
** Description:
**      Procedure reads specified file, analyzes input (expected
**	to be of same format as produced by STATDUMP) and stores
**	info in statistics catalogs. If requested row and page
**	info is also updated.
**
** Inputs:
**	g		    ptr to global data struct
**	  opq_utilid	    name of utility from which this proc is called
**	  opq_owner	    ptr to owner name
**	  opq_dbname	    ptr to database name
**	  opq_adfcb	    ptr to ADF control block
**	  opq_dbcaps	    database capabilities struct
**	    cat_updateable  mask showing which standard catalogs are
**			    updateable.
**	inf		    input file descriptor
**	argv		    ptr to array of ptrs to command line arguments
**
** Outputs:
**	none
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    updates IISTATS, IIHISTOGRAMS, IITABLES
**	    catalogs or their equivalent.
**
** History:
**      20-jan-89 (stec)
**          Written.
**	10-jul-89 (stec)
**	    Added user length, scale to column info scanned.
**	31-jul-89 (stec)
**	    When updating number of rows for subject table in iitables
**	    update the same for indexes on the table.
**	    Also fixed an AV occurring when opq_scanf for TEXT3 was processed.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	    Improved error handling in the case number of declared cells
**	    differs from the number of actual entries read in.
**	06-jul-90 (stec)
**	    In case of STAR we will commit before and after issuing update 
**	    iitables query.
**	    Update queries for iistats and iihistograms get routed over a
**	    different LDB server association than update queries for iitables.
**	    This would, at this time, cause a problem in a VAXcluster 
**	    environment, in which 2PC feature in not supported.
**	10-jan-91 (stec)
**	    Changed length of ERreport() buffer to ER_MAX_LEN.
**	    Substituted new defines from <clfloat.h> in place of the obsolete
**	    ones from <mh.h>.
**	22-jan-91 (stec)
**	    Change code to recognize complete flag value read from file,
**	    check it and pass it to insert_histo().
**	28-feb-91 (stec)
**	    Changed code calculating epsilon for the cumulative cell count.
**	16-apr-92 (rganski)
**	    If the lower and upper values of a histogram cell are the same,
**	    don't try to correct if the datatype is character and the
**	    value is the null string. Otherwise you get \377, which is
**	    greater, since characters are unsigned.
**	06-jul-92 (rganski)
**	    Added (char *) typecast to PTR variable, since dereference of PTR
**	    is not allowed.
**	09-nov-92 (rganski)
**	    Padded argument list to opq_scanf() so that calls contain
**	    same number of parameters as the declaration.
**	18-jan-93 (rganski)
**	    Added dynamic allocation of exacthist. Since boundary values can
**	    now be up to 1950 bytes long, it is inefficient to statically
**	    allocate maximum-size histograms.
**	    Added space for character set statistics, if column is of character
**	    type.
**	    Changed references to exacthist from array dereferencing to pointer
**	    arithmetic.
**	    Changed scanning of lines to accomodate new version and value
**	    length fields (see Character Histogram Enhancements spec).
**	    Added bound_length parameter to calls to setattinfo().
**	    Replaced local variables line[] and buf[] with global variable
**	    opq_line and opq_value_buf, since maximum line length and maximum
**	    value length have increased a lot. 
**	26-apr-93 (rganski)
**	    Character Histogram Enhancements project:
**	    Added reading of character set statistics if they are present.
**	    Changed reading of cells to for loop, to allow separate input for
**	    character set statistics.
*  	10-apr-95 (nick) (cross-int by angusm)
**      Removed the pgupdate parameter - this is now a global. #44193
**      Don't update relprim if '-zp' is specified as this trashes
**      ISAM and HASH relations. Just update relpages and reltups as these
**      are only approximate values anyway. #67058
**	9-jan-96 (inkdo01)
**	    Added support of per-cell repetition factors. Now reads them from 
**	    input file. If not present (old statdump format), propagates
**	    table-wide repetition factor value.
**	19-jul-00 (inkdo01)
**	    Eliminate OP0990 caused by statdump row count significantly 
**	    changed from when orig optimizedb was run.
**      28-aug-2000 (chash01) 07-jun-99 (chash01)
**	    use update_row_and_page_stats() at the end of this routine.
**	 3-jun-04 (hayke02)
**	    Increase the number of cells that we can read in from the stats
**	    file before reporting E_OP0949 from MAXCELLS to (MAXCELLS+MAXCELLS).
**	    This reflects the max possible inexact histogram cells when exact
**	    histogram cells are inserted for skewed data (see bug 92234). This
**	    change fixes problem INGSRV 2834, bug 112369.
**	15-apr-05 (inkdo01)
**	    Add support for 0-cell histograms (column is all null).
**	13-mar-08 (wanfr01) 
**	    Bug 119927
**	    Initialize icells to 0 per loop since no histogram cells may exist
**	    for this column, and do not fail with E_OP0973 for 0 cell histogram
**	10-Nov-2008 (kibro01) b121063
**	    Don't use collation on a char field which is actually a
**	    composite index of non-char fields.
**	17-Dec-2008 (kiria01) SIR120473
**	    Initialise the ADF_FN_BLK.adf_pat_flags.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
**      09-Jul-2010 (coomi01) b124051
**          Allow hex strings to be absorbed from stats file.
*/
static
DB_STATUS
read_process(
OPQ_GLOBAL  *g,
FILE	    *inf,
char	    **argv)
{
    DB_STATUS		stat;
    bool		found;
    i4			acnt;
    char		dname[DB_GW1_MAXNAME+1],
			iversion[DB_GW1_MAXNAME+1];
    DB_TAB_STR		tname;
    DB_ATT_STR		cname;
    DB_TYPE_STR		typename;
    DB_TYPE_STR		nullable;
    char		*version;
    char 		iuflag;
    i4			userlen, scale;
    bool		wrong_histo;
    bool		empty_histo;
    bool		complete_flag;
    bool		rfpercell;
    bool                hexFlag;
    OPS_DTLENGTH        histlength;	/* Length of a histogram element */

    /* opq_scanf requires f8 for floats, and i4 for regular ints */
    f8		inunique, incount;
    f8		irptfactor, irepf, icount, tcount;
    i4		irows, icflag,
		idomain, ihcells,
		icellno, ipages,
		ioverflow;
    PTR		save_collation;

    register OPQ_RLIST **relp;
    register OPQ_ALIST **attrp;

    save_collation = g->opq_adfcb->adf_collation;

    for (stat = 0; stat != ENDFILE;)
    {
	/* Read next set of statistics in the file */
	i4    	cells;
	i4	stlen;
	char	*pos;
	char	*namestart;
	i4	stlen2;

	icellno = 0;
	g->opq_adfcb->adf_collation = save_collation;
	
	stat = SIgetrec(opq_line, sizeof(opq_line), inf);
	if (stat != OK)
	{
	    if (stat != ENDFILE)
	    {
		char	buf[ER_MAX_LEN];

		if ((ERreport(stat, buf)) != OK)
		{
		    buf[0] = '\0';
		}

		opq_error((DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0940_READ_ERR, (i4)4,
		    (i4)sizeof(stat), (PTR)&stat,
		    (i4)0, (PTR)buf);
		(*opq_exit)();
	    }
	    return((DB_STATUS)E_DB_OK);
	}
	
	acnt = opq_scanf(opq_line, TEXT1I,
			 g->opq_adfcb->adf_decimal.db_decimal, (PTR)dname,
			 (PTR)iversion, (PTR)0, (PTR)0, (PTR)0);

	if (acnt == 2)
	    version = iversion;		/* Version field is present */
	else
	    if (acnt == 1)
		/* No version field present */
		version = NULL;
	    else
		/* Either EOF or junk line: continue until a valid first line
		** is found or EOF is recognized */
		continue;

	stat = SIgetrec(opq_line, sizeof(opq_line), inf);
	if (stat != OK)
	{
	    if (stat != ENDFILE)
	    {
		char	buf[ER_MAX_LEN];

		if ((ERreport(stat, buf)) != OK)
		{
		    buf[0] = '\0';
		}

		opq_error((DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0940_READ_ERR, (i4)4,
		    (i4)sizeof(stat), (PTR)&stat,
		    (i4)0, (PTR)buf);
		(*opq_exit)();
	    }
	    return((DB_STATUS)E_DB_OK);
	}
	
	stlen = STlength(TEXT2A);
	if (STncmp(opq_line, TEXT2A, stlen ))
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
		(i4)0, (PTR)TEXT2);
	    (*opq_exit)();
	}
	/* Read table name */
        {
	    /* Look for string "rows:". Start by looking for char ':' */
	    stlen2 = STlength(TEXT2C);	    /* "rows" */
	    pos = namestart = opq_line + stlen;	/* Start search here */
	    for( ; ; )
	    {
		pos = STchr(pos+1, *((char *)(TEXT2B + stlen2)) );/* Look for ';' */
		if (pos == NULL)
		{
		    opq_error((DB_STATUS)E_DB_ERROR,
			      (i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
			      (i4)0, (PTR)TEXT2);
		    (*opq_exit)();
		}
		if (!STncmp(pos-stlen2, TEXT2B, 5 ))
		    /* Found "rows:" */
		    break;
	    }
	    
	    /* Copy table name */
	    STncpy( tname, namestart, pos-stlen2-namestart);
	    tname[ pos-stlen2-namestart ] = '\0';
	    STtrmwhite(tname);
	}
	    
	acnt = opq_scanf(pos-stlen2, TEXT2B,
			 g->opq_adfcb->adf_decimal.db_decimal, (PTR)&irows,
			 (PTR)&ipages, (PTR)&ioverflow, (PTR)0, (PTR)0);

	if (acnt != 3)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
		(i4)0, (PTR)TEXT2);
	    (*opq_exit)();
	}

	stat = SIgetrec(opq_line, sizeof(opq_line), inf);
	if (stat != OK)
	{
	    if (stat != ENDFILE)
	    {
		char	buf[ER_MAX_LEN];

		if ((ERreport(stat, buf)) != OK)
		{
		    buf[0] = '\0';
		}

		opq_error((DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0940_READ_ERR, (i4)4,
		    (i4)sizeof(stat), (PTR)&stat,
		    (i4)0, (PTR)buf);
		(*opq_exit)();
	    }
	    return((DB_STATUS)E_DB_OK);
	}

	stlen = STlength(TEXT3A);
	if (STncmp(opq_line, TEXT3A, stlen ))
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
		(i4)0, (PTR)TEXT2);
	    (*opq_exit)();
	}
	/* Read column name */
        {
	    /* Look for string "of type". Search for char 'y' to start */
	    stlen2 = STlength(TEXT3C);		/* "of t" */
	    pos = namestart = opq_line + stlen;	/* Start search here */
	    for( ; ; )
	    {
		pos = STchr(pos+1, *((char *)(TEXT3B + stlen2)));/* Look for 'y' */
		if (pos == NULL)
		{
		    opq_error((DB_STATUS)E_DB_ERROR,
			      (i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
			      (i4)0, (PTR)TEXT2);
		    (*opq_exit)();
		}
		if (!STncmp(pos-stlen2, TEXT3B, 7 ))
		    /* Found "of type" */
		    break;
	    }
	    
	    /* Copy column name */
	    STncpy( cname, namestart, pos-stlen2-namestart);
	    cname[ pos-stlen2-namestart ] = '\0';
	    STtrmwhite(cname);
	}
	    
	acnt = opq_scanf(pos-stlen2, TEXT3B,
			 g->opq_adfcb->adf_decimal.db_decimal, 
			 (PTR)typename, (PTR)&userlen, (PTR)&scale,
			 (PTR)nullable, (PTR)0);
	if (acnt != 4)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
		(i4)0, (PTR)TEXT2);
	    (*opq_exit)();
	}

	for (relp = rellist, found = FALSE; *relp; relp++)
	{
	    if (!STcompare(tname, (char *)&(*relp)->relname))
	    {
		found = TRUE;
		break;
	    }
	}

	if (found != TRUE)
	{   /* print warning if relation cannot be found in the
	    ** existing list of relations */
	    opq_error( (DB_STATUS)E_DB_WARN,
		    (i4)I_OP092D_REL_NOT_FOUND, (i4)4, 
		    (i4)0, (PTR)g->opq_dbname,
		    (i4)0, (PTR)tname);
		/* optimizedb: database '%0c' table '%1c' cannot be found */
	    continue;
	}

	/* Check for composite histogram. */
	if (!STcompare(cname, "IICOMPOSITE")) (*relp)->comphist = TRUE;
	else (*relp)->comphist = FALSE;

	if (att_list(g, attlist, *relp, argv) == (bool)FALSE)
	{
	    return((DB_STATUS)E_DB_ERROR);
	}

	if ((*relp)->comphist)
	{
	    for (attrp = attlist; *attrp; attrp++)
	    {
		if (!collatable_type((*attrp)->attr_dt.db_datatype))
		{
		    g->opq_adfcb->adf_collation = NULL;
		    break;
		}
	    }
	}

	/* Loop exits with attrp pointing to matching column. If 
	** composite histogram, it points to first column. */
	for (attrp = attlist, found = FALSE; 
			!((*relp)->comphist) && *attrp; attrp++)
	{
	    if (!STcompare(cname, (char *)&(*attrp)->attname))
	    {
		found = TRUE;
		break;
	    }
	}

	if ((*relp)->comphist)
	{
	    (*attrp)->hist_dt.db_datatype = DB_BYTE_TYPE;
	    (*attrp)->attr_dt.db_datatype = DB_BYTE_TYPE;
	    STcopy(cname, (char *) &(*attrp)->attname);
	    (*attrp)->attrno.db_att_id = -1;
	    (*attrp)->nullable = 'N';
	    (*attrp)->scale = 0;
	    found = TRUE;
	}

	if (found != TRUE)
	    continue;

	stat = SIgetrec(opq_line, sizeof(opq_line), inf);
	if (stat != OK)
	{
	    if (stat != ENDFILE)
	    {
		char	buf[ER_MAX_LEN];

		if ((ERreport(stat, buf)) != OK)
		{
		    buf[0] = '\0';
		}

		opq_error((DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0940_READ_ERR, (i4)4,
		    (i4)sizeof(stat), (PTR)&stat,
		    (i4)0, (PTR)buf);
		(*opq_exit)();
	    }
	    return((DB_STATUS)E_DB_OK);
	}

	/* There is possibility that date will be expressed as 
	** dd-mmm-yyyy or dd-mmm-yyyy hh:mm:ss or dd-mmm-yyyy hh:mm:ss GMT,
	** so we may have to scan thrice (we start with the most likely one).
	*/
	acnt = opq_scanf(opq_line, TEXT4I3,
			 g->opq_adfcb->adf_decimal.db_decimal, (PTR)&inunique,
			 (PTR)0, (PTR)0, (PTR)0, (PTR)0);

	if (acnt != 1)
	{
	    acnt = opq_scanf(opq_line, TEXT4I2,
			     g->opq_adfcb->adf_decimal.db_decimal,
			     (PTR)&inunique, (PTR)0, (PTR)0, (PTR)0, (PTR)0); 

	    if (acnt != 1)
	    {
		acnt = opq_scanf(opq_line, TEXT4I1,
				 g->opq_adfcb->adf_decimal.db_decimal,
				 (PTR)&inunique, (PTR)0, (PTR)0, (PTR)0,
				 (PTR)0);

		if (acnt != 1)
		{
		    opq_error((DB_STATUS)E_DB_ERROR,
			(i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
			(i4)0, (PTR)TEXT4I1);
		    (*opq_exit)();
		}
	    }
	}

	stat = SIgetrec(opq_line, sizeof(opq_line), inf);
	if (stat != OK)
	{
	    if (stat != ENDFILE)
	    {
		char	buf[ER_MAX_LEN];

		if ((ERreport(stat, buf)) != OK)
		{
		    buf[0] = '\0';
		}

		opq_error((DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0940_READ_ERR, (i4)4,
		    (i4)sizeof(stat), (PTR)&stat,
		    (i4)0, (PTR)buf);
		(*opq_exit)();
	    }
	    return((DB_STATUS)E_DB_OK);
	}

	acnt = opq_scanf(opq_line, TEXT5, g->opq_adfcb->adf_decimal.db_decimal,
			 (PTR)&irptfactor, (PTR)&iuflag, (PTR)&icflag, (PTR)0,
			 (PTR)0);

	if (acnt != 3)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
		(i4)0, (PTR)TEXT5);
	    (*opq_exit)();
	}

	/* Check if value of the unique flag is acceptable */
	CMtoupper(&iuflag, &iuflag);
	if (iuflag != 'Y' && iuflag != 'N')
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP095E_BAD_UNIQUE, (i4)6,
		(i4)1, (PTR)&iuflag,
		(i4)0, (PTR)cname,
		(i4)0, (PTR)tname);
	    (*opq_exit)();
	}

	/*
	** Check if the value of the complete flag is 
	** acceptable and set local boolean variable.
	*/
	if (icflag == 0)
	{
	    complete_flag = FALSE;
	}
	else if (icflag == 1)
	{
	    complete_flag = TRUE;
	}
	else
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0928_BAD_COMPLETE, (i4)6,
		(i4)sizeof(icflag), (PTR)&icflag,
		(i4)0, (PTR)cname,
		(i4)0, (PTR)tname);
	    (*opq_exit)();
	}

	stat = SIgetrec(opq_line, sizeof(opq_line), inf);
	if (stat != OK)
	{
	    if (stat != ENDFILE)
	    {
		char	buf[ER_MAX_LEN];

		if ((ERreport(stat, buf)) != OK)
		{
		    buf[0] = '\0';
		}

		opq_error((DB_STATUS)E_DB_ERROR,
		    (i4)E_OP0940_READ_ERR, (i4)4,
		    (i4)sizeof(stat), (PTR)&stat,
		    (i4)0, (PTR)buf);
		(*opq_exit)();
	    }
	    return((DB_STATUS)E_DB_OK);
	}

	acnt = opq_scanf(opq_line, TEXT6I,
			 g->opq_adfcb->adf_decimal.db_decimal, (PTR)&idomain,
			 (PTR)&ihcells, (PTR)&incount, (PTR)&histlength,
			 (PTR)0);

	if (version != NULL)
	{
	    /* There is a version field, so there should be a value length
	    ** field */
	    if (acnt != 4)
	    {
		opq_error((DB_STATUS)E_DB_ERROR,
			  (i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
			  (i4)0, (PTR)TEXT6I);
		(*opq_exit)();
	    }
	}
	else
	{
	    /* There is no version field, so there should be no value length
	    ** field */
	    if (acnt != 3)
	    {
		opq_error((DB_STATUS)E_DB_ERROR,
			  (i4)E_OP0941_TEXT_NOTFOUND, (i4)2,
			  (i4)0, (PTR)TEXT6I);
		(*opq_exit)();
	    }
	    else
	    {
		/* For character type, maximum value length is 8, since
		** this is an old-style histogram */
		if (((*attrp)->hist_dt.db_datatype == DB_CHA_TYPE ||
		     (*attrp)->hist_dt.db_datatype == DB_BYTE_TYPE) &&
		    (*attrp)->hist_dt.db_length > OPH_NH_LENGTH)
		    (*attrp)->hist_dt.db_length = OPH_NH_LENGTH;
		histlength = (*attrp)->hist_dt.db_length;
	    }
	}

	if ((*relp)->comphist)
	{
	    (*attrp)->hist_dt.db_length = histlength;
	    (*attrp)->attr_dt.db_length = histlength;
	}

# ifdef xDEBUG
	if (opq_xdebug)
	{
	    SIprintf("\n\ndbname='%s', tablename='%s'",
		    dname, tname);
	    SIprintf("\npages=%d, overflow_pages=%d",
		    ipages, ioverflow);
	    SIprintf("\ncolumnname='%s', typename='%s', %s",
		    cname, typename, nullable);
	    SIprintf("\nrows=%d, number_of_unique=%f",
		    irows, inunique, g->opq_adfcb->adf_decimal.db_decimal);
	    SIprintf("\nrpt_factor=%f, unique_flag='%c', complete_flag=%d",
		    irptfactor, g->opq_adfcb->adf_decimal.db_decimal,
		    iuflag, icflag);
	    SIprintf("\ndomain=%d, hist_cells=%d, f4null_count=%f",
		    idomain, ihcells,
		    incount, g->opq_adfcb->adf_decimal.db_decimal);
	}
# endif /* xDEBUG */

	/* value checks */
	if (inunique < (f8)1.0)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0951_BAD_NO_UNIQUE, (i4)6,
		(i4)sizeof(inunique), (PTR)&inunique,
		(i4)0, (PTR)tname,
		(i4)0, (PTR)cname);
	    continue;
	}
	if (irows < (i4)1)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0952_BAD_NO_ROWS, (i4)6,
		(i4)sizeof(irows), (PTR)&irows,
		(i4)0, (PTR)tname,
		(i4)0, (PTR)cname);
	    continue;
	}
	if (ipages < (i4)1)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0953_BAD_NO_PAGES, (i4)6,
		(i4)sizeof(ipages), (PTR)&ipages,
		(i4)0, (PTR)tname,
		(i4)0, (PTR)cname);
	    continue;
	}
	if (ioverflow < (i4)0)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0954_BAD_NO_OVFLOW, (i4)6,
		(i4)sizeof(ioverflow), (PTR)&ioverflow,
		(i4)0, (PTR)tname,
		(i4)0, (PTR)cname);
	    continue;
	}
	if (incount < (f8)0.0)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0955_BAD_NULL_COUNT, (i4)6,
		(i4)sizeof(incount), (PTR)&incount,
		(i4)0, (PTR)tname,
		(i4)0, (PTR)cname);
	    continue;
	}
	if (irptfactor < 1.0)
	{
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP0990_BAD_RPTFACTOR, (i4)8,
		(i4)sizeof(irptfactor), (PTR)&irptfactor,
		(i4)0, (PTR)cname,
		(i4)0, (PTR)tname,
		(i4)sizeof(irows), (PTR)&irows);
	    continue;
	}

	/* If this is a new statdump (has version field)
	** see if boundary value length makes sense. Should be same as what
	** adc_hg_dtln() returns except possibly for character types.
	*/
	if (version != NULL)
	{
	    if (histlength != (*attrp)->hist_dt.db_length)
	    {
		if (((*attrp)->hist_dt.db_datatype != DB_CHA_TYPE &&
		     (*attrp)->hist_dt.db_datatype != DB_BYTE_TYPE)
		    || (histlength < 1))
		{
		    opq_error((DB_STATUS)E_DB_ERROR,
			      (i4)E_OP091E_BAD_VALUE_LENGTH, (i4)6,
			      (i4)sizeof(histlength), (PTR)&histlength,
			      (i4)0, (PTR)tname,
			      (i4)0, (PTR)cname);
		    continue;
		}
		else
		{
		    (*attrp)->hist_dt.db_length = histlength;
		}
	    }
	}

	cells = 0;
	wrong_histo = (bool)FALSE;
	empty_histo = (bool)FALSE;
	tcount = (f8)incount;

	/* Force space allocation for histogram db_data_value */
	(*attrp)->hist_dt.db_data = (PTR) NULL;
	(VOID) getspace((char *)g->opq_utilid,
			(PTR *)&(*attrp)->hist_dt.db_data, 
			(u_i4)histlength);

	/* Allocate exacthist for boundary values and repetition factors.
	** If character type, include space for character set
	** statistics.
	*/
        {
	    u_i4	extra;	/* Extra space required, if any, for character
				** set statistics. */

	    if ((*attrp)->hist_dt.db_datatype == DB_CHA_TYPE ||
		(*attrp)->hist_dt.db_datatype == DB_BYTE_TYPE)
		extra = histlength * (sizeof(i4) + sizeof(f4));
	    else
		extra = 0;
	
	    (VOID) getspace((char *)g->opq_utilid, (PTR *)&exacthist, 
			    (u_i4)((histlength + sizeof(OPN_PERCENT)) * 
			    ihcells + extra)); 
	}

	/* Process statistical data. Execution of loop below will
	** result in reconstruction of histogram information.
	** When this info is ready, it will be inserted into catalogs
	** via insert_histo().
	*/
	
	rfpercell = TRUE;	/* assume per-cell rep factors are present */
	for (cells = 0; cells < ihcells; cells++)
	{
	    stat = SIgetrec(opq_line, sizeof(opq_line), inf);
	    if (stat != OK)
	    {
		if (stat != ENDFILE)
		{
		    char	buf[ER_MAX_LEN];

		    if ((ERreport(stat, buf)) != OK)
		    {
			buf[0] = '\0';
		    }

		    opq_error((DB_STATUS)E_DB_ERROR,
			(i4)E_OP0940_READ_ERR, (i4)4,
			(i4)sizeof(stat), (PTR)&stat,
			(i4)0, (PTR)buf);
		    (*opq_exit)();
		}
		if (cells == 0)
		    empty_histo = (bool)TRUE;
		break;
	    }

	    if (rfpercell)
	    {
		acnt = opq_scanf(opq_line, TEXT7I1,
		     g->opq_adfcb->adf_decimal.db_decimal, 
		     (PTR)&icellno, (PTR)&icount, (PTR)&irepf, (PTR)0, (PTR)0);
		if (cells == 0 && acnt == 2)
		{	/* 1st iihist row & no rep factor - must be old style */
		    rfpercell = FALSE;
		    irepf = irptfactor;		/* set default */
		}
	    }
	    if (!rfpercell)		/* old style, use old format */
		acnt = opq_scanf(opq_line, TEXT7I,
		     g->opq_adfcb->adf_decimal.db_decimal, 
		     (PTR)&icellno, (PTR)&icount, (PTR)0, (PTR)0, (PTR)0);

	    /* Do some cursory checking of input data */

	    if (cells == 0 || cells == 1)
	    {
		/*
		** There must be at least 2 cells
		** in the simplest histogram.
		*/
		if (!(rfpercell && acnt == 3 || !rfpercell && acnt == 2))
		{		/* not a valid cell record */
		    i4	    mincells = MINCELLS + 1;

		    if (cells == 0 && ihcells != 0)
		    {
		        opq_error((DB_STATUS)E_DB_ERROR,
			    (i4)W_OP096B_NO_HISTOGRAM, (i4)0);
		        empty_histo = (bool)TRUE;
		    } else
		    {
		        opq_error((DB_STATUS)E_DB_ERROR,
			    (i4)E_OP0948_CELL_TOO_FEW, (i4)2,
			    (i4)sizeof(i4), (PTR)&mincells);
		        wrong_histo = (bool)TRUE;
		    }
		    break;
		}
	    }

	    if (rfpercell && acnt == 3 || !rfpercell && acnt == 2)
	    {
		char		*p;
		char	    	m;
		i4	    	dlen;
		i4	    	reqs;
		i4	    	lnum;
		bool	    	pmchars;
		DB_DATA_VALUE 	idt;
		DB_DATA_VALUE 	unpackedHex;
		OPQ_IO_VALUE    unpack;
		ADI_FI_ID   	fid;
		DB_STATUS   	s;

		if (icellno >= (MAXCELLS+MAXCELLS))
		{
		    /* too many cells */
		    opq_error((DB_STATUS)E_DB_ERROR,
			(i4)E_OP0949_CELL_TOO_MANY, (i4)0); 
		    wrong_histo = (bool)TRUE;
		    break;
		}

		if (icellno != cells)
		{
		    /* Cell numbers are supposed to start at 0
		    ** and should increase.
		    */
		    opq_error((DB_STATUS)E_DB_ERROR,
			(i4)E_OP0946_CELL_NO, (i4)0); 
		    wrong_histo = (bool)TRUE;
		    break;
		}

		if (!(icount >= (f8)0.0 && icount <= (f8)1.0))
		{
		    /* count not in 0.0 .. 1.0 range */
		    opq_error((DB_STATUS)E_DB_ERROR,
			(i4)E_OP0947_WRONG_COUNT, (i4)0); 
		    wrong_histo = (bool)TRUE;
		    break;
		}

		tcount += icount;

		/* Read boundary value for this cell and convert it from string
		** to column's datatype
		*/

		m = 'v';
		p = STchr(opq_line, m);

		/*
		** Look to see if hex encoded buffer value
		*/
		hexFlag = (0 == STncmp(p,"value HEX",9));		

		/*
		** Now move pointer upto the value itself.
		*/
		m = ':';
		p = STchr(p, m);
		p++;



		dlen = STlength(p);
		dlen--;
		MEcopy((PTR)p, dlen,
		    (PTR)((DB_TEXT_STRING *) &opq_value_buf)->db_t_text);
		((DB_TEXT_STRING *) &opq_value_buf)->db_t_count = dlen;
		idt.db_data = (PTR)&opq_value_buf;
		idt.db_datatype = DB_LTXT_TYPE;
		idt.db_length = dlen + DB_CNTSIZE;
		idt.db_prec = 0;

		if (hexFlag)
		{
		    /* 
		    ** first unpack the hex
		    */
		    unpackedHex.db_data     = (PTR)&unpack;
		    unpackedHex.db_datatype = DB_LTXT_TYPE;
		    unpackedHex.db_prec     = 0;	
		    unpackedHex.db_collID   = 0;

		    /*
		    ** Do the work, and check result code.
		    */
		    s = adu_unhex(g->opq_adfcb, &idt, &unpackedHex);
		    if (DB_FAILURE_MACRO(s))
		    {
			opq_adferror(g->opq_adfcb, (i4)E_AD5007_BAD_HEX_CHAR,
				     (i4)0, (PTR)cname, (i4)0, (PTR)tname);
		    }

		    /*
		    ** Read the length produced in the un-pack process
		    */
		    unpackedHex.db_length = ((DB_TEXT_STRING *)unpackedHex.db_data)->db_t_count + DB_CNTSIZE;

		    /*
		    ** Look up coercion
		    */
		    s = adi_ficoerce(g->opq_adfcb, idt.db_datatype,
                                     (*attrp)->hist_dt.db_datatype, &fid);
		    if (DB_FAILURE_MACRO(s))
		    {
			opq_adferror(g->opq_adfcb, (i4)E_OP0981_ADI_FICOERCE, 
				     (i4)0, (PTR)cname, (i4)0, (PTR)tname);
		    }

		    {
			/*
			** Set up the coercion
			*/
			ADF_FN_BLK fblk;
			fblk.adf_fi_id = fid;
			fblk.adf_fi_desc = NULL;
			STRUCT_ASSIGN_MACRO((*attrp)->hist_dt, fblk.adf_r_dv);
			STRUCT_ASSIGN_MACRO(unpackedHex, fblk.adf_1_dv);
			fblk.adf_dv_n = 1;
			fblk.adf_pat_flags = AD_PAT_NO_ESCAPE;

			/*
			** Now do the coercion
			*/
			s = adf_func(g->opq_adfcb, &fblk);
		    }

		    if (DB_FAILURE_MACRO(s))
			opq_adferror(g->opq_adfcb,(i4)E_OP0982_ADF_FUNC, 
				     (i4)0, (PTR)cname, (i4)0, (PTR)tname);
		}
		else
		{
		    reqs = ADI_DO_BACKSLASH | ADI_DO_MOD_LEN;
		    s = adi_pm_encode(g->opq_adfcb, reqs,
				      &idt, &lnum, &pmchars);

		    if (DB_FAILURE_MACRO(s))
			opq_adferror(g->opq_adfcb, (i4)E_OP0980_ADI_PM_ENCODE,
				     (i4)0, (PTR)cname, (i4)0, (PTR)tname);

		    s = adi_ficoerce(g->opq_adfcb, idt.db_datatype, 
				     (*attrp)->hist_dt.db_datatype, &fid);

		    if (DB_FAILURE_MACRO(s))
			opq_adferror(g->opq_adfcb, (i4)E_OP0981_ADI_FICOERCE, 
				     (i4)0, (PTR)cname, (i4)0, (PTR)tname);

		    {
			ADF_FN_BLK fblk;

			fblk.adf_fi_id = fid;
			fblk.adf_fi_desc = NULL;
			STRUCT_ASSIGN_MACRO((*attrp)->hist_dt, fblk.adf_r_dv);
			STRUCT_ASSIGN_MACRO(idt, fblk.adf_1_dv);
			fblk.adf_dv_n = 1;
			fblk.adf_pat_flags = AD_PAT_NO_ESCAPE;
			s = adf_func(g->opq_adfcb, &fblk);
		    }
		    if (DB_FAILURE_MACRO(s))
			opq_adferror(g->opq_adfcb,(i4)E_OP0982_ADF_FUNC, 
				     (i4)0, (PTR)cname, (i4)0, (PTR)tname);
		}


		/* The value produced by operations above
		** should correspond to what adc_helem would
		** have produced.
		*/

		/* Store the current value in histogram struct */
		MEcopy((PTR)(*attrp)->hist_dt.db_data, histlength,
		       (PTR)(exacthist + icellno * histlength));

		exactcount[icellno] = icount;
		uniquepercell[icellno] = irepf;
	    }
	    else	/* acnt != 2 */
	    {
		/* Not a cell line, so not enough cells */
		break;
	    }
	}

	if (cells != ihcells && !empty_histo)
	{
	    /* declared no. of cells diffs from actual */
	    opq_error((DB_STATUS)E_DB_ERROR,
		(i4)E_OP093F_BAD_COUNT, (i4)4,
		(i4)sizeof(cells), (PTR)&cells,
		(i4)sizeof(ihcells), (PTR)&ihcells); 
	    wrong_histo = (bool)TRUE;
	}

	if (wrong_histo == (bool)TRUE)
	    continue;
	if (empty_histo)
	{
	    ihcells = 0;
	    icellno = 0;
	}

	/* To check the cumulative count we want to
	** calculate maximum tolerance first.
	*/	
	{
	    f8	eps;

	    /*
	    ** Determine epsilon for 1 cell, assumming that 
	    ** the cumulative count is approximately 1.0.
	    */
	    if (sizeof(OPN_PERCENT) == sizeof(f8))
		eps = DBL_EPSILON;
	    else
		eps = FLT_EPSILON;

	    /*
	    ** Calculate cumulative epsilon: epsilon * # of additions, ie.,
	    ** ((cells - 1 ) + 1 - for null count).
	    */
	    eps *= cells;	

	    if ((ihcells != 0) && (tcount > (1.0 + eps) || tcount < (1.0 - eps)))
	    {
		char    b_eps[50];	/* buf for 'eps' in ascii format */
		i2	b_eps_len;
		char    b_tcount[50];	/* buf for 'tcount' in ascii format */
		i2	b_tcount_len;

		/* convert tcount to an easily readable
		** string which ERlookup does not do.
		*/
		(VOID)CVfa(tcount,
		    (i4)g->opq_adfcb->adf_outarg.ad_f8width,
		    (i4)g->opq_adfcb->adf_outarg.ad_f8prec,
		    (char)g->opq_adfcb->adf_outarg.ad_f8style,
		    (char)g->opq_adfcb->adf_decimal.db_decimal,
		    b_tcount, &b_tcount_len);

		/* convert eps to an easily readable
		** string which ERlookup does not do.
		*/
		(VOID)CVfa(eps,
		    (i4)g->opq_adfcb->adf_outarg.ad_f8width,
		    (i4)g->opq_adfcb->adf_outarg.ad_f8prec,
		    (char)g->opq_adfcb->adf_outarg.ad_f8style,
		    (char)g->opq_adfcb->adf_decimal.db_decimal,
		    b_eps, &b_eps_len);
		
		/* issue a warning */
		opq_error((DB_STATUS)E_DB_ERROR,
		    (i4)W_OP0973_TOTAL_COUNT_OFF, (i4)8,
		    (i4)b_tcount_len, (PTR)b_tcount,
		    (i4)0, (PTR)tname,
		    (i4)0, (PTR)cname,
		    (i4)b_eps_len, (PTR)b_eps);
	    }
	}

	/* See if the histo values are in ascending order */
	{
	    register	    i4  i;
	    DB_DATA_VALUE   dv1, dv2;

	    STRUCT_ASSIGN_MACRO((*attrp)->hist_dt, dv1);
	    STRUCT_ASSIGN_MACRO((*attrp)->hist_dt, dv2);

	    for (i = cells - 2; i >= 0; i--)
	    {
		i4	    res;

		dv1.db_data = (PTR)(exacthist + i * histlength);
		dv2.db_data = (PTR)((OPH_CELLS)dv1.db_data + histlength);
		
		res = compare_values(g->opq_adfcb, &dv1, &dv2, &cname);

		if (res < 0)
		{
		    /* current value smaller than next - OK */
		}
		else if (res == 0)
		{
		    /* current value equal to next;
		    ** correction needed.
		    */
		    if ( (*(char *)(dv1.db_data) == 0) 
			&& (dv1.db_datatype == DB_CHA_TYPE ||
			    dv1.db_datatype == DB_BYTE_TYPE) )
		    {
			/* Do not decrement if character datatype and value is
			** null string (\000). You would wind up with \377,
			** which is greater since characters are unsigned.
			*/
		    }
		    else
		    {
			DB_STATUS	s;
			
			(*attrp)->hist_dt.db_data = 
			    (PTR)(exacthist + i * histlength);
			
			s = adc_hdec(g->opq_adfcb, &(*attrp)->hist_dt,
				     &(*attrp)->hist_dt);
			
			if (DB_FAILURE_MACRO(s))
			    opq_adferror(g->opq_adfcb,
					 (i4)E_OP0929_ADC_HDEC,
					 (i4)0, (PTR)&(*attrp)->attname,
					 (i4)0, (PTR)&(*relp)->relname);
		    }
		}
		else
		{
		    i4	    j;

		    /* i is register var and we need to pass
		    ** the address of cell no. to opq_error
		    */
		    j = i;

		    /* current greater than next - WRONG */
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0945_HISTVAL_NOT_INC, (i4)6,
			(i4)0, (PTR)tname,
			(i4)0, (PTR)cname,
			(i4)sizeof(j), (PTR)&j);
		    wrong_histo = (bool)TRUE;
		    break;
		}
	    }
	}

	if (wrong_histo == (bool)TRUE)
	    continue;
	
	/* If the column is of character type and there is a version
	** (statistics are from 6.5 or later), read character statistics and
	** add to end of histogram buffer */
	if (((*attrp)->hist_dt.db_datatype == DB_CHA_TYPE ||
	     (*attrp)->hist_dt.db_datatype == DB_BYTE_TYPE)&& 
		icellno > 0)
	    if (version != NULL)
	    	read_char_stats(g, inf, exacthist + ihcells * histlength,
			    histlength);
	    else
	    {
		/* We'll help out here by substituting default char stats 
		** (this also helps per-cell repetition factors to work) 
		*/
		i4	i;

    		/* charnunique and chardensity are global arrays */
    
    		for (i = 0; i < histlength; i++)
    		{
		    charnunique[i] = DB_DEFAULT_CHARNUNIQUE;
		    chardensity[i] = DB_DEFAULT_CHARDENSITY;
    		}
    		/* Copy default character set statistics to histogram buffer */
    		MEcopy((PTR)charnunique, histlength * sizeof(i4),
			(PTR)(exacthist + ihcells * histlength));
    		MEcopy((PTR)chardensity, histlength * sizeof(f4),
			(PTR)(exacthist + (ihcells + sizeof(i4)) * histlength));
		version = (char *)DU_DB6_CUR_STDCAT_LEVEL;
	    }

	/* add statistics to database */
	if (icellno == 0)
	    icellno = -1;
	insert_histo(g, *relp, *attrp, (OPO_TUPLES)inunique, exacthist, 
		     (OPN_PERCENT *)exactcount, (i4)icellno, (i4)irows,
		     (i4)ipages, (i4)ioverflow, (OPO_TUPLES)incount,
		     (bool)FALSE, (bool)TRUE, (char)iuflag, complete_flag,
		     (f8)irptfactor, (char *)version, (bool)TRUE);

        /* 07-jun-1999 (chash01) convert code to equivalent routine */
	if (pgupdate)
	{
	    (VOID) update_row_and_page_stats(g, (OPQ_RLIST *)*relp, 
		(i4)irows, (i4)ipages, (i4)ioverflow);
	}
    }

    return((DB_STATUS)E_DB_OK);
}

/*{
** Name: opq_sample - create a table with sample data
**
** Description:
**	Gather sample statistics for a table. This is accomplished by
**	creating a temp table and selecting rows at random and inserting
**	them into the table. Regular statistics are then creating on the
**	temp table holding sample data.
**
** Inputs:
**	g		pointer to global data structure
**	utilid		name of utility from which this procedure is called
**      relp		ptr to relation descriptor for table to gather 
**                      statistics on
**      attlst		ptr to array of ptrs to attribute
**                      descriptors for relation referred to by reldescp
**	samplepct
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      06-mar-89 (stec)
**          created.
**	13-sep-89 (stec)
**	    Fix code to process 1 row table correctly (bug 7933).
**	    Correct calculation of buffer length for statements.
**	01-nov-89 (stec)
**	    Treat TIDs as unsigned i4's rather than signed.
**	14-dec-89 (stec)
**	    Improve error reporting when creation of a table fails.
**	22-mar-90 (stec)
**	    To minimize logging added commit after each insert statement
**	    in the opq_sample() routine creating table with sample data.
**	26-apr-93 (rganski)
**	    Added typecasts to ST params to quiet warnings caused by ST
**	    prototypes.
**	20-sep-93 (rganski)
**	    Delimited ID support. Use relp->delimname instead of relp->relname
**	    and attrp->delimname instead of attrp->attname in select statement
**	    from base table. Unnormalize the name of the sample table, since it
**	    may contain embedded quotes and blanks.
**      02-Jan-1997
**          Change long to i4 to address BUG 79066 E_LQ000A conversion failure
**          on axp.osf
**	11-may-98 (inkdo01)
**	    Dropped OPQ_TERMS to 90, because it was causing server stack to 
**	    blow during compile of query with 100 tid's in where clause.
**      5-jul-00 (inkdo01)
**          Modified to use "where random() = ..." to do the sampling, and to
**          save results in a global temporary table.
**	27-apr-01 (inkdo01)
**	    Previous change neglected to restore "whenever" settings and 
**	    caused later SQL errors to be ignored.
**	29-may-01 (inkdo01)
**	    Removed superfluous sortsample parameter and TID column projection.
**	    Also added logic to build temp table for new fast query when cols
**	    in question warrant it.
**     31-Jan-2003 (hanal04) Bug 109519 INGSRV 2075
**          Added a two digit cycling sequence number to generated
**          temporary table names to prevent duplicate table names
**          when long table names which start with the same characters
**          are sampled in the same second.
**	21-may-03 (inkdo01)
**	    Use of session temp tables requires more robust name uniquification
**	    logic. Old loop just generates same name 10 times.
**	13-jun-03 (inkdo01)
**	    Changed random(0, i) function to randomf() for more precision.
**	28-apr-04 (hayke02)
**	    Modify previous change so that c_seq_no now has room for the null
**	    terminator. This change fixes problem INGSRV 2789, bug 112123. 
*/
static VOID
opq_sample(
OPQ_GLOBAL	*g,
char            *utilid,
OPQ_RLIST       *relp,
OPQ_ALIST       *attlst[],
f8		samplepct)
{
    char	*stmt;
    i4		secs;
    i4		retry;
    f4		random_comp = samplepct / 100.;
    bool	created;
    static u_i2 seq_no = 0;
    char        c_seq_no[3];
    bool	nosamp = FALSE;
 
# define OPQ_2RQRY   "declare global temporary table session.%s as select "
# define OPQ_3RQRY   " from %s where randomf() <= %-15.7f on commit preserve rows with norecovery"
# define OPQ_DD2RQRY "create table %s as select "
# define OPQ_DD3RQRY " from %s where randomf() <= %-15.7f with nojournaling"
# define OPQ_3ARQRY   " from %s on commit preserve rows with norecovery"
# define OPQ_DD3ARQRY   " from %s with nojournaling"
/* # define OPQ_3ARQRY   " from %s on commit preserve rows with norecovery, structure = heap, compression" */

    if (attlst[0] == (OPQ_ALIST *)NULL ||
	relp == (OPQ_RLIST*)NULL)
    {
	return;
    }

    if (samplepct <= 0.0)
     nosamp = TRUE;

    /* allocate space for query buffer */
    {
	register i4  len;

        if (g->opq_dbms.dbms_type & OPQ_STAR)
        {
	    len = sizeof(OPQ_DD2RQRY) - 1 + DB_GW1_MAXNAME+2;
	    len += sizeof(OPQ_DD3RQRY) - 1;
        }
        else
        {
	    len = sizeof(OPQ_2RQRY) - 1 + DB_GW1_MAXNAME+2;
	    len += sizeof(OPQ_3RQRY) - 1;
        }
	len += (relp->attcount + 1) * (DB_GW1_MAXNAME + 4 /* ", "*/);
	len += DB_GW1_MAXNAME+2;		/* relation name */
	len++;				/* terminating null */

	opq_bufalloc((PTR)utilid, (PTR *)&stmt, (i4)len, &opq_sbuf);
    }

    exec sql whenever sqlerror continue;
    exec sql whenever sqlwarning continue;
    secs = TMsecs();	/* init outside of loop */
    secs %= 65536;

    for (retry = 0, created = (bool)FALSE; retry < 50; retry++)
    {
	/* Now that we know that there is some work to be done
	** let's come up with a name for the sample data table
	*/
	{
	    i4	    	len;
	    char    	buf[DB_GW1_MAXNAME/2 + 1];
	    DB_TAB_STR	tempname;

	    len = STlength((char *)&relp->relname);

	    if (len > DB_GW1_MAXNAME/2)
	    {
		len = DB_GW1_MAXNAME/2;
	    }
	    
	    STncpy((char *)&tempname, (char *)&relp->relname,
			   len);
	    ((char *)(&tempname))[ len ] = '\0';

	    secs++;	/* increment each time through */
	    CVla(secs, buf);

	    (VOID) STcat((char *)&tempname, buf);

            seq_no++;
            if(seq_no > 99)
            {
                seq_no = 1;
            }
            STprintf(c_seq_no, "%02d", seq_no);
            (VOID) STcat((char *)&tempname, c_seq_no);

	    /* Unnormalize the sample table name because it may have embedded
	    ** quotes and blanks */
	    opq_idunorm((char *)&tempname, (char *)&relp->samplename);
	}

	/* Fill in the statement buffer */
	{
	    register OPQ_ALIST  **attlstp;  /* used to traverse the list of
					    ** attribute descriptors */
	    register bool	firstelem;

            if (g->opq_dbms.dbms_type & OPQ_STAR)
            {
	        (VOID) STprintf(stmt, OPQ_DD2RQRY, &relp->samplename);
            }
            else
            {
	        (VOID) STprintf(stmt, OPQ_2RQRY, &relp->samplename);
            }

	    for (attlstp = &attlst[0], firstelem = TRUE; *attlstp; attlstp++)
	    {
		if (!firstelem)
		{
		    (VOID) STcat(stmt, ", ");
		}
		else
		{
		    firstelem = FALSE;
		}

		(VOID) STcat(stmt, (char *)&(*attlstp)->delimname);
	    }

	    if (nosamp)
	    {
                if (g->opq_dbms.dbms_type & OPQ_STAR)
                {
		    char    elem[sizeof(OPQ_DD3ARQRY) + DB_GW1_MAXNAME+2];
      
		    (VOID) STprintf(elem, OPQ_DD3ARQRY, &relp->delimname);
		    (VOID) STcat(stmt, elem);
                }
                else
                {
		    char    elem[sizeof(OPQ_3ARQRY) + DB_GW1_MAXNAME+2];
      
		    (VOID) STprintf(elem, OPQ_3ARQRY, &relp->delimname);
		    (VOID) STcat(stmt, elem);
                }
	    }
	    else
	    {
                if (g->opq_dbms.dbms_type & OPQ_STAR)
                {
		    char    elem[sizeof(OPQ_DD3RQRY) + DB_GW1_MAXNAME+2];
      
		    (VOID) STprintf(elem, OPQ_DD3RQRY, &relp->delimname,
			    random_comp, g->opq_adfcb->adf_decimal.db_decimal);
		    (VOID) STcat(stmt, elem);
                }
                else
                { 
		    char    elem[sizeof(OPQ_3RQRY) + DB_GW1_MAXNAME+2];
      
		    (VOID) STprintf(elem, OPQ_3RQRY, &relp->delimname,
			    random_comp, g->opq_adfcb->adf_decimal.db_decimal);
		    (VOID) STcat(stmt, elem);
                }
	    }
	}

# ifdef xDEBUG
    {
	long	timeval;
	struct tm *timeval1;
	time(&timeval);
	timeval1 = localtime(&timeval);
	SIprintf("Start of table creation for %s, %s \n", relp->delimname, 
		asctime(timeval1));
    }
# endif /* (xDEBUG) */

	/* Create table for sample data */
	exec sql execute immediate :stmt;

	if (sqlca.sqlcode >= GE_OK)
	{
	    created = (bool)TRUE;
	    relp->samplecrtd = (bool)TRUE;
	    relp->nsample = sqlca.sqlerrd[2];
	    exec sql commit;
	    break;
	}
    }

    if (created == (bool)FALSE)
    {
	exec sql begin declare section;
	    char  error_msg[ER_MAX_LEN + 1];
	exec sql end declare section;

	exec sql inquire_ingres(:error_msg = errortext);

	/* Sample data table could not be created */
	opq_error( (DB_STATUS)E_DB_ERROR,
	    (i4)E_OP094C_ERR_CRE_SAMPLE, (i4)6,
	    (i4)sizeof(sqlca.sqlcode), (PTR)&sqlca.sqlcode,
	    (i4)sizeof(sqlca.sqlerrd[0]), (PTR)&sqlca.sqlerrd[0],
	    (i4)0, (PTR)error_msg);
	(*opq_exit)();
    }

# ifdef xDEBUG
    {
	long	timeval;
	struct tm *timeval1;
	time(&timeval);
	timeval1 = localtime(&timeval);
	SIprintf("End of table creation for %s, %s \n", relp->delimname, 
		asctime(timeval1));
    }
# endif  /* (xDEBUG) */

    exec sql whenever sqlerror call opq_sqlerror;
    exec sql whenever sqlwarning call opq_sqlerror;

}

/*{
** Name: opq_singres	execute ingres specific stuff.
**
** Description:
**  Executes commands that are INGRES specific. They are executed
**  to improve performance of the utility.
**
** Inputs:
**	g		ptr to global data struct
**	  opq_utilid	name of utility from which this proc is called
**	  opq_dbms	dbms info struct
**	relp		ptr to relation descriptor
**
** Outputs:
**	none
**
**	Returns:
**	    boolean indicating whether statistics on the table can be created.
**	    It will normally be TRUE, unless we have a non-Ingres table in the
**	    STAR environment and sampling has been requested.
**	Exceptions:
**	    none
**
** Side Effects:
**
** History:
**      08-mar-89 (stec)
**          Written.
**	22-mar-90 (stec)
**	    Introduced OPQ_GLOBAL structure to consolidate state information
**	    and pass around a ptr to it rather than a number of values. 
**	24-jul-90 (stec)
**	    Modified code to return status.
**	22-jan-91 (stec)
**	    Changed rollback to commit, since it is cheaper and it does not
**	    matter here, we just want to start with a clean TX, because this
**	    is what the "set lockmode" statement requires.
**	5-jun-91 (seputis)
**	    Commit after any direct execute immediate statement since STAR
**	    thinks it has an update transaction otherwise
**	20-sep-93 (rganski)
**	    Delimited ID support. Changed relp->relname to relp->delimname.
**  21-Jan-05 (nansa02)
**      Table name with delimited identifiers should be enclosed within '"'.
**      (Bug 113786)
**	28-Jan-2005 (schka24)
**	    No, already using delimname.
**	21-Mar-2005 (schka24)
**	    Un-normalize Star local table name before doing direct lockmode.
*/
static bool
opq_singres(
OPQ_GLOBAL	*g,
OPQ_RLIST       *relp)
{
# define SETLOCKMODE "set lockmode on %s where readlock = nolock"

    /* SET LOCKMODE is to be executed only when 
    ** the table is an Ingres table (has TIDs).
    ** Also for the RMS gateway where it is presently
    ** a noop but may not be it in the future.
    */
    if (g->opq_dbms.dbms_type & (OPQ_INGRES | OPQ_RMSGW))
    {
	exec sql begin declare section;
	    char	*stmt;
	exec sql end declare section;

	exec sql commit;

	opq_bufalloc((PTR)g->opq_utilid, (PTR *)&stmt,
	    (i4)(sizeof(SETLOCKMODE) + DB_GW1_MAXNAME+2), &opq_sbuf);

	(VOID) STprintf(stmt, SETLOCKMODE, &relp->delimname);

	exec sql execute immediate :stmt;

    }

    if (g->opq_dbms.dbms_type & OPQ_STAR)
    {
	exec sql begin declare section;
	    char	*es_objname;
	    char	*es_objowner;
	    char	es_node[DB_NODE_MAXNAME + 1];
	    char	es_dbms[DB_TYPE_MAXLEN + 1];
	    char	es_database[DB_DB_MAXNAME + 1];
	    char	es_locname[DB_LOC_MAXNAME + 1];
	exec sql end declare section;

	char	    es_delimname[DB_GW1_MAXNAME+2+1];
	i4	    cnt = 0;

	es_objname = (char *)&relp->relname;
	es_objowner = (char *)&relp->ownname;

	/*
	** Find out if the subject table is an Ingres table.
	** If so, "set lockmode" statement can be issued to
	** minimize locking and table can be sampled.
	*/
	exec sql repeated select i.ldb_node, i.ldb_dbms,
		    i.ldb_database, t.table_name
	    into    :es_node, :es_dbms,
		    :es_database, :es_locname
	    from    iiddb_ldbids i, iiddb_objects o,
		    iiddb_ldb_dbcaps c, iiddb_tableinfo t
	    where   o.object_name = :es_objname and
		    o.object_owner = :es_objowner and
		    o.object_base = t.object_base and
		    o.object_index = t.object_index and
		    o.object_type in ('L', 'T') and
		    t.ldb_id = c.ldb_id and
		    t.ldb_id = i.ldb_id and
		    t.local_type = 'T' and
		    c.ldb_id = i.ldb_id and
		    c.cap_capability = 'INGRES' and
		    c.cap_value = 'Y';

	exec sql begin;
	    {
		cnt++;
	    }
	exec sql end;

	if (cnt == 0)
	{
	    relp->sampleok = (bool)FALSE;

	    return (FALSE);
	}
	else if (cnt > 1)
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP093B_TOO_MANY_ROWS, (i4)0);
	    (*opq_exit)();
	}
	else	/* cnt == 1 */
	{
	    exec sql begin declare section;
		char	*stmt;
	    exec sql end declare section;

	    {
		i4 len;

		len = sizeof(SETLOCKMODE) + DB_GW1_MAXNAME + 2;

		opq_bufalloc((PTR)g->opq_utilid, (PTR *)&stmt, len, &opq_sbuf);
	    }

	    exec sql commit;	/* Start a new TX */

	    /* Un-normalize to add delimiters just in case they are needed */
	    opq_idunorm(es_locname, &es_delimname[0]);
	    (VOID) STprintf(stmt, SETLOCKMODE, es_delimname);

	    exec sql direct execute immediate :stmt
		with    node	= :es_node,
			dbms	= :es_dbms,
			database= :es_database;
	    exec sql commit;		/* commit, else STAR thinks this
					** may be an update transaction */
	}
    }

    return (TRUE);
}
static i4
opq_initcollate(ADULTABLE   *tbl)
{
    /*
    ** Test the collation table to see if it's 'simple'
    ** i.e., if it consists only of one-to-one mappings
    */
    if (tbl != NULL && tbl->nstate == 0)
    {
        i4  tblsize = sizeof(tbl->firstchar) / sizeof(i2);
        i4  i;

        for (i = 0; i < tblsize; i++)
        {
            if (tbl->firstchar[i] % COL_MULTI != 0)
                break;
        }
        if (i == tblsize)
            return 1; 
    }
    return 0;
}

/*{
** Name: main	- main entry point to optimizedb utility
**
** Description:
**      Program to generate statistics for cost based query optimizer
**
** Inputs:
**	argc	    argument count
**	argv	    array of ptr to command line arguments
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    updates IISTATISTICS catalog and IIHISTOGRAM catalog
**	    or their equivalent.
**
** History:
**      25-nov-86 (seputis)
**          initial creation from optimizdb.qc
**	12-02-88 (stec)
**	    Convert to Common SQL, use standard catalogs, cleanup.
**	    Add create statistics from a file data feature.
**	20-sep-89 (stec)
**	    Change interface to badarglist (add 1 argument for "-zqq" flag
**	    to be ignored in optimizedb).
**	22-mar-90 (stec)
**	    Changed exception handling code in main() to use defines rather
**	    than hardcoded constants.
**	    Changed the way statistics flag is set: for INGRES and gateways
**	    it's set after stats for each column get created, for STAR only
**	    after the first and last column.
**	10-apr-90 (stec)
**	    Allocates and initializes relation and attribute arrays.
**	06-jul-90 (stec)
**	    In case of STAR we will commit before calling opq_setstats().
**	    Update queries for iistats and iihistograms get routed over a
**	    different LDB server association than update queries for "set
**	    statistics". This would, at this time, cause a problem in a
**	    VAXcluster environment, in which 2PC feature in not supported.
**	10-jan-91 (stec)
**	    Changed length of ERreport() buffer to ER_MAX_LEN.
**	    Added IIUGinit() call to initialize CM attribute table (requested
**	    by stevet).
**	22-jan-91 (stec)
**	    Added support for "zw" flag. 
**	13-jan-91 (stec)
**	    Removed call to inithistdec().
**	12-feb-92(rog)
**	    Change OPQ_ESTATUS to FAIL.
**	14-dec-92 (rganski)
**	    Changed application flag from DBA_VERIFYDB (-A5) to DBA_OPTIMIZEDB.
**	    Changed to use macro DBA_OPTIMIZEDB instead of hard-coded constant.
**	    Added some explanatory comments to existing code.
**	18-jan-93 (rganski)
**	    Added bound_length parameter to badarglist() call. This allows user
**	    to control boundary value lengths with new -length flag.
**	18-oct-93 (rganski)
**	    Added call to opq_collation to get local collation table, for calls
**	    to ADF. b48831.
**	2-nov-93 (robf)
**          Verify we are authorized to run optimizedb
**	10-nov-93 (robf)
**          Save password prompt (if any ) so user doesn't get asked twice.
**  10-apr-95 (nick)(cross-int angusm)
**      read_process() no longer takes pgupdate as a parameter - it is now
**      a global. #44193
**	13-sep-96 (nick)
**	    Call EXsetclient() else we don't catch user interrupts.
**      15-Jun-98 (wanya01) bug 91345
**          Reinitialize ADF errblk before process next column
**  29-Mar-99 (wanfr01)
**      INGSRV 714, Bug 95292: 
**	    If table >= 10 relextends, skip sampling for last "relextend"
**	    pages.  Otherwise run with no sampling.  This avoids the E_DM93A7
**	    error that occurs when you select a tid on an uninitialized page.
**	9-june-99 (inkdo01)
**	    Changes to support "-o <filename>" option to create output file
**	    instead of updating catalog.
**	6-apr-00 (inkdo01)
**	    Changes for composite histograms.
**	05-jul-00 (inkdo01)
**	    Change queries for sampled histograms to reference global temp 
**	    table.
**      28-aug-2000 (chash01) 07-jun-99 (chash01)
**      roll in modifications that make optimizedb work for rmsgateway also.
**	29-may-01 (inkdo01)
**	    Various changes to support fast aggregate query and use of temp
**	    table for fast query.
**	17-july-01 (inkdo01)
**	    Changes for support of "-xr" command line option.
**	21-Mar-2005 (schka24)
**	    Ditch the relextend adjustment, not needed now that
**	    sampling doesn't generate random tids.
**	22-mar-2005 (thaju02)
**	    Changes for support of "+w" SQL option.
**	15-july-05 (inkdo01)
**	    Change default behaviour to sample any table bigger than
**	    1000000 rows (or possibly some paramterized value) unless
**	    overridden by -zns or -zs#.
**	11-oct-05 (inkdo01)
**	    Call opq_setstats() after relp loop to minimize locking 
**	    interference.
**	26-oct-05 (inkdo01)
**	    Set g->opq_sample to avoid confusion with -zfq's that generate
**	    temp tables.
**	01-nov-2005 (thaju02)
**	    Add statdump param to i_rel_list_from_input().
**	16-mar-06 (wanfr01)
**	    Bug 115043
**	    Change relextend from short to int so it can handle tables 
**	    with more than 65K relextends.
**	29-Nov-2006 (kschendel)
**	    Allocate freqvec any time -zdn is specified, without looking
**	    for user specified sampling;  optimizedb itself decides to sample
**	    large tables now.
**	    Force serializable isolation level to avoid catalog deadlock.
**	6-dec-2006 (dougi)
**	    Reposition calls to opq_setstats() to execute after each table
**	    is processed, so we don't flood the server with locks.
**	23-May-2007 (kibro01) b118054
**	    If no stats are added (e.g. heap table, no index) then don't
**	    set statistics on the table.
**	6-Jun-2007 (kibro01) b118406
**	    Extend fix for b118054 so that tables with no rows (and hence)
**	    no created statistics, also don't get their table_stats updated.
**	23-Jul-2007 (kibro01) b118730
**	    Don't update table_stats if a optimizedb -o <outfile> 
**      31-Mar-2008 (huazh01)
**          if both '-zk' and '-zcpk' flag is set, toggle the value of 
**          "opq_comppkey" and loop over the code that generates statistics
**          for a column. Otherwise, the '-zk' flag will be ignored.
**          bug 120141. 
**	29-may-2008 (Holly's birthday) (Dougi) Bug 120435
**	    Very small -zr values trip over the exact cell logic and overlay
**	    memory.
**	02-sep-08 (hayke02)
**	    Commit after each "set statistics" command to avoid taking
**	    exclusive locks on every table in the database in a single
**	    transaction.
*/
i4
main(
i4	argc,
char	*argv[])
{
    exec sql begin declare section;
	char	    *ifs[MAXIFLAGS];	/* flags to ingres */
	char	    *user;
        char	    appl_flag[20];
	i4          relextend;
	i4	    tbl_reltid;
	i4	    tbl_reltidx;
	char        *waitopt = NULL;
        char        *stmt;
        i4          len;
    exec sql end declare section;

#define OPQ_DDTDROP "drop %s"
    char	*argvp;
    char	**dbnameptr;
    EX_CONTEXT	excontext;
    i4		parmno;
    char	*infnm;		    /* input file name ptr */
    char	*outfnm;	    /* output file name ptr */
    FILE	*inf;
    FILE	*outf;
    OPQ_RLIST	**ex_rellist = NULL;
    f8		samplepct;
    bool	default_sample;     /* default value for sampling - 
				    ** will change if table too small 
				    */
    bool	nosample;	    /* for "-zns" option - overrides new
				    ** table cardinality-based sampling 
				    */
    bool	excl = FALSE;
    bool	*nothing_done_list = NULL;	/* were no stats required? */
    bool	*was_nothing_done;
    bool        firsttime = TRUE, retry = FALSE; 

    (void)EXsetclient(EX_INGRES_TOOL);

    MEadvise(ME_INGRES_ALLOC);

    /* Initialize CM attribute table. */
    if (IIUGinit() != OK)
    {
	PCexit((i4)FAIL);
    }

    /* set the exit function */
    opq_exit = opt_opq_exit;
    usage = opt_usage;

    /* zero fill the global struct */
    MEfill (sizeof(OPQ_GLOBAL), 0, (PTR)&opq_global);

    opq_global.opq_adfcb = &opq_global.opq_adfscb;
    opq_global.opq_utilid = "optimizedb";

    /* initialize histogram pointers */
    exacthist = inexacthist = (OPH_CELLS) NULL;
    freqvec = (f4 *) NULL;

    opq_retry = OPQ_NO_RETRIES;
    if (EXdeclare(opq_exhandler, &excontext) != OK)
    {
	if (opq_excstat == OPQ_INTR || opq_excstat == OPQ_DEFAULT)
	    IIbreak();
	else
	    /* serialization error has occurred */
	    opq_error((DB_STATUS)E_DB_WARN, (i4)W_OP0970_SERIALIZATION, 
		      (i4)0);
	opq_exit();	    /* should not return - exit after closing database */
    }

    /*
    ** Initialize ADF for internal use and initialize ADF
    ** parameters for ingres, so that results are consistent.
    */
    adfinit(&opq_global);

    /* read info from command file if necessary */
    fileinput(opq_global.opq_utilid, &argv, &argc);

    /* validate the parameter list */
    {
	bool	junk;			    /* uninteresting options */

	if (badarglist(&opq_global, argc, argv, &parmno, ifs,
	    &dbnameptr, &verbose, &histoprint, &key_attsplus, &minmaxonly,
	    &uniquecells, &regcells, &junk, &junk, &user, &infnm, &outfnm, 
	    &pgupdate, &samplepct, &junk, &opq_cflag, &nosetstats,
	    &bound_length, &waitopt, &nosample))
	{
	    /* print usage and exit */
	    usage();
	}
    }

    /* Set exinexcells and allocate corresponding array. It tries to add
    ** 25% exact cells with minimum 25 and maximum 1000. */
    exinexcells = (i4) (((f4)regcells) * 0.25);
    if (exinexcells < 25)
	exinexcells = 25;
    else if (exinexcells > 1000)
	exinexcells = 1000;
    (VOID) getspace((char *)opq_global.opq_utilid, (PTR *)&hcarray,
	(u_i4)(exinexcells * sizeof(RCARRAY)));

    if (regcells < exinexcells)
	regcells = exinexcells;		/* min regcells to avoid boundary bug */

    opq_global.opq_sample = sample = (samplepct != 0.0);
    if (opq_global.opq_newrepfest)
    {
	/* New distinct value estimation requires allocation of
	** frequency count array. */
	(VOID) getspace((char *)opq_global.opq_utilid, (PTR *)&freqvec,
		(i4)(MAXFREQ * sizeof(f4)));
    }

    /* parmno will contain the offset of the database name */
    while ((argvp = argv[parmno]) && CMalpha(argvp))	/* a database name */
    {
	exec sql begin declare section;
	    char	*dbname;
	exec sql end declare section;

	bool		retval;

	dbname = argvp;			/* get the database name, note that 
                                        ** dbnameptr will place the database 
                                        ** name in the ifs array, so that the
                                        ** following ingres command will pick
                                        ** it up */

	opq_global.opq_dbname = dbname;

	parmno++;			/* set ptr to beginning of relation
					** parameters */

	/*
	** Options to Ingres and gateways differ, we have to log in twice.
	** First time we do not pass any options that the user might have
	** specified. After connecting we check iidbcapabilities to see if
	** it is an Ingres database, or not, then we log out and reconnect
	** with user specified options.
	*/

	/*
	** Save passwords if necessary for prompting
	*/
	exec sql set_sql (savepassword=1);

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

	/* See if Ingres dbms and if it's the correct version */
	opq_dbmsinfo (&opq_global);

	exec sql disconnect;

	opq_global.opq_env.dbopen = FALSE;
        /*
	** Check if we  are allowed to run optimizedb
	*/
	if(opq_global.opq_no_tblstats==TRUE)
	{
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP0984_NO_PRIV_TABLE_STATS, (i4)0);
	    opq_exit();
	}


	/*
	** If sampling specified, and not STAR and not Ingres then
	** error (no sampling on GWs). In case of STAR, sampling may
	** be possible but is not guaranteed, further checking will
	** be done later.
	*/
	if (sample == TRUE &&
	    !(opq_global.opq_dbms.dbms_type & (OPQ_STAR | OPQ_INGRES))
	   )
	{   /* sampling on a non-Ingres database not allowed */
	    opq_error( (DB_STATUS)E_DB_ERROR,
		(i4)E_OP094D_CANT_SAMPLE, (i4)2,
		(i4)STlength(SystemProductName), SystemProductName);
	    opq_exit();
	}

	if (opq_global.opq_dbms.dbms_type & 
		(OPQ_INGRES | OPQ_STAR | OPQ_RMSGW))
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
	    /* If Ingres DBMS, ensure we're in serializable mode to
	    ** help prevent catalog deadlock.
	    */
	    if (opq_global.opq_dbms.dbms_type & OPQ_INGRES)
	    {
		exec sql set session isolation level serializable;
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

	/* Find out collation sequence, if any, and initialize ADF control
	** block's collation sequence field */
	opq_collation(&opq_global);
        collationtbl = (ADULTABLE *)(opq_global.opq_adfcb->adf_collation);
        if (opq_initcollate(collationtbl) == 0)
	    collationtbl = NULL;

	/* Get Unicode collation table, too. */
	opq_ucollation(&opq_global);

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

	    size = (opq_global.opq_maxrel + 1) * sizeof(bool);

	    /* Get array to use to hold a "don't update statistics" flag in
	    ** case no statistics are actually added (kibro01) b118054
	    */
	    nothing_done_list=(bool*)getspace((char *)opq_global.opq_utilid,
		(PTR *)&nothing_done_list, size);
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

		retval = i_rel_list_from_input(&opq_global, ex_rellist, &argv[parmno], (bool)FALSE);
		retval = r_rel_list_from_rel_rel(&opq_global, rellist, ex_rellist, 
								(bool)FALSE);
	    }
	    else
	    {
		/* fetch ONLY relation names specified since -r is specified */
		retval = i_rel_list_from_input(&opq_global, rellist, &argv[parmno], (bool)FALSE);
	    }
	}
	else
	{
	    /* fetch all relations in database since -r was NOT specified */
	    retval = r_rel_list_from_rel_rel(&opq_global, rellist, NULL, 
								(bool)FALSE);
	}

	if (retval)	/* relations read and initialized successfully */
	{
	    OPQ_RLIST	**relp;

	    inf = (FILE *) NULL;
	    outf = (FILE *) NULL;

	    if (infnm)
	    {
		/* open the input file */
		LOCATION    inf_loc;
		STATUS	    status;

		if (outfnm)	/* can't have both -i and -o */
		{
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0968_OPTDB_IANDO, (i4)0);
		    opq_exit();
		}

		if (
		    ((status = LOfroms(PATH & FILENAME, infnm, &inf_loc)) != OK)
		    ||
		    ((status = SIopen(&inf_loc, "r", &inf)) != OK)
		    || 
		    (inf == (FILE *) NULL)
		   )
		{   /* report error opening file */
		    char	buf[ER_MAX_LEN];

		    if ((ERreport(status, buf)) != OK)
		    {
			buf[0] = '\0';
		    }

		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0937_OPEN_ERROR, (i4)8,
			(i4)0, (PTR)opq_global.opq_utilid,
			(i4)0, (PTR)infnm, 
			(i4)sizeof(status), (PTR)&status,
			(i4)0, (PTR)buf);
		    opq_exit();
		}

		opq_global.opq_env.ifid = inf;	    
	    }
	    else if (outfnm)
	    {
		/* open the output file */
		LOCATION    outf_loc;
		STATUS	    status;

		if (
		    ((status = LOfroms(PATH & FILENAME, outfnm, &outf_loc)) != OK)
		    ||
		    ((status = SIopen(&outf_loc, "w", &outf)) != OK)
		    || 
		    (outf == (FILE *) NULL)
		   )
		{   /* report error opening file */
		    char	buf[ER_MAX_LEN];

		    if ((ERreport(status, buf)) != OK)
		    {
			buf[0] = '\0';
		    }

		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0937_OPEN_ERROR, (i4)8,
			(i4)0, (PTR)opq_global.opq_utilid,
			(i4)0, (PTR)outfnm, 
			(i4)sizeof(status), (PTR)&status,
			(i4)0, (PTR)buf);
		    opq_exit();
		}

		opq_global.opq_env.ofid = outf;	    
	    }

	    if (inf)
	    {
		DB_STATUS   stat;

		if (minmaxonly)
		{
		    /* Request to read statistics from a file and
		    ** create minmax statistics is not a valid one.
		    */
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP093D_MINMAX_NOT_ALLOWED, (i4)0);
		    opq_exit();
		}

		if (sample == (bool)TRUE)
		{
		    /* Request to read statistics from a file and
		    ** create sampled statistics is not a valid one.
		    */
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP094F_SAMPLE_NOT_ALLOWED, (i4)0);
		    opq_exit();
		}

		/* process statistics from the input file */
		stat = read_process(&opq_global, inf, &argv[parmno]);
		if (DB_FAILURE_MACRO(stat))
		{
		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP093C_INP_FAIL, (i4)4,
			(i4)0, (PTR)opq_global.opq_utilid,
			(i4)0, (PTR)infnm);
		    opq_exit();
		}

		/* close the input file */
		stat = SIclose(inf);
		if (stat != OK)
		{   /* report error closing file */
		    char	buf[ER_MAX_LEN];

		    if ((ERreport(stat, buf)) != OK)
		    {
			buf[0] = '\0';
		    }

		    opq_error( (DB_STATUS)E_DB_ERROR,
			(i4)E_OP0938_CLOSE_ERROR, (i4)8,
			(i4)0, (PTR)opq_global.opq_utilid,
			(i4)0, (PTR)infnm, 
			(i4)sizeof(stat), (PTR)&stat,
			(i4)0, (PTR)buf);
		    opq_exit();
		}		    

		opq_global.opq_env.ifid = (FILE *)NULL;
	    }
	    else	/* input file not specified */
	    {
		opq_global.opq_env.ifid = (FILE *)NULL;
		default_sample = sample;

		for (relp = rellist, was_nothing_done = nothing_done_list;
			*relp;
			relp++, was_nothing_done++)
		{
		    bool	tempt;

		    tempt = FALSE;
		    opq_global.opq_sample = sample = default_sample;

		    if (!sample && !nosample &&
			(*relp)->ntups > OPQ_DEFSAMPLE)
		    {
			opq_global.opq_sample = sample = TRUE;
			samplepct = (f8) OPQ_DEFSAMPLE / (f8)(*relp)->ntups 
				* 100.0; /* 1M / ntups * 100 to make it % */
		    }

		    opq_retry = OPQ_SER_RETRIES;
		    (VOID) EXdelete();
		    if (EXdeclare(opq_exhandler, &excontext) != OK)
		    {
			if (opq_excstat == OPQ_INTR ||
			    opq_excstat == OPQ_DEFAULT
			   )
			{
			    IIbreak();
			    opq_exit();	/* should not return - exit 
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

		    /*
		    ** Set lockmode (issued from within opq_singres)
		    ** must be the very first statement in a 
		    ** transaction, so opq_singres rolls back first.
		    ** For STAR also determine if the table can be sampled.
		    */
		    {
			bool	ret;

			ret = opq_singres(&opq_global, (*relp)); 

			if (sample == TRUE && ret == FALSE)
			{
			    /*
			    ** Issue a warning, we can't sample this
			    ** non-Ingres table, continue with the next one.
			    */
			    opq_error( (DB_STATUS)E_DB_WARN,
				(i4)W_OP0950_NOSAMPLING, (i4)4,
				(i4)STlength(SystemProductName),
					SystemProductName,
				(i4)0, (PTR)&(*relp)->relname);

			    continue;
			}
		    }

                    /* bug 120141: if both -zk and -zcpk flag is on, 
                    ** then loop over the code and do the '-zk' and 
                    ** '-zcpk' work one by one. 
                    */ 
                    if (key_attsplus && opq_global.opq_comppkey)
                    {
                       retry = TRUE; 
                       opq_global.opq_comppkey = FALSE; 
                    }

                    firsttime = TRUE; 
                    do
                    {
                    if (retry && !firsttime)
                    {
                       retry = FALSE;
                       opq_global.opq_comppkey = TRUE;
                    }
                    firsttime = FALSE;

    		    if (att_list(&opq_global, attlist, *relp, &argv[parmno]))
		    {
			OPQ_ALIST	**attrp;
			f8	rowprop;
			i4	totcolsize;

			/* Compute sum of column sizes. If histogrammed cols
			** represent less than 30% of total rowsize and there
			** are at least 3 columns, the new aggregate query
			** will go faster yet on an instantiated session 
			** temp table. */
			for (totcolsize = 0, attrp = attlist; *attrp; attrp++)
			    totcolsize += (*attrp)->attr_dt.db_length;
			rowprop = ((f8)totcolsize) / ((f8)(*relp)->relwid);
						/* proportion of row repr.
						** by histo. columns */

			if (sample == (bool)TRUE ||
				opq_global.opq_newquery && rowprop < 0.3 &&
				!opq_global.opq_nott && (*relp)->attcount > 2)
			{
  			/* If table <= 10 relextends, skip sampling.  */
			/* FIXME should we be checking this for the
			** newquery case as well ??? (schka24)
			** probably should skip this test entirely.
			*/

		 	    tbl_reltid=(*relp)->reltid;
			    tbl_reltidx=(*relp)->reltidx;                        
                            if(opq_global.opq_dbms.dbms_type & OPQ_STAR)
                            {
                                relextend = 0;
                            }
                            else
                            {
                                exec sql repeated select relextend into
                                          :relextend from iirelation i
                                   where reltid  = :tbl_reltid and
                                          reltidx = :tbl_reltidx;
                            }

			    if (relextend * 10 > (*relp)->pages)
                            {
                                sample = (bool)FALSE;
                            } 
                            else
                            {
				opq_sample(&opq_global, opq_global.opq_utilid, 
				    *relp, attlist, samplepct);
				tempt = TRUE;
			    }
			}

 			if (minmaxonly)
			{
			    minmax(&opq_global, *relp, attlist);

			    /* Commit to save minmax stats for the relation */
			    exec sql commit;
			}
			else
			{

			    /* If there is nothing in the first attribute, then
			    ** there is nothing to do, so mark that we've done
			    ** nothing (kibro01) b118054
			    */
			    if (*attlist == NULL)
				*was_nothing_done = TRUE;

			    for (attrp = attlist; *attrp; attrp++)
			    {
				DB_STATUS	stat;

                                opq_global.opq_adfscb.adf_errcb.ad_errcode=0;
                                opq_global.opq_adfscb.adf_errcb.ad_errclass=0;
				opq_retry = OPQ_SER_RETRIES;
				(VOID) EXdelete();
				if (EXdeclare(opq_exhandler, &excontext) != OK)
				{
				    if (opq_excstat == OPQ_INTR ||
					opq_excstat == OPQ_DEFAULT
				       )
				    {
					IIbreak();
					opq_exit(); /* should not return - exit 
						    ** after closing database.
						    */
				    }
				    else
				    {
					/* serialization error has occurred */
					opq_error((DB_STATUS)E_DB_WARN,
					   (i4)W_OP0970_SERIALIZATION,
					   (i4)0);
				    }
				}

				if (opq_global.opq_newquery)
				    stat = process_newq(&opq_global, *relp, 
								attrp, sample, tempt);
				else stat = process_oldq(&opq_global, *relp, 
								attrp, sample);

				if (stat == E_DB_OK)
				{
				    /* It is desirable to commit after each 
				    ** column is processed, since gathering 
				    ** statistics is time consuming and we want
				    ** to save as much as possible in case of a
				    ** failure, and we want catalogs to be
				    ** consistent.
				    */
				    exec sql commit;
				    if ((*relp)->index || 
						opq_global.opq_comppkey) break;
						/* composites are done */
				}
				else if (stat == E_DB_WARN)
				{
				    /*
				    ** NO ROws, or NULL values only, no
				    ** stats created just go on to the 
				    ** next column/table.
				    ** ...which of course means nothing was
				    ** actually done (kibro01) b118406
				    */
				    *was_nothing_done = TRUE;
				    break;
				}
				else
				{
				    break;
				}

			    } /* for each attribute */

			    if (opq_global.opq_dbms.dbms_type & OPQ_STAR)
			    {
				/* We must commit in case of Ingres/STAR
				** because we have just updated iistats and
				** iihistograms, and these queries get routed 
				** over non-privileged association; set
				** statistics will result in an update of
				** iitables routed over the privileged
				** association. If we did not commit, 2PC on
				** a VAXcluster would return an error.
				*/
				exec sql commit;

				/*
				** In case of STAR we minimize set stats,
				** we do it after the first attribute and
				** when table is done.
				*/
				/* Don't update if -o <outfile>
				** (kibro01) b118730
				*/
				if (!*was_nothing_done &&
				    !opq_global.opq_env.ofid)
				    opq_setstats(&opq_global, (*relp));
			    }

			} 

			opq_retry = OPQ_SER_RETRIES;
			(VOID) EXdelete();
			if (EXdeclare(opq_exhandler, &excontext) != OK)
			{
			    if (opq_excstat == OPQ_INTR ||
				opq_excstat == OPQ_DEFAULT
			       )
			    {
				IIbreak();
				opq_exit(); /* should not return - exit 
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
		    } /* if (att_list()) */
                    }
                    while (retry); 

                    if ((opq_global.opq_dbms.dbms_type & OPQ_STAR) &&
                        tempt)
                    {
                        /* STAR does not support GTTs so tempt means we
                        ** created a real table, must drop it.
                        */
                        len = sizeof(OPQ_DDTDROP) - 1 + DB_GW1_MAXNAME+2 + 1;
                        opq_bufalloc((PTR)opq_global.opq_utilid, (PTR *)&stmt, 
                                     (i4)len, &opq_sbuf);
                        (VOID) STprintf(stmt, OPQ_DDTDROP, 
                                        &(*relp)->samplename);

                        /* drop 'temp' table for sample data */
                        exec sql execute immediate :stmt;
                    }
		}

		/* Loop over all relp's doing "set statistics". It is
		** deferred 'til here to minimize locking interference 
		** with underlying tables. */
		if (!(opq_global.opq_dbms.dbms_type & OPQ_STAR))
		{
		    for (relp = rellist, was_nothing_done = nothing_done_list;
			*relp;
			relp++, was_nothing_done++)
		    {
			/* Don't update if -o <outfile> (kibro01) b118730 */
			if ((*relp)->statset == FALSE && !*was_nothing_done &&
				!opq_global.opq_env.ofid)
                      {
			    opq_setstats(&opq_global, (*relp));
                            exec sql commit;
                      }
		    }

		    exec sql commit;
		} /* for each table */
	    }
	}

	while (argv[parmno] && argv[parmno][0] == '-')
	    parmno++;				/* skip to next db */

	exec sql disconnect;

	opq_global.opq_env.dbopen = FALSE;
    }

    (VOID) EXdelete();
    PCexit((i4)OK);
    /*NOTREACHED*/
    return 0;
}

/*{
** Name: opt_opq_exit() -	exit handler for optimizer utilities
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
**	19-dec-89 (stec)
**	    Added cleanup code for temporary sampling tables.
**	12-feb-92 (rog)
**	    Changed call to PCexit() to FAIL instead of OK.
**	9-june-99 (inkdo01)
**	    Support for "-o <filename>" option.
**      07-Feb-2000 (hanal04) Bug 100372 INGSRV 1107
**         Prevent an infinite loop of opt_opq_exit(), opq_sqlerror() calls
**         by disabling the jump to opq_sqlerror() on sqlerror.
*/
static
VOID
opt_opq_exit(VOID)
{
    static i4     exit_calls=0;

    exit_calls++;
    /* Allow MAXEDIT levels of recursion before disabling normal
    ** error handling.
    */
    if(exit_calls > MAXEXIT)
    {
        exec sql whenever sqlerror continue;
        exec sql whenever sqlwarning continue;
    }

    if (opq_global.opq_env.dbopen)
    {
	/* Rollback will close all open cursors. */
	exec sql rollback;

	exec sql disconnect;
	opq_global.opq_env.dbopen = FALSE;
    }

    if (opq_global.opq_env.ifid)
    {
	(VOID)SIclose(opq_global.opq_env.ifid);
	opq_global.opq_env.ifid = (FILE *)NULL;
    }

    if (opq_global.opq_env.ofid)
    {
	(VOID)SIclose(opq_global.opq_env.ofid);
	opq_global.opq_env.ofid = (FILE *)NULL;
    }

    (VOID) EXdelete();
    PCexit((i4)FAIL);
}

/*{
** Name: opq_bufalloc	- manage buffer for dynamic SQL queries.
**
** Description:
**      This routine will check if dynamic SQL buffer has been
**	allocated, and/or whether it is large enough. Existing
**	buffer will be deallocated when necessary and a new one
**	wiil be allocated.
**
** Inputs:
**      utilid			ptr to string with name of utility
**	mptr			address of location where pointer
**				to allocated memory should be placed
**      len			size in bytes of memory required
**	buf			ptr to buffer struct
**
** Outputs:
**	mptr			address of location with pointer
**				to allocated memory
**
**	Returns:
**	    None
**	Exceptions:
**
**
** Side Effects:
**	    Allocates memory.
**
** History:
**      23-jan-89 (stec)
**          Written.
**	21-jun-93 (rganski)
**	    Check status returned by MEfree.
*/
static VOID
opq_bufalloc(
PTR	    utilid,
PTR	    *mptr,
i4	    len,
OPQ_DBUF    *buf)
{
    STATUS	status;

    if (buf->length >= len)
    {
	*mptr = buf->bufptr;
	return;
    }
    else
    {
	if (buf->length != 0)
	{
	    /* release existing buffer */
	    status = MEfree(buf->bufptr);
	    if (status != OK)
		/* Abort with memory error */
		nostackspace(utilid, status, len);

	    buf->bufptr = (PTR) NULL;
	    buf->length = 0;
	}

	/* allocate new buffer */
	buf->bufptr = (PTR) NULL;
	(VOID) getspace((char *)utilid, (PTR *)&buf->bufptr, (u_i4)len);
	buf->length = len;	

	*mptr = buf->bufptr;
    }
}

/*{
** Name: update_row_and_page_stats - Update system catalogs with new row & page count
**
** Description:
**      The row count and page counts used to populate the statistics for
**	the attribute of this relation are now stored directly into the 
**	system catalogs.
**
** Inputs:
**      g                   ptr to global data struct
**        opq_dbcaps        database capabilities struct
**          cat_updateable  mask showing which standard catalogs are
**                          updateable.
**      relp                ptr to relation descriptor
**      attrp               ptr to attribute descriptor upon
**                          which the histogram is built
**      rows                number of tuples in relation
**      pages               number of prime pages
**      overflow            number of overflow pages
**
** Outputs:
**      none
**
**      Returns:
**          status              TRUE if error, else FALSE
**      Exceptions:
**          none
**
** Side Effects:
**          updates IITABLES catalogs or their equivalent.
**
** History:
**	05-dec-97 (chash01)
**	    Moved out of "read_process()" and generalized for use by other
**	    routines.
**
**	    For RMS-GW databases:  update related INDEX information in
**	    IIRELATION as follows:
**		* num_rows       = same as assoc. base table (no change here)
**		* number_pages   = same as assoc. base table.
**		* overflow_pages = same as assoc. base table (=0)
**				
**          NOTE 1: RMS-GW secondary indexes are not really underlying tables,
**                  but this info should simulate for the optimizer at least
**                  these pertinent facts.  Hopefully this helps it choose a
**                  good plan for RMS-GW database accesses.
**
**	    NOTE 2: IITABLES is a (non-updateable) view in RMS-Gateway databases
**                  so handling is done to IIRELATION.
**
**	    NOTE 3: On setting number_pages:  it is probably best to
**                  overestimate this value so that the query optimizer decides
**                  to do a full SCAN of a base table or index as its worst-case
**                  choice.  The base_table value is already set in the gateway
**                  server to the allocation page size independent of
**                  fill_factor, so this is as big an overestimate as is prudent
**                  at this time.
**	28-Jan-2005 (schka24)
**	    Partitioned tables compute master row/page counts from the
**	    partitions.  When jamming row/page stuff for partitioned
**	    tables, jam into the partitions too, otherwise the table looks
**	    like it's still the original number of rows/pages.  Assume an
**	    even distribution to the partitions (no better info is available).
*/
static 
DB_STATUS
update_row_and_page_stats(g, relp, rows, pages, overflow)
OPQ_GLOBAL         *g;
OPQ_RLIST          *relp;
i4            rows;
i4            pages;
i4            overflow;
{
    /*
    ** If the system catalogs are UPDATEABLE then
    ** use IITABLES, otherwise use IIRELATION ...
    */
    if (g->opq_dbcaps.cat_updateable & OPQ_TABLS_UPD)
    {
	exec sql begin declare section;
	    char    *es1_tname;
	    char    *es1_owner;
	    i4      es1_rows;
	    i4      es1_pages;
	    i4      es1_overflow;
	exec sql end declare section;

        if (opq_global.opq_dbms.dbms_type & OPQ_STAR)
        {
		/* We must commit in case of Ingres/STAR
		** because we have just updated iistats and
		** iihistograms, and these queries get routed
		** over non-privileged association; query
		** below will update iitables and will be routed
		** over the privileged association. If we did not
		** commit, 2PC on a VAXcluster would return an error.
		*/
		exec sql commit;
        }

        es1_tname = (char *)&(relp)->relname;
        es1_owner = (char *)&(relp)->ownname;
        es1_pages = (i4)pages;
        es1_overflow = (i4)overflow;
        es1_rows = (i4)rows;

        exec sql repeated update iitables
        set number_pages   = :es1_pages,
	    overflow_pages = :es1_overflow,
	    num_rows       = :es1_rows
        where table_name  = :es1_tname and
		table_owner = :es1_owner;

	/* Update related index information. */
	exec sql repeated update iitables
	    set num_rows       = :es1_rows
	    where table_owner  = :es1_owner and
	    table_name  in (select index_name
		from iiindexes
		where base_name  = :es1_tname and
		base_owner = :es1_owner);
# ifdef FUTURE_USE
		exec sql repeated update iiphysical_tables
		    set number_pages   = :es1_pages,
			overflow_pages = :es1_overflow,
			num_rows       = :es1_rows
		    where table_name  = :es1_tname and
			table_owner = :es1_owner;
# endif /* FUTURE_USE */

        if (opq_global.opq_dbms.dbms_type & OPQ_STAR)
        {
		/* Since we are in a loop, more statistics may have
		** to be inserted into iistats, iihistograms therefore
		** we want to commit here. The principle is not to mix
		** updates of iistats, iihistograms with other catalog
		** updates because 2PC on a VAXcluster is not supported
		** at this time.
		*/
		exec sql commit;
        }
    }
    else
    {
	exec sql begin declare section;
		    i4	es2_reltid;
		    i4	es2_xreltid;
		    i4	es2_rows;
		    i4	es2_pgtotal;
                    i4  es2_pgprim;
                    i2  es2_relspec;
	            i4  idx_pgtotal;    /* 05-dec-97 (chash01) */
		    i4	num_partitions;
		    i4	rows_per_partition;
		    i4	pages_per_partition;
	exec sql end declare section;

	es2_reltid  = relp->reltid;
	es2_xreltid = relp->reltidx;
	es2_pgtotal = (i4)pages;
	es2_rows = (i4)rows;

        if(!(opq_global.opq_dbms.dbms_type & OPQ_RMSGW))
        {       
            exec sql select relprim, relspec, relnparts
                     into :es2_pgprim, :es2_relspec, :num_partitions
                     from iirelation i
                    where reltid  = :es2_reltid and
                          reltidx = :es2_xreltid;
/*
                if (es2_relspec == 5 || es2_relspec == 7) 
                    es2_pgprim = es2_origpgprim; 
*/
	    exec sql repeated update iirelation
		    set reltups  = :es2_rows,
			relpages = :es2_pgtotal
		    where reltid  = :es2_reltid and
			  reltidx = :es2_xreltid;

	    /* Update partitions too.  Divide evenly among partitions,
	    ** give any leftover to the zero'th partition.
	    */
	    if (num_partitions > 0)
	    {
		rows_per_partition = es2_rows / num_partitions;
		pages_per_partition = es2_pgtotal / num_partitions;
		exec sql update iirelation
		    set reltups = :rows_per_partition,
			relpages = :pages_per_partition
		    where reltid = :es2_reltid
			and reltidx < 0
			and relnparts != 0;

		/* Compute extra that might go into first partition, so that
		** total adds up to total rows and pages exactly.
		*/
		rows_per_partition = es2_rows - (rows_per_partition * (num_partitions-1));
		pages_per_partition = es2_pgtotal - (pages_per_partition * (num_partitions-1));
		exec sql update iirelation
		    set reltups = :rows_per_partition,
			relpages = :pages_per_partition
		    where reltid = :es2_reltid
			and reltidx < 0
			and relnparts = 0;
	    }

	    /* Update related index information. */
	    exec sql repeated update iirelation
		    set reltups   = :es2_rows
		    where reltid  = :es2_reltid and
			  reltidx in (select indexid
				from iiindex
				where baseid = :es2_reltid);
        }
        else
        {
	    es2_reltid  = (relp)->reltid;
	    es2_xreltid = (relp)->reltidx;
	    es2_pgprim = (i4)(pages - overflow);
	    es2_rows = (i4)rows;

            /* we must caulcuate the page estimate for RMSGW */
            /* the calculation for repeating group table is  */
            /* very rough, but this is the only way          */
            es2_pgtotal = relp->relwid*rows/512 + 1;
            if (relp->pages < (relp->relwid*relp->ntups/512+1))
            {
                /*
                ** filesize is smaller than calculated size, even if 
                ** there are less rows, we still use the earlier 
                ** page number
                */
                es2_pgtotal = relp->pages;
            }
            
	    exec sql repeated update iirelation
	         set reltups  = :es2_rows,
		     relpages = :es2_pgtotal,
		     relprim  = :es2_pgprim
	         where reltid  = :es2_reltid and
		     reltidx = :es2_xreltid;

	    /*
	    ** Update related index information.
	    ** 21-apr-94 (jimg)
	    ** Handle RMS-GW database differently ...
	    */
            {
	        f8  w = 1.0;        /* page-weighting factor */
	        idx_pgtotal = (i4)(es2_pgtotal * w);

                exec sql repeated update iirelation
		     set reltups  = :es2_rows,
		         relpages = :idx_pgtotal
		     where reltid  = :es2_reltid and
		     reltidx in (select indexid
		     from iiindex
		     where baseid = :es2_reltid);
            }
        }
    }
    return((DB_STATUS)E_DB_OK);
}

static bool		
collatable_type(i4	datatype)
{
	switch (abs(datatype))
	{
	case DB_CHA_TYPE:
	case DB_VCH_TYPE:
	case DB_VBYTE_TYPE:
	case DB_VBIT_TYPE:
	case DB_TXT_TYPE:
	case DB_NVCHR_TYPE:
		return(TRUE);
	default:
		return(FALSE);
	}
}

