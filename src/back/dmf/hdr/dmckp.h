/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DMCKP.H - Types, Constants and Macros for checkpoint and restore
**		  fucntions needed.
**
** Description:
**
** History:
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery project:
**		Created.
**	06-jan-1995 (shero03)
**	    Added Checkpoint support
**      07-jan-1995 (alison)
**          Added DMCKP_SAVE_FILE
**	23-june-1998(angusm)
**	    Change proto. of dmckp_init(), move constants from
**	    dmckp.c to here
**	30-mar-1999 (hanch04)
**	    Added support for raw locations
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


#define	MAXCOMMAND	5120

/* constants for variable 'op_type' */ 
#define		DMCKP_OP_BEGIN			0x000001L
#define		DMCKP_OP_END			0x000002L
#define		DMCKP_OP_PRE_WORK		0x000004L
#define		DMCKP_OP_WORK			0x000008L
#define	        DMCKP_OP_END_TBL_WORK  		0x000010L
#define	        DMCKP_OP_BEGIN_TBL_WORK		0x000020L

/* constants for variable 'direction' */
#define		DMCKP_SAVE			0x000001L
#define		DMCKP_RESTORE			0x000002L
#define		DMCKP_JOURNAL			0x000004L
#define		DMCKP_DELETE			0x000008L
#define		DMCKP_CHECK_SNAPSHOT		0x000010L
#define		DMCKP_DUMP			0x000020L

/* constants for variable 'granularity' */ 
#define		DMCKP_GRAN_DB			0x000001L
#define		DMCKP_GRAN_TBL			0x000002L
#define		DMCKP_GRAN_ALL			0x000004L
#define		DMCKP_RAW_DEVICE		0x000008L

typedef struct _DMCKP_CB	DMCKP_CB;


/*}
** Name: DMCKP_CB - Control block holds all the information need for a
**		   dmckp call.
**
** Description:
**    Control block holding all the information needed by the DMCKP routines.
**    Added this as know that there will be quite a few changes here so
**    instead of keep changing
**
**
** History
**	14-sep-1994 (andyw)
**	    Partial Backup/Recovery Project:
**	  	Created.
**	06-jan-1995 (shero03)
**	    Change Table name to be a character array
*/
struct	_DMCKP_CB
{
    u_i4		dmckp_op_type;
#define	DMCKP_RESTORE_DB		0x000001L
#define	DMCKP_RESTORE_FILE		0x000002L
#define	DMCKP_SAVE_DB			0x000004L
#define	DMCKP_DELETE_FILE		0x000008L
#define	DMCKP_DELETE_CKP		0x000010L
#define DMCKP_SAVE_FILE                 0x000020L
    BITFLD              dmckp_use_65_tmpl:1; /*
 					     ** Use the 6.5 style template file
					     */
    BITFLD              dmckp_bits_free:31;  /* Reserved for future use */

    u_i4		dmckp_l_tab_name;
    char		*dmckp_tab_name;     /* Current table name(s) */

    i4			dmckp_dev_cnt;   /*  Number of devices. */
    PID			*dmckp_ckp_path_pid; /* Process using ckp_path */
    DB_STATUS		*dmckp_ckp_path_status; /* Status of process   */
    u_i4		dmckp_ckp_l_path;
    char		*dmckp_ckp_path;     /*
					     ** Pathname to checkpoint
					     ** file
					     */
    char		*dmckp_ckp_file;     /* Checkpoint filename */
    u_i4		dmckp_ckp_l_file;
    char		*dmckp_di_path;	     /*
					     ** DI directory the operation is on
					     */
    u_i4		dmckp_di_l_path;
    char		*dmckp_di_file;	     /*
					     ** DI filename operation is on
					     */
    u_i4		dmckp_di_l_file;
    char		*dmckp_jnl_path;     /* Path to journal files */
    u_i4		dmckp_jnl_l_path;
    char		*dmckp_jnl_file;     /* Journal file name */
    u_i4		dmckp_jnl_l_file;
    char		*dmckp_dmp_path;     /* Path to dump files */
    u_i4		dmckp_dmp_l_path;
    char		*dmckp_dmp_file;     /* Dump file name */
    u_i4		dmckp_dmp_l_file;
    u_i4		dmckp_device_type;   /* Device type */
#define	DMCKP_DISK_FILE			1L
#define	DMCKP_TAPE_FILE			2L
    i4			dmckp_num_locs;	     /*
				             ** Total number of locations
					     ** Being operated on
					     */
    i4			dmckp_loc_number;    /*
					     ** Current location number
					     ** Being operated on
					     */
    i4			dmckp_num_files;     /*
                                             ** Total number of files
					     ** being operated on
					     */
    i4			dmckp_num_files_at_loc;
			                     /* Total number of files
					     ** being operated on at
					     ** current location
					     */
    i4			dmckp_file_num;	     /*
					     ** Current file number being
					     ** operated on
					     */
    char		*dmckp_user_string;  /*
					     ** User passed in string
					     ** which is passed on
					     ** blindly to the operation
					     */
    u_i4		dmckp_l_user_string;
    u_i4		dmckp_jnl_phase;     /* Journal phase */
#define	DMCKP_JNL_DO			1L
#define	DMCKP_JNL_UNDO			2L
    i4			dmckp_first_jnl_file;/*
					     ** First journal file of interest
					     */
    i4			dmckp_last_jnl_file; /*
					     ** Last journal file of interest
					     */
    i4			dmckp_first_dmp_file;/*
					     ** First dump file of interest
					     */
    i4			dmckp_last_dmp_file; /*
					     ** Last dump file of interest
					     */
    LOCATION		dmckp_cktmpl_loc;    /*
					     ** Location of the template
					     ** file.
					     */
    i4			dmckp_raw_start;     /*
					     ** Starting block of raw device
					     ** in units of DM2F_RAW_BLKSIZE
					     */
    i4			dmckp_raw_blocks;    /*
					     ** Number of raw blocks
					     ** in units of DM2F_RAW_BLKSIZE
					     */
    i4			dmckp_raw_total_blocks;/*
					     ** Total number of blocks in
					     ** raw area
					     ** in units of DM2F_RAW_BLKSIZE
					     */
    char		dmckp_command[ MAXCOMMAND ];
					     /*
					     ** Space for command line, here
					     ** to stop having to allocate
					     ** space all the time.
					     */

};


/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS dmckp_init(
		DMCKP_CB    *dmckp,
		u_i4   direction,
		i4     *err_code );

FUNC_EXTERN DB_STATUS dmckp_pre_restore_loc(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code );

FUNC_EXTERN DB_STATUS dmckp_restore_loc(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code );

FUNC_EXTERN DB_STATUS dmckp_begin_restore_db(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_end_restore_db(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_begin_restore_tbl(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_end_restore_tbl(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_pre_restore_file(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_restore_file(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_begin_restore_tbls_at_loc(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_end_restore_tbls_at_loc(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_begin_journal_phase(
		DMCKP_CB    *dmckp,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_end_journal_phase(
		DMCKP_CB    *dmckp,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_work_journal_phase(
		DMCKP_CB    *dmckp,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_delete_loc(
		DMCKP_CB    *dmckp,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_delete_file(
		DMCKP_CB    *dmckp,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_check_snapshot(
		DMCKP_CB    *dmckp,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_pre_save_loc(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code );

FUNC_EXTERN DB_STATUS dmckp_save_loc(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code );

FUNC_EXTERN DB_STATUS dmckp_begin_save_db(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_end_save_db(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_begin_save_tbl(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_end_save_tbl(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_pre_save_file(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;

FUNC_EXTERN DB_STATUS dmckp_save_file(
		DMCKP_CB    *dmckp,
		DMP_DCB     *d,
		i4     *err_code) ;
