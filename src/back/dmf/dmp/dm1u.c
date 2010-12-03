/*
** Copyright (c) 1989, 2010 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <gl.h>

#include    <cm.h>
#include    <cs.h>
#include    <di.h>
#include    <ex.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <tr.h>
#include    <st.h>

#include    <iicommon.h>
#include    <dbdbms.h>

#include    <adf.h>
#include    <adp.h>
#include    <scf.h>
#include    <ulf.h>

#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>

#include    <lg.h>
#include    <lk.h>

#include    <dm.h>

#include    <dmftrace.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dmppn.h>
#include    <dm0m.h>
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm1cx.h>
#include    <dm1p.h>
#include    <dm1u.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dmpepcb.h>
#include    <dmpecpn.h>
#include    <sqlstate.h>

/**
**  Name: DM1U.C - DMF Verify Table Logic.
**
**  Description:
**
**      A user table consists of two portions:
**	    1.  The description of the table in the system catalogs -- this
**		tells the characteristics of the table: name, owner, storage
**		structure, indexing, how many columns, data-type of each column,
**		name of each column, whether or not the data is compressed, if
**		duplicates are permitted, etc.  However, the data dictionary 
**		does not contain any of the user data associated with that 
**		table.
**	    2.  The data is contained in a disk file.  There are 4 different 
**		storage structures:  heap, hash, isam, btree.  The pages in the
**		data file are intrepreted based on the table's storage type.
**		Some table types have only data (or data overflow) pages, and
**		others have assorted index-type of pages to help the server find
**		the desired data page faster.  But all user data is contained
**		on data (or data overflow) pages.   This data cannot be
**		interpreted without information from the DBMS catalogs about the
**		table.
**
**	A data page has three sections on it:
**	    1.  Header information - contains dynamic information about the
**		transaction and static information about the page, such as
**		the number of records on the page, the page type, forward 
**		pointers to other pages in the files, etc.
**	    2.  page_line_tab - this is an array of offsets to the data that
**		comprises one tuple.  There is a separate page_line_tab entry
**		for every tuple on every page.  This corresponds to the TID
**		(part of the TID is the page number, and part of the tid is the
**		offset into the page_line_tab array).
**	    3.  The data portion of the page -- this portion of the page contains
**		the actual data on a page.
**
**	The check file logic examines the file for correct page pointer chains
**	(including pages that are referenced more than one and pages that are
**	not referenced at all), that all entries on index pages correctly point
**	to a valid data page, and that the data page itself appears valid.
**
**	The patch file logic attempts to repair damage to the file.  It does
**	this by converting the file's structure to heap, validating data pages
**	and throwing away all non-data pages.  (Since most corruption is ususally
**	to index page pointers rather than to the data page itself, this 
**	strategy will usually get back all of the user data.  HOWEVER, IT CANNOT
**	BE A SUPPORTED FEATURE.  WHENEVER WE ATTEMPT TO RECOVER A DAMAGED FILE,
**	WE RISK LOSING ALL USER DATA.
**	
**	NOTE: There is always the possibility that a file may be spread over
**	      several locations.  If one of those locations is unavailable
**	      (disk off line, etc), then the check/patch operation cannot 
**	      accurately deal with the table.  Therefore, it is the user's
**	      responsibility to assure that all locations are online/accessable
**	      before attempting to check/patch a table.
**
**	This module contains the following external functions:
**	    dm1u_verify - Main verify table entry point.
**
**  History:
**      11-Aug-1989 (teg)
**          initial creation.
**	25-OCT-1989 (teg)
**	    added support for checking hash, heap and btree storage structures.
**      22-feb-1990 (teg)
**          add check isam support.
**	13-jun-1990 (teresa)
**	    add support for user interrupts by calling fix_page to check
**	    for user interrupts then call dm0p_fix_page.  It used to call
**	    dm0p_fix_page directly.
**      12-jun-1990 (teresa)
**          fixed bug 30771 where isam key comparisions were done wrong and 
**          introduced new routine isam_copy_key
**	22-jun-1990 (bryanp)
**	    Fixed a bug in find_next(). 
**	    Changed the DMZ_TBL_MACRO calls to use decimal constants rather than
**	    octal constants, to conform with the rest of DMF (and to fix a bug
**	    introduced by accidentally depending on a VAX C extension where 08
**	    and 09 are legal octal constants with the values decimal 8 and
**	    decimal 9). Only decimal constants should be used in DMZ_TBL_MACRO
**	    calls.
**	    Added page_dump() routine to dump a page
**	    with TRdisplay, and called it under control of new trace point 10
**	    from find_next() when find_next() decides the page is invalid. Added
**	    comments to find_next() and validate_line_table() to indicate
**	    that they should only be called for data pages, and added a comment
**	    to dm1u_patch() to describe its reliance on the page_stat bits.
**	10-oct-90 (teresa)
**	    Cleaned up above changes, fixed bug in dm1u_patch() and in 
**	    validate_line_table().
**	19-nov-90 (rogerk)
**	    Initialize the server's timezone setting in the adf control block.
**	    This was done as part of the DBMS Timezone changes.
**	15-mar-1991 (Derek)
**	    Re-organized to add support for allocation bitmaps verification
**	    and repair.  Changed the interface so that the verifydb support
**	    and the debug support through modify use the same interface.
**	    Made many changes to handle new pages that can appear in a 
**	    table.
**	29-mar-91 (teresa)
**	    Fix alignment problem in call to dm1c_get
**	8-aug-1991 (bryanp)
**	    B37626, B38241: On BYTE_ALIGN machines, we must properly align each
**	    column's value before calling adc_valchk to check the value.
**	21-aug-1991 (bryanp)
**	    Added support for Btree index compression. At this time, I merely
**	    translated direct dm1b macro page accesses to the appropriate
**	    dm1cx() layer subroutine calls. At some future time we need to
**	    consider whether additional checks should be added to examine
**	    compressed Btree index pages for structural integrity.
**	17-oct-1991 (rogerk)
**	    Added checks to btree_index and isam_index to make sure we don't
**	    exit leaving an index page fixed.
**      24-oct-1991 (jnash)
**          Fix a number of LINT problems, note that there are many more 
**	    that remain unfixed.
**	28-feb-1992 (bryanp)
**	    B41841: It's OK for a totally empty data page to be orphaned in a
**	    Btree. This is a normal occurrence in Btrees and does not indicate
**	    corruption.
**	29-may-1992 (bryanp)
**	    B44465: Btree free-list pages do not need to be patched.
**	7-july-1992 (jnash)
**	    Add DMF function prototypes.
**      08-jul-1992 (mikem)
**          Fixed code problems uncovered by prototypes.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Many changes including replacement of routine 
**	    data-next-line with an accessor. Replaced dmqc_format, dm1c_get, 
**	    dm1c_put calls with accessors. Replaced setting zero in line table
**	    to 'zap' a tuple with a proper delete code sequence. Changed
**	    calls on dmpp_get_offset_macro to be calls on an accessor.
**	14-sep-1992 (bryanp)
**	    B45176: page_version is unpredictable on converted R5 tables.
**      25-sep-1992 (stevet)
**           Get default timezone from TMtz_init() instead of TMzone().
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	13-oct-1992 (jnash)
**	    Reduced loggin project. 
**	      - Call tuple level vectors to perform compression operations.
**	      - Substitute dmpp_tuplecount() for dmpp_isempty().
**	26-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.  Also replaced
**	    DM_CMP_LIST with DB_CMP_LIST.
**	08-feb-1993 (rmuth)
**	    Various changes
**	      - Use dm1p_checkhwm and tcb_lpageno to determine the end of
**	        the table as dm2f_sense_file returns the end-of-file which
**	        is not the same as end-of-table.
**	      - dm1p_verify no longer takes a flag parameter.
**	      - Change log_error to dm1u_log_error.
**	      - A Previous change added a dm1u_cb parameter to the talk
**	        routine, not all calls to talk had been changed accordingly.
**	      - Add dm1u_repair function.
**	      - Change talk to be dm1u_talk and remove the static restriction
**	        so that it can be called by the dm1p code which verifies
**		FHDR/FMAP pages as parts of dm1u
**	      - Log an information message in the error log recording the
**	        tablename etc...if the user runs patch/repair on a table.
**	30-mar-1993 (rmuth)
**	    Check table code was incorrectly allocating memory for its
**	    bitmaps. This caused the code to think it was encountering
**	    pages twice and hence generating errors.
**
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**		Include <lg.h> to pick up logging system typedefs
**	21-jun-1993 (rogerk)
**	    Fix compile warnings.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	23-July-1993 (rmuth)
**	    - Log information message when a user runs check on a table.
**	    - When patching a table, if possible try and use the tables
**	      existing FHDR/FMAP this is the only data structure which knows
**	      where the end-of-table is.
**	26-jul-1993 (rogerk)
**	    Fix problems with table_verify of btrees that have empty leaf
**	    and overflow pages.
**      13-sep-93 (smc)
**          Made TRdisplay use %p for pointers.
**	18-oct-1993 (rmuth)
**	    B45710, Correct a problem where a dm1u_talk call was 
**	    missing a comma between last two parameters.
**      08-Feb-1996 (mckba01)
**          b74050, patched table becomes HEAP, UNIQUE constraint is not
**          allowed.  Removed UNIQUE id from relstat field when updating
**          iirelation to reflect changes to patched table.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove now-unused page_dump() subroutine.
**	    Pass tcb_rel.relpgsize as page_size argument to dmpp_check_page.
**	    When reformatting pages, pass tcb_rel.relpgsize as the page_size
**		argument to dmpp_format.
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_reclen.
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_tuplecount.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate either the DM1U_CB or the DM1U_ADF control block on
**		the stack. These control blocks are very large and must be
**		dynamically allocated.
**      06-mar-1996 (stial01)
**          Use tcb_rel.relpgsize in btree routines, 
**          dm1u_verify() alloc tuple buf (once) in case we need it.
**          data_page() don't alloc tuple buf on stack
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Change page header references to use macros.
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      18-jul-1996 (ramra01 for bryanp)
**          Extract the row's version and pass it to the uncompress routine.
**          Pass tcb_rel.relversion to dmpp_put.
**	30-aug-1996 (kch)
**	    Added NO_OPTIM for Siemens Nixdorf SINIX (rmx_us5) to prevent
**	    SIGBUS when MEcopy() is called from data_page(). This is a
**	    temporary solution to allow completion of a patch.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass temporary buffer
**	    for uncompression.  Use the temporary buffer for uncompression
**	    when available.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space
**          Put accessor: changed DMPP_TUPLE_INFO param to table_version
**	17-apr-1997 (pchang)
**	    Verifydb SIGBUS on a BYTE_ALIGN machine (axp/osf) in data_page()
**	    as described above due to code optimization by the compiler.
**	    The subject for optimization in dm1u_verify() involved the field
**	    dm1u_colbuf used for BYTE_ALIGN storage.  Modified its parent
**	    structure (DM1U_ADF) to eliminate the problem.  Also, removed the
**	    NO_OPTIM hint previously added for rmx_us5 to address the same
**	    problem.  (B79871)
**      10-mar-97 (stial01)
**          dmu_verify() Pass relwid to dm0m_tballoc()
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**      07-jul-98 (stial01)
**          status E_DB_WARN from dm1cxget is ok for V2 pages
**      12-Feb-1999 (thaju02)
**          Calls to MEfill to clear the bitmaps were casted to u_i2, resulted
**          in verifydb -mreport on large tables greater than 1GB
**          erroneously reporting structural problems. Created
**          clean_bmap(). This change is based on natjo01's oping12
**          change 435757 to address bug 88922 in OI 1.2/01. (B88922)
**	14-aug-1998 (hanch04)
**	    replace ule_format with ule_doformat
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      12-Apr-1999 (stial01)
**          Distinguish leaf/index info for BTREE/RTREE indexes
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	20-oct-1999 (nanpr01)
**	    Optimized the code to minimize the tuple header copy.
**	21-oct-1999 (nanpr01)
**	    More CPU Optimization. Do not copy tuple header when we do not
**	    need to.
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**      15-nov-1999 (hweho01)
**          Corrected argument mismatch in ule_format() call.
**      18-jan-2000 (stial01)
**          Validate tcb_kperpage, tcb_kperleaf for BTREE tables
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026) 
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      30-oct-2000 (stial01)
**          Fix references to sizeof V2 page elements.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**          Peform DIRECT updates here in Verify Table code
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      01-may-2001 (stial01)
**          Init adf_ucollation
**	19-Dec-2002 (hanch04)
**	    Fix bad cross integration
**	01-apr-2003 (devjo01) b109956
**	    Fix table patch algorithm, to prevent data corruption
**	    when resultant table is used for inserts.
**	22-Jan-2004 (jenjo02)
**	    A host of changes for Partitioning and Global indexes,
**	    particularly row-locking with TID8s (which include
**	    partition number (search for "partno") along with TID).
**	    LEAF pages in Global indexes will contain DM_TID8s 
**	    rather than DM_TIDs; the TID size is now stored on
**	    the page (4 for all but LEAF, 8 for TID8 leaves).
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      25-jun-2004 (stial01)
**          Modularize get processing for compressed, altered, or segmented rows
**	09-Sep-2004 (bonro01)
**	    Fix bad integration of last change.
**	16-dec-04 (inkdo01)
**	    Add various collID's.
**	21-feb-05 (inkdo01)
**	    Init. Unicode normalization flag.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	13-Jun-2006 (jenjo02)
**	    Using DM1B_MAXSTACKKEY instead of DM1B_KEYLENGTH, call dm1b_AllocKeyBuf() 
**	    to validate/allocate Leaf work space. DM1B_KEYLENGTH is now only the 
**	    max length of key attributes, not necessarily the length of the Leaf entry.
**	19-Feb-2008 (kschendel)
**	    Reorganize tuple access info into DMP_ROWACCESS.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2f_?, dm2t_?, dm2r_? functions converted to DB_ERROR *
**	02-Dec-2008 (jonj)
**	    SIR 120874: dm1b_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? dm1u_? functions converted to DB_ERROR *, use new
**	    form uleFormat
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm1c_?, dm0p_? functions converted to DB_ERROR *
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	19-Aug-2009 (kschendel) 121804
**	    Need cm.h for proper CM declarations (gcc 4.3).
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	16-Nov-2009 (kschendel for jgramling) SIR 122890
**	    Implement find_fhdr() routine, called in response to the DM1U_REPAIR
**	    verify operation (modify tab to table_repair). This routine will
**	    now attempt to find a valid fhdr page in the table, and update
**	    the iirelation relfhdr accordingly.
**      24-nov-2009 (stial01)
**          Check if table has segmented rows (instead of if pagetype supports)
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	4-May-2010 (kschendel)
**	    DM9026 can't take parameters, fix here.
**	01-Nov-2010 (frima01) BUG 124670
**	    Enforce 8 Byte alignment for dm1u_colbuf to avoid BUS errors.
*/

#define MAXUI2    (2 * MAXI2 + 1)    /* Largest u_i2 */

#define	CHECK_MAP_MACRO(dm1u, page)\
    ((dm1u)->dm1u_map[(page) / 8] & (1 << ((page) & 7)))
#define	CHECK_CHAIN_MACRO(dm1u, page)\
    ((dm1u)->dm1u_chain[(page) / 8] & (1 << ((page) & 7)))
#define SET_MAP_MACRO(dm1u, page)\
    ((dm1u)->dm1u_map[(page) / 8] |= (1 << ((page) & 7)))
#define SET_CHAIN_MACRO(dm1u, page)\
    ((dm1u)->dm1u_chain[(page) / 8] |= (1 << ((page) & 7)))

#define		MAX_BAD_SEGMENTS	10


/*
**  Forward and/or External function references.
*/

static DB_STATUS btree(
    DM1U_CB	   *dm1u,
    DB_ERROR	   *dberr);

static i4   btree_data(
    DM1U_CB	   *dm1u,
    u_i4	   page_num,
    u_i4	   line_num,
    DB_ERROR	   *dberr);

static DB_STATUS btree_index(
    DM1U_CB	   *dm1u,
    DM_PAGENO	   index_page_number,
    DB_ERROR	   *dberr);

static DB_STATUS btree_lchain(
    DM1U_CB	   *dm1u,
    DB_ERROR	   *dberr);

static DB_STATUS btree_leaf(
    DM1U_CB	   *dm1u,
    DM_PAGENO	   leaf_page_number,
    DB_ERROR	   *dberr);

static DB_STATUS btree_line_table(
    DM1U_CB	   *dm1u,
    DMPP_PAGE	   *page,
    u_i4	   page_num,
    char	   *key,
    DB_ERROR	   *dberr);

static DB_STATUS check(
    DM1U_CB	   *dm1u,
    DB_ERROR	   *dberr);

static DB_STATUS periph_check(
    DM1U_CB	   *dm1u,
    DB_ERROR	   *dberr);
    
static DB_STATUS dm1u_repair(
    DM1U_CB	   *dm1u,
    DB_ERROR	   *dberr);


static DB_STATUS check_orphan(
    DM1U_CB	   *dm1u_cb,
    DB_ERROR	   *dberr);

static VOID      data_page(
    DM1U_CB	   *dm1u,
    DMPP_PAGE	   *page,
    DB_ERROR	   *dberr);

static DB_STATUS fix_page(
    DM1U_CB	   *dm1u,
    DM_PAGENO      page_number,
    i4        action,
    DMP_PINFO	   *pinfo,
    DB_ERROR	   *dberr);

static STATUS    handler(
    EX_ARGS        *args);

static DB_STATUS hash(
    DM1U_CB	   *dm1u,
    DB_ERROR	   *dberr);

static DB_STATUS heap(
    DM1U_CB	   *dm1u,
    DB_ERROR	   *dberr);

static	DB_STATUS isam(
    DM1U_CB	   *dm1u,
    DB_ERROR	   *dberr);

static DB_STATUS isam_index(
    DM1U_CB	   *dm1u,
    DM_PAGENO	   index_page_number,
    DB_ERROR	   *dberr);

static VOID dm1u_log_error(
    DB_ERROR	   *dberr);

static DB_STATUS main_chain(
    DM1U_CB	   *dm1u,
    DM_PAGENO	   main_page,
    DM_PAGENO	   exp_page_main,
    DB_ERROR	   *dberr);

static DB_STATUS patch_physical(
    DM1U_CB	   *dm1u,
    i4		   *fhdr_pageno,
    DB_ERROR	   *dberr);

static DB_STATUS patch_relation(
    DM1U_CB	   *dm1u,
    i4		   fhdr_pageno,
    DB_ERROR	   *dberr);

static DB_STATUS find_fhdr(
    DM1U_CB		   *dm1u,
    i4		   *fhdr_pageno,
    DB_ERROR	   *dberr);

static VOID      unfix_page(
    DM1U_CB	   *dm1u,
    DMP_PINFO	   *pinfo);

static VOID	 clean_bmap(
    i4		   size,
    char	   *map);

static PTR       tmtz_cb=0;


/*{
** Name: dm1u_verify	- Main entry for all verify type operations.
**
** Description:
**	This routine contains the code to drive the execution of any verify
**	table operation.  Two basic modes or operation are supported:
**	report and fix.  In report mode, all problems are reported but
**	nothing is changed.  In fix mode, all problems are reported and
**	a fix is attempted.  The scope of the report of fix operations
**	can be varied.  The scope includes: bitmaps, links, records and
**	attributes.  Multiple levels can be checked a once.  A scope of
**	attrubute inplies record and link, and a scope of record implies
**	link.
**
**	It calls the appropriate routines to process the check or patch table
**	request.
**
** Inputs:
**      rcb                             Record Control Block.  Contains
**      xcb				Transaction control block
**	type				Type of table operation.
**
** Outputs:
**      err_code                        error code:
**					    E_DM9104_ERR_CK_PATCH_TBL
**					    
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_FATAL
**	Exceptions:
**	    EX_JNP_LJMP
**
** Side Effects:
**	    Pages in the file are fixed and unfixed
**	    IF type=DM1U_PATCH
**		The user table becomes a heap
**		Tuples may disappear from the user table
**
** History:
**      16-Aug-89 (teg)
**	    Initial Creation.
**	1-nov-89  (teg)
**	    tuck rcb ptr into dm1u_cb so we dont have to pass so many 
**	    parameters to low level support routines.
**	19-nov-90 (rogerk)
**	    Initialize the server's timezone setting in the adf control block.
**	    This was done as part of the DBMS Timezone changes.
**	15-mar-1991 (Derek)
**	    Re-organized to include support for new allocation bitmaps.
**	8-aug-1991 (bryanp)
**	    B37626, B38241: On BYTE_ALIGN machines, we must properly align each
**	    column's value before calling adc_valchk to check the value.
**	    To do so, we allocate a special buffer in the DM1U_ADF, called
**	    dm1u_colbuf, and copy each value to that buffer.
**      25-sep-1992 (stevet)
**           Get default timezone from TMtz_init() instead of TMzone().
**	23-oct-92 (teresa)
**	    Implement SIR 42498 by accepting a parameter to indicate whether
**	    to suppress or display informative messages.
**	30-feb-1993 (rmuth)
**	    Various changes to get verifydb to work.
**	      - Add calls to dm1u_repair.
**	      - Log an information message in the error log recording the
**	        tablename etc...if the user runs patch/repair on a table.
**	23-July-1993 (rmuth)
**	    - Log the information message even if the user is just running
**	      in check mode. This is because if the table is corrupt we
**	      may log errors to the dbms log file so want to know where
**	      they came from.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate either the DM1U_CB or the DM1U_ADF control block on
**		the stack. These control blocks are very large and must be
**		dynamically allocated.
**	13-sep-1996 (canor01)
**	    Use the temporary buffer passed as part of the rcb for data
**	    uncompression.
**	17-apr-1997 (pchang)
**	    Verifydb SIGBUS on a BYTE_ALIGN machine (axp/osf) when doing an
**	    MEcopy() in data_page() due to code optimization by the compiler.
**	    The subject for optimization involved the field dm1u_colbuf used
**	    for BYTE_ALIGN storage.  Modified its parent structure (DM1U_ADF)
**	    to eliminate the problem.  (B79871)
**	11-aug-97 (nanpr01)
**	    Add the t->tcb_comp_atts_count with relwid in dm0m_tballoc call.
**	15-jan-1999 (nanpr01)
**	    Pass ptr to ptr to dm0m_deallocate call.
**      14-Jul-2004 (hanal04) Bug 112283 INGSRV2819
**          Provide a local buffer to be referenced by ad_errmsgp so that
**          SPerror2() in the spatial routines in ads can return error
**          text to the caller.
**	11-May-2007 (gupsh01)
**	    Initialize adf_utf8_flag.
**	27-Jun-2007 (kschendel) b118588
**	    Need to pass the fhdr through patch_relation and patch_physical
**	    for patching into iirelation.
**	28-Jun-2007 (jgramling) SIR 122890
**	    Call find_fhdr() routine in response to the DM1U_REPAIR verify
**	    operation (modify tab to table_repair). Note that this change
**	    alters the behavior of the table_repair operation: previously,
**	    the operation called dm1p_repair directly, which assumes that
**	    the open table's relfhdr attribute points to a valid but
**	    potentially corrupted FHDR. This would blindly reinitialize
**	    whatever page relfhdr happened to point to, turning it into
**	    an FHDR (even if it happened to be a non-fhdr page, or a valid
**	    data page).  Since there is just as much chance that relfhdr
**	    is wrong as that the FHDR is broken, this blind reinitialization
**	    of an FHDR is not terribly useful.
**	    Now, instead of immediately attempting repair of the SMS pages,
**	    we will first check if the relfhdr actually points to a valid FHDR
**	    page; if not, we will attempt to find a valid fhdr page in the
**	    table, and then update the iirelation relfhdr accordingly.  Only
**	    if no valid FHDR can be found, will dm1u_repaid be called to
**	    reinitialize an FHDR.
*/
DB_STATUS
dm1u_verify(
DMP_RCB		*rcb, 		/* record control block for open table */
DML_XCB		*xcb,		/* transaction control block */
i4		type,		/* Verify operation & modifier. */
DB_ERROR	*dberr)
{
    EX_CONTEXT	ex;
    STATUS	ret_val;
    DB_STATUS	status = E_DB_OK;
    DMP_TCB	*t = rcb->rcb_tcb_ptr;
    DMP_DCB	*dcb = t->tcb_dcb_ptr;
    i4		error;
    i4		i;
    i4		fhdr_pageno;
    DM1U_ADF	*adf;
    DM1U_CB	*dm1u;
    DMP_MISC	*misc_cb;
    char	msg_buf[DM1U_MSGBUF_SIZE+1];
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*	Initialize things touched by exception cleanup. */

    ret_val = OK;

    status = dm0m_allocate( sizeof(DMP_MISC) + sizeof(DM1U_CB),
	    DM0M_ZERO, (i4) MISC_CB, 
	    (i4) MISC_ASCII_ID, (char *) 0, 
	    (DM_OBJECT **) &misc_cb, dberr);
    if ( status != E_DB_OK )
    {
	dm1u_log_error(dberr);
	return (status);
    }
    dm1u = (DM1U_CB *)(&misc_cb[1]);
    misc_cb->misc_data = (char *)dm1u;

    dm1u->dm1u_rcb = rcb;

    /* verbose flag may be set in type.  If so, strip it off and put it into
    ** the dm1u_cb by setting the DM1U_VERBOSE bit in dm1u_cb.dm1u_flags 
    */
    if (type & DM1U_VERBOSE_FL)
    {
	type &= ~DM1U_VERBOSE_FL;	/* strip off this bit */
	dm1u->dm1u_flags = DM1U_VERBOSE;
    }

#ifdef xDEBUG
    if (DMZ_TBL_MACRO(2))
    {
	TRdisplay("DM1U_VERIFY: Table (%d,%d) (%~t,%~t) OP=%w, MOD=%v\n",
	      t->tcb_rel.reltid.db_tab_base,t->tcb_rel.reltid.db_tab_index,
	      sizeof(t->tcb_rel.relid), &t->tcb_rel.relid,
	      sizeof(t->tcb_rel.relowner), &t->tcb_rel.relowner,
	      ",CHECK,REBUILD,DEBUG,PATCH,FPATCH", type & DM1U_OPMASK,
	      "BITMAP,LINK,RECORD,ATTRIBUTE", type >> 4);
    }
#endif

    /* 
    **	now initialize the ADF information needed for calls to ADC_VALCHK() 
    **	Get the information from the tcb's copies of the iirelation and
    **  iiattribute tuples to build DVs for each attribute
    **  The dm1u CB is initialized to zero, so no need to MEfill the ADF_CB.
    */

    adf = &dm1u->dm1u_adf;
    adf->dm1u_cbadf.adf_errcb.ad_ebuflen = DM1U_MSGBUF_SIZE;
    adf->dm1u_cbadf.adf_errcb.ad_errmsgp = msg_buf;

    adf->dm1u_cbadf.adf_maxstring = DB_MAXSTRING;
    adf->dm1u_cbadf.adf_collation = rcb->rcb_collation;
    adf->dm1u_cbadf.adf_ucollation = rcb->rcb_ucollation;
    adf->dm1u_cbadf.adf_uninorm_flag = 0;

    if (CMischarset_utf8())
      adf->dm1u_cbadf.adf_utf8_flag = AD_UTF8_ENABLED;
    else
      adf->dm1u_cbadf.adf_utf8_flag = 0;

    ret_val = TMtz_init(&adf->dm1u_cbadf.adf_tzcb);
    if( ret_val != OK)
    {
	SETDBERR(dberr, 0, ret_val);

	dm1u_log_error(dberr);
	dm0m_deallocate((DM_OBJECT **)&misc_cb);
	return( E_DB_ERROR);
    }
    tmtz_cb = adf->dm1u_cbadf.adf_tzcb;
    adf->dm1u_numdv = t->tcb_rel.relatts;

    /*
    ** initialize a DV for each attribute. On machines which permit unaligned
    ** access, we may point the DB_DATA_VALUE directly into the tuple buffer,
    ** but on alignment sensitive machines we must correctly align each data
    ** value before we pass it to ADF, so we use a separate buffer.
    */

    for (i = 0; i < adf->dm1u_numdv; i++)
    {
	adf->dm1u_dv[i].db_length =
	    t->tcb_atts_ptr[i+1].length;
	adf->dm1u_dv[i].db_datatype =
	    (DB_DT_ID) t->tcb_atts_ptr[i+1].type;
	adf->dm1u_dv[i].db_prec =
	    t->tcb_atts_ptr[i+1].precision;
	adf->dm1u_dv[i].db_data = 
#ifdef BYTE_ALIGN
	    adf->dm1u_colbuf.colbuf;
#else
	    (PTR) &adf->dm1u_record[t->tcb_atts_ptr[i+1].offset];
#endif
	adf->dm1u_dv[i].db_collID =
	    t->tcb_atts_ptr[i+1].collID;
    }    

    if (t->tcb_data_rac.compression_type != TCB_C_NONE)
    {
        if (rcb->rcb_tupbuf == NULL)
            rcb->rcb_tupbuf = dm0m_tballoc( (t->tcb_rel.relwid +
					t->tcb_data_rac.worstcase_expansion));
        if ((dm1u->dm1u_tupbuf = rcb->rcb_tupbuf) == NULL)
        {
	    SETDBERR(dberr, 0, E_DM923D_DM0M_NOMORE);
	    dm1u_log_error(dberr);
	    dm0m_deallocate((DM_OBJECT **)&misc_cb);
	    return (E_DB_ERROR);
	}
    }
    else
	dm1u->dm1u_tupbuf = NULL;

    /*	Handle main logic with exception handler. */

    if (EXdeclare(handler, &ex) == OK)
    {
	dm1u->dm1u_op = type;
	switch (type & DM1U_OPMASK)
	{
	case DM1U_REPAIR:
	    uleFormat( NULL, E_DM910A_DM1U_VERIFY_INFO, (CL_ERR_DESC *)NULL,  ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code,
			8,
			sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
			sizeof(DB_DB_NAME), 
			t->tcb_dcb_ptr->dcb_name.db_db_name,
			0, type);

	    if (t->tcb_rel.relstat & TCB_IS_PARTITIONED) {

		fhdr_pageno = DM_TBL_INVALID_FHDR_PAGENO;
		status = E_DB_OK;
	    }
	    else
	    {
		status = find_fhdr(dm1u, &fhdr_pageno, dberr);
		if (status == E_DB_OK)
		    status = patch_relation(dm1u, fhdr_pageno, dberr);
		if (status == E_DB_INFO)
		    status = E_DB_OK;
	    }
	    break;

	case DM1U_VERIFY:
	    uleFormat( NULL, E_DM910A_DM1U_VERIFY_INFO, (CL_ERR_DESC *)NULL,  ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code,
			8,
			sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
			sizeof(DB_DB_NAME), 
			t->tcb_dcb_ptr->dcb_name.db_db_name,
			0, type);
	    /* We'll go through all the partitions as well, so ignore
	    ** a request to check the master table (kibro01) b117215
	    */
	    if (!(t->tcb_rel.relstat & TCB_IS_PARTITIONED))
	        status = check(dm1u, dberr);
	    break;

	case DM1U_PATCH:
	case DM1U_FPATCH:
	    uleFormat( NULL, E_DM910A_DM1U_VERIFY_INFO, (CL_ERR_DESC *)NULL,  ULE_LOG, 
			NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code,
			8,
			sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
			sizeof(DB_DB_NAME), 
			t->tcb_dcb_ptr->dcb_name.db_db_name,
			0, type);
	    if (t->tcb_rel.relstat & TCB_IS_PARTITIONED)
	    {
		fhdr_pageno = DM_TBL_INVALID_FHDR_PAGENO;
		status = E_DB_OK;
	    }
	    else
	        status = patch_physical(dm1u, &fhdr_pageno, dberr);
	    if (status == E_DB_OK)
	        status = patch_relation(dm1u, fhdr_pageno, dberr);
	    break;

	case DM1U_DEBUG:
	    if (type & DM1U_BITMAP)
		status = dm1p_dump(rcb, DM1P_D_BITMAP, dberr);
	    if ((status == E_DB_OK) && (type & DM1U_LINK))
		status = dm1p_dump(rcb, DM1P_D_PAGETYPE, dberr);
	    break;
	}
    }

    /*	Delete exception handler. */

    EXdelete();

    dm0m_deallocate((DM_OBJECT **)&misc_cb);

    return (status);
}

/*{
** Name: patch	- patch a table to heap (fixing problems as it goes)
**
** Description:
**      Patch exists to "fix" a broken table by reading each page from
**	the file, validating data pages and throwing away all non-data pages.
**	It converts the table to a heap.
**
**	It assumes that the table is open, and that no pages are FIXED in the
**	table.  It will fix and unfix pages as it goes.  It will never have
**	more than a single page fixed at a time.  Every page in the file
**	will be fixed precisely once.
**
**	Note that we trust the page stat bits to tell us which pages in the
**	table are non-data and which are data. If the page stat bits are wrong,
**	we may make a mistake: If the page stat bits tell us that a non-data
**	page is actually a data page, then we will attempt to validate the
**	page as a data page. The validation routines will typically reject the
**	page as structurally unsound (correctly), and we will throw the page
**	away. Thus it is "safe" for the page stat bits to be wrong in this case.
**
**	If, however, the page stat bits tell us that a data page is actually
**	a NON-datapage, then we will throw the page away. This is the "unsafe"
**	manner in which the page stat bits are wrong.
**
** Inputs:
**	dm1u			DM1U_CB patching control block
**	fhdr_pageno		Pointer to place to return FHDR page number
**	err_code		Pointer to returned error code if error
**
** Outputs:
**	Updates caller's fhdr_pageno with (new?) FHDR location.
**
**	Returns:
**	    NONE.
**	    It calls TALK, which will generate an exception if an error
**	    occurs and relinquish control back to dm1u_table's exception
**	    declaration/management section.
**
** Side Effects:
**	    the pages in the file will be modified.
**
** History:
**      31-Aug-89 (teg)
**          initial creation.
**	13-jun-1990 (teresa)
**	    add support for user interrupts by calling fix_page to check
**	    for user interrupts then call dm0p_fix_page.  It used to call
**	    dm0p_fix_page directly.
**	10-oct-90 (teresa)
**	    Fix bug where page_stat was overwritten, losing DMPP_PATCHED that
**	    may have been set by previous processing.
**	15-mar-1991 (Derek)
**	    Modified as part of the re-organization.
**	29-may-1992 (bryanp)
**	    B44465: Btree free-list pages do not need to be patched.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	30-feb-1993 (rmuth)
**	    Once we have rebuilt the table as a heap make sure that the
**	    FHDR/FMAP are ok.
**	23-July-1993 (rmuth)
**	    If possible try and use the FHDR/FMAP(s) to determine the
**	    end-of-table. If this fails resort to sense file.
**      08-Feb-1996 (mckba01)
**          b74050, removed UNIQUE id from relstat field when updating
**          iirelation to reflect changes to patched table.
**	06-mar-1996 (stial01 for bryanp)
**	    When reformatting pages, pass tcb_rel.relpgsize as the page_size
**		argument to dmpp_format.
**	06-mar-1996 (stial01 for bryanp)
**	    Don't allocate either the DM1U_CB or the DM1U_ADF control block on
**		the stack. These control blocks are very large and must be
**		dynamically allocated. (Actually, in this routine an unneeded
**		DM1U_CB argument was removed).
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Change page header references to use macros.
**	01-apr-2003 (devjo01) b109956
**	    Fix table patch algorithm, to prevent data corruption
**	    when resultant table is used for inserts.
**	11-Jun-2007 (kibro01) b117215
**	    Change name of function from patch() to patch_physical() to
**	    reflect the fact that it is now merely half of the original.  This
**	    function only performs the actions on the table, not the 
**	    description of the table in iirelation.  This is so that both
**	    can be called for normal tables and partitions of partitioned 
**	    tables, but this is NOT called for the master table of a partitioned
**	    table.
**	27-Jun-2007 (kschendel) b118588
**	    Need to pass the fhdr back to caller for patching into iirelation.
**	
[@history_template@]...
*/
static DB_STATUS
patch_physical(
DM1U_CB		   *dm1u,
i4		   *fhdr_pageno,
DB_ERROR	   *dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DML_XCB	    *xcb = rcb->rcb_xcb_ptr;
    DB_STATUS	    status;
    i4	    page_number;
    i4	    last_used_page_number;
    i4	    last_data_page_number;
    i4	    last_alloc_page_number;
    i4	    badcount=0,error;
    i4	    mask;
    DMPP_PAGE	    *page;
    bool	    using_end_of_file = FALSE;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /*
    ** Determine the end-of-file
    */
    status = dm2f_sense_file( 
	    t->tcb_table_io.tbio_location_array,
	    t->tcb_table_io.tbio_loc_count, 
	    &(t->tcb_rel.relid),
	    &(t->tcb_dcb_ptr->dcb_name), 
	    &last_alloc_page_number,
	    dberr);
    if (status != E_DB_OK)
    {
	/*
	** Cannot determine where the end-of-file is so bail
	*/
	dm1u_log_error(dberr);
	return (status);
    }

    /*
    ** Try and determine the end-of-table using the FHDR/FMAP(s)
    */
    status = dm1p_checkhwm( rcb, dberr);

    if (status != E_DB_OK)
    {
	/*  
	** FHDR/FMAP corrupt so use sense file. This returns end-of-file
	** so we have to add some special checking to look for the 
	** highwater-mark.
	** 
	*/
	last_used_page_number = last_alloc_page_number;
	using_end_of_file = TRUE;
    }
    else
    {
	last_used_page_number = t->tcb_table_io.tbio_lpageno;
    }

    mask = DMPP_DIRECT | DMPP_INDEX | DMPP_LEAF | DMPP_CHAIN |
	   DMPP_FHDR | DMPP_FMAP;
    rcb->rcb_state |= RCB_READAHEAD;

    /*
    ** Scan whole table looking for data pages to
    ** link into a heap structure
    */
    last_data_page_number = 0;
    for (page_number = 0; page_number <= last_used_page_number; page_number++)
    {
	status = dm0p_fix_page( rcb, (DM_PAGENO) page_number, 
	    		        DM0P_WRITE, &rcb->rcb_data, dberr);
	if (status != E_DB_OK)
	{
	    /*
	    ** If we have hit the tables highwater mark then stop scanning
	    ** table as all remaining pages are unformatted
	    */
	    if ( (dberr->err_code == E_DM9206_BM_BAD_PAGE_NUMBER) && 
		 (using_end_of_file ) )
	    {
		page_number--;
		break;
	    }

	    /*
	    ** We will allow at most half the table to be bad
	    */
	    if (badcount < (last_used_page_number + 3) / 2)
	    {
		badcount++;
		continue;
	    }
	    else
	    {
		dm1u_talk(dm1u,W_DM51FD_TOO_MANY_BAD_PAGES, 0);
		SETDBERR(dberr, 0, E_DM9104_ERR_CK_PATCH_TBL);
		return (E_DB_ERROR);
	    }
	} /* endif bad page read */

	page = rcb->rcb_data.page;

	if  (( DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) != 0 ) &&
	    (( DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & 
		DMPP_FREE ) == 0) &&
	    ((!(DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & 
		mask)) || 
		(DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & 
		DMPP_DATA) ) )
	{
	    /* 
	    ** We have a data page, check the data page, chain it to
	    ** the other pages, then unfix it.
	    */
	    data_page (dm1u, page, dberr);
	    last_data_page_number = page_number;
	}
	else 
	{
	    if ( DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page) == 0 )
	    {
		/*
		** Page zero in a HEAP table must always be a data page
		*/
		(*t->tcb_acc_plv->dmpp_format)(t->tcb_rel.relpgtype, 
			t->tcb_rel.relpgsize, page, (DM_PAGENO) page_number,
			DMPP_DATA, DM1C_ZERO_FILL);
	    }
	    else
	    {
		/*
		** Format all other pages as FREE pages but do not link
		** them into the heap structure
		*/
		dm1u_talk( dm1u,I_DM53FD_NOT_DATA_PAGE, 2, sizeof(page_number),
		      &page_number);

		(*t->tcb_acc_plv->dmpp_format)(t->tcb_rel.relpgtype,
			t->tcb_rel.relpgsize, page, (DM_PAGENO) page_number,
			(DMPP_MODIFY | DMPP_FREE), DM1C_ZERO_FILL);

		status = dm0p_unfix_page( rcb, DM0P_UNFIX, 
					  &rcb->rcb_data,
					   dberr);
		if ( status != E_DB_OK )
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error,0);
		    dm1u_talk( dm1u, W_DM51FC_ERROR_PUTTING_PAGE, 2, 
			  sizeof(page_number), &page_number);
		    return( status );
		}

		continue;
	    }
	}

	DMPP_VPT_SET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
	    rcb->rcb_data.page, 0);
	DMPP_VPT_SET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,
	    rcb->rcb_data.page, 0);
	DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
	    rcb->rcb_data.page, DMPP_MODIFY);

	if (rcb->rcb_other.page)
	{
	    DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
		rcb->rcb_other.page, DMPP_MODIFY);
	    DMPP_VPT_SET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,
		rcb->rcb_other.page, page_number);
	    status = dm0p_unfix_page(rcb, DM0P_UNFIX, &rcb->rcb_other,
		dberr);
	    if (status == E_DB_ERROR)
	    {
		/* an error unfixing a fixed page may be more serious than it
		** is being treated here.  For now, print an error and generate
		** an exception.
		*/
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
		dm1u_talk( dm1u,W_DM51FC_ERROR_PUTTING_PAGE, 2, 
			   sizeof(page_number),
			    &page_number);
		return (E_DB_ERROR);
	    }
	}
	rcb->rcb_other.page = rcb->rcb_data.page;
	rcb->rcb_data.page = NULL;
    }


    if (rcb->rcb_other.page)
    {
	status = dm0p_unfix_page(rcb, DM0P_UNFIX, &rcb->rcb_other, dberr);
	if (status == E_DB_ERROR)
	{
	    /* an error unfixing a fixed page may be more serrious than it
	    ** is being treated here.  For now, print an error and generate
	    ** an exception.
	    */
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, NULL, 0, NULL, &error, 0);
	    dm1u_talk(dm1u,W_DM51FC_ERROR_PUTTING_PAGE, 2, sizeof(page_number),
			&page_number);
	    return (E_DB_ERROR);
	}
    }

    /*
    ** Rebuild the tables FHDR/FMAP pages
    */
    *fhdr_pageno =  DM_TBL_INVALID_FHDR_PAGENO;
    status = dm1p_rebuild( rcb, last_alloc_page_number, last_data_page_number,
			   fhdr_pageno, dberr );
    if ( status != E_DB_OK)
	return(E_DB_ERROR);
    return (status);
}


/*{
** Name: patch_relation	- patch a table to heap (iirelation amendments)
**
** Description:
**      Patch exists to "fix" a broken table by reading each page from
**	the file, validating data pages and throwing away all non-data pages.
**	It converts the table to a heap.
**
**	It assumes that the table is open, and that no pages are FIXED in the
**	table.  It will fix and unfix pages as it goes.  It will never have
**	more than a single page fixed at a time.  Every page in the file
**	will be fixed precisely once.
**
**	Note that we trust the page stat bits to tell us which pages in the
**	table are non-data and which are data. If the page stat bits are wrong,
**	we may make a mistake: If the page stat bits tell us that a non-data
**	page is actually a data page, then we will attempt to validate the
**	page as a data page. The validation routines will typically reject the
**	page as structurally unsound (correctly), and we will throw the page
**	away. Thus it is "safe" for the page stat bits to be wrong in this case.
**
**	If, however, the page stat bits tell us that a data page is actually
**	a NON-datapage, then we will throw the page away. This is the "unsafe"
**	manner in which the page stat bits are wrong.
**
** Inputs:
**      dm1u		Modify control block
**	fhdr_pageno	Page number of fhdr from physical patching
**
** Outputs:
**	err_code	Error code
**
**	Returns:
**	    status
**
** Side Effects:
**	    the iirelation table for the table will be modified to describe
**	    the table as a heap.
**
** History:
**      11-Jun-2007 (kibro01) b117215
**	    Initial creation to separate out the parts of the original
**	    patch() function which apply to the master table of a partitioned
**	    table.
**	27-Jun-2007 (kschendel) b118588
**	    Caller will supply proper fhdr page number (if applicable).
**	29-Jun-2007 (jgramling) SIR 122890
**	    Add changes to update only relfhdr, if the verify operation is 
**	    DM1U_REPAIR (find and update valid FHDR).
**	    (kschendel) fix wildly broken indentation.
*/

static DB_STATUS
patch_relation(
DM1U_CB		   *dm1u,
i4		   fhdr_pageno,
DB_ERROR	   *dberr)
{
    DMP_RCB	*rcb = dm1u->dm1u_rcb;
    DB_STATUS	status;
    DML_XCB	*xcb = rcb->rcb_xcb_ptr;
    DB_TAB_ID	tab_id;
    DMP_RCB	*rel_rcb = (DMP_RCB *) 0;
    DMP_TCB	*t;
    DMP_DCB	*dcb;
    i4		error=0;
    DM2R_KEY_DESC	key_desc;
    DB_TAB_TIMESTAMP	timestamp;
    DMP_RELATION	relrecord;	  /* holds the contents of 1 iirelation tuple */
    DM_TID	tid;
    i4		loc_stat;
    DB_ERROR	local_dberr;
    i4		*err_code = &dberr->err_code;

    CLRDBERR(dberr);

    t = (DMP_TCB *) rcb->rcb_tcb_ptr; 
    dcb = (DMP_DCB *) t->tcb_dcb_ptr;

    tab_id.db_tab_base = DM_B_RELATION_TAB_ID;
    tab_id.db_tab_index = DM_I_RELATION_TAB_ID;

    status = dm2t_open(  (DMP_DCB *)xcb->xcb_odcb_ptr->odcb_dcb_ptr,
		&tab_id, DM2T_IX, DM2T_UDIRECT, DM2T_A_WRITE, (i4)0, 
		(i4)20, xcb->xcb_sp_id, xcb->xcb_log_id, xcb->xcb_lk_id, 
		(i4)0, (i4)0, DM2T_X, &xcb->xcb_tran_id, 
		&timestamp, &rel_rcb, (DML_SCB *)NULL, dberr);
    if (status)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL,
			NULL, 0, NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9026_REL_UPDATE_ERR);
	return (E_DB_ERROR);
    }

    /*  Position table using base table id of table being patched. */

    tab_id.db_tab_base = t->tcb_rel.reltid.db_tab_base;
    tab_id.db_tab_index = t->tcb_rel.reltid.db_tab_index;
    key_desc.attr_operator = DM2R_EQ;
    key_desc.attr_number = 1;
    key_desc.attr_value = (char *) &tab_id.db_tab_base; 

    status = dm2r_position(rel_rcb, DM2R_QUAL, &key_desc, (i4)1,
                          (DM_TID *)NULL, dberr);
    if (status != E_DB_OK)
    {
	uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9026_REL_UPDATE_ERR);
	return (E_DB_ERROR);
    }
    else
    {
	/* 
	** we have opened the iirelation table and have set up to start
	** search for the desired tuple successfully.
	*/

	for (;;)
	{
	    /* 
	    ** Get a qualifying relation record.  This will retrieve
	    ** all records for a base table (i.e. the base and all
	    ** indices), or just one for an index. 
	    */
	    *err_code = E_DM9026_REL_UPDATE_ERR;
	    status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT, 
		    (char *)&relrecord, dberr);
	    if (status != E_DB_OK)
	    {
		if (dberr->err_code == E_DM0055_NONEXT)
		    break;
		uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, 
				NULL, 0, NULL, &error,0);
		SETDBERR(dberr, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    /* 
	    ** If this isnt exactly the tuple we are searching for (such 
	    ** as a different index table than the one we want), skip it 
	    */
	    if (relrecord.reltid.db_tab_index != tab_id.db_tab_index)
		continue;

	    /* 
	    ** this condition should never be met -- dm2r_get should always
	    ** return a tuple with the db_tab_base that it was asked to 
	    ** match. However, we check for it anyhow (in case of 
	    ** corruption) 
	    */
	    if (relrecord.reltid.db_tab_base != tab_id.db_tab_base)
		continue;

	    /*
	    ** Located the correct tuple :
	    **     - Update the record to indicate a heap.  
	    **     - Update the timestamp just in case there are any
	    **  	 query plans laying around.
	    **     - Update the tables relfhdr field as the FHDR may
	    **	 have moved.
	    ** if the verify operation is fhdr_patch, then just update
	    ** relfhdr; for now, any other operation will mark the 
	    ** table as heap
	    */
	    if ((dm1u->dm1u_op & DM1U_OPMASK) != DM1U_REPAIR)
	    {
		relrecord.relprim = 1;
		relrecord.relmain = 1;
		relrecord.relifill = 0;
		relrecord.reldfill = DM_F_HEAP;
		relrecord.rellfill = 0;
		relrecord.relspec = TCB_HEAP;
		relrecord.relkeys = 0;
		relrecord.relstat = (relrecord.relstat & ~TCB_UNIQUE);
		TMget_stamp((TM_STAMP *)&relrecord.relstamp12);
	    }
	    relrecord.relfhdr = fhdr_pageno;
	    status = dm2r_replace(rel_rcb, &tid, 
		    DM2R_BYPOSITION, (char*)&relrecord, (char *)NULL, dberr);
	    if (status != E_DB_OK )
	    {
		uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				NULL, 0, NULL, &error,0);
		SETDBERR(dberr, 0, E_DM9026_REL_UPDATE_ERR);
		break;
	    }

	    if ((dm1u->dm1u_op & DM1U_OPMASK) != DM1U_REPAIR)
	    {
		/* Not clear why this is done, the caller (dm2umod) will
		** close-purge the TCB, and we've left other stuff unchanged...
		*/
		t->tcb_rel.relspec = TCB_HEAP;
	    }

	    break;

	} /* end loop searching for iirelation tuple to update */

    }  /* endif iirelation was opened and postioned successfully */

    dm1u_talk(dm1u,I_DM53FC_PATCH_DONE, 0);
    /* 
    ** close the open iirelation table.  If we hit an error closing it,
    ** report that error and signal an exception.  OTherwise, if we hit 
    ** any errors during processing, signal an exception.  
    ** If not, return via normal mechanisms.
    */

    if (rel_rcb)
    {
	loc_stat = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
	if (loc_stat != E_DB_OK && status == E_DB_OK )
	{
	    *dberr = local_dberr;
	    status = loc_stat;
	}
	rel_rcb = NULL;
    }
    return (status);
}

/*{
** Name: find_fhdr	- scan a table looking for the FHDR page, as part of
**                        the fhdr_patch operation
**
** Description:
**
**	Derived from patch_physical() to scan a table, looking for its FHDR.
**      This is designed to fix an invalid relfhdr, when there is reasonable
**      certainty that a table's files have not been physically corrupted. In 
**	particular, it can be used when a table's underlying file(s) have been
**	copied and renamed from another table with an identical structure, but
**	leaving therefore the dynamic catalog information out of sync.
**
**      The routine will validate the open tcb's FHDR to see if the relfhdr
**	is currently valid before searching the file.  If no valid FHDR page
**	is found by scanning, it will return an error to the caller.  (The
**	caller can try inventing a new FHDR.)
**
**      If an FHDR page is believed to have been found, we will validate the
**      FHDR/FMAP before updating the table's relfhdr.
**
**	This function assumes that the table is open, and that no pages are
**	FIXED in the table.  It will fix and unfix pages as it goes.  It
**	will never have more than a single page fixed at a time.
**
** Inputs:
**	dm1u			DM1U_CB patching control block
**	fhdr_pageno		Pointer to place to return FHDR page number
**	err_code		Pointer to returned error code if error
**
** Outputs:
**	Updates caller's fhdr_pageno with FHDR location.
**
**	Returns:
**	    NONE.
**	    It calls TALK, which will generate an exception if an error
**	    occurs and relinquish control back to dm1u_table's exception
**	    declaration/management section.
**
** Side Effects:
**	    none.
**
** History:
**      28-jun-2007 (jgramling) SIR 122890
**          initial creation from patch_physical().
*/
static DB_STATUS
find_fhdr(
DM1U_CB		   *dm1u,
i4		   *fhdr_pageno,
DB_ERROR	   *dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DML_XCB	    *xcb = rcb->rcb_xcb_ptr;
    DB_STATUS	    status;
    i4	    page_number;
    i4	    last_used_page_number;
    i4	    fhdr_page_number;
    i4	    last_alloc_page_number;
    i4	    error;
    DMPP_PAGE	    *page;
    bool	    using_end_of_file = FALSE;

    CLRDBERR(dberr);

    /*
    ** Determine the end-of-file
    */
    status = dm2f_sense_file( 
	    t->tcb_table_io.tbio_location_array,
	    t->tcb_table_io.tbio_loc_count, 
	    &(t->tcb_rel.relid),
	    &(t->tcb_dcb_ptr->dcb_name), 
	    &last_alloc_page_number,
	    dberr);
    if (status != E_DB_OK)
    {
	/*
	** Cannot determine where the end-of-file is so bail
	*/
	dm1u_log_error(dberr);
	return (status);
    }

    /*
    ** check the current FHDR/FMAP; nothing to fix if it's OK 
    */
    status = dm1p_checkhwm( rcb, dberr);
    if (status == E_DB_OK)
    {
	/* 
	** The current value of iirelation's relfhdr correctly 
	** reflects the page# of the FHDR. Also, the
	** FHDR/FMAP is valid, so nothing needs to be patched.
	**
	** Return E_DB_INFO, which will be transformed into E_DB_OK
	** by caller.
	*/

	return (E_DB_INFO);
     }

     if  (status == E_DB_WARN && dberr->err_code==E_DM92DD_DM1P_FIX_HEADER) {

	/*  
	** The FHDR was not where expected (according to the tcb's relfhdr)
	**  so we will scan the table file looking for it.
	*/
	last_used_page_number = last_alloc_page_number;
	using_end_of_file = TRUE;
	CLRDBERR(dberr);
    }
    else
    {
	/*  
	** The FHDR was where expected, but something else went wrong
	** (maybe it points off the end of file or something).  We
	** won't do anything about it here; let's just pass the error
	** back to caller: This is a job for "dm1p_repair", which
	** should be able to rebuild the FHDR/FMAP if the table isn't
	** too damaged.
	*/

	dm1u_talk(dm1u, W_DM51FD_TOO_MANY_BAD_PAGES, 0); 
	SETDBERR(dberr, 0, E_DM9104_ERR_CK_PATCH_TBL);
	return (E_DB_ERROR);

    }

    rcb->rcb_state |= RCB_READAHEAD;

    /*
    ** Scan whole table looking for the FHDR page
    */
    fhdr_page_number = 0;
    rcb->rcb_data.page = NULL;
    for (page_number = 0; page_number <= last_used_page_number; page_number++)
    {

        status = fix_page(dm1u, (DM_PAGENO) page_number,
            DM0P_READ | DM0P_READAHEAD, &rcb->rcb_data, dberr);

	if (status != E_DB_OK)
	{
	    /*
	    ** We have hit the highwater mark or unformatted pages,
	    ** with no FHDR.  So we give up and go home.
	    */

	    dm1u_talk(dm1u, W_DM5065_NO_FHDR_FOUND, 0); 
	    SETDBERR(dberr, 0, E_DM9104_ERR_CK_PATCH_TBL);
	    return (E_DB_ERROR);

	} /* endif bad page read */

	page = rcb->rcb_data.page;

        if ( (DM1P_VPT_GET_FHDR_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & DMPP_FHDR) )
        {
	    /* 
	    ** We have found an FHDR page.
	    */

	    fhdr_page_number = page_number;
	    break;
	}

	unfix_page(dm1u, &rcb->rcb_data);

	rcb->rcb_data.page = NULL;
    }


    if (rcb->rcb_data.page)
    {
	unfix_page(dm1u, &rcb->rcb_data);

    }

    *fhdr_pageno =  fhdr_page_number;

    return (status);

} /* find_fhdr */

/*{
** Name: dm1u_repair	- Repair a table.
**
** Description:
**	This routine tries to repair an existing table without converting
**      the table to a HEAP structure.
**
**	Currently the only part of a table that we try and repair in this
**      fashion are the FHDR/FMAP(s) structures.
**
** Inputs:
**	dm1u				dm1u control block.
**	operation			operation code.
** Outputs:
**	none.
**
** Side Effects:
**	    the pages in the file will be modified.
**
** History:
**      07-mar-1993 (rmuth)
**	    Created.
*/
static DB_STATUS
dm1u_repair(
    DM1U_CB	    *dm1u,
    DB_ERROR	    *dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB         *t = rcb->rcb_tcb_ptr;
    i4	    size;
    i4	    i;
    DB_STATUS	    status = E_DB_OK;

    CLRDBERR(dberr);

    if (dm1u->dm1u_op & DM1U_BITMAP)
	status = dm1p_repair( rcb, dm1u, dberr);
   
    if ( status != E_DB_OK )
	dm1u_log_error( dberr );

    return( status );

}

/*{
** Name: check	- Check/Rebuild a table.
**
** Description:
**	Check exists to "check" that a tables structure, records, attributes
**	and allocation bitmaps are correct.  The bitmaps can be rebuilt if
**	damaged.
**
**	The level of checking performed can be varied.
**	    BITMAPS	can be checked independent of others.
**	    LINKS	can be checked independent of others.
**	    RECORDS	assume that LINKS are being checked.
**	    ATTRIBUTES  assumes that RECORDS are being checked.
**
**      The following is a brief description of the type of checking performed
**	for each level:
**	    BITMAPS	Check that the bitmaps are readable.
**			Check that the page status bits correspond to the bitmap
**	    LINKS	Check that pages are logically correctly linked.
**			This can also mean checking an index.
**	    RECORDS	Check that all records on the page are correct.
**			Check that all keys in an index are correct.
**	    ATTRIBUTES	Check that all values in a record are correct.
**			Check that all values in a key are correct.
** Inputs:
**      rcb                             record control block.
**	xcb				transaction control block.
**	operation			operation code.
** Outputs:
**	none.
**
**	Returns:
**	    NONE.
**	    It calls TALK, which will generate an exception if an error
**	    occurs and relinquish control back to dm1u_table's exception
**	    declaration/management section.
**
** Side Effects:
**	    the pages in the file will be modified.
**
** History:
**      15-mar-1991 (Derek)
**	    Created.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	08-feb-1993 (rmuth)
**	      - Use dm1p_checkhwm and tcb_lpageno to determine the end of
**	        the table as dm2f_sense_file returns the end-of-file which
**	        is not the same as end-of-table.
**	      - Restructure so that when we encounter an error we 
**	        do not bother continuing.
**	      - Restructure to cater for better error checking
**	      - dm1p_verify no longer takes a flag parameter.
**	30-mar-1993 (rmuth)
**	    This routine uses bitmaps to track pages that have already 
**	    been visited. The bitmap size is dependent on the table size.
**	    The code was allocating the memory before it had found out
**	    the table size this has now been fixed.
**	21-jun-1993 (rogerk)
**	    Fix compile warnings.
**	13-apr-99 (stephenb)
**	    Add code to check peripherals
*/
static DB_STATUS
check(
DM1U_CB		    *dm1u,
DB_ERROR	    *dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB         *t = rcb->rcb_tcb_ptr;
    i4	    size;
    i4	    i;
    DB_STATUS	    status;
    DB_ERROR	    local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    do
    {

	rcb->rcb_state |= RCB_READAHEAD;

	/*
	** If specfied check the FHDR/FMAP(s)
	*/
	if (dm1u->dm1u_op & DM1U_BITMAP)
	{
	    /*
	    ** Verify the FHDR and FMAP(s) for a table a return
	    ** code of E_DB_INFO means that inconsistency occurred
	    ** between the FMAP(s) and pages on disc. I.e. a used
	    ** page is marked as free in the FMAP.
	    */
	    status = dm1p_verify( rcb, dm1u, dberr );
	    if ( status != E_DB_OK )
	    {
		if ( status == E_DB_INFO )
		{
		    dm1u->dm1u_e_cnt++;
		    status = E_DB_OK;
		}
		else
		{
		    if ( status == E_DB_WARN )
		    {
			dm1u->dm1u_e_cnt++;
			status = E_DB_OK;
		    }
		    break;
		}
	    }
	}

	/*
	** Find out how many pages in the table so we can allocate
	** the bitmaps.
	*/
	status = dm1p_checkhwm( rcb, dberr);
	if (status != E_DB_OK)
	    break;

	dm1u->dm1u_lastpage = t->tcb_table_io.tbio_lpageno;

	/*	
	** Allocate Map and Chain bitmaps, if we fail allocating this 
	** space then just bail as we cannot continue
	*/
        dm1u->dm1u_xchain = 0;
	dm1u->dm1u_xmap = 0;

	size = sizeof(DMP_MISC) + ((dm1u->dm1u_lastpage + 1) / 8) + 1;
	status = dm0m_allocate( size, DM0M_ZERO, (i4) MISC_CB, 
	    (i4) MISC_ASCII_ID, (char *) 0, 
	    (DM_OBJECT **) &dm1u->dm1u_xmap, dberr);
	if ( status != E_DB_OK )
	    break;

	dm1u->dm1u_map = (u_char *) &dm1u->dm1u_xmap[1];
	dm1u->dm1u_xmap->misc_data = (char *) dm1u->dm1u_map;
	dm1u->dm1u_size = size - sizeof(DMP_MISC);

	status = dm0m_allocate( size, DM0M_ZERO, (i4) MISC_CB, 
	    (i4) MISC_ASCII_ID, (char *) 0, 
	    (DM_OBJECT **) &dm1u->dm1u_xchain, dberr);
	if ( status != E_DB_OK )
	    break;

	dm1u->dm1u_chain =  (u_char *) &dm1u->dm1u_xchain[1];
	dm1u->dm1u_xchain->misc_data = (char *) dm1u->dm1u_chain;


	/*
	** Perform requested operation, if we encounter an unexpected
	** error then just bail
	*/
	if (dm1u->dm1u_op & 
		(DM1U_LINK | DM1U_RECORD | DM1U_ATTRIBUTE | DM1U_KPERPAGE))
	{
	    switch (t->tcb_rel.relspec)
	    {
	    case TCB_HEAP:
		status = heap(dm1u, dberr);
		break;

	    case TCB_HASH:
		status = hash(dm1u, dberr);
		break;

	    case TCB_ISAM:
		status = isam(dm1u, dberr);
		break;

	    case TCB_BTREE:
		dm1u->dm1u_1stleaf = (DM_PAGENO) BTREE_ROOT;
		status = btree(dm1u, dberr);
		break;

	    default:
		dm1u->dm1u_e_cnt++;
		dm1u_talk(dm1u,W_DM51FF_BAD_RELSPEC, 0);
		SETDBERR(dberr, 0, W_DM51FF_BAD_RELSPEC);
		status = E_DB_ERROR;
		break;
	    }
	}
	if (dm1u->dm1u_op & DM1U_PERIPHERAL)
	{
	    status = periph_check(dm1u, dberr);
	}

	/*
	** If we encountered an unexpected error while checking table
	** then just bail as no point it trying to do anything else
	*/
	if ( status != E_DB_OK )
	    break;

    } while (FALSE);

    if ( status != E_DB_OK )
	dm1u_log_error( dberr );
    else
        dm1u_talk(dm1u,I_DM53FB_CHECK_DONE, 0);

    if (dm1u->dm1u_xchain)
        dm0m_deallocate((DM_OBJECT **)&dm1u->dm1u_xchain);

    if (dm1u->dm1u_xmap)
        dm0m_deallocate((DM_OBJECT **)&dm1u->dm1u_xmap);

    return (status);
}

/*{
** Name: heap	- check/rebuild a heap table
**
** Description:
**      A heap table is merely a single main data page which may or may not 
**	have overflow data pages.  This routine calls main_chain once for the
**	main page, which also takes care of checking any overflow pages chained 
**	to that main page.  Then, it calls check_orphan to look for and
**	process any orphan pages in the file.
**
** Inputs:
**	dm1u_cb				Patch/Check Table Control Block
**
** Outputs:
**	err_code			Reason for error status.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** Side Effects:
**
** History:
**      24-oct-89 (teg)
**          initial creation.
**	15-mar-1991 (Derek)
**	    Re-organized to support file allocation.
[@history_template@]...
*/
static DB_STATUS
heap(
DM1U_CB		*dm1u,
DB_ERROR	*dberr)
{
    DB_STATUS	status;

    /*
    **	Since a heap is just a single main page with a chain of overflow
    **	pages, we just use the main+ovfl page chain checking routine to
    **	find each page of the file.
    */

    status = main_chain(dm1u, (DM_PAGENO) 0, (DM_PAGENO) 0, dberr);
    if (status == E_DB_OK)
	status = check_orphan(dm1u, dberr);
    return (status);
}

/*{
** Name: hash	- check/rebuild a hash table
**
** Description:
**      A hash table is merely a series of main data pages (from 0 to
**	iirelation.relmain-1) which may or may not have overflow data 
**	pages.  This routine calls main_chain once for each main page,
**	which also takes care of checking any overflow pages chained to
**	that main page.  Then, it calls check_orphan to look for and
**	process any orphan pages in the file.
**
** Inputs:
**	dm1u_cb				Patch/Check Table Control Block
** Outputs:
**	err_code			Reason for error return status.
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
** Side Effects:
**
** History:
**      24-oct-89 (teg)
**          initial creation.
**	15-mar-1991 (Derek)
**	    Re-organized to support file allocation.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
[@history_template@]...
*/
static DB_STATUS
hash(
DM1U_CB		*dm1u,
DB_ERROR	*dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DM_PAGENO	    i;
    DB_STATUS	    status;
    DB_STATUS	    return_status = E_DB_OK;

    CLRDBERR(dberr);

    if (t->tcb_rel.relmain >= dm1u->dm1u_lastpage)
    {
	i4	    pages = dm1u->dm1u_lastpage + 1;

	/*  IIrelation doesn't match the file we are processing. */

	dm1u_talk(dm1u,E_DM9109_BAD_RELMAIN, 4,
	    sizeof(t->tcb_rel.relmain), &t->tcb_rel.relmain,
	    sizeof(pages), &pages);
	dm1u->dm1u_e_cnt++;
	return (E_DB_ERROR);
    }

    /*
    **	Process each main page bucket as a main+ovfl chain.
    */

    for (i = 0; i < (DM_PAGENO)t->tcb_rel.relmain; i++)
    {
	status = main_chain(dm1u, (DM_PAGENO)i,
	    (i + 1 == t->tcb_rel.relmain) ? 0 :  (DM_PAGENO)i + 1,
	    dberr);
	if (status != E_DB_OK)
	    return_status = status;
    }
    if (return_status != E_DB_OK)
	return (return_status);
    status = check_orphan(dm1u, dberr);
    return (status);
}

/*{
** Name: main_chain	- check data page overflow chains
**
** Description:
**      This routine fixes the specified page, and then calls data_page
**	to check the given data page.  It also verifies that this page is
**	not referenced more than once, and that page_main is correct for this
**	page.  The calling routine determines what page_main should be, based
**	on table type and where in the table this page is located.  Then it 
**	unfixes the page.  If the page happens to have an overflow page, 
**	it remains in a loop until no more overflow pages are encountered.
**
**	If this routine is unable to fix a page, it prints a warning and exits.
**	However, if this routine is unable to unfix a page that it previously 
**	fixed, then it generates an exception and discontinues processing of 
**	this table.
**
** Inputs:
**	dm1u_cb				Check/Patch table control block
**      main_page			number of page to check.
**	exp_page_main			expected value for page_main
**
** Outputs:
**
**	Returns:
**	    none.
**	Exceptions:
**	    calls talk, which will genereate an exception in case of an error.
**	    Also, will generate an exception if unable to unfix a previously 
**		fixed data page.
**
** Side Effects:
**	    one or more data pages will be fixed, then unfixed.  This routine
**	    expects no fixed pages when called, and if any pages are fixed to
**	    rcb->rcb_data.page, the reference to that page will be lost.
**
** History:
**      24-oct-89 (teg)
**          initial creation.
**	13-jun-1990 (teresa)
**	    add support for user interrupts by calling fix_page to check
**	    for user interrupts then call dm0p_fix_page.  It used to call
**	    dm0p_fix_page directly.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		- Change page header references to use macros.
[@history_template@]...
*/
static DB_STATUS
main_chain(
DM1U_CB		*dm1u,
DM_PAGENO	main_page,
DM_PAGENO	exp_page_main,
DB_ERROR	*dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB         *t = rcb->rcb_tcb_ptr;
    i4	    page_number;
    DMPP_PAGE	    *page;
    DB_STATUS	    status;

    CLRDBERR(dberr);

    /*	Check the chain pages bitmap. */

    clean_bmap(dm1u->dm1u_size, (PTR)dm1u->dm1u_chain);

    for (page_number = main_page;;)
    {
	if (CHECK_MAP_MACRO(dm1u, page_number))
	{
	    if (CHECK_CHAIN_MACRO(dm1u, page_number))
	    {
		/*  This page was seen on the current chain. */

		dm1u_talk(dm1u,W_DM500A_CIRCULAR_DATA_CHAIN, 2, sizeof(page_number),
		    &page_number);
	    }
	    else
		dm1u_talk(dm1u,W_DM5009_MERGED_DATA_CHAIN, 2, sizeof(page_number),
		    &page_number);
	    dm1u->dm1u_e_cnt++;
	    return (E_DB_OK);
	}

	status = fix_page(dm1u, (DM_PAGENO) page_number,
	    DM0P_WRITE | DM0P_NOREADAHEAD, &rcb->rcb_data, dberr);
	if (status != E_DB_OK)
	    return (status);
	page = rcb->rcb_data.page;
	if ((DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
	    page) & DMPP_FREE) == 0)
	{
	    /* Mark the page as being processed, and in the current chain. */

	    SET_MAP_MACRO(dm1u, page_number);
	    SET_CHAIN_MACRO(dm1u, page_number);

	    /*  Check that the main page pointer is correct. */

	    if (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,
		page) != exp_page_main)
	    {
		if (exp_page_main != -1 )
		{
		    dm1u->dm1u_e_cnt++;
		    dm1u_talk(dm1u,W_DM500B_WRONG_PAGE_MAIN, 6,
			sizeof(page_number), &page_number,
			sizeof(exp_page_main), &exp_page_main,
		        sizeof(DM_PAGENO),
			DMPP_VPT_ADDR_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, 
			page));
		}
	    }

	    /*  Check contents of data page. */

	    data_page(dm1u, page, dberr);

	    /*  Get overflow page pointer. */

	    page_number = DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, page);
	}
	else
	    page_number = 0;

	/* now unfix the page */

	unfix_page(dm1u, &rcb->rcb_data);

	if (!page_number)
	    return (E_DB_OK);
    }
}

/*{
** Name: isam	- Check/Rebuild ISAM table
**
** Description:
**	this is the executive to check ISAM tables.  An isam table has a root
**	page, which (if all is ok) means that walking the tree will touch all
**	pages. 
**
**	first walk the tree starting at the root page, then go check for any
**	unreferenced pages.
**
** Inputs:
**	dm1u_cb		    check/patch table control block
**	    .dm1u_rcb		Record control block pointer
**		.rcb_tcb_ptr	    Table control block pointer
**		    .relprim		page number of isam root page
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    none.
** Side Effects:
**	    many pages are fixed and unfixed during processing of subordinate
**	    routines.
**
** History:
**      22-feb-90 (teg)
**          initial creation.
[@history_template@]...
*/
static	DB_STATUS
isam(
DM1U_CB		*dm1u,
DB_ERROR	*dberr)
{
    DB_STATUS	    status;

    /*  Start root of the isam index and walk down the index.	*/

    status = isam_index(dm1u, dm1u->dm1u_rcb->rcb_tcb_ptr->tcb_rel.relprim,
	dberr);
    if (status != E_DB_OK)
	return (status);
    status = check_orphan(dm1u, dberr);
    return (status);
}

/*{
** Name: isam_index	- Check ISAM index pages
**
** Description:
**      This is a recursive routine used to check isam index pages.  The
**	isam index page has a few interesting characteristics:
**	    page_main = level of index page (level 0 -> this page references
**					     data pages)
**	    page_ovfl = page number of index/data page containing smallest
**			key on this page.  The 2nd key resides on 
**			page_ovfl+1, the 3rd on page_ovfl+2, etc.
**	If the index page is not a bottom level index page (page_main=0),
**	then it MUST reference other index pages.  All index pages in an
**	isam file are between pages iirelation.relmain to iirelation.relprim.
**	If this is a bottom level index page, it must reference a main data
**	page.  All main data pages in the isam file must have a page number
**	between 0 and iirelation.relmain-1.
**
**	This routine recurses if it is NOT a bottom level index page.  If
**	it is a bottom level index page, it calls routine dm1u_idata to check
**	a main data page.
**
**	HOWEVER, if we are processing an orphan page, we do NOT want to walk
**	subordinate pages.  The dm1u_iorphan will individually handle each
**	orphan page in the file.
**
**	Since this routine is recursive, it needs to know to stop walking the
**	chain if the chain is corrupted.  This is accomplished by checking the
**	dm1u_cb->dm1u_broken_chain flag.
**
** Inputs:
**      this_page                       page number of index page to check
**      orphan                          flag.  If true, we are called for 
**					    orphan processing
**	dm1u_cb				check/patch table control block
**
** Outputs:
**	dm1u_cb->dm1u_broken_chain	    set flag true if chain is broken.
**
**	Returns:
**	    none
**	Exceptions:
**	    calls talk, which may signal an exception if it encounters an
**	    error.   Also signals an excecption if it finds a fixed page when
**	    it is not expecting one.
**
** Side Effects:
**	    many pages are fixed and unfixed during processing of this
**	    routine and also by subordinate routines.
**
** History:
**      22-feb-90 (teg)
**	    initial creation.
**	12-jun-1990 (teresa)
**	    fixed bug 30771 where isam key comparisions were done incorrectly
**	17-oct-1991 (rogerk)
**	    Ensure that index is page is properly unfixed before exiting
**	    routine.  Previously, if the page had no children then we
**	    would forget to unfix the page leading to DMD_CHECKs.
**	    Also, put in a return following a W_DM504A_MISSING_ISAM_KEY
**	    error; otherwise we would continue on referencing an unfixed page.
**	24-oct-1991 (jnash)
**	    Lint induced changes:
**		1) Move declarations of "key" and "isam_key" to above 
**		top of loop in which they are used.
**		2) Remove degenerate test of unsigned i2 for < 0.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	30-feb-1993 (rmuth)
**	    A talk call did not include the dm1u_cb parameter.
**	9-may-94 (robf)
**          Add check for adt_kkcmp() failure.
**	06-may-1996 (thaju02)
**	    New Page Format Project: 
**		Change page header references to use macros.
**	20-jun-2002 (thaju02)
**	    verifydb -otable on isam table with pagesize > 2k would
**	    falsely report W_DM5047, W_DM500C or W_DM5050, W_DM500C.
**	    2.5 change 440330, which removed 512 entry limitation on ISAM 
**	    index pages, needs to be extended to verifydb code. (B108033)
**	    - remove check 'if (page_next_line > DM_TIDEOF)'
**	    - for pgsize > 2k, isam index entries do not have tuple hdrs.
**	      do not call dmpp_get_offset/dm1cnv2_get_offset to calculate
**	      offset, rather use dmppn_get_offset_macro.
**	    - since line table index can be > 512, remove isam_key(),
**	      which retrieved entry based on tid (tid_line is 
**	      limited to 512). Instead calculate key entry based on 
**	      offset.
[@history_template@]...
*/
static DB_STATUS
isam_index(
DM1U_CB		*dm1u,
DM_PAGENO	index_page_number,
DB_ERROR	*dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    u_i2	    page_next_line;
    u_i2	    idx_tidno;
    i4	    expected=0;
    i4	    bad;
    DMPP_PAGE	    *ipage;
    u_i4	    idx_page;
    DB_STATUS	    status;
    DM_OFFSET	    offset;
    DM_TID	    tempbid;
    u_i4	    indexlevel;
    DB_STATUS	    s;

    CLRDBERR(dberr);

    dm1u_talk(dm1u,I_DM5220_CHECK_IINDEX, 2,
                        sizeof(index_page_number), &index_page_number);

    /*	Read the next index page. */

    status = fix_page(dm1u, (DM_PAGENO) index_page_number, DM0P_READ,
	    &rcb->rcb_data, dberr);
    if (status != E_DB_OK)
    {
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM5044_IDX_CHAIN_BROKEN, 0);
	return (status);
    }

    ipage = rcb->rcb_data.page;

    /* first, assure that the specified page is really an index page. */

    if (!( DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, ipage) & 
	DMPP_DIRECT))
    {
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM5045_NOT_INDEX_PG, 2,
	    sizeof(index_page_number), &index_page_number);
	dm1u_talk(dm1u,W_DM5044_IDX_CHAIN_BROKEN, 0);
	unfix_page(dm1u, &rcb->rcb_data);
	return (E_DB_WARN);
    }

    /* Check that page hasn't been seen before. */

    if (CHECK_MAP_MACRO(dm1u, index_page_number))
    {
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM5028_MULT_REF_INDEX_PG, 2, 
		    sizeof(index_page_number), &index_page_number);
	dm1u_talk(dm1u,W_DM5044_IDX_CHAIN_BROKEN, 0);
	unfix_page(dm1u, &rcb->rcb_data);
	return (E_DB_WARN);
    }

    /*	Mark page as seen. */

    SET_MAP_MACRO(dm1u, index_page_number);

    /*	Check the page_main pointer for valid contents. */

    if (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, ipage) == 0)
    {
	/*  A leaf index page points to a data page. */

	if (DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ipage) > 
	    t->tcb_rel.relmain)
	{
	    /*	Must point into main page area. */

	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5048_INVALID_PAGE_OVFL, 4,
		sizeof(index_page_number), &index_page_number, 
		sizeof(DM_PAGENO),
		DMPP_VPT_ADDR_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ipage));
	    dm1u_talk(dm1u,W_DM5044_IDX_CHAIN_BROKEN, 0);
	    unfix_page(dm1u, &rcb->rcb_data);
	    return (E_DB_WARN);
	}
    }
    else 
    {
	if (DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ipage) < 
	    t->tcb_rel.relmain || 
	    DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ipage) > 
	    t->tcb_rel.relprim)
        {
	    /*	Must point into index area. */
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5049_INVALID_PAGE_OVFL, 4,
		sizeof(index_page_number), &index_page_number, 
		sizeof(DM_PAGENO),
		DMPP_VPT_ADDR_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ipage));
	    dm1u_talk(dm1u,W_DM5044_IDX_CHAIN_BROKEN, 0);
	    unfix_page(dm1u, &rcb->rcb_data);
	    return (E_DB_WARN);
	}
    }


    page_next_line = 
	DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype, ipage);
    /*
    **	Check that no index entry is empty and that the keys are in sorted
    **	order.  These checks are skipped if we are not processing at the
    **	RECORD level.
    */

    if (dm1u->dm1u_op & (DM1U_ATTRIBUTE | DM1U_RECORD))
    {
	char        *last_key;
	char        *key;

        for (idx_tidno = 0; idx_tidno < page_next_line; idx_tidno++)
	{
	    DM_LINEID	*lp = 
		DMPPN_VPT_PAGE_LINE_TAB_MACRO(t->tcb_rel.relpgtype, ipage);

	    offset = dmppn_vpt_get_offset_macro(t->tcb_rel.relpgtype, 
						lp, idx_tidno);
	    if (offset)
	    {
		/* Check this key with previous key. */

		if (idx_tidno)
		{
		    i4		    adt_cmp_result;

		    last_key = key;
		    key = (char *)ipage + offset;
		    s=adt_kkcmp((ADF_CB*)&dm1u->dm1u_adf.dm1u_cbadf, 
			t->tcb_keys, t->tcb_key_atts, last_key, key, 
			&adt_cmp_result);

		    if (s != E_DB_OK)
		    {
			SETDBERR(dberr, 0, E_DMF012_ADT_FAIL);
			return E_DB_ERROR;
		    }

		    if (adt_cmp_result >= DM1B_SAME)
		    {
			/* the first key was NOT less than the second key */
			dm1u_talk(dm1u,W_DM5050_BAD_ISAM_KEY_ORDER, 2,
			    sizeof(index_page_number), &index_page_number);
			unfix_page(dm1u, &rcb->rcb_data);
			return (E_DB_WARN);
		    }
		}
		else
		    key = (char *)ipage + offset;
	    }
	    else
	    {
		dm1u->dm1u_e_cnt++;
		dm1u_talk(dm1u,W_DM504A_MISSING_ISAM_KEY, 2,
		    sizeof(idx_tidno), &idx_tidno);
		unfix_page(dm1u, &rcb->rcb_data);
		return (E_DB_WARN);
	    }
	}
    }

    /*
    **	Either walk down to the next index or process the main page chain
    **	for each index entry.
    */

    for (idx_tidno = 0; idx_tidno < page_next_line; idx_tidno++)
    {
	idx_page = idx_tidno + 
	    DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ipage);
	indexlevel = DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, ipage);
	unfix_page(dm1u, &rcb->rcb_data);

	/* Process next index page or main page chain. */

	if (indexlevel)
	{
	    isam_index(dm1u, idx_page, dberr);
	}
	else
	{
	    i4	exp_main_page = idx_page + 1;

	    if (exp_main_page == t->tcb_rel.relmain)
		exp_main_page = 0;
	    if ((u_i2)(idx_tidno + 1) >= page_next_line)
		exp_main_page = -1;

	    main_chain(dm1u, idx_page, exp_main_page, dberr);
	}

	if ((u_i2)(idx_tidno + 1) < page_next_line)
	{
	    status = fix_page(dm1u, (DM_PAGENO) index_page_number,
		DM0P_READ, &rcb->rcb_data, dberr);
	    if (status != E_DB_OK)
		return (status);
	    ipage = rcb->rcb_data.page;
	}
    }

    /*
    ** Check for leftover fixed page.
    */
    if (rcb->rcb_data.page)
	unfix_page(dm1u, &rcb->rcb_data);

    return (status);
}

/*{
** Name: btree	- check/rebuild btree storage structure tables
**
** Description:
**	
**      This routine checks a btree table.  A btree consists of several different
**	types of pages:  index, leaf, leaf overflow, free list header, free page
**	and data page.  First dm1u_btree calls dm1u_freehdr to check the free 
**	page chain.  Then it calls dm1u_bindex to start at the root page and 
**	walk the tree, checking each page.  Afterwards, tic alls dm1u_borphan
**	to look for any pages that were not referenced/checked while walking the
**	tree.  Last, it calls dm1u_lchain to check the leaf page sideways pointer
**	chain.
**	
** Inputs:
**	
**	dm1u_cb			    Patch/Check Table Control Block
**	    dm1u_map			map of pages already referenced 
**	    rcb                         Record Control Block.  Contains
**		rcb_tcb_ptr		    Table control block.  Contains
**		    tcb_rel			cached copy of iirelation tuple
**		    relstat			    table status info:
**							TCB_INDEX - is index table
**
** Outputs:
**      none
**
**	Returns:
**	    none.
**	Exceptions:
**	    calls talk, which will genereate an exception in case of an error.
**	    subordinate routines fix/unfix pages.  Any of these may signal an
**		exception if they are unable to unfix a previously fixed page.
**
** Side Effects:
**	    several pages will be fixed, then unfixed.  This routine
**	    expects no fixed pages when called.
**
** History:
**      25-oct-89 (teg)
**	    initial creation.
**	22-feb-90 (teg)
**	    add dm1u_broken_chain mechanism to keep recursive routine from
**	    walking index chain after it is broken.
[@history_template@]...
*/
static DB_STATUS
btree(
DM1U_CB		*dm1u,
DB_ERROR	*dberr)
{
    DB_STATUS	status;

    /*	Process the BTREE by recursively descending the index. */

    status = btree_index(dm1u, BTREE_ROOT, dberr);
    /* If DM1U_KPERPAGE, return after checking the index */
    if (dm1u->dm1u_op & DM1U_KPERPAGE)
	return(status);
    if (status == E_DB_OK)
	status = btree_lchain(dm1u, dberr);
    if (status == E_DB_OK)
	status = check_orphan(dm1u, dberr);
    return (status);
}

/*{
** Name: btree_index	- CHECK BTREE INDEX PAGES.
**
** Description:
**
**      this routine is a recursive routine to check btree index pages.
**	It fixes the specified page into the dm1u_cb, performs some
**	structural checks on the page, then enters a loop to check the BIDS.
**  
**	For each bid on the page it will either
**	    a) call dm1u_leaf (if DMPP_SPRIG) to chech the leaf page and
**		any overflow pages chained to that page
**	    b) recurse (call itself) to check any lower level index pages.
**
**	This routine may take an exception exit, so it takes care to NEVER
**	have more than 1 index page fixed, regardless of the level of
**	recursion.  This means that it unfixes the index page before recursing
**	(or before calling dm1u_leaf), and fixes it again after getting  back.
**	NOTE: FIXING AND UNFIXING A PAGE ARE NOT SYNOMOUS WITH DISK I/O.  THE
**	ASSUMPTION IS THAT THE BUFFER MANAGER IS GOOD AT MANAGEING MEMORY, AND
**	WILL TEND NOT TO FLUSH PAGES TO DISK UNLESS NECESSARY.
**
**	NOTE: btree BIDS are like TIDs, except that the line number portion is
**	      not used.  So, the BID indicates the page number of the next lower
**	      level index page (unless this is a sprig page -- in that case, the
**	      BID indicates the leaf page number.)
**
** Inputs:
**      bindex_cb                       BTREE Index Control Block
**	    .dm1ucb			    ptr to Check Table Control Block
**		.dm1u_rcb			ptr to record control block
**		.dm1u_map			map of referenced pages
**	    .page_num			    page number of index page to check
**	    .orphan			    flag indicating if called from
**						orphan processing.
**
** Outputs:
**      bindex_cb                       BTREE Index Control Block
**	    .dm1ucb			    ptr to Check Table Control Block
**		.dm1u_map			map of referenced pages
**	    .page_num			    page number of index page to check
**
**	Returns:
**	    none
**	Exceptions:
**	    will signal an exception if unable to unfix a fixed page.
**	    also, calls talk, which may signal an exception if it encounters
**		an error.
**
** Side Effects:
**	    pages are fixed and unfixed.
**
** History:
**      03-nov-89  (teg)
**	    initial creation.
**	22-feb-90 (teg)
**	    add dm1u_broken_chain mechanism to keep recursive routine from
**	    walking index chain after it is broken.
**	13-jun-1990 (teresa)
**	    add support for user interrupts by calling fix_page to check
**	    for user interrupts then call dm0p_fix_page.  It used to call
**	    dm0p_fix_page directly.
**	17-oct-1991 (rogerk)
**	    Ensure that index is page is properly unfixed before exiting
**	    routine.  Previously, if the page had no children then we
**	    would forget to unfix the page leading to DMD_CHECKs.
**	24-oct-1991 (jnash)
**	    Add (E_DB_WARN) params to two RETURN's which had no param
**          specified (to satisfy LINT).
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		- Change page header references to use macros.
**      20-Aug-2003 (huazh01)
**          Do not report W_DM502B on a V2 Btree INDEX page that 
**          contains more than 512 entries. 
**          bug 110355.
[@history_template@]...
*/
static DB_STATUS
btree_index(
DM1U_CB		*dm1u,
DM_PAGENO	index_page_number,
DB_ERROR	*dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB         *t = rcb->rcb_tcb_ptr;
    u_i4	    idx_tidno;
    u_i4	    idx_page;
    DB_STATUS	    status;
    i4	    expected=0;
    i4	    bad;
    DM_OFFSET	    offset;
    DMPP_PAGE	    *ipage;
    i4	    page_stat;
    DM_TID	    tempbid;
    i4		    bt_kids;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    dm1u_talk(dm1u,I_DM5210_CHECK_BINDEX, 2,
                        sizeof(index_page_number), &index_page_number);

    /*	Fix index page for processing. */

    status = fix_page(dm1u, (DM_PAGENO) index_page_number, DM0P_READ,
	&rcb->rcb_data, dberr);
    if (status != E_DB_OK)
	return (status);

    ipage = rcb->rcb_data.page;
    page_stat = DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, ipage);

    /* first, assure that the specified page is really an index page. */

    if (!(page_stat & DMPP_INDEX))
    {
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM5027_NOT_INDEX_PG, 2,
	    sizeof(index_page_number), &index_page_number);
	unfix_page(dm1u, &rcb->rcb_data);
	return (E_DB_ERROR);
    }

    /* Check that page has not been referenced before. */

    if (CHECK_MAP_MACRO(dm1u, index_page_number))
    {
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM5028_MULT_REF_INDEX_PG, 2, 
	    sizeof(index_page_number), &index_page_number);
	unfix_page(dm1u, &rcb->rcb_data);
	return (E_DB_ERROR);
    }

    if (dm1u->dm1u_op & DM1U_KPERPAGE)
    {
	TRdisplay("DM1U_VERIFY: Table (%d,%d) (%~t,%~t) index %d \n",
	      t->tcb_rel.reltid.db_tab_base,
	      t->tcb_rel.reltid.db_tab_index,
	      sizeof(t->tcb_rel.relid), &t->tcb_rel.relid,
	      sizeof(t->tcb_rel.relowner), &t->tcb_rel.relowner,
	      index_page_number);
    }

    /*	Mark page as referenced. */

    SET_MAP_MACRO(dm1u, index_page_number);

    /*
    ** If no key compression, validate kperpage in tcb for root index page
    */
    if (DM1B_INDEX_COMPRESSION(rcb) == DM1CX_UNCOMPRESSED &&
	(index_page_number == BTREE_ROOT))
    {
	DM_LINEID       *lp;
	char            *lrange_addr;
	i4              alloc_kids;
	i4              size_lp;

	/* This assumes LRANGE entry is first entry in data space */
	lrange_addr = dm1b_vpt_entry_macro(t->tcb_rel.relpgtype, ipage, 
	    (i4)DM1B_LRANGE);
	lp = DM1B_VPT_BT_SEQUENCE_MACRO(t->tcb_rel.relpgtype, ipage);
	size_lp = (char *)lrange_addr - (char *)lp;
	alloc_kids = size_lp / (DM_VPT_SIZEOF_LINEID_MACRO(t->tcb_rel.relpgtype));

	if (alloc_kids != t->tcb_kperpage + DM1B_OFFSEQ)
	{
	    dm1u->dm1u_e_cnt++;
	    uleFormat(NULL, W_DM5062_INDEX_KPERPAGE, 
		(CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, (i4)0, (i4)0,
		err_code, 10, 
		sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
		sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
		sizeof(index_page_number), &index_page_number,
		sizeof(alloc_kids), &alloc_kids,
		sizeof(t->tcb_kperpage), &t->tcb_kperpage);
	    unfix_page(dm1u, &rcb->rcb_data);
	    SETDBERR(dberr, 0, E_DM926B_BUILD_TCB); /* bad tcb_kperpage */
	    return (E_DB_ERROR);
	}
    }

    /*	Check for bad pointers. */

    {
	i4	    expected = 0;

	if (DM1B_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, ipage))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM500B_WRONG_PAGE_MAIN, 6,
		sizeof(index_page_number), &index_page_number,
		sizeof(expected), &expected,
		sizeof(DM_PAGENO),
		DM1B_VPT_ADDR_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, ipage));
	}
	if (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ipage))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM500D_WRONG_PAGE_OVFL, 4,
		sizeof(index_page_number), &index_page_number,
		sizeof(DM_PAGENO),
		DM1B_VPT_ADDR_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, ipage));
	}
	if (DM1B_VPT_GET_BT_DATA_MACRO(t->tcb_rel.relpgtype, ipage))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM502A_WRONG_BT_DATA, 4,
		sizeof(index_page_number), &index_page_number,
		sizeof(DM_PAGENO),
		DM1B_VPT_ADDR_BT_DATA_MACRO(t->tcb_rel.relpgtype, ipage));
	}
    }

    bt_kids = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, ipage);
    if ((bt_kids > DM1B_MAXBID || bt_kids < 0) &&
        (t->tcb_rel.relpgsize == DM_COMPAT_PGSIZE))
    {
	i4	    expected = DM1B_MAXBID;

	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM502B_INVALID_BT_KIDS, 6, 
	    sizeof(index_page_number), &index_page_number,
	    sizeof(bt_kids), &bt_kids,
	    sizeof(expected), &expected);
	dm1u_talk(dm1u,W_DM5026_IDX_CHAIN_BROKEN, 0);
	unfix_page(dm1u, &rcb->rcb_data);
	return (E_DB_ERROR);
    }

    /* now set up a loop to check the index pages referenced by BIDS on
    **	this index page.  First, assure that offset to bid is valid */

    for (idx_tidno = 0; idx_tidno < bt_kids; idx_tidno++)
    {
	offset = (DM_OFFSET) dm1bvpt_kidoff_macro(t->tcb_rel.relpgtype, 
			ipage, idx_tidno);
	if (offset)
	{
	    /* get page number from bid */
	    if ((offset < (DM_OFFSET) DM1B_VPT_OVER(t->tcb_rel.relpgtype)) || 
		(offset > (DM_OFFSET) t->tcb_rel.relpgsize))
	    {
		dm1u->dm1u_e_cnt++;
		dm1u_talk(dm1u,W_DM502C_BAD_BID_OFFSET, 4, 
		    sizeof(idx_tidno), &idx_tidno,
		    sizeof(index_page_number), &index_page_number);
		unfix_page(dm1u, &rcb->rcb_data);
		return (E_DB_ERROR);
	    }
	    dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, ipage,
			idx_tidno, &tempbid, (i4*)NULL);
	    idx_page = (u_i4) tempbid.tid_tid.tid_page;
	    
	    unfix_page(dm1u, &rcb->rcb_data);

            if (idx_page == index_page_number)
            {
                dm1u_talk(dm1u,W_DM502D_IDX_REFERENCES_SELF, 4,
                           sizeof(index_page_number), &index_page_number,
			   sizeof(idx_tidno), &idx_tidno);
                return (E_DB_WARN); 
	    } 
	    else 
	    if (idx_page == BTREE_ROOT)
            {
                dm1u_talk(dm1u,W_DM502E_IDX_REFERENCES_ROOT, 4,
                           sizeof(index_page_number), &index_page_number,
			   sizeof(idx_tidno), &idx_tidno);
                return (E_DB_WARN);
            }

	    /* go examine leaf or index page that this page points to */

	    if (page_stat & DMPP_SPRIG)
		status = btree_leaf(dm1u, idx_page, dberr);
	    else
		status = btree_index(dm1u, idx_page, dberr);

	    /*
	    ** Return if we just wanted to verify KPERPAGE
	    ** If we have processed the first leaf, we have also done
	    ** an index page as well
	    */
	    if ((dm1u->dm1u_op & DM1U_KPERPAGE) &&
		(dm1u->dm1u_1stleaf || status != E_DB_OK))
		return (status);

	    if (idx_tidno + 1 < bt_kids)
	    {
		status = fix_page(dm1u, (DM_PAGENO) index_page_number,
		    DM0P_READ, &rcb->rcb_data, dberr);
		if (status != E_DB_OK)
		    return (status);
		ipage = rcb->rcb_data.page;
	    }

	} /* endif there is a bid to check at this bt_sequence[] */

    } /* end loop for each BID on index page */

    /*
    ** Check for leftover fixed page.
    */
    if (rcb->rcb_data.page)
	unfix_page(dm1u, &rcb->rcb_data);

    return (E_DB_OK);
}

/*{
** Name: btree_leaf	- check a btree leaf page
**
** Description:
**
**      This routine checks btree leaf pages, and any overflow leaf pages 
**	that may be chained to the specified leaf page.  First it reads the
**	specified leaf page and verifies it is really a leaf page.  Then it
**	checks to see if this page has been referenced before -- it should not
**	be previously referenced!  If the page is previouisly referenced, it
**	will try to determine if the chain is merged or circular.  However,
**	it needs the dm1u_cb->dm1u_chain bitmap to make that determination.
**	If there was not enough memory to allocate the dm1u_chain bitmap,  then
**	dm1u_leaf skips trying to determine whether the chain is circular or
**	merged.
**
**	It checks each leaf page for positive and negative infinity.  The
**	leaf page that contains the smallest key in the table should be
**	marked as negative infinity, and the leaf page with the largest key
**	should be marked as positive infinity.  There should never be more than
**	one leaf page marked as positive infinity and never more than one leaf
**	page marked as negative infinity.  However, in a small file there may
**	only be 1 leaf page.  In that case, the same leaf page will be marked
**	as both positive and negative infinity.
**
**	Next, it gets the smallest key on the leaf page (unless the page is
**	empty), and calls btree_line_table() to check each tid on the page.
**
**	If there is a leaf overflow page, dm1u_leaf will enter a loop to
**	process each overflow page.  dm1u_leaf will fix the leaf page to
**	rcb->rcb_data.page.  That page will remain fixed throughout
**	leaf overflow page processing. Each leaf overflow page will be fixed
**	to rcb->rcb_other.page, processed, then unfixed (before the next
**	leaf overflow page is fixed).
**
** Inputs:
**      dm1u_cb                         Check Table Control Block.  Contains:
**	leaf_page_number		Leaf page number.
**
** Outputs:
**
**	Returns:
**	    none
**	Exceptions:
**	    it will signal an exception if it cannot unfix a fixed page.
**	    Also, it calls dm1u_talk(), which may signal an exception if it
**		encounters an error
**
** Side Effects:
**	    leaf and leaf overflow pages are fixed and unfixed.  No leaf or
**	    leaf overflow or data pages should be fixed when this routine is
**	    called.  This routine calls btree_line_table, which may fix/unfix
**	    data pages.
**
** History:
**      07-nov-89  (teg)
**          initial creation.
**	22-feb-90 (teg)
**	    add dm1u_broken_chain mechanism to keep recursive routine from
**	    walking index chain after it is broken.
**	13-jun-1990 (teresa)
**	    add support for user interrupts by calling fix_page to check
**	    for user interrupts then call dm0p_fix_page.  It used to call
**	    dm0p_fix_page directly.
**	29-jan-1991 (bryanp)
**	    Added support for Btree index compression.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	30-feb-1993 (rmuth)
**	    Add  dm1u_cb paramter to a talk call.
**	26-jul-1993 (rogerk)
**	    Fix problems with table_verify of btrees that have empty leaf
**	    and overflow pages.  The btree_leaf routine extracts the first
**	    key from leaf/overflow pages to verify that entries on the page
**	    are in correct sorted order.  This key extraction was not paying
**	    attention to whether there were any entries on the current page.
**	    This caused errors to be reported when empty leaf pages were
**	    encountered in btrees with compressed indexes.  Also, when scanning
**	    an overflow chain, the routine attempted to take the first key
**	    off of an overflow chain if all previous pages on the chain were
**	    empty.  Instead, this routine always tried to extract the first
**	    key off of the original leaf page.  This again caused problems when
**	    when the leaf page was empty.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Project:
**		Change page header references to use macros.
**	    Added page_size parameter to dm1cxlog_error routine.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      12-jun-97 (stial01)
**          btree_leaf() Pass tlv to dm1cxget instead of tcb.
[@history_template@]...
*/
static DB_STATUS
btree_leaf(
DM1U_CB		*dm1u,
DM_PAGENO	leaf_page_number,
DB_ERROR	*dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB         *t = rcb->rcb_tcb_ptr;
    ADF_CB	    *adfcb = rcb->rcb_adf_cb;
    char	    *KeyBuf, *AllocKbuf = NULL;
    char	    key[DM1B_MAXSTACKKEY];    
    char	    *keyptr;
    DM_TID	    tid;
    DB_STATUS	    status;
    DB_STATUS       get_status;
    DMPP_PAGE	    *leafpage;
    DMPP_PAGE	    *ovflpage;
    u_i4	    ovfl;
    DM_TID	    infinity;
    i4	    klen;
    i4		    line_zero = 0;
    bool	    have_key = FALSE;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* first, fix the page.  If we cannot fix page, report that chain is
    ** broken and give up on this page. */

    status = fix_page(dm1u, (DM_PAGENO) leaf_page_number, 
	 DM0P_READ, &rcb->rcb_data, dberr);
    if (status != E_DB_OK)
	return (status);
    leafpage = rcb->rcb_data.page;

    /* see if the page is really an unreferenced leaf */

    if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
	leafpage) & DMPP_LEAF)
    {
        dm1u_talk(dm1u,I_DM520A_LEAF_PAGE, 2,
	    sizeof(leaf_page_number), &leaf_page_number);
        dm1u_talk(dm1u,I_DM520F_CHECK_LEAF, 2,
	    sizeof(leaf_page_number), &leaf_page_number);
 	if (CHECK_MAP_MACRO(dm1u, leaf_page_number))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5031_MULT_REFERENCED_LEAF, 2,
		sizeof(leaf_page_number), &leaf_page_number);
	    if (CHECK_CHAIN_MACRO(dm1u, leaf_page_number))
		dm1u_talk(dm1u,W_DM5033_CIRCULAR_LEAF_CHAIN, 2,
		    sizeof(leaf_page_number), &leaf_page_number);
	    else
		dm1u_talk(dm1u,W_DM5032_MERGED_LEAF_CHAIN, 2,
		    sizeof(leaf_page_number), &leaf_page_number);
	    unfix_page(dm1u, &rcb->rcb_data);
	    return (E_DB_ERROR);
	}

	dm1u->dm1u_numleafs++;
	SET_CHAIN_MACRO(dm1u, leaf_page_number);
	SET_MAP_MACRO(dm1u, leaf_page_number);
    }
    else
    {
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM5030_NOT_LEAF_PG, 2,
	    sizeof(leaf_page_number), &leaf_page_number);
	unfix_page(dm1u, &rcb->rcb_data);
	return (E_DB_ERROR);
    }

    if (dm1u->dm1u_op & DM1U_KPERPAGE)
    {
	TRdisplay("DM1U_VERIFY: Table (%d,%d) (%~t,%~t) leaf %d \n",
	      t->tcb_rel.reltid.db_tab_base,
	      t->tcb_rel.reltid.db_tab_index,
	      sizeof(t->tcb_rel.relid), &t->tcb_rel.relid,
	      sizeof(t->tcb_rel.relowner), &t->tcb_rel.relowner,
	      leaf_page_number);
    }

    /*	Save the leaf end-points and check for multiple end points. */

    if (dm1bvpt_kidoff_macro(t->tcb_rel.relpgtype, leafpage, DM1B_LRANGE))
    {
	dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leafpage, 
		DM1B_LRANGE, &infinity, (i4*)NULL); 
	if (infinity.tid_i4 == TRUE)
	{
	    if (dm1u->dm1u_1stleaf)
	    {
		dm1u->dm1u_e_cnt++;
		dm1u_talk(dm1u,W_DM5034_TOOMANY_NEG_INFINITY, 4,
		    sizeof(leaf_page_number), &leaf_page_number, 
		    sizeof(dm1u->dm1u_1stleaf), &dm1u->dm1u_1stleaf);
	    }
	    else
	    {
		dm1u->dm1u_1stleaf = leaf_page_number;

		/*
		** If no key compression, validate kperpage in tcb for 1stleaf 
		*/
		if (DM1B_INDEX_COMPRESSION(rcb) == DM1CX_UNCOMPRESSED)
		{
		    DM_LINEID       *lp;
		    char            *lrange_addr;
		    i4              alloc_kids;
		    i4              size_lp;

		    /* This assumes LRANGE entry is first entry in data space */
		    lrange_addr = dm1b_vpt_entry_macro(t->tcb_rel.relpgtype, 
				leafpage, (i4)DM1B_LRANGE); 
		    lp = DM1B_VPT_BT_SEQUENCE_MACRO(t->tcb_rel.relpgtype, leafpage);
		    size_lp = (char *)lrange_addr - (char *)lp;
		    alloc_kids = size_lp / 
			(DM_VPT_SIZEOF_LINEID_MACRO(t->tcb_rel.relpgtype));

		    if (alloc_kids != t->tcb_kperleaf + DM1B_OFFSEQ)
		    {
			dm1u->dm1u_e_cnt++;
			uleFormat(NULL, W_DM5063_LEAF_KPERPAGE,
			    (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
			    (i4)0, (i4)0, err_code, 10,
			    sizeof(DB_TAB_NAME), t->tcb_rel.relid.db_tab_name,
			    sizeof(DB_OWN_NAME), t->tcb_rel.relowner.db_own_name,
			    sizeof(leaf_page_number), &leaf_page_number,
			    sizeof(alloc_kids), &alloc_kids,
			    sizeof(t->tcb_kperleaf), &t->tcb_kperleaf);
			unfix_page(dm1u, &rcb->rcb_data);
			SETDBERR(dberr, 0, E_DM926B_BUILD_TCB); /* bad tcb_kperleaf */
			return (E_DB_ERROR);
		    }
		}
	    }
	}
    }
    if (dm1bvpt_kidoff_macro(t->tcb_rel.relpgtype, leafpage, DM1B_RRANGE))
    {
	dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leafpage, 
		DM1B_RRANGE, &infinity, (i4*)NULL);
	if (infinity.tid_i4 == TRUE)
	{
	    if (dm1u->dm1u_lastleaf)
	    {
		dm1u->dm1u_e_cnt++;
		dm1u_talk(dm1u,W_DM5035_TOOMANY_POS_INFINITY, 4,
		    sizeof(leaf_page_number), &leaf_page_number, 
		    sizeof(dm1u->dm1u_lastleaf), &dm1u->dm1u_lastleaf);
	    }
	    else
		dm1u->dm1u_lastleaf = leaf_page_number;
	}
    }

    if ( status = dm1b_AllocKeyBuf(t->tcb_klen, key,
				&KeyBuf, &AllocKbuf, dberr) )
	return(status);

    /*
    ** If the page has key entries, then extract the first entry and use
    ** it to check all other keys on the page.
    */
    if (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
	leafpage) > 0)
    {
	if (dm1bvpt_kidoff_macro(t->tcb_rel.relpgtype, leafpage, 0) == 0)
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5036_CANT_READ_LEAF_TID, 4,
		sizeof(line_zero), &line_zero,
		sizeof(leaf_page_number), &leaf_page_number);
	    unfix_page(dm1u, &rcb->rcb_data);
	    return (E_DB_ERROR);
	}
	keyptr = KeyBuf;
	klen = t->tcb_klen;
	status = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, leafpage,
		    &t->tcb_leaf_rac,
		    line_zero, &keyptr,
		    (DM_TID *)NULL,  (i4*)NULL,
		    &klen, NULL, NULL, adfcb);
	get_status = status;
	if (status == E_DB_WARN 
		&& (t->tcb_rel.relpgtype != TCB_PG_V1) )
	    status = E_DB_OK;
	if (status != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, rcb, leafpage,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			    line_zero);
	    dm1u_talk(dm1u, W_DM5059_BAD_KEY_GET, 4, sizeof(i4),
			    DM1B_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			    leafpage),
			    sizeof(i4), &line_zero);
	    dm1u_talk(dm1u, W_DM5026_IDX_CHAIN_BROKEN, 0);
	    unfix_page (dm1u, &rcb->rcb_data);
	    /* Discard any allocated key buffer */
	    if ( AllocKbuf )
		dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);
	    return (E_DB_ERROR);
	}
	if (keyptr != KeyBuf)
	    MEcopy(keyptr, klen, KeyBuf);
	have_key = TRUE;
	btree_line_table(dm1u, leafpage, leaf_page_number, KeyBuf, dberr);
    }

    /*	Prepare to check each of the overflow pages. */

    for (ovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype,
	leafpage); ovfl; )
    {
	if (CHECK_MAP_MACRO(dm1u, ovfl))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5031_MULT_REFERENCED_LEAF, 2, sizeof(ovfl), &ovfl);
	    if (CHECK_CHAIN_MACRO(dm1u, ovfl))
		dm1u_talk(dm1u,W_DM5033_CIRCULAR_LEAF_CHAIN, 2, sizeof(ovfl), &ovfl);
	    else
		dm1u_talk(dm1u,W_DM5032_MERGED_LEAF_CHAIN, 2, sizeof(ovfl), &ovfl);
	    break;
	}

	/* Now fix the overflow page */

	status = fix_page(dm1u, (DM_PAGENO)ovfl, DM0P_READ,
	    &rcb->rcb_other, dberr);
	if (status != E_DB_OK)
	    break;
	ovflpage = rcb->rcb_other.page;

	/* Make sure this page really is an overflow page */

	if (!(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
	    ovflpage) & DMPP_CHAIN))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5037_NOT_LEAF_OVFL_PG, 2, sizeof(ovfl), &ovfl);
	    break;	
	}
	else
        {
            dm1u_talk(dm1u,I_DM520B_LEAF_OVERFLOW, 2, sizeof(ovfl), &ovfl);
        }
	SET_MAP_MACRO(dm1u, ovfl);

	if (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
	    leafpage) != 
	    DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype,
	    ovflpage))
        {
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5038_WRONG_BTNEXTPG, 4, 
		 sizeof(ovfl), &ovfl,
		 sizeof(leaf_page_number), &leaf_page_number);
	}
	if (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, 
	    leafpage) != 
	    DM1B_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,
	    ovflpage))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5039_WRONG_OVFL_PAGEMAIN, 4, 
		 sizeof(ovfl), &ovfl,
		 sizeof(leaf_page_number), &leaf_page_number);
	}    
	if (DM1B_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
	    ovflpage) != (DM_PAGENO) ovfl)
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5001_BAD_PG_NUMBER, 4, 
		 sizeof(ovfl), &ovfl, 
		 sizeof(DM_PAGENO),
		 DM1B_VPT_ADDR_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype,
		 ovflpage));
	}

	/*
	** If this page has key entries, check that they are all the same and
	** that they match the parent leaf page keys (all keys on an overflow
	** chain must be duplicates).
	**
	** (the loop here is used to allow breaks to the bottom, it is
	** not actually expected to loop).
	*/
	while (DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype,
	    ovflpage) > 0)
	{
	    /*
	    ** If the parent leaf page and all previous overflow pages were
	    ** empty, then we will not yet have a key to use for the line
	    ** table checks.  If this is the case, then we need to extract
	    ** the first key off of this page to use.
	    */
	    if ( ! have_key)
	    {
		if (dm1bvpt_kidoff_macro(t->tcb_rel.relpgtype, ovflpage, 0) == 0)
		{
		    dm1u->dm1u_e_cnt++;
		    dm1u_talk(dm1u,W_DM5036_CANT_READ_LEAF_TID, 4,
			sizeof(line_zero), &line_zero, sizeof(ovfl), &ovfl);
		    break;
		}
		keyptr = KeyBuf;
		klen = t->tcb_klen;
		status = dm1cxget(t->tcb_rel.relpgtype, 
				    t->tcb_rel.relpgsize, ovflpage,
				    &t->tcb_leaf_rac,
				    0, 
				    &keyptr, (DM_TID *)NULL, (i4*)NULL,
				    &klen, NULL, NULL, adfcb);
		if (status != E_DB_OK)
		{
		    dm1cxlog_error(E_DM93E3_BAD_INDEX_GET, rcb, ovflpage,
			       t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 0);
		    dm1u_talk(dm1u, W_DM5059_BAD_KEY_GET, 4, sizeof(i4),
			DM1B_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			ovflpage),
			sizeof(i4), &line_zero);
		    dm1u_talk(dm1u, W_DM5026_IDX_CHAIN_BROKEN, 0);
		    break;
		}
		if (keyptr != KeyBuf)
		    MEcopy(keyptr, klen, KeyBuf);
		have_key = TRUE;
	    }
	    btree_line_table(dm1u, ovflpage, ovfl, KeyBuf, dberr);
	    break;
	}
	ovfl = DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, 
		ovflpage);
	unfix_page (dm1u, &rcb->rcb_other);

    }	/* end loop for overflow pages */

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);
    
    /* unfix any fixed pages */
    if (rcb->rcb_data.page)
	unfix_page (dm1u, &rcb->rcb_data);
    if (rcb->rcb_other.page)
	unfix_page (dm1u, &rcb->rcb_other);
    return(status);
}

/*{
** Name: btree_line_table	- do structural check on leaf pages
**
** Description:
**
**      This routine checks the keys and tids on the leaf page.  It assures
**	that the keys on overflow pages (or leaf pages that have overflow pages)
**	are identical.  It assures that keys on regular leaf pages are in
**	accending order (with duplicates permitted).
** 
**	btee_line_table assures that all tids specified on the leaf/overflow
**	page really exist on a data page, by calling dm1u_bdata.  (Also,
**	dm1u_bdata will check the structural/data integrity of the data page
**	if this is the first time the data page is referenced.)
**
**	The leaf page to check is expected to be fixed on entry, and remains
**	fixed on exit.  That page must NOT be fixed to rcb->rcb_data.page
**	because subordinate routine dm1u_bdata will fix/unfix data pages to
**	that spot.
**
**	If the ORPHAN flag is set  to TRUE, then btree_line_table will skip the
**	call to dm1u_bdata.
**
** Inputs:
**	page				a fixed btree leaf or overflow page
**	    bt_kids			    number of tids on this page
**	    bt_sequence			    offsets to the tid/tid pairs
**	    bt_pagedata			    the actual key/tid pairs
**	page_num			the page number of that fixed page
**	key				lowest possible key for that page.
**	orphan				flag.  If true, skip check of data
**					    pages referenced by this page.
**      dm1u_cb                         Check Table Control Block.  Contains:
**	    .dm1u_rcb			  Ptr to Record Control Block. Contains:
**		.rcb_tcb_ptr		    Ptr to Table Control block.  Contains:
**		    .tcb_keys		       Number of attributes in key
**		    .tcb_key_atts	       array of DVs describing key
**
** Outputs:
**	*key				last key on that page.
**
**	Returns:
**	    none
**	Exceptions:
**	    calls talk, which may signal an exception if it encounters an
**	    error
**
** Side Effects:
**	    the value of input parameter key may change
**	    data pages may be fixed and unfixed
**
** History:
**      07-nov-89  (teg)
**          initial creation.
**	22-feb-90 (teg)
**	    add dm1u_broken_chain mechanism to keep recursive routine from
**	    walking index chain after it is broken.
**	29-jan-1991 (bryanp)
**	    Added support for Btree index compression.
**	24-oct-1991 (jnash)
**	    Fix LINT problems where return statement had no assoc value,
**	    and where no returns existed at all.  Noted that this
**	    fn has many other such problems.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	30-feb-1993 (rmuth)
**	    Add dm1u_cb parameter to talk call.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Project:
**		Change page header references to use macros.
**	    Added page_size parameter to dm1cxlog_error routine.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
[@history_template@]...
*/
static DB_STATUS
btree_line_table(
DM1U_CB		*dm1u,
DMPP_PAGE	*page,
u_i4		page_num,
char		*key,
DB_ERROR	*dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    i4		    i;
    DM_OFFSET	    offset;
    i4	    compare=0;
    DM_TID	    temptid;
    i4		    bt_kids = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, 
				page);
    DB_STATUS	    stat;
    DB_STATUS       get_status;
    char	    *KeyBuf, *AllocKbuf = NULL;
    char	    temp_key[DM1B_MAXSTACKKEY];
    char	    *key_ptr;
    i4	    klen;

    CLRDBERR(dberr);

    if ( bt_kids > DM1B_MAXBID || bt_kids < 0 )
    {
	i4	    expected = DM1B_MAXBID;

	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM502B_INVALID_BT_KIDS, 6, 
	    sizeof(page_num), &page_num, 
	    sizeof(bt_kids), &bt_kids,
	    sizeof(expected), &expected);
	return(E_DB_ERROR);
    }


    if ( stat = dm1b_AllocKeyBuf(t->tcb_klen, temp_key,
				&KeyBuf, &AllocKbuf, dberr) )
	return(stat);

    /* loop for each tid on leaf page */

    for (i=0; i<bt_kids; i++)
    {
	/* first see if offset to tid data is legal */

	offset = (DM_OFFSET)dm1bvpt_kidoff_macro(t->tcb_rel.relpgtype,page, i);
	if (offset)
	{
	    /* get page number from bid */
	    if ((offset < (DM_OFFSET) DM1B_VPT_OVER(t->tcb_rel.relpgtype)) || 
		(offset > (DM_OFFSET) t->tcb_rel.relpgsize) )
	    {
		/* the offset to the bid is invalid, so report error and continue to
		** next bid on page */
		dm1u->dm1u_e_cnt++;
		dm1u_talk(dm1u,W_DM502C_BAD_BID_OFFSET, 4, 
		    sizeof(i), &i, sizeof(page_num), &page_num);
		break;
	    }
	}
	else
	{
	    /* it is not valid to have a null offset on a leaf page, so report
	    ** error */

	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM502C_BAD_BID_OFFSET, 4, 
		sizeof(i), &i, sizeof(page_num), &page_num);
	    break;
	}

	key_ptr = KeyBuf;
	klen = t->tcb_klen;
	stat = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page,
			&t->tcb_leaf_rac,
			i, &key_ptr,
		        (DM_TID *)NULL, (i4*)NULL,
			&klen, NULL, NULL, rcb->rcb_adf_cb);
	get_status = stat;
	if (stat == E_DB_WARN 
		&& (t->tcb_rel.relpgtype != TCB_PG_V1) )
	    stat = E_DB_OK;
	if (stat != E_DB_OK)
	{
	    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, rcb, page,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, i);
	    dm1u_talk(dm1u, W_DM5059_BAD_KEY_GET, 4, sizeof(i4),
			    DM1B_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			    page),
			    sizeof(i4), &i);
	    break;
	}

	/* if we get here, the page appears structurally sound, so check key */
	adt_kkcmp((ADF_CB*) &dm1u->dm1u_adf.dm1u_cbadf, t->tcb_keys, 
			t->tcb_leafkeys, key_ptr, key, &compare);

	if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,page) & DMPP_CHAIN)
	{
	    if (compare != DM1B_SAME)
	    {
		dm1u->dm1u_e_cnt++;
		dm1u_talk(dm1u,W_DM503A_BAD_OVFL_PG_KEY, 4,
		    sizeof(page_num), &page_num, sizeof(i), &i);
		break;
	    }
	}
	else if ( DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, page) && 
	    (compare != DM1B_SAME) )
	{
	    /* this is the parent of an overflow chain, and all keys on
	    ** this page should be identical, but are not. */
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM503B_BAD_OVFL_PARENT_KEY, 4,
		sizeof(page_num), &page_num, sizeof(i), &i);
	    break;
	}
	else if (compare < DM1B_SAME)
	{
	    /* here the key is less than the earlier key, which is an error */
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM503C_BAD_LEAF_KEY, 4,
		sizeof(page_num), &page_num, sizeof(i), &i);
	    break;
	}

	if ( (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & 
	    DMPP_LEAF) && 
	    (DM1B_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, page) == 0) )
	{
	    /* update the value of key to this tid's key */
	    MEcopy(key_ptr, klen, key);
	}

	/* Don't get data for deleted keys */
	if (get_status == E_DB_WARN)
	    continue;
	
	/* the key is ok, so get the tid */

	dm1cxtget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, page, i,
			&temptid, (i4*)NULL);
	if (btree_data(dm1u, (u_i4) temptid.tid_tid.tid_page, 
	    (u_i4) temptid.tid_tid.tid_line, dberr) == FALSE)
	{
	    u_i4    pag = (u_i4) temptid.tid_tid.tid_page;
	    u_i4    lin = (u_i4) temptid.tid_tid.tid_line;

	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM503D_NO_SUCH_TID, 6, 
		sizeof(page_num), &page_num,
		sizeof(page), &pag, sizeof(lin), &lin);
	}

    }  /* end loop for each bid */

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    return(E_DB_OK);
}

/*{
** Name: btree_data	- check btree data page
**
** Description:
**
**      this routine first determines whether or not to expect data pages in
**	the file as follows:  if this table is a secondary index, then do NOT
**	expect data pages.  Otherwise, expect data pages in the file. If this
**	table is a secondary index,  dm1u_bdata just exits.
**
**	Otherwise, dm1u_bdata fixes the data page, checks a few page fields that
**	should contain zero, then calls dm1u_datapg() to check the rest of the
**	page.  After dm1u_datapg() returns, then dm1u_bdata checks to see if the
**	specified tid exists on this page.  If it exists, dm1u_bdata returns
**	TRUE. Otherwise it returns FALSE.
**
**	This routine fixes pages to dm1u_cb->dm1u_data.page, and unfixes
**	the page on exit.  If assumes that dm1u_table's exception handling logic
**	will unfix this page if dm1u_bdata generates an exception exit.
**
**	WARNING:  THIS ROUTINE HAS A WEAKNESS THAT IT DOES NOT VALIDATE THE
**		    TID FROM THE LEAF PAGE FOR SECONDARY INDEXES.  HOWEVER, IT
**		    SHOULD BE NOTED THAT IS IS ACTUALLY FASTER TO DESTROY THE
**		    SECONDARY INDEX AND RECREATE IT AND MODIFY IT TO BTREE THAN
**		    IT IS TO CHECK TIDS IN A DIFFERENT FILE.
**
** Inputs:
**      page_num                        page_number of data page
**	line_num			line number from TID.  This means that
**					    page_line_tab[line_num] should be
**					    nonzero and line_num < page_next_line.
**	dm1u_cb				Check Table control block.  Contains:
**	    .dm1u_map			    bit map of visited pages
**	    .rcb			    record control block.  Contains:
**		.rcb_tcb_ptr			ptr to Table Control Block. 
**						 TCB contains:
**		    .tcb_rel			    copy of iirelation tuple
**			.relspec			status word: TCB_INDEX
** Outputs:
**	dm1u_cb				Check Table control block.  Contains:
**	    .dm1u_map			    bit map of visited pages
**
**	Returns:
**	    TRUE  -> specified TID exists on page
**	    FALSE -> specified TID did not exist on page
**	Exceptions:
**	    it will signal an exception if it cannot unfix a fixed data page.
**	    Also, it calls dm1u_talk(), which may signal an exception if it
**		encounters an error
**
** Side Effects:
**	    the data page is fixed and then unfixed.
**
** History:
**      01-nov-89 (teg)
**	    initial creation
**	13-jun-1990 (teresa)
**	    add support for user interrupts by calling fix_page to check
**	    for user interrupts then call dm0p_fix_page.  It used to call
**	    dm0p_fix_page directly.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	18-oct-1993 (rmuth)
**	    B45710, Correct a problem where a dm1u_talk call was 
**	    missing a comma between last two parameters.
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Change page header references to use macros.
[@history_template@]...
*/
static i4
btree_data(
DM1U_CB		 *dm1u,
u_i4		 page_num,
u_i4		 line_num,
DB_ERROR	 *dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DB_STATUS	     status;
    DMPP_PAGE	    *page;
    i4	     temp = 0;
    i4		     page_next_line;

    CLRDBERR(dberr);

    if (t->tcb_rel.relstat & TCB_INDEX)
	return (TRUE);

    status = fix_page(dm1u, (DM_PAGENO) page_num, 
	DM0P_WRITE | DM0P_NOREADAHEAD, &dm1u->dm1u_data, dberr);
    if (status != E_DB_OK)
	return (FALSE);

    page = (DMPP_PAGE *)dm1u->dm1u_data.page;
    if ( !(DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & DMPP_DATA) )
    {
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM5042_NOT_DATA_PAGE, 2,sizeof(page_num), &page_num);
	unfix_page(dm1u, &dm1u->dm1u_data);
	return (FALSE);
    }

    if (!CHECK_MAP_MACRO(dm1u, page_num))
    {
	/* we have not visited this page before, so check it now */

	SET_MAP_MACRO(dm1u, page_num);

	if (DMPP_VPT_GET_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, page))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM500B_WRONG_PAGE_MAIN, 6,
		 sizeof(page_num), &page_num, sizeof(temp), &temp,
		 sizeof(DM_PAGENO),
		 DMPP_VPT_ADDR_PAGE_MAIN_MACRO(t->tcb_rel.relpgtype, page));
	}
	if (DMPP_VPT_GET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, page))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM500D_WRONG_PAGE_OVFL, 4, 
		sizeof(page_num), &page_num, 
		sizeof(DM_PAGENO),
		DMPP_VPT_ADDR_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, page));
	}

	/* this page has not been checked before, so check it now */

	data_page(dm1u, (DMPP_PAGE *)dm1u->dm1u_data.page, dberr);
    }

    page_next_line = DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype,page);
    temp = TRUE;
    if ( line_num > page_next_line)
	temp=FALSE;
    if ( (*t->tcb_acc_plv->dmpp_get_offset)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, page, line_num) == 0 )
	temp=FALSE;

    unfix_page(dm1u, &dm1u->dm1u_data);

    return (temp);
}

/*{
** Name: btree_lchain 	- check leaf page chain
**
** Description:
**      Leaf pages  have a sideways pointer (bt_nextpage), which point to the
**	next leaf page in the file.  DMF uses this for full or partial scans
**	of a table.
**
**	This routine walks that leaf page chain, starting at the first
**	leaf page (or leaf page containing the smallest key).  It expects every
**	page in the chain to be a leaf page, expects every leaf page in the
**	file to be referenced, and no leaf pages to be referenced more than once.
**
**	Also, it expects a continuity of keys -- ie, the first key on page
**	n+1 should be identical to the last key on page n.
**
** Inputs:
**      dm1u_cb                         Check Table Control Block
**
** Outputs:
**	none.
**
**	Returns:
**	    none
**	Exceptions:
**	    It will signal an exception if it cannot unfix a fixed page.
**	    Also, it calls dm1u_talk(), which may signal an exception if it
**		encounters an error
**
** Side Effects:
**	    leaf pages are fixed to rcb->rcb_data.page, and then
**	    unfixed.  It expects no fixed pages when called.
**
** History:
**      08-nov-1989  (teg)
**          initial creation.
**	13-jun-1990 (teresa)
**	    add support for user interrupts by calling fix_page to check
**	    for user interrupts then call dm0p_fix_page.  It used to call
**	    dm0p_fix_page directly.
**	29-jan-1991 (bryanp)
**	    Added support for Btree index compression.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	30-feb-1993 (rmuth)
**	    Add dm1u_cb parameter to some talk calls missed.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Project:
**		Change page header references to use macros.
**	    Added page_size parameter to dm1cxlog_error routine.
**      03-june-1996 (stial01)
**          Added DMPP_TUPLE_INFO argument to dm1cxget() 
**      12-jun-97 (stial01)
**          Pass tlv to dm1cxget instead of tcb.
[@history_template@]...
*/
static DB_STATUS
btree_lchain(
DM1U_CB		*dm1u,
DB_ERROR	*dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    i4	    	    leafctr=0;
    i4	    	    leaf;
    DM_PAGENO	    thisleaf;
    DMPP_PAGE	    *leafpage;
    i4		    havekey = FALSE;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    i4	    	    compare=0;
    i4		    KeyBufSize;
    char	    *KeyBuf, *AllocKbuf, *TempKey;
    char	    key_buf[DM1B_MAXSTACKKEY];
    char	    *key_ptr;
    i4	    	    klen;
    DB_STATUS	    stat;
    i4		    line;
    DB_STATUS	    status;

    CLRDBERR(dberr);

    /* clear the bit map */
    clean_bmap(dm1u->dm1u_size, (PTR)dm1u->dm1u_chain);

    if ((leaf = dm1u->dm1u_1stleaf) == 0)
    {
	/* we do not know where to start walking the leaf chain. */
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u, W_DM5017_NO_1ST_LEAF, 0);
	return (E_DB_OK);
    }

    /* Manufacture a Key buffer with room for two keys */
    KeyBufSize = DB_ALIGN_MACRO(t->tcb_klen);

    if ( stat = dm1b_AllocKeyBuf(2 * KeyBufSize, key_buf,
				&KeyBuf, &AllocKbuf, dberr) )
	return(stat);

    TempKey = KeyBuf + KeyBufSize;

    while (leaf)
    {
	thisleaf = leaf;
	status = fix_page(dm1u, leaf, DM0P_READ | DM0P_NOREADAHEAD,
	    &rcb->rcb_data, dberr);
	if (status != E_DB_OK)
	    break;
	leafpage = rcb->rcb_data.page;

	/* make sure this is a leaf page. */

	if (!(DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, leafpage) & 
	    DMPP_LEAF))
	{
	    /* this is not a leaf page */
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM503F_NOT_A_LEAF, 2, sizeof(leaf), &leaf);
	    break;
	}
	
	/* make sure we're not in a circular chain. */

	if (CHECK_CHAIN_MACRO(dm1u, leaf))
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5019_LEAF_CHAIN_DUPLICATE, 2, sizeof(leaf),&leaf);
	    break;
	}
	SET_CHAIN_MACRO(dm1u, leaf);
	leafctr++;

	/*  Double check that we found the leaf by descending the index. */

	if (CHECK_MAP_MACRO(dm1u, leaf) == 0)
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM5019_LEAF_CHAIN_DUPLICATE, 2, sizeof(leaf),&leaf);
	}

	/* test that the last key on previous page is identical to the first
	** key on this page */

	if ( DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, leafpage) )
	{
	    /* page is not empty */
	    if ( havekey )
	    {
		key_ptr = TempKey;
		klen = t->tcb_klen;
		stat = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				leafpage, &t->tcb_leaf_rac,
				(i4)0, &key_ptr, 
			    (DM_TID *)NULL, (i4*)NULL,
			    &klen, NULL, NULL, rcb->rcb_adf_cb);
		if (stat == E_DB_WARN 
			&& (t->tcb_rel.relpgtype != TCB_PG_V1) )
		    stat = E_DB_OK;
		if (stat != E_DB_OK)
		{
		    i4	line_zero = 0;	/* for errmsg parameter */

		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, rcb, leafpage,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, 
			    line_zero);
		    dm1u_talk(dm1u, W_DM5059_BAD_KEY_GET, 4, sizeof(i4),
			    DM1B_VPT_ADDR_PAGE_PAGE_MACRO( t->tcb_rel.relpgtype,
			    leafpage),
			    sizeof(i4), &line_zero);
		    break;
		}
		adt_kkcmp((ADF_CB*) &dm1u->dm1u_adf.dm1u_cbadf, t->tcb_keys,
			  t->tcb_leafkeys, key_ptr, KeyBuf, &compare);
		if (DM1B_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, 
		    leafpage) & DMPP_CHAIN) 
		    if (compare != DM1B_SAME)
		    {
			dm1u->dm1u_e_cnt++;
			dm1u_talk(dm1u,W_DM5040_KEY_HOLE, 2, sizeof(leaf), &leaf);
		    }
	    }

	    line = DM1B_VPT_GET_BT_KIDS_MACRO(t->tcb_rel.relpgtype, leafpage) - 1;
	    if (((i4)(dm1bvpt_kidoff_macro(t->tcb_rel.relpgtype,leafpage, line))
				>= DM1B_VPT_OVER(t->tcb_rel.relpgtype)) &&
	      ((i4)(dm1bvpt_kidoff_macro(t->tcb_rel.relpgtype, leafpage, line))
				<= t->tcb_rel.relpgsize))
	    {
		key_ptr = KeyBuf;
		klen = t->tcb_klen;
		stat = dm1cxget(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
			    leafpage, &t->tcb_leaf_rac,
			    line,
			    &key_ptr, (DM_TID *)NULL, (i4*)NULL,
			    &klen, 
			    NULL, NULL, rcb->rcb_adf_cb);
		if (stat == E_DB_WARN 
			&& (t->tcb_rel.relpgtype != TCB_PG_V1) )
		    stat = E_DB_OK;
		if (stat != E_DB_OK)
		{
		    dm1cxlog_error( E_DM93E3_BAD_INDEX_GET, rcb, leafpage,
			    t->tcb_rel.relpgtype, t->tcb_rel.relpgsize, line);
		    dm1u_talk(dm1u, W_DM5059_BAD_KEY_GET, 4, sizeof(i4),
			    DM1B_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			    leafpage),
			    sizeof(i4), &line);
		    break;
		}
		if (key_ptr != KeyBuf)
		    MEcopy(key_ptr, klen, KeyBuf);
		havekey = TRUE;
	    }
	}

	/* make sure the next guy in the chain is legal */

	if (DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, leafpage) > 
	    dm1u->dm1u_lastpage)
	{
	    dm1u->dm1u_e_cnt++;
	    dm1u_talk(dm1u,W_DM501A_BAD_LINK_IN_CHAIN, 2, sizeof(leaf), &leaf);
	    break;
	}

	leaf = DM1B_VPT_GET_BT_NEXTPAGE_MACRO(t->tcb_rel.relpgtype, leafpage);

	unfix_page (dm1u, &rcb->rcb_data);

    }	/* end loop for each leaf page in chain */

    /* Discard any allocated key buffer */
    if ( AllocKbuf )
	dm1b_DeallocKeyBuf(&AllocKbuf, &KeyBuf);

    /* see if there is a fixed page.  If so, unfix it */
    if (rcb->rcb_data.page)
	unfix_page (dm1u, &rcb->rcb_data);

    /* see if any leaf pages were left out of the chain */

    if ( leafctr < dm1u->dm1u_numleafs )
    {
	i4	tmp2 = dm1u->dm1u_numleafs - leafctr;

	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM501B_UNREF_LEAF_FROM_CHAIN, 2, sizeof(tmp2), &tmp2);
    }
    else if ( leafctr > dm1u->dm1u_numleafs )
    {
	i4	tmp = leafctr - dm1u->dm1u_numleafs;	
	dm1u->dm1u_e_cnt++;
	dm1u_talk(dm1u,W_DM5041_MULTIREF_LEAFS, 2, sizeof(tmp), &tmp);
    }
    return(E_DB_OK);
}

/*{
** Name: check_orphan	- find/process orphan pages.
**
** Description:
**      this routine walks the bit map, looking for unreferenced pages.  If
**	it finds an unreferenced page, it reports the page as an orphan, then
**	calls dm1u_dchain to check that data page and any data pages chained to
**	it.
**
**	The calling routine must assure that there are not any fixed pages when
**	this routine is called.  This routine does not fix any pages itself, but
**	the subordinate routine dm1u_dchain does fix/unfix pages. When this 
**	routine exits, there will not be any fixed pages.
**
** Inputs:
**      rcb                             Record Control Block.  Contains
**	    rcb_tcb_ptr			    pointer to Table Control Block
**		tcb_rel				copy of iirelation tuple for table
**		    relspec			    table storage structure
**		    relmain			    number of main pages in file
**	dm1u_cb				Patch/Check Table Control Block
**	    dm1u_map			    map of pages already referenced 
**
** Outputs:
**	none
**
**	Returns:
**	    none.
**	Exceptions:
**	    this routine calls talk, which will signal an exception if it
**	    encounters an error communicating with the FE program via SCF message
**	    interface.
**
** Side Effects:
**	    subordinate routines will clobber the value of
**	    rcb->rcb_data.page, so calling routine should not have any fixed
**	    data pages.
**
** History:
**      24-oct-89 (teg)
**	    initial creation.
**	28-feb-1992 (bryanp)
**	    B41841: It's OK for a totally empty data page to be orphaned in a
**	    Btree. This is a normal occurrence in Btrees and does not indicate
**	    corruption.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	26-oct-1992 (jnash)
**	    Reduced logging project.  Substitute dmpp_tuplecount for 
**	    dmpp_isempty().
**	30-feb-1993 (rmuth)
**	    Orphaned isam index pages where being grouped into illegal
**	    page type, add code to recognise these page types.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as the page_size argument to dmpp_tuplecount.
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Change page header references to use macros.
[@history_template@]...
*/
static DB_STATUS
check_orphan(
DM1U_CB		*dm1u_cb,
DB_ERROR	*dberr)
{
    DMP_RCB	    *rcb = dm1u_cb->dm1u_rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DM_PAGENO	    exp_page_main;
    u_i4	    i;
    DB_STATUS	    status;
    i4		    type;

    CLRDBERR(dberr);

    /*	Mark that we are processing orphans. */

    dm1u_cb->dm1u_state |= DM1U_S_ORPHAN;
    rcb->rcb_state |= RCB_READAHEAD;

    /* Loop for every page in table to look for orphans. */

    for (i = 0; i <= dm1u_cb->dm1u_lastpage; i++)
    {
	if (!CHECK_MAP_MACRO(dm1u_cb, i))
	{
	    status = fix_page(dm1u_cb, (DM_PAGENO) i, DM0P_READ,
		&rcb->rcb_data,  dberr);
	    if (status == E_DB_OK)
	    {
		DMPP_PAGE   *page = rcb->rcb_data.page;

		if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & 
		    DMPP_DATA)
		{
		    if (t->tcb_rel.relspec == TCB_BTREE &&
			(*t->tcb_acc_plv->dmpp_tuplecount)(page,
				t->tcb_rel.relpgsize) == 0 ) 
			type = DM1U_9EMPTYDATA;
		    else
			type = DM1U_1DATAPAGE;
		}
		else if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
		   		page) & DMPP_FREE)
		    type = DM1U_2FREEPAGE;
		else if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
				page) & DMPP_LEAF)
		    type = DM1U_3LEAFPAGE;
		else if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
				page) & DMPP_CHAIN)
		     type = DM1U_4LEAFOVFL;
		else if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
				page) & DMPP_INDEX)
		    type = DM1U_5INDEXPAGE;
		else if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
				page) & DMPP_FHDR)
		    type = DM1U_6FHDRPAGE;
		else if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
				page) & DMPP_FMAP)
		    type = DM1U_7FMAPPAGE;
		else if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype,
				page) & DMPP_DIRECT )
		    type = DM1U_10ISAM_INDEX;
		else
		    type = DM1U_8UNKNOWN;

		unfix_page(dm1u_cb, &rcb->rcb_data);
	    }
	    else
	    {
		/*  Read pass highwater mark? */

		if (dberr->err_code == E_DM9206_BM_BAD_PAGE_NUMBER)
		    return (E_DB_OK);
		return (E_DB_ERROR);
	    }

	    switch (type)
	    {
	    case DM1U_2FREEPAGE:
	    case DM1U_6FHDRPAGE:
	    case DM1U_7FMAPPAGE:
	    case DM1U_9EMPTYDATA:
		/*  These are expected and are not orphans. */
		continue;

	    case DM1U_1DATAPAGE:
		dm1u_talk(dm1u_cb,W_DM500C_ORPHAN_DATA_PAGE, 2, sizeof(i),
		    &i);
		break;

	    case DM1U_3LEAFPAGE:
		dm1u_talk(dm1u_cb,W_DM5021_ORPHAN_BTREE_LEAF_PG, 2, 
		    sizeof(i), &i);
		break;

	    case DM1U_4LEAFOVFL:
		dm1u_talk(dm1u_cb,W_DM5022_ORPHAN_BTREE_OVFL_PG, 2, 
		    sizeof(i), &i);
		break;

	    case DM1U_5INDEXPAGE:
		dm1u_talk(dm1u_cb,W_DM5023_ORPHAN_BTREE_INDEX, 2, 
		    sizeof(i), &i);
		break;

	    case DM1U_10ISAM_INDEX:
		dm1u_talk(dm1u_cb,W_DM5057_ORPHAN_ISAM_INDEX_PG, 2, 
		    sizeof(i), &i);
		break;

	    case DM1U_8UNKNOWN:
		dm1u_talk(dm1u_cb,W_DM5025_ORPHAN_ILLEGAL_PG, 2, 
		    sizeof(i), &i);
		break;
	    }
	}   /* end if -- we found an orphan */
    }	/* end loop for each page */

    return (E_DB_OK);
}

/*{
** Name: data_page  - Validate/fix a data page
**
** Description:
**
**      This routine checks the fields on a data page.  If they are incorrect,
**	it will (mode permitting) take a corrective action.  The mode criteria
**	is as follows:  DM1U_CHECK, DM1U_STRICT (PATCH), DM1U_FORGIVING (PATCH).
**
**	This routine does not check/process the page_main or page_ovfl fields.
**	The calling routine must do that, because the interpretations of these
**	fields change with the type of storage structure.  (In the case of the
**	patch operation, these will be modified to look like heap by the calling
**	routine, so the current values are not important.)
**
**	This routine does not check the page_seq_number, page_tran_id or 
**	page_log_addr fields because these are dynamic fields that are only
**	meaningful to DMF when the page is in memory.
**
**	This routine checks/patches page_next_line and page_line_tab via a call
**	to dmpp_check_page().  If the page is structurally unsound and
**	dm1u_datapg is doing a patch, it calls dm1c_format to reformat the page
**	as an empty data page.  If the page is structurally unsound and the
**	check operation is being performed, there is nothing more dm1u_datapg
**	can do on this page.  It will exit.
**	
**	However, if the page is structurally sound, then dm1u_datapg will enter
**	a loop to get each record from the page (one at a time) and then call
**	adf to validate each attribute in the tuple.  If a single attribute
**	is invalid, it will ask ADF for an "empty" data value for that tuple.
**	If more than one attribute in the record is invalid, it will delete
**	the whole tuple by zeroing the page_line_tab entry pointing to that
**	tuple.  It will not compress the other tuples on the page, because it
**	does not know if this page_line_tab entry or other entries are correct.
**
** Inputs:
**      dm1u_cb                         check/patch operation control block
**      page                            a fixed data page
** Outputs:
**	page				some/all of the fields on the page
**					    may be modified.
**	Returns:
**	    none
**	Exceptions:
**	    This routine calls talk, which will declare an exception if
**	    it encounters an error sending a message via SCF.
**
** Side Effects:
**	    Some of the page_line_tab[] members may be zeroed. Also, the whole
**	    page may be reformatted to an empty data page.
**
** History:
**      8-30-89 (teg)
**	    Initial Creation.
**	19-nov-90 (rogerk)
**	    Initialize the server's timezone setting in the adf control block.
**	    Also fixed place where collation sequence should have also been
**	    reset in the adf control block.
**	    This was done as part of the DBMS Timezone changes.
**	29-mar-91 (teresa)
**	    Fix alignment problem in call to dm1c_get
**	8-aug-1991 (bryanp)
**	    B37626,B38241: If this is an alignment-sensitive machine, copy the
**	    column's value to properly-aligned storage before calling ADF.
**	14-sep-1992 (bryanp)
**	    B45176: page_version is unpredictable on converted R5 tables. This
**	    subroutine used to check page->page_version and issue an error if
**	    it wasn't 0. However, converted release 5 Btree tables have a 60
**	    in this field, and other converted release 5 tables may have other
**	    unknown contents. Since it doesn't seem like an enormously
**	    important check, we've decided just to take this check out. This
**	    means that message DM5003 is no longer issued here.
**      25-sep-1992 (stevet)
**           Get default timezone from TMtz_init() instead of TMzone().
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
**	26-oct-1992 (jnash)
**	    Reduced loggin project.  Call tuple level vectors to perform 
**	    compression operations.
**      03-nov-92 (jnash)
**          Reduced logging project.  Pass line number instead of tid to 
**	    dmpp_reclen.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tcb_rel.relpgsize as page_size argument to dmpp_check_page,
**		dmpp_reclen.
**	06-may-1996 (thaju02)
**	    New Page Format Project:
**		Change page header references to use macros.
**	20-may-1996 (ramra01)
**	    Added DMPP_TUPLE_INFO argument to the get accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      18-jul-1996 (ramra01 for bryanp)
**          Extract the row's version and pass it to the uncompress routine.
**          Pass tcb_rel.relversion to dmpp_put.
**	13-sep-1996 (canor01)
**	    Add rcb_tupbuf to dmpp_uncompress call to pass buffer for data
**	    uncompression.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space
**          Put accessor: changed DMPP_TUPLE_INFO param to table_version
**	13-aug-99 (stephenb)
**	    alter uncompress calls to new proto.
**      16-mar-2004 (gupsh01)
**          Modified dm1r_cvt_row to include adf control block in 
**	    parameter list.
**      14-Jul-2004 (hanal04) Bug 112283 INGSRV2819
**          We must ensure the coupon passed to adc_valchk() holds a valid
**          rcb not the one that was in use when the coupon last hit disk.
**          This prevents SEGV in dmpe_get().
**          Also added W_DM5064_BAD_ATTRIBUTE as a verbose form of
**          W_DM5006_BAD_ATTRIBUTE which prints the error text generated
**          by the spatial routines in adf/ads.
**	7-Sep-2004 (schka24)
**	    Remove coupon part of above, coupon's don't hold RCB's any more
**	    and dmpe_get doesn't need the coupon tcb.
**	11-May-2007 (gupsh01)
**	    Initialize adf_utf8_flag.
**	15-Apr-2010 (kschendel) SIR 123485
**	    Short-term part of DMF coupon changed, fix here.
*/
static VOID
data_page(
DM1U_CB		*dm1u,
DMPP_PAGE	*page,
DB_ERROR	*dberr)
{
    DM1U_ADF	    *adf = (DM1U_ADF *) &dm1u->dm1u_adf;
    i2		    mask;
    i4		    valstat;
    DB_STATUS	    status;
    DMP_RCB	    *rcb = (DMP_RCB *) dm1u->dm1u_rcb;
    DMP_TCB	    *t = rcb->rcb_tcb_ptr;
    DMP_ROWACCESS   *rac = rcb->rcb_data_rac;
    i4	    tup_width;
    i4	    uncompressed_length;
    DMPE_COUPON		*cpn;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);
 
    dm1u_talk(dm1u,I_DM53FE_DATA_PAGE, 2,
	sizeof(DM_PAGENO),
	DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page));

    mask = ~(DMPP_DATA | DMPP_MODIFY | DMPP_ASSOC | DMPP_PRIM);
    if (t->tcb_rel.relspec == TCB_ISAM)
	mask &= ~DMPP_CHAIN;
    else if (t->tcb_rel.relspec == TCB_BTREE)
	mask &=  ~(DMPP_ASSOC | DMPP_PRIM);
    if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & mask)
    {
	/* this condition should only be detected on the check operation, as
	**  the patch operation will use page->page_stat to drive whether or
	**  not this is a data page.
	*/
/* -- skip this test for now-- it doesnt handle hash main pages that have
**    overflow pages
	dm1u_talk(dm1u,W_DM5002_BAD_DATA_PAGESTAT,2,sizeof(pg_num),&pg_num);
*/
    }

    valstat = (*t->tcb_acc_plv->dmpp_check_page)(dm1u, page,
		    t->tcb_rel.relpgsize);
    if (valstat == E_DB_ERROR)
    {
	/* structural damage has occurred to the page.  Report the damage.
	** If we are in either patch mode, then zap the page.
	*/
	if ((dm1u->dm1u_op & DM1U_OPMASK) != DM1U_VERIFY)
	{
	    (*t->tcb_acc_plv->dmpp_format)(t->tcb_rel.relpgtype, 
		t->tcb_rel.relpgsize, page,
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page),
		(DMPP_DATA | DMPP_FREE),  DM1C_ZERO_FILL);
	    dm1u_talk(dm1u,I_DM5204_STRUCTURAL_DAMAGE, 2,
		 sizeof(DM_PAGENO),
		 DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page));
	}
	else
	{
	    dm1u_talk(dm1u,W_DM5004_STRUCTURAL_DAMAGE, 2,
		sizeof(DM_PAGENO),
		DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page));
	    dm1u->dm1u_e_cnt++;
	}
    }
    else
    {
	/* the page was either good or was fixed.  In either case, enter a
	**  loop to examine with each tuple on the page 
	*/
#define	MAXBAD	    DB_MAX_COLS
        /* max number of bad attributes allowed in a tuple--
	** currently set to the max number of attributes possible
	** in a tuple cuz the design review decided to take out
	** the logic to throw away whole tuple if too many
	** pieces are bad.  However, I am leaving this in
	** as a no-op incase we wish to get back the toss whole
	** tuple if too many attributes are bad logic...
	*/
	i4	    badcount;	    /* number of bad attributes in the tuple */
	i4	    i,j;
	DM_TID	    localtid;       /* Tid used for getting data. */
	char	    *rec_ptr;
	char	    *crec;	    /* ptr to  compressed record */
	i4     crec_len;	    /* lenght of record when compressed */
	i4	    page_next_line;
	i4	    row_version = 0;	
	i4	    *row_ver_ptr;

	if (t->tcb_rel.relversion)
	    row_ver_ptr = &row_version;
	else
	    row_ver_ptr = NULL;

	if (valstat != E_DB_OK)
	    dm1u_talk(dm1u,I_DM5202_FIXED_DATA_PAGE, 2, 
		sizeof(DM_PAGENO),
		DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page));

	localtid.tid_tid.tid_page = 
	    DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page);
	page_next_line = 
	    DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(t->tcb_rel.relpgtype, page);
	for (i=0; i<page_next_line; i++)
	{
	    /* 
	    ** get a tuple from the page
	    ** FIX ME need more verifydb changes if DMPP_VPT_PAGE_HAS_SEGMENTS
	    */

	    localtid.tid_tid.tid_line = i;
	    tup_width = t->tcb_rel.relwid;
	    status = (*t->tcb_acc_plv->dmpp_get)(t->tcb_rel.relpgtype,
		t->tcb_rel.relpgsize, page, &localtid, 
		&tup_width, &rec_ptr, row_ver_ptr, NULL, NULL, (DMPP_SEG_HDR *)NULL);

	    if (status == E_DB_WARN)
		continue;	/* No tuple at TID */

	    /* Additional processing if compressed, altered, or segmented */
	    if (status == E_DB_OK && 
		(rac->compression_type != TCB_C_NONE || 
		row_version != t->tcb_rel.relversion ||
		t->tcb_seg_rows))
	    {
		status = dm1c_get(rcb, page, &localtid, 
			(char *)&adf->dm1u_record[0], dberr);
		rec_ptr = (char *)&adf->dm1u_record[0];
	    }

	    if (status != E_DB_OK)
	    {
    		dm1u->dm1u_e_cnt++;
		dm1u_talk(dm1u,W_DM5005_INVALID_TID, 4, 
		    sizeof(DM_PAGENO),
		    DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page),
		    sizeof (i), &i);
		if ((dm1u->dm1u_op & DM1U_OPMASK) != DM1U_VERIFY)
		{
		    i4 del_size;
		    /* Delete the tuple */
		    (*t->tcb_acc_plv->dmpp_reclen)
				(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				page, localtid.tid_tid.tid_line, &del_size);
		    (*t->tcb_acc_plv->dmpp_delete)
				(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				page, (i4)DM1C_DIRECT, (i4)TRUE,
				&rcb->rcb_tran_id, (u_i2)0, &localtid, del_size);
		    dm1u_talk(dm1u,I_DM5205_INVALID_TID, 4, 
			sizeof(DM_PAGENO),
			DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, page),
			sizeof (i), &i);
		}
		continue;
	    }

	    /*
	    ** DM1C_GET always returns a pointer onto the page.  
	    ** We need to copy the tuple to the buffer and reset rec_ptr 
	    ** to point to that buffer (unless we have already done so as
	    ** a side effect of uncompressing the tuple).
	    */
	    if ( rec_ptr != (char *) &adf->dm1u_record[0])
	    {
		MEcopy((PTR) rec_ptr, t->tcb_rel.relwid,
		    (PTR) &adf->dm1u_record[0]);
		rec_ptr = (char *) &adf->dm1u_record[0];
	    }

	    badcount = 0;
	    for (j = 0; j < adf->dm1u_numdv; j++)
	    {
		if (t->tcb_atts_ptr[j+1].ver_dropped != 0)
		   continue;

		/* enter a loop for each attribute in the tuple.  Ask ADF to
		** validiate the tuple.  If it is invalid, then clear it
		*/
#ifdef BYTE_ALIGN
		/*
		** ADF requires properly aligned operands. Copy the column's
		** value to the properly aligned buffer in the DB_DATA_VALUE:
		*/
		MEcopy( (PTR) &adf->dm1u_record[
				t->tcb_atts_ptr[j+1].offset],
			adf->dm1u_dv[j].db_length,
			adf->dm1u_dv[j].db_data);
#endif
		if(t->tcb_atts_ptr[j+1].flag & ATT_PERIPHERAL)
		{
		    cpn = (DMPE_COUPON *) &((ADP_PERIPHERAL *)adf->dm1u_dv[j].db_data)->per_value.val_coupon;
		    /* Fill in short term things that dmpe-get wants. */
		    cpn->cpn_base_id = t->tcb_rel.reltid.db_tab_base;
		    cpn->cpn_att_id = j+1;
		    cpn->cpn_flags = 0;
		}

		status = adc_valchk(&adf->dm1u_cbadf, &(adf->dm1u_dv[j]));
		if (status == E_DB_ERROR)
		{
		    if ((adf->dm1u_cbadf.adf_errcb.ad_errcode == 
			E_AD1014_BAD_VALUE_FOR_DT) ||
                        ((adf->dm1u_cbadf.adf_errcb.ad_errcode == 0) &&
                           ((adf->dm1u_cbadf.adf_errcb.ad_usererr 
				& E_SPATIAL_MASK) == E_SPATIAL_MASK)))
		    {
			dm1u->dm1u_e_cnt++;
                        if((adf->dm1u_cbadf.adf_errcb.ad_usererr 
			       & E_SPATIAL_MASK) == E_SPATIAL_MASK)
                        {
			    dm1u_talk(dm1u,W_DM5064_BAD_ATTRIBUTE, 8,
			     sizeof(j), &j,
			     sizeof(DM_PAGENO),
			     DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			     page),
			     sizeof (i), &i,
                             STlength(adf->dm1u_cbadf.adf_errcb.ad_errmsgp),
                             adf->dm1u_cbadf.adf_errcb.ad_errmsgp);
                        }
                        else
                        {
			    dm1u_talk(dm1u,W_DM5006_BAD_ATTRIBUTE, 6,
			     sizeof(j), &j,
			     sizeof(DM_PAGENO),
			     DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			     page),
			     sizeof (i), &i);
                        }
			if ((dm1u->dm1u_op & DM1U_OPMASK) == DM1U_VERIFY)
			    continue;
			if ( (t->tcb_rel.relstat & TCB_UNIQUE) ||
			     ((t->tcb_rel.relstat & TCB_DUPLICATES) == 0))
			{
			    /* either duplicate keys or duplicate tuples are not
			    ** permitted.  We cannot take the chance of clearing
			    ** a single attribute, cuz that may make a duplicate
			    ** with some other tuple that nas a null for this
			    ** attribute.  Therefore, throw away the whole
			    ** tuple.
			    */
			    i4 del_size;
			    (*t->tcb_acc_plv->dmpp_reclen)
				(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				page, localtid.tid_tid.tid_line, &del_size);
			    (*t->tcb_acc_plv->dmpp_delete)
				(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				page, (i4)DM1C_DIRECT, (i4)TRUE, 
				&rcb->rcb_tran_id, (u_i2)0, &localtid, del_size);
			    dm1u_talk(dm1u,I_DM5206_BAD_ATTRIBUTE, 
				sizeof(j), &j,
				sizeof(DM_PAGENO),
				DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
				page),
				sizeof(i), &i);
			    continue;
			}			
			else if (badcount < MAXBAD)
			{
			    /* re-initialize adf scb */
			    MEfill ( sizeof(ADF_CB), '\0',
				      (PTR) &adf->dm1u_cbadf);
			    adf->dm1u_cbadf.adf_maxstring = DB_MAXSTRING;
                            adf->dm1u_cbadf.adf_collation = rcb->rcb_collation;
			    adf->dm1u_cbadf.adf_ucollation = rcb->rcb_ucollation;
			    adf->dm1u_cbadf.adf_uninorm_flag = 0;
        		    if (CMischarset_utf8())
     			      adf->dm1u_cbadf.adf_utf8_flag = AD_UTF8_ENABLED;
    			    else
      			      adf->dm1u_cbadf.adf_utf8_flag = 0;

                            adf->dm1u_cbadf.adf_tzcb = tmtz_cb;

			    /* the data in this attribute is invalid.  Try to 
			    ** replace it with an "empty" value 
			    */
			    badcount++;
			    status = adc_getempty( &adf->dm1u_cbadf, 
					    &(adf->dm1u_dv[j]) );
			    if (status != E_DB_OK)
			    {
				dm1u_talk(dm1u,W_DM5007_ADF_ERROR, 8,
				    sizeof(adf->dm1u_cbadf.adf_errcb.ad_errcode),
				    &adf->dm1u_cbadf.adf_errcb.ad_errcode,
				    sizeof(j), &j, sizeof(i), &i, 
				    sizeof(DM_PAGENO),
				    DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
				    page));
			    }

			    if (rac->compression_type != TCB_C_NONE)
			    {
				/* tuple needs to be compressed then put
				** on the page.
				*/
				crec = dm1u->dm1u_tupbuf;
				status = (*rac->dmpp_compress)
					(rac,
					rec_ptr,
					(i4)t->tcb_rel.relwid, 
					crec, &crec_len);
				if (status != E_DB_OK)
				{
				    dm1u_talk(dm1u,E_DM9107_BAD_DM1U_COMPRESS, 0);
				    if ((dm1u->dm1u_op & DM1U_OPMASK)
					== DM1U_PATCH)
				    {
		/*	    <<<<<<<<<<<<<   			*/
			    /* Delete the duff tuple */
			    i4 del_size;
			    (*t->tcb_acc_plv->dmpp_reclen)
				(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				page, localtid.tid_tid.tid_line, &del_size);
			    (*t->tcb_acc_plv->dmpp_delete)
				(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				page, (i4)DM1C_DIRECT, (i4)TRUE, 
				&rcb->rcb_tran_id, (u_i2)0, &localtid, del_size); 
		/*	    >>>>>>>>>>>>>   			*/
			    dm1u_talk(dm1u,I_DM5205_INVALID_TID, 4, 
				sizeof(DM_PAGENO),
				DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
				page),
				sizeof (i), &i);
				    }
				    continue;
				}
				(*t->tcb_acc_plv->dmpp_put)
				    (t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				    page, (i4)DM1C_DIRECT, 
				    &rcb->rcb_tran_id, (u_i2)0, &localtid, 
				    crec_len, crec,
				    t->tcb_rel.relversion, 
				    (DMPP_SEG_HDR *)NULL); /* FIX ME */
			    }
			    else
			    {
				/* tuple is not compressed, just put it back
				** on to the page
				*/
				(*t->tcb_acc_plv->dmpp_put)
				    (t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				    page, (i4)DM1C_DIRECT, 
				    &rcb->rcb_tran_id, (u_i2)0, &localtid,
				    (i4)(t->tcb_rel.relwid), 
				    rec_ptr,
				    t->tcb_rel.relversion,
				    (DMPP_SEG_HDR *)NULL); /* FIX ME */
			    }
			    continue;
			}
			else
			{
			    /* there are too many bad attributes in this tuple.
			    ** therefore delete it */
			    i4 del_size;
			    dm1u_talk(dm1u,W_DM5008_TOO_MANY_BAD_ATTS, 4,
				sizeof(DM_PAGENO),
				DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
				page),
				sizeof (i), &i);
			    if ((dm1u->dm1u_op & DM1U_OPMASK) == DM1U_VERIFY)
				continue;
			    (*t->tcb_acc_plv->dmpp_reclen)
				(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				page, localtid.tid_tid.tid_line, &del_size);
			    (*t->tcb_acc_plv->dmpp_delete)
				(t->tcb_rel.relpgtype, t->tcb_rel.relpgsize,
				page, (i4)DM1C_DIRECT, (i4)TRUE, 
				&rcb->rcb_tran_id, (u_i2)0, &localtid, del_size);
    			    dm1u_talk(dm1u,I_DM5208_TOO_MANY_BAD_ATTS, 4,
				sizeof(DM_PAGENO),
				DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
				page),
				sizeof (i), &i);
			}
		    }
		    else
		    {
			/* we have an unexpected adf error */
			dm1u_talk(dm1u,W_DM5007_ADF_ERROR, 8, 
			    sizeof (adf->dm1u_cbadf.adf_errcb.ad_errcode),
			    &adf->dm1u_cbadf.adf_errcb.ad_errcode, sizeof(j),
			    &j, sizeof(i), &i,
			    sizeof(DM_PAGENO),
			    DMPP_VPT_ADDR_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
				page));
			continue;
		    }
		} /* endif this attribute is valid */
	    }	/* end of loop for each attribute in the tuple */
	} /* end of loop for each tuple on the page */
    }
    return;
}

/*{
** Name: fix_page	- fix a page after check for user interupts
**
** Description:
**      
**      This routine checks for user interrupts or force aborts.  If neither
**	has occurred, (the usual case) then it calls dm0p_fix_page to
**	fix the page.
**
** Inputs:
**	dm1u_cb				check/patch table control block
**      page_number                     The page number to fix.
**      action                          The specific fix action.
**
** Outputs:
**      pinfo->page                     The pointer to the fixed page.
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**          E_DB_FATAL
**      Exceptions:
**          none
**
** Side Effects:
**	    1 or more pages are fixed and unfixed.  If a user interrupt has
**	    occurred, then an error exit will be taken via a longjump (ie,
**	    signal an exception).  Otherwise, this routine will call
**	    dm0p_fix_page to fix the page and will return whatever status
**	    dm0p_fix_page gives it.
**
** History:
**      01-nov-89 (teg)
**          initial creation.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
[@history_template@]...
*/
static DB_STATUS
fix_page(
DM1U_CB		   *dm1u,
DM_PAGENO          page_number,
i4            action,
DMP_PINFO	   *pinfo,
DB_ERROR	   *dberr)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DB_STATUS	    status;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    /* 
    **	check to see if user interrupt occurred. 
    */

    if (*rcb->rcb_uiptr & SCB_USER_INTR)
    {
	dm1u_talk(dm1u,E_DM0065_USER_INTR, 0);
	dm1u->dm1u_errcode = E_DM0065_USER_INTR;
	EXsignal ( (EX) EX_JNP_LJMP, 1, dm1u);
    }

    /* 
    **	check to see if force abort occurred. 
    */

    if (*rcb->rcb_uiptr & SCB_FORCE_ABORT)
    {
	dm1u_talk(dm1u,E_DM010C_TRAN_ABORTED, 0);
	dm1u->dm1u_errcode = E_DM010C_TRAN_ABORTED;
	EXsignal ( (EX) EX_JNP_LJMP, 1, dm1u);
    }

    /* 
    **	not a user interrupt, and not a force abort, so go ask dm0p to fix
    **  the page and return whatever status/error_code dm0p_fix_page() gives
    */

    status = dm0p_fix_page(rcb, page_number, action, pinfo, dberr);
    if (status != E_DB_OK)
    {
	i4	    bad_page = -1;

	if ((dm1u->dm1u_state & DM1U_S_ORPHAN) == 0)
	{
	    if (dberr->err_code == E_DM9206_BM_BAD_PAGE_NUMBER)
		dm1u_talk(dm1u,W_DM5001_BAD_PG_NUMBER, 4, 
		   sizeof(page_number), &page_number,
		   sizeof(page_number), &bad_page);
	    if (dberr->err_code == E_DM9202_BM_BAD_FILE_PAGE_ADDR)
		dm1u_talk(dm1u,W_DM5001_BAD_PG_NUMBER, 4, 
		   sizeof(page_number), &page_number,
		   sizeof(dm1u->dm1u_lastpage), &dm1u->dm1u_lastpage);
	}
    }
    return (status);
}

/*{
** Name: unfix_page	- this routine unfixes a page that is fixed.
**
** Description:
**
**      this routine call dmpp_unfix_page to unfix a fixed page.  If an
**	error is returned, it ule_formats the error, informs the user of
**	the error  W_DM51FC_ERROR_PUTTING_PAGE, and then signals an exception.
**	(That effectively aborts the statement).
**
**	This routine assumes that the page is fixed.  It does not verify that
**	fact.  It is up to the caller to assure that pinfo->page has the address
**	of a valid DMPP_PAGE.
**
** Inputs:
**	page_num		the page number of the page being unfixed.
**	pinfo->			pointer to the fixed page.
**	dm1u_cb			check table control block.
**
** Outputs:
**	pinfo->page		the value is zeroed by dm0p_unfix_page
**
**	Returns:
**	    none.
**	Exceptions:
**	    will signal an exception if unable to unfix the page.
**
** Side Effects:
**	    none
**
** History:
**      01-nov-89   (teg)
**          initial creation.
**	16-jul-1992 (kwatts)
**	    Added a return statement.
**	23-oct-92 (teresa)
**	    add DM1U_CB parameter to dm1u_talk() call for sir 42498.
[@history_template@]...
*/
static void
unfix_page(
DM1U_CB		*dm1u,
DMP_PINFO	   *pinfo)
{
    DMP_RCB	    *rcb = dm1u->dm1u_rcb;
    DMP_TCB         *t = rcb->rcb_tcb_ptr;
    i4	    page_number = DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype,
			      pinfo->page);
    DB_STATUS	    status;
    DB_ERROR	    local_dberr;

    /* unfix the page */

    status = dm0p_unfix_page(rcb, DM0P_UNFIX, pinfo, &local_dberr);
    if (status != E_DB_OK)
    {
	dm1u_log_error(&local_dberr);
	dm1u_talk(dm1u,W_DM51FC_ERROR_PUTTING_PAGE, 2, 
	    sizeof(page_number), &page_number);
	dm1u->dm1u_errcode = W_DM51FC_ERROR_PUTTING_PAGE;
	EXsignal ( (EX) EX_JNP_LJMP, 1, dm1u);
    }
    return;
}

/*
** Name  handler    - exception handler for dm1u
**
** Description:
**
**	This handler will manage exceptions that occur during logic to 
**	check or patch a table's file structure.  The only "expected" 
**	exception is an error building an scf control block and requesting SCF
**	to send a message to the FE.  This routine will resignal any unexpected
**	exceptions.
**
**      The new DMF code will use the following rules for generating an
**      exception:
**              1.  It will not generate an exception while holding any
**                  resources (allocated memory, control blocks, etc) that
**                  it allocated/acquired unless that resource is described
**                  in the DM1U_CB.
**              2.  The routine that signals the exception will put the
**                  integer error code value into the dm1u_cb.
**
**      This exception handler is simply a long jump to dm1u_table, which
**      is where the logic to handle the exception is defined.  That logic
**      will do the following:
**              1.  deallocate/free all resources defined in the dm1u_cb.
**              2.  assign the error code to the value in the dm1u_cb.
**              3.  set status to E_DB_ERROR
**              4.  format/print the error message
**              5.  return control back to dm1u_table's caller.
**
** Inputs:
**      args			argument list from exception handler
**
** Outputs:
**      none
**      Returns:
**          EXDECLARE	        let declarer report message.
**	    EXRESIGNAL		not recognized, so resignal it.
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	22-aug-89 (teg)
**	    initial creation
**	08-jul-1993 (rog)
**	    Remove EX_DECLARE case because it won't happen, and EX_OK is not a
**	    valid return status from an exception handler.
[@history_line@]...
*/
static STATUS
handler(
EX_ARGS                 *args)
{
    STATUS	ret_status;

    if (DMZ_TBL_MACRO(1))
    {
	TRdisplay ("dm1u_handler: Called with args->exarg_num = %d\n",
		    (i4) args->exarg_num);
	TRdisplay ("  1=>EX_JNP_LJMP\n");
    }

    if (EXmatch(args->exarg_num, 1, EX_JNP_LJMP) == 1)
    {
	ret_status = EXDECLARE;  /* return to EXdeclare point */
    }
    else
    {
        ret_status = EXRESIGNAL;  /* resignal the unexpected exception */
    }

    return(ret_status);
}

/*
** Name: clean_bmap      - initialize chain bitmap
**
** History:
**      11-feb-1999 (thaju02)
**          Created. (B88922)
*/
static VOID
clean_bmap(
i4            size,
char          *map)
{
    i4   i = size, j = 0, k = 0;
 
    while (i > 0)
    {
        j = (i > MAXUI2 ? MAXUI2 : i);
        MEfill(j, '\0', (PTR)map + k);
        i -= j; k += j;
    }
}

/*
** Name: periph_check
**
** Description:
**	Check that peripheral data-type coupons match the record entries
**	in the extension tables. The following checks are performed
**
**	    - The etab table pointed to by the coupon exists
**	    - The coupon has corresponding etab segments in that table
**	    - The combined length of the etab segments is equal to the length
**	      stated in the coupon
**
**	The function depends upon the fact that the base table records are
**	accessible through the normal channels (dm2r_get), which means that
**	the base table must first be repaired with other verify operations if 
**	it is corrupted
**
** Inputs:
**	dm1u		verifydb info
**
** Outputs:
**	err_code	returned error
**
** Returns:
**	DB_STATUS	returned status
**
** History:
**	13-apr-99 (stephenb)
**	    Created
**      10-sep-99 (stial01)
**          Fixed args to adi_per_under
**      10-dec-99 (stial01)
**          Backout 10-sep-99 change to adi_per_under args
**	21-Oct-2009 (kschendel)
**	    Clear dberr when setting status to OK (found while integrating
**	    122739 rowaccessor stuff).
*/
static DB_STATUS
periph_check(
DM1U_CB		*dm1u,
DB_ERROR	*dberr)
{
    DB_STATUS		status;
    DMP_RCB	    	*rcb = dm1u->dm1u_rcb;
    DMP_TCB         	*t = rcb->rcb_tcb_ptr;
    DMP_DCB		*dcb = t->tcb_dcb_ptr;
    ADF_CB		*adf_cb = &dm1u->dm1u_adf.dm1u_cbadf;
    union {
	ADP_PERIPHERAL	coupon;
	char		buf[sizeof(ADP_PERIPHERAL) + 1];
    }			cbuf;    
    DMPE_COUPON		*dmf_coupon = 
			      (DMPE_COUPON *)&cbuf.coupon.per_value.val_coupon;
    DM2R_KEY_DESC	qual_list[3];
    char		*record;
    STATUS		cl_stat;
    DMP_RCB		*rel_rcb = NULL;
    DMP_RCB		*etab_rcb = NULL;
    DM_TID		tid;
    DMP_RELATION	relrecord;
    DMPE_RECORD		etab_record;
    DB_TEXT_STRING	*segment = (DB_TEXT_STRING *)etab_record.prd_user_space;
    DB_TAB_TIMESTAMP	timestamp;
    DB_STATUS		local_err;
    i4			curr_etab_id = 0;
    i4			zero = 0;
    i4			segno;
    i4			bad_segments;
    u_i4		blob_length;
    DB_DATA_VALUE	under_dv;
    DB_DATA_VALUE	coupon_dv;
    i4			dtinfo;
    i4			i;
    i4			null_value;
    DB_ERROR		local_dberr;
    i4		    *err_code = &dberr->err_code;

    CLRDBERR(dberr);

    record = MEreqmem(0, t->tcb_rel.relwid, TRUE, &cl_stat);

    if (record == NULL)
    {
	dm1u_talk(dm1u,E_DM9114_VERIFY_MEM_ERR, 0);
	*err_code = E_DM9114_VERIFY_MEM_ERR;
	return (E_DB_ERROR);
    }

    for(;;) /* to break from */
    {
	/*
	** allocate RCB for iirelation
	*/
	status = dm2r_rcb_allocate(dcb->dcb_rel_tcb_ptr, 
	    (DMP_RCB *)NULL, &rcb->rcb_tran_id, rcb->rcb_lk_id, 
	    rcb->rcb_log_id, &rel_rcb, dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
		NULL, 0, NULL, err_code, 0);
	    dm1u_talk(dm1u,E_DM911C_RCB_ALLOC_ERR, 0);
	    SETDBERR(dberr, 0, E_DM911C_RCB_ALLOC_ERR);
	    break;
	}
	rel_rcb->rcb_k_duration |= RCB_K_TEMPORARY;
	/* check for peripheral fields */
	for (i = 1; i <= t->tcb_rel.relatts; i++)
	{
	    if (t->tcb_atts_ptr[i].flag & ATT_PERIPHERAL)
	    {
		status = adi_per_under(adf_cb, t->tcb_atts_ptr[i].type, 
		    &under_dv);
		if (status != E_DB_OK)
		{
		    dm1u_talk(dm1u,E_DM911A_NO_UNDER_DV, 2, 
			t->tcb_atts_ptr[i].attnmlen,
			t->tcb_atts_ptr[i].attnmstr);
		    SETDBERR(dberr, 0, E_DM911A_NO_UNDER_DV);
		    status = E_DB_OK;
		    continue;
		}
		status = adi_dtinfo(adf_cb, under_dv.db_datatype, &dtinfo);
		if (status != E_DB_OK)
		{
		    dm1u_talk(dm1u,E_DM911B_NO_DT_INFO, 2, 
			t->tcb_atts_ptr[i].attnmlen,
			t->tcb_atts_ptr[i].attnmstr);
		    SETDBERR(dberr, 0, E_DM911B_NO_DT_INFO);
		    status = E_DB_OK;
		    continue;
		}
		/* peripheral data-type, check it out */
		status = dm2r_position(rcb, DM2R_ALL, NULL, 0,
				       NULL, dberr);
		if (status != E_DB_OK)
		{
		    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
			NULL, 0, NULL, err_code, 0);
		    dm1u_talk(dm1u,E_DM9113_PERPH_POS_ERR, 0);
		    SETDBERR(dberr, 0, E_DM9113_PERPH_POS_ERR);
		    break;
		}
		for (;;)
		{
		    status = dm2r_get(rcb, &tid, DM2R_GETNEXT, record, 
			dberr);
		    if (status != E_DB_OK)
		    {
			if (dberr->err_code == E_DM0055_NONEXT)
			{
			    /* no more records */
			    status = E_DB_OK;
			    CLRDBERR(dberr);
			    break;
			}
			else
			{
			    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				NULL, 0, NULL, err_code, 0);
			    dm1u_talk(dm1u,E_DM9115_PERPH_GET_ERR, 0);
			    SETDBERR(dberr, 0, E_DM9115_PERPH_GET_ERR);
			    break;
			}
		    }
		    MEcopy(record + t->tcb_atts_ptr[i].offset, 
			t->tcb_atts_ptr[i].length, cbuf.buf);
		    /* check for nulls */
		    coupon_dv.db_length = t->tcb_atts_ptr[i].length;
		    coupon_dv.db_datatype = t->tcb_atts_ptr[i].type;
		    coupon_dv.db_prec = t->tcb_atts_ptr[i].precision;
		    coupon_dv.db_data = cbuf.buf;
		    coupon_dv.db_collID = t->tcb_atts_ptr[i].collID;
		    status = adc_isnull(adf_cb, &coupon_dv, &null_value);
		    if (status != E_DB_OK)
		    {
			dm1u_talk(dm1u,E_DM911D_NO_NULL_INFO, 2, 
			    t->tcb_atts_ptr[i].attnmlen,
			    t->tcb_atts_ptr[i].attnmstr);
			SETDBERR(dberr, 0, E_DM911D_NO_NULL_INFO);
			status = E_DB_OK;
			continue;
		    }
		    if (null_value)
			continue;
		    if (dmf_coupon->cpn_etab_id <= 0)
		    {
			/* O.K. if no coupon length */
			if (cbuf.coupon.per_length1 != 0)
			{
			    /* table doesn't exist */
			    dm1u_talk( dm1u,W_DM505E_NO_ETAB_TABLE, 
				6, sizeof(dmf_coupon->cpn_etab_id),
				&dmf_coupon->cpn_etab_id,
				t->tcb_atts_ptr[i].attnmlen,
				t->tcb_atts_ptr[i].attnmstr,
				sizeof(rcb->rcb_currenttid.tid_i4),
				&rcb->rcb_currenttid.tid_i4);
			}
			status = E_DB_OK;
			continue;
		    }
		    else if (dmf_coupon->cpn_etab_id != curr_etab_id)
		    {
			/* check that etab table exists */
			/* position iirelation */
			qual_list[0].attr_number = DM_1_RELATION_KEY;
			qual_list[0].attr_operator = DM2R_EQ;
			qual_list[0].attr_value = 
			    (char *)&dmf_coupon->cpn_etab_id;
			qual_list[1].attr_number = DM_2_ATTRIBUTE_KEY;
			qual_list[1].attr_operator = DM2R_EQ;
			qual_list[1].attr_value = (char *)&zero;
			status = dm2r_position(rel_rcb, DM2R_QUAL, qual_list, 
			    (i4)2, (DM_TID *)NULL, dberr);
			if (status != E_DB_OK)
			{
			    if (dberr->err_code == E_DM0055_NONEXT)
			    {
				/* table doesn't exist */
				dm1u_talk( dm1u,W_DM505E_NO_ETAB_TABLE, 
				    6, sizeof(dmf_coupon->cpn_etab_id),
				    &dmf_coupon->cpn_etab_id, 
				    t->tcb_atts_ptr[i].attnmlen,
				    t->tcb_atts_ptr[i].attnmstr,
				    sizeof(rcb->rcb_currenttid.tid_i4),
				    &rcb->rcb_currenttid.tid_i4);
				CLRDBERR(dberr);
				status = E_DB_OK;
				continue;
			    }
			    else
			    {
				uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				    NULL, 0, NULL, err_code, 0);
				dm1u_talk(dm1u,E_DM9116_REL_POS_ERR, 0);
				SETDBERR(dberr, 0, E_DM9116_REL_POS_ERR);
				break;
			    }
			}
			/* try to get the record */
			status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT,
			    (char *)&relrecord, dberr);
			if (status != E_DB_OK)
			{
			    if (dberr->err_code == E_DM0055_NONEXT)
			    {
				/* table doesn't exist */
				dm1u_talk( dm1u,W_DM505E_NO_ETAB_TABLE, 
				    6, sizeof(dmf_coupon->cpn_etab_id),
				    &dmf_coupon->cpn_etab_id, 
				    t->tcb_atts_ptr[i].attnmlen,
				    t->tcb_atts_ptr[i].attnmstr,
				    sizeof(rcb->rcb_currenttid.tid_i4),
				    &rcb->rcb_currenttid.tid_i4);
				CLRDBERR(dberr);
				status = E_DB_OK;
				continue;
			    }
			    else
			    {
				uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				    NULL, 0, NULL, err_code, 0);
				dm1u_talk(dm1u,E_DM9117_REL_GET_ERR, 0);
				SETDBERR(dberr, 0, E_DM9117_REL_GET_ERR);
				break;
			    }
			}
			/* close previous etab */
			if (etab_rcb)
			{
			    status = dm2t_close(etab_rcb, (i4)0, dberr);
			    if (status != E_DB_OK)
			    {
				uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				    NULL, 0, NULL, err_code, 0);
				dm1u_talk(dm1u,E_DM9116_REL_POS_ERR, 0);
				SETDBERR(dberr, 0, E_DM9116_REL_POS_ERR);
				break;
			    }
			    etab_rcb = NULL;
			}
			curr_etab_id = dmf_coupon->cpn_etab_id;
			/* etab table exists, open it */
			status = dm2t_open(dcb, &relrecord.reltid, DM2T_X, 
			    DM2T_UDIRECT, DM2T_A_READ, (i4)0, 50, (i4)0, 
			    rcb->rcb_log_id, rcb->rcb_lk_id, (i4)0, (i4)0, 
			    (i4)0, &rcb->rcb_tran_id, &timestamp, &etab_rcb, 
			    (DML_SCB *)NULL, dberr);
			if (status != E_DB_OK)
			{
			    uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				    NULL, 0, NULL, err_code, 0);
			    dm1u_talk(dm1u,E_DM9118_ETAB_OPEN_ERR, 2, 
				sizeof(relrecord.reltid.db_tab_base), 
				&relrecord.reltid.db_tab_base);
			    SETDBERR(dberr, 0, E_DM9118_ETAB_OPEN_ERR);
			    break;
			}
		    }
		    /* and get the segments */
		    qual_list[0].attr_number = 1;
		    qual_list[0].attr_operator = DM2R_EQ;
		    qual_list[0].attr_value = (char *)&dmf_coupon->cpn_log_key;
		    qual_list[1].attr_number = 2;
		    qual_list[1].attr_operator = DM2R_EQ;
		    qual_list[1].attr_value = (char *)&zero;
		    qual_list[2].attr_number = 3;
		    qual_list[2].attr_operator = DM2R_EQ;
		    qual_list[2].attr_value = (char *)&segno;
		    for (segno = 1,bad_segments = 0, blob_length = 0;;segno++)
		    {
			status = dm2r_position(etab_rcb, DM2R_QUAL, qual_list, 
			    (i4)3, (DM_TID *)NULL, dberr);
			if (status != E_DB_OK)
			{
			    if (dberr->err_code == E_DM0055_NONEXT)
			    {
				/* can't find segment */
				bad_segments++;
				dm1u_talk( dm1u,W_DM5060_NO_ETAB_SEGMENT, 
				    8, sizeof(segno), &segno,
				    sizeof(dmf_coupon->cpn_etab_id),
				    &dmf_coupon->cpn_etab_id,
				    t->tcb_atts_ptr[i].attnmlen,
				    t->tcb_atts_ptr[i].attnmstr,
				    sizeof(rcb->rcb_currenttid.tid_i4),
				    &rcb->rcb_currenttid.tid_i4);
				SETDBERR(dberr, 0, W_DM5060_NO_ETAB_SEGMENT);
				if (bad_segments <= MAX_BAD_SEGMENTS)
				{
				    status = E_DB_OK;
				    continue;
				}
				else
				{
				    dm1u_talk( dm1u,W_DM505F_TOO_MANY_BAD_SEGS, 
					6, sizeof(dmf_coupon->cpn_etab_id),
					&dmf_coupon->cpn_etab_id, 
					t->tcb_atts_ptr[i].attnmlen,
					t->tcb_atts_ptr[i].attnmstr,
					sizeof(rcb->rcb_currenttid.tid_i4),
					&rcb->rcb_currenttid.tid_i4);
				    SETDBERR(dberr, 0, W_DM505F_TOO_MANY_BAD_SEGS);
				    status = E_DB_OK;
				    break;
				}
			    }
			    else
			    {
				uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				    NULL, 0, NULL, err_code, 0);
				dm1u_talk(dm1u,E_DM9119_ETAB_POS_ERR, 2, 
				    sizeof(relrecord.reltid.db_tab_base), 
				    &relrecord.reltid.db_tab_base);
				SETDBERR(dberr, 0, E_DM9119_ETAB_POS_ERR);
				break;
			    }
			}
			status = dm2r_get(etab_rcb, &tid, DM2R_GETNEXT,
			    (char *)&etab_record, dberr);
			if (status != E_DB_OK)
			{
			    if (dberr->err_code == E_DM0055_NONEXT)
			    {
				/* can't find segment */
				bad_segments++;
				dm1u_talk( dm1u,W_DM5060_NO_ETAB_SEGMENT, 
				    8, sizeof(segno), &segno,
				    sizeof(dmf_coupon->cpn_etab_id),
				    &dmf_coupon->cpn_etab_id,
				    t->tcb_atts_ptr[i].attnmlen,
				    t->tcb_atts_ptr[i].attnmstr,
				    sizeof(rcb->rcb_currenttid.tid_i4),
				    &rcb->rcb_currenttid.tid_i4);
				SETDBERR(dberr, 0, W_DM5060_NO_ETAB_SEGMENT);
				if (bad_segments <= MAX_BAD_SEGMENTS)
				{
				    status = E_DB_OK;
				    continue;
				}
				else
				{
				    dm1u_talk( dm1u,W_DM505F_TOO_MANY_BAD_SEGS, 
					6, sizeof(dmf_coupon->cpn_etab_id),
					&dmf_coupon->cpn_etab_id, 
					t->tcb_atts_ptr[i].attnmlen,
					t->tcb_atts_ptr[i].attnmstr,
					sizeof(rcb->rcb_currenttid.tid_i4),
					&rcb->rcb_currenttid.tid_i4);
				    SETDBERR(dberr, 0, W_DM505F_TOO_MANY_BAD_SEGS);
				    status = E_DB_OK;
				    break;
				}
			    }
			    else
			    {
				uleFormat(dberr, 0, NULL, ULE_LOG, NULL, 
				    NULL, 0, NULL, err_code, 0);
				dm1u_talk(dm1u,E_DM9119_ETAB_POS_ERR, 2, 
				    sizeof(relrecord.reltid.db_tab_base), 
				    &relrecord.reltid.db_tab_base);
				SETDBERR(dberr, 0, E_DM9119_ETAB_POS_ERR);
				break;
			    }
			}
			/* 
			** if we got here, then we have the current segment,
			** add to the total length, and check for next segment
			*/
			if (under_dv.db_datatype == DB_VCH_TYPE ||
			    under_dv.db_datatype == DB_TXT_TYPE ||
			    under_dv.db_datatype == DB_VBYTE_TYPE ||
			    under_dv.db_datatype == DB_LTXT_TYPE ||
			    dtinfo & AD_VARIABLE_LEN)
			blob_length += segment->db_t_count;
			if (etab_record.prd_r_next == 0)
			    break;
		    }
		    if (status != E_DB_OK)
			break;
		    /* check total blob length */
		    if (blob_length && blob_length != cbuf.coupon.per_length1)
		    {
			dm1u_talk( dm1u,W_DM5061_WRONG_BLOB_LENGTH, 8, 
			    t->tcb_atts_ptr[i].attnmlen,
			    t->tcb_atts_ptr[i].attnmstr,
			    sizeof(rcb->rcb_currenttid.tid_i4),
			    &rcb->rcb_currenttid.tid_i4, sizeof(u_i4),
			    &cbuf.coupon.per_length1, sizeof(u_i4),
			    &blob_length);
			SETDBERR(dberr, 0, W_DM5061_WRONG_BLOB_LENGTH);
		    }
		}
		if (status != E_DB_OK)
		    break;
	    }
	}
	break;
    }
    /* cleanup */
    if (record)
	(VOID)MEfree(record);

    if (rel_rcb)
	(VOID)dm2r_release_rcb(&rel_rcb, &local_dberr);

    if (etab_rcb)
	(VOID)dm2t_close(etab_rcb, (i4)0, &local_dberr);

    return(status);
}

/*{
** Name: dm1u_talk	- DMF message generation routine
**
** Description:
**      DMF check/patch table operation sends messages to the FE using the
**	same mechanism that DB Procedures use to send messages.  This routine
**	handles formatting the message and sending it to the FE via SCF's
**	SCC_MESSAGE mechanism.
**
** Inputs:
**
** Outputs:
**
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**	12-oct-92 (teresa)
**	    Implement Sir 42498.
**	30-feb-1993 (rmuth)
**	    Rename to dm1u_talk.
**	24-Oct-2008 (jonj)
**	    Replace ule_doformat with ule_format.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Rewrote with variable number of params for prototyping.
*/
VOID
dm1u_talk(
DM1U_CB *dm1u,
i4	msg_id,
i4	count,
... )
{
#define NUM_ER_ARGS	12
    char        cbuf[DM1U_MSGBUF_SIZE];
    SCF_CB      scf_cb;
    DB_STATUS   status;
    DB_STATUS	err_code;
    i4	mlen;
    char	*sqlstate_str = SS5000G_DBPROC_MESSAGE;
    i4		i;
    va_list	ap;
    ER_ARGUMENT er_args[NUM_ER_ARGS];

    /* suppress the message if its informative (5201 - 53ff) and the
    ** the verbose optiion is not specified. (SIR 42498)
    */
    if (
	(msg_id >= I_DM5201_BAD_PG_NUMBER) 
	&&
	!(dm1u->dm1u_flags & DM1U_VERBOSE)
       )
	    return;

    va_start( ap, count );
    for ( i = 0; i < count && i < NUM_ER_ARGS; i++ )
    {
        er_args[i].er_size = (i4) va_arg( ap, i4 );
	er_args[i].er_value = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    status = uleFormat(NULL, msg_id, NULL, ULE_LOOKUP, NULL, cbuf, DM1U_MSGBUF_SIZE, 
	&mlen, &err_code, i, 
	er_args[0].er_size, er_args[0].er_value,
	er_args[1].er_size, er_args[1].er_value,
	er_args[2].er_size, er_args[2].er_value,
	er_args[3].er_size, er_args[3].er_value,
	er_args[4].er_size, er_args[4].er_value,
	er_args[5].er_size, er_args[5].er_value,
	er_args[6].er_size, er_args[6].er_value,
	er_args[7].er_size, er_args[7].er_value,
	er_args[8].er_size, er_args[8].er_value,
	er_args[9].er_size, er_args[9].er_value,
	er_args[10].er_size, er_args[10].er_value,
	er_args[11].er_size, er_args[11].er_value);

    /* Format SCF request block */

    scf_cb.scf_facility = DB_DMF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_nbr_union.scf_local_error = msg_id;
    for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate[i] = sqlstate_str[i];
	
    scf_cb.scf_ptr_union.scf_buffer = cbuf;
    scf_cb.scf_len_union.scf_blength = mlen;
    (VOID) scf_call(SCC_MESSAGE, &scf_cb);
}
static VOID
dm1u_log_error(
DB_ERROR   *dberr)
{
    i4 lerror;

    if ( dberr->err_code )
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &lerror, 0);
}
