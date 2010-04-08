/*
**Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<er.h>
# include       <ex.h>
# include	<lo.h>
# include	<nm.h>
# include	<si.h>
# include	<st.h>
# include	"nmlocal.h"
 

/**
** Name: INGSETENV.C - Set INGRES Environment Variable
**
** Description:
**	This program defines and sets INGRES environment variables
**	in the file, "~ingres/files/symbol.tbl".
**    
**      main()        	-  main program to perform the the setting  
**
** History: Log:	ingsetenv.c,v
 * Revision 1.1  88/08/05  13:43:10  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
** Revision 1.3  87/11/13  11:33:36  mikem
** use nmlocal.h rather than "nm.h" to disambiguate the global nm.h header from
** the local nm.h header.
**      
**      Revision 1.2  87/11/10  14:43:56  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:05:21  mikem
**      Updated to meet jupiter coding standards.
**      
**	2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**      10/14/94(nanpr01)-- Removed #include "nmerr.h". Bug # 63157
**      9/28/94 (cwaldman) -- Disable interrupts during ingsetenv execution
**                            to avoid destruction of symbol.tbl (bug 44445).
**	12/7/94 (ramra01) -- Cross Integ from 6.4 bug 44445
**	18-oct-96 (mcgem01)
**		Remove hardcoded reference to utility name.
**	31-dec-97 (funka01)
**		Modify non descript error message to usage.
**	20-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-nov-2002 (gupsh01)
**	    Modified ingsetenv to prompt for parameters if less than 
**	    two are specified at the command line.
**	13-jan-2004 (somsa01)
**	    Properly turn off exceptions.
**	03-feb-2004 (somsa01)
**	    Backed out last changes for now.
**/

/*
**		
NEEDLIBS = COMPAT MALLOCLIB
**
UNDEFS =	II_copyright

PROGRAM = (PROG2PRFX)setenv
*/
 

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: main - set an INGRES environment variable.
**
** Description:
**	This program defines and sets INGRES environment variables
**	in the file, "~ingres/files/symbol.tbl".
**
** Inputs:
**	
**	argv[1]				    Environment variable to set
**	argv[2]				    Value to set the variable to.
**
** Output:
**	none
**
**      Returns:
**	    (exit status of program)
**	    OK
**	    FAIL
** History:
**      20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	18-nov-02 (gupsh01)
**	    Modified ingsetenv to prompt for parameters if less
**	    are specified.
*/
main(argc, argv)
i4	argc;
char	**argv;
{
	char		errbuf[ER_MAX_LEN];
	register i4	errnum = FAIL;
	char            *nameptr = 0;
	char            *valptr = 0; 
	char            namebuf[MAXLINE];
	char            valuebuf[MAXLINE];
	bool		name_ok = FALSE;	
	bool            arg_ok = FALSE;
 
	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );

        /* Disable interrupts during ingsetenv */
        EXinterrupt(EX_OFF);

	CMset_locale(); /* set the locale to II_CHARSET locale */

	if (argc < 3)
	{
	  /* prompt the user for input */
	  if (argc == 2)
	  {
	    nameptr = argv[1];
	    name_ok = TRUE;	
	  }
	  else
	  { 
	    SIprintf("Variable name : ");
	    if( OK != SIgetrec(namebuf, MAXLINE, stdin) )
	      SIprintf("%s: invalid arguments\n", argv[0]);
	    else
	    { 
	      if (STtrmwhite(namebuf) > 0)	
	      {
	        nameptr = namebuf;
                name_ok = TRUE;	
	      }
              else
	        SIprintf("%s: invalid arguments\n", argv[0]);
	    }
	  }

	  if (name_ok == TRUE)
	  {
	    SIprintf("Value : ");
	    if( OK != SIgetrec(valuebuf, MAXLINE, stdin) )
	      SIprintf("%s: invalid arguments\n", argv[0]);
	    else
	    {
	      if (STtrmwhite(valuebuf) > 0)
	      {
	        valptr = valuebuf;
	        arg_ok = TRUE;
	      }
	      else
	        SIprintf("%s: invalid arguments\n", argv[0]);
	    }
	  }
	}
	else if (argc > 3)
	{
	  SIprintf("Usage: %s <variable> <value>\n", argv[0]);
	}
	else
	{
	  nameptr = argv[1];
	  valptr = argv[2];
	  arg_ok = TRUE;
	}

	if ((arg_ok == TRUE) && 
	    ((errnum = NMstIngAt(nameptr, valptr)) != OK))
	{
		SIprintf("%s: unable to set variable \"%s\"\n",
			argv[0], nameptr);
		ERreport(errnum, errbuf);
		SIprintf("\t%s\n", errbuf);
		SIprintf("\tonly the %s super-user can use %s\n",
			SystemProductName, argv[0]);
	}

        /* Re-enable interrupts */
        EXinterrupt(EX_ON);

	PCexit( errnum ? FAIL : OK );
}
