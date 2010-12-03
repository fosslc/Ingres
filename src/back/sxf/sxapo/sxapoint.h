/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: SXAPOINT.H - audit file access mechanisms data definitions
**
** Description:
**	This file contains structure and data type definitions used
**	by the default audit file access routines. The definitions
**	in this file should only be visible to the SXAPO modules, 
**	no other modules should include this file.
**
** History:
**	23-dec-93 (stephenb)
**	    took sxapint.h and re-hashed for sxapo
**	4-jan-94 (stephenb)
**	    Added SXAPOSTATS and SXAPO_MSG_DESC structures for the conversion 
**	    routines stolen from gwsxa.
**	4-feb-94 (stephenb)
**	    Add declaration for sxapo_alter;
**	11-feb-94 (stephenb)
**	    Update SXAPO_RSCB to current SA spec.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Add missing prototypes
*/

# define	SAXPO_DISP_PARAM_MAX	10
# define	SXAPO_ERR_USER		2
# define	SXAPO_ERR_LOG		4
/*
** Name: SXAPO_RSCB - record stream control block
**
** Description:
**	This structure describes the instance of an open audit trail. It
**	is created at file open time and must be given for all subsequent 
**	accesses to the trail.
**
** History:
**	5-jan-94 (stephenb)
**	    initial creation.
**	11-feb-94 (stephenb)
**	    Alter sxapo_desc to a PTR to come in line with current SA spec.
*/
typedef struct _SXAPO_RSCB
{
    PTR			rs_memory;	/* The memory stream associated with
					** this control block.
					*/
    PTR			sxapo_desc;	/* Descriptor for this audit trail
					** returned by a call to SAopen()
					*/
} SXAPO_RSCB;

/*
** Name: SXAPO_CB - control structure for the audit file access routine
**
** Description:
**	This structure contains control information for the default audit
**	file access routines. Only one of these structures shall exist and
**	it will remain for the life of the server. It will be allocated from
**	the server control block's memory stream (sxf_svcb_mem).
**
** History:
**	6-jan-94 (stephenb)
**	    Created from SXAP_CB.
*/
typedef struct _SXAPO_CB
{
    i4			sxapo_status;
# define	SXAPO_ACTIVE		0x01
# define	SXAPO_STOPAUDIT		0x02
# define	SXAPO_SHUTDOWN		0x04
    SXAPO_RSCB		*sxapo_curr_rscb;	/* Pointer to the SXAPO_RSCB 
						** that is currently open for
						** writing. Only one audit trail
						** may be open for writing at
						** any time.
						*/
    i4		sxapo_writes;		/* Number of records written
						** since last flush
						*/
} SXAPO_CB;

GLOBALREF  SXAPO_CB  *Sxapo_cb;


/*
** Name: SXAPO_RECID - audit trail record identifier
**
** Description:
**	This structure is used as a simple counter store the number of records
**	written by a session since the last flush.
**
** History:
**	6-jan-94 (stephenb)
**	    Initial creation.
*/
typedef struct _SXAPO_RECID
{
    i4		recno;
} SXAPO_RECID;

/*
** Name: SXAPO_SCB - SXAPO scb
**
** Description:
**	Session control block for SXAPO
**
** History:
**	3-apr-94 (robf)
**         Created
*/
typedef struct _SXAPO_SCB
{
	SXAPO_RECID sxapo_recid;
} SXAPO_SCB;



/*
** Definition of function prototypes for the SXAP routines.
*/
FUNC_EXTERN DB_STATUS sxapo_shutdown(
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_bgn_ses(
    SXF_SCB		*scb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_end_ses(
    SXF_SCB		*scb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_open(
    PTR			filename,
    i4			mode,
    SXF_RSCB		*rscb,
    i4		*filesize,
    PTR			stamp,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_close(
    SXF_RSCB		*rscb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_position(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    i4			pos_type,
    PTR			pos_key,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_read(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_write(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_flush(
    SXF_SCB		*scb,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_show(
    PTR			filename,
    i4		*max_file,
    i4		*err_code);

FUNC_EXTERN DB_STATUS sxapo_alter(
    SXF_SCB         *scb,
    PTR             filename,
    i4         flags,
    i4         *err_code);

FUNC_EXTERN DB_STATUS sxapo_msgid_to_desc(
    i4         		msgid,
    DB_DATA_VALUE	*desc);

/*
** Name - SXAPOSTATS
**
** Description:
** 	Structure to hold SXAPO statistics for the conversion rouitnes 
**	in sxapocv.c
**
** History:
**	4-jan-94 (stephenb)
**	    Initial creation from GWSXASTATS in gwfsxa.h
*/
typedef struct
{
	i4		sxapo_num_msgid_lookup;
	i4		sxapo_num_msgid_fail;
	i4		sxapo_num_msgid;
} SXAPOSTATS;

/*
** Name: SXAPO_MSG_DESC
**
** Description:
**	Message cache structure, used when converting message id's to external
**	descriptions.
**
** History:
**	4-jan-94 (stephenb)
**	    renamed from SXA_MSG_DESC in gwsxa for use in sxapocv.c
*/
# define SXAPO_MAX_DESC_LEN 128

typedef struct _SXAPO_MSG_DESC
{
	i4 msgid;
	char    db_data[SXAPO_MAX_DESC_LEN+1];
	i4      db_length;
	struct  _SXAPO_MSG_DESC *left,*right;
} SXAPO_MSG_DESC;
