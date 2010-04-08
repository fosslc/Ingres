/*
**  Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<cv.h>		/* 6-x_PC_80x86 */
#include	<st.h>
#include	<lo.h>
#include	<nm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<uf.h>
#include	<eqlang.h>
#include	<ft.h>
#include	<fmt.h>
#include	<adf.h>
#include	<frame.h>
#include	<frserrno.h>
#include	<erfi.h>
# include	<lqgdata.h>

/*
** Name:    uffindex.c -	Front-End Utility Form Index File Access Module.
**
** Description:
**	Contains routines that support locating and loading forms from the
**	form index files.  Defines:
**
**	IIUFlcfLocateForm()	returns the location of the form index file.
**	IIUFgtfGetForm()	initializes form from a form index file.
**
** History:
**	87/07  peter
**	Added "II_FORMFILE" logical and ER support.
**
**	86/12  kobayashi
**	Initial revision.
**
**	10/28/88 (dkh) - Performance changes.
**	16-mar-89 (bruceb)
**		Added call on IIFRaff() after sucessful call on IIaddrun.
**	07/22/89 (dkh) - Integrated changes from IBM.
**	11/02/90 (dkh) - Replaced IILIBQgdata with IILQgdata().
**	4/23/96 (chech02) - included uf.h for windows 3.1 port.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-sep-2002 (somsa01)
**	    In IIUFlcfLocateForm(), if ADD_ON64 is set, grab the form file
**	    from the lp64 directory.
**	07-Mar-2005 (hanje04)
**	    SIR 114034
**	    Add support for reverse hybrid builds. First part of much wider
** 	    change to allow support of 32bit clients on pure 64bit platforms.
**	    Changes made here allow application statically linked against 
**	    the standard 32bit Intel Linux release to run against the new
**   	    reverse hybrid ports without relinking.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
*/

#define	STDFILE		ERx("rtiforms.fnx")

FUNC_EXTERN	VOID	IIFRaffAddFrmFrres2();
FUNC_EXTERN	i4	IIlang();
FUNC_EXTERN	i4	IIaddrun();


/*{
** Name:    IIUFlcfLocateForm() - returns the location of the form index file.
**
** Description:
**	Get the location of the standard front end forms file.  This is
**	defined as the file pointed to by II_FORMFILE, if defined.  If
**	that name is not defined, then the file STDFILE in the directory
**	pointed to by ING_FILES, in the subdirectory pointed to by
**	II_LANGUAGE, will be used.
**
**	And this returns the pointer of this location.
**	This procedure will be used in the parameter 
**	of IIUFgtfGetForm procedure to get the location of form index file.
**
** Returns:
**	{LOCATION *}  A reference to the location.
**		if anything failed, display error message and return NULL 
**		pointer.
**
** History:
**	20-Dec-1986 (Kobayashi)
**		Create new for 5.0K.
**	31-jul-1987 (peter)
**		Update for 6.0.  Add the II_FORMFILE name.
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	06-sep-2002 (somsa01)
**	    If ADD_ON64 is set, grab the form file from the lp64 directory.
**	17-Jun-2004 (schka24)
**	    Safe env var handling.
**      07-Mar-2005 (hanje04)
**          SIR 114034
**          Add support for reverse hybrid builds. First part of much wider
**          change to allow support of 32bit clients on pure 64bit platforms.
**          Changes made here allow application statically linked against
**          the standard 32bit Intel Linux release to run against the new
**          reverse hybrid ports without relinking.
*/

static 	  LOCATION	loc;
static    char    loc_buf[MAX_LOC + 1];
GLOBALREF i4	  IIUFlfiLocateFormInit ;

LOCATION *
IIUFlcfLocateForm()
{
    char    *dp;

    if (IIUFlfiLocateFormInit != 0)
    {
	return(&loc);
    }
    NMgtAt(ERx("II_FORMFILE"), &dp);
    if (dp != NULL && *dp != EOS)
    {
	STlcopy(dp, loc_buf, sizeof(loc_buf)-1);
	LOfroms(PATH & FILENAME, loc_buf, &loc);
    }
    else
    {
	LOCATION	tloc;
	i4		langcode;
	char    	langstr[ER_MAX_LANGSTR+1];

    	if (ERlangcode((char *)NULL, &langcode) != OK)
        {
	    char	*lang;

	    NMgtAt(ERx("II_LANGUAGE"), &lang);
	    IIUGerr(E_UG0001_Bad_Language, 0, 1, (PTR)lang);
	    return NULL;
        }

	NMloc(FILES, PATH, (char *)NULL, &tloc);
	LOcopy(&tloc, loc_buf, &loc);

        _VOID_ ERlangstr(langcode, langstr);
        LOfaddpath(&loc, langstr, &loc);
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH64)
	LOfaddpath(&loc, ERx("lp64"), &loc);
#endif  /* hybrid */
#if defined(conf_BUILD_ARCH_64_32) && defined(BUILD_ARCH32)
    {
        /*
        ** Reverse hybrid support must be available in ALL
        ** 32bit binaries
        */
        char    *rhbsup;
        NMgtAt("II_LP32_ENABLED", &rhbsup);
        if ( (rhbsup && *rhbsup) &&
       ( !(STbcompare(rhbsup, 0, "ON", 0, TRUE)) ||
         !(STbcompare(rhbsup, 0, "TRUE", 0, TRUE))))
            LOfaddpath(&loc, "lp32", &loc);
    }
#endif /* reverse hybrid */
   
        LOfstfile(STDFILE, &loc);
    }

    if (LOexist(&loc) != OK)
    {
	LOtos(&loc, &dp);
	IIUGerr(E_UG000A_No_Form_File, 0, 1, (PTR)dp);
	return NULL;
    }

    IIUFlfiLocateFormInit = 1;
    return &loc;
}

/*{
** Name:    IIUFgtfGetForm() -	Initializes form from a form index file.
**
** Description:
**	This procedure searchs the index, loads cfdata 
**	with form from the form index file, and initializes form. If the form 
**	index file has not be opened yet, the form index file will be opened.
**	The form index file is created by formindex command.
**
**	And it initializes the form using the cfdata related form name.
**
**	This procedure is almost same function of iiforminit procedure.
**	But iiforminit procedure reads the form data from database, FEgetform
**	procedure reads from extracted form index file.
**
** Inputs:
**	fileloc			The location of the form index file to use.
**	formname		The name of initialized form.
**
** Returns:
**	{STATUS} OK	if everthing worked.
**		 FAIL	if anything failed.
**
**	Exception:
**		none.
**
** Side Effects:
**	none.
**
** History:
**	20-Dec-1986 (Kobayashi)
**		Create new for 5.0K.
**	31-jul-1987 (peter)
**		Change name to IIUFgtfGetForm.
*/

FRAME	*IIFDfrcrfle();

STATUS
IIUFgtfGetForm ( fileloc, formnm )
LOCATION	*fileloc;
char		*formnm;
{
	FRAME		*frm;
	reg char	*name;
	i4		savelang;
	char		fbuf[FE_MAXNAME+1];
	i4		tag;

	/*
	**	Check all variables make sure they are valid.
	**	Also check to see that the forms system has been
	**	initialized.
	*/
	if ( fileloc == NULL || formnm == NULL || *formnm == EOS )
	{
		return FAIL;
	}

	if( !IILQgdata()->form_on )
	{
		IIUGerr( RTNOFRM, UG_ERR_ERROR, 0 );
		return FAIL;
	}

	/* set form name */
	if ( STlcopy( formnm, name = fbuf, FE_MAXNAME ) <= 0 ||
			STtrmwhite( name ) <= 0 )
		return FAIL;

	CVlower(name);

	/*
	**	Call 'IIFDfrcrfle()' to retrieve and intialize the frame from
	**	the Form Index file.  This routine does all the dirty work.
	**	Start a new (unique) tagged memory region for the allocated
	**	frame.
	*/

	tag = FEbegintag();
	savelang = IIlang(hostC);
	frm = IIFDfrcrfle(name, fileloc);
	IIlang(savelang);
	FEendtag();

	if (frm != NULL)
	{
	    frm->frtag = tag;
	    if (IIaddrun(frm, name))
	    {
		IIFRaffAddFrmFrres2(frm);
		return(OK);
	    }
	}

	IIUGtagFree(tag);
	return(FAIL);
}
