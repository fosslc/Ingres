/*
**  getargs.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  get arguments for vifred
**
**  History
**
**	6/22/85 - Added support for titleless table fields. (dkh)
**	10/1/85 - Changed to default prompting flag. (peter)
**	06/05/87 (dkh) - Added -S flag for ABF.
**	08/14/87 (dkh) - ER changes.
**	09/11/87 (dkh) - Fixed to check return value of FEtest().
**	16-nov-87 (bab)
**		-q flag now additionally indicates scrolling field usage.
**	11/28/87 (dkh) - Added testing support for calling QBF.
**	18-may-88 (sylviap)
**		Changed the length of the database name from 32 to FE_MAXDBNM.
**	17-jun-88 (sylviap)
**		Took out CVlower.  Fdcats will lower case Vfformname if not
**		the -t flag (tables).  On DG, tables can be mixed-case.
**	05-apr-89 (bruceb)
**		Removed IIVFsflScrollFld(); no longer used.
**	09/21/89 (dkh) - Porting changes integration.
**	09/22/89 (dkh) - More porting changes.
**	14-dec-89 (bruceb)
**		Added code for role, password and group flags.
**	02-jan-90 (kenl)
**		Added support for the +c flag (WITH clause on DB connection).
**	02-may-90 (bruceb)
**		Removed support for the role flag.
**	14-sep-93 (dianeh)
**		Removed NO_OPTIM setting for obsolete Sequent ports (sqs_us5
**		and sqs_u42).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<si.h>
# include	<st.h>
# include	<er.h>
# include	"ervf.h"

FUNC_EXTERN	STATUS	FEtest();
static		VOID	vfbadflag(char *flag);
static		VOID	vftoomany();

vfgetargs(argc, argv)
i4	argc;
char	**argv;
{
	bool	gotdb = FALSE;
	bool	vfpromptflag = TRUE;
	char	reply[100];
	char    *with_ptr;
	i4	len;

	argc--;
	argv++;
	for(; argc > 0; argc--, argv++)
	{
		switch (**argv)
		{
			case '-':
				(*argv)++;
				switch (**argv)
				{
					case 'C':
						/*
						** the form is to be compiled
						** after the
						** call to vifred.
						*/
						(*argv)++;
						Vfcompile = *argv;
						break;

					case 'Z':
						(*argv)++;
						if (FEtest(*argv) != OK)
						{
							vfexit(-1);
						}
						IIVFzflg = TRUE;
						break;

					case 'e':
						vfnoload = TRUE;
						break;

					case 'f':
					case 'F':
						/*
						** start up with the name
						** of a form
						*/
						vfspec = **argv;
						break;

					case 'D':
					case 'd':
						/*
						** get the name of the
						** debug file
						*/
						(*argv)++;
						vfdname = *argv;
						break;

					case 'G':
						/*
						** get the group id
						*/
						(*argv)--;
						IIVFgidGroupID = *argv;
						break;

					case 'I':
						vfrname = (++(*argv));
						IIVFiflg = TRUE;
						break;

					case 'j':
						vfspec = 'j';
						break;

					case 'p':
						/*
						** prompt flag set
						*/
						vfpromptflag = TRUE;
						break;

					case 'P':
						/*
						** prompt for the password
						*/
						(*argv)--;
						IIVFpwPasswd = *argv;
						break;

					case 'Q':
					case 'q':
						iivfRTIspecial = TRUE;
						break;

					case 'S':
						Vfcfsym = (++(*argv));
						break;

					case 't':
						vfspec = 't';
						break;

					case 'u':
					case 'U':
						/*
						** get the user name
						*/
						(*argv)--;
						vfuname = *argv;
						vfuserflag = TRUE;
						break;

					case 'X':
						(*argv)--;
						Vfxpipe = *argv;
						break;

					default:
						vfbadflag(*argv);
						break;
				}
				break;

			case '+':
				(*argv)++;
				switch (**argv)
				{
					case 'c':  /* WITH clause for CONNECT */
					case 'C':
						with_ptr = STalloc(*argv + 1);
						IIUIswc_SetWithClause(with_ptr);
						break;

					case 'p':
						/*
						** prompt flag off
						*/
						vfpromptflag = FALSE;
						break;

					default:
						vfbadflag(*argv);
						break;
				}
				break;

			default:
				if (!gotdb)
				{
					STlcopy(*argv, dbname, FE_MAXDBNM);
					gotdb = TRUE;
				}
				else if (Vfformname == NULL)
				{
					Vfformname = *argv;
				}
				else
					vftoomany();
				break;
		}
	}
	if (!gotdb)
	{
		if (!vfpromptflag)
		{
			SIfprintf(stderr,ERget(S_VF006E_Missing_database_name));
			vfexit(-1);
		}
		FEprompt(ERget(S_VF006F_Database), TRUE, FE_MAXDBNM, dbname);
	}

	if (vfspec != '\0' && Vfformname == NULL)
	{
		switch (vfspec)
		{
		case 'F':
		case 'f':
			if (!vfpromptflag)
			{
				SIfprintf(stderr,
					ERget(S_VF0070_Missing_form_name_n));
				vfexit(-1);
			}
			FEprompt(ERget(S_VF0071_Form_name), TRUE, 100, reply);
			break;

		case 'j':
			if (!vfpromptflag)
			{
				SIfprintf(stderr,
					ERget(S_VF0072_Missing_joindef_name_));
				vfexit(-1);
			}
			FEprompt(ERget(S_VF0073_Joindef_name), TRUE,100, reply);
			break;

		case 't':
			if (!vfpromptflag)
			{
				SIfprintf(stderr,
					ERget(S_VF0074_Missing_table_name_n));
				vfexit(-1);
			}
			FEprompt(ERget(S_VF0075_Table_name), TRUE, 100, reply);
			break;
		}
		Vfformname = STalloc(reply);
	}
}

/*
** a bad flag
*/
static VOID
vfbadflag(flag)
char	*flag;
{
	SIfprintf(stderr, ERget(S_VF0076_Bad_flag___s__in_comm), flag);
	SIfprintf(stderr, ERget(S_VF010C_Usage));
	SIflush(stderr);
	vfexit(-1);
}

static VOID
vftoomany()
{
	SIfprintf(stderr, ERget(S_VF0077_Too_many_arguments_in));
	SIflush(stderr);
	vfexit(-1);
}
