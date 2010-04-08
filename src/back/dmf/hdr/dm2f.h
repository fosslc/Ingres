/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DM2F.H - Definitions of the file handling routines.
**
** Description:
**      This module defines the function that manipulate FCB's.
**
** History:
**      07-dec-1985 (derek)
**          Created new for jupiter.
**	08-sep-1988 (sandyh)
**	    Added support flags for sort filenames.
**	15-jan-1992 (bryanp)
**	    Added DM2F_DEFERRED_IO flag as part of temp table enhancements.
**	16-apr-1992 (bryanp)
**	    Prototypes for dm2f functions.
**	16-jul-1992 (kwatts)
**	    MPF project. Changed prototypes for dm2f_init/write_file.
**	26-oct-1992 (rogerk & jnash)
**	    Reduced Logging Project:  
**	      - Added prototypes and flag constants for new dm2f routines.
**	      - Added DM2F_LOCID_MACRO.
**	      - Changed dm2f_build_fcb to take a tableio pointer, not a tcb.
**	      - Eliminate dm2f_init_file FUNC_EXTERN.
**	30-October-1992 (rmuth)
**	    Add dm2f_galloc_file and dm2f_guarantee_space.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Project (phase 2):
**	    Removed DMPP_ACC_PLV argument from dm2f_write_file.
**	21-June-1993 (rmuth)
**	    Add prototype for dm2f_round_up.
**	18-jan-1995 (cohmi01)
**	    For RAW I/O, add location parm to dm2f_rename(), add defines.
**	26-mar-96 (nick)
**	    Add DM2F_LOC_ISOPEN_MACRO.
**      06-mar-96 (stial01)
**          Added page size argument to dm2f_build_fcb(), dm2f_read_file(),
**          dm2f_write_file(), dm2f_add_fcb()
**      31-oct-1996 (nanpr01 for ICL phil.p)
**          Added ANB's async i/o ammendments.
**      31-oct-1996 (nanpr01 for itb ICL)
**          Added dm2f_write_list declaration.
** 	19-jun-1997 (wonst02)
** 	    Added DM2F_READONLY flag for dm2f_build_fcb (readonly database).
**	26-Aug-1997 (jenjo02)
**	    Added log_id (LG_LXID) to dm2f_build_fcb(), dm2f_open_file(),
**	    dm2f_create_file(), dm2f_delete_file(), dm2f_rename(),
**	    dm2f_add_fcb() dm2f_release_fcb() prototypes so that it 
**	    can be passed on to dm2t_reclaim_tcb().
**	03-Dec-1998 (jenjo02)
**	    Prototyped dm2f_alloc_rawextent(), modified raw file definitions
**	    to accomodate multi-page-sized extents.
**      21-mar-1999 (nanpr01)
**          Support raw locations.
**	10-May-1999 (jenjo02)
**	    Added function prototypes for dm2f_check_list, dm2f_force_list,
**	    deleted DM2F_CLOSURE structure.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-Mar-2001 (jenjo02)
**	    Productized RAW file support.
** 	09-Apr-2001 (jenjo02)
**	    Added gw_io, gw_pages to dm2f_write_list() prototype.
**	11-May-2001 (jenjo02)
**	    Upped DM2F_RAW_MINBLKS from 4 to a more reasonable 16.
**	6-Feb-2004 (schka24)
**	    Updated prototypes for build fcb, filename; remove DM2F_OVF.
**	01-Dec-2008 (jonj)
**	    SIR 120874: dm2f_? functions converted to DB_ERROR *
**	15-Mar-2010 (smeke01) b119529
**	    DM0P_BSTAT fields bs_gw_pages and bs_gw_io are now u_i8s.
**/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN DB_STATUS dm2f_build_fcb(i4 lock_list, i4 log_id,
				    DMP_LOCATION *loc,
				    i4 l_count, i4 page_size,
				    i4 flag,
				    i4 fn_flag, DB_TAB_ID *table_id,
				    i2 name_id, i4 name_gen,
				    DMP_TABLE_IO *tcb,
				    u_i4 sync_flag, DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_release_fcb(i4 lock_list, i4 log_id,
				    DMP_LOCATION *loc,
				    i4 l_count, i4 flag,
				    DB_ERROR *dberr);
FUNC_EXTERN VOID    dm2f_filename(i4 flag, DB_TAB_ID *table_id,
				    u_i4 loc_id, i2 name_id, i4 name_gen,
				    DM_FILE *filename);
FUNC_EXTERN DB_STATUS dm2f_open_file(i4 lock_list, i4 log_id,
				    DMP_LOCATION *loc,
				    i4 l_count, i4 blk_size,
				    i4 flag, u_i4 sync_flag,
				    DM_OBJECT *cb, DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_create_file(i4 lock_list, i4 log_id,
				    DMP_LOCATION *loc,
				    i4 l_count, i4 blk_size,
				    u_i4 sync_flag, DM_OBJECT *cb,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_close_file(DMP_LOCATION *loc, i4 l_count,
				    DM_OBJECT *cb, DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_delete_file(i4 lock_list, i4 log_id,
				    DMP_LOCATION *loc,
				    i4 l_count, DM_OBJECT *cb,
				    i4 flag, DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_alloc_file(DMP_LOCATION *loc, i4 l_count,
				    DB_TAB_NAME *tab_name, DB_DB_NAME *db_name,
				    i4 count, i4 *page,
				    DB_ERROR *dberr);
FUNC_EXTERN i4 dm2f_file_size(i4 allocation, i4 no_locations,
				    i4 current_location);
FUNC_EXTERN DB_STATUS dm2f_force_file(DMP_LOCATION *loc, i4 l_count,
				    DB_TAB_NAME *tab_name, DB_DB_NAME *db_name,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_flush_file(DMP_LOCATION *loc, i4 l_count,
				    DB_TAB_NAME *tab_name, DB_DB_NAME *db_name,
				    DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_read_file(DMP_LOCATION *loc, i4 l_count,
				    i4 page_size,
				    DB_TAB_NAME *tab_name, DB_DB_NAME *db_name,
				    i4 *count, i4 page,
				    char *data, DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_sense_file(DMP_LOCATION *loc, i4 l_count,
				    DB_TAB_NAME *tab_name, DB_DB_NAME *db_name,
				    i4 *page, DB_ERROR *dberr);
FUNC_EXTERN DB_STATUS dm2f_rename(i4 lock_list, i4 log_id,
				    DM_PATH *phys,
				    i4 l_phys, DM_FILE *ofname,
				    i4 l_ofname, DM_FILE *nfname,
				    i4 l_nfname, DB_DB_NAME *db_name,
				    DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dm2f_write_file(DMP_LOCATION *loc, i4 l_count,
				    i4 page_size,
				    DB_TAB_NAME *tab_name, DB_DB_NAME *db_name,
				    i4 *count, i4 page,
				    char *data, DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dm2f_write_list(DMP_CLOSURE *clo,
				    char *di,
				    DMP_LOCATION *loc, i4 l_count,
				    i4 page_size,
				    DB_TAB_NAME *tab_name, DB_DB_NAME *db_name,
				    i4 *count, i4 page,
				    char *data, VOID (*evcomp)(),
				    PTR closure, i4 *queued_count, 
				    u_i8 *gw_pages, u_i8 *gw_io, 
				    DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS dm2f_force_list(VOID);

FUNC_EXTERN i4 dm2f_check_list(VOID);

FUNC_EXTERN VOID dm2f_set_alloc(DMP_LOCATION *loc, i4 l_count,
				    i4 count);

FUNC_EXTERN DB_STATUS dm2f_add_fcb(i4 lock_list, i4 log_id,
				    DMP_LOCATION *loc,
				    i4 page_size,
				    i4 l_count, i4 flag, 
				    DB_TAB_ID *table_id, u_i4 sync_flag,
				    DB_ERROR *dberr);
FUNC_EXTERN bool dm2f_check_access(DMP_LOCATION *loc, i4 l_count, 
				    DM_PAGENO page_number); 


FUNC_EXTERN DB_STATUS 	dm2f_galloc_file(
				DMP_LOCATION *loc, 
				i4 l_count,
				DB_TAB_NAME *tab_name, 
				DB_DB_NAME *db_name,
				i4 count, 
				i4 *page,
				DB_ERROR *dberr);

FUNC_EXTERN DB_STATUS 	dm2f_guarantee_space(
				DMP_LOCATION *loc, 
				i4 l_count,
				DB_TAB_NAME *tab_name, 
				DB_DB_NAME *db_name,
				i4 start_page, 
				i4 number_of_pages,
				DB_ERROR *dberr);

FUNC_EXTERN  VOID	dm2f_round_up(
				i4	*value);

FUNC_EXTERN  DB_STATUS	dm2f_sense_raw(
				char 	*path, 
				i4	l_path,
				i4	*raw_blocks,
				i4	lock_list,
				DB_ERROR *dberr);


/*
**  Constants for call to dm2f_open_file.
*/

#define	    DM2F_A_READ		1
#define	    DM2F_A_WRITE	2
#define	    DM2F_E_READ		3
#define	    DM2F_E_WRITE	4
/*
**  Constants for call to dm2f_filename.
*/

#define	    DM2F_TAB		1
#define	    DM2F_DES	        2
#define	    DM2F_MOD		3
#define	    DM2F_SM1	        4
/* notused #define  DM2F_OVF    5 */
#define	    DM2F_SRI	        6
#define	    DM2F_SRO	        7
/*
**  Constants for call to dm2f build and release routines.
*/

#define	    DM2F_OPEN		1
#define	    DM2F_TEMP		2
#define	    DM2F_LOAD		4	/* (obsolete) */
#define	    DM2F_ALLOC		8
#define	    DM2F_DEFERRED_IO	16
#define	    DM2F_READONLY	64	/* w/open, for read-only database */

 
/*
**  Macro which allows callers to request the location number to which a
**  specific page of a multi-location table would belong.
*/
#define		DM2F_LOCID_MACRO(loc_count, page_number)              \
				    ((page_number / DI_EXTENT_SIZE) % loc_count)

#define		DM2F_LOC_ISOPEN_MACRO(loc) \
			(((loc)->loc_fcb->fcb_state & FCB_OPEN) != 0)


/*
** RAW disk io items.
**
** Caution is advised if changing any of these values. Corresponding
** changes must also be made to admin/install/shell/mkraware.sh
*/
#define DM2F_RAW_KBLKSIZE  64		/* Unit size of a raw file block */
#define DM2F_RAW_BLKSIZE (DM2F_RAW_KBLKSIZE * 1024) /* ... in bytes */
#define DM2F_RAW_MINBLKS   16		/* Minimum number of blocks in location */
#define DM2F_RAW_LINKNAME "iirawdevice" /* Filename of link to raw device */
#define DM2F_RAW_BLKSNAME "iirawblocks" /* Filename where total blocks are stored */
#define DM2F_RAW_BLKSSIZE 16		/* Min size of buffer to read total blocks */
