# include	<compat.h>
# include	<er.h>
# include	<lo.h>
# include	<si.h>
# include	<me.h>
# include	<st.h>
# include	<equel.h>
# include	<eqsym.h>
# include	<equtils.h>
# include	<ereq.h>
# include	<ere2.h>
# include	<eqpas.h>

/*
** Copyright (c) 1985, 2001 Ingres Corporation
*/

/*
+* Filename:	paslab.c
** Purpose:	Manage information required to generate PASCAL declarations
**		of labels used by EQUEL later in the code.
** Defines:
**	pasLbInit()			- Initialize EQP Pass 2 Label manager.
**	pasLbFile( push, suprs )	- Push/Pop a file.
**	pasLbProc( proc, push, blk )	- Push/Pop a PASCAL procedure scope.
**	pasLbLabel( label )		- Add a label string.
**	pasLbGen()			- Execute pass 2 into output file.
**	pasLbSave()			- Save temp file ($_pasfile).
**	pasLbDump()			- Dump local data structures.
** Locals:	
-*	pasLbAlloc( size, name )	- Standard allocation routine.
**
** Notes:
**  1.  Many EQUEL statements generate labels.  In PASCAL (and others) all 
**	labels must be declared.  This is done by running a second pass over
**	the output file, and dumping label declarations into predefined spots,
**	marked by '##%' in the first column. By using the '##%' we ensure that
**	we will not confuse with user markers.  There are a few different cases
**	of label processing:
**  1.1 A single file with one procedure is the simplest case - all the labels
**	used are collected and dumped at one time into one mass-declaration.
**  1.2 Multiple procedures that are not nested (sequential down the file) have
**	a list of groups of label declarations, with the output file having
**	a few markers.
**  1.3 When nesting procedures the groups are listed (for sequential procs) 
**	and stacked (for nested procs) - again each procedure has its own 
**	marker.
**  1.4 For include files (non-inline) we make the restriction that if the 
**	PROCEDURE line is included then the whole procedure must be included.
**	This is so that all labels collected in the file (and which must be 
**	decalred in that file at the PROCEDURE line) can be processed on the
**	second pass of that file.  In this case we stack the file environment 
**	on top of case 1.3 (by using a file number) so as to only process the 
**	file just being closed.  You can have a PROCEDURE statement in a parent
**	file, and the labels used in a nested file.
**  2.	If the grammar gets out of sync with these routines, ie: syntax errors 
**	on END statements, then we may find that the ##% markers and the label
**	lists are also out of sync, causing internal errors.  These should be
**	cleaned up, or at least document together with the internal errors that
** 	the may be a result of syntax errors on block boundaries.
**  3.  Because this file runs a second pass over the output file, we make sure
**	that we are not writing to stdout - if we are then there is no need to
**	run the second pass, but we do save label info for testing/debugging.
**
** Example:
**	EQP Source:					Pass 1 Output(+indent):
**
**		proc p1;				##%1
**			dcl l1_1, l1_2;
**			proc p2;			##%2
**				dcl l2_1, l2_2;
**				proc p3;		##%3
**					use l3_1;
**				end;
**				use l2_3;
**			end;
**			use l1_3;
**			proc p4;			##%2
**			end;
**			proc p5;			##%2
**				use l5_1;
**	X---->
**				use l5_2;
**			end;
**		end;
**
**	At point X the label-head stack, label-head list, and label-number
**	lists look like:
**
**	Label-head List:		Label-head Stack:
**  [1]	 p1 -> l1_1, l1_2, l1_3		[Top   ]  -> p5
**	 |				[Top -1]  -> p1
**  [2]	 p2 -> l2_1, l2_2, l2_3
**	 |
**  [3]	 p3 -> l3_1
**	 |
**  [4]	 p4
**	 |
**  [5]	 p5 -> l5_1
**
**	This example shows that on file exit we can just walk down the 
**	Label-head list (for that file), and for each marker on the first-pass 
**	output we can dump the full list of label numbers.
**
**	The sequence of calls for the above example (upto point X) is:
**
**	pasLbInit();
**	pasLbProc( "p1", TRUE, 1 );
**	pasLbLabel( "l1_1" );
**	pasLbLabel( "l1_2" );
**	pasLbProc( "p2", TRUE, 2 );
**	pasLbLabel( "l2_1" );
**	pasLbLabel( "l2_2" );
**	pasLbProc( "p3", TRUE, 3 );
**	pasLbLabel( "l3_1" );
**	pasLbProc( "p3", FALSE, 0 );
**	pasLbLabel( "l2_3" );
**	pasLbProc( "p2", FALSE, 0 );
**	pasLbLabel( "l1_3" );
**	pasLbProc( "p4", TRUE, 2 );
**	pasLbProc( "p4", FALSE, 0 );
**	pasLbProc( "p5", TRUE, 2 );
**
** History:
**	27-dec-85	- Written for EQP Pass 2. (ncg)
**      08-30-93 (huffman)
**              add <st.h>
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
*/

/*
** Pas-Label-Number - 	For each label added we need a node that points to
**			the string representation of the label.  Labels can
** be user names as well as numbers.  These labels make up a collection of
** labels that are generated with a particular LABEL statement.
*/

typedef struct pln {
    char	*pln_name;		/* String rep of label */
    struct pln	*pln_next;
} PASL_NODE;

/*
** Pas-Label-List -	For each procedure we have list node that keeps all
** 			labels associated with it.  Each one of these maps to
** a ##% marker in the first pass file.  Besides general list stuff, the
** procedure name is kept for status (possibly indent/block level too).
*/

typedef struct pll {
    char	*pll_proc;		/* Procedure name */
    i4		pll_line;		/* Source line */
    i4		pll_block;		/* Indentation/block level */
    PASL_NODE	*pll_lnhead;		/* Labels for procedure */
    PASL_NODE	*pll_lntail;		/* Tail, used as a queue */
    struct pll	*pll_next;		/* Next lab list (procedure) */
    struct pll	*pll_stknext;		/* Next lab in stack */
} PASL_LIST;

/*
** Pas-LabFile-Env -	File environment in which we are working. If the name
**			stack has more than one element than we are generating 
** output into a non-inline include file.  Each file element contains a file 
** name and number to which a Pas-Label-List is associated.
*/

# define	PASfilMAX	20

typedef struct plf {		/* Mini-file descriptor */
    char	*plf_name;		/* Input file name (for include) */
    i4		plf_err;		/* Error processing labels */
    PASL_LIST	*plf_lbparent;		/* Last label list of parent file */
    PASL_LIST	*plf_lbfirst;		/* First label list of current file */
} PASL_FILE;

typedef struct ple {		/* Complete EQP environment */
    PASL_FILE	ple_file[PASfilMAX];	/* Stack of mini file descs indexed by
					** ple_filenum			    */
    i4		ple_filenum;		/* Input file number <=> pll_number */
    PASL_LIST	*ple_stack;		/* Stack of nested procs (not files) */
    PASL_LIST	*ple_lbhead;		/* EQP's label list head */
    PASL_LIST	*ple_lbtail;		/* 	and tail 	  */
} PASL_ENV;

# define PASlbMAX	5	/* Max labels on a line */

/*
** Each EQP uses tagged memory to get rid of things when done with a command
** line file in one go.  The tag is  PASmemTAG. (Note that when linking with SQL
** it uses some lower tags for its symbol table, and when linking with the
** frame lib it seems to reserve all tags above 50. Weird!)
*/
# define PASmemTAG	20

/* String table for all names and labels used in one EQP */
# define PASstrSIZE	300
static	 STR_TABLE	*pas_str ZERO_FILL;

static   PASL_ENV	pasLabEnv ZERO_FILL;		/* Global file env */

static	 i4		pas_pass2save ZERO_FILL;	/* Save pass2 file */

static i4		*pasLbAlloc();
void			pasLbGen();

/*
+* Procedure:	pasLbInit
** Purpose:	Initialize local label info for each file on command line.
** Parameters:
**	None
** Return Values:
-*	None
** Notes:
**	If anything is not in the NULL state, then we can assume that data
**  	was left over from the previous command line file, or somehow the file
** 	was aborted.  In any case reset all globals to original state.
*/

void
pasLbInit()
{
    extern   i4	(*inc_pass2)();
    register PASL_ENV	*ple = &pasLabEnv;

    if (pasLabEnv.ple_lbhead)			/* Stuff left over */
	MEtfree( PASmemTAG );
    if (pas_str != STRNULL)
	str_free( pas_str, (char *)0 );
    else 					/* Initialize string table */
	pas_str = str_new( PASstrSIZE );

    inc_pass2 = pasLbFile;			/* Set up for Include files */

    MEfill( sizeof(PASL_ENV), '\0', ple );
    ple->ple_file[0].plf_name = str_add( pas_str, ERx("Input") );
    ple->ple_file[0].plf_err = FALSE;
    ple->ple_filenum = 0;
    ple->ple_stack = (PASL_LIST *)0;
    ple->ple_lbhead = ple->ple_lbtail = (PASL_LIST *)0;
}

/*
+* Procedure:	pasLbFile
** Purpose:	Push/pop a file environment.
** 		May be indirectly called for Include files to get at our 
**		routines.
** Parameters:
**	push	- i4	- TRUE (enter) or FALSE (exit).
**	supress	- i4	- TRUE if the Include file has output suppressed,
**			  FALSE otherwise.  This argument is only used on
**			  file exit.
** Return Values:
-*	None
** Notes:
**  1.  On file PUSH, allocate a Pas-Lab-File. Set required fields and push 
**	onto Pas-File-Stack.
**  2.  On file POP generate all labels, and pop the Pas-File-Stack.
** Called:
**	Besides from GR_CLOSE also called indirectly from scinclude.c with
**	extra "output suppressed" argument.
*/

i4
pasLbFile( push, suppress )
i4	push;
i4	suppress;
{
    register PASL_ENV	*ple = &pasLabEnv;
    register PASL_FILE	*plf;

    if (push)		/* Push the name of include and up the file number */
    {
	if (ple->ple_filenum+1 >= PASfilMAX)
	{
	    er_write( E_E20009_hpLBINC, EQ_ERROR, 1, er_na(PASfilMAX) );
	    ple->ple_file[ple->ple_filenum].plf_err = TRUE;
	}
	else
	{
	    plf = &ple->ple_file[++ple->ple_filenum];
	    MEfill( sizeof(PASL_FILE), '\0', plf );
	    plf->plf_name = str_add( pas_str, eq->eq_filename );
	    /* Set parent list to last list in parent file */
	    plf->plf_lbparent = ple->ple_lbtail;
	}
    }
    else	/* pop global or include file */
    {
	if (ple->ple_filenum < 0)	/* Must be error on EOF or something */
		return;
	if (!suppress)			/* Include output not suppressed */
	    pasLbGen();
	plf = &ple->ple_file[ple->ple_filenum--];
	/* On file pop get rid of trailing proc lists */
	if (plf->plf_lbparent != (PASL_LIST *)0)
	{
	    plf->plf_lbparent->pll_next = (PASL_LIST *)0;	/* Disconnect */
	    ple->ple_lbtail = plf->plf_lbparent;
	}
	else 
	{
	    /* No parents - nothing before this file */
	    ple->ple_lbhead = ple->ple_lbtail = (PASL_LIST *)0;
	}
	MEfill( sizeof(PASL_FILE), '\0', plf );
    }
}


/*
+* Procedure:	pasLbProc
** Purpose:	Enter/Exit a procedure and start a new list.
** Parameters:
**	proc	- char *- Name of procedure (ignored on exit).
**	push	- i4	- TRUE (enter) or FALSE (exit).
**	block	- i4	- Block level of procedure (decide if push onto stack).
** Return Values:
-*	None
** Notes:
**  1.	On proc PUSH get a new Pas-Label-List and set fields. Determine if this
**	is a nested procedure (block level is greater than that on top of 
**	Pas-Label-Stack), and if so push onto that stack.  Add to tail of
**	label lists.
**  2.	On proc POP just pop the Pas-Label-Stack; by default it is the procedure
**	we just finished parsing.
*/

void
pasLbProc( proc, push, block )
char	*proc;
i4	push;
i4	block;
{
    char		buf[MAX_LOC +1];
    register PASL_LIST	*pll;
    register PASL_ENV	*ple = &pasLabEnv;
    register PASL_FILE	*plf;

    if (push)
    {
	/* Put out marker into output file */
	STprintf( buf, ERx("##%% LABELS: '%s', line %d\n"), proc, eq->eq_line );
	out_emit( buf );

	/* Alloc and set fields */
	pll = (PASL_LIST *)pasLbAlloc( sizeof(PASL_LIST), ERx("pasLbProc()") );
	pll->pll_proc = str_add( pas_str, proc );
	pll->pll_line = eq->eq_line;
	pll->pll_block = block;
	/* Add onto tail of label lists of file */
	if (ple->ple_lbhead)
	    ple->ple_lbtail->pll_next = pll;
	else
	    ple->ple_lbhead = pll;
	ple->ple_lbtail = pll;

	plf = &ple->ple_file[ple->ple_filenum];
	if (plf->plf_lbfirst == (PASL_LIST *)0)	 /* Set as first list in file */
	    plf->plf_lbfirst = pll;
	/*
	** The procedure must be entered into the top of the stack.  In all
	** cases (based on the block level and the fact that on block exit the
	** stack is popped) it has to be pushed on the stack.
	*/
	pll->pll_stknext = ple->ple_stack;
	ple->ple_stack = pll;			/* Now put at top */
    }
    else	/* pop */
    {
	/* Pop the stack */
	if (pll = ple->ple_stack)			/* Can't be null here */
	{
	    ple->ple_stack = pll->pll_stknext;
	    pll->pll_stknext = (PASL_LIST *)0;		/* Disconnect */
	}
    }
}

/*
+* Procedure:	pasLbLabel
** Purpose:	Add a label to most recent Pas-Label-list that is on top of
**		the Pas-Label-Stack.
** Parameters:
**	label	- char *- Caller's label to add.
** Return Values:
-*	None
** Called:
**	From code generator when putting out a label definition.
*/

void
pasLbLabel( label )
char	*label;
{
#   define   PASLnodes	100
    register PASL_NODE	*pln;
    register PASL_LIST	*pll;
    register PASL_ENV	*ple = &pasLabEnv;
    	     PASL_FILE	*plf;
             i4    	i;
    static   PASL_NODE	*pn_list = (PASL_NODE *)0;
    
    /*
    ** If there is no label list on top of stack then there was no entry via
    ** pasLbProc, and this is an illegal use of a label.  Print error saying
    ** no previous ## procedure (or ## label for old timers).
    */
    if ((pll = ple->ple_stack) == (PASL_LIST *)0)
    {
	plf = &ple->ple_file[ple->ple_filenum];
	if (plf->plf_err == FALSE)
	{
	    if (dml->dm_lang == DML_ESQL)
		er_write( E_E20023_hpLBSQNONE, EQ_ERROR, 0 );
	    else
		er_write( E_E2000A_hpLBNONE, EQ_ERROR, 0 );
	    plf->plf_err = TRUE;
	}
	return;
    }

    /* 
    ** See if we can grab one from a pool - if not then alloc PASLnodes of them
    ** and make array of storage into linked list.
    */
    if (pn_list == (PASL_NODE *)0)
    {
	pn_list = (PASL_NODE *)pasLbAlloc( PASLnodes * sizeof(PASL_NODE),
	    				   ERx("pasLbLabel") );
	for (i = 1, pln = pn_list; i < PASLnodes; i++, pln++)
	    pln->pln_next = pln + 1;
	pln->pln_next = (PASL_NODE *)0;
    }
    pln = pn_list;
    pn_list = pln->pln_next;
    pln->pln_next = (PASL_NODE *)0;		/* Disconnect from list */

    pln->pln_name = str_add( pas_str, label );

    /* Add node onto tail node of current label list */
    if (pll->pll_lnhead)
	pll->pll_lntail->pln_next = pln;
    else
	pll->pll_lnhead = pln;
    pll->pll_lntail = pln;
}


/*
+* Procedure:	pasLbAlloc
** Purpose:	Get tagged memory and exit if we fail.
** Parameters:
**	size	- i4	- How many bytes of memory to allocate.
**	name	- char *- Name of caller for errors. 
** Return Values:
-*	i4 *	- A pointer to the allocated space.
** Notes:
**	Let MEtcalloc do all of the work -- have it clear memory first.
*/

static i4 *
pasLbAlloc( size, name )
i4	size;
char	*name;
{
    i4		*p;

    p = (i4 *) MEreqmem(PASmemTAG,size,TRUE,NULL);
    if ( p != NULL )
	return (i4 *) p;
    er_write( E_EQ0001_eqNOMEM, EQ_FATAL, 2, er_na(size), name );
}


/*
+* Procedure:	pasLbSave
** Purpose:	Save temp file after pass 2.
** Parameters:
**	None
** Return Values:
-*	None
*/

void
pasLbSave()
{
    pas_pass2save = TRUE;
}

/*
+* Procedure:	pasLbGen
** Purpose:	Generate all label declarations for a file.
** Parameters:
**	None
** Return Values:
-*	None
** Notes:
**  1. 	This routine assumes that the output file has been closed but that
**	eq->eq_outname is still set. We need the name for the second pass.
**	When writing data to stdout (-f used), then make sure we leave 
**	eq_outfile still set to stdout.
**  2.  Method used: If some kind of label error occured while processing file
**	or no labels were used or PASCAL source went to stdout, then just quit
**	here.  Otherwise set up files - rename original output file to a temp
**	name, and reopen the original named output file for writing.  Walk the
**	temp file, and for each special marker (##%) find the corresponding
**	Pas-Label-List - dump all labels on that list (for non-marked lines
**	just spit out stuff on line).  When done with the file delete the 
**	temp file.
** History:
**	12-oct-1988	- Updated to generate unique file name in
**			  output directory. (ncg)
*/

void
pasLbGen()
{
    register PASL_ENV	*ple = &pasLabEnv;
    register PASL_LIST	*pll;
    register PASL_NODE	*pln;
    PASL_FILE	   	*plf;
    LOCATION		pass2loc;	/* Rename orig outfile to tmp pas2 */
    char		pass2buf[MAX_LOC+1];
    char		*pass2s;
    FILE		*pass2f;
    LOCATION		outloc;		/* New output file */
    char		outbuf[MAX_LOC+1];
    FILE		*outf;
    char		*outs;
    char		tmpbuf[MAX_LOC +1];
    i4			cnt;

    plf = &ple->ple_file[ple->ple_filenum];
    if (plf->plf_err == TRUE) 			/* Error entering label name */
	return;
    if (plf->plf_lbfirst == (PASL_LIST *)0)	/* No labels used in file */
	return;
    if (eq->eq_outfile == stdout)		/* Cannot re-read stdout! */
	return;

    STcopy( eq->eq_outname, outbuf );
    LOfroms(PATH & FILENAME, outbuf, &outloc);
    LOcopy( &outloc, pass2buf, &pass2loc );
    LOuniq(ERx("iip"), ERx("eqp"), &pass2loc);
    LOtos( &outloc, &outs );
    LOtos( &pass2loc, &pass2s );
    /* 
    ** outloc 	- Real output location.
    ** pass2loc	- Temp output location for second pass.
    **
    ** Now rename the original output file (with the ##% markers) to the temp 
    ** file. Open (for reading) the temp file (pass2loc) and open (for writing)
    ** the original-named output file for a final pass.
    **
    ** Various failures may occur, but the standard error message is:
    **		Cannot execute EQP Pass 2 from %0 to %0.
    */
    if (LOrename(&outloc, &pass2loc) != OK)
    {
	er_write( E_E2000B_hpLBPASS2, EQ_ERROR, 2, plf->plf_name, pass2s );
	return;
    }
    if (SIopen(&pass2loc, ERx("r"), &pass2f) != OK)
    {
	er_write( E_E2000B_hpLBPASS2, EQ_ERROR, 2, plf->plf_name, pass2s );
	return;
    }
    if (SIopen(&outloc, ERx("w"), &outf) != OK)
    {
	er_write( E_E2000B_hpLBPASS2, EQ_ERROR, 2, plf->plf_name, outs );
	return;
    }
    /*
    SIfprintf( outf, ERx("{EQUEL/PASCAL Pass 2 of \"%s\"}\n"), plf->plf_name );
    */
    pll = plf->plf_lbfirst;		/* First label list in file */
    while (SIgetrec(tmpbuf, MAX_LOC, pass2f) == OK)
    {
	if (tmpbuf[0] != '#' || tmpbuf[1] != '#' || tmpbuf[2] != '%')
	{
	    SIputrec( tmpbuf, outf );
	    continue;
	}
	/* This is a marked line - get labels */
	if (pll == (PASL_LIST *)0)
	{
	    if (plf->plf_err == FALSE)
	    {
		/* EQP inconsistency error: too many markers */
		er_write( E_E2000C_hpLBSYNC, EQ_ERROR, 1, plf->plf_name );
		plf->plf_err = TRUE;
	    }
	    continue;
	}
	if (pll->pll_lnhead == (PASL_NODE *)0)	/* No labels for procedure */
	{
	    SIfprintf( outf, ERx("  {No EQP labels for routine \"%s\"}\n"), 
		       pll->pll_proc );
	    pll = pll->pll_next;
	    continue;
	}
	SIfprintf( outf, ERx("  {EQP labels for routine \"%s\"}\n  label "),
		   pll->pll_proc );
	cnt = 0;
	for (pln = pll->pll_lnhead; pln; pln = pln->pln_next)
	{
	    SIfprintf( outf, ERx("%s"), pln->pln_name );
	    if (pln->pln_next == (PASL_NODE *)0)
		SIfprintf( outf, ERx(";\n") );
	    else
	    {
		SIfprintf( outf, ERx(",") );
	    	if (++cnt == PASlbMAX)
		{
		    SIfprintf( outf, ERx("\n        ") );
		    cnt = 0;
		}
	    }
	}
	pll = pll->pll_next;
    }
    /* Should be done with file and with labels */
    if (pll != (PASL_LIST *)0)
    {
	if (plf->plf_err == FALSE)
	{
	    /* EQP inconsistency error: not enough markers */
	    er_write( E_E2000C_hpLBSYNC, EQ_ERROR, 1, plf->plf_name );
	    plf->plf_err = TRUE;
	}
    }
    SIclose( outf );
    SIclose( pass2f );
    if (!pas_pass2save)		/* Saved by $_pasfile */
	LOdelete( &pass2loc );
}


/*
+* Procedure:	pasLbDump
** Purpose:	Dump all Label information stored
** Parameters:
**	None
** Return Values:
-*	None
** Notes:
**	Indirectly called via comment debug $_paslab.
*/

void
pasLbDump()
{
    register PASL_ENV	*ple = &pasLabEnv;
    register PASL_LIST	*pll;
    register PASL_NODE	*pln;
    PASL_FILE		*plf;
    i4			lbcnt;
    i4			cnt;
    FILE		*df = eq->eq_dumpfile;

    SIfprintf( df, ERx("PASCAL Label Manager:\n") );
    for (cnt = 0,plf = &ple->ple_file[0]; cnt <= ple->ple_filenum; 
	 cnt++, plf++)
    {
	SIfprintf( df, ERx(" File [%2d] plf_name = '%s', plf_err = %d\n"), 
		   cnt, plf->plf_name, plf->plf_err );
	if ((pll = plf->plf_lbparent) == (PASL_LIST *)0)
	    SIfprintf( df, ERx("           plf_lbparent = NULL\n") );
	else
	{
	    SIfprintf( df,
		    ERx("           plf_lbparent: proc='%s', block=%d\n"),
		    pll->pll_proc, pll->pll_block );
	}
	if ((pll = plf->plf_lbfirst) == (PASL_LIST *)0)
	    SIfprintf( df, ERx("           plf_lbfirst = NULL\n") );
	else
	{
	    SIfprintf( df,
		    ERx("           plf_lbfirst: proc = '%s', block = %d\n"),
		    pll->pll_proc, pll->pll_block );
	}
    }
    SIfprintf( df, ERx(" ple_filenum = %d\n"), ple->ple_filenum );
    if ((pll = ple->ple_stack) == (PASL_LIST *)0)
	SIfprintf( df, ERx(" ple_stack = NULL\n") );
    else
    {
	SIfprintf( df, ERx(" ple_stack:\n") );
	for (cnt = 0; pll; pll = pll->pll_stknext, cnt++)
	    SIfprintf( df, ERx("    [%2d] proc = '%s', block = %d\n"), 
		    cnt, pll->pll_proc, pll->pll_block );
    }
    if ((pll = ple->ple_lbhead) == (PASL_LIST *)0)
	SIfprintf( df, ERx(" ple_lbhead = ple_lbtail = NULL\n") );
    else
    {
	SIfprintf( df, ERx(" ple_lblist:\n") );
	for (lbcnt = 0; pll; pll = pll->pll_next, lbcnt++)
	{
	    SIfprintf( df,
		    ERx("    [%2d] proc = '%s', line = %d, block = %d\n"),
		    lbcnt, pll->pll_proc, pll->pll_line, pll->pll_block );
	    cnt = 0;
	    SIfprintf( df, ERx("      LB: ") );
	    for (pln = pll->pll_lnhead; pln; pln = pln->pln_next)
	    {
		SIfprintf( df, ERx("%s,"), pln->pln_name );
		if (pln->pln_next != (PASL_NODE *)0 && ++cnt == PASlbMAX)
		{
		    SIfprintf( df, ERx("\n          ") );
		    cnt = 0;
		}
	    }
	    SIfprintf( df, ERx("\n") );
	}
    }
    SIflush( df );
    str_dump( pas_str, ERx("Pascal Label String Table") );
}
