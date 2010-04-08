/*
**	Copyright (c) 2004 Ingres Corporation
*/

/*
** Name:    doglobi.h -	FE Object Deletion Utility Global Declarations.
**
** Description:
**	Contains declarations for all the functions, types, and global variables
**	which are specific to the FE Object Deletion Utility (.
**
**	This version of the file deals with the actual allocation of variables.
**
**	THIS FILE MUST BE KEPT IN SYNC WITH doglob.h !!!
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
	OOID	low_ooid;	/* Low value of object_id for match	*/
	OOID	hi_ooid;	/* High value of object_id for match	*/
	ER_MSGID msg_name;	/* Name of message to issue as info	*/
	STATUS	(*delproc)();	/* Address of actual deletion procedure	*/
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
	GLOBALDEF	FE_DEL	*Ptr_fd_top = (FE_DEL *)NULL;
			/*
			** Current FE_DEL being acted upon
			*/
	GLOBALDEF	FE_DEL	*Cact_fd = (FE_DEL *)NULL;
			/*
			** File containing FE object names to delete
			*/
	GLOBALDEF	char	*Dobj_dfile = NULL;
			/*
			** Target database name
			*/
	GLOBALDEF	char  *Dobj_database = NULL;
	GLOBALDEF	char  *Dobj_rep_owner = NULL;
			/*
			** Database Connection parameters (user,
			** group, and password)
			*/
	GLOBALDEF char  *Dobj_uflag = NULL;
	GLOBALDEF char  *Dobj_gidflag = NULL;
	GLOBALDEF char  *Dobj_passflag = NULL;

			/*
			** Enable wildcard expansion if TRUE
			*/
	GLOBALDEF bool  Dobj_wildcard = FALSE;

			/*
			** Display status and/or error messages if TRUE
			*/
	GLOBALDEF bool  Dobj_silent = FALSE;

			/*
			** Prompt string containing
			** object type being deleted.
			*/
	GLOBALDEF char  *Dobj_pr_str = NULL;

/*
**	External Function Declarations
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

	GLOBALCONSTDEF OBJ_DEL_DATA obj_dl_data[(MAX_DO_OC + 1)] =
	{
    		{OC_APPL,	OC_APPL,	F_DE0003_App,	do_del_app,
		 ERx("do_del_app")},
		{OC_FORM,	OC_FORM,	F_DE0004_Form,	do_del_gen,
		 ERx("do_del_gen")},
		{OC_GRAPH,	OC_GRAPH,	F_DE0005_Graph,	do_del_gen,
		 ERx("do_del_gen")},
		{OC_JOINDEF,	OC_JOINDEF,	F_DE0006_Jdef,	do_del_gen, 
		 ERx("do_del_gen")},
		{OC_QBFNAME,	OC_QBFNAME,	F_DE0007_Qbfnm,	do_del_gen,
		 ERx("do_del_gen")},
		{OC_REPORT,	OC_RBFREP,	F_DE0008_Rep,	do_del_gen,
		 ERx("do_del_gen")},
		{(OOID)0,	(OOID)0,	(ER_MSGID)NULL,	(STATUS (*)())
									NULL,
		 ERx("")},
	};

