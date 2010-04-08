/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: DUB.H -    definitions and typedefs that are used by the dub routines.
**
** Description:
**        This file contains definitions and typedefs that are used by
**	the "dub" routines.  The "dub" routines are those routines used
**	by the database utilities which operate on DBMS objects such as
**	the data directories and the table files.  These routines have
**	been put in a separate module, as they will always be under the
**	DBMS group's domain.
**	  This file should be included in any module that references
**	"dub" routines.
**
**	Dependencies:
**	    di.h
**	    duerr.h
**
** History: $Log-for RCS$
**      17-Oct-86 (ericj)
**          Initial creation.
[@history_template@]...
**/

/*
[@forward_type_references@]
*/


/*
**  Forward and/or External function references.
*/


/*
**	Forward functions references for procedures used only by
**	the database utility, createdb.  All of the routines can
**	be found in dubcrdb.c
*/
FUNC_EXTERN DU_STATUS dub_copy_file();	    /* Copy a file. */


/*
**	Forward functions references for procedures used only by
**	the database utility, destroydb.  All of the routines can
**	be found in dubdsdb.c
*/
FUNC_EXTERN DU_STATUS dub_del_dbms_dirs();  /* Deletes directories under
					    ** DBMS control.
					    */
FUNC_EXTERN DU_STATUS dub_file_del();	    /* Delete a DBMS file. */
FUNC_EXTERN DU_STATUS dub_dir_del();	    /* Delete a DBMS directory and all
					    ** of the contained files.
					    */

/*
**	Forward functions references for the locking procedures.
**	All of the routines can be found in dublock.c
*/
FUNC_EXTERN DU_STATUS dub_create_lklist();  /* Create a lock list. */
FUNC_EXTERN DU_STATUS dub_init_lk();	    /* Initialize the locking routines
					    ** for the calling process.
					    */
FUNC_EXTERN DU_STATUS dub_0dblk_request();  /* Lock a database in the given
					    ** mode.
					    */
FUNC_EXTERN DU_STATUS dub_1dblk_release();  /* Release a database lock and
					    ** all associated locks.
					    */




/*
**  Defines of other constants.
*/

/*
**      Define a constant for the configuration file name.
*/
#define                 DUB_CONFIG_FNAME "aaaaaaaa.cnf"


/*
[@group_of_defined_constants@]...
*/

/*
[@global_variable_references@]
*/


/*}
** Name: DUB_PDESC -	structure for defining a physical path to an Ingres
**			location.
**
** Description:
**        This structure is used to define a physical path to an Ingres
**	location.  This struct is dependent on DI.h
**
** History:
**      16-Oct-86 (ericj)
**          Initial creation.
[@history_template@]...
*/
typedef struct _DUB_PDESC
{
    i4              dub_plength;        /* The actual length of the path
					** description.
					*/
    char            dub_pname[DI_PATH_MAX]; /* Buffer for the path
					    ** description.
					    */
}   DUB_PDESC;



/*}
** Name: DUB_FDESC -	structure for defining an Ingres filename.
**
** Description:
**        This structure is used to define an Ingres DBMS filename.
**	This struct is dependent on DI.h
**
** History:
**      16-Oct-86 (ericj)
**          Initial creation.
[@history_template@]...
*/
typedef struct _DUB_FDESC
{
    i4              dub_flength;            /* The actual length of the filename
					    ** description.
					    */
    char            dub_fname[DI_FILENAME_MAX]; /* Buffer for the filename
						** description.
						*/
}   DUB_FDESC;

/*
[@type_definitions@]
*/
