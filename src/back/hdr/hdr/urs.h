/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
**
**  Name: URS.H - User Request Services header
**
**  Description:
**      This file contains common definitions and prototypes for the
**      User Request Services Manager of the Frontier Application Server.
**
**  History:
**      10-Nov-1997 wonst02
**          Original User Request Services Manager.
** 	20-Jan-1998 wonst02
** 	    Add URS_EXEC_METHOD, URS_BEGIN_PARM, URS_ADD_PARM; add to URSB.
** 	05-Mar-1998 wonst02
** 	    Add URS_GET_TYPE.
** 	20-Mar-1998 wonst02
** 	    Add ursb_ddg_session for Frontier Data Dictionary Services.
** 	10-Apr-1998 wonst02
** 	    Add URS_REFRESH_INTFC.
** 	18-Jun-1998 wonst02
** 	    Add ursb_flags values.
**	03-Aug-1998 (fanra01)
**	    Removed duplicate typedefs on unix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**/

#ifndef URS_H
#define URS_H
#include <iifas.h>

/******************************************************************************
**
** Name: URSB - User Request Services Block
**
** Description:
** 	The URS Block is used to pass information to/from User Request Services
** 	by way of urs_call().
**
** 	This protocol for the ursm_error field guides its use: if the severity 
** 	of an error (E_DB_WARN, E_DB_ERROR, etc.) is greater than the stored
** 	value in ursb_error.err_data, then URSM overwrites the error code in 
** 	ursb_error.err_code and saves the severity in ursb_error.err_data.
**
******************************************************************************/

typedef struct _URSB URSB;
typedef struct _URSB_PARM URSB_PARM;

struct _URSB
{
	i4		ursb_flags;
#define 		URSB_INPUT	1 /* Build an input parameter */
#define			URSB_OUTPUT	2 /* Build an output parameter */
#define			URSB_INOUT	3 /* Build an input/output parameter */
	ULM_RCB		ursb_ulm;
	SIZE_TYPE	ursb_memleft;
	DB_ERROR	ursb_error;
	PTR		ursb_method;	/* Ptr to function/method block */
	URSB_PARM	*ursb_parm;   	/* for BEGIN/ADD_PARM et al */
	PTR		ursb_td_data;	/* ptr to tuple descriptor area */
	i4		ursb_tsize;	/* size of tuple */
	i4		ursb_num_ele;	/* number of elements in tuple */
	i4		ursb_num_recs;	/* number of records (tuples) */
};

/*
** For converting DB_DATA_VALUEs to/from IIAPI_DATAVALUEs via BEGIN_PARM,
** ADD_PARM, and GET_TYPE, and for EXEC_METHOD and REFRESH_INTFC.
*/
struct _URSB_PARM
{
	char 			*interfacename; /* interface name */ 
	char 			*methodname;  	/* method name */ 
	short 			sNumDescs;  	/* number of parm descriptors */ 
	IIAPI_DESCRIPTOR 	*pDescs;  	/* array of parm descriptors */ 
	short 			sNumInValues;  	/* number of input values */ 
	IIAPI_DATAVALUE 	*pInValues;  	/* array of input value ptrs */ 
	short 			sNumOutValues;	/* number of output values */ 
	IIAPI_DATAVALUE 	*pOutValues;	/* array of output value ptrs */ 
	DB_DATA_VALUE		data_value;  	/* a DB_DATA_VALUE */ 
};

/*
**  Function prototype
*/

/* Name: URS_CALL - Call User Request Services */
DB_STATUS
urs_call(i4		func_code,
	 URSB		*urs_cb,
	 DB_ERROR	*db_error);

#define URS_NONE	0	/* Beginning of URS function codes */
#define URS_STARTUP	1	/* Start User Request Services and apps */
#define URS_SHUTDOWN	2	/* Shut down User Request Services and apps */
#define URS_EXEC_METHOD	3	/* Execute application method */
#define URS_BEGIN_PARM	4	/* Begin in/out/in-out parameter list */
#define URS_ADD_PARM	5	/* Add in/out/in-out parameter */
#define URS_FREE_PARM	6	/* Free in/out/in-out parameter list */
#define URS_GET_TYPE	7	/* Get parameter type from output list */
#define URS_REFRESH_INTFC 8	/* Refresh an interface def'n */
#define URS_GET_INTFC	9	/* Get the next interface def'n */
#define URS_GET_METHOD	10	/* Get the next method def'n */
#define URS_GET_ARG	11	/* Get the next argument def'n */
#define URS_INVALID	12	/* End of valid function codes */

#endif
