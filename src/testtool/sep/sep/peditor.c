/*
** peditor.c - SEP's "pseudo-editor".
**
** History:
**	##-XXX-1989 (eduardo)
**		Created.
**	30-aug-1989 (mca)
**		Changed MING hint - we want to use SEPWINLIB rather than
**		UGLIB/FTLIB.
**	24-may-1990 (rog)
**		Because of too much duplication of function key code,
**		we need to use FTLIB/UGLIB rather than SEPWINLIB.
**	21-feb-1991 (gillnh2o)
**		Per jonb, added LIBINGRES to NEEDLIBS and remove everything
**		else that isn't built out of SEP. This may obviate the need
**		to change NEEDLIBS for every new build.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified
**	    and clarified some control loops.
**	05-jul-1992 (donj)
**	    Rewrote how peditor used the ctrlW char to refresh the screen when
**	    peditor starts and finishes.
**	30-nov-1992 (donj)
**	    Added SEP_LOexists.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	 5-may-1993 (donj)
**	    Add function prototypes, init some pointers.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**	 7-may-1993 (donj)
**	    REsolved a conflict with reporting().
**       2-jun-1993 (donj)
**          Isolated Function definitions in a new header file, fndefs.h. Added
**          definition for module, and include for fndefs.h.
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**	02-dec-1993 (donj)
**	    Take out underscores in filenames for hp9_mpe.
**	24-may-1994 (donj)
**	    Clean up some casting and definition complaints that QA_C and c89
**	    had.
**	25-Jan-1996 (somsa01)
**	    Added ifndef NT_GENERIC for "^W" direct screen writes.
**      05-Feb-96 (fanra01)
**          Added conditional for using variables when linking with DLLs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Nov-2006 (kiria01) b117042
**	    Conform CM macro calls to relaxed bool form
*/

/*
**    Include files
*/
#include <compat.h>
#include <si.h>
#include <st.h>
#include <lo.h>
#include <nm.h>
#include <er.h>
#include <cm.h>
#include <te.h>
#include <tc.h>
#include <kst.h>
#include <termdr.h>

#define peditor_c

#include <fndefs.h>

#define CONNECT_Y   (char *)ERx("Connecting to SEP module")
#define CONNECT_N   (char *)ERx("Could not connect to SEP module")
#define FILE_QST    (char *)ERx("Enter name of file to be included: ")
#define SUCCESS	    (char *)ERx("source file was copied into target file")
#define FAILURE_1   (char *)ERx("source file could not be copied into target file")
#define FAILURE_2   (char *)ERx("source file was not found")

#define S_REFRESH   '\027'

#ifdef main
#undef main
#endif

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF    i4            lx ;
GLOBALDLLREF    i4            ly ;
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF    i4            lx ;
GLOBALREF    i4            ly ;
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALDEF    i4            tracing = 0 ;
GLOBALDEF    FILE         *traceptr ;
FUNC_EXTERN  KEYSTRUCT    *TDgetch             ( WINDOW *awin ) ;
             VOID          reporting (char *message,TCFILE *commfile) ;
/*
**      ming hints
**

NEEDLIBS = SEPCLLIB LIBINGRES 

**
*/

main(i4 argc,char *argv[])
{
    WINDOW                *mainW = NULL ;
    TCFILE                *outComm = NULL ;
    char                  *param = NULL ;
    char                   buffer_1 [80] ;
    char                   buffer_2 [80] ;
    FILE                  *newPtr = NULL ;
    FILE                  *oldPtr = NULL ;
    LOCATION               newLoc ;
    LOCATION               oldLoc ;
    char                  *frsflags = NULL ;
    char                  *flagcp = NULL ;
    char                   flagbuf [80] ;   /* II_FRSFLAGS value */
    KEYSTRUCT             *key = NULL ;
    static char           *cptr1 = NULL ;
    static char           *cptr2 = NULL ;
    static char           *cptr3 = NULL ;
    char		  *cgp = NULL ;
 
    /* get name of file to be created */

    if ((argc < 2) || ((param = argv[1]) == NULL) || (*param == '\0'))
	PCexit(0);
    
    /* open standard files */

    if (SIeqinit() != OK)
	PCexit(0);

    /* read II_FRSFLAGS logical */

    NMgtAt(ERx("II_FRSFLAGS"),&frsflags);

    if (frsflags == NULL)
	PCexit(0);

    STcopy(frsflags, flagbuf);

    for (flagcp = flagbuf;;)
    {
	if (*flagcp == '\0')
	    PCexit(0);

        if ((flagcp = STindex(flagcp, ERx("-"), 0)) == NULL)
	    PCexit(0);

        CMnext(flagcp);

	if (*flagcp == '*')
	{
	    flagcp++;
	    if (tc_connection(flagcp,&outComm) == OK)
		break;
	}
    }

    /* open output file */

    LOfroms(FILENAME & PATH, param, &newLoc);
    if (SIopen(&newLoc,ERx("w"),&newPtr) != OK)
    {
	PCexit(0);
    }
    TDinitscr();
    mainW = TDnewwin(LINES,COLS,0,0);

    /* connect to SEP module */

    TDmvwprintw(mainW,0,0,ERx("%s"),CONNECT_Y);
    TDrefresh(mainW);
    key = TDgetch(mainW); /* read TC_EOQ sent by SEP */

    if (key->ks_ch[0] == TC_EOQ)
    {
	reporting(CONNECT_Y,outComm);
	TCputc('\n',outComm);
	TCputc(TC_EQR,outComm);
	TCflush(outComm);
    }
    else
    {
	TDmvwprintw(mainW,0,0,ERx("%s"),CONNECT_N);
	TDrefresh(mainW);
 	reporting(CONNECT_N,outComm);
	TCputc('\n',outComm);
	SIclose(newPtr);
	TCclose(outComm);
	TDendwin();
	TEclose();
	PCexit(0);
    }

    for (;;)
    {
	TDerase(mainW);
	TDmvwprintw(mainW,0,0,ERx("%s"),FILE_QST);
	TDrefresh(mainW);
	getstr(mainW,buffer_1,TRUE);
	cptr1 = buffer_1;

	for (cptr1 = buffer_1;
	     (*cptr1 != EOS && !CMnmchar(cptr1)); CMnext(cptr1))
	{
	    if (*cptr1 == S_REFRESH)
	    {

		/* SEP sends a ^W each time it wants to redraw the screen */

#ifndef NT_GENERIC
		cgp = TDtgoto(CM, 0, 23);	/* delete bottom of screen */
		TEwrite(cgp,STlength(cgp));
		TEwrite(CE,STlength(CE));
		cgp = TDtgoto(CM, lx+1, ly);
		TEwrite(cgp,STlength(cgp));
		TEflush();
#endif
	    }
	}
	if (*cptr1 == EOS)
	    continue;

	reporting(FILE_QST,outComm);
	TCputc('\n',outComm);
	TDmvwprintw(mainW,2,0,ERx("Is %s OK ? (y/n) "),cptr1);
	TDrefresh(mainW);
	getstr(mainW,buffer_2,TRUE);

	if (CMcmpnocase(buffer_2,ERx("Y")) != 0)
	    continue;

	LOfroms(FILENAME & PATH, cptr1, &oldLoc);
	if (SEP_LOexists(&oldLoc))
	{
	    if (SIcat(&oldLoc,newPtr) == OK)
	    {
		TDerase(mainW);
		TDmvwprintw(mainW,0,0,ERx("%s"),SUCCESS);
		TDrefresh(mainW);
		reporting(SUCCESS,outComm);
		TCputc('\n',outComm);
	    }
	    else
	    {
		TDerase(mainW);
		TDmvwprintw(mainW,0,0,ERx("%s"),FAILURE_1);
		TDrefresh(mainW);
		reporting(FAILURE_1,outComm);
		TCputc('\n',outComm);
	    }
	}
	else
	{
	    TDerase(mainW);
	    TDmvwprintw(mainW,0,0,ERx("%s"),FAILURE_2);
	    TDrefresh(mainW);
	    reporting(FAILURE_2,outComm);
	    TCputc('\n',outComm);
	}
	break;
    }
    SIclose(newPtr);
    TCclose(outComm);
    TDendwin();
    TEclose();
    PCexit(0);
}



STATUS
tc_connection(char *flaginfo,TCFILE **infile)
{
    TCFILE                *commfile = NULL ;
    char                  *comma = NULL ;
    LOCATION               floc ;
    char                   sepinfile [80] ;
    char                   sepoutfile [80] ;
    bool                   noout = TRUE ;
    i4                     out_type ;

    /* check if working in batch mode */

    if ((comma = STindex(flaginfo,ERx(","),0)) != NULL)
	noout = FALSE;

    if (!noout && *(comma + 1) != 'B' && *(comma + 1) != 'b')
	noout = TRUE;

    if (comma)
	*comma = '\0';

    /* name input COMM-file */

#ifdef hp9_mpe
    STprintf(sepinfile,ERx("in%s.sep"),flaginfo);
#else
    STprintf(sepinfile,ERx("in_%s.sep"),flaginfo);
#endif
 
    /* name output RES file if working in batch mode */

    if (!noout)
    {
#ifdef hp9_mpe
        STprintf(sepoutfile,ERx("b%s.sep"),  flaginfo);
#else
	STprintf(sepoutfile,ERx("b_%s.sep"), flaginfo);
#endif
	out_type = TE_SI_FILE;
    }
    else
    {
	*sepoutfile = '\0';
	out_type = TE_NO_FILE;
    }

    /* redirect input to COMM-file */

    if (TEtest(sepinfile, TE_TC_FILE, sepoutfile, out_type) == FAIL)
	return(FAIL);

    /* open output COMM file */

#ifdef hp9_mpe
    STprintf(sepoutfile,ERx("out%s.sep"),flaginfo);
#else
    STprintf(sepoutfile,ERx("out_%s.sep"),flaginfo);
#endif
    LOfroms(FILENAME & PATH,sepoutfile, &floc);
    if (TCopen(&floc, ERx("w"), &commfile) != OK)
    	return(FAIL);

    /* send beginning of session message */

    TCputc(TC_BOS,commfile);
    TCflush(commfile);

    /* initialize FRAME/RUNTIME pointer */

    *infile = commfile;

    return(OK);

}

VOID
reporting(char *message,TCFILE *commfile)
{
    register char         *cp = message;

    for (; *cp != EOS; cp++)
    {
	TCputc(*cp, commfile);
    }
}
