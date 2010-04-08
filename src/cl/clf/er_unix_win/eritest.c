/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <er.h>
#include    <si.h>
#include    <lo.h>
#include    <st.h>
#include    <pc.h>


/*
PROGRAM =	eritest
**
NEEDLIBS =	COMPATLIB MALLOCLIB
*/


/*
** History:
**	15-may-90 (blaise)
**	    Integrated changes from 61 and ug:
**		Replace literal loop count with the macro MAX_NUM_OF_LOOKUPS;
**		Exit if EOF on input instead of looping.
**	26-oct-92 (andre)
**	    replaced calls to ERlookup() with calls to ERslookup()
**	28-jul-92 (daveb)
**	    Make the prompt a bit more informative.  all input must be 
**	    in hex, but without a leading 0x.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	04-mar-1997 (canor01)
**	    Add '-o' flag to test output to operator (system/event log).
**	23-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	18-jun-1999 (canor01)
**	    Add '-n' flag to test output with no parameter substitution.
**      28-mar-2000 (hweho01)
**          Changed %p to %x for AIX 64 bit platform (ris_u64).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Sep-2002 (hweho01)
**          Added change for hybrid build on AIX platform.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
*/

#if defined(su4_u42)
#define       MAX_NUM_OF_LOOKUPS      50
#else
#define       MAX_NUM_OF_LOOKUPS      20
#endif /* su4_u42 */

ER_ARGUMENT  er_arg[] =
{
    {(i4)4, (PTR)"abcd"},
    {(i4)4, (PTR)"ABCD"},
    {(i4)4, (PTR)"abcd"},
    {(i4)4, (PTR)"abcd"},
    {(i4)4, (PTR)"AAAA"}
};

int
main(int c, char **v)
{
    i4		msg_id;
    i4		cnt = 0;
    char	mbuf[ER_MAX_LEN];
    i4		ret_len = 0;
    char	msgbuf[101];
    CL_ERR_DESC	clerror;
    CL_ERR_DESC	err_code;
    STATUS	stat;
    i4		flag = 0;
    bool	testsysdep = FALSE;
    bool	logoper = FALSE;

    MEadvise(ME_INGRES_ALLOC);

    for (++v, --c; c > 0; ++v, --c)
    {
	char *w = *v;
	if (w[0] == '-')
	{
	    switch (w[1])
	    {
	        case 't':
		    flag |= ER_TIMESTAMP;
		    break;
	        case 'n':
		    flag |= ER_NOPARAM;
		    break;
	        case 's':
		    testsysdep = TRUE;
		    break;
		case 'o':
		    logoper = TRUE;
		    break;
	        default:
		    SIfprintf(stderr, "only valid options are -t, -s, -n and -o\n");
		    PCexit(FAIL);
	    }
	}
	else
	    SIfprintf(stderr, "ignoring invalid argument \"%s\"\n", v);
    }

    if (testsysdep)    /* test system dependent error reporting */
    {
        extern int    errno;

	errno = 13;    /* EACCES */
	SETCLERR(&clerror, 0x10911, /* ER_INTERR_TESTMSG */  ER_open);
	STcopy("II_MSGDIR/erclf.msg", clerror.moreinfo[0].data.string);
	clerror.moreinfo[0].size = STlength(clerror.moreinfo[0].data.string);
    }

    while ((cnt++) < MAX_NUM_OF_LOOKUPS)
    {
	msg_id = -1;
	SIfprintf(stderr, "msg id, in hex, no 0x%s: ",
	    testsysdep? " (try 10904) " : "");
	if( OK != SIgetrec(msgbuf, 100, stdin) )
           break;
#if defined(any_aix) && defined(BUILD_ARCH64)
	(VOID) STscanf(msgbuf, "%x", &msg_id);
#else
	(VOID) STscanf(msgbuf, "%p", &msg_id);
#endif

	if (msg_id == -1)
	    break;

	if ((stat = ERslookup(msg_id, testsysdep? &clerror : NULL, flag, 
	    (char *) NULL, mbuf, ER_MAX_LEN, -1, &ret_len, &err_code, 5, 
	    er_arg)) != OK)
	{
	    SIfprintf(stderr,
		"ERslookup error. msg id is 0x%x, status is 0x%x\n",
		msg_id, stat);
	}
	else
	{
	    SIfprintf(stderr, "*** ERslookup:\n%s\n", mbuf);
            SIfprintf(stderr, "*** ERget:\n%s\n", ERget(msg_id));
	}

	if (testsysdep)
	{
	    if ((stat = ERslookup(0, &clerror, flag, (char *) NULL, mbuf, 
		ER_MAX_LEN, -1, &ret_len, &err_code, 5, er_arg)) != OK)
	    {
	        SIfprintf(stderr,
		    "ERslookup error on CL_ERR_DESC. Status is 0x%x\n", stat);
	    }
	    else
	        SIfprintf(stderr, "*** ERslookup (CL error):\n%s\n",mbuf);
        }
	
	if (logoper)
	{
	    ERsend( ER_ERROR_MSG|ER_OPER_MSG, mbuf, ret_len, &err_code );
	}
    }
    PCexit(OK);
}
