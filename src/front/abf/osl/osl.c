/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<pc.h>		/* 6-x_PC_80x86 */
#include	<ex.h> 
#include	<lo.h>
#include	<nm.h>
#include	<si.h>
#include	<er.h>
#include	<cv.h>
#include	<iicommon.h>
#include	<oslconf.h>
#include	<osfiles.h>
#include	<oserr.h>
#include	<osglobs.h>
#include	<osmem.h>
#include	<gl.h>
#include	<fe.h>

/**
** Name:	osl.c -	OSL Translator Main Line.
**
** Description:
**	Contains the OSL translator ("osl*") main line routine and exit routine.
**	Defines:
**
**	osl()		main line of "osl*".
**	osexit()	OSL translator exit.
**
** History:
**	Revision 3.0
**	Written (jrc)
**
**	Revision 5.1  86/11/04  15:07:35  wong
**	Modified to receive configuration routines (parser, etc.) as parameters.
**		
**	Revision 6.0  87/06  wong
**	Clean-up as part of 6.0.
**
**	Revision 6.2  89/01  billc
**	Added support for generating C code.
**	89/03  wong  Added support for error listing file.
**
**	Revision 6.3/03/00
**	11/90 (Mike S)
**		In oshandler, output OSBUG if we get an unrecognized exception.
**
**	Revision 6.4
**	03/13/91 (emerson)
**		Added new logic to store the value specifed
**		by the logical symbol II_4GL_DECIMAL into osDecimal.
**		(This will now take the place of II_DECIMAL in 
**		the 4GL compiler).
**	08/17/91 (emerson)
**		Added missing #include <nm.h>.
**	08/17/91 (emerson)
**		Removed inadvertently duplicated #include <ex.h>.
**
**	Revision 6.5
**	16-jun-92 (davel)
**		added support for II_NUMERIC_LITERAL - set global bool 
**		osFloatLiteral to TRUE if the logical is set to "FLOAT".
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) 121804
**	    Need iicommon.h to satisfy gcc 4.3.
*/

/*{
** Name:    osl() -	Main Line of "osl*".
**
** Description:
**	The main line of the OSL translators.
**
** Input:
**	parser	{nat (*)()}  Parser to use.
**	argc	{nat}  Number of arguments.
**	argv	{char **}  Argument vector.
**
** Called by:
**	main()
*/

GLOBALREF char  osDecimal;
GLOBALREF char  osFloatLiteral;

static    EX	oshandler();
EX	FEhandler();

STATUS
osl (parser, argc, argv)
i4	(*parser)();
i4	argc;
char	**argv;
{
    STATUS	rs;
    EX_CONTEXT	context;

    if (EXdeclare(oshandler, &context) != OK)
    {
	EXdelete();
	return FAIL;
    }

    osIGframe = NULL;

    osgetargs(argc, argv);

    if ( *osXflag == EOS && ( osDatabase == NULL || *osDatabase == EOS ) )
	osuerr(OSNODB, 0);

    if (FEingres(osDatabase, osXflag, osUflag, (char *)NULL) != OK)
	return FAIL;

    /*
    ** If caller did not specify an input file but did specify an output file,
    ** then just skip the following 4GL translation and call the code generator
    ** directly; the IL will be read from the DBMS.  Otherwise, assume test mode
    ** and read the 4GL source from the standard input.
    */
    if ( osIfilname != NULL || osOfilname == NULL )
    { /* 4GL compilation */

	char	*cp;

	if ( osIfilname == NULL )
	{
		osIfile = stdin;
	}
	else
	{ 
		/* Open Input File */
		LOCATION	loc;
		char		lbuf[MAX_LOC+1];

		osIfile = NULL;
		STlcopy(osIfilname, lbuf, sizeof(lbuf));
		if ( LOfroms(PATH&FILENAME, lbuf, &loc) != OK  
		  || SIopen(&loc, ERx("r"), &osIfile) != OK )
		{
 			osuerr(OSNOINP, 1, osIfilname);
		}
	}

	/* Determine the decimal point character for 4GL source input */
	osDecimal = '.';
	NMgtAt(ERx("II_4GL_DECIMAL"), &cp);
    	if ( cp != NULL && *cp != EOS )
    	{
	    if (*(cp+1) != EOS || (*cp != '.' && *cp != ','))
	    {
		IIUGerr(E_OS0050_BadDecimal, 0, 1, (PTR)cp);
		/* default is `.' */
	    }
	    else
	    {
	    	osDecimal = cp[0];
	    }
    	}

	/* set float literal backward-compat indicator */
	osFloatLiteral = FALSE;
	NMgtAt(ERx("II_NUMERIC_LITERAL"), &cp);
    	if ( cp != NULL && *cp != EOS )
    	{
	    CVlower(cp);
	    osFloatLiteral = (bool) STequal(cp, ERx("float"));
    	}

	osadf_start(osldml);
	osMem();
	(*parser)();

	if ( osIfile != stdin )
		SIclose(osIfile);

	if ( osOfile != NULL )
		SIclose(osOfile);	/* debug IL output */

	if ( osLisfile != NULL )
	{ /* Close error listing file */
		SIclose(osLisfile);
		if ( osErrcnt == 0 && osWarncnt == 0 && osLfilname != NULL )
		{ 
			/* Delete error listing file */
			LOCATION	errloc;
			char		elbuf[MAX_LOC+1];

			STlcopy(osLfilname, elbuf, sizeof(elbuf));
			if ( LOfroms(PATH&FILENAME, elbuf, &errloc) == OK )
			{
				LOpurge(&errloc, 0);
			}
		}
	}
    }

    /* if caller specified output file, call code generator to produce it. */
    if ( osErrcnt == 0 && osFid > 0 && osOfilname != NULL )
    {
	STATUS	IICGcgCodeGen();

        if ( IICGcgCodeGen(osFrm, osForm, osSymbol, 
			osOfilname, osFid, osIGframe, osStatic
		) != OK )
	{
		++osErrcnt;
	}
    }

    FEing_exit();

    if ( osErrcnt == 0 )
    {
 	SIfprintf(osErrfile, ERget(_NoErrors));
	rs = OK;
    }
    else
    {
 	SIfprintf(osErrfile, ERget(osErrcnt > 1 ? _Errors : _1Error), osErrcnt);
 	rs = FAIL;
    }

    EXdelete();
    return rs;
}

/*
** Name:	oshandler()	- OSL exception handler.
*/

static bool Called = FALSE;

static EX
oshandler(ex)
EX_ARGS	*ex;
{
	char		*exstr;
	i4		extype = ex->exarg_num;
	ER_MSGID	eid = (ER_MSGID) 0;
	char		*unknown = ERx("<unknown>");
	if (extype == EXFEMEM)
		eid = OSNOMEM;
	else if (extype == EXFEBUG)
		eid = OSBUG;

	if (Called || eid == (ER_MSGID) 0)
	{
		/* 
		** Either we don't know how to print this message, or an
		** exception re-raised while we were trying to handle it.
		** So, give up trying and let the REAL handler do it.
		*/
		FEhandler(ex);
	}
	Called = TRUE;

	/* EXFEMEM could have 0 or 1 args. */
	if (eid == (ER_MSGID) 0)
		osuerr( OSBUG, 1, unknown);
	else if (ex->exarg_count == 0)
		osuerr( eid, 1, unknown);
	else
		osuerr( eid, 1, ex->exarg_array[0]);
	/* NOTREACHED */
}

/*{
** Name:	osexit() -	OSL Parser Exit Routine.
**
** Description:
**	Exit from OSL compiler.
**
** Input:
**	val	{nat}  Exit value.
*/
VOID
osexit (val)
i4	val;
{
	if ( osLisfile != NULL )
	{ 
		/* Close error listing file */
		SIclose(osLisfile);
		if ( val == OK 
		   && osErrcnt == 0 
		   && osWarncnt == 0 
		   && osLfilname != NULL)
		{ 
			/* Delete error listing file */
			LOCATION	loc;
			char		lbuf[MAX_LOC+1];

			STlcopy(osLfilname, lbuf, sizeof(lbuf));
			if (LOfroms(PATH&FILENAME, lbuf, &loc) == OK )
			{
				LOpurge(&loc, 0);
			}
		}
	}
	FEexits((char *)NULL);
	PCexit(val);
}
