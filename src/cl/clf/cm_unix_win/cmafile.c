/*
** Copyright (c) 1989, 2009 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <clsecret.h>
# include <systypes.h>
# include <cs.h>
# include <cm.h>
# include <er.h>
# include <nm.h>
# include <st.h>
# include <lo.h>
# include <si.h>

# include <locale.h>
# ifdef NT_GENERIC
# include <windows.h>
# include <winnls.h>
# endif /* NT_GENERIC */

# ifdef xCL_006_FCNTL_H_EXISTS
# include       <fcntl.h>
# endif /* xCL_006_FCNTL_H_EXISTS */
# include <ernames.h>

#ifdef  VMS
#include <fab.h>
#include <rab.h>
#include <rmsdef.h>
#endif

#include <errno.h>


/**
**
**  Name: CMAFILE.C - Routines to read and write attribute files
**
**  Description:
**	The file contains routines to read and write character attribute
**	description file.  This does not use SI because that is not legal
**	for the DB server.
**
**		CMget_charset_name - Get installation charset name
**		CMset_attr - set attribute table for CM macros.
**		CMset_charset - set attribute table for installation charset
**		CMset_locale - set locale for the current process
**		CMwrite_attr - write attribute file
**
**  History:
**	9/89 (bobm)    Created.
**      25-sep-90 (bonobo)
**	   Added #include <systypes.h>, <clsecret.h> and #ifdef 
**	   xCL_006_FCNTL_H_EXISTS for portability.
**	07-jan-1992 (bonobo)
**	   Added the following change from ingres63p cl: 
**	   01-jul-1991 (bobm)
**	01-jul-1991 (bobm)
**	   Change search path in attrfile().
**
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-apr-95 (canor01)
**	    added <errno.h>
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	03-jun-1996 (canor01)
**	    Remove NMloc() semaphore.
**	23-sep-1996 (canor01)
**	    Move all global data definitions to cmdata.c.
**	11-Nov-1999 (jenjo02)
**	    Use CL_CLEAR_ERR instead of SETCLERR to clear CL_ERR_DESC.
**	    The latter causes a wasted ___errno function call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-may-2002 (somsa01)
**	    Added CMset_locale().
**	22-may-2002 (somsa01)
**	    Corrected setting of charsets for non-NT platforms.
**      25-Jun-2002 (fanra01)
**          Sir 108122
**          Add ability to open a specified character map file.
**	05-Nov-2002 (gupsh01)
**	    Added SLAV852.
**	23-jan-20004 (stephenb)
**	    work for generic double-byte build,
**	    extend ENTRY to include double-byte attributes
**	23-Jan-2004 (gupsh01)
**	    Added CM_getcharset to obtain the characterset for the
**	    platform.
**	05-Mar-2004 (hweho01)
**          Enabled CMread_attr() to Unix platforms.
**	14-Jun-2004 (schka24)
**	    Changes for safer string handling.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	2-Dec-2004 (bonro01)
**	    CMset_attr() was previously changed so that it returns an
**	    error when a character set is not in the charsets[] table.
**	    Need to add additional supported character sets to the table.
**	    Add check for value column so that if the value is NULL it
**	    will default to the Windows 1252 codepage.
**	14-Dec-2004 (gupsh01)
**	    Use correct values for windows OEM code pages for ISO 8859..
**	    charset values.
**	06-Jul-2005 (gupsh01)
**	    Moved CM_getcharset to cmgetcp.c
**	18-Oct-2005 (gupsh01)
**	    Added 88597, win1252 and pc747 to charsets table.
**	28-Jun-2006 (kiria01) b116284
**	    Change THAI charset definition to be single byte as per
**	    CP874 definition.
**	11-Apr-2007 (gupsh01) 
**	    Added support for UTF8.
**	14-Jul-2008 (gupsh01)
**	    Modify CM_utf8casetab and CM_utf8attrtab to 
**	    CM_casetab_UTF8 and CM_attrtab_UTF8.
**      05-feb-2009 (joea)
**          Replace CMget_attr by CMischarset_doublebyte.  Rename
**          CM_ischarsetUTF8 to CMischarset_utf8.  Remove CMgetUTF8.
**       5-Nov-2009 (hanal04) Bug 122807
**          CM_CaseTab needs to be u_char or we get values wrapping and
**          failing integer equality tests.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    UTF8 is now doublebyte
**	    Initialize CM_isDBL based on II_CHARSET
**	    Double_byte has same function as CM_isDBL now - removing.
**/

/*
**  External variables
*/

/*
** the default, compiled in translation.
*/
GLOBALREF u_i2 CM_DefAttrTab[];
GLOBALREF char CM_DefCaseTab[];
GLOBALREF i4 CM_BytesForUTF8[];
GLOBALREF CM_UTF8ATTR CM_attrtab_UTF8[];
GLOBALREF CM_UTF8CASE CM_casetab_UTF8[];

static char *attrfile();

static CMATTR Readattr;

GLOBALREF u_i2 *CM_AttrTab;
GLOBALREF u_char *CM_CaseTab;
GLOBALREF bool CM_isUTF8;
GLOBALREF bool CM_isDBL;
GLOBALREF i4   *CM_UTF8Bytes;
GLOBALREF CM_UTF8ATTR *CM_UTF8AttrTab;
GLOBALREF CM_UTF8CASE *CM_UTF8CaseTab;

/*
** Local definitions
*/
typedef struct tagENTRY
{
    char value[128];
    char key[128];
    bool isdouble; /* is character set double-byte */
}ENTRY;

static ENTRY charsets[]=
{
    {"737", "PC737", TRUE},
    {"874", "THAI", FALSE},
    {"874", "WTHAI", FALSE},
    {"932", "SHIFTJIS", TRUE},
    {"932", "KANJIEUC", TRUE},
    {"936", "CHINESES", TRUE},
    {"936", "CHINESET", TRUE},
    {"936", "CSGBK", TRUE},
    {"936", "CSGB2312", TRUE},
    {"949", "KOREAN", TRUE},
    {"950", "CHTBIG5", TRUE},
    {"950", "CHTEUC", TRUE},
    {"950", "CHTHP", TRUE},
    {"852",  "SLAV852", FALSE},
    {"1200", "UCS2", FALSE},
    {"1200", "UCS4", FALSE},
    {"1200", "UTF7", FALSE},
    {"1200", "UTF8", TRUE},
    {"1250", "IBMPC850", FALSE},
    {"1250", "WIN1250", FALSE},
    {"1251", "IBMPC866", FALSE},
    {"1251", "ALT", FALSE},
    {"1251", "CW", FALSE},
    {"1252", "WIN1252", FALSE},
    {"28597", "GREEK", FALSE},
    {"1253", "WIN1253", FALSE},
    {"1254", "PC857", FALSE},
    {"1255", "HEBREW", FALSE},
    {"1255", "WHEBREW", FALSE},
    {"1255", "PCHEBREW", FALSE},
    {"1256", "ARABIC", FALSE},
    {"1256", "WARABIC", FALSE},
    {"1256", "DOSASMO", FALSE},
    {"1257", "WIN1252", FALSE},
    {"28591", "ISO88591", FALSE},
    {"437", "IBMPC437", FALSE},
    {"", "HPROMAN8", FALSE},
    {"28605", "IS885915", FALSE},
    {"28592", "ISO88592", FALSE},
    {"28595", "ISO88595", FALSE},
    {"28597", "ISO88597", FALSE},
    {"28599", "ISO88599", FALSE},
    {"", "DECMULTI", FALSE},
    {"855", "KOI8", FALSE},
    {"1252", "INST1252", FALSE},
    {0,0,0}
};

/*{
**  Name: CMset_attr - set character attributes.
**
**  Description:
**	Sets character attribute and case translation to correspond
**	to installed character set.  If an alternate character set has
**	never been installed, assure that the default character set is
**	represented.  If an error status is returned, the default character
**	set will be reflected, also.
**
**  Inputs:
**	name - name of character set.
**
**  Outputs:
**	Returns:
**		CM_NOCHARSET - character set does not exist.
**		STATUS
**
**  Side Effects:
**	may perform file i/o on a "hidden" file name.
**
**  History:
**	9/89 (bobm)    Created.
**      3/91 (stevet)  Fixed SETCLERR argument error.
**      21-May-95   (fanra01)
**          Modified open to read in binary mode.
**	04-Aug-95 (wonst02)
**	    Modified open to write in binary mode for DESKTOP systems.
**	25-jun-2002 (fanra01)
**	    Initialize lbuf as a value here now specifies the file to open.
**	23-jan-2004 (stephenb)
**	    Set double-byte attribute
**	11-apr-2007 (gupsh01)
**	    Set UTF8 attribute.
**
*/

STATUS
CMset_attr(name,err)
char *name;
CL_ERR_DESC *err;
{
#ifdef VMS
	struct FAB fab;
	struct RAB rab;
	long status;
#else
	i4 fd;
#endif
	char *fn;
	bool exists;
	LOCATION loc;
	char lbuf[MAX_LOC+1] = { '\0' };
	i4	i;

	CL_CLEAR_ERR( err );

	/*
	** set defaults in case we fail.
	*/
	CM_AttrTab = CM_DefAttrTab;
	CM_CaseTab = CM_DefCaseTab;
	CM_UTF8Bytes = CM_BytesForUTF8;

	if (name == NULL || *name == EOS)
		return OK;

	if ((fn = attrfile(name,&loc,lbuf,err,&exists)) == NULL)
		return FAIL;

	if (! exists)
	{
		SETCLERR(err, 0, ER_open);
		return CM_NOCHARSET;
	}
	if (STbcompare(name, 0, "utf8", 0, TRUE) == 0)
	  CM_isUTF8 = 1;

	for (i = 0; STbcompare(name, 0, charsets[i].key, 0, TRUE); i++)
	{
	    if (charsets[i].key[0] == 0)
	    {
		SETCLERR(err, 0, ER_open);
		return CM_NOCHARSET;
	    }
	}
	CM_isDBL = charsets[i].isdouble;
		 

#ifdef VMS

	if (init_frab(fn,&fab,&rab,FALSE) != OK)
	{
		SETCLERR(err, 0, ER_open);
		return FAIL;
	}

	rab.rab$l_rbf = &Readattr;

	status = sys$get(&rab);
	if ((status & 1) == 0)
	{
		sys$close(&fab);
		SETCLERR(err, 0, ER_write);
		return FAIL;
	}

	/*
	** if we can't close the file, don't call it an error -
	** We did manage to retrieve the attributes, so we will
	** return OK.
	*/
	sys$close(&fab);

#else

# ifdef DESKTOP    /* Special treatment for binary files. */
	if ((fd = open(fn,O_RDONLY|O_BINARY)) < 0)
# else
	if ((fd = open(fn,O_RDONLY)) < 0)
# endif
	{
		SETCLERR(err, 0, ER_open);
		return FAIL;
	}

	if (read(fd, (char *) &Readattr, sizeof(CMATTR)) != sizeof(CMATTR))
	{
		close(fd);
		SETCLERR(err, 0, ER_read);
		return FAIL;
	}

	close(fd);

#endif

	CM_AttrTab = Readattr.attr;
	CM_CaseTab = Readattr.xcase;

	return OK;
}

/*{
**  Name: CMischarset_doublebyte - Is the current character set a DBCS?
**
**  Description:
**      Is the current character set a double-byte character set?
**
**  Inputs:
**	None.
**
**  Outputs:
**      None
**
** Returns
**      TRUE if current charset is double-byte
**      FALSE otherwise
**
**  History:
**	05-feb-2009 (joea)
**	    Created.
*/
bool
CMischarset_doublebyte()
{
    return CM_isDBL;
}

/*{
**  Name: CMwrite_attr - write character attribute file.
**
**  Description:
**	Use a passed-in CMATTR to write a named character attributes and case
**	translation file.  Once this call is successful, CMset_attr calls 
**	using that character set will reflect the given CMATTR.
**
**  Inputs:
**
**	name - name of character set.
**	attr - character attributes and case translation.
**
**  Outputs:
**	Returns:
**		STATUS
**
**  Side Effects:
**	Perform file i/o on a "hidden" file name.
**
**  History:
**	9/89 (bobm)    Created.
**	25-jun-2002 (fanra01)
**	    Initialize lbuf as a value here now specifies the file to open.
**
*/

STATUS
CMwrite_attr(name,attr,err)
char *name;
CMATTR *attr;
CL_ERR_DESC *err;
{
#ifdef VMS
	struct FAB fab;
	struct RAB rab;
	long status;
#else
	int fd;
#endif
	char *fn;
	bool trash;
	LOCATION loc;
	char lbuf[MAX_LOC+1] = { '\0' };

	CL_CLEAR_ERR( err );

	if ((fn = attrfile(name,&loc,lbuf,err,&trash)) == NULL)
		return FAIL;

#ifdef VMS

	if (init_frab(fn,&fab,&rab,TRUE) != OK)
	{
		SETCLERR(err, 0, ER_open);
		return FAIL;
	}

	rab.rab$l_rbf = attr;

	status = sys$put(&rab);
	if ((status & 1) == 0)
	{
		sys$close(&fab);
		SETCLERR(err, 0, ER_write);
		return FAIL;
	}

	status = sys$close(&fab);
	if ((status & 1) == 0)
	{
		SETCLERR(err, 0, ER_write);
		return FAIL;
	}
#else

# ifdef DESKTOP    /* Special treatment for binary files. */
	if ((fd = open(fn,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,0666)) < 0)
# else
	if ((fd = open(fn,O_WRONLY|O_CREAT|O_TRUNC,0666)) < 0)
# endif
	{
		SETCLERR(err, 0, ER_open);
		return FAIL;
	}

	if (write(fd, (char *) attr, sizeof(CMATTR)) != sizeof(CMATTR))
	{
		close(fd);
		SETCLERR(err, 0, ER_write);
		return FAIL;
	}

	close(fd);

#endif

	return OK;
}

/*
** routine to concoct the file name using NM / LO, etc.  Also returns
** whether it exists or not.
*/

static char *
attrfile(name,loc,lbuf,err,exists)
char *name;
LOCATION *loc;
char *lbuf;
CL_ERR_DESC *err;
bool *exists;
{
	LOCATION	tloc;
	char		*str;
	STATUS		st;
	char		nm[CM_MAXATTRNAME+1];

	if (STlength(name) > CM_MAXATTRNAME)
	{
		SETCLERR(err, 0, ER_open);
		return NULL;
	}

	STcopy(name,nm);
	name = nm;

	/*
	** Check name via 'a' - 'z', 'A' - 'Z', '1' - '9'
	** ruse - we're inside the CL so we can get away with this.
	** Done this way so that we don't worry about our current
	** CM translation.  Must be done some different way on a system
	** where these characters are not consecutive.
	*/
	for ( str = name; *str != EOS; ++str)
	{
		if ( *str >= 'a' && *str <= 'z' )
			continue;
		if ( *str >= 'A' && *str <= 'Z' )
		{
			*str += 'a' - 'A';
			continue;
		}
		if ( *str < '0' || *str > '9' )
		{
			SETCLERR(err, 0, ER_open);
			return NULL;
		}
	}

	/*
	** If the path has been specified, use it.  Otherwise default to looking
	** in the charset name directory.
	*/
	if (lbuf && *lbuf)
	{
	    LOfroms( (PATH & FILENAME), lbuf, loc );
	}
	else
	{
	    st = NMloc(FILES, PATH, (char *)NULL, &tloc);
	    if (st != OK)
	    {
		SETCLERR(err, 0, ER_open);
		return NULL;
	    }

	    LOcopy(&tloc,lbuf,loc);

	    LOfaddpath(loc,"charsets",loc);
	    LOfaddpath(loc,name,loc);
	    LOfstfile("desc.chx",loc);
	}
	LOtos(loc, &str);

	if (LOexist(loc) != OK)
		*exists = FALSE;
	else
		*exists = TRUE;

	return str;
}

/*{
**  Name: CMset_attr - set process' locale based upon II_CHARSETxx
**
**  Description:
**	This procedure grabs the value of II_CHARSETxx, using it to
**	determine the machine's proper locale mapping. It then uses
**	setlocale() to set the particular code page for the current
**	process.
**
**  Inputs:
**	none
**
**  Outputs:
**	Returns:
**		OK
**		FAIL
**
**  Side Effects:
**	none
**
**  History:
**	14-may-2002 (somsa01)
**	    Created.
*/

STATUS
CMset_locale()
{
    char	*ii_installation, charset_var[16+1], *ii_charset;
    int		i, total_charsets = sizeof(charsets)/sizeof(ENTRY);
    char	code_page[16+1];

#ifdef NT_GENERIC
    NMgtAt(ERx("II_INSTALLATION"), &ii_installation);
    if (ii_installation == NULL || *ii_installation == EOS)
	return(FAIL);

    STlpolycat(2, sizeof(charset_var)-1,
		"II_CHARSET", ii_installation, &charset_var[0]);
    NMgtAt(charset_var, &ii_charset);
    if (ii_charset == NULL || *ii_charset == EOS)
	return(FAIL);
 
    /*
    ** Set the default to the 1252 code page.
    */
    STcopy(".1252", code_page);

    for (i=0; i<total_charsets; i++)
    {
	if (STcompare(charsets[i].key, ii_charset) == 0)
	{
	    if (charsets[i].value[0] != 0)
		STprintf(code_page, ".%s", charsets[i].value);
	    break;
	}
    }

    setlocale(LC_ALL, code_page);
#endif  /* NT_GENERIC */

    return(OK);
}

#ifdef VMS

static
init_frab(fab,rab,fn,creat)
FAB *fab;
RAB *rab;
char *fn;
bool creat
{
	MEfill(sizeof(FAB), 0, fab);
	MEfill(sizeof(RAB), 0, rab);
	fab->fab$b_bid = FAB$C_BID;
	fab->fab$b_bln = FAB$C_BLN;
	rab->rab$b_bid = RAB$C_BID;
	rab->rab$b_bln = RAB$C_BLN;
	rab->rab$l_fab = fab;
	fab->fab$b_rfm = FAB$C_FIX;
	fab->fab$b_org = FAB$C_SEQ;
	rab->rab$w_rsz = fab->fab$w_mrs = sizeof(CMATTR);
	fab->fab$l_fna = fn;
	fab->fab$b_fns = STlength(fn);
	rab->rab$b_rac = RAB$C_SEQ;

	if (creat)
		status = sys$create(fab);
	else
		status = sys$open(fab);
	if ((status & 1) == 0)
		return FAIL;

	status = sys$connect(rab);
	if ((status & 1) == 0)
		return FAIL;

	return OK;
}

#endif


# if defined(NT_GENERIC)
/*
**
**      12-Dec-95 (fanra01)
**          Added functions to return address of CM_AttrTab and CM_CaseTab.
**          Only used in DLLs to resolve global data in macros.
*/
/*
**  Name: CMgetAttrTab - get the pointer CM_AttrTab
**
**  Description:
**      Function added to return the pointer CM_AttrTab for DLL implementation
**      only.
**
**  History:
**      20-Sep-95 (fanra01)
**          Created.
*/

u_i2 * CMgetAttrTab (void)
{
    return(CM_AttrTab);
}

/*
**  Name: CMgetCaseTab - get the pointer CM_CaseTab
**
**  Description:
**      Function added to return the pointer CM_AttrTab for DLL implementation
**      only.
**
**  History:
**      20-Sep-95 (fanra01)
**          Created.
*/

u_char *CMgetCaseTab  (void)
{
    return(CM_CaseTab);
}

CM_UTF8ATTR *CMgetUTF8AttrTab (void)
{
    return(CM_UTF8AttrTab);
}

CM_UTF8CASE *CMgetUTF8CaseTab (void)
{
    return(CM_UTF8CaseTab);
}

/*
**  Name: CMgetUTF8Bytes - returns the table CM_UTF8Bytes
**
**  Description:
**      Function added to return the value of CM_UTF8Bytes for DLL 
**	implementation only.
**
**  History:
**      12-Jun-2007 (gupsh01)
**          Fixed the return type to i4 *.
*/
i4 *CMgetUTF8Bytes(void)
{
    return(CM_UTF8Bytes);
}
# endif             /* NT_GENERIC  */

/*{
**  Name: CMischarset_utf8 - is the current character set UTF-8
**
**  Description:
**      Returns TRUE if II_CHARSETxx is set to UTF8 else returns FALSE.
**
**  History:
**      26-Apr-2007 (gupsh01)
**          Created.
**      05-feb-2009 (joea)
**          Renamed and simplified logic.
*/
bool
CMischarset_utf8(void)
{
    return CM_isUTF8;
}

/*{
**  Name: CMread_attr - readn and set character attributes.
**
**  Description:
**	Sets character attribute and case translation to correspond
**	to installed character set.  If an alternate character set has
**	never been installed, assure that the default character set is
**	represented.  If an error status is returned, the default character
**	set will be reflected, also.
**
**  Inputs:
**	name - name of character set.
**
**  Outputs:
**	Returns:
**		CM_NOCHARSET - character set does not exist.
**		STATUS
**
**  Side Effects:
**	may perform file i/o on a "hidden" file name.
**
**  History:
**      21-Jun-2002 (fanra01)
**          Created based on CMset_attr in clf!cm   cmafile.c
**
*/

STATUS
CMread_attr( char* name, char* filespec, CL_ERR_DESC* err)
{
        STATUS status = OK;
	i4 fd;
	char *fn;
	bool exists;
	LOCATION loc;
	char lbuf[MAX_LOC+1] = {0};

	CL_CLEAR_ERR( err );

	/*
	** set defaults in case we fail.
	*/
	CM_AttrTab = CM_DefAttrTab;
	CM_CaseTab = CM_DefCaseTab;
	CM_UTF8Bytes = CM_BytesForUTF8;

	if (name == NULL || *name == EOS)
		return OK;

        if ((filespec != NULL) && (*filespec))
        {
            STlcopy( filespec, lbuf, MAX_LOC );
        }

	if ((fn = attrfile(name,&loc,lbuf,err,&exists)) == NULL)
		return FAIL;

	if (! exists)
	{
		SETCLERR(err, 0, ER_open);
		return CM_NOCHARSET;
	}

# ifdef DESKTOP    /* Special treatment for binary files. */
	if ((fd = open(fn,O_RDONLY|O_BINARY)) < 0)
# else
	if ((fd = open(fn,O_RDONLY)) < 0)
# endif
	{
		SETCLERR(err, 0, ER_open);
		return FAIL;
	}

	if (read(fd, (char *) &Readattr, sizeof(CMATTR)) != sizeof(CMATTR))
	{
		close(fd);
		SETCLERR(err, 0, ER_read);
		return FAIL;
	}

	close(fd);


	CM_AttrTab = Readattr.attr;
	CM_CaseTab = Readattr.xcase;

	return OK;
}


/* Name: CMget_charset_name - Get II_CHARSET name
**
** Description:
**	The Ingres character set is in an environment variable
**	named either II_CHARSET or II_CHARSETxx where xx is the
**	installation ID.  Fetching the character set name is
**	sufficiently common throughout Ingres that it's worth doing
**	a little routine to fetch it -- safely, with no buffer
**	overrun holes!
**
** Inputs:
**	charset		An output, actually
**
** Outputs:
**	charset		Caller supplied char[CM_MAXATTRNAME+1] area.
**			Name of character set is returned here.
**			If no II_CHARSET env vars at all, this is set
**			to a null string.
**
** History:
**	14-Jun-2004 (schka24)
**	    Write for buffer overrun safety.
**	22-Jun-2004 (schka24)
**	    Oops.  Avoid barfo if no II_CHARSET set, as advertised.
*/

void
CMget_charset_name(char *charset)
{

    char csevname[30+1];		/* II_CHARSETxx */
    char *evp;				/* Pointer to env var value */

    NMgtAt("II_INSTALLATION", &evp);	/* Get installation ID */
    if (evp == NULL || *evp == '\0')
    {
	/* No II_INSTALLATION, use II_CHARSET */
	NMgtAt("II_CHARSET", &evp);
    }
    else
    {
	/* Construct II_CHARSETxx */
	STlpolycat(2, sizeof(csevname)-1, "II_CHARSET", evp, &csevname[0]);
	NMgtAt(csevname, &evp);
    }
    charset[0] = '\0';
    if (evp != NULL)
	STlcopy(evp, &charset[0], CM_MAXATTRNAME);

} /* CMget_charset_name */

/* Name: CMset_charset - Set up CM character set attributes
**
** Description:
**	Quite a few Ingres components need to call CMset_attr with the
**	installation character set name, sometime during initialization.
**	This convenience routine simply combines calls to CMget_charset_name
**	(to get the installation character set name) and CMset_attr.
**
** Inputs:
**	cl_err			An output
**
** Outputs:
**	Returns OK or not-OK status (see CMset_attr)
**	cl_err			Place to return a CL_ERR_DESC describing
**				errors, if any. 
**
** History:
**	14-Jun-2004 (schka24)
**	    Write for buffer overrun fixups.
*/

STATUS
CMset_charset(CL_ERR_DESC *cl_err)
{

    char chset[CM_MAXATTRNAME+1];	/* Installation charset name */

    CMget_charset_name(&chset[0]);

    return (CMset_attr(chset, cl_err));

} /* CMset_charset */
