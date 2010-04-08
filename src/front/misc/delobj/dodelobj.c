/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<er.h>
# include	<si.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<ooclass.h>
# include	<uigdata.h>
# include	<erde.h>
# include	"doglob.h"


/*
** DO_DEL_OBJ - delete a set of FE objects (reports, forms, JoinDefs, etc.)
**		from the FE catalogs.
**
** Parameters:
**	None.
**
** Returns:
**	TRUE		if everything went well.
**	FALSE		if any errors occured.
**
** History:
**	12-jan-1993 (rdrane)
**		Created.
**	11-feb-1993 (rdrane)
**		Conditionally compile with debugging statements (also disables
**		actual deletion).  Ensure that all messages eventually end with
**		a newline.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-apr-2009 (stial01)
**          Use bigger buffer for S_DE0007
*/

bool
do_del_obj(VOID)
{
	i4	i;
	bool	del_status;
	bool	anyerror;
	OBJ_DEL_DATA *odd_ptr;
	char	buf[(2 * FE_MAXNAME) + ER_MAX_LEN + 1];


	/*
	** The FE_DEL list should NEVER be NULL if we got this far!
	*/
	if  (Ptr_fd_top == (FE_DEL *)NULL)
	{
		return(FALSE);
	}

	anyerror = FALSE;

	iiuicw1_CatWriteOn();		/* Enable writing FE catalogs */

	Cact_fd = Ptr_fd_top;
	while (Cact_fd != (FE_DEL *)NULL)
	{
		odd_ptr = &obj_dl_data[0];
		while (odd_ptr->low_ooid != (OOID)0)
		{
			if  ((Cact_fd->fd_type >= odd_ptr->low_ooid) &&
			     (Cact_fd->fd_type <= odd_ptr->hi_ooid))
			{
				break;
			}
			odd_ptr++;
		}

		if  (odd_ptr->low_ooid == (OOID)0)
		{
			/*
			** Internal Error!!!  We should do something
			** about this ...
			*/
 			Cact_fd = Cact_fd->fd_below;
			continue;
		}

		if  (!Dobj_silent)	/* Indicate which object we're on */
		{
			IIUGfmt(&buf[0],sizeof(buf),
				ERget(S_DE0007_Status),
				3,
				ERget(odd_ptr->msg_name),
				Cact_fd->fd_name_info->name_dest,
				Cact_fd->fd_name_info->owner_dest);
			STcat(&buf[0],ERx("\n"));
			SIfprintf(stdout,&buf[0],NULL);
			SIflush(stdout);
		}

# ifndef DO_DEBUG
		del_status =
			(*odd_ptr->delproc)(Cact_fd->fd_name_info->name_dest,
					    Cact_fd->fd_name_info->owner_dest,
					    Cact_fd->fd_type,
					    Cact_fd->fd_id);
# else
		del_status = OK;
		SIfprintf(stdout,"Deleted %s owned by %s\nof type %d via %s\n",
			  Cact_fd->fd_name_info->name_dest,
			  Cact_fd->fd_name_info->owner_dest,
			  Cact_fd->fd_type,
			  &odd_ptr->delproc_name[0]);
# endif

		if ((del_status != OK) && (!Dobj_silent))
		{
			IIUGerr(E_DE0009_GenError,UG_ERR_ERROR,(i4)4,
				(PTR)ERget(odd_ptr->msg_name),
				(PTR)Cact_fd->fd_name_info->name_dest,
				(PTR)Cact_fd->fd_name_info->owner_dest,
				(PTR)&del_status);
			anyerror = TRUE;
		}
 		Cact_fd = Cact_fd->fd_below;
	}

	iiuicw0_CatWriteOff();	/* Disable writing FE catalogs */

	return(!anyerror);
}

/*
** DO_DEL_GEN -	Delete an FE object which requires no special consideration.
**		Note that this is called generically, and so its calling
**		sequence does include unused parameters.
**
** Parameters:
**	oname	- ptr to the name of the FE object to be deleted
**		  (not used).
**	oowner  - ptr to the owner of the FE Object to be deleted
**		  (not used).
**	oclass  - object class of the FE object to be deleted.
**	oid	- object id of FE object to be deleted.
**
** Returns:
**	STATUS	- OK if successful, otherwise reflects any database error(s).
**
** History:
**	12-jan-1993 (rdrane)
**		Written from the DELETER module dlgen.qsc.
*/

STATUS
do_del_gen(char *oname,char *oowner,OOID oclass,OOID oid)
{
	STATUS	del_status;


	del_status = IICUdoDeleteObject(oclass,oid);

	return(del_status);
}
