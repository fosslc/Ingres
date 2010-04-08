/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	sc0e.h
**
** Description:
**	Func externs for sc0e clients.
**
** History:
**	2-Jul-1993 (daveb)
**	    created.
**	11-oct-93 (swm)
**	    Bug #56448
**	    Altered sc0e_trace calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to sc0e_trace.
**	    This works because STprintf is a varargs function in the CL.
**	    Defined SC0E_FMTSIZE (as 80) for maximum format buffer size,
**	    value taken from hard-wired value in sc0e_trace function.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      23-Apr-2001 (hweho01) 
**          Changed sc0e_put() to varargs function, avoid stack
**          corruptions during runtime.
**      12-Sep-2007 (hanal04) Bug 119107
**          Changed SC0E_FMTSIZE to 256 bytes. On 64-bit platforms 80
**          characters was insufficient and lead to stack corrution in
**          SIdofrmt().
**	10-Sep-2008 (jonj)
**	    SIR 120874: Rename sc0e_put() to sc0ePutFcn(), called by
**	    sc0ePut, sc0e_put, sc0e_0_put macros.
**	    Made sc0e_0_uput a macro passing 0 parms to sc0e_uput.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**/

/* Used where function pointer needed and __FILE__, __LINE__ unknown */
FUNC_EXTERN VOID sc0e_putAsFcn(i4 err_code,
                          CL_ERR_DESC *os_error,
                          i4 num_params,
                          ...);

FUNC_EXTERN VOID sc0ePutFcn(
			  DB_ERROR *dberror,
			  i4	err_code,
                          CL_ERR_DESC *os_error,
			  PTR FileName,
			  i4  LineNumber,
                          i4 num_params,
                          ...);

#ifdef HAS_VARIADIC_MACROS

/* The preferred macro form, including dberror */
#define sc0ePut(dberror,err_code,os_error,...) \
	    sc0ePutFcn(dberror,err_code,os_error,__FILE__,__LINE__,__VA_ARGS__)

/* Bridge forms, excluding dberror */
#define	sc0e_put(err_code,os_error,...) \
	    sc0ePutFcn(NULL,err_code,os_error,__FILE__,__LINE__,__VA_ARGS__)
#else

/* Variadic macros not supported */
#define sc0ePut sc0ePutNV

FUNC_EXTERN VOID sc0ePutNV(
			  DB_ERROR *dberror,
			  i4	err_code,
                          CL_ERR_DESC *os_error,
                          i4 num_params,
                          ...);

#define sc0e_put sc0e_putAsFcn

#endif /* HAS_VARIADIC_MACROS */

#define	sc0e_0_put(err_code,os_error) \
	    sc0e_put(err_code,os_error,0)

FUNC_EXTERN VOID sc0e_uput(i4 err_code,
			   CL_ERR_DESC *os_error,
			   i4  num_params,
			   ...);

#define sc0e_0_uput(err_code,os_error) \
	    sc0e_uput(err_code,os_error,0)

FUNC_EXTERN VOID sc0e_trace( char *fmt );
#define	SC0E_FMTSIZE	256

FUNC_EXTERN i4 sc0e_tput( PTR arg1, i4 msg_length, char *msg_buffer );
