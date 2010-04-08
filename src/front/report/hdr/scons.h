/*
** Copyright (c) 2004 Ingres Corporation
*/
/*	static char	Sccsid[] = "@(#)scons.h	1.1	1/24/85";	*/

/*
**	Constants for the Report Specifier 
**
**	HISTORY:
**	6/19/89 (elein) garnet
**		added REPSPEC constants for Setup and Cleanup
**	7/26/89 (elein) re: ingres61bug change # 1770 by russ
**		Shortening the FN_RCOMMAND string. Otherwise it produces
**		a filename longer than 14 characters.
**	8/21/89 (elein ) garnet
**		Added REPSPEC constant for include command.
**	9/22/89 (elein) UG changes
**		ingresug change #90025 shorten FN_RCOMMAND
**	30-oct-89 (marian)
**		Remove '.' from 'rc.'. LOuniq() already puts
**		a '.' in.
**	05-apr-90 (sylviap)
**		Added S_PGWIDTH. (#129, #588)
**	12-oct-90 (sylviap)
**		Added BG_COMMENT and END_COMMENT. (#32781)
**	24-sep-92 (rdrane)
**		Added S_ANSI92 for 6.5 delimited identifiers and
**		owner.tablename.
**	25-nov-92 (rdrane)
**		Renamed S_ANSI92 to S_DELIMID for 6.5 delimited identifiers as
**		per the LRC.  Note that owner.tablename recognition is now
**		unconditionally enabled.
*/

/*
**	File names
*/

# define	FN_RCOMMAND	"rc"		/* file for RCOMMAND */

/*
**	States for the Text file reader 
*/

# define	IN_PRINT	1		/* in print command */
# define	IN_OTHER	2		/* in other command */
# define	IN_NOTHING	3		/* searching for new command */
# define	WANT_COMMAND	4		/* want a command name */
# define	IN_LIST		5		/* in FORMAT, TFORMAT, or 
						** POSITION command */
# define	IN_IF		6		/* in if or elseif command, expecting boolean expression */
# define	IN_IFTHEN	7		/* in if or elseif command, expecting .THEN */
# define	IN_LREMARK	8		/* in long remark command */
# define	IN_LET		9		/* in let assignment command */
# define	IN_LET_EQUALS	10		/* expecting [:]= */
# define	IN_LET_RIGHT	11		/* expecting expression on right
						** side of assignment */

/*
**	Codes for the REPSPEC Commands.
*/

# define	S_ERROR		-1		/* error code */
# define	S_HEADER	1		/* header command */
# define	S_FOOTER	2		/* footer command */
# define	S_NAME		3		/* name command */
# define	S_SORT		4		/* sort command */
# define	S_DATA		5		/* data command */
# define	S_QUERY		6		/* query command */
# define	S_OUTPUT	7		/* output command */
# define	S_DETAIL	8		/* detail command */
# define	S_SREMARK	9		/* short remark command */
# define	S_LREMARK	10		/* long remark command */
# define	S_ENDREMARK	11		/* end long remark */
# define	S_BREAK		12		/* break command */
# define	S_DECLARE	13		/* declare command */
# define	S_SQUERY	14		/* setup query command */
# define	S_CQUERY	15		/* cleanup query command */
# define	S_INCLUDE	16		/* include command */
# define	S_PGWIDTH	17		/* pagewidth command */
# define	S_DELIMID	18		/* eXpanded NameSpace command */

/*
**	Special codes for Report Specifier
*/

# define	F_DELIM		"\t"		/* delimiter used in COPY */
# define	MAXFLINE	310		/* maximum size of lines written
						** to RCOMMAND file */

/*
**	Types for query setup
*/

# define 	Q_QUEL		1		/* query is quel */
# define	Q_SQL		2		/* query is sql */

/*
**	Types for query parsing - using in s_g_skip
*/

# define 	BG_COMMENT	1		/* hit begin comment */
# define 	END_COMMENT	2		/* hit end comment */
