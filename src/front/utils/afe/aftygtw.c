/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>

/**
** Name:	aftygtw.c -  map an INGRES relation datatype into a gateway type 
**
** Description:
**  This file contains and defines:
**
**  IIAFgtm_gtwTypeMap   -  Map an INGRES datatype into a gateway type 
**
** History:
**	Revision 6.1  88/02/25  danielt
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIAFgtm_gtwTypeMap   -   Map INGRES datatype to Gateway type
**
** Description:
** This routine assumes that the caller will pass in a DBDV which
** contains a correct INGRES datatype.  It will translate that datatype
** into the string for the correct type if possible.
**
** Inputs:
**
**  dbv
**      .db_datatype    The ADF typeid corresponding to the type string. 
**
**      .db_length 	The internal length of the type
**              	corresponding to the type string.
**
** Outputs:
**
**  user_type      	A pointer to a DB_USER_TYPE.
**	.dbut_name	String containing the type name.
**		
**
**	Returns:
**	    OK 		datatype could be directly mapped onto gateway
**			type (e.g. i4 -> integer, c10 -> char(10))
**	    FAIL	datatype could not be directly mapped (e.g. text(),
**			i1)
**
** History:
**	25-feb-1988 (danielt) written
**	11/89 (jhw) -- Modified to support DATE for GateWays and
**			to use 'afe_tyoutput()' for the SQL types.
*/
DB_STATUS
IIAFgtm_gtwTypeMap ( dbv, user_type )
register DB_DATA_VALUE	*dbv;
DB_USER_TYPE		*user_type;
{
    	DB_STATUS	rval;
	i4		length;

	ADF_CB	*FEadfcb();

	if ( dbv == NULL || user_type == NULL )
	{
		rval = FAIL;
	}
	else switch ( AFE_DATATYPE(dbv) )
	{
	  case DB_INT_TYPE:
		length = dbv->db_length;
		if ( AFE_NULLABLE(dbv->db_datatype) )
			--length;
		if ( length == sizeof(i4) )
		{
			STcopy(ERx("integer"), user_type->dbut_name);
			rval = OK;
		}
		else if ( length == sizeof(i2) )
		{
			STcopy(ERx("smallint"), user_type->dbut_name);
			rval = OK;
		}
		else
		{
			rval = FAIL;
		}
		break;
	  case DB_FLT_TYPE:
		length = dbv->db_length;
		if ( AFE_NULLABLE(dbv->db_datatype) )
			--length;
		if ( length == sizeof(f8) )
		{
			STcopy(ERx("float"), user_type->dbut_name);
			rval = OK;	
		}
		else
		{
			rval = FAIL;
			/*
			** even though f4 is not supported by all the gateways,
			** if the gateway supports it then do the mapping, but
			** return the FAIL error code
			*/
			STcopy(ERx("float4"), user_type->dbut_name);
		}
   		break;
	
	  case DB_CHA_TYPE: 
	  case DB_VCH_TYPE:
	  case DB_DTE_TYPE:
		rval = afe_tyoutput(FEadfcb(), dbv, user_type);
		break;

	  default:
		rval = FAIL;
		break;
	} /* end switch */
	return rval;
}
