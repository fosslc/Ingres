/*
**Copyright (c) 2004 Ingres Corporation
*/

# include   <compat.h>
# include   <gl.h>
# include   <st.h>
# include   <clconfig.h>
# include   <systypes.h>
# include   <er.h>
# include   <cs.h>      /* Needed for "erloc.h" */
# include   "erloc.h"
# include   <me.h>
# include   <errno.h>
#ifdef xCL_006_FCNTL_H_EXISTS
# include   <fcntl.h>
#endif /* xCL_006_FCNTL_H_EXISTS */
#ifdef xCL_007_FILE_H_EXISTS
# include   <sys/file.h>
#endif /* xCL_007_FILE_H_EXISTS */
# include <clsocket.h>

# define    SIMPLE                  's'
# define    COMPLEX                 'c'

/*
PROGRAM =   ercodeexam
**
NEEDLIBS =  COMPAT MALLOCLIB
*/


/*
**  Example program showing how to code with CL_ERR_DESC/ERslookup() on Unix.
**  Since this is only a 'test' program, this does not have to be compiled
**  successfully. If you cannot compile this, remove this from your dir,
**  mkming and then do mk.
**
**  Running this program with the argument `s' (for simple) gives:
**
**    Error opening the XX temporary file, "/some/unix/file".
**    open() failed with UNIX error 2 (No such file or directory)
**
**  Running it with `c' (for complex) gives:
**
**    XXcomplex: can't do my job.
**    Couldn't create socket, format=2, type=4, protocol=0
**    socket() failed with UNIX error 43 (Protocol not supported)
**
**  Both result from calling ERslookup twice, once for the externally
**  specified error returned by XXsimple() in the first case, XXcomplex()
**  in the second, and again for the internal error returned by those
**  routines in the CL_ERR_DESC passed in to them.  The second example
**  involves an internal "CL routine" failure (in XXsocket()). Both
**  examples have a Unix call failure underlying the error.
**  1-Mar-1989 (fredv)
**	include <systypes.h> and <fcntl.h>. Add ifdef xCL... from clconfig.h
**  19-Apr-89 (markd)
**	Moved socket.h inclusion to clsocket.h
**  25-jul-89 (rogerk)
**	Added generic error argument to ERlookup.
**  15-may-90 (blaise)
**	Added comment to emphasise that this is only a test program (change
**	integrated from 61).
**  26-oct-92 (andre)
**	replaced calls to ERlookup() with calls to ERslookup()
**  19-apr-1995 (canor01)
**	changed errno to errnum for reentrancy
**  03-sep-98 (toumi01)
**	Conditional const declaration of sys_errlist for Linux
**	systems using libc6 (else we get a compile error).
**  06-oct-99 (toumi01)
**	Change Linux config string from lnx_us5 to int_lnx.
**  20-nov-98 (stephenb)
**	Use strerror to compare error strings, Solaris 7 doesn't handle
**	direct comparisons.
**  14-jun-00 (hanje04)
**	Add ibm_lnx to conditional declaration of sys_errlist for systems
**	using libc6.
**  07-Sep-2000 (hanje04)
**	Added axp_lnx (Alpha Linux) to libc6 const declaration
**  04-Dec-2001 (hanje04)
**	Removed defn of sys_errlist[] as it's no longer used.
**    25-Oct-2005 (hanje04)
**        Add prototype for XXsocket() to prevent compiler
**        errors with GCC 4.0 due to conflict with implicit declaration.
**	  Only compile "dummy ERslookup()" if USE_DUMMY_ERSLOOKUP is defined.
*/

/* local prototypes */

static STATUS XXsocket(
		CL_ERR_DESC* errdesc );

main(argc, argv)
char **argv;
{
    int status = FAIL;

    MEadvise(ME_INGRES_ALLOC);

    if (argc == 2)
    {
        switch (*argv[1])
        {
            case SIMPLE:
            case COMPLEX:
                if(client(*argv[1]))
			break;
                status = OK;
        }
    }

    exit(status);
}


client(mode)
char mode;
{
    CL_ERR_DESC clerror, er_err;
    int status, junk;

    if (mode == SIMPLE)
        status = XXsimple(&clerror);
    else
        status = XXcomplex(&clerror);

    if (status != OK)
    {
        /* look up external CL error */
        (VOID) ERslookup(status, &clerror, 0, (char *) NULL, NULL, 0, -1, &junk,
			 &er_err, 0, NULL);

        /* look up internal CL error */
        (VOID) ERslookup(0, &clerror, 0, (char *) NULL, NULL, 0, -1, &junk, 
			&er_err, 0, NULL);
    }
    return(OK);
}


# define    XX_BADOPEN              1

/*
**  XXsimple is a public CL entry that has a simple implementation
**  and returns either OK or XX_BADOPEN.
*/
STATUS
XXsimple(errdesc)
CL_ERR_DESC* errdesc;
{
    char *filename = "/some/unix/file";

    if (open(filename, O_RDONLY) == -1)
    {
        SETCLERR(errdesc, 0, ER_open);
        errdesc->moreinfo[0].size = STlength(filename);
        STcopy(filename, errdesc->moreinfo[0].data.string);

        /*
        **  The message for XX_BADOPEN in erclf.msg:
        **
        **      E_CL_0000_XX_BADOPEN
        **      "Error opening the XX temporary file, \"%0c\"."
        */
        return (XX_BADOPEN);
    }

    return (OK);
}


# define    XX_CANT                 2

/*
**  XXcomplex is a public CL entry that has a hairy implementation
**  (most of the work is done in a private routine, XXsocket) and
**  returns either OK or XX_CANT.
*/
STATUS
XXcomplex(errdesc)
CL_ERR_DESC* errdesc;
{
    int status;

    if ((status = XXsocket(errdesc)) != OK)
    {
        errdesc->intern = status;   /* or could let XXsocket() set */

        /*
        **  The message for XX_CANT in erclf.msg:
        **
        **      E_CL_0000_XX_CANT
        **      "XXcomplex: can't do my job."
        */
        return (XX_CANT);
    }

    return (status);
}


# define    XX_PRIVATE_RESOURCELIM  3
# define    XX_PRIVATE_BADSOCKOP        4
# define    XX_PRIVATE_DUNNO        5

/*
**  XXsocket interacts with the operating system, and returns
**  one of OK, XX_PRIVATE_RESOURCELIM, XX_PRIVATE_BADSOCKOP,
**  or XX_PRIVATE_DUNNO, which are internal CL error codes.
*/
static STATUS
XXsocket(errdesc)
CL_ERR_DESC* errdesc;
{
    extern int errno;
    STATUS status = OK;
    int s;

    if ((s = socket(AF_INET, SOCK_RDM, 0)) < 0) /* generates an error */
    {
        switch (errno)
        {
            case EMFILE:
            case ENOBUFS:
                status = XX_PRIVATE_RESOURCELIM;
                break;
            case ESOCKTNOSUPPORT:
            case EPROTONOSUPPORT:
                /* This is the error case in our example. */
                status = XX_PRIVATE_BADSOCKOP;
                errdesc->moreinfo[0].size = errdesc->moreinfo[1].size =
                    errdesc->moreinfo[2].size = sizeof(int);

                errdesc->moreinfo[0].data._nat = AF_INET;
                errdesc->moreinfo[1].data._nat = SOCK_RDM;
                errdesc->moreinfo[2].data._nat = 0;
                break;
            default:
                status = XX_PRIVATE_DUNNO;
                break;
        }

        SETCLERR(errdesc, 0 /* or could set here */, ER_socket);

        /*
        **  The message for XX_PRIVATE_BADSOCKOP in erclf.msg:
        **
        **      E_CL_0000_XX_PRIVATE_BADSOCKOP
        **      "Couldn't create socket, format=%0d, type=%1d, protocol=%2d"
        */
    }

    return (status);
}

/*
**                  YOU NEEDN'T READ ANY FURTHER.
**-----------------------------------------------------------------------------
** This is a dummy version of ERslookup() for illustrative purposes only.
*/
# ifdef USE_DUMMY_ERSLOOKUP
static int level = -1;

static STATUS
ERslookup(msg_number, clerror, flags, sqlstate, msg_buf, msg_buf_size, 
        language, msg_length, err_code, num_param, param)
i4         msg_number;
CL_ERR_DESC     *clerror;
i4              flags;
char            *msg_buf;
char		*sqlstate;
i4              msg_buf_size;
i4              language;
i4              *msg_length;
CL_ERR_DESC     *err_code;
i4              num_param;
ER_ARGUMENT     *param;
{
    ER_ARGUMENT hidden[CLE_INFO_ITEMS]; /* to access info in clerror */
    char emsg[64], imsg[64];
    i4  i;

    switch (msg_number)
    {
        case XX_BADOPEN:
            STcopy("XX_BADOPEN", emsg);
            break;
        case XX_CANT:
            STcopy("XX_CANT", emsg);
            break;
        case XX_PRIVATE_RESOURCELIM:
            STcopy("XX_PRIVATE_RESOURCELIM", emsg);
            break;
        case XX_PRIVATE_BADSOCKOP:
            STcopy("XX_PRIVATE_BADSOCKOP", emsg);
            break;
        case XX_PRIVATE_DUNNO:
            STcopy("XX_PRIVATE_DUNNO", emsg);
            break;
        case ER_UNIXERROR:
            STcopy("ER_UNIXERROR", emsg);
            break;
        case 0:
            STcopy("0", emsg);
    }

    /*
    ++level;
    for (i = 0; i < level;++i)
        printf("    ");

    printf("ERslookup(%s, %x, %x, .., ..., ..., ..., ..., %d, %x)\n",
        emsg, clerror, flags, num_param, param);
    */

    if (clerror)
    {
        switch (clerror->intern)
        {
            case XX_PRIVATE_RESOURCELIM:
                STcopy("XX_PRIVATE_RESOURCELIM", imsg);
                break;
            case XX_PRIVATE_BADSOCKOP:
                STcopy("XX_PRIVATE_BADSOCKOP", imsg);
                break;
            case XX_PRIVATE_DUNNO:
                STcopy("XX_PRIVATE_DUNNO", imsg);
                break;
            case 0:
                STcopy("0", imsg);
        }

        if (msg_number) /* Look up message after system error */
        {
            for (i = 0; i < CLE_INFO_ITEMS; ++i)
            {
                hidden[i].er_value = (PTR) &clerror->moreinfo[i].data._i4;
                hidden[i].er_size = clerror->moreinfo[i].size;
            }

            param = &hidden[0];
        }
        else        /* retrieve system-dependent error messages */
        {
            i4          junk;
            ER_ARGUMENT argv[3];

            if (clerror->intern)    /* look up internal CL error */
            {
                for (i = 0; i < CLE_INFO_ITEMS; ++i)
                {
                    argv[i].er_value = 
				(PTR) &clerror->moreinfo[i].data._i4;
                    argv[i].er_size = clerror->moreinfo[i].size;
                }

                (VOID) ERslookup((i4) clerror->intern,
                (CL_ERR_DESC*) NULL, flags & ~ER_TIMESTAMP | ER_TEXTONLY, 
		(char *) NULL, NULL, 0, -1, &junk, err_code, CLE_INFO_ITEMS, 
		argv);
            }

            if (clerror->callid) /* look up system error message text */
            {
                argv[0].er_size = argv[1].er_size = argv[2].er_size =
                ER_PTR_ARGUMENT;

                argv[0].er_value = (PTR) &clerror->errnum;
                argv[1].er_value = (PTR) ERnames[clerror->callid];
                argv[2].er_value = (PTR) strerror(errno);

                (VOID) ERslookup(ER_UNIXERROR, (CL_ERR_DESC*) NULL,
                        flags & ~ER_TIMESTAMP | ER_TEXTONLY, (char *) NULL, 
			NULL, 0, -1, &junk, err_code, 3, argv);
            }

            return (OK);
        }
    }

    --level;

    switch (msg_number)
    {
        case XX_BADOPEN:
            printf("Error opening the XX temporary file, \"%s\".\n",
                clerror->moreinfo[0].data.string);
            break;
        case XX_CANT:
            printf("XXcomplex: can't do my job.\n");
            break;
        case XX_PRIVATE_BADSOCKOP:
            STcopy("XX_PRIVATE_BADSOCKOP", emsg);
            printf("Couldn't create socket, format=2, type=4, protocol=0\n");
            break;
        case ER_UNIXERROR:
            STcopy("ER_UNIXERROR", emsg);
            if (*((int*)(param[0].er_value)) == 2)
            printf(
            "open() failed with UNIX error 2 (No such file or directory)\n");
            else
            printf(
            "socket() failed with UNIX error 43 (Protocol not supported)\n");
            break;
    }

    return (0);
}

# endif /* USE_DUMMY_ERSLOOKUP */
