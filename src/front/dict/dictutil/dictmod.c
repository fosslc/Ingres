/*
** Copyright (c) 1990, 2008 Ingres Corporation
**		All rights reserved.
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<ex.h>
# include	<nm.h>
# include	<pc.h>
# include	<si.h>
# include	<st.h>
# include	<ug.h>
# include	<ui.h>
# include	<uigdata.h>
# include	<dictutil.h>
# include	"dctdll.h"	 
# include	"erdu.h"

/**
** Name:	dictmod.c
**
** Description:
**	Utility to modify Ingres Front-End Dictionary.  Given a list of client
**	names, this utility will issue the modify commands and perform
**	module cleanup for each module which is used by the requested clients.
**	If no clients are specified on the command line then all of the
**	installed modules are modified.  If any given client name is invalid
**	or represents an uninstalled module then an error message is printed
**	and processing continues.
**
**	The command line syntax is:
**
**		modifyfe database [-uuser] [-b] [-c] [+w|-w] client {client}
**			-page_size=[2048|4096|8192|16384|32768|65536]
**
**	The UTEXE definition is:
**
**  modifyfe modifyfe PROCESS - database [-uuser] [-b] [-c]
**	client {client}
**		database        "%S"    1       Database?               %S
**		client          %S      2       ?                       %100S
**		wait            "+w"    0       ?                       +w
**		nowait          "-w"    0       ?                       -w
**		equel           "-X%S"  0       ?                       -X%-
**		user            "-u%S"  0       ?                       -u%-
**              connect         +c%S
**
**	This file defines:
**
**	main			- main procedure for modifyfe.
**	modifyallmodules	- modify modules for all requested clients.
**	DDexit			- exit after DB connection made.
**
** History:
** 	24-apr-1990 (rdesmond)
**		First Written.
** 	04-jun-1990 (rdesmond)
**		Now doesn't display 'Connecting to DB ...' message.
**	06-jun-1990 (pete)
**		set username to "$ingres" if not specified by the user. Also
**		remove the +U connect option (this made it so only Ingres
**		super-users can run this program). Also, remove code which
**		checked after the connect, and only allowed the DBA to
**		run this program. Also, make the connect an exclusive one (-l).
**		also add new error message E_DD001B_NO_DB_PRIV if user
**		doesn't have permission to use the "-u" flag; Also, make
**		sure a NULL argument isn't passed to FEningres before last arg.
**      19-jul-1990 (pete)
**              Add GLOBALDEF so modifyfe will link now that we have
**		support for "-s" flag.
**	20-sep-1990 (pete)
**		change dictinstall to upgradefe; dictmod to modifyfe.
**      12-oct-1990 (sandyd)
**              Removed obsolete FEtesting undef.  The sun4 loader ignored the
**              fact that this symbol was never resolved, but on Sequent it
**              was generating an error.
**      18-oct-1990 (sandyd)
**              Fixed #include of local erdu.h to use "" instead of <>.
**	27-dec-1990 (pete)
**		Fix for bug 34847, where the +w and -w flags weren't working.
**	2-20-1991 (pete)
**		Changed  modifymodule() call to mmsModifyModuleStar().
**      18-jul-1991 (pete)
**              Added support for +c flag; WITH clause for Gateways. Bug 38706.
**      29-jul-1991 (pete)
**              Remove -u$ingres on connect statement for cases when user
**              doesn't specify the -u flag. Following connect, make
**              sure that dbmsinfo('username') = dbmsinfo('dba'). If not, then
**              disconnect with error; added error E_DD0027_NOT_DBA.
**              Also, remove -l from connect statement. SIR 38903.
**              Also, factored out common error check code between modifyfe and
**              upgradefe and created routine IIDDcseCheckStartupErrs().
**		If II_DDTRACE is set, then dump module info (related to fix
**		for bug 38984).
**	17-aug-91 (leighb) DeskTop Porting Changes:
**		Added includes of si.h & nm.h;
**		Reference data via a function call instead of referencing
**		data across facilities (for DLL's on IBM/PC).
**	05-sep-91 (leighb) DeskTop Porting Change:
**		Unconditionally include dctdll.h header in all modules 
**		that reference data across facilities; 
**		move cross facilitiy GLOBALREF's to dctdll.h.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**      20-sep-93 (huffman)
**              Readonly memory is passed to CVlower function, causing
**              Access Violation on AXP VMS.
**	9-nov-1993 (jpk)
**		Fixing my oversight in cat_exists() removes need for
**		previous change; cat_exists now supplies the storage
**		so callers don't have to.
**	23-nov-1993 (peterk)
**		fix typo in 9-nov-93 change - caught by HP c89 ANSI C
**		compiler
**	07-jul-1995 (lawst01)
**		Put in ifdef NT_GENERIC for variable name 'connect' - 
**		for NT platform will use variable name 'connect_w'.
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-nov-2001 (stial01)
**          Added support for modifyfe -page_size=%N
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**	07-Feb-2008 (hanje04)
**	    SIR S119978
**	    Rename variable wait to Wait to top compiler errors caused by
**	    conflicts with definitions in sys/wait.h
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	23-Aug-2009 (kschendel) 121804
**	    Fix IIDDcseCheckStartupErrs usage.
*/

/*
**	MKMFIN Hints
**
PROGRAM =	modifyfe

NEEDLIBS =	DICTUTILLIB \
		SHQLIB \
		COMPATLIB \
		SHEMBEDLIB
		
UNDEFS = II_copyright
*/

FUNC_EXTERN bool        IIDDidIsDistributed();
FUNC_EXTERN STATUS      IIDDccCdbConnect();
FUNC_EXTERN STATUS      IIDDcdCdbDisconnect();
FUNC_EXTERN VOID        IIDDcd2CdbDisconnect2();
FUNC_EXTERN bool        IIDDcseCheckStartupErrs();
FUNC_EXTERN char	*IIDDscStatusDecode();

STATUS	modifyallmodules();
static	STATUS	mmsModifyModuleStar();
static  VOID	pmsPrintModuleStatus();

GLOBALREF       bool    IIDDsilent;
GLOBALREF	i4	IIDDpagesize;

static VOID	DDexit();
static char	*dbname;
static char	*uname;
static char	*xflag;
static bool	Wait	= FALSE;
static bool	nowait	= FALSE;
static char	*Wait_final;
static i4	page_size = 0;
#ifdef NT_GENERIC
static char     *connect_w = ERx("");
#else
static char     *connect = ERx("");
#endif

static const char _Pgm[]	= ERx("modifyfe");
static const char _Empty[]	= ERx("");
static const char _Wait[]	= ERx("+w");
static const char _Nowait[]	= ERx("-w");

  static ARG_DESC args[] =
{
    /* Required arguments */
    {ERx("database"),	 DB_CHR_TYPE,	FARG_PROMPT,	(PTR) &dbname},

    /* Optional arguments */
    {ERx("user"),	 DB_CHR_TYPE,	FARG_FAIL,	(PTR) &uname},
    {ERx("wait"),    	 DB_BOO_TYPE,	FARG_FAIL,	(PTR) &Wait},
    {ERx("nowait"),    	 DB_BOO_TYPE,	FARG_FAIL,	(PTR) &nowait},
#ifdef NT_GENERIC
    {ERx("connect"),     DB_CHR_TYPE,   FARG_FAIL,      (PTR) &connect_w},
#else
    {ERx("connect"),     DB_CHR_TYPE,   FARG_FAIL,      (PTR) &connect},
#endif

    /* Internal Optional arguments */
    {ERx("equel"),	 DB_CHR_TYPE,	FARG_FAIL,	(PTR) &xflag},
    {ERx("page_size"),  DB_INT_TYPE,   FARG_FAIL,	(PTR) &page_size},
    NULL,		 0,		0,		NULL
};

main(argc, argv)
i4	argc;
char	**argv;
{
    STATUS	rval;
    ARGRET	rarg;
    i4		pos;
    i4		numobj;
    bool	all_selected;
    char	*p_nm;

    /* Capability checks should go here. */

    /* assign default values for command line arguments */
    dbname = _Empty;
    uname = _Empty;
    xflag = _Empty;

    if ((rval = IIUGuapParse(argc, argv, _Pgm, args)) != OK)
	PCexit(rval);

    if (page_size == 0 || page_size == 2048 || page_size == 4096 || 
	page_size == 8192 || page_size == 16384 || page_size == 32768 ||
	page_size == 65536)
    {
	IIDDpagesize = page_size;
    }
    else
    {
	IIUGerr(E_DD0028_BAD_PAGESIZE, UG_ERR_ERROR, 1, (PTR)&page_size);
	PCexit(FAIL);
    }

    if (Wait)
	Wait_final = _Wait;
    else if (nowait)
	Wait_final = _Nowait;
    else
	Wait_final = _Empty;
	
#ifdef NT_GENERIC
    if (*connect_w != EOS)
    {
        IIUIswc_SetWithClause(connect_w);
    }
#else
    if (*connect != EOS)
    {
        IIUIswc_SetWithClause(connect);
    }
#endif

    rval = FEningres((char *)NULL, 0, dbname, uname, xflag, Wait_final,
				(char*)NULL);

    if (page_size != 0)
    {
	if (valid_dbms_pagesize(page_size) != OK)
	{
	    IIUGerr(E_DD0028_BAD_PAGESIZE, UG_ERR_ERROR, 1, (PTR)&page_size);
	    PCexit(FAIL);
	}
    }

    if (! IIDDcseCheckStartupErrs(rval, dbname, _Pgm, F_DD0005_MODIFY))
	PCexit(FAIL);	/* issues own error messages */

    if ((rval = init_moduletable(NULL)) != OK)
    {
	if (rval == UI_NOTDIRECTORY)
	{
	    IIUGerr(E_DD001A_II_DICTFILES_NOT_DIR, UG_ERR_ERROR, 0);
	    DDexit(FAIL);
	}
	else
	{
	    IIUGerr(E_DD0005_CANT_INIT_MODTABLE, UG_ERR_ERROR, 0);
	    DDexit(FAIL);
	}
    }

    if (init_clienttable() != OK)
    {
	IIUGerr(E_DD0006_CANT_INIT_CLITABLE, UG_ERR_ERROR, 0);
	DDexit(FAIL);
    }

    /* get client list and place in hash table */
    if ((rval = FEutaopen(argc, argv, _Pgm)) != OK)
	return(FAIL);

    /* place all required clients into the clienttable */
    all_selected = TRUE;
    for (numobj = 0;
      (FEutaget(ERx("client"), numobj, FARG_FAIL, &rarg, &pos) == OK);
      numobj++)
    {
	if ((rval = enter_client(rarg.dat.name, 0)) == OK)
	{
	    all_selected = FALSE;
	}
	else
	{
	    if (rval == UI_NO_CLIENT)
	    {
		IIUGerr(E_DD0007_NO_CLIENT, UG_ERR_ERROR, 1, 
		  (PTR)rarg.dat.name);
	    }
	    else if (rval == FAIL)
	    {
		FEutaclose();
		DDexit(FAIL);
	    }
	}
    }
    FEutaclose();

    rval = modifyallmodules(all_selected);

    /* print runtime info? */
    NMgtAt(ERx("II_DDTRACE"), &p_nm);
    if ( (p_nm != NULL) && (*p_nm != EOS))
    {
	pmsPrintModuleStatus();
    }

    DDexit(rval);
    /*NOTREACHED*/
}

/*
**  Dump info on each module in moduletable.
*/
static VOID
pmsPrintModuleStatus()
{
    bool        scanflag;
    MODULEDESC  *thismodule;
    char        *dummy;

    for (scanflag = FALSE;
      IIUGhsHtabScan(moduletable, scanflag, &dummy, &thismodule) != 0;
      scanflag = TRUE)
    {
	SIprintf("\n    <module %s, vers %d status %s>",
	    thismodule->modname, thismodule->modversion,
	    IIDDscStatusDecode(thismodule->status));
    }

    SIprintf("\n");
}

/*{
** Name:	modifyallmodules - modify modules for all requested clients.
**
** Description:
**	This modifies the requested modules or, if no valid modules are
**	specified, all of the installed modules.
**
** Inputs:
**	select_all	- flag which is set if all of the installed modules
**			    are to be modified.
**
** Outputs:
**
**	Returns:
**		STATUS	-	OK:	modules modified.
**				FAIL:	modules not modified.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	May 1990 (rdesmond).
**		first created.
**	27-dec-1990 (pete)
**		Fix bug 35002 so it's not an error to modify a database that
**		was created as "createdb dbname -f nofeclients". Change 2
**		places where "return;" was done to "return(OK)".
**	8-jan-1993 (jpk)
**		maintained FErelexists, added second parameter
**		for owner of table (FIPS support)
**	22-july-1993 (jpk)
**		replaced call to FErelexists with call to cat_exists
**	9-nov-1993 (jpk)
**		Fixing my oversight in cat_exists() removes need for
**		previous change; cat_exists now supplies the storage
**		so callers don't have to.
**	23-nov-1993 (peterk)
**		fix typo in 9-nov-93 change - caught by HP c89 ANSI C
**		compiler
*/
STATUS
modifyallmodules(select_all)
bool	select_all;
{
    char	*dummy;
    CLIENTDESC	*thisclient;
    bool	scanflag;
    MODULEDESC	*thismodule;
    bool	atleastonefailed;

    /* Core must exist before going further (it contains ii_dict_modules) */
    if (cat_exists(ERx("ii_dict_modules"), ""))
    {
	MODULEDESC	hashkey;
	i4		i;

	if (cat_exists(ERx("ii_objects"), ""))
	    /* No front-end catalogs to modify. */
	    return(OK);

	/*
	    This is a 6.[123] database so we should only modify CORE_61 v1
	    and APPLICATION_DEVELOPMENT_1 v1
	*/
	for (i = 0; i < 2; i++)
	{
	    if (i == 0)
		STcopy(ERx("CORE_61"), hashkey.modname);
	    else
		STcopy(ERx("APPLICATION_DEVELOPMENT_1"), hashkey.modname);

	    /* find entry in hash module table to modify */
	    hashkey.modversion = 1;
	    if (IIUGhfHtabFind(moduletable, &hashkey, &thismodule) != OK)
	    {
		IIUGerr(E_DD000D_CANT_FIND_MOD_DEF, UG_ERR_ERROR, 2,
		    hashkey.modname, &hashkey.modversion);
		return (FAIL);
	    }
	    if (mmsModifyModuleStar(thismodule, (bool)TRUE) != OK)
		return (FAIL);
	}
	return(OK);
    }

    /* set status to INSTALLED for all installed modules */
    if (mod_dict_status() != OK)
	return (FAIL);

    atleastonefailed = FALSE;
    if (select_all)
    {
	/* if we want to modify all the modules, step through module table */
	for (scanflag = FALSE; 
	  IIUGhsHtabScan(moduletable, scanflag, &dummy, (PTR) &thismodule) != 0;
	  scanflag = TRUE)
	{
	    if (thismodule->status == INSTALLED &&
		mmsModifyModuleStar(thismodule, (bool)TRUE) != OK)
		    atleastonefailed = TRUE;
	    thismodule->status = MODIFIED;
	}
	return (atleastonefailed ? FAIL : OK);
    }

    /* If any clients are specified then scan the client table. */
    for (scanflag = FALSE; 
      IIUGhsHtabScan(clienttable, scanflag, &dummy, (PTR) &thisclient) != 0;
      scanflag = TRUE)
    {
	i4	index;

	/* if not requested to be installed then ignore it */
	if (!thisclient->required)
	    continue;

	for (index = 0; index < thisclient->numsupmods; index++)
	{
	    thismodule = thisclient->supmods[index];
	    if (thismodule->status == INSTALLED &&
		mmsModifyModuleStar(thismodule, (bool)TRUE) != OK)
		    atleastonefailed = TRUE;
	    thismodule->status = MODIFIED;
	}
    }
    return (atleastonefailed ? FAIL : OK);
}

/*{
** Name:        mmsModifyModuleStar - execute modify script & support star.
**
** Description:
**      This is a wrapper routine, which calls "modifymodule", but if we
**      are connected to Star, it first connects to the CDB.
**
** Inputs:
**      moduledesc      - MODULEDESC for module to modify.
**      cleanup         - flag which is set if cleanup is also desired.
**
** Outputs:
**
**      Returns:
**              OK if module modified correctly.
**		FAIL otherwise.
**
** History:
**      20-feb-1991 (pete)
**              Initial Version.
*/
static STATUS
mmsModifyModuleStar(moduledesc, cleanup)
MODULEDESC      *moduledesc;
bool            cleanup;
{
    STATUS stat = OK;
    bool fail = FALSE;

    if (IIDDidIsDistributed())
    {
        /* We're connected to Star; must execute statements on CDB. Routines
        ** "create_from_file" and "modifymodule" each save up names
        ** of tables involved in CREATE TABLE, DROP TABLE & MODIFY statements.
        */
        if (IIDDccCdbConnect() != OK)
        {
            fail = TRUE;
            goto done;
        }
    }

    /* modifymodule() ends up running each stmt in .mfy file as separate
    ** transaction.
    */
    if ((stat = modifymodule(moduledesc, cleanup)) != OK)
    {
        fail = TRUE;
        goto done;
    }

    if (IIDDidIsDistributed())
    {
        /* Above work was done while attached to CDB; register it with Star
	** & then disconnect from CDB.
	*/
        if (IIDDcdCdbDisconnect() != OK)        /* note: this COMMITs */
        {
            fail = TRUE;
            goto done;
        }
    }

done:

    if (fail)
    {
        /* error above could have left us still connected to CDB */
        if (IIDDidIsDistributed())
        {
	    IIDDcd2CdbDisconnect2();	/* direct disconnect from CDB. */

            if (stat == OK)
                /* if modifymodule() returned FAIL, then it gave its own error*/
                IIUGerr(E_DD0024_CantModifyStar, UG_ERR_ERROR, 0);
        }
    }

    return (fail ? FAIL : OK);
}

/*{
** Name:	DDexit - exit after DB connection made.
**
** Description:
**	Closes Ingres connection and exits with the passed in return code.
**
** Inputs:
**	status		- status code to exit with.
**
** Outputs:
**
**	Returns:
**		NICHEVO
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	May 1990 (rdesmond).
**		first created.
*/
static
VOID	
DDexit(status)
STATUS status;
{
    FEing_exit();
    PCexit(status);
}
