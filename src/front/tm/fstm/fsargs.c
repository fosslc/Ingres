/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<adf.h>
#include	<ermf.h>

/**
** Name:	fsargs.c -	Interactive Terminal Monitor
**					Argument Parsing Routine.
** Description:
**	Contains the routine used to parse the command line parameters
**	to the interactive terminal monitor.  Defines:
**
**	FSargs()	parse and return command-line arguments.
**
** History:
**	85/08  stevel
**		Initial revision.
**	09/29/86 (a.dea) -- Don't define WANT_SQL for CMS, where
**		iquel and isql are two separate links,
**		one of which is done with WANT_SQL defined
**		on the compile line.
**	88/05  wong
**		Rewrote to pass in ARG_DESC.
**	11-aug-88 (bruceb)
**		No longer CVlower the prompted database name.  A
**		database name on the command line was never altered,
**		and in any case, probably don't want to lower case
**		the name everywhere.
**	19-aug-88 (bruceb)
**		Accept '+' flags, passing them on down to the dbms.
**	12-dec-89 (teresal)
**		Recognize DBMS flag '-R' (Role) and prompt for Role Id
**		if required.
**	28-dec-89 (kenl)
**		Added handling of the +c (connection/with clause) flag.
**	27-jul-90 (kathryn) Bug 31932
**		Need to pass FEfmtflag formatting flags to the DBMS.
**	10-dec-1990 (kathryn)
**		Set globals passwd_flag (-P) and role_flag (-R), if either of
**		these flags is specified at startup time, to reflect its  
**		position in the argument list. (0 to maxargs-1).
**	04-feb-1991 (kathryn)
**		No longer call FEfmtflg() to process formatting flags.
**		LIBQ will handle these now.
**      03-oct-1996 (rodjo04)
**              Changed "ermf.h" to <ermf.h>
**	04-sep-1997 (dacri01) Bug #85196
**		Check to see if the database name provided on the command line
**	 	is an empty string .  If it is then ignore it. Continue 
**		processing for when the database name is not provided.  The 
**		shell in axp.osf passes in this empty string when no database 
**		name is provided.
**	10-sep-1997 (dacri01) Bug #85196
**              Previous fix caused compilation problem on other platforms.
**		Compare to NULL instead of '' empty char. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

GLOBALREF	int	passwd_flag, role_flag;

STATUS
FSargs (argc, argv, params, dbname, args, mxargs)
register i4  	argc;
register char	**argv;
ARG_DESC	params[];
char		**dbname;
char		*args[];
i4   		mxargs;
{
	register i4	j;
	char			*with_ptr;
	bool			prompt = TRUE;

	extern ADF_CB	*FS_adfscb;

	*dbname = NULL;

	for ( j = 0 ; j < mxargs ; ++j )
		args[j] = NULL;
	
	j = 0;
	while ( --argc > 0 )
	{
		if ( **++argv == '-' )
		{
			register ARG_DESC	*p;

			for ( p = params ; p->_name != NULL ; ++p )
			{
				i4	len = STlength(p->_name);

				if ( STbcompare(p->_name, len, *argv+1, len,
					FALSE) == 0 )
				{
					if ( p->_type == DB_BOO_TYPE )
					{
						if ( *(*argv + len + 1) != EOS )
							continue;
						*(bool *)p->_ref = TRUE;
					}
					else if ( p->_type == DB_CHR_TYPE )
					{
						if ( *(*argv + len + 1) == EOS )
						{
						    IIUGerr(E_UG0012_NoArgValue,
							0, 1, (PTR)*argv);
						    IIUGerr(E_UG0011_ArgSyntax,
							0, 2, ERx("command"),
							STalloc(ERget(E_MF2001_Syntax)));
						    return FAIL;
						}
						*(char **)p->_ref
							= *argv + len + 1;
					}
					break;
				}
			}
			if ( p->_name != NULL )
				continue;
			else if ( STequal(ERx("fsp"), *argv+1) )
				prompt = FALSE;
			else
			{ /* unrecognized parameter;
			  **	return in 'args' for DBMS
			  */
				if ( j + 1 >= mxargs )
				{
					IIUGerr( E_MF2002_TooManyArgs, 0, 1,
					    (PTR)*argv );
					return FAIL;
				}
				args[j++] = *argv;

				/* Prompt for role id if '-R' with no id */
				if ((*argv)[1] == 'R')
				{

				    if (prompt && ((*argv)[2] == EOS))
	    			    {
		    			char	rbuf[DB_MAXNAME+3];

			    		FEprompt(ERx("Enter Role Id:"), FALSE, 
						sizeof(rbuf) - 1, rbuf+2);
			    		if (rbuf[2] != EOS)
					{
					    rbuf[0] = '-';
					    rbuf[1] = 'R';
					    args[j-1] = STalloc(rbuf);
					}
				     }
				   role_flag = j-1;
				 }
				 else if ((*argv)[1] == 'P')
					passwd_flag = j-1;

			}
		}
		else if ( **argv == '+' )
		{
		    if (*(*argv + 1) == 'c' || *(*argv + 1) == 'C')
		    {
			/* set up WITH clause for CONNECT */
			with_ptr = STalloc(*argv + 2);
			IIUIswc_SetWithClause(with_ptr);
		    }
		    else
 		    {
		        /* Pass down such flags as +U and +Y to the dbms */
		        args[j++] = *argv;
		    }
		}
		else if ( *dbname != NULL )
		{
		    IIUGerr(E_MF2003_TooManyDBs, 0, 1, (PTR)*argv);
		    IIUGerr(E_UG0011_ArgSyntax, 0, 2, ERx("command"),
			STalloc(ERget(E_MF2001_Syntax)));
		    return FAIL;
		}
		else
		{
                    if (**argv != EOS)
		        *dbname = *argv;
		}
	} /* end while */
 
	if ( *dbname == NULL )
	{
	    if ( !prompt )
	    {
		    IIUGerr( E_MF2004_DBrequired, 0, 0 );
	    }
	    else
	    {
		    i4		i;
		    char	bufr[80];

		    for (i = 0 ; i < 3 ; ++i)
		    {
			    FEprompt(ERx("Database"), FALSE, sizeof(bufr) - 1,
				    bufr);
			    if (STtrmwhite(bufr) > 0 && bufr[0] != EOS)
			    {
				    *dbname = STalloc(bufr);
				    return OK;
			    }
		    }
		    IIUGerr(E_UG0010_NoPrompt, 0, 1, ERx("Database"));
	    }
	    IIUGerr(E_UG0011_ArgSyntax, 0, 2, ERx("command"),
		    STalloc(ERget(E_MF2001_Syntax)));
	    return FAIL;
	}

	return OK;
}
