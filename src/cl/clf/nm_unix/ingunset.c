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
** Name: INGUNSET.C - Unsetet INGRES Environment Variable
**
** Description:
**	This program undefines and unsets INGRES environment variables
**	in the appropriate symbol table file in $II_SYSTEM/files.
**	This program does not report failure if the symbol was not
**	defined.
**    
**      main()        	-  main program to perform the unsetting  
**      NMzapIngAt()	- unset a value in the symbol.tbl.
**
** History: Log:	ingunset.c,v
 * Revision 1.1  88/08/05  13:43:10  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
** Revision 1.3  87/11/13  11:33:41  mikem
** use nmlocal.h rather than "nm.h" to disambiguate the global nm.h header from
** the local nm.h header.
**      
**      Revision 1.2  87/11/10  14:44:01  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**	6/20/86 daveb -- created from ingsetenv.
**	25-jan-1989 (greg)
**	    zap obsolete junk.
**	    add comment about not reporting failure when symbol is
**		not defined.
**	24-may-90 (blaise)
**	    Integrated changes from 61 and ingresug:
**		Move NMzapIngAt routine to a separate module.
**      14-oct-94 (nanpr01)
**          Removed #include "nmerr.h". Bug # 63157.
**	07-dec-94 (ramra01)
**	    Cross Integ 6.4 bug fix 44445.
**     	    28-sep-94 (cwaldman)
**          Disabled interrupts during execution of ingunset to
**          avoid destruction of symbol.tbl (bug 44445).
**	18-oct-96 (mcgem01)
**	    Remove hard-coded references to the utility name.
**	20-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
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

PROGRAM = (PROG2PRFX)unset
*/
 

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*{
** Name: main - unset an INGRES environment variable.
**
** Description:
**	This program unsets INGRES environment variables
**	in the file, "~ingres/files/symbol.tbl".
**
** Inputs:
**	
**	argv[1]				    Environment variable to be unset
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
*/
main(argc, argv)
i4	argc;
char	**argv;
{
	char		errbuf[ER_MAX_LEN];
	register i4	errnum = FAIL;
 
	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );
 
        /* Disable interrupts during ingunset execution */
        EXinterrupt(EX_OFF);

	if (argc < 2)
	{
		SIprintf("%s: too few arguments\n", argv[0]);
	}
	else if (argc > 2)
	{
		SIprintf("%s: too many arguments\n", argv[0]);
	}
	else if ((errnum = NMzapIngAt(argv[1])) != OK)
	{
		SIprintf("%s: unable to unset variable \"%s\"\n", 
			argv[0], argv[1]);
		ERreport(errnum, errbuf);
		SIprintf("\t%s\n", errbuf);
		SIprintf("\tonly the %s super-user can use %s\n",
			SystemProductName, argv[0]);
	}
 
        /* Re-enable interrupts */
        EXinterrupt(EX_ON);

	PCexit( errnum ? FAIL : OK );
}
