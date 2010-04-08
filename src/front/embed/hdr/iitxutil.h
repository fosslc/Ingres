/*--------------------------- iitxutil.h -----------------------------------*/
/* Copyright (c) 2004 Ingres Corporation */
/*
** Name: iitxutil.h
**
** Description:
**	Header file for Tuxedo bridge contains entry points for 
**      tuxedo-specific functions
**
** History:
**	01-Oct-93	(mhughes)
**	First Implementation
**	16-nov-93 (larrym)
**	    added IItux_build_as_service_name
**	03-Dec-1993 (mhughes)_
**	    Added IItux_userlog
**	17-Dec-1993 (mhughes)
**	    Check return statuses from convert functions
**	07-Dec-1993 (mhughes)
**	    Add argument to IItux_userlog call.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

#ifndef IITXUTIL_H
#define IITXUTIL_H

/*
** General LIBQTXXA routines
*/

FUNC_EXTERN DB_STATUS
IItux_fmt_server_id( i4  server_id, char *server_id_str );

FUNC_EXTERN DB_STATUS
IItux_unfmt_server_id( char *server_id_str, i4  *server_id );

FUNC_EXTERN VOID
IItux_get_server_group( char *server_group );

/*
**
**  Name:       IItux_build_as_service_name
**
**  Description:
**      build the TUXEDO server name from the xa_call, the group ID, and the
**      SVN.
**
**  Inputs:
**      as_svc_code             - the type of AS service to call
**      SVN                     - the server number of the AS to call.
**
**  Outputs:
**      iitux_as_service_name   - the tpadvertised name of the service to call
**
**      Returns:
**          E_DB_OK             - normal execution.
**          E_DB_ERROR          - input args invalid
**
**  Side Effects:
**      None.
**
**  Notes:
**      Refer to the TUXEDO Bridge Arch and Design spec for the algorithm
**      description and rationale.
**
**  History:
**      08-nov-1993 (larrym)
**          written.
**      16-nov-1993 (larrym)
**          modified to be callable from the ICAS as well as the TMS.
*/
FUNC_EXTERN DB_STATUS
IItux_build_as_service_name ( char      *iitux_as_svc_name,
			      i4        as_svc_code,
			      i4        SVN);



/*
**   Name: IItux_userlog()
**
**   Description: 
**      Given an AS id as an integer, converts it to hex and ships
**	back as a two char string.
**      Assumes output buffer large enough to take name string
**	(8 chars) to prevent overwrite errors in CVlx .
**   Inputs:
**	tp_function	i4	Action code for boot/shutdown/tpcall/tpreturn
**	service_name	char*	Name of service (If applicable)
**	tpreturn_status i4	Return status from service
**	tpcall_flags	i4	Flags used in making tpcall
**
**   Outputs:
**	None to code
**      Status message written to tuxedo userlog file.
**
** 	Returns: Nothing. (Not critical if this fails)
**
**   History:
**      03-Dec-1993 (mhughes)
**          First written
*/
VOID
IItux_userlog( i4	tp_function,
	       char	*service_name,
	       i4	tpreturn_status, 
	       i4	tpcall_flags,
	       PTR	ip_buffer,
	       PTR	*ret_buffer );



FUNC_EXTERN VOID
IItux_error(                           /* VARARGS */
/*                         i4   generic_errno,
                           i4   iitx_errno,
                           i4        err_param_count,
                           PTR       err_param1,
                           PTR       err_param2,
                           PTR       err_param3,
                           PTR       err_param4,
                           PTR       err_param5    */);


/*
** LIBQTXXA for Tuxedo processing for an TMS
*/

/* Flag that this process is an icas */

FUNC_EXTERN  bool  
IItux_is_tms();

/*
** LIBQTXXA for Tuxedo processing for an AS
*/

/* Flag that this process is an icas */

FUNC_EXTERN  bool  
IItux_is_as();

/*
** LIBQTXXA for Tuxedo processing for an ICAS
*/

/* Flag that this process is an icas */

FUNC_EXTERN  bool  
IItux_is_icas();

#endif /* ifndef IITXUTIL_H */
/*--------------------------- iitxutil.h -----------------------------------*/









