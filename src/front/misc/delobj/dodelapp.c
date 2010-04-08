/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<lo.h>
# include	<nm.h>
# include	<st.h>
# include	<fe.h>
# include	<ug.h>
# include	<ui.h>
# include	<ooclass.h>
# include	<uigdata.h>
# include	<erde.h>
# include	"doglob.h"


static	char	abDbdir[MAX_LOC] = ERx("");
static	char	abObjdir[MAX_LOC] = ERx("");

static	VOID	abdirinit(VOID);
static	VOID	abdirset(char *);
static	VOID	abdirdestroy(VOID);						 

/*
** DO_DEL_APP -	Delete an application and it's object files.  Note that this
**		is called generically, and so its calling sequence does include
**		unused parameters.
**
** Parameters:
**	oname	- ptr to the name of the application to be deleted.
**	oowner  - ptr to the name of the owner application to be deleted
**		  (not used).
**	oclass  - object class of the application to be deleted.
**	oid	- object id of application to be deleted.
**
** Returns:
**	STATUS	- OK if successful, otherwise reflects any database error(s).
**
** History:
**	12-jan-1993 (rdrane)
**		Written from the DELETER module dlabfdir.c.
*/

STATUS
do_del_app(char *oname,char *oowner,OOID oclass,OOID oid)
{
	STATUS	retstat;


	if  ((retstat = IICUdoDeleteObject(oclass,oid)) != OK)
	{
		return(retstat);
	}

	/*
	** Now delete the object directory
	*/
	abdirinit();
	abdirset(oname);
	abdirdestroy();

	return(OK);
}


/*
** ABDIRINIT -	Move to the object directory for the application.
**
** Parameters:
**	None.
**
** Returns:
**	Nothing.
**
** History:
**	12-jan-1993 (rdrane)
**		Written from the DELETER module dlabfdir.c.
**	17-Jun-2004 (schka24)
**	    Safe env variable handling.
*/

static
VOID							 
abdirinit(VOID)
{
	char		*ptr;
	LOCATION	loc;
	char		node_name[MAX_LOC];
	char		dbname[MAX_LOC];
	char		tbuf[MAX_LOC];
	char		lbuf[MAX_LOC];


	NMgtAt(ERx("ING_ABFDIR"),&ptr);

	STlcopy(ptr,&lbuf[0], sizeof(lbuf)-1);

	LOfroms(PATH,&lbuf[0],&loc);

	STcopy(IIUIdbdata()->database,&tbuf[0]);
	FEnetname(&tbuf[0],&node_name,&dbname[0]);
	LOfaddpath(&loc,&dbname[0],&loc);
	LOtos(&loc,&ptr);

	STlcopy(ptr,&abDbdir[0], sizeof(abDbdir)-1);

	return;
}

/*
** ABDIRSET -	Set the current directory to the target application object
**		directory.
**
** Parameters:
**	oname - ptr to the target application name.
**
** Returns:
**	Nothing.
**
** History:
**	12-jan-1993 (rdrane)
**		Written from the DELETER module dlabfdir.c.
*/

static
VOID							 
abdirset(char *oname)
{
	char		*ptr;
	LOCATION	dbdir;
	char		dbbuf[MAX_LOC];
	char		buf[MAX_LOC];


	STcopy(&abDbdir[0],&dbbuf[0]);

	LOfroms(PATH,&dbbuf[0],&dbdir);

	STcopy(oname,&buf[0]);

	LOfaddpath(&dbdir,&buf[0],&dbdir);
	LOtos(&dbdir,&ptr);

	STcopy(ptr,&abObjdir[0]);

	return;
}


/*
** ABDIRDESTROY - Destroy the directory for a particular application.
**		  This effectively removes all files for the application.
**		  Upon completion, the directory will that reflected by
**		  abObjdir.
**
** Parameters:
**	None.	Uses the directory name previously set in the static global
**		abObjdir.
**
** Returns:
**	Nothing.
**
** History:
**	12-jan-1993 (rdrane)
**		Written from the DELETER module dlabfdir.c.
*/

static
VOID							 
abdirdestroy(VOID)						 
{
	LOCATION	dir;
	char		buf[MAX_LOC];


	STcopy(&abObjdir[0],buf);

	LOfroms(PATH,&buf[0],&dir);
	LOdelete(&dir);

	return;
}
