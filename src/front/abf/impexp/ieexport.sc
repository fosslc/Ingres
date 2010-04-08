/*
**	Copyright (c) 1983, 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<pc.h>
# include	<st.h>
# include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ooclass.h>
# include	<oodep.h>
# include	<oocat.h>
# include       <abclass.h>
# include	<cu.h>
# include	<acatrec.h>
# include	"erie.h"
# include	"ieimpexp.h"

/**
** Name:	ieexport.sc
** 
** Defines:	IIIEiie_IIExport        -  Write application objects to file
**		IIIEfoo_FoundOneObject
**		IIIEaci_AllClassInfo
**
** History:
** 	17-aug-1993 (daver)
**		Documented as First Written. Really written awhile ago. Much of
**		the logic was "borrowed" from abf!copyapp!caout.sc.
**	07-sep-1993 (daver)
**		Documented, cleaned up, and put msgs in erie.msg.
**	27-dec-1993 (daver)
**		Fix bug 58232; don't overwrite the frame's object id in
**		the param lists with the formref's id.
**	19-jan-1994 (daver)
**		Fixed bug 57685; added error checking to report when
**		iiexport can't find what is specified on the command line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN	VOID	IIIEeum_ExportUsageMsg();

char	*IICUrtoRectypeToObjname();		/* copyapp definition */

EXEC SQL INCLUDE SQLCA;
EXEC SQL WHENEVER SQLERROR CALL SQLPRINT;

static	bool	IIIEfoo_FoundOneObject();
static	bool	IIIEaci_AllClassInfo();
static	bool	IIIEtbc_ToBeCopied();
static	STATUS	IIIEcnt_CheckNotThere();
static	VOID	IIIEcaf_CheckAllFlag();
static	VOID	shme1debug();

/*{
** Name:	IIIEiie_IIExport	-  Write application objects to file.
**
** Description:
**	This routine writes all the application objects to a file.
**	It uses the copyutil write routines to do this.
**
** Inputs:
**	char		*appname	The name of the application.
**
**	int		allflags	Bit mask for -allframes, -allprocs, etc
**
**	IEPARAMS	**plist		2-D array of params. Index indicates
**					type of param (frame, proc, etc)
**
**	FILE		*intfile	The file to write to.  The FILE pointer
**					must be open and pointing to a 
**					writeable file.
**
**	FILE		*listfile	If specified, an open and writable 
**					pointer to a FILE which contains the 
**					names of non-generateable source code.
** Outputs:
**	None
**
** Returns:
**	STATUS of the write.
**
** Side Effects:
**	Writes an intermediate file, and, if specified, a listing file of 
**	source files to be transfered to the target application directory
**
** History:
**	30-jun-1993 (daver)
**		First Written.
**	07-sep-1993 (daver)
**		Documented, cleaned up, added listfile support, and 
**		put msgs in erie.msg.
**	19-jan-1994 (daver)
**		Fixed bug 57685; added error checking to report when
**		iiexport can't find what is specified on the command line.
**		Return FAIL if no valid objects were found, informing
**		main routine to delete the intermediate file.
*/
STATUS
IIIEiie_IIExport(appname, allflags, plist, intfile, listfile)
char		*appname;
i4		allflags;
IEPARAMS	**plist;
FILE		*intfile;
FILE		*listfile;
{
	EXEC SQL BEGIN DECLARE SECTION;
	i4	app_class;
	i4	class;
	i4	app_id;
	i4	id;
	EXEC SQL END DECLARE SECTION;

	ACATREC		*app;
	ACATREC		*olist;		/* List of objects in app. */
	ACATREC		*obj;
	STATUS		rval = OK;
	char		long_remark[OOLONGREMSIZE+1];
	i4		foundflags = 0;

	CURECORD	*cu_recfor();

	if (IEDebug)
		shme1debug(plist,allflags);

	cu_wrhdr(ERx("IIEXPORT"), intfile);

	/*
	** IIAMoiObjId is in abf!iaom!iamoid.qsc
	** it is called to pick up the object id and object class for the given
	** application name (although the object class will be OC_APPL)
	**
	** IIAMcgCatGet is in abf!iaom!iamcget.c
	** given the application's object id, IIAMcgCatGet calls iiamCatRead
	** to pick up application info to write (even though this is
	** ignored by iiimport, it might be used later, and it doesn't
	** take long to do this).
	** 
	** Once we have the application info, we call the routines again,
	** this time getting all of the application objects for the given
	** application. Its all part of the IAOM model. Nice, huh?
	**
	** We use the same "application doesn't exist" error message
	** 'cause copyapp did. If any of these catalog queries has a
	** problem, the application might exist, but its catalog entries
	** are corrupted. We'll assume it doesn't exist; why alarm a user
	** about corrupted catalogs when the 98% case is that they mistyped
	** their application name?
	*/

	if (IEDebug)
		SIprintf("\n DEBUG is ON .... \n");

	if ( IIAMoiObjId(appname, OC_APPL, 0, &app_id, &app_class) != OK )
	{
		IIUGerr(E_IE0013_No_Such_App, UG_ERR_ERROR, 1, (PTR) appname);
		return E_IE0013_No_Such_App;
	}
	if (IEDebug)
		SIprintf("appid = %d and class = %d \n Now the CatGet...\n",
		app_id, app_class);

	if ( IIAMcgCatGet(app_id, app_id, &app) != OK)
	{
		IIUGerr(E_IE0013_No_Such_App, UG_ERR_ERROR, 1, (PTR) appname);
		return E_IE0013_No_Such_App;
	}

	if (IEDebug)
		SIprintf("did the catget...\n");


	if ( IICUgtLongRemark(app_id, long_remark) != OK )
	{
		IIUGerr(E_IE0013_No_Such_App, UG_ERR_ERROR, 1, (PTR) appname);
		return E_IE0013_No_Such_App;
	}

	if (IEDebug)
		SIprintf("did the longremark...\n");

	/* this *is* a copyapplib routine!! */
	IICAaslApplSourceLoc(app->source);
	iicaWrite(app, long_remark, 0, intfile);

	if (IEDebug)
		SIprintf("wrote to the file...\n");

	/*
	** This could totally be optimized if performance is deemed to
	** be a problem. We'd need some sort of heuristic, like 
	** if no allflags are used and total number of objects is < N and
	** total number of classes is < M, then go ahead and issue singular
	** fetches for the desired objects. Commom case could optimize on 
	** a single object. Otherwise, I think we're wasting our time.
	**
	** But for now, we'll go ahead and grab all the objects just like
	** copyapp did, and the application will restrict the writing of the
	** file to only those objects we want. Its probably better to send one
	** bigger query than several little ones; again, it wouldn't be
	** tough to put in a "fast" case which optimizes on the single object,
	** single class (i.e., -frame=oneframe) case. Passing in OC_UNDEFINED
	** inidicates "get everything for this application".
	*/
	if ( (rval = IIAMcgCatGet(app_id, (i4)OC_UNDEFINED, &olist)) != OK)
	{
		IIUGerr(E_IE0014_Bad_CatGet, 0, 2, (PTR) appname, (PTR) &rval);
		return E_IE0014_Bad_CatGet;
	}

	for (obj = olist; obj != NULL; obj = obj->next)
	{
		if (IEDebug)
			SIprintf("class is %d, id is %d, name is %s\n",
			obj->class, obj->ooid, obj->name);

		if (IIIEfoo_FoundOneObject(plist, allflags, &foundflags, obj))
		{
			if ( IICUgtLongRemark(obj->ooid, long_remark) != OK )
			{
				IIUGerr( E_IE0015_Err_LongRemark, 0,
					2, (PTR)appname, (PTR)obj->name);
				return E_IE0015_Err_LongRemark;
			}
			else
			{
				if (IEDebug)
				{
					SIprintf("\n WE ARE WRITING:\n");
					SIprintf("\tclass=%d, id=%d, name=%s\n",
					obj->class, obj->ooid, obj->name);
					SIprintf("\tflags=%d, source=%s\n\n",
						obj->flags, obj->source);
				}

				iicaWrite(obj, long_remark, 1, intfile);
			}
			/* 
			** tell the world, unless the object is a formref.
			** we'll print the form messages in the cursor loop, 
			** below.
			*/
			if (obj->class != OC_AFORMREF)
				IIUGmsg(ERget(S_IE0004_Wrote_One), FALSE, 2, 
				(PTR)obj->name, 
				(PTR)IICUrtoRectypeToObjname(obj->class));

			if ( (listfile != NULL) && 
			      IIIEtbc_ToBeCopied(obj->class, obj->flags) )
			{
				if (IEDebug)
					SIprintf("LIST file:%s\n", obj->source);
				SIfprintf(listfile, "%s\n", obj->source);
			}
		}
	}

	/* if nothing was found, return fail. file will be deleted */
	if (IIIEcnt_CheckNotThere(plist, allflags, foundflags) != OK)
		return FAIL;

	/*
	** At this point all the records for the application and the
	** objects in the application have been written.  Now must
	** write the definitions for the dependent objects in the application
	** like the forms and report.
	**
	** Start a transaction so that the cursor
	** and other queries can run at the same time
	**
	*/
	IIUIbeginXaction();
	/*
	** Fix for BUG 1519.
	** Do a select distinct so only one copy of
	** the object is written to the file.
	**
	** Need to join to ii_objects so we only fetch legitimate objects
	** which have not been deleted.
	**
	** No nned to join to ii_abfobjects now that application id is in 
	** ii_abfdependncies.
	*/

	if (IEDebug)
		SIprintf("\ncursor fetch...\n\n");

	EXEC SQL DECLARE iidepcur CURSOR FOR
	SELECT DISTINCT d.abfdef_name, d.object_class, o.object_id
	FROM ii_abfdependencies d, ii_objects o
	WHERE d.abfdef_applid = :app_id			
	AND d.object_id = o.object_id
	AND d.abfdef_deptype = 3502;	/* OC_DTDBREF */

	EXEC SQL OPEN iidepcur;
	/*
	** Loop is broken out of inside
	*/
	while (sqlca.sqlcode == 0)
	{
		register CURECORD	*curec;
	    	char			*classname;
		EXEC SQL BEGIN DECLARE SECTION;
		char			name[FE_MAXNAME+1];
		EXEC SQL END DECLARE SECTION;
	
		EXEC SQL FETCH iidepcur INTO :name, :class, :id;
		if (sqlca.sqlcode != 0)
		    break;
		/*
		** Fix for BUG 1511.
		** If the name if empty don't try and
		** copy the object.
		*/
		if ( STtrmwhite(name) <= 0 )
		    continue;

		if (IEDebug)
			SIprintf("cursor: class is %d, id is %d, name is %s\n",
			class, id, name);
		/*
		** If the dependency is an AFORMREF, then the object
		** to copy is a form.
		*/
		if (class == OC_AFORMREF)
		    class = OC_FORM;
		/*
		** Don't try to copy out ILCODE, database procedures or 
		** dummy records (fix for copyapp bug #36919: add OC_DBPROC 
		** to the list below)
		*/
		if (class == OC_ILCODE || class == OC_DBPROC || class == 0)
		    continue;
	
		if (!(IIIEfco_FoundCursorObject(plist[EXP_FRAME], 
						allflags, id)))
		    continue;
	
		if (IEDebug)
		{
			SIprintf("\nCURSOR!!!! WE ARE WRITING:\n");
			SIprintf("\tclass is %d, id is %d, name is %s\n\n",
			class, id, name);
		}
	
		if ( 	((curec = cu_recfor(class)) == NULL) || 
			(curec->cuoutfunc == NULL )	)
		{
			i4	t;
	
			t = class;
			IIUGerr(E_IE0016_No_Outfunc, UG_ERR_ERROR, 2, 
							(PTR)name, (PTR)&t);
			rval = E_IE0016_No_Outfunc;
			continue;
		}
		/*
		** print "copying" messages AFTER the fact, even though
		** it is a little misleading.  We don't want to print
		** messages for things we did not copy.  We DON'T call with
		** TRUE for "notthereok", because then we would not get
		** reasonable return status back.
		*/
		classname = IICUrtoRectypeToObjname(class);
		if ((*curec->cuoutfunc)(name, class, FALSE, intfile) == OK)
		{
			if (classname != NULL)
			{
				IIUGmsg(ERget(S_IE0004_Wrote_One), FALSE, 2,
						(PTR)name, (PTR)classname);
		    	}
		}
		else
		{
			/* copyapp only printed for forms */
			IIUGerr(E_IE0017_No_Class_Thing, UG_ERR_ERROR, 2, 
						(PTR)name, (PTR)classname);
		}

	} /* end while sqlcode == 0 */

	EXEC SQL CLOSE iidepcur;
	IIUIendXaction();

	return rval;
}


/*{
** Name: IIIEcnt_CheckNotThere      -- check if cmd-line objects are "there"
**
** Description:
**	Checks thru what was specified on the command line to make sure
**	each flag instance resulted in finding some application object.
**	Reports to the user if something wasn't found; returns FAIL if
**	nothing on the command line was found (which informs the callers
**	to delete the intermediate file).
**
** Input:
**	IEPARAMS    **plist	- list of explicit objects (e.g, -frame=blah)
**	i4	    allflags	- bitmask of the possible -all flags
**	i4	    foundflags	- bitmask of which -all flags found objects
**
** Output:
**	None
**
** Returns:
**	OK if at least one object was found
**	FAIL otherwise
**
** Side Effects:
**	Prints a message if object(s) were specified on the command
**	line, either explicitly or via the -allxxx mechanism, and were not 
**	found for the given application.
**
** History:
**      19-jan-1994 (daver)
**      	First Written/Documented, to fix bug 57685.
*/
static STATUS
IIIEcnt_CheckNotThere(plist, allflags, foundflags)
IEPARAMS	**plist;
i4		allflags;
i4		foundflags;
{
	i4		i;
	IEPARAMS	*p;
	bool		nothing_found = TRUE;


	/* first flip thru the paramlist; then check the allflags */

	for (i = 0; i < EXP_OBJECTS; i++)
	{
		p = plist[i];
		while (p != NULL)
		{
			if (p->id == -1)
			{
				IIUGerr(E_IE0041_No_Specified_Object, 
					UG_ERR_ERROR, 1, (PTR)p->name);
			}
			else
			{
				nothing_found = FALSE;
			}
			p = p->next;

		}
	}
	IIIEcaf_CheckAllFlag( (allflags & ALLFRAMES), (foundflags & ALLFRAMES), 
			&nothing_found, ERx("-allframes"),
			ERx("frames") );

	IIIEcaf_CheckAllFlag( (allflags & ALLPROCS), (foundflags & ALLPROCS), 
			&nothing_found, ERx("-allprocs"),
			ERx("procedures") );
	
	IIIEcaf_CheckAllFlag( (allflags & ALLRECS), (foundflags & ALLRECS), 
			&nothing_found, ERx("-allrecords"),
			ERx("records") );

	IIIEcaf_CheckAllFlag( (allflags & ALLGLOBS), (foundflags & ALLGLOBS), 
			&nothing_found, ERx("-allglobals"),
			ERx("globals") );

	IIIEcaf_CheckAllFlag( (allflags & ALLCONSTS), (foundflags & ALLCONSTS), 
			&nothing_found, ERx("-allconstants"),
			ERx("constants") );

	if (nothing_found)
	{
		IIUGerr(E_IE0042_Nothing_Found, UG_ERR_ERROR, 0);
		/* return FAIL will remove the file */
		return (FAIL);
	}

	return (OK);
}

		

/*{
** Name: IIIEcaf_CheckAllFlag      -- check if -allxxxx hit paydirt
**
** Description:
**	Given a specific "allflag" (e.g, -allframes, -allprocs, etc),
**	this routine checks the corresponding flag in "foundflags" to 
**	see if some component of that type was actually found.
**
** Input:
**	bool	allset		- was -allflags set?
**	bool	foundset	- did we find anything of this type?
**	bool	*nothing_found	- if we did, set this to FALSE
**	char	*flagname	- if not, the flag's name for error reporting
**	char	*companme	- and the component's name for error reporting
**
** Output:
**	bool	*nothing_found	set to FALSE if something was found
**
** Returns:
**	None
**
** History:
**      19-jan-1994 (daver)
**      	First Written/Documented, to fix bug 57685.
*/
static VOID
IIIEcaf_CheckAllFlag(allset, foundset, nothing_found, flagname, compname)
bool	allset;
bool	foundset;
bool	*nothing_found;
char	*flagname;
char	*compname;
{

	if (allset)
	{
		if (foundset)
			*nothing_found = FALSE;
		else
			IIUGmsg(ERget(S_IE0040_No_Allflag_Object), 
					FALSE, 2, (PTR)flagname,
					(PTR)compname);
	}
}


/*{
** Name: IIIEtbc_ToBeCopied      -- does object have source code to be copied?
**
** Description:
**      Returns true if object is an User Frame, a 4GL/3GL Procedure, or 
**	a Customized Vision Frame. 
**
** Input:
**	i4	class		class of object
**	i4	flags		abf flags for object; is it Customized?
**
** Output:
**	None
**
** Returns:
**	TRUE if object has source to be copied
**	FALSE otherwise
**
** History:
**      8-sep-1993 (daver)
**      	First Written/Documented.
*/
static bool
IIIEtbc_ToBeCopied(class, flags)
i4	class;
i4	flags;
{
	if (IEDebug)
		SIprintf("In TBC, class = %d, flags = %d, cust = %d, and ", 
			class, flags, flags & CUSTOM_EDIT);

	switch (class)
	{
		case OC_APPLPROC:
		case OC_HLPROC:
		case OC_OSLPROC:
		case OC_OSLLOCALPROC: 
		case OC_OSLFRAME: 
		case OC_OLDOSLFRAME: 

			if (IEDebug)
				SIprintf("an OSL frame/proc\n");

			return TRUE;

		case OC_MUFRAME:
		case OC_APPFRAME:
		case OC_UPDFRAME:
		case OC_BRWFRAME:
			
			if (IEDebug)
				SIprintf("a CUSTOM EDIT\n");

			return (flags & CUSTOM_EDIT); 

		default:
			
			return FALSE;
	}
}


/*{
** Name: IIIEfoo_FoundOneObject      -- object specified on command line?
**
** Description:
**      Returns TRUE if the object in question was either explicitly
**	mentioned on the command line, or if covered in an "-all" flag syntax
**
** Input:
**
**	IEPARAMS	**paramlist	2D array of explicitly mentioned objs
**	i4		allflags	bit mask for all possible -all flags
**	i4		*foundflags	bits get set when objs get found
**	ACATREC		*obj		the object in question
**
** Output:
**	None
**
** Returns:
**	TRUE if object was specified
**	FALSE otherwise
**
** History:
**      18-aug-1993 (daver)
**	      First Written/Documented.
**	27-dec-1993 (daver)
**		Don't set the id in the paramlist if the object is a
**		formref; dependencies on forms are on the frame, not the
**		formref object in the database. Fixes bug 58232.
**	19-jan-1994 (daver)
**		Added foundflags param; when we find an object based on
**		an -allflags parameter, set the correspoinding bit in 
**		the foundflags bitmask as well. Fixed bug 57685.
*/
static bool
IIIEfoo_FoundOneObject(paramlist, allflags, foundflags, obj )
IEPARAMS	**paramlist;
i4		allflags;
i4		*foundflags;
ACATREC		*obj;
{
	i4		i;
	IEPARAMS	*p;
	bool		found;

	/*
	** never want to "find" the appl record; we don't manipulate
	** the application object at all
	*/
	if (obj->class == OC_APPL)
		return FALSE;
	/*
	** check allflags based on class. 
	** if allflags for this class is set, return TRUE
	** while we're at it, convert class into param list array, too 
	*/
	if (IIIEaci_AllClassInfo(obj->class, allflags, foundflags, &i))
		return TRUE;

	/* use p. save the extra array references (and characters) */
	p = paramlist[i];

	/* search list for name; if found, load id, and return true */
	found = FALSE;
	while (p != NULL)
	{
		if (STcompare(obj->name, p->name) == 0)
		{
			found = TRUE;
			/*
			** don't set the id if the object is a formref;
			** in this case, the id should be the id of the
			** frame in question. return TRUE, so the
			** formref record gets written to the file.
			** Fixes bug 58232
			*/
			if (obj->class != OC_AFORMREF)
				p->id = obj->ooid;
			break;
		}
		p = p->next;
	}

	return (found);
}



/*{
** Name: IIIEaci_AllClassInfo      -- class covered by its -all flag?
**
** Description:
**      Returns TRUE if the class of object has been covered by the
**	appropriate -all flag (e.g., one of the many frame classes are all
**	covered by the "-allframes" flag). Based on class, sets index to
**	point to the right index in the 2D paramlist array.
**
** Input:
**	i4      	class		class of object
**	i4     	flags		bit mask of all the -allflags
**	i4		*foundflags	bitmask of the -allflags found
**	i4     	*index		ptr to integer index 
**
** Output:
**	i4		*index		set to correct index for 2D paramlist
**
** Returns:
**	TRUE if class is covered by its allflag component
**	FALSE otherwise
**
** History:
**      18-aug-1993 (daver)
**	      First Written/Documented.
**	8-sep-1993 (daver)
**		Really Documented, cleaned, and ER-messaged.
**	19-jan-1994 (daver)
**		Added foundflags param; when we find an object based on
**		an -allflags parameter, set the correspoinding bit in 
**		the foundflags bitmask as well. Fixed bug 57685.
*/
static bool
IIIEaci_AllClassInfo(class, flags, foundflags, index)
i4	class;
i4	flags;
i4	*foundflags;
i4	*index;
{

	if ( (class > OC_APPLPROC) && (class < OC_AMAXPROC))
	{
		*index = EXP_PROC;
		if (flags & ALLPROCS)
			*foundflags = (*foundflags | ALLPROCS);
		return (flags & ALLPROCS);
	}

	if (class == OC_GLOBAL) 
	{
		*index = EXP_GLOB;
		if (flags & ALLGLOBS)
			*foundflags = (*foundflags | ALLGLOBS);
		return (flags & ALLGLOBS);
	}

	if (class == OC_CONST)
	{
		*index = EXP_CONST;
		if (flags & ALLCONSTS)
			*foundflags = (*foundflags | ALLCONSTS);
		return (flags & ALLCONSTS);
	}

	if (class == OC_RECORD)
	{
		*index = EXP_REC;
		if (flags & ALLRECS)
			*foundflags = (*foundflags | ALLRECS);
		return (flags & ALLRECS);
	}

	if ( (class > OC_APPLFRAME && class < OC_AMAXFRAME) ||
	     (class == OC_AFORMREF) )
	{
		*index = EXP_FRAME;
		if (flags & ALLFRAMES)
			*foundflags = (*foundflags | ALLFRAMES);
		return (flags & ALLFRAMES);
	}
	
	/* impossible condition check and message. should never get here */
	IIUGerr(E_IE0018_Unknown_Class, UG_ERR_ERROR, 1, (PTR)class);

	*index = 0; /* set to something, don't AV */
	return FALSE;
}



static VOID
shme1debug(parray, allflags)
IEPARAMS	**parray;
int		allflags;
{
	i4	i;
	char	s;

	SIprintf("\noh, hello, we are IIEXPORT\n\n");
	SIprintf("params are:\n");
	for (i = 0; i < EXP_OBJECTS; i++)
	{
		switch (i)
		{
		  case EXP_FRAME:
			if (parray[i] != NULL)
				SIprintf("\tframes\t= %s\n", parray[i]->name);
			break;
		  case EXP_PROC:
			if (parray[i] != NULL)
				SIprintf("\tprocs\t= %s\n", parray[i]->name);
			break;
		  case EXP_GLOB:
			if (parray[i] != NULL)
				SIprintf("\tglobals\t= %s\n", parray[i]->name);
			break;
		  case EXP_CONST:
			if (parray[i] != NULL)
				SIprintf("\tconsts\t= %s\n", parray[i]->name);
			break;
		  case EXP_REC:
			if (parray[i] != NULL)
				SIprintf("\trecords\t= %s\n", parray[i]->name);
			break;
		  default:
			SIprintf("WHOA!!!!! NELLIEEEE!!!!!\n");
			break;
		}
	}
	if (allflags & ALLFRAMES)
		SIprintf("\t and ALL FRAMES\n");
	if (allflags & ALLPROCS)
		SIprintf("\t and ALL PROCS\n");
	if (allflags & ALLGLOBS)
		SIprintf("\t and ALL GLOBS\n");
	if (allflags & ALLRECS)
		SIprintf("\t and ALL RECS\n");
	if (allflags & ALLCONSTS)
		SIprintf("\t and ALL CONSTS\n");

	SIprintf("\t By the way, all is %d\n",allflags);
}
