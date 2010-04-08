/*
** Copyright (c) 1989, 2009 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <me.h>
# include <cm.h>
# include <nm.h>
# include <st.h>
# include <lo.h>
# include <si.h>
# include <ernames.h>

# include <locale.h>
# ifdef xCL_006_FCNTL_H_EXISTS
# include       <fcntl.h>
# endif /* xCL_006_FCNTL_H_EXISTS */

#include <fab.h>
#include <rab.h>
#include <rmsdef.h>
#include <starlet.h>

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
**	    Added #include <systypes.h>, <clsecret.h> and #ifdef 
**	    xCL_006_FCNTL_H_EXISTS for portability
**      04-feb-1991 (wolf)
**	    Change VMS definition of *CM_AttrTab and *CM_CaseTab to make the
**	    DBMS group's CL image more static.
**      1-mar-91 (stevet)
**	    Fixed RMS problem with CMset_attr.
**	4/91 (Mike S)
**	   Moved global declarations to cmglobs.c .
**	7/91 (bobm)
**	   change search path in attrfile().
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by adding missing includes and
**	    forward function references
**	01-dec-2000	(kinte01)
**		Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**		from VMS CL as the use is no longer allowed
**	14-may-2002 (somsa01)
**	    Added CMset_locale().
**	23-jan-20004 (stephenb)
**	    work for generic double-byte build,
**	    extend ENTRY to include double-byte attributes
**	23-Jan-2004 (gupsh01)
**	    Added CM_getcharset to obtain the characterset for the
**	    platform.
**	14-Jun-2004 (schka24)
**	    Changes for safer string handling.
**	14-jan-2005 (abbjo03)
**	    Add CMgetDefCS.
**	28-Jun-2006 (kiria01) b116284
**	    Change THAI charset definition to be single byte as per
**	    CP874 definition.
**	11-apr-2007 (gupsh01)
**	    Added CM_isUTF8.
**	03-jun-2008 (gupsh01)
**	    Update VMS CM with changes for UTF8.
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC. Remove non-VMS-specific
**	    code.
**      05-feb-2009 (joea)
**          Replace CMget_attr by CMischarset_doublebyte.  Rename
**          CM_ischarsetUTF8 to CMischarset_utf8.
**       5-Nov-2009 (hanal04) Bug 122807
**          CM_CaseTab needs to be u_char or we get values wrapping and
**          failing integer equality tests.
**	03-Dec-2009 (wanfr01) Bug 122994
**	    UTF8 is now doublebyte
**	    Initialize CM_isDBL based on II_CHARSET
**/

/*
** the default, compiled in translation.
*/
GLOBALREF u_i2 CM_DefAttrTab[];
GLOBALREF char CM_DefCaseTab[];
GLOBALREF i4   CM_BytesForUTF8[];
GLOBALREF CM_UTF8ATTR CM_utf8attrtab[];
GLOBALREF CM_UTF8CASE CM_utf8casetab[];

GLOBALREF i4 CMdefcs;

static char *attrfile();
static bool Double_byte = FALSE;
static STATUS init_frab();

static CMATTR Readattr;

/* Pointers to the current tables */
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
    {"1253", "GREEK", FALSE},
    {"1253", "ELOT437", FALSE},
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
**	23-jan-2004 (stephenb)
**	    Set double-byte attribute
**	11-apr-2007 (gupsh01)
**	    Added CM_isUTF8.
**	03-jun-2008 (gupsh01)
**	    Update for UTF8 support.
*/
STATUS
CMset_attr(
char		*name,
CL_ERR_DESC	*err)
{
	struct FAB fab;
	struct RAB rab;
	long status;
	char *fn;
	bool exists;
	LOCATION loc;
	char lbuf[MAX_LOC+1];
	i4	i;

	CL_CLEAR_ERR(err);

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
	Double_byte = charsets[i].isdouble;
	CM_isDBL = charsets[i].isdouble;

	if (init_frab(&fab,&rab,fn,FALSE,&status) != OK)
	{
		err->error = status;
		return FAIL;
	}

	rab.rab$l_ubf = &Readattr;

	status = sys$get(&rab);
	if ((status & 1) == 0 || rab.rab$w_rsz != sizeof(CMATTR))
	{
		err->error = RMS$_USZ;
		sys$close(&fab);
		return FAIL;
	}

	/*
	** if we can't close the file, don't call it an error -
	** We did manage to retrieve the attributes, so we will
	** return OK.
	*/
	sys$close(&fab);

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
    return Double_byte;
}

/*{
**  Name: CMischarset_utf8 - is the current character set UTF-8
**
**  Description:
**      Returns TRUE if II_CHARSETxx is set to UTF8 else returns FALSE.
**
**  History:
**      26-Apr-2007 (gupsh01)
**          Created.
*/
bool
CMischarset_utf8(void)
{
    return CM_isUTF8;
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
**
*/

STATUS
CMwrite_attr(name,attr,err)
char *name;
CMATTR *attr;
CL_ERR_DESC *err;
{
	struct FAB fab;
	struct RAB rab;
	long status;
	char *fn;
	bool trash;
	LOCATION loc;
	char lbuf[MAX_LOC+1];

	CL_CLEAR_ERR(err);

	if ((fn = attrfile(name,&loc,lbuf,err,&trash)) == NULL)
		return FAIL;

	if (init_frab(&fab,&rab,fn,TRUE,&status) != OK)
	{
		err->error = status;
		return FAIL;
	}

	rab.rab$l_rbf = attr;

	status = sys$put(&rab);
	if ((status & 1) == 0)
	{
		sys$close(&fab);
		err->error = status;
		return FAIL;
	}

	status = sys$close(&fab);
	if ((status & 1) == 0)
	{
		err->error = status;
		return FAIL;
	}

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

	st =NMloc(FILES, PATH, (char *)NULL, &tloc);
	if (st != OK)
	{
		SETCLERR(err, 0, ER_open);
		return NULL;
	}

	LOcopy(&tloc,lbuf,loc);

	LOfaddpath(loc,"charsets",loc);
	LOfaddpath(loc,name,loc);
	LOfstfile("desc.chx",loc);

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
    return OK;
}

/* Name: CM_getcharset- Get Platforms default character set
**
** Input:
**   pcs - Pointer to hold the value of platform characterset. The
**	   calling funtion allocates the memory for this variable.
** Output:
**   pcs - The platform character set.
** Results:
**   returns OK if successful, FAIL if error.
** History:
**
**	23-Jan-2004 (gupsh01)
**	    Created.
*/
STATUS
CM_getcharset(
char	*pcs)
{
    char *charset = NULL;

    if (!pcs)
	return FAIL;

    if (charset = setlocale (LC_CTYPE,0))
    {
	STlcopy (charset, pcs, CM_MAXLOCALE);
	return OK;
    }
    else
	return FAIL;
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

i4 CMgetDefCS()
{
    return CMdefcs;
}


static STATUS
init_frab(fab,rab,fn,creat,rstat)
struct FAB *fab;
struct RAB *rab;
char *fn;
bool creat;
i4 *rstat;
{
	i4 status;

	MEfill(sizeof(struct FAB), 0, (PTR)fab);
	MEfill(sizeof(struct RAB), 0, (PTR)rab);
	fab->fab$b_bid = FAB$C_BID;
	fab->fab$b_bln = FAB$C_BLN;
	rab->rab$b_bid = RAB$C_BID;
	rab->rab$b_bln = RAB$C_BLN;
	rab->rab$l_fab = fab;
	fab->fab$b_rfm = FAB$C_FIX;
	fab->fab$b_org = FAB$C_SEQ;
	rab->rab$w_usz = rab->rab$w_rsz = fab->fab$w_mrs = sizeof(CMATTR);
	fab->fab$l_fna = fn;
	fab->fab$b_fns = STlength(fn);
	rab->rab$b_rac = RAB$C_SEQ;

	if (creat)
		status = sys$create(fab);
	else
		status = sys$open(fab);
	if ((status & 1) == 0)
	{
		*rstat = status;
		return FAIL;
	}

	status = sys$connect(rab);
	if ((status & 1) == 0)
	{
		*rstat = status;
		return FAIL;
	}

	return OK;
}
