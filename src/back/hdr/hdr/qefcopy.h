/*
**Copyright (c) 2010 Ingres Corporation
*/

/**
** Name: QEFCOPY.H - copy control blocks
**
** Description:
**	Description of control block used to sequence the COPY operation.
**	This block is filled in by PSF and then used by the Sequencer to
**	control the copy operation and sent to QEF to retrieve or append
**	rows. Described here is the mail QEU_COPY block as well as the
**	QEU_CPFILED which describes a copy file, and the QEU_CPDOMD
**	struct, which describes a copy file domain.
**
** History: $Log-for RCS$
**	05-dec-86 (rogerk)
**	    Moved from QEFQEU.H
**	06-apr-87 (rogerk)
**	    Added fields for new error handling features.
**	29-apr-87 (puree)
**	    Add fields for COPY WITH NULL in QEU_CPDOMD.
**	20-sep-89 (jrb)
**	    Add fields for precision during COPY for DECIMAL project.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-oct-2002 (horda03) Bug 77895
**          Added QEU_MISSING_COL typedef to store information on
**          column default values for columns not specified in a
**          COPY FROM command. The list of missing column default
**          values has been added to the QEU_COPY structure.
**      18-jun-2004 (stial01)
**          Added mc_dv, mc_dtbits to QEU_MISSING_COL (b112521)
**	13-Jun-2006 (kschendel)
**	    Added sequence poop to QEU_MISSING_COL.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _QEU_CPFILED	QEU_CPFILED;
typedef struct _QEU_CPDOMD	QEU_CPDOMD;
typedef struct _QEU_CPATTINFO	QEU_CPATTINFO;
typedef struct _QEU_MISSING_COL QEU_MISSING_COL;

/*
**  Defines of other constants.
*/

#define	QEU_SYNC_BLOCKS		20	/* default sync_frequency during copy */


/*}
** Name: QEU_CPDOMD - copy file domain descriptor
**
** Description:
**
** History:
**	27-may-86 (daved)
**          written
**	29-apr-87 (puree)
**	    Add fields for COPY WITH NULL.
**	20-sep-89 (jrb)
**	    Add fields for precision for DECIMAL project.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C 
**	    compiler warnings.
*/
struct _QEU_CPDOMD
{
    QEU_CPDOMD	*cp_next;		/* next domain descriptor */
    char	cp_domname[DB_ATT_MAXNAME]; /* domain name */
    DB_DT_ID	cp_type;		/* domain type */
    i4	cp_length;		/* domain length */
    i4	cp_prec;		/* domain precision */
    bool	cp_user_delim;		/* TRUE if there is a delimiter */
    char	cp_delimiter[CPY_MAX_DELIM];	/* delimiter char */
    i4	cp_tupmap;		/* tuple attribute corresponding to
					** this file row domain
					*/
    ADI_FI_ID	cp_cvid;		/* conversion id to map row to tuple */
    i4	cp_cvlen;		/* conversion result length */
    i4	cp_cvprec;		/* conversion result precision */
    bool	cp_withnull;		/* TRUE if "with null" specified */
    i4	cp_nullen;		/* length of the value for null */
    i4	cp_nulprec;		/* precision of the value for null */
    PTR		cp_nuldata;		/* ptr to the value for null */
    /* The data in cp_nulldbv below is only used by the backend grammar and 
       is never sent to the front end */
    DB_DATA_VALUE cp_nulldbv;		/* raw value for null */
};

/*}
** Name: QEU_CPATTINFO - useful attribute information
**
** Description: 
**	This structure contains information about attributes that need
**	special QEF processing in COPY, typically because their 
**	formatting has to be handled in the DBMS, e.g export conversion or
**	defaults.
**
** History:
**	14-jul-93 (robf)
**	    Created
*/
struct _QEU_CPATTINFO 
{
    QEU_CPATTINFO *cp_next;		/* next attribute descriptor */
    DB_DT_ID	cp_type;		/* attribute type */
    i4	cp_length;		/* attribute length */
    i4	cp_prec;		/* attribute precision */
    i4	cp_tup_offset;		/* attribute offset in DMF tuple */
    i4	cp_ext_offset;		/* attribute offset in external tuple */
    i4	cp_attseq;		/* attribute sequence */
    i4	cp_flags;		/* special flags */
#define QEU_CPATT_EXP_INP      0x01       /* export/inport processing needed */
} ;
	

/*}
** Name: QEU_CPFILED - copy file descriptor
**
** Description:
**
** History:
**     27-may-86 (daved)
**          written
*/
struct _QEU_CPFILED
{
    i4	cp_filedoms;		/* num of attributes in file row */
    char	*cp_filename;		/* file name */
    QEU_CPDOMD	*cp_fdom_desc;		/* points to list of domain descs */
    QEU_CPDOMD	*cp_cur_fdd;		/* currently used domain desc */
};

/*
** Name: QEU_MISSING_COL - Missing Column information.
**
** Description :
**
**      Information about default values for columns
**      not specified in a COPY FROM command.
**
**	mc_dv's db_data pointer will point at the actual data item
**	to be supplied as a default.  Space for the data item is
**	allocated from the same stream as the QEU_MISSING_COL structure;
**	we can get away with that because COPY doesn't really generate
**	query plans, there is no chance of sharing these things.
**	The data item is bit-copied into the row in the proper place.
**
**	If the default is a constant, the parser can set up the value
**	at parse time.  For non-constants, such as sequences, the
**	runtime has to set up the value.  For sequence defaults
**	specifically, the new sequence values are fetched at the
**	start of each row, and coerced into the missing-value data
**	item for the column.  The mc_seqdv member contains the sequence
**	datatype and pointer to the native sequence value for coercion.
**
** History :
**      21-Oct-2002 (horda03)
**         Created.
**	13-Jul-2006 (kschendel)
**	    Add sequence default support; remove redundant mc_data.
*/
struct _QEU_MISSING_COL
{
    QEU_MISSING_COL  *mc_next;
    i4               mc_tup_offset;
    i4               mc_attseq;
    DB_DATA_VALUE    mc_dv;		/* Column datatype, default value */
    i4		     mc_dtbits;
    DB_DATA_VALUE    mc_seqdv;		/* Only if sequence default */
};

/*}
** Name: QEU_COPY - copy control block
**
** Description:
**      This control block contains the information necessary to perform
** the copy operation.
**
**	Ideally, this data structure should only contain data needed for
**	controlling the COPY across facilities.  It's created by PSF,
**	used by the sequencer to communicate with the front-end bits,
**	and used by QEF.  Data that is strictly local to QEF can reside
**	in a separate control block, see qeucopy.c.
**
** History:
**     27-may-86 (daved)
**          written
**     06-apr-87 (rogerk)
**	    added qeu_logname and qeu_maxerrs for new error handling features.
**     14-sep-87 (rogerk)
**          added qeu_load field which signifies to use load rather than puts.
**     14-jul-93 (robf)
**	    added qeu_ext_length to indicate how big the external tuple size is
**	25-Apr-2004 (schka24)
**	    Replace QEU_CB with pointer to copy control info.
**	25-oct-2009 (stephenb)
**	    Add fields for batch processing optimization.
**	19-Mar-2010 (kschendel) SIR 123448
**	    Delete unused fields, move "load" flag and tmptuple to QEU local.
**	25-Mar-2010 (kschendel) SIR 123485
**	    Forgot to delete qeu-moving-data, do that.  Comment updates.
**	    Add LOB couponify/redeem state variables formerly in sequencer
**	    session control block.
**	15-feb-2010 (toumi01) SIR 122403
**	    For encryption add qeu_tup_physical.
**	12-Apr-2010 (kschendel) SIR 123485
**	    Add base table access ID for COPY FROM, for use with blobs.
**	26-jun-2010 (stephenb)
**	    Add qeu_cbuf, qeu_cptr, qeu_csize, qeu_cleft, qeu_load_buf,
**	    and qeu_insert_data to support buffering of batch insert 
**	    rows for copy
*/
struct _QEU_COPY
{
    i4			qeu_stat;           /* input specification status */
    i4			qeu_direction;      /* copy into or from */
    DB_TAB_ID		qeu_tab_id;         /* table identification */
    DB_TAB_TIMESTAMP	qeu_timestamp;	    /* timestamp of relation */

    /* qeu_count is running total of whole tuples successfully copied.
    ** maintained by SCF for copy-into and QEF for copy-from.
    ** (For bulk-load, qeu_count is only updated at the end.)
    */
    i4		qeu_count;          /* number of tuples */
    i4		qeu_duptup;         /* number of duplicate tuples */
    i4		qeu_tup_length;	    /* width of tuples being copied */
    i4		qeu_tup_physical;   /* physical width (e.g. encrypted) */
    i4		qeu_ext_length;	    /* external width of tuples */
    i4		qeu_maxerrs;	    /* num errs till have to end copy */
    i4		qeu_uputerr[5];     /* array of errors to send to FE */
					    /*  temporary tilGCF */
    i4		qeu_rowspace;       /* max tuples per comm block */

    /* COPY FROM, qeu_cur_tups is the number of (full) tuples that
    ** QEF should send to the table.  COPY INTO, qeu_cur_tups is the
    ** max number of rows that SCF will accept, and upon return, QEF
    ** will set qeu_cur_tups to the actual number of rows returned.
    */
    i4			qeu_cur_tups;	    /* number of tuples in this block */
    i4			qeu_partial_tup;    /* tuple split between com blocks */
    i4			qeu_error;	    /* error encountered in COPY */
    PTR			*qeu_tdesc;         /* description of copy table */
    QEU_CPFILED		*qeu_fdesc;         /* description of copy file */
    char		*qeu_logname;	    /* error log file - NULL if none */

    /* IMPORTANT:  there may be more QEF_DATA elements on the qeu_input
    ** or qeu_output lists than QEF is supposed to read from or supply to
    ** the sequencer.  (e.g. in the FROM direction, the last qef-data may
    ** contain a partial input tuple, with the remainder still en route
    ** from the front-end.)  It is essential that QEF obey the
    ** qeu_cur_tups counter supplied by the sequencer.
    */
    QEF_DATA		*qeu_input;         /* input (copy from) tuples */
    QEF_DATA		*qeu_output;        /* output (copy into) tuples */

    /* qeu_sbuf and related variables are for the sequencer, so that it
    ** can manage the data flow between front-end and QEF.  There are two
    ** general uses for qeu_sbuf:
    ** 1) as a holder for partial tuples arriving from the front-end,
    **    until the FE gets around to sending the rest of the tuple;
    ** 2) as a holder for the non-LOB part of a tuple containing LOB's,
    **    to keep the non-LOB data out of the way as the LOB data is
    **    streamed in and couponified (or redeemed and streamed out).
    */
    PTR			qeu_sbuf;	    /* storage for unfinished tuple */
    i4			qeu_ssize;	    /* size of storage */
    i4			qeu_sused;	    /* amount of storage used */
    char		*qeu_sptr;	    /* pointer into storage */
    
    /*
    ** similar to qeu_sbuf above. qeu_cbuf is allocated by
    ** PSF from the proto stream (which is destroyed by PSF on
    ** commit). It is used to store the input records as they come
    ** in from prepared "insert" statements when running the insert to
    ** copy optimization. When the buffer fills up, qeu_r_copy will
    ** be called to load the tuples, and the buffer is re-used. This
    ** allows us to use the copy code as it was intended (for several
    ** records at once), and gives us at least some insight into
    ** how many records we might have in a batch, allowing us to
    ** optimize the copy code for very small batches. We could over-load
    ** the above "sbuf" variables to have yet another meaning, but
    ** the storage is cheap enough (only one of these per copy session).
    */
    PTR			qeu_cbuf;	    /* record storage */
    i4			qeu_csize;	    /* size of the buffer */
    i4			qeu_cleft;	    /* amount left */
    char		*qeu_cptr;	    /* current location */
    bool		qeu_load_buf;	    /* QEF should load the buffer */

    /* The following set of variables is used for the sequencer when
    ** there are large objects involved.  The sequencer will be called
    ** potentially many times as the LOB data is streamed into or out of
    ** the DBMS server.  These variables remember where we are in the
    ** tuple and input or output buffer.  When there are no large objects
    ** in the table, all of these variables (except qeu_cp_att_count)
    ** are initialized to -1 and stay that way.
    */
    i4			qeu_cp_cur_att;	    /* att # on which we're working,
					    ** -1 means beginning (or end)
					    ** of row, start another.
					    */
    i4			qeu_cp_att_need;    /* Amt needed to finish it */
    i4			qeu_cp_in_offset;   /* current offset in inp buffer */
    i4			qeu_cp_out_offset;  /* Ditto for output */
    i4			qeu_cp_in_lobj;	    /* is current column large */
    i4			qeu_cp_att_count;   /* # atts in the table */

    QEU_CPATTINFO	*qeu_att_info;	    /* pointer to useful att info*/
    QEU_CPATTINFO	*qeu_att_array;	    /* head of allocated array; primarily
					    ** for insert to copy optimization. In 
					    ** this instance the memory is contiguous
					    ** and ordered in order of the query 
					    ** parameters, the above qeu_att_info
					    ** is a linked list which points to
					    ** elements of this array in order of
					    ** table attributes
					    */
    QEF_INS_DATA	*qeu_insert_data;   /* insert data array for insert to copy
					    ** optimization. In this case each row
					    ** may have a different format, so we need
					    ** some way to keep the format for every row
					    ** in a copy block. The row will be converted
					    ** to the fixed internal tuple format in
					    ** qeu_r_copy
					    */
    QEF_INS_DATA	*qeu_ins_last;	    /* last element in above array
					    ** (saves scanning the linked lis
					    ** to find it ) */
    QEU_MISSING_COL     *qeu_missing;       /* pointer to missing column defaults */
    struct _DMR_CB	*qeu_dmrcb;	    /* Pointer to parser created DMR
					    ** CB to point to optional DMU-
					    ** CHAR_ARRAY entries. Probably
					    ** should just be a DM_DATA.
					    */

    struct _DMS_CB	*qeu_dmscb;	    /* Pointer to parser created DMS
					    ** CB for sequence defaults.  DMSCB
					    ** is filled out with sequence
					    ** entries and ptrs to data slots.
					    ** NULL if no sequence defaults.
					    */

    /* For COPY FROM into a partitioned table, we need the partition
    ** definition.  The parse makes a copy and sticks the address here.
    */
    DB_PART_DEF		*qeu_part_def;	    /* Address of partition def */

    /* When COPY startup in QEF opens the copy table, the "access ID"
    ** supplied by DMF is stored here.  This is the open in the session
    ** thread, not any copy child.  If the table is partitioned, this
    ** is the open of the partitioned master.
    ** As far as most of COPY is concerned, this access ID is just a
    ** cookie.  DMF knows that it's an RCB pointer, and will use the
    ** access ID to do nice things with blobs.  Blob-less copies don't
    ** need this at present.
    */
    PTR			qeu_access_id;	    /* Base table access ID from
					    ** table open in main thread.
					    */

    /* This copy control info pointed to here is defined locally
    ** in qefcopy.c;  nobody outside cares what's in here.
    */
    struct _QEU_COPY_CTL *qeu_copy_ctl;	    /* Pointer to COPY controller */
};
