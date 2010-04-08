/*
**	Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<lo.h>
# include	<st.h>
# include	<si.h>
# include	<pc.h>

/*
PROGRAM = yycase
**
NEEDLIBS = COMPATLIB
*/

/*
+* Filename:	yycase.c
** Purpose:	Hack on output of YACC to produce compilable code for case
-*		statements with the compiler boundaries.
** Usage:
**		yycase input output
** Example:
**		yycase y.tab.c esqlc.c
**
** History:
**		20-sep-1985	- Written for UNIX compiler (ncg)
**		03-jul-1989	- Enhanced to handle any number of case
**				  labels. (bjb)
**		07-aug-1989	- Changed again.  Previous solution of
**				  breaking up case statement was not sufficient
**				  after grammar had grown by a few more
**				  hundred lines.  Now we break routines
**				  every hundred case statements; break
**				  case statements in between. (barbara)
**		30-oct-1990	- Added ming hint so this will build properly
**				  under LIRE tools.  (sandyd)
**		13-oct-93 (brett)
**			On WNT and MSW always break the switch into
**			functions.  Add NEEDLIBS.  Use a back filled
**			buffer to pass through the function type of yyparse.
**	16-mar-94 (brett)
**		Use code_seg pragma instead of the alloc_text pragma.
**	16-mar-94 (brett)
**		Change yy__case%d function and yym to short, this matches
**		the grammar files.
**	17-mar-94 (brett)
**		Remove declaration for yy_case%d and let it default to int,
**		yym should remain short, if it were changed to int and
**		another daclaration line was added to iyacc.par, yycase
**		would need to account for the extra line.
**	03-Sep-2009 (frima01) 122490
**		Add include of pc.h.
**
** Notes:
**  1. This file is very dependent on the output of YACC.
**     We make the following assumptions:
** 	1.1.  "yyparse" begins on a line by itself, and 4 lines down is the
**	      declaration of "yypvt".  We make "yypvt" global, and delete its
**	      local declaration.
**	      NOTE: Since enhancement of 03-jul-1989, we no longer search
**	      for yyparse because the case statements are left in one
**	      routine after being broken up.
**	1.2.  "case" statements are one tab from left, and there are less than
**	      1000 distinct labels.
**	      NOTE:  The number of labels is limitless since enhancement
**	      of 03-jul-1989.
**	1.3.  The very executable (closing) statement in the file is spaces
**	      followed by "goto yystack;".
**	      NOTE: The last statement is not searched for after enhancement
**	      of 03-jul-1989.
**     If the above format changes the strings being looked for can be chosen 
**     based on an input argument.
**  2. Generates (before 03-jul-1989):
**	2.1.  Slightly modifies the "yyparse" line.
**	2.2.  At case 50?:
**
**		default:
**			if (yy__case(yym)) YYERROR;
**			break;
**		} 		- close switch -
**		goto yystack;	- just like regular yacc -
**	}			- close function -
**
**	yy__case( yym )
**	int yym;
**	{
**	# define YYERROR return 1	- error case -
**		switch (yym) {
**			- code from input file till end -
**		}	- closing brace from input file -
**		return 0;		- success -
**	}
**  3. Generates (since 03-jul-1989 and until 07-aug-1989)
**	3.1.  At case 50?, 100?, 150?, 200?, etc.
**		
**		default:
**		  goto label 50? {100?|150?|200?|...}	
**		}
**
**	    label_500:
**	      switch (yym) {		
**
**  3. Generates (since 07-aug-1989) a combination of the above two
** 	solutions:
**	3.1.  At case 50?, 150?, 250?, etc.
**		Same as 3 above.
**
**	3.2   At case 100?, 200?, etc.
**		(More or less) same as 2.2 above.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define	YY_INIT		0
# define	YY_IN_SUB	1


main( argc, argv )
i4	argc;
char	*argv[];
{
    char	buf[MAX_LOC+1];
    LOCATION	loc;
    FILE	*in_f;
    FILE	*out_f;
    i4		i;
    i4		return_flag;
    i4		state_flag = YY_INIT;
    char	case_buf[80];
    i4		case_size;
    i4		case_level;
    char	buf1[2][MAX_LOC+1];
    i4		toggle;
    i4		first_time;

    if (argc < 3)
    {
	SIprintf( "YYCASE Usage is: yycase infilename outfilename\n" );
	SIprintf( "Example:         yycase y.tab.c esqlc.c\n" );
	PCexit( -1 );
    }
    STcopy( argv[1], buf );
    LOfroms( FILENAME, buf, &loc );
    if (SIopen(&loc, "r", &in_f) != OK)
    {
	SIprintf( "Cannot read '%s'\n", buf );
	PCexit( -1 );
    }
    STcopy( argv[2], buf );
    LOfroms( FILENAME, buf, &loc );
    if (SIopen(&loc, "w", &out_f) != OK)
    {
	SIprintf( "Cannot write to '%s'\n", buf );
	PCexit( -1 );
    }
    /* Back up position of yypvt variable so it's global */
    toggle = 0;
    first_time = 1;
    while (SIgetrec(buf1[toggle], MAX_LOC, in_f) == OK)
    {
	if (STbcompare(buf1[toggle], 7, "yyparse", 0, FALSE) == 0)
	{
	    SIfprintf( out_f,
		"YYSTYPE *yypvt; /* Moved to global space from yyparse*/\n\n");
	    /* Emit yyparse() and next three lines, but omit yypvt decl */
	    toggle = toggle == 0;
	    SIputrec( buf1[toggle], out_f );
	    toggle = toggle == 0;
	    for (i = 0; i < 4; i++)
	    {
		SIputrec( buf1[toggle], out_f );
		SIgetrec( buf1[toggle], MAX_LOC, in_f );
	    }
	    break;
	}
	else
	{
	    toggle = toggle == 0;
	    if (first_time)
	    	first_time = 0;
	    else
		SIputrec( buf1[toggle], out_f );
	}
    }
    STcopy( "\tcase 5", case_buf );
    case_level = 5;
    case_size = 7;
    return_flag = 0;
    while (SIgetrec(buf, MAX_LOC, in_f) == OK)
    {
	if (STbcompare(buf, case_size, case_buf, 0, FALSE) == 0 && 
		buf[case_size] != ':' && buf[case_size +1] != ':')
	{
	    SIfprintf( out_f, "\tdefault:\n" );
#if !defined(wgl_wnt) && !defined(wgl_win)
	    if (case_level%10 == 0)
#endif /* !wgl_wnt && !wgl_win */
	    {
		STprintf( case_buf, "\t\t    if (yy__case%d(yym)) YYERROR;\n",
			case_level);
		SIfprintf( out_f, case_buf );
		SIfprintf( out_f, "\t\t    break;\n" );
		SIfprintf( out_f, "\t}\n" );	/* Close switch */
		if (state_flag == YY_INIT)
		{
		    SIfprintf( out_f,
			"\tgoto yystack; /* stack new stuff */\n" );
		    SIfprintf( out_f, "}\n\n\n" ); 	/* Close yyparse */
		}
		else /* YY_IN_SUB */
		{
		    SIfprintf( out_f, "\treturn 0; /* Success */\n}\n\n" );
		}
#if defined(wgl_wnt) || defined(wgl_win)
		SIfprintf( out_f, "#pragma code_seg(\"CASE%d_CODE\")\n\n", case_level);
#endif /* wgl_wnt || wgl_win */
		SIfprintf( out_f, "yy__case%d(yym)\n", case_level);
		SIfprintf( out_f, "short yym;\n{\n" );
		SIfprintf( out_f, "#ifdef YYERROR\n" );
		SIfprintf( out_f, "#undef YYERROR\n" );
		SIfprintf( out_f, "#endif /* YYERROR */\n" );
		SIfprintf( out_f, "#define YYERROR return 1\n\n" );
		state_flag = YY_IN_SUB;
	    }	
#if !defined(wgl_wnt) && !defined(wgl_win)
	    else
	    {
		SIfprintf( out_f, "\t\t    goto label_%d00;\n", case_level );
		SIfprintf( out_f, "\t}\n\n" );	/* Close switch */
		SIfprintf( out_f, "label_%d00:\n", case_level );
	    }
#endif /* !wgl_wnt && !wgl_win */
	    SIfprintf( out_f, "\tswitch (yym) {\n" );
	    SIputrec( buf, out_f );
	    case_level += 5;
	    STprintf( case_buf, "\tcase %d", case_level );
	    case_size = STlength(case_buf);
	}
	else if (  (   STbcompare(buf, 15, "\t\tgoto yystack;", 0, FALSE) == 0
		    || STbcompare(buf, 17, "    goto yystack;", 0, FALSE) == 0)
		 && state_flag == YY_IN_SUB) 
	{
	    SIfprintf( out_f, "\treturn 0; /* Success */\n" );
	}
	else
	{
	    SIputrec( buf, out_f );
	    return_flag = 1;
	}
    }
    SIclose( in_f );
    SIclose( out_f );
    PCexit( (return_flag == 1) ? OK : FAIL );
}
