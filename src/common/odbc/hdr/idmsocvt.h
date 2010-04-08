/*
//  IDMSOCVT.H
//
//  Header file for ODBC conversion routines.
//
//  Copyright (c) 1993,2008 Ingres Corporation
//
//
//  Change History
//  --------------
//
//  date        programmer          description
//
//   1/29/1993  Dave Ross          Initial coding
//  02/13/1997  Jean Teng          chgd fIdmsType to fIngApiType
//  06/09/1997  Jean Teng          added CvtTimestampChar_DB2 
//  02/06/1998  Dave Thole         Increase CvtCharBin cbValue to 32-bits 
//  04/15/1998  Dave Thole         Increased tSQLTYPEATTR fields to UDWORDs
//  02/17/1999  Dave Thole         Ported to UNIX
//  03/13/2000  Ralph Loen         Added CvtDoubleToChar
//  10/16/2000  Ralph Loen         Added support for II_DECIMAL (Bug 102666).
**      19-aug-2002 (loera01) Bug 108536
**          First phase of support for SQL_C_NUMERIC.
**      04-Nov-2002 (loera01) Bug 108536
**          Second phase of support for SQL_C_NUMERIC.  Renamed
**          CvtSqlNumericChar and CvtSqlNumericDec to CvtSqlNumericStr and
**          CvtStrSqlNumeric, respectively.  Added error checking.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**     02-dec-2005 (loera01) Bug 115582
**          Added CVT_FRAC_TRUNC (fractional truncation) error.
**     10-Aug-2006 (Ralph Loen) SIR 116477
**          Add support for ISO dates, times, and intervals (first phase).
**     16-Aug-2006 (Ralph Loen) SIR 116477 (second phase)
**          Add support for ISO timestamps and DS intervals.
**     23-Oct-2006 (Ralph Loen) SIR 116477
**          Obsoleted CvtCharIsoTime(); replaced with common
**          GetTimeOrIntervalChar().
**     14-Nov-2008 (Ralph Loen) SIR 121228
**         Added date connection level arguments to CvtGetSqlType() and
**         CvtGetCType().
*/

#ifndef _INC_IDMSOCVT
#define _INC_IDMSOCVT

/*
//  Conversion routine return codes:
*/
#define CVT_SUCCESS         0x00000000  /* Data converted successfully. */
#define CVT_NEED_DATA       0x00000001  /* Need more data. */
#define CVT_TRUNCATION      0x00000002  /* Data truncated. */
#define CVT_STRING_TRUNC    0x00000004  /* String data right truncation. */
#define CVT_OUT_OF_RANGE    0x00000008  /* Numeric value out of range. */
#define CVT_ERROR           0x00000010  /* Assignment error. */
#define CVT_DATE_OVERFLOW   0x00000020  /* Datetime overflow. */
#define CVT_INVALID         0x00000040  /* Invalid conversion (restricted). */
#define CVT_NOT_CAPABLE     0x00000080  /* Driver does not support conversion. */
#define CVT_FRAC_TRUNC      0x00000100  /* Fractional truncation */
/*
**  Maximum SQL_C_NUMERIC display digits
*/
#define MAX_NUM_DIGITS 39
/*
**  ODBC data type attribute table format:
*/
typedef struct tSQLTYPEATTR
{
    SWORD fSqlType;                     /* ODBC SQL type. */
    SWORD fCType;                       /* ODBC C type. */
    SWORD fVerboseType;                 /* ODBC verbose type */
    SWORD fIngApiType;                  /* Ingres OpenAPI data type. */
    UDWORD cbPrec;                      /* Precision (length). */
    UDWORD cbDisplay;                   /* Display size. */
    UDWORD cbSize;                      /* C type size. */
    UDWORD cbLength;                    /* Ingres type length. */
    BOOL cbSupported;                   /* Supported in this release? */
    SWORD fSqlCode;                     /* ODBC SQL Code */
}
SQLTYPEATTR,  * LPSQLTYPEATTR;

typedef struct tagISO_TIME_STRUCT
{
        SQLUSMALLINT   hour;
        SQLUSMALLINT   minute;
        SQLUSMALLINT   second;
        SQLUINTEGER    fraction;
} ISO_TIME_STRUCT;

/*
//  ODBC - IDMS data format conversion functions:
*/

UWORD CvtBigChar(
    CHAR   * szValue,
    CHAR   * rgbData);

VOID CvtBinChar(
    CHAR   * szValue,
    CHAR   * rgbData,
    UWORD       cbData);

long double CvtBigFloat(
    CHAR   * rgbData);

UDWORD CvtCharBig(
    CHAR   * rgbData,
    CHAR   * rgbValue,
    UWORD       cbValue);

UDWORD CvtCharBin(
    CHAR   * rgbData,
    CHAR   * rgbValue,
    UDWORD      cbData);

UDWORD CvtCharDate(
    DATE_STRUCT  * pValue,
    CHAR         * rgbData,
    UWORD             cbData);

UDWORD CvtCharFloat(
    SDOUBLE  * rgbValue,
    CHAR   * rgbData,
    UWORD       cbData);

UDWORD CvtCharInt(
    SDWORD  * pValue,
    CHAR   * rgbData,
    UWORD       cbData);

UDWORD CvtCharInt64(
    ODBCINT64  * pValue,
    CHAR       * rgbData,
    UWORD       cbData);

UDWORD CvtCharNum(
    CHAR   * rgbData,
    CHAR   * rgbValue,
    UWORD       cbValue,
    SWORD       cbPrecision,
    SWORD       cbScale,
    BOOL        fUnsigned);

UDWORD CvtCharNumStr(
    CHAR   * szValue,
    SDWORD      cbValueMax,
    CHAR   * rgbData,
    UWORD       cbData,
    CHAR   *  * prgbNext,
    UWORD  * pcbNext);

UDWORD CvtCharInterval(
    CHAR         * rgbData,
    SQL_INTERVAL_STRUCT * pValue,
    SWORD        fSqlType);

UDWORD CvtCharTime(
    ISO_TIME_STRUCT  * pValue,
    CHAR         * rgbData,
    UWORD        cbData,
    UDWORD       apiLevel);

UDWORD CvtCharTimestamp(
    TIMESTAMP_STRUCT  * pValue,
    CHAR         * rgbData,
    UWORD             cbData,
    UDWORD        apiLevel);

void CvtCurrStampDate(
    TIMESTAMP_STRUCT  * pstamp,
    UDWORD apiLevel);

UDWORD CvtDateChar(
    DATE_STRUCT  * pdate,
    CHAR   * rgbData,
    UWORD  cbData,
    UDWORD apiLevel);

UDWORD CvtEditDate(
    DATE_STRUCT  * pdate);

UDWORD CvtEditTime(
    ISO_TIME_STRUCT  * ptime, UDWORD apiLevel);

UDWORD CvtEditTimestamp(
   TIMESTAMP_STRUCT  * pstamp, UDWORD apiLevel);

UDWORD CvtFloatBig(
    CHAR   * rgbData,
    long double floatValue);

UDWORD CvtFloatNum(
    CHAR   * rgbData,
    SDOUBLE     floatValue,
    SWORD       cbPrecision,
    SWORD       cbScale,
    BOOL        fUnsigned);

UDWORD CvtGetToken(
    CHAR   * pToken,
    UWORD       cbToken,
    CHAR   * rgbData,
    UWORD       cbData);

LPSQLTYPEATTR CvtGetCType(
    SWORD fType, UDWORD apiLevel, UDWORD dateConnLevel);

LPSQLTYPEATTR CvtGetSqlType(
    SWORD fType, UDWORD apiLevel, UDWORD dateConnLevel);

LPSQLTYPEATTR CvtGetIngresDateSqlType(
    SWORD fType);

LPSQLTYPEATTR CvtGetIngresDateCType(
    SWORD fType);

UDWORD CvtIntNum(
    CHAR   * rgbData,
    SDWORD      intValue,
    SWORD       cbPrecision,
    SWORD       cbScale,
    BOOL        fUnsigned);

UDWORD CvtInt64Num(
    CHAR   * rgbData,
    ODBCINT64   intValue,
    SWORD       cbPrecision,
    SWORD       cbScale,
    BOOL        fUnsigned);

UDWORD CvtNumChar(
    CHAR   * szValue,
    CHAR   * rgbData,
    SWORD       cbPrecision,
    SWORD       cbScale);

UDWORD CvtNumNum(
    CHAR   * rgbData,
    SWORD       cbPrecision,
    SWORD       cbScale,
    CHAR   * szValue,
    int         dec,
    int         sign);

VOID CvtPack(
    CHAR   * rgbDecimal,
    UWORD       cbDecimal,
    CHAR   * rgbNumeric,
    UWORD       cbNumeric);

CHAR   * CvtSkipSpace(
    CHAR   * rgbData,
    UWORD       cbData,
    UWORD  * pcbData);

UDWORD CvtTimeChar(
    ISO_TIME_STRUCT  * ptime,
    CHAR   * rgbData,
    UWORD       cbData,
    UDWORD  apiLevel);

UDWORD CvtIntervalChar(
    SQL_INTERVAL_STRUCT  * pinterval,
    CHAR   * rgbData,
    SWORD        fSqlType);

UDWORD CvtTimestampChar(
    TIMESTAMP_STRUCT  * pstamp,
    CHAR   * rgbData,
    UWORD       cbData,
    UWORD       cbPrecision,
    UDWORD apiLevel);

UDWORD CvtTimestampChar_DB2(
    TIMESTAMP_STRUCT  * pstamp,
    CHAR   * rgbData,
    UWORD       cbData,
    UWORD       cbPrecision);

VOID CvtUltoa(
    UDWORD       value,
    CHAR   *  string);

VOID CvtUnpack(
    CHAR   * rgbNumeric,
    CHAR   * rgbDecimal,
    UWORD       cbDecimal);

UWORD CvtDoubleToChar (
    SDOUBLE FloatValue, 
    UWORD Precision,
    CHAR  * rgbData,
    UCHAR decimal_char); 

UWORD CvtStrSqlNumeric( 
    SQL_NUMERIC_STRUCT *numericValBuff,
    LPDESCFLD pird,
    SQLINTEGER cbData,
    CHAR  *rgbData ); 

UWORD CvtSqlNumericStr(
    CHAR  * DecimalStr,
    UWORD precision, 
    UWORD scale, 
    SQL_NUMERIC_STRUCT ns);

BOOL CvtApiTypes(
    CHAR * pTarget,
    CHAR *pSource, 
    VOID *envHndl,
    LPDESCFLD pipd, 
    UWORD dstDataType,
    UWORD srcDataType);

#endif  /* _INC_IDMSCVT */
