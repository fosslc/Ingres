/*
** Copyright (c) 1982, 2005 Ingres Corporation
**
*/

#include	<compat.h>
#include	<ut.h>		 
#include	<pe.h>		 
#include	<nm.h>
#include	<lo.h>
#include	<st.h>
#include	<si.h>
#include	<tm.h>
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<uf.h>
#include	<abclass.h>
#include	<abftrace.h>
#include	<abfcnsts.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<abfcompl.h>
#include	"abfgolnk.h"
#include	"ablink.h"
#include	"abffile.h"
#include	"erab.h"
#include	"abut.h"

/**
** Name:	ablink.c -	ABF Link Application Module.
**
** Description:
**	Contains all the routines used to link an ABF application image.
**	Defines:
**
**	IIABlnkInit()	initialize for linking.
**	IIABlnxExtFile() initialize link extract file.
**	IIABilnkImage()	get and link an executable image for an application.
**	iiabInterpLink() link the interpreter for an application.
**
** History:
**	Revision 2.0  82/07  joe
**	Initial revision.
**
**	Revision 6.0  87/06  wong
**	Extracted 'ablnkapp()' to "abimgfrm.qc".  Moved 'IIABlnxExtFile()' here
**	from "abfdir.c", 02/88.
**
**	23-jun-1988 (Joe)
**		Added 5.0 CCB change 3464.  Call the program error
**		routine if the loader fails.
**
**	21-sep-88 (kenl)
**		Changed MEcalloc() calls to MEreqmem() calls.
**	Revision 6.1  88/11  wong
**	Replaced IMTDY conditional logic with not NODY.
**
**	Revision 6.2  89/07/12  wong
**	Check for executable existence as part of link.  Return error if
**	it does not exist (even though 'UTlink()' returned OK.)  JupBug #3690.
**	89/01  wong Added ABLNKTST parameter to 'IIABtestLink()' (which was
**	renamed from 'ablnkrun()'.)
**
**	Revision 6.3/03/00  89/11  wong
**	Modified 'iiabImgLink()' to pass through error display function to
**	'iiabAppCompile()'.  Moved 'IIABtestLink()' to "abtstbld.c".
**
**	90/03 (Mike S)  Redirect link output into a file.
**      05-24-1990 (fredv)
**              Renamed abfmain.obj and abfimain.obj to abfmain.o and
**              abfimain.o for ris_us5. The IBM RS/6000
**              C compiler doesn't like this suffix.
**  25-Apr-1991 (mguthrie)
**		BULL linker has same problem as ris_us5. 
**	14-aug-91 (blaise)
**		Added support for (currently undocumented) imageapp -nocompile
**		flag. If this flag has been set, don't compile source files,
**		and check that all object files exist before linking.
**	22-aug-91 (blaise)
**		Removed reference to locp->fname from the last change, since
**		the LOCATION structure is machine-specific. Instead use LOtos
**		to obtain the file name.
**
**	30-aug-91 (leighb) Revived ImageBld.
**
**	10-oct-92 (leighb) Load correct Image Name before linking when
**		doing an ImageBld.
**
**	14-oct-92 (leighb) use *.obw for Tools for Windows
**
**	17-Dec-92 (fredb)
**		Porting change for MPE/iX (hp9_mpe) to _buildExtract.
**      12-oct-93 (daver)
**              Casted parameters to MEcopy() to avoid compiler warnings 
**		when this CL routine became prototyped.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**      09-feb-95 (chech02)
**              Added rs4_us5 for AIX 4.1.
**	10-may-1999 (walro03)
**		Remove obsolete version string bu2_us5, bu3_us5.
**      02-Aug-1999 (hweho01)
**              Added support for ris_u64 (AIX 64-bit platform).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Jun-2004 (schka24)
**	    Safer handling of env vars.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	25-Aug-2009 (kschendel) b121804
**	    iiabAppCompile type was wrong, fix.
*/

GLOBALREF	char **IIar_Dbname;
GLOBALREF	char *IIabRoleID;
GLOBALREF	bool IIabKME;
GLOBALREF	i4  IIOIimageBld;		 

LOCATION	*iiabMkObjLoc();
LOCATION	*abfilsrc();
bool		iiabAppCompile();
char		*IIUIroleID();

static STATUS	_getImageObjs();
static STATUS	_allocObjList();
static STATUS	_link();
static STATUS	_lnkstart();

static bool	_chkHLdates();
static bool	_chkHLsrc();
static STATUS	_buildExtract();

/*{
** Name:	IIABlnkInit() -	Initialize for Linking Applications.
**
** Description:
**	Initialize the link application module.  If dynamic linking is to be
**	supported for the calling program, and the user also wishes to use
**	dynamic linking (specified through the "II_ABF_RUNOPT" environment
**	variable,) then initialize and check the symbol table for the calling
**	program.
**
** Input:
**	dynamiclink	{bool}  Whether to initialize the dynamic linking (DY)
**						module.
** History:
**	Written 7/26/82 (jrc)
**	14-aug-1986 (Joe) Moved initialization of abfmain file to here.
**	22-feb-90 (mgw)  NMgtAt() is a void, don't check it's return value.
*/

static LOCATION	Abrtsmain = {0};
static char	Abrtsbuf[MAX_LOC+1];
static LOCATION	Abrtsimain = {0};
static char	Abrtsibuf[MAX_LOC+1];

VOID
IIABlnkInit (dynamiclink)
bool	dynamiclink;
{
	LOCATION	tloc;
	char		locbuf[MAX_LOC+1];
	char		*cp;

#ifdef txROUT
	if (TRgettrace(ABTROUTENT, -1))
		abtrsrout("", (char *) NULL);
#endif

#ifdef useDY
	if (dynamiclink)
	{
		if (IIabRunOpt == ABRUN_UNDEF)
		{
			char	*opt;

			NMgtAt("II_ABF_RUNOPT", &opt);
			IIabRunOpt =
				( opt == NULL || *opt == EOS ||
					STbcompare(opt, STlength(opt),
						"dynamic",0,TRUE) == 0 )
				? ABRUN_DYN : ABRUN_IMG;
		}

#ifndef NODY
		if (IIabRunOpt == ABRUN_DYN)
		{
			LOCATION	stab;

			NMgtAt("ING_ABF_STB", &cp);
			if ( cp == NULL || *cp == EOS )
			{
				NMloc(FILES, FILENAME, ABSTAB, &tloc);
			}
			else
			{
				STlcopy(cp, locbuf, sizeof(locbuf)-1);
				LOfroms(PATH, locbuf, &tloc);
				LOfstfile(ABSTAB, &tloc);
			}
			LOcopy(&tloc, locbuf, &stab);

			if (DYinit(&stab, TRUE) != OK)
			{
				abproerr("IIABlnkInit", ABBADSYM, locbuf,
					(char *)NULL
				);
			}
		}
#endif /* not NODY */
	}
#endif /* use DY */

#ifdef txROUT
	if (TRgettrace(ABTROUTEXIT, -1))
		abtrerout("", (char *) NULL);
#endif

	/*
	** Initialize the special files Abrtsmain, Abrtsimain.
	*/
	NMgtAt("II_ABFMAIN", &cp);
	if ( cp == NULL || *cp == EOS )
	{
# ifdef CMS
		NMloc(LIB, FILENAME, "abfmain.text", &tloc);
# else
#  if defined(any_aix)
                NMloc(LIB, FILENAME, "abfmain.o", &tloc);
#  else
#   ifdef PMFEWIN3						 
                NMloc(LIB, FILENAME, "abfmain.obw", &tloc);	 
#   else							 
                NMloc(LIB, FILENAME, "abfmain.obj", &tloc);
#   endif /* PMFEWIN3 */					 
#  endif /* aix */
# endif /* CMS */
		LOtos(&tloc, &cp);
	}
	STlcopy(cp, Abrtsbuf, sizeof(Abrtsbuf)-1);
	_VOID_ LOfroms(PATH&FILENAME, Abrtsbuf, &Abrtsmain);

	NMgtAt("II_ABFIMAIN", &cp);
	if ( cp == NULL || *cp == EOS )
	{
# ifdef CMS
		NMloc(LIB, FILENAME, "abfimain.text", &tloc);
# else
#  if defined(any_aix)
                NMloc(LIB, FILENAME, "abfimain.o", &tloc);
#  else
#   ifdef PMFEWIN3						 
                NMloc(LIB, FILENAME, "abfimain.obw", &tloc);	 
#   else							 
                NMloc(LIB, FILENAME, "abfimain.obj", &tloc);
#   endif /* PMFEWIN3 */					 
#  endif /* aix  */
# endif /* CMS */
		LOtos(&tloc, &cp);
	}
	STlcopy(cp, Abrtsibuf, sizeof(Abrtsibuf)-1);
	_VOID_ LOfroms(PATH&FILENAME, Abrtsibuf, &Abrtsimain);
}

/*{
** Name:	IIABlnxExtractFile() -	Initialize the Link Extract File.
**
** Description:
**	This routine initializes the special link extract file object for an
**	application given the object code directory.  This file is created
**	whenever an application is linked and will contain source code (either
**	C or assembler, depending on the system) that defines the run-time
**	structures that define the application, and which references the object
**	code symbols that comprise it.
**
** Input:
**	object_dir	{LOCATION *}  The path of the object code directory
**					for the application.
**
** History
**	1-sep-1986 (Joe) -- Moved to "abfdir.c".
**	02/88 (jhw) -- Moved here from "abfdir.c" to localize to this module.
**			Also, simplified and changed to use static allocation.
**      11-Dec-89 (mrelac)
**              Modified for MPE/XL (hp9_mpe).  The algorithm for this function
**              doesn't work for MPE/XL; trying to LOfstfile() a location with
**              a NULL group (which is what this code produces) causes the
**              resulting path & filename to be 'mashed' by a special filter.
**              One way to prevent filename filtration is not to call
**              LO[f]setfile.  We just build the path and filename from the
**              parameters supplied.
**              Note also that we use ABEXTNAM for the source file name without
**              extension.  This define was added to abfcnsts.h.
**              Also added a temporary location variable called 'tmploc'.
**	07/90 (jhw) -- Modified to get 'object_dir' as location.
*/

static ABFILE	ExtractFile = {
			"",		/* source directory */
			ABEXTSCR,   /* (const) source file name and extension */
# ifdef hp9_mpe
                        ABEXTNAM,       /* (const) source file name */
# else
			"abextract",	/* (const) source file name */
# endif
			{NULL},		/* object code location */
			NULL,		/* object code file name buffer */
			1		/* reference count */
};

static char	ExtractObj[MAX_LOC+1] = {EOS};

VOID
IIABlnxExtFile (object_dir)
LOCATION	*object_dir;
{
# ifdef hp9_mpe
        LOCATION        *tmploc;
# endif

	/*
	** Set the object code directory for the file.
	*/
	LOtos(object_dir, &ExtractFile.fisdir);

	/*
	** Now set the location of the object code file for the extract file.
	*/
# ifdef hp9_mpe
	tmploc = iiabMkLoc (ExtractFile.fisdir, ABEXTOBJ);
        LOcopy (tmploc, ExtractObj, ExtractFile.fiofile);
# else
	LOcopy(object_dir, ExtractObj, &ExtractFile.fiofile);
	_VOID_ LOfstfile(ABEXTOBJ, &ExtractFile.fiofile);
# endif
}

/*{
** Name:	IIABilnkImage() -	Get and Link Image for an Application.
**
** Description:
**	Fetches the definition of an application from the database, 
**	sets the object code directory (and creates it if necessary)
**	and then links the applicaition.
**
** Inputs:
**	appname		The name of the application to link.
**
**	executable	The name to give the linked executable file.
**			If empty, then it points to a buffer that should
**			be filled with the name of the linked executable
**			file that is created by default.
**			If it is NULL, then the executable should be given
**			a default name.
**
**	options		{nat}  Compilation options; force, object-code, etc.
**
** Outputs:
**	executable	If it is empty on input, then fill the buffer
**			it points to with the name of the executable
**			file created (The buffer should be at least MAX_LOC+1
**			characters long).   Note on a failure, the executable
**			name is not guarenteed to be put in this parameter.
**
** History:
**	29-Feb-1988 (Joe)
**		Changed so caller could get name of the executable if
**		default name is used.
**	28-oct-88 (mgw) Bug #2874
**		Don't call IIABsdirSet() or IIABcdirCreate() here. They'll
**		get called by IIABgcatGet() if the application is cataloged.
**	02/22/90 (Mike S) Jupbug # 20178
**		Return error if we can't fetch the application.
**	11/90 (Mike S)	Check for default role
*/
static VOID	_exeName();

STATUS
IIABilnkImage ( appname, id, executable, options )
char	*appname;
OOID	id;
char	*executable;
i4	options;
{
	APPL	app;
	STATUS 	status;
	char	buf[MAX_LOC+1];
	ABLNKARGS link_args;

	app.ooid = id;
	app.data.inDatabase = FALSE;
	app.roleid = ERx("");
	if ( (status = IIABappFetch(&app, appname, OC_UNDEFINED, TRUE)) != OK )
		return status;

	if (executable == NULL)
		executable = buf;
	else if (*executable == EOS)
	{
		_exeName(&app, executable);
	}

	/* 
	** If a role was specified on the command line, we'll use it.
	** Otherwise, if the application has a default role, and Knowledge 
	** Management is present, use the default role.
	*/
	if ((*IIabRoleID == EOS) && IIabKME && (*app.roleid != EOS))
	{
		static char buf[FE_MAXNAME + 3]; /* "-R<role>" */

		/* Get the role's password */
		STpolycat(2, ERx("-R"), app.roleid, buf);
		if ((IIabRoleID = IIUIroleID(buf)) == NULL)
			IIabRoleID = buf;
	}

	if (IIOIimageBld)				 
	{
		ABLNKTST	test_image;

		test_image.user_link  = FALSE;
		test_image.one_frame  = NULL;
		test_image.plus_tree  = FALSE;
		test_image.abintimage = executable;	 

		return(IIABarunApp(&app, &test_image, app.start_name, 
		  ( app.start_proc || app.batch ) ? OC_UNPROC : OC_APPLFRAME));
	}
	else						 
	{
		link_args.link_type = ABLNK_IMAGE;
		link_args.app = &app;
		link_args.executable = executable;
		link_args.options = ABF_ERRIMMED|options;

		return IIABlkaLinKApplication(&link_args);
	}						 
}

/*
** Name:	_exeName() -	Generate Default Name for Application Image.
**
** Description:
**	Generates the default name for an ABF image file from the application
**	image file name, or if that is not specified, from the application
**	name.
**
**	It uses the value of ABIMGEXT.
**
** Inputs:
**	app	{APP *} The application object.
**
** Outputs:
**	buf	{char []}  Address of buffer that will contain generated name.
**
** History:
**	9-jun-1987 (Joe)
**		Moved from ablnkdef.
**	10/14/88 (Elein)
**		B2313: Use the application name (abname) instead of abexname
**		to generate the image name.  The image name is no longer
**		stored in abexname.
**	02/89 (jhw)  Use the application image file name if specified.  Also,
**			renamed from 'ablnkname()'.
*/
static VOID
_exeName ( app, buf )
register APPL	*app;
char		buf[];
{
	if ( app->executable != NULL && *app->executable != EOS )
		STcopy( app->executable, buf );
	else
	{
#ifndef CMS
		_VOID_ STprintf(buf, "%s%s", app->name, ABIMGEXT);
#else
		_VOID_ STprintf(buf, "%s%s%s", app->source, app->name,ABIMGEXT);
#endif
	}
}

/*
** Name:	set_file() -	Set Link Options File Location.
**
** Description:
**	Set the input location (plus buffer) for the input environment logical
**	or file name if the former is not set.
**
** Inputs:
**	filename	{char *}  The link options file name (relative to
**					II_CONFIG.)
**	envar		{char *}  The name of the environment logical.
**
** Outputs:
**	buf		{char *}  The location buffer, which will contain the
**					pathname for the location.
**	loc		{LOCATION *}  The location.
**
** History:
**	04/89 (jhw) -- Written.
**	22-feb-90 (mgw)  NMgtAt() is a void, don't check it's return value.
*/
static VOID
set_file ( filename, envvar, buf, loc )
char		*filename;
char		*envvar;
char		*buf;
LOCATION	*loc;
{
	char		*logical;
	LOCATION	tloc;
	char		lbuf[MAX_LOC+1];

	NMgtAt(envvar, &logical);
	if (logical != NULL && *logical != EOS)
	{
		STlcopy(logical, lbuf, sizeof(lbuf)-1);
		if (LOfroms(PATH&FILENAME, lbuf, &tloc) == OK)
		{
			LOcopy(&tloc, buf, loc);
			return;
		}
	}
	
	if (NMloc(FILES, FILENAME, filename, &tloc) == OK )
	{ /* get path for filename */
		LOcopy(&tloc, buf, loc);
	}
	return;
}

/*{
** Name:	iiabImgLink() - Link an Exexcutable Image for an Application.
**
** Description:
**	Given an application and the name of an image file, link the
**	application into the file.  Compiles are done if needed.
**
** Inputs:
**	app		{APPL *}  The application object.
**	executable	{char *}  The name of the executable to build.
**				MUST BE SMALLER THAN MAX_LOC CHARACTERS!
**	options		{nat}  Compilation options; force, object-code, etc.
**
** History:
**	1-sep-1986 (Joe)
**		Extracted from ablnkapp.
**	9-jun-1987 (Joe)
**		Added the common part of compiling and building
**		table for call by abfimage.  Also changed arguments.
**	14-aug-91 (blaise)
**		Added new argument, options, to _getImageObjs() call.
*/
static LOCATION	lnk_options ZERO_FILL;
static char	lnk_buf[MAX_LOC+1] = {EOS};

STATUS
iiabImgLink ( app, executable, options )
APPL		*app;
char		*executable;
i4		options;
{
	LOCATION	**objs;
	STATUS		rstatus;
	bool		user_link;

	if ( _lnkstart(app, options) != OK ||
			_getImageObjs( app, options, &objs ) != OK )
	{
		return FAIL;
	}

	if ( lnk_buf[0] == EOS )
#ifdef	PMFEWIN3									 
		set_file("abflnkw.opt", "II_ABF_LNK_OPT", lnk_buf, &lnk_options);	 
#else											 
		set_file("abflnk.opt", "II_ABF_LNK_OPT", lnk_buf, &lnk_options);
#endif											 
	rstatus =
	 _link( app, (char *)NULL, objs, &lnk_options, executable, &user_link ); 

	MEfree((PTR)objs);

	return rstatus;
}

/*
** Name:	_lnkstart() -	Start Image Link for Application.
**
** Description:
**
** History:
**	14-aug-91 (blaise)
**		Don't compile source files if the nocompile option was set.
*/
static STATUS
_lnkstart ( app, options )
APPL	*app;
i4	options;
{
	VOID	iiabGnImgExtract();

	/* Don't compile if the nocompile option was set */
	if (!(options & ABF_NOCOMPILE))
	{
		if ( !iiabAppCompile(app, options))
		{ /* an error occured */
			return FAIL;
		}
	}
	if ( _buildExtract(app, &ExtractFile, iiabGnImgExtract) != OK )
	return FAIL;

	IIABdcmDispCompMsg(S_AB0041_BuildingImage, TRUE, 1, app->name);

	return OK;
}

/*
** Name:	_getImageObjs() -	Get the Object-Code File Location List
**						for an Application.
**
** Description:
**	Get the object-code file location lists for a linking an application
**	image.  This list consist of locations for all the object-code files
**	that were generated for the application.
**
**	The library lists (there are now two) are the list of user
**	objects to link with the application.  These are character strings.
**	The library list must be a character string because they are
**	user and system specific objects that might be libraries or
**	object modules or even shared libraries.  Since specifying this
**	different kinds of objects will vary from system to system (on
**	VMS for example libraries are file/library ) we depend on the
**	user and the installation to have the right format for these.
**	There are two library lists, one for dynamic linking, the other
**	for regular links.  This is necessary for VMS because of the use
**	of shared libraries.
**	The library lists are filled first from a user file pointed to
**	by ING_ABFOPT1, and second by an INGRES file abfdyn.opt or abflnk.opt
**	depending on whether the dynamic list or the regular list is getting
**	filled.	 Two INTERNAL names II_ABF_DYN_OPT and II_ABF_LNK_OPT can
**	be set to point to alternates for the files.
**
** Inputs:
**	app	{APPL *}  The application object.
**	options	{nat}  Compilation options; force, object-code, etc.
**
** Outputs:
**	objs	{LOCATION **}  A reference to an address of an array of
**				locations, which will be the object list.
**				The first is abfmain.
** History:
**	(joe)
**		Written
**	12/18/85 (joe)
**		Modified for shared libraries.
**	21-sep-88 (kenl)
**		Changed MEcalloc() calls to MEreqmem() calls.
**	21-sep-90 (Mike S)
**		Check flag bit to see if we're using a compiled form.
**	14-aug-91 (blaise)
**		Added third argument, options, to check whether the nocompile
**		option is set. If so, check that object files already exist.
*/
static STATUS
_getImageObjs ( app, options, objs )
APPL		*app;
i4		options;
LOCATION	***objs;
{
	register LOCATION	**lp;
	register APPL_COMP	*comp;
	register LOCATION	*locp;
	register char		*bufp;

	LOCATION	*locs;
	LOINFORMATION	locinfo;
	char		*bufs;
	i4		locflags;
	char		*objpath;
	char		junk[MAX_LOC+1];

	register i4	cnt = 0;

	for ( comp = (APPL_COMP *)app->comps ; comp != NULL ;
			comp = comp->_next )
	{
		switch ( comp->class )
		{
		  case OC_HLPROC:
			if ( *((HLPROC *)comp)->source == EOS )
				break;	/* ... not library procedures */
			if ( !_chkHLsrc((APPL_COMP*)app->comps, (HLPROC*)comp) )
				break;	/* ... or duplicate source */
			/*fall through ...*/
		  case OC_OLDOSLFRAME:
		  case OC_OSLFRAME:
		  case OC_MUFRAME:
		  case OC_APPFRAME:
		  case OC_UPDFRAME:
		  case OC_BRWFRAME:
		  case OC_OSLPROC:
		  case OC_AFORMREF:
			cnt += 1;
		}
	}
		
	/*
	** allocate 1 more than the maximum number we could have.
	** This is a ZERO'ed alloc, which causes the list to
	** be NULL terminated.
	*/
	if ( _allocObjList(cnt + 3, objs, &locs, &bufs) != OK )
	{
		return FAIL;
	}

	lp = *objs;
	*lp++ = &Abrtsmain;
	*lp++ = &(ExtractFile.fiofile);

	locp = locs;
	bufp = bufs;
	for ( comp = (APPL_COMP *)app->comps ; comp != NULL ;
			comp = comp->_next )
	{
		switch ( comp->class )
		{
		  case OC_HLPROC:
			if ( *((HLPROC *)comp)->source == EOS )
				continue;	/* ... not library procedures */
			if ( !_chkHLsrc((APPL_COMP*)app->comps, (HLPROC*)comp) )
				continue;	/* ... or duplicate source */
			LOcopy( iiabMkObjLoc(((HLPROC *)comp)->source),
					bufp, locp
			);
			break;
		  case OC_OLDOSLFRAME:
		  case OC_OSLFRAME:
		  case OC_MUFRAME:
		  case OC_APPFRAME:
		  case OC_UPDFRAME:
		  case OC_BRWFRAME:
			LOcopy( iiabMkObjLoc(((USER_FRAME *)comp)->source),
					bufp, locp
			);
			break;
		  case OC_OSLPROC:
			LOcopy( iiabMkObjLoc(((_4GLPROC *)comp)->source),
					bufp, locp
			);
			break;
		  case OC_AFORMREF:
			/* Form References have optional compiled source */
			if ((comp->flags & APC_DBFORM) != 0)
				continue;	/* Database form */
			if ( *((FORM_REF *)comp)->source == EOS )
				continue;	/* ... no source, no object */
			LOcopy( iiabMkObjLoc(((FORM_REF *)comp)->source),
					bufp, locp
			);
			break;
		  default:
			/* nothing else should generate a location */
			continue;
		} /* end switch */

		/*
		** If the application is being imaged with the nocompile
		** option, check that the object file exists.
		*/
		if (options & ABF_NOCOMPILE)
		{
			locflags = LO_I_TYPE;
			if (LOinfo(locp, &locflags, &locinfo) != OK)
			{
				LOtos(locp, &objpath);
				IIUGerr(E_AB0154_NoObjFile, UG_ERR_ERROR, 1,
								objpath);
				return FAIL;
			}
		}

		*lp++ = locp++;
		bufp += MAX_LOC + 1;
	}

	*lp = NULL;	/* safety */
	return OK;
}

/*
** Name:	_chkHLsrc() -	Check for Unique Host-Language Source.
**
** Description:
**	Check that the source-code file for the current host-language procedure
**	component is unique with respect to any already examined host-language
**	procedure components.
**
** Input:
**	comps	{APPL_COMP *}  The application components list.
**	hlproc	{HLPROC *}  The current host-language component in the list.
**
** Returns:
**	{bool}  TRUE if unique, FALSE otherwise.
**
** History:
**	05/89 (jhw) -- Written.
*/
static bool
_chkHLsrc ( comps, hlproc )
register APPL_COMP	*comps;
register HLPROC		*hlproc;
{
	while ( comps != (APPL_COMP *)hlproc )
	{
		if ( comps->class == OC_HLPROC &&
			    STequal(((HLPROC *)comps)->source, hlproc->source) )
			return FALSE;
		comps = comps->_next;
	}
	return TRUE;
}

/*
** link an interpreter executable for the application.
*/
static LOCATION	dyn_options ZERO_FILL;
static char	dyn_buf[MAX_LOC+1] = {EOS};

STATUS
iiabInterpLink ( app, tstimg, exeloc )
APPL		*app;
ABLNKTST	*tstimg;
LOCATION	*exeloc;
{
	register LOCATION	**op;
	register APPL_COMP	*comp;
	register LOCATION	*locp;
	register char		*bufp;

	LOCATION	**objs;
	LOCATION	*locs;
	char		*bufs;
	char		*interp;
	STATUS		rstatus;

	VOID	IIABieIntExt();

	/*
	** Build the abextract.c file, which we will write our main
	** and HL procedure table into.
	*/
	if ( (rstatus = _buildExtract(app, &ExtractFile, IIABieIntExt)) != OK )
	{
		return rstatus;
	}

	IIABdcmDispCompMsg(S_AB026E_linking, TRUE, 0);

	/*
	** We need main obj + hl_proc + the abextract obj + NULL terminator
	*/
	if ( _allocObjList(tstimg->hl_count + 3, &objs, &locs, &bufs) != OK )
		return FAIL;

	op = objs;
	locp = locs;
	bufp = bufs;
	
	/*
	** first object is the main obj, then the abextract file, which
	** contains the HL procedure list.
	*/
	*op++ = &Abrtsimain;
	*op++ = &(ExtractFile.fiofile);

	/*
	** now the HL procudures
	*/
	for ( comp = (APPL_COMP *)app->comps ; comp != NULL ;
			comp = comp->_next )
	{
		if ( comp->class == OC_HLPROC &&
				*((HLPROC *)comp)->source != EOS &&
			    _chkHLsrc((APPL_COMP *)app->comps, (HLPROC* )comp) )
		{
			LOcopy( iiabMkObjLoc(((HLPROC *)comp)->source),
					bufp, locp
			);
			*op++ = locp++;
			bufp += MAX_LOC + 1;
		}
	}

	if ( dyn_buf[0] == EOS )
#ifdef	PMFEWIN3									 
		set_file("abfdynw.opt", "II_ABF_DYN_OPT", dyn_buf, &dyn_options);	 
#else											 
		set_file("abfdyn.opt", "II_ABF_DYN_OPT", dyn_buf, &dyn_options);
#endif											 

	LOtos(exeloc, &interp);
	rstatus = _link( app, (char *)NULL, objs, &dyn_options, interp,
				&tstimg->user_link
	); 

	MEfree((PTR) objs);

	return rstatus;
}

/*
** Name:	_buildExtract() -	Build and Compile a Link Extract Table.
**
** Description:
**	Given the application, a file object, and a build function,
**
** Inputs:
**	app	{APPL *}  The application object.
**	file	{ABFILE *}  The link extract file.
**	build	{VOID (*)()}  The build function.
**
** History:
**	10/89 (jhw) -- Modified to delete extract file before rebuilding.
**			JupBug #5375.  (Purging has been moved to "utcom.def".)
**	02/90 (mikes)  Corrected 'PEworld()' argument from "+rw".  JupBug #9950.
**	05/90 (jhw)  Purge extract file and move object-code file protection
**		change to "utcom.def".  JupBug #9999.
**	17-Dec-92 (fredb)
**		Add ifdef'd code for MPE/iX (hp9_mpe) to use SIfopen instead
**		of SIopen because MPE's compilers require ascii files.
*/
static STATUS
_buildExtract ( app, file, build )
APPL	*app;
ABFILE	*file;
VOID	(*build)();
{
	STATUS	rval;
	bool	ignore;
	LOCATION	tmpfile;
	LOCATION	outfile;
	char		outbuf[MAX_LOC+1];
# ifndef IMTINTERP
	LOCATION	*src;
	FILE		*fp;

	src = abfilsrc(file);
#ifndef CMS
# ifdef hp9_mpe
	if ( SIfopen(src, "w", SI_TXT, 128, &fp) != OK )
# else
	if ( SIopen(src, "w", &fp) != OK )
# endif
#else
#ifdef VMS
	if ( LOexist(src) == OK )
		_VOID_ LOdelete(src);
#endif
	if ( SIfopen(src, "w", SI_TXT, -80, &fp) != OK )
#endif
	{
		IIUGerr(NOEXTRACT, UG_ERR_ERROR, 2, file->fisname, file->fisdir);
		return FAIL;
	}

	(*build)(app, fp);
	SIclose(fp);
	LOpurge(src, 1);
	PEworld("+r+w", src); /* so others can recreate it ... */

	/* Make the output file */
	_VOID_ NMloc(TEMP, PATH, (char *)NULL, &tmpfile);
	_VOID_ LOuniq(ERx("cex"), ERx("out"), &tmpfile);
	_VOID_ LOcopy(&tmpfile, outbuf, &outfile);

	if ( IIUTcompile((char *)NULL, abfilsrc(file),
		S_AB0017_CompExtFile, (PTR)NULL, &outfile, &ignore)
			!= OK )

	{ /* couldn't compile it */
		IIABdcmDispCompMsg(S_AB0013_CompFailed, FALSE, 0);
		IIUGerr( COMEXTRACT, UG_ERR_ERROR,
			2, file->fisname, file->fisdir
		);
		LOdelete(&(file->fiofile));
		LOdelete(&outfile);
		return FAIL;
	}
	IIABdcmDispCompMsg(S_AB0014_CompWorked, FALSE, 0);
	LOdelete(&outfile);
# else	/* IMTINTERP */
	char		*cp;
	u_i4		tag;
	ABRTSOBJ	*abobj;

	STATUS	abrtadef();

	tag = FEgettag();
	if ((rval = abrtadef(app, tag, &abobj)) != OK)
	{
		char	err[20];

		CVna(rval, err);
		IIUGerr(RTTMEM, UG_ERR_ERROR, 1, err);
		return rval;
	}
	abobj->abrodbname = *IIar_Dbname;
	LOtos(abfilsrc(file), &cp);
	if ((rval = IIOItcTstfilCreate(cp, abobj)) != OK)
	{
		char	err[20];

		CVna(rval, err);
		IIUGerr(RTTFILE, UG_ERR_ERROR, 1, err);
		return rval;
	}
	FEfree(tag);
# endif /* IMTINTERP */
	return OK;
}

/*
** Name:	_allocObjList() -	Allocate Object-Code Location List.
**
** Description:
**	Allocates memory for the object-code location list, the object-code
**	locations, and the location buffers.
**
** Inputs:
**	cnt	{nat}  The number of locations in the list.  This includes
**			the NULL terminator.
**
** Outputs:
**	objs	{LOCATION ***}  The reference to the reference to the
**					object-code location list.
**	locs	{LOCATION **}  The reference to the locations.
**	bufs	{char **}  The reference to the location buffers.
**
** Returns:
**	{STATUS}
**
** History:
**	02/89 (jhw)  Written.
*/
static STATUS
_allocObjList ( cnt, objs, locs, bufs )
i4		cnt;
LOCATION	***objs;
LOCATION	**locs;
char		**bufs;
{
	register char	*bp;
	i4		size;

	size = cnt * sizeof(LOCATION *) +
			(cnt - 1) * (sizeof(LOCATION) + MAX_LOC + 1);

	if ( (bp = MEreqmem(0, size, TRUE, (STATUS *)NULL)) == NULL )
		return FAIL;

	*objs = (LOCATION **)bp;
	*locs = (LOCATION *)(bp += cnt * sizeof(LOCATION *));
	*bufs = bp + (cnt - 1) * sizeof(LOCATION);

	return OK;
}

/*
** Name:	_link() -	Link Image for Application.
**
** Description:
**	Link image for application given libraries and link options.  Primarily,
**	this adds in any user link options and then links.
**
** History:
**	08/89 (jhw) -- Added 'user_link' to mark when user options are added.
**	22-feb-90 (mgw)  NMgtAt() is a void, don't check it's return value.
*/
static char	*_abfopt1 = NULL;

static STATUS
_link ( app, symbol, libs, link_options, image, user_link )
APPL		*app;
char		*symbol;
LOCATION	*libs[];	/* really objects right now */
LOCATION	*link_options;
char		*image;
bool		*user_link;
{
	register i4	next;
	STATUS		stat;
	char		*options;
	LOCATION	*opts[3];
	LOCATION	user_options;
	char		user_optbuf[MAX_LOC+1];
	char		path[MAX_LOC+1];

	next = 0;

	/* Get user link options. */
	*user_link = FALSE;
	if ( (options = app->link_file) == NULL || *options == EOS )
	{ /* check for ING_ABFOPT1, backwards compatibility */
		if ( _abfopt1 == NULL )
		{
			char	*logical;

			NMgtAt("ING_ABFOPT1", &logical);
			_abfopt1 = (logical != NULL && *logical != EOS)
				? STalloc(logical) : "";
		}
		if ( *_abfopt1 != EOS)
		{
			STlcopy(_abfopt1, user_optbuf, sizeof(user_optbuf)-1);
			if (LOfroms(PATH&FILENAME, user_optbuf, &user_options)
					== OK )
			{
				opts[next++] = &user_options;
				*user_link = TRUE;
			}
		}
	}
	else
	{ /* application specific user link options */
		STcopy(options, user_optbuf);
		if ( LOfroms(PATH&FILENAME, user_optbuf, &user_options) != OK )
		{ /* look in application source directory */
			STcopy(app->source, path);
			if ((stat = LOfroms(PATH, path, &user_options)) != OK ||
				(stat = LOfstfile(options,&user_options)) != OK)
			{
				return stat;
			}
		}
		opts[next++] = &user_options;
		*user_link = TRUE;
	}
	opts[next++] = link_options;
	opts[next] = NULL;

	if ( (stat = IIUTlink(symbol, libs, opts, image)) != OK )
	{
		i4 errstat = stat;

		IIUGerr(E_AB0020_LinkFailure, 0, 1, &errstat);
		*user_link = FALSE;
	}

	return stat;
}

/*{
** Name:	IIUTlink() -	Link an Executable Image.
**
** Description:
**
** History:
**	07/89 (jhw) -- Check for existence after successful link (in case
**		the link does not return a bad status.)  JupBug #3690.
**	01/90 (Mike S) -- Append the system's extension for executables
**			  before using the file name.
**	3/90 (Mike S)  -- Improve the preceding quick fix.  Append the extension
**			  only if the executable name has no extension. This
**			  will work for UNIX and VMS; porting groups may
**			  need to ifdef
**	3/90 (Mike S)  -- Redirect link output to a file; then display it.
**	4/90 (Mike S)  -- See if the date on the executable changes; if not, 
**			  assume a new one wasn't created.
*/
static char	**_optionList();
static VOID	_readOptFile();

STATUS
IIUTlink ( symbol, objs, lnkopt, image )
char		*symbol;
LOCATION	*objs[];
LOCATION	*lnkopt[];
char		*image;
{
	STATUS		rstatus;
	char		**liblist;
	LOCATION	exeloc;
	char		exeloc_buf[MAX_LOC+1];
	bool		pristine;
	CL_ERR_DESC	clerror;
	LOCATION	tmpfile;
	LOCATION	outfile;
	char		outbuf[MAX_LOC+1];
	LOINFORMATION	old;
	LOINFORMATION	new;
	bool		was_old;
	i4		loflags;

	/* Get the options list (constructed if necessary.) */
	liblist = _optionList(lnkopt);

	/* Construct the executable's name */
	STcopy(image, exeloc_buf);
	if ((rstatus = LOfroms(PATH&FILENAME, exeloc_buf, &exeloc)) != OK)
		return (rstatus);

	/* If the name has no extension, apply the default, if there is one */
	if ((ABIMGEXT != NULL) && (*ABIMGEXT != EOS))
	{
		char	dummy[MAX_LOC+1];
		char	extension[MAX_LOC+1];

		if ( LOdetail(&exeloc, dummy, dummy, dummy, extension, dummy)
				== OK && *extension == EOS )
		{
			STpolycat(2, image, ABIMGEXT, exeloc_buf);
			if ((rstatus = LOfroms(PATH&FILENAME, exeloc_buf, 
						&exeloc)) != OK)
				return (rstatus);
		}
	}

	/* Make the output file */
	_VOID_ NMloc(TEMP, PATH, (char *)NULL, &tmpfile);
	_VOID_ LOuniq(ERx("lnk"), ERx("out"), &tmpfile);
	_VOID_ LOcopy(&tmpfile, outbuf, &outfile);

	/* See if the executable already existed; if so, get its date */
	loflags = LO_I_LAST;
	was_old = (OK == LOinfo(&exeloc, &loflags, &old)) && 
		  ((loflags & LO_I_LAST) != 0);

	/* OK, link the executable. */
	if ( (rstatus = UTlink(objs, liblist, &exeloc, &outfile, 
				&pristine, &clerror)) 
		!= OK )
	{
		if ( LOexist(&exeloc) == OK )
			LOdelete(&exeloc);
	}
	else
	{
		/* If an executable exists now, check its date */
		loflags = LO_I_LAST;
		rstatus = LOinfo(&exeloc, &loflags, &new);
		if ( rstatus == OK && was_old && (loflags & LO_I_LAST) != 0
				&& TMcmp(&old.li_last, &new.li_last) >= 0 )
		{
			/* We didn't make a newer executable */
			rstatus = FAIL;
		}
	}

	/* Restore the screen (if need be) and display the result */
	if (! pristine)
		IIABrsRestoreScreen();
	IIABdcfDispCompFile(&outfile);
	LOdelete(&outfile);

	return rstatus;
}

/*
** Name:	_optionList() -	Returns the Link Options (Library) List.
**
** Description:
**	Returns a reference to a link library (option) list.  This list is
**	generated from the lists contained in the passed in list of option
**	file locations.  The options for each file are fetched and stored
**	internally, and then combined before being output.
**
**	Note:  This assumes at most two options files will be specified:
**	an optional user options file, and an internal ABF options file.
**	The internal options file will be either for the dynamic case or
**	for the non-dynamic case.
**
** Inputs:
**	lnkopts	{LOCATION *[]}  An list of references to locations of the
**					options files.
** Returns:
**	{char **}  The list of options from the options files.
**
** History:
**	12/16/85 (joe) Changed for shared libraries.
**	04/89 (jhw)  Rewritten to generate options from a list of files.
*/

static i4	_getOptions();

typedef struct {
	i4	nopts;		/* number of options */
	i4	opts_size;	/* number of option slots */
	char	**opts;		/* option slots */
	char	pathname[MAX_LOC+1];
	SYSTIME	touched;
} OPTIONS;

#define _START_CNT	20

static char	*tusropts[2*_START_CNT] = {NULL};
static char	*topts1[_START_CNT] = {NULL};
static char	*topts2[_START_CNT] = {NULL};

static OPTIONS	usrOptions = {0, 2*_START_CNT, tusropts, {EOS}};
static OPTIONS	lnkOptions[2] = {
			{0, _START_CNT, topts1, {EOS}},
			{0, _START_CNT, topts2, {EOS}}
};

static char **
_optionList ( lnkopts )
LOCATION	*lnkopts[3];
{
	i4		extra;
	LOCATION	*abf_opt;
	char		*filename;
	register OPTIONS *opts;

	/*
	** Libraries (link options) are in external lists.  We need to read in
	** the lists and then allocate them.  List are re-read when they change.
	** Since we don't know how many options there are, we read in the list
	** first allocating space for the strings we get.  Then we re-read the
	** lists.
	*/

	abf_opt = lnkopts[( lnkopts[1] != NULL ) ? 1 : 0];

	LOtos(abf_opt, &filename);
	if ( STequal(lnkOptions[0].pathname, filename) )
	{
		extra = _getOptions(abf_opt, 0, opts = &lnkOptions[0]);
	}
	else if ( STequal(lnkOptions[1].pathname, filename) )
	{
		extra = _getOptions(abf_opt, 0, opts = &lnkOptions[1]);
	}
	else
	{
		extra = _getOptions( abf_opt, 0, opts =
			&lnkOptions[lnkOptions[0].pathname[0] == EOS ? 0 : 1]
		);
	}

	if ( lnkopts[1] != NULL )
	{ /* user options file */
		register char	**p;
		register char	**s;
		i4		cnt;

		cnt = _getOptions( lnkopts[0], extra != 0 ? extra
				: max(lnkOptions[0].nopts, lnkOptions[1].nopts),
				&usrOptions
		);

		/* (re-)add the ABF options */
		s = &usrOptions.opts[cnt];
		for ( p = opts->opts ; *p != NULL ; )
			*s++ = *p++;
		*s = NULL;

		opts = &usrOptions;
	}

	return opts->opts;
}

/*
** Name:	_getOptions() -	Generate a Link Option (Library) List.
**
** Description:
**	Sets the passed in pointer to an array of strings containing
**	the entries for the library list. The caller passes in an array
**	of size tsize to hold the list.	 If the array is too small, this
**	routine allocates a big enough one.  The array is first filled in
**	with lines from the file pointed to by ING_ABFOPT1 (it does not
**	have to exist).	 After that, it uses the file given by filename
**	or if logname is defined it uses that.
**
** Inputs:
**	file	{LOCATION *}  The location of the link options file that
**				contains the list.
**	extra	{nat}	The size of extra list entries to be allocated.
**	list	{OPTIONS *}  The options list.
**			.opts_size	{nat}  The size of the list.
**			.opts		{char **}  The list.
**			.pathname {char *}  The name of the file containing
**						the list.
**			.touched {SYSTIME}  Last modification of the file.
** Outputs:
**	list	{OPTIONS *}  The options list.
**			.nopts		{nat}  The number of options.
**			.opts_size	{nat}  The new size of the list.
**			.opts	{char **}  The new list.
**			.pathname {char *}  The name of 'file' if it was a
**						different (new) file.
**			.touched {SYSTIME}  Last modification of the file.
**
** History:
**	12/16/85 (joe) Written for shared libraries.
**	04/89 (jhw)  Rewritten to regenerate options when a file changes.
**      12-oct-93 (daver)
**              Casted parameters to MEcopy() to avoid compiler warnings
**              when this CL routine became prototyped.
*/
static i4
_getOptions ( file, extra, list )
LOCATION	*file;
i4		extra;
register OPTIONS *list;
{
	char	*filename;
	SYSTIME	last;

	LOtos(file, &filename);
	if ( !STequal(list->pathname, filename) ||
		LOlast(file, &last) != OK ||
			TMcmp(&last, &list->touched) > 0 )
	{ /* fill list */
		i4	tlcount;

		do
		{
			tlcount = 0;
			_readOptFile( file, list->opts_size,
					list->opts, &tlcount
			);
			/*
			** BUG 2321
			** If static array is too small, allocate a new one
			*/
			if ( tlcount + extra > list->opts_size - 1 )
			{ /* allocate more */
				tlcount += 1;
				if ( list->opts_size > _START_CNT )
					MEfree(list->opts);
 				if ( (list->opts = (char**)MEreqmem((u_i4)0,
 					(u_i4)(tlcount+extra)*sizeof(char*),
					TRUE,
 					(STATUS *)NULL)) == NULL )
 				{
 					IIUGbmaBadMemoryAllocation(
						"ablink:  _getOptions()"
					);
 				}
				list->opts_size = tlcount + extra;
			}
		} while ( list->opts_size - 1 < tlcount + extra );
		list->opts[tlcount] = NULL;

		if ( LOlast(file, &last) == OK )
			MEcopy((PTR)&last, (u_i2)sizeof(list->touched), 
				(PTR)&list->touched);

		STcopy(filename, list->pathname);
		list->nopts = tlcount;
	}
	return list->nopts;
}

static VOID
_readOptFile ( file, size, lp, cursize )
LOCATION	*file;
register i4	size;
register char	**lp;
i4		*cursize;
{
	register i4	nolines = 0;
	FILE		*fp;
	char		buf[MAX_LOC+1];

	if ( SIopen(file, "r", &fp) != OK )
		return;

	while ( SIgetrec(buf, sizeof(buf)-1, fp) == OK )
	{
		if ( nolines++ < size - 1 )
		{
			register char	*cp;

			if ( (cp = STindex(buf, "\n", 0)) != NULL )
				*cp = EOS;
			else
			/*
			** Allow old style VMS options files that
			** may have ,- at the end of the line.
			*/
				cp = &buf[STlength(buf)];

			if (*(cp-1) == '-' && *(cp-2) == ',')
				*(cp-2) = EOS;
			*(lp++) = STalloc(buf);
		}
	}

	SIclose(fp);

	*cursize = nolines;
}
