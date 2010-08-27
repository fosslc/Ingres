/*
** Copyright (c) 1986, 2004 Ingres Corporation
*/

/*
**  Name: iicopy.h - defines structures used by libq to execute COPY command.
**
**  Description:
**	This file describes the structures that hold information needed
**	to execute the COPY command. 
**
**  History:
**	01-dec-1986	- Written (rogerk)
**	26-oct-87 (puree)
**	    change CPY_VCHLEN from 4 to 5 to support longer varchar.
**	29-mar-1991 (barbara)
**	    Added II_CP_USR struct to keep track of variable length rows
**	    copied from program.
**	06-aug-1991 (teresal)
**	    Modify II_CP_MAP to save the type of the 'with null' constant.
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-jan-2004 (somsa01)
**	    Updated prototypes of BLOB functions to account for
**	    "COPY INTO/FROM... PROGRAM" syntax.
**      24-jan-2005 (stial01)
**          Added cp_dbgname, cp_dbg_ptr for COPY tracing. (b113792)
**	30-Nov-2009 (kschendel) SIR 122974
**	    Remove declarations of routines that are local to iicopy.c.
**	    Lots of changes to copy map and ii-cp-struct for new buffering,
**	    CSV handling, performance improvements.  Invent notion of
**	    column groups to allow LOB and non-LOB copy to work the same.
*/

/* varchar length specifier as 5-byte ascii value */

# define		CPY_VCHLEN	5

/*}
+*  Name: II_CP_MAP - 
**
**  Description:
**
**	This structure defines one column to be copied.  It is derived
**	from the GCA copy map that is sent from the server to the front-
**	end.
**
**	The table column's attributes are in cp_tuptype, cp_tupprec,
**	cp_tuplen.  The COPY-file value attributes are cp_rowtype, cp_rowprec,
**	cp_rowlen.  The FI id to convert from one to the other will be
**	in cp_cvid.
**
**	Some specific variations and special cases:
**	* The pure-binary copy, ie copy table() into/from, will list all
**	  columns with cp_cvid set to ADI_NOFI.
**
**	* All other copies will have a valid cp_cvid (unless it's a dummy,
**	  of course).  This is true even if the table and file datatypes
**	  are identical, e.g. int_column = integer will still pass the
**	  id of the I_TO_I FI. (If this sounds silly, it is;  but the
**	  situation is actually pretty rare.  One usually sees either full
**	  binary copy(), or character forms on the file side.)
**
**	* WITH NULL (no value) has cp_withnull TRUE and a nullable row type.
**	  This form cannot occur for variable width row types.
**	  WITH NULL (value) has cp_withnull TRUE and a non-nullable row type.
**	  Do not make the mistake of thinking that cp_nullen == 0 implies
**	  the nullable form, it could be WITH NULL ('').
**
**	For the COPY FROM direction, we'll decipher our way to the actual
**	coercion function call (probably an adu function), and use that
**	directly.  This avoids a bunch of adf_func double-checking overhead.
**	This may get extended to COPY INTO later, we'll see.
-*
**  History:
**	01-dec-1986	- Written (rogerk)
**	24-feb-88 (puree)
**	    rearrange for word alignment.  Added 2 bytes to cp_name
**	    for the null terminator.
**	06-aug-1991 (teresal)
**	    Add a data type for the 'with null' constant value.
**	    A new COPY map being sent from the server now
**	    sends the null value in a DBV form.  For now,
**	    cp_nultype is the same as cp_rowtype.
**	23-apr-1996 (chech02)
**	    Add function prototypes for windows 3.1 port.
**	16-aug-1996 (thoda04)
**	    Corrected function prototypes for windows 3.1 port.
**	24-Jun-2005 (schka24)
**	    Added unicode normalization work buffer.
**	3-Dec-2009 (kschendel) SIR 122974
**	    Many additions: truncation check length, direct coercion call,
**	    more convenience bools, CSV delimiter.
**	18-Aug-2010 (kschendel) b124272
**	    Add a "normalize me" bool.
*/

typedef struct {
    PTR		cp_normbuf;		/* Unicode normalization work buffer */
    PTR		cp_srcbuf;		/* Unicode copy column data */
    PTR		cp_nuldata;		/* ptr to constant value for null.
					** Only for cp_withnull TRUE and
					** cp_rowtype not nullable, see below */
    DB_STATUS	(*cp_cvfunc)();		/* COPY FROM: coercion function */
    i4		cp_rowlen;		/* domain length as specified in 
					** the COPY command. ADE_LEN_UNKNOWN
					** if variable length domain */
    i4		cp_fieldlen;		/* actual field length in the copy
					** file. */
    i4		cp_tuplen;		/* corresponding attribute length */
    i4		cp_trunc_len;		/* If >0, effective available length for
					** char-type column (for truncation
					** check) */
    i4		cp_attrmap;		/* corresponding attribute number */
    i4		cp_groupoffset;		/* Offset within tuple relative to the
					** start of the column group that the
					** column belongs to. */
    DB_DT_ID	cp_rowtype;		/* Domain (column) type in the copied
					** row.  Nullable only if WITH NULL
					** (and no with-null value). */
    i2		cp_rowprec;		/* domain decimal precision */
    DB_DT_ID	cp_tuptype;		/* corresponding attribute type */
    i2		cp_tupprec;		/* corresponding attribute precision */
    ADI_FI_ID	cp_cvid;		/* adf function id for conversion */
    i4		cp_cvlen;		/* length of result of function id */
    i2		cp_cvprec;		/* precision of result of function id */
    DB_DT_ID	cp_nultype;		/* type of constant value for null */
    /* The backend sends a cp_nullen that corresponds to cp_nultype (and
    ** cp_nuldata).  This might be something inconvenient, like a
    ** DB_TEXT_STRING;  and, it might have an inconvenient length, like zero.
    ** Copy initialization rearranges things so that after initialization,
    ** cp_nullen is the bare value length, ie just data not including
    ** DB_TEXT_STRING or similar overhead;  cp_nuldata is likewise, just
    ** the data;  cp_withnull is TRUE if there's any sort of WITH NULL
    ** clause;  cp_rowtype is nullable if there is no null replacement value,
    ** ie the clause was "WITH NULL" (meaning output a trailing null
    ** indicator).  cp_withnull TRUE, cp_rowtype non-nullable, cp_nullen zero
    ** is "WITH NULL ('')", the empty string.
    */
    i4		cp_nullen;		/* Length of null replacement,
					** if any (see above) */
    i2		cp_nulprec;		/* precis'n of constant val for null */
    i2		cp_isdelim;		/* is there a delimiter: 0 = no,
					** 1 = specified delimiter in cp_delim,
					** -1 = default delimiter (any of comma,
					** tab, or newline).  Also see cp_csv.
					*/
    i2		cp_delim_len;		/* Effective length of delimiter.
					** Delimiters are one "character",
					** but in doublebyte or unicode might
					** be two octets.  delim_len range is
					** 0 to 2.
					*/
    ADF_FN_BLK	cp_adf_fnblk;		/* ADF function block set up for
					** row/tuple conversion calls */
    bool	cp_withnull;		/* TRUE if "with null" specified */
    bool	cp_valchk;		/* do we need value checking */
    bool	cp_csv;			/* TRUE if CSV / SSV style field
					** (implies variable length char-type).
					** Delim is set to , ; or newline as
					** appropriate, by copy front-end.
					** Includes quoted-string processing.
					*/
    bool	cp_ssv;			/* TRUE if cp_csv and it was SSV.
					** COPY INTO needs this to decide when
					** to quote if the delimiter is
					** newline (ie last column). */
    bool	cp_counted;		/* Convenience indicator for "counted"
					** row-types, ie varchar, utf8, and
					** byte varying. */
    bool	cp_tuplob;		/* TRUE if tuple-value is long type */
    bool	cp_unorm;		/* TRUE if column data needs to be
					** normalized.  Usually set for nchar
					** and nvarchar, but can be set for
					** ordinary string-like columns when
					** the character set is UTF8.
					*/
    char	cp_name[GCA_MAXNAME+2];	/* domain name */
    char	cp_delim[CPY_MAX_DELIM+2];/* domain delimiter; the +2 is to
					** assure multibyte delimiter compares
					** don't overrun and match by
					** accident.  CPY_MAX_DELIM is 2. */
} II_CP_MAP;


/*
** Name: II_CP_GROUP - Column group structure for COPY
**
** Description:
**
**	COPY operates by column groups.  A column group is basically a
**	fragment of the entire table tuple that can be filled in (FROM)
**	or sent to the output (INTO) without any buffering dumps caused
**	by LOB's.  So, a column group is either one LOB column, or a
**	sequence of columns in between LOB's.  If there are no LOB's in
**	the table, there will only be one group.
**
**	The COPY setup allows row domains to be reordered within a group,
**	but not across groups.  (Another way to say this is that you can
**	reorder domains in between LOB's but not across LOB's, nor can you
**	reorder LOB's.)  An error is produced if the user tries this.
**
**	When doing formatted (non-binary) COPY, ie with an explicit
**	column list, it may be that a group has one or more columns
**	omitted from the row list.  This is OK, we simply supply
**	empty/null values (COPY FROM), or ignore them (COPY INTO).
**
**	It's also possible that the only row domains that belong in a
**	group are DUMMY domains. In fact, it's possible to have a
**	group that contains no columns, but does contain (only) DUMMY
**	domains!  (Such a group would have tuple length zero but not
**	be a LOB group either.)  It's equally possible to have a
**	group that is the reverse, has columns but no domains.  (e.g.
**	a group covering copy-into columns which are all omitted from
**	the output list.)  A no-domains group will have null cmap pointers.
**
** History:
**	11-Dec-2009 (kschendel) SIR 122974
**	    Invent the column group data structure to help unify LOB and
**	    non-LOB COPY.
*/

typedef struct {
    II_CP_MAP	*cpg_first_map;		/* First cmap belonging to group */
    II_CP_MAP	*cpg_last_map;		/* Last cmap belonging to group (could
					** just as easily be # of cmaps).
					** If first/last are NULL, the entire
					** group was omitted from the domain
					** list. */
    char	*cpg_ucv_buf;		/* LOB only: UTF8 conversion buf */
    i4		cpg_ucv_len;		/* Length of cpg_ucv_buf */

    i4		cpg_row_length;		/* Row (file) length total for cmaps
					** in this group. COPY INTO sets this
					** to the generated length if varrows
					** or LOB.
					** COPY FROM sets this to the actual
					** length if !row-lob. */

    i4		cpg_tup_offset;		/* Offset of first column in group
					** relative to the start of the
					** tuple, ignoring LOBs. */
    i4		cpg_tup_length;		/* Length of group, zero if LOB or
					** an all-dummy group. */
    i2		cpg_first_attno;	/* First column # in group, 0 origin */
    i2		cpg_last_attno;		/* Last column # in group */
    i1		cpg_refill_control;	/* Initial refill control setting,
					** see values later on */
    bool	cpg_is_row_lob;		/* Row form is LOB (tuple form must
					** be LOB also).  For code convenience,
					** row-lob is also set if LOB is not
					** in the file (read-and-toss if INTO,
					** send empty/null LOB if FROM).
					*/
    bool	cpg_is_tup_lob;		/* Tuple form is LOB (row form might
					** not be) */
    bool	cpg_validate;		/* Binary COPY FROM: some tuple values
					** in the group need post-validation
					*/
    bool	cpg_varrows;		/* TRUE if variable length row;
					** FALSE if fixed length, or none */
    bool	cpg_compressed;		/* TRUE for COPY INTO if any (non-LOB)
					** columns in this group are susceptible
					** to GCA compression. */
} II_CP_GROUP;


/*}
+*  Name: II_CPY_BUFH
**
**  Description:    This structure is used for keeping track of GCA buffer
**		    used for sending tuples to the DBMS.
-*
**  History:
**	29-sep-87 (puree)
**	    writtetn for GCA architecture
**	3-Jan-2010 (kschendel) SIR 122974
**	    Add end-of-row indicator.
*/

typedef struct {
    DB_DATA_VALUE	cpy_dbv;
    i4		cpy_bytesused;
    bool	end_of_data;	/* TRUE if last thing written into the msg
				** buffer was the end of a row; if the buffer
				** is sent now, mark the send with EOD.
				** (FROM direction only) */

} II_CPY_BUFH;			/* tuple buffer header */

/*}
+*  Name: II_CP_STRUCT
**
**  Description:
-*
**  History:
**	01-dec-1986	- Written (rogerk)
**	24-feb-88 (puree)
**	    rearrange for better word alignment.
**	29-mar-1991 (barbara)
**	    Added II_CP_USR struct to keep track of variable length rows
**	    copied from program.
**	30-Nov-2009 (kschendel) SIR 122974
**	    Rearrange the way COPY FROM row buffering works.
**	    Add COPY PROGRAM getter/putter so copy doesn't have to pass it
**	    around as a parameter all the time.  Add many convenience bools.
**	3-Jan-2010 (kschendel) SIR 122974
**	    Delete redundant not-flushed bool.
**	10-Mar-2010 (kschendel) SIR 122974
**	    Improve buffer handling a wee bit by eliminating the count, use
**	    an end-of-data pointer instead.
*/

typedef struct {
    bool	cp_rowlog;		/* is there a log for bad rows */
    bool	cp_at_eof;		/* Input file reached EOF */
    bool	cp_convert;		/* is row/tuple conversion needed */
    bool	cp_continue;		/* should we continue on error */
    bool	cp_program;		/* is this an in-memory copy */
    bool	cp_dbmserr;		/* is there an error on the dbms side */
    bool	cp_error;		/* is there a fatal error */
    bool	cp_has_blobs;		/* are there any LOBs involved? */
    bool	cp_sent;		/* FROM: we probably flushed the GCA
					** buffer while doing this row.
					** For controlling error polling. */

    /* Specialty control for cp_refill to micro-optimize it... */
    i4	cp_refill_control;	/* See below for states */

    i4	cp_status;		/* status bits - defined in COPY.H */
    i4	cp_direction;		/* CPY_FROM or CPY_INTO */
    i4	cp_row_length;		/* width of copy row - 0 if variable */
    i4	cp_tup_length;		/* width of tuple in copy table.
				** Beware!  lob's have zero length, an all-lob
				** table row has zero cp_tup_length. */
    i4	cp_num_domains;		/* number of domains in copy file */
    i4	cp_num_groups;		/* number of column groups. */
    i4	cp_rowcount;		/* count of rows processed */
    i4	cp_truncated;		/* count of truncated domains */
    i4	cp_blanked;		/* domains with control chars blanked */
    i4	cp_warnings;		/* count of warnings given */
    i4	cp_badrows;		/* rows that couldn't be processed */
    i4	cp_logged;		/* count of rows written to log file */
    i4	cp_maxerrs;		/* errors allowed till terminate copy */
    i4	cp_filetype;		/* user-specified filetype */
    i4	cp_rbuf_size;		/* total size of cp_rowbuf below */
    i4	cp_rbuf_worksize;	/* Size of prefix (workspace) in cp_rowbuf */
    i4	cp_rbuf_readsize;	/* Size of read-buffer part of cp_rowbuf */
    i4	cp_log_size;		/* Size of cp_log_buf */
    i4	cp_log_left;		/* Bytes left in cp_log_buf (can go negative) */
    i4	cp_pollyet;		/* FROM: rough count of to-backend flushes,
				** attempting to count only one per row.
				** This is used to decide how often to poll
				** the backend for dbms errors.
				** Reset to zero after each poll. */
    char	*cp_filename;		/* copy file name */
    FILE	*cp_file_ptr;		/* copy file descriptor */
    char	*cp_logname;		/* error row log name */
    FILE	*cp_log_ptr;		/* error row log descriptor */
    char	*cp_dbgname;		/* debug log name */
    FILE	*cp_dbg_ptr;		/* debug file descriptor */
    i4		(*cp_usr_row)(i4 *, PTR, i4 *); /* COPY PROGRAM getter
					** or putter routine */
    II_CP_MAP	*cp_copy_map;		/* file domain descriptors */
    II_CP_GROUP	*cp_copy_group;		/* Column group descriptors */
    II_CPY_BUFH	*cp_cpy_bufh;		/* Copy tuple-buffer tracker */

    /* For COPY FROM: cp_rowbuf is the start of the buffer.  The start of
    ** the buffer is work-space for split rows, of length cp_rbuf_worksize.
    ** From there to the end of cp_rowbuf is an area of size cp_rbuf_readsize
    ** which is used as a read buffer.  cp_rbuf_worksize is aligned such that
    ** the read buffer part is 8-byte (or ALIGN_RESTRICT if larger) aligned.
    ** Note: if copy from program, no conversion, and fixed width input rows,
    ** there is no row-buffer;  copy reads directly into the tuple area.
    ** All other cases use a row-buffer, even copy () from file (for buffering).
    */
    PTR		cp_rowbuf_all;		/* The start of the buffer including
					** hidden guard space. */
    PTR		cp_rowbuf;		/* buffer for copy rows */
    PTR		cp_readbuf;		/* Read-buffer within cp_rowbuf */

    /* Caution, important that these are unsigned char, since we can be
    ** reading arbitrary binary data. */
    u_char	*cp_valstart;		/* Where the cur value starts */
    u_char	*cp_rowptr;		/* Current char position in rowbuf */
    u_char	*cp_dataend;		/* Last valid char in rowbuf, plus 1 */
    PTR		cp_tupbuf;		/* buffer for one tuple; NULL
					** if cp_tup_length is zero.  Reflects
					** non-LOB columns only. */
    PTR		cp_zerotup;		/* initialized tuple with no data; NULL
					** if all-LOB tuple. Same length as
					** cp_tupbuf, ie cp_tup_length. */
    PTR		cp_buffer;		/* temporary buffer */
    PTR		cp_decomp_buf;		/* COPY INTO only: holding area
					** used when tuple splits messages.
					** Size is cp_tup_length. */
    PTR		cp_log_buf;		/* Line image for COPY FROM logging */
    PTR		cp_log_bufptr;		/* Line image for COPY FROM logging */
} II_CP_STRUCT;


/* Macro for reading a byte out of the row buffer (COPY FROM).
** Returns i4:  either the next byte, or COPY_EOF or COPY_FAIL.
** rowptr and (possibly) dataend are updated as side effects.
** dataend, rowptr correspond to, but need NOT be identical to,
** cp_dataend and cp_rowptr.  Note that we allow the cp_dataend
** and cp_rowptr members to get out of date!
** IT IS UP TO THE CALLER TO UPDATE cp_dataend and cp_rowptr at some
** appropriate time, from the updated dataend/rowptr.
**
** For gcc, hint that the *rowptr++ branch is the expected one.  This is
** just an optimization, not essential.
*/
#if defined(__GNUC__) && __GNUC__ >= 4
# define CP_GETC_MACRO(dataend,rowptr,copy_blk) \
    (__builtin_expect(rowptr >= dataend,0) ? cp_refill(&dataend, &rowptr, copy_blk) : *rowptr++)
#else
# define CP_GETC_MACRO(dataend,rowptr,copy_blk) \
    (rowptr >= dataend ? cp_refill(&dataend, &rowptr, copy_blk) : *rowptr++)
#endif

/* Macro for backing up rowptr. Does not touch anything
** in II_CP_STRUCT, just backs up the supplied variable.
*/
#define CP_UNGETC_MACRO(rowptr) \
    (--rowptr)

/* EOF and FAILURE indicators from CP_GETC_MACRO (or the cp_refill
** routine it depends on).  The standard FAIL indicator is unsuitable.
*/
#define COPY_EOF -1
#define COPY_FAIL -2

/* "refill control" states to help ever so slightly optimize things. */
#define COPY_REFILL_NORMAL	0	/* Nothing special */
#define COPY_REFILL_EOF_FAILS	1	/* EOF is not allowed, return FAIL */
#define COPY_REFILL_EOF_NL	2	/* EOF invents a newline (for csv) */


/* Macro for copying one character into the copy-from logging line image */
#define CP_LOGCH_MACRO(ch, copy_blk) \
    if (copy_blk->cp_rowlog && --copy_blk->cp_log_left >= 0) *copy_blk->cp_log_bufptr++ = (ch);

/* Macro for copying N bytes from cp_valstart upto (but not including)
** rowptr into the copy-from logging line image.
*/
#define CP_LOGTO_MACRO(rowptr, copy_blk) \
    if (copy_blk->cp_rowlog) IIcplgcopyn(rowptr, copy_blk);
