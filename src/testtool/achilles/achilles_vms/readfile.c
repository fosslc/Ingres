/*
 * readfile.c
 *
 * Contains the readfile() routine, which reads the configuration file and
 * loads the data structures for test types. Also, unfortunately, contains
 * all the code for searching the path for the executable, determining if
 * the executable is an a.out or a shell script (the latter can't be exec'd
 * directly), and general error checking. This is the messiest part.
**
**	13-mar-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from testtool code as the use is no longer allowed
 */

#include "achilles.h"

#define ST_NEWLINE 0
#define ST_EXEC    1
#define ST_NKIDS   2
#define ST_NITERS  3
#define ST_INTGRP  4
#define ST_KILLGRP 5
#define ST_INTINT  6
#define ST_KILLINT 7
#define ST_INFILE  8
#define ST_OUTFILE 9
#define ST_USER    10
#define ST_INVALID 11


readfile (fname, testarr)
char	   *fname;
TESTDESC ***testarr;
{
    i4	      c,
	      i,
	      j,
	      cur_argv,
	      rval,
	      fd;
    char      fstr[MAX_LOC],
	      tempdata[512],
	     *gooddata,
	     *data_end;
    LOCATION  floc;
    TESTDESC *testhead,
	     *newtest;
    FILE     *in;
    i4	      tdind = 0,
	      errcount = 0,
	      testcount = 0,
	      state = ST_NEWLINE,
	      linenum = 1;
    TESTDESC *curtest = NULL;

    STcopy(fname, fstr);
    LOfroms(PATH & FILENAME, fstr, &floc);
    if (SIopen(&floc, "r", &in) != OK)
    {
	SIprintf("Can't find/open %s.\n", fname);
	return(-1);
    }

    while ( (c = SIgetc(in) ) != EOF)
    {
	/* Found a comment. Discard to end of line, leaving the newline
	   so the next SIgetc() will take care of end-of-line processing.
	*/
	if (c == '#')
	{
	    while ( ( (c = SIgetc(in) ) != '\n') && (c != EOF) )
		;
	    SIungetc('\n', in);
	    continue;
	}

	/* Found spaces or newline. Unless we're still reading the command
	   line for the test, we're at the end of a field. If we're in the
	   command line, we do special processing such as verifying that
	   executables exist and expanding the <DBNAME> variable.
	*/
	if (CMwhite(&c) )
	{
	    tempdata[tdind] = '\0';
	    switch (state)
	    {
		case ST_EXEC:

		    /* Semicolon ends the command line. */
		    if (*tempdata == ';')
			curtest->t_argv[cur_argv] = NULL;
		    else
		    {
			gooddata = NULL;

			/* If this word is supposed to be the name of the
			   executable (ie it's argv[0]) try to find the
			   executable.
			*/
			if (cur_argv == 0)
			{
			    if (ACfindExec(tempdata, &gooddata, &data_end,
				curtest->t_argv, &cur_argv) != OK)
			    {
				SIprintf("Line %d, %s: %s: commmand not found.\n",
					 linenum, fname, tempdata);
				errcount++;
				state = ST_NEWLINE;
				SIungetc('#', in);
			    }
			}
			else
			{
			    /* If argument is <DBNAME>, we'll copy the data-
			       base name rather than the argument.
			    */
			    rval = STcompare(tempdata, "<DBNAME>");
			    if ( (!rval) && (!dbname) )
			    {
				SIprintf("Line %d, %s: executable %s requires a database name, but none set.\n",
					 linenum, fname, curtest->t_argv[0]);
				errcount++;
			    }
			    else
				gooddata = (rval ? tempdata : dbname);
			}
			/* If, after all this, we've got a vaild command
			   line argument, add it to the argv for this test
			   type.
			*/
			if (gooddata)
			{
			    STcopy(gooddata, data_end);
			    curtest->t_argv[cur_argv++] = data_end;
			    data_end += STlength(gooddata) + 1;
			}
			/* The following line counteracts the '++state' 
			   farther down. In this special case, we don't 
			   change state just because we encounterd white 
			   space - we're still collecting parts of the test
			   command line.
			*/
			state--;
		    }
		    break;

		case ST_NKIDS:
		    CVan(tempdata, &curtest->t_nkids);
		    break;

		case ST_NITERS:
		    CVan(tempdata, &curtest->t_niters);
		    break;

		case ST_INTGRP:
		    CVan(tempdata, &curtest->t_intgrp);
		    curtest->t_intgrp = atoi(tempdata);
		    if (curtest->t_intgrp > curtest->t_nkids)
		    {
			SIprintf("Line %d, %s: Interrupt cluster size cannot exceed total number\n",
				 linenum, fname);
			SIprintf("of child processes.\n");
			errcount++;
			state = ST_NEWLINE;
			SIungetc('#', in);
		    }
		    break;

		case ST_KILLGRP:
		    CVan(tempdata, &curtest->t_killgrp);
		    /* Since we don't send both an interrupt signal and a
		       kill signal to a process simultaneously, the sum of
		       the interrupt cluster size and the kill cluster size
		       shouldn't me greater than the total number of
		       processes.  Make sure this is the case.
		    */
		    /* Well, on second thought, if the interrupt interval
		       and kill interval are mutually prime, we want to
		       allow interupts and kills on the same process at
		       different times. So for now, just make sure that
		       intgrp and killgrp individually aren't too big.
		    */
		    if (curtest->t_killgrp > curtest->t_nkids)
		    {
			SIprintf("Line %d, %s: Kill cluster size cannot exceed total number\n",
				 linenum, fname);
			SIprintf("of child processes.\n");
			errcount++;
			state = ST_NEWLINE;
			ungetc('#', in);
		    }
		    break;

		case ST_INTINT:
		    CVan(tempdata, &curtest->t_intint);
		    break;

		case ST_KILLINT:
		    CVan(tempdata, &curtest->t_killint);
		    break;

		case ST_INFILE:
		    if (!STcompare(tempdata, "<NONE>") )
			*curtest->t_infile = '\0';
		    else
			STcopy(tempdata, curtest->t_infile);
			break;

		case ST_OUTFILE:
		    if (!STcompare(tempdata, "<NONE>") )
			*curtest->t_outfile = '\0';
		    else
			STcopy(tempdata, curtest->t_outfile);
		    break;

		case ST_USER:
		    if ( (rval = ACgetUser(tempdata, &curtest->t_user) )
			!= OK)
		    {
			switch (rval)
			{
			    case NO_SUCH_USER:
				SIprintf("Line %d, %s: %s: unknown user.\n",
				  	 linenum, fname, tempdata);
				break;
			    case NOT_ROOT:
				SIprintf("Line %d, %s: frontends can't be run as super-user unless you are the\n",
					 linenum, fname);
				SIprintf("super-user.\n");
				break;
			}
			errcount++;
			state = ST_NEWLINE;
			SIungetc('#', in);
		    }
		    else
			STcopy( tempdata, curtest->t_uname );
		    break;
	    }
	    tdind = 0;
	    /* By the time we get to the end of a line, there should either
	       be no fields specified (ie blank line or comment) or "enough"
	       fields specified. Complain if otherwise.
	    */
	    if (c == '\n')
	    {
		if ( (state != ST_NEWLINE) && (state != ST_INVALID-1) )
		{
		    SIprintf("Line %d, %s: not enough fields on a line.\n", 
			     linenum, fname);
		    errcount++;
		}
		linenum++;
		state = ST_NEWLINE;
	    }
	    else
	    {
		/* If >1 consecutive whitespace character, skip over the rest.
		*/
		while (1)
		{
		    c = SIgetc(in);
		    if ( (c == EOF) || ( (c != ' ') && (c != '\t') ) )
		    {
			SIungetc(c, in);
			break;
		    }
		}
		/* It's arguable that extra args on a line should be
		   silently ignored, but for now we complain - no reason to
		   assume that the last args are the incorrect ones.
		*/
		if (++state == ST_INVALID)
		{
		    SIprintf("Line %d, %s: too many fields on a line.\n",
			     linenum, fname);
		    errcount++;
		    SIungetc('#', in);
		    state = ST_NEWLINE;
		}
	    }
	    continue;
	}
	/* On the first "legitimate" (non-comment, non-white) character on a
	   line, allocate a new test structure. We keep these structures in
	   a linked list until we've read the whole config file, at which
	   time we allocate a "right-sized" array of pointers.
	*/
	if (state == ST_NEWLINE)
	{
	    testcount++;
	    MEalloc(1, sizeof(TESTDESC), (PTR *) &newtest);
	    newtest->t_next = NULL;
	    if (curtest)
		curtest->t_next = newtest;
	    else
		testhead = newtest;
	    curtest = newtest;
	    cur_argv = 0;
	    data_end = curtest->t_argvdata;
	    state = ST_EXEC;
	}
	/* If there's nothing at all exciting about the current character,
	   add it to the data string.
	*/
	tempdata[tdind++] = (char) c;
    }
    SIclose(in);

    /* Print some nice info for the user. */
    if (errcount == 0)
	for (i = 1,curtest = testhead ; curtest ; curtest = curtest->t_next)
	{
	    SIprintf("Test #%d:\n", i++);
	    SIprintf("   Executable = '");
	    for (j = 0 ; curtest->t_argv[j] ; j++)
	    {
		SIprintf("%s", curtest->t_argv[j]);
		SIputc(' ', stdout);
	    }
	    SIprintf("\b'\n");
	    SIprintf("   Number of children = %d\n", curtest->t_nkids);
	    SIprintf("   Number of iterations = %d\n", curtest->t_niters);
	    SIprintf("   Interrupt cluster size = %d\n", curtest->t_intgrp);
	    SIprintf("   Kill cluster size = %d\n", curtest->t_killgrp);
	    SIprintf("   Interrupt interval = %d seconds.\n",
		     curtest->t_intint);
	    SIprintf("   Kill interval = %d seconds.\n",
		     curtest->t_killint);
	    SIprintf("   Input filename = '%s'\n", curtest->t_infile);
	    SIprintf("   Output filename = '%s'\n", curtest->t_outfile);
	    ACprintUser(curtest->t_uname);
	}
    SIprintf("There %s %d error%s found in the test description file.\n", 
	     (errcount == 1 ? "was" : "were"), errcount,
	     (errcount == 1 ? "" : "s") );
    SIflush(stdout);
    if (errcount)
	return(-1);
    else
    {
	/* Make an array of pointers, and assign them to the test structures.
	*/
	MEalloc(testcount, sizeof(TESTDESC *), (PTR *) testarr);
	for (i = 0, curtest = testhead ; i < testcount ; i++)
	{
	    (*testarr)[i] = curtest;
	    curtest = curtest->t_next;
	}
	return(testcount);
    }
} /* readfile */
