/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: apitrace.h
**
** Description:
**	This file contains general definition of API tracing parameters
**      and functions.
**
** History:
**      02-nov-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
**	23-Jan-95 (gordy)
**	    Cleaned up error handling.
**	 9-Feb-95 (gordy)
**	    Made traces ordered rather than bit masks.
**	19-Jan-96 (gordy)
**	    Added global data structure.  Implement short functions as macros.
**	 2-Oct-96 (gordy)
**	    Replaced original SQL state machines which a generic
**	    interface to facilitate additional connection types.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-10 (gordy)
**	    Log DBMS trace messages.
*/

# ifndef __APITRACE_H__
# define __APITRACE_H__

/*
** Tracing is designed to be compile time selectable
** through the definition of IIAPI_DEBUG.  For the
** present, there does not seem to be a reason not
** to build with tracing capability.
*/

# define IIAPI_DEBUG

/*
** Name: API Traces
**
** Description:
**      These in-line functions invoke the trace functions to display
**      tracing information.
**
** History:
**      12-oct-93 (ctham)
**          creation
**      17-may-94 (ctham)
**          change all prefix from CLI to IIAPI.
*/

# ifdef IIAPI_DEBUG

# include <tr.h>

# define IIAPI_TRACE_LEVEL		\
	(IIapi_static ? IIapi_static->api_trace_level : (II_LONG)-1)

# define IIAPI_TRACE(level)	if ( (level) <= IIAPI_TRACE_LEVEL )  TRdisplay
# define IIAPI_DBGBUF		dbgBuf
# define IIAPI_DBGDECLARE	char dbgBuf[100]
# define IIAPI_INITTRACE()	IIapi_initTrace()
# define IIAPI_TERMTRACE()	IIapi_termTrace()
# define IIAPI_PRINTEVENT(a)	IIapi_printEvent(a)
# define IIAPI_PRINTSTATUS(a)	IIapi_printStatus(a)
# define IIAPI_PRINT_ID(a,b,c)	IIapi_printID((i4)a,(i4)b,c)
# define IIAPI_TRANID2STR(a,b)	IIapi_tranID2Str(a,b)

/* 
** Valid trace levels 
*/
# define IIAPI_TR_FATAL		0x0001
# define IIAPI_TR_ERROR		0x0002
# define IIAPI_TR_WARNING	0x0003
# define IIAPI_TR_TRACE		0x0004
# define IIAPI_TR_INFO		0x0005
# define IIAPI_TR_VERBOSE	0x0006
# define IIAPI_TR_DETAIL	0x0007

# else

# define IIAPI_TRACE(x)		if ( 0 )  IIapi_nopDisplay
# define IIAPI_DBGBUF		0   
# define IIAPI_DBGDECLARE
# define IIAPI_INITTRACE()
# define IIAPI_TERMTRACE()
# define IIAPI_PRINTEVENT(a)	0
# define IIAPI_PRINTSTATUS(a)	0
# define IIAPI_PRINT_ID(a,b,c)	0
# define IIAPI_TRANID2STR(a,b)	0
    
# endif
    
    
/*
** Global Functions.
*/
    
# ifdef IIAPI_DEBUG
    
II_EXTERN II_VOID	IIapi_initTrace( II_VOID );
II_EXTERN II_VOID	IIapi_termTrace( II_VOID );
II_EXTERN II_VOID	IIapi_printTrace( PTR, bool, char *, i4 );
II_EXTERN II_VOID	IIapi_flushTrace( II_VOID );
II_EXTERN char		*IIapi_printEvent( IIAPI_EVENT evntNo );
II_EXTERN char		*IIapi_printStatus( IIAPI_STATUS status );
II_EXTERN char		*IIapi_printID( i4  id, i4  id_cnt, char **ids );
II_EXTERN char		*IIapi_tranID2Str( IIAPI_TRAN_ID *tranID,char *buffer );

# else

II_EXTERN II_VOID	IIapi_nopDisplay( II_VOID );

# endif

# endif /* __APITRACE_H__ */
