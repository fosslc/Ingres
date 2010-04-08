/*
**    Copyright (c) 1987, 2001 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<cm.h>
# include	<er.h>
# include       <ex.h>
# include	<lo.h>
# include	<nm.h>
# include	<si.h>
# include	<st.h>
# include	<pc.h>
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
**	27-may-97 (mcgem01)
**		Clean up compiler warnings.
**	27-nov-1997 (canor01)
**		Allow for the variable value to be enclosed in double quotes
**		so pathnames with embedded spaces can be set.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	07-feb-2001 (somsa01)
**	    Changed type of "len" and "qlen" to SIZE_TYPE.
**	18-nov-2002 (gupsh01)
**	    Modified ingsetenv to prompt for values if less are provided.
**	13-jan-2004 (somsa01)
**	    Properly turn off exceptions.
**	03-feb-2004 (somsa01)
**	    Backed out last changes for now.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**/

/*
**		
NEEDLIBS = COMPAT 
**
UNDEFS =	II_copyright
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
**	07-feb-2001 (somsa01)
**	    Changed type of "len" and "qlen" to SIZE_TYPE.
**	05-nov-2002 (gupsh01)
**	    Modified ingsetenv to prompt for input parameters
**	    when input is less than 3 parameters. Also fixed the
**	    case when the value string consists of a quoted string.
**	    and added call to CMset_locale to set the character set
**	    locale to user's specified characterset.
**	18-nov-2002 (gupsh01)
**	    If one argument is provided then modified only to prompt 
**	    for value. The first argument is interpreted as var name.
**	11-May-2007 (drivi01)
**	    Fixed "if (name_ok = TRUE)" condition. Condition requires
**	    double quotes otherwise it's always true and may result
**	    in SEGV.
*/
main(argc, argv)
i4	argc;
char	**argv;
{
	char		errbuf[ER_MAX_LEN];
	register i4 	errnum = FAIL;
	char		tmpbuf[MAXLINE];
	char		*nameptr = argv[1];
	char		namebuf[MAXLINE];
	char		valuebuf[MAXLINE];
	char		*startquote;
	char		*endquote;
	char		*srcptr = argv[2];
	char		*dstptr;
        bool		name_ok = FALSE;
        bool		arg_ok = FALSE;
 
	CMset_locale(); /* set the locale to II_CHARSET locale */

	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );

        /* Disable interrupts during ingsetenv */
        EXinterrupt(EX_OFF);

	if (argc > 3)
	{
          SIprintf("%s: too many arguments\n", argv[0]);
        }
	else if (argc < 3)
	{
	  if (argc == 2)
	  {
	    nameptr = argv[1];
	    name_ok = TRUE;
	  }
	  else
	  {
 	    /* prompt the user for input */
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
	        arg_ok = TRUE;
	        srcptr = valuebuf;
	      }	
	      else 
		SIprintf("%s: invalid arguments\n", argv[0]);
    	    }	
          } 
        }
	else
	{	
	    srcptr = argv[2];
	    arg_ok = TRUE;
	}

        if (arg_ok == TRUE)
        {
	  dstptr = tmpbuf;
	  /* remove matched double quotes */
	  if ( (startquote = STindex( srcptr, DQUOTESTR, 0 )) != NULL )
	  {
		SIZE_TYPE	len;
		SIZE_TYPE	qlen;
		SIZE_TYPE	slen;
		SIZE_TYPE	plen = 0;

		qlen = STlength(DQUOTESTR);
		slen = STlength(srcptr);	

		len = startquote - srcptr;
		MECOPY_VAR_MACRO( srcptr, len, dstptr );
		dstptr += len;
		srcptr += len + qlen;
		plen += len + qlen;

		while ( srcptr && (plen < slen) )
		{
		    if ((endquote = STindex( srcptr+qlen, 
					     DQUOTESTR, 0)) != NULL)
		    {
			len = endquote - srcptr;
			MECOPY_VAR_MACRO( srcptr, len, dstptr );
			dstptr += len;
			srcptr += len + qlen;
			plen += len + qlen;
		    }
		    else
		    {
			len = STlength( srcptr );
			MECOPY_VAR_MACRO( srcptr, len, dstptr );
			dstptr += len;
			srcptr += len;
			plen += len; 
		    }
		}
		*dstptr = '\0';
	    }
	    else
	    {
		STcopy( srcptr, tmpbuf );
	    }
	    if ((errnum = NMstIngAt(nameptr, tmpbuf)) != OK)
	    {
		SIprintf("%s: unable to set variable \"%s\"\n",
			argv[0], nameptr);
		ERreport(errnum, errbuf);
		SIprintf("\t%s\n", errbuf);
		SIprintf("\tonly the %s super-user can use %s\n",
			SystemProductName, argv[0]);
	    }
	}

        /* Re-enable interrupts */
        EXinterrupt(EX_ON);

	PCexit( errnum ? FAIL : OK );
}
