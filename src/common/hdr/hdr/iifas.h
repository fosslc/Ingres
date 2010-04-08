/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name:    iifas.h
**
** Description:
**
** History:
**      29-Jan-1999 (fanra01)
**          Integrated with the version from w4gl for (dansa02).
**          Updated types to use IIAPI types.
*/
#ifndef IIFAS_INCLUDE
#define IIFAS_INCLUDE

#include <iiapi.h>

/*
**
*/
int AS_Initiate(II_CHAR *);
int AS_ExecuteMethod(
		II_CHAR *interfacename, II_CHAR *methodname,
		II_INT2 sNumDescs,      IIAPI_DESCRIPTOR *pDescs,
		II_INT2 sNumInValues,   IIAPI_DATAVALUE *pInValues,
		II_INT2 *sNumOutValues, IIAPI_DATAVALUE **ppOutValues,
		II_LONG *plStatus);
II_INT AS_Terminate(II_VOID);
II_INT AS_MEfree(II_VOID *); /* free memory in ppOutValues */

/*
** Error codes from AS_ExecuteMethod
*/
#define AS_OK                   0
#define AS_ERROR                1
#define	AS_UNKNOWN_INTERFACE    2
#define AS_UNKNOWN_METHOD       3
#define AS_MEMORY_FAILURE       4

/*
** The AttrNames array has to be one dimension
** The AttrNames array has to be a BSTR array.
** The AttrNames array does not contain enough elements.
** The AttrNames array contains a name that is not
**      an attribute of the inparam user class.
** Cannot access an element of the AttrNames array
*/
#define AS_ATTRNAMEARRAY_WRONG_DIMENSION        20
#define AS_ATTRNAMEARRAY_NOT_BSTR               21
#define AS_ATTRNAMEARRAY_RANGE                  22
#define AS_ATTRNAMEARRAY_UNKNOWN_ATTRNAME       23
#define AS_ATTRNAMEARRAY_CANNOT_ACCESS          24

/*
** The InValues array has to be either one- or two-dimenstional.
** The InValues array has to be of type Variant.
** The Invalues array's number of columns is not the same as
**      nNumInAttrParams.
** Cannot access an element of the InValues array.
** A variant in the InValues array cannot be converted
**      to the OpenROAD data type.
*/
#define AS_INVALUEARRAY_WRONG_DIMENSION         30
#define AS_INVALUEARRAY_NOT_VARIANT             31
#define AS_INVALUEARRAY_COLUMN_MISMATCH         32
#define AS_INVALUEARRAY_CANNOT_ACCESS           33
#define AS_INVALUEARRAY_TYPEMISMATCH            34


typedef struct  _IIFAS_PARMDESC
{
    II_INT2     iifas_dataType;
    II_BOOL     iifas_nullable;
    II_UINT2    iifas_length;
    II_INT2     iifas_precision;
    II_INT2     iifas_scale;
    II_INT2     iifas_colType;
    II_CHAR     *iifas_colName;
}  IIFAS_PARMDESC;

#endif
