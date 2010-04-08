/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <drviiutil.h>
#include <erddf.h>

/**
**
**  Name: drviiutil.c - Data Dictionary Services Facility Ingres Driver 
**						for Data Dictionary function
**
**  Description:
**		This file is composed of all ingres basic function 
**		like getting errors, description...
**		
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      28-May-98 (fanra01)
**          Updated message ids.
**      18-Jan-1999 (fanra01)
**          Changed waitParm initialisation to runtime.
**      06-Aug-1999 (fanra01)
**          Changed all nat, u_nat and longnat to i4, u_i4 and i4 respectively
**          for building in the 3.0 codeline.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name: OI_Query_Type()	- Determine the query type
**
** Description:
**
** Inputs:
**	char *		: statement
**
** Outputs:
**	u_nat*		: type : OI_SELECT_QUERY, OI_EXECUTE_QUERY, OI_SYSTEM_QUERY
**
** Returns:
**	GSTATUS		: 
**				  GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS 
OI_Query_Type (
	char *stmt, 
	u_i4 *type) 
{
	while (stmt[0] <= ' ') stmt++;

	switch (stmt[0]) 
	{
	case 'S':
	case 's':
		if (stmt[1] == 'E' || stmt[1] == 'e')
			switch (stmt[2]) 
			{
			case 'L':
			case 'l':
					*type = OI_SELECT_QUERY;
					break;
			default:
				*type = OI_SYSTEM_QUERY;
			}
		else
			*type = OI_SYSTEM_QUERY;
		break;
	case 'I':
	case 'i':
	case 'U':
	case 'u':
	case 'D':
	case 'd':
		*type = OI_EXECUTE_QUERY;
		break;
	default:
		*type = OI_SYSTEM_QUERY;
	}
return(GSTAT_OK);
}

/*
** Name: OI_GetError()	- Trap ingres error 
**
** Description:
**	generate the ingres error message and put it in the GSTATUS info part
**
** Inputs:
**	char *		: statement
**
** Outputs:
**	u_nat*		: type : OI_SELECT_QUERY, OI_EXECUTE_QUERY, OI_SYSTEM_QUERY
**
** Returns:
**	GSTATUS		: 
**					E_OG0001_OUT_OF_MEMORY
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**	
** History:
*/

GSTATUS  
OI_GetError( 
	II_PTR handle, 
	GSTATUS  error ) 
{
	GSTATUS err = GSTAT_OK;
  IIAPI_GETEINFOPARM 	gete;
  char*		info = NULL;
	i4			i;

	if (error) 
	{
		gete.ge_errorHandle = (II_PTR)handle;
	  do
		{ 
   		IIapi_getErrorInfo( &gete );

	   	if ( gete.ge_status != IIAPI_ST_SUCCESS )
		    break;
	
	   	if ( gete.ge_serverInfoAvail)
	  	{
	    for( i = 0; err == GSTAT_OK && i < gete.ge_serverInfo->svr_parmCount; i++ )
			{
					if ( gete.ge_serverInfo->svr_parmDescr[i].ds_columnName &&
						*gete.ge_serverInfo->svr_parmDescr[i].ds_columnName)
					{
						err = GAlloc(&info, 
								  				(info) ? STlength(info) : 0 +
													STlength(gete.ge_serverInfo->svr_parmDescr[i].ds_columnName)
													+ 10, FALSE);
						if (err == GSTAT_OK)
						{
							STpolycat(
												2, 
												gete.ge_serverInfo->svr_parmDescr[i].ds_columnName,
												" = ", 
												info);
						}
					}
					if (err == GSTAT_OK)
					{
						u_i4	position = (info) ? STlength(info) : 0;
						if ( gete.ge_serverInfo->svr_parmDescr[i].ds_nullable  &&  
								 gete.ge_serverInfo->svr_parmValue[i].dv_null )
						{
							err = GAlloc (&info, position + 10, TRUE);
							if (err == GSTAT_OK)
								STcat(info, "NULL" );
						}
						else 
						{
							switch( gete.ge_serverInfo->svr_parmDescr[i].ds_dataType)
							{
								case IIAPI_CHA_TYPE :
									{
										u_i4 length = gete.ge_serverInfo->svr_parmValue[i].dv_length;
										err = GAlloc (&info, position + length + 10, TRUE);
										if (err == GSTAT_OK)
										{
											STprintf(info + position, "'%*.*s'", 
													length,
													length,
													gete.ge_serverInfo->svr_parmValue[i].dv_value );
										}
										break;
									}
								default : 
									err = GAlloc (&info, position + 25 + NUM_MAX_INT_STR, TRUE);
									if (err == GSTAT_OK)
										STprintf(
												info + position, 
												"Datatype %d not displayed",
												gete.ge_serverInfo->svr_parmDescr[i].ds_dataType);
							}
						}
					}
				}
			}
		} while( !err );

		if (info != NULL)
		{
			DDFStatusInfo(error, info);
			MEfree((PTR) info);
		}
	}
return (err);
}

/*
** Name: OI_GetResult()	- Get execution result 
**
** Description:
**	Trap ingres error, create a GSTATUS error, 
**	generate the ingres error message and put it in the GSTATUS info part
**
** Inputs:
**	char *		: statement
**
** Outputs:
**	u_nat*		: type : OI_SELECT_QUERY, OI_EXECUTE_QUERY, OI_SYSTEM_QUERY
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_DB_ST_ERROR   
**					E_OD0003_DB_ST_FAILURE 
**					E_OD0011_DB_ST_NOT_INITIALIZED 
**					E_OD0004_DB_ST_INVALID_HANDLE  
**					E_OD0005_DB_ST_OUT_OF_MEMORY
**					E_OD0006_DB_ST_UNKNOWN_ERROR
**					E_OG0001_OUT_OF_MEMORY
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
OI_GetResult( 
	IIAPI_GENPARM 	*genParm,
	i4					timeout) 
{
	GSTATUS err = GSTAT_OK;

    IIAPI_WAITPARM waitParm ;

    waitParm.wt_timeout = timeout;

/*	CSsuspend(CS_INTERRUPT_MASK, 0, NULL); */
	while( genParm->gp_completed == FALSE )
		IIapi_wait( &waitParm );

	if ((genParm->gp_status != IIAPI_ST_SUCCESS) && 
		(genParm->gp_status != IIAPI_ST_MESSAGE) && 
		(genParm->gp_status != IIAPI_ST_WARNING)) 
	{

		err = DDFStatusAlloc ((genParm->gp_status == IIAPI_ST_NO_DATA) ?  
			E_DF0020_DB_ST_NO_DATA :
          (genParm->gp_status == IIAPI_ST_ERROR)   ?  
			E_DF0021_DB_ST_ERROR   :
          (genParm->gp_status == IIAPI_ST_FAILURE) ? 
			E_DF0022_DB_ST_FAILURE :
          (genParm->gp_status == IIAPI_ST_NOT_INITIALIZED) ?
			E_DF0028_DB_ST_NOT_INITIALIZED :
          (genParm->gp_status == IIAPI_ST_INVALID_HANDLE) ?
			E_DF0023_DB_ST_INVALID_HANDLE  :
          (genParm->gp_status == IIAPI_ST_OUT_OF_MEMORY) ?
			E_DF0024_DB_ST_OUT_OF_MEMORY   : E_DF0025_DB_ST_UNKNOWN_ERROR);

	    if ( genParm->gp_errorHandle )
				OI_GetError( genParm->gp_errorHandle, err );
	}
return(err);
}

/*
** Name: OI_Initialize()	- Ingres initialization function
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_DF0026_DB_BAD_INIT 
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
OI_Initialize(i4 timeout) 
{
	GSTATUS err = GSTAT_OK;
	IIAPI_INITPARM  initParm;

	initParm.in_timeout = timeout;
	initParm.in_version = IIAPI_VERSION_1;

	IIapi_initialize( &initParm );

	if ( initParm.in_status != IIAPI_ST_SUCCESS ) 
	{
		err = DDFStatusAlloc (E_DF0026_DB_BAD_INIT);
	}
return(err);
}

/*
** Name: OI_GetQInfo()	- Ingres initialization function
**
** Description:
**
** Inputs:
**	II_PTR		: OpenAPI statement handle
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_DB_ST_ERROR   
**					E_OD0003_DB_ST_FAILURE 
**					E_OD0011_DB_ST_NOT_INITIALIZED 
**					E_OD0004_DB_ST_INVALID_HANDLE  
**					E_OD0005_DB_ST_OUT_OF_MEMORY
**					E_OD0006_DB_ST_UNKNOWN_ERROR
**					E_OG0001_OUT_OF_MEMORY
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
OI_GetQInfo( 
	II_PTR	stmtHandle,
	u_i4		*count,
	i4 timeout) 
{
	GSTATUS err = GSTAT_OK;
  IIAPI_GETQINFOPARM		getQInfoParm;

  getQInfoParm.gq_genParm.gp_callback = NULL;
  getQInfoParm.gq_genParm.gp_closure = NULL;
  getQInfoParm.gq_stmtHandle = stmtHandle;

  IIapi_getQueryInfo( &getQInfoParm );

	err = OI_GetResult( &getQInfoParm.gq_genParm, timeout );
  if (err == GSTAT_OK && count != NULL)
		if (getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT)
			*count = getQInfoParm.gq_rowCount;
		else
			*count = 0;
return(err);
}

/*
** Name: OI_GetDescriptor()	- Get ingres descriptor
**
** Description:
**
** Inputs:
**	IIAPI_GETDESCRPARM*	: OpenAPI descriptor structure
**	II_PTR				: OpenAPI statement handle 
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_DB_ST_ERROR   
**					E_OD0003_DB_ST_FAILURE 
**					E_OD0011_DB_ST_NOT_INITIALIZED 
**					E_OD0004_DB_ST_INVALID_HANDLE  
**					E_OD0005_DB_ST_OUT_OF_MEMORY
**					E_OD0006_DB_ST_UNKNOWN_ERROR
**					E_OG0001_OUT_OF_MEMORY
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
OI_GetDescriptor( 
	IIAPI_GETDESCRPARM	*getDescrParm, 
	II_PTR stmtHandle,
	i4	timeout) 
{
	GSTATUS err = GSTAT_OK;

    getDescrParm->gd_stmtHandle = stmtHandle;

	IIapi_getDescriptor( getDescrParm );
	err = OI_GetResult( &(getDescrParm->gd_genParm), timeout );

return(err);
}

/*
** Name: OI_PutParam()	- Put ingres parameter for execution
**
** Description:
**
** Inputs:
**	IIAPI_QUERYPARM	*	: OpenAPI query structure
**	POI_PARAM			: parameters list
**	II_INT2				: number of parameters
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_DB_ST_ERROR   
**					E_OD0003_DB_ST_FAILURE 
**					E_OD0011_DB_ST_NOT_INITIALIZED 
**					E_OD0004_DB_ST_INVALID_HANDLE  
**					E_OD0005_DB_ST_OUT_OF_MEMORY
**					E_OD0006_DB_ST_UNKNOWN_ERROR
**					E_OG0001_OUT_OF_MEMORY
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
OI_PutParam( 
	IIAPI_QUERYPARM	*queryParm, 
	POI_PARAM params, 
	II_INT2 VarCounter,
	i4	timeout) 
{
	GSTATUS err = GSTAT_OK;
    IIAPI_SETDESCRPARM 	setDescrParm;
    IIAPI_PUTPARMPARM	putParmParm;
	POI_PARAM tmp;
	short i;
	bool last_is_blob = FALSE;

	if (params != NULL) 
	{
		setDescrParm.sd_genParm.gp_callback = NULL;
		setDescrParm.sd_genParm.gp_closure = NULL;
	    setDescrParm.sd_stmtHandle = queryParm->qy_stmtHandle;
		setDescrParm.sd_descriptorCount = VarCounter;
		err = G_ME_REQ_MEM(
				0, 
				setDescrParm.sd_descriptor, 
				IIAPI_DESCRIPTOR, 
				setDescrParm.sd_descriptorCount );

		tmp = params;
		for (i = 0; tmp != NULL; i++) 
		{
			setDescrParm.sd_descriptor[i].ds_dataType = tmp->type;
			setDescrParm.sd_descriptor[i].ds_nullable = FALSE;
			setDescrParm.sd_descriptor[i].ds_length = (tmp->size > 0) ? tmp->size : 1;
			setDescrParm.sd_descriptor[i].ds_precision = 0;
			setDescrParm.sd_descriptor[i].ds_scale = 0;
			setDescrParm.sd_descriptor[i].ds_columnType = tmp->columnType;
			setDescrParm.sd_descriptor[i].ds_columnName = NULL;
			tmp = tmp->next;
		}


		IIapi_setDescriptor( &setDescrParm );

		err = OI_GetResult( &setDescrParm.sd_genParm, timeout );
		MEfree( ( II_PTR )setDescrParm.sd_descriptor );

		if (err == GSTAT_OK) 
		{
			IIAPI_DATAVALUE *buffer;
			putParmParm.pp_genParm.gp_callback = NULL;
			putParmParm.pp_genParm.gp_closure = NULL;
			putParmParm.pp_stmtHandle = queryParm->qy_stmtHandle;
			err = G_ME_REQ_MEM (0, buffer, IIAPI_DATAVALUE, VarCounter );
			putParmParm.pp_parmData = buffer;
			putParmParm.pp_moreSegments = FALSE;

			tmp = params;
			for (i = 0; tmp != NULL; i++) 
			{
				if (tmp->type == IIAPI_LBYTE_TYPE) 
				{
					putParmParm.pp_parmCount = i;
					IIapi_putParms( &putParmParm );
					err = OI_GetResult( &putParmParm.pp_genParm, timeout );
					if (err == GSTAT_OK)
					{
						FILE *fp;
						i4 cRead = BLOB_BUFFER_SIZE;
						u_i4 buffer_size = BLOB_BUFFER_SIZE + sizeof (II_UINT2);
						char *byte;
						LOCATION sLoc;

						err = G_ME_REQ_MEM(
								0, 
								byte, 
								char, 
								buffer_size);

						if (err == GSTAT_OK)
						{
							LOfroms(PATH & FILENAME, tmp->value, &sLoc);
							SIfopen (&sLoc, "r", SI_BIN, 0, &fp);


							if (fp)
							{
								putParmParm.pp_parmCount = 1;
	              putParmParm.pp_moreSegments = TRUE;
		            putParmParm.pp_parmData = 
								&putParmParm.pp_parmData[i];
			          do
				        {
					        SIread (
														fp, 
														BLOB_BUFFER_SIZE, 
														&cRead, 
														&(byte[sizeof(II_UINT2)]));

						      if (cRead > 0)
							    {
										if (cRead < BLOB_BUFFER_SIZE)
											putParmParm.pp_moreSegments = FALSE;

								    putParmParm.pp_parmData[0].dv_length =
												cRead + sizeof(II_UINT2);
										putParmParm.pp_parmData[0].dv_null = 
												FALSE;
										putParmParm.pp_parmData[0].dv_value = 
												byte;

									  *((II_UINT2*)byte) = (II_UINT2)cRead;
	                  IIapi_putParms (&putParmParm);
		                err = OI_GetResult(&putParmParm.pp_genParm, timeout );
									}
								}
                while (err == GSTAT_OK && cRead == BLOB_BUFFER_SIZE);

                SIclose (fp);
							}
							else
								err = DDFStatusAlloc (E_DF0062_DB_CANNOT_OPEN);

							MEfree ( (PTR) byte );
						}
					}
					i = 0;
					last_is_blob = TRUE;
				}
				else
				{
					putParmParm.pp_parmData[i].dv_null = FALSE;
					putParmParm.pp_parmData[i].dv_length = tmp->size;
					if (tmp->size == 0)
						putParmParm.pp_parmData[i].dv_value = "  ";
					else
						putParmParm.pp_parmData[i].dv_value = tmp->value;
					last_is_blob = FALSE;
				}
				tmp = tmp->next;
			}


			if (last_is_blob == FALSE)
			{
				putParmParm.pp_moreSegments = FALSE;			
				putParmParm.pp_parmCount = i;			
				IIapi_putParms( &putParmParm );

				err = OI_GetResult( &putParmParm.pp_genParm, timeout );
			}

			MEfree( ( II_PTR )buffer );		
		}
	}

return(err);
}

/*
** Name: OI_Close()	- Close an ingres execution
**
** Description:
**
** Inputs:
**	II_PTR		: OpenAPI statement handle
**
** Outputs:
**
** Returns:
**	GSTATUS		: 
**					E_OD0002_DB_ST_ERROR   
**					E_OD0003_DB_ST_FAILURE 
**					E_OD0011_DB_ST_NOT_INITIALIZED 
**					E_OD0004_DB_ST_INVALID_HANDLE  
**					E_OD0005_DB_ST_OUT_OF_MEMORY
**					E_OD0006_DB_ST_UNKNOWN_ERROR
**					E_OG0001_OUT_OF_MEMORY
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
OI_Close( 
	II_PTR	stmtHandle, 
	i4	timeout) 
{
	GSTATUS			err = GSTAT_OK;
	IIAPI_CLOSEPARM	closeParm;

  closeParm.cl_genParm.gp_callback = NULL;
  closeParm.cl_genParm.gp_closure = NULL;
  closeParm.cl_stmtHandle = stmtHandle;

  IIapi_close( &closeParm );
	err = OI_GetResult( &closeParm.cl_genParm, timeout );
return(err);
}
