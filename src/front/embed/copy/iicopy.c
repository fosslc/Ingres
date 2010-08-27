/*
** Copyright (c) 1986, 2005 Ingres Corporation
**
*/
 
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<lo.h>
# include	<si.h>
# include	<cm.h>
# include	<st.h>
# include	<ex.h>
# include	<nm.h>
# include	<er.h>
# include	<me.h>
# include	<gl.h>
# include	<iicommon.h>
# include	<generr.h>
# include	<copy.h>
# include	<adf.h>
# include	<adp.h>
# include	<ade.h>
# include	<gca.h>
# include	<eqrun.h>
# include	<iicgca.h>
# include	<iirowdsc.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<iicopy.h>
# include	<erco.h>

# define MAX_SEG_FROM_FILE 32768	/* Maximum segment size FROM file. */

# define MAX_SEG_INTO_FILE 4096		/* Maximum segment size INTO file */

/* Define size of read-buffer part of the row buffer for COPY FROM.
** Needs to be at least MAX_SEG_FROM_FILE.
*/
#define READ_BUF_SIZE	65536	/* 64K */

/* Structures to communicate with the LOB datahandler.  LIBQ allows a
** single parameter to be passed to the datahandler, so we need something
** that ties up all the pieces needed.
*/

typedef struct {
	II_LBQ_CB	*libq_cb;
	II_CP_STRUCT	*cblk;
	II_CP_GROUP	*cgroup;	/* Group consisting of LOB */
	i4		firstseglen;
	bool		failure;
} LOBULKFROM;

typedef struct {
	II_LBQ_CB	*libq_cb;
	II_CP_STRUCT	*cblk;
	II_CP_GROUP	*cgroup;	/* Group consisting of LOB */
	bool		failure;
} LOBULKINTO;

/* 
**  Name: loc_alloc_i - a version of loc_alloc() that initializes the memory 
**
**  History:
**	05-jul-2000 (wanfr01)
**	    Written.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      04-jun-2002 (stial01)
**	    Change MAX_SEG_FILE from 4096 to MAXI2 (b107934)
**      06-jun-2002 (stial01)
**	    Backout 04-jun-2002 change
**      07-jun-2002 (stial01)
**	    Copy table FROM file, support segment size MAXI2
**      24-jan-2005 (stial01)
**          Fixes for binary copy LONG NVARCHAR (DB_LNVCHR_TYPE) (b113792)
**	30-Nov-2009 (kschendel) SIR 122974
**	    Use modern style prototypes.
*/
 
static STATUS
loc_alloc_i(i4 n1,i4 n2,PTR *p)
{
      *p = MEreqmem(0,n1*n2,TRUE,NULL);

      if (*p == NULL)
              return(FAIL);
      return(OK);
}
 
static STATUS
loc_alloc(i4 n1,i4 n2,PTR *p)
{
	*p = MEreqmem(0,n1*n2,FALSE,NULL);
 
	if (*p == NULL)
		return(FAIL);
	return(OK);
}
 
 
/*
+*  Name: iicopy.c - Runtime support for COPY command.  Contains routines
**	needed in Front End to execute COPY.
**
**  Defines:
**	IIcopy		- Drives COPY
**	IIcpinit	- Initialize for COPY processing
**	IIcptdescr	- Allocate and build GCA tuple descriptor.
**	IIcpdbread	- Rad group from the DBMS during COPY INTO
**	IIcpinto_init	- Initialize for next group during COPY INTO
**	IIcprowformat	- Convert tuple to row format for COPY INTO
**	IIcpinto_lob	- Large-object processing for COPY INTO
**	IIcpinto_lobHandler - Large-object callback for COPY INTO
**	IIcpputrow	- Write row data to file for COPY INTO
**	IIcpfrom_init	- Initialize for next group during COPY FROM
**	IIcpfrom_binary - Binary non-LOB COPY () FROM group handler
**	IIcpfrom_formatted - Formatted copy(list) FROM group handler
**	IIcpfrom_lob	- LOB COPY () FROM group handler
**	IIcpdbwrite	- Write group to the DBMS during COPY FROM
**	IIcpfopen	- Open copy file
**	IIcpfclose	- Close copy file
**	IIcpgetch	- Char handler for IIcpgetrow
**	IIcpgetbytes	- Bytes handler for IIcpgetrow
**	IIcpdbflush	- Flush output to the DBMS
**	IIcpendcopy	- Do FE/DBMS handshaking at end of copy
**	IIcplgopen	- Open bad-row log file
**	IIcplgclose	- Close bad-row log file
**	IIcpcleanup	- Clean up memory, close files, etc. ...
**	IIcpabort	- Communicate with the DBMS to abort the COPY command.
**	IIcp_handler	- Handle interrupts during COPY
**
-*
**  History:
**	01-dec-1986 (rogerk)
**	    Written.
**	02-apr-1987 (ncg)
**	    Modified for new row descriptors.
**	24-apr-1987
**	    Add logging of bad rows, bug fixes.
**	09-jun-1987
**	    Write out varchar types with ascii len specifier.
**	25-aug-87 (puree)
**	    fix varchar, text, and variable length field
**	27-aug-87 (puree)
**	    fix exception handler for control-C
**	04-sep-87 (puree)
**	    change IIlocerr interface.
**	09-sep-87 (puree)
**	    change error message codes sent to IIlocerr.
**	10-sep-87 (puree)
**	    convert to GCF architecture.
**	17-nov-87 (puree)
**	    fixed IIcpendcopy for rollback option, fixed many places
**	    for better error handling.
**	24-feb-88 (puree)
**	    Modified IIcpinit to null terminate column names for better
**	    error reporting format.
**	04-aug-88 (neil)
**	    Added some consistency checks throughout the file (validating
**	    data sizes, etc).
**	11/11/88 (dkh/barabar) - Changed code in IIcpgetrow to synchronize
**	    "row" and "row_ptr" correctly when the buffer size is increased.
**	14-nov-88 (barbara)
**	    fixed bugs #3912, #3695 and #3314
**	12-dec-88 (neil)
**	    generic error processing changed all calls to IIlocerr plus some
**	    lint checks.
**	20-jan-89 (barbara)
**	    fixed bug #4370.  Also fixed COPY INTO bug where delimiter appeared
**	    prematurely in fixed field output.
**	21-jun-89 (barbara)
**	    fixed bug #6380.  dummy 'nul' format was not copying null[s]
**	    into file.
**	14-jul-89 (barbara)
**	    fixed bug #5798.  default null format was not being blank padded
**	    on COPY INTO using variable char or c format.
**	22-sep-89 (barbara)
**	    updated for decimal.
**	26-nov-89 (barbara)
**	    fixed bug 8886.  On COPY TO don't pad default null value unless
**	    column width is wider than length of default null value.
**	22-jan-90 (barbara)
**	    Fixed up calls to LOfroms.
**	24-jan-90 (fredp)
**	    Fixed alignment problems with DB_TEXT_STRING data types in 
**	    IIcprowformat() and IIcptupformat().
**	06-feb-90 (fredp)
**	    Fixed alignment problems for non-string datatypes too.
**	07-feb-90 (fredp)
**	    Fixed an alignment problem in IIcpinit() where adc_getempty().
**	    was called with unaligned DB_DATA_VALUEs.
**	14-feb-90 (fredp)
**	    Fix a bug introduced in the alignment fix of 06-feb-90.
**	15-feb-90 (fredp)
**	    Initialize align_buf in IIcptupformat().
**	26-feb-1990 (barbara)
**	    To enhance SandyH's libqgca fix to bug #7927, IICGC_COPY_MASK 
**	    is now turned off in IIcgc_read_results.  Removed code in this
**	    module that turned off IICGC_COPY_MASK flag.
**	05-apr-90 (teresal)
**	    Fix for bug 8725 so warning messages show correct row count for
**	    number of rows copied.  Also, added increment of row count when
**	    doing a COPY INTO and the number of badrows has exceeded max
**	    errors or else warning message's row count won't be accurate.
**	27-apr-1990 (barbara)
**	    All the clean up of byte alignment problems for varchar and text
**	    has exposed some bugs in the way we were copying nullable varchar
**	    and text formats.  Basically, these formats were getting copied
**	    into and read from file as "internal" data.  Fixed this.
**	03-may-1990 (barbara)
**	    IIcptupformat should only copy from aligned buf to tup buf if
**	    cp_convert is set.
**	18-jun-1990 (barbara)
**	    Changes for bug fix 21832.  LIBQ's new default behavior is to
**	    not quit on association failures.  Changes needed in IIcpdbread
**	    and IIcpendcopy.
**	24-jul-1990 (barbara)
**	    One more byte alignment problem in IIcptupformat.  When fieldlen
**	    is zero, adc_valchk was being called with possibly non-aligned data.
**	07-aug-1990 (mgw) Bug # 21432
**	    Report user error too in IIcptupformat if applicable.
**	22-may-90 (fredb)
**	    MPE/XL (hp9_mpe) specific change in IIcpinit to force the copy
**	    into file type to VAR.
**      02-oct-90 (teresal)
**          Modified IIcptupformat for bug fix 32562.  Give an error if
**          COPY FROM attempts to put NULL data into a non-nullable field
**          using the 'with null' option.
**	08-oct-90 (teresal)
**	    Bug fix 20335: fixed so a warning message will be displayed
**	    whenever character data is truncated when COPY FROM is done.
**      15-oct-90 (teresal) 
**	    Bug fix 20335: Took out this bug fix as this caused problems
**	    when 'value' in WITH NULL option was longer than the column's
**	    length.
**	19-oct-90 (teresal)
**	    Bug fix 20335: Put in a new bug fix to display a warning
**	    message when character data is truncated for variable length
**	    copy formats.  This fix handles a null value which is longer
**	    then the column's length by not truncating this value.
**	30-nov-90 (teresal)
**	    Bug fix 34405.  Fixed so COPY's ending error/warning messages
**	    will show the correct row count copied if rows with duplicate 
**	    keys were found. To comply with SQL semantics, a warning 
**	    message "COPY: Warning: %0c rows not copied because duplicate 
**	    key detected" will be displayed, for SQL, if the number 
**	    of rows added by the server is less than the number of rows 
**	    sent by COPY.  A requirement of the COPY dbms/fe interface 
**	    is: if the number of rows added by the server is less than 
**	    the number of rows sent by fe COPY, COPY can assume that 
**	    the rows were not added because duplicate key data was found.
**	26-dec-90 (teresal)
**	    Modified IIcpinit to display an error if a TEXT filetype
**	    is specified with a bulk copy.  Bug 6936.
**	04-jan-91 (teresal)
**	    Modified IIcpinit to only look for a filetype if VMS.  This
**	    fix was requested from ICL so filenames with commas can 
**	    be used.
**	10-jan-91 (teresal)
**	    Bug fix 35154.  COPY INTO with format "varchar(0)" was 
**	    causing an AV on UNIX because the row buffer allocated 
**	    wasn't big enough.  Modified IIcpinit to calculate
**	    "cp_row_length" correctly.
**	22-feb-91 (teresal)
**	    Bug fix 36059: avoid ERlookup error by calling IIdberr
**	    rather than IIlocerr to display formatted ADF error.
**	    Problem occurred when error message has >= 1 parameters.
**	29-mar-91 (barbara)
**	    Extended COPY FROM PROGRAM support to allow copying variable
**	    length rows.  Abstracted the IIcpgetrow code that "gets" a char
**	    (either from file or from user buffer).
**	09-apr-91 (barbara)
**	    Fixed bug (missing argument on call to IIcpgetbytes).
**	11-apr-91 (barbara)
**	    Fixed bug 36900.  IIcpgetbytes was complaining about EOF even
**	    in legitimate places. 
**	06-may-91 (teresal)
**	    Bug fix 37341 - avoid AV on unix when varchar(0) has a null string
**	    longer than the field.  This bug was fixed previously but broke 
**	    when bug fix 35154 was put in.  "cp_nullen" as sent from the 
**	    DBMS uses a 2 byte length specifier, we need to allocate 5 bytes 
**	    for writing out the null string to the file.
**	06-aug-1991 (teresal)
**	    Modify IIcpinit to accept a new copy map format (GCA Protocol 
**	    level 50). The 'with null' value is now sent as a DBV rather than 
**	    as a character data string.  This fixes bug 37564 where
**	    the null value wasn't being converted correctly over a
**	    hetnet connection.
**	29-nov-91 (seg)
**	    EXdeclare takes a (STATUS (*)()), not (EX (*)()).
**	    Can't dereference or do arithmetic on PTR
**	    Fixed bogus if statement in IIcpgetrow.
**	07-feb-1992 (teresal)
**	    Bug fix 41780: Modify IIcptupformat to validate DATE fields in
**	    the file for a COPY FROM when the copy format is
**	    a DATE type and tuple format is a DATE type. A DATE to DATE
**	    coersion doesn't validate that the date is in an internal 
**	    INGRES format, thus, COPY was causing invalid dates to get 
**	    into the database.
**	22-jul-1992 (sweeney)
**	    Switch off optimization for apl_us5 - compiler fails with:
**	    [Error #144]  Compiler failure, registers locked.
**	13-dec-1992 (teresal)
**	    For decimal changes, modify all references to the old global adf 
**	    control block and instead reference the new session specific 
**	    adf control block.
**	15-mar-1993 (mgw)
**	    Added handling for copying large objects INTO file. Also added
**	    limitted handling for copying large objects FROM file, but left
**	    that disabled since it doesn't work yet.
**	13-sep-1993 (dianeh)
**	    Removed NO_OPTIM setting for obsolete Apollo port (apl_us5).
**	23-sep-1993 (mgw)
**	    Add DB_VBYTE_TYPE cases to enable proper handling of BYTE
**	    Varying datatype.
**	01-oct-1993 (kathryn)
**	    First parameter to IILQlgd_LoGetData and IILQlpd_LoPutData
**	    (routines to get and put segments of large objects) has been
**	    changed to a bit mask.  Valid values are defined in eqrun.h.
**	17-feb-1994 (johnst)
**	    Bug #60013
**	    Integrated the (kchin) 6.4/03 fix for platforms with 64-bit 
**	    pointers (eg DEC alpha axp_osf):
**          "roll the GCA_COL_ATT element by element to avoid inclusion of 
**	    structure pads present on 64 bit pointer machines."
**	    Also, for earlier Bug #58882 change, corrected file merge problem 
**	    where the "#define NOT_ALIGNED..." line was not deleted in the 
**	    submission. Delete the line now.
**	20-feb-1995 (shero03)
**	    Bug #65565
**	    Support long User Data Types
**      12-sep-1995 (lawst01)
**          Bug #71149
**          If delimiter is omitted from format c0 field - default to '\n'
**          delimiter.
**	28-sep-1995 (kch)
**	    Add new argument (end_of_tuple) to function IICPcfgCheckForGaps().
**	    This change fixes bug 71595.
**	31-oct-95 (kch)
**	    In the function IIcopy(), switch off II_LO_END flag for ascii
**	    'copy ... from' in Large Object Info structure
**	    (IIlbqcb->ii_lq_lodata.ii_lo_flags). This allows multiple ascii
**	    'copy ... froms' in the same tm/itm session. This change fixes
**	    bug 72235.
**	07-nov-1995 (kch)
**	    Bug 72054: In the function IIcpinit(), the blob check has been
**	    changed from (IIDT_LONGTYPE_MACRO && IIDT_UDT_MACRO) to
**	    (IIDT_LONGTYPE_MACRO || IIDT_UDT_MACRO). The AND case could
**	    never be true; with the two macros ORed, an ascii 'copy .. into'
**	    will write the correct null blob values to the data file.
**	    Bug 72172: In the function IICPfbwFmtBlobWrite(), the null
**	    indicator variable, nulblob, is now reset to zero at the start of
**	    each domain in order to allow an ascii 'copy ... from' of data
**	    containing multiple rows, at least one of which contains a null
**	    blob.
**	21-nov-1995 (kch)
**	    In the function IIcpinit(), do not set copy_blk->cp_varrows to 1
**	    if the column is dummy. This change fixes bug 72678.
**	01-mar-1996 (kch)
**	    In the function IICPbbwBulkBlobWrite(), the null indicator
**	    variable, nulblob, is now reset to zero at the start of
**	    each domain in order to allow an bulk 'copy ... from' of data
**	    containing multiple rows, at least one of which contains a null
**	    blob. This change fixes bug 74057.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**      06-Feb-96 (fanra01)
**          Made data extraction generic.
**	15-feb-1996 (canor01)
**	    Bug 73304: on Windows NT.  Accept the default delimiters as
**	    text delimiters when determining how to open input file.
**	25-mar-96 (kch)
**	    In the functions IIcpgetrow() and IICPfbwFmtBlobWrite() we
**	    now set end_ptr once before the domain by domain read from the
**	    copy file (calls to IICPgcGetCol()). Previously, end_ptr was
**	    being wrongly moved after each domain, and this was preventing
**	    new memory from being allocated for the row buffer, row_ptr, in
**	    IICPgcGetCol(). Subsequently, if the row width was greater than
**	    8K, unallocated memory could be written to during the filling
**	    of the row buffer.
**	    In IICPgcGetCol(), end_ptr is now correctly reset after the
**	    allocation of a bigger row buffer.
**	    These changes fixes bug 75452.
**	26-mar-96 (kch)
**	    In the function IICPgcGetCol(), we test for delimited fixed width
**	    columns, and if found, increment the byte count in the call to
**	    IIcpgetbytes(). Similarly, the row buffer, row_ptr, is incremented
**	    by an extra byte. This change fixes bug 75437, which is a side
**	    effect of the previous fix for bug 72678 (the fix for 72678 has
**	    been backed out in the function IIcpinit()).
**	05-apr-1996 (loera01)
**	    Modified  IIcpinit() to support compressed variable-length
**	    datatypes.
**      14-jun-1996 (loera01)
**          Modified IIcpdbread(),IICPbbrBulkBlobRead(),IICPbdrBlobDbRead() 
**          so that variable-length datatype decompression is performed
**          in LIBQ, rather than in LIBQGCA.  A new routine, IICPdecompress(),
**          decompresses individual rows for bulk copy.
**	10-jul-96 (chech02)
**	    bug#77549: When calls to STprintf("%d"), arguments should be nat.
**      16-jul-96 (loera01)
**          Modified IIICPbdrBlobDbRead() so that single columns may be 
**          copied if the row contains one or more compressed varchars. Fixes
**          bug 77127.
**      13-aug-96 (loera01)
**          Modified IICPbbrBulkBlobRead() and IICPbdrBlobDbRead() to 
**          correctly handle partial tuples.  Fixes bug 77129.
**          Modified IIcpinit() so that the allocation of cp_rowbuf 
**          and cp_tupbuf consist of the row or tuple length plus MAX_SEG_FILE.
**          With variable page size, non-blob columns plus a blob may be sent 
**	    in the same message from the back end.  Fixes bug 77129.
**          Fixed memory leaks in IIcpdbread(),IICPdecompress,
**          IICPbdrBlobDbRead(), and IICPbbrBulkBlobRead().
**	    For IICPfbwFmtBlobWrite(), re-initialize copy_blk->cp_zerotup 
**          with saved pointer value, instead of decrementing by 
**	    copy_blk->cp_tup_length.  If the tuple contained a blob and a 
**          skipped non-blob column, the pointer did not advance by the 
**	    full tuple length.
**	21-aug-96 (chech02) bug#77549
**	    'stack overflow' if doing a 'copy ... from' statement from a file 
**	    that has too many rows. Fixes: Moved txt_buf, align_buf and 
**	    alignrow_buf in  IIcptupformat() from stack area to static data 
**	    area for windows 3.1 port.
**	04-sep-96 (thoda04)
**	    Last arg of IIcgcp3_write_data() takes addr of dbv, not dbv itself.
**	03-oct-1996 (chech02) bug#78827
**	    moved local variables to global heap area for windows 3.1
**	    port to prevent 'stack overflow'.
**	20-nov-96 (kch)
**	    In the function IIcpinit(), we now do not add the default delimiter
**	    ('\0') for a copy into of non varchar formats that have no
**	    delimiter specified. This change fixes bug 79476 (side effect of
**	    the fixes for bugs 71149 and 73304).
**	20-feb-97 (kch)
**	    In the function IICPfbwFmtBlobWrite(), we now save the width of
**	    each column in the zero tuple and use this to move to the start of
**	    the zero tuple when the domain by domain processing is done. This
**	    change fixes bug 79834 (side effect of fix for bug 73123).
**	02-dec-1996 (donc)
**	    Add an other arg to an SIfprintf call. This will 1) allow the
**	    module to compile with compilers w/ strict argument checking and 2)
**	    probably NOT gpf (the old call would have likely done this).
**	18-dec-96 (chech02)
**	    Fixed redefined symbols error: ii_copy_struct.
**	04-apr-1997 (canor01)
**	    Windows NT must always open the file in binary mode
**	    when reading, and do the CRLF=>LF translation at
**	    character read time to prevent ^Z character from
**	    causing EOF.
**	25-apr-1997 (somsa01)
**	    Due to the previous change to IIcpgetbytes(), the "status" variable
**	    was not being set, yet it was being checked. Thus, on success from
**	    the NT_GENERIC loop, set status=OK. Also, needed to propagate
**	    previous changes to IIcpgetrow() as well, since ^Z  characters are
**	    allowed in varchars as well.
**	22-may-97 (hayke02)
**	    In iicpendcopy(), tighten up the check for duplicate rows
**	    (E_CO003F) - the number of rows copied must be greater than zero.
**	    If zero rows were copied (probably due to running out of disk
**	    space during a copy into a non heap table), return E_CO0029.
**	    This change fixes bug 80478.
**  6-jun-1997 (thaju02)
**	    bug #82109 - 'copy into' with blob column resulted in E_LQ00E1
**	    error. Error due to #ifdef PTR_BITS_64 code, which mutilated
**	    cgc->cgc_tdescr->gca_col_desc[]; in the transposing of data to
**	    td_ptr, neglected to account for the gca_attname field, and
**	    also the MEcopy destination parameter is incorrect (it is
**	    (char *)&cgc->cgc_tdescr->gca_col_desc[0].gca_attdbv, but should
**	    be (char *)&cgc->cgc_tdescr->gca_col_desc[0]), resulting in
**	    us making a complete mess of cgc->cgc_tdesc->gca_col_desc[]. This
**	    #ifdef PTR_BITS_64 was added (17-feb-94) to remove extra
**	    alignment padding added by the compiler, because gca_to_fmt() was/is
**	    not expecting padding. But #ifdef PTR_BITS_64 is not necessary
**	    since the gca_col_desc[] structure/fields are being referenced
**	    through the structure/fields and not by offsets in gca_to_fmt(),
**	    thus as long as we are consistent in accessing the gca_col_desc[] 
**	    thru indices and structure/fields, no need to fiddle with it.
**	17-Sep-97 (fanra01)
**	    Bug 85435
**	    Switch off varchar compression when performing COPY FROM as this
**	    is not supported and can cause net to misinterpret the tuple
**	    contents.
**	18-Nov-1997 (thaju02)
**	    Back out of 6-jun-1997 (thaju02) change for bug #82109, caused
**	    hetnet problems. 
**	18-Nov-1997 (thaju02)
**	    Bug #82109 - for PTR_BITS_64, attempting to remove extra padding
**	    from cgc->cgc_tdescr, trashed the gca row descriptor. In IIcopy()
**	    build the cgc->cgc_tdescr from the column attributes in the
**	    gca row descriptor in gca byte stream format.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**      04-Aug-1998 (wanya01)
**          X-integrate change #436526. Bug 91904. 	    
**          In IIcpinit().  no 'continue' when incrementing long_count on non
**          byte-aligned platforms. Led to exception in MEdoAlloc() when
**          coping in ascii tables with long byte/varchar columns. 
**	14-apr-1998 (shust01)
**	    In IICPfbwFmtBlobWrite(), the variable copy_blk->cp_zerotup
**	    is being incremented within the for loop.  If we prematurely
**	    exit the routine before the for loop completes, the variable
**	    is never reset.  If variable is not reset, could SIGBUS when
**	    MEfree storage.  Changed all return statements within loop to
**	    breaks, to allow the variable to be reset before returning to
**	    calling routine.  bug 93805.
**    	23-nov-98 (holgl01) bug 93563
**          In IIcpfopen(), set rms buffer for vms systems for a TXT (text) 
**          file to the maximum record that can be written (SI_MAX_TXT_REC) 
**	    for SIfopen. Otherwise, files greater than 4096 in length cannot
**	    be opened.
**	22-dec-1998 (holgl01) Revision of Bug 93563
**	    Pass SI_MAX_TXT_REC directly  to SIfopen in IIcpfopen
**	    instead of resetting  cp_row_length to SI_MAX_TXT_REC 
**          and passing that.
**	05-jan-1998 (kinte01) 
**	    Modify previous change to use #elif defined(VMS) instead of 
**	    #elif VMS to satisfy problems with DEC C compiler
**	23-Feb-1999 (toumi01)
**	    Add NO_OPTIM for Linux (lnx_us5) to avoid these errors
**		E_CO0040 COPY : Unexpected end of user data ...
**		E_CO002A COPY : Copy has been aborted.
**	    running (e.g.) sep tests acc00.sep and cpy15.sep.  This has also
**	    occurred in user situations.  This seems to be a compiler bug,
**	    and occurs in both libc5 and libc6 versions of Linux using the
**	    gcc compiler.  The problem occurs in routine IIcpdbread when
**	    calls are made to IICPdecompress and (it appears that) the bool
**	    "partial" is not set on return.  Debugging is difficult, since
**	    any code changes or OPTIM changes get rid of the bug.  Even adding
**	    an uncalled routine to the end of the assembler deck gets around
**	    the problem.  See bug 94449.
**	18-jun-1999 (schte01) 
**	    Added cast macros to avoid unaligned acccesses for axp_osf. 
**	16-Aug-1999 (hanch04)
**	    Bug 98420
**	    Need to check the return value for IIcpgetch in IICPgcGetCol
**	    Removed NO_OPTIM for Linux (lnx_us5)
**      02-Sep-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
**	17-jan-2000 (toumi01)
**	    NO_OPTIM for Linux has been removed; problem may be fixed by
**	    hanch04 change for bug 98420.  This is a hand-modification of
**	    the integration of change 444204 from oping20 to main, which
**	    would have changed the NO_OPTIM string from lnx_us5 to int_lnx.
**      10-Mar-2000 (hweho01)
**          Moved ex.h after erco.h to avoid compile errors from
**          replacement of jmpbuf for ris_u64. 
**      10-Oct-2000 (hweho01)
**          Removed the above change, it is not needed after change #446542 
**          in excl.h.
**      11-May-2001 (loera01)
**          Added support for Unicode varchar compression.
**	21-Dec-2001 (gordy)
**	    Removed PTR from GCA_COL_ATT and updated associated access code.
**	    Removed PTR_BITS_64 code as no longer relevant.  However, had to
**	    introduce a similar function for all platforms to generate a GCA
**	    tuple descriptor from a row descriptor since the two now differ.
**	20-jan-2004 (somsa01)
**	    Updated "COPY FROM/INTO... PROGRAM" case to work with BLOBs.
**	03-feb-2004 (somsa01)
**	    Make sure we pass an i4 as the segment length to get_row() in the
**	    the case of "COPY FROM/INTO... PROGRAM" with BLOBs.
**      05-jul-2005 (mutma03)
**          Filtered ctrl<M> from the column data from the files that were
**          generated from Windows to run on other platforms.
**	    if we are ending on a row boundary. (B116562)
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	12-oct-2006 (thaju02)
**	    In IICPfbrFmtBlobRead, also check lastattr to determine
**      06-oct-2009 (joea)
**          Add case for DB_BOO_TYPE in gtr_type.
**	30-Nov-2009 (kschendel) SIR 122974
**	    Major internal overhaul.
**	    Reduce the astounding amount of time that formatted COPY FROM was
**	    taking just to move characters around, by redoing the input
**	    buffering scheme:  Scan out of the read buffer instead of
**	    laboriously copying characters to and fro.
**	    Integrate LOB and non-LOB code by introducing concept of
**	    copy groups.
**	    Various minor cleanups:
**	    Remove obsolete/incorrect i2 casts on MEcopy/fill calls.
**	    Declare various local-only routines here, as static.
**	28-Jan-2010 (kschendel) SIR 122974
**	    Fixes for Windows text files with \r\n line-enders.
**	    Delete some unreferenced locals that the MS compiler caught.
**	25-Feb-2010 (kschendel) SIR 122974
**	    Fix howler bug initializing zerotup, was screwed up after
**	    a LOB.  Update comments re sending end-of-data.
**	10-Mar-2010 (kschendel) SIR 122974
**	    Squeeze out a bit more performance by eliminating rowleft in favor
**	    of a guard pointer.
**      01-Jul-2010 (horda03) B124010
**          If the file for a COPY FROM cannot be opened, then terminate the
**          command but don't ABORT it. If the COPY was to a Global Temporary
**          Table, aborting the COPY in this instance will drop the table,
**          but no data has been transferred, so the GTT has not been modified.
*/ 

/* Not sure why these aren't declared elsewhere, but whatever... */

FUNC_EXTERN void IILQldh_LoDataHandler(i4 type, i2 *ind, void (*datahdlr)(), PTR arg);
FUNC_EXTERN i4 IILQled_LoEndData(void);

/* Forward declarations */

static char *gtr_type(i4, char *);

static STATUS IIcpinit(
	II_LBQ_CB * lpqcb,
	II_CP_STRUCT * copy_blk,
	i4 msg_type);
 
static GCA_TD_DATA * IIcptdescr(
	ROW_DESC *row_desc);

static i4 IIcpdbread(
	II_LBQ_CB *IIlbqcb, 
	II_CP_STRUCT *copy_blk,
	II_CP_GROUP *cgroup,
	PTR *tuple);

static i4 IIcpinto_init(
	II_CP_STRUCT *copy_blk,
	char **row,
	char *tuple);

static i4 IIcprowformat(
	II_LBQ_CB * lbqcb,
	II_CP_STRUCT * copy_blk,
	II_CP_GROUP *cgroup,
	PTR tuple,
	PTR row);

static i4 IIcpinto_lob(
	II_LBQ_CB *libqcb,
	II_CP_STRUCT *cblk,
	II_CP_GROUP *cgroup);

static void IICPfbFlushBlob();

static void IIcpinto_lobHandler(LOBULKINTO *lob_info);

static i4 IIcpputrow(
	II_CP_STRUCT *copy_blk,
	i4 len,
	PTR row);

static void IIcpinto_csv(
	II_CP_STRUCT *cblk,
	II_CP_MAP *cmap,
	DB_TEXT_STRING *txt_ptr,
	char *row_ptr);

static STATUS IIcpfrom_init (
	II_LBQ_CB *IIlbqcb, 
	II_CP_STRUCT *copy_blk, 
	II_CP_GROUP *cgroup,
	PTR *tuple);

static STATUS IIcpfrom_binary(
	II_THR_CB *thr_cb,
	II_CP_STRUCT *copy_blk,
	II_CP_GROUP *cgroup,
	PTR tuple);

static STATUS IIcpfrom_formatted(
	II_THR_CB *thr_cb,
	II_CP_STRUCT *copy_blk,
	II_CP_GROUP *cgroup,
	PTR tuple);

static bool IIcp_isnull(
	II_CP_MAP *cmap,
	u_char *val_start,
	i4 vallen);

static i4 IIcpfopen(
	II_CP_STRUCT *copy_blk);

static i4 IIcpfclose(II_CP_STRUCT *copy_blk);

static i4 cp_refill(
	u_char **dataend_p,
	u_char **rowptr_p,
	II_CP_STRUCT *copy_blk);

static STATUS IIcpdbwrite(
	II_LBQ_CB *IIlbqcb,
	II_CP_STRUCT *copy_blk,
	II_CP_GROUP *cgroup);

static i4 IIcpdbflush(
	II_LBQ_CB *IIlbqcb,
	II_CP_STRUCT *copy_blk);

static void IIcpendcopy(
	II_LBQ_CB *IIlbqcb,
	II_CP_STRUCT *copy_blk);

static void IIcpabort(
	II_LBQ_CB *IIlbqcb,
	II_CP_STRUCT *copy_blk);

static void IIcpcleanup(
	II_CP_STRUCT *copy_blk);

static STATUS IIcpfrom_lob(
	II_THR_CB	*thr_cb,
	II_CP_STRUCT	*cblk,
	II_CP_GROUP	*cgroup);

static i4 IIcpfrom_seglen(
	II_CP_STRUCT *cblk,
	II_CP_MAP *cmap,
	bool extendval);

static void IICPsebSendEmptyBlob();

static void IIcpfrom_lobHandler(LOBULKFROM *lob_info);

static i4 IIcplgopen(II_CP_STRUCT *copy_blk);

static void IIcplginto(
	II_CP_STRUCT *copy_blk,
	PTR tuple);

static void IIcplgcopyn(u_char *rowptr, II_CP_STRUCT *cblk);

static void IIcplgfrom(
	II_CP_STRUCT *copy_blk);

static i4 IIcplgclose(II_CP_STRUCT *copy_blk);

static STATUS IIcp_handler(
	EX_ARGS *exargs);

static void IICPwemWriteErrorMsg(
	i4 ernum,
	II_CP_STRUCT *cblk,
	char *col_name,
	char *msg);

static STATUS IICPvcValCheck(
	II_LBQ_CB *IIlbqcb,
	char *col_aligned, 
	II_CP_MAP *copy_map,
	II_CP_STRUCT *copy_blk );

static void IICPdecompress(
	ROW_DESC *rdesc,
	i4 first_attno,
	i4 last_attno,
	PTR bptr,
	i4 bytesleft,
	i4 *dif,
	PTR tuple_ptr,
	bool *partial);

static void IIcpdbgdump(
	II_LBQ_CB *IIlbqcb,
	II_CP_STRUCT *copy_blk,
	char *copydbg_file );

static void IIcpdbgopen(
	II_CP_STRUCT *copy_blk);

static void IIcpdbgclose(
	II_CP_STRUCT *copy_blk);

/*{
+*  Name: IIcopy - Drives FE side of COPY protocol.
**
**  Description:
**	IIcopy does all of FE side of COPY.
**
**	During COPY INTO, it reads blocks of tuples from the DBMS, converts
**	them to the copy file format and writes them into the COPY FILE.
**
**	During COPY FROM, it reads rows from a COPY FILE, converts them to
**	tuples, and sends blocks of tuples to the DBMS for loading.
**
**	IIcopy is called from LIBQ.  The front end sends a copy statement off
**	to the DBMS, and the DBMS sends back a GCA_C_INTO or a GCA_C_FROM 
**	message block signalling that a copy is to be performed.  This block
**	contains all the information needed for the front end to process the
**	COPY statement.  When LIBQ receives the GCA_C_INTO or the GCA_C_FROM
**	block, it will call IIcopy.  IIcopy will remain in control until the 
**	copy is completed or interrupted by the user.
**
**	The backend reads or sends entire table tuples in table definition
**	order (no reordering of columns).  In order to work with LOB's, COPY
**	(either direction) operates on groups.  A group is either a single
**	LOB, or a set of non-LOB columns in between LOB's.  In the simplest
**	case of no LOBs at all, the entire tuple is one group.  The column
**	group idea allows LOB and non-LOB COPY to operate in the same way.
**	(Originally, LOB copy was a totally separate code path, which
**	obviously caused a lot of code duplication and misery.)
**
**  Inputs:
**	msg_type	- Message type to IIcopy.
**	user_routine	- user routine to get/put copy rows, rather than doing
**			  IO to a copy file.  Described above.
**
**  Outputs:
**	Returns: OK, FAIL
-*
**  Side Effects:
**	IICGC_COPY_MASK is set in IIlbqcb->ii_lq_gca->cgc_g_state.  This will
**	    prevent LIBQ from calling IIcopy again while already processing
**	    a COPY.  This bit will be turned off when COPY is done.
**	The libq control block contains an ADF_CB that is used throughout libq.
**	    This ADF control block keeps track of the number of noprintable
**	    characters that ADF encountered and just converted to blank.  This
**	    field is set to zero so that we can count just those that occur
**	    during copy.
**
**  History:
**	01-dec-1986 (rogerk)
**	    Written.
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr parameter.
**	10-sep-87 (puree)
**	    convert to GCF architecture.
**	04-aug-88 (neil)
**	    added check to trace statement.
**	31-oct-95 (kch)
**	    switch off II_LO_END flag for ascii 'copy ... from' in Large
**	    Object Info structure (IIlbqcb->ii_lq_lodata.ii_lo_flags).
**	    This allows multiple ascii 'copy ... froms' in the same
**	    tm/itm session. This change fixes bug 72235.
**	21-Dec-01 (gordy)
**	    Removed PTR_BITS_64 code since GCA_COL_ATT no longer has PTR. 
**	27-jan-2004 (somsa01)
**	    Pass user_routine to BLOB handler routines.
**	31-oct-2006 (kibro01) bug 116565
**	    If we can't open the output file for a 'copy ... into' then
**	    end cleanly rather than aborting so the server knows to get
**	    rid of the structures associated with the query.
**	31-oct-2007 (wanfr01) bug 119148
**	    Remove redundant IIdbg_dump from IIcopy
**	    If II_G_DEBUG is set, you already called it from IIdbg_gca.
**	30-Nov-2009 (kschendel) SIR 122974
**	    Invent grouping notion to unify LOB and non-LOB copying.
*/

i4
IIcopy(msg_type, user_routine)
i4	    msg_type;			/* GCA message type */
i4	    (*user_routine)();		/* Function to get/put rows */
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    PTR			row;
    PTR			tuple;
    i4			status;
    i4			save_truncated;
    II_CP_STRUCT	copy_blk;
    II_CP_GROUP		*cgroup, *last_cgroup;
    EX_CONTEXT		excontext;
    GCA_AK_DATA		gca_ack;
    II_CPY_BUFH		cpy_bufh;	/* COPY tuple buffer header */
    char		*copydbgbuf = NULL;
    char		*copydbgbuf2 = NULL;
 
    /*
    ** Set mask signifying that a COPY is in progress.
    **
    ** LIBQ calls IIcopy when it gets a GCA_C_INTO or GCA_C_FROM block from
    ** the DBMS.  If this routine has been called directly from LIBQ, we don't
    ** want to call IIcopy again, so this bit tells LIBQ that we are already
    ** in copy processing.  This also prevents multiple IIcopy calls if the 
    ** startup information requires more than one GCA_C_INTO/GCA_C_FROM block.
    */
 
    cgc->cgc_g_state |= IICGC_COPY_MASK;
    cgc->cgc_tdescr = (GCA_TD_DATA *)NULL;

    /*
    ** Set global pointer to copy struct and define interrupt handler.  If
    ** an interrupt is received, the interrupt handler will use the copy
    ** struct to determine what has to be cleaned up.  Zero out copy struct
    ** before declaring handler, so we know that handler can trust that the
    ** values in the struct aren't garbage.
    */
    MEfill(sizeof(II_CP_STRUCT), '\0', (PTR) &copy_blk);
    thr_cb->ii_th_copy = (PTR)&copy_blk;
    if (EXdeclare(IIcp_handler, &excontext))
    {
	/* Should never get to here!! */
	IIlocerr(GE_LOGICAL_ERROR, E_CO0022_COPY_INIT_ERR, II_FAT,
		 0, (char *)0);
	IIcpabort( IIlbqcb, &copy_blk );
	EXdelete();
	return (FAIL);
    }

    /* Open II_COPY_DEBUG file now in case init wants to use it. */
    copy_blk.cp_dbgname = NULL;
    copy_blk.cp_dbg_ptr= NULL;
    NMgtAt(ERx("II_COPY_DEBUG"), &copydbgbuf2);
    if ((copydbgbuf2 != NULL) && (*copydbgbuf2 != EOS))
    {
	i4	dbg_len;

	dbg_len = STlength(copydbgbuf2);
	if (loc_alloc(1, dbg_len, (PTR *)&copy_blk.cp_dbgname) == OK)
	    MEcopy(copydbgbuf2, dbg_len, copy_blk.cp_dbgname);
	IIcpdbgopen(&copy_blk);
    }

    /*
    ** Build copy struct from GCA_C_INTO/GCA_C_FROM block.
    **
    ** This fills in all the information on the tuple and row formats, as
    ** well as the ADF conversion information on how go from one to the other.
    */
    copy_blk.cp_cpy_bufh = &cpy_bufh;
    if ((status = IIcpinit( IIlbqcb, &copy_blk, msg_type)) != OK)
    {
	/*
	** If status is FAIL, then error has already been reported by IIcpinit.
	** Otherwise, we need to report the error.
	*/
	if (status != FAIL)
	    IIlocerr(GE_LOGICAL_ERROR, status, II_ERR, 0, (char *)0);
 
	IIcpabort( IIlbqcb, &copy_blk );
	EXdelete();
	return (FAIL);
    }

    /* print out the II_CP_STRUCT and II_CP_MAP structures for debugging */
    NMgtAt(ERx("II_COPY_DBG_DUMP"), &copydbgbuf);
    if ((copydbgbuf != NULL) && (*copydbgbuf != EOS))
	IIcpdbgdump( IIlbqcb, &copy_blk, copydbgbuf );

    /*
    ** If this is a normal (not in_memory) copy, then open the copy file.
    */
    if (copy_blk.cp_program)
	copy_blk.cp_usr_row = (i4 (*)(i4 *,PTR,i4 *))user_routine;
    else
    {
	if ((status = IIcpfopen(&copy_blk)) != OK)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_CO0022_COPY_INIT_ERR, II_FAT,
		     0, (char *)0);
	    /* Note the error, but end nicely - this is a problem caused
	    ** entirely at the client end, so the server needs to be
	    ** told about it to get rid of QEF plans
	    ** Bug 116565 (kibro01)
	    */
	    copy_blk.cp_error = TRUE;
	    IIcpendcopy( IIlbqcb, &copy_blk);
	    EXdelete();
	    return (FAIL);
	}
    }
    /*
    ** If error row logging has been requested, open the error row log.
    */
    if (copy_blk.cp_rowlog)
    {
	if ((status = IIcplgopen(&copy_blk)) != OK)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_CO0022_COPY_INIT_ERR, II_FAT,
		     0, (char *)0);
	    copy_blk.cp_error = TRUE;
	    IIcpendcopy( IIlbqcb, &copy_blk);
	    EXdelete();
	    return (FAIL);
	}
    }

    if ( copy_blk.cp_has_blobs )
    {
	/* Must have LOBs, turn on copy flag in libq.  This tells LIBQ
	** to generate CDATA instead of TUPLES messages when it's
	** streaming large objects.
	*/
	IIlbqcb->ii_lq_flags |= II_L_COPY;
    }

    /*
    ** Everything is set.  Send an ACK block to the DBMS.
    */
    gca_ack.gca_ak_data = 0;
    IIcgcp1_write_ack(cgc, &gca_ack);
    /*
    ** THE COPY LOOP:
    ** Loop over rows;  for each row, loop over copy groups.
    */

    last_cgroup = &copy_blk.cp_copy_group[copy_blk.cp_num_groups-1];
    do
    {
	for (cgroup = &copy_blk.cp_copy_group[0]; cgroup <= last_cgroup; ++cgroup)
	{
	    if (copy_blk.cp_direction == CPY_INTO)
	    {
		/* Read group full or partial tuple from server.
		** For LOB group this will do all the work because the
		** LOB may have to be streamed into the file.
		*/
		status = IIcpdbread(IIlbqcb, &copy_blk, cgroup, &tuple);
		if ( status != OK )
		{
		    /*
		    ** If return is ENDFILE, then we have received all the
		    ** tuples, that is not an error, but break out.
		    */
		    if (status != ENDFILE)
			copy_blk.cp_error = TRUE;
		    break; 
		}
		/* LOB group is all done, cycle around.
		** For "toss" groups that exist just to read and move past
		** data not needed in the output, we've read the group,
		** just leave it on the floor.
		*/
		if (cgroup->cpg_is_tup_lob || cgroup->cpg_first_map == NULL)
		    continue;

		/* Initialize pointer to file row buffer */
		if ((status = IIcpinto_init(&copy_blk, &row, tuple)) != OK)
		{
		    copy_blk.cp_error = TRUE;
		    break; 
		}
		/*
		** Convert group tuple to row format and write to the copy
		** file.  In a bulk copy, no conversion is necessary, since
		** the row and tuple formats are identical.
		*/
		if (copy_blk.cp_convert)
		{
		    status = IIcprowformat( IIlbqcb, &copy_blk, cgroup, tuple, row);
		    if ( status != OK)
		    {
			/* 
			** If row logging, then write the tuple to the log file.
			** Note that we only row log if there are no LOBs,
			** ie there's just one group.
			*/ 
			if (copy_blk.cp_rowlog)
			    IIcplginto(&copy_blk, tuple);
			break;
		    }
		}
		/*
		** Only write out the group if the above conversion
		** was successful.
		*/
		if ((status = IIcpputrow(&copy_blk, cgroup->cpg_row_length, row)) != OK)
		{
		    copy_blk.cp_error = TRUE;
		    break;
		}
	    } /* COPY INTO */
	    else
	    {
		/* COPY FROM direction */
		/* Set up target tuple pointer/buffer */
		if ( (status = IIcpfrom_init( IIlbqcb, &copy_blk,
					cgroup, &tuple )) != OK )
		{
		    copy_blk.cp_error = TRUE;
		    break; 
		}

		if (cgroup->cpg_is_row_lob)
		{
		    /* Do entire LOB, formatted or not */
		    status = IIcpfrom_lob(thr_cb, &copy_blk, cgroup);
		    if (status != OK)
			break;
		}
		else
		{

		    /* Save truncated count in case we get some truncates
		    ** and then an error, row not copied to table.
		    */
		    save_truncated = copy_blk.cp_truncated;

		    /* Get a group.  Converting copy has to bumble about,
		    ** column by column.  Non-converting copy can do simple
		    ** moves, perhaps with post-validation or
		    ** post-normalization.
		    */
		    if (copy_blk.cp_convert)
			status = IIcpfrom_formatted(thr_cb, &copy_blk, cgroup, tuple);
		    else
			status = IIcpfrom_binary(thr_cb, &copy_blk, cgroup, tuple);

		    /*
		    ** Break out if error or EOF.  Error may log the row first.
		    */
		    if (status == ENDFILE)
			break;
		    else if (status != OK)
		    {
			/* 
			** Write the input row to the log file if logging.
			*/ 
			if (copy_blk.cp_rowlog)
			    IIcplgfrom(&copy_blk);

			/* Don't include any truncations in error row */
			copy_blk.cp_truncated = save_truncated;
			break;
		    }
		}
		/*
		** Only send the tuple if the above conversion was successful.
		*/
		if ((status = IIcpdbwrite(IIlbqcb, &copy_blk, cgroup)) != OK)
		{
		    copy_blk.cp_error = TRUE;
		    break;
		}
	    } /* copy FROM */
	} /* end group FOR */

	/* Row post-processing */
	if (status == OK)
	{
	    ++copy_blk.cp_rowcount;
	    /*
	    ** Decide whether to poll the DBMS to see if it got an error.
	    ** (If we don't poll, we could end up spraying the entire input
	    ** file at the backend to no purpose.)  A poll involves a
	    ** no-timeout read on the expedited channel, so while it's
	    ** not all that expensive, we don't want to do it too often
	    ** either.
	    ** If a flush probably happened this row, count up the "poll yet"
	    ** counter, and after 10 (a completely arbitrary number),
	    ** we'll check for dbms errors.
	    */
	    if (copy_blk.cp_direction == CPY_FROM)
	    {
		if (copy_blk.cp_sent)
		    ++ copy_blk.cp_pollyet;
		if (copy_blk.cp_pollyet >= 10)
		{
		    if (IIcgcp4_write_read_poll(cgc, GCA_CDATA) 
					    != GCA_CONTINUE_MASK)
		    {
			copy_blk.cp_dbmserr = TRUE;
			status = FAIL;
			break;
		    }
		    copy_blk.cp_pollyet = 0;
		}
		/* Reset for next row */
		copy_blk.cp_sent = FALSE;
	    }
	}
	else if (status != ENDFILE)
	{
	    /* 
	    ** If we have exceeded the maximum errors allowed, then
	    ** terminate the copy.  Count row "processed" whether OK or error.
	    */ 
	    ++copy_blk.cp_badrows;
	    ++copy_blk.cp_rowcount;
	    if ((!copy_blk.cp_continue) &&
		(copy_blk.cp_badrows >= copy_blk.cp_maxerrs))
	    {
		copy_blk.cp_error = TRUE;
		break;
	    }
	    status = OK;
	}
    } while (status == OK);


    /* Assure that any LOB flags are turned off, if they were ever on */
    IIlbqcb->ii_lq_flags &= ~II_L_COPY;
    /* Bug 72235 - Turn off II_LO_END flag in libq */
    if (copy_blk.cp_convert)
	IIlbqcb->ii_lq_lodata.ii_lo_flags &= ~II_LO_END;

    /*
    ** Print out any ending warnings, send status to the DBMS if copy ended
    ** due to an error.  Clean up memory, close files ...
    */
    IIcpendcopy( IIlbqcb, &copy_blk);
    EXdelete();
    return (copy_blk.cp_error ? FAIL : OK);
}

/*{
+*  Name: IIcpinit - Initialize for COPY processing.
**
**  Description:
**	This routine initializes the copy structure.  This includes:
**	    - Read in the GCA_C_INTO/GCA_C_FROM information from the DBMS.
**	      This information includes:
**		- direction of COPY
**		- tuple format of copy table
**		- row format of copy file
**		- copy file name (if there is one)
**		- whether this is a PROGRAM copy
**		- action to take on errors (CONTINUE, TERMINATE)
**		- maximum error count
**		- error row log name (if there is one)
**	    - Figure out the tuple row length.
**	    - Fill in the ADF function block for each copy domain with
**	      the conversion information, source and dest data types.
**	    - Figure out the copy group structure and map domains to groups.
**	    - Figure out whether data value checking is necessary.
**	    - Figure out whether conversion is necessary.
**	    - Figure out the copy file type
**	    - Build an "empty" tuple that can be used to build tuples during
**		COPY FROM
**	    - Initialize the ADF count of nonprintable chars converted to blank
**		so we can count just the ones that occured in COPY.
**	    - Set up row buffering.
**
**  Inputs:
**	copy_blk	- copy control block
**	msg_type	- GCA message type
**
**  Outputs:
**	copy_blk	- copy control block is filled in
**	cpy_bufh
**	    cpy_dbv.db_data	- point to the beginning of tuple buffer.
**	    cpy_dbv.db_length	- available space in the tuple buffer.
**	    cpy_bytesused	= 0
**
**	Returns: OK, FAIL
-*
**  Side Effects:
**	The libq control block contains an ADF_CB that is used throughout libq.
**	This ADF control block keeps track of the number of noprintable chars
**	that ADF encountered and just converted to blank.  This field is set
**	to zero so that we can count just those that occur during copy.
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	25-aug-87 (puree)
**	    fix problem with variable length fields.
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	10-sep-87 (puree)
**	    convert to GCF architecture.
**	24-feb-88 (puree)
**	    Null terminated column names for better error reporting format.
**	09-mar-88 (puree)
**	    cannot use STtrmwhite to terminate the column name.  It strips
**	    off delimiter names (such as nl = d1).  Must prefill the
**	    field with 0.
**      10-nov-88 (barbara)
**          fix bug #3314 regarding rowlen for dummy domain on COPY FROM
**	21-jun-89 (barbara)
**	    fixed bug #6380 where cp_name is an empty string for null
**	    dummy columns.
**	22-sep-89 (barbara)
**	    added support for decimal datatype.
**	07-feb-90 (fredp)
**	    On BYTE_ALIGN machines, adc_getempty() expects aligned
**	    DB_DATA_VALUEs.  So, if needed, we use aligned temp. space 
**	    then subsequently copy the "empty" value into 
**	    copy_blk->cp_zerotup.
**	07-feb-90 (fredp)
**	    On BYTE_ALIGN machines, adc_getempty() expects aligned
**	    DB_DATA_VALUEs.  So, if needed, we use aligned temp. space 
**	    then subsequently copy the "empty" value into 
**	    copy_blk->cp_zerotup.
**	22-may-90 (fredb)
**	    Force CPY_NOBIN status for MPE/XL "copy Into" requests. MPE/XL's
**	    file system rounds odd record lengths up to an even length for
**	    BIN type files. Using a VAR type file gets around this problem.
**	    It is still not perfect, but it works ...
**	11-oct-90 (teresal)
**	    Put in Ken Watts fix so that copy will detect a newline dummy
**	    field (e.g., nl=d1) at the end of a copy format and create a
**	    TXT type file.
**	18-oct-90 (teresal)
**	    Put in fix for Unknown Exception error which occurred because row
**	    buffer was not big enough to handle a variable length copy format
**	    with "WITH NULL(value)" where value was longer than the column's
**	    length.  Row buffer size is now increased by the null length
**	    to make room for the large null string.
**	26-dec-90 (teresal)
**	    Bug fix 6936 - give an error if text filetype is used with a
**	    bulk copy.
**	04-jan-91 (teresal)
**	    ICL fix - only look for filetype if VMS.  Filetype is not
**	    applicable otherwise.
**	10-jan-91 (teresal)
**	    Bug fix 35154: Fixed to calculate row length correctly.
**	    For "varchar(0)" copy format, need to add the ascii length
**	    specifier (5 bytes) to row length.
**	22-apr-1991 (teresal)
**	    Put back declarations for "cp" and "cp2" - these
**	    were inadvertently deleted and are needed for VMS.
**	06-may-1991 (teresal)
**	    Bug fix 37341.
**	06-aug-1991 (teresal) 
**	    Accept new copy map format for the 'with null' value.
**	    Note the new copy map also sends precision as a 
**	    separate value.
**	23-sep-1993 (mgw) Bug #55199
**	    Don't build an "empty" tuple for use when converting from
**	    row format if there are no non-long columns
**	    (copy_blk->cp_tup_length == 0)
**      11-nov-1993 (smc)
**	    Bug #58882
**          Removed ALIGN_RESTRICT & NOT_ALIGNED both of which are more
**          elegently defined in mecl.h. NOT_ALIGNED is replaced with 
**          ME_NOT_ALIGNED_MACRO which has the advantage of not
**          generating wheelbarrows full of lint truncation warnings
**          on boxes where size of ptr > int.
**      17-feb-1993 (johnst)
**	    Bug #58882
**          Corrected file merge problem in previous change, where the
**          "#define NOT_ALIGNED..." line was not deleted in the submission.
**          Delete the line now.
**	07-nov-1995 (kch)
**	    Change blob check from (IIDT_LONGTYPE_MACRO && IIDT_UDT_MACRO)
**	    to (IIDT_LONGTYPE_MACRO || IIDT_UDT_MACRO). The AND case could
**	    never be true; with the two macros ORed, the value
**	    copy_map->cp_nuldata will be correctly set, and an ascii
**	    'copy .. into' will write the correct null blob values to the
**	    data file. This change fixes bug 72054.
**	21-nov-1995 (kch)
**	    Do not set copy_blk->cp_varrows to 1 if the column is dummy.
**	    This change fixes bug 72678.
**	13-dec-1995 (kch)
**	    Add counter for long types, long_count, and increment this
**	    counter if a long type column is encountered. Then use
**	    long_count to commpensate for the fact that the offset for
**	    a long type column is (-1) when filling the "empty" tuple,
**	    copy_blk->cp_zerotup. This change fixes bug 73123.
**	07-feb-1996 (kch)
**	    The long type counter, long_count, is now also incremented for
**	    non byte aligned platforms.
**	26-mar-1996 (kch)
**	    Back out previous fix for bug 72678 (this change was causing
**	    75437). Bug 72678 has now been fixed elsewhere as part of 'fix'
**	    for 75437.
**	05-apr-1996 (loera01)
** 	    Modified to support compressed variable-length datatypes.
**      13-aug-1996 (loera01)
**          Modified allocation of cp_rowbuf and cp_tupbuf so that the
**          buffer lengths consist of the row or tuple length plus
**          MAX_SEG_FILE.  With variable page size, non-blob columns plus 
**          a blob may be sent in the same message from the back end.
**          Fixes bug 77129.
**	20-nov-96 (kch)
**	    We now do not add the default delimiter ('\0') for a copy into
**	    of non varchar formats that have no delimiter specified. This
**	    change fixes bug 79476 (side effect of the fixes for bugs 71149
**	    and 73304).
**      17-Sep-97 (fanra01)
**          Ensure that the compression bit flag is only set if performing a
**          COPY INTO operation.
**          Switch off varchar compression when performing COPY FROM as this
**          is not supported and can cause net to misinterpret the tuple
**          contents.
**      23-feb-98 (thaju02)
**          Bug 89147 - changed STlcopy() to MEcopy().In copying the cp_nuldata
**          STlcopy appended null terminator which overwrote another memory
**          buffers header, resulting in either E_CO0037 or exception in
**          MEdoAlloc() or MEfree().
**      04-Aug-1998 (wanya01)
**          X-integrate change #436526. Bug 91904.      
**          No 'continue' when incrementing long_count on non
**          byte-aligned platforms. Led to exception in MEdoAlloc() when
**          coping in ascii tables with long byte/varchar columns.
**	21-Dec-2001 (gordy)
**	    Added function to generate GCA tuple descriptor.
**	03-Dec-2003 (gupsh01)
**	    Modified copy code so that nchar and nvarchar types will now use 
**	    UTF8 to read and write from the text file for a formatted copy.
**	17-Jan-2006 (thaju02)
**	    Test copy_map->cp_nultype for blob datatype. (B115747)
**	22-May-2006 (thaju02)
**	    For long nvarchar, coerce null value to UTF8. (B116124)
**	03-Aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
**	8-Sep-2008 (gupsh01)
**	    Fix usage of CMdbl1st for UTF8 and multibyte character sets
**	    which can can be upto four bytes long. Use CMbytecnt to ensure
**	    we do not assume that a multbyte string can only be a maximum
**	    of two bytes long. (Bug 120865)
**	2-Dec-2009 (kschendel) SIR 122974
**	    Revise buffer-size calculations.  Set various new copy-struct
**	    and copy-map fields to support new buffering.  Build group
**	    data structures.
**	3-Jan-2010 (kschendel) SIR 122974
**	    Missed a same-desc under VMS, fix to be !convert.
**	28-Jan-2010 (kschendel) SIR 122974
**	    A copy list like (x=c10) fails on VMS;  the setting of
**	    cp_row_length was deleted when the copy group work was done.
**	    Restore the cp_row_length calculation (it only has to be right
**	    for fixed length records).
**	    Don't treat unicode domains as "text" on Windows, such files
**	    can contain control-Z which translates to EOF in text mode. :-(
**	25-Feb-2010 (kschendel) SIR 122974
**	    Fix the initialization of zerotup: the attr_offsets array
**	    no longer includes LOB's at all, so don't adjust for them.
**	2-Mar-2010 (kschendel) SIR 122974
**	    Fix a couple null-data issues:  byte varying is a db-text-string
**	    type, and we don't want to fool with nvarchar null-data.
**	7-May-2010 (kschendel) SIR 122974
**	    Total row length (cp_row_length) still wrong, fix.
**	10-May-2010 (kschendel) SIR 122974
**	    Very large binary (non-converting) COPY FROM needs an unusually
**	    large read size.
**	7-Jul-2010 (kschendel) SIR 124038
**	    Allow Windows to specify a filetype so that advanced users can
**	    control the file interpretation mode.  (CRLF conversion, control-Z
**	    as EOF.)
**	4-Aug-2010 (kschendel) b124193
**	    Re-enable the error-log file if copy from program.
**	    At one point, I had thought to implement this by seeking the
**	    input (since COPY no longer constructs an entire input row).
**	    This would have been impossible for copy from program.
**	    This was changed to copy on the fly to an error row buffer, but
**	    logging was never turned back on for program.  Fix.
*/

static STATUS
IIcpinit( II_LBQ_CB *IIlbqcb, II_CP_STRUCT *copy_blk, i4  msg_type )
{
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    DB_DATA_VALUE	*col_desc;
    II_CP_MAP		*copy_map, *cmap, *last_cmap;
    II_CP_GROUP		*cgroup;
    II_CPY_BUFH		*cpy_bufh = copy_blk->cp_cpy_bufh;
    ROW_DESC		*row_desc;		/* Row descriptor */
    DB_DATA_VALUE	dbv, dbv2, dbv4;
    DB_STATUS		dbstat;
    PTR			cp, cp2;
    i4			*attr_offset;	/* Column offsets array */
    i4			offset;
    i4			i2_val;
    i4 			i4_val;
    i4			fname_length, lname_length;
    i4			i;
    i4			num_read;
    i4			cp_namelen;
    i4			max_rowlen;
    i4			absttype;
    bool		non_lob_group;
    bool		all_chr_type;
    bool		chr_row_type;
    bool		any_varrows;

 
    copy_blk->cp_rowcount = 0;
 
    /*
    ** Fill in the Copy control block from the CGA_C_INTO/CGA_C_FROM
    ** block sent from the DBMS.
    */
 
    II_DB_SCAL_MACRO(dbv2, DB_INT_TYPE, sizeof(i4), &i2_val);
    II_DB_SCAL_MACRO(dbv4, DB_INT_TYPE, sizeof(i4), &i4_val);
 
    /* gca_status_cp */
    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv4))
		!= sizeof(i4))
	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
 
    copy_blk->cp_status = (i4)i4_val;

    /* Set convert according to CPY_NOTARG (no targets).
    ** NOTARG --> no convert, otherwise it's a formatted copy and
    ** we insist on all non-dummy targets having coercion FI's.
    */
    copy_blk->cp_convert = ((copy_blk->cp_status & CPY_NOTARG) == 0);
 
    /* gca_direction_cp */
    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv4)) 
		!= sizeof(i4))
	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
 
    copy_blk->cp_direction = (i4)i4_val;

# ifdef hp9_mpe         /* MPE/XL specific kludge */
    /*
    ** MPE/XL's copy files should not be fixed binary due to the file
    ** system rounding odd numbered recordlengths up to an even number.
    */
    if (copy_blk->cp_direction == CPY_INTO)
	copy_blk->cp_status |= CPY_NOTBIN;
# endif
 
    /* gca_maxerrs_cp */ 
    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv4)) 
		!= sizeof(i4))
	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
    copy_blk->cp_maxerrs = (i4)i4_val;
 
    /* gca_l_fname_cp */
    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
		!= sizeof(i4))
	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
    fname_length = (i4)i2_val;
    if (i2_val < 0 || i2_val > MAX_LOC)
	return (E_CO0022_COPY_INIT_ERR);
 
    /*
    ** If there is a filename, then allocate memory for it and read it in.
    ** The value of "fname_length" must be preserved as it is used below.
    */
    if (fname_length > 0)
    {
	if (loc_alloc(1, fname_length, (PTR *) &copy_blk->cp_filename) != OK)
	    return (E_CO0037_MEM_INIT_ERR);
 
	II_DB_SCAL_MACRO(dbv, DB_CHR_TYPE, fname_length, copy_blk->cp_filename);
 
	/* gca_fname_cp[] */
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv)) 
		    != fname_length)
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
    }
    else 
    {
	copy_blk->cp_filename = NULL;
    }
 
    /*
    ** Read in the logfile length and name (if there is one).
    */
    /* gca_l_logname_cp */
    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
		!= sizeof(i4))
	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
    lname_length = (i4)i2_val;
    if (i2_val < 0 || i2_val > MAX_LOC)
	return (E_CO0022_COPY_INIT_ERR);
 
    if (lname_length > 0)
    {
	if (loc_alloc(1, lname_length, (PTR *)&copy_blk->cp_logname) != OK)
	{
	    return (E_CO0037_MEM_INIT_ERR);
	}
	II_DB_SCAL_MACRO(dbv, DB_CHR_TYPE, lname_length, copy_blk->cp_logname);
	/* gca_logname_cp[] */
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv)) 
		!= lname_length)
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
    }
    else 
    {
	copy_blk->cp_logname = NULL;
    }
 
    /* tuple descriptor - gca_tup_desc_cp */
    row_desc = &IIlbqcb->ii_lq_retdesc.ii_rd_desc;
    if (IIrdDescribe(msg_type, row_desc) != OK)
	return (E_CO0022_COPY_INIT_ERR);
 
    /*
    ** Set up descriptor for INGRES/NET - allows presentation layer
    ** to process the tuple data.
    */
    if ( copy_blk->cp_direction == CPY_FROM )
    {
	if ( (cgc->cgc_tdescr = IIcptdescr( row_desc )) == NULL )
	    return( E_CO0022_COPY_INIT_ERR );
    }
    else  if (row_desc->rd_flags & RD_CVCH)
	row_desc->rd_gca->rd_gc_res_mod |= GCA_COMP_MASK;
	
    copy_blk->cp_tup_length = (i4)row_desc->rd_gca->rd_gc_tsize;
 
    /*
    ** Get the number of row domains and allocate an II_CP_MAP struct for
    ** each one.  These structs will be filled in with the rest of the
    ** information in the copy block.
    */
    /* Number of file row domains - gca_l_row_desc_cp */
    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv4)) 
		!= sizeof(i4))
	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
    copy_blk->cp_num_domains = (i4)i4_val;
    if (i4_val < 0 || i4_val > RD_MAX_COLS)
	return (E_CO0022_COPY_INIT_ERR);
 
    /* Allocate and zero all the copy maps */
    if (loc_alloc_i(copy_blk->cp_num_domains, sizeof(II_CP_MAP),
	(PTR *) &copy_blk->cp_copy_map) != OK)
    {
	return (E_CO0037_MEM_INIT_ERR);
    }
 
    /*
    **  Read in the copy map information (II_CP_MAP) for each file row domain
    **  from gca_row_desc.
    */
    for (i = 0; i < copy_blk->cp_num_domains; i++)
    {
	copy_map = &copy_blk->cp_copy_map[i];
	/* Zeroed by allocation */
 
	/* domain name - gca_domname_cp[] */
	II_DB_SCAL_MACRO(dbv, DB_CHR_TYPE, GCA_MAXNAME, copy_map->cp_name);
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv)) 
		    != GCA_MAXNAME)
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
 
	/* row type - gca_type_cp */
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
		    != sizeof(i4))
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	copy_map->cp_rowtype = (DB_DT_ID)i2_val;
 
	/* Only read gca_precision_cp if we have a new copy map */
	if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_50)
	{
	    /* row precision - gca_precision_cp  */
	    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
			!= sizeof(i4))
	    	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	    copy_map->cp_rowprec = (i4)i2_val;
	}
	else
	    copy_map->cp_rowprec = 0;	
		
	/* row length - gca_length_cp */ 
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv4)) 
		    != sizeof(i4))
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	copy_map->cp_rowlen = (i4)i4_val;

	/* user delimiter flag - gca_user_delim_cp */
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
		    != sizeof(i4))
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	copy_map->cp_isdelim = i2_val;
 
	/* number of delimiter characters - gca_l_delim_cp */ 
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
		    != sizeof(i4))
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	/* *** Toss this; the backend always sends CPY_MAX_DELIM
	** delimiter chars, so gca_l_delim_cp is always CPY_MAX_DELIM.
	** Could check that i2_val is == CPY_MAX_DELIM, I guess.
	*/
 
	/* delimiter characters - gca_delim_cp[] */
	II_DB_SCAL_MACRO(dbv, DB_CHR_TYPE, CPY_MAX_DELIM, copy_map->cp_delim);
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv)) 
		    != CPY_MAX_DELIM)
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
 
	/* attribute map - gca_tupmap_cp */
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv4)) 
		    != sizeof(i4))
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	copy_map->cp_attrmap = (i4)i4_val;
 
 
	/* ADF function instance id - gca_cvid_cp */
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
		    != sizeof(i4))
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	copy_map->cp_cvid = (ADI_FI_ID)i2_val;
 
	/* Only read gca_cvprec_cp if we have a new copy map */
	if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_50)
	{
	    /* precision of conversion result - gca_cvprec.cp  */
	    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
			!= sizeof(i4))
	    	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	    copy_map->cp_cvprec = (i4)i2_val;
	}
	else	
	    copy_map->cp_cvprec = 0; 

	/* length of conversion result - gca_cvlen_cp */
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
		    != sizeof(i4))
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	copy_map->cp_cvlen = i2_val;

	/* withnull option flag - gca_withnull_cp */    
	if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
		    != sizeof(i4))
	    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	copy_map->cp_withnull = (bool)i2_val;

	/*
	** Check GCA protocol level to determine if we have a new
	** version of the copy map.  Old version of the copy map
	** has gca_nullen_cp, new version has the null value information
	** for type, length, prec and data in a GCA_DATA_VALUE format.  
	** The new copy map is required to fix bug 37564.
	*/
	if (cgc->cgc_version >= GCA_PROTOCOL_LEVEL_50)
	{
	    /* New version of copy map - gca_nullvalue_cp */		

	    /* with null value sent? - gca_l_nullvalue_cp */    
	    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
	    		!= sizeof(i4))
	    	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);

	    /* gca_nullvalue_cp is only sent if gca_l_nullvalue_cp is TRUE */
	    if (i2_val)
	    {
		/* null data type */
		if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
			!= sizeof(i4))
		    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
		copy_map->cp_nultype = (DB_DT_ID)i2_val;

		/* null precision */
		if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
			!= sizeof(i4))
		    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
		copy_map->cp_nulprec = (i2)i2_val;

		/* null length */
		if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv4)) 
			!= sizeof(i4))
		    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
		copy_map->cp_nullen = (i4)i4_val;

	    }
	    else
	    {
	        copy_map->cp_nultype = 0;
	        copy_map->cp_nullen = 0;
	        copy_map->cp_nulprec = 0;
	    }
	}
	else
	{
	    /* Old version of copy map */	

	    /* length of value for null - gca_nullen_cp */
	    if ((num_read = IIcgc_get_value(cgc, msg_type, IICGC_VVAL, &dbv2)) 
			!= sizeof(i4))
	    	return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
	    copy_map->cp_nullen = (i4)i2_val;
	    copy_map->cp_nultype = copy_map->cp_rowtype;
	    copy_map->cp_nulprec = 0;
	}

	/* Read the WITH NULL value if it's there. */
	if (copy_map->cp_nullen == 0)
	{
	    copy_map->cp_nuldata = NULL;
	}
	else
	{
	    dbv.db_prec = copy_map->cp_nulprec;

	    /* Check to see if this is a BLOB column */
	    col_desc = &row_desc->RD_DBVS_MACRO(copy_map->cp_attrmap);

	    if (IIDT_LONGTYPE_MACRO(copy_map->cp_nultype) ||
		IIDT_UDT_MACRO(copy_map->cp_nultype))  /* BLOB column */
	    {
		ADP_PERIPHERAL	*nulladpbuf;
		i2		tmpnullen;

		/*
		** Read the ADP_PERIPHERAL and get the cp_nullen and
		** cp_nuldata from there.
		*/
		if (loc_alloc(1, copy_map->cp_nullen, (PTR *) &nulladpbuf) != OK)
		    return (E_CO0037_MEM_INIT_ERR);

		II_DB_SCAL_MACRO(dbv, copy_map->cp_nultype, copy_map->cp_nullen,
		    (PTR) nulladpbuf);

		if ((num_read = IIcgc_get_value(cgc,msg_type,IICGC_VVAL,&dbv))
			!= copy_map->cp_nullen)
		    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);

		/* "WITH NULL ('')" sends no value! be careful... */
		if (nulladpbuf->per_length1 == 0)
		    tmpnullen = 0;
		else
		    MEcopy((PTR)nulladpbuf + ADP_HDR_SIZE + sizeof(i4),
		       sizeof(i2), (PTR) &tmpnullen);
		copy_map->cp_nullen = (i4) tmpnullen;
 
		if (tmpnullen == 0)
		    copy_map->cp_nuldata = NULL;
		else
		{
		    if (loc_alloc(1, copy_map->cp_nullen,
				&copy_map->cp_nuldata) != OK)
			return (E_CO0037_MEM_INIT_ERR);

		    if (abs(col_desc->db_datatype) == DB_LNVCHR_TYPE)
		    {
			DB_DATA_VALUE	ucs2_dv, rdv;
			ALIGN_RESTRICT	utf8_buf[(DB_MAXSTRING+DB_CNTSIZE+sizeof(ALIGN_RESTRICT)-1)/sizeof(ALIGN_RESTRICT)];
			ucs2_dv.db_datatype = DB_NCHR_TYPE;
			ucs2_dv.db_length = copy_map->cp_nullen * sizeof(UCS2);
			ucs2_dv.db_data = (PTR) nulladpbuf + ADP_HDR_SIZE + sizeof(i4) + sizeof(i2);
			rdv.db_datatype = DB_UTF8_TYPE;
			rdv.db_length = copy_map->cp_nullen + sizeof(i2);
			rdv.db_data = (PTR) utf8_buf;
			if (adu_nvchr_toutf8(IIlbqcb->ii_lq_adf, &ucs2_dv, &rdv) != E_DB_OK)
			    return(E_CO0022_COPY_INIT_ERR);
			MEcopy((char *)utf8_buf+DB_CNTSIZE, copy_map->cp_nullen,
			    copy_map->cp_nuldata);
		    }
		    else
		    {
			MEcopy((PTR)nulladpbuf + ADP_HDR_SIZE + sizeof(i4) + sizeof(i2),
			    copy_map->cp_nullen, copy_map->cp_nuldata);
		    }
		}
		MEfree(nulladpbuf);
	    }
	    else
	    {
		i4 absntype = abs(copy_map->cp_nultype);

		/* Not a BLOB column.  Read the data, and post-process
		** it to reset nullen to the actual data length, and to
		** clip off any DB_TEXT_STRING type overhead.
		** Leave NCHAR or NVCHR null-data alone, however.
		** Those two should correspond strictly to nchar(n) or
		** nvarchar(n) row-types, which are emitted basically as-is.
		*/
		if (loc_alloc(1, copy_map->cp_nullen, 
			&copy_map->cp_nuldata) != OK)
		    return (E_CO0037_MEM_INIT_ERR);
 
		/* value for null */
		II_DB_SCAL_MACRO(dbv, copy_map->cp_nultype, copy_map->cp_nullen,
		    copy_map->cp_nuldata);
 
		if ((num_read = IIcgc_get_value(cgc,msg_type,IICGC_VVAL,&dbv))
			!= copy_map->cp_nullen)
		    return ((num_read < 0) ? FAIL : E_CO0022_COPY_INIT_ERR);
		if (absntype == DB_VCH_TYPE || absntype == DB_TXT_TYPE
		  || absntype == DB_UTF8_TYPE || absntype == DB_LTXT_TYPE
		  || absntype == DB_VBYTE_TYPE)
		{
		    copy_map->cp_nullen = ((DB_TEXT_STRING *)(copy_map->cp_nuldata))->db_t_count;
		    copy_map->cp_nuldata += DB_CNTSIZE;
		}
	    }
	}
	
    }
 
    /*
    ** Fill in more copy map stuff, mostly based on type information:
    **   - decide whether this is binary or formatted copy
    **   - fill in the ADF function blocks for the row/tuple conversions.
    **   - fill in the tuple information from the tuple descriptor read earlier.
    **   - look for TEXT, DUMMY, and variable length fields for special 
    **	    processing.
    */
    for (i = 0; i < copy_blk->cp_num_domains; i++)
    {
	i4	absrtype, len;

	copy_map = &copy_blk->cp_copy_map[i];
 
	/*
	** Find the attribute descriptor and fill in the tuple information in
	** the copy map.  If this is a dummy domain, then there is no attibute
	** descriptor.
	*/
	absrtype = copy_map->cp_rowtype;
	if (absrtype != CPY_DUMMY_TYPE)
	{
	    /* Find the column descriptor corresponding to this domain.  */
	    absrtype = abs(copy_map->cp_rowtype);
	    col_desc = &row_desc->RD_DBVS_MACRO(copy_map->cp_attrmap);
 
	    copy_map->cp_tuptype = col_desc->db_datatype;
	    copy_map->cp_tuplen = col_desc->db_length;
	    copy_map->cp_tupprec = col_desc->db_prec;
	    /* groupoffset comes later... */
	    /* Set a convenience indicator for row types preceded by a
	    ** 5-byte length count.
	    */
	    copy_map->cp_counted = (absrtype == DB_VCH_TYPE
					|| absrtype == DB_UTF8_TYPE
					|| absrtype == DB_VBYTE_TYPE);
	}
	else
	{
	    copy_map->cp_tuptype = 0;
	    copy_map->cp_cvid = ADI_NOFI;
	}
	/* Delimiters are one character, but may be one or two bytes.
	** (Can't be more, CPY_MAX_DELIM is two.)
	*/
	copy_map->cp_delim_len = 0;
	if (copy_map->cp_isdelim)
	    copy_map->cp_delim_len = CMbytecnt(copy_map->cp_delim);

	absttype = abs(copy_map->cp_tuptype);
	if (IIDT_LONGTYPE_MACRO(absttype))
	    copy_map->cp_tuplob = TRUE;
 
	/* For char-type columns, set the effective length of the table
	** column, to detect and count string truncation for COPY FROM.
	*/
	if (copy_blk->cp_direction == CPY_FROM)
	{
	    switch(absttype)
	    {
		case DB_BYTE_TYPE:
		case DB_CHR_TYPE:
		case DB_CHA_TYPE:
		     len = copy_map->cp_tuplen;
		     break;
		case DB_TXT_TYPE:	
		case DB_VCH_TYPE:
		case DB_VBYTE_TYPE:
		     len = copy_map->cp_tuplen - DB_CNTSIZE;
		     break;
		default:
		     len = 0;
		     break;
	    }
	    if (copy_map->cp_tuptype < 0 && len > 0)
		--len;
	    copy_map->cp_trunc_len = len;
	}

	if (copy_map->cp_rowlen == ADE_LEN_UNKNOWN)
	{
	    if (copy_blk->cp_direction == CPY_FROM && 
		!copy_map->cp_isdelim &&
		!copy_map->cp_counted &&
		!copy_map->cp_tuplob)
	    {
		/*
		** If copy FROM with type other than VARCHAR, and no
		** delimiter was specified, then mark this to use default delim.
		** (VARCHAR(0) type does not need a delimiter).
		** Zero out the delimiter storage since we will check the value
		** stored there to see if it is double-byte (default delims are
		** single-byte.
		*/
                copy_map->cp_isdelim = -1;
                copy_map->cp_delim[0] = '\0';
		copy_map->cp_delim_len = 1;
	    }
	    copy_blk->cp_status |= CPY_NOTBIN;
	    if (copy_map->cp_isdelim > 0
	      && (copy_map->cp_delim[0] == CPY_CSV_DELIM
		  || copy_map->cp_delim[0] == CPY_SSV_DELIM) )
	    {
		/* It's CSV or SSV format.  Turn on the CSV flag and set
		** the delimiter to comma, or semicolon, or newline if
		** we're on the very last column.
		*/
		copy_map->cp_csv = TRUE;
		if (copy_map->cp_delim[0] == CPY_SSV_DELIM)
		    copy_map->cp_ssv = TRUE;
		if (i == copy_blk->cp_num_domains-1)
		    copy_map->cp_delim[0] = '\n';
		else if (copy_map->cp_delim[0] == CPY_CSV_DELIM)
		    copy_map->cp_delim[0] = ',';
		else
		    copy_map->cp_delim[0] = ';';
		copy_map->cp_delim_len = 1;
	    }
	}
	/*
	** Fill in ADF function block for conversion calls.
	** Recall that the copy-map entry is pre-zeroed, so things like
	** escape, pat-flags, etc are already zero.
	*/
	copy_map->cp_cvfunc = NULL;
	copy_map->cp_adf_fnblk.adf_fi_id = copy_map->cp_cvid;
	copy_map->cp_adf_fnblk.adf_fi_desc = NULL;
	copy_map->cp_adf_fnblk.adf_dv_n = 1;
	if (copy_map->cp_cvid == ADI_NOFI)
	{
	    /* Double check that the copy is binary, not converting */
	    /* FIXME supply reason, also other internal errors */
	    if (copy_blk->cp_convert && absrtype != CPY_DUMMY_TYPE)
		return (E_CO0022_COPY_INIT_ERR);
	}
	else
	{
	    /* Conversion FI given, must not be binary copy */
	    if (!copy_blk->cp_convert)
		return (E_CO0022_COPY_INIT_ERR);
	    /* Set up conversion FI's.  Arrange for the value and result
	    ** types / lengths to be the non-nullable flavor, if either
	    ** is nullable;  we're going to deal with nulls by hand.
	    */
	    if (copy_blk->cp_direction == CPY_FROM)
	    {
		copy_map->cp_adf_fnblk.adf_1_dv.db_datatype = absrtype;
		copy_map->cp_adf_fnblk.adf_1_dv.db_length = copy_map->cp_rowlen;
		if (copy_map->cp_rowtype < 0)
		    --copy_map->cp_adf_fnblk.adf_1_dv.db_length;
		copy_map->cp_adf_fnblk.adf_1_dv.db_prec = copy_map->cp_rowprec;
		copy_map->cp_adf_fnblk.adf_r_dv.db_datatype = absttype;
		copy_map->cp_adf_fnblk.adf_r_dv.db_length = copy_map->cp_tuplen;
		if (copy_map->cp_tuptype < 0)
		    -- copy_map->cp_adf_fnblk.adf_r_dv.db_length;
		copy_map->cp_adf_fnblk.adf_r_dv.db_prec = copy_map->cp_tupprec;
	    }
	    else /* INTO */
	    {
		copy_map->cp_adf_fnblk.adf_1_dv.db_datatype = absttype;
		copy_map->cp_adf_fnblk.adf_1_dv.db_length = copy_map->cp_tuplen;
		if (copy_map->cp_tuptype < 0)
		    -- copy_map->cp_adf_fnblk.adf_1_dv.db_length;
		copy_map->cp_adf_fnblk.adf_1_dv.db_prec = copy_map->cp_tupprec;
		copy_map->cp_adf_fnblk.adf_r_dv.db_datatype = absrtype;
		copy_map->cp_adf_fnblk.adf_r_dv.db_length = copy_map->cp_cvlen;
		if (copy_map->cp_rowtype < 0)
		    --copy_map->cp_adf_fnblk.adf_r_dv.db_length;
		copy_map->cp_adf_fnblk.adf_r_dv.db_prec = copy_map->cp_cvprec;
	    }
	    /* Fetch the FI description and the FI function, so that
	    ** the converter loops can use the conversion function directly.
	    ** This cuts out a bunch of preventive checking that adf_func
	    ** does, which is (should be!) unnecessary because we trust the
	    ** backend to set up all the DBDV stuff properly!
	    ** If there's no adu function available, fall back to the
	    ** generic adf_func call.
	    */
	    dbstat = adi_fidesc(IIlbqcb->ii_lq_adf, copy_map->cp_cvid,
			&copy_map->cp_adf_fnblk.adf_fi_desc);
	    if (dbstat == E_DB_OK && copy_map->cp_adf_fnblk.adf_fi_desc != NULL)
		dbstat = adi_function(IIlbqcb->ii_lq_adf,
			copy_map->cp_adf_fnblk.adf_fi_desc,
			&copy_map->cp_cvfunc);
	    if (dbstat != E_DB_OK)
		copy_map->cp_cvfunc = NULL;
	} /* if not NOFI */

	/*
	** For counted and TEXT types, the server sets rowlen thinking
	** that the data is DB_TEXT_STRING, which is not the case.  Adjust
	** the row length for those types.
	** If the row type is "counted", then we will write length specifier in
	** ascii. Adjust the actual fieldlen (in the copy file) to show the 
	** ascii length value that preceeds the varchar text data.
	**
	** If the row type is text, then the count field will be stripped
	** off.  The field length is smaller than the rowlen by the size
	** of the count field.
	*/
	copy_map->cp_fieldlen = copy_map->cp_rowlen;
	if (copy_map->cp_rowlen != ADE_LEN_UNKNOWN && copy_blk->cp_convert)
	{
	    if (copy_map->cp_counted)
		copy_map->cp_fieldlen += (CPY_VCHLEN - DB_CNTSIZE);
	    if ( absrtype == DB_TXT_TYPE )
		copy_map->cp_fieldlen -= DB_CNTSIZE;
	}

    } /* for */
 
    /* Allocate a temporary array of offsets.
    ** cp_buffer is used so that we can clean it up if error in init.
    */
    if (loc_alloc(row_desc->rd_numcols, sizeof(i4), &copy_blk->cp_buffer) != OK)
    {
	return (E_CO0037_MEM_INIT_ERR);
    }
    attr_offset = (i4 *) copy_blk->cp_buffer;

    /* Figure out the column group structure */
    non_lob_group = FALSE;
    offset = 0;
    /* As we're counting groups, assume that there might be a
    ** dummy group in between every LOB (and at the start and end).
    ** This will almost certainly over-estimate the number of
    ** groups if LOBs are around, but it makes the job ever so much
    ** easier -- and wasting a few bytes is no big deal here.
    */
    for (i=0; i<row_desc->rd_numcols; i++)
    {
	col_desc = &row_desc->RD_DBVS_MACRO(i);
	if (IIDT_LONGTYPE_MACRO(col_desc->db_datatype)
	  || IIDT_UDT_MACRO(col_desc->db_datatype))
	{
	    /* One group for the LOB, and assume we need a
	    ** dummy before the LOB if we didn't have something already.
	    */
	    if (! non_lob_group)
		++ copy_blk->cp_num_groups;
	    ++ copy_blk->cp_num_groups;
	    non_lob_group = FALSE;
	    continue;	/* Do not include LOB in offsets! */
	}
	else if (! non_lob_group)
	{
	    ++ copy_blk->cp_num_groups;
	    non_lob_group = TRUE;
	}
	/* else non-lob part of group already counted */
	attr_offset[i] = offset;
	offset += col_desc->db_length;
    }
    /* and a trailing dummy if we ended with a LOB */
    if (! non_lob_group)
	++ copy_blk->cp_num_groups;

    /* Allocate and ZERO group array */
    if (loc_alloc_i(copy_blk->cp_num_groups, sizeof(II_CP_GROUP),
	    (PTR *) &copy_blk->cp_copy_group) != OK)
	return (E_CO0037_MEM_INIT_ERR);

    /* If there are multiple groups (ie at least one LOB), make sure
    ** that the cmaps are properly ordered.  cmaps aren't matched to
    ** groups yet, but it's easy enough to ensure that copy domains
    ** aren't reordered across a LOB in either direction.
    ** For each LOB copy domain that is table attribute #N, all preceding
    ** (non-dummy) copy domains must be table attribute <N,
    ** and all following copy domains must be table attribute >N.
    ** (Only check if user specified domain list, no need to check if
    ** binary copy.)
    **
    ** As a side note, the rule could in theory be relaxed for COPY FROM
    ** to only check in the forward direction.  If the code kept the
    ** entire non-lob tuple image in memory, a non-LOB appearing "early"
    ** could set its value and let it sit there until that group is
    ** processed and sent.  This not a very big deal IMHO and doing
    ** everything strictly group by group makes life easier.
    */
    if (copy_blk->cp_num_groups > 1 && copy_blk->cp_convert)
    {
	II_CP_MAP *cmap, *cmap1;
	i4 attno;

	last_cmap = &copy_blk->cp_copy_map[copy_blk->cp_num_domains-1];
	for (cmap = &copy_blk->cp_copy_map[0];
	     cmap <= last_cmap;
	     ++cmap)
	{
	    if (cmap->cp_tuplob)
	    {
		/* Check rule forward */
		attno = cmap->cp_attrmap;
		cmap1 = cmap + 1;
		while (cmap1 <= last_cmap)
		{
		    if (cmap1->cp_rowtype != CPY_DUMMY_TYPE
		      && cmap1->cp_attrmap < attno)
		    {
			IIlocerr(GE_LOGICAL_ERROR, E_CO0044_FMT_BLOB_ORDER, II_ERR, 
				    2, cmap->cp_name, cmap1->cp_name);
			return (FAIL);
		    }
		    ++cmap1;
		}
		/* Check rule backwards.  Note that if we hit another LOB,
		** stop, we already checked the cmaps before that lob.
		*/
		cmap1 = cmap - 1;
		while (cmap1 >= &copy_blk->cp_copy_map[0])
		{
		    if (cmap1->cp_tuplob)
			break;
		    if (cmap1->cp_rowtype != CPY_DUMMY_TYPE
		      && cmap1->cp_attrmap > attno)
		    {
			IIlocerr(GE_LOGICAL_ERROR, E_CO0044_FMT_BLOB_ORDER, II_ERR, 
				    2, cmap1->cp_name, cmap->cp_name);
			return (FAIL);
		    }
		    --cmap1;
		}
	    } /* if LOB */
	}
    } /* if lobs and convert */

    /* Go through the groups and cmaps together.  Assign cmaps and
    ** attribute ranges to groups, figure out offsets, etc.
    ** Doing this is slightly tricky because we can have groups
    ** that don't correspond to any cmaps, and dummy cmaps that have
    ** to be assigned to groups.  Also, non-LOB cmaps in between
    ** LOB positions don't have to be in attribute order...
    */
    cmap = &copy_blk->cp_copy_map[0];
    last_cmap = &copy_blk->cp_copy_map[copy_blk->cp_num_domains-1];
    cgroup = &copy_blk->cp_copy_group[-1];	/* One before first entry */
    non_lob_group = FALSE;
    for (i=0; i<row_desc->rd_numcols; i++)
    {
	/* Note: at the top of the loop, if !non_lob_group, cgroup is
	** pointing to the last LOB group (or to start-1).  If non_lob_group,
	** cgroup points to a non_LOB group being filled in.
	*/
	col_desc = &row_desc->RD_DBVS_MACRO(i);
	absttype = abs(col_desc->db_datatype);
	if (IIDT_LONGTYPE_MACRO(absttype)
	  || IIDT_UDT_MACRO(absttype))
	{
	    /* If the current cmap is dummy or a non-lob that precedes
	    ** this LOB, attach cmaps to the current non-lob group until
	    ** we run off the end, or hit a cmap for an attribute >= the LOB.
	    */
	    if (cmap != NULL
	      && (cmap->cp_rowtype == CPY_DUMMY_TYPE
		  || cmap->cp_attrmap < i))
	    {
		if (! non_lob_group)
		{
		    ++cgroup;
		    non_lob_group = TRUE;
		}
		if (cgroup->cpg_first_map == NULL)
		    cgroup->cpg_first_map = cmap;
		do
		{
		    cgroup->cpg_last_map = cmap;
		    ++cmap;
		} while (cmap <= last_cmap
		  && (cmap->cp_rowtype == CPY_DUMMY_TYPE || cmap->cp_attrmap < i));
		if (cmap > last_cmap)
		    cmap = NULL;
	    }
	    ++cgroup;
	    non_lob_group = FALSE;
	    cgroup->cpg_first_attno = cgroup->cpg_last_attno = i;
	    cgroup->cpg_is_tup_lob = TRUE;
	    cgroup->cpg_is_row_lob = TRUE;
	    copy_blk->cp_has_blobs = TRUE;
	    /* Leave lengths/offsets zero */
	    /* If the current cmap is this LOB, hook group and cmap. */
	    if (cmap != NULL && cmap->cp_attrmap == i)
	    {
		cgroup->cpg_first_map = cgroup->cpg_last_map = cmap;
		if (! IIDT_LONGTYPE_MACRO(cmap->cp_rowtype))
		    cgroup->cpg_is_row_lob = FALSE;
		++ cmap;
		if (cmap > last_cmap)
		    cmap = NULL;
	    }
	}
	else
	{
	    if (!non_lob_group)
	    {
		++cgroup;
		non_lob_group = TRUE;
	    }
	    if (cgroup->cpg_tup_length == 0)
	    {
		cgroup->cpg_first_attno = i;
		cgroup->cpg_tup_offset = attr_offset[i];
	    }
	    /* For COPY INTO, note whether any column in this group might be
	    ** GCA-compressed, which complicates IIcpdbread.
	    */
	    if (copy_blk->cp_direction == CPY_INTO
	      && row_desc->rd_flags & RD_CVCH
	      && (absttype == DB_LTXT_TYPE
		  || absttype == DB_NVCHR_TYPE
		  || absttype == DB_TXT_TYPE
		  || absttype == DB_UTF8_TYPE
		  || absttype == DB_VBYTE_TYPE
		  || absttype == DB_VCH_TYPE) )
		cgroup->cpg_compressed = TRUE;

	    cgroup->cpg_last_attno = i;
	    cgroup->cpg_tup_length += col_desc->db_length;
	    /* Don't worry about non-lob columns vs cmaps, we'll figure it
	    ** all out at the next LOB, or at the end.
	    */
	}
    }
    /* If we have cmaps hanging around, assign them to the last group */
    if (cmap != NULL)
    {
	if (! non_lob_group)
	    ++cgroup;
	if (cgroup->cpg_first_map == NULL)
	    cgroup->cpg_first_map = cmap;
	cgroup->cpg_last_map = last_cmap;
    }
    /* Set actual number of cgroups needed */
    copy_blk->cp_num_groups = cgroup - copy_blk->cp_copy_group + 1;

    /* Do a quickie run thru the groups and set the "refill control"
    ** to EOF FAILS for all except the group with the first domain.
    ** Ie we're only allowed a real EOF at the start of a new row.
    */
    if (copy_blk->cp_direction == CPY_FROM)
    {
	i4 rf = COPY_REFILL_NORMAL;

	for (cgroup = &copy_blk->cp_copy_group[0];
	     cgroup <= &copy_blk->cp_copy_group[copy_blk->cp_num_groups-1];
	     ++cgroup)
	{
	    cgroup->cpg_refill_control = rf;
	    if (cgroup->cpg_first_map != NULL)
		rf = COPY_REFILL_EOF_FAILS;
	}
    }

    /*
    ** One more column map loop, this time accumulating lengths.
    */
    all_chr_type = TRUE;
    any_varrows = FALSE;
    max_rowlen = 0;
    cgroup = &copy_blk->cp_copy_group[0];
    for (i = 0; i < copy_blk->cp_num_domains; i++)
    {
	i4	absrtype, len;

	copy_map = &copy_blk->cp_copy_map[i];
	/* If this domain is in a later-on group, move to the right one */
	while (cgroup->cpg_first_map == NULL || copy_map > cgroup->cpg_last_map)
	    ++cgroup;


	absttype = abs(copy_map->cp_tuptype);

	/* Set tuple value offset in group (meaningless for lob) */
	absrtype = copy_map->cp_rowtype;
	if (absrtype != CPY_DUMMY_TYPE)
	{
	    absrtype = abs(absrtype);
	    copy_map->cp_groupoffset = attr_offset[copy_map->cp_attrmap] - cgroup->cpg_tup_offset;
	}

	/*
	** Accumulate the copy row length.
	**
	** Normally, we just need to add the rowlen for this domain to the
	** group row length.  There are three exceptions.
	**
	** If this is a dummy domain for a COPY INTO, then the domain name
	** will be used as the value for the copy domain.  It will be
	** written as many times as the domain length specifies.  So add
	** domain length * name length to the copy row length.
	**
	** If this is a variable length field in a COPY FROM, then we
	** pick the maximum size.  If it's in a COPY INTO then we use ADF
	** conversion length.
	**
	** If this is a variable length field in a COPY INTO and the null
	** value specified by the WITH NULL(value) option is longer than the
	** column length.  Add the null string length to the copy row length.
	*/
	if (copy_map->cp_rowlen != ADE_LEN_UNKNOWN)
	{
	    if (absrtype == CPY_DUMMY_TYPE)
	    {
		if (copy_blk->cp_direction == CPY_INTO)
		{
		    /* 
		    ** Bug #6380 - cp_name will be an empty string
		    ** for null dummy columns
		    */
		    cp_namelen = STlength(copy_map->cp_name); 
		    if (cp_namelen == 0)
			cp_namelen = 1;
		    len = copy_map->cp_rowlen * cp_namelen;
		}
		else
		    len = copy_map->cp_rowlen;
	    }
	    else
		len = copy_map->cp_fieldlen;
	    cgroup->cpg_row_length += len;
	}
	else
	{
	    /* Long varchar will come through here too */

	    cgroup->cpg_varrows = TRUE;
	    any_varrows = TRUE;
	    if (copy_blk->cp_direction == CPY_INTO)
	    {
		/*
		** If row type is varchar, copy outputs an ascii length
		** specifier (5 bytes).  However, the length of the 
		** conversion result (cp_cvlen) as sent by the server includes 
		** the length of a binary length specifier (2 bytes).  Need
		** to adjust row length so it will be big enough to hold the
		** ascii length specifier. Bug 35154.
		**
		** If the null string length is longer than the conversion
		** length, use the null length to calculate row length.
		** Don't forget the 5-character ascii length for counted
		** types.
		*/
	        if (copy_map->cp_counted)
		{
		    if (copy_map->cp_nullen > copy_map->cp_cvlen)
			len = copy_map->cp_nullen + CPY_VCHLEN;
		    else
			len = copy_map->cp_cvlen + CPY_VCHLEN;
		}
		else
		{
		    if (copy_map->cp_nullen > copy_map->cp_cvlen)
			len = copy_map->cp_nullen;
		    else
			len = copy_map->cp_cvlen;
		}
		cgroup->cpg_row_length += len;
	    }
	}
	/*
	** If a delimiter is to be written to the copy file, add it to the
	** row length.
	*/
	cgroup->cpg_row_length += copy_map->cp_delim_len;
	if (cgroup->cpg_row_length > max_rowlen)
	    max_rowlen = cgroup->cpg_row_length;

	/*
	** If any binary datatypes are specified for the copy file, turn off
	** 'all_chr_type' so that we won't use a text file for the copy.
	** UTF8 (nchar/nvarchar) domains are considered binary on Windows
	** since a control-Z is possible, which sets EOF in text mode.
	** Same goes for VBYTE.
	*/
	if ((absrtype != DB_CHR_TYPE) &&
	    (absrtype != DB_CHA_TYPE) &&
	    (absrtype != DB_VCH_TYPE) &&
	    (absrtype != DB_TXT_TYPE) &&
#ifndef NT_GENERIC
	    (absrtype != DB_UTF8_TYPE) &&
	    (absrtype != DB_VBYTE_TYPE) &&
#endif
	    (absrtype != CPY_DUMMY_TYPE))
	{
	    chr_row_type = FALSE;
	    all_chr_type = FALSE;
	}
	else
	{
	    chr_row_type = TRUE;
	}
 
	/*
	** If this is a COPY FROM, then we may have to check the validity
	** of the data being read from the COPY file (to make sure it specifies
	** a legal Ingres data value).
	**
	** If we'll call a conversion FI, don't check, because the converter
	** will presumably do it.  The exception is a date-to-itself
	** "conversion", which does a convert to internal, then convert back
	** to the date format, and that may or may not verify validity.
	**
	** If there is no conversion FI (and it's not dummy), then it's
	** binary copy().  We don't need to check ints, floats, char/nchar/byte
	** as all values are legal.  Check anything else.
	*/
	if (copy_blk->cp_direction == CPY_INTO)
	    copy_map->cp_valchk = 0;
	else
	{
	    copy_map->cp_valchk = 1;
	    if (copy_map->cp_cvid == ADI_NOFI
	      && (absrtype == CPY_DUMMY_TYPE
		    || copy_map->cp_tuplob
		    || absttype == DB_INT_TYPE
		    || absttype == DB_FLT_TYPE
		    || absttype == DB_CHA_TYPE
		    || absttype == DB_NCHR_TYPE
		    || absttype == DB_BYTE_TYPE
		    || absttype == DB_LOGKEY_TYPE
		    || absttype == DB_TABKEY_TYPE))
		copy_map->cp_valchk = 0;
	    if (copy_map->cp_cvid != ADI_NOFI
	      && (absrtype != absttype
		  || (absttype != DB_DTE_TYPE
		    && absttype != DB_ADTE_TYPE
		    && absttype != DB_TMWO_TYPE
		    && absttype != DB_TME_TYPE
		    && absttype != DB_TMW_TYPE
		    && absttype != DB_TSWO_TYPE
		    && absttype != DB_TSW_TYPE
		    && absttype != DB_TSTMP_TYPE
		    && absttype != DB_INYM_TYPE
		    && absttype != DB_INDS_TYPE)))
		copy_map->cp_valchk = 0;
	    /* Note that the above test turns off valchk for binary
	    ** nchar, nvarchar.  Those are not valchk'ed, rather they are
	    ** unormed.
	    */
	    if (copy_map->cp_valchk
	      || (copy_map->cp_cvid == ADI_NOFI
		  && (absttype == DB_NCHR_TYPE || absttype == DB_NVCHR_TYPE)))
		cgroup->cpg_validate = TRUE;
	}
    }

    /* Accumulate total length, mostly for VMS fixed-width copy so that
    ** it has a valid record length.  This only has to be right for
    ** fixed-length rows; text or var rows won't use row-length.
    */
    for (cgroup = &copy_blk->cp_copy_group[0];
	 cgroup <= &copy_blk->cp_copy_group[copy_blk->cp_num_groups-1];
	 ++cgroup)
    {
	if (cgroup->cpg_row_length > 0)
	    copy_blk->cp_row_length += cgroup->cpg_row_length;
    }

    /*
    ** Figure out file type.
    */
    if ((copy_blk->cp_status & CPY_PROGRAM) == 0)
    {
	copy_blk->cp_filetype = -1;	/* Not chosen yet */
#ifdef VMS 
	/*
	** VMS only: Check if filename includes the file type. 
	** Look for comma in filename.  "fname_length" is set above.
	*/
	cp = STindex(copy_blk->cp_filename, ",", fname_length);
	if (cp) 
	{
	    if ((cp2 = STindex(copy_blk->cp_filename, "]", fname_length)) ||
		(cp2 = STindex(copy_blk->cp_filename, ">", fname_length)))
	    {
		cp = STindex(cp, ",", STlength(cp2));
	    }
	}
	if (cp) 
	{
	    /*
	    ** Found a comma, now look for file type following it.  
	    ** Note: file types are only applicable for VMS.
	    */
	    *cp = '\0';
	    CMnext(cp);
	    while (CMspace(cp))
		CMnext(cp);
	    switch (*cp)
	    {
	      case 'B':
	      case 'b':
		copy_blk->cp_filetype = SI_BIN;
		break; 
	      case 'T':
	      case 't':
		copy_blk->cp_filetype = SI_TXT;
		/*
		** TEXT filetype is not allowed with
		** bulk copy.  A TEXT file written out using
		** bulk copy can't be copied back in because
		** the bulk copy won't handle the carriage
		** return after each record - give an error
		** for COPY INTO to catch this
		** situation up front. (Bug 6936) 
		*/
		if ((!copy_blk->cp_convert) &&
		    (copy_blk->cp_direction == CPY_INTO))
		{
		    return (E_CO0017_NOTEXT);
		}
		break; 
	      case 'V':
	      case 'v':
		copy_blk->cp_filetype = SI_VAR;
		break; 
	      default:
		IIlocerr(GE_SYNTAX_ERROR, E_CO0018_BAD_FILETYPE, II_ERR, 1, cp);
		copy_blk->cp_warnings++;
		return (FAIL);
	    }
 
	    /*
	    ** If we got a SI_BIN type and they are not allowed, then it is an
	    ** error.
	    */
	    if ((copy_blk->cp_filetype == SI_BIN) &&
		(copy_blk->cp_status & CPY_NOTBIN) &&
		(copy_blk->cp_direction == CPY_INTO))
	    {
		return (E_CO0019_NOBINARY);
	    }
	}
#elif NT_GENERIC
	/* Windows:  allow B or T as filetypes.  Note that unlike VMS,
	** which has historically allowed ,binary etc (or even ,booger
	** since it only checks the first letter), for Windows I'll
	** require a single letter at the end of the filename.
	** No spaces between comma and filetype.  E.g.
	** 'foo.dat,T' is correct.  'foo.dat, t' is not and will cause
	** 'foo.dat, t' to be used as the filename.
	** "fname_length" is set up above, it INCLUDES the trailing null.
	*/
	--fname_length;		/* Don't include null */
	cp = &copy_blk->cp_filename[fname_length-1];
	if (fname_length > 2 && *(cp-1) == ',')
	{
	    if (*cp == 't' || *cp == 'T')
	    {
		/* Text mode requested */

		*(cp-1) = '\0';
		copy_blk->cp_filetype = SI_TXT;
	    }
	    else if (*cp == 'b' || *cp == 'B')
	    {
		/* Binary mode requested */
		/* Unlike VMS, Windows has no restriction on using binary;
		** it's really a translation mode, not a true file type.
		*/

		*(cp-1) = '\0';
		copy_blk->cp_filetype = SI_BIN;
	    }
	}
#endif /* VMS or Windows */
	if (copy_blk->cp_filetype == -1)
	{
	    /*
	    ** Use default filetypes.
	    **   If all domains are character (c, char, text, vchar) and the
	    **   rows end in newlines, then use TEXT file. 
	    **   ("character" does not include LOB.)
	    **   15-feb-96 (also if rows end with default delimiters)
	    **   Otherwise use SI_BIN unless this is a variable length file.
	    */
	    i = copy_blk->cp_num_domains - 1;
	    if ((all_chr_type) &&
		(
		 (copy_blk->cp_copy_map[i].cp_isdelim &&
		  copy_blk->cp_copy_map[i].cp_delim[0] == '\n')
		 ||
		 (copy_blk->cp_copy_map[i].cp_rowtype == CPY_DUMMY_TYPE &&
		  CMcmpcase(copy_blk->cp_copy_map[i].cp_name, "\n") == 0)
		 ||
		 (copy_blk->cp_copy_map[i].cp_isdelim == -1)
		)
	       )	
	    {
		copy_blk->cp_filetype = SI_TXT;
	    }
	    else if (copy_blk->cp_status & CPY_NOTBIN)
	    {
		copy_blk->cp_filetype = SI_VAR;
	    }
	    else
	    {
		copy_blk->cp_filetype = SI_BIN;
	    }
	}
    }
 
    /*
    ** cp_continue specifies to not halt copy processing if an error occurs
    ** converting a row.
    */
    copy_blk->cp_continue = (copy_blk->cp_status & CPY_CONTINUE) != 0;
 
    /*
    ** cp_program specifies that there is no copy file, we should use the
    ** user defined routines to read and write copy rows.
    */
    copy_blk->cp_program = (copy_blk->cp_status & CPY_PROGRAM) != 0;

    /*
    ** cp_rowlog specifies that bad rows or tuples (ones that cannot be
    ** converted to the specified format) should be written to a log file.
    ** Don't however log if there are LOB's, too hard.
    */
    copy_blk->cp_rowlog = (copy_blk->cp_logname != NULL);
    if (copy_blk->cp_has_blobs )
	copy_blk->cp_rowlog = FALSE;
    if (copy_blk->cp_rowlog && copy_blk->cp_direction == CPY_FROM)
    {
	/* COPY FROM has to keep a separate row image for logging, since
	** it does not normally operate row-wise.  What a nuisance!
	** Fortunately logging is not all that often used, at least
	** not when speed is important...
	*/
	copy_blk->cp_log_size = DB_MAXSTRING;
	if (loc_alloc(1, copy_blk->cp_log_size, &copy_blk->cp_log_buf) != OK)
	{
	    return (E_CO0037_MEM_INIT_ERR);
	}
    }

    copy_blk->cp_tupbuf = NULL;
    if (copy_blk->cp_tup_length > 0
      && loc_alloc(1, copy_blk->cp_tup_length, &copy_blk->cp_tupbuf) != OK)
    {
	return (E_CO0037_MEM_INIT_ERR);
    }
 
    /*
    ** Build an "empty" tuple for use when converting from row format
    ** to tuple format.  Attributes with no corresponding domain in
    ** the copy file will be left with this default value.
    **
    ** 55199 - check cp_tup_length too, if no non-blob columns, don't allocate
    */
    if ((copy_blk->cp_convert) && (copy_blk->cp_direction == CPY_FROM) &&
	(copy_blk->cp_tup_length > 0))
    {
	i4 att;
	PTR zval_ptr;
#ifdef	BYTE_ALIGN
	bool align;
	ALIGN_RESTRICT	align_buf[(DB_MAXSTRING+DB_CNTSIZE+sizeof(ALIGN_RESTRICT)) / sizeof(ALIGN_RESTRICT)];
#endif	/* BYTE_ALIGN */

	if (loc_alloc(1, copy_blk->cp_tup_length, &copy_blk->cp_zerotup))
	    return (E_CO0037_MEM_INIT_ERR);

	/* Run thru all the non-LOB attributes and generate "empty"
	** values at the proper (LOB-ignoring) offset within zerotup.
	** The from-formatted (non-LOB) routine can then simply init
	** the tuple fragment being filled in from the zerotup, starting
	** at the group offset.
	*/
	for (att = 0; att < row_desc->rd_numcols; ++att)
	{
	    absttype = abs(row_desc->RD_DBVS_MACRO(att).db_datatype);
	    if (IIDT_LONGTYPE_MACRO(absttype))
		continue;		/* Ignore LOB entirely */
	    zval_ptr = copy_blk->cp_zerotup + attr_offset[att];
	    row_desc->RD_DBVS_MACRO(att).db_data = zval_ptr;
#ifdef BYTE_ALIGN
	    align = FALSE;
	    if (ME_NOT_ALIGNED_MACRO(zval_ptr)
	      && absttype != DB_BYTE_TYPE
	      && absttype != DB_CHR_TYPE
	      && absttype != DB_CHA_TYPE)
	    {
		align = TRUE;
		row_desc->RD_DBVS_MACRO(att).db_data = (PTR) align_buf;
	    }
#endif
 
	    if (adc_getempty(IIlbqcb->ii_lq_adf,
		 &row_desc->RD_DBVS_MACRO(att)) != E_DB_OK)
		return (E_CO0022_COPY_INIT_ERR);
#ifdef BYTE_ALIGN
	    if ( align )
	    {
		MEcopy((PTR)align_buf,
			row_desc->RD_DBVS_MACRO(att).db_length, zval_ptr);
	    }
#endif /* BYTE_ALIGN */
	}
    }
 
    /*
    ** Free up memory used to find attribute offset mappings
    */
    MEfree(copy_blk->cp_buffer);
    copy_blk->cp_buffer = NULL;
    attr_offset = NULL;
 
    /*
    ** Reset the count of chars converted to blank by ADF.  This is a
    ** Kludgy way of doing this, but we want to count only the
    ** warnings generated during COPY.
    */
    IIlbqcb->ii_lq_adf->adf_warncb.ad_noprint_cnt = 0;
 
    /*
    ** Initialize copy buffer header and GCA message buffer.
    */
    cpy_bufh->cpy_dbv.db_length = 0;
    cpy_bufh->cpy_bytesused = 0;
    cpy_bufh->end_of_data = FALSE;
    if (copy_blk->cp_direction == CPY_INTO)
    {

	/* If converting, allocate a row buffer.  (If not converting,
	** we can operate directly from the tuple buffer, and hope that
	** SIwrite has buffering behind it!)
	** Allocate according to the widest group seen.  (Even if variable
	** rows, the server is supposed to supply the max size of each
	** converted column.  Just for safety, though, if varrows we'll
	** add a bit of extra slop.)  If LOB's are present, allow enough
	** to read the largest segment, plus a bit extra.  The largest
	** segment is MAX_SEG_INTO_FILE characters which might be
	** MAX_SEG_INTO_FILE*2 octets.
	** (The widest row value is doubled to allow for the extreme
	** case of a CSV that is all double-quotes!)
	*/
	if (copy_blk->cp_convert || copy_blk->cp_has_blobs)
	{
	    copy_blk->cp_rbuf_size = 2*max_rowlen+10;
	    if (any_varrows)
		copy_blk->cp_rbuf_size += 200;
	    if (copy_blk->cp_has_blobs && max_rowlen < MAX_SEG_INTO_FILE*2+50)
		copy_blk->cp_rbuf_size = MAX_SEG_INTO_FILE*2 + 50;
	    if(loc_alloc(1, copy_blk->cp_rbuf_size, &copy_blk->cp_rowbuf_all))
		return (E_CO0037_MEM_INIT_ERR);
	    copy_blk->cp_rowbuf = copy_blk->cp_rowbuf_all;
	}
    }
    else
    {
	/* COPY FROM: allocate a buffer for reading from the input file.
	** If COPY FROM PROGRAM and no conversions and fixed width rows,
	** no row-buffer is needed;  we'll read directly into the tuple
	** area.
	** If the next input value to be processed isn't entirely in the
	** buffer, copy processing will slide the partial value to before
	** of the read-buffer proper, and read another cp_rbuf_readsize
	** chunk of data.
	** The size of this prefix work-space could be set to the largest
	** column value size, plus a bit, but there no rule that says
	** user values have to be clean;  they might be over-size, blank
	** padded, garbage, etc.  Simply leave room for "largest string"
	** plus a bit.  Fixed-length rows without conversion (ie binary
	** copy) are figured at (largest group)-1, since the row size is
	** unambiguous in that case.
	** The read-buffer part of the buffer can be READ_BUF_SIZE,
	** which is currently defined at 64K.
	** We allow at least an i8 before and after the workspace+buffer;
	** sometimes values are prefixed by DB_TEXT_STRING counts, or
	** nulls stuffed in at the end, and the guard space guarantees that
	** this will work.
	*/
	if (copy_blk->cp_program && !copy_blk->cp_convert && !any_varrows
	  && !copy_blk->cp_has_blobs)
	{
	    copy_blk->cp_rbuf_worksize = copy_blk->cp_rbuf_readsize = 0;
	    copy_blk->cp_rbuf_size = 0;
	    copy_blk->cp_rowbuf = copy_blk->cp_rowbuf_all = NULL;
	    copy_blk->cp_rowptr = NULL;
	    copy_blk->cp_dataend = NULL;
	}
	else
	{
	    i4 worklen, total;

	    if (copy_blk->cp_convert)
		worklen = DB_MAXSTRING + 200;	/* +200 is arbitrary slop */
	    else
		worklen = max_rowlen;
	    if (copy_blk->cp_has_blobs && worklen < MAX_SEG_FROM_FILE+20)
		worklen = MAX_SEG_FROM_FILE+20;	/* segment plus next count */
	    /* Force alignment to larger of 8 and DB_ALIGN_SZ, the easy way */
	    worklen = DB_ALIGNTO_MACRO(worklen,8);
	    worklen = DB_ALIGN_MACRO(worklen);
	    copy_blk->cp_rbuf_worksize = worklen;
	    copy_blk->cp_rbuf_readsize = READ_BUF_SIZE;
	    /* cp-refill expects to only have to do one read, so if the
	    ** worklen is larger than normal, make sure the read size is too.
	    ** This should only happen for binary (no-convert) copies into
	    ** extremely wide rows.
	    */
	    if (worklen > READ_BUF_SIZE)
		copy_blk->cp_rbuf_readsize = worklen;
	    copy_blk->cp_rbuf_size = worklen + copy_blk->cp_rbuf_readsize;;
	    /* include hidden leading and trailing guard space.  8 keeps
	    ** everything aligned even in worst case.
	    */
	    total = copy_blk->cp_rbuf_size + 8 + 8;
	    if(loc_alloc(1, total, &copy_blk->cp_rowbuf_all))
		return (E_CO0037_MEM_INIT_ERR);
	    copy_blk->cp_rowbuf = copy_blk->cp_rowbuf_all + 8;
	    copy_blk->cp_readbuf = copy_blk->cp_rowbuf + copy_blk->cp_rbuf_worksize;
	    copy_blk->cp_rowptr = (u_char *) copy_blk->cp_readbuf;
	    copy_blk->cp_dataend = copy_blk->cp_rowptr;
	    copy_blk->cp_at_eof = FALSE;
	    copy_blk->cp_refill_control = COPY_REFILL_NORMAL;
	}
    }
    return (OK);
}

/*
**  Name: IIcptdescr - Create GCA tuple descriptor
**
**  Description:
**	Allocate and build a GCA tuple descriptor from a row descriptor.
**
**  Inputs:
**	row_desc	- copy row descriptor
**
**  Outputs:
**
**  Returns: 
**	GCA_TD_DATA *	- GCA tuple descriptor
**
**  Side Effects:
**	Allocates memory for tuple descriptor.
**
**  History:
**	21-Dec-01 (gordy)
**	    Created.
**	30-Nov-2009 (kschendel) SIR 122974
**	    The tdescr being built is aligned, can use structure members
**	    for initialization instead of MEcopy (or even I4ASSIGN).
*/
 
static GCA_TD_DATA *
IIcptdescr( ROW_DESC *row_desc )
{
    GCA_COL_ATT		*coldesc;
    GCA_TD_DATA		*tdescr;
    char                *td_ptr;
    i4                  i;
    i4                  hdrsize = sizeof(GCA_TD_DATA) - sizeof(GCA_COL_ATT);
    i4                  attsize = sizeof(tdescr->gca_col_desc[0].gca_attdbv) + 
				  sizeof(tdescr->gca_col_desc[0].gca_l_attname);
    i4			bufsize = hdrsize + 
				  (row_desc->rd_gca->rd_gc_l_cols * attsize);

    /*
    ** Allocate buffer for tuple descriptor
    */
    if ((td_ptr = (char *)MEreqmem(0, bufsize, FALSE, (STATUS *)NULL)) == NULL)
	return( NULL );

    tdescr = (GCA_TD_DATA *)td_ptr;

    /* 
    ** Build tuple descriptor header.
    ** Note that since this GCA_TD_DESCR (aka GCA_TUP_DESCR) is aligned,
    ** we can use structure member names to initialize it.  (Unlike GCA
    ** elements in a GCA message, which may not necessarily be aligned.)
    ** Also, we're not sending any column names, keeping the att descrs
    ** at least i4-aligned as well.
    */
    tdescr->gca_tsize = row_desc->rd_gca->rd_gc_tsize;
    tdescr->gca_result_modifier = row_desc->rd_gca->rd_gc_res_mod;
    tdescr->gca_id_tdscr = row_desc->rd_gca->rd_gc_tid;
    tdescr->gca_l_col_desc = row_desc->rd_gca->rd_gc_l_cols;
    coldesc = (GCA_COL_ATT *) (td_ptr + hdrsize);

    /*
    ** Build column attributes
    */
    for( i = 0; i < row_desc->rd_gca->rd_gc_l_cols; i++ )
    {
	DB_DATA_VALUE	*dbv = &row_desc->rd_gca->rd_gc_col_desc[i].rd_dbv;

	coldesc->gca_attdbv.db_data = 0;
	coldesc->gca_attdbv.db_length = dbv->db_length;
	coldesc->gca_attdbv.db_datatype = dbv->db_datatype;
	coldesc->gca_attdbv.db_prec = dbv->db_prec;
	coldesc->gca_l_attname = 0;

	coldesc = (GCA_COL_ATT *) ((char *)coldesc + attsize);
    }

    return( tdescr );
}


/*{
+*  Name: IIcpdbread -	Read a tuple from the DBMS during COPY INTO.
**
** Description:
**
**	This routine reads from the database for COPY INTO.  The
**	copy-group to process might be an ordinary set of columns;
**	we ensure that all columns in the group have been read and
**	uncompressed.  (In the simplest case, we can simply point
**	at the data in the GCA buffer;  in more complex situations,
**	the tuple data is copied to a tuple buffer.)  Or, the copy
**	group might be one LOB;  LOB's require special handling and
**	we vanish off into the LOB routines.  Or, the copy group
**	might be nothing but dummy columns (fixed data to be put to
**	the output file), with no database involvement.
**
**	It may be necessary to read multiple GCA messages, either because
**	the tuple [group] is larger than one message, or because
**	the data got fragmented somewhere along the pipeline.  The
**	case involving multiple messages and decompression is particularly
**	tricky.  In that case, we'll glue together the compressed messages
**	into cp_decomp_buf until we have enough data to generate one
**	fully-decompressed tuple [group] in cp_tupbuf.  It ought to be
**	unusual to require more than one additional read to get the
**	full tuple, so the decompression is done the "dumb" way, by
**	starting over from the beginning each time.
**
**  Inputs:
**	copy_blk	- copy control block
**	cgroup		- copy group to process
**	tuple		- addr of tuple buffer pointer
**
**  Outputs:
**	*tuple		- set to the next tuple buffer
**
**	Returns: OK, ENDFILE
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	10-sep-87 (puree)
**	    convert to GCF architecture.
**	15-jun-90 (barbara)
**	    IIcgcp5_read_data will return GCA_RESPONSE even when it really means
**	    that the association failed.
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
**      14-jun-96 (loera01)
**          Modified so that decompression is performed in LIBQ, rather than in
**          LIBQGCA. 
**      13-aug-96 (loera01)
**          Fixed memory leaks in compressed varchar case by using tptr, which
**          is pre-allocated, and deallocated at the end of the copy function. 
**	3-dec-1997 (nanpr01)
**	    bug 87198 : partial tuples are not handled correctly.
**      27-may-1998 (wanya01)
**          bug 90926 : for uncompressed data, if tuple size is wide enough 
**          and need to span more than one GCA message, complete tuple can 
**          not be returned to calling function correctly.  
**	22-feb-1999 (nicph02)
**	    Bug 94410: Rewrote the code that retrieves compressed tuples which
**	    spans more that one GCA message. Also documented it.
**	11-aug-1999 (hayke02)
**	    Modified fix for bug 94410 so that when bytesleft < tuplen and
**	    support_compression is true (vch_compression ON)
**	    bufh->cpy_bytesused is correctly updated for non-partial tuples
**	    (ie. full uncompressed tuples). Also removed two unnecessary
**	    assignments of bytesleft. This change fixes bug 98334.
**	21-Dec-2001 (gordy)
**	    IICPdecompress() now takes a ROW_DESC instead of GCA tuple desc.
**	15-Dec-2009 (kschendel) SIR 122974
**	    All the INTO LOB stuff gets fired off from here now.
**	    Operate group-wise.
*/
static i4
IIcpdbread( II_LBQ_CB *IIlbqcb, II_CP_STRUCT *copy_blk,
	II_CP_GROUP *cgroup, PTR *tuple)
{
    IICGC_DESC	*cgc = IIlbqcb->ii_lq_gca;
    II_CPY_BUFH	*bufh = copy_blk->cp_cpy_bufh;
    ROW_DESC	*rdesc;
    i4		tuplen = cgroup->cpg_tup_length;
    i4		msg_type;
    i4		bytesleft;
    i4		bytescopy;
    PTR		cptr;		/* Pointer to where we're assembling compressed
				** message */
    PTR		gcaptr;		/* Pointer into GCA message buffer */
    PTR		tup_ptr;	/* Pointer to (decompressed) result */
    i4		tcount = 0;	/* Bytes of tup_ptr or cptr we have so far */
    i4		dif;
    bool	partial;
    STATUS	sts;

    /* If this is a dummy group corresponding to no table columns,
    ** just skip out now.
    */
    if (tuplen == 0 && !cgroup->cpg_is_tup_lob)
    {
	*tuple = NULL;		/* Make sure nothing uses it! */
	return (OK);
    }

    /*
    ** Get data for the next [partial] tuple from the DBMS.
    */
    rdesc = &IIlbqcb->ii_lq_retdesc.ii_rd_desc;
    tup_ptr = copy_blk->cp_tupbuf;
    if (cgroup->cpg_compressed)
    {
	/* May or may not need this, but go ahead and allocate it */
	if (copy_blk->cp_decomp_buf == NULL)
	{
	    /* Full-row tuplen is an upper bound for compressed size */
	    copy_blk->cp_decomp_buf = MEreqmem(0, copy_blk->cp_tup_length, 0, NULL);
	    if (copy_blk->cp_decomp_buf == NULL)
	       return(FAIL);
	}
	cptr = copy_blk->cp_decomp_buf;
    }

    for (;;)		/* loop until a complete tuple is received */
    {
	gcaptr = (char *)bufh->cpy_dbv.db_data + bufh->cpy_bytesused;
	bytesleft = bufh->cpy_dbv.db_length - bufh->cpy_bytesused;

	if (bytesleft == 0)
	{   /* Time to read a new message buffer */
            msg_type = IIcgcp5_read_data(cgc, &bufh->cpy_dbv);
	    if (msg_type == GCA_RESPONSE)	/* a RESPONSE block received */
	    {
		/* msg_type might be faked by IIcgcp5_read_data */
		if ( ! (IIlbqcb->ii_lq_flags & II_L_CONNECT) )
		    return FAIL;
		/* Proper end is only on the start of the first group */
		if (cgroup != &copy_blk->cp_copy_group[0] || tcount != 0)
		    return (FAIL);
		return (ENDFILE);
	    }
 
	    gcaptr = (char *)bufh->cpy_dbv.db_data;
	    bytesleft = bufh->cpy_dbv.db_length;
	    bufh->cpy_bytesused = 0;
	}

	/* Now that we've ensured that it's not EOF from the server,
	** see if this is a LOB group -- if so, let LOB code do it.
	** LOB reading doesn't know anything about our "cpy_bufh",
	** so update the underlying cgc stuff prior, then update
	** our cpy_bufh when the LOB is done.
	*/
	if (cgroup->cpg_is_tup_lob)
	{
	    cgc->cgc_result_msg.cgc_d_used = bufh->cpy_bytesused;
	    sts = IIcpinto_lob(IIlbqcb, copy_blk, cgroup);
	    /* Update bufh to current GCA position after LOB */
	    bufh->cpy_bytesused = cgc->cgc_result_msg.cgc_d_used;
	    bufh->cpy_dbv.db_length = cgc->cgc_result_msg.cgc_d_length;
	    return (sts);
	}

	/* If we are collecting a fragmented tuple already, or there's
	** (probably) not enough in the buffer to fill out the tuple
	** that we need, gather up fragments.
	*/
	if (tcount || bytesleft < tuplen)
	{
	    /* Smaller of what's there and what we still need.
	    ** ("what we still need" is an upper bound if GCA-compressed.)
	    */
	    bytescopy = min(bytesleft, tuplen-tcount);

	    if (!cgroup->cpg_compressed)
	    {
		/* simply glue the partial tuple onto what's in tupbuf */
		MEcopy(gcaptr, bytescopy, tup_ptr);
		tup_ptr += bytescopy;
		tcount += bytescopy;
		bufh->cpy_bytesused += bytescopy;
		if (tcount == tuplen)
		{
		    *tuple = copy_blk->cp_tupbuf;
		    return (OK);
		}
		/* Else loop to read more, still need tuplen - tcount */
	    }
	    else
	    {
		/* For compression, use decomp-buf to build up a
		** compressed tuple, decompress it into tup_buf.
		** If we got the whole thing, great;  otherwise keep
		** reading messages until we have it all.
		** The decompressor is not super-clever, it decompresses
		** the *entire* compressed tuple and returns whether
		** it's all there (partial=false) or not.  Since we don't
		** normally expect many trips through the loop, it seems
		** OK to leave it this way for now.
		*/
		MEcopy(gcaptr, bytescopy, cptr);
		tcount += bytescopy;
		IICPdecompress( rdesc,
			cgroup->cpg_first_attno, cgroup->cpg_last_attno,
			copy_blk->cp_decomp_buf,
			tcount, &dif, tup_ptr, &partial );
		if (partial)
		{
		    /* Couldn't decompress it all, need another message.
		    ** Advance cptr to reflect the (compressed) data
		    ** so far, and get more.  (tcount already advanced.)
		    */
		    cptr += bytescopy;
		    bufh->cpy_bytesused = bufh->cpy_dbv.db_length;
		    continue;
		}
		else
		{
		    /* Got it all now.  dif is how much larger the un-
		    ** compressed tuple is vs the compressed one.
		    ** The interesting question is how much data was part
		    ** of this compressed tuple, given that bytescopy is
		    ** potentially an over estimate:
		    **
		    ** |---+--|----|---A-B----|
		    **     |-------------| == tcount (old tcount+bytescopy)
		    **     |-----------| == what was actually needed to
		    **			    generate the decompressed tuple,
		    **			    this is tuplen-dif
		    ** difference == B - A == tcount - (tuplen - dif)
		    ** where bytesused should end up == A which is
		    ** bytescopy - (B - A).
		    */
		    *tuple = copy_blk->cp_tupbuf;
		    bufh->cpy_bytesused += bytescopy - (tcount - (tuplen - dif));
		    return (OK);
		}
	    }
	}
	else
	{
	    /* At least a full [partial] tuple is in the GCA buffer */
	    if (cgroup->cpg_compressed)
	    {
		IICPdecompress( rdesc,
			cgroup->cpg_first_attno, cgroup->cpg_last_attno,
			gcaptr,
			bytesleft, &dif, tup_ptr, &partial );
		/* Can't return partial */
		*tuple = copy_blk->cp_tupbuf;
		bufh->cpy_bytesused += tuplen - dif;
	    }
	    else
	    {
		*tuple = (char *)bufh->cpy_dbv.db_data + bufh->cpy_bytesused;
		bufh->cpy_bytesused += tuplen;
	    }
	    return (OK);
	}	    
    }
    /*NOTREACHED*/
    return (OK);
}

/*{
+*  Name: IIcpinto_init - Initialize for next row in COPY INTO
**
**  Description:
**	Initialize pointer to row buffer for next row in COPY INTO.
**
**	If a row/tuple conversion is necessary, make sure there is a buffer
**	allocated to format the next copy row.  If no conversion is needed,
**	point "row" to "tuple" so that we will just write the tuple directly
**	to the copy file.
**
**	We don't get here for LOB groups or "toss" groups with no domains.
**
**  Inputs:
**	copy_blk	- copy control block
**	row		- address of row buffer pointer
**	tuple		- tuple buffer pointer
**
**  Outputs:
**	*tuple		- set to the next tuple buffer
**	*row		- set to row buffer
**
**	Returns: OK, FAIL
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	10-sep-87 (puree)
**	    convert to GCF architecture.
*/
 
static i4
IIcpinto_init(II_CP_STRUCT *copy_blk, char **row, char *tuple)
{
    /*
    ** Find position for the copy file row.  If there is no conversion,
    ** then just use the tuple buffer directly.
    ** Otherwise, use the file row buffer.
    */
    if (copy_blk->cp_convert == FALSE)
    {
	*row = tuple;
    }
    else 
    {
	*row = copy_blk->cp_rowbuf;
    }
    return (OK);
} /* IIcpinto_init */

/*{
+*  Name: IIcprowformat - Transform copy tuple into row format.
**
**  Description:
**	Builds a copy row given a tuple and descriptions of row and
**	tuple format.
**
**	For each field of the copy row, call ADF to transform the attribute
**	value to the row domain value.  The ADF function blocks for each
**	copy row domain are already set up at copy initialization time.
**	Fields that may not be set up are:
**
**	    - Pointers to the data buffers must be set.
**	    - If we were not able to predict the result length of the ADF
**	      conversion (because the row domain is variable length), then
**	      we need to call adc_calclen to figure it out now.
**
**	If the row domain is TEXT type, then we have to do special processing
**	since COPY uses different TEXT format than the internal DB_TEXT_TYPE.
**	We give ADF a real DB_TEXT_TYPE to convert into, and then we strip off
**	the two-byte length specifier for the row domain value.
**
**	For dummy domains, the domain value is derived from the domain name.
**
**	Neither LOB nor "toss" groups (ie no domains) get here.
**
**  Inputs:
**	copy_blk	- copy control block
**	tuple		- pointer to tuple from the DBMS table
**
**  Outputs:
**	row		- filled in with copy row
**
**	Returns: OK, FAIL
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	25-aug-87 (puree)
**	    fix problem with varchar and text fields.
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	14-nov-88 (barbara)
**	    fixed bug #3912 -- value in db_t_count inaccurate causing av.
**	14-nov-88 (barbara)
**	    fixed bug #3695 -- isnull flag was never reset.
**	20-jan-89 (barbara)
**	    fixed bug whereby delimiter appears in the wrong place in output
**	    when a null varchar column is COPYed OUT as a fixed length field.
**	03-may-89 (kathryn) Bug #5583
**	    Set isnull to false when NOT NULL has been specified for a column.
**	    Fixes copy.out bug (AV) where col_1 has null, col_2 has been 
**	    specified NOT NULL so adc_isnull() is never called to reset isnull.
**	20-may-89 (kathryn) Bug #5605
**	    When value is null set txt_ptr->db_t_count to length of a NULL.
**	21-jun-89 (barbara)
**	    fixed bug #6380 where cp_name is an empty string for null
**	    dummy columns.
**	14-jun-89 (barbara)
**	    Fixed bug #5798 where default null format was not being blank
**	    padded into c0 or char(0) file format.
**	26-nov-89 (barbara)
**	    Fixed bug #8886.  On COPY TO don't pad default null value unless
**	    column width is wider than length of default null value.
**	24-jan-90 (fredp)
**	    String types represented by 'DB_TEXT_STRING's must be aligned
**	    in memory on BYTE_ALIGN machines. For these types, we MEcopy the
**	    tuple data to an aligned buffer and reference it instead of the
**	    (possibly) unaligned tuple.
**	06-feb-90 (fredp)
**	    Fixed alignment problems for non-string datatypes too.
**	23-apr-90 (mgw)
**	    Check to see if we need data to be BYTE_ALIGN'd on nullable
**	    text and varchar row formats too, not just non-nullable.
**	27-apr-90 (barbara)
**	    Nullable fixed-length varchar must put a 5-char count field
**	    into file.  Previously nullable text and varchar were "slipping
**	    through" the case statement and internal format was written to
**	    file.  Also, only MEfill with nulls if data is not null --
**	    otherwise the trailing null indicator byte will be overwritten.
**	12-oct-90 (teresal)
**	    Fixed alignment problems where an unaligned row buffer was
**	    being sent to ADF for non-string datatypes. Bug fix 33862.
**      03-Dec-2003 (gupsh01)
**          Modified copy code so that nchar and nvarchar types will now use
**          UTF8 to read and write from the text file for a formatted copy.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
**	15-Dec-2009 (kschendel) SIR 122974
**	    Call conversion function directly if available.  Add
**	    post-processing call to do CSV formatting.  Test for null
**	    directly instead of using adc_isnull, which merely adds overhead.
**	2-Mar-2010 (kschendel) SIR 122974
**	    Null-data padding for byte should be zeros, not spaces.  I doubt
**	    anyone would care, but might as well get it right.
*/

static i4
IIcprowformat( II_LBQ_CB *IIlbqcb, II_CP_STRUCT *copy_blk,
	II_CP_GROUP *cgroup, PTR tuple, PTR row )
{
    DB_STATUS		status;
    II_CP_MAP		*cmap;
    DB_TEXT_STRING	*txt_ptr;
    i4			absrtype, absttype;
    i4			j;
    i4			name_len;
    u_char		isnull;
    char		*row_ptr;
    char		*tup_ptr;
    char		*delim_ptr;
    ALIGN_RESTRICT	temp_buf[((DB_GW3_MAXTUP - 1) / sizeof(ALIGN_RESTRICT)) +1];
    i4			fillcnt;
#ifdef BYTE_ALIGN
    ALIGN_RESTRICT	align_buf[((DB_GW3_MAXTUP - 1) / sizeof(ALIGN_RESTRICT)) +1];
    ALIGN_RESTRICT	alignrow_buf[((DB_GW3_MAXTUP - 1) / sizeof(ALIGN_RESTRICT)) +1];
    i4			align_row = FALSE;
 
#endif /* BYTE_ALIGN */
 
    copy_blk->cp_status &=  ~CPY_ERROR;

    row_ptr = (char *) row;
 
    for (cmap = cgroup->cpg_first_map; cmap <= cgroup->cpg_last_map; ++cmap)
    {
	if (cmap->cp_rowlen == ADE_LEN_UNKNOWN)
	    cmap->cp_fieldlen = cmap->cp_cvlen;
 
	absrtype = cmap->cp_rowtype;
	if (absrtype != CPY_DUMMY_TYPE)
	    absrtype = abs(absrtype);
	absttype = abs(cmap->cp_tuptype);
	/*
	** If this is a dummy domain, construct the domain value.
	*/
	if (absrtype == CPY_DUMMY_TYPE)
	{
	    if (cmap->cp_rowlen > 0)
	    {
		/* 
		** Bug #6380 - cp_name will be an empty string
		** for null dummy columns
		*/
		name_len = STlength(cmap->cp_name);
		if (name_len == 0)
		    name_len = 1;
		for (j = 0; j < cmap->cp_rowlen; j++)
		{
		    MEcopy(cmap->cp_name, name_len, row_ptr);
		    row_ptr += name_len;
		}
	    }
	}
	else
	{
	    /* Convert the tuple value to row value */
#ifdef BYTE_ALIGN
	    if (ME_NOT_ALIGNED_MACRO((char*)tuple + cmap->cp_groupoffset)
		&& (absttype != DB_CHR_TYPE)
		&& (absttype != DB_CHA_TYPE)
		&& (absttype != DB_BYTE_TYPE))
	    {
		/*
		**  This data type must be aligned, so we copy
		**  it into an aligned buffer.
		*/
		MEcopy((PTR)((char *) tuple + cmap->cp_groupoffset),
			cmap->cp_tuplen, (PTR)align_buf);
 
		tup_ptr = (char *)align_buf;
	    }
	    else
#endif /* BYTE_ALIGN */
	    {
		tup_ptr = (char *) tuple + cmap->cp_groupoffset;
	    }
	    cmap->cp_adf_fnblk.adf_1_dv.db_data = tup_ptr;
 
	    isnull = 0;
	    if (cmap->cp_tuptype < 0)
	    {
		isnull = (*(u_char *)(tup_ptr+cmap->cp_tuplen-1) != 0);
	    }

	    /*
	    ** If this domain is a TEXT or VARCHAR field, convert into a
	    ** temp buffer and produce the final result in an external form.
	    ** Likewise if a CSV type field, coerce it into a temp buffer
	    ** so that the result can be checked for newlines, commas,
	    ** double quotes.
	    */
	    txt_ptr = NULL;
	    if ((absrtype == DB_TXT_TYPE)    ||
		(absrtype == DB_VBYTE_TYPE)  ||
		(absrtype == DB_VCH_TYPE)    ||
		(absrtype == DB_UTF8_TYPE))
	    {
		cmap->cp_adf_fnblk.adf_r_dv.db_data = (PTR) temp_buf;
		txt_ptr = (DB_TEXT_STRING *) temp_buf;
	    }
	    else if (cmap->cp_csv)
	    {
		cmap->cp_adf_fnblk.adf_r_dv.db_data = (PTR) temp_buf;
	    }
#ifdef BYTE_ALIGN
	    else if (ME_NOT_ALIGNED_MACRO(row_ptr)
		&& (absrtype !=  DB_BYTE_TYPE)
		&& (absrtype !=  DB_CHR_TYPE)
		&& (absrtype !=  DB_CHA_TYPE))
	    {
		/*
		** Non-string data must be formatted in an aligned buffer.
		** Before conversion, point to an aligned buffer to store
		** the result row data in and copy it the to row later on.
		** Bug 33862.
		*/
		cmap->cp_adf_fnblk.adf_r_dv.db_data = (PTR) alignrow_buf;
		align_row = TRUE;
	    }
#endif /* BYTE_ALIGN */
	    else
	    {
		cmap->cp_adf_fnblk.adf_r_dv.db_data = row_ptr;
	    }
 
	    /*
	    ** Do the conversion.
	    */
	    if (cmap->cp_rowtype < 0)
	    {
		/* Nullable row type, implies nullen is zero and withnull
		** is set.  Either way, make sure the null indicator is
		** properly set or cleared.
		** Even if we convert, the rdv length is set to the non
		** nullable length, so the null indicator is left alone.
		*/
		*(u_char *)(cmap->cp_adf_fnblk.adf_r_dv.db_data + cmap->cp_fieldlen - 1) = isnull;
	    }
	    if (!isnull)
	    {
		/* Fast-path if we have one, else use general call */
		if (cmap->cp_cvfunc != NULL)
		    status = (*cmap->cp_cvfunc)(IIlbqcb->ii_lq_adf,
			&cmap->cp_adf_fnblk.adf_1_dv,
			&cmap->cp_adf_fnblk.adf_r_dv);
		else
		    status = adf_func(IIlbqcb->ii_lq_adf, &cmap->cp_adf_fnblk);
		if (copy_blk->cp_status & CPY_ERROR)
		{
		    status = E_DB_ERROR;
		}
		if (status)
		{
		    char err_buf[15];
		    CVla(copy_blk->cp_rowcount + 1, err_buf);
		    IIlocerr(GE_LOGICAL_ERROR, E_CO003A_ROW_FORMAT_ERR, II_ERR, 
			     2, err_buf, cmap->cp_name);
		    copy_blk->cp_warnings++;
		    return (FAIL);
		}
	    }
	    else if (cmap->cp_withnull && cmap->cp_rowtype > 0)
	    {
		char *p;
		i4 len = cmap->cp_nullen;

		/* There's a null substitute.  Output it to where the
		** conversion would go.  If the row type uses a DB_TEXT_STRING,
		** go ahead and fill in the length;  this allows all of the
		** goofy DB_TEXT_STRING special cases to work (null pads
		** and so on), CSV post processing, etc etc.
		** (The null substitute in cp_nuldata is just text if it's
		** non-unicode string-like, no DB_TEXT_STRING-ness.)
		*/
		p = (char *)cmap->cp_adf_fnblk.adf_r_dv.db_data;
		if (txt_ptr != NULL)
		    p = (char *) &txt_ptr->db_t_text[0];
		MEcopy(cmap->cp_nuldata, len, p);
 
		if (txt_ptr != NULL)
		    txt_ptr->db_t_count = len;
		else if (cmap->cp_rowlen == ADE_LEN_UNKNOWN)
		{
		    /* Must be char(0) or c0; pad like coercion would have */
		    fillcnt = cmap->cp_fieldlen - len;
		    if (fillcnt > 0)
		    {
			char f = ' ';
			if (absrtype == DB_BYTE_TYPE)
			    f = '\0';
			MEfill(fillcnt, f, p + len);
		    }
		    if (len > cmap->cp_fieldlen)
			cmap->cp_fieldlen = len;
		}    
	    }
	    else if (!cmap->cp_withnull)
	    {
		i4 rc = copy_blk->cp_rowcount+1;
		/* NULL, but no WITH NULL clause */
		/* This used to give a vague "cannot convert" from the
		** null to not-nullable adf coercion, but we can do
		** better than that now that the null is handled separately.
		*/
		IIlocerr(GE_DATA_EXCEPTION, E_CO0004_NULL_INTO_NOTNULL,
			II_ERR, 2, &rc, cmap->cp_name);
		copy_blk->cp_warnings++;
		return (FAIL);
	    }
	    /* else it's withnull && nullable rowtype and we already stuck the
	    ** null indicator into the output value, above.
	    */
 
	    if (cmap->cp_csv)
		IIcpinto_csv(copy_blk, cmap, txt_ptr, row_ptr);
	    else if (txt_ptr != NULL)
	    {
		char	*ptr;

		ptr = row_ptr;

		if (cmap->cp_rowlen == ADE_LEN_UNKNOWN)
		{
		    /*
		    ** Variable length varchar or byte type. Put length
		    ** specifier then the string into the row buffer.
		    */
		    cmap->cp_fieldlen = txt_ptr->db_t_count;
		    if (absrtype != DB_TXT_TYPE)
		    {
			cmap->cp_fieldlen += CPY_VCHLEN;
			STprintf(ptr, "%5d", (i4)txt_ptr->db_t_count);
			ptr += CPY_VCHLEN;
		    }
		    MEcopy((PTR)txt_ptr->db_t_text,
			txt_ptr->db_t_count, (PTR)ptr);
		}
		else
		{
		    /*
		    ** Fixed length varchar type.  Put length specifier and
		    ** then the string, pad with nulls.
		    */
		    if (absrtype != DB_TXT_TYPE)
		    {
			if (isnull && cmap->cp_rowtype < 0)
			    MEfill(CPY_VCHLEN, ' ', ptr);
			else
			    STprintf(ptr, "%5d", (i4)txt_ptr->db_t_count);
			ptr += CPY_VCHLEN;
		    }
		    if (isnull && cmap->cp_rowtype < 0)
		    {
			/* Just set the indicator, ignore the rest */
			*(row_ptr + cmap->cp_fieldlen - 1) = 1;
		    }
		    else
		    {
			MEcopy((PTR)txt_ptr->db_t_text, 
				txt_ptr->db_t_count, (PTR)ptr);
			fillcnt = cmap->cp_rowlen - DB_CNTSIZE
				    - txt_ptr->db_t_count;
			if (fillcnt > 0)
			    MEfill(fillcnt, '\0',
				    (PTR)(ptr + txt_ptr->db_t_count));
		    }
		}
	    }
#ifdef BYTE_ALIGN
	    else if (align_row)
	    {
		/*
		** Copy non-string data from the aligned buffer into the
		** row buffer.
		*/
		MEcopy((PTR)alignrow_buf, cmap->cp_fieldlen,
				 (PTR) row_ptr);
		align_row = FALSE;
	    }
#endif /* BYTE_ALIGN */
 
	    row_ptr += cmap->cp_fieldlen;
	}
 
	/*
	** If there is a delimiter specified, write it to the row buffer.
	*/
	if (cmap->cp_isdelim)
	{
	    delim_ptr = cmap->cp_delim;
	    CMcpyinc(delim_ptr, row_ptr);
	}
    }
 
    /*
    ** If this row format has variable length, then set the copy control block
    ** row length field to the length of the current row so other routines will
    ** know how to handle it.
    */
    if (cgroup->cpg_varrows)
	cgroup->cpg_row_length = row_ptr - (char *) row;
 
    return (OK);
} /* IIcprowformat */

/*
** Name: IIcpinto_lob - Large object handling for COPY INTO.
**
** Description:
**
**	This routine handles a copy group which is (one) large object,
**	for COPY INTO.
**
**	If the LOB does not correspond to any file domain (cmaps are null),
**	just read the LOB and throw it away.
**
**	If we aren't converting (binary copy into), write the LOB as
**	segments preceded by i2 lengths, max length MAX_SEG_INTO_FILE.
**	The length is in characters (== 2 octets each if NVARCHAR) and
**	the data is in native form (UCS2 if NVARCHAR).  If the LOB column
**	is nullable, write a null indicator after the last (zero-length)
**	lob segment.
**
**	For converting copies, the LOB is likewise written as segments,
**	but the segment lengths are written as space-terminated ASCII
**	numbers (rather than i2 binary);  and there is no null indicator.
**	A NULL is written as the WITH NULL value.  NVARCHAR columns are
**	written as UTF8.
**
**	Unlike COPY FROM, it appears that the back-end does not allow
**	long -> not long copies;  so we can assume that the row
**	value will be segmented and flushed before returning.
**
** Inputs:
**	IIlbqcb		- LIBQ control block
**	cblk		- copy control data block
**	cgroup		- copy group corresponding to this LOB
**
** Outputs:
**	Returns OK, ENDFILE, or FAIL
**
** History:
**	15-Dec-2009 (kschendel) SIR 122974
**	    Adapt LOB handling to new copy framework.
*/

static STATUS
IIcpinto_lob(II_LBQ_CB *IIlbqcb, II_CP_STRUCT *cblk, II_CP_GROUP *cgroup)
{

    II_CP_MAP	*cmap;
    LOBULKINTO	lob_info;
    i4		len;
    i2		nulblob;
    STATUS	sts;

    cmap = cgroup->cpg_first_map;
    lob_info.cblk = cblk;
    lob_info.libq_cb = IIlbqcb;
    lob_info.cgroup = cgroup;
    IIlbqcb->ii_lq_retdesc.ii_rd_colnum = cgroup->cpg_first_attno;

    /* We'll write into the row buffer.  The preceding group will have
    ** flushed out the buffer (or not used it at all).  Use cp_dataend
    ** and cp_rowptr to track the row buffer position.  LOB segments
    ** are supposed to fit into the row buffer, so the dataend checks are
    ** just for safety.
    */
    cblk->cp_rowptr = (u_char *) cblk->cp_rowbuf;
    cblk->cp_dataend = cblk->cp_rowptr + cblk->cp_rbuf_size;

    /* If no corresponding file domain, just read and toss the LOB */
    if (cmap == NULL)
    {
	IILQldh_LoDataHandler(1, &nulblob, IICPfbFlushBlob, NULL);
	return (OK);
    }

    /* Run the output data-handler which will read and write the LOB.
    ** The LOB handler may (probably will) or may not do its own row-buffer
    ** flushing;  but if it decides to leave "stuff" in the row buffer,
    ** it agrees to set rowptr appropriately.
    */
    lob_info.failure = FALSE;
    IILQldh_LoDataHandler(1, &nulblob, IIcpinto_lobHandler, (PTR) &lob_info);
    if (lob_info.failure)
	return (FAIL);

    if (nulblob != DB_EMB_NULL)
    {
	/* LOB was not null.  If we're doing a binary INTO and the LOB
	** is nullable, append a zero as a not-null indicator.
	*/
	if (!cblk->cp_convert && cmap->cp_tuptype < 0)
	{
	    if (cblk->cp_rowptr >= cblk->cp_dataend)
	    {
		IICPwemWriteErrorMsg(E_CO0041_FILE_WRITE_MSG_ERR,
			cblk, cmap->cp_name,
			ERget(S_CO0054_NO_NOT_NULL));
		return (FAIL);
	    }
	    *cblk->cp_rowptr++ = '\0';
	}
    }
    else
    {
	/* LOB was a NULL.  The lobHandler is not called for nulls, we
	** have to output either an empty segment with a null indicator
	** (for binary), or the null marker value and an empty segment
	** (for formatted), or an error (if there is no null marker).
	*/
	if (!cblk->cp_convert)
	{
	    /* i2 length, byte indicator */
	    if (cblk->cp_rowptr + sizeof(i2) + 1 > cblk->cp_dataend)
	    {
		IICPwemWriteErrorMsg(E_CO0041_FILE_WRITE_MSG_ERR,
			cblk, cmap->cp_name,
			ERget(S_CO0052_NO_NULL_IND));
		return (FAIL);
	    }
	    nulblob = 0;
	    I2ASSIGN_MACRO(nulblob, *cblk->cp_rowptr);
	    cblk->cp_rowptr += sizeof(i2);
	    *cblk->cp_rowptr++ = 1;
	}
	else if (!cmap->cp_withnull)
	{
	    i4 rc = cblk->cp_rowcount+1;
	    /* Formatted, but no NULL marker was defined */
	    IIlocerr(GE_DATA_EXCEPTION, E_CO0004_NULL_INTO_NOTNULL, II_ERR,
		2, &rc, cmap->cp_name);
	    return (FAIL);
	}
	else
	{
	    char *p = (char *) cblk->cp_rowptr;

	    /* Formatted, write the WITH NULL string as a segment, then
	    ** the terminator empty segment. For example, '5 !NUL!0 '.
	    ** Assume the null length is 99999 or less here.
	    ** Be careful to handle "WITH NULL ('')" properly too.
	    */
	    if ((u_char *)p + (cmap->cp_nullen+8) > cblk->cp_dataend)
	    {
		IICPwemWriteErrorMsg(E_CO0041_FILE_WRITE_MSG_ERR,
			cblk, cmap->cp_name,
			ERget(S_CO0062_WRT_NULL_VAL));
		return (FAIL);
	    }
	    if (cmap->cp_nullen == 0)
	    {
		STcopy("0 ", p);
		p += 2;
	    }
	    else
	    {
		CVna(cmap->cp_nullen, p);
		p += STlength(p);
		*p++ = ' ';
		STlcopy(cmap->cp_nuldata, p, cmap->cp_nullen);
		STcat(p, "0 ");
		p += STlength(p);
	    }
	    cblk->cp_rowptr = (u_char *) p;
	}
    }

    /* If formatted copy and delimiter requested, add the delimiter */
    if (cblk->cp_convert && cmap->cp_isdelim)
    {
	if (cblk->cp_rowptr + cmap->cp_delim_len > cblk->cp_dataend)
	{
	    IICPwemWriteErrorMsg(E_CO0041_FILE_WRITE_MSG_ERR,
		cblk, cmap->cp_name,
		ERget(S_CO0059_NO_SEG));
	    return (FAIL);
	}
	MEcopy(cmap->cp_delim, cmap->cp_delim_len, (PTR) cblk->cp_rowptr);
	cblk->cp_rowptr += cmap->cp_delim_len;
    }

    /* If there's anything left in the row buffer to write, write it.
    ** The LOB handler may or may not have taken care of all of it.
    */
    len = cblk->cp_rowptr - (u_char *) cblk->cp_rowbuf;
    sts = OK;
    if (len > 0)
	sts = IIcpputrow(cblk, len, cblk->cp_rowbuf);
    return (sts);

} /* IIcpinto_lob */

/*{
+*  Name: IICPfbFlushBlob - Data Handler for flushing a blob from the DBMS
**
**  Description:            This routine calls IILQled_LoEndData() to flush
**                          the current blob from the DBMS into oblivion
**                          since it wasn't asked for on the Format line.
**  Inputs:
**	none
**
**  Outputs:
**	none
**
**  Returns: void
**
**  History:
**	jun-1993 (mgw)	- Written
*/
static void
IICPfbFlushBlob()
{
	/* Flush the blob */
	IILQled_LoEndData();
}

/*
** Name: IIcpinto_lobHandler -- Data handler for LOB during COPY INTO.
**
** Description:
**
**	This routine is the LOB callback used during COPY INTO.
**	LIBQ feeds us a LOB segment of some length, and we'll write
**	it out to the INTO file as segments, possibly after character
**	set transformation.  After all segments are written, a zero
**	length segment is also written to end the LOB.  The caller
**	will take care of any trailing null-indicator needed.
**
**	NULL LOB's don't get here.  A NULL from the back-end is
**	reported immediately to the into-lob driver, who will do
**	the right thing.
**
**	The incoming LOB will be read into the row buffer (which is
**	of length MAX_SEG_INTO_FILE, plus a hundred or so bytes for slop.)
**	The caller will prepare cp_rowptr and cp_dataend to point to
**	an empty row buffer.  Upon return, cp_rowptr will reflect
**	what's still in the row buffer to be dumped, which might be
**	(will be, at present) nothing at all.
**
** Inputs:
**	lob_info	- LOBULKINTO structure bundling copy data
**
** Outputs:
**	lob_info.failure set TRUE if error, plus we issue any messages.
**
** History:
**	16-Dec-2009 (kschendel) SIR 122974
**	    Adapt old LOB handling to new copy framework.
*/

static void
IIcpinto_lobHandler(LOBULKINTO *lob_info)

{
    II_CP_STRUCT  *cblk = lob_info->cblk;
    II_CP_GROUP	*cgroup = lob_info->cgroup;
    II_CP_MAP	*cmap = cgroup->cpg_first_map;
    char	numbuf[15];	/* "nnnn " for formatted length */
    DB_DATA_VALUE ucs2_dv, rdv;	/* For UCS2 -> UTF8 conversion */
    DB_STATUS	dbstat;
    i4		absttype;
    i4		curseg;		/* for messages */
    i4		data_end;	/* End-of-data indicator */
    i4		lo_type;	/* Underlying type (char or nchar) */
    i4		segbytes;	/* Segment length in bytes */
    i4		seg_flags;
    i4		seglen;		/* Segment length in characters */
    i2		i2len;		/* Binary segment lengths are output as i2 */
    PTR		segbuf;		/* Where incoming segments go */
    STATUS	sts;

    absttype = abs(cmap->cp_tuptype);
    lo_type = DB_CHR_TYPE;
    if (absttype == DB_LNVCHR_TYPE)
    {
	lo_type = DB_NCHR_TYPE;
	if (cblk->cp_convert)
	{
	    /* Set up UCS2 -> UTF8 conversion */
	    rdv.db_datatype = DB_UTF8_TYPE;
	    ucs2_dv.db_datatype = DB_NCHR_TYPE;
	    /* prec, collID uninteresting; length, addr set below */
	    /* Set up a conversion area if we haven't already.  Worst case
	    ** is 6 UTF8 octets for each input UCS2 char = 2 octets, ie a
	    ** 3x expansion.
	    */
	    if (cgroup->cpg_ucv_buf == NULL)
	    {
		cgroup->cpg_ucv_len = sizeof(i2) + MAX_SEG_INTO_FILE * 3;
		cgroup->cpg_ucv_buf = MEreqmem(0, cgroup->cpg_ucv_len, FALSE, NULL);
		if (cgroup->cpg_ucv_buf == NULL)
		{
		    IIlocerr(GE_LOGICAL_ERROR, E_CO0037_MEM_INIT_ERR, II_ERR,
			    0, NULL);
		    lob_info->failure = TRUE;
		    return;
		}
	    }
	}
    }

    seg_flags = II_DATSEG | II_DATLEN;
    if (absttype == DB_LNVCHR_TYPE || !cblk->cp_convert)
	seg_flags |= II_BCOPY;		/* Omit UCS2 -> wchar_t conversion */
    data_end = 0;
    curseg = 1;

    /* Loop reading and writing segments */
    do
    {
	segbuf = cblk->cp_rowbuf;
	IILQlgd_LoGetData(seg_flags, lo_type, 0,
			segbuf, MAX_SEG_INTO_FILE, &seglen, &data_end);
	segbytes = seglen;
	if (lo_type == DB_NCHR_TYPE)
	    segbytes = segbytes * sizeof(UCS2);

	if (cblk->cp_dbg_ptr)
	    SIfprintf(cblk->cp_dbg_ptr, "COPYINTO: seg %d len %d bytes %d data_end %d\n", 
		curseg, seglen, segbytes, data_end);

	if (cblk->cp_convert && lo_type == DB_NCHR_TYPE)
	{
	    /* convert UCS2 into UTF8 for formatted copy into */
	    ucs2_dv.db_data = segbuf;
	    ucs2_dv.db_length = segbytes;
	    rdv.db_data = cgroup->cpg_ucv_buf;
	    rdv.db_length = cgroup->cpg_ucv_len;
	    ((DB_TEXT_STRING *)rdv.db_data)->db_t_count = cgroup->cpg_ucv_len - sizeof(i2);
	    dbstat = adu_nvchr_toutf8(lob_info->libq_cb->ii_lq_adf, &ucs2_dv, &rdv);
	    if (dbstat != E_DB_OK)
	    {
		IICPwemWriteErrorMsg(E_CO003A_ROW_FORMAT_ERR,
			cblk, cmap->cp_name, "");
		lob_info->failure = TRUE;
		return;
	    }
	    segbuf = (PTR) &((DB_TEXT_STRING *)rdv.db_data)->db_t_text[0];
	    segbytes = ((DB_TEXT_STRING *)rdv.db_data)->db_t_count;
	    seglen = segbytes;
	}
	/* Write the length (in characters) of the segment, an i2 if binary,
	** or "seglen " if formatted.
	*/
	if (!cblk->cp_convert)
	{
	    i2len = seglen;
	    sts = IIcpputrow(cblk, sizeof(i2), (PTR) &i2len);
	}
	else
	{
	    CVla(seglen, numbuf);
	    STcat(numbuf, " ");
	    sts = IIcpputrow(cblk, STlength(numbuf), (PTR) &numbuf);
	}
	if (sts != OK)
	{
	    IICPwemWriteErrorMsg(E_CO0041_FILE_WRITE_MSG_ERR,
		cblk, cmap->cp_name, ERget(S_CO0058_NO_SEG_LEN));
	    lob_info->failure = TRUE;
	    return;
	}
	/* Now the data, if there is any */
	if (segbytes > 0)
	{
	    sts = IIcpputrow(cblk, segbytes, segbuf);
	    if (sts != OK)
	    {
		IICPwemWriteErrorMsg(E_CO0041_FILE_WRITE_MSG_ERR,
		    cblk, cmap->cp_name, ERget(S_CO0059_NO_SEG));
		lob_info->failure = TRUE;
		return;
	    }
	}
    } while (!data_end);

    /* If the last segment output was non-empty, append a "no more data"
    ** segment indicating the end.  This just a bare zero segment length.
    */
    if (segbytes > 0)
    {
	if (!cblk->cp_convert)
	{
	    i2len = 0;
	    sts = IIcpputrow(cblk, sizeof(i2), (PTR) &i2len);
	}
	else
	{
	    sts = IIcpputrow(cblk, 2, (PTR) "0 ");
	}
	if (sts != OK)
	{
	    IICPwemWriteErrorMsg(E_CO0041_FILE_WRITE_MSG_ERR,
		cblk, cmap->cp_name, ERget(S_CO0058_NO_SEG_LEN));
	    lob_info->failure = TRUE;
	    return;
	}
    }
} /* IIcpinto_lobHandler */

/*{
**  Name: IIcpputrow - write out copy row during COPY INTO.
**
**  Description:
**	If this is a COPY INTO PROGRAM, this routine calls the user supplied
**	"put_row" routine to dispose of the copy row.
**
**	If this is a normal copy, the row is written to the copy file.  The
**	copy file must be opened for writing.
**
**	Neither LOB nor "toss" groups get here.
**
**  Inputs:
**	copy_blk	- copy control block
**	len		- Length in bytes to write
**	row		- points to copy row
**
**  Outputs:
**	Returns: OK, FAIL
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	16-Dec-2009 (kschendel) SIR 122974
**	    Change call to supply length.
*/
 
static i4
IIcpputrow(II_CP_STRUCT *copy_blk, i4 len, PTR row)
{
    STATUS	status;
    i4		save_len;
    i4		num_written;
 
    /*
    ** Write row to copy file.
    **
    ** If this is an in-memory copy, just send this row to the user-specified
    ** output routine.
    */
    if (copy_blk->cp_program)
    {
	save_len = len;		/* Don't trust user callback */
	status = (*copy_blk->cp_usr_row)(&save_len, row, &num_written);
    }
    else 
    {
	status = SIwrite(len, row, &num_written, copy_blk->cp_file_ptr);
    }
    if ((status != OK) || (num_written != len))
    {
	char err_buf[15];
	CVla(copy_blk->cp_rowcount + 1, err_buf);
	IIlocerr(GE_LOGICAL_ERROR, 
		copy_blk->cp_program ? E_CO003D_PROG_WRITE_ERR : E_CO003B_FILE_WRITE_ERR,
		II_ERR, 1, err_buf);
	copy_blk->cp_warnings++;
	return (FAIL);
    }
 
    return (OK);
}

/*
** Name: IIcpinto_csv -- Comma-Separated Value formatting for COPY FROM
**
** Description:
**	The CSV format says that a value must be double quoted if
**	it contains commas, double quotes, or newlines.  A double
**	quote inside a double-quoted string is doubled.  Since we've
**	also implemented an SSV variant using a semicolon separator,
**	we'll use similar rules for it (but quoting semicolons, rather
**	than commas).
**
**	The caller supplies a coerced copy-into value, either a bare
**	character string of length cp_rowlen, or a DB_TEXT_STRING;
**	and a pointer to the output buffer where we'll build the value.
**
**	Making the assumption that the need for double quoting is
**	comparatively rare, we'll inspect the result string as we
**	copy it to the output area.  If we hit one of the quote-
**	needed character, we do it again, but with quoting and doubling.
**
** Inputs:
**	cblk		- Copy control data block.
**	cmap		- Copy domain's info block
**	txt_ptr		- Pointer to a DB_TEXT_STRING holding the value;
**			  if NULL, use cmap->cp_adf_fnblk.adf_r_db.db_data.
**	row_ptr		- Pointer to result.  We'll assume (!) that it is
**			  large enough.
**
** Outputs:
**	sets cp_fieldlen to actual output length
**
** History:
**	16-Dec-2009 (kschendel) SIR 122974
**	   Write CSV output.
*/

static void
IIcpinto_csv(II_CP_STRUCT *cblk, II_CP_MAP *cmap,
	DB_TEXT_STRING *txt_ptr, char *row_ptr)
{

    register char *taker, *putter;
    register char ch;
    char sep;		/* comma or semicolon */
    register i4 len;

    sep = ',';
    if (cmap->cp_ssv)
	sep = ';';
    putter = row_ptr;
    /* Set up character source, either DB_TEXT_STRING or ordinary string */
    if (txt_ptr != NULL)
    {
	len = txt_ptr->db_t_count;
	taker = (char *) txt_ptr->db_t_text;
    }
    else
    {
	len = cmap->cp_cvlen;
	taker = (char *) cmap->cp_adf_fnblk.adf_r_dv.db_data;
    }
    /* Copy while scanning for special chars */
    while (--len >= 0)
    {
	ch = *taker++;
	if (ch == sep || ch == '\n' || ch == '"')
	    break;
	*putter++ = ch;
    }
    if (len < 0)
    {
	cmap->cp_fieldlen = putter - row_ptr;
	return;		/* No quoting needed */
    }

    /* Do it again, this time with quoting and quote doubling. */
    putter = row_ptr;
    if (txt_ptr != NULL)
    {
	len = txt_ptr->db_t_count;
	taker = (char *) txt_ptr->db_t_text;
    }
    else
    {
	len = cmap->cp_fieldlen;
	taker = (char *) cmap->cp_adf_fnblk.adf_r_dv.db_data;
    }
    *putter++ = '"';
    while (--len >= 0)
    {
	ch = *taker++;
	if (ch == '"')
	    *putter++ = '"';	/* Quote doubling here */
	*putter++ = ch;
    }
    *putter++ = '"';
    cmap->cp_fieldlen = putter - row_ptr;
    return;

} /* IIcpinto_csv */

/*{
+*  Name: IIcpfrom_init - Initialize for next group in COPY FROM
**
**  Description:
**
**	Init for another group by resetting the input buffering and logging
**	info.  If the group is not a dummy group, decide where the
**	group tuple data goes:
**	- if it's a LOB group, datahandlers will do it, but the underlying
**	  CGC structures have to be updated from the copy buffer tracker;
**	- if the group is larger than the GCA buffer, flush the buffer if
**	  it's nonempty, and build the tuple in cp_tupbuf for later
**	  transmission in pieces.
**	- If the group can fit in the GCA buffer, flush the buffer if it's
**	  too full, and then build the tuple in the GCA buffer.
**
**	When flushing the GCA buffer, we set EOD if the last thing
**	in the buffer was the end of (the previous) row; the buffer
**	tracker has that info.  (It turns out that EOD during the CDATA
**	stream is not important, now that the sequencer is fixed, but
**	we'll continue to set it according to whether a complete row
**	has been sent or not.)
**
**  Inputs:
**	IIlbqcb		- thread's libq control block
**	copy_blk	- copy control block
**	cgroup		- copy group info
**	tuple		- addr of tuple buffer pointer
**
**  Outputs:
**	*tuple		- set to the next tuple buffer (meaningless if LOB)
**
**	Returns: OK, FAIL
**
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	10-sep-87 (puree)
**	    convert to GCF architecture.
**	29-nov-91 (seg)
**	    Can't dereference or do arithmetic on PTR
**	1-Dec-2009 (kschendel) SIR 122974
**	    Don't worry about row-buffer side here.
**	    Do the right thing for LOB groups, dummy groups.
**	29-Dec-2009 (kschendel) SIR 122974
**	    Use lazy flushing: flush only if this group doesn't fit.
*/
static STATUS
IIcpfrom_init ( 
    II_LBQ_CB		*IIlbqcb, 
    II_CP_STRUCT	*copy_blk, 
    II_CP_GROUP		*cgroup,
    PTR			*tuple
)
{
    IICGC_DESC	*cgc;
    II_CPY_BUFH	*cpy_bufh;
    STATUS	status;
 
    /* If logging, reset the log line image pointer and count */
    if (copy_blk->cp_rowlog)
    {
	copy_blk->cp_log_left = copy_blk->cp_log_size;
	copy_blk->cp_log_bufptr = copy_blk->cp_log_buf;
    }

    /* Set file buffer refill control: EOF is OK on the first group, but
    ** not thereafter.  Irrelevant if no domains in this group.
    */
    copy_blk->cp_refill_control = cgroup->cpg_refill_control;

    /* An all-dummy domain just tosses stuff out of the input file,
    ** no tuple-side activity this group.
    */
    if (cgroup->cpg_tup_length == 0 && !cgroup->cpg_is_tup_lob)
    {
	*tuple = NULL;		/* Make sure nobody uses it! */
	return (OK);
    }
    cgc = IIlbqcb->ii_lq_gca;
    cpy_bufh = copy_blk->cp_cpy_bufh;
    status = OK;

    /* 
    ** If there is nothing in GCA buffer, initialize it for a
    ** a new message.
    */
    if (cpy_bufh->cpy_bytesused == 0 )
    {
	IIcgc_init_msg(cgc, GCA_CDATA, cgc->cgc_query_lang, 0);
	/* Doesn't really write, just inits cpy-dbv */
	status = IIcgcp3_write_data(cgc, FALSE, (i4)0, &cpy_bufh->cpy_dbv);
	cpy_bufh->cpy_bytesused = 0;
    }
 
    if (cgroup->cpg_is_tup_lob)
    {
	*tuple = NULL;		/* Meaningless so make sure not used */
	/* Update cgc stuff from our buffer tracker since LOB bypasses the
	** copy buffer tracker inside the datahandlers.
	*/
	cgc->cgc_write_msg.cgc_d_used = cpy_bufh->cpy_bytesused;
    }
    else
    {
	if (cpy_bufh->cpy_bytesused > 0
	  && cpy_bufh->cpy_bytesused + cgroup->cpg_tup_length > cpy_bufh->cpy_dbv.db_length)
	{
	    /* Need to flush what's in there now. */
	    if (IIcgcp3_write_data(cgc, cpy_bufh->end_of_data,
			cpy_bufh->cpy_bytesused, &cpy_bufh->cpy_dbv) != OK)
		return (FAIL);
	    copy_blk->cp_sent = TRUE;
	    cpy_bufh->cpy_bytesused = 0;
	}
	*tuple = cpy_bufh->cpy_dbv.db_data + cpy_bufh->cpy_bytesused;
	if (cpy_bufh->cpy_dbv.db_length < cgroup->cpg_tup_length)
	    *tuple = copy_blk->cp_tupbuf;
    }
    cpy_bufh->end_of_data = FALSE;

    return (status);
}
 

/*
** Name: IIcpfrom_binary -- Read binary file row for COPY FROM
**
** Description:
**
**	This routine reads a binary row out of the COPY FROM file
**	and moves it to the current tuple area.  If the row needs
**	any sort of post-validation or unicode normalization,
**	we'll creep through the columns doing same.
**
**	Since binary copy uses fixed length input (and output) rows,
**	buffer handling is simplified as compared to the formatted
**	style.  In fact, if the input is FROM PROGRAM, no row buffer
**	is used at all -- reads are done directly into the tuple
**	area.  (For input from a file, we use a row buffer mostly
**	to assure decent read sizes;  there's no guarantee that SIread
**	has buffering behind it.)
**
**	This code is not used for LOB columns.  LOB binary values are
**	indefinite length, and are read in segments from the input file.
**	In addition, since this is binary copy, the group is guaranteed
**	to have real columns in it (ie it's not a dummy-only group).
**
** Inputs:
**	thr_cb			LIBQ thread control block
**	copy_blk		Overall copy control block pointer
**	cgroup			Copy group info
**	tuple			Where to put the output tuple
**
** Outputs:
**	Returns OK, ENDFILE, or failure status.
**
** History:
**	8-Dec-2009 (kschendel) SIR 122974
**	    Redo the COPY FROM code for speed.
**      8-Mar-2010 (hanal04) Bug 122974
**          IICPvcValCheck() will not normalize UTF8 data. We still need
**          to normalize even if cp_valchk is true.
**      4-Mar-2010 (hanal04) Bug 123365
**          Having called adu_unorm() to convert to NFD/NFC we need to copy
**          the result to the tuple that is being inserted, not the
**          copy buffer that's being read.
**      21-Jun-2010 (horda03) b123926
**          Because adu_unorm() and adu_utf8_unorm() are also called via 
**          adu_lo_filter() change parameter order.
**	28-Jun-2010 (kschendel) b123990
**	    Use column length, not group length, when validating and
**	    normalizing unicode columns.
*/

static STATUS
IIcpfrom_binary(II_THR_CB *thr_cb, II_CP_STRUCT *copy_blk,
	II_CP_GROUP *cgroup, PTR tuple)
{
    DB_STATUS status;
    II_CP_MAP *cmap;
    II_LBQ_CB *IIlbqcb = thr_cb->ii_th_session;
    i4 absttype;
    i4 len;
    i4 rowlen;
    PTR tup_ptr;
    STATUS sts;

    rowlen = cgroup->cpg_row_length;
    if (copy_blk->cp_program && !copy_blk->cp_has_blobs)
    {
	i4 num_read, save_len;

	/* LOB-less COPY FROM program reads direct into the tuple buffer */
	len = rowlen;
	tup_ptr = tuple;
	do
	{
	    save_len = len;	/* len shouldn't be smashed, but be safe */
	    sts = (*copy_blk->cp_usr_row)(&len, tup_ptr, &num_read);
	    if (sts == ENDFILE && save_len == rowlen)
		return (ENDFILE);
	    else if (sts != OK)
	    {
		char err_buf[15];
		CVla(copy_blk->cp_rowcount + 1, err_buf);
		IIlocerr(GE_LOGICAL_ERROR, E_CO003C_PROG_READ_ERR, II_ERR,
			1, err_buf);
		return (FAIL);
	    }
	    /* Don't assume program will return as much as requested */
	    len = save_len - num_read;
	    tup_ptr += num_read;
	} while (len > 0);
    }
    else
    {
	/* Buffered COPY FROM, make sure entire group is available in the
	** row buffer.
	*/

	copy_blk->cp_valstart = copy_blk->cp_rowptr;
	copy_blk->cp_rowptr += rowlen;
	if (copy_blk->cp_rowptr > copy_blk->cp_dataend)
	{
	    -- copy_blk->cp_rowptr;	/* cp_refill precondition */
	    sts = cp_refill(&copy_blk->cp_dataend, &copy_blk->cp_rowptr, copy_blk);
	    if (sts == COPY_EOF)
		return (ENDFILE);
	    else if (sts == COPY_FAIL)
	    {
		char err_buf[15];
		CVla(copy_blk->cp_rowcount + 1, err_buf);
		IIlocerr(GE_LOGICAL_ERROR, E_CO0024_FILE_READ_ERR, II_ERR,
			1, err_buf);
		return (FAIL);
	    }
	}
	MEcopy((PTR) copy_blk->cp_valstart, rowlen, tuple);
	CP_LOGTO_MACRO(copy_blk->cp_rowptr, copy_blk);
    }
    if (! cgroup->cpg_validate)
	return (OK);

    /* Run through the columns in the tuple buffer and validate, or
    ** unicode-normalize, as required.
    */
    tup_ptr = tuple;
    for (cmap = cgroup->cpg_first_map; cmap <= cgroup->cpg_last_map; ++cmap)
    {
	absttype = abs(cmap->cp_tuptype);
	if (!cmap->cp_valchk
	  && absttype != DB_NCHR_TYPE && absttype != DB_NVCHR_TYPE)
	{
	    /* No action on this one */
	    tup_ptr += cmap->cp_tuplen;
	    continue;
	}
	if (cmap->cp_tuptype < 0
	  && *(tup_ptr + cmap->cp_tuplen - 1) != 0)
	{
	    /* Don't validate or normalize NULL's! */
	    tup_ptr += cmap->cp_tuplen;
	    continue;
	}
	if (cmap->cp_valchk)
	{
	    PTR aligned_tupval;

#ifdef BYTE_ALIGN
	    ALIGN_RESTRICT tupval_buf[(DB_MAXSTRING+DB_CNTSIZE+sizeof(ALIGN_RESTRICT)) / sizeof(ALIGN_RESTRICT)];

	    /* Copy to alignment buffer if needed */
	    if (ME_NOT_ALIGNED_MACRO(tup_ptr))
	    {
		MEcopy(tup_ptr, cmap->cp_tuplen, (PTR) tupval_buf);
		aligned_tupval = (PTR) tupval_buf;
	    }
	    else
#endif
		aligned_tupval = tup_ptr;
	    sts = IICPvcValCheck( IIlbqcb, aligned_tupval, cmap, copy_blk );
	    if (sts != OK)
		return (FAIL);
	}

	if (absttype == DB_NCHR_TYPE || absttype == DB_NVCHR_TYPE)
	{
	    /* Binary unicode, need to normalize the representation.
	    ** Since this is binary copy, tup and row types/lengths are
	    ** necessarily the same.  Do the normalization into a work
	    ** buffer in case the value grows larger than the column
	    ** has room for.  (Which should throw a truncation warning,
	    ** but doesn't at present...)
	    */

	    DB_DATA_VALUE  dst_dv;
	    DB_DATA_VALUE  src_dv;
	    i4 collen;

	    collen = cmap->cp_tuplen;
	    if (cmap->cp_tuptype < 0)
		--collen;		/* Don't worry about null indicator */
	    if (cmap->cp_normbuf == NULL)
		cmap->cp_normbuf = MEreqmem(0, collen * 4, 0, NULL);

	    if (cmap->cp_srcbuf == NULL)
		cmap->cp_srcbuf = MEreqmem(0, collen, 0, NULL);

	    src_dv.db_collID = dst_dv.db_collID = 0;
	    src_dv.db_datatype = dst_dv.db_datatype = absttype;
	    src_dv.db_length = dst_dv.db_length = collen;
	    src_dv.db_prec = dst_dv.db_prec = cmap->cp_cvprec;
	    src_dv.db_data = cmap->cp_srcbuf;
	    dst_dv.db_data = cmap->cp_normbuf;

	    MEcopy (tup_ptr, collen, cmap->cp_srcbuf);

	    if ((status = 
		adu_unorm(IIlbqcb->ii_lq_adf, &src_dv, &dst_dv)) != E_DB_OK)
	    {
		char err_buf[15];
		CVla(copy_blk->cp_rowcount + 1, err_buf);
		IIlocerr(GE_LOGICAL_ERROR, E_CO003A_ROW_FORMAT_ERR,
				II_ERR, 2, err_buf, cmap->cp_name);
		copy_blk->cp_warnings++;
		return (FAIL);
	    }

            MEcopy((PTR)dst_dv.db_data, collen, (PTR)tup_ptr);
	}

	tup_ptr += cmap->cp_tuplen;
    } /* column validation loop */

    return (OK);

} /* IIcpfrom_binary */

/*
** Name: IIcpfrom_formatted -- Read file row, transform to tuple format
**
** Description:
**
**	This routine reads columns out of the COPY FROM file and
**	translates each column into the tuple format.  I.e. this is
**	the COPY (list) FROM style of copy.  Every column being copied
**	goes through a conversion, even if it's the null conversion
**	such as int to int).  This is not quite as horrid as it sounds,
**	because 99.99% of all formatted COPY FROM's will use character
**	row types, requiring either coercions or character copying.
**
**	One copy group is read, which is either a bunch of non-LOB
**	columns, or one LOB column (with a non-LOB row value;  row-LOB
**	domains are handled elsewhere, since the file value is then of
**	indefinite length).  The non-LOB columns may be "real" or dummies
**	or some mix of both.
**
**	The general idea is to work through the column list.  For each
**	column, we locate the start and end of the column.  For fixed
**	length columns, this is quite simple:  the start is "current",
**	and the length is cp_fieldlen plus cp_delim_len.  Well, yes,
**	there are a couple special cases involving TEXT or counted
**	(varchar etc) row-types;  see inline.  Fixed length domains
**	may also be nullable, with a standard null indicator.
**
**	For variable length columns, which are necessarily dummy or
**	character-type columns, finding the start and end is a
**	bit trickier.  Basically we scan for a delimiter, being careful
**	to not fall off the end of the buffer, but there are a few
**	special cases:  some rowtypes (e.g. c0) allow multiple default
**	delimiters, namely space tab or newline;  and then there's the
**	CSV type, where if the first non-whitespace seen is a double
**	quote, we scan to the closing double quote (dealing with quote
**	doubling and possibly \-escaping within the quotes).  Variable
**	length VARCHAR (and vbyte and utf8) columns are even weirder,
**	since they start with a length count, but after skipping past
**	that length, we might still scan for a delimiter.
**
**	Once the start and end of the row value is found, we may need to
**	align it or turn it into a DB_TEXT_STRING.  If the row format is
**	anything but char or c, the value has some sort of binary structure,
**	and the conversion routines will expect it to be aligned; if
**	alignment is needed, the row value is copied to an aligned temp.
**	If the row value is one of the DB_TEXT_STRING-like types, we
**	have to make it look like a DB_TEXT_STRING by stuffing the length
**	into the row buffer in front of the string. Finally, for
**	varchar/utf8/vbyte, there's the leading 5-digit byte count
**	to translate and deal with.
**
**	After the row value is finally set up, it's checked for null
**	(if WITH NULL specified), and the row value is converted into
**	the tuple buffer, or another aligned temp.  Finally, if there is
**	normalization or value checking to do, it's done against the
**	converted value, and the aligned temp copied into the tuple
**	buffer.
**
**	Running the loop a column at a time allows the row buffering to
**	be fairly simple.  We can set a maximum size for a single value,
**	say DB_MAXSTRING plus a bit for slop and delimiters.  The
**	read buffer only needs to have enough extra space for that maximum
**	size to be moved "out of the way" when reading more data.
**	Any variable-width row value that is larger than can be buffered
**	can be declared to be in error.
**
**	There is one big problem with column-at-a-time buffering, though,
**	which is error logging.  The LOG= option says that rows with
**	format conversion errors are supposed to be dumped off to the
**	log file.  If we've read beyond the start of the row, that is
**	hard to do.  I had thought about seeking the input file back to
**	the start of the row, but that appears to not be feasible on
**	VMS (as I read it, you can't randomly seek VMS text files.)
**	So, if logging, the input is copied to a separate log-line
**	buffer.  If the line is too long, too bad.  Log copying is done
**	in chunks (if possible) to minimize the impact on performance.
**
**	There is funky CR/LF handling in various places.  These are *not*
**	inside NT_GENERIC conditionals, on purpose.  It's fairly common
**	for a non-Windows load server to get data files written on Windows;
**	mandating that such files be de-CRLF'ed manually would just cause
**	problems.
**
** Inputs:
**	thr_cb			LIBQ thread control block
**	copy_blk		Overall copy control block pointer
**	cgroup			Copy group info
**	tuple			Where to put the output [partial] tuple
**
** Outputs:
**	Returns OK, ENDFILE, or failure status.
**
** History:
**	4-Dec-2009 (kschendel) SIR 122947
**	    Combine and reorganize original IIcpgetrow and IIcptupformat,
**	    to (greatly) eliminate character-shuffling overhead, and add
**	    CSV format handling.  Allow a non-LOB row-value to be fed to
**	    a large object.
**	28-Dec-2009 (kschendel) SIR 122974
**	    Fix typos if BYTE_ALIGN, caused compile failures.
**	3-Jan-2010 (kschendel) SIR 122974
**	    One more dum-dum on big-endian byte-align, have to i2assign
**	    from an i2, not an i4.
**	25-Jan-2010 (kschendel) SIR 122974
**	    Windows /r/n related fixes:
**	    HOQA uses the "nl=d1" idiom to skip newlines.  Unfortunately
**	    on windows, newlines can be 2 bytes, \r\n.  Fix.
**	    CSV/SSV wasn't dealing properly with \r\n after a quoted value.
**	2-Mar-2010 (kschendel) SIR 122974
**	    The original COPY code did \-escaping for dummy format (d0)
**	    as well as c0, fix here.
*/

static STATUS
IIcpfrom_formatted(II_THR_CB *thr_cb,
    register II_CP_STRUCT *copy_blk, II_CP_GROUP *cgroup, PTR tuple)
{

    bool		dquote;		/* CSV style double-quoted value */
    bool		noconv;		/* Looking for end of row, no convert */
    bool		isescape;	/* Escape or doubled-quote seen */
    bool		isnull;		/* Nullable row-value is null */
    DB_STATUS		status;
    DB_TEXT_STRING	*txt_ptr;	/* For DB_TEXT_STRING-izing values */
    register II_CP_MAP	*cmap;
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    i4			absrtype;
    register i4		ch;		/* A char including EOF/error */
    i4			delim_offset;	/* Offset from value start */
    i4			i;
    i4			vallen;
    i4			vch_len;
    u_char		*dataend;	/* End-of-data (+1) in buffer */
    u_char		*rowptr;	/* Buffer pointer to next char */
    u_char		*cp;		/* Utility */
    u_char		*val_start;
    PTR			tup_ptr;
    STATUS		sts;

    copy_blk->cp_status &=  ~CPY_ERROR;

    /*
    ** Initialize 'tuple' to be an "empty" tuple.
    */
    if (cgroup->cpg_tup_length > 0)
	MEcopy(copy_blk->cp_zerotup + cgroup->cpg_tup_offset,
		cgroup->cpg_tup_length, tuple);

    /* If no cmaps, this is a row fragment that is entirely defaulted
    ** (ie between / before / after LOB's), just return since there's no
    ** file data corresponding to the group.  We filled in the defaults.
    */
    if (cgroup->cpg_first_map == NULL)
	return (OK);

    /* Big loop over group's columns */
    noconv = FALSE;
    rowptr = copy_blk->cp_rowptr;
    dataend = copy_blk->cp_dataend;
    copy_blk->cp_valstart = rowptr;
    cmap = &copy_blk->cp_copy_map[0];	/* In case of immediate fail */
    /* Caller sets refill control to NORMAL if first group, EOF-FAILS if not */
    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
    if (ch == COPY_FAIL)
	goto readerr_out;
    else if (ch == COPY_EOF)
	return (ENDFILE);	/* This is normal EOF */
    CP_UNGETC_MACRO(rowptr);
    for (cmap = cgroup->cpg_first_map; cmap <= cgroup->cpg_last_map; ++cmap)
    {
	copy_blk->cp_refill_control = COPY_REFILL_EOF_FAILS;

	absrtype = cmap->cp_rowtype;
	if (absrtype != CPY_DUMMY_TYPE)
	    absrtype = abs(absrtype);
	copy_blk->cp_valstart = rowptr;
	tup_ptr = tuple + cmap->cp_groupoffset;
	isnull = FALSE;

	if (cmap->cp_rowlen != ADE_LEN_UNKNOWN)
	{
	    /* This is the (relatively) easy one, fixed size field. */
	    rowptr += cmap->cp_fieldlen;
	    if (rowptr > dataend)
	    {
		-- rowptr;	/* Conform to CP_GETC_MACRO usage */
		ch = cp_refill(&dataend, &rowptr, copy_blk);
		if (ch == COPY_FAIL)
		    goto readerr_out;
	    }
	    if (absrtype == CPY_DUMMY_TYPE)
	    {
		/* This bit is unfortunate, but necessary since a common
		** eat-newline idiom in HOQA is "nl = d1".  We can't rely
		** on the dummy domain-name, so eat crlf's in fixed width
		** dummy domains all the time.  (When running under windows,
		** the TEXT filetype translation will fix things;  but all
		** too depressingly often, Windows-created data files
		** end up on unix machines...)
		*/
		cp = copy_blk->cp_valstart;
		while (cp < rowptr)
		{
		    if (*cp++ == '\r')
		    {
			/* Eat one more char if it's \r\n.  Can't check
			** for the \n until we ensure it's there.
			*/
			(void) CP_GETC_MACRO(dataend, rowptr, copy_blk);
			if (*cp != '\n')
			{
			    CP_UNGETC_MACRO(rowptr);
			}
		    }
		}
	    }
	    else if (cmap->cp_rowtype < 0)
	    {
		/* Fixed length inputs can have null indicators. */
		isnull = (*(rowptr-1) != 0);
	    }

	    /* It's bogus to have delimiters with fixed fields, but it's
	    ** allowed.  Ugh!  If the delimiter was newline, do the CR/LF
	    ** foolishness here, in case our file was created on windows.
	    */
	    if (cmap->cp_isdelim)
	    {
		ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
		if (ch == COPY_FAIL)
		    goto readerr_out;
		if (cmap->cp_delim[0] == '\n' && ch == '\r')
		{
		    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
		    if (ch != '\n')
		    {
			/* Just leave char for next column */
			CP_UNGETC_MACRO(rowptr);
		    }
		}
		else if (cmap->cp_delim_len > 1)
		{
		    /* Silly multibyte (ie 2 byte) delimiter */
		    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
		    if (ch == COPY_FAIL)
			goto readerr_out;
		}
	    }
	    CP_LOGTO_MACRO(rowptr,copy_blk);
	    vallen = cmap->cp_fieldlen;

	    /* Fixed length TEXT and the fixed-length counted-length
	    ** string row types have special cases.
	    */
	    if (absrtype == DB_TXT_TYPE && !isnull)
	    {
		/* Fixed length TEXT value stops at the first null. */
		vallen = STnlength(vallen, copy_blk->cp_valstart);
	    }
	    else if (cmap->cp_counted && !isnull)
	    {
		/* For counted-length row types such as VARCHAR, go ahead and
		** interpret the count here, for use below.  (Variable width
		** counted-length columns do the same but just a little
		** differently due to buffer handling.)
		**
		** Don't interpret the count for a null input.
		*/
		val_start = copy_blk->cp_valstart;
		cp = val_start + CPY_VCHLEN;
		ch = *cp;
		*cp = '\0';
		sts = CVan(val_start, &vch_len);
		*cp = ch;
		/* Unlike the variable length case, here we test against
		/* the declared row length.  (If the count in the data
		** is larger, which do you believe?  and where do you
		** start the next field?)
		*/
		if (sts != OK || vch_len < 0 || vch_len > (cmap->cp_fieldlen - CPY_VCHLEN))
		{
		    char err_buf[15];
		    CVla(copy_blk->cp_rowcount + 1, err_buf);
		    IIlocerr(GE_DATA_EXCEPTION, E_CO000B_LEN_SPEC_ERR, 
				  II_ERR, 2, cmap->cp_name, err_buf);
		    copy_blk->cp_warnings++;
		    noconv = TRUE;
		    continue;		/* Just scan for end now */
		}
		vallen = vch_len;
		copy_blk->cp_valstart += CPY_VCHLEN;
	    }
	}
	else
	{
	    /* Variable width field, which will ALMOST always have a
	    ** delimiter.  (The exception being counted(0), eg varchar(0),
	    ** which relies entirely on the count and has no default
	    ** delimiter.)
	    ** Note that UTF8 can get here, but not NCHAR/NVCHR row types;
	    ** the latter are fixed length only.
	    */

	    /* The row value starts at rowptr.  If it's CSV with a
	    ** quoted value, the row value will start after the first
	    ** double-quote, but we'll figure that one out later.
	    */

	    /* First order of business is to dispose of "counted"
	    ** columns, which have a 5-byte length count immediately
	    ** at the start of the value.  We can't start looking for
	    ** delimiters until that length is passed.
	    */
	    if (cmap->cp_counted)
	    {
		rowptr += CPY_VCHLEN;
		if (rowptr > dataend)
		{
		    -- rowptr;	/* Conform to CP_GETC_MACRO usage */
		    ch = cp_refill(&dataend, &rowptr, copy_blk);
		    if (ch == COPY_FAIL)
			goto readerr_out;
		}
		CP_LOGTO_MACRO(rowptr, copy_blk);  /* just the length */
		/* This looks bad, because rowptr might be beyond the end
		** of the buffer!  which is precisely why we added an end-
		** guard when the buffer was allocated.  We don't want to
		** do a getc here because it might be 00000<eof> at the
		** very end...and that's legal.
		*/
		ch = *rowptr;
		*rowptr = '\0';
		sts = CVan(copy_blk->cp_valstart, &vch_len);
		*rowptr = ch;
		/* Note we test against MAXSTRING, not tuplen;  string
		** truncation is allowed.
		*/
		if (sts != OK || vch_len < 0 || vch_len > DB_MAXSTRING)
		{
		    char err_buf[15];
		    CVla(copy_blk->cp_rowcount + 1, err_buf);
		    IIlocerr(GE_DATA_EXCEPTION, E_CO000B_LEN_SPEC_ERR, 
				  II_ERR, 2, cmap->cp_name, err_buf);
		    copy_blk->cp_warnings++;
		    noconv = TRUE;
		    continue;		/* Just scan to end now */
		}
		copy_blk->cp_valstart += CPY_VCHLEN;
		rowptr += vch_len;
		if (rowptr > dataend)
		{
		    -- rowptr;	/* Conform to CP_GETC_MACRO usage */
		    ch = cp_refill(&dataend, &rowptr, copy_blk);
		    if (ch == COPY_FAIL)
			goto readerr_out;
		}
		/* counted(0) input doesn't have any default delimiter,
		** so skip all the delimiter scan poop now and jump
		** right to the conversion.  (the goto is unfortunate but
		** the indents get out of control otherwise, and this is
		** an exceptional case.)
		*/
		if (! cmap->cp_isdelim)
		{
		    CP_LOGTO_MACRO(rowptr, copy_blk);
		    vallen = vch_len;
		    goto got_value;
		}
	    } /* counted-string input */
	    /* If CSV style, skip to first non-blank/non-tab char to see
	    ** if we have a double quoted value.
	    */
	    dquote = FALSE;
	    if (cmap->cp_csv)
	    {
		if (cmap->cp_delim[0] == '\n')
		    copy_blk->cp_refill_control = COPY_REFILL_EOF_NL;
		do
		{
		    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
		} while (ch == ' ' || ch == '\t');
		if (ch == COPY_FAIL)
		    goto readerr_out;
		else if (ch == '"')
		{
		    /* CSV value is the thing inside the "string",
		    ** so reset "value start" -- buffering can toss the
		    ** whitespace and leading dquote if necessary.
		    */
		    dquote = TRUE;
		    CP_LOGTO_MACRO(rowptr, copy_blk);
		    copy_blk->cp_valstart = rowptr;
		    copy_blk->cp_refill_control = COPY_REFILL_EOF_FAILS;
		}
		else
		{
		    CP_UNGETC_MACRO(rowptr);
		}
	    }
	    /* Ok, scan for a delimiter.  If the value ends up not fitting
	    ** into the row buffer, declare an error, even if most of it is
	    ** whitespace (or a char field that could be truncated);
	    ** the row buffer is large enough that such situations
	    ** constitute abuse, and deserve an error.  :-)
	    **
	    ** The delimiter scan is complicated by the possibility of
	    ** escaped characters and/or doubled quotes.  In either case,
	    ** the escape or doubling-quote is to be squeezed out of the
	    ** column value, which implies copying chars back into the
	    ** row buffer (*) at a prior position.  Since this is likely
	    ** to rare, as an optimization, a separate loop is used once
	    ** squeezing becomes necessary.
	    ** Note that a rowtype of 'c0\' is not meaningful, and
	    ** one hopes that it's disallowed somewhere along the chain...
	    **
	    ** (*) or somewhere else, but we might as well just use the
	    ** row buffer itself.
	    **
	    ** Note that the CP_GETC_MACRO calls can move the value being
	    ** built up, so we can't save a pointer to the potential
	    ** delimiter -- have to save an offset.
	    **
	    ** Also note that newlines that aren't delimiters are just data.
	    ** One might imagine that it would make sense to error/warn on
	    ** an unexpected, un-escaped, unquoted newline, since it probably
	    ** means bad data.  However, copy has never worked that way,
	    ** and it seems imprudent to change it now.
	    */
	    isescape = FALSE;
	    for (;;)
	    {
		delim_offset = rowptr - copy_blk->cp_valstart; /* maybe */
		ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
		if (ch == COPY_FAIL)
		    goto readerr_out;
		cp = copy_blk->cp_valstart + delim_offset;
		if ( (i = CMbytecnt(cp)) > 1)
		{
		    /* The only way we care about multi-byte characters
		    ** is if we have a multi-byte delimiter;  otherwise,
		    ** take care to move past the entire multi-byte input
		    ** character to keep in sync.
		    */
		    --i;	/* Already got the first */
		    do
		    {
			ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			if (ch == COPY_FAIL)
			    goto readerr_out;
		    } while (--i > 0);
		    cp = copy_blk->cp_valstart + delim_offset;
		    if (cmap->cp_delim_len > 1 && CMcmpcase(cp, cmap->cp_delim) == 0)
			break;		/* Got the delimiter */
		    /* else */
		    continue;
		}

		if (ch == CPY_ESCAPE
		  && (absrtype == DB_CHR_TYPE || absrtype == CPY_DUMMY_TYPE))
		{
		    i = copy_blk->cp_refill_control;
		    copy_blk->cp_refill_control = COPY_REFILL_EOF_FAILS;
		    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
		    if (ch == COPY_FAIL)
			goto readerr_out;
		    copy_blk->cp_refill_control = i;
		    isescape = TRUE;
		    break;
		}
		else if (dquote)
		{
		    /* Must be CSV style... */
		    if (ch != '"')
		    {
			/* Just another character, unless it's \r...
			** Normalize CRLF inside of quoted strings to
			** be just \n.  (If we're on windows, writing
			** the \n with non-binary COPY INTO will produce
			** \r\n in the output file, and it all works.)
			*/
			if (ch == '\r')
			{
			    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			    if (ch == COPY_FAIL)
				goto readerr_out;
			    if (ch == '\n')
			    {
				isescape = TRUE;
				break;		/* requires squishing */
			    }
			    CP_UNGETC_MACRO(rowptr);
			    ch = '\r';
			}
			continue;	/* Plow thru "quoted string" */
		    }
		    /* Might be the close quote, might be doubling. */
		    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
		    if (ch == COPY_FAIL)
			goto readerr_out;
		    if (ch == '"')
			isescape = TRUE;
		    else
		    {
			/* Was really the close quote, buzz past optional
			** whitespace to find the delimiter.  If non-white
			** non-delimiter found, it's an error.
			*/
			if (cmap->cp_delim[0] == '\n')
			    copy_blk->cp_refill_control = COPY_REFILL_EOF_NL;
			while (ch == ' ' || ch == '\t' || ch == '\r')
			{
			    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			    if (ch == COPY_FAIL)
				goto readerr_out;
			}
			if (ch != cmap->cp_delim[0])
			{
			    char err_buf[2]; i4 rc;
			    err_buf[0] = ch;
			    err_buf[1] = '\0';
			    rc = copy_blk->cp_rowcount+1;
			    IIlocerr(GE_DATA_EXCEPTION, E_CO004B_CSV_NONWHITE, 
				II_ERR, 3, &rc, err_buf, cmap->cp_name);
			    copy_blk->cp_warnings++;
			    noconv = TRUE;
			    break;
			}
			/* Note that delim_offset still points to the value's
			** "delimiter", ie the closing double quote.
			*/
		    }
		    break;	/* whether done or quote-doubling */
		}
		else if (cmap->cp_isdelim > 0
		  && (ch == cmap->cp_delim[0]
		      || (ch == '\r' && cmap->cp_delim[0] == '\n')) )
		{
		    /* Looks like we have a delimiter, although if we have
		    ** a CR, make sure a LF follows if newline was the
		    ** requested delimiter.  Bare CR doesn't count as \n.
		    */
		    if (ch == '\r' && cmap->cp_delim[0] == '\n')
		    {
			ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			if (ch == COPY_FAIL)
			    goto readerr_out;
			if (ch != '\n')
			{
			    /* Rescan the not-LF, keep looking */
			    CP_UNGETC_MACRO(rowptr);
			    continue;
			}
		    }
		    break;	/* Got the delimiter */
		}
		else if (cmap->cp_isdelim < 0)
		{
		    /* Char(0)/c0/text(0) input with no delimiter specified,
		    ** the default is any of comma, tab, or newline.
		    */
		    if (ch == ',' || ch == '\t' || ch == '\n')
			break;
		    /* Same sort of CRLF crap as seen above... */
		    if (ch == '\r')
		    {
			ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			if (ch == COPY_FAIL)
			    goto readerr_out;
			if (ch != '\n')
			{
			    /* Rescan the not-LF, keep looking */
			    CP_UNGETC_MACRO(rowptr);
			    continue;
			}
			break;	/* Got the delimiter */
		    }
		    /* else not a delimiter yet */
		}
	    } /* delim scan for */
	    CP_LOGTO_MACRO(rowptr, copy_blk);

	    /* If the for exited with isescape set, we have a situation
	    ** that requires squeezing characters out of the value string:
	    ** - C format and a \-escaped character seen
	    ** - quote doubling inside CSV quoted input
	    ** - CRLF seen inside CSV quoted input (normalize to just LF)
	    ** Since we now have to copy every character back into the
	    ** buffer at an earlier offset, and logging (if needed) gets
	    ** harder, I've chosen to run this loop as a separate loop.
	    ** obviously it makes the code larger, but (I think) easier
	    ** to understand, and it makes the normal no-squeeze loop faster.
	    */
	    if (isescape)
	    {
		i4 offset = 2;

		/* fetched chars get put to *(rowptr-offset).
		** (offset starts at 2 because rowptr is post-incremented.)
		** Here at the top, we've read the char after the one
		** prompting the escape (the \-escaped char, or the second
		** dquote, or the LF after CR.)  Store the char (in ch)
		** over top of the previous char, and continue scanning
		** for a delimiter and squishing.
		*/
		*(rowptr-offset) = ch;
		for (;;)
		{
		    delim_offset = rowptr - copy_blk->cp_valstart;
		    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
		    if (ch == COPY_FAIL)
			goto readerr_out;
		    CP_LOGCH_MACRO(ch, copy_blk);
		    cp = copy_blk->cp_valstart + delim_offset;
		    if ( (i = CMbytecnt(cp)) > 1)
		    {
			/* The only way we care about multi-byte characters
			** is if we have a multi-byte delimiter;  otherwise,
			** take care to move past the entire multi-byte input
			** character to keep in sync.
			*/
			--i;	/* Already got the first */
			do
			{
			    *(rowptr-offset) = ch;
			    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			    if (ch == COPY_FAIL)
				goto readerr_out;
			    CP_LOGCH_MACRO(ch, copy_blk);
			} while (--i > 0);
			cp = copy_blk->cp_valstart + delim_offset;
			if (cmap->cp_delim_len > 1 && CMcmpcase(cp, cmap->cp_delim) == 0)
			    break;		/* Got the delimiter */
			/* else */
			*(rowptr-offset) = ch;
			continue;
		    }

		    if (ch == CPY_ESCAPE
		      && (absrtype == DB_CHR_TYPE || absrtype == CPY_DUMMY_TYPE))
		    {
			i = copy_blk->cp_refill_control;
			copy_blk->cp_refill_control = COPY_REFILL_EOF_FAILS;
			ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			if (ch == COPY_FAIL)
			    goto readerr_out;
			copy_blk->cp_refill_control = i;
			CP_LOGCH_MACRO(ch, copy_blk);
			++ offset;
			*(rowptr-offset) = ch;
			continue;
		    }
		    else if (dquote)
		    {
			/* Must be CSV style... */
			if (ch != '"')
			{
			    /* Just another character, unless it's \r...
			    ** Normalize CRLF inside of quoted strings to
			    ** be just \n.  (If we're on windows, writing
			    ** the \n with non-binary COPY INTO will produce
			    ** \r\n in the output file, and it all works.)
			    */
			    if (ch == '\r')
			    {
				ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
				if (ch == COPY_FAIL)
				    goto readerr_out;
				if (ch == '\n')
				{
				    CP_LOGCH_MACRO(ch, copy_blk);
				    ++ offset;
				}
				else
				{
				    CP_UNGETC_MACRO(rowptr);
				    ch = '\r';
				}
			    }
			    *(rowptr-offset) = ch;
			    continue;	/* Plow thru "quoted string" */
			}
			/* Might be the close quote, might be doubling. */
			ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			if (ch == COPY_FAIL)
			    goto readerr_out;
			CP_LOGCH_MACRO(ch, copy_blk);
			if (ch == '"')
			{
			    ++ offset;
			    *(rowptr-offset) = ch;
			    continue;
			}
			else
			{
			    /* Was really the close quote, buzz past optional
			    ** whitespace to find the delimiter.  If non-white
			    ** non-delimiter found, it's an error.
			    ** (just eat these, no need to store)
			    */
			    if (cmap->cp_delim[0] == '\n')
				copy_blk->cp_refill_control = COPY_REFILL_EOF_NL;
			    while (ch == ' ' || ch == '\t' || ch == '\r')
			    {
				ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
				if (ch == COPY_FAIL)
				    goto readerr_out;
				CP_LOGCH_MACRO(ch, copy_blk);
			    }
			    if (ch != cmap->cp_delim[0])
			    {
				char err_buf[2]; i4 rc;
				err_buf[0] = ch;
				err_buf[1] = '\0';
				rc = copy_blk->cp_rowcount+1;
				IIlocerr(GE_DATA_EXCEPTION, E_CO004B_CSV_NONWHITE, 
					II_ERR, 3, &rc, err_buf, cmap->cp_name);
				copy_blk->cp_warnings++;
				noconv = TRUE;
				break;
			    }
			    /* Note that delim_offset still points to the value
			    ** ender, ie the closing double quote.
			    */
			}
			break;	/* whether done or quote-doubling */
		    }
		    else if (cmap->cp_isdelim > 0
		      && (ch == cmap->cp_delim[0]
			  || (ch == '\r' && cmap->cp_delim[0] == '\n')) )
		    {
			/* Looks like we have a delimiter, although if we have
			** a CR, make sure a LF follows if newline was the
			** requested delimiter.  Bare CR doesn't count as \n.
			*/
			if (ch == '\r' && cmap->cp_delim[0] == '\n')
			{
			    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			    if (ch == COPY_FAIL)
				goto readerr_out;
			    if (ch != '\n')
			    {
				/* Rescan the not-LF, keep looking */
				CP_UNGETC_MACRO(rowptr);
				*(rowptr-offset) = '\r';
				continue;
			    }
			    CP_LOGCH_MACRO(ch, copy_blk);
			}
			break;	/* Got the delimiter */
		    }
		    else if (cmap->cp_isdelim < 0)
		    {
			/* Char(0)/c0/text(0) input with no delimiter specified,
			** the default is any of comma, tab, or newline.
			*/
			if (ch == ',' || ch == '\t' || ch == '\n')
			{
			    CP_LOGCH_MACRO(ch, copy_blk);
			    break;
			}
			/* Same sort of CRLF crap as seen above... */
			if (ch == '\r')
			{
			    ch = CP_GETC_MACRO(dataend, rowptr, copy_blk);
			    if (ch == COPY_FAIL)
				goto readerr_out;
			    if (ch != '\n')
			    {
				/* Rescan the not-LF, keep looking */
				CP_UNGETC_MACRO(rowptr);
				*(rowptr-offset) = '\r';
				continue;
			    }
			    CP_LOGCH_MACRO(ch, copy_blk);
			    break;	/* Got the delimiter */
			}
		    }
		    *(rowptr-offset) = ch;
		} /* delim scan for */
		/* Adjust delim_offset back to the squished position,
		** rather than the scan position.  offset is for the
		** advanced rowptr, delim_offset points at a delim rather
		** than past it, so offset-1 is the right adjustment.
		*/
		delim_offset -= (offset-1);
	    } /* if isescape */

	    /* Set the length of the value not counting delimiters. */
	    if (cmap->cp_counted)
		vallen = vch_len;
	    else
		vallen = delim_offset;
	} /* if variable length row value */

	/* Row value starts at copy_blk->cp_valstart, and extends for
	** vallen bytes.
	*/

	/* varchar(0) (no delimiter scan) jumps here */
got_value:

	/* Actual conversion to tuple format here.  If we're just scanning
	** for the end of the input line for LOG= and a conversion error,
	** skip around this bit.
	*/
	if (cgroup->cpg_is_tup_lob && ! noconv)
	{
	    /* Non-LONG row value -> LONG column.
	    ** Post the actual value length somewhere convenient, and do
	    ** blobby things to send it.
	    */
	    cgroup->cpg_row_length = vallen;
	    sts = IIcpfrom_lob(thr_cb, copy_blk, cgroup);
	    if (sts != OK)
	    {
		noconv = TRUE;
		break;			/* No other columns in LOB group */
	    }
	}
	else if (absrtype != CPY_DUMMY_TYPE && ! noconv)
	{
	    val_start = copy_blk->cp_valstart;
	    if (isnull || vallen == 0 || IIcp_isnull(cmap, val_start, vallen))
	    {
		/* If really NULL, make sure the column is nullable.
		** Note that an empty field can be just a default,
		** not necessarily null.  (Which I don't approve of;
		** but that's how COPY has always worked.)
		*/
		if ((isnull || vallen > 0) && cmap->cp_tuptype > 0)
		{
		    /* Attempt to put NULL data into a not nullable field */
		    IIlocerr(GE_LOGICAL_ERROR, E_CO003E_NOT_NULLABLE,
			    II_ERR, 1, cmap->cp_name);
		    copy_blk->cp_warnings++;
		    noconv = TRUE;
		    continue;		/* Just skip to end now */
		}
		/* Just fall thru and take the preinitialized "empty" value,			** which will of course be NULL for nullables.
		*/
	    }
	    else
	    {
		i4 absttype = abs(cmap->cp_tuptype);
		PTR aligned_tupval;
#ifdef BYTE_ALIGN
		ALIGN_RESTRICT rowval_buf[(DB_MAXSTRING+DB_CNTSIZE+sizeof(ALIGN_RESTRICT)) / sizeof(ALIGN_RESTRICT)];
		ALIGN_RESTRICT tupval_buf[(DB_MAXSTRING+DB_CNTSIZE+sizeof(ALIGN_RESTRICT)) / sizeof(ALIGN_RESTRICT)];
#endif

		/* Run the conversion;  but first, there are yet more
		** preparatory details to handle.
		** If the tuple-type is nullable, clear the null indicator.
		** If the row-type is one of the DB_TEXT_STRING-like
		** types, the ADF coercions are going to expect a
		** DB_TEXT_STRING.  This can be achieved by stuffing
		** the db-text-count in front of the string in the buffer;
		** the buffer is allocated with guard space so that this
		** always works, even if the value starts at the very
		** beginning of the buffer.
		** Alignment issues:  if the platform requires
		** alignment, and either the row value or the result
		** tuple address is unaligned, that value has to be
		** placed into an aligned buffer.
		*/
		if (cmap->cp_tuptype < 0)
		    *(tup_ptr + cmap->cp_tuplen - 1) = 0;

		if (cmap->cp_trunc_len > 0
		  && vallen > cmap->cp_trunc_len)
		    ++copy_blk->cp_truncated;

		if (cmap->cp_counted	/* varchar, utf8, varbyte */
		  || absrtype == DB_TXT_TYPE)
		{
		    i2 cnt = vallen;
		    txt_ptr = (DB_TEXT_STRING *) (val_start - DB_CNTSIZE);
		    I2ASSIGN_MACRO(cnt, txt_ptr->db_t_count);
		    val_start = (u_char *) txt_ptr;
		    vallen += DB_CNTSIZE;
		}
		aligned_tupval = tup_ptr;
#ifdef BYTE_ALIGN
		if (! (absttype == DB_CHR_TYPE
			|| absttype == DB_BYTE_TYPE
			|| absttype == DB_CHA_TYPE
			|| ((absttype == DB_VCH_TYPE || absttype == DB_TXT_TYPE
			     || absttype == DB_VBYTE_TYPE)
			    && tup_ptr == (PTR) ME_ALIGN_MACRO(tup_ptr, sizeof(i2)))
			|| tup_ptr == (PTR) ME_ALIGN_MACRO(tup_ptr, DB_ALIGN_SZ) ) )
		{
		    aligned_tupval = (PTR) tupval_buf;
		    /* Set to empty in case no-conversion or something.
		    ** Don't copy null indicator if any. (leave clear)
		    */
		    MEcopy(tup_ptr, cmap->cp_adf_fnblk.adf_r_dv.db_length, aligned_tupval);
		}
		if (! (absrtype == DB_CHR_TYPE
			|| absttype == DB_BYTE_TYPE
			|| absrtype == DB_CHA_TYPE
			|| ((absrtype == DB_VCH_TYPE || absrtype == DB_TXT_TYPE
			     || absrtype == DB_VBYTE_TYPE)
			    && val_start == (u_char *) ME_ALIGN_MACRO(val_start, sizeof(i2)))
			|| val_start == (u_char *) ME_ALIGN_MACRO(val_start, DB_ALIGN_SZ) ) )
		{
		    MEcopy(val_start, vallen, rowval_buf);
		    val_start = (u_char *) rowval_buf;
		    txt_ptr = (DB_TEXT_STRING *) rowval_buf; /* might be utf8 */
		}
#endif /* BYTE_ALIGN */
		/*
		**  Run through the UTF8 string and verify it only contains 
		**	legal UTF8 characters.  Loading in invalid characters 
		**	can cause random errors in later queries
		** I don't know why this isn't part of the coercions from
		** UTF8, but whatever ... [kschendel]
		*/
		if (absrtype == DB_UTF8_TYPE)
		{
		    u_char *next_char, *val_end;

		    /* UTF8 has been set up as a db-text-string above.
		    ** val-end is actually end+1 here.
		    */
		    next_char = &txt_ptr->db_t_text[0];
		    val_end = &txt_ptr->db_t_text[txt_ptr->db_t_count];

		    while (next_char < val_end)
		    {
			if (!(CM_isLegalUTF8Sequence(next_char, val_end)))
			{
			    char err_buf[15];
			    CVla(copy_blk->cp_rowcount + 1, err_buf);
			    IIlocerr(GE_DATA_EXCEPTION, E_CO0032_BAD_DATA,
				II_ERR, 2, cmap->cp_name, err_buf);
			    ++copy_blk->cp_warnings;
			    noconv = TRUE;
			    break;
			}

			next_char += CMUTF8cnt(next_char);
		    }
		    if (noconv)
			continue;	/* Give up if error */
		}
		/* Set up function-call block for coercion.
		** Use direct call if possible, avoids lots of unnecessary
		** adf-func double checking.
		*/
		cmap->cp_adf_fnblk.adf_r_dv.db_data = aligned_tupval;
		cmap->cp_adf_fnblk.adf_1_dv.db_length = vallen;
		cmap->cp_adf_fnblk.adf_1_dv.db_data = (PTR) val_start;
		if (cmap->cp_cvfunc != NULL)
		    status = (*cmap->cp_cvfunc)(IIlbqcb->ii_lq_adf,
			&cmap->cp_adf_fnblk.adf_1_dv,
			&cmap->cp_adf_fnblk.adf_r_dv);
		else
		    status = adf_func(IIlbqcb->ii_lq_adf, &cmap->cp_adf_fnblk);
		if (copy_blk->cp_status & CPY_ERROR)
		{
		    status = E_DB_ERROR;
		}
		if (status)
		{
		    char err_buf[15];
		    CVla(copy_blk->cp_rowcount + 1, err_buf);
		    IIlocerr(GE_LOGICAL_ERROR, E_CO0039_TUP_FORMAT_ERR, II_ERR, 
			     2, err_buf, cmap->cp_name);
		    copy_blk->cp_warnings++;

		    /* Report user error if applicable (Bug 21432) */
		    /* 
		    ** Display error message formatted by ADF, note:
		    ** these messages do not have timestamps (Bug 36059) 
		    */
		    if (IIlbqcb->ii_lq_adf->adf_errcb.ad_errclass == ADF_USER_ERROR)
		    {
			IIsdberr( thr_cb, 
			    &IIlbqcb->ii_lq_adf->adf_errcb.ad_sqlstate,
			    ss_2_ge(
			      IIlbqcb->ii_lq_adf->adf_errcb.ad_sqlstate.db_sqlstate,
			      IIlbqcb->ii_lq_adf->adf_errcb.ad_usererr),
			    IIlbqcb->ii_lq_adf->adf_errcb.ad_usererr,
			    0, 
			    IIlbqcb->ii_lq_adf->adf_errcb.ad_emsglen,
			    IIlbqcb->ii_lq_adf->adf_errcb.ad_errmsgp,
			    FALSE);
		    }
		    noconv = TRUE;	/* Stop converting this row */
		}

		/*
		** Check if ADF converted any unprintable chars to blank here.
		** Although it is a bit kludgy, we look directly into the ADF
		** session control block to get this information.  The clean way
		** is to call adx_chkwarn, but that has its drawbacks:
		**	   - it returns the first warning it finds - thus you
		**	     may have to call it several times to get the
		**	     warnings we are looking for.
		**	   - since it counts chars converted, rather than
		**	     strings which had chars in them converted, we
		**	     would have to do this loop of calls to adx_chkwarn
		**	     after each conversion in order to get the number
		**	     of domains affected.
		**	     this could be massively expensive.
		**	   - adx_chkwarn will format a message in the ADF
		**	     control block if there is a pointer to a message
		**	     buffer.  we don't want a message, but we don't
		**	     want to zero out the LIBQ adf control block's
		**	     error buffer either.
		*/
		if (IIlbqcb->ii_lq_adf->adf_warncb.ad_noprint_cnt)
		{
		    copy_blk->cp_blanked++;
		    IIlbqcb->ii_lq_adf->adf_warncb.ad_noprint_cnt = 0;
		}
		/*
		** Have ADF make sure that the data value just read in is a
		** legal Ingres data value.
		*/
		if (! noconv && cmap->cp_valchk)
		{
		    if (IICPvcValCheck( IIlbqcb, tup_ptr, cmap, copy_blk ) != OK)
			noconv = TRUE;	/* Stop converting */
		}
#ifdef BYTE_ALIGN
		if (! noconv && aligned_tupval != tup_ptr)
		{
		    MEcopy(aligned_tupval, cmap->cp_adf_fnblk.adf_r_dv.db_length, tup_ptr);
		}
#endif
	    }

	} /* if really converting */

    } /* giant column loop */

    /* Store buffer variables for next time */
    copy_blk->cp_rowptr = rowptr;
    copy_blk->cp_dataend = dataend;
    if (noconv)
	return (FAIL);
    return (OK);

/* Common exit point for error or eof while reading the input;  there's
** no sense in trying to scan more, so issue a message and return now
** instead of doing the "noconv" thing.
** Note that cmap should be set to the current column map data.
*/
readerr_out:
    copy_blk->cp_rowptr = rowptr;
    copy_blk->cp_dataend = dataend;
    {
	char err_buf[15];
	CVla(copy_blk->cp_rowcount + 1, err_buf);
	IIlocerr(GE_DATA_EXCEPTION, E_CO0040_END_OF_DATA, II_ERR, 2,
		cmap->cp_name, err_buf);
    }
    return FAIL;

} /* IIcpfrom_formatted */

/*
** Name: IIcpfrom_lob - Handle LOB group for COPY FROM
**
** Description:
**
**	This routine handles a LOB column / group for COPY FROM.
**	Since a LOB is of indefinite length up to 2 Gb, it's unreasonable
**	to hope to buffer it up, and it must be read and sent in segments.
**	To help cater to this, the LOB will be in a copy-group all by
**	itself, so that we don't need to worry about preceding or
**	following columns.
**
**	There are a couple variations, all dealt with here:
**
**	- it might be binary copy: there will be a cmap and it's conversion
**	  ID will be ADI_NOFI.  A binary LOB row-value looks like i2 lengths
**	  followed by N bytes;  the last length must be zero, followed by
**	  the null indicator (if nullable).  An actual NULL must be one zero
**	  length followed by a nonzero indicator.
**
**	- it might be a formatted copy with a long row-value.  This one
**	  works more or less like binary, except that a) the segment lengths
**	  are space-terminated ASCII numbers (eg '1234 '), and b) NULL is
**	  indicated by matching the WITH NULL value.
**
**	- it might be a formatted copy with a non-LONG row-value.  The
**	  ordinary FROM scanner has the row-value all set to go in cp_valstart
**	  and length cpg_row_length.  The value is a straight character
**	  string (no DB_TEXT_STRING glop), and will be treated as a single
**	  segment.  Coercion (if any) has to be done by hand, since the
**	  coercion FI selected by the dbms server is useless in this case.
**	  We also need to apply the WITH NULL value test if any.
**
**	- it might be omitted from the input file:  there will not be any
**	  cmap.  We send an empty LOB value.  (Unlike ordinary columns, we
**	  can't pre-set a NULL into the tuple buffer and just send it along
**	  with everything else, LOB's need special GCA handling.)
**
** Inputs:
**	thr_cb			LIBQ thread control block
**	copy_blk		Overall copy control block pointer
**	cgroup			Copy group info
**
** Outputs:
**	Returns OK, ENDFILE, FAIL status
**
** History:
**	14-Dec-2009 (kschendel) SIR 122974
**	    Unify LOB and non-LOB copy handling.
*/

static STATUS
IIcpfrom_lob(II_THR_CB *thr_cb, II_CP_STRUCT *cblk, II_CP_GROUP *cgroup)
{

    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;
    II_CP_MAP	*cmap = cgroup->cpg_first_map;
    LOBULKFROM	lob_info;
    bool	isnull;
    i2		nulblob = 0;
    i4		att_type;
    i4		ch;
    ROW_DESC	*rdesc;

    /* Some preliminary LIBQ setup */

    rdesc = &IIlbqcb->ii_lq_retdesc.ii_rd_desc;
    att_type = rdesc->RD_DBVS_MACRO(cgroup->cpg_first_attno).db_datatype;
    IIlbqcb->ii_lq_lodata.ii_lo_datatype = att_type;

    if (cmap == NULL)
    {
	/* We're stuffing an empty or null LOB, decide which */
	if (att_type < 0)
	    nulblob = DB_EMB_NULL;
	IILQldh_LoDataHandler(2, &nulblob, IICPsebSendEmptyBlob, NULL);
	return (OK);
    }

    /* There's a value.  It might be a LONG segmented value (row-lob TRUE),
    ** or an already scanned non-LONG non-segmented value (row-lob FALSE).
    ** The LONG style might be binary (convert FALSE) or character.
    */

    lob_info.cblk = cblk;
    lob_info.cgroup = cgroup;
    lob_info.libq_cb = IIlbqcb;
    isnull = FALSE;
    if (cgroup->cpg_is_row_lob)
    {
	/* Get the first segment length.  The refill-control is set
	** properly by the caller (in case the LOB is the first domain).
	*/
	lob_info.firstseglen = IIcpfrom_seglen(cblk, cmap, FALSE);
	cblk->cp_refill_control = COPY_REFILL_EOF_FAILS;
	if (lob_info.firstseglen == COPY_EOF)
	    return (ENDFILE);
	else if (lob_info.firstseglen == COPY_FAIL)
	    return (FAIL);

	/* Check this first segment for NULL-ness if nullable */
	if (cblk->cp_convert && cmap->cp_withnull
	  && cmap->cp_nullen == lob_info.firstseglen)
	{
	    /* Read the first segment, might be the NULL marker */
	    if (cmap->cp_nullen == 0)
	    {
		/* empty segment automatically matches empty string */
		isnull = TRUE;
	    }
	    else
	    {
		cblk->cp_valstart = cblk->cp_rowptr;
		cblk->cp_rowptr += lob_info.firstseglen;
		if (cblk->cp_rowptr > cblk->cp_dataend)
		{
		    --cblk->cp_rowptr;	/* usual protocol */
		    ch = cp_refill(&cblk->cp_dataend, &cblk->cp_rowptr, cblk);
		    if (ch == COPY_FAIL)
			return (FAIL);
		}
		isnull = (MEcmp((PTR) cblk->cp_valstart, cmap->cp_nuldata,
				lob_info.firstseglen) == 0);
	    }
	    if (isnull)
	    {
		if (att_type > 0)
		{
		    IIlocerr(GE_LOGICAL_ERROR, E_CO003E_NOT_NULLABLE,
			    II_ERR, 1, cmap->cp_name);
		    return (FAIL);
		}
		if (cmap->cp_nullen > 0)
		{
		    /* Read another segment, had better be zero length.
		    ** Ie don't allow '4 JUNK9 MORE JUNK0 ' when the null
		    ** marker is JUNK.
		    */
		    lob_info.firstseglen = IIcpfrom_seglen(cblk, cmap, FALSE);
		    if (lob_info.firstseglen != 0)
		    {
			IICPwemWriteErrorMsg(E_CO0047_FILE_READ_MSG,
			    cblk, cmap->cp_name,
			    ERget(S_CO0067_SEEK_LEN));
			return (FAIL);
		    }
		}
	    }
	    else
	    {
		/* Un-get the segment, let datahandler have it. */
		cblk->cp_rowptr -= lob_info.firstseglen;
	    }
	}
	else if (!cblk->cp_convert && att_type < 0
	  && lob_info.firstseglen == 0)
	{
	    /* Binary nullable LOB has a real null indicator byte at
	    ** the end of the LOB.  If the first segment is empty,
	    ** interpret the indicator.  (Note that nnn <data> 000 <isnull>
	    ** in binary is ERROR, we only allow 000 <isnull>.)
	    */
	    ch = CP_GETC_MACRO(cblk->cp_dataend, cblk->cp_rowptr, cblk);
	    if (ch == COPY_FAIL)
		return (FAIL);
	    else if (ch == 0)
	    {
		/* Put not-null indicator back for data handler */
		CP_UNGETC_MACRO(cblk->cp_rowptr);
	    }
	    else
		isnull = TRUE;
	}
    }
    else
    {
	/* Ordinary value already picked off by from-formatted.
	** The start of the value is in cp_valstart, and the length
	** of the value is in cpg_row_length.  Before feeding it off
	** to the datahandler, if there's a NULL marker defined,
	** check the value against the marker.
	**
	** Note that LOB's don't follow the empty-is-default convention
	** that non-LOB columns use.  I don't know whether that was a
	** conscious decision, or an accident of the old code.  For
	** what it's worth, I think that LOB's do it right...  [kschendel]
	*/
	if (cmap->cp_rowtype < 0)
	    isnull = (*(cblk->cp_valstart + cgroup->cpg_row_length - 1) != 0);
	else
	    isnull = IIcp_isnull(cmap, cblk->cp_valstart, cgroup->cpg_row_length);
	if (isnull && att_type > 0)
	{
	    IIlocerr(GE_LOGICAL_ERROR, E_CO003E_NOT_NULLABLE,
		    II_ERR, 1, cmap->cp_name);
	    return (FAIL);
	}
	lob_info.firstseglen = cgroup->cpg_row_length;
	/* FIXME coercion??? we do utf8 -> ucs2 in the data handler, is
	** there anything else that has to be done?  (I think not.)
	*/
    }

    /* There's something there, whether it be NULL or a real value
    ** or an empty segment or whatever.  The first segment's length
    ** is in firstseglen.  Fill out the rest of the data handler
    ** info and send out the rest of the LOB.
    */
    if (isnull)
	nulblob = DB_EMB_NULL;
    lob_info.failure = FALSE;
    IILQldh_LoDataHandler(2, &nulblob, IIcpfrom_lobHandler, (PTR) &lob_info);
    if (lob_info.failure)
	return (FAIL);

    /* One last silly thing to do if LONG row-value and converting,
    ** there might be a delimiter.  Read off and toss it.
    ** (For non-LONG row values, the from-formatted driver read the delim.)
    */
    if (cgroup->cpg_is_row_lob && cblk->cp_convert && cmap->cp_isdelim)
    {
	cblk->cp_valstart = cblk->cp_rowptr;
	cblk->cp_rowptr += cmap->cp_delim_len;
	if (cblk->cp_rowptr > cblk->cp_dataend)
	{
	    --cblk->cp_rowptr;
	    ch = cp_refill(&cblk->cp_dataend, &cblk->cp_rowptr, cblk);
	    if (ch == COPY_FAIL)
	    {
		IICPwemWriteErrorMsg(E_CO0047_FILE_READ_MSG,cblk,
			cmap->cp_name, ERget(S_CO006A_BAD_DELIM));
		return (FAIL);
	    }
	}
	ch = *cblk->cp_valstart;
	if (cmap->cp_delim[0] == '\n' && ch == '\r')
	{
	    /* Usual CRLF stuff in case file was a DOS file */
	    ch = CP_GETC_MACRO(cblk->cp_dataend, cblk->cp_rowptr, cblk);
	    if (ch != '\n')
	    {
		/* Just leave char for next column */
		CP_UNGETC_MACRO(cblk->cp_rowptr);
	    }
	}
	else if (CMcmpcase(cblk->cp_valstart, cmap->cp_delim) != 0)
	{
	    IICPwemWriteErrorMsg(E_CO0047_FILE_READ_MSG,cblk,
			cmap->cp_name, ERget(S_CO006A_BAD_DELIM));
	    return (FAIL);
	}
    }

    return (OK);

} /* IIcpfrom_lob */

/*
** Name: IIcpfrom_seglen - Get segment length for LONG row-value
**
** Description:
**
**	During a COPY FROM of a LOB, the (long) file value has to
**	be divided into segments.  Each segment starts with a segment
**	length, which is either an i2 (binary) or a digit string ended
**	with a space (converting).  This routine reads the segment
**	length that is to be next in the input stream, and returns it.
**
**	The caller can specify whether to read the segment length as
**	its own value, or to extend onto the value already buffered.  The
**	latter case is used when we've read a segment and want to read
**	the next segment length before sending, so as to properly set
**	data-end.
**
** Inputs:
**	cblk		- Copy control data block
**	cmap		- Copy map info for the LOB column
**	extendval	- TRUE to keep / add on to value starting at valstart
**
** Outputs:
**	Returns: segment length (may be 0), or COPY_EOF or COPY_FAIL.
**	(Note that the latter two are both negative and can't be
**	confused with a valid length).  For binary LONG NVARCHAR,
**	a character count is returned, half the byte count.
**
**	If an error occurs, an error message is issued.
**
**	If no error occurs, the input stream is positioned after the
**	length i2, or the length-terminating space.
**
** History:
**	14-Dec-2009 (kschendel) SIR 122974
**	    Adapt old segment-length reader to new environs.
*/

static i4
IIcpfrom_seglen(II_CP_STRUCT *cblk, II_CP_MAP *cmap, bool extendval)
{

    i2 i2seglen;
    i4 ch;
    i4 offset;
    i4 seglen;
    u_char *dataend;
    u_char *rowptr, *p;
    STATUS sts;

    dataend = cblk->cp_dataend;
    rowptr = cblk->cp_rowptr;
    if (!extendval)
	cblk->cp_valstart = rowptr;
    /* Track start of length as offset from valstart, since the thing
    ** can move around as the buffer refills
    */
    offset = rowptr - cblk->cp_valstart;
    if (!cblk->cp_convert)
    {
	/* Binary flavor */
	rowptr += sizeof(i2);
	if (rowptr > dataend)
	{
	    --rowptr;	/* cp_refill precondition */
	    ch = cp_refill(&dataend, &rowptr, cblk);
	    if (ch == COPY_EOF)
		return (COPY_EOF);
	    else if (ch == COPY_FAIL)
		goto seglen_err;
	}
	p = cblk->cp_valstart + offset;
	I2ASSIGN_MACRO(*p, i2seglen);
	seglen = (i4) i2seglen;
	sts = OK;
    }
    else
    {
	/* long value, read digits to and including space.  Anything
	** other than digit or space is an error.  Insist on
	** ordinary spaces, no weird utf8 spaces.  :)
	** Max seg from file is 32K, so allow 5 digits+space max.
	*/
	do
	{
	    ch = CP_GETC_MACRO(dataend, rowptr, cblk);
	    if (ch == COPY_EOF)
		return (COPY_EOF);
	    else if (ch == COPY_FAIL)
		break;
	    cblk->cp_refill_control = COPY_REFILL_EOF_FAILS;
	    if (ch == ' ' || ch < '0' || ch > '9')
		break;
	} while (rowptr - (cblk->cp_valstart+offset) <= 6);
	if (ch != ' ')
	    goto seglen_err;
	*(rowptr-1) = '\0';	/* Overwrite space with null terminator */
	sts = CVal(cblk->cp_valstart+offset, &seglen);
    }
    /* Binary LONG NVARCHAR stores a character count (just like the data
    ** format inside the DBMS), not a byte count.  Check against byte count
    ** but return the char count.
    */
    if (sts != OK || seglen < 0 || seglen > MAX_SEG_FROM_FILE
      || (!cblk->cp_convert && abs(cmap->cp_tuptype) == DB_LNVCHR_TYPE
	  && seglen > (MAX_SEG_FROM_FILE/sizeof(UCS2)) ) )
	goto seglen_err;
    cblk->cp_dataend = dataend;
    cblk->cp_rowptr = rowptr;
    return (seglen);

seglen_err:
    cblk->cp_dataend = dataend;
    cblk->cp_rowptr = rowptr;
    IICPwemWriteErrorMsg(E_CO0047_FILE_READ_MSG,
		cblk, cmap->cp_name, ERget(S_CO005F_FMT_SEG_LEN));
    return (COPY_FAIL);

} /* IIcpfrom_seglen */

/*{
+*  Name: IICPsebSendEmptyBlob - Data Handler for sending an empty blob to DBMS
**
**  Description:                 This routine calls IILQlpd_LoPutData() with
**                               an empty segment buffer and DATAEND set to
**                               TRUE to send an empty blob to the DBMS.
**  Inputs:
**	none
**
**  Outputs:
**	none
**
**  Returns: void
**
**  History:
**	jun-1993 (mgw)	- Written
*/
static void
IICPsebSendEmptyBlob()
{
	/* Send an empty blob */
	IILQlpd_LoPutData(0, DB_CHR_TYPE, 0, (char *) 0, 0, 1);
}

/*
** Name: IIcpfrom_lobHandler - LOB datahandler for COPY FROM 
**
** Description:
**
**	This is the datahandler callback, called from LIBQ to read
**	and send segments of a LOB from the from-file to the dbms.
**
**	A LOBULKFROM object is passed to carry the necessary COPY
**	data from from_lob to this handler.  The first segment length
**	has already been read.  We'll read and send the data part for
**	that segment, then the next length, and so on until a zero
**	segment length is read (and sent).
**
**	A few special cases:
**	- A NULL value must be set up by the caller;  it's intercepted
**	  by LIBQ and control never gets here for NULL.
**	- For binary copy (!cp_convert), and a nullable column/row type,
**	  the LOB row-value will end with a null indicator.  We'll require
**	  that the indicator be zero (not NULL).
**	- For non-binary COPY, if the row-value is not a LONG type, the
**	  value is already read in and we don't have to read it again.
**	  The value is also the one and only segment, so after sending it,
**	  send a data-end with no more reading.
**	- LONG NVARCHAR data is either UCS2 or UTF8, see inline.
**
** Inputs:
**	lob_info	- Points to LOB info cookie that connects to COPY data
**
** Outputs:
**	lob_info.failure set TRUE if something bad happened, we emit any
**	error message.
**
** History:
**	14-Dec-2009 (kschendel) SIR 122974
**	    Adapt original LOB handlers to new copy framework.
*/

static void
IIcpfrom_lobHandler(LOBULKFROM *lob_info)
{
    DB_STATUS		dbstat;
    DB_DATA_VALUE	utf8_dv, rdv;	/* For UTF8 -> UCS2 conversion */
    II_CP_STRUCT	*cblk = lob_info->cblk;
    II_CP_GROUP		*cgroup = lob_info->cgroup;
    II_CP_MAP		*cmap = cgroup->cpg_first_map;
    i4			absttype;
    i4			ch;
    i4			curseg = 1;	/* For messages */
    i4			lotype;
    i4			data_end = 0;
    i4			nextseglen;
    i4			segbytes, seglen, seg_flags;
    u_char		*valstart;

    seglen = lob_info->firstseglen;
    absttype = abs(cmap->cp_tuptype);
    lotype = DB_CHR_TYPE;
    if (absttype == DB_LNVCHR_TYPE)
    {
	lotype = DB_NCHR_TYPE;
	if (cblk->cp_convert)
	{
	    /* In a nonbinary context, LONG NVARCHAR means UTF8.
	    ** Set up a conversion area if we don't have one already.
	    ** Worst case is 1 UCS2 char (= 2 octets) for each UTF8 char
	    ** input.  (One can get 2 UCS2 chars out, but that requires
	    ** at least 3 UTF8 octets in.)
	    */
	    rdv.db_datatype = DB_NVCHR_TYPE;
	    utf8_dv.db_datatype = DB_BYTE_TYPE;
	    if (cgroup->cpg_ucv_buf == NULL)
	    {
		cgroup->cpg_ucv_len = sizeof(i2) + MAX_SEG_FROM_FILE * sizeof(UCS2);
		cgroup->cpg_ucv_buf = MEreqmem(0, cgroup->cpg_ucv_len, FALSE, NULL);
		if (cgroup->cpg_ucv_buf == NULL)
		{
		    IIlocerr(GE_LOGICAL_ERROR, E_CO0037_MEM_INIT_ERR, II_ERR,
				0, NULL);
		    lob_info->failure = TRUE;
		    return;
		}
	    }
	}
    }

    /* Loop to read this segment and the length of the next.  For non-LOB
    ** row values, there is only this one segment, and it's been read.
    ** If this segment is empty, we're at the end already.
    */
    do
    {
	segbytes = seglen;
	/* In a binary context, LONG NVARCHAR is UCS2 just like in the DBMS */
	if (absttype == DB_LNVCHR_TYPE && !cblk->cp_convert)
	    segbytes = seglen * sizeof(UCS2);
	nextseglen = 0;
	if (seglen > 0 && cgroup->cpg_is_row_lob)
	{
	    /* Read the actual LOB value */
	    cblk->cp_valstart = cblk->cp_rowptr;
	    cblk->cp_rowptr += segbytes;
	    if (cblk->cp_rowptr > cblk->cp_dataend)
	    {
		--cblk->cp_rowptr;	/* Usual refill protocol */
		ch = cp_refill(&cblk->cp_dataend, &cblk->cp_rowptr, cblk);
		if (ch == COPY_FAIL)
		{
		    char err_buf[15], msg[90+1], cursegbuf[15];
		    lob_info->failure = TRUE;
		    CVla(cblk->cp_rowcount + 1, err_buf);
		    STlcopy(ERget(S_CO0060_SEG_VAL), msg, 90);
		    CVla(curseg, cursegbuf);
		    IIlocerr(GE_LOGICAL_ERROR, E_CO0043_BLOB_SEG_READ_ERR,
				II_ERR, 4, err_buf, cmap->cp_name, msg,
				cursegbuf);
		    return;
		}
	    }
	    ++curseg;
	    /* Read the next segment length too so we know whether to
	    ** set data end.  (but don't lose the current segment!)
	    */
	    nextseglen = IIcpfrom_seglen(cblk, cmap, TRUE);
	    if (nextseglen == COPY_FAIL)
	    {
		lob_info->failure = TRUE;
		return;
	    }
	}
	if (nextseglen == 0)
	    data_end = 1;

	/* Ship out this segment, after possibly converting it. */
	if (segbytes == 0)
	{
	    seg_flags = 0;
	    valstart = NULL;
	}
	else
	{
	    valstart = cblk->cp_valstart;
	    seg_flags = II_DATSEG | II_DATLEN;
	    if (!cblk->cp_convert)
		seg_flags |= II_BCOPY;
	    else if (lotype == DB_NCHR_TYPE)
	    {
		utf8_dv.db_data = (PTR) valstart;
		utf8_dv.db_length = segbytes;
		rdv.db_data = cgroup->cpg_ucv_buf;
		rdv.db_length = cgroup->cpg_ucv_len;
		((DB_NVCHR_STRING *)rdv.db_data)->count =
			(cgroup->cpg_ucv_len - sizeof(i2)) / sizeof(UCS2);
		dbstat = adu_nvchr_fromutf8(lob_info->libq_cb->ii_lq_adf,
				&utf8_dv, &rdv, 0);
		if (dbstat != E_DB_OK)
		{
		    IICPwemWriteErrorMsg(E_CO0039_TUP_FORMAT_ERR,
				cblk, cmap->cp_name, "");
		    lob_info->failure = TRUE;
		    return;
		}
		valstart = (u_char *) &((DB_NVCHR_STRING *)rdv.db_data)->element_array[0];
		seglen = ((DB_NVCHR_STRING *)rdv.db_data)->count;
		/* Tell data putter that it's UCS2, not wchar_t */
		seg_flags |= II_BCOPY;
	    }
	}

	if (cblk->cp_dbg_ptr)
	    SIfprintf(cblk->cp_dbg_ptr, "COPYFROM: seg %d len %d bytes %d data_end %d\n",
		curseg-1, seglen, segbytes, data_end);

	IILQlpd_LoPutData(seg_flags, lotype, 0, (char *) valstart, seglen, data_end);
	seglen = nextseglen;
    } while (data_end == 0);

    /* Eat null indicator if binary copy of nullable.  Can't be a real null
    ** or we wouldn't be here at all.
    */
    if (!cblk->cp_convert && cmap->cp_tuptype < 0)
    {
	cblk->cp_valstart = cblk->cp_rowptr;
	ch = CP_GETC_MACRO(cblk->cp_dataend, cblk->cp_rowptr, cblk);
	if (ch == COPY_FAIL)
	{
	    /* Error row X: unexpected EOF reading Null indicator */
	    /* for column COL_NAME */
	    char err_buf[15], msg[80+1];
	    CVla(cblk->cp_rowcount + 1, err_buf);
	    STlcopy(ERget(S_CO005E_NULL_IND), msg, 80);
	    IIlocerr(GE_LOGICAL_ERROR, E_CO0042_BLOB_READ_ERR, II_ERR,
			3, err_buf, cmap->cp_name, msg);
	    lob_info->failure = TRUE;
	    return;
	}
	else if (ch != 0)
	{
	    char err_buf[15];
	    CVla(cblk->cp_rowcount + 1, err_buf);
	    IIlocerr(GE_LOGICAL_ERROR, E_CO0046_NOZERO_NULL_BLOB, II_ERR,
			2, err_buf, cmap->cp_name);
	    lob_info->failure = TRUE;
	    return;
	}
    }

} /* IIcpfrom_lobHandler */

/*{
+*  Name: IIcp_isnull	- Check for a pattern that represents a null value
**			  in a row domain.
**
**  Description:
**
**  Inputs:
**	cmap		- copy map ptr
**	val_start	- Pointer to start of row value; for counted or
**			  text row-types, this points to the actual chars,
**			  not the count prefix or DB_TEXT_STRING.
**	vallen		- Length in bytes of row value (actual chars)
**
**  Outputs:
**	Returns:	1 if the pattern matches the null value.
**			0 otherwise.
-*
**  Side Effects:
**	none
**
**  History:
**	12-jun-87 (puree)
**	    written
**	20-jan-89 (barbara)
**	    Fixed bug #4370.  For varchar count field must be converted into
**		ascii representation before comparison is done.  For text
**		count field should be ignored.
**	20-may-89 (kathryn)  Bug #5605
**		Add CP_VCHLEN to nullen for the comparison to a variable length
**		field, so that actual data values and not just the blanks 
**		preceding the data are used for comparison. CP_VCHLEN is the
**		the number of blanks preceding both the null data and the
**		row data.
**      08-Jun-98 (wanya01) bug 91259
**              Add fieldlen to detect if cp_nuldata appears at begining of
**              row_ptr and also is a substring of row_ptr.
**      19-Jun-98 (wanya01) revised change for bug 91259
**              Trailing blanks should not be counted when comparing null
**              value with actual data value.
**	15-nov-2000 (gupsh01)
**		copying out of money fields was not done correctly when
**		specifying a with null clause. This is related to bug 
**		91259. The field length was not being calculated correctly.
**      03-Dec-2003 (gupsh01)
**          Modified copy code so that nchar and nvarchar types will now use
**          UTF8 to read and write from the text file for a formatted copy.
**	7-Dec-2009 (kschendel) SIR 122974
**	    Pass length and pointer to start of data, not count.
**	    Stop scan when either length runs out.
**	    Properly deal with (ie allow) WITH NULL ('') empty string.
*/
static bool
IIcp_isnull(II_CP_MAP *cmap, u_char *val_start, i4 vallen)
{
    i4		nullen;
    u_char	*nullptr;

    /*
    ** NULL pattern allowed only when "with null (value)" clause
    ** was specified.  If the file value is nullable (rowtype < 0),
    ** there is no NULL pattern.  Let the caller figure out null indicators.
    */
 
    if (!cmap->cp_withnull || cmap->cp_rowtype < 0)
	return (FALSE);

    nullptr = (u_char *) cmap->cp_nuldata;
    nullen = cmap->cp_nullen;

    while (nullen > 0 && vallen > 0)
    {
	if ((*nullptr++) != (*val_start++))
	    break; 
	nullen--;
	vallen--;
    }
    if (nullen == 0)
    {
      /* for Varchar , char and varbyte type
      ** and if the length is known then
      ** there may be trailing blanks in the
      ** field length, we skip them for
      ** comparison.
      */
      if (vallen > 0)
      {
	  while(vallen &&
		(*val_start == ' ' || *val_start == '\0'))
	  {
	      vallen--;
	      val_start++;
	  }
       }
       if (vallen == 0)
	 return(TRUE);
    }
 
    return(FALSE);
}
 

/*
** Name: cp_refill - Refill COPY FROM row buffer
**
** Description:
**
**	This routine refills the COPY FROM row buffer, preserving
**	any value between cp_valstart and the current buffer pointer
**	position by moving it to the workspace immediately in front
**	of the actual read buffer area.
**
**	cp_refill runs from a buffer pointer and an end-of-data (+1) pointer,
**	which are operated in "the usual" manner.  I.e. the buffer pointer
**	is positioned at the next byte to read, and is used in
**	*ptr++ manner.  The normal usage sequence is:
**
**	    (ptr<dataend) ? *ptr++ : cp_refill(&dataend,&ptr,copy_blk))
**
**	so that when cp_refill is called, the pointer has not yet
**	been incremented.
**
**	In addition to refilling the buffer, cp_refill has the side
**	effects:
**	- dataend is set to the new end-of-data position, plus 1.
**	- ptr is incremented, so that it points to the NEXT available
**	  byte position (which is now at some offset from the start of
**	  the buffer, rather than the end);
**	- cp_valstart is moved to reflect the new position of the
**	  (saved) value.
**
**	cp_refill returns the next input byte, ie the byte that would have
**	been read if the buffer had not emptied.  If no more input is
**	available, it returns COPY_EOF.  If a read error occurs, it returns
**	COPY_FAIL.
**
**	A couple important notes.
**
**	First, it is allowable to read a string of N bytes, with the
**	proper idiom being:
**	    ptr += N;
**	    if (ptr > dataend)	Note! > not >=
**	    {
**		--ptr;
**		ch = cp_refill(...); check ch for status;
**	    }
**	    N bytes are in the buffer up, at (ptr-N) thru (ptr-1)
**	With this style of cp_refill call, if cp_refill reads some but
**	not all of the N bytes, it returns COPY_FAIL instead of COPY_EOF.
**	(The idea being that reading none of the value might be EOF,
**	but reading some-but-not-all of it is certainly improper.)
**
**	Second, the dataend and pointer can be, but need not be, the
**	stored ones in the II_CP_STRUCT;  namely, cp_dataend and
**	cp_rowptr.  If the caller chooses to use locals for the dataend
**	and pointer, presumably to speed things up, it's on the caller's
**	head to eventually store the updated dataend and pointer back into
**	the II_CP_STRUCT.  cp_refill does NOT change cp_dataend nor
**	cp_rowptr.
**
**	Third, if cp_valstart thru the current ptr is too large to
**	move into the buffer work-space, COPY_FAIL is declared.  This
**	implies a single column value larger than the workspace, which
**	should not happen and is considered input abuse.  (LOB's are
**	handled in a segmented manner to avoid this problem.)
**
**	One thing that cp_refill DOES NOT do is normalize \r\n (the Windows
**	line-ender) to just \n.  There are two reasons for this.  First,
**	it's faster to do the \r-tossing in-line, as opposed to rewriting
**	the buffer to squeeze out the \r's.  Second, it is not entirely
**	clear what an \-escaped \r should mean, but I'm going to take it
**	to mean "take the \r literally" (because otherwise there is no
**	good way to get the actual \r\n into the table if that is what's
**	desired.)  So, it will be up to the caller to skip \r before \n.
**
**	Callers can optimize EOF handling ever so slightly by setting
**	special values into the "refill control".  Possibilities are
**	EOF is FAIL, or EOF is a newline (the latter is for CSV).
**
** Inputs:
**	dataend_p	Pointer to end-of-data, ie last available byte + 1
**	rowptr_p	Pointer to not-yet-incremented cur-byte pointer
**	copy_blk	II_CP_STRUCT control block
**
** Outputs:
**	*dataend_p	Updated to reflect refilled buffer
**	*rowptr_p	Updated to reflect refilled buffer, and incremented
**
** Returns next character, or COPY_EOF, or COPY_FAIL.
**
** History:
**	8-Dec-2009 (kschendel) SIR 122974
**	    Copy buffer handling rewrite.
**	10-Mar-2010 (kschendel) SIR 122974
**	    Spiff up a bit more by not maintaining byte-left, just compare
**	    against a buffer end guard.
**	5-Aug-2010 (kschendel) b124197
**	    Turn off "continue" when we get an error here, since continuing
**	    would be worse than fruitless.
*/

static i4
cp_refill(u_char **dataend_p, u_char **rowptr_p, II_CP_STRUCT *copy_blk)
{
    i4 ch;
    i4 num_read, len;
    i4 save_len;
    u_char *dataend = *dataend_p;
    u_char *fillptr;
    u_char *rowptr = *rowptr_p;
    u_char *val_end;
    STATUS sts;

    if (copy_blk->cp_at_eof)
    {
	if (copy_blk->cp_refill_control == COPY_REFILL_EOF_NL)
	{
	    ++ *rowptr_p;
	    return ('\n');
	}
	copy_blk->cp_continue = FALSE;
	if (copy_blk->cp_refill_control == COPY_REFILL_EOF_FAILS)
	    return (COPY_FAIL);
	else
	    return (COPY_EOF);
    }

    /* The last byte actually in the buffer is at dataend - 1. */
    val_end = dataend - 1;
    if (copy_blk->cp_valstart != NULL)
    {
	len = 0;
	if (copy_blk->cp_valstart <= val_end)
	{
	    len = val_end - copy_blk->cp_valstart + 1;
	    if (len > copy_blk->cp_rbuf_worksize)
	    {
		copy_blk->cp_continue = FALSE;
		return (COPY_FAIL);		/* Issue message here??? */
	    }
	    MEcopy((PTR) copy_blk->cp_valstart, len, copy_blk->cp_readbuf-len);
	}
	copy_blk->cp_valstart = (u_char *) copy_blk->cp_readbuf - len;
    }

    /* Rowptr enters at some position >= dataend.  Readjust it so that it
    ** is at the same position relative to the buffer start instead.
    ** The normal entry case for cp-getc-macro is that rowptr == dataend,
    ** we want to set rowptr = cp_readbuf and then we'll return the
    ** desired byte with *rowptr++.
    */
    rowptr = (u_char *) copy_blk->cp_readbuf + (rowptr - dataend);

    /* Refill the buffer from program or file. Require reading enough to
    ** at least fill the byte at rowptr.
    ** Note that SIread has the bad habit of signaling EOF even though it
    ** may have returned some data.  Ignore non-OK statuses unless the
    ** bytes-read is returned as zero also.
    */
    fillptr = (u_char *) copy_blk->cp_readbuf;
    len = copy_blk->cp_rbuf_readsize;
    do
    {
	save_len = len;		/* len shouldn't be smashed, but be safe */
	num_read = 0;		/* Again, safety */
	if (copy_blk->cp_program)
	    sts = (*copy_blk->cp_usr_row)(&len, (PTR) fillptr, &num_read);
	else
	    sts = SIread(copy_blk->cp_file_ptr, len, &num_read, (PTR) fillptr);
	if (num_read == 0)
	{
	    if (sts == ENDFILE)
		break;
	    else
	    {
		char err_buf[15];
		i4 err;

		CVla(copy_blk->cp_rowcount + 1, err_buf);
		err = E_CO0024_FILE_READ_ERR;
		if (copy_blk->cp_program)
		    err = E_CO003C_PROG_READ_ERR;
		IIlocerr(GE_LOGICAL_ERROR, err, II_ERR, 1, err_buf);
		copy_blk->cp_continue = FALSE;
		return (COPY_FAIL);
	    }
	}
	sts = OK;
	/* Don't assume refill will return as much as requested */
	len = save_len - num_read;
	fillptr += num_read;
    } while (fillptr <= rowptr);

    if (sts != OK)
    {
	/* Must be EOF.  If nothing at all was read, return EOF.
	** Otherwise some but not all of a multibyte read requirement
	** was read, treat that as FAIL.
	*/
	copy_blk->cp_at_eof = TRUE;
	if (fillptr == (u_char *) copy_blk->cp_readbuf)
	{
	    if (copy_blk->cp_refill_control == COPY_REFILL_EOF_NL)
	    {
		/* Leave "continue" alone for this one case */
		*dataend_p = rowptr;
		*rowptr_p = rowptr;
		return ('\n');
	    }
	    copy_blk->cp_continue = FALSE;
	    if (copy_blk->cp_refill_control == COPY_REFILL_EOF_FAILS)
		return (COPY_FAIL);
	    else
		return (COPY_EOF);
	}
	copy_blk->cp_continue = FALSE;
	return (COPY_FAIL);
    }

    ch = *rowptr++;
    *rowptr_p = rowptr;
    *dataend_p = fillptr;
    return (ch);

} /* cp_refill */

/*{
+*  Name: IIcpdbwrite - Send a group to the DBMS for COPY FROM
**
**  Description:
**
**	The routine name is a bit of a misnomer.  What this routine does
**	is to update the buffer tracker for the copy group just finished.
**	If the group was a LOB, the buffer tracker is reset from the
**	low level CGC info, and it might be that we have to send an
**	end-of-data.  (We send EOD if the LOB is the end of the row,
**	and the lob handler dumped the GCA buffer to the DBMS.)
**
**	If the group is not a LOB, but was too big for the GCA buffer,
**	we'll send it in pieces in its entirety.  (We could leave
**	the last bit in the GCA buffer, but this is a fairly rare
**	case, and the optimization is not worth the effort.)
**
**	We also track end-of-data.  If a GCA buffer is sent, we set
**	end-of-data on that send if and only if the last thing in the
**	buffer was the end of a row.  It turns out that this is not
**	really essential during the CDATA stream (given repairs to
**	the back-end!), but it doesn't hurt anything.
**
**  Inputs:
**	copy_blk	- copy control block
**	cgroup		- Copy group info
**
**  Outputs:
**	cpy_bufh
**	    .cpy_bytesused  - updated.
**	Returns: OK, FAIL
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	10-sep-87 (puree)
**	    convert to GCF architecture.
**	13-Dec-2009 (kschendel) SIR 122974
**	    Rearrange so that we can write a group less than a whole row.
**	29-Dec-2009 (kschendel) SIR 122974
**	    Understand end-of-data a bit better, so eliminate the need to
**	    forcibly flush GCA after each LOB.  (LOB handlers might do it
**	    anyway, but that's not our problem!)
*/
static STATUS
IIcpdbwrite( II_LBQ_CB *IIlbqcb, II_CP_STRUCT *copy_blk, II_CP_GROUP *cgroup)
{
    IICGC_DESC	*cgc;
    II_CPY_BUFH	*cpy_bufh;
    bool	last_group;
    i4		tuplen;
    i4		buflen;
    STATUS	sts;

    cgc = IIlbqcb->ii_lq_gca;
    cpy_bufh = copy_blk->cp_cpy_bufh;

    sts = OK;
    last_group = (cgroup == &copy_blk->cp_copy_group[copy_blk->cp_num_groups-1]);
    if (cgroup->cpg_tup_length == 0)
    {
	/* Dummy or LOB groups.  (If it's a dummy, the last non-dummy had
	** to be LOB.)  LOB sending bypasses our buffer tracker, so reset it.
	*/
	if (cgroup->cpg_is_tup_lob)
	{
	    cpy_bufh->cpy_bytesused = cgc->cgc_write_msg.cgc_d_used;
	    cpy_bufh->cpy_dbv.db_length = cgc->cgc_write_msg.cgc_d_length;
	    cpy_bufh->cpy_dbv.db_data = (PTR) cgc->cgc_write_msg.cgc_data;
	}
	/* LOB's always send "stuff", if our buffer is empty now,
	** what we just sent may have ended a row -- and we have to
	** tell GCA that.
	*/
	if (last_group && cpy_bufh->cpy_bytesused == 0)
	{
	    cpy_bufh->end_of_data = FALSE;
	    copy_blk->cp_sent = TRUE;
	    return (IIcc1_send(cgc, TRUE));
	}
	cpy_bufh->end_of_data = last_group;
	return (OK);
    }

    /*
    ** If the GCA message buffer is smaller than a tuple, the tuple has
    ** been built in the tuple buffer(copy_blk->cp_tupbuf).  We have to
    ** copy it into GCA buffer and send it out.  The from-init routine
    ** ensures an empty GCA buffer before the group starts.
    */
    tuplen = cgroup->cpg_tup_length;
    buflen = cpy_bufh->cpy_dbv.db_length;
    if (buflen < tuplen)
    {
	bool	eom;
	i4	bytescopy;
	PTR	bptr = cpy_bufh->cpy_dbv.db_data;
	PTR	tptr = copy_blk->cp_tupbuf;

	while (tuplen)
	{
	    bytescopy = min(buflen, tuplen);
	    MEcopy(tptr, bytescopy, bptr);
	    tptr += bytescopy;
	    tuplen -= bytescopy;
	    eom = (tuplen == 0 && last_group);
	    if (IIcgcp3_write_data(cgc, eom, bytescopy, &cpy_bufh->cpy_dbv)
			!= OK)
		return(FAIL);
	    copy_blk->cp_sent = TRUE;
	}
	cpy_bufh->cpy_bytesused = 0;
	cpy_bufh->end_of_data = FALSE;
    }
    else
    {
	/* The group just filled in fit in the GCA buffer, so the [partial]
	** tuple was built right in the GCA buffer.  We need to increment the
	** bytes-used counter and note whether this is the end of a row.
	** The buffer is not yet flushed, we'll do it if the next group
	** needs the room.
	*/
	cpy_bufh->cpy_bytesused += tuplen;
	cpy_bufh->end_of_data = last_group;
    }
    return (OK);
}

/*{
+*  Name: IIcpfopen - Open the copy file.
**
**  Description:
**	Call SI to open the copy file.  Open in write mode if COPY INTO, read
**	mode if COPY FROM.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: OK, FAIL
-*
**  Side Effects:
**	File is opened, possibly created (if COPY FROM).
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	22-jan-90 (barbara)
**	    Fixed up call to LOfroms to use correct sized buffer.
**	04-apr-1997 (canor01)
**	    Windows NT must always open the file in binary mode
**	    when reading, and do the CRLF=>LF translation at
**	    character read time to prevent ^Z character from
**	    causing EOF.
**    	23-nov-98 (holgl01) bug 93563
**         Set rms buffer for vms systems for a TXT (text) file to the 
**         maximum record that can be written (SI_MAX_TXT_REC) 
**	   for SIfopen. Otherwise, files greater than 4096 in length cannot
**	   be opened.
**	22-dec-1998 (holgl01) Revision of Bug 93563
**	    Pass SI_MAX_TXT_REC directly  to SIfopen in IIcpfopen
**	    instead of resetting  cp_row_length to SI_MAX_TXT_REC 
**          and passing that.
**	28-Jan-2010 (kschendel) SIR 122974
**	    Reverse Orlan's apr-1997 decision.  If the copy domain list
**	    is all-character, such that cp-init has decided on text
**	    filetype, use text.  This does the CRLF -> LF translation,
**	    solving innumerable HOQA and real-life issues.  If anyone has
**	    a control-Z in a file that is to be read with an all-character
**	    newline-ended domain list, that's just too bad and they
**	    shouldn't be doing that.  (A workaround would be to end the
**	    domain list with xx=d1 instead of nl=d1, but then of course
**	    the \r\n's come back, putting all your counted-length domains
**	    at risk of failure.)
**      01-Jul-2010 (horda03) B124010
**          If SIfopen fails for a COPY FROM, then don't abort the
**          query, just end. Thus preserving a Global Temp. Table
**          (if it was the target of the copy).
*/
static i4
IIcpfopen(II_CP_STRUCT *copy_blk)
{
    LOCATION	loc;
    char	locbuf[MAX_LOC +1];	/* Holds location string */
    DB_STATUS	status;
    i4		rec_len;
 
    STcopy( copy_blk->cp_filename, locbuf );
    LOfroms(PATH & FILENAME, locbuf, &loc);
 
    if (copy_blk->cp_direction == CPY_INTO)
    {
	status = SIfopen(&loc, "w", copy_blk->cp_filetype,
	    copy_blk->cp_row_length, &copy_blk->cp_file_ptr);
	if (status)
	    IIlocerr(GE_HOST_ERROR, E_CO0006_FILE_CREATE_ERR, II_ERR, 
		     1, copy_blk->cp_filename);
    }
    else 
    {
	rec_len = copy_blk->cp_row_length;
# if defined(VMS)
        /*
        **  holgl01 bug 93563
        **      Pass maximum record length allowed for text files
        **      so that the rms (vax) buffer will be large enough
        **      to read any record from a text file.
        */
        if(copy_blk->cp_filetype == SI_TXT)
	    rec_len = SI_MAX_TXT_REC;
#endif /* VMS */
        status = SIfopen(&loc, "r", copy_blk->cp_filetype,
            rec_len, &copy_blk->cp_file_ptr);
	if (status)
        {
            /* As the COPY failed to open the file for reading, no
            ** data has been copied to the table, so no need to
            ** Abort. We don't want to abort the COPY in this circumstance
            ** as if the table is a Global Temporary Table it would
            ** be dropped.
            */
            copy_blk->cp_status |= CPY_NOABORT;

	    IIlocerr(GE_HOST_ERROR, E_CO0005_FILE_OPEN_ERR, II_ERR,
		     1, copy_blk->cp_filename);
        }
	copy_blk->cp_at_eof = FALSE;
    }
 
    if (status)
    {
	copy_blk->cp_warnings++;
	return (FAIL);
    }
    return (OK);
}

/*{
+*  Name: IIcpfclose - Close copy file
**
**  Description:
**	Closes copy file.
**	Zero's out the cp_file_ptr in the copy control block.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: OK, FAIL
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
*/
 
static i4
IIcpfclose(II_CP_STRUCT *copy_blk)
{
    i4		status;
 
    status = SIclose(copy_blk->cp_file_ptr);
    if (status)
    {
	IIlocerr(GE_HOST_ERROR, E_CO0031_FILE_CLOSE_ERR, II_ERR,
		 1, copy_blk->cp_filename);
	copy_blk->cp_warnings++;
	return (FAIL);
    }
 
    copy_blk->cp_file_ptr = NULL;
    return (OK);
}

/*{
+*  Name: IIcpendcopy - Perform end-of-copy protocol.
**
**  Description:
**	At finish of COPY, print out warning messages for following:
**	    - non-printable chars converted to blanks
**	    - copy domains that had to be truncated
**	    - rows that could not be processed
**	    - bad rows written to log file
**	    - rows not copied because duplicate key data found
**
**	If COPY finished successfully, but with warnings, give the ending
**	message to that effect.  If the COPY finished unsuccessfully, the
**	DBMS will generate the ending message.
**
**	If this is a COPY FROM and there are no errors, flush all the tuples
**	in the communication buffer with the EOM option and send a response
**	message to the DBMS.
**
**	Call IIcpcleanup to clean up memory and files.
**
**  Error Conditions Handling:
**	COPY INTO:
**	    1.  Errors from FE:
**		    - cp_error set by the calling routine.
**		    - report warning/error messages.
**		    - send interrupt query to DBMS if rollback is enabled.
**		    - send reponse message to DBMS if rollback is disabled. 
**		    - clean up.
**
**	    2. Errors from DBMS: (Message from the DBMS will be taken care
**	       of by LIBQ).
**		    - report FE warning messages.
**		    - clean up.
**		    
**	COPY FROM:
**	    1.  Errors from FE:
**		    - cp_error set by the calling routine.
**		    - report warning/error messages.
**		    - send interrupt query to DBMS
**		    - clean up.
**
**	    2. Errors from DBMS: (Message from the DBMS will be taken care
**	       of by LIBQ).
**		    - cp_dbmserr set by the error polling routine.
**		    - IIcpendcopy is called by IIcopy.
**		    - report warning messages.
**		    - clean up.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: none
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	10-sep-87 (puree)
**	    convert to GCF architecture.
**	17-nov-87 (puree)
**	    fixed rollback option and clean up error handling.
**	07-sep-88 (sandyh)
**	    fixed DBMS error handling to prevent GCA_RESPONSE
**	    from being sent in DBMS error condition.
**	26-feb-1990 (barbara)
**	    Don't turn off IICGC_COPY_MASK up here because IIcgc_read_results
**	    will test for that mask as part of fix to bug #7927.
**	05-apr-90 (teresal)
**	    Fix for bug 8725 so warning messages show correct row count for
**	    number of rows copied. Rowcount no longer incremented by one.
**	15-jun-90 (barbara)
**	    LIBQ now continues after association failures (bug 21832).  Make
**	    sure we don't attempt to read/write from/to the dbms.
**	13-dec-90 (teresal)
**	    Bug fix 34405 - read response block to detect if rows with
**	    duplicate keys were dropped by the dbms.
**	22-may-97 (hayke02)
**	    Tighten up the check for duplicate rows (E_CO003F) - the
**	    number of rows copied must be greater than zero. If zero rows
**	    were copied (probably due to running out of disk space during
**	    a copy into a non heap table), return E_CO0029. This change
**	    fixes bug 80478.
**      21-Oct-98 (wanya01)
**          X-integrate change 436382
**          Added new error message (E_CO0048_ALLDUPS_OR_DISKFULL) to
**          replace the E_CO0029 added by the previous change. This new
**          error message will inform the user that 0 rows were copied
**          because either all rows were duplcates or there was a disk
**          full problem. This change fixes bug 90968.
*/
static void
IIcpendcopy(II_LBQ_CB *IIlbqcb, II_CP_STRUCT *copy_blk)
{
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    II_CPY_BUFH		*cpy_bufh = copy_blk->cp_cpy_bufh;
    GCA_RE_DATA		gca_rsp;
    char		num1_buf[20];
    char		num2_buf[20];
    bool		read_resp;		/* Read dbms response ? */
 
    /*
    ** 0.   If there has been an association failure, we just
    **	    	want to clean up and get out.
    */
    if ( ! (IIlbqcb->ii_lq_flags & II_L_CONNECT) )
    {
	IIcpcleanup(copy_blk);
	return;
    }

    /*
    **  1.  Report warning messages:
    **      1.a)  non-printing characters converted to blanks.
    */
    if ((copy_blk->cp_blanked) &&
	( (!copy_blk->cp_dbmserr) || copy_blk->cp_direction == CPY_INTO))
    {
	CVla(copy_blk->cp_blanked, num1_buf);
	IIlocerr(GE_DATA_EXCEPTION, E_CO0014_BLANKED_CHARS, II_ERR,
		 1, num1_buf);
	copy_blk->cp_warnings++;
    }
 
    /*	    1.b)  copy domains that had to be truncated */
 
    if ((copy_blk->cp_truncated) &&
	( (!copy_blk->cp_dbmserr) || copy_blk->cp_direction == CPY_INTO))
    {
	CVla(copy_blk->cp_truncated, num1_buf);
	IIlocerr(GE_DATA_EXCEPTION, E_CO0015_TRUNCATED_CHARS, II_ERR,
		 1, num1_buf);
	copy_blk->cp_warnings++;
    }
 
    /*
    **	    1.c)  copy rows that could not be processed.  if the rows were
    **	        logged, then say so - otherwise just give generic message.
    */
    if (copy_blk->cp_logged)
    {
	CVla(copy_blk->cp_logged, num1_buf);
	IIlocerr(GE_LOGICAL_ERROR, I_CO002C_LOGGED, II_ERR, 2, num1_buf,
		copy_blk->cp_logname);
	/* Don't count this as a warning! */
    }
    else if (copy_blk->cp_badrows)
    {
	CVla(copy_blk->cp_badrows, num1_buf);
	IIlocerr(GE_LOGICAL_ERROR, E_CO002B_BADROWS, II_ERR, 1, num1_buf);
	copy_blk->cp_warnings++;
    }

    /*
    **  2. If COPY FROM and have not received an error from the DBMS, flush
    **     the remaining tuples to the DBMS even if we may have errors in
    **	   the front end.
    */
    if ((copy_blk->cp_direction == CPY_FROM) && (!copy_blk->cp_dbmserr))
    {
	if (IIcpdbflush( IIlbqcb, copy_blk) != OK)
	    copy_blk->cp_error = TRUE;
	if (IIcgcp4_write_read_poll(cgc, GCA_CDATA) != GCA_CONTINUE_MASK)
	{
	    copy_blk->cp_dbmserr = TRUE;
	}
    }
    /*
    **  3.  Take care of error situation, if any.  Keep track 
    **	    if an interrupt has been sent so we know if a 
    **	    response block needs to be read.
    **
    **	3.a For COPY INTO, if an error is detected by the front end, send
    **	    an interrupt block to the DBMS.
    */
    read_resp = TRUE;	/* Assume DBMS will send back a response */
    if (copy_blk->cp_direction == CPY_INTO)
    {
	if (copy_blk->cp_error)
	{
	    IIcgc_break(cgc, GCA_INTALL);
	    CVla((copy_blk->cp_rowcount - copy_blk->cp_badrows), num1_buf);
	    IIlocerr(GE_LOGICAL_ERROR, E_CO0029_COPY_TERMINATED, II_ERR,
			1, num1_buf);
	    read_resp = FALSE;	/* DBMS won't send back a response */
	}
    }
    /*
    **  3.b For COPY FROM, we will send an interrupt message to the DBMS
    **	    if there is an error detected in the front end AND the rollback
    **	    option is enabled.  If an error is detected but rollback is
    **	    disable, we will issue user error message (later on) but
    **	    send a response message to the DBMS as if COPY is successfully 
    **	    completed. 
    */
    else
    {
	gca_rsp.gca_rqstatus = GCA_OK_MASK;
	gca_rsp.gca_rowcount = copy_blk->cp_rowcount;
 
	if (copy_blk->cp_error)
	{
	    if (copy_blk->cp_status & CPY_NOABORT)
	    {
		IIcgcp2_write_response(cgc, &gca_rsp);
	    }
	    else
	    {
		IIcgc_break(cgc, GCA_INTALL);
		IIlocerr(GE_LOGICAL_ERROR, E_CO002A_COPY_ABORTED, II_ERR,
			 0, (char *)0);
		read_resp = FALSE;	/* DBMS won't send back a response */
	    }
	}
	else if (!copy_blk->cp_dbmserr)	/* DBMS ready to receive response */
	{
	    IIcgcp2_write_response(cgc, &gca_rsp);
	}
    }
    /*	4.  Read response from DBMS to get accurate row count */

    /*	4.a The DBMS won't send a response block for an IIbreak so
    **	    don't bother reading if an IIbreak was called.
    */

    copy_blk->cp_rowcount -= copy_blk->cp_badrows; /* # rows sent by COPY */

    if (read_resp && !copy_blk->cp_dbmserr)
    {
	/* If response block has not been read, don't compare rowcount */	
	if (IIcgc_read_results(cgc, TRUE, GCA_RESPONSE) == 0)
	{
	    /*	
	    **  Check response block - if the actual number of rows
	    **  added by the DBMS was less than the number of rows sent
	    **  by COPY, it is because those rows not copied contain
	    **  duplicate key data.  A warning message is displayed
	    **  for SQL only. (Bug 34405)
	    */
	    if ((cgc->cgc_resp.gca_rowcount > 0) &&
		(cgc->cgc_resp.gca_rowcount < copy_blk->cp_rowcount))
	    {
		if (IIlbqcb->ii_lq_dml == DB_SQL)
		{
		    /* Calculate no. duplicate rows found */
		    copy_blk->cp_rowcount -= cgc->cgc_resp.gca_rowcount; 
		    CVla(copy_blk->cp_rowcount, num1_buf);
		    IIlocerr(GE_LOGICAL_ERROR, E_CO003F_DUPLICATES_FOUND, 
				II_ERR, 1, num1_buf);
		    copy_blk->cp_warnings++;
		}
	    /* DBMS rowcount is accurate # rows copied */
	    copy_blk->cp_rowcount = cgc->cgc_resp.gca_rowcount;
	    }
	    /*
            ** Bugs 80478, 90968: out of disk space during copy into a non-heap
            **                    table or all rows are duplicates.
	    */
	    if ((cgc->cgc_resp.gca_rowcount == 0) &&
		(copy_blk->cp_rowcount > 0) && (IIlbqcb->ii_lq_dml == DB_SQL))
	    {
		char err_buf[15];
		CVla(0, err_buf);
                IIlocerr(GE_LOGICAL_ERROR, E_CO0048_ALLDUPS_OR_DISKFULL, II_ERR,
			    1, err_buf);
	    }
	}
    }

    /*	5.a Display error message if rollback disabled and error in
    **	    COPY FROM.  Error message displayed here so
    **	    it will include the correct rowcount (Bug 34405).
    */
    if ((copy_blk->cp_direction == CPY_FROM) &&
	(copy_blk->cp_error) && (copy_blk->cp_status & CPY_NOABORT))
    {
	CVla((copy_blk->cp_rowcount), num1_buf);
	IIlocerr(GE_LOGICAL_ERROR, E_CO0029_COPY_TERMINATED, II_ERR,
		1, num1_buf);
    }

    /*	5.b Display count of warnings during COPY - Use correct
    **	    rowcount (Bug 34405)
    */
    if ((copy_blk->cp_warnings) && (!copy_blk->cp_error))
    {
	CVla(copy_blk->cp_warnings, num1_buf);
	CVla((copy_blk->cp_rowcount), num2_buf);
	IIlocerr(GE_LOGICAL_ERROR, E_CO0028_WARNINGS, II_ERR,
		 2, num1_buf, num2_buf);
    }
    /*
    **	6.  Clean up.
    */
    IIcpcleanup(copy_blk);
    /*
    **	7.  Turn off IN_COPY bit.
    **	    Note that this flag is now turned off in IIcgc_read_results.
    **	    (IIsyncup-->IIqry_end-->IIcgc_read_results)
    **	    This is part of fix to bug #7927.
    */
    /* cgc->cgc_g_state &= (~IICGC_COPY_MASK); */
 
    return;
}
 

/*{
+*  Name: IIcpcleanup - Clean up before exiting copy.
**
**  Description:
**	Clean up memory allocated for COPY, close files opened during COPY.
**	Turn off ULC bit that marks us as in the middle of a COPY.
**
**	This routine must be called before exiting COPY processing.  This
**	routine may be called multiple times with no bad effects (other than
**	performance).  This routine should be called on interrupts if we
**	will longjump out of copy processing.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: none
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	10-sep-87 (puree)
**	    convert to GCF architecture.
**	21-Dec-01 (gordy)
**	    Free GCA tuple descriptor allocated by IIcptdescr().
**	24-Jun-2005 (schka24)
**	    Free up unicode normalization work bufs.
**	15-Dec-2009 (kschendel) SIR 122974
**	    Free copy groups, various new control blocks.
*/
static void
IIcpcleanup(II_CP_STRUCT *copy_blk)
{
    II_THR_CB		*thr_cb = IILQthThread();
    II_LBQ_CB		*IIlbqcb = thr_cb->ii_th_session;
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    i4			i;
    II_CP_GROUP		*cgroup;
    II_CP_MAP		*cmap;

    /*
    ** Free GCA tuple descriptor
    */
    if ( cgc->cgc_tdescr )
    {
	MEfree((PTR) cgc->cgc_tdescr);
	cgc->cgc_tdescr = (GCA_TD_DATA *)NULL;
    }

    /*
    ** Close copy file if there is one.
    */
    if ((!copy_blk->cp_program) && (copy_blk->cp_file_ptr))
	(void) IIcpfclose(copy_blk);
 
    /*
    ** Close log file if there is one.
    */
    if ((copy_blk->cp_rowlog) && (copy_blk->cp_log_ptr))
	(void) IIcplgclose(copy_blk);
 
    /*
    ** Close debug file if there is one
    */
    if (copy_blk->cp_dbg_ptr)
	IIcpdbgclose(copy_blk);

    /*
    ** Free up memory allocated for copy structure.
    */
    if (copy_blk->cp_filename)
    {
	MEfree((PTR) copy_blk->cp_filename);
	copy_blk->cp_filename = NULL;
    }
    if (copy_blk->cp_copy_group)
    {
	for (cgroup = &copy_blk->cp_copy_group[0];
	     cgroup < &copy_blk->cp_copy_group[copy_blk->cp_num_groups-1];
	     ++cgroup)
	{
	    if (cgroup->cpg_ucv_buf != NULL)
		MEfree(cgroup->cpg_ucv_buf);
	}
	MEfree(copy_blk->cp_copy_group);
    }
    if (copy_blk->cp_copy_map)
    {
	for (i = 0; i < copy_blk->cp_num_domains; i++)
	{
	    cmap = &copy_blk->cp_copy_map[i];
	    if (cmap->cp_normbuf != NULL)
		MEfree(cmap->cp_normbuf);
	    if (cmap->cp_srcbuf != NULL)
		MEfree(cmap->cp_srcbuf);
	}
	MEfree((PTR) copy_blk->cp_copy_map);
	copy_blk->cp_copy_map = NULL;
    }
    if (copy_blk->cp_zerotup)
    {
	MEfree(copy_blk->cp_zerotup);
	copy_blk->cp_zerotup = NULL;
    }
    if (copy_blk->cp_tupbuf)
    {
	MEfree(copy_blk->cp_tupbuf);
	copy_blk->cp_tupbuf = NULL;
    }
    if (copy_blk->cp_rowbuf_all)
    {
	MEfree(copy_blk->cp_rowbuf_all);
	copy_blk->cp_rowbuf_all = NULL;
    }
    if (copy_blk->cp_decomp_buf)
    {
	MEfree(copy_blk->cp_decomp_buf);
	copy_blk->cp_decomp_buf = NULL;
    }
    if (copy_blk->cp_log_buf)
    {
	MEfree(copy_blk->cp_log_buf);
	copy_blk->cp_log_buf = NULL;
    }
    if (copy_blk->cp_buffer)
    {
	MEfree(copy_blk->cp_buffer);
	copy_blk->cp_buffer = NULL;
    }
    if (copy_blk->cp_logname)
    {
	MEfree((PTR) copy_blk->cp_logname);
	copy_blk->cp_logname = NULL;
    }
 
 
    return;
}
 

/*{
+*  Name: IIcpdbflush - Flush output waiting in the communication block.
**
**  Description:
**	If there are tuples sitting in the communication block to send to the
**	DBMS, we need to flush them before ending the copy.
**
**	We are ending the copy, so if there's anything to flush, the
**	GCA buffer necessarily ends with the end of a row.  So, send EOD.
**	This end-of-data IS important, as it's the end of the CDATA
**	stream.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: OK, FAIL
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	10-sep-87 (puree)
**	    convert to GCF architecture.
*/
static i4
IIcpdbflush(II_LBQ_CB *IIlbqcb, II_CP_STRUCT *copy_blk)
{
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    II_CPY_BUFH		*cpy_bufh = copy_blk->cp_cpy_bufh;
 

    if (IIcgcp3_write_data(cgc, TRUE, cpy_bufh->cpy_bytesused,
	    &cpy_bufh->cpy_dbv) != OK)
	return(FAIL);
 
    cpy_bufh->cpy_bytesused = (i4)0;
    cpy_bufh->end_of_data = FALSE;
    return(OK);
}
 
 

/*{
+*  Name: IIcpabort - Abort COPY command during initialization phase.
**
**  Description:
**		    If a fatal error is detected when IIcopy receives
**		    the copy map from the DBMS, this routine is called
**		    to signal the DBMS to abort the COPY command.
**
**		    After sending the copy map to the front end,  the
**		    DBMS waits for an ACK block.  Instead of sending
**		    the ACK block, if IIcopy detects a fatal error, it
**		    send a RESPONSE block to the DBMS.  The DBMS aborts
**		    the COPY command and sends a RESPONSE block to the
**		    front end to go ahead with the next query.
**
**		    IIcpabort is reponsible for sending and reading the
**		    RESPONSE block to and from the DBMS for the purpose
**		    of aborting the COPY commands.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: none
-*
**  Side Effects:
**	none
**
**  History:
**	24-sep-87 (puree)
**	    written for GCF architecture.
**	26-feb-1990 (barbara)
**	    Don't need to turn off IICGC_COPY_MASK because IIcgc_read_results
**	    now turns it off as part of fix to bug #7927
*/

static void
IIcpabort( II_LBQ_CB *IIlbqcb, II_CP_STRUCT *copy_blk )
{
    IICGC_DESC		*cgc = IIlbqcb->ii_lq_gca;
    GCA_RE_DATA		gca_rsp;
 
 
 
    /* Free all memory acquired */
    IIcpcleanup(copy_blk);
 
    /* Format and send a RESPONSE message to the DBMS */
 
    gca_rsp.gca_rqstatus = GCA_FAIL_MASK;
    gca_rsp.gca_rowcount = GCA_NO_ROW_COUNT;
    IIcgcp2_write_response(cgc, &gca_rsp);
 
    /* Read and discard the corresponding RESPONSE message from the DBMS */
 
    while (IIcgc_read_results(cgc, TRUE, GCA_RESPONSE));
    
    /* Turn off IN_COPY bit */
 
    /* Mask will have been turned on in IIcgc_read_results */

    /* cgc->cgc_g_state &= (~IICGC_COPY_MASK); */
}
 
 

/*{
+*  Name: IIcplgopen - Open copy log file/
**
**  Description:
**	Open log file to be used for bad copy rows.  File is opened for
**	writing.
**
**	If COPY FROM, open file with same attributes as the copy file.
**	If COPY INTO, make it fixed lenght binary (it will hold tuples).
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: OK, FAIL
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	22-jan-90 (barbara)
**	    Fixed up call to LOfroms to use correct sized buffer.
**	10-Dec-2009 (kschendel) SIR 122974
**	    Windows copy-from always uses binary, we're copying rows
**	    from an input file opened binary.
*/
static i4
IIcplgopen(II_CP_STRUCT *copy_blk)
{
    LOCATION	loc;
    char	locbuf[MAX_LOC +1];	/* Used to hold location string */
    i4	filewidth;
    i4		filetype;
 
    STcopy( copy_blk->cp_logname, locbuf );
    LOfroms(PATH & FILENAME, locbuf, &loc);
 
    if (copy_blk->cp_direction == CPY_FROM)
    {
	/*
	** Find out real attributes of copy file to create similiar one.
	*/
	/* SIgetattr(&filetype, &filewidth); */
 
	if (copy_blk->cp_row_length)
	{
	    filewidth = copy_blk->cp_row_length;
	    filetype = copy_blk->cp_filetype;
	}
	else
	{
	    filewidth = DB_GW3_MAXTUP;
	    filetype = SI_VAR;
	}
#ifdef NT_GENERIC
	filetype = SI_BIN;
#endif
    }
    else 
    {
	filewidth = copy_blk->cp_tup_length;
	filetype = SI_BIN;
    }
 
    if (SIfopen(&loc, "w", filetype, filewidth,	&copy_blk->cp_log_ptr) != OK)
    {
	IIlocerr(GE_HOST_ERROR, E_CO0036_LOG_OPEN_ERR, II_ERR,
		 1, copy_blk->cp_logname);
	return (FAIL);
    }
 
    return (OK);
}

/*
** Name: IIcplginto - Write binary tuple to log file for COPY INTO logging.
**
** Description:
**	COPY INTO logging is relatively simple, since the binary tuple
**	is available.  This is a non-LOB tuple;  we don't honor LOG= when
**	there are LOB columns.
**
**  Inputs:
**	copy_blk	- copy control block
**	tuple		- pointer to tuple to log
**
**  Outputs:
**	Returns: none
**
** History:
**	10-Dec-2009 (kschendel) SIR 122974
**	    Split up into and from logging.
*/

static void
IIcplginto(II_CP_STRUCT *copy_blk, PTR tuple)
{
    STATUS	status;
    i4		num_written;
 
    status = SIwrite(copy_blk->cp_tup_length, tuple, &num_written,
	copy_blk->cp_log_ptr);
    if ((status != OK) || (num_written != copy_blk->cp_tup_length))
    {
	char err_buf[15];
	CVla(copy_blk->cp_rowcount + 1, err_buf);
	IIlocerr(GE_HOST_ERROR, E_CO002F_LOG_WRITE_ERR, II_ERR, 
		 2, copy_blk->cp_logname, err_buf);
    }
    copy_blk->cp_logged++;
}

/*
** Name: IIcplgcopyn - Copy bytes to FROM-log line image
**
** Description:
**	This routine is called by the CP_LOGTO_MACRO if we're copying
**	stuff (ie N bytes at a time) to the logging line image.
**	Copy from copy_blk->cp_valstart up to but not including rowptr.
**
** Inputs:
**	rowptr		Row buffer position to stop before
**	copy_blk	COPY control data block
**
** Outputs:
**	none
**
** History:
**	10-Dec-2009 (kschendel) SIR 122974
**	    Logging is annoying with the new copy from main-loop.
*/

static void
IIcplgcopyn(u_char *rowptr, II_CP_STRUCT *cblk)
{
    i4 len;

    len = rowptr - cblk->cp_valstart;
    if (len > 0)
    {
	if (len > cblk->cp_log_left)
	{
	    len = cblk->cp_log_left;
	    if (len <= 0)
		return;
	}
	MEcopy((PTR) cblk->cp_valstart, len, cblk->cp_log_bufptr);
	cblk->cp_log_left -= len;
	cblk->cp_log_bufptr += len;
    }
} /* IIcplgcopyn */

/*{
+*  Name: IIcplgfrom - Write FROM row to log file.
**
**  Description:
**	If copy row could not be processed, it is written to the log file.
**	COPY FROM uses a separate logging-line buffer, because the
**	main copy-from loop operates column-wise and not row-wise.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: none
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	10-Dec-2009 (kschendel) SIR 122974
**	    Split into and from logging.
*/
static void
IIcplgfrom(II_CP_STRUCT *copy_blk)
{
    STATUS	status;
    i4		len;
    i4		num_written;
 
    if (copy_blk->cp_rowlog)
    {
	len = copy_blk->cp_log_size;
	/* log-left might go negative if line too long, clip at zero */
	if (copy_blk->cp_log_left >= 0)
	    len = len - copy_blk->cp_log_left;

	if (len > 0)
	{
	    status = SIwrite(len, copy_blk->cp_log_buf, &num_written,
		copy_blk->cp_log_ptr);
	    if ((status != OK) || (num_written != len))
	    {
		char err_buf[15];
		CVla(copy_blk->cp_rowcount + 1, err_buf);
		IIlocerr(GE_HOST_ERROR, E_CO002F_LOG_WRITE_ERR, II_ERR, 
		     2, copy_blk->cp_logname, err_buf);
	    }
	    copy_blk->cp_logged++;
	}
    }
}

/*{
+*  Name: IIcplgclose - Close copy log file.
**
**  Description:
**	Close the log file used to collect bad copy rows.
**	If no rows were written to log, then delete it.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: OK, FAIL
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	04-sep-87 (puree)
**	    added severity flag to IIlocerr calls.
**	22-jan-90 (barbara)
**	    Fixed up call to LOfroms to pass correct sized buffer.
*/
static i4
IIcplgclose(II_CP_STRUCT *copy_blk)
{
    LOCATION	loc;
    char	locbuf[MAX_LOC +1];	/* Used to hold location string */
    i4		status;
 
    status = SIclose(copy_blk->cp_log_ptr);
    if (status)
    {
	IIlocerr(GE_HOST_ERROR, E_CO0030_LOG_CLOSE_ERR, II_ERR,
		 1, copy_blk->cp_logname);
	copy_blk->cp_warnings++;
	return (FAIL);
    }
 
    copy_blk->cp_log_ptr = NULL;
    if (copy_blk->cp_logged == 0)
    {
	STcopy( copy_blk->cp_logname, locbuf );
	LOfroms(PATH & FILENAME, locbuf, &loc);
	LOdelete(&loc);
    }
 
    return (OK);
}
 

/*{
+*  Name: IIcp_handler - Handle exceptions during COPY.
**
**  Description:
**	This handler is declared at the start of copy processing.
**
**	It handles no exceptions on its own, it always passes signal on to
**	next handler.
**
**	If some other exception handler decides to longjump out of copy
**	processing, this routine will be called with EX_UNWIND.  In this
**	case we will call IIcpcleanup() to clean up the copy before
**	exiting.
**
**  Inputs:
**	exargs		- exception handler argument, gives reason for signal
**
**  Outputs:
**	Returns: EXRESIGNAL to pass exception on to next handler.
-*
**  Side Effects:
**	none
**
**  History:
**	01-dec-1986	- Written (rogerk).
**	27-aug-87 (puree)
**	    fix problem with control-C.
**	26-feb-1990 (barbara)
**	    Don't turn off IICGC_COPY_MASK up here because IIcgc_read_results
**	    will test for that mask as part of fix to bug #7927.
*/
static STATUS
IIcp_handler(EX_ARGS *exargs)
{
    II_THR_CB	*thr_cb = IILQthThread();
    II_LBQ_CB	*IIlbqcb = thr_cb->ii_th_session;

    switch (exargs->exarg_num)
    {
	/*  If Control-C or unwound, clean up */
	case EXINTR:
	case EX_UNWIND:
	{
	    IIcpcleanup((II_CP_STRUCT *)thr_cb->ii_th_copy);
	    return (EXRESIGNAL);
	}
 
	/* For math exception */
	case EXFLTDIV:
	case EXFLTOVF:
	case EXFLTUND:
	case EXINTDIV:
	case EXINTOVF:
	case EXMNYDIV:
	case EXMNYOVF:
	case EXDECOVF:
	{
	    /* Check for ADF exception */
	 
	    IIlbqcb->ii_lq_adf->adf_errcb.ad_ebuflen = 0;
	    IIlbqcb->ii_lq_adf->adf_errcb.ad_errmsgp = (char *) NULL;
	    IIlbqcb->ii_lq_adf->adf_exmathopt = ADX_ERR_MATHEX;
	 
	    if (adx_handler(IIlbqcb->ii_lq_adf, exargs) != E_DB_OK)
	    {
		((II_CP_STRUCT *)thr_cb->ii_th_copy)->cp_status |= CPY_ERROR;
		return(EXCONTINUES);
	    }
	}
 
	/* For everything else, clean up and terminate copy */
	default:
	{
	    IIcpcleanup((II_CP_STRUCT *)thr_cb->ii_th_copy);
	    return (EXRESIGNAL);
	} 
    } 
}

/*{
+*  Name: IIcpdbgdump - Print out II_CP_STRUCT and II_CP_MAP structures
**
**  Description:
**
**  Inputs:
**	copy_blk	- The II_CP_STRUCT
**	copydbg_file	- name of file to dump structure info into.
**
**  Outputs:
-*
**  Side Effects:
**	Prints out data from the COPY data structures into the desired file.
**
**  History:
**	7-jan-1993 (mgw) Written
**	17-Jun-2004 (schka24)
**	    Avoid buffer overrun on filename.
**	15-Dec-2009 (kschendel) SIR 122974
**	    Dump updated data structures.
*/
static void
IIcpdbgdump( II_LBQ_CB *IIlbqcb, II_CP_STRUCT *cblk, char *copydbg_file )
{
    LOCATION	l;
    char	locbuf[MAX_LOC+1];
    FILE	*fp;
    char	buf[256];
    i4		i;

    /* open copydbg_file */
    STlcopy(copydbg_file, locbuf, sizeof(locbuf)-1);
    LOfroms(FILENAME, locbuf, &l);
    SIfopen(&l, "a", SI_TXT, 80, &fp);

    /* First print the query */
    SIfprintf(fp, "%s\n\n", IIlbqcb->ii_lq_gca->cgc_qry_buf.cgc_q_buf);

    /* Next, the structures */
    SIfprintf(fp, "II_CP_STRUCT cblk\n");
    SIfprintf(fp, "  cp_status (octal): %o\n", cblk->cp_status);
    if (cblk->cp_direction == CPY_FROM)
	STcopy("CPY_FROM", buf);
    else if (cblk->cp_direction == CPY_INTO)
	STcopy("CPY_INTO", buf);
    else
	STcopy("UNKNOWN", buf);
    SIfprintf(fp, "  cp_direction: %s\n", buf);
    SIfprintf(fp, "  cp_row_length: %d;   cp_tup_length: %d\n",
	cblk->cp_row_length, cblk->cp_tup_length);
    SIfprintf(fp, "  cp_num_domains: %d;   cp_num_groups: %d\n",
	cblk->cp_num_domains, cblk->cp_num_groups);
    SIfprintf(fp, "  cp_rbuf_size: %d;   worksize: %d;   readsize: %d\n",
	cblk->cp_rbuf_size, cblk->cp_rbuf_worksize, cblk->cp_rbuf_readsize);
    SIfprintf(fp, "  cp_log_size: %d;   cp_logname: %s\n",
	cblk->cp_log_size, cblk->cp_logname);
    SIfprintf(fp, "  cp_rowcount: %d\n", cblk->cp_rowcount);
    SIfprintf(fp, "  cp_truncated: %d\n", cblk->cp_truncated);
    SIfprintf(fp, "  cp_blanked: %d\n", cblk->cp_blanked);
    SIfprintf(fp, "  cp_warnings: %d\n", cblk->cp_warnings);
    SIfprintf(fp, "  cp_badrows: %d\n", cblk->cp_badrows);
    SIfprintf(fp, "  cp_logged: %d\n", cblk->cp_logged);
    SIfprintf(fp, "  cp_maxerrs: %d\n", cblk->cp_maxerrs);
    SIfprintf(fp, "  cp_filename: %s\n", cblk->cp_filename);
    if (cblk->cp_filetype == SI_BIN)
	STcopy("SI_BIN", buf);
    else if (cblk->cp_filetype == SI_TXT)
	STcopy("SI_TXT", buf);
    else if (cblk->cp_filetype == SI_VAR)
	STcopy("SI_VAR", buf);
    else
	STcopy("UNKNOWN", buf);
    SIfprintf(fp, "  cp_filetype: %s\n", buf);
    SIfprintf(fp, "  rowlog? %s;   at-eof? %s\n",
		cblk->cp_rowlog ? "Y" : "N",
		cblk->cp_at_eof ? "Y" : "N");
    SIfprintf(fp, "  convert? %s;   continue? %s;   program? %s\n",
		cblk->cp_convert ? "Y" : "N",
		cblk->cp_continue ? "Y" : "N",
		cblk->cp_program ? "Y" : "N");
    SIfprintf(fp, "  dbmserr? %s;   error? %s;  has-blobs? %s\n",
		cblk->cp_dbmserr ? "Y" : "N",
		cblk->cp_error ? "Y" : "N",
		cblk->cp_has_blobs ? "Y" : "N");

    for (i=0; i<cblk->cp_num_groups; i++)
    {
	II_CP_GROUP	*cgroup = &cblk->cp_copy_group[i];
	SIfprintf(fp, "  II_CP_GROUP cblk->cp_copy_group[%d]\n", i);
	if (cgroup->cpg_first_map == NULL)
	    SIfprintf(fp, "    First map: NONE (\"toss\" group)\n");
	else
	    SIfprintf(fp, "    First map: %d;  last map: %d\n",
		cgroup->cpg_first_map - cblk->cp_copy_map,
		cgroup->cpg_last_map - cblk->cp_copy_map);
	SIfprintf(fp, "    First attno: %d;   last attno: %d\n",
		cgroup->cpg_first_attno, cgroup->cpg_last_attno);
	SIfprintf(fp, "    Tup offset: %d;  tup length: %d;   row length: %d\n",
		cgroup->cpg_tup_offset, cgroup->cpg_tup_length,
		cgroup->cpg_row_length);
	SIfprintf(fp, "    unicode cv buffer is %sallocated, len %d\n",
		cgroup->cpg_ucv_buf == NULL ? "not " : "",
		cgroup->cpg_ucv_len);
	SIfprintf(fp, "    row-lob? %s;   tup-lob? %s;   validate? %s\n",
		cgroup->cpg_is_row_lob ? "Y" : "N",
		cgroup->cpg_is_tup_lob ? "Y" : "N",
		cgroup->cpg_validate ? "Y" : "N");
	SIfprintf(fp, "    varrows? %s;   compressed? %s\n",
		cgroup->cpg_varrows ? "Y" : "N",
		cgroup->cpg_compressed ? "Y" : "N");
    }

    for (i=0; i < cblk->cp_num_domains; i++)
    {
	II_CP_MAP	*cmap = &cblk->cp_copy_map[i];

	SIfprintf(fp, "  II_CP_MAP cblk->cp_copy_map[%d]\n", i);
	SIfprintf(fp, "    cp_rowlen: %d;   cp_fieldlen: %d;   cp_trunc_len: %d\n",
		cmap->cp_rowlen, cmap->cp_fieldlen, cmap->cp_trunc_len);
	SIfprintf(fp, "    cp_tuplen: %d\n", cmap->cp_tuplen);
	SIfprintf(fp, "    cp_attrmap: %d;   cp_groupoffset: %d\n",
		cmap->cp_attrmap, cmap->cp_groupoffset);
	SIfprintf(fp, "    cp_rowtype: %s\n",
				gtr_type(cmap->cp_rowtype, buf));
	SIfprintf(fp, "    cp_rowprec: %d\n", cmap->cp_rowprec);
	SIfprintf(fp, "    cp_tuptype: %s\n",
				gtr_type(cmap->cp_tuptype, buf));
	SIfprintf(fp, "    cp_tupprec: %d\n", cmap->cp_tupprec);
	SIfprintf(fp, "    cp_cvid: %d;   cp_cvlen: %d;   cp_cvprev: %d\n",
		cmap->cp_cvid, cmap->cp_cvlen, cmap->cp_cvprec);
	SIfprintf(fp, "    cp_cvfunc: %s\n",
		cmap->cp_cvfunc != NULL ? "set" : "NULL");

	SIfprintf(fp, "    cp_withnull: %d\n", cmap->cp_withnull);
	SIfprintf(fp, "    cp_nultype: %d;   cp_nullen: %d;   cp_nulprec: %d\n",
		cmap->cp_nultype, cmap->cp_nullen, cmap->cp_nulprec);
	if (cmap->cp_nuldata != (PTR) NULL)
	{
	    STlcopy(cmap->cp_nuldata, buf, min(255,cmap->cp_nullen));
	    SIfprintf(fp, "    cp_nuldata: %s\n", buf);
	}
	STlcopy(cmap->cp_delim, buf, cmap->cp_delim_len);
	if (buf[0] == '\t')
	    STcopy("\\t",buf);
	else if (buf[0] == '\n')
	    STcopy("\\n",buf);
	SIfprintf(fp, "    cp_isdelim: %d;   cp_delim: %s\n",
		cmap->cp_isdelim, buf);
	STcopy(cmap->cp_name, buf);
	if (strcmp(buf, "\t") == 0)
	    STcopy("\\t",buf);
	else if (strcmp(buf, "\n") == 0)
	    STcopy("\\n",buf);
	SIfprintf(fp, "    cp_name: %s\n", buf);
	SIfprintf(fp, "    with-null? %s;   valchk? %s;   csv? %s;   counted? %s\n",
		cmap->cp_withnull ? "Y" : "N",
		cmap->cp_valchk ? "Y" : "N",
		cmap->cp_csv ? "Y" : "N",
		cmap->cp_counted ? "Y" : "N");
	SIfprintf(fp, "    tuplob? %s\n",
		cmap->cp_tuplob ? "Y" : "N");

/*
** Not printed:
**  ADF_FN_BLK	cp_adf_fnblk;		ADF function block set up for
**					row/tuple conversion calls
*/
    }
    SIfprintf(fp, "-------------------------------------------------\n\n");
    SIclose(fp);
}

/*{
+*  Name: gtr_type - Trace type name mapping routine.
**
**  Description:
**
**  Inputs:
**	type		- Type id.
**
**  Outputs:
**	typename	- Type name mapped as:
**				type id : [nullable] type name
**
**	Returns:
**	    	Pointer to typename.
**	Errors:
**		If invalid type sets typename to DB_unknown_TYPE.
**
**  Side Effects:
-*	
**  History:
**	19-sep-1988	- Written for GCA tracing (ncg)
**	19-oct-1992 (kathryn)
**	    Added DB_LVCH_TYPE and DB_LBIT_TYPE as valid types for new 
**	    large object support.
**	jan-1993 (mgw)	- copied wholesale into iicopy.c for debugging.
**      03-Dec-2003 (gupsh01)
**          Modified copy code so that nchar and nvarchar types will now use
**          UTF8 to read and write from the text file for a formatted copy.
**	03-Aug-2006 (gupsh01)
**	    Added support for ANSI date/time types.
**	4-Dec-2009 (kschendel) SIR 122974
**	    Rip off ade's type-name table to bring it up to date.
*/

static char *type_name_list[] = {
/* 0 - 19 */
    NULL,	    NULL,	  NULL,	    "DB_DTE_TYPE",
    "DB_ADTE_TYPE", "DB_MNY_TYPE",  "DB_TMWO_TYPE", "DB_TMW_TYPE",
    "DB_TME_TYPE",  "DB_TSWO_TYPE", "DB_DEC_TYPE",  "DB_LOGKEY_TYPE",
    "DB_TABKEY_TYPE","DB_PAT_TYPE", "DB_BIT_TYPE",  "DB_VBIT_TYPE",
    "DB_LBIT_TYPE", "DB_CPN_TYPE",  "DB_TSW_TYPE",  "DB_TSTMP_TYPE",
/* 20 - 39 */
    "DB_CHA_TYPE",  "DB_VCH_TYPE",  "DB_LVCH_TYPE", "DB_BYTE_TYPE",
    "DB_VBYTE_TYPE","DB_LBYTE_TYPE","DB_NCHR_TYPE", "DB_NVCHR_TYPE",
    "DB_LNVCHR_TYPE","DB_LNLOC_TYPE","DB_INT_TYPE",  "DB_FLT_TYPE",
    "DB_CHR_TYPE",  "DB_INYM_TYPE", "DB_INDS_TYPE", "DB_LBLOC_TYPE",
    "DB_LCLOC_TYPE","DB_TXT_TYPE",  "DB_BOO_TYPE",   "DB_ALL_TYPE",
/* 40 - 47 */
    NULL,	    "DB_LTXT_TYPE", NULL,	    NULL,
    NULL,	    NULL,	    NULL,	    "DB_UTF8_TYPE"
};

static char *
gtr_type(i4 type, char *typename)
{
    char	*tn;

    STprintf(typename, ERx("%d : "), type); 
    if (type == CPY_DUMMY_TYPE)
    {
	STcat(typename, "Dummy");
	return typename;
    }
    if (type < 0)
    {
	STcat(typename, ERx("nullable "));
	type = abs(type);
    }
    tn = "unknown";
    if (type < sizeof(type_name_list)/sizeof(char *)
      && type_name_list[type] != NULL)
	tn = type_name_list[type];
    STcat(typename, tn);
    return typename;
}

/*{
+*  Name: IICPwemWriteErrorMsg - Write the given error message and text to user
**
**  Description:                 Takes the given ernum and calls IIlocerr
**                               with the given row number, column name,
**                               and trailing message. Converts the row
**                               number to a char string and copies the
**                               msg to a private buffer since msg is the
**                               result of an ERget() call and as such, can't
**                               be used as the input to an ERlookup (which
**                               is what IIlocerr will do to it).
**
**	For calling convenience, 0047 FILE READ and 0041 FILE WRITE
**	messages are translated into 004A PROG READ and 0049 PROG WRITE
**	messages if appropriate.  Easier to do it here than in the caller.
**
**  Inputs:
**	ernum		- Error message to be called
**	cblk		- Copy control block
**	col_name	- Current column name to be inserted into message
**	msg		- Descriptive text to be added to message
**
**  Outputs:
**	none
**
**  Returns: void
**
**  History:
**	jun-1993 (mgw)	- Written
*/
static void
IICPwemWriteErrorMsg(i4 ernum, II_CP_STRUCT *cblk, char *col_name, char *msg)
{
    char msgbuf[80];
    char err_buf[15];

    if (cblk->cp_program)
    {
	if (ernum == E_CO0041_FILE_WRITE_MSG_ERR)
	    ernum = E_CO0049_PROG_WRITE_MSG_ERR;
	else if (ernum == E_CO0047_FILE_READ_MSG)
	    ernum = E_CO004A_PROG_READ_MSG_ERR;
    }
    CVla(cblk->cp_rowcount+1, err_buf);
    STlcopy(msg, msgbuf, 80);	/* Can't count on ERget() buffer */
    IIlocerr(GE_LOGICAL_ERROR, ernum, II_ERR, 3, err_buf, col_name, msgbuf);
}

/*{
+*  Name: IICPvcValCheck - validate read in column value via adc_valchk()
**
**  Description:           Send the aligned read in column value to
**			   adc_valchk() to ensure that the value is legal.
**			   If it's a DB_CHR_TYPE (plus or minus) and there
**			   are non-printing characters, convert them to
**			   blanks and declare the value OK.
**
**  Inputs:
**	col_aligned	- aligned column value for validation
**	copy_map	- the copy map for this column
**	copy_blk	- the copy block for this table
**
**  Outputs:
**	none
**
**  Returns: OK, FAIL
**
**  History:
**	mar-1993 (mgw)	- Written
**	8-Sep-2008 (gupsh01)
**	    Fix usage of CMdbl1st for UTF8 and multibyte character sets
**	    which can can be upto four bytes long. Use CMbytecnt to ensure
**	    we do not assume that a multbyte string can only be a maximum
**	    of two bytes long. (Bug 120865)
*/
static STATUS
IICPvcValCheck( II_LBQ_CB *IIlbqcb, char *col_aligned, 
		II_CP_MAP *copy_map, II_CP_STRUCT *copy_blk )
{
    DB_DATA_VALUE	dbv;
    char		*str;
    STATUS		status;
    i4			iter, byte_cnt;

    dbv.db_data = (PTR) col_aligned;
    dbv.db_datatype = copy_map->cp_tuptype;
    dbv.db_length = copy_map->cp_tuplen;
    dbv.db_prec = copy_map->cp_tupprec;
    if ((status = adc_valchk(IIlbqcb->ii_lq_adf, &dbv)) != OK)
    {
	/*
	** If the value is invalid because of non-printing
	** characters in a C-type field, convert them to
	** blanks.
	*/
	if ((IIlbqcb->ii_lq_adf->adf_errcb.ad_errcode ==
			    E_AD1014_BAD_VALUE_FOR_DT) &&
	    ((copy_map->cp_tuptype == DB_CHR_TYPE) ||
	     (copy_map->cp_tuptype == -DB_CHR_TYPE)))
	{
	    for (str = dbv.db_data; *str != NULLCHAR; CMnext(str))
	    {
		/*
		** Convert any non-printing chars to blanks. If
		** the nonprinting char is a double-byte, then
		** put in two blanks.
		*/
		if (!CMprint(str))
		{
		    if (CMdbl1st(str))
		    {
			byte_cnt = CMbytecnt(str);
			for (iter = byte_cnt; iter > 1; iter--)
			  *str++ = ' ';
		    }
		    *str = ' ';
		}
  	    }
	    copy_blk->cp_blanked++;
	    status = OK;
	}
 
	if (status != OK)
	{
	    char err_buf[15];
	    CVla(copy_blk->cp_rowcount + 1, err_buf);
	    if (IIlbqcb->ii_lq_adf->adf_errcb.ad_errcode ==
			    E_AD1014_BAD_VALUE_FOR_DT)
		IIlocerr(GE_DATA_EXCEPTION, E_CO0032_BAD_DATA,
			 II_ERR, 2, copy_map->cp_name, err_buf);
	    else
		IIlocerr(GE_DATA_EXCEPTION, E_CO0023_COPY_INTRNL_ERR,
			 II_ERR, 1, err_buf);
	    copy_blk->cp_warnings++;
	}
    }
    return (status);
}

/*{
+*  Name: IICPdecompress - Decompress for bulk COPY
**
**  Description: This routine decompresses a tuple which contains no large
**               objects.
**  
**  Inputs:
**      rdesc     -     Row descriptor.
**	first_attno, last_attno - First and last attributes to process
**	bptr      -     The compressed tuple.
**      bytesleft -     Bytes left in GCA buffer.
**      tuple_ptr -     Pointer to the decompressed tuple.
**
**  Outputs:
**	dif       -     Pointer to the difference in byte length between the 
**                      compressed and decompressed tuple.
**      partial   -     Indicates this is a partial tuple.
**
**  Returns: void
**
**  History:
**	04-jun-96 (loera01)
**	    Created.
**      13-aug-96 (loera01)
**          Fixed memory leaks by setting decompress_buf to passed tuple_ptr
**          argument.
**	21-Dec-01 (gordy)
**	    GCA tuple descriptor is now only available for COPY FROM, so
**	    replaced cgc parameter with a ROW_DESC.
**      03-Dec-2003 (gupsh01)
**          Modified copy code so that nchar and nvarchar types will now use
**          UTF8 to read and write from the text file for a formatted copy.
**	14-Dec-2009 (kschendel) SIR 122974
**	    Caller passes start and end attributes now.
*/
static void 
IICPdecompress( ROW_DESC *rdesc, i4 first_attno, i4 last_attno,
	PTR bptr, i4  bytesleft, i4  *dif, PTR decompress_buf, bool *partial )
{
    i4                  size;
    DB_DATA_VALUE	*cur_att;
    i4                  compressed_type;
    i4                  i;
    i4			att_type;

    *dif = 0;
    *partial = FALSE;

    for( i = first_attno; i <= last_attno; i++ ) 
    {
	cur_att = (DB_DATA_VALUE *)&rdesc->rd_gca->rd_gc_col_desc[i].rd_dbv;
#ifdef axp_osf
	/* Huh??? */
	att_type = I2CAST_MACRO(&cur_att->db_datatype);
#else
	att_type = cur_att->db_datatype;
#endif

        if (! bytesleft)
        {
            *partial = TRUE;
            break; 
        }
        switch (abs(att_type))
        {
	    case DB_NVCHR_TYPE:
            case DB_TXT_TYPE:
            case DB_VCH_TYPE:
            case DB_UTF8_TYPE:
            case DB_VBYTE_TYPE:
            case DB_LTXT_TYPE:
            {
	        if (abs(att_type) == DB_NVCHR_TYPE)
                    compressed_type = att_type < 0 ?
	            IICGC_NVCHR : IICGC_NVCHRN;
                else
                    compressed_type = att_type < 0 ?
	            IICGC_VCH : IICGC_VCHN;
		if (bytesleft == sizeof(char))
		{
		  /* fragmented size */
		  *partial = TRUE;
		  break;
		}
	        size = IIcgc_find_cvar_len((PTR)bptr, compressed_type);
                MEcopy(bptr, size, decompress_buf);
                if (att_type < 0)
                {
                    decompress_buf[cur_att->db_length-1]
                    = bptr[size-1];
                }
		if (bytesleft < size)
		{
		  *partial = TRUE;
		  break;
		}
                bytesleft -= size;
                bptr += size;
                *dif += cur_att->db_length - size;
                decompress_buf += cur_att->db_length;
                break;
            }
            default:
            {
		if (bytesleft < cur_att->db_length)
		{
		  *partial = TRUE;
		  break;
		}
                MEcopy(bptr, cur_att->db_length, decompress_buf);
                bytesleft -= cur_att->db_length;
                bptr += cur_att->db_length;
                decompress_buf += cur_att->db_length;
                break;
            }
        } /* switch */

	if (*partial) break;
    }
}

/*{
+*  Name: IIcpdbgopen - Open copy debug file
**
**  Description:
**	Open copy debug file.
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: nothing
-*
**  Side Effects:
**	none
**
**  History:
**      24-jan-2005 (stial01)
**          Created from IIcplgopen
*/
static void
IIcpdbgopen(II_CP_STRUCT *copy_blk)
{
    LOCATION	loc;
    char	locbuf[MAX_LOC +1];	/* Used to hold location string */
 
    STcopy( copy_blk->cp_dbgname, locbuf );
    LOfroms(PATH & FILENAME, locbuf, &loc);
 
    if (SIfopen(&loc, "a", SI_TXT, 80, &copy_blk->cp_dbg_ptr) != OK)
	copy_blk->cp_dbg_ptr = NULL;
    else
    {
	SIfprintf(copy_blk->cp_dbg_ptr, "Open copy debug file\n");
    }
}

/*{
+*  Name: IIcpdbgclose - Close copy debug file.
**
**  Description:
**	Close the copy debug file 
**
**  Inputs:
**	copy_blk	- copy control block
**
**  Outputs:
**	Returns: nothing
-*
**  Side Effects:
**	none
**
**  History:
**      24-jan-2005 (stial01)
**          Created from IIcplgclose
*/
static void
IIcpdbgclose(II_CP_STRUCT *copy_blk)
{
    i4		status;
 
    if (copy_blk->cp_dbg_ptr)
    {
	SIfprintf(copy_blk->cp_dbg_ptr, "Closing copy debug file\n");
	status = SIclose(copy_blk->cp_dbg_ptr);
    }
    copy_blk->cp_dbg_ptr = NULL;
}
