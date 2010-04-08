/*
** Copyright (c) 1990, 2008 Ingres Corporation
**		All rights reserved.
*/
# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<cv.h>
# include	<er.h>
# include	<ex.h>
# include	<lo.h>
# include	<ug.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<ui.h>
# include	<uigdata.h>
# include	<dictutil.h>
# include	"erdu.h"


/**
** Name:	dctinstl.c
**
** Description:
**	Utility to install Ingres Front-End Dictionary
**	(Historical note: upgradefe was formerly known as "dictinstall")
**
**	The command line syntax is:
**
**		upgradefe database [-uuser] [-b] [-c] [-s] [-vversion]
**				client {client}
**
**	The UTEXE definition is:
**
**  upgradefe upgradefe PROCESS - upgradefe database [-uuser] [-b] [-c]
**	[-s] [-vversion] client {client}
**		database        '%S'    1       Database?               %S
**		user            '-u%S'  0       ?                       -u%-
**		basecats        "-b"    0       ?                       -b
**		noupgrade       "-c"    0       ?                       -c
**		silent	        "-s"    0
** 		client          '%S'    0       ?                       %100S
**		version         "-v%N"  0       ?                       -v%N
** 		sesdbg          "-R%S"  0       ?                       -R%-
** 		client          "%S"    0       ?                       %100S
**		connect		+c%S
**
** History:
** 	24-apr-1990 (rdesmond)
**		First Written.
** 	04-jun-1990 (rdesmond)
**		Now doesn't display 'Connecting to DB ...' message.
**	12-jun-1990 (pete)
**              Also set username to "$ingres" if not specified by the user.
**              Also, remove code which
**              checked after the connect, and only allowed the DBA to
**              run this program. Also, make the connect an exclusive one (-l).
**              Also add new error message E_DD001B_NO_DB_PRIV if user
**              doesn't have permission to use the "-u" flag; Also, make
**              sure a NULL argument isn't passed to FEningres before last arg.
**	13-jul-1990 (pete)
**		Add support for "silent" flag. This will be passed by createdb
**		when the user doesn't specify client names.
**	20-sep-1990 (pete)
**		change dictinstall to upgradefe.
**	03-oct-1990 (pete)
**		Prompt for client name if none specified on command line.
**	12-oct-1990 (sandyd)
**		Removed obsolete FEtesting undef.  The sun4 loader ignored the
**		fact that this symbol was never resolved, but on Sequent it
**		was generating an error.
**	18-oct-1990 (sandyd)
**		Fixed #include of local erdu.h to use "" instead of <>.
**	15-jan-1990 (pete)
**		Add check after FEningres to make sure not being run against
**		the CDB. Upgradefe must be run against Star -- not against CDB.
**	20-jun-1991 (pete)
**		Add FUNC_EXTERN at Saber C's suggestion.
**      18-jul-1991 (pete)
**		Added support for +c flag; WITH clause for Gateways. Bug 38706.
**	29-jul-1991 (pete)
**		Remove -u$ingres from default connect statement. Following
**		connect, make sure dbmsinfo('username') = dbmsinfo('dba').
**		If not, disconnect with error; added error E_DD0027_NOT_DBA.
**		Also, remove -l from connect statement (not supported on Star
**		or Gateways). Also, factored out common Connect error-check
**		code between modifyfe and upgradefe and created routine
**		IIDDcseCheckStartupErrs(). SIR 38903.
**	31-jul-1991 (pete)
**		Following successful upgrade, install all other clients
**		that are compatible with installed dictionary modules.
**		Bug 38984.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	07-jul-1995 (lawst01)
**		Put in ifdef NT_GENERIC for variable name 'connect' - for
**		NT platform will use variable name 'connect_w'.
**      17-aug-1995 (last01) #70547
**          If a user requested product (-f) option cannot be found,
**          the program will issue an error and return without any 
**          updates
**      24-sep-96 (hanch04)
**              Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	23-Aug-2009 (kschendel) 121804
**	    Fix IIDDcseCheckStartupErrs return confusion.
*/

/*
**	MKMFIN Hints
**
PROGRAM =	upgradefe

NEEDLIBS =	DICTUTILLIB \
		SHQLIB \
		COMPATLIB \
		SHEMBEDLIB
		
UNDEFS = II_copyright
*/

FUNC_EXTERN STATUS 	installallmodules();
FUNC_EXTERN bool	IIDDicIsCDB();
FUNC_EXTERN STATUS	IIDDiccInstallCompatibleClients();
FUNC_EXTERN bool	IIDDcseCheckStartupErrs();

GLOBALREF	bool	IIDDsilent;

static VOID	DDexit();
static STATUS	add_client();

static char	*dbname;
static char	*uname;
static bool	basecats;
static bool	noupgrade;
static char	*xflag;
 
#ifdef NT_GENERIC
static char     *connect_w;
#else
static char	*connect;
#endif
 
static i4	version;
static const char _Pgm[]     = ERx("upgradefe");
static const char _Empty[]   = ERx("");

  static ARG_DESC args[] =
{
    /* Required arguments */
    {ERx("database"),	 DB_CHR_TYPE,	FARG_PROMPT,	(PTR) &dbname},

    /* Optional arguments */
    {ERx("user"),	 DB_CHR_TYPE,	FARG_FAIL,	(PTR) &uname},
    {ERx("basecats"),    DB_BOO_TYPE,	FARG_FAIL,	(PTR) &basecats},
    {ERx("silent"),      DB_BOO_TYPE,	FARG_FAIL,	(PTR) &IIDDsilent},
    {ERx("noupgrade"),   DB_BOO_TYPE,	FARG_FAIL,	(PTR) &noupgrade},
    {ERx("version"),   	 DB_INT_TYPE,	FARG_FAIL,	(PTR) &version},
#ifdef NT_GENERIC
    {ERx("connect"),     DB_CHR_TYPE,   FARG_FAIL,      (PTR) &connect_w},
#else
    {ERx("connect"), 	 DB_CHR_TYPE,	FARG_FAIL,	(PTR) &connect},
#endif

    /* Internal Optional arguments */
    {ERx("equel"),	 DB_CHR_TYPE,	FARG_FAIL,	(PTR) &xflag},
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
    bool	goodclients;
    i4		maxcoreversion;

    /* Capability checks should go here. */

    /* assign default values for command line arguments */
    dbname = NULL;
    uname = _Empty;
    basecats = FALSE;
    IIDDsilent = FALSE;
    version = 0;
    noupgrade = FALSE;
    xflag = _Empty;
#ifdef NT_GENERIC
    connect_w = _Empty;
#else
    connect = _Empty;
#endif

    if ((rval = IIUGuapParse(argc, argv, _Pgm, args)) != OK)
	PCexit(rval);

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

    rval = FEningres((char *)NULL, 0, dbname, uname, xflag, (char*)NULL);


    if (! IIDDcseCheckStartupErrs(rval, dbname, _Pgm, F_DD0004_UPGRADE))
	PCexit(FAIL);   /* issues own error messages */

    /* Make sure user isn't trying to run on a Star CDB. That breaks Star. */
    if (IIDDicIsCDB() == TRUE)
    {
	IIUGerr(E_DD0023_Cant_Run_On_Star_CDB, UG_ERR_FATAL,
		2, (PTR) dbname, (PTR) _Pgm);
        /*NOTREACHED*/
    }

    if ((rval = init_moduletable(&maxcoreversion)) != OK)
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
	DDexit(FAIL);

    /* place all required clients into the clienttable */
    goodclients = FALSE;
    for (numobj = 0;
      (FEutaget(ERx("client"), numobj, FARG_FAIL, &rarg, &pos) == OK);
      numobj++)
    {
	if ((rval=add_client(rarg.dat.name, version)) == OK)
	{
	    goodclients = TRUE;
	}
	else
        if (rval == UI_NO_CLIENT)
	{
	    FEutaclose();
	    DDexit(OK);
        }
        else
	{
	    FEutaclose();
	    DDexit(FAIL);
	}
    }
    FEutaclose();

    if ((numobj == 0) && (!basecats))
    {
	/* no clients found above and -b not specified; prompt for client name*/

	char client[FE_PROMPTSIZE + 1];

	client[0] = EOS;
	FEprompt(ERget(S_DD0021_NO_CLIENTS_PROMPT), FALSE,
			FE_PROMPTSIZE, client);
	if (client[0] == EOS)
	{
	    IIUGerr(E_DD001C_NO_CLIENTS_SPEC, UG_ERR_ERROR, 0);
	    FEutaclose();
	    DDexit(FAIL);
	}
	else
	{
	    if (add_client(client, 0) == OK)
		goodclients = TRUE;
	    else
		DDexit(FAIL);
	}
    }

    /* if -b flag is specified then create most recent version of
    ** INGRES base catalogs.
    */
    if (basecats)
    {
	if (add_client(ERx("INGRES"), 0) == OK)
	    goodclients = TRUE;
	else
	    DDexit(FAIL);
    }

    if (!goodclients)
    {
	DDexit(FAIL);
    }

    rval = installallmodules(noupgrade, maxcoreversion);

    if (rval == OK)
    	rval = IIDDiccInstallCompatibleClients();

    DDexit(rval);
}


/*{
** Name:        add_client - request a client to be supported.
**
** Description:
**	Wrapper routine for add_client, which gives error messages
**	if problems occur.
**
** Inputs:
**      name     - name of client which needs to be supported.
**      version  - version number of the client, 0 for highest known.
**
** Outputs:
**
**      Returns:
**		STATUS  -	OK:     client added.
**				FAIL:   client not added.
** History:
**	3-oct-1990 (pete)
**		Initial version.
*/
static STATUS
add_client (client, version)
char	*client;
i4	version;
{
	STATUS rval;

        rval = enter_client(client, version); /*returns OK, FAIL, UI_NO_CLIENT*/

        if (rval == UI_NO_CLIENT)
        {
	    /* give warning message: specified client does not exist */

            if (!IIDDsilent)
            {
                if (version)
                    /* user specified version (-v) on command line */
                    IIUGerr(E_DD001E_NO_CLIENT_VERS, UG_ERR_ERROR,
                            2, (PTR)client, (PTR) &version);
            	else
                {
                    IIUGerr(E_DD0007_NO_CLIENT, UG_ERR_ERROR, 1, (PTR)client);
                    rval = UI_NO_CLIENT;
                    return (rval);
                }
            }
            else
            if ( ! version )
                {
                    IIUGerr(E_DD0007_NO_CLIENT, UG_ERR_ERROR, 1, (PTR)client);
                    rval = UI_NO_CLIENT;
                    return (rval);
                }
	    rval = OK;
        }

	return (rval);
}

static
VOID	
DDexit(status)
STATUS status;
{
    FEing_exit();
    PCexit(status);
}
