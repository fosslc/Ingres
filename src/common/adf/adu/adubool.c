/*
** Copyright (c) 2009 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <st.h>
#include <iicommon.h>
#include <adf.h>
#include <ulf.h>
#include <adfint.h>

/**
** Name: adubool.c - Functions for SQL BOOLEAN operations
**
** This file includes the following externally visible routines:
**
**      adu_bool_coerce   -  Coerces some types to boolean
**      adu_bool_isfalse  -  Checks if a boolean is FALSE
**      adu_bool_istrue   -  Checks if a boolean is TRUE
**
**  History:
**	29-sep-2009 (joea)
**	    Created.
**      27-oct-2009 (joea)
**          Use STxcompare and ADF_TRIMBLANKS_MACRO instead of STbcompare
**          in adu_bool_coerce.  Replace E_AD9999 by E_AD106A for invalid
**          character casts.  Implement integer casts.
**      25-jan-2010 (joea)
**          Correct adu_bool_isfalse and adu_bool_istrue to deal correctly
**          with null values.
**/


/*{
** Name: adu_bool_coerce - Coerce some values to boolean
**
** Description:
**      Coerces a boolean into another boolean, some character/text types
**      to a boolean or integer types to boolean.
**
** Inputs:
**      adf_scb	        Pointer to an ADF session control block
**      dv              Pointer to DB_DATA_VALUE containing source argument
**      rdv             Pointer to DB_DATA_VALUE containing target argument
**
** Outputs:
**      rdv             Pointer to DB_DATA_VALUE containing target argument
**
** Returns:
**	E_DB_OK			  Completed sucessfully.
**      E_AD106A_INV_STR_FOR_BOOL_CAST  Character value cannot be converted
**                                      to BOOLEAN
**      E_AD106B_INV_INT_FOR_BOOL_CAST  Integer value cannot be converted to
**                                      BOOLEAN
**	E_AD9999_INTERNAL_ERROR	  argument datatypes were incompatible with
**					this operation.
*/
DB_STATUS
adu_bool_coerce(
ADF_CB *adf_scb,
DB_DATA_VALUE *dv,
DB_DATA_VALUE *rdv)
{
    char *s;
    i4 len;
    i8 intval;

    switch (rdv->db_datatype)
    {
    case DB_BOO_TYPE:
        switch (dv->db_datatype)
	{
        case DB_BOO_TYPE:
            ((DB_ANYTYPE *)rdv->db_data)->db_booltype =
                    ((DB_ANYTYPE *)dv->db_data)->db_booltype;
            break;

        case DB_CHR_TYPE:
        case DB_CHA_TYPE:
            s = ((DB_ANYTYPE *)dv->db_data)->db_ctype;
            len = dv->db_length;
            ADF_TRIMBLANKS_MACRO(s, len);
            if (STxcompare("FALSE", 5, s, len, TRUE,
                           (bool)(dv->db_datatype == DB_CHR_TYPE)) == 0)
                ((DB_ANYTYPE *)rdv->db_data)->db_booltype = DB_FALSE;
            else if (STxcompare("TRUE", 4, s, len, TRUE,
                                (bool)(dv->db_datatype == DB_CHR_TYPE)) == 0)
                ((DB_ANYTYPE *)rdv->db_data)->db_booltype = DB_TRUE;
            else
                return adu_error(adf_scb, E_AD106A_INV_STR_FOR_BOOL_CAST, 2,
                                 dv->db_length, s);
            break;

        case DB_LTXT_TYPE:
        case DB_TXT_TYPE:
        case DB_VCH_TYPE:
            s = ((DB_ANYTYPE *)dv->db_data)->db_textype.db_t_text;
            len = ((DB_ANYTYPE *)dv->db_data)->db_textype.db_t_count;
            ADF_TRIMBLANKS_MACRO(s, len);
            if (STxcompare("FALSE", 5, s, len, TRUE, FALSE) == 0)
                ((DB_ANYTYPE *)rdv->db_data)->db_booltype = DB_FALSE;
            else if (STxcompare("TRUE", 4, s, len, TRUE, FALSE) == 0)
                ((DB_ANYTYPE *)rdv->db_data)->db_booltype = DB_TRUE;
            else
                return adu_error(adf_scb, E_AD106A_INV_STR_FOR_BOOL_CAST, 2,
                                 ((DB_ANYTYPE *)dv->db_data)->
                                                db_textype.db_t_count, s);
            break;

        case DB_INT_TYPE:
            switch (dv->db_length)
            {
            case 1:
                intval = (i8)((DB_ANYTYPE *)dv->db_data)->db_i1type;
                break;
            case 2:
                intval = (i8)((DB_ANYTYPE *)dv->db_data)->db_i2type;
                break;
            case 4:
                intval = (i8)((DB_ANYTYPE *)dv->db_data)->db_i4type;
                break;
            case 8:
                intval = ((DB_ANYTYPE *)dv->db_data)->db_i8type;
                break;
            default:
                return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
            }
            if (intval == 0)
                ((DB_ANYTYPE *)rdv->db_data)->db_booltype = DB_FALSE;
            else if (intval == 1)
                ((DB_ANYTYPE *)rdv->db_data)->db_booltype = DB_TRUE;
            else
                return adu_error(adf_scb, E_AD106B_INV_INT_FOR_BOOL_CAST, 2,
                                 dv->db_length, dv->db_data);
            break;

        default:
            return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
        }
        break;

    default:
        return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
    }

    return E_DB_OK;
}

/*{
** Name: adu_bool_isfalse - Check boolean for SQL FALSE
**
** Description:
**      Checks if a boolean is FALSE
**
** Inputs:
**      adf_scb	        Pointer to an ADF session control block
**      dv1             Pointer to DB_DATA_VALUE containing argument
**
** Outputs:
**      rcmp		Result of comparison, as follows:
**			   0  if  dv1 is not false
**			   1  if  dv1 is false
**
** Returns:
**	E_DB_OK			  Completed sucessfully.
**	E_AD9999_INTERNAL_ERROR	  dv1's datatype was incompatible with
**					this operation.
*/
DB_STATUS
adu_bool_isfalse(
ADF_CB *adf_scb,
DB_DATA_VALUE *dv1,
i4 *rcmp)
{
    DB_STATUS db_stat = E_DB_OK;

    switch (dv1->db_datatype)
    {
    case DB_BOO_TYPE:
        *rcmp = ((DB_ANYTYPE *)dv1->db_data)->db_booltype == DB_FALSE;
        break;

    case -DB_BOO_TYPE:
	if (ADI_ISNULL_MACRO(dv1))
            *rcmp = FALSE;
        else
            *rcmp = ((DB_ANYTYPE *)dv1->db_data)->db_booltype == DB_FALSE;
	break;

    default:
        return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
    }

    return db_stat;
}


/*{
** Name: adu_bool_istrue - Check boolean for SQL TRUE
**
** Description:
**      Checks if a boolean is TRUE
**
** Inputs:
**      adf_scb	        Pointer to an ADF session control block
**      dv1             Pointer to DB_DATA_VALUE containing argument
**
** Outputs:
**      rcmp		Result of comparison, as follows:
**			   0  if  dv1 is not true
**			   1  if  dv1 is true
**
** Returns:
**	E_DB_OK			  Completed sucessfully.
**	E_AD9999_INTERNAL_ERROR	  dv1's datatype was incompatible with
**					this operation.
*/
DB_STATUS
adu_bool_istrue(
ADF_CB *adf_scb,
DB_DATA_VALUE *dv1,
i4 *rcmp)
{
    DB_STATUS db_stat = E_DB_OK;

    switch (dv1->db_datatype)
    {
    case DB_BOO_TYPE:
        *rcmp = ((DB_ANYTYPE *)dv1->db_data)->db_booltype == DB_TRUE;
        break;

    case -DB_BOO_TYPE:
	if (ADI_ISNULL_MACRO(dv1))
            *rcmp = FALSE;
        else
            *rcmp = ((DB_ANYTYPE *)dv1->db_data)->db_booltype == DB_TRUE;
	break;

    default:
        return adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
    }

    return db_stat;
}
