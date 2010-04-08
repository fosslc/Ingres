/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <lo.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cs.h>
#include    <st.h>
#include    <scf.h>

#include    <ursm.h>

/**
**
**  Name: URSM.C - User Request Services Manager
**
**  Description:
**      This file contains internally used routines of the User Request Services
**	Manager of the Frontier Application Server.
**
**	URS_PALLOC 	- Allocate memory from a ulm stream.
** 	URS_ERROR	- Format and report a URS facility error.
**
**  History:
**      22-Dec-1997 wonst02
**          Original.
**	10-Aug-1998 (fanra01)
**	    Updated incomplete last line.
**	31-aug-98 (mcgem01)
**	    Take the ule_format prototype from the ulf header file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name: URS_PALLOC - Allocate memory from a ulm stream.
**
** Description:
** 		Allocate a piece of ULM memory from the caller's ULM stream.
**
** Inputs:
**
** Outputs:
** 
**	Returns:
**	
**  Exceptions:
**	    none
**
** Side Effects:
**
** History:
** 		27-Mar-1998 (wonst02)
** 			Add urs_palloc()
**	7-oct-2004 (thaju02)
**	    Change zero var to SIZE_TYPE.
*/

PTR
urs_palloc(ULM_RCB	*ulm,
	   URSB		*ursb,
	   i4		size)
{
	ulm->ulm_psize = size;
    	if (ulm_palloc(ulm) != E_DB_OK)
    	{
	    SIZE_TYPE	zero = 0;

            urs_error(ulm->ulm_error.err_code, URS_INTERR, 0);
            urs_error(E_UR0314_ULM_PALLOC_ERROR, URS_INTERR, 4,
		      sizeof(*ulm->ulm_memleft), 
		      ulm->ulm_memleft ? ulm->ulm_memleft : &zero,
		      sizeof(ulm->ulm_sizepool), &ulm->ulm_sizepool,
		      sizeof(ulm->ulm_blocksize), &ulm->ulm_blocksize,
		      sizeof(ulm->ulm_psize), &ulm->ulm_psize);
            if (ulm->ulm_error.err_code == E_UL0005_NOMEM)
	    	SET_URSB_ERROR(ursb, E_UR0600_NO_MEM, E_DB_ERROR)
            else
	    	SET_URSB_ERROR(ursb, E_UR0601_MEM_ERROR, E_DB_ERROR)
	    return NULL;
    	}
	return ulm->ulm_pptr;
}


/*{
** Name: URS_ERROR	- Format and report a URS facility error.
**
** Description:
**      This function formats and reports a URS facility error.  There are
**      three types of errors: user errors, internal errors, and caller errors.
**	User errors are not logged, but the text is sent to the user.  Internal
**	errors are logged , but the text is not sent to the user.  Caller errors
**	are neither logged nor sent to the user.  Sometimes there will be
**      an operating system error associated with the error; this can be passed
**      as a separate parameter.
**
**	The error number will be put into the error control block in the
**	control block used to call the parser facility.  User errors will be
**	recorded in the error block as E_PS0001_USER_ERROR, because the caller
**	wants to know if the user made a mistake, but doesn't particularly care
**	what the mistake was.  Internal errors are recorded in the error block
**	as E_PS0002_INTERNAL_ERROR, because the caller doesn't need to know
**	the particulars of why the parser facility screwed up (more specific
**	information will be recorded in the log.  Caller errors will be recorded
**	in the error block as is.
**
** Inputs:
**      errorno                         Error number
**      detail				Error number that caused this error
**					(if, e.g., the error is of type
**					URS_INTERR, generated after a bad
**					status return from a call to another
**					facility, then this would be the error
**					code from the control block for that
**					facility)
**      err_type                        Type of error
**					    URS_USERERR - user error
**					    URS_INTERR  - internal error
**					    URS_CALLERR - caller error
**	err_code			Ptr to reason for error return, if any
**	err_blk				Pointer to error block
**	num_parms			Number of parameter pairs to follow.
** NOTE: The following parameters are optional, and come in pairs.  The first
**  of each pair gives the length of a value to be inserted in the error
**  message.  The second of each pair is a pointer to the value to be inserted.
**      plen1				First length parameter, if any
**      parm1				First error parameter, if any
**      plen2				Second lengthparameter, if any
**      parm2				Second error parameter, if any
**      plen3				Third length parameter, if any
**      parm3				Third error parameter, if any
**	plen4				Fourth length parameter, if any
**	parm4				Fourth error parameter, if any
**	plen5				Fifth length parameter, if any
**	parm5				Fifth error parameter, if any
**
** Outputs:
**      err_code                        Filled in with reason for error return
**	err_blk				Filled in with error
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Error by caller
**	    E_DB_FATAL			Couldn't format or report error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can cause message to be logged or sent to user.
**
** History:
**      15-Dec-1997 (wonst02)
**          Original - cloned from gwf_error()
*/

/* VARARGS12 */
VOID
urs_error(i4 errorno, i4  err_type, i4  num_parms,
	  i4 plen1, i4 parm1,
	  i4 plen2, i4 parm2,
	  i4 plen3, i4 parm3,
	  i4 plen4, i4 parm4,
	  i4 plen5, i4 parm5,
	  i4 plen6, i4 parm6,
	  i4 plen7, i4 parm7,
	  i4 plen8, i4 parm8,
	  i4 plen9, i4 parm9,
	  i4 plen10, i4 parm10,
	  i4 plen11, i4 parm11,
	  i4 plen12, i4 parm12)
{
    i4		    uletype;
    DB_STATUS		    uleret;
    i4		    ulecode;
    i4		    msglen;
    DB_STATUS		    ret_val = E_DB_OK;
    char		    errbuf[DB_ERR_SIZE];
    SCF_CB		    scf_cb;

    for (;;)	/* Not a loop, just gives a place to break to */
    {
	/*
	** Log internal errors, don't log user errors or caller errors.
	*/
	if (err_type == URS_INTERR)
	    uletype = ULE_LOG;
	else
	    uletype = 0L;

	/*
	** Get error message text.  Don't bother with caller errors, since
	** we don't want to report them or log them.
	*/
	if (err_type != URS_CALLERR)
	{
	    uleret = ule_format(errorno, 0L, uletype,
		&scf_cb.scf_aux_union.scf_sqlstate, errbuf,
		(i4) sizeof(errbuf), &msglen, &ulecode, num_parms,
		plen1, parm1, plen2, parm2, plen3, parm3,
		plen4, parm4, plen5, parm5, plen6, parm6,
		plen7, parm7, plen8, parm8, plen9, parm9,
		plen10, parm10, plen11, parm11, plen12, parm12);

	    /*
	    ** If ule_format failed, we probably can't report any error.
	    ** Instead, just propagate the error up to the user.
	    */
	    if (uleret != E_DB_OK)
	    {
		STprintf(errbuf, "Error message corresponding to %d (%x) \
		    not found in INGRES error file", errorno, errorno);
		msglen = STlength(errbuf);
		ule_format(0L, 0L, ULE_MESSAGE, NULL, errbuf, msglen, 0,
		    &ulecode, 0);
		break;
	    }
	}

	/*
	** Only send text of user errors to user.  Internal error has already
	** been logged by ule_format.  This logic keys off of err_type, which
	** must be URS_USERERR.  If a further error occurs, it calls this
	** routine again with err_type = URS_INTERR.  If you change this,
	** ponder the possibilities of infinite recursion.
	*/

	if (err_type == URS_USERERR)
	{

	    scf_cb.scf_length = sizeof(scf_cb);
	    scf_cb.scf_type = SCF_CB_TYPE;
	    scf_cb.scf_facility = DB_URS_ID;
	    scf_cb.scf_session = DB_NOSESSION;
	    scf_cb.scf_nbr_union.scf_local_error = errorno;
	    scf_cb.scf_len_union.scf_blength = msglen;
	    scf_cb.scf_ptr_union.scf_buffer = errbuf;
	    ret_val = scf_call(SCC_ERROR, &scf_cb);

	    if ( ret_val != E_DB_OK)
	    {
		urs_error( scf_cb.scf_error.err_code, URS_INTERR, 0,
			   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
	    }
	}	/* end if err_type == URS_USERERR */

	break;
    }

    return ;
}
