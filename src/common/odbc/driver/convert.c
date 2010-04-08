/*
** Copyright (c) 1993, 2007 Ingres Corporation
*/

#include <compat.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <gl.h>
#include <iicommon.h>
#include <cm.h>
#include <cv.h>
#include <me.h>
#include <mh.h>                     /* for 64-bit integer support in WINDOWS*/
#include <st.h> 
#include <tm.h> 
#include <iiapi.h>                  /* Ingres API */

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */

#include "idmsodbc.h"               /* ODBC driver definitions             */
#include "idmsocvt.h"               /* conversion functions */

#ifdef NT_AMD64
#undef Multiply128
#endif

/*
** Name: CONVERT.C
**
** Description:
**	Data conversion routines for ODBC driver.
**
** History:
**	28-jan-1993 (rosda01)
**	    Initial coding
**	14-mar-1995 (rosda01)
**	    ODBC 2.0 upgrade...
**	10-feb-1997 (tenge01)
**	    CvtTimestampChar, chged date time separator 
**	    to ' ', timestamp time separator to ':'.     
**	13-feb-1997 (tenge01)
**	    chgd TYPE_TABLE to use Ingres type and length
**	09-jun-1997 (tenge01)
**	    added CvtTimestampChar_DB2 
**	05-dec-1997 (tenge01)
**	    conver C run-time functions to CL
**	06-feb-1998 (thoda04)
**	    Fixed sizes from 16-bit to 32-bit
**	15-apr-1998 (thoda04)
**	    Set precision and size of blobs to 2G-1
**	20-may-1998 (thoda04)
**	    Strip trailing '.' for float->char conversions
**	17-jul-1998 (Dmitriy Bubis)
**	    Blank out '9999-12-31' date value
**	17-feb-1999 (thoda04)
**	    Ported to UNIX
**	10-mar-2000 (loera01)
**	    Bug 97372: Created CvtFloatToChar, which
**	(month/day unknown)-2000 (thoda04)
**	    uses _fcvt() to convert doubles to char. 
**	25-may-2000 (thoda04)
**	    Add a local cmkcheck function
**	16-oct-2000 (loera01)
**	    Added support for II_DECIMAL (Bug 102666).
**	17-jan-2001 (thoda04)
**	    Don't do a CMnext on packed decimal or binary data
**	12-jul-2001 (somsa01)
**	    Cleaned up 64-bit compiler errors.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**      19-aug-2002 (loera01) Bug 108536
**          First phase of support for SQL_C_NUMERIC. Add CvtSqlNumericChar()
**          and CvtSqlNumericDec().  A number of internal routines were added
**          for support of 128-bit integers.
**      30-aug-2002 (loera01)
**          Fixed casting error and a logic error in Subtract128.
**      15-oct-2002 (loera01)
**          Conditional reference to STchr() broke on HP compilers.  Broke
**          the reference into two statements.
**      04-Nov-2002 (loera01) Bug 108536
**          Second phase of support for SQL_C_NUMERIC.  Renamed 
**          CvtSqlNumericChar and CvtSqlNumericDec to CvtSqlNumericStr and
**          CvtStrSqlNumeric, respectively.  Added error checking.  Added
**          CvtApitypes() to convert IIAPI_CHA_TYPE to IIAPI_DEC_TYPE and
**          vice versa. 
**     20-dec-2002 (loera01) Bug 108536
**          In CvtStrSqlNumeric(), addressed accvio when input string had no
**          significant digits in front of the decimal point.
**          For CvtApiTypes(), fixed precision when converting from a decimal
**          to a char type.
**     17-jan-2003 (loera01) Bug 109459
**          In CvtStrSqlNumeric(), don't remove trailing zeros from the input
**          string.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      29-may-2003 (loera01)
**          Make inclusion of IIAPI_FORMATPARM dependent on IIAPI_VERSION_2
**	02-oct-2003 (somsa01)
**	    On AMD64, we have a Multiply128 intrinsic. Undefine this so that
**	    we can use what we have defined here. We may want to use the
**	    intrinsic for performance in the future.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**     27-apr-2004 (loera01)
**          First phase of support for SQL_BIGINT.
**     29-apr-2004 (loera01)
**          Second phase of support for SQL_BIGINT.
**     10-dec-2004 (Ralph.Loen@ca.com) SIR 113610
**          Added support for typeless nulls.
**     10-Aug-2006 (Ralph Loen) SIR 116477
**          Add support for ISO dates, times, and intervals (first phase). 
**     16-Aug-2006 (Ralph Loen) SIR 116477 (second phase)
**          Add support for ISO timestamps and DS intervals.
**     28-Sep-2006 (Ralph Loen) SIR 116477
**          Adjusted treatment of ISO date/times according to new API/ADF rules.
**     03-Oct-2006 (Ralph Loen) SIR 116477
**          Dynamic parameters for dates and timestamps are now sent as binary
**          values for backward-compatibility with ingresdate.:
**     23-Oct-2006 (Ralph Loen) SIR 116477
**          Obsoleted CvtCharIsoTime(); replaced with common 
**          GetTimeOrIntervalChar().
**     10-Nov-2006 (Ralph Loen) SIR 116477
**          In CvtIntervalChar(), add treatment of fractions to intervals
**          with a "seconds" component.
**     22-Nov-2006 (Ralph Loen) SIR 116477
**          In CvtCharInterval(), populated interval_type with the correct
**          enum.  Allowed for negative month values.
**     11-May-2007 (thoda04) Bug 118311
**          In CvtCharTime(), pass i4 fields to CVal(), not i2.
**     04-Feb-2008 (weife01) Bug 119724
**          IIAPI_TSWO_TYPE was overwritten in TYPE_TABLE, cause API 
**          conversion failure.
**     31-Jul-2008 (Ralph Loen) Bug 120718
**          In CvtApiTypes), allow for adjusted lengths in the case of
**          string to decimal conversion.  Dump obsolete #ifdef 
**          IIAPI_VERSION_2 macro.
**     14-Nov-2008 (Ralph Loen) SIR 121228
**         Add support for OPT_INGRESDATE configuration parameter.  If set,
**         ODBC date escape syntax is converted to ingresdate syntax, and
**         parameters converted to IIAPI_DATE_TYPE are converted instead to
**         IIAPI_DTE_TYPE.
**      05-May-2009 (Ralph Loen) Bug 122019
**         In CvtCharTimestamp(), correct conditional detecting a plus sign
**         ("+") in the timestamp string.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**    25-Jan-2010 (Ralph Loen) Bug 123175
**         In CvtCharTimestamp(), allow blank date strings, depending on
**         connection level.  In CvtTimestampChar() convert 9999-12-31
**         dates to blank strings, dependent on connection level.  
**    06-Feb-2010 (Ralph Loen) Bug 123228
**         Modify CvtGetSqlType() and CvtGetCType() to return Ingres date
**         data only if pstmt->dateConnLevel is IIAPI_LEVEL_3 and
**         the ODBC or SQL type is a timestamp or date type.
**    08-Feb-2010 (Ralph Loen)  SIR 123266
**         Replaced IIAPI_BYTE_TYPE with IIAPI_BOOL_TYPE in TYPE_TABLE
**         for SQL_C_BIT.  CvtGetSqlType() and CvtGetCType() return 
**         information for tinyints if the API connection level does 
**         not support boolean data types.  Replaced byte length for tinyints
**         to 1 from 2.  Replaced Ingres lengths for integers with
**         definitions from iiapi.h, instead of hard-coded references.
**         
*/

/*
**  Binary to character conversion table:
*/
static const CHAR  BIN_CHAR[] =
 {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static const SQLTYPEATTR INGRES_DATE_TYPE_TABLE[] = 
{
    { SQL_TYPE_DATE,     SQL_C_TYPE_DATE,           SQL_DATETIME,
        IIAPI_DTE_TYPE,  10,        10,      sizeof(DATE_STRUCT),      
        10,     TRUE, SQL_CODE_DATE },
    { SQL_TYPE_TIME,     SQL_C_TYPE_TIME,           SQL_DATETIME,
        IIAPI_DTE_TYPE,  8,         8,      sizeof(TIME_STRUCT),       
        8,     TRUE, SQL_CODE_TIME },
    { SQL_TYPE_TIMESTAMP,SQL_C_TYPE_TIMESTAMP,      SQL_DATETIME,
        IIAPI_DTE_TYPE,  19,        19,      sizeof(TIMESTAMP_STRUCT), 
        26,     TRUE, SQL_CODE_TIMESTAMP }
};

#define INGRES_DATE_TYPE_NUM sizeof(INGRES_DATE_TYPE_TABLE) \
/ sizeof(SQLTYPEATTR)

/*
**  ODBC data type attribute table:
*/
static const SQLTYPEATTR TYPE_TABLE[] =
{
/*    SQL type           C Type              Verbose Type
**    Ingres Type (parms) Precision Display  C Size                     
**    Ing length  Supported
*/
    { SQL_CHAR,          SQL_C_CHAR,        0,
        IIAPI_CHA_TYPE,  0,         0,      0,                         
        0,     TRUE, 0 },
    { SQL_VARCHAR,       SQL_C_CHAR,        0,
        IIAPI_VCH_TYPE,  0,         0,      0,                         
        0,     TRUE, 0 },
    { SQL_LONGVARCHAR,   SQL_C_CHAR,        0,
        IIAPI_LVCH_TYPE,  MAXI4,    MAXI4,  0,                         
        0,     TRUE, 0 },
    { SQL_DECIMAL,       SQL_C_CHAR,        0,
        IIAPI_DEC_TYPE,       0,         0,      0,                         
        0,     TRUE, 0 },
    { SQL_NUMERIC,       SQL_C_CHAR,                0,
        IIAPI_DEC_TYPE,  0,         0,      0,                         
        0,     TRUE, 0 },
    { SQL_NUMERIC,       SQL_C_NUMERIC,             0,
        IIAPI_DEC_TYPE,  DB_MAX_DECPREC,   DB_MAX_DECPREC+3, 
        sizeof(SQL_NUMERIC_STRUCT), 
        8,     TRUE, 0 },
    { SQL_BIT,           SQL_C_BIT,                 0,
        IIAPI_BOOL_TYPE, 1,         1,      sizeof(UCHAR),             
        IIAPI_I1_LEN,     TRUE, 0 },
    { SQL_TINYINT,       SQL_C_STINYINT,            0,
        IIAPI_INT_TYPE,   3,         4,      sizeof(SCHAR),             
        IIAPI_I1_LEN,     TRUE, 0 },
    { SQL_TINYINT,       SQL_C_UTINYINT,            0,
        IIAPI_INT_TYPE,   3,         4,      sizeof(SCHAR),             
        IIAPI_I1_LEN,     TRUE, 0 },
    { SQL_TINYINT,       SQL_C_TINYINT,             0,
        IIAPI_INT_TYPE,   3,         4,      sizeof(SCHAR),             
        IIAPI_I1_LEN,     TRUE, 0 },
    { SQL_SMALLINT,      SQL_C_SSHORT,              0,
        IIAPI_INT_TYPE,   5,         6,      sizeof(SWORD),             
        IIAPI_I2_LEN,     TRUE, 0 },
    { SQL_SMALLINT,      SQL_C_USHORT,              0,
        IIAPI_INT_TYPE,   5,         6,      sizeof(SWORD),             
        IIAPI_I2_LEN,     TRUE, 0 },
    { SQL_SMALLINT,      SQL_C_SHORT,               0,
        IIAPI_INT_TYPE,   5,         6,      sizeof(SWORD),             
        IIAPI_I2_LEN,     TRUE, 0 },
    { SQL_INTEGER,       SQL_C_SLONG,               0,
        IIAPI_INT_TYPE,   10,        11,      sizeof(SDWORD),            
        IIAPI_I4_LEN,     TRUE, 0 },
    { SQL_INTEGER,       SQL_C_ULONG,               0,
        IIAPI_INT_TYPE,  10,        11,      sizeof(SDWORD),            
        IIAPI_I4_LEN,     TRUE, 0 },
    { SQL_INTEGER,       SQL_C_LONG,                0,
        IIAPI_INT_TYPE,  10,        11,      sizeof(SDWORD),            
        IIAPI_I4_LEN,     TRUE, 0 },
    { SQL_BIGINT,        SQL_C_UBIGINT,             0,
        IIAPI_INT_TYPE,  19,        20,      sizeof(ODBCINT64),         
        IIAPI_I8_LEN,     TRUE, 0 },
    { SQL_BIGINT,        SQL_C_SBIGINT,              0,
        IIAPI_INT_TYPE,   19,        20,      sizeof(ODBCINT64),         
        IIAPI_I8_LEN,     TRUE, 0 },
    { SQL_REAL,          SQL_C_FLOAT,               0,
        IIAPI_FLT_TYPE,   7,        13,      sizeof(SFLOAT),            
        4,     TRUE, 0 },
    { SQL_FLOAT,         SQL_C_DOUBLE,              0,
        IIAPI_FLT_TYPE,   15,        22,      sizeof(SDOUBLE),           
        8,     TRUE, 0 },
    { SQL_DOUBLE,        SQL_C_DOUBLE,              0,
        IIAPI_FLT_TYPE,  15,        22,      sizeof(SDOUBLE),           
        8,     TRUE, 0 },
    { SQL_BINARY,        SQL_C_BINARY,              0,
        IIAPI_BYTE_TYPE,  0,         0,      0,                         
        0,     TRUE, 0 },
    { SQL_VARBINARY,     SQL_C_BINARY,              0,
        IIAPI_VBYTE_TYPE, 0,         0,      0,                         
        0,     TRUE, 0 },
    { SQL_LONGVARBINARY, SQL_C_BINARY,              0,
        IIAPI_LBYTE_TYPE,MAXI4,      MAXI4/2*2,     0,                         
        0,     TRUE, 0 },
    { SQL_TYPE_DATE,     SQL_C_TYPE_DATE,           SQL_DATETIME,
        IIAPI_DATE_TYPE, 10,        10,      sizeof(DATE_STRUCT), 
        10,    TRUE, SQL_CODE_DATE },
    { SQL_TYPE_TIME,     SQL_C_TYPE_TIME,           SQL_DATETIME,
        IIAPI_TMWO_TYPE, 8,         8,       sizeof(TIME_STRUCT), 
        21,    TRUE, SQL_CODE_TIME },
    { SQL_TYPE_TIMESTAMP,SQL_C_TYPE_TIMESTAMP,      SQL_DATETIME,
        IIAPI_TSWO_TYPE, 19,        19,      sizeof(TIMESTAMP_STRUCT), 
         ODBC_TSWO_OUTLENGTH+1,     TRUE, SQL_CODE_TIMESTAMP },
    { SQL_INTERVAL_YEAR,  SQL_C_INTERVAL_YEAR,      SQL_INTERVAL,
        IIAPI_INTYM_TYPE, ODBC_INTYM_LEN, ODBC_INTYM_LEN,      
        sizeof(SQL_YEAR_MONTH_STRUCT),ODBC_INTYM_OUTLENGTH+1,TRUE,SQL_CODE_YEAR },
    { SQL_INTERVAL_MONTH, SQL_C_INTERVAL_MONTH,     SQL_INTERVAL,
        IIAPI_INTYM_TYPE, ODBC_INTYM_LEN, ODBC_INTYM_LEN,      
        sizeof(SQL_YEAR_MONTH_STRUCT), ODBC_INTYM_OUTLENGTH+1,    TRUE, SQL_CODE_MONTH },
    { SQL_INTERVAL_YEAR_TO_MONTH,SQL_C_INTERVAL_YEAR_TO_MONTH, SQL_INTERVAL,
        IIAPI_INTYM_TYPE, ODBC_INTYM_LEN, ODBC_INTYM_LEN,
        sizeof(SQL_YEAR_MONTH_STRUCT), ODBC_INTYM_OUTLENGTH+1, TRUE, 
        SQL_CODE_YEAR_TO_MONTH },
    { SQL_INTERVAL_DAY,   SQL_C_INTERVAL_DAY,       SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN,
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1, TRUE, SQL_CODE_DAY },
    { SQL_INTERVAL_HOUR,  SQL_C_INTERVAL_HOUR,      SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN,
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1,TRUE, SQL_CODE_HOUR },
    { SQL_INTERVAL_MINUTE,SQL_C_INTERVAL_MINUTE,    SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN,      
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1, TRUE, SQL_CODE_MINUTE },
    { SQL_INTERVAL_SECOND,SQL_C_INTERVAL_SECOND,    SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN,      
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1,  TRUE, SQL_CODE_SECOND },
    { SQL_INTERVAL_DAY_TO_HOUR,SQL_C_INTERVAL_DAY_TO_HOUR,SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN, 
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1, TRUE, 
        SQL_CODE_DAY_TO_HOUR },
    { SQL_INTERVAL_DAY_TO_MINUTE,SQL_C_INTERVAL_DAY_TO_MINUTE,SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN, 
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1, TRUE, 
        SQL_CODE_DAY_TO_MINUTE },
    { SQL_INTERVAL_DAY_TO_SECOND,SQL_C_INTERVAL_DAY_TO_SECOND,SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN, 
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1, TRUE, 
        SQL_CODE_DAY_TO_SECOND },
    { SQL_INTERVAL_HOUR_TO_MINUTE,SQL_C_INTERVAL_HOUR_TO_MINUTE,SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN, 
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1, TRUE, 
        SQL_CODE_HOUR_TO_MINUTE },
    { SQL_INTERVAL_HOUR_TO_SECOND,SQL_C_INTERVAL_HOUR_TO_SECOND,SQL_INTERVAL,
        IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, ODBC_INTDS_LEN, 
        sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1, TRUE, 
        SQL_CODE_HOUR_TO_SECOND },
    { SQL_INTERVAL_MINUTE_TO_SECOND,SQL_C_INTERVAL_MINUTE_TO_SECOND,
        SQL_INTERVAL, IIAPI_INTDS_TYPE, ODBC_INTDS_LEN, 
        ODBC_INTDS_LEN, sizeof(SQL_INTERVAL_STRUCT), ODBC_INTDS_OUTLENGTH+1, 
        TRUE, SQL_CODE_HOUR_TO_SECOND },
    { SQL_TYPE_NULL,     SQL_C_TYPE_NULL,           0,
        IIAPI_LTXT_TYPE, 0,         0,      0,                         
        0,     TRUE, 0 },
# ifdef IIAPI_NCHA_TYPE
    { SQL_WCHAR ,        SQL_C_WCHAR,               0,
        IIAPI_NCHA_TYPE, 0,         0,      0,                         
        0,     TRUE, 0 },
    { SQL_WVARCHAR,      SQL_C_WCHAR,               0,
        IIAPI_NVCH_TYPE, 0,         0,      0,                         
        0,     TRUE, 0 },
    { SQL_WLONGVARCHAR,  SQL_C_WCHAR,               0,
        IIAPI_LNVCH_TYPE, MAXI4, MAXI4,      0,                         
        0,     TRUE, 0 },
# else
    { SQL_WCHAR ,        SQL_C_WCHAR,               0, 
       0,     0,         0,      0,                         
       0,     FALSE, 0 },
    { SQL_WVARCHAR,      SQL_C_WCHAR,               0,
        0,    0,         0,      0,                         
        0,     FALSE, 0 },
    { SQL_WLONGVARCHAR,  SQL_C_WCHAR,               0,
        0, MAXI4, MAXI4,      0,                         
        0,     FALSE, 0 }
# endif
};
#define TYPE_NUM sizeof(TYPE_TABLE) / sizeof(SQLTYPEATTR)
#define BIT_32 0x80000000

/*
**  Internal conversion routines.
*/

void Add128( UCHAR *, UCHAR *, UCHAR * );
void Multiply128( UCHAR *, UCHAR * );
void Subtract128( UCHAR *,  UCHAR *, UCHAR * );
void Divide128( UCHAR *, UCHAR *, UCHAR * );
BOOL LessThanOrEqual128( UCHAR *, UCHAR * );
BOOL EqualZero128( UCHAR * );
void RollRight128( UCHAR * ); 
void RollLeft128( UCHAR * ); 

/*
**  CvtBigChar
**
**  Convert an SQL long integer to a character value.
**
**  On entry: szValue -->where to return character value.
**            rbgData -->SQL_BIGINT data.
**
**  On exit:  szValue contains character string.
**
**  Returns:  length of character string.
**
**  Note:     Output buffer must be at least 21 bytes.
**
**            This is not used in the 32 bit driver because
**            long double is now really double, and only has
**            53 bits of precision.  Instead we fetch all
**            LONG INTEGERS as DECIMAL, and let the server
**            do the work.
*/
#ifdef WIN16

UWORD CvtBigChar(
    CHAR      * szValue,
    CHAR      * rgbData)
{
    CHAR        sz[21];
    UWORD       cb, cbValue;
    SDOUBLE     hiValue, loValue;
    long double floatValue;

    floatValue = CvtBigFloat (rgbData);

    hiValue = (SDOUBLE)floatValue;
    loValue = (SDOUBLE)(floatValue - hiValue);

    _gcvt (hiValue, 21, szValue);
    cbValue = STlength (szValue);
    if (cbValue  &&  szValue[cbValue-1]=='.')
        szValue[--cbValue]='\0';   /* strip trailing '.' */

    if  (loValue > 0)
    {
        _gcvt (loValue, sizeof(sz), sz);
        cb = STlength (sz);
        if (cb  &&  sz[cb-1]=='.')
            sz[--cb]='\0';   /* strip trailing '.' */
        STcopy (sz, szValue + cbValue - cb);
    }
    cbValue--;
    *(szValue + cbValue) = 0;
    return (cbValue);
}

#endif


/*
**  CvtBigFloat
**
**  Convert an SQL long integer to a floating point value.
**
**  On entry: rbgData-->SQL_BIGINT data.
**
**  Returns:  long double precision floating point value.
**
**  Note: Win32 maps long double to double.
*/

long double CvtBigFloat(
    CHAR      * rgbData)
{
    SDWORD      hiInt;
    UDWORD      loInt;
    long double floatValue;

    hiInt = *((SDWORD     *)rgbData + 1);
    loInt = *(UDWORD     *)rgbData;

    floatValue  = (long double)hiInt * 4294967296.0L + (long double)loInt;

    return (floatValue);
}


/*
**  CvtBinChar
**
**  Convert binary data to a character string value.
**
**  On entry: szValue -->where to return character value.
**            rbgData -->binary data.
**            cbData   = length of binary data.
**
**  On exit:  szValue contains character string.
**
**  Returns:  nothing.
**
**  Note:     Output buffer assumed large enough.
*/

VOID CvtBinChar(
    CHAR      * szValue,
    CHAR      * rgbData,
    UWORD       cbData)
{
    CHAR      * sz;
    CHAR      * rgb;
    UWORD       i;

    sz  = szValue;
    rgb = rgbData;

    for (i = 0; i < cbData; i++)
    {
        *sz++ = BIN_CHAR[*rgb >> 4 & 0x0F];
        *sz++ = BIN_CHAR[*rgb & 0x0F];
        rgb++;
    }
    CMcpychar("\0",sz); 

    return;
}


/*
**  CvtCharBig
**
**  Convert a character value to a SQL BIGINT (LONINT).
**
**  On entry: rgbData -->where to return BIGINT data.
**            rgbValue-->character value.
**            cbValue  = length of character value.
**
**  On exit:  Value is returned if successful.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_TRUNCATION   if fraction digits truncated.
**            CVT_OUT_OF_RANGE if significant digits truncated.
**            CVT_ERROR        if not a number.
*/

#ifndef WIN16

/*
**  32 bit version:
**
*/
static void CvtNumStrInt (CHAR  *, UDWORD *, int);

UDWORD CvtCharBig(
    CHAR      * rgbData,
    CHAR      * rgbValue,
    UWORD       cbValue)
{
    UDWORD      rc;
    CHAR        szValue[34];
    CHAR  *     p;
    UWORD       i;
    UDWORD *    pdwLow  = (UDWORD *)rgbData;
    UDWORD *    pdwHigh = (UDWORD *)rgbData + 1;
    BOOL        fMinus = FALSE;

    /*
    **  Get a numeric string from character value:
    */
    rc = CvtCharNumStr (szValue, sizeof (szValue), rgbValue, cbValue, &p, &i);
    if (rc != CVT_SUCCESS) return (rc);
    if (p) return (CVT_ERROR);

    /*
    **  Find fraction, if any, make sure it's just trailing 0's,    
    **  and terminate string at decimal point.
    */
    p = STindex (szValue, ".", 0);
    if (p)
    {
        *p = 0;
		CMnext(p);
        while (*p)
		{
            if (*p != EOS)
                return CVT_TRUNCATION;
			CMnext(p);
		}
    }

    /*
    **  Check sign, if any, and replace sign with leading 0.
    **  If smallest negative number, we're done now.
    */
    switch (szValue[0])
    {
    case '-':

        fMinus = TRUE;
        if (STcompare (szValue, "-9223372036854775808") == 0)
        {
            *pdwLow  = 0xFFFFFFFF;
            *pdwHigh = 0xFFFFFFFF;
            return CVT_SUCCESS;
        }

    case '+':

        szValue[0] = '0';
    }
    /*
    **  Convert string to long integer in pieces, factoring out 2's,
    **  after each call szValue has what's left to convert:
    */
    CvtNumStrInt (szValue, pdwLow,  32);
    CvtNumStrInt (szValue, pdwHigh, 31);

    /*
    **  2's complement if negative:
    */
    if (fMinus)
    {
        if (*pdwLow = 0xFFFFFFFF)
        {
            *pdwLow = 0;
            *pdwHigh++;
        }
        else
        {
            *pdwLow++;
        }
    }
        
    /*
    **  If all significant digits fit, we're OK:
    */
    p = szValue;
    while (!CMcmpcase(p,"0")) CMnext(p);
    return (*p == EOS) ? CVT_SUCCESS : CVT_OUT_OF_RANGE;
}

/*
**  CvtNumStrInt
**
**  Local subroutine to convert a numeric string into an integer,
**  allows an abitrarily long string to be converted into an integer
**  of any length that is a multiple of 4.  This works by repeatedly
**  dividing by 2 and ORing the remainder as the corresponding power
**  of 2 into the result.
**  
**  On entry: szValue -->numeric string.
**            rgbValue-->output integer destination.
**
**  On exit:  szValue -->numeric string divided by 2.
**            rgbValue-->4 bytes of integer value of string.
**
**  Returns:  Nothing.
*/
static void CvtNumStrInt (CHAR  * szValue, UDWORD * rgbValue, int iBits)
{
    int     d, i, n, r = 0;
    CHAR  * p;
    
    *rgbValue = 0;
    
    for (i = 0, p = szValue; i < iBits && *p; i++)
    {
        while (!CMcmpcase(p,"0")) CMnext(p);   /* Skip any leading 0's in string. */

        while (*p != EOS)                      /* Divide numeric character string by 2 */
        {
            n = r + (*p - '0');         /*   n = digit value. */
            d = n / 2;                  /*   d = half of it. */
            r = n % 2;                  /*   r = remainder (1 or 0). */
            *p = (CHAR)('0' + d);       /*   store result as character. */
			CMnext(p);
        }
        if (r) *rgbValue |= 2^i;        /* OR in remainder as power of 2. */
    }
    return;
}

#else  /* end not WIN16 */

/*
**  16 bit version:
**
*/
UDWORD CvtCharBig(
    CHAR      * rgbData,
    CHAR      * rgbValue,
    UWORD       cbValue)
{
    UDWORD      rc;
    CHAR        szValue[34];
    CHAR  *     p;
    UWORD       i;
    long double floatValue;

    /*
    **  Get a numeric string from character value:
    */
    rc = CvtCharNumStr (szValue, sizeof (szValue), rgbValue, cbValue, &p, &i);
    if (rc != CVT_SUCCESS) return (rc);
    if (p) return (CVT_ERROR);

    /*
    **  Convert it to an 80 bit floating point value
    **  (which has 64 bits of decimal precision), 
    **  and convert that to a BIGINT:
    */
    floatValue = _strtold ((const CHAR      *)szValue, NULL);
    return (CvtFloatBig (rgbData, floatValue));
}

#endif /* WIN16 */


/*
**  CvtCharBin
**
**  Convert a character string value binary to data.
**
**  On entry: rbgData -->where to return binary data.
**            rbgValue-->character representation of binary value.
**            cbData   = length of binary data.
**
**  On exit:  Value is returned if valid.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_ERROR        if not a binary representation.
*/

UDWORD CvtCharBin(
    CHAR      * rgbData,
    CHAR      * rgbValue,
    UDWORD      cbData)
{
    CHAR      * pc;
    CHAR      * pb;
    UWORD       i;

    pc = rgbValue;

    for (i = 0, pb = rgbData;
         i < cbData;
         i++, pb++)
    {
        /*
        **  High nybble:
        */
        if (CMdigit (pc))
            *pb = (CHAR)((*pc - '0') << 4);
        else
        if (CMhex (pc))
            *pb = (CHAR)((CMtoupper (pc, pc) - 'A' + 10) << 4);
        else
            return (CVT_ERROR);
        CMnext(pc);

        /*
        **  Low nybble:
        */
        if (CMdigit (pc))
            *pb |= *pc - '0';
        else
        if (CMhex (pc))
            *pb |= (CHAR)(CMtoupper (pc, pc) - 'A' + 10);
        else
            return (CVT_ERROR);
        CMnext(pc);

    }
    return (CVT_SUCCESS);
}


/*
**  CvtCharDate
**
**  Convert character data or an SQL date to an ODBC date value.
**
**  On entry: pdate  ->ODBC date structure to receive value.
**            rbgData-->character data.
**            cbData  = length of character data.
**
**  On exit:  Value is returned if a valid date.
**
**  Returns:  CVT_SUCCESS
**            CVT_ERROR
*/

UDWORD CvtCharDate(
    DATE_STRUCT     * pValue,
    CHAR            * rgbData,
    UWORD             cbData)
{
    UDWORD      rc;
    DATE_STRUCT date;
    CHAR        sz[11];
    UWORD       i;

    /*
    **  Extract assumed date from SQL character data:
    */
    if (CvtGetToken (sz, 10, rgbData, cbData) != CVT_SUCCESS)
        return (CVT_ERROR);

    /*
    **  Extract year:
    */
    for (i = 0; i < 4; i++)
        if (!CMdigit (&sz[i]))
            return (CVT_ERROR);

    CMcpychar("\0",&sz[4]); 
    date.year = (SQLSMALLINT)atoi (&sz[0]);

    /*
    **  Extract month:
    */
    for (i = 5; i < 7; i++)
        if (!CMdigit (&sz[i]))
            return (CVT_ERROR);

    CMcpychar("\0",&sz[7]); 
    date.month = (SQLUSMALLINT)atoi (&sz[5]);

    /*
    **  Extract day:
    */
    for (i = 8; i < 10; i++)
        if (!CMdigit (&sz[i]))
            return (CVT_ERROR);

    CMcpychar("\0",&sz[10]); 
    date.day = (SQLUSMALLINT)atoi (&sz[8]);

    /*
    **  Return to caller if valid:
    */
    rc = CvtEditDate(&date);

    if (rc == CVT_SUCCESS)
    {
        pValue->year  = date.year;
        pValue->month = date.month;
        pValue->day   = date.day;
    }

    return (rc);
}


/*
**  CvtCharFloat
**
**  Convert character data to a double precision floating point value.
**
**  On entry: pValue -->where to return converted value.
**            rbgData-->character data.
**            cbData  = length of character data.
**
**  On exit:  Value is returned if not out of range or invalid.
**
**  Returns:  CVT_SUCCESS       Valid number found.
**            CVT_OUT_OF_RANGE  Exponent greater than 308.
**            CVT_ERROR         Not a valid number.
*/

UDWORD CvtCharFloat(
    SDOUBLE     * pValue,
    CHAR      * rgbData,
    UWORD       cbData)
{
    UDWORD      rc;
    SDOUBLE     floatValue;
    CHAR        szFloat[100], szExp[6];
    CHAR      * c;
    UWORD       i;
    STATUS      status;

    c = rgbData;
    i = cbData;

    /*
    **  Extract mantissa from character data:
    */
    rc = CvtCharNumStr (szFloat, sizeof(szFloat) - sizeof(szExp), c, i, &c, &i);
    if (rc == CVT_ERROR || rc == CVT_OUT_OF_RANGE)
        return (rc);

    /*
    **  Extract exponent, if any:
    */
    if (c) 
    {
        CMtoupper (c, c);
        if (*c == 'E')
        {
            CMnext(c);
            i--;
            if (i == 0)
                return (CVT_ERROR);
    
        /*
        **  Extract numeric value of exponent from character data,
        **  return an error if not numeric, decimal in exponent,
        **  or something other than trailing white space in data.
        */
            rc = CvtCharNumStr (szExp, sizeof(szExp), c, i, &c, &i);
            if ((rc == CVT_ERROR) || (rc == CVT_OUT_OF_RANGE))
                return (rc);
            if (STindex (szExp, ".", 0) || c)
                return (CVT_ERROR);
    
        /*
        **  Add exponent to floating point string:
        */
            if (!CMcmpcase(&szExp[0],"-") )
                STcat (szFloat, "E");
            else
                STcat (szFloat, "E+");
            STcat (szFloat, szExp);
        }
    }

    /*
    **  Convert edited string to a float and check range:
    */
    status = CVaf(szFloat, '.', &floatValue);
    if (status != OK)
        return (CVT_OUT_OF_RANGE);

    *pValue = floatValue;
    return (CVT_SUCCESS);

}


/*
**  CvtCharInt
**
**  Convert character data to a long integer value:
**
**  On entry: pValue -->where to return value.
**            rbgData-->character data.
**            cbData  = length of character data.
**
**  On exit:  Value is returned if not out of range or error.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_TRUNCATION   if fraction digits truncated.
**            CVT_OUT_OF_RANGE if significant digits truncated.
**            CVT_ERROR        if not a number.
*/

UDWORD CvtCharInt(
    SDWORD     * pValue,
    CHAR      * rgbData,
    UWORD       cbData)
{
    UDWORD      rc;
    i4          intValue;
    i4          intTemp;
    CHAR        szInt[12];
    CHAR      * c;
    UWORD       i;
    STATUS      status;

    c = rgbData;
    i = cbData;

    /*
    **  Extract integer data,
    **  note error if trailing data is not white space:
    */
    rc = CvtCharNumStr (szInt, sizeof(szInt), c, i, &c, &i);
    if (rc == CVT_ERROR || rc == CVT_OUT_OF_RANGE)
        return (rc);
    if (c)
        return (CVT_ERROR);

    /*
    **  Convert numeric string to long integer:
    */
    if ((c = STindex(szInt, ".", 0)) != NULL) /* first, find fraction part */
       {char * t;
        t=c;         /* t-> '.' */
        CMnext(c);   /* c-> fraction string just past '.' */
        *t = EOS;    /* null term the integer part        */
       }

    status = CVal(szInt, &intValue); /* convert char szInt to int intValue */
    if (status != OK)
        return (CVT_OUT_OF_RANGE);

    if (c)    /* if fractional part, it better zeros */
       {
        intTemp = 0;
        status = CVal(c, &intTemp); /* convert fractional part to int */
        if (intTemp != 0)           /* if fraction != 0 */
            rc = CVT_TRUNCATION;    /*    return (TRUNCATION) */
       }

    *pValue = intValue;

    return (rc);

}

/*
**  CvtCharInt64
**
**  Convert character data to a "long long" integer value:
**
**  On entry: pValue -->where to return value.
**            rbgData-->character data.
**            cbData  = length of character data.
**
**  On exit:  Value is returned if not out of range or error.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_TRUNCATION   if fraction digits truncated.
**            CVT_OUT_OF_RANGE if significant digits truncated.
**            CVT_ERROR        if not a number.
*/

UDWORD CvtCharInt64(
    ODBCINT64 * pValue,
    CHAR      * rgbData,
    UWORD       cbData)
{
    UDWORD      rc;
    ODBCINT64   intValue;
    ODBCINT64   intTemp;
    CHAR        szInt[24];
    CHAR      * c;
    UWORD       i;
    STATUS      status;

    c = rgbData;
    i = cbData;

    /*
    **  Extract integer data,
    **  note error if trailing data is not white space:
    */
    rc = CvtCharNumStr (szInt, sizeof(szInt), c, i, &c, &i);
    if (rc == CVT_ERROR || rc == CVT_OUT_OF_RANGE)
        return (rc);
    if (c)
        return (CVT_ERROR);

    /*
    **  Convert numeric string to long integer:
    */
    if ((c = STindex(szInt, ".", 0)) != NULL) /* first, find fraction part */
       {char * t;
        t=c;         /* t-> '.' */
        CMnext(c);   /* c-> fraction string just past '.' */
        *t = EOS;    /* null term the integer part        */
       }

    status = CVal8(szInt, (i8 *)&intValue); /* convert char szInt to int intValue */
    if (status != OK)
        return (CVT_OUT_OF_RANGE);

    if (c)    /* if fractional part, it better zeros */
       {
        intTemp = 0;
        status = CVal8(c, (i8 *)&intTemp); /* convert fractional part to int */
        if (intTemp != 0)           /* if fraction != 0 */
            rc = CVT_TRUNCATION;    /*    return (TRUNCATION) */
       }

    *pValue = intValue;

    return (rc);

}


/*
**  CvtCharNum
**
**  Convert character a value to SQL NUMERIC data.
**
**  On entry: rbgData    -->where to return NUMERIC data.
**            rgbValue    = character value.
**            cbValue     = length of character value.
**            cbPrecision = SQL precision.
**            cbScale     = SQL scale.
**            fUnsigned   = TRUE if UNSIGNED NUMERIC
**
**  On exit:  Value is returned if not truncated or out of range.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_TRUNCATION   if fraction digits truncated.
**            CVT_OUT_OF_RANGE if significant digits truncated.
**            CVT_ERROR        if not a number.
*/

UDWORD CvtCharNum(
    CHAR      * rgbData,
    CHAR      * rgbValue,
    UWORD       cbValue,
    SWORD       cbPrecision,
    SWORD       cbScale,
    BOOL        fUnsigned)
{
    UDWORD      rc;
    CHAR        szValue[34];
    CHAR      * sz;
    CHAR      * c;
    CHAR      * p;
	CHAR      * p1;
    UWORD       i;
    int         dec, sign;

    /*
    **  Make sure character value is really a number:
    */
    rc = CvtCharNumStr (szValue, sizeof (szValue), rgbValue, cbValue, &c, &i);
    if (rc != CVT_SUCCESS)
        return (rc);
    if (c)
        return (CVT_ERROR);
    sz = szValue;

    /*
    **  Convert to _fcvt format:
    **
    **  1. Extract sign.
    **  2. Strip leading 0's.
    **  3. Extract decimal, if any.
    */
    sign = 0;
    if (!CMcmpcase(sz,"-"))
    {
        CMnext(sz);
        sign = 1;
        if (fUnsigned)
            return (CVT_OUT_OF_RANGE);
    }

    while (!CMcmpcase(sz,"0")) 
        CMnext(sz);

    p = STindex (sz, ".", 0);
    if (p == NULL)
        dec = (int)STlength (sz);
    else
    {
        dec = (int)(p - sz);
		p1 = p;
		CMnext(p1);
        STcopy (p1, p);

        /*
        **  Strip extra 0's in fraction:
        **
        **  1. Leading 0's if no integer part.
        **  2. Trailing 0's.
        */
        if (dec == 0)
        {
            while (!CMcmpcase(sz,"0"))
            {
                CMnext(sz);
                dec--;
            }
        }

        p = sz + STlength (sz) - 1;
        while (!CMcmpcase(p,"0"))
        {
            *p = 0;
            CMprev(p, sz);
        }
    }

    /*
    **  Convert to zoned decimal:
    */
    return (CvtNumNum (rgbData, cbPrecision, cbScale, sz, dec, sign));
}


/*
**  CvtCharNumStr
**
**  Convert character data to a numeric string value.
**  Leading and trailing zeros are suppressed, string is null
**  terminated, and leading and trailing spaces are skipped.
**
**  On entry: szValue   -->where to return numeric value.
**            cbValueMax = max length of output buffer.
**            rgbData   -->character data.
**            cbData     = length of character data.
**            prgbNext  -->where to return -->next character in data.
**            pcbNext   -->where to return length remaining.
**
**  On exit:  Value is always returned, even if some error.
**
**  Returns:  CVT_SUCCESS       if converted successfully.
**            CVT_TRUNCATION    if fraction truncated.
**            CVT_OUT_OF_RANGE  if significant digits truncated.
**            CVT_ERROR         if not numeric.
*/

UDWORD CvtCharNumStr(
    CHAR      * szValue,
    SDWORD      cbValueMax,
    CHAR      * rgbData,
    UWORD       cbData,
    CHAR      * FAR * prgbNext,
    UWORD     * pcbNext)
{
    CHAR      * c;
    CHAR      * p;
    CHAR      * sz;
    UWORD       i;
    SDWORD      cbMax;
    SWORD       cbPrec, cbScale;
    UWORD       fZero;

    c       = rgbData;
    i       = cbData;
    sz      = szValue;
    cbMax   = cbValueMax - 1;
    cbPrec  = 0;
    cbScale = 0;
    fZero   = FALSE;

    /*
    **  Skip leading white space, if any:
    */
    c = CvtSkipSpace (c, i, &i);
    if (c == NULL)
        return (CVT_ERROR);

    /*
    **  Get sign, if any:
    */

    switch (*c)
    {
    case '-':

        *sz = *c;
        CMnext(sz);
        cbMax--;

    case '+':

        CMnext(c); 
        i--;
        if (i == 0)
            return (CVT_ERROR);
    }

    /*
    **  Get significant digits:
    */
    while ((i > 0) && (*c == '0'))
    {
        fZero = TRUE;
        CMnext(c);
        i--;
    }
    while ((i > 0) && (CMdigit (c)))
    {
        if (cbPrec < cbMax)
        {
            *sz = *c;
            CMnext(sz);
        }
        CMnext(c); 
        i--;
        cbPrec++;
    }
    if (cbPrec == 0 && fZero)
    {
        *sz  = '0';
        CMnext(sz);
        cbPrec = 1;
    }

    /*
    **  Get fraction, if any:
    */
    if (i > 0 && !CMcmpcase(c,"."))
    {
        if (cbPrec < cbMax)
        {
            CMcpychar(".",sz); 
			CMnext(sz);
            cbMax--;
        }
        CMnext(c); 
        i--;

        while ((i > 0) && (CMdigit (c)))
        {
            if (cbPrec < cbMax)
            {
                CMcpychar(c,sz);
                CMnext(sz);
            }
            cbScale++;
            cbPrec++;
            CMnext(c);
            i--;
        }
        /*
        **  Adjust scale and precision for trailing 0's,
        **  dump "." if nothing after it:
        */
        p = c;
        while ((cbScale > 0) && (*p == '0'))
        {
            CMprev(p, szValue);
            cbPrec--;
            cbScale--;
            if (cbPrec < cbMax)
                CMprev(sz, szValue);
        }
        if (cbScale == 0)
            CMprev(sz, szValue);
    }

    /*
    **  Null terminate:
    */
    CMcpychar("\0",sz); 

    /*
    **  Skip trailing white space, if any:
    */
    c = CvtSkipSpace (c, i, &i);
    if (prgbNext)
        *prgbNext = c;
    if (pcbNext)
        *pcbNext  = i;

    /*
    **  Tell caller if numeric value fit in buffer:
    */
    if (cbPrec - cbScale > cbMax)
        return (CVT_OUT_OF_RANGE);

    if (cbPrec > cbMax)
        return (CVT_TRUNCATION);

    return (CVT_SUCCESS);
}


/*
**  CvtCharInterval
**
**  Convert character data or an SQL interval to an ODBC interval value.
**
**  On entry: pValue -->ODBC interval structure to receive value.
**            rbgData-->character data.
**            cbData  = length of character data.
**
**  On exit:  Value returned if a valid time.
**
**  Returns:  CVT_SUCCESS
**            CVT_ERROR
*/

UDWORD CvtCharInterval(
    CHAR * rgbData,
    SQL_INTERVAL_STRUCT *interval,
    SWORD fSqlType)
{
    UDWORD      rc = CVT_SUCCESS;
    CHAR        intervalStr[80], *sz = &intervalStr[0], *szSave = &intervalStr[0];
    i4 i;
    BOOL hasYear = FALSE;
    BOOL hasMonth = FALSE;
    BOOL hasDay = FALSE;
    BOOL hasHour = FALSE;
    BOOL hasMinute = FALSE;
    BOOL hasSecond = FALSE;

    SQL_YEAR_MONTH_STRUCT *ym = &interval->intval.year_month;
    SQL_DAY_SECOND_STRUCT *ds = &interval->intval.day_second;
    BOOL isYearMonth=FALSE;
    i2 precision = 4;
    SQLINTEGER intValue = 0;
    CHAR *p = NULL;
    CHAR *p1 = NULL;
	CHAR *p2 = NULL;

    hasYear = STstrindex(rgbData,"yr", 0, FALSE) != NULL ? TRUE : FALSE;
    hasMonth = STstrindex(rgbData,"mo", 0, FALSE) != NULL ? TRUE : FALSE;
    hasDay = STstrindex(rgbData,"day", 0, FALSE) != NULL ? TRUE : FALSE;
    hasHour = STstrindex(rgbData,"hr", 0, FALSE) != NULL ? TRUE : FALSE;
    hasMinute = STstrindex(rgbData,"min", 0, FALSE) != NULL ? TRUE : FALSE;
    hasSecond = STstrindex(rgbData,"sec", 0, FALSE) != NULL ? TRUE : FALSE;

    ym->year = 0;
    ym->month = 0;
    ds->day = 0;
    ds->hour = 0;
    ds->minute = 0;
    ds->second = 0;
    ds->fraction = 0;

    STlcopy(rgbData, sz, STlength(rgbData));
    /* 
    ** Extract sign, if present. 
    */
    if (*sz == '+' || *sz == '-')
    {
        interval->interval_sign = ( *sz == '-' ? SQL_TRUE : SQL_FALSE);
        CMnext(sz);
        szSave = sz;
    }
    else
        interval->interval_sign = 0;

    switch (fSqlType)
    {
        case SQL_INTERVAL_YEAR:
        case SQL_INTERVAL_MONTH:
        case SQL_INTERVAL_YEAR_TO_MONTH:
            if (hasDay || hasHour || hasMinute || hasSecond)
            {
                rc = CVT_OUT_OF_RANGE;
                goto funcReturn;
            }
            interval->interval_type = SQL_IS_YEAR_TO_MONTH;
            if (hasYear || hasMonth)
            {    
                if (hasYear)
                {
                    /*
                    **  Extract year.
                    */
                    for (i = 0; i < precision && !CMwhite(sz); i++)
                    {
                        if (!CMdigit (sz))
                        {
                            rc = CVT_ERROR;
                            goto funcReturn;
                        }
                        CMnext(sz);
                    }
                    CMcpychar("\0",sz); 
                    CVal(szSave, &ym->year);
                    /* 
                    ** Truncate if interval contains a month, but is SQL_YEAR.
                    */
                    if (fSqlType == SQL_INTERVAL_YEAR)
                    {
                        ym->month = 0;
                        goto funcReturn;
                    }
                    CMnext(sz);
                    while (*sz && !CMdigit(sz)) CMnext(sz);
                    precision = 2;
                } /* if (hasYear) */
                if (hasMonth)
                {
                    szSave = sz;
                    /*
                    **  Extract month.
                    */
                    if (*sz == '+' || *sz == '-')
                    {
                        if (!interval->interval_sign)
                            interval->interval_sign = ( *sz == '-' ? SQL_TRUE : SQL_FALSE);
                        CMnext(sz);
                        szSave = sz;
                    }
                } /* if (hasMonth) */
                for (i = 0; i < precision && !CMwhite(sz); i++)
                {
                    if (!CMdigit(sz))
                    {
                        rc = CVT_ERROR;
                        goto funcReturn;
                    }
                    sz++;
                }
                CMcpychar("\0",sz); 
                CVal(szSave, &intValue);
                if (intValue < 0)
                    intValue = -intValue;
                ym->month = intValue;
                if (fSqlType == SQL_INTERVAL_MONTH)
                {
                    ym->month += ym->year * 12;
                    ym->year = 0;
                    goto funcReturn;
                }
                CMnext(sz);
                while (*sz && !CMdigit(sz)) CMnext(sz);
                goto funcReturn;
            } /* if hasYear || hasMonth */
            p = STindex(sz, "-", 0);
            if (p)
                CMcpychar("\0",p);
            if (CVal(sz, (i4 *)&intValue) != OK)
                rc = CVT_INVALID;
            else
                ym->year = intValue;
            if (p)
            {
                CMnext(p);
                if (CVal(p, (i4 *)&intValue) != OK)
                    rc = CVT_INVALID;
                else
                {
                    if (intValue < 0)
                        intValue = -intValue;
                    ym->month = intValue;
                }
            }
            goto funcReturn;
            break;
        
        case SQL_INTERVAL_DAY:
        case SQL_INTERVAL_HOUR:
        case SQL_INTERVAL_MINUTE:
        case SQL_INTERVAL_SECOND:
        case SQL_INTERVAL_DAY_TO_HOUR:
        case SQL_INTERVAL_DAY_TO_MINUTE:
        case SQL_INTERVAL_DAY_TO_SECOND:
        case SQL_INTERVAL_HOUR_TO_MINUTE:
        case SQL_INTERVAL_HOUR_TO_SECOND:
        case SQL_INTERVAL_MINUTE_TO_SECOND:
		    interval->interval_type = SQL_IS_DAY_TO_SECOND;
            if (hasYear || hasMonth)
            {
                rc = CVT_OUT_OF_RANGE;
                goto funcReturn;
            }

            isYearMonth = FALSE;
            break;
        
        default:
            rc = CVT_OUT_OF_RANGE;
            goto funcReturn;
            break;
    }
    
    if (hasDay)
    {
        /*
        **  Extract day.
        */
        for (i = 0; i < precision && !CMwhite(sz); i++)
        {
            if (!CMdigit (sz))
            {
                rc = CVT_ERROR;
                goto funcReturn;
            }
            CMnext(sz);
        }
        CMcpychar("\0",sz); 
        CVal(szSave, &ds->day);
       /* 
        ** Truncate if interval contains an hour, minute, or, second,
        ** but is SQL_DAY.
        */
        if (fSqlType == SQL_INTERVAL_DAY)
            goto funcReturn;
        CMnext(sz);
        while (*sz && !CMdigit(sz)) CMnext(sz);
        precision = 2;
    } /* if (ds->day >=0) */
    if (hasHour)
    {
        szSave = sz;
        /*
        **  Extract hour.
        */
        if (*sz == '+' || *sz == '-')
        {
            if (!interval->interval_sign)
                interval->interval_sign = ( *sz == '-' ? SQL_TRUE : SQL_FALSE);
            CMnext(sz);
            szSave = sz;
        }
        
        for (i = 0; i < precision && !CMwhite(sz); i++)
        {
            if (!CMdigit(sz))
            {
                rc = CVT_ERROR;
                goto funcReturn;
            }
            sz++;
        }
        CMcpychar("\0",sz); 
        CVal(szSave, &ds->hour);
        switch (fSqlType)
        {
        case SQL_INTERVAL_HOUR:
            ds->hour += ds->day * 24;
            ds->day = 0;
            break;
            
        case SQL_INTERVAL_DAY_TO_HOUR:
            break;
        }
        CMnext(sz);
        while (*sz && !CMdigit(sz)) CMnext(sz);
        precision = 2;
    } /* if (hasHour) */
    if (hasMinute)
    {
        szSave = sz;
        /*
        **  Extract minute.
        */
        if (*sz == '+' || *sz == '-')
        {
            if (!interval->interval_sign)
                interval->interval_sign = ( *sz == '-' ? SQL_TRUE : SQL_FALSE);
            CMnext(sz);
            szSave = sz;
        }
        
        for (i = 0; i < precision && !CMwhite(sz); i++)
        {
            if (!CMdigit(sz))
            {
                rc = CVT_ERROR;
                goto funcReturn;
            }
            sz++;
        }
        CMcpychar("\0",sz); 
        CVal(szSave, &ds->minute);
        switch (fSqlType)
        {
        case SQL_INTERVAL_MINUTE:
            ds->hour += ds->day * 24;
            ds->minute += ds->hour * 60;
            ds->day = 0;
            ds->hour = 0;
            goto funcReturn;
            break;
        case SQL_INTERVAL_HOUR_TO_MINUTE:
            ds->hour += ds->day * 24;
            ds->day = 0;
            goto funcReturn;
            break;

        case SQL_INTERVAL_DAY_TO_MINUTE:
            goto funcReturn;
            break;
        }
        CMnext(sz);
        while (*sz && !CMdigit(sz)) CMnext(sz);
        precision = 2;
    } /* if (hasMinute) */
    if (hasSecond)
    {
        szSave = sz;
        /*
        **  Extract second.
        */
        if (*sz == '+' || *sz == '-')
        {
            if (!interval->interval_sign)
                interval->interval_sign = ( *sz == '-' ? SQL_TRUE : SQL_FALSE);
            CMnext(sz);
            szSave = sz;
        }
        
        for (i = 0; i < precision && !CMwhite(sz); i++)
        {
            if (!CMdigit(sz))
            {
                rc = CVT_ERROR;
                goto funcReturn;;
            }
            sz++;
        }
        CMcpychar("\0",sz); 
        CVal(szSave, &ds->second);
        switch (fSqlType)
        {
        case SQL_INTERVAL_SECOND:
            ds->hour += ds->day * 24;
            ds->minute += ds->hour * 60;
            ds->second += ds->minute * 60;
            ds->day = 0;
            ds->hour = 0;
            ds->minute = 0;
            goto funcReturn;
            break;
         
        case SQL_INTERVAL_MINUTE_TO_SECOND:
            ds->hour += ds->day * 24;
            ds->minute += ds->hour * 60;
            ds->day = 0;
            ds->hour = 0;
            goto funcReturn;
            break;
        
        case SQL_INTERVAL_HOUR_TO_SECOND:
            ds->hour += ds->day * 24;
            ds->day = 0;
            goto funcReturn;
            break;
        
        case SQL_INTERVAL_DAY_TO_SECOND:
            goto funcReturn;
            break;
        }
    } /* if (hasSecond) */
    if (!(hasDay && hasHour && hasMinute && hasSecond))
    {
        p = STindex(sz, " ", 0);
        if (p)
            CMcpychar("\0",p);
        if (CVal(sz, (i4 *)&intValue) != OK)
        {
            rc = CVT_INVALID;
            goto funcReturn;
        }
        else
            ds->day = intValue;
        if (p)
        {
            CMnext(p);
            p1 = STindex(p, ":", 0);
            if (p1)
                CMcpychar("\0",p1);
            if (CVal(p, (i4 *)&intValue) != OK)
            {
                rc = CVT_INVALID;
                goto funcReturn;
            }
            else
                ds->hour = intValue;
            CMnext(p1);
            p = p1;
            p1 = STindex(p, ":", 0);
            if (p1)
                CMcpychar("\0",p1);
            if (CVal(p, (i4 *)&intValue) != OK)
            {
                rc = CVT_INVALID;
                goto funcReturn;
            }
            else
                ds->minute = intValue;
            CMnext(p1);
			p = p1;
			p1 = STindex(p, ".", 0);
            if (p1)
			{
				CMcpychar("\0", p1);
                if (CVal(p, (i4 *)&intValue) != OK)
                {
                    rc = CVT_INVALID;
                    goto funcReturn;
                }
				else
					ds->second = intValue;
				CMnext(p1);
				if (CVal(p1, (i4 *)&intValue) != OK)
				{
					rc = CVT_INVALID;
					goto funcReturn;
				}
				else
					ds->fraction = intValue;
			} /* if (p1) */
			else if (CVal(p, (i4 *)&intValue) != OK)
			{
				rc = CVT_INVALID;
				goto funcReturn;
			}
			else
				ds->second = intValue;
        } /* if (p) */
    } /* if (!(hasDay && hasHour && hasMinute && hasSecond)) */
 
funcReturn:
    return (rc);    
}

/*
**  CvtCharTime
**
**  Convert character data or an SQL time to an ODBC time value.
**
**  On entry: pValue -->ODBC time structure to receive value.
**            rbgData-->character data.
**            cbData  = length of character data.
**
**  On exit:  Value returned if a valid time.
**
**  Returns:  CVT_SUCCESS
**            CVT_ERROR
*/

UDWORD CvtCharTime(
    ISO_TIME_STRUCT     * pValue,
    CHAR            * rgbData,
    UWORD             cbData,
    UDWORD           apiLevel)
{
    UDWORD      rc;
    ISO_TIME_STRUCT time;
    CHAR        sz[100];
    UWORD       i;
	i4          worki4;

    /*
    **  Extract assumed time from SQL character data:
    */
    if (apiLevel < IIAPI_LEVEL_3)
	{
        if (CvtGetToken (sz, 8, rgbData, 8) != CVT_SUCCESS)
        return (CVT_ERROR);
	}
    MEcopy(rgbData, STlength(rgbData), sz);
    sz[cbData] = '\0';

    /*
    **  Extract hour.
    */
    for (i = 0; i < 2; i++)
        if (!CMdigit (&sz[i]))
            return (CVT_ERROR);

    CMcpychar("\0",&sz[2]); 
    CVal(&sz[0], &worki4);
    time.hour = (SQLUSMALLINT)worki4;

    /*
    **  Extract minute.
    */
    for (i = 3; i < 5; i++)
        if (!CMdigit (&sz[i]))
            return (CVT_ERROR);

    CMcpychar("\0",&sz[5]); 
    CVal(&sz[3], &worki4);
    time.minute = (SQLUSMALLINT)worki4;

    /*
    **  Extract second.
    */
    for (i = 6; i < 8; i++)
        if (!CMdigit (&sz[i]))
            return (CVT_ERROR);

    CMcpychar("\0",&sz[8]); 
    CVal(&sz[6], &worki4);
    time.second = (SQLUSMALLINT)worki4;

    /*
    ** Extract fraction, if present.
    */
    if (apiLevel > IIAPI_LEVEL_3 && CMdigit(&sz[9]))
    {
        for (i = 9; i < cbData; i++)
        {
            if (!CMdigit(&sz[i]))
                break;
        }
        CMcpychar("\0",&sz[i]); 
        CVal(&sz[9], (i4 *)&time.fraction);
    }
    else
        time.fraction = 0;

    rc = CvtEditTime(&time, apiLevel);

    if (rc == CVT_SUCCESS)
    {
        pValue->hour   = time.hour;
        pValue->minute = time.minute;
        pValue->second = time.second;
		pValue->fraction = time.fraction;
    }

    return (rc);
}


/*
**  CvtCharTimestamp
**
**  Convert character data or an SQL timestamp to an ODBC timestamp value.
**
**  On entry: pValue -->ODBC timestamp structure to receive value.
**            rbgData-->character data.
**            cbData  = length of character data.
**
**  On exit:  value returned if a valid timestamp.
**
**  Returns:  CVT_SUCCESS
**            CVT_ERROR
*/

UDWORD CvtCharTimestamp(
    TIMESTAMP_STRUCT     * pValue,
    CHAR            * rgbData,
    UWORD             cbData,
    UDWORD            apiLevel)
{
    UDWORD      rc;
    DATE_STRUCT date;
    ISO_TIME_STRUCT time;
    CHAR        szFrac[11];
    CHAR        sz[10];
    CHAR      * c;
    UWORD       i;
    UDWORD      f;

    c = rgbData;
    i = cbData;
    f = 0;

	/*
	** An empty string counts as blank string in this case.
	*/
    if (*c == '\0')
        i = 0;

    /*
    **  Skip leading white space, if any:
    */
    c = CvtSkipSpace (c, i, &i);

    /*
    ** Blank strings are OK for ingresdate.
    */
    if ((apiLevel < IIAPI_LEVEL_4) && (i == 0))
	{
        pValue->year     = 9999;
        pValue->month    = 12;
        pValue->day      = 31;
        pValue->hour     = 0;
        pValue->minute   = 0;
        pValue->second   = 0;
        pValue->fraction = 0;
        return (CVT_SUCCESS);
	}
    if (i < 19)
        return (CVT_ERROR);

    /*
    **  Extract date:
    */
    rc = CvtCharDate (&date, c, 10);
    if (rc != CVT_SUCCESS)
        return (rc);
    c += 11;
    i -= 11;

    /*
    **  Extract time:
    */
    rc = CvtCharTime (&time, c, cbData, apiLevel);
    if (rc != CVT_SUCCESS)
        return (rc);
    c += 8;
    i -= 8;

    /*
    **  Extract fraction of second, if any:
    */
    if (i > 1)
        if (!CMcmpcase(c,"."))
        {
            CMnext(c);
            i--;
            rc = CvtCharNumStr (szFrac, sizeof(szFrac), c, i, &c, &i);
            if (rc == CVT_OUT_OF_RANGE || rc == CVT_ERROR)
                return (rc);
            if (c)
            {
                if (apiLevel < IIAPI_LEVEL_4)
                    return (CVT_ERROR);
            else if (CMcmpcase(c,"-") && CMcmpcase(c,"+"))
                    return (CVT_ERROR);
            }
             
            STcopy ("000000000", sz);
            MEcopy (szFrac, STlength (szFrac), sz);
            f  = strtoul (sz, NULL, 10);
        }

    /*
    **  Finally set values in caller's TIMESTAMP structure:
    */
    pValue->year     = date.year;
    pValue->month    = date.month;
    pValue->day      = date.day;
    pValue->hour     = time.hour;
    pValue->minute   = time.minute;
    pValue->second   = time.second;
    pValue->fraction = f;

    return (CVT_SUCCESS);

}

/*
**  CvtCurrStampDate
**
**  Insert current local date into the date portions of a C timestamp value.
**
**  On entry: pValue -->ODBC timestamp structure to receive value.
**
**  Returns:  nothing
*/

void CvtCurrStampDate(
    TIMESTAMP_STRUCT     * pstamp,
	UDWORD       apiLevel)
{
    SYSTIME stime;
    struct TMhuman tm;
    i4      year =0;
    i4      month=0;
    i4      day  =0;

    MEfill(sizeof(struct TMhuman), 0, &tm);
    TMnow(&stime);         /* stime = seconds since 00:00:00 UCT 1970-01-01 */
    TMbreak(&stime, &tm);  /* break convert SYSTIME to TMhuman mm, dd, yy struct*/

    CVan(tm.year, &year);
    CVan(tm.day,  &day);
    TM_lookup_month(tm.month, &month);

    pstamp->year  = (SQLSMALLINT) year;
    pstamp->month = (SQLUSMALLINT)month;
    pstamp->day   = (SQLUSMALLINT)day;
}


/*
**  CvtDateChar
**
**  Convert an ODBC date value to SQL date (character format).
**
**  On entry: pdate  -->ODBC date structure.
**            rbgData-->where to return SQL date.
**            cbData  = length of output buffer.
**
**  On exit:  Value is assumed tp be a valid date.
**
**  Returns:  CVT_SUCCESS
*/

UDWORD CvtDateChar(
    DATE_STRUCT     * pdate,
    CHAR      * rgbData,
    UWORD       cbData,
	UDWORD      apiLevel)
{
    CHAR  sz[6];
    CHAR  *spacer;

/*    if (cbData < 10)
        return (CVT_OUT_OF_RANGE);
*/
    if (apiLevel < IIAPI_LEVEL_4)
        spacer = "-";
	else
        spacer = "-";


    /*
    ** 9999-12-31 date value in ODBC came from select 
    ** from a blank date. We should treat '9999-12-31'
    ** value as a special case value and blank out this 
    ** value if it's used as a parameter value when used
    ** in a SQL statement
    ** Applies to pre-ISO connection levels, as these rules
    ** to not apply to ANSI dates.
    */
    
    if ( (pdate->year == 9999 && pdate->month == 12 && pdate->day == 31) 
        && (apiLevel < IIAPI_LEVEL_4) )
    {
        *(rgbData + 0) = '\0';
        return (CVT_SUCCESS);
    }
    STprintf (sz, "%04.4d", (i4)pdate->year);
    MEcopy (sz, 4, rgbData);

    CMcpychar(spacer,(rgbData + 4)); 
    STprintf (sz, "%02.2d", (i4)pdate->month);
    MEcopy (sz, 2, rgbData + 5);
    CMcpychar(spacer,(rgbData + 7)); 
    STprintf (sz, "%02.2d", (i4)pdate->day);
    MEcopy (sz, 2, rgbData + 8);

    return (CVT_SUCCESS);
}


/*
**  CvtEditDate
**
**  Check for valid date.
**
**  On entry: pdate-->ODBC date structure.
**
**  Returns:  CVT_SUCCESS
**            CVT_DATE_OVERFLOW
*/

UDWORD CvtEditDate(
    DATE_STRUCT     * pdate)
{
    if (pdate->year < 1 || pdate->year > 9999)
        return (CVT_DATE_OVERFLOW);

    if (pdate->month < 1 || pdate->month > 12)
        return (CVT_DATE_OVERFLOW);

    if (pdate->day < 1)
        return (CVT_DATE_OVERFLOW);

    switch (pdate->month)
    {
    case 2:

        if ((pdate->day > 29) ||
            ((pdate->day > 28) &&
             ((pdate->year % 4 != 0) ||
              ((pdate->year % 100 == 0) && (pdate->year % 400 != 0)))))
            return (CVT_DATE_OVERFLOW);
        break;

    case 4:
    case 6:
    case 9:
    case 11:

        if (pdate->day > 30)
            return (CVT_DATE_OVERFLOW);
        break;

    default:

        if (pdate->day > 31)
            return (CVT_DATE_OVERFLOW);
        break;
    }

    return (CVT_SUCCESS);
}

/*
**  CvtEditTime
**
**  Check for valid time.
**
**  On entry: ptime-->ODBC time structure.
**
**  Returns:  CVT_SUCCESS
**            CVT_DATE_OVERFLOW
*/

UDWORD CvtEditTime(
    ISO_TIME_STRUCT     * ptime,
	UDWORD apiLevel)
{
    if (ptime->hour > 23)
        return (CVT_DATE_OVERFLOW);

    if (ptime->minute > 59)
        return (CVT_DATE_OVERFLOW);

    if (ptime->second > 59)
        return (CVT_DATE_OVERFLOW);

    if (apiLevel > IIAPI_LEVEL_3)
		if (ptime->fraction > 999999999)
			return CVT_DATE_OVERFLOW;

    return (CVT_SUCCESS);
}


/*
**  CvtEditTimestamp
**
**  Check for valid timestamp.
**
**  On entry: pstamp-->ODBC timestamp structure.
**
**  Returns:  CVT_SUCCESS
**            CVT_DATE_OVERFLOW
*/

UDWORD CvtEditTimestamp(
   TIMESTAMP_STRUCT     * pstamp,
   UDWORD apiLevel)
{
    UDWORD      rc;
    DATE_STRUCT dateValue;
    ISO_TIME_STRUCT timeValue;

    dateValue.year   = pstamp->year;
    dateValue.month  = pstamp->month;
    dateValue.day    = pstamp->day;
    
    rc = CvtEditDate (&dateValue);
    if (rc != CVT_SUCCESS)
        return (rc);
    
    timeValue.hour   = pstamp->hour;
    timeValue.minute = pstamp->minute;
    timeValue.second = pstamp->second;
    if (apiLevel > IIAPI_LEVEL_3)
        timeValue.fraction = pstamp->fraction;
	else
        timeValue.fraction = 0;
    rc = CvtEditTime (&timeValue, apiLevel);
    if (rc != CVT_SUCCESS)
        return (rc);

    return (CVT_SUCCESS);
}


/*
**  CvtFloatBig
**
**  Convert an float value to SQL BIGINT (LONGLONG) data.
**
**  On entry: rbgData    -->where to rturn NUMERIC data.
**            floatValue  = integer value.
**
**  On exit:  Value is returned if not truncated or out of range.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_TRUNCATION   if non-significant digits truncated.
**            CVT_OUT_OF_RANGE if significant digits truncated.
**
*/

#ifdef NT_GENERIC

UDWORD CvtFloatBig(
    CHAR      * rgbData,
    long double floatValue)
{
    long double f, h, l;
    UDWORD      hi, lo;

    /*
    **  Ensure we can represent value as a 64 bit signed integer:
    */
    if (modfl (floatValue, &f) != 0L)
        return (CVT_TRUNCATION);
    if (f < -9223372036854775808.0L || f >  9223372036854775807.0L)
        return (CVT_OUT_OF_RANGE);

    /*
    **  Extract high and low order parts of 64 bit integer:
    */
    f  = fabsl (f);
    l  = fmodl (f, 4294967296.0L);
    h  = (f - l) / 4294967296.0L;

    lo = (UDWORD)l;
    hi = (UDWORD)h;

    /*
    **  Two's complement result if negative:
    */
    if (floatValue < 0.0L)
    {
        lo = ~lo;
        hi = ~hi;

        if (lo < 0xFFFFFFFF)
        {
            lo++;
        }
        else
        {
            lo = 0;
            hi++;
        }
    }

    *(UDWORD *)rgbData       = lo;
    *((UDWORD *)rgbData + 1) = hi;

    return (CVT_SUCCESS);
}
#endif


/*
**  CvtFloatNum
**
**  Convert a float value to SQL NUMERIC data.
**
**  On entry: rbgData    -->where to return NUMERIC data.
**            floatValue  = integer value.
**            cbPrecision = SQL precision.
**            cbScale     = SQL scale.
**
**  On exit:  Value is returned if not truncated or out of range.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_TRUNCATION   if fraction digits truncated.
**            CVT_OUT_OF_RANGE if significant digits truncated.
*/

UDWORD CvtFloatNum(
    CHAR      * rgbData,
    SDOUBLE     floatValue,
    SWORD       cbPrecision,
    SWORD       cbScale,
    BOOL        fUnsigned)
{
    char        buf[256];
    int         sign;
    i4          digits, decpt;

    if (fUnsigned && floatValue < 0)
        return (CVT_OUT_OF_RANGE);

/*  sz = _fcvt (floatValue, cbScale, &decpt, &sign); */
    CVfcvt(floatValue, buf, &digits, &decpt, cbScale);
    sign = (*buf == '-') ? 1 : 0;

    return (CvtNumNum (rgbData, cbPrecision, cbScale, buf+1, decpt, sign));
}


/*
**  CvtGetCType
**
**  Find entry in data type attribute table:
**
**  On entry: fType = ODBC C type code.
**
**  Returns:  -->SQLTYPEATTR if found and supported type, else NULL
*/

LPSQLTYPEATTR CvtGetCType(
    SWORD fType, 
    UDWORD fAPILevel, 
    UDWORD dateConnLevel)
{
    UWORD      i;
    SWORD  fCType = fType;

    if (fAPILevel < IIAPI_LEVEL_6 && fType == SQL_BIT)
        fCType = SQL_TINYINT;


    for (i = 0; i < TYPE_NUM; i++)
        if (fCType == TYPE_TABLE[i].fCType)
        {
            if (TYPE_TABLE[i].fVerboseType == SQL_INTERVAL ||
                TYPE_TABLE[i].fVerboseType == SQL_DATETIME)
            {
                if ( dateConnLevel < IIAPI_LEVEL_4 &&
                    (TYPE_TABLE[i].fCType == SQL_C_TYPE_DATE ||
                    TYPE_TABLE[i].fCType == SQL_C_TYPE_TIMESTAMP))
                    /*
                    **  If support for Ingres dates is indicated,
                    **  return pre-ISO overloaded Ingres date information,
                    **  if the destination type is a date or timestamp.
                    */
                    return (CvtGetIngresDateCType(fType));
                else if (fAPILevel < IIAPI_LEVEL_4)
                {
                    /*
                    **  If the server is "pre-ISO dates" GCA protocol level, 
                    **  return pre-ISO overloaded Ingres date information.
                    */
                    return (CvtGetIngresDateCType(fType));
                }
            }
            return ((LPSQLTYPEATTR)&TYPE_TABLE[i]);
        }
        
    return (NULL);
}

/*
**  CvtGetSqlType
**
**  Find entry in data type attribute table:
**
**  On entry: fType = ODBC SQL type code.
**
**  Returns:  -->SQLTYPEATTR if found, else NULL
*/

LPSQLTYPEATTR CvtGetSqlType(
    SWORD fType,  UDWORD fAPILevel, UDWORD dateConnLevel)
{
    UWORD      i;
    SWORD  fSqlType = fType;

    if (fAPILevel < IIAPI_LEVEL_6 && fType == SQL_BIT)
        fSqlType = SQL_TINYINT;
    
    for (i = 0; i < TYPE_NUM; i++)
        if (fSqlType == TYPE_TABLE[i].fSqlType)
        {
           if (TYPE_TABLE[i].fVerboseType == SQL_INTERVAL ||
               TYPE_TABLE[i].fVerboseType == SQL_DATETIME)
           {
                if (dateConnLevel < IIAPI_LEVEL_4 &&
                     (TYPE_TABLE[i].fSqlType == SQL_TYPE_DATE ||
                     TYPE_TABLE[i].fSqlType == SQL_TYPE_TIMESTAMP))
                {
                    /*
                    **  If support for Ingres dates is indicated,
                    **  return pre-ISO overloaded Ingres date information,
                    **  if the destination type is a date or timestamp.
                    */
                    return (CvtGetIngresDateSqlType(fType));
                }
                else if (fAPILevel < IIAPI_LEVEL_4 )
                {
                    /*
                    **  If the server is "pre-ISO dates" GCA protocol level, 
                    **  return pre-ISO overloaded Ingres date information.
                    */
                    return (CvtGetIngresDateSqlType(fType));
                }
            }
            return ((LPSQLTYPEATTR)&TYPE_TABLE[i]);
        }
        
    return (NULL);
}

/*
**  CvtGetIngresDateCType
**
**  Find entry in Ingres date data type attribute table.
**
**  On entry: fType = ODBC C type code.
**
**  Returns:  -->SQLTYPEATTR if found and supported type, else NULL
*/

LPSQLTYPEATTR CvtGetIngresDateCType(
    SWORD fType)
{
    UWORD      i;

    for (i = 0; i < INGRES_DATE_TYPE_NUM; i++)
        if (fType == INGRES_DATE_TYPE_TABLE[i].fCType)
           return ((LPSQLTYPEATTR)&INGRES_DATE_TYPE_TABLE[i]);

    return (NULL);
}

/*
**  CvtGetIngresDateSqlType
**
**  Find entry in Ingres date data type attribute table.
**
**  On entry: fType = ODBC Sql type code.
**
**  Returns:  -->SQLTYPEATTR if found and supported type, else NULL
*/

LPSQLTYPEATTR CvtGetIngresDateSqlType(
    SWORD fType)
{
    UWORD      i;

    for (i = 0; i < INGRES_DATE_TYPE_NUM; i++)
        if (fType == INGRES_DATE_TYPE_TABLE[i].fSqlType)
                return ((LPSQLTYPEATTR)&INGRES_DATE_TYPE_TABLE[i]);

    return (NULL);
}

/*
**  CvtGetToken
**
**  Extract a token from an SQL character value.
**
**  On entry: rbgData-->SQL character data.
**            cbData  = length SQL character data.
**            pToken -->where to return token.
**            cbToken = length of token to extract.
**
**  On exit:  Token returned if success or truncation.
**
**  Returns:  CVT_SUCCESS         Token is alone in data
**            CVT_TRUNCATION      Token has company.
**            CVT_ERROR           No token found.
*/

UDWORD CvtGetToken(
    CHAR      * pToken,
    UWORD       cbToken,
    CHAR      * rgbData,
    UWORD       cbData)
{
    CHAR      * c;
    UWORD       i;

    c = rgbData;
    i = cbData;

    /*
    **  Skip leading white space, if any:
    */
    c = CvtSkipSpace (c, i, &i);
    if (i < cbToken)
        return (CVT_ERROR);

    /*
    **  Extract token itself:
    */
    MEcopy (c, cbToken, pToken);

    /*
    **  Make sure any trailing characters are white space:
    */

    c = CvtSkipSpace (c + cbToken, (UWORD)(i - cbToken), &i);
    if (i != 0)
        return (CVT_TRUNCATION);

    return (CVT_SUCCESS);
}


/*
**  CvtIntNum
**
**  Convert an integer value to SQL NUMERIC data.
**
**  On entry: rbgData    -->where to rturn NUMERIC data.
**            intValue    = integer value.
**            cbPrecision = SQL precision.
**            cbScale     = SQL scale.
**            fUnsigned   = Convert to unsigned integer.
**
**  On exit:  Value is returned if not truncated or out of range.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_OUT_OF_RANGE if significant digits truncated.
*/

UDWORD CvtIntNum(
    CHAR      * rgbData,
    SDWORD      intValue,
    SWORD       cbPrecision,
    SWORD       cbScale,
    BOOL        fUnsigned)
{
    CHAR        sz[12];
    int         dec, sign;

    if (fUnsigned)
    {
        CvtUltoa((UDWORD)intValue, sz);
        sign = 0;
    }
    else
    {
        CVla((i4)(intValue < 0 ? -intValue : intValue), sz);
        sign = (intValue < 0) ? 1 : 0;
    }
    dec  = (int)STlength (sz);

    return (CvtNumNum (rgbData, cbPrecision, cbScale, sz, dec, sign));
}

/*
**  CvtInt64Num
**
**  Convert a 64-bit integer value to SQL NUMERIC data.
**
**  On entry: rbgData    -->where to rturn NUMERIC data.
**            intValue    = integer value.
**            cbPrecision = SQL precision.
**            cbScale     = SQL scale.
**            fUnsigned   = Convert to unsigned integer.
**
**  On exit:  Value is returned if not truncated or out of range.
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_OUT_OF_RANGE if significant digits truncated.
*/

UDWORD CvtInt64Num(
    CHAR      * rgbData,
    ODBCINT64   intValue,
    SWORD       cbPrecision,
    SWORD       cbScale,
    BOOL        fUnsigned)
{
    CHAR        sz[24];
    i4         dec, sign;

    CVla8((i8)(intValue < 0 ? -intValue : intValue), sz);
        sign = (intValue < 0) ? 1 : 0;

    dec  = (i4)STlength (sz);

    return (CvtNumNum (rgbData, cbPrecision, cbScale, sz, dec, sign));
}


/*
**  CvtNumChar
**
**  Convert SQL NUMERIC data to a character string.
**
**  On entry: szValue    -->where to return character string.
**            rbgValue   -->SQL numeric data.
**            cbPrecision = SQL precision.
**            cbScale     = SQL scale.
**
**  Returns:  CVT_SUCCESS  Converted successfully.
**            CVT_ERROR    Data not numeric.
**
**  Note: szValue is assumed to be large enough to hold any numeric value, 34
**        bytes should do it.  Something is always returned, even if not numeric.
*/

UDWORD CvtNumChar(
    CHAR      * szValue,
    CHAR      * rgbData,
    SWORD       cbPrecision,
    SWORD       cbScale)
{
    UDWORD      rc = CVT_SUCCESS;
    UWORD       cb = (UWORD)(cbPrecision - cbScale);
    CHAR      * sz = szValue;
    CHAR      * pz = rgbData + cbPrecision - 1;
    CHAR        z;
    CHAR        n;
    CHAR      * p;

    /*
    **  Check all but last digit for valid numeric data:
    for (p = rgbData; (p < pz) && (rc == CVT_SUCCESS); CMnext(p) )
        if (!CMdigit (p)) rc = CVT_ERROR;
    */
    for (p = rgbData; (p < pz); CMnext(p))
    {
        if (!CMdigit (p)) 
        {
             rc = CVT_ERROR;
             break;
        }
    }
    /*
    **  Convert valid S370 (or Realia or MicroFocus) zone to sign:
    */
    switch (*pz)
    {
    case '}':
        *sz = '-';
        CMnext(sz);

    case '{':

       n = 0;
       break;

    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':

       n = (CHAR)(*pz - 'A' + 1);
       break;

    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':

       *sz = '-';
        CMnext(sz);
       n = (CHAR)(*pz - 'J' + 1);
       break;

    default:

        z = (CHAR)(*pz & 0xF0);
        n = (CHAR)(*pz & 0x0F);

        if (n > 0x0A || (z != 0x20 && z != 0x30 && z != 0x70))
            rc = CVT_ERROR;

        if (z == 0x20 || z == 0x70)
        {
            *sz = '-';
            CMnext(sz);
        }
    }

    /*
    **  Copy significant digits:
    */
    MEcopy (rgbData, cb, sz);
    sz += cb;

    /*
    **  Copy fraction:
    */
    if (cbScale > 0)
    {
        CMcpychar(".",sz); 
        CMnext (sz);
        MEcopy (rgbData + cb, cbScale, sz);
        sz += cbScale;
    }

    /*
    **  Set numeric in last digit unless something wrong:
    */
    if (rc == CVT_SUCCESS)
        *(sz - 1) = (CHAR)(0x30 | n);

    /*
    **  Null terminate and return results:
    */
    CMcpychar("\0",sz); 
    return rc;
}


/*
**  CvtNumNum
**
**  Convert a numeric string to SQL NUMERIC data:
**
**  On entry: rgbData    -->numeric value buffer.
**            cbPrecision = SQL precision.
**            cbScale     = SQL scale.
**            szValue    -->numeric string.
**            dec         = position of decimal (as for _ecvt, _fcvt).
**            sign        = sign (0 = +, 1 = -) (as for _ecvt, _fcvt).
**
**  Returns:  CVT_SUCCESS      if converted successfully.
**            CVT_TRUNCATION   if fraction digits truncated.
**            CVT_OUT_OF_RANGE if significant digits truncated.
*/

UDWORD CvtNumNum(
    CHAR      * rgbData,
    SWORD       cbPrecision,
    SWORD       cbScale,
    CHAR      * szValue,
    int         dec,
    int         sign)
{
    int         cb;
    i4          len;
    CHAR      * p;

    /*
    **  Make sure value will fit:
    */
    len = STlength (szValue);
    cb  = cbPrecision - cbScale;

    if (dec > cb)
        return (CVT_OUT_OF_RANGE);
    if (len - dec > cbScale)
        return (CVT_TRUNCATION);

    /*
    **  Stuff value into buffer:
    */
    MEfill (cbPrecision, '0', rgbData);
    MEcopy (szValue, len, rgbData + cb - dec);

    /*
    **  Stuff zoned decimal sign using REALIA COBOL rules:
    */
    p = rgbData + cbPrecision - 1;

    if (sign)
        *p = (CHAR)((*p & 0x0F) | 0x20);
    else
        *p = (CHAR)((*p & 0x0F) | 0x30);

    return (CVT_SUCCESS);
}


/*
**  CvtPack
**
**  Convert an SQL NUMERIC (zoned decimal) to SQL DECIMAL (packed decimal).
**
**  On entry: rgbDecimal-->where to return decimal data.
**            cbDecimal  = length of decimal buffer.
**            rbgNumeric-->nuerica data.
**            cbNumeric  = length of numeric buffer.
**
**  Returns:  Nothing.
*/

VOID CvtPack(
    CHAR      * rgbDecimal,
    UWORD       cbDecimal,
    CHAR      * rgbNumeric,
    UWORD       cbNumeric)
{
    CHAR      * pd;
    CHAR      * pn;
    int         i;
    CHAR        z;

    MEfill (cbDecimal, 0, rgbDecimal);

    pd = rgbDecimal + cbDecimal - 1;
    pn = rgbNumeric + cbNumeric - 1;
    z  = (CHAR)(*pn & 0xF0);

    if ((z == 0x20 || z == 0x70) &&
        (*pn & 0x0F) < 0x0A)
    {
        *pd  = (CHAR)((*pn & 0x0F) << 4);
        *pd |= 0x0D;
    }
    else
    if ((z == 0x30) &&
        (*pn & 0x0F) < 0x0A)
    {
        *pd  = (CHAR)((*pn - '0') << 4);
        *pd |= 0x0F;
    }
    else
    {
       switch (*pn)
       {
       case '{':

           *pd = 0x0C;
           break;

       case 'A':
       case 'B':
       case 'C':
       case 'D':
       case 'E':
       case 'F':
       case 'G':
       case 'H':
       case 'I':

           *pd  = (CHAR)((*pn - 'A' + 1) << 4);
           *pd |= 0x0C;
           break;

       case '}':

           *pd = 0x0D;
           break;

       case 'J':
       case 'K':
       case 'L':
       case 'M':
       case 'N':
       case 'O':
       case 'P':
       case 'Q':
       case 'R':

           *pd  = (CHAR)((*pn - 'J' + 1) << 4);
           *pd |= 0x0D;
           break;
       }
    }

    pn--;
    pd--;
    for (i = cbNumeric - 1; i > 1; i -= 2)
    {
        *pd  =  *pn-- - '0';
        *pd |= (*pn-- - '0') << 4;
        pd--;
    }
    if (i)
        *pd  =  (CHAR)(*pn - '0');

    return;
}


/*
**  CvtSkipSpace
**
**  Skip white space in character string:
**
**  On entry: rgbData-->character data.
**            cbData  = length of character data.
**            pcbData-->where to return length remaining.
**
**  Returns:  -->remaining string, or NULL.
*/

CHAR      * CvtSkipSpace(
    CHAR      * rgbData,
    UWORD       cbData,
    UWORD     * pcbData)
{
    UWORD       i;
    CHAR      * c;

    i = cbData;
    c = rgbData;

    /*
    **  Skip white space, if any:
    */
    while ((i > 0) && (CMwhite(c)))
    {
        CMnext(c);
        i--;
    }

    *pcbData = i;

    if (i == 0)
        c = NULL;

    return (c);
}


/*
**  CvtIntervalChar
**
**  Convert an ODBC interval string to SQL interval (character format).
**
**  On entry: ptime  -->ODBC time structure
**            rbgData-->where to return SQL time.
**            cbData -->length of output buffer.
**            sqlType-->ODBC data type. 
**
**  On exit:  Value is assumed to be a valid interval.
**
**  Returns:  CVT_SUCCESS
**            CVT_OUT_OF_RANGE
*/

UDWORD CvtIntervalChar(
    SQL_INTERVAL_STRUCT *interval,
    CHAR      * rgbData,
    SWORD        fSqlType)
{
    SQL_YEAR_MONTH_STRUCT *ym = &interval->intval.year_month;
    SQL_DAY_SECOND_STRUCT *ds = &interval->intval.day_second;
    SQLSMALLINT interval_sign = interval->interval_sign;
    i4 i;
    UDWORD rc = CVT_SUCCESS;

    CHAR  sz[50];
     CHAR *rgbPtr = rgbData;

    if (interval_sign == SQL_TRUE)
    {
        CMcpychar("-",rgbData);
        CMnext(rgbData);
    }
    else if (interval_sign != SQL_FALSE)
        return CVT_INVALID;

    switch (fSqlType)
    {
    case SQL_INTERVAL_YEAR:
    case SQL_INTERVAL_YEAR_TO_MONTH:
        STprintf (sz, "%d", ym->year);
        MEcopy(sz, STlength(sz), rgbData);
        for (i = 0; i < (i4)STlength(sz); i++) CMnext(rgbData);
     
        if (fSqlType == SQL_INTERVAL_YEAR)
        {
            CMcpychar("\0",rgbData);        
            break;
        }            
        CMcpychar("-",rgbData);
        CMnext(rgbData);   
        /* note fallthrough for SQL_INTERVAL_MONTH */

    case SQL_INTERVAL_MONTH:   
        if (fSqlType == SQL_INTERVAL_MONTH)
        {
            ym->month += (ym->year * 12);
            ym->year = 0;
            STprintf (sz, "0-%d", ym->month);
        }
        else
        {
            STprintf (sz, "%d", ym->month);
        }
        MEcopy (sz, STlength(sz), rgbData);
        CMcpychar("\0", &rgbData[STlength(rgbData)]);
        break;

    case SQL_INTERVAL_DAY:
    case SQL_INTERVAL_DAY_TO_HOUR:
    case SQL_INTERVAL_DAY_TO_MINUTE:
    case SQL_INTERVAL_DAY_TO_SECOND:
        STprintf (sz, "%d", ds->day);
        MEcopy(sz, STlength(sz), rgbData);
        for (i = 0; i < (i4)STlength(sz); i++) CMnext(rgbData);
     
        if (fSqlType == SQL_INTERVAL_DAY)
        {
            CMcpychar("\0",rgbData);        
            break;
        }            
        CMcpychar(" ",rgbData);
        CMnext(rgbData);   
        /* note fallthrough for SQL_INTERVAL_HOUR */
    case SQL_INTERVAL_HOUR:
    case SQL_INTERVAL_HOUR_TO_MINUTE:
    case SQL_INTERVAL_HOUR_TO_SECOND:
        if (fSqlType == SQL_INTERVAL_HOUR)
        {
            ds->hour += (ds->day * 24);
            ds->day = 0;
        }
        STprintf (sz, "%d", (i4)ds->hour);
        MEcopy (sz, STlength(sz), rgbData);
        for (i = 0; i < (i4)STlength(sz); i++) CMnext(rgbData);
        if (fSqlType == SQL_INTERVAL_DAY_TO_HOUR || fSqlType ==
            SQL_INTERVAL_HOUR)
        {
            CMcpychar("\0", rgbData);
            break;
        }
        CMcpychar(":", rgbData);
        CMnext(rgbData);
        /* note fallthrough for SQL_INTERVAL_MINUTE */
    case SQL_INTERVAL_MINUTE:
    case SQL_INTERVAL_MINUTE_TO_SECOND:
        if (fSqlType == SQL_INTERVAL_MINUTE)
        {
            ds->hour += ds->day * 24;
            ds->minute +=  ds->hour * 60;
            ds->day = 0;
            ds->hour = 0;
        }
        STprintf (sz, "%d", ds->minute);
        MEcopy (sz, STlength(sz), rgbData);
        for (i = 0; i < (i4)STlength(sz); i++) CMnext(rgbData);
        if (fSqlType == SQL_INTERVAL_DAY_TO_MINUTE || fSqlType ==
            SQL_INTERVAL_HOUR_TO_MINUTE || fSqlType == SQL_INTERVAL_MINUTE)
        {
            CMcpychar("\0", rgbData);
            break;
        }
        CMcpychar(":", rgbData);
        CMnext(rgbData);
    case SQL_INTERVAL_SECOND:
        if (fSqlType == SQL_INTERVAL_SECOND)
        {
            ds->hour += ds->day * 24;
            ds->minute +=  ds->hour * 60;
            ds->second += ds->minute * 60;
            ds->day = 0;
            ds->hour = 0;
            ds->minute = 0;
        }
        if (ds->fraction)
            STprintf(sz, "%d.%d", ds->second, ds->fraction);
        else
            STprintf (sz, "%d", ds->second);
        STlcopy (sz, rgbData, STlength(sz));
        break;
        
    default:
        rc = CVT_INVALID;
        break;

    } /* switch (interval->interval_type) */
    return (rc);
}

/*
**  CvtTimeChar
**
**  Convert an ODBC time value to SQL time (character format).
**
**  On entry: ptime  -->ODBC time structure
**            rbgData-->where to return SQL time.
**            cbData  = length of output buffer.
**
**  On exit:  Value is assumed tp be a valid time.
**
**  Returns:  CVT_SUCCESS
**            CVT_OUT_OF_RANGE
*/

UDWORD CvtTimeChar(
    ISO_TIME_STRUCT     * ptime,
    CHAR      * rgbData,
    UWORD       cbData,
    UDWORD     apiLevel)
{
    CHAR  sz[40];

   if (apiLevel < IIAPI_LEVEL_4)
	   if (cbData < 8)
           return (CVT_OUT_OF_RANGE);

    STprintf (sz, "%02.2d", (i4)ptime->hour);
    MEcopy (sz, 2, rgbData);
    CMcpychar(":",(rgbData + 2));                /* ODBC time separator */
    STprintf (sz, "%02.2d", (i4)ptime->minute);
    MEcopy (sz, 2, rgbData + 3);
    CMcpychar(":",(rgbData + 5));                /* ODBC time separator */
    STprintf (sz, "%02.2d", (i4)ptime->second);
    MEcopy (sz, 2, rgbData + 6);
    if (ptime->fraction)
	{
	    CMcpychar(".", rgbData + 8);             /* ODBC fraction deparator */
        STprintf (sz, "%d", ptime->fraction);
        MEcopy(sz, STlength(sz), rgbData + 9);
	}
    return (CVT_SUCCESS);
}


/*
**  CvtTimestampChar
**
**  Convert an ODBC timestamp value to SQL timestamp (character format),
**  using CA-DB/CA-IDMS default punctuation.
**
**  On entry: pstamp     -->ODBC timestamp structure
**            rbgData    -->where to return SQL timestamp.
**            cbData      = length of output buffer.
**            cbPrecision = SQL precision of timestamp fraction.
**
**  On exit:  Value is assumed to be a valid timestamp.
**
**  Returns:  CVT_SUCCESS       if it fits.
**            CVT_TRUNCATION    if fraction truncated.
**            CVT_OUT_OF_RANGE  if buffer too small.
*/

UDWORD CvtTimestampChar(
    TIMESTAMP_STRUCT     * pstamp,
    CHAR      * rgbData,
    UWORD       cbData,
    UWORD       cbPrecision,
	UDWORD apiLevel)
{
    DATE_STRUCT dateValue;
    ISO_TIME_STRUCT timeValue;

    if (pstamp->fraction && cbPrecision && cbData < (UWORD)(20 + cbPrecision))
        return (CVT_TRUNCATION);

    /*
    ** 9999-12-31 date value in ODBC came from select 
    ** from a blank date. We should treat '9999-12-31'
    ** value as a special case value and blank out this 
    ** value if it's used as a parameter value when used
    ** in a SQL statement
    ** Applies to pre-ISO connection levels, as these rules
    ** to not apply to ANSI dates.
    */
    
    if ( (pstamp->year == 9999 && pstamp->month == 12 && pstamp->day == 31) 
        && (apiLevel < IIAPI_LEVEL_4) )
    {
        *(rgbData + 0) = '\0';
    }
    else
    {	
        dateValue.year   = pstamp->year;
        dateValue.month  = pstamp->month;
        dateValue.day    = pstamp->day;
    
        CvtDateChar (&dateValue, rgbData, 10, apiLevel);
    
        CMcpychar(" ",(rgbData + 10));      /* Ingres Date time separator */
    
        timeValue.hour   = pstamp->hour;
        timeValue.minute = pstamp->minute;
        timeValue.second = pstamp->second;
        if (apiLevel > IIAPI_LEVEL_3)
		    timeValue.fraction = pstamp->fraction;
		else
            timeValue.fraction = 0;

        CvtTimeChar (&timeValue, rgbData + 11, 8, apiLevel);
        CMcpychar(":",(rgbData + 13));     
        CMcpychar(":",(rgbData + 16));     
    
 /*       if (apiLevel > IIAPI_LEVEL_3)
        {
                CMcpychar(".",(rgbData + 19));     
                STprintf (sz, "%09.9lu", pstamp->fraction);
                MEcopy (sz, cbPrecision, rgbData + 20);
        }*/
    }
 
	
    return (CVT_SUCCESS);
}

/*
**  CvtTimestampChar_DB2
**
**  Same as CvtTimestampChar but for DB2
*/

UDWORD CvtTimestampChar_DB2(
    TIMESTAMP_STRUCT     * pstamp,
    CHAR      * rgbData,
    UWORD       cbData,
    UWORD       cbPrecision)
{
    DATE_STRUCT dateValue;
    ISO_TIME_STRUCT timeValue;
    CHAR        sz[12];

    if (cbData < 19)
        return (CVT_OUT_OF_RANGE);
    if (pstamp->fraction && cbPrecision && cbData < (UWORD)(20 + cbPrecision))
        return (CVT_TRUNCATION);

    dateValue.year   = pstamp->year;
    dateValue.month  = pstamp->month;
    dateValue.day    = pstamp->day;

    CvtDateChar (&dateValue, rgbData, 10, IIAPI_VERSION_1);
    
	CMcpychar("-",(rgbData + 10));      /* DB2 Date time separator */

    timeValue.hour   = pstamp->hour;
    timeValue.minute = pstamp->minute;
    timeValue.second = pstamp->second;
    timeValue.fraction = 0;

    CvtTimeChar (&timeValue, rgbData + 11, 8, IIAPI_VERSION_3);

    CMcpychar(".",(rgbData + 13));     /* DB2 timestamp time separator */
    CMcpychar(".",(rgbData + 16)); 

    if (cbPrecision > 0)
    {
        CMcpychar(".",(rgbData + 19)); 
        STprintf (sz, "%09.9lu", pstamp->fraction);
        MEcopy (sz, cbPrecision, rgbData + 20);
    }

    return (CVT_SUCCESS);
}

/*
**  CvtUltoa
**
**  Convert unsigned long integer to a character string:
**
**  This routine converts signed long integer 
**  to character string almost like ultoa
**  but without relying on a C run-time library.
**
**  On entry: value = input unsigned long integer.
**            string-->where to placed output string.
**
**  On exit:  value always returned.
**
**  Returns:  nothing.
*/

VOID CvtUltoa(UDWORD       uintValue,  
              CHAR      *  string)
{
    CHAR buf[12] = "0000000000";
    CHAR table[] = "0123456789";
    CHAR * psz;
    int    i;

    for (psz = &buf[9]; uintValue > 0; psz--)
        {                 /* perform a ulong_to_a */
         i = uintValue % 10;
         *psz = table[i]; 
         uintValue = uintValue / 10;
        }

    for (i=0, psz=buf; i<8; i++, psz++) /* skip leading insignificant zeros */
        if (*psz !=  '0')
            break;
    STcopy(psz, string);                /* copy significant digits to output */
}


/*
**  CvtUnpack
**
**  Convert SQL DECIMAL into SQL NUMERIC:
**
**  This routine implements the IBM 370 UNPK instruction.
**  This always returns Realia COBOL sign nibbles.
**
**  On entry: rgbNumeric-->where to return unpacked number.
**            rgbDecimal-->packed number.
**            cbDecimal  = length of packed number.
**
**  On exit:  Unpacked value always returned.
**
**  Returns:  nothing.
*/

VOID CvtUnpack(
    CHAR      * rgbNumeric,
    CHAR      * rgbDecimal,
    UWORD       cbDecimal)
{
    CHAR      * pd;
    CHAR      * pn;
    CHAR        s, n;
    UWORD       i;

    pd = rgbDecimal;
    pn = rgbNumeric;

    for (i = 1; i < cbDecimal; i++)
    {
        *pn++ = '0' + (*pd >> 4 & 0x0F);
        *pn++ = '0' + (*pd & 0x0F);
        pd++;
    }

    s = (CHAR)(*pd & 0x0F);
    n = (CHAR)(*pd >> 4 & 0x0F);

    switch (s)
    {
    case 0x0B:
    case 0x0D:

        *pn = (CHAR)(0x20 + n);
        break;

    default:

        *pn = (CHAR)(0x30 + n);
        break;
    }

    return;

}


/*
//  CvtFloatToChar
//
//  Convert a SQL float to a character value.
//
//  On entry: szValue -->where to return character value.
//            precision -->the floating-point precision.
//            rbgData -->SQL_FLOAT data.
//            decimal_char -->character specified by II_DECIMAL.
//
//  On exit:  szValue contains character string.
//
//  Returns:  length of character string.
*/

UWORD CvtDoubleToChar (SDOUBLE number, UWORD precision, CHAR     * buffer,
    UCHAR decimal_char)
{
#ifdef USE_WINDOWS_FCVT
   char     *temp ;

   int  decimal_spot,
        sign,
        count,
        fcvtcount,
        current_location = 0 ;

   char szValue[34] = "\0";

   temp = _fcvt (number, precision, &decimal_spot, &sign) ;

/* Add negative sign if required. */ 

   if (sign)
      buffer [current_location++] = '-' ;

/* Place decimal point in the correct location. */ 

   if (decimal_spot > 0)
   {
      strncpy (&buffer [current_location], temp, decimal_spot) ;
      buffer [decimal_spot + current_location] = decimal_char ;
      strcpy (&buffer [decimal_spot + current_location + 1],
                      &temp [decimal_spot]) ;
   }
   else
   {
      buffer [current_location++] = '0' ;
      buffer [current_location] = decimal_char ;
      for(count = current_location;
             count<abs(decimal_spot)+current_location; count++)
         buffer [count + 1] = '0' ;
      strcpy (&buffer [count + 1], temp) ;
   }
/*
**  Strip off trailing zeros until non-zero character or decimal point.
*/
   count = strlen(buffer) - 1;
   while (buffer[count] == '0' && count > current_location )
   {
       buffer[count] = '\0';
       count--;
   }

/*  Strip trailing decimal point */
   if (count > 0  &&  buffer[count] == decimal_char)
       buffer[count--] = '\0';

   fcvtcount = count + 1;

/*
**  return the shortest character string that represents the float
*/
   if (fcvtcount <= 6)      /* best exponent format len is strlen("x.E+nn") */
       return(fcvtcount);   /* return now if we can't beat that */

   count = strlen(_gcvt(number, precision, szValue));
   if (count > 0  &&  szValue[count-1] == '.')
       szValue[--count] = '\0';

   if (precision == 7  &&  count>=5) /* convert n.nnnne+012  to n.nnnne+12   */
      {char *s = &szValue[count-5];  /* so that it will fit in DISPLAY_LEN=13 */
       if ( (*s=='e'  ||  *s=='E')  &&  *(s+2)=='0')
          { int i;
            s =s+2;               /* change "012\0" to "12\0\0" */
            for (i=0; i<3; i++, s++)
                {  *s = *(s+1); 
                }
            count--;           /* decrement count for the lost exponent digit */
          }
      }

   if (fcvtcount <= count)  /* return if fcvt is the shortest */
       return(fcvtcount);

   /* Replace period with comma if II_DECIMAL setting dictates */

   if (decimal_char == ',') 
   {
       if ((decimal_spot = strcspn(szValue,".")) < strlen(szValue)) 
           szValue[decimal_spot] = decimal_char; 
   }
   strcpy(buffer, szValue); /* gcvt format is the shortest */

   return (count);
#else

   char        szWork[34] = "\0";
   char      * szValue = &szWork[0];
   i2          res_width;          /* result width from CVfa */
   UWORD       cbData;             /* length of parm in buffer */
   char        szdecimal_char[] = ".";

   szdecimal_char[0] = decimal_char;  /* build decimal-pt string from char */

   if (precision == 15)
      {
            /* Display size of SQL_FLOAT or SQL_DOUBLE must be <= 22 */
            CVfa(number, 22, 14, 'g', decimal_char, szValue, &res_width);
            STtrmwhite(szValue);
      }
   else /* (precision == 7) */
      {
            CVfa(number, 14, 6, 'g', decimal_char, szValue, &res_width);
            STtrmwhite(szValue);
            cbData = (UWORD)STlength (szValue);
            if (cbData>=5)                     /* convert n.nnnne+012  to n.nnnne+12   */
               {CHAR *s  = &szValue[cbData-5];  /* so that it will fit in DISPLAY_LEN=13 */
                if ( (*s=='e'  ||  *s=='E')  &&  *(s+2)=='0')
                   { int i;
                     s =s+2;               /* change "012\0" to "12\0\0" */
                     for (i=0; i<3; i++, s++)
                         {  *s = *(s+1); 
                         }
                   }
               }
      }

   if (*szValue == ' ')  /* leading space, squeeze it out */
        szValue++;

   cbData = (UWORD)STlength (szValue);

   /* strip down the string to the minimum size */
   if (STindex(szValue,"e",0)!=NULL  ||  /* return if has exponent */
       STindex(szValue,"E",0)!=NULL)
         goto CvtDoubleToChar_xit;

   if (STindex(szValue,szdecimal_char,0)==NULL) /* return if has no decimal places */
         goto CvtDoubleToChar_xit;

   while (szValue[cbData-1]=='0')        /* 1.000000 */
      szValue[--cbData] = '\0';          /* strip trailing zeros in decimal */

   if    (szValue[cbData-1]==decimal_char) /* 1.  */
      szValue[--cbData] = '\0';          /* strip orphaned decimal point */

CvtDoubleToChar_xit:
   STcopy(szValue,buffer);
   return cbData;

#endif
}

/*
**  CvtStrSqlNumeric
**
**  Convert a numeric character string in default format to a 
**     SQL_NUMERIC_STRUCT.
**
**  On entry: ns -->buffer containing returned SQL_NUMERIC_STRUCT.
**            pird -->the parameter implementation row descriptor.
**            cbData -->length of the string.
**            szValue -->the input numeric string.
**
**  On exit:  ns contains SQL_NUMERIC_STRUCT.
**
**  Returns:  CVT_SUCCESS
**            CVT_OUT_OF_RANGE
**            CVT_TRUNCATION
*/

UWORD CvtStrSqlNumeric( SQL_NUMERIC_STRUCT *ns, LPDESCFLD pird, SQLINTEGER cbData,
    CHAR     *szValue )
{
    UCHAR Product[SQL_MAX_NUMERIC_LEN+4];
    UCHAR Temp[SQL_MAX_NUMERIC_LEN+4];
    UCHAR Num[SQL_MAX_NUMERIC_LEN+4];
    CHAR *endPtr=NULL, *scratchStr, *t = MEreqmem(0, cbData+4, TRUE, NULL), 
        *tmpStr = NULL;
    i4 i, current_scale, exponent=0;
    BOOL isPositive = TRUE;
    UWORD rc = CVT_SUCCESS;

    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, Product);
    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, Temp);
    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, Num);
    tmpStr = t;
    MEcopy(szValue, cbData, tmpStr);
    ns->precision = (UCHAR)pird->Precision;
    current_scale = ns->scale = (SQLCHAR)pird->Scale;

    if (ns->scale > (SQLSCHAR)ns->precision)
        return CVT_OUT_OF_RANGE;

    /*
    **  Extract any exponents.
    */
    CVupper( tmpStr );
    scratchStr = STindex(tmpStr, "E", 0);
    if (scratchStr)
    {
        *scratchStr = '\0';
        CMnext(scratchStr); 
        if (*scratchStr == '-')
            isPositive = FALSE;
        else if (*scratchStr == '+')
            isPositive = TRUE;
        else
            rc = CVT_OUT_OF_RANGE;
        if (rc == CVT_SUCCESS)  
        {
            CMnext(scratchStr);
            if (CVal(scratchStr, &exponent) != OK)
                 rc = CVT_ERROR;
            else
                 ns->scale = (UCHAR)exponent;
        }
    }
    if ((rc == CVT_ERROR) || (rc == CVT_OUT_OF_RANGE))
    {
        MEfree(t);
        return (rc);
    }
    switch (tmpStr[0])
    {
        case '-':
            ns->sign = 0;
            tmpStr++;
            break;

        case '+':
            ns->sign = 1;
            tmpStr++;
            break;

        default:
            ns->sign = 1;
            break;
    } 
    /*
    ** Remove any decimal points from the numeric string and note
    ** the provided scale.
    */
    scratchStr = STindex(tmpStr, ".", cbData);
    if (scratchStr)
    {
        current_scale = STlength(scratchStr) - 1;
        if (!ns->scale)
            ns->scale = (UCHAR)(current_scale);
        *scratchStr = '\0';
        scratchStr++;
        STcat(tmpStr,scratchStr);
    }

    /*
    ** If the numeric string's scale is less than the scale provided,
    ** append ASCII zeros until the string is filled up to the scale.
    */
    if (*tmpStr == '0')
        tmpStr++;
    else
    {
        scratchStr = tmpStr + STlength(tmpStr);
        if (current_scale < ns->scale)
        {
            for (i=0;i<(ns->scale - current_scale); i++)
            {
                *scratchStr = '0';
                CMnext(scratchStr);
            }
        }
        *scratchStr = '\0';
    }
    /*
    **  Convert each ASCII digit to a number and iteratively add the value * 10.
    */
    i = 0;
    while(*tmpStr <= '9' && *tmpStr >= '0')
    {
        i++;
        Num[0] = (UCHAR)(*tmpStr - '0');
        Multiply128( Product, Temp );
	Add128( ns->val, Product, Num );
        MEcopy( ns->val, SQL_MAX_NUMERIC_LEN, Temp );
        tmpStr++;
    }
    if (i > (i4)ns->precision)
        rc = CVT_TRUNCATION;

    MEfree(t);
    return rc;
}

/*
**  Add128
**
**  Add two 128-bit integers.
**
**  On entry: result -->128-bit buffer containing sum.
**            NumA -->first number added.
**            NumB -->second number added.
**
**  On exit:  result contains sum.
**
**  Returns:  void.
*/
void Add128(UCHAR *result, UCHAR *numA, UCHAR *numB)
{
    u_i2 i; 
    u_i4 carry=0,temp,temp1,temp2;

    for (i = 0; i < SQL_MAX_NUMERIC_LEN; i++)
    {
        temp = (u_i4)numA[i] + (u_i4)numB[i];
        temp1 = temp;
        temp += carry;
	temp2 = (temp % 0x100);
	*(result+1) = 0;
	*(result+2) = 0;
	*(result+3) = 0;
        *result = (UCHAR)temp2;
        if (carry && !temp2)
            carry += (temp1 / 0x100);
        else
            carry = temp1 / 0x100;
        result++;
    }
}

/*
**  Multiply128
**
**  Multiply a 128-bit integer by 10.
**
**  On entry: result -->128-bit buffer containing product.
**            multiplier -->multplier.
**
**  On exit:  result contains product.
**
**  Returns:  void.
*/
void Multiply128( UCHAR * result, UCHAR * inputBuff)
{
    UCHAR temp[SQL_MAX_NUMERIC_LEN],temp1[SQL_MAX_NUMERIC_LEN];
 
    MEfill(SQL_MAX_NUMERIC_LEN, 0, temp);
    MEfill(SQL_MAX_NUMERIC_LEN, 0, temp1);
    MEcopy(inputBuff,SQL_MAX_NUMERIC_LEN,temp);
    MEcopy(inputBuff,SQL_MAX_NUMERIC_LEN,temp1);
    RollLeft128(temp);
    RollLeft128(temp1);
    RollLeft128(temp1);
    RollLeft128(temp1);
    Add128( result, (UCHAR *)temp, (UCHAR *)temp1 );
}    

/*
**  RollLeft128
**
** Roll left once on a 128-bit buffer.
**
**  On entry: operand -->Buffer containing 128-bit integer.
**
**  On exit:  operand is rolled left one bit.
**
**  Returns:  void.
*/
void RollLeft128(UCHAR *operand)
{
    i4 i;
    UCHAR carry[SQL_MAX_NUMERIC_LEN];

    MEfill(SQL_MAX_NUMERIC_LEN, 0, carry);
    for ( i = 0; i < SQL_MAX_NUMERIC_LEN; i++ )
    {
        if ( operand[i] & 0x80 )
        {
            carry[i] = 1; 
        }
        operand[i] = operand[i] << 1;
        if (i)
        {
            if ( carry[i-1])
            {
                operand[i] |= 1; 
                carry[i-1] = 0;
            }
        }
    }
}
/*
**  RollRight128
**
** Roll right once on a 128-bit buffer.
**
**  On entry: operand -->Buffer containing 128-bit integer.
**
**  On exit:  operand is rolled right one bit.
**
**  Returns:  void.
*/
void RollRight128( UCHAR *operand )
{
    i4 i; 
    UCHAR carry[SQL_MAX_NUMERIC_LEN];

    MEfill(SQL_MAX_NUMERIC_LEN, 0, carry);
    for ( i = SQL_MAX_NUMERIC_LEN-1; i >=0; i-- )
    {
        if (operand[i] & 1)
            carry[i] = 1; 
        operand[i] = operand[i] >> 1;
        if ( i < SQL_MAX_NUMERIC_LEN-1 )
	{
            if ( carry[i+1] )
	    {
                operand[i] |= 0x80;
                carry[i+1] = 0;
            }
        }
    }
}

/*
**  Subtract128
**
**  Subtract two 128-bit integers.
**
**  On entry: result -->128-bit buffer containing difference.
**            NumA -->number to subtracted.
**            NumB -->subtractor.
**
**  On exit:  result contains sum.
**
**  Returns:  void.
*/
void Subtract128(UCHAR *result, UCHAR *numA, UCHAR *numB)
{
    i4 i;

    UCHAR carry = 0, saveCarry = 0;

    for (i=0;i<SQL_MAX_NUMERIC_LEN;i++)
    {
        if (numA[i])
	{
	    if ((UCHAR)(numA[i] - saveCarry) < numB[i])
                carry = 1;
            else
                carry = 0;
        }
	else
	{
	    if (numB[i])
                carry = 1;
            else
                carry = 0;
	}
        result[i] = numA[i] - numB[i] - saveCarry;
        saveCarry = carry;
    }
}

/*
**  LessThanOrEqual128
**
** Conditional function to see if operand is <= operator.
**
**  On entry: operator -->128-bit integer to compare.
**            operand -->128-bit integer compared.
**
**  On exit:  Returns TRUE if operand is <= operator, FALSE if not.
**
**  Returns:  void.
*/
BOOL LessThanOrEqual128(UCHAR *Operator, UCHAR *Operand)
{
    i4 i;

    for (i = SQL_MAX_NUMERIC_LEN-1; i >= 0; i--)
    {
        if ( Operator[i] > Operand[i] )
            return TRUE;
        else if ( Operator[i] < Operand[i] )
            return FALSE;
    }
    return TRUE;
}

/*
**  EqualZero128
**
** Conditional function to see if 128-bit integer is zero.
**
**  On entry: Num -->128-bit integer to compare.
**
**  On exit:  Returns TRUE if Num is 0, FALSE if not.
**
**  Returns:  void.
*/
BOOL EqualZero128(UCHAR *Num)
{
	i4 i;

	for (i = 0; i < SQL_MAX_NUMERIC_LEN; i++)
	{
	    if ( Num[i] )
	        return FALSE;
	}
	return TRUE;
}

/*
**  CvtSqlNumericStr
**
**  Convert a SQL_NUMERIC_STRUCT structure to a numeric string..
**
**  On entry: numStr -->buffer containing returned numeric string.
**            scale -->the scale of the target column.
**            cbData -->SQL_NUMERIC_STRUCT containing the numeric structure.
**
**  On exit:  numStr contains the numeric string in this format:
**
**          [ - ] digit(s) . [digit(s) ] 
**          Examples:
**                 0.123
**             -1234.56
**                12
**
**  Returns:  CVT_SUCCESS
**            CVT_TRUNCATION
**            CVT_OUT_OF_RANGE
*/

UWORD CvtSqlNumericStr(CHAR     * numStr, UWORD precision, UWORD scale, 
    SQL_NUMERIC_STRUCT ns )
{
    i4 i;
    UCHAR Result[SQL_MAX_NUMERIC_LEN+4];
    UCHAR Remainder[SQL_MAX_NUMERIC_LEN+4];
    CHAR *dStr;
    CHAR *tmpStr;
    CHAR *tmp, *DecimalStr;
    UCHAR Numeric[SQL_MAX_NUMERIC_LEN+4];
    UWORD rc = CVT_SUCCESS;

    if (precision < scale)
         return CVT_OUT_OF_RANGE;
    dStr = MEreqmem(0, precision+1, TRUE, NULL);
    tmpStr = MEreqmem(0, precision+1, TRUE, NULL);
    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, Result);
    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, Remainder);
    MEfill(precision, '0', tmpStr);
    MEfill(precision, '0', dStr);
    dStr[precision] = '\0';
    tmpStr[precision] = '\0';
    tmp = tmpStr + precision - 1 ;
    DecimalStr = dStr + precision - 1 ;
    MEcopy( ns.val, SQL_MAX_NUMERIC_LEN, Numeric );
    /*
    **  This section does the 128-bit equivalent of CVla().
    */
    for (i=0;i<(i4)precision;i++)
    {
        Divide128(Remainder, Result, Numeric );
        *tmp-- = (CHAR)Remainder[0] + '0';
        if ( EqualZero128( Result ) )
            break;
        DecimalStr--;
        MEcopy( (UCHAR *)Result, SQL_MAX_NUMERIC_LEN, Numeric );
    }
    if ( !EqualZero128( Result ) )
        rc = CVT_TRUNCATION;
    if (!ns.sign)
    {
        if ( dStr[0] != '0' )
        {
            rc = CVT_TRUNCATION;
        } 
        else
        {
            *numStr++ = '-';
        }
    }
    do
    {
        *DecimalStr++ = *++tmp;
    }  while (*tmp); 
    /*
    **  Remove leading zeros.
    */
    i = 0;
    while (i < (i4)(precision - scale - 1))
    {
        if (*(dStr+i) == '0')
           i++;
        else 
           break;
    }
    MEcopy(dStr + i, (precision - i - scale), numStr);
    if (scale)
    {
        numStr += (precision - i - scale);
        *numStr = '.';
        numStr++;
        STcopy(dStr + (precision - scale), numStr);  
    }
    MEfree(tmpStr);
    MEfree(dStr);
    return rc;
}

/*
**  Divide128
**
**  Perform long division on a 128-bit integer.  The divisor in this
**  case is always 10.
**
**  The algorithm performs the following using a 128-bit dividand per this
**  eight-bit example:
**            ____________
**      10111 | 10000111
**
**  Start by shifting b until you get a 1 in the msb (10111000) (c = 3 shifts)
**  Then:
** 
**  Loop c+1 times    
**    1) is a >= b?
**    2) if yes, result=result+1 and  a=a-b
**    3) b=b>>1  result=result<<1 
**
** The remainder is in a.
**
**  On entry: remainder -->buffer pointing to the remainder.
**            quotient -->buffer pointing to the quotient.
**            dividand -->buffer containing the number to be divided.
**
**  On exit:  remainder contains the remainder; quotient contains the
**            result of the division.
**
**  Returns:  void.
*/
void Divide128( UCHAR *remainder, UCHAR *quotient, UCHAR *dividand)
{
    UCHAR temp1[SQL_MAX_NUMERIC_LEN+4];
    UCHAR diff[SQL_MAX_NUMERIC_LEN+4];
    UCHAR one[SQL_MAX_NUMERIC_LEN+4];
    UCHAR b[SQL_MAX_NUMERIC_LEN+4];

    i4 c=1;

    MEfill(SQL_MAX_NUMERIC_LEN, 0, quotient);
    MEfill(SQL_MAX_NUMERIC_LEN, 0, remainder);
    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, temp1);
    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, diff);
    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, one);
    MEfill(SQL_MAX_NUMERIC_LEN+4, 0, b);
    one[0] = 1;
    b[0] = 10;
    if ( EqualZero128( dividand ) )
    {
        return;
    }

    while ( !(b[SQL_MAX_NUMERIC_LEN-1] & 0x80 ) )
    {
        RollLeft128 ( b );
        c++;
    }
    while ( c-- )
    {
        if ( LessThanOrEqual128( dividand, b ))
        {
            Add128( temp1, one, quotient );
            MEcopy( temp1, SQL_MAX_NUMERIC_LEN, quotient );
            Subtract128( diff, dividand, b );
            MEcopy( diff, SQL_MAX_NUMERIC_LEN, dividand );
        }
        RollRight128( b );
        RollLeft128( quotient );
    } 
    RollRight128( quotient );
    MEcopy( dividand, SQL_MAX_NUMERIC_LEN, remainder );
}
/*
**  CvtApiTypes 
**  
**  Convert the source API datatype into the target API datatype.
**
**  On entry: pTarget     -->Buffer pointing to the target data.
**            pSource     -->Buffer pointing to the source data.
**            envHndl     -->API environment handle.
**            pipd        -->IPD (Implementation Parameter Descriptor)
**            dstDataType -->Target datatype.
**            srcDataType -->Source datatype.
**
**  On exit:  pTarget contains the converted data.
**
**  Returns:  TRUE  - Successful conversion.
**            FALSE - Unsuccessful conversion.
**
*/
BOOL CvtApiTypes(CHAR *pTarget, CHAR *pSource,  
    VOID *envHndl, LPDESCFLD pipd, UWORD dstDataType, UWORD srcDataType)
{
    u_i2 dstLen = (u_i2)pipd->Precision;
    u_i2 srcLen = (u_i2)pipd->Precision;
    IIAPI_FORMATPARM   fp;


    if (srcDataType == IIAPI_DEC_TYPE && dstDataType == IIAPI_CHA_TYPE)
        dstLen =  pipd->Precision + 3; 
    else if (srcDataType == IIAPI_CHA_TYPE && dstDataType == IIAPI_DEC_TYPE)
        srcLen = STlength(pSource)+1;

    fp.fd_envHandle = envHndl;
    fp.fd_srcDesc.ds_dataType   = srcDataType; 
    fp.fd_srcDesc.ds_nullable   = FALSE;  
    fp.fd_srcDesc.ds_length     = (II_UINT2) srcLen;
    fp.fd_srcDesc.ds_precision  = (II_UINT2) pipd->Precision;
    fp.fd_srcDesc.ds_scale      = pipd->Scale;
    fp.fd_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
    fp.fd_srcDesc.ds_columnName = NULL;  
    fp.fd_srcValue.dv_null      = FALSE; 
    fp.fd_srcValue.dv_length    = (II_UINT2) srcLen;
    fp.fd_srcValue.dv_value     = pSource; 
          
    fp.fd_dstDesc.ds_dataType   = dstDataType; 
    fp.fd_dstDesc.ds_nullable   = FALSE;
    fp.fd_dstDesc.ds_length     = (II_UINT2) dstLen;
    fp.fd_dstDesc.ds_precision  = (II_UINT2) pipd->Precision;
    fp.fd_dstDesc.ds_scale      = (II_UINT2) pipd->Scale;
    fp.fd_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
    fp.fd_dstDesc.ds_columnName = NULL;
    fp.fd_dstValue.dv_null      = FALSE;
    fp.fd_dstValue.dv_length    = (II_UINT2) pipd->Precision;
    fp.fd_dstValue.dv_value     = pTarget; 

    IIapi_formatData( &fp );
    
    if (fp.fd_status != IIAPI_ST_SUCCESS)
        return(FALSE);

    return(TRUE); 
}
