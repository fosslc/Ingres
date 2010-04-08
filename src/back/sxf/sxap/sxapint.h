/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: SXAPINT.H - audit file access mechanisms data definitions
**
** Description:
**	This file contains structure and data type definitions used
**	by the default audit file access routines. The definitions
**	in this file should only be visible to the SXAP modules, 
**	no other modules should include this file.
**
** History:
**	 11-aug-92 (markg)
**	    Initial creation.
**	  8-dec-92 (robf)
**	    Added definitions for startup options, stats block.
**	2-aug-93 (robf)
**	    Added file change by name definitions, more comments
**	13-jan-94 (robf)
**          Added sxap semaphore, rscb state and description of
**	    semaphore protocol.
**	 1-nov-95 (nick)
**	    Moved some definitions to sxf.h
**	10-oct-1996 (canor01)
**	    Changed 'iiaudit.mem' to 'caaudit.mem'
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-Oct-2008 (jonj)
**          Add sxap_sem_lock(), sxap_sem_unlock() functions.
*/

# define	SXAP_SHMNAME	"caaudit.mem"

# define	SXAP_MIN_PGSIZE	2048   /* Minimum page size */
# define	SXAP_DEF_PGSIZE	2048   /* Default page size */
# define	SXAP_MAX_PGSIZE	32768  /* Maximum page size */

# define        SXAP_DEF_SWITCHPROG "" /* Default onswitchlog program */
# define        SXAP_DEF_MAXFILE  512 /* 512 K bytes default size */
# define        SXAP_MIN_MAXFILE  100 /* Must be at least 100K */
# define 	SXAP_MAX_LOGFILES 512 /* Up to 512 logfiles may be specified */
# define	SXAP_SHM_DEF_BUFFERS (Sxf_svcb->sxf_max_ses<5?10:(Sxf_svcb->sxf_max_ses*2))
# define	SXAP_DEF_AUD_WRITER_START (SXAP_SHM_DEF_BUFFERS<10?SXAP_SHM_DEF_BUFFERS:10)
/*
**	These defines are for sxap_find_curfile.
*/
# define       SXAP_FINDCUR_INIT 1	/* Initialization */
# define       SXAP_FINDCUR_NEXT 2	/* Next available file */
# define       SXAP_FINDCUR_STAMP 3	/* Find matching stamp */
# define       SXAP_FINDCUR_NAME 4	/* Find matching name */
/*
**	These defines are for sxap_change_file
*/
# define   	SXAP_CHANGE_NEXT  1	/* Get the "next" file */
# define	SXAP_CHANGE_STAMP 2	/* Change to match a specific file */
# define	SXAP_CHANGE_NAME 3	/* Use specified file name */

# define	SXAP_DEF_GRP_INTERVAL 50 /* Default group flush time */

/*
** Name: SXAP_RECID - audit file record identifier
**
** Description:
**	This typedef defines the structure of an audit file record
**	identifier. Each record is uniquely identified by its offset
**	in the page, and its page number in the audit file.
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
*/

typedef		i4		SXAP_PGNO;
typedef		i4		SXAP_OFFSET;

typedef struct _SXAP_RECID
{
    SXAP_PGNO		pageno;		/* The audit page number where the
					** record exists.
					*/
    SXAP_OFFSET		offset;		/* The records offset within
					** the audit page.
					*/
} SXAP_RECID;
/*
**	The unique "stamp" for an audit file
*/
typedef struct _SXAP_STAMP {
	char stamp[8];
} SXAP_STAMP;

/*
** Name: SXAP_PGHDR - an audit page header record
**
** Description:
**	Each page in the audit file, regardless of type, will contain an
**	audit page header record as the first record on the page. This 
**	records will contain control information for the page.
**
** History:
**	11-aug-1992 (markg)
**	    Initial creation.
*/
typedef struct _SXAP_PGHDR
{
    SXAP_PGNO		pg_no;		/* The number of this page, all
					** pages are numbered sequentially.
					*/
    i4			pg_chksum;	/* The checksum of the page, calculated
					** when gets written to disk.
					*/
    i2			pg_type;	/* The page type */
# define	SXAP_HEADER		1	/* Header page */
# define	SXAP_DATA		2	/* Data page */
    SXAP_OFFSET		pg_next_byte;	/* The offset of the next free byte
					** in the page.
					*/
} SXAP_PGHDR;
/*
** Name: SXAP_DPG - an audit file data page
**
** Description:
**	This typedef describe the structure of a data page in an audit file.
**	An audit file consists of a header page (SXAP_HPG) followed 
**	by a number of SXAP_DPG pages.
**
**	Each SXAP_DPG is made up of two destinct parts:-
**
**		1. The page header
**		2. The data area
**
**	The page header contains information that identifies the page, and
**	also control information about the state of the page.
**
**	The data area contains the raw tuples. These are built from the 
**	top of the page downwards.
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
*/
typedef struct _SXAP_DPG
{
    SXAP_PGHDR		dpg_header;	/* The page header record */
    char		dpg_tuples[1];	/* The data area */
} SXAP_DPG;

/*
** Name: SXAP_HPG - audit file header page
**
** Description:
**	This typdef describes the structure of an audit file header page.
**	A header page will be the first page in every audit file, it contains
**	information about the file version and when it was created.
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
**	7-jul-93 (robf)
**	    Added Version 2 header type
*/
typedef struct _SXAP_HPG
{
    SXAP_PGHDR		hpg_header;	/* The page header */
    i4			hpg_version;	/* The sxf version id, used to ensure
					** the the audit file format is one
					** we can cope with.
					*/

/* 
** Initial version (6.5 C2) 
*/
# define	SXAP_VER_1		CV_C_CONST_MACRO('0','1','.','0')
/* 
** Secure 2.0 version:
** - detail text len is 2 bytes
** - Session id handled
*/
# define	SXAP_VER_2		CV_C_CONST_MACRO('0','2','.','0')
/*
** Secure 2.1 version:
** - header includes page size
*/
# define	SXAP_VER_21		CV_C_CONST_MACRO('0','2','.','1')

/* Current version */
# define	SXAP_VERSION		SXAP_VER_21

    SXAP_STAMP		hpg_stamp;	/* The unique stamp for the file */
    i4		hpg_pgsize;	/* Page size for this file */

} SXAP_HPG;


/*
** Name: SXAP_APB - an audit page buffer
**
** Description:
**
**	This structure describes an audit page buffer used for
**	buffering reads and writes to audit files. If this is a 
**	write buffer it may be configured in shared memory, and
**	be used by all servers on the node.
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
*/
typedef  struct  _SXAP_APB
{
    i4		apb_status;	/* The page buffer status mask */
# define	SXAP_READ		0x01L /* Read buffer */
# define	SXAP_WRITE		0x02L /* Write buffer */
# define	SXAP_MODIFY		0x04L /* Buffer contains dirty data */
# define	SXAP_INCONSISTENT	0x08L /* Buffer is inconsistent */	
    SXAP_RECID		apb_last_rec; 	/* The id of the last record written
					** to, or read from, the page buffer. 
					*/
    ALIGN_RESTRICT	apb_auditpage[1 + (SXAP_MAX_PGSIZE/sizeof(ALIGN_RESTRICT))];
					/* An aligned buffer for storing a
					** copy of a page from an audit file.
					*/

} SXAP_APB;
/*
** Name: SXAP_QBUF - Queue buffer
**
** Description:
**	A queue buffer used to store records in shared memory prior to
**	writing.
**
** History:
**	30-mar-94 (robf)
**	    Created
*/
typedef struct _SXAP_QBUF
{
	i4		flags;		/* Flags mask */
# define   SXAP_QEMPTY	0x001	/* Empty */
# define   SXAP_QUSED   0x002   /* Used */
# define   SXAP_QDETAIL 0x004   /* Record has detail */

	SXF_AUDIT_REC 	auditrec;	/* Audit record */
	char		detail_txt[SXF_DETAIL_TXT_LEN];
} SXAP_QBUF;

/*
** Name: SXAP_QLOC - Queue location
**
** Description:
**	This is a location in the shared queue.
**
** History:
**	30-mar-94 (robf)
**         Created
*/
typedef struct _SXAP_QLOC
{
	i4 trip;
# define SXAP_QLOC_MAXTRIP MAXI4
	i4	  buf;
} SXAP_QLOC;

/*
** Name: SXAP_SHM_SEG - Shared memory segment definition
**
** Description:
**	The SXAP shared memory segment definition
**
** History:
**	30-mar-94 (robf)
**	    Created
*/
typedef struct _SXAP_SHM_SEG 
{
	i4		shm_version;	/* SHM version */
# define	SXAP_SHM_VER		2
	i4  shm_status;	/* SHM Status */
        SXAP_STAMP      shm_stamp;		/* Stamp for log file for this
						** buffer 
						*/
	SXAP_APB	shm_apb;		/* In-memory page buffer */
	i4		shm_num_qbuf;		/* Number of shared buffers */
	u_i4		shm_page_size;		/* Page size of buffer */
	SXAP_QLOC	shm_qstart;		/* Start of queue */
	SXAP_QLOC	shm_qend;		/* End of queue */
	SXAP_QLOC	shm_qwrite;		/* Last written queue element*/
	/* This must be at the end of the segment */
	SXAP_QBUF	shm_qbuf[1];		/* Array of queue buffers */
} SXAP_SHM_SEG;

/*
** Name: SXAP_RSCB - record stream control block
**
** Description:
**	This structure describes the instance of an open audit file. It
**	is created at file open time and must be given for all subsequent 
**	accesses to the file.
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
**	7-jul-93 (robf)
**	    Add rs_version to the RSCB
**	13-jan-94 (stephenb)
**        Integrate robf fix:
**	    Add rs_state to SXAP_RSCB
*/
typedef struct _SXAP_RSCB
{
    DI_IO		rs_di_io;	/* The DI_IO descriptor returned
					** by the DIopen call for this file.
					*/
    i4			rs_state;	/* Flags Mask */
# define SXAP_RS_INVALID -1	/* invalid CB */
# define SXAP_RS_OPEN	  1	/* Open CB */
# define SXAP_RS_CLOSE    2     /* Closed CB */
    i4		rs_last_page;	/* The number of the last page
					** in an audit file. This value
					** is obtained by calling DIsense
					** at file open time. It does not
					** reflect any change of size that
					** has happened to a file once it
					** has been opened.
					*/
    PTR			rs_memory;	/* The memory stream associated with
					** this control block.
					*/
    char		rs_filename[MAX_LOC + 1];
					/* The name of the audit file
					** associated with this RSCB.
					*/
    SXAP_APB		*rs_auditbuf;	/* The audit page buffer used for
					** buffering IO operations to this
					** audit file.
					*/
   i4			rs_version;	/* Version for this file */ 
   i4		rs_pgsize;	/* Page size of file */
} SXAP_RSCB;

/*
** Name: SXAP_CB - control structure for the audit file access routine
**
** Description:
**	This structure contains control information for the default audit
**	file access routines. Only one of these structures shall exist and
**	it will remain for the life of the server. It will be allocated from
**	the server control block's memory stream (sxf_svcb_mem).
**
**      The sxap_semaphore provides a finer degree of control over access
**      to the CB. Although the access lock provides the main concurrency
**      control to the SXAP subsystem, using a lock can be expensive so for
**      requests which may not need the lock (primarily SXAP_FLUSH) we use
**      the semaphore to stop things changing which examining the CB.
**      To prevent deadlock the following protocol is used:
**        - Either the access lock or the semaphore may be taken individually.
**        - The semaphore may be taken if the access lock is already held.
**        - The access lock may *not* be taken if the semaphore is already
**          taken.
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
**	13-jan-94 (stephenb)
**         Integrate robf fix;
**	    Added sxap_semaphore to control access to the CB, add protocol
**          for using this semaphore above.
**	3-jun-94 (robf)
**         Moved sxf_aud_writer CID here from ASCB since its private to
**	   SXAP.
**      01-Oct-2008 (jonj)
**         Add sxap_sem_owner, sxap_sem_pins.
*/
typedef struct _SXAP_CB
{
    i4			sxap_status;
# define	SXAP_ACTIVE		0x01
# define 	SXAP_SWITCHPROG		0x08
# define	SXAP_AUDIT_WRITER_BUSY  0x10
    i4		sxap_max_file;		/* Maximum size of a security
						** audit log file.
						*/
    i4			sxap_act_on_full;	/* Action to take on a file
						** full event.
						*/
# define	SXAP_SHUTDOWN		0x01
# define	SXAP_STOPAUDIT		0x02
# define	SXAP_SUSPEND		0x04

    char		sxap_curr_file[MAX_LOC + 1];
						/* Name of the current audit
						** file.
						*/
    char               	sxap_switch_prog[MAX_LOC+1];
						/*
						** Name of  the program to
						** run when switching files.
						*/
    SXAP_RSCB		*sxap_curr_rscb;	/* Pointer to the SXDEF_RSCB 
						** that is currently open for
						** writing. Only one file
						** may be open for writing at
						** any time.
						*/
    LK_LKID		sxap_acc_lockid;	/* The identifier of the access
						** control lock. Used by
						** SXAP to synchronize access
						** to the audit buffer.
						*/
    LK_LKID		sxap_shm_lockid;	/* The identifier of the shared
						** memory control lock. Used by
						** SXAP to determine if an other
						** servers are connected to
						** the SXAP shared memory 
						** segment.
						*/
    SXAP_SHM_SEG	*sxap_shm;		/* Pointer to the SXAP shared 
						** memory segment.
						*/
    SXAP_STAMP		sxap_stamp;		/* The current value of the
						** SXAP log stamp, as seen by
						** this server.This is used to 
						** check if the current audit
						** configuration has been 
						** changed by another server.
						*/
    CS_SID              sxap_sem_owner;         /* Who holds the sem */
    i4                  sxap_sem_pins;          /* Pin count; when 0, v_sem */
   CS_SEMAPHORE        sxap_semaphore;          /* Semaphore controlling
						** access to this structure,
						** See above for description
						** of using this.
						*/
    i4			sxap_num_shm_qbuf;	/* Number of shared qbufs */

    i4			sxap_page_size;		/* Configured page size */
    i4			sxap_aud_writer_start;	/* Audit writer start  value */
    i4			sxap_force_flush;	/* How we handle force flush */
# define	SXAP_FF_OFF	1	/* Flush off */
# define	SXAP_FF_ON	2	/* Flush on */
# define	SXAP_FF_SYSTEM  3	/* Flush on by the system */

    CS_SID		sxap_aud_writer;/* The session id of the SXF
					** audit writer thread.
					*/

} SXAP_CB;

GLOBALREF  SXAP_CB  *Sxap_cb;


/*
** Name: SXAP_SCB 
**
** Description:
**	SXAP Session control block. This contains data structures
**	private to SXAP.
**
** History:
**	2-apr-94 (robf)
**         Created
*/
typedef struct _SXAP_SCB
{
	i4	flags;
#define SXAP_SESS_MODIFY	0x001   /* Records queued */
	SXAP_QLOC  sess_lastq;	/* Last record written for the session */
	SXAP_RECID sess_lastrec;

} SXAP_SCB;
/*
** Definition of function prototypes for the SXAP routines.
*/
FUNC_EXTERN DB_STATUS sxap_shutdown(
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_bgn_ses(
    SXF_SCB		*scb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_end_ses(
    SXF_SCB		*scb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_open(
    PTR			filename,
    i4			mode,
    SXF_RSCB		*rscb,
    i4		*filesize,
    PTR			stamp,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_create(
    PTR			filename,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_close(
    SXF_RSCB		*rscb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_position(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    i4			pos_type,
    PTR			pos_key,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_read(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_write(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_write_page(
    DI_IO		*di_io,
    i4		pageno,
    PTR			page_buf,
    bool		force,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_read_page(
    DI_IO		*di_io,
    i4		pageno,
    PTR			page_buf,
    i4		page_size,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_alloc_page(
    DI_IO		*di_io,
    PTR			page_buf,
    i4		*pageno,
    i4		page_size,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_flush(
    SXF_SCB		*scb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_alter(
    SXF_SCB 		*scb,
    PTR		        filename,
    i4		flags,
    i4		*err_code
);

FUNC_EXTERN DB_STATUS sxap_show(
    PTR			filename,
    i4		*max_file,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_init_cnf(
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_chk_cnf(
    LK_VALUE		*lk_val,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_change_file(
    i4			oper,
    SXAP_STAMP		*stamp,
    PTR			filename,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_find_curfile(
    i4			oper,
    char		*fileloc,
    SXAP_STAMP		*stamp,
    char		*filename,
    i4		*err_code);

FUNC_EXTERN VOID sxap_imm_shutdown (void);

FUNC_EXTERN i4 sxap_stamp_cmp(
			SXAP_STAMP*, 
			SXAP_STAMP *
);

FUNC_EXTERN DB_STATUS sxap_stamp_to_lockval (
    SXAP_STAMP 		*stamp,
    LK_VALUE   		*lock_val

);
FUNC_EXTERN DB_STATUS sxap_lockval_to_stamp (
    LK_VALUE   		*lock_val,
    SXAP_STAMP 		*stamp

);

FUNC_EXTERN DB_STATUS sxap_flushbuff(
    i4 		*err_code
);

FUNC_EXTERN VOID sxap_run_onswitchlog(
	char		*filename
);

FUNC_EXTERN VOID sxap_stamp_now (
	SXAP_STAMP 	*stamp
);

FUNC_EXTERN VOID sxap_lkerr_reason (
	char		*lk_name,
	STATUS		cl_status
);
FUNC_EXTERN DB_STATUS sxap_q_lock(
    SXF_SCB		*scb,
    i4			lkmode,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    i4		*err_code
);
FUNC_EXTERN DB_STATUS sxap_ac_lock(
    SXF_SCB		*scb,
    i4			lkmode,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    i4		*err_code
);
FUNC_EXTERN DB_STATUS sxap_qwrite(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code);


FUNC_EXTERN DB_STATUS sxap_q_unlock(
    SXF_SCB		*scb,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    i4		*err_code
);

FUNC_EXTERN DB_STATUS sxap_ac_unlock(
    SXF_SCB		*scb,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    i4		*err_code
);
FUNC_EXTERN DB_STATUS sxap_qflush(
    SXF_SCB		*scb,
    i4		*err_code
);
FUNC_EXTERN VOID sxap_qloc_incr (SXAP_QLOC *qloc);

FUNC_EXTERN DB_STATUS sxap_qallflush(
    SXF_SCB		*scb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_audit_writer(
    SXF_SCB		*scb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxap_sem_lock(i4 *err_code);

FUNC_EXTERN DB_STATUS sxap_sem_unlock(i4 *err_code);
