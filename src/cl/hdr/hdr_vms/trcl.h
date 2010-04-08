/*
** Copyright (c) 1987, 2005 Ingres Corporation
**
*/

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
**	26-june-1992 (jnash)
**	    Add TR_F_APPEND.
**	7-jun-93 (ed)
**	    created glhdr version contain prototypes, renamed existing file
**	    to xxcl.h and removed any func_externs
**      13-may-1993 (walt)
**          Add Alpha definitions for TR_INPUT, TR_L_INPUT, TR_OUTPUT,
**          TR_L_OUTPUT that match the UNIX definitions because the Alpha
**          TR was taken from UNIX rather than from VMS.
**      26-jul-1993 (walt)
**          Add Alpha definitions for TR_INPUT, TR_L_INPUT, TR_OUTPUT,
**          TR_L_OUTPUT that match the UNIX definitions because the Alpha
**          TR was taken from UNIX rather than from VMS.
**	09-jun-2003 (devjo01) b110302
**	    Add TR_F_SAFEOPEN, TR_SAFE_STRING
**	02-may-2005 (devjo01)
**	    Add TR_NULLOUT, TR_L_NULLOUT to allow redirection of TRdisplay
**	    output to the NULL device.
**      19-dec-2008 (stegr01)
**          Itanium VMS port. Removed obsolete VAX declarations.
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

# define                        TR_OUTPUT       "stdio"
# define                        TR_L_OUTPUT     5
# define                        TR_INPUT        "stdio"
# define                        TR_L_INPUT      5

# define                        TR_NULLOUT       "NLA0:"
# define                        TR_L_NULLOUT     5


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
