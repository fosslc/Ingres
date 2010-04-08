/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <er.h>
#include    <gl.h>
#include    <lo.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cs.h>
#include    <scf.h>

#include    <ursm.h>

/**
**
**  Name: URSREPOS.C - Repository interface
**
**  Description:
**	    URS_READREPOSITORY - 
** 		Read the repository definitions and build control blocks
**
**  History:
** 	29-may-1998 (wonst02)
** 	    Original: Moved code from ursinit.c to ursrepos.c
**	10-Aug-1998 (fanra01)
**	    Fixed incomplete line problem.
**/

/*{
** Name: URS_READREPOSITORY - 
** 		Read the repository definitions and build control blocks
**
** Description:
**
** Inputs:
**      ursm				The URS Manager Control Block
** 	ursb				User Request Services Block
**
** Outputs:
**	Returns:
** 		E_DB_OK			Data dictionary read OK
** 		E_DB_WARN		No application/interface/method defined
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	29-may-1998 (wonst02)
** 	    Moved code from ursinit.c to ursrepos.c
*/
DB_STATUS
urs_readrepository(URS_MGR_CB	*ursm,
		   URSB		*ursb)
{
	FAS_SRVR	*fas = ursm->ursm_srvr; /* Ptr to the appl srvr block */
	ULM_RCB		*ulm = &fas->fas_srvr_ulm_rcb; /* ULM Request block */
	FAS_APPL	appl ZERO_FILL;		/* Temp application block */
	FAS_APPL	**applptr;		/* Ptr to application blk ptr */
	FAS_APPL	*curappl;		/* Current application block */
	FAS_INTFC	intfc ZERO_FILL;	/* Temp interface blk */
	FAS_INTFC	**intfcptr;		/* Temp ptr to intfc blk ptr */
	FAS_INTFC	*curintfc;	        /* Current interface block */
	FAS_METHOD	method ZERO_FILL;		/* Temp method block */
	FAS_METHOD	**methodptr;		/* Ptr to method block ptr */
	FAS_METHOD	*curmethod;		/* Current method block */
	FAS_ARG		arg ZERO_FILL;		/* Temp argument block */
	FAS_ARG		**argptr;		/* Ptr to argument block ptr */
	DB_STATUS	status = E_DB_OK;

	/*
	** Allocate & init memory for all the application blocks.
	*/
	appl.fas_appl_srvr = fas;
	for (applptr = &fas->fas_srvr_appl; ;
	     applptr = &(**applptr).fas_appl_next)
	{
	    /*
	    ** Get next application description
	    */
	    status = urd_get_appl(ursm, &appl, ursb);
	    if ( DB_FAILURE_MACRO(status) )
	    {	
		SET_URSB_ERROR(ursb, E_UR0131_SRVR_STARTUP, E_DB_ERROR)
		return E_DB_ERROR;
	    }
	    if ( status == E_DB_INFO )	/* No more applications */
		break;
	    /* 
	    ** Allocate memory and copy information for this application block
	    */
	    *applptr = (FAS_APPL *)urs_palloc(ulm, ursb, sizeof(FAS_APPL));
	    if (*applptr == NULL)
		return E_DB_ERROR;
	    STRUCT_ASSIGN_MACRO(appl, **applptr);
	}
	if (fas->fas_srvr_appl == NULL)
	    return E_DB_WARN;

	curappl = fas->fas_srvr_appl;
	intfcptr = &curappl->fas_appl_intfc;
	do
	{
	    /*
	    ** Get next interface description
	    */
	    status = urd_get_intfc(ursm, &intfc, ursb);
	    if ( DB_FAILURE_MACRO(status) )
		return E_DB_ERROR;
	    if ( status == E_DB_INFO )	/* No more interfaces */
		break;
	    if (intfc.fas_intfc_appl_id != curappl->fas_appl_id)
	    {	
		curappl = urs_get_appl(ursm, intfc.fas_intfc_appl_id);
		if (curappl == NULL)
		{
	    	    urs_error(E_UR0730_URS_APPL_NOT_FOUND, URS_INTERR, 2,
		      	      sizeof intfc.fas_intfc_appl_id, 
			        &intfc.fas_intfc_appl_id,
	    	      	      sizeof intfc.fas_intfc_name.db_name,
		      		intfc.fas_intfc_name.db_name);
		    return E_DB_ERROR;
		}
	    	intfcptr = &curappl->fas_appl_intfc;
	    }
	    intfc.fas_intfc_appl = curappl;
	    /*
	    ** Allocate memory and copy information for this interface block 
	    */
	    *intfcptr = (FAS_INTFC *)urs_palloc(ulm, ursb, sizeof(FAS_INTFC));
	    if (*intfcptr == NULL)
	    	return E_DB_ERROR;
	    STRUCT_ASSIGN_MACRO(intfc, **intfcptr);
	    intfcptr = &(**intfcptr).fas_intfc_next;
	} while (1);

	curintfc = fas->fas_srvr_appl->fas_appl_intfc;
	if (curintfc == NULL)
	    return E_DB_WARN;
	methodptr = &curintfc->fas_intfc_method;
	do
	{
	    /*
	    ** Get next method description
	    */
	    status = urd_get_method(ursm, &method, ursb);
	    if ( DB_FAILURE_MACRO(status) )
	        return E_DB_ERROR;
	    if ( status == E_DB_INFO )	/* No more methods */
	        break;
	    if (method.fas_method_appl_id  != curintfc->fas_intfc_appl_id ||
	    	method.fas_method_intfc_id != curintfc->fas_intfc_id)
	    {
	    	curintfc = urs_find_intfc(ursm, method.fas_method_appl_id,
					  method.fas_method_intfc_id);
		if (curintfc == NULL)	  
		{
	    	    urs_error(E_UR0721_URS_INTFC_NOT_FOUND, URS_INTERR, 3,
			      sizeof method.fas_method_appl_id, 
				&method.fas_method_appl_id,
			      sizeof method.fas_method_intfc_id, 
				&method.fas_method_intfc_id,
		    	      sizeof method.fas_method_name.db_name,
		      		method.fas_method_name.db_name);
		    return E_DB_ERROR;
		}
		methodptr = &curintfc->fas_intfc_method;
	    }
	    method.fas_method_intfc = curintfc;
	    /*
	    ** Allocate memory and copy information for this method block 
	    */
	    *methodptr = (FAS_METHOD *)urs_palloc(ulm, ursb, sizeof(FAS_METHOD));
	    if (*methodptr == NULL)
		return E_DB_ERROR;
	    STRUCT_ASSIGN_MACRO(method, **methodptr);
	    methodptr = &(**methodptr).fas_method_next;
	} while(1);

	curmethod = fas->fas_srvr_appl->fas_appl_intfc->fas_intfc_method;
	if (curmethod == NULL)
	    return E_DB_WARN;
	argptr = &curmethod->fas_method_arg;
	do
	{
	    /*
	    ** Get next argument description
	    */
	    status = urd_get_arg(ursm, &arg, ursb);
	    if ( DB_FAILURE_MACRO(status) )
	       	return E_DB_ERROR;
	    if (status == E_DB_INFO)	/* No more arguments */
	       	break;
	    if (arg.fas_arg_appl_id   != curmethod->fas_method_appl_id ||
	    	arg.fas_arg_intfc_id  != curmethod->fas_method_intfc_id ||
	    	arg.fas_arg_method_id != curmethod->fas_method_id)
	    {
	    	curmethod = urs_find_method(ursm, arg.fas_arg_appl_id,
					    arg.fas_arg_intfc_id,
					    arg.fas_arg_method_id);
		if (curmethod == NULL)	  
		{
	    	    urs_error(E_UR0715_URS_METHOD_NOT_FOUND, URS_INTERR, 4,
			      sizeof arg.fas_arg_appl_id, 
				&arg.fas_arg_appl_id,
			      sizeof arg.fas_arg_intfc_id, 
				&arg.fas_arg_intfc_id,
			      sizeof arg.fas_arg_method_id,
			        arg.fas_arg_method_id,
		    	      sizeof arg.fas_arg_name.db_name,
		      		arg.fas_arg_name.db_name);
		    return E_DB_ERROR;
		}
		argptr = &curmethod->fas_method_arg;
	    }
	    arg.fas_arg_method = curmethod;
	    ++curmethod->fas_method_argcount;
	    if (arg.fas_arg_type == FAS_ARG_IN || 
		arg.fas_arg_type == FAS_ARG_INOUT)
		++curmethod->fas_method_inargcount;
	    if (arg.fas_arg_type == FAS_ARG_OUT || 
		arg.fas_arg_type == FAS_ARG_INOUT)
		++curmethod->fas_method_outargcount;
	    /*
	    ** Allocate memory and copy information for this arg blk
	    */
	    *argptr = (FAS_ARG *)urs_palloc(ulm, ursb, sizeof(FAS_ARG));
	    if (*argptr == NULL)
		return E_DB_ERROR;
	    STRUCT_ASSIGN_MACRO(arg, **argptr);
	    argptr = &(**argptr).fas_arg_next;
	} while (1);

	return E_DB_OK;
}
