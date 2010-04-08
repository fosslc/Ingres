# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<cs.h>
# include	<meprivate.h>
# include	<me.h>

/*
 *Copyright (c) 2004 Ingres Corporation
 *
 *	Name:
 *		ME.c
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		initialize ME global variables
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83 - (gb) written.
 *
 *		16-Dec-1986 -- (daveb) rworked for layering on malloc.
**	23-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**	08-feb-93 (pearl)
**	    Add #include of me.h.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      15-may-1995 (thoro01)
**          Added NO_OPTIM  hp8_us5 to file.
**      19-jun-1995 (hanch04)
**          remove NO_OPTIM, installed HP patch PHSS_5504 for c89
**	03-jun-1996 (canor01)
**	    Add synchronization variables for use with operating-system
**	    threads.
**	03-apr-1997 (canor01)
**	    Add GLOBALDEFs to acknowledge status of globals, and explicitly
**	    initialize them.
**  26-Aug-1997 (merja01/toumi01)
**      Change definition of MEpage_tid from i4  to CS_THREAD_ID.  
**      This is used to hold a thread ID and CS_THREAD_ID is
**      the proper generic definition for OS threads.  Under SYS V
**      this should be a thread_t and under POSIX threads a pthread_t.
**  24-May-2000 (hanal04) Bug 98820 INGSRV 1094
**      Introduced the MEalloctab_mutex to protect the MEalloctab.
**  25-Sep-2000 (hanal04) Bug 102725 INGSRV 1275.
**      Introduced the MEbrk_mutex to protect the sbrk() a.k.a. the
**      end of the MEalloctab.
**             
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
 */

GLOBALDEF ME_HEAD	MElist	     ZERO_FILL;
GLOBALDEF ME_HEAD	MEfreelist   ZERO_FILL;
GLOBALDEF i2		ME_pid	     = 0;
GLOBALDEF bool		MEsetup      = FALSE;
GLOBALDEF bool		MEgotadvice  = FALSE;
GLOBALDEF i4		MEadvice     = ME_USER_ALLOC;

# ifdef OS_THREADS_USED
GLOBALDEF CS_SYNCH	MEbrk_mutex ZERO_FILL;
GLOBALDEF CS_SYNCH	MEalloctab_mutex ZERO_FILL;
GLOBALDEF CS_SYNCH	MEfreelist_mutex ZERO_FILL;
GLOBALDEF CS_SYNCH	MElist_mutex ZERO_FILL;
GLOBALDEF CS_SYNCH	MEtaglist_mutex ZERO_FILL;
GLOBALDEF CS_SYNCH	MEpage_mutex ZERO_FILL;
GLOBALDEF CS_THREAD_ID	MEpage_tid ZERO_FILL;
GLOBALDEF i4		MEpage_recurse ZERO_FILL;
# endif /* OS_THREADS_USED */
