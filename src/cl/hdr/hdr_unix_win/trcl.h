/*
** Copyright (c) 1985, 2005 Ingres Corporation
**
**
*/

#include <systypes.h>
# if !defined(__STDARG_H__)  && !defined(any_hpux) && !defined(any_aix)
#include    <stdarg.h>
#endif

/**CL_SPEC
** Name: TR.H - TR compatibility library definitions.
**
** Description:
**      This header contains the description of the types, constants and
**      function used to call TR routines.
**
** Specification:
**  
**  Description:
**      The TR routines format and write Trace information on the 
**      Trace output file(device) or files(devices) defined for 
**      a user session (thread) and accept trace input from the
**      input file(device). 
** 
**  Intended Uses:
**	The TR routines are used to format and display tracing information
**      on one or more files defined for each of the threads of a multi-threaded
**      server.  These routines allow each user to specify if he wants
**      the trace output to go to his home terminal only, to a log file 
**      only or to both.  These routines will also allow different devices
**      to be defined for each user (thread) of the server.
**      
**  Assumptions:
**	It is assumed that the DBMS code will use only the TR routines for
**      printing debugging and trace information.  It was also assumed that most
**      of the tracing calls would be within ifdef xDEBUG and ifdef xDEV_TEST
**      statements and would therefore not be left in the released code.
**      It was assumed that the formatting routines would be extensive
**      enough to format complicated control structures like control blocks,
**      and memory dumps.   For example it includes formatting commands for
**      symbolic representation of ordinals and bit masks as well as commands
**      to systematically step through arrays of structures or arrays of 
**      pointers to objects. (For detailed information see the Procedure
**      description for TRdisplay).  TR also includes routines to set and 
**      fetch trace flags.
**
**      For the input file(device), records can be the log output of a previous run.  
**      The input command ignores output data.  This allows the same trace input
**      options to be run and the resulting file diff'd or just used to setup
**      for bug fixing.
**
**  Definitions:
**
**  Concepts:
**
**	It is assumed that TRset_file is called before any TRdisplay or
**      TRrequest calls are made.  It is also assumed that TRmaketrace is
**      called prior to calling TRgettrace or TRsettrace.  
**    
**      The TRdisplay routine wraps long lines so that they are visible
**      on the screen or printer.  It also has SCF hooks for asynchronous 
**      I/O for running in a server.  
**
** History:
**      30-sep-1985 (derek)
**          Created new for 5.0.
**	11-may-89 (daveb)
**		Remove RCS log and useless old history.
**	13-jun-1991 (bryanp)
**	    Change TR_OUTPUT and TR_INPUT definitions for TRset_file() use.
**	27-may-1992 (jnash)
**	    Add TR_F_APPEND to append to end of existing file.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**	19-may-95 (emmag)
**	    Added DESKTOP to define, to enable TR_OUTPUT etc.
**	01-feb-2000 (marro11)
**	    Relocated the recent introduction of __STDARG_H__ from gl's cl.h
**	    to cl's trcl.h.  Once here, we're free to ifdef out it's inclusion 
**	    for several platforms (tr.c fails to compile).  Platform list:
**	    dg8_us5, dgi_us5, hp8_us5, hpb_us5, ris_us5, rs4_us5, and su4_u42.
**      23-Feb-2000 (hanal04) Bug 100334.
**          Extended the above change to include rmx_us5 and rux_us5.
**	09-jun-2003 (devjo01) b110302
**	    Add TR_F_SAFEOPEN, TR_SAFE_STRING
**	02-apr-2004 (somsa01)
**	    Include systypes.h to resolve compile problems on HP-UX.
**	03-may-2005 (devjo01)
**	    Add TR_NULLOUT, and TR_L_NULLOUT to support redirection of
**	    TRdisplay output to the NULL device.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.
**/

/*
**  Forward and/or External function references.
*/


/*
**  Defined Constants.
*/

/*  TR return status codes. */

#define                 TR_OK           0
#define			TR_BADCREATE	(E_CL_MASK + E_TR_MASK + 0x01)
#define                 TR_BADOPEN      (E_CL_MASK + E_TR_MASK + 0x02)
#define			TR_ENDINPUT	(E_CL_MASK + E_TR_MASK + 0x03)
#define			TR_BADPARAM	(E_CL_MASK + E_TR_MASK + 0x04)
#define			TR_BADREAD	(E_CL_MASK + E_TR_MASK + 0x05)
#define			TR_BADCLOSE	(E_CL_MASK + E_TR_MASK + 0x06)
#define			TR_NOT_OPENED	(E_CL_MASK + E_TR_MASK + 0x07)

/* Function modifier codes. */

#define			TR_F_OPEN	1
#define			TR_F_CLOSE	2
#define			TR_T_OPEN	3
#define			TR_T_CLOSE	4
#define			TR_I_OPEN	5
#define			TR_I_CLOSE	6
#define			TR_F_APPEND	7
#define			TR_F_SAFEOPEN	8
#define			TR_NOOVERRIDE_MASK	0x8000

/* Initial line of a "safe" trace log. */
#define			TR_SAFE_STRING	"Begin Trace Log at"
#define			TR_SAFE_STRING_LEN	18

/*
**  Known file names.  Known file can be passed to TRset_file to
**  cause the standard process output or the standard process input to
**  be opened.
*/

#ifdef	VMS

#define			TR_OUTPUT	"SYS$OUTPUT"
#define			TR_L_OUTPUT	10
#define			TR_INPUT	"SYS$INPUT"
#define			TR_L_INPUT	9
#define			TR_NULLOUT	"NLA0:"
#define			TR_L_NULLOUT	5

#endif	/* VMS */

#if defined (UNIX)

#define			TR_OUTPUT	"stdio"
#define			TR_L_OUTPUT	5
#define			TR_INPUT	"stdio"
#define			TR_L_INPUT	5
#define			TR_NULLOUT	"/dev/null"
#define			TR_L_NULLOUT	9

#endif	/* UNIX */

/*
** Name: TR_CONTEXT - Trace I/O context
**
** Description:
**      A block of storage used to hold TR context for trace input or
**	output.  This context is interpreted by the TR routines it has
**	no structure to the caller other than a array of characters.
**
** History:
**     30-sep-1985 (derek)
**          Created new for 5.0.
*/
typedef struct _TR_CONTEXT
{
    char            tr_data[16];        /* Data used by TR. */
}   TR_CONTEXT;
