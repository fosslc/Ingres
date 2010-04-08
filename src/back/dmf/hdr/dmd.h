/*
** Copyright (c) 1985, 2008 Ingres Corporation
*/

/**
** Name: DMD.H - This defines the constants, macros for debugging and tracing.
**
** Description:
**      This file contains the constants and macros needed for the dmf debugging
**      and tracings routines.
**
** History:
**      22-oct-1985 (derek)
**          Created new for Jupiter.
**	6-jul-1992 (jnash)
**	    Add DMF Prototyping.
**	13-aug-1992 (fred)
**	    Added proto's for dmd_petrace() for peripheral object tracing.
**	29-August-1992 (rmuth)
**	    Add dmd_build_iotrace
**	04-nov-92 (jrb)
**	    Change proto for dmd_siotrace since I'm changing the interface for
**	    ML Sorts.
**	10-feb-1993 (jnash)
**	    Add dmd_format_log, dmd_format_dm_hdr, dmd_format_lg_hdr and
**	    dmd_put_line FUNC_EXTERNs.
**	26-apr-1993 (jnash)
**	    Add 'reserved' param to dmd_logtrace() FUNC_EXTERN. 
**	22-nov-1993 (jnash)
**	    Add dmd_lock_info() and dmd_lkrqst_trace() FUNC_EXTERNs, 
**	    Add DMTR_LOCKSTAT constants.
**      07-oct-93 (johnst)
**	    Bug #56442
**          Changed dmd_fmt_cb() flag argument type from (i4) to 
**	    (i4 *) to match new dm0m_search() arg type of (i4 *).
**	    The dm0m_search() user-supplied search function args must now be
**	    passed by reference to correctly support 64-bit pointer
**	    arguments. On DEC alpha, for example, ptrs are 64-bit and ints
**	    are 32-bit, so ptr/int overloading will no longer work!
**	12-Jan-1996 (jenjo02)
**	    Added DMTR_SMALL_HEADER directive to display a smaller
**	    dmcstop-sized header. dmd_lock_info() is now also called
**	    by dmcstop during server termination.
**	20-aug-96 (nick)
**	    Add DMTR_LOCK_HELP.
**	24-Apr-1997 (jenjo02)
**	    Added DMTR_LOCK_DIRTY
**      12-apr-1999 (stial01)
**          Changed prototype for dmd_prkey
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      06-apr-2001
**          Added prototype for dmd_page_types()
**      13-mar-2002
**          changed prototype for dmd_call
** 	15-Mar-2006 (jenjo02)
**	    Defined dmd_log_info() as a common place to 
**	    TRdisplay stuff about the log file and its state.
**	13-Apr-2006 (jenjo02)
**	    Add **atts to dmd_prkey() prototype.
**	20-nov-2006
**	    SIR 117586
**	    Added parameter for dmd_prkey
**	06-feb-2008 (joea)
**	    Add DMTR_LOCK_DLM and DMTR_LOCK_DLM_ALL. Add prototype for
**	    dmd_dlm_lock_info.
**      12-oct-2009 (stial01)
**          Add routines to print btree keys without rcb.
**	20-Oct-2009 (kschendel) SIR 122739
**	    Update above for new rowaccessor scheme.
*/

/*
**  Defines of other constants.
*/

/*
** 	Directives to dmd_lock_info().
*/
#define DMTR_LOCK_SUMMARY        0x0001
#define DMTR_LOCK_STATISTICS     0x0002
#define DMTR_LOCK_USER_LISTS     0x0004
#define DMTR_LOCK_SPECIAL_LISTS  0x0008
#define DMTR_LOCK_LISTS          (DMTR_LOCK_USER_LISTS|\
					DMTR_LOCK_SPECIAL_LISTS)
#define DMTR_LOCK_RESOURCES      0x0010
#define DMTR_LOCK_HELP           0x0020
#define DMTR_LOCK_DLM            0x0040  /* cluster only: DLM lock info */
#define DMTR_LOCK_DLM_ALL        0x0080  /* ditto but including non-waiters */
#define DMTR_LOCK_ALL            (DMTR_LOCK_SUMMARY|\
					DMTR_LOCK_STATISTICS|\
                                	DMTR_LOCK_LISTS|\
					DMTR_LOCK_RESOURCES \
					| DMTR_LOCK_DLM)
#define DMTR_LOCK_DIRTY          0x0100 /* dirty-read lock structures */
#define DMTR_SMALL_HEADER        0x1000 /* directive to print shorter
					** header line to fit in
					** dmcstop-size tracelog. */



/*
**      Constants used as TRACE flags.
*/

#define			DMTR_BT         0x0100L
#define                 DMTR_BTCR       0x0110L 
#define                 DMTR_NEXT_FREE  0x0130L
#define                 DMTR_RCR        0x0210L

/*
** 	Directives to dmd_log_info().
*/

#define			DMLG_STATISTICS		0x0001L
#define			DMLG_HEADER		0x0002L
#define			DMLG_PROCESSES		0x0004L
#define			DMLG_DATABASES		0x0008L
#define			DMLG_TRANSACTIONS	0x0010L
#define			DMLG_BUFFERS		0x0020L
#define			DMLG_USER_TRANS		0x0040L
#define			DMLG_SPECIAL_TRANS	0x0080L
#define			DMLG_BUFFER_UTIL	0x0100L

/*
**      Macros for checking and setting trace flags.
*/

/* For now, pretend all masks are set except crash tests. */
#define		dmtr_macro(mask, value)	    ((mask) != DMTR_BTCR && (mask) != DMTR_RCR)


/*
**  External function references.
*/

FUNC_EXTERN VOID dmd_buffer(VOID);

FUNC_EXTERN VOID dmd_call(
			i4		operation,
			PTR		control_block,
			i4		error);

FUNC_EXTERN VOID dmd_return(
			i4		operation,
			PTR		control_block,
			i4		status );

FUNC_EXTERN VOID dmd_timer(
			i4		type,
			i4		operation,
			i4		error );

FUNC_EXTERN VOID dmd_dmp_pool(
			i4	flag );

FUNC_EXTERN VOID dmd_fmt_cb(
			i4 	*flag,
			DM_OBJECT	*obj );

FUNC_EXTERN VOID dmd_prall(
			DMP_RCB	*rcb );

FUNC_EXTERN VOID dmd_prdata(
			DMP_RCB		*rcb,
			DMPP_PAGE	*data );

FUNC_EXTERN VOID dmd_prindex(
			DMP_RCB		*rcb,
			DMPP_PAGE	*b,
			i4		indent );

FUNC_EXTERN VOID dmd_prkey(
			DMP_RCB		*rcb,
			char		*key,
			i4              page_type,
			DB_ATTS		**atts,
			i4		suppress_newline,
			i4		keys_given );

FUNC_EXTERN VOID dmd_print_key(
			char			*key,
			i4                      page_type,
			DB_ATTS			**katt,
			i4			suppress_newline,
			i4			keys_given,
			ADF_CB			*adf_cb);

FUNC_EXTERN VOID dmd_prordered(
			DMP_RCB	*rcb );


FUNC_EXTERN VOID dmd_prrecord(
			DMP_RCB		*rcb,
			char		*record );

FUNC_EXTERN VOID dmdprbrange(
			DMP_RCB		*rcb,
			DMPP_PAGE	*b );

FUNC_EXTERN VOID dmd_print_brange(
			i4		page_type,
			i4		page_size,
			DMP_ROWACCESS	*rac,
			i4		rngklen,
			i4		keys,
			ADF_CB		*adf_cb,
			DMPP_PAGE	*b);

FUNC_EXTERN VOID dmd_log(
			bool		verbose_flag,
			PTR		record,
			i4		size );

FUNC_EXTERN STATUS dmd_lock(
			LK_LOCK_KEY	*lock_key,
			i4		lockid,
			i4		request_type,
			i4		request_flag,
			i4		lock_mode,
			i4		timeout,
			DB_TRAN_ID	*tran_id,
			DB_TAB_NAME	*table_name,
			DB_DB_NAME	*database_name );

FUNC_EXTERN VOID dmd_lkrqst_trace(
			i4		(*format_routine)(
					PTR		arg,
					i4		length,
					char 		*buffer),
			LK_LOCK_KEY	*lock_key,
			i4		lockid,
			i4		request_type,
			i4		request_flag,
			i4		lock_mode,
			i4		timeout,
			DB_TRAN_ID	*tran_id,
			DB_TAB_NAME	*table_name,
			DB_DB_NAME	*database_name );

FUNC_EXTERN STATUS dmd_iotrace(
			i4		io_type,
			DM_PAGENO	first_page,
			i4		number_pages,
			DB_TAB_NAME	*table_name,
			DM_FILE		*file_name,
			DB_DB_NAME	*database_name );

FUNC_EXTERN STATUS dmd_build_iotrace(
			i4		io_type,
			DM_PAGENO	first_page,
			i4		number_pages,
			DB_TAB_NAME	*table_name,
			DM_FILE		*file_name,
			DB_DB_NAME	*database_name );

FUNC_EXTERN STATUS dmd_logtrace(
			DM0L_HEADER	*log_hdr,
			i4		reserved);

FUNC_EXTERN STATUS dmd_rstat(
			DMP_RCB	*rcb);

FUNC_EXTERN STATUS dmd_txstat(
			DML_XCB		*xcb,
			i4		state);

FUNC_EXTERN STATUS dmd_siotrace(
			i4     io_type,
			i4	    in_or_out,
			i4	    block,
			i4	    filenum,
			i4	    location,
			i4	    rblock);

FUNC_EXTERN i4 dmd_put_line(
			PTR		arg,
			i4		length,
			char  		*buffer);

FUNC_EXTERN VOID dmd_format_lg_hdr(
					   i4	(*format_routine)(
					   PTR		arg,
					   i4		length,
					   char 	*buffer),
			LG_LA 	    *lga,
			LG_RECORD   *lgr,
			i4     log_block_size);

FUNC_EXTERN VOID dmd_format_dm_hdr(
					   i4	(*format_routine)(
					   PTR		arg,
					   i4		length,
					   char 	*buffer),
			DM0L_HEADER *h,
			char	    *line_buffer,
			i4	    l_line_buffer);

FUNC_EXTERN VOID dmd_format_log(  
			bool	    verbose_flag,
					   i4	(*format_routine)(
					   PTR		arg,
					   i4		length,
					   char 	*buffer),
			char	    *record,
			i4	    log_page_size,
			char	    *line_buffer,
			i4	    l_line_buffer);

/*
FUNC_EXTERN VOID dmd_petrace(
			char		   *operation,
			DMPE_RECORD        *record,
			i4		   base,
			i4		   extension );
*/

FUNC_EXTERN VOID dmd_lock_info(i4 options);

FUNC_EXTERN VOID dmd_dlm_lock_info(bool waiters_only);

FUNC_EXTERN bool dmd_reptrace(VOID);

FUNC_EXTERN VOID dmd_page_types(VOID);

FUNC_EXTERN VOID dmd_log_info(i4 options);
