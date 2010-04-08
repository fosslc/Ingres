/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ULX.H - control block for ULX general exception handler routines
**
** Description:
**      This file contains the request control block to be used to make calls
**      to the general exception handler routines (ULX).
**
** History: 
**      29-aug-1992 (rog)
**          Created.
**	25-oct-92 (andre)
**	    replaced generic_error with sqlstate in the declaration of
**	    ulx_sccerror()
**      03-jun-1993 (rog)
**          Added prototype for ulx_format_query().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN STATUS	ulx_exception( EX_ARGS *args, DB_FACILITY facility,
				       i4 fac_error, bool dump_session );
FUNC_EXTERN VOID	ulx_rverror( char *errorid, DB_STATUS status,
				     DB_ERRTYPE error, DB_ERRTYPE foreign,
				     ULF_LOG logmessage, DB_FACILITY facility );
FUNC_EXTERN DB_STATUS	ulx_sccerror( DB_STATUS status, DB_SQLSTATE *sqlstate,
				      DB_ERRTYPE error, char *msg_buffer,
				      i4  msg_length, DB_FACILITY facility );
FUNC_EXTERN VOID	ulx_format_query( char *inbuf, i4  inlen, char *outbuf,
					  i4  outlen );
