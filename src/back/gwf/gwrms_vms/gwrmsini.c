/*
** Copyright (c) 1990, 2000 Ingres Corporation
** All Rights Reserved
*/

#include    <compat.h>
#include    <dbms.h>
#include    <adf.h>
#include    <add.h>
#include    "gwrmsdt.h"
#include    "gwrmsdtmap.h"
#include    "gwrmserr.h"

/**
**
**  Name: GWRMSINI.C - Initialize the datatype area of the RMS gateway
**
**  Description:
**	This file contains routines to initialize and shutdown the datatype
**	module of the RMS gateway server.
**
**          rms_dt_init - Initialize datatype module of RMS gateway server
[@func_list@]...
**
**
**  History:
**	17-apr-90 (jrb)
**	    Created.
**      17-dec-1992 (schang)
**          prototype
**	28-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from RMS GW code as the use is no longer allowed
[@history_template@]...
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]

/*
** Definition of all global variables owned by this file.
*/

GLOBALDEF   RMS_ADFFI_CVT   Rms_adffi_arr[] =
{
	DB_CHA_TYPE,	DB_DTE_TYPE,    0,		/* RMS_00CHA2DTE_CVT */
	DB_FLT_TYPE,	DB_MNY_TYPE,    0,		/* RMS_01FLT2MNY_CVT */
	DB_CHA_TYPE,	DB_MNY_TYPE,    0,		/* RMS_02CHA2MNY_CVT */
	DB_MNY_TYPE,	DB_VCH_TYPE,    0,		/* RMS_03MNY2VCH_CVT */
	DB_DTE_TYPE,	DB_VCH_TYPE,    0		/* RMS_04DTE2VCH_CVT */
};

GLOBALDEF   Rms_adffi_sz = sizeof(Rms_adffi_arr)/sizeof(RMS_ADFFI_CVT);

/*
[@static_variable_or_function_definition@]...
*/


/*{
** Name: rms_dt_init	- Initialize datatype section of RMS gateway server
**
** Description:
**	This routine initializes the datatype area of the RMS gateway.  This
**	should be called before any of the RMS datatypes are manipulated by
**	the gateway (it will presumably be called at server startup).
**
**	This should be called after both ADF and SCF have been started up.
**
** Inputs:
**	adf_scb		    ADF session control block (for returning errors)
**
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK	    Everything went fine
**	    E_DB_ERROR	    Something went wrong...
**	    
**	Exceptions:
**	    none
**
** Side Effects:
**	    The global function instance conversion table is filled in with
**	    function instance id's.
**
** History:
**	17-apr-90 (jrb)
**	    Created.
*/
DB_STATUS
rms_dt_init
(
    ADF_CB  *adf_scb
)
{
    DB_STATUS	    db_stat;
    ADI_FI_ID	    id;
    i4		    i;

    for (i=0; i < Rms_adffi_sz; i++)
    {
	/* first try to find a normal coercion */
	if (db_stat = adi_ficoerce(adf_scb, Rms_adffi_arr[i].rms_fromdt,
					Rms_adffi_arr[i].rms_todt, &id)
	    != E_DB_OK)
	{
	    if (adf_scb->adf_errcb.ad_errcode != E_AD2009_NOCOERCION)
		return(db_stat);

	    /* ...then try copy coercion */
	    if (db_stat = adi_cpficoerce(adf_scb, Rms_adffi_arr[i].rms_fromdt,
					    Rms_adffi_arr[i].rms_todt, &id)
		!= E_DB_OK)
	    {
		if (adf_scb->adf_errcb.ad_errcode != E_AD200A_NOCOPYCOERCION)
		    return(db_stat);

		/* still no luck, return an error... */
		return(rms_dterr(adf_scb, E_GW7100_NOCVT_AT_STARTUP, 2,
				sizeof(DB_DT_ID), &Rms_adffi_arr[i].rms_fromdt,
				sizeof(DB_DT_ID), &Rms_adffi_arr[i].rms_todt));
	    }
	}

	/* We have a function instance for the conversion; store it */
	Rms_adffi_arr[i].rms_fiid = id;
    }

    return(E_DB_OK);
}

/*
[@function_definition@]...
*/
