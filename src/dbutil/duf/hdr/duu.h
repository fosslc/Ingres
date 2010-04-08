/*
** Copyright (c) 2002, 2004 Ingres Corporation
*/

/*
** Name: DUU.H -    Prototypes and definitions for functions in DUU.
**
** Description:
**	Protoypes for functions in DUU.
**
** History:
**      30-Sep-2002 (hajne04)
**          Created.
**	    BUG 108823
**	    Not having the functions in DUU prototyped was causing
**	    problems on 64bit Linux where the functions has a return type
**	    of char * but it was defaulting to i4 in the calling function.
**      26-Nov-2002 (hweho01)
**          modified the du_error() prototype declaration, it should   
**          be the same as it was defined in duerr.h file.
**	29-sep-2004 (devjo01)
**	    Removed du_chk3_locname.  (use cui_chk3_locname instead)
*/

STATUS du_chk1_dbname(); /* check the syntax of a database name */
i4 du_chk2_usrname(); /* check user name syntax */
DU_STATUS du_error( DU_ERROR *, int, int, ... );
STATUS du_brk_usr(); /* initialize a DU_USER struct from a line */
		    /* obtained from the users file. */
i4 du_find_usr(); /* get line from user file based on name */
VOID du_get_usrname(); /* get user name from the environment */
VOID du_envinit(); /* initialize the system utility environment struct. */
VOID du_reset_err(); /* reset the system utility error-handling control block */
char * duu_xflag(); /* Return value of the -X flags for GCA interface. */
DU_STATUS duu_chkenv(); /* Check Environment Attributes. */
STATUS du_ajnl_ingpath(); /* build the path for the active journal dir. */
STATUS du_ckp_ingpath(); /* build the path for the checkpoint directory. */
STATUS du_db_ingpath(); /* build the path for the deault DB  directory. */
STATUS du_ejnl_ingpath(); /* build the path for the expired journal directory. */
STATUS du_extdb_ingpath(); /* build the path for an extended database */
			  /* directory. */
STATUS du_work_ingpath(); /* build the path for a work location. */
STATUS du_fjnl_ingpath(); /* build the path for the full journal directory. */
STATUS du_jnl_ingpath(); /* build the path for the journal directory. */
STATUS du_dmp_ingpath(); /* build the path for the dump directory */
