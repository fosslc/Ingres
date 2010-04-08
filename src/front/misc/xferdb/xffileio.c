/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<lo.h>
# include	<pe.h>		/* 6-x_PC_80x86 */
# include	<cm.h>
# include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<er.h>
# include	<fe.h>
# include	<me.h>
# include	<st.h>
# include	<si.h>
# include	<gn.h>
# include	<ug.h>
# include       <adf.h>
# include       <afe.h>
# include	<xf.h>
# include	"erxf.h"

/**
** Name:	xffileio.c - module contain file input/output routines for
**				copydb and unloaddb.
**
** Description:
**	These are all the file name-handling procedures for COPYDB and UNLOADDB.
**	These routines generate the filenames for COPY statements.
**	This file defines:
**
**	xfsetdirs	set directories for subsequent file open calls.
**	xfnewfname	return a unique filename string for a name and owner.
**	xfaddpath	add path + filename, return string representation.
**	xffileinput	sets the arguments from a parameter file.
**
** History:
**	15-jul-1987 (rdesmond) written.
**	25-jan-1989 (marian)
**		Add XF_CAT.  Add information to open/close/write to the
**		new cp*.cat file which is used to copy in the FE catalogs
**		as $ingres.
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**	29-aug-90 (billc)
**		incorporated fredb's porting changes:
    **      04-May-1990 (fredb)
    **              Added MPE/XL specific code to use SIfopen instead of
    **              SIopen for xf_open.  We need to extra control given by
    **              the additional parameters in SIfopen.
    **      15-May-1990 (fredb)
    **              Changed so everyone uses Sifopen, but MPE/XL uses the
    **              file type SI_BIN while everyone else uses SI_TXT.
    **      18-May-1990 (fredb)
    **              Added #define'd value for record length in SIfopen call.
    **              XF_LINELEN turned out to be too short ...
    **              Also use MPE/XL's special SI file type, SI_CMD, which will
    **              yield a fixed record length ASCII file.
    **      25-Jul-1990 (fredb)
    **              Now we find that 256 bytes fixed length overruns the SQL
    **              buffer when the generated query is executed.  We'll now
    **              try using the fixed length records only where required --
    **              for the unload & reload files (types 3 & 4).
**	08-aug-91 (billc)
**		Fix 39060, I think.  xfnewfname was returning a pointer to
**		a local buffer!  Just luck that it didn't blow up on Sun.
**		Also, some cleanup.
**	20-jan-92 (billc)
**		Fix 9005.  Wrong test for kanji chars in map_funny. 
**      08-30-93 (huffman)
**              add <me.h>
**	16-dec-1995 (canor01)
**		Add xlate_backslash() function for NT.
**	31-dec-1996 (chech02)
**		changed Xf_dir to GLOBALREF.
**      05-Jun-2000 (hanal04) Bug 101667 INGDBA 65
**              Prevent SIGSEGV is we can not determine pwd in xfsetdirs().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-nov-2001 (gupsh01)
**	    Created new function xffileinput to support -param_file=filename
**	    option of copydb. 
**/

/* # define's */

/* GLOBALDEF's */
GLOBALREF LOCATION	Xf_dir;

/* extern's */

/* static's */
static char	*Xf_from_path = NULL;
static char	*Xf_into_path = NULL;

static char	*saveloc(char	*path);
# ifdef NT_GENERIC
static void	xlate_backslash(char	*path);
# endif /* NT_GENERIC */


/*{
** Name:	xfsetdirs - set directory for subsequent file open calls.
**
** Description:
**	Given user specified directory strings, this sets the path in the
**	global locations.  All filenames which are generated for COPY cmds
**	will use these paths.
**
**	the 'dir', if given, is validated and relative pathnames 
**	expanded.
**
**	Frompath and intopath, if given, are not validated and used as
**	is.  These override dir.
**
** Inputs:
**	dir		user-specified default directory.
**	frompath	user-specified directory name for COPY FROM files.
**	intopath	user-specified directory name for COPY INTO files.
**
** Outputs:
**	none.
**
**	Returns:
**		OK	successful
**		FAIL	not successful
** History:
**	30-jul-87 (rdesmond) written.
**	24-jan-89 (marian)
**		Added information for writing to the new catalog file.  This
**		file will be used to reload the catalogs.
**      05-Jun-2000 (hanal04) Bug 101667 INGDBA 65
**              Add error detection to calls to LOgt(). Undetected failure
**              can lead to SIGSEGV. If we want the PWD and the OS call to
**              pwd fails (unix only) we can fake the PWD with a call to
**              LOfakepwd().
**      20-Nov-2000 (wansh01) 
**              Changed call to LOfakepwd for ifdef xCL_067_GETCWD_EXISTS only
**      25-Jan-2001 (wansh01) 
**              take out the previous change since xCL_067_GETCWD_EXISTS only   
**              defined in the cl routines. 
**	08-Sep-2004 (gilta01) Bug 87965 INGDBA 10
**		Checks for an empty string for intopath and frompath 
**	        before trying to change from the default (pwd) to the 
**		specified value.
*/

STATUS
xfsetdirs(char *dir, char *frompath, char *intopath)
{
    auto i2	dirflag;
    auto char 	*s;
    char	*locbuf;
    STATUS	status;

    locbuf = XF_REQMEM(MAX_LOC + 1, FALSE);

    if (dir == NULL || *dir == EOS)	
    {
	/* Default to current directory. */
	status = LOgt(locbuf, &Xf_dir);
        if(status != OK)
        {
#ifdef UNIX                 
            status = LOfakepwd(locbuf, &Xf_dir);
#endif
            if(status != OK)
            {
                IIUGerr(E_XF0061_Can_Not_Determine_Dir, UG_ERR_ERROR, 0);
                return(FAIL);
            }
        }
	dir = NULL;
    }
    else
    {
	STlcopy(dir, locbuf, MAX_LOC);
	if (LOfroms(PATH, locbuf, &Xf_dir) != OK)
	{
	    IIUGerr(E_XF0006_Invalid_directory_pat, UG_ERR_ERROR, 1, dir);
	    return (FAIL);
	}

	if (!LOisfull(&Xf_dir))
	{
	    auto LOCATION	xloc;
	    char		xbuf[MAX_LOC + 1];

	    /* 
	    ** path spec. is a relative specification -- we must expand it 
	    ** to a full pathname. 
	    */
	    LOcopy(&Xf_dir, xbuf, &xloc);
	    status = LOgt(locbuf, &Xf_dir);
            if(status != OK)
            {
                IIUGerr(E_XF0061_Can_Not_Determine_Dir, UG_ERR_ERROR, 0);
                return(FAIL);
            }
	    LOaddpath(&Xf_dir, &xloc, &Xf_dir);
	}

	/* LOisdir and LOexist are archaic and should be replaced by LOinfo. */
	LOisdir(&Xf_dir, &dirflag);
	if (dirflag != ISDIR || LOexist(&Xf_dir) != OK)
	{
	    IIUGerr(E_XF0006_Invalid_directory_pat, UG_ERR_ERROR, 1, dir);
	    return (FAIL);
	}
    }
    LOtos(&Xf_dir, &s);

    /* the COPY FROM path doesn't have to exist. */
    if (frompath != NULL && *frompath != EOS)
	Xf_from_path = saveloc(frompath);

    /* the COPY INTO path doesn't have to exist. */
    if (intopath != NULL && *intopath != EOS)
	Xf_into_path = saveloc(intopath);

    IIUGmsg(ERget(S_XF0064_Unload_Directory_is), FALSE, 1, 
		(PTR)(Xf_into_path == NULL ? s : Xf_into_path));
    IIUGmsg(ERget(S_XF0065_Reload_Directory_is), FALSE, 1, 
		(PTR)(Xf_from_path == NULL ? s : Xf_from_path));

    return (OK);
}

# define	MAX_PATH_LEN	MAX_LOC

static char *
saveloc(char	*path)
{
    char	*rval = NULL;
    char	ptmp[MAX_PATH_LEN + 2];

    if (STlength(path) > MAX_PATH_LEN)
    {
	i4 maxl = MAX_PATH_LEN;

	IIUGmsg(ERget(E_XF0130_Path_Too_Long), FALSE, 2, path, &maxl);

	/* Truncate and save */
	(void) STlcopy(path, ptmp, MAX_PATH_LEN);
	rval = STalloc(ptmp);
    }
    else
    {
	rval = STalloc(path);
    }

    /* Truncate and save */
    (void) STlcopy(path, ptmp, MAX_PATH_LEN);
    rval = STalloc(ptmp);

    return (rval);
}


/*{
** Name:	xfnewfname - generate a unique file name.
**
** Description: Generate a unique filename based on object name and
**		owner name.  Illegal characters are mapped to something
**		inoccuous, but there's a hole in the story (see comments
**		to "map_funny()").
**
** Inputs:
**	table_name	table name on which string should be based.
**	owner		name owner to embed in file name string.
**
** Outputs:
**	none
**
**	Returns:
**		ptr to file name string.  This is in a static struct
**		that is overwritten on subsequent calls, so use it or lose it.
**
** History:
**	23-jul-87 (rdesmond) written.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		move static decl of map_funny() outside calling function.
**	30-Mar-93 (billc) Incorporate fred's 6.4 change, but use a
**		define for the length parameter.
**      03-Mar-93 (fredb) hp9_mpe
**              Remove LOuniq call specific to MPE and add a call to
**              IIGNssStartSpace that is specific to MPE.
**	20-jul-95 (emmag)
**		Append a trailing \ to the path if one doesn't exist.
**	16-nov-2000 (gupsh01)
**		Remove call to IIGNgnGenName, we will add the whole 
**		filename as the file length is no longer restricted
**		to 8 chars.
**      05-Feb-2003 (inifa01) bug 109576 INGSRV 2091
**		Unloaddb does not generate unique copy filenames when multiple
**		users with similar names (eg pvusr1 & pvusr2) own tables with
**		the same name in a db.
**		Replaced call to IIGNgnGenName, removed above, to generate 
**		unique names.  Modified XF_UNIQUE in xf.qsh to 32 (max Ingres
**		object name length) as file length is no longer restricted to
**		8 chars.
**      19-Mar-2003 (hanch04)
**              Rework change for bug 109576.  The generated file name will now
**              be the tablename.username where username is no longer chopped
**              down to only three characters.  This will insure a unique 
**              filename.
**       1-Jun-2004 (hanal04) Bug 112379 INGDBA280
**              Reinstate the use of IIGNgnGenName() to generate a 
**              unique file name. map_funny() will replace non-printing
**              characters with an underscore. test#1 and test$1 will 
**              generate duplicate file names without the sequence
**              number provided by IIGNgnGenName().
*/

static	LOCATION	Tmppath;
static	char		Tmppath_buf[MAX_LOC + 1];
static  char		Namebuf[MAX_LOC + 1];
static  char		Prefixbuf[MAX_LOC + 1];

static	void map_funny(char *str);

static bool uncalled = TRUE;

char *
xfnewfname(char *table_name, char *owner)
{
    char	short_owner[4];
    char	*prefix;

    if (uncalled)
    {
        if (IIGNssStartSpace(1, ERx(""), XF_UNIQUE, XF_UNIQUE,
                GN_LOWER, GNA_XUNDER, (void (*)()) NULL) != OK)
        {
            return (NULL);
        }
        uncalled = FALSE;
    }
    Namebuf[0] = EOS;

    STcopy(table_name, Prefixbuf);

    /* map any funny bytes to underscores. */
    map_funny(Prefixbuf);

    if (IIGNgnGenName(1, Prefixbuf, &prefix) != OK)
        return (NULL);

    STcopy(prefix, Namebuf);
    STcat(Namebuf, ERx("."));
    
    STcopy(owner, Prefixbuf);

    /* map any funny bytes to underscores. */
    map_funny(Prefixbuf);

    STcat(Namebuf, Prefixbuf);
    return (Namebuf);
}

/*{
** Name:	xfaddpath - generate a unique file name.
**
** Description: add a filename to a path.  Returns pointer to
**		name of path + name of file, the pointer is to volatile
**		memory so user it or lose it.
**
** Inputs:
**	filename	file name
**	in_or_out	hack attack: indicate which path we want, the
**				COPY INTO or the COPY FROM path.
**
** Outputs:
**	none.
**
**	Returns:
**		char * - name of full path with file.
**
** History:
**	23-jul-87 (rdesmond) written.
**	17-aug-91 (leighb) DeskTop Porting Change:
**		move static decl of map_funny() outside calling function.
**	16-dec-1995 (canor01)
**		Windows NT: paths are created with backslash separators,
**		but tm will try to parse the backslash as a special-character
**		escape.  Since tm can understand and convert forward slashes 
**		generate paths with forward slashes using xlate_backslash().
**      14-dec-2004 (ashco01) - bug 87965, Problem: INGDBA 10
**              Corrected test for trailing forward-slash '/' in source and
**              target directory names to ensure that trailing slashes are
**              not appended to paths already containing them.
*/

static char	Whole_name[ (MAX_LOC * 2) + 1 ];

char *
xfaddpath(char *filename, i4  in_or_out)
{
    auto char	*wholename;

    if (filename == NULL)
    {
	wholename = NULL;
    }
    else if (in_or_out == XF_INTOFILE && Xf_into_path != NULL)
    {
	STcopy(Xf_into_path, Whole_name);

# ifdef UNIX
	if (Whole_name[STlength(Whole_name)-1] != '/')
	    STcat(Whole_name, ERx("/"));
# else 
# ifdef DESKTOP
	if (Whole_name[STlength(Whole_name)] != '\\')
	    STcat(Whole_name, ERx("\\"));
# endif /* DESKTOP */
# endif

	wholename = STcat(Whole_name, filename);
    }
    else if (in_or_out == XF_FROMFILE && Xf_from_path != NULL)
    {
	STcopy(Xf_from_path, Whole_name);

# ifdef UNIX
	if (Whole_name[STlength(Whole_name)-1] != '/') 
	    STcat(Whole_name, ERx("/"));
# else
# ifdef DESKTOP
	if (Whole_name[STlength(Whole_name)] != '\\')
	    STcat(Whole_name, ERx("\\"));
# endif /* DESKTOP */
# endif
	wholename = STcat(Whole_name, filename);
    }
    else
    {
	LOcopy(&Xf_dir, Tmppath_buf, &Tmppath);

	/* add the file name to location */
	LOfstfile(filename, &Tmppath);

	LOtos(&Tmppath, &wholename);
    }
# ifdef NT_GENERIC
    xlate_backslash(wholename);
# endif /* NT_GENERIC */
    return (wholename);
}
/*
** map_funny -- map 'funny' characters to underscores.  Inspired by
** 	the fact that table 'ii_foo' owned by $ingres mapped to a filename
**	of ii_foo.$in which confuses the UNIX command shell since $ indicates
**	a shell variable.  However, this mapping keeps the Kanji version
**	sane, as well.
*/

static void
map_funny(char *str)
{
    register char	*from = str;
    register char	*to = str;

    /*
    **	Fix 9005: using wrong test, wasn't mapping Kanji chars, so VMS 
    **		filenames were bad.
    **	WARNING: this means we won't support Kanji filenames!
    **		Also, this doesn't solve the problem of 8-bit chars in the
    **		European character sets.  We need a CMlegal_filename_char
    **		routine.
    */

    /* map any funny bytes to underscores. */
    while (*from != EOS)
    {
	i4	cnt = CMbytecnt(from);

	if ( (!CMalpha(from) && !CMdigit(from)) || CMdbl1st(from) )
	    *to++ = '_';
	else
	    *to++ = *from;
	from += cnt;
    }
    *to = EOS;
}

# ifdef NT_GENERIC
static void
xlate_backslash(char *filename)
{
    char *cp;

    while ( (cp = STindex( filename, "\\", 0 )) != NULL )
    {
	*cp = '/';
    }
}
# endif /* NT_GENERIC */
/*{
** Name: xffileinput      - get input files
**
** Description:
**      This routine will make input from the command line, transparent
**      from input from a file name.  This will be done by reading
**      the input file and building an argv,argc parameter from the
**      file specification.
**
**
** Inputs:
**	paramfile		Name of the parameter file 
**				which is passed in
**										to this routine.
**      argv                    ptr to command line parameters
**                              which may point to a file name
**                              containing input specifications
**      argc                    ptr to number of command line parameters
**
** Outputs:
**      argv                    ptr to command line parameters
**                              which may have been created from
**                              an input file specification
**      argc                    ptr to number of command line parameters
**
**      Returns:
**          OK  : if successful.
**	    FAIL: if failed 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      31-oct-01 (gupsh01)
**          Created.(Based on fileinput:opqutils.sc.)
**
*/
STATUS
xffileinput(
char 		   *paramfile,
char               ***argv,
i4                 *argc)
{
    FILE        *inf;
    STATUS      stat = FAIL;
    LOCATION    inf_loc;

   /* create argv and argc to simulate a "long" command input line */
   /* open the file containing the specifications */

   inf = NULL;

   if (
      ((stat =LOfroms(PATH & FILENAME, paramfile, &inf_loc)) != OK)
       ||
       ((stat = SIopen(&inf_loc, "r", &inf)) != OK)
       ||
       (inf == NULL)
      )
   {   /* report error dereferencing or opening file */
        char        buf[ER_MAX_LEN];
        if ((ERreport(stat, buf)) != OK) buf[0] = '\0';
	IIUGerr(E_XF0007_Cannot_open_param, UG_ERR_ERROR, 1, paramfile);
	return (FAIL);
    }  

    {
        char  buf[XF_MAXBUFLEN]; /* buffer used to read lines 
				 ** from the command file */
	/* ptr to current parameter to be copied from temp buffer */
        char            **targv=(char **)NULL; 
        i4              targc   ;/* number of parameters 
				 ** which have been copied */
        SIZE_TYPE       len;	/* size of dyn.mem. to be allocated. */
        i4              argcount = XF_MAXARGLEN;

        /* get room for maxarglen args */
        len = XF_MAXARGLEN * sizeof(char *);
	targv = (char **)NULL;
	targv = (char **)XF_REQMEM(len, FALSE);

        targc           = 1;
        buf[XF_MAXBUFLEN-1]= 0;

        while ((stat = SIgetrec((PTR)&buf[0], (i4)(XF_MAXBUFLEN-1), inf))
                == OK)
        {
            char *cp = &buf[0];
            do
            {
                if (targc >= argcount)
                {
                    i4  i;
                    char **targv1 = NULL;

                   /* Allocate another vector, 
		   ** twice the size of the prev. */
	           targv = (char **)XF_REQMEM((i4)
			     (argcount * 2 * sizeof(char *)), FALSE);
                    /* Copy previous into current. */
                    for (i = 0; i < argcount; i++)
                        targv1[i] = targv[i];
                    targv = targv1;
                    argcount += argcount;
                }

                {
                    char *tp = (char *) NULL;
                    i4  arglen = STlength(cp);
                    tp = (char *)XF_REQMEM (arglen, FALSE);
                    targv[targc++] = tp;
		    {
                        i4 len = STlength(cp);
                        cp[len - 1] = EOS; 
                        STskipblank(cp,(len -1));
                        STtrmwhite(cp);
                        STcopy(cp, tp);
                        cp = &cp[len - 1];
                    }
                }
            }
            while (*cp);
        }

        if (stat != ENDFILE)
        { 
	    /* report error unable to read line from the file */
	    char        buf[ER_MAX_LEN];
	    if ((ERreport(stat, buf)) != OK) buf[0] = '\0';
	    IIUGerr(E_XF0008_Cannot_read_param, UG_ERR_ERROR, 1, paramfile);
	    return (FAIL);
        }
        *argv = targv;
        *argc = targc;
    }

    /* Close input file */
    stat = SIclose(inf);
    if (stat != OK)
    {   /* report error closing file */
	char        buf[ER_MAX_LEN];
	if ((ERreport(stat, buf)) != OK) buf[0] = '\0';
	IIUGerr(E_XF0025_Cannot_close_param, UG_ERR_ERROR, 1, paramfile);
	return (FAIL);
    }   
    return OK;
}
