/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
** Name:    doglob.h -	FE Object Deletion Utility Global Declarations.
**
** Description:
**	Contains declarations for all the functions, types, and global variables
**	which are specific to the FE Object Deletion Utility.
**
**	This version of the file deals with references only - no variables are
**	actually allocated.
**
**	THIS FILE MUST BE KEPT IN SYNC WITH doglobi.h !!!
**
** History:
**	12-jan-1993 (rdrane)
**		Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**	FE_DEL - structure containing all information necessary to describe the
**		 FE object to be found and deleted.
*/

typedef struct fe_del
{
	OOID		fd_id;			/* FE object ID		      */
	OOID		fd_type;		/* FE object type (OC_ values)*/
	FE_RSLV_NAME	*fd_name_info;		/* FE object name/owner info  */
	struct		fe_del *fd_below;	/* Next FE_DEL in linked list */
}   FE_DEL;

/*
**	OBJ_DEL_DATA -	structure containing all information necessary to describe
**			the actual deletion message and method for a particular
**			FE object.
*/

typedef	struct
{
	OOID	low_ooid;		/* Low value of object_id for match  */
	OOID	hi_ooid;		/* High value of object_id for match */
	ER_MSGID msg_name;		/* Name of object type for info msgs */
	STATUS	(*delproc)();		/* Address of actual deletion proc   */
	char	delproc_name[16];	/*
					** Name of actual deletion procedure
					** (DEBUG Only)
					*/
} OBJ_DEL_DATA;

/*
**	Global variables
*/
			/*
			** Head of FE_DEL linked list
			*/
	GLOBALREF	FE_DEL	*Ptr_fd_top;
			/*
			** Current FE_DEL being acted upon
			*/
	GLOBALREF	FE_DEL	*Cact_fd;
			/*
			** File containing FE object names to delete
			*/
	GLOBALREF	char	*Dobj_dfile;
			/*
			** Target database name
			*/
	GLOBALREF	char  *Dobj_database;
	GLOBALREF	char  *Dobj_rep_owner;
			/*
			** Database Connection parameters (user,
			** group, and password)
			*/
	GLOBALREF char  *Dobj_uflag;
	GLOBALREF char  *Dobj_gidflag;
	GLOBALREF char  *Dobj_passflag;

			/*
			** Enable wildcard expansion if TRUE
			*/
	GLOBALREF bool  Dobj_wildcard;

			/*
			** Display status and/or error messages if TRUE
			*/
	GLOBALREF bool  Dobj_silent;

			/*
			** Prompt string containing
			** object type being deleted.
			*/
	GLOBALREF char  *Dobj_pr_str;

/*
**	External Function Declarations (w/ prototypes)
*/

	FUNC_EXTERN	VOID	do_crack_cmd(i4,char **);
	FUNC_EXTERN	STATUS	do_del_app(char *,char *,OOID,OOID);
	FUNC_EXTERN	bool	do_del_obj(VOID);
	FUNC_EXTERN	STATUS	do_del_gen(char *,char *,OOID,OOID);
	FUNC_EXTERN	FE_DEL	*do_expand_name(FE_DEL *,FE_DEL **,OOID);
	FUNC_EXTERN	i4	do_ifile_parse(char *,OOID);


/*
** Constant object data table
*/

#define	MAX_DO_OC	6

	GLOBALCONSTREF OBJ_DEL_DATA obj_dl_data[];

