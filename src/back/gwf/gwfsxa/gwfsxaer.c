/*
**Copyright (c) 2004 Ingres Corporation
*/
# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <dudbms.h>
# include    <cm.h>
# include    <cs.h>
# include    <st.h>
# include    <er.h>
# include    <tm.h>
# include    <ulf.h>
# include    <adf.h>
# include    <scf.h>
# include    <dmf.h>
# include    <dmucb.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <gwf.h>
# include    <gwfint.h>
# include    "gwfsxa.h"

/*
** Name: GWFSXAER.C - Error handler for SXA gateway
**
** Description:
**	This file contains the error handler for  SXA gateway.
**
** History:
**	14-sep-92 (robf)
**	    Created
**	11-nov-92 (robf)
**	    Changed error handling from generic_error to sqlstate
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      21-May-1998 (hanch04)
**          Removed declaration of ule_format
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Rewrote as var arg function invoked by gwfsxa_error macro,
**	    use new uleFormat, prototyped
*/
/*
**	Name: gwsxaErrorFcn - Generic SXA gateway error printing routines
**
**	Inputs:
**		gwx_rcb		RCB for the session
**		err_code	Error number
**		err_type	Type of error
**	        FileName	whence the error occurred.
**		LineNumber
**		num_parm	Number of parms
**		parms		Parms to print
**
*/
/* VARARGS4 */
VOID
gwsxaErrorFcn(
GWX_RCB		*gwx_rcb,
i4		err_code,
i4		err_type,
PTR		FileName,
i4		LineNumber,
i4		num_parms,
... )
{
#define NUM_ER_ARGS	12 
    i4             uletype;
    i4		ulecode;
    DB_STATUS           uleret;
    DB_SQLSTATE		sqlstate;
    i4		msglen;
    DB_STATUS		ret_val = E_DB_OK;
    char		errbuf[1024];
    SCF_CB		scf_cb;
    DB_ERROR		localDBerror, *DBerror;
    i4			i;
    va_list		ap;
    ER_ARGUMENT		er_args[NUM_ER_ARGS];

    va_start( ap, num_parms );
    for ( i = 0; i < num_parms && i < NUM_ER_ARGS; i++ )
    {
        er_args[i].er_size = (i4) va_arg( ap, i4 );
	er_args[i].er_value = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    if ( !gwx_rcb || err_code )
    {
	DBerror = &localDBerror; 
	DBerror->err_file = FileName;
	DBerror->err_line = LineNumber;
	DBerror->err_code = err_code;
	DBerror->err_data = 0;

	/* Fill caller's dberror with that used */
	if ( gwx_rcb )
	    gwx_rcb->xrcb_error = *DBerror;
    }
    else
        DBerror = &gwx_rcb->xrcb_error;

    for (;;)	/* Not a loop, just gives a place to break to */
    {
	/*
	** Log internal errors and errors which the caller asked to be
	** logged.
	*/
	if ((err_type & SXA_ERR_INTERNAL) ||
	    (err_type & SXA_ERR_LOG))
	    uletype = ULE_LOG;
	else
	    uletype = 0L;

	/*
	** Get error message text.  
	*/
	uleret = uleFormat(DBerror, 0, NULL, uletype, &sqlstate, errbuf,
		(i4) sizeof(errbuf), &msglen, &ulecode, i,
		er_args[0].er_size, er_args[0].er_value,
		er_args[1].er_size, er_args[1].er_value,
		er_args[2].er_size, er_args[2].er_value,
		er_args[3].er_size, er_args[3].er_value,
		er_args[4].er_size, er_args[4].er_value,
		er_args[5].er_size, er_args[5].er_value,
		er_args[6].er_size, er_args[6].er_value,
		er_args[7].er_size, er_args[7].er_value,
		er_args[8].er_size, er_args[8].er_value,
		er_args[9].er_size, er_args[9].er_value,
		er_args[10].er_size, er_args[10].er_value,
		er_args[11].er_size, er_args[11].er_value );

	/*
	** If uleFormat failed, we probably can't report any error.
	** Instead, just propagate the error up to the user.
	*/
	if (uleret != E_DB_OK)
	{
		STprintf(errbuf, "SXA Gateway: Error message corresponding to %d (X%x) \
		    not found in INGRES error file", err_code, err_code);
		msglen = STlength(errbuf);
		uleFormat(NULL, 0L, NULL, ULE_MESSAGE|ULE_LOG, NULL, errbuf, msglen, 0,
		    &ulecode, 0);
	}
	/*
	**	Only send to session if USER or INTERNAL
	*/
	if(err_type & (SXA_ERR_INTERNAL|SXA_ERR_USER))
	{
		scf_cb.scf_length = sizeof(scf_cb);
		scf_cb.scf_type = SCF_CB_TYPE;
		scf_cb.scf_facility = DB_GWF_ID;
		scf_cb.scf_session = DB_NOSESSION;
		scf_cb.scf_nbr_union.scf_local_error = err_code;
		STRUCT_ASSIGN_MACRO(sqlstate,scf_cb.scf_aux_union.scf_sqlstate);
		scf_cb.scf_len_union.scf_blength = msglen;
		scf_cb.scf_ptr_union.scf_buffer = errbuf;
		ret_val = scf_call(SCC_ERROR, &scf_cb);
		if (ret_val != E_DB_OK)
		{
		    if (gwx_rcb!=NULL)
		    {
			gwx_rcb->xrcb_error.err_code= scf_cb.scf_error.err_code;
		    }
		    break;
		}
	}
	break;
    }

    return;
}
/* Non-variadic form of gwsxa_error, insert __FILE__, __LINE__ manually */
VOID
gwsxaErrorNV(
	 GWX_RCB *gwx_rcb,
	 i4 err_code,
         i4 err_type,
         i4 num_parms,
         ... )
{
    i4	 		i; 
    va_list 		ap;
    ER_ARGUMENT	        er_args[NUM_ER_ARGS];

    va_start( ap , num_parms );

    for( i = 0;  i < num_parms && i < NUM_ER_ARGS; i++ )
    {
        er_args[i].er_size = (i4) va_arg( ap, i4 );
	er_args[i].er_value = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    gwsxaErrorFcn(
		gwx_rcb,
	        err_code,
	        err_type,
	        __FILE__,
	        __LINE__,
	        i,
		er_args[0].er_size, er_args[0].er_value,
		er_args[1].er_size, er_args[1].er_value,
		er_args[2].er_size, er_args[2].er_value,
		er_args[3].er_size, er_args[3].er_value,
		er_args[4].er_size, er_args[4].er_value,
		er_args[5].er_size, er_args[5].er_value,
		er_args[6].er_size, er_args[6].er_value,
		er_args[7].er_size, er_args[7].er_value,
		er_args[8].er_size, er_args[8].er_value,
		er_args[9].er_size, er_args[9].er_value,
		er_args[10].er_size, er_args[10].er_value,
		er_args[11].er_size, er_args[11].er_value );
}

