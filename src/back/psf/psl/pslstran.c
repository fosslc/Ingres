/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cv.h>
#include    <cs.h>
#include    <qu.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefmain.h>
#include    <qefcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/*
** Name: PSLSTRAN.C:	This file contains functions used by the SQL
**			grammar in parsing of SET TRANSACTION statement.
**
** Function Name Format:  (this applies to externally visible functions only)
** 
**	PSL_st<number>_<production name>
**	
**	where <number> is a unique (within this file) number between 1 and 99
**	
** Description:
**
**	psl_st0_settransaction()    - semantic actions for SET TRANSACTION
**					statement as a whole
**	psl_st1_settranstmnt()	    - semantic actions for settranstmnt
**					production
**
** History:
**	23-aug-1993 (bryanp)
**	    created
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**      14-oct-1997 (stial01)
**          Removed extra functions not needed when isolation level is passed
**          as parameter.
**          Removed extra functions not needed when access mode is passed 
**          as parameter.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
*/

/* TABLE OF CONTENTS */
i4 psl_st0_settransaction(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
i4 psl_st1_settranstmnt(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb);
i4 psl_st2_settranisolation_level(
	PSS_SESBLK *sess_cb,
	DB_ERROR *err_blk,
	i4 isolation_level);
i4 psl_st3_accessmode(
	PSS_SESBLK *sess_cb,
	DB_ERROR *err_blk,
	i4 accessmode);

/* Useful defines */
					/*
					** maximum number of SET TRANSACTION
					** characteristics
					*/
#define	    MAX_SETTRAN_CHARS	    2
					/*
					** The following constants are used to
					** remember the type of characteristic
					** being set
					*/
#define                 ISOLATIONLEVEL       1
#define                 ACCESSMODE           2

/*
** Name: psl_st0_settransaction - semantic actions for the set trans stmt
**
** Description:
**	perform semantic action for SETTRANSTMNT production in SQL
**
** Input:
**	sess_cb		    PSF session CB
**	    pss_distrib	    DB_3_DDB_SESS if distributed thread
**	psq_cb		    PSF request CB
**
** Output:
**	psq_cb
**	    psq_error	    filled in if an error occurred
**
** Returns:
**	E_DB_{OK, ERROR, SEVERE}
**
**  History:
**	23-aug-1993 (bryanp)
**	    created.
*/
DB_STATUS
psl_st0_settransaction(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    return (E_DB_OK);
}

/*
** Name: psl_st1_settranstmnt	- Semantic actions for the settranstmnt
**			         production
**
** Description:
**	perform semantic action for SETTRANSTMNT production in SQL
**
** Input:
**	sess_cb		    PSF session CB
**	    pss_distrib	    DB_3_DDB_SESS if distributed thread
**	    pss_ostream	    stream to be opened for memory allocation
**	psq_cb		    PSF request CB
**
** Output:
**	sess_cb
**	    pss_ostream	    stream has been opened for memory allocation
**	    pss_object	    point to the root of a new QSF object
**			    (of type (DMC_CB *) or (QEF_RCB *)).
**	psq_cb
**	    psq_mode	    set to PSQ_STRANSACTION
**	    psq_error	    filled in if an error occurred
**
** Returns:
**	E_DB_{OK, ERROR, SEVERE}
**
** Side effects:
**	Opens a memory stream and allocates memory
**
**  History:
**	23-aug-1993 (bryanp)
**	    created.
**	11-Jun-2010 (kiria01) b123908
**	    Initialise pointers after psf_mopen would have invalidated any
**	    prior content.
*/
DB_STATUS
psl_st1_settranstmnt(
	PSS_SESBLK	*sess_cb,
	PSQ_CB		*psq_cb)
{
    DB_STATUS		status;
    DMC_CB		*dmc_cb;

    psq_cb->psq_mode = PSQ_STRANSACTION;

    /* Create control block for DMC_ALTER */
    status = psf_mopen(sess_cb, QSO_QP_OBJ, &sess_cb->pss_ostream, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);
    sess_cb->pss_stk_freelist = NULL;

    if (sess_cb->pss_distrib & DB_3_DDB_SESS)
    {
	/* distributed doesn't currently support this. */
	return (FAIL);
    }

    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, sizeof(DMC_CB), (PTR *) &dmc_cb,
	&psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    status = psf_mroot(sess_cb, &sess_cb->pss_ostream, (PTR) dmc_cb, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    sess_cb->pss_object		= (PTR) dmc_cb;
    dmc_cb->type		= DMC_CONTROL_CB;
    dmc_cb->length		= sizeof (DMC_CB);
    dmc_cb->dmc_op_type		= DMC_SESSION_OP;
    /*dmc_cb->dmc_id		= sess_cb->pss_sessid;*/
    dmc_cb->dmc_session_id    = (PTR)sess_cb->pss_sessid;
    dmc_cb->dmc_flags_mask	= DMC_SETTRANSACTION;
    dmc_cb->dmc_db_id		= sess_cb->pss_dbid;
    dmc_cb->dmc_db_access_mode  =
    dmc_cb->dmc_lock_mode	= 0;

    /* need to allocate characteristics array with MAX_SETTRAN_CHARS entries */
    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
	sizeof(DMC_CHAR_ENTRY) * MAX_SETTRAN_CHARS,
	(PTR *) &dmc_cb->dmc_char_array.data_address, &psq_cb->psq_error);
    if (status != E_DB_OK)
	return (status);

    dmc_cb->dmc_char_array.data_in_size = 0;

    return(E_DB_OK);
}

/*
** Name psl_st2_settranisolation_level() - perform semantic action for
**		    settranisolation_level production
**
** Description:
**	perform semantic action for SETTRANISOLATION_LEVEL production in SQL
**
** Input:
**	sess_cb		    PSF session CB
**	    pss_distrib	    DB_3_DDB_SESS if distributed thread
**	char_name	    name of a characteristic
**
** Output:
**	err_blk		    filled in if an error occurred
**
** Returns:
**	E_DB_{OK, ERROR}
**
** Side effects:
**	none
**
**  History:
**	23-aug-1993 (bryanp)
**	    created.
*/
DB_STATUS
psl_st2_settranisolation_level(
	PSS_SESBLK  *sess_cb,
	DB_ERROR    *err_blk,
	i4     isolation_level)
{
    i4		err_code;
    DMC_CHAR_ENTRY	*chr;
    DM_DATA	    	*char_array;
    bool		not_distr = ~sess_cb->pss_distrib & DB_3_DDB_SESS;

    if (not_distr)
    {
    	char_array = &((DMC_CB *)sess_cb->pss_object)->dmc_char_array;

    	if (char_array->data_in_size / sizeof (DMC_CHAR_ENTRY) ==
	    MAX_SETTRAN_CHARS)
    	{
	    (VOID) psf_error(5931L, 0L, PSF_USERERR, &err_code, err_blk, 0);
	    return (E_DB_ERROR);	/* non-zero return means error */
    	}   

    	chr = (DMC_CHAR_ENTRY *)
	    ((char *) char_array->data_address + char_array->data_in_size);
	chr->char_id = DMC_C_ISOLATION_LEVEL;
	chr->char_value = isolation_level;
	char_array->data_in_size += sizeof (DMC_CHAR_ENTRY);
    }
    return (E_DB_OK);
}

/*
** Name: psl_st7_accessmode    - ACCESS MODE: READ ONLY / READ WRITE
**
** Input:
**      sess_cb             PSF session CB
**          pss_distrib     DB_3_DDB_SESS if distributed thread
**          pss_object      pointer to the root of a QSF object
**                          (of type (DMC_CB *) or (QEF_RCB *)).
**
** Output:
**      err_blk             filled in if an error occurred
**      sess_cb             PSF session CB
**          pss_object      characteristics of SETTRANSACTION filled in.
**
** Returns:
**      E_DB_{OK, ERROR, SEVERE}
**
** Side effects:
**      None
**
**  History:
**	27-Feb-1997 (jenjo02)
**	    created.
*/
DB_STATUS
psl_st3_accessmode(
        PSS_SESBLK  *sess_cb,
        DB_ERROR    *err_blk,
	i4     accessmode)
{
    i4             err_code;
    DMC_CHAR_ENTRY      *chr;
    DM_DATA             *char_array;
    bool                not_distr = ~sess_cb->pss_distrib & DB_3_DDB_SESS;
 
    if (not_distr)
    {
        char_array = &((DMC_CB *)sess_cb->pss_object)->dmc_char_array;
 
        if (char_array->data_in_size / sizeof (DMC_CHAR_ENTRY) ==
            MAX_SETTRAN_CHARS)
        {
            (VOID) psf_error(5931L, 0L, PSF_USERERR, &err_code, err_blk, 0);
            return (E_DB_ERROR);        /* non-zero return means error */
        }
 
        chr = (DMC_CHAR_ENTRY *)
            ((char *) char_array->data_address + char_array->data_in_size);
        chr->char_id = DMC_C_TRAN_ACCESS_MODE;
        chr->char_value = accessmode;
        char_array->data_in_size += sizeof (DMC_CHAR_ENTRY);
    } 
    return(E_DB_OK);
} 
