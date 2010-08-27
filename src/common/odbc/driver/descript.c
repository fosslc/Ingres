/*
** Copyright (c) 2000, 2006 Ingres Corporation
*/

#include <compat.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <tr.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>


#include <cm.h>
#include <cs.h>
#include <cv.h>
#include <lo.h>
#include <me.h>
#include <nm.h>  
#include <si.h>  
#include <st.h> 
#include <er.h>
#include <erodbc.h>
#include <iiapi.h>

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

/*
** Name: DESCRIPT.C
**
** Description:
**	Descriptor routines for ODBC driver.
**
** History:
**	27-nov-2000 (thoda04)
**	    Initial coding
**	26-jun-2001 (thoda04)
**	    Use I2ASSIGN and I4ASSIGN to return desc info
**	12-jul-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	11-dec-2001 (thoda04)
**	    Additional 64-bit changes for overloaded attributes.
**	11-mar-2002 (thoda04)
**	    Add support for row-producing procedures.
**	01-apr-2002 (thoda04)
**	    SQLSetDescRec needs to unlock the DESC if 
**	    CheckDescField failed.
**      30-may-2002 (weife01)
**          Set display size of SQL_VARCHAR and SQL_CHAR to 
**          Precision instead of Length in GetDescDisplaySize()
**          BUG#107867
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**     22-sep-2003 (loera01)
**          Added internal ODBC tracing.
**     20-nov-2003 (loera01)
**          In FreeDescriptor(), free memory from any data pointers.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**     06-jan-2005 (Ralph.Loen@ca.com) Bug 113712
**          In GetDescDisplaySize(), added Unicode data types.
**     11-jan-2005 (Ralph.Loen@ca.com) Bug 113712
**          In GetDescDisplaySize() make references to Unicode types 
**          conditionally compiled.
**     04-Jan-2006 (loera01) Bug 115623
**          In SQLGetColAttribute_InternalCall(), and  
**          SQLGetDescField_InternalCall(), return an octet length of
**          FETMAX instead of the default octet length, if the column
**          is a blob.  
**     08-Jul-2006 (weife01) Bug 116356
**          In SQLGetColAttribute_InternalCall(), return SQL_DESC_OCTET_LENGTH
**          for SQL_VARCHAR, SQL_WVARCHAR and SQL_VARBINARY instead of using
**          default.
**     10-Aug-2006 (Ralph Loen) SIR 116477
**          Add support for ISO dates, times, and intervals (first phase).
**     16-Aug-2006 (Ralph Loen) SIR 116477 (second phase)
**          Update octet length for DS intervals.
**     28-Sep-2006 (Ralph Loen) SIR 116477
**          Adjusted treatment of ISO date/times according to new API/ADF rules.
**     03-Oct-2006 (Ralph Loen) SIR 116477
**          Dynamic parameters for dates and timestamps are now sent as binary
**          values for backward-compatibility with ingresdate.
**     10-Nov-2006 (Ralph Loen) SIR 116477
**          In GetDescLiteralPrefix() and GetDescTypeName(), add 
**          ISO dates/times/intervals.  
**     22-Nov-2006 (Ralph Loen) SIR 116477
**          In SetDescDefaultsFromType(), allowed for ISO time precision to
**          be treated as a separate filed in the IRD.
**     08-Jan-2007 (weife01) Bug 116908
**          The driver returns actual column name for SQL_DESC_BASE_COLUMN_NAME
**          instead of returning empty string. 
**     20-Sep-2007 (weife01) SIR 119140
**          Added support for new DSN/connection string configuration
**          option:"Convert int8 from DBMS to int4".
**     14-Nov-2008 (Ralph Loen) SIR 121228
**         Add support for OPT_INGRESDATE configuration parameter.  
**         CvtGetSqlType() now supports the API connection level and the
**         date connection level.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**     01-Jul-2009 (Ralph Loen) Bug 122174
**          For SQLColAttribute_InternalCall() and 
**          SQLGetDescField_InternalCall(), return the column size for
**          SQL_DESC_LENGTH instead of the byte length.
**     08-Feb-2010 (Ralph Loen) SIR 123266
**          In SetDescDefaultsFromType(), add support for IIAPI_BOOL_TYPE 
**          (boolean).
**     29-Mar-2010 (drivi01)
**	    Update lenValuePtr to point to pxxd->OctetLength to avoid
**          SEGVs as the current pointer causes SEGVs on x64.
**     24-Jun-2010 (Ralph Loen)  Bug 122174
**          Expand 01-Jul-2009 fix to include SQLDescribeCol_InternalCall(),
**          and SQLGetDescField_internalCall(), and SQLColAttributes().  
**          Include date/time data types.
**     25-Jun-2010 (Ralph Loen)  Bug 122174
**          Correct SQL_DESC_OCTET_LENGTH, SQL_DESC_LENGTH, and
**          SQL_DESC_PRECISION for interval types.
**     10-Aug-2010 (drivi01)
**	    Fix pird->OctetLength for 64-bit Windows platform
**	    because pird isn't defined.  Use pxxd instead.
*/

/*
**  Internal functions:
*/
RETCODE CheckDescField(LPDESC pdesc, LPDESCFLD  papd);

/*
**  Static Constants:
*/
static char LiteralPrefixSuffix[3] = "X'";
static struct tag_DescPermissionTable
    {
        i2  FieldIdentifier;
        i2  Permission;
#define       ARD_R    0x1000
#define       ARD_W    0x2000
#define       ARD_RW   ARD_R | ARD_W
#define       APD_R    0x1000   /* ARD and APD have same permissions always */
#define       APD_W    0x2000
#define       APD_RW   APD_R | APD_W
#define       IRD_R    0x0010
#define       IRD_W    0x0020
#define       IRD_RW   IRD_R | IRD_W
#define       IPD_R    0x0001
#define       IPD_W    0x0002
#define       IPD_RW   IPD_R | IPD_W
    }
DescPermissionTable[] =
{                               /* Descriptor Header */
    {SQL_DESC_ALLOC_TYPE,                  ARD_R  | APD_R  | IRD_R  | IPD_R  },
    {SQL_DESC_ARRAY_SIZE,                  ARD_RW | APD_RW                   },
    {SQL_DESC_ARRAY_STATUS_PTR,            ARD_RW | APD_RW | IRD_RW | IPD_RW },
    {SQL_DESC_BIND_OFFSET_PTR,             ARD_RW | APD_RW                   },
    {SQL_DESC_BIND_TYPE,                   ARD_RW | APD_RW                   },
    {SQL_DESC_COUNT,                       ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_ROWS_PROCESSED_PTR,                            IRD_RW | IPD_RW },
                                /* Descriptor Fields */
    {SQL_DESC_AUTO_UNIQUE_VALUE,                             IRD_R           },
    {SQL_DESC_BASE_COLUMN_NAME,                              IRD_R           },
    {SQL_DESC_BASE_TABLE_NAME,                               IRD_R           },
    {SQL_DESC_CASE_SENSITIVE,                                IRD_R  | IPD_R  },
    {SQL_DESC_CATALOG_NAME,                                  IRD_R           },
    {SQL_DESC_CONCISE_TYPE,                ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_DATA_PTR,                    ARD_RW | APD_RW          | IPD_W  },
    {SQL_DESC_DATETIME_INTERVAL_CODE,      ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_DATETIME_INTERVAL_PRECISION, ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_DISPLAY_SIZE,                                  IRD_R           },
    {SQL_DESC_FIXED_PREC_SCALE,                              IRD_R  | IPD_R  },
    {SQL_DESC_INDICATOR_PTR,               ARD_RW | APD_RW                   },
    {SQL_DESC_LABEL,                                         IRD_R           },
    {SQL_DESC_LENGTH,                      ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_LITERAL_PREFIX,                                IRD_R           },
    {SQL_DESC_LITERAL_SUFFIX,                                IRD_R           },
    {SQL_DESC_LOCAL_TYPE_NAME,                               IRD_R /*|IPD_R*/}, /* quiktest says no IPD */
    {SQL_DESC_NAME,                                          IRD_R  | IPD_RW },
    {SQL_DESC_NULLABLE,                                      IRD_R  | IPD_R  },
    {SQL_DESC_NUM_PREC_RADIX,              ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_OCTET_LENGTH,                ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_OCTET_LENGTH_PTR,            ARD_RW | APD_RW                   },
    {SQL_DESC_PARAMETER_TYPE,                                         IPD_RW },
    {SQL_DESC_PRECISION,                   ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_ROWVER,                                        IRD_R  | IPD_R  },
    {SQL_DESC_SCALE,                       ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_SCHEMA_NAME,                                   IRD_R           },
    {SQL_DESC_SEARCHABLE,                                    IRD_R           },
    {SQL_DESC_TABLE_NAME,                                    IRD_R           },
    {SQL_DESC_TYPE,                        ARD_RW | APD_RW | IRD_R  | IPD_RW },
    {SQL_DESC_TYPE_NAME,                                     IRD_R  | IPD_R  },
    {SQL_DESC_UNNAMED,                                       IRD_R  | IPD_RW },
    {SQL_DESC_UNSIGNED,                                      IRD_R  | IPD_R  },
    {SQL_DESC_UPDATABLE,                                     IRD_R           }
};
#define sizeofDescPermissionTable (sizeof(DescPermissionTable)/ \
                                   sizeof(DescPermissionTable[0]))

/*
**  SQLColAttribute
**
**  Returns descriptor information for a column in a result set. 
**  Descriptor information is returned as a character string, 
**  a 32-bit descriptor-dependent value, or an integer value. 
**
**  On entry:
**    StatementHandle [Input]
**    ColumnNumber [Input]
**      The number of the record in the IRD from which the field value 
**      is to be retrieved. This argument corresponds to the column number 
**      of result data, ordered sequentially in increasing column order, 
**      starting at 1. Columns can be described in any order.
**      Column 0 can be specified in this argument, but all values 
**      except SQL_DESC_TYPE and SQL_DESC_OCTET_LENGTH will 
**      return undefined values.
**    FieldIdentifier [Input]
**      The field in row ColumnNumber of the IRD that is to be returned.
**    CharacterAttributePtr [Output]
**      Pointer to a buffer in which to return the value in the 
**      FieldIdentifier field of the ColumnNumber row of the IRD, 
**      if the field is a character string. Otherwise, the field is unused.
**    BufferLength [Input]
**      If FieldIdentifier is an ODBC-defined field and CharacterAttributePtr 
**      points to a character string or binary buffer, this argument should be 
**      the length of *CharacterAttributePtr. 
**      If FieldIdentifier is an ODBC-defined field and *CharacterAttributePtr 
**      is an integer, this field is ignored. 
**      If FieldIdentifier is a driver-defined field, the application 
**      indicates the nature of the field to the Driver Manager by setting 
**      the BufferLength argument.
**      BufferLength can have the following values:
**         If CharacterAttributePtr is a pointer to a pointer, then 
**            BufferLength should have the value SQL_IS_POINTER. 
**         If CharacterAttributePtr is a pointer to a character string, then 
**            BufferLength is the length of the string or SQL_NTS.
**         If CharacterAttributePtr is a pointer to a binary buffer, then 
**            the application places the result of the 
**         SQL_LEN_BINARY_ATTR(length) macro in BufferLength. 
**            This places a negative value in BufferLength.
**         If CharacterAttributePtr is a pointer to a fixed-length data type, 
**            then BufferLength is either SQL_IS_INTEGER, SQL_IS_UNINTEGER, 
**            SQL_SMALLINT, or SQLUSMALLINT.
**    StringLengthPtr [Output]
**      Pointer to a buffer in which to return the total number of bytes 
**      (excluding the null-termination byte for character data) available to 
**      return in *CharacterAttributePtr.
**      For character data, if the number of bytes available to return is 
**      greater than or equal to BufferLength, the descriptor information 
**      in *CharacterAttributePtr is truncated to BufferLength minus the 
**      length of a null-termination character and is null-terminated by 
**      the driver.
**      For all other types of data, the value of BufferLength is ignored 
**      and the driver assumes the size of *CharacterAttributePtr is 32 bits.
**    NumericAttributePtr [Output]
**      Pointer to an integer buffer in which to return the value in the 
**      FieldIdentifier field of the ColumnNumber row of the IRD, if the 
**      field is a numeric descriptor type, such as SQL_DESC_COLUMN_LENGTH. 
**      Otherwise, the field is unused.
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLColAttribute (
    SQLHSTMT     StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier,
    SQLPOINTER   ValuePtrParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLPOINTER   NumericAttributePtr)
{
    return SQLColAttribute_InternalCall(
                 StatementHandle,
                 ColumnNumber,
                 FieldIdentifier,
                 ValuePtrParm,
                 BufferLength,
                 StringLengthPtr,
                 NumericAttributePtr);
}

SQLRETURN  SQL_API SQLColAttribute_InternalCall (
    SQLHSTMT     StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier,
    SQLPOINTER   ValuePtrParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLPOINTER   NumericAttributePtr)
{
    LPSTMT       pstmt       = (LPSTMT)StatementHandle;
    CHAR        *ValuePtr    = (CHAR *)ValuePtrParm;
    SQLINTEGER  *i4ValuePtr  = (SQLINTEGER*)NumericAttributePtr;
    SQLINTEGER      *lenValuePtr = (SQLINTEGER  *)  NumericAttributePtr;
    SQLUINTEGER     *ulenValuePtr= (SQLUINTEGER *)  NumericAttributePtr;
    SQLRETURN    rc          = SQL_SUCCESS;
    LPDESC       pIRD;   /* Implementation Row Descriptor */
    LPDESCFLD    pird;
    char        *s;
	SQLINTEGER   option_bigint = 0;

    TRACE_INTL(pstmt,"SQLColAttribute_InternalCall");
    if (!LockStmt (pstmt))
         return SQL_INVALID_HANDLE;

    ErrResetStmt (pstmt);           /* clear any errors on STMT */

    pIRD = pstmt->pIRD;            /* pdesc->IRD */


    if (FieldIdentifier!= SQL_DESC_COUNT  &&  ColumnNumber > pIRD->Count)
        ErrState(SQL_07009, pstmt); /* Invalid descriptor index */

    pird = pIRD->pcol + ColumnNumber; /* pird->IRD descriptor field */
	option_bigint = pstmt->fOptions & OPT_CONVERTINT8TOINT4;
    switch (FieldIdentifier)
    {
    case SQL_DESC_AUTO_UNIQUE_VALUE:

        if (i4ValuePtr)
            *i4ValuePtr = FALSE; /* = pdesc->AutoUniqueValue; */
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_BASE_COLUMN_NAME:

        rc = GetChar(pstmt, pird->Name,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_BASE_TABLE_NAME:

        rc = GetChar(pstmt, "", /* pird->BaseTableName */
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_CASE_SENSITIVE:

        if (i4ValuePtr)
            *i4ValuePtr = GetDescCaseSensitive(pird);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_CATALOG_NAME:

        rc = GetChar(pstmt, "" /*pird->CatalogName*/,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_COUNT:

        if (lenValuePtr)
           *lenValuePtr = pIRD->Count;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_CONCISE_TYPE:

        if (i4ValuePtr)
            *i4ValuePtr = pird->ConciseType;
        if (option_bigint && *i4ValuePtr == SQL_BIGINT)
            *i4ValuePtr = SQL_INTEGER;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_DISPLAY_SIZE:

        if (lenValuePtr)
        {
           *lenValuePtr = GetDescDisplaySize(pird);
           if (*lenValuePtr == SQL_NO_TOTAL)
               ErrState(SQL_HYC00, pstmt); /* Unsupported argument */
        }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_FIXED_PREC_SCALE:

        if (i4ValuePtr)
            *i4ValuePtr  = pird->FixedPrecScale;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_LABEL:

        rc = GetChar(pstmt, pird->Name, /* pird->Label */
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_LENGTH:

        if (ulenValuePtr)
            *ulenValuePtr = pird->Length;

        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLUINTEGER);
        break;


    case SQL_DESC_LITERAL_PREFIX:

        s = GetDescLiteralPrefix(pird);
        rc = GetChar(pstmt, s /* pird->LiteralPrefix*/,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_LITERAL_SUFFIX:

        s = GetDescLiteralSuffix(pird);
        rc = GetChar(pstmt, s /* pird->LiteralSuffix*/,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_LOCAL_TYPE_NAME:

        rc = GetChar(pstmt, "" /* pird->LocalTypeName*/,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_NAME:

        rc = GetChar(pstmt, pird->Name,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_NULLABLE:

        if (i4ValuePtr)
            *i4ValuePtr  = isaIPD(pIRD)?TRUE:pird->Nullable;
            if ( option_bigint && pird->ConciseType == SQL_BIGINT)
                    *i4ValuePtr  = 0;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_NUM_PREC_RADIX:

        if (i4ValuePtr)
            *i4ValuePtr  = GetDescRadix(pird);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_OCTET_LENGTH:

        if (i4ValuePtr)
        {
            switch (pird->ConciseType)
            {
            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
                *lenValuePtr = sizeof(SQL_DATE_STRUCT);
                break;

            case SQL_TYPE_TIMESTAMP:
                *lenValuePtr = sizeof(SQL_TIMESTAMP_STRUCT);
                break;

            case SQL_LONGVARCHAR:
            case SQL_WLONGVARCHAR:
            case SQL_LONGVARBINARY:
                *lenValuePtr  = FETMAX;
                break;
            
            case SQL_VARCHAR:
            case SQL_WVARCHAR:
            case SQL_VARBINARY:
                *lenValuePtr = pird->OctetLength - sizeof(i2);
                break;
            
            case SQL_BIGINT:
                if (option_bigint)
                {
                    *lenValuePtr = sizeof(SQL_INTEGER); 
                    break;
                }/*if not, fall through*/

            case SQL_DECIMAL:
            case SQL_NUMERIC:
                *lenValuePtr = pird->Precision + 2;
                break;

            default:
                if (pird->VerboseType == SQL_INTERVAL)
                    *lenValuePtr = sizeof(SQL_INTERVAL_STRUCT);
                else
                    *lenValuePtr  = pird->OctetLength;
                break;
            }
        }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_PRECISION:

        if (i4ValuePtr)
        {
            switch (pird->fIngApiType)
            {
            case IIAPI_TS_TYPE:
            case IIAPI_TSTZ_TYPE:
            case IIAPI_TSWO_TYPE:
            case IIAPI_TIME_TYPE:
            case IIAPI_TMTZ_TYPE:
            case IIAPI_TMWO_TYPE:
            case IIAPI_INTDS_TYPE:
                *i4ValuePtr  = (SQLINTEGER)pird->IsoTimePrecision;
                break;

            case IIAPI_DEC_TYPE:
                *i4ValuePtr = pird->Precision;
                break;

            default:
                *i4ValuePtr  = (SQLINTEGER)pird->Precision;
                break;
            }
        }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_ROWVER:

        if (i4ValuePtr)
            *i4ValuePtr  = FALSE;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_SCALE:

        if (i4ValuePtr)
            *i4ValuePtr  = pird->Scale;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_SCHEMA_NAME:

        rc = GetChar(pstmt, "", /* pird->SchemaName */
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_SEARCHABLE:

        if (i4ValuePtr)
            *i4ValuePtr  = GetDescSearchable(pird);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_TABLE_NAME:

        rc = GetChar(pstmt, pird->TableName,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_TYPE:

        if (i4ValuePtr)
        {
            *i4ValuePtr  = pird->VerboseType;
            if((option_bigint) && (pird->VerboseType == SQL_BIGINT))
                *i4ValuePtr = SQL_INTEGER;
        }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_TYPE_NAME:

        rc = GetChar(pstmt, GetDescTypeName(pird),
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_UNNAMED:

        if (i4ValuePtr)
            *i4ValuePtr  = pird->Unnamed;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_UNSIGNED:

        if (i4ValuePtr)
            *i4ValuePtr  = GetDescUnsigned(pird);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_UPDATABLE:

        if (i4ValuePtr)
            *i4ValuePtr  = SQL_ATTR_READWRITE_UNKNOWN;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_COLUMN_LENGTH:      /* 2.x LENGTH definition differs from 3.x */

        if (lenValuePtr)
        {
           if (pird->fIngApiType == IIAPI_DEC_TYPE)
               *lenValuePtr = pird->Precision + 2;
           else
               *lenValuePtr = pird->OctetLength;     
        }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_COLUMN_PRECISION:   /* 2.x PRECISION definition differs from 3.x */

        if (i4ValuePtr)
            *i4ValuePtr  = (SQLINTEGER)pird->Length;   /* ???? different for 2.x ????*/
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_COLUMN_SCALE:       /* 2.x SCALE definition differs from 3.x */

        if (i4ValuePtr)
            *i4ValuePtr  = pird->Scale;       /* ???? different for 2.x ????*/
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    default:                        /* not a valid FieldIdentifier */

        rc = ErrState(SQL_HY091,pstmt);
            /* unknown FieldIdentifier: Invalid descriptor field identifier */
        break;

    }

    UnlockStmt(pstmt);
    return(rc);
}


/*
**  SQLCopyDesc
**
**  SQLCopyDesc copies descriptor information from one 
**              descriptor handle to another. 
**
**  On entry:
**    SourceDescHandle [Input]
**    TargetDescHandle [Input]
**      ARD, IRD, or IPD.  Cannot be a IRD.
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
**  Notes:
**    Can copy from any descriptor.
**    Can copy from an IRD only if statement in prepared or executed state
**    Can copy into any descriptor but not into an IRD.
**
*/
SQLRETURN  SQL_API SQLCopyDesc (
    SQLHDESC SourceDescHandle,
    SQLHDESC TargetDescHandle)
{
    LPDESC        pdescsrc = (LPDESC)SourceDescHandle;
    LPDESC        pdesctgt = (LPDESC)TargetDescHandle;
    LPDESCFLD     psrc;
    LPDESCFLD     ptgt;
    SQLSMALLINT   Count;
    SQLSMALLINT   i;

    if (!LockDesc (pdesctgt))
         return SQL_INVALID_HANDLE;

    ErrResetDesc (pdesctgt);        /* clear any errors on DESC */

    /*
    **  Ensure that target descriptor is large enough, else re-allocate:
    */
    Count = pdescsrc->Count;

    if (Count > pdesctgt->CountAllocated)
        if (ReallocDescriptor(pdesctgt, Count) == NULL)
           {
            RETCODE rc = ErrState (SQL_HY001, pdesctgt, F_OD0001_IDS_BIND_ARRAY);
            UnlockDesc (pdesctgt);
            return rc;
           }

    /*
    **  Copy descriptor fields (including bookmark at field 0):
    */
    psrc = pdescsrc->pcol;   /* psrc->source fields */
    ptgt = pdesctgt->pcol;   /* ptgt->target fields */

    for (i=0; i<= Count; i++,psrc++,ptgt++)
        {
         STcopy (psrc->Name, ptgt->Name);
/*       ptgt->AutoUniqueValue           = psrc->AutoUniqueValue; */
         ptgt->BaseColumnName            = psrc->BaseColumnName;
         ptgt->BaseTableName             = psrc->BaseTableName;
/*       ptgt->CaseSensitive             = psrc->CaseSensitive;   */
/*       ptgt->CatalogName               = psrc->CatalogName;     */
         ptgt->ConciseType               = psrc->ConciseType;
         ptgt->DataPtr                   = psrc->DataPtr;
         ptgt->DatetimeIntervalCode      = psrc->DatetimeIntervalCode;
         ptgt->DatetimeIntervalPrecision = psrc->DatetimeIntervalPrecision;
         ptgt->DisplaySize               = psrc->DisplaySize;
/*       ptgt->FixedPrecScale            = psrc->FixedPrecScale; */
         ptgt->IndicatorPtr              = psrc->IndicatorPtr;
         ptgt->Label                     = psrc->Label;
         ptgt->Length                    = psrc->Length;
/*       ptgt->LiteralPrefix             = psrc->LiteralPrefix; */
/*       ptgt->LiteralSuffix             = psrc->LiteralSuffix; */
/*       ptgt->LocalTypeName             = psrc->LocalTypeName; */
         ptgt->Nullable                  = psrc->Nullable;
/*       ptgt->NumPrecRadix              = psrc->NumPrecRadix;  */
         ptgt->OctetLength               = psrc->OctetLength;
         ptgt->OctetLengthPtr            = psrc->OctetLengthPtr;
         ptgt->ParameterType             = psrc->ParameterType;
         ptgt->Precision                 = psrc->Precision;
/*       ptgt->Rowver                    = psrc->Rowver; */
         ptgt->Scale                     = psrc->Scale;
         ptgt->SchemaName                = psrc->SchemaName;
/*       ptgt->Searchable                = psrc->Searchable; */
         ptgt->TableName                 = psrc->TableName;
         ptgt->VerboseType               = psrc->VerboseType;
/*       ptgt->TypeName                  = psrc->TypeName; */
         ptgt->Unnamed                   = psrc->Unnamed;
/*       ptgt->Unsigned                  = psrc->Unsigned; */
/*       ptgt->Updatable                 = psrc->Updatable;*/
         ptgt->fStatus                   = psrc->fStatus;
         ptgt->BindOffsetPtr             = psrc->BindOffsetPtr;
        }  /* end for loop through descriptor fields */

    ResetDescriptorFields(pdesctgt, pdescsrc->Count+1, pdesctgt->Count);
        /* clear out unused fields in target beyond the new Count */

    /*
    **  Copy descriptor header fields
    */
    pdesctgt->ArraySize                  = pdescsrc->ArraySize;
    pdesctgt->ArrayStatusPtr             = pdescsrc->ArrayStatusPtr;
    pdesctgt->BindOffsetPtr              = pdescsrc->BindOffsetPtr;
    pdesctgt->BindType                   = pdescsrc->BindType;
    pdesctgt->Count                      = pdescsrc->Count;
    pdesctgt->RowsProcessedPtr           = pdescsrc->RowsProcessedPtr;

    UnlockDesc(pdesctgt);
    return(SQL_SUCCESS);
}


/*
**  SQLGetDescField
**
**  SQLGetDescField returns the current setting or value of a 
**  single field of a APD, ARD, IRD, or IPD descriptor.
**
**  On entry:
**    DescriptorHandle [Input]
**    RecNumber        [Input]
*       Descriptor field number.  Ignored if getting a header field.
**    FieldIdentifier  [Input]
**    ValuePtr         [Output]
**      A pointer to memory in which to return the current value of the
**      descriptor field specified by RecNumber.
**    BufferLength     [Input]
**      If *ValuePtr is an integer, BufferLength is ignored
**    StringLengthPtr  [Output]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLGetDescField (
    SQLHDESC    DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER  ValuePtr,
    SQLINTEGER  BufferLength,
    SQLINTEGER *StringLengthPtr)
{
    return SQLGetDescField_InternalCall (
                DescriptorHandle,
                RecNumber,
                FieldIdentifier,
                ValuePtr,
                BufferLength,
                StringLengthPtr);
}

SQLRETURN  SQL_API SQLGetDescField_InternalCall (
    SQLHDESC    DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER  ValuePtr,
    SQLINTEGER  BufferLength,
    SQLINTEGER *StringLengthPtr)
{
    LPDESC        pdesc = (LPDESC)DescriptorHandle;
    LPDESCFLD     pxxd;
    SQLRETURN     rc    = SQL_SUCCESS;
    SQLSMALLINT  *i2ValuePtr = (SQLSMALLINT *)ValuePtr;
    SQLINTEGER   *i4ValuePtr = (SQLINTEGER  *)ValuePtr;
    SQLUINTEGER  *u4ValuePtr = (SQLUINTEGER *)ValuePtr;
    SQLUSMALLINT *u2ValuePtr = (SQLUSMALLINT*)ValuePtr;
    SQLUINTEGER      *ulenValuePtr=(SQLUINTEGER     *)ValuePtr;
    SQLINTEGER       *lenValuePtr =(SQLINTEGER      *)ValuePtr;
    SQLPOINTER   *pvalue     = (SQLPOINTER*)  ValuePtr;
    BOOL          foundit;
    i2            PermissionNeeded;
    i2            i;
    i2            i2temp;
    SQLINTEGER        i4temp;
    char         *s;

    if (!LockDesc (pdesc))
         return SQL_INVALID_HANDLE;

    ErrResetDesc (pdesc);           /* clear any errors on DESC */

    if (FieldIdentifier == SQL_DESC_ALLOC_TYPE  ||
        FieldIdentifier == SQL_DESC_ARRAY_SIZE  ||
        FieldIdentifier == SQL_DESC_ARRAY_STATUS_PTR  ||
        FieldIdentifier == SQL_DESC_BIND_OFFSET_PTR  ||
        FieldIdentifier == SQL_DESC_BIND_TYPE  ||
        FieldIdentifier == SQL_DESC_COUNT  ||
        FieldIdentifier == SQL_DESC_ROWS_PROCESSED_PTR)
            RecNumber = 0;   /* RecNumber is ignored for header fields */

    if (RecNumber > pdesc->Count)   /* if RecNumber off the edge of the world */
       {                            /*    then return SQL_NO_DATA */
        UnlockDesc(pdesc);
        return(SQL_NO_DATA);
       }

    for (i=0; i<sizeofDescPermissionTable; i++)  /* find the RW permission */
        if (FieldIdentifier == DescPermissionTable[i].FieldIdentifier)
            break;
    if (i >= sizeofDescPermissionTable)  /* error if not in table */
        return(ErrUnlockDesc(SQL_HY091,pdesc)); /* invalid field id */

    if      (isaIRD(pdesc))   PermissionNeeded = IRD_R;
    else if (isaIPD(pdesc))   PermissionNeeded = IPD_R;
    else                      PermissionNeeded = APD_R;  /* APD and ARD same */

    if (!(PermissionNeeded & DescPermissionTable[i].Permission))
        return(ErrUnlockDesc(SQL_HY091,pdesc)); /* invalid field id */

    foundit = TRUE;                 /* assume we find a DESC header field */
    switch(FieldIdentifier)         /* looking for DESC header fields */
    {
    case SQL_DESC_ALLOC_TYPE:

        if (ValuePtr)
            I2ASSIGN_MACRO(pdesc->AllocType, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_ARRAY_SIZE:

        if (ValuePtr)
#ifdef _WIN64
            *ulenValuePtr = pdesc->ArraySize;
#else
            I4ASSIGN_MACRO(pdesc->ArraySize, *u4ValuePtr);
#endif
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLUINTEGER);
        break;

    case SQL_DESC_ARRAY_STATUS_PTR:

        if (ValuePtr)
            *pvalue     = pdesc->ArrayStatusPtr;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLUSMALLINT*);
        break;

    case SQL_DESC_BIND_OFFSET_PTR:

        if (ValuePtr)
            *pvalue     = pdesc->BindOffsetPtr;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLUINTEGER*);
        break;

    case SQL_DESC_BIND_TYPE:

        if (ValuePtr)
            I4ASSIGN_MACRO(pdesc->BindType, *i4ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_COUNT:

#ifdef _WIN64
        if (ValuePtr)
            *lenValuePtr = pdesc->Count;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
#else
        if (ValuePtr)
            I2ASSIGN_MACRO(pdesc->Count, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
#endif
        break;

    case SQL_DESC_ROWS_PROCESSED_PTR:

        if (ValuePtr)
            *pvalue     = pdesc->RowsProcessedPtr;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLPOINTER);
        break;

    default:                        /* not a DESC header field */

        foundit = FALSE;               /* indicate we found nothing so far */
        break;
    }   /* end switch(FieldIdentifier) */


    if (foundit)   /* if we found the FieldIdentifier in the header, all done! */
       {
        UnlockDesc(pdesc);
        return(SQL_SUCCESS);
       }

    /* 
    ** if not found yet, try the fields
    */

    if ( RecNumber < 0  ||  
        (RecNumber == 0  &&  isaIPD(pdesc)))
            return ErrUnlockDesc(SQL_07009, pdesc); /* Invalid descriptor index */


    pxxd = pdesc->pcol + RecNumber; /* pxxd->descriptor field */


    switch(FieldIdentifier)         /* looking for DESC header fields */
    {
    case SQL_DESC_AUTO_UNIQUE_VALUE:

        if (ValuePtr)
           {i4temp = FALSE;      /* = pdesc->AutoUniqueValue; */
            I4ASSIGN_MACRO(i4temp, *i4ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_BASE_COLUMN_NAME:

        rc = GetCharAndLength(pdesc, pxxd->Name,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_BASE_TABLE_NAME:

        rc = GetCharAndLength(pdesc, "", /* pxxd->BaseTableName */
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_CASE_SENSITIVE:

        if (ValuePtr)
           {i4temp = GetDescCaseSensitive(pxxd);
            I4ASSIGN_MACRO(i4temp, *i4ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_CATALOG_NAME:

        rc = GetCharAndLength(pdesc, "" /*pxxd->CatalogName*/,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_CONCISE_TYPE:

        if (ValuePtr)
            I2ASSIGN_MACRO(pxxd->ConciseType, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_DATA_PTR:

        if (ValuePtr)
            *pvalue = pxxd->DataPtr;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLPOINTER);
        break;

    case SQL_DESC_DATETIME_INTERVAL_CODE:

        if (ValuePtr)
            I2ASSIGN_MACRO(pxxd->DatetimeIntervalCode, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_DATETIME_INTERVAL_PRECISION:

        if (ValuePtr)
           {i4temp = pxxd->DatetimeIntervalPrecision;
            I4ASSIGN_MACRO(i4temp, *i4ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_DISPLAY_SIZE:

        if (ValuePtr)
           {
#ifdef _WIN64
            *lenValuePtr = GetDescDisplaySize(pxxd);
         
#else
            i4temp = GetDescDisplaySize(pxxd);
            I4ASSIGN_MACRO(i4temp, *i4ValuePtr);
#endif
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_FIXED_PREC_SCALE:

        if (ValuePtr)
            I2ASSIGN_MACRO(pxxd->FixedPrecScale, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_INDICATOR_PTR:

        if (ValuePtr)
            *pvalue = pxxd->IndicatorPtr;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLPOINTER);
        break;

    case SQL_DESC_LABEL:

        rc = GetCharAndLength(pdesc, pxxd->Name, /* pxxd->Label */
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_LENGTH:

        if (ValuePtr)
        {
#ifdef _WIN64
            *ulenValuePtr = (u_i4)pxxd->Length;
#else
            i4temp = pxxd->Length;
            I4ASSIGN_MACRO(i4temp, *u4ValuePtr);
#endif
        }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLUINTEGER);
        break;


    case SQL_DESC_LITERAL_PREFIX:

        s = GetDescLiteralPrefix(pxxd);
        rc = GetCharAndLength(pdesc, s /* pxxd->LiteralPrefix*/,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_LITERAL_SUFFIX:

        s = GetDescLiteralSuffix(pxxd);
        rc = GetCharAndLength(pdesc, s /* pxxd->LiteralSuffix*/,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_LOCAL_TYPE_NAME:

        rc = GetCharAndLength(pdesc, "" /* pxxd->LocalTypeName*/,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_NAME:

        rc = GetCharAndLength(pdesc, pxxd->Name,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_NULLABLE:

        if (ValuePtr)
           {i2temp = isaIPD(pdesc)?TRUE:pxxd->Nullable;
            I2ASSIGN_MACRO(i2temp, *i2ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_NUM_PREC_RADIX:

        if (ValuePtr)
           {i4temp = GetDescRadix(pxxd);
            I4ASSIGN_MACRO(i4temp, *i4ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_OCTET_LENGTH:

        if (ValuePtr)
        {
            switch (pxxd->ConciseType)
            {
                case SQL_LONGVARCHAR:
                case SQL_WLONGVARCHAR:
                case SQL_LONGVARBINARY:
#ifdef _WIN64
                    *lenValuePtr = FETMAX;
#else
                    i4temp = FETMAX;
                    I4ASSIGN_MACRO(i4temp, *i4ValuePtr);
#endif
                    break;

                default:
#ifdef _WIN64
                    *lenValuePtr  = pxxd->OctetLength;
#else
                    i4temp = pxxd->OctetLength;
                    I4ASSIGN_MACRO(i4temp, *i4ValuePtr);
#endif
                    break;
             }
        }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLINTEGER);
        break;

    case SQL_DESC_OCTET_LENGTH_PTR:

        if (ValuePtr)
            *pvalue = pxxd->OctetLengthPtr;
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLPOINTER);
        break;

    case SQL_DESC_PARAMETER_TYPE:

        if (ValuePtr)
            I2ASSIGN_MACRO(pxxd->ParameterType, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_PRECISION:

        if (ValuePtr)
        { 
            i2temp = (i2) pxxd->Precision;

            switch (pxxd->fIngApiType)
            {
                case IIAPI_TS_TYPE:
                case IIAPI_TSTZ_TYPE:
                case IIAPI_TSWO_TYPE:
                case IIAPI_TIME_TYPE:
                case IIAPI_TMTZ_TYPE:
                case IIAPI_TMWO_TYPE:
                case IIAPI_INTDS_TYPE:
                    i2temp = (i2) pxxd->IsoTimePrecision;
                    break;
            }
            I2ASSIGN_MACRO(i2temp, *i2ValuePtr);
        }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_ROWVER:

        if (ValuePtr)
           {i2temp = FALSE;
            I2ASSIGN_MACRO(i2temp, *i2ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_SCALE:

        if (ValuePtr)
            I2ASSIGN_MACRO(pxxd->Scale, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_SCHEMA_NAME:

        rc = GetCharAndLength(pdesc, "", /*pxxd->SchemaName */
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_SEARCHABLE:

        if (ValuePtr)
           {i2temp = GetDescSearchable(pxxd);
            I2ASSIGN_MACRO(i2temp, *i2ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_TABLE_NAME:

        rc = GetCharAndLength(pdesc, pxxd->TableName,
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_TYPE:

        if (ValuePtr)
            I2ASSIGN_MACRO(pxxd->VerboseType, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_TYPE_NAME:

        rc = GetCharAndLength(pdesc, GetDescTypeName(pxxd),
                 ValuePtr, BufferLength, StringLengthPtr);
        break;

    case SQL_DESC_UNNAMED:

        if (ValuePtr)
            I2ASSIGN_MACRO(pxxd->Unnamed, *i2ValuePtr);
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_UNSIGNED:

        if (ValuePtr)
           {i2temp = GetDescUnsigned(pxxd);
            I2ASSIGN_MACRO(i2temp, *i2ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    case SQL_DESC_UPDATABLE:

        if (ValuePtr)
           {i2temp = SQL_ATTR_READWRITE_UNKNOWN;
            I2ASSIGN_MACRO(i2temp, *i2ValuePtr);
           }
        if (StringLengthPtr)
           *StringLengthPtr = sizeof(SQLSMALLINT);
        break;

    default:                        /* not a DESC field */

        rc = ErrState(SQL_HY091,pdesc);
            /* unknown FieldIdentifier: Invalid descriptor field identifier */
        break;
    }   /* end switch(FieldIdentifier) */


    UnlockDesc(pdesc);
    return(rc);
}


/*
**  SQLGetDescRec
**
**  SQLGetDescRec returns the current settings or values of multiple fields
**                of a descriptor record. The fields returned describe the
**                name, data type, and storage of column or parameter data.
**
**  On entry:
**    DescriptorHandle [Input]
**    RecNumber [Input]
**    Name [Output]
**      A pointer to a buffer in which to return the SQL_DESC_NAME field 
**      for the descriptor record.
**    BufferLength [Input]
**    StringLengthPtr [Output]
**    TypePtr [Output]
**      A pointer to a buffer in which to return the value of the
**      SQL_DESC_TYPE field for the descriptor record.
**    SubTypePtr [Output]
**      For records whose type is SQL_DATETIME or SQL_INTERVAL,
**      this is a pointer to a buffer in which to return the value of the 
**      SQL_DESC_DATETIME_INTERVAL_CODE field.
**    LengthPtr [Output]
**      A pointer to a buffer in which to return the value of the 
**      SQL_DESC_OCTET_LENGTH field for the descriptor record.
**    PrecisionPtr [Output]
**      A pointer to a buffer in which to return the value of the 
**      SQL_DESC_PRECISION field for the descriptor record.
**    ScalePtr [Output]
**      A pointer to a buffer in which to return the value of the
**      SQL_DESC_SCALE field for the descriptor record.
**    NullablePtr [Output]
**      A pointer to a buffer in which to return the value of the
**      SQL_DESC_NULLABLE field for the descriptor record.
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
*/
SQLRETURN  SQL_API SQLGetDescRec (
    SQLHDESC     DescriptorHandle,
    SQLSMALLINT  RecNumber,
    SQLCHAR     *Name,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLSMALLINT *TypePtr,
    SQLSMALLINT *SubTypePtr,
    SQLINTEGER      *LengthPtr,
    SQLSMALLINT *PrecisionPtr,
    SQLSMALLINT *ScalePtr,
    SQLSMALLINT *NullablePtr)
{
    return SQLGetDescRec_InternalCall (
                 DescriptorHandle,
                 RecNumber,
                 Name,
                 BufferLength,
                 StringLengthPtr,
                 TypePtr,
                 SubTypePtr,
                 LengthPtr,
                 PrecisionPtr,
                 ScalePtr,
                 NullablePtr);
}

SQLRETURN  SQL_API SQLGetDescRec_InternalCall (
    SQLHDESC     DescriptorHandle,
    SQLSMALLINT  RecNumber,
    SQLCHAR     *Name,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLSMALLINT *TypePtr,
    SQLSMALLINT *SubTypePtr,
    SQLINTEGER      *LengthPtr,
    SQLSMALLINT *PrecisionPtr,
    SQLSMALLINT *ScalePtr,
    SQLSMALLINT *NullablePtr)
{
    LPDESC        pdesc = (LPDESC)DescriptorHandle;
    LPDESCFLD     pxxd;
    SQLRETURN     rc    = SQL_SUCCESS;
    SQLINTEGER    StringLength;


    if (!LockDesc (pdesc))
         return SQL_INVALID_HANDLE;

    ErrResetDesc (pdesc);           /* clear any errors on DESC */

    if (RecNumber > pdesc->Count)
       {
        UnlockDesc(pdesc);
        return(SQL_NO_DATA);
       }

    if ( RecNumber < 0  ||
        (RecNumber == 0  &&  isaIPD(pdesc)))
            return ErrUnlockDesc(SQL_07009, pdesc); /* Invalid descriptor index */

    pxxd = pdesc->pcol + RecNumber;

    /* SQL_DESC_NAME */
    rc = GetCharAndLength(pdesc, pxxd->Name,
             (CHAR*)Name, BufferLength, &StringLength);
    if ( StringLengthPtr)
        *StringLengthPtr = (SQLSMALLINT)StringLength;

    /* SQL_DESC_TYPE */
    if ( TypePtr)
        *TypePtr  = pxxd->VerboseType;

    /* SQL_DESC_DATETIME_INTERVAL_CODE */
    *SubTypePtr = pxxd->DatetimeIntervalCode;

    /* SQL_DESC_OCTET_LENGTH */
    if ( LengthPtr)
    { 
        switch (pxxd->ConciseType)
        {
        case SQL_TYPE_DATE:
        case SQL_TYPE_TIME:
            *LengthPtr = sizeof(SQL_DATE_STRUCT);
            break;

        case SQL_TYPE_TIMESTAMP:
            *LengthPtr = sizeof(SQL_TIMESTAMP_STRUCT);
            break;

        default:
            if (pxxd->VerboseType == SQL_INTERVAL)
                *LengthPtr = sizeof(SQL_INTERVAL_STRUCT);
            else
                *LengthPtr  = pxxd->OctetLength;
            break;
        }
    }

    /* SQL_DESC_PRECISION */
    if ( PrecisionPtr )
    {
        switch (pxxd->fIngApiType)
        {
        case IIAPI_TS_TYPE:
        case IIAPI_TSTZ_TYPE:
        case IIAPI_TSWO_TYPE:
        case IIAPI_TIME_TYPE:
        case IIAPI_TMTZ_TYPE:
        case IIAPI_TMWO_TYPE:
        case IIAPI_INTDS_TYPE:
            *PrecisionPtr  = (SQLSMALLINT)pxxd->IsoTimePrecision;
            break;
        
        default:
            *PrecisionPtr  = (SQLSMALLINT) pxxd->Precision;
            break;
        }
    }

    /* SQL_DESC_SCALE */
    if ( ScalePtr)
        *ScalePtr  = pxxd->Scale;

    /* SQL_DESC_NULLABLE */
    if ( NullablePtr)
        *NullablePtr  = isaIPD(pdesc)?TRUE:pxxd->Nullable;

    UnlockDesc(pdesc);
    return(rc);
}


/*
**  SQLSetDescField
**
**  SQLSetDescField sets the value of a single field of a single descriptor.
**  Can be called to set any field in any descriptor type, provided the
**  field can be set.
**
**  On entry:
**    DescriptorHandle [Input]
**    RecNumber [Input] numbered from 0, with 0 being the bookmark
**    FieldIdentifier [Input]
**      Indicates the field of the diagnostic whose value is to be set.
**    ValuePtr [Output]
**      Pointer to a buffer containing the descriptor information,
**      or a 4-byte value depending on the value of FieldIdentifier.
**    BufferLength [Input]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
**  Notes:
**    The record field becomes unbound if any field is set other than
**    SQL_DESC_COUNT, SQL_DESC_DATA_PTR, SQL_DESC_OCTET_LENGTH_PTR,
**    or SQL_DESC_INDICATOR_PTR.
**
*/
SQLRETURN  SQL_API SQLSetDescField (
    SQLHDESC    DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER  ValuePtr,
    SQLINTEGER  BufferLength)
{
    return SQLSetDescField_InternalCall (
                DescriptorHandle,
                RecNumber,
                FieldIdentifier,
                ValuePtr,
                BufferLength);
}

SQLRETURN  SQL_API SQLSetDescField_InternalCall (
    SQLHDESC    DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER  ValuePtr,
    SQLINTEGER  BufferLength)
{
    LPDESC        pdesc      = (LPDESC)DescriptorHandle;
    LPDESCFLD     pxxd;
    SQLRETURN     rc         = SQL_SUCCESS;
    SQLSMALLINT   i2Value    = (SQLSMALLINT) (SCALARP)ValuePtr;
    SQLINTEGER    i4Value    = (SQLINTEGER ) (SCALARP)ValuePtr;
    SQLUINTEGER   ui4Value   = (SQLUINTEGER) (SCALARP)ValuePtr;
    SQLINTEGER        lenValue   = (SQLINTEGER ) (SCALARP)ValuePtr;
    SQLUINTEGER       ulenValue  = (SQLUINTEGER) (SCALARP)ValuePtr;
    SQLUSMALLINT  ui2Value   = (SQLUSMALLINT)(SCALARP)ValuePtr;
    i2            Type;
    BOOL          foundit;
    SQLSMALLINT   OldCount;
    i2            PermissionNeeded;
    i2            i;
    LPSQLTYPEATTR ptype;
    LPDBC pdbc = pdesc->pdbc;
    UDWORD dateConnLevel = pdbc->fOptions & OPT_INGRESDATE ? 
        IIAPI_LEVEL_3 : pdbc->fAPILevel;

    if (!LockDesc (pdesc))
         return SQL_INVALID_HANDLE;

    ErrResetDesc (pdesc);           /* clear any errors on DESC */

    for (i=0; i<sizeofDescPermissionTable; i++)  /* find the RW permission */
        if (FieldIdentifier == DescPermissionTable[i].FieldIdentifier)
            break;

    if (i >= sizeofDescPermissionTable)  /* error if not in table */
        return(ErrUnlockDesc(SQL_HY091,pdesc));

    if      (isaIRD(pdesc))   PermissionNeeded = IRD_W;
    else if (isaIPD(pdesc))   PermissionNeeded = IPD_W;
    else                      PermissionNeeded = APD_W;  /* APD and ARD same */

    if (!(PermissionNeeded & DescPermissionTable[i].Permission))
        if (isaIRD(pdesc))
             return(ErrUnlockDesc(SQL_HY016,pdesc)); /* can't modify IRD */
        else return(ErrUnlockDesc(SQL_HY091,pdesc)); /* invalid field id */

    OldCount = pdesc->Count;        /* save the Count in case need to restore */

    foundit = TRUE;                 /* assume we find a DESC header field */

    switch(FieldIdentifier)         /* looking for DESC header fields */
    {
    case SQL_DESC_ALLOC_TYPE:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_ARRAY_SIZE:

        pdesc->ArraySize      = ulenValue;
        break;

    case SQL_DESC_ARRAY_STATUS_PTR:

        pdesc->ArrayStatusPtr = ValuePtr;
        break;

    case SQL_DESC_BIND_OFFSET_PTR:

        pdesc->BindOffsetPtr  = ValuePtr;
        break;

    case SQL_DESC_BIND_TYPE:

        pdesc->BindType       = i4Value;
        break;

    case SQL_DESC_COUNT:

        if (lenValue > pdesc->CountAllocated)  /* if need more col, re-allocate */
            if (ReallocDescriptor(pdesc, i2Value) == NULL)
               {
                RETCODE rc = ErrState (SQL_HY001, pdesc, F_OD0001_IDS_BIND_ARRAY);
                UnlockDesc (pdesc);
                return rc;
               }

/* ????? trigger unbind check if newCount <= OldCount ?????*/
        if (pdesc->Count < i2Value)
           {
            ResetDescriptorFields(pdesc, pdesc->Count+1, i2Value);
           }

        pdesc->Count          = (SQLSMALLINT)lenValue;
        break;

    case SQL_DESC_ROWS_PROCESSED_PTR:

        pdesc->RowsProcessedPtr  = ValuePtr;
        break;

    default:                        /* not a DESC header field */

        foundit = FALSE;               /* indicate we found nothing so far */
        break;
    }   /* end switch(FieldIdentifier) */

    if (foundit)             /* if we found a DESC header field then all done */
       {
        UnlockDesc(pdesc);   /* unlock DESC */
        return(rc);          /* return SQL_SUCCESS or SQL_ERROR */
       }


    if (RecNumber < 0  &&  (isaARD(pdesc) || isaAPD(pdesc)))
            return ErrUnlockDesc(SQL_07009, pdesc); /* Invalid descriptor index */

    if (RecNumber == 0  &&  isaIPD(pdesc))
            return ErrUnlockDesc(SQL_07009, pdesc); /* Invalid descriptor index */

    if (RecNumber > pdesc->CountAllocated)  /* if need more col, re-allocate */
        if (ReallocDescriptor(pdesc, RecNumber) == NULL)
           {
            RETCODE rc = ErrState (SQL_HY001, pdesc, F_OD0001_IDS_BIND_ARRAY);
            UnlockDesc (pdesc);
            return rc;
           }


    if (pdesc->Count < RecNumber)
       {
        ResetDescriptorFields(pdesc, pdesc->Count+1, RecNumber);
        pdesc->Count = RecNumber;  /* bump up count of fields with data */
       }

    pxxd = pdesc->pcol + RecNumber;


    switch(FieldIdentifier)         /* looking for DESC header fields */
    {
    case SQL_DESC_AUTO_UNIQUE_VALUE:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_BASE_COLUMN_NAME:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_BASE_TABLE_NAME:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_CASE_SENSITIVE:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_CATALOG_NAME:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_CONCISE_TYPE:

        pxxd->ConciseType      = i2Value;

        ptype = CvtGetSqlType(ui2Value, pdbc->fAPILevel, dateConnLevel);
        if (ptype == NULL || !ptype->cbSupported)
             return(ErrUnlockDesc(SQL_HY004,pdesc));  /* SQL type unsupported */

        if (ptype->fVerboseType == SQL_DATETIME || ptype->fVerboseType 
             == SQL_INTERVAL)
        {
             pxxd->VerboseType          = ptype->fVerboseType;
             pxxd->DatetimeIntervalCode = ptype->fSqlCode;
             pxxd->Precision            = 0;
        }
        else
        {
             pxxd->VerboseType = i2Value;  /* others, verbose = concise type */
             pxxd->DatetimeIntervalCode = 0;
        }
        break;

    case SQL_DESC_DATA_PTR:

        if (!isaIPD(pdesc))  /* don't muckup IPD */
            pxxd->DataPtr          = ValuePtr;

        if (isaAPD(pdesc))
            if  (pxxd->DataPtr != NULL)         /* if TargetValuePtr is not NULL */
                 pxxd->fStatus  |=  COL_BOUND;    /* indicate column is bound */

        if (isaARD(pdesc))
            if  (pxxd->DataPtr != NULL)         /* if TargetValuePtr is not NULL */
                 pxxd->fStatus  |=  COL_BOUND;    /* indicate column is bound */
            else pxxd->fStatus  &= ~COL_BOUND;    /* indicate column is unbound */

        rc = CheckDescField(pdesc, pxxd);
             /* check APD, ARD, or IPD for consistency */
        break;

    case SQL_DESC_DATETIME_INTERVAL_CODE:

        pxxd->DatetimeIntervalCode = i2Value;
        if      (i2Value==SQL_CODE_DATE  ||  i2Value==SQL_CODE_TIME)
             pxxd->Precision = 0;
        else if (i2Value==SQL_CODE_TIMESTAMP)
             pxxd->Precision = 6;
        break;

    case SQL_DESC_DATETIME_INTERVAL_PRECISION:

        pxxd->DatetimeIntervalPrecision = i4Value;
        break;

    case SQL_DESC_DISPLAY_SIZE:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_FIXED_PREC_SCALE:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_INDICATOR_PTR:

        pxxd->IndicatorPtr = ValuePtr;
        break;

    case SQL_DESC_LABEL:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_LENGTH:

        pxxd->Length      = ulenValue;
        break;

    case SQL_DESC_LITERAL_PREFIX:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_LITERAL_SUFFIX:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_LOCAL_TYPE_NAME:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_NAME:

        STlcopy (ValuePtr, pxxd->Name, sizeof(pxxd->Name)-1);
        break;

    case SQL_DESC_NULLABLE:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_NUM_PREC_RADIX:

/*      pxxd->NumPrecRadix     = i4Value;  ignore */
        break;

    case SQL_DESC_OCTET_LENGTH:

        pxxd->OctetLength      = lenValue;
        break;

    case SQL_DESC_OCTET_LENGTH_PTR:

        pxxd->OctetLengthPtr = ValuePtr;
        break;

    case SQL_DESC_PARAMETER_TYPE:

        pxxd->ParameterType      = i2Value;
        break;

    case SQL_DESC_PRECISION:

        pxxd->Precision          = i2Value;
        break;

    case SQL_DESC_ROWVER:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_SCALE:

        pxxd->Scale          = i2Value;
        break;

    case SQL_DESC_SCHEMA_NAME:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_SEARCHABLE:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_TABLE_NAME:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_TYPE:

        pxxd->VerboseType = Type   = i2Value;
        if      (Type==SQL_CHAR    ||  Type==SQL_VARCHAR)
           { pxxd->Length    =  1;
             pxxd->Precision =  0;
           }
        else if (Type==SQL_DECIMAL ||  Type==SQL_NUMERIC)
           { pxxd->Precision = 31;
             pxxd->Scale     =  0;
           }
        else if (Type==SQL_FLOAT   ||  Type==SQL_C_FLOAT)
           { pxxd->Precision = 53;
             pxxd->Scale     =  0;
           }

        if (Type!=SQL_DATETIME  &&  Type!=SQL_INTERVAL)
           {
            pxxd->ConciseType          = pxxd->VerboseType;
            pxxd->DatetimeIntervalCode = 0;
           }
        break;

    case SQL_DESC_TYPE_NAME:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_UNNAMED:

        if (isaIPD(pdesc)  &&  i2Value==SQL_UNNAMED)
            pxxd->Unnamed   = i2Value;
        else;
            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_UNSIGNED:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    case SQL_DESC_UPDATABLE:

            /* read-only: SQL_HY091 - Invalid descriptor field identifier */
        break;

    default:                        /* not a DESC field */

        rc = ErrState(SQL_HY091,pdesc);
            /* unknown FieldIdentifier: Invalid descriptor field identifier */
        break;
    }   /* end switch(FieldIdentifier) */

    if (rc == SQL_ERROR)
        pdesc->Count = OldCount;  /* restore count of fields with data */

    /* The record becomes unbound if the application sets any field other than
    **    SQL_DESC_DATA_PTR
    **    SQL_DESC_OCTET_LENGTH_PTR
    **    SQL_DESC_INDICATOR_PTR
    */
    if (FieldIdentifier != SQL_DESC_DATA_PTR  &&
        FieldIdentifier != SQL_DESC_OCTET_LENGTH_PTR  &&
        FieldIdentifier != SQL_DESC_INDICATOR_PTR)
           pxxd->fStatus &= ~COL_BOUND;

    UnlockDesc(pdesc);
    return(rc);
}


/*
**  SQLSetDescRec
**
**  SQLSetDescRec sets multiple descriptor fields that affect the data type
**                and buffer bound to a column or parameter data.
**
**  On entry:
**    DescriptorHandle [Input]
**    RecNumber        [Input]
**    Type             [Input]
**    SubType          [Input]
**    Length           [Input]
**    Precision        [Input]
**    Scale            [Input]
**    Data             [Deferred Input or Output]
**   *StringLength     [Deferred Input or Output]
**   *Indicator        [Deferred Input or Output]
**
**  Returns:
**    SQL_SUCCESS
**    SQL_SUCCESS_WITH_INFO
**    SQL_NO_DATA
**    SQL_INVALID_HANDLE
**    SQL_ERROR
**
**  Notes:
**    If an attempt is made to set SQL_DESC_DATA_PTR in an IPD, the field 
**    is not actually set but does trigger a consistency check of the
**    IPD fields.
**
*/
SQLRETURN  SQL_API SQLSetDescRec (
    SQLHDESC    DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT VerboseType,
    SQLSMALLINT SubType,
    SQLINTEGER      Length,
    SQLSMALLINT Precision,
    SQLSMALLINT Scale,
    SQLPOINTER  DataPtr,
    SQLINTEGER     *StringLengthPtr,
    SQLINTEGER     *IndicatorPtr)
{
    SQLRETURN   rc    = SQL_SUCCESS;
    LPDESC      pdesc = (LPDESC)DescriptorHandle;
    LPDESCFLD   pxxd;

    if (!LockDesc (pdesc))
         return SQL_INVALID_HANDLE;

    ErrResetDesc (pdesc);           /* clear any errors on DESC */

    if ( RecNumber < 0  ||
        (RecNumber == 0  &&  isaIPD(pdesc))  ||
        (RecNumber == 0  &&  isaAPD(pdesc)  && pdesc->AllocType == SQL_DESC_ALLOC_AUTO))
        return ErrUnlockDesc(SQL_07009, pdesc); /* Invalid descriptor index */

    if (isaIRD(pdesc))
        return(ErrUnlockDesc(SQL_HY016,pdesc)); /* can't modify an IRD */

    if (RecNumber > pdesc->CountAllocated)  /* if need more col, re-allocate */
        if (ReallocDescriptor(pdesc, RecNumber) == NULL)
           {
            RETCODE rc = ErrState (SQL_HY001, pdesc, F_OD0001_IDS_BIND_ARRAY);
            UnlockDesc (pdesc);
            return rc;
           }

    if (pdesc->Count < RecNumber)
       {
        ResetDescriptorFields(pdesc, pdesc->Count+1, RecNumber);
        pdesc->Count = RecNumber;  /* bump up count of fields with data */
       }

    pxxd = pdesc->pcol + RecNumber;

    pxxd->VerboseType   = VerboseType;        /* SQL_DESC_TYPE */
    pxxd->ConciseType   = VerboseType;        /* SQL_DESC_CONCISE_TYPE */

                                              /* SQL_DESC_DATETIME_INTERVAL_CODE */
    if (VerboseType == SQL_DATETIME)
       {pxxd->DatetimeIntervalCode = SubType;
        if      (SubType==SQL_CODE_DATE)
            pxxd->ConciseType = SQL_TYPE_DATE;
        else if (SubType==SQL_CODE_TIME)
            pxxd->ConciseType = SQL_TYPE_TIME;
        else if (SubType==SQL_CODE_TIMESTAMP)
            pxxd->ConciseType = SQL_TYPE_TIMESTAMP;
       }
    else if (VerboseType == SQL_INTERVAL)  /* Appendix D: Data Types */
       {pxxd->DatetimeIntervalCode = SubType;
        if      (SubType ==         SQL_CODE_MONTH)
            pxxd->ConciseType = SQL_INTERVAL_MONTH;
        else if (SubType ==         SQL_CODE_YEAR)
            pxxd->ConciseType = SQL_INTERVAL_YEAR;
        else if (SubType ==         SQL_CODE_YEAR_TO_MONTH)
            pxxd->ConciseType = SQL_INTERVAL_YEAR_TO_MONTH;
        else if (SubType ==         SQL_CODE_DAY)
            pxxd->ConciseType = SQL_INTERVAL_DAY;
        else if (SubType ==         SQL_CODE_HOUR)
            pxxd->ConciseType = SQL_INTERVAL_HOUR;
        else if (SubType ==         SQL_CODE_MINUTE)
            pxxd->ConciseType = SQL_INTERVAL_MINUTE;
        else if (SubType ==         SQL_CODE_SECOND)
            pxxd->ConciseType = SQL_INTERVAL_SECOND;
        else if (SubType ==         SQL_CODE_DAY_TO_HOUR)
            pxxd->ConciseType = SQL_INTERVAL_DAY_TO_HOUR;
        else if (SubType ==         SQL_CODE_DAY_TO_MINUTE)
            pxxd->ConciseType = SQL_INTERVAL_DAY_TO_MINUTE;
        else if (SubType ==         SQL_CODE_DAY_TO_SECOND)
            pxxd->ConciseType = SQL_INTERVAL_DAY_TO_SECOND;
        else if (SubType ==         SQL_CODE_HOUR_TO_MINUTE)
            pxxd->ConciseType = SQL_INTERVAL_HOUR_TO_MINUTE;
        else if (SubType ==         SQL_CODE_HOUR_TO_SECOND)
            pxxd->ConciseType = SQL_INTERVAL_HOUR_TO_SECOND;
        else if (SubType ==         SQL_CODE_MINUTE_TO_SECOND)
            pxxd->ConciseType = SQL_INTERVAL_MINUTE_TO_SECOND;
       }
    else
        pxxd->DatetimeIntervalCode = 0;

    pxxd->OctetLength   = Length;             /* SQL_DESC_OCTET_LENGTH */

    pxxd->Precision     = Precision;          /* SQL_DESC_PRECISION */

    pxxd->Scale         = Scale;              /* SQL_DESC_SCALE */

                                              /* SQL_DESC_DATA_PTR */
    if (!isaIPD(pdesc)  &&  !isaIRD(pdesc))   /* don't muckup IPD and IRD */
        pxxd->DataPtr   = DataPtr;

    if (isaAPD(pdesc))
        if (pxxd->DataPtr != NULL)         /* if TargetValuePtr is not NULL */
             pxxd->fStatus  |=  COL_BOUND;    /* indicate column is bound */

    if (isaARD(pdesc))
        if (pxxd->DataPtr != NULL)        /* if TargetValuePtr is not NULL */
             pxxd->fStatus  |=  COL_BOUND;    /* indicate column is bound */
        else pxxd->fStatus  &= ~COL_BOUND;    /* indicate column is unbound */

    pxxd->OctetLengthPtr= StringLengthPtr;    /* SQL_DESC_OCTET_LENGTH_PTR */

    pxxd->IndicatorPtr  = IndicatorPtr;       /* SQL_DESC_INDICATOR_PTR */

    /* last thing to do is consistency check */
    rc = CheckDescField(pdesc, pxxd);
         /* check APD, ARD, or IPD for consistency */
    if (rc==SQL_ERROR)
       {
        ResetDescriptorFields(pdesc, RecNumber, RecNumber);  /* clear it */
       }

    UnlockDesc(pdesc);
    return(rc);
}


/*
**  AllocDescriptor
**
**  Lock DBC and allocate a Descriptor Header and initial fields
**
**  On entry: pdbc  -->data base connection block.
**            count =  initial count of fields (columns);
**                     if 0, then default to a handful.
**
**  Returns:  pdesc-->descriptor block.
**            NULL if could not allocate descriptor
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  09/14/2000  Dave Thole          Created 
*/

RETCODE AllocDescriptor(
    LPDBC      pdbc,
    LPDESC    *OutputHandle)
{
    RETCODE    rc = SQL_SUCCESS;
    LPDESC     pdesc;

    if (!LockDbc ((LPDBC)pdbc)) 
         return SQL_INVALID_HANDLE;

    ErrResetDbc (pdbc);        /* clear any errors on DBC */

    pdesc = AllocDesc_InternalCall(pdbc, SQL_DESC_ALLOC_USER, 
                                  INITIAL_DESCFLD_ALLOCATION);
    if (pdesc == NULL)
        rc = ErrState (SQL_HY001, pdbc, F_OD0014_IDS_STATEMENT);

    if (OutputHandle)          /* safety check */
       *OutputHandle = pdesc;  /* return pDESCHDR or NULL */

    UnlockDbc (pdbc);
    return rc;
}

/*
**  AllocDesc_InternalCall
**
**  Allocate a Descriptor Header and initial fields without locking the DBC
**
**  On entry: pdbc  -->data base connection block.
**            count =  initial count of fields (columns);
**                     if 0, then default to a handful.
**
**  Returns:  pdesc-->descriptor block.
**            NULL if could not allocate descriptor
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  09/14/2000  Dave Thole          Created 
*/

LPDESC AllocDesc_InternalCall(LPDBC pdbc, SQLSMALLINT AllocType, SQLSMALLINT count)
{
    LPDESC    pdesc;
    LPDESC    pdescPrior;
    LPDESCFLD pxxd;
    i4        i;

    /*
    **  Allocate DESC:
    */
    pdesc = (LPDESC)MEreqmem(0, sizeof(DESC), TRUE, NULL);
    if (pdesc == NULL)
        return NULL;

    STcopy ("DESC*", pdesc->szEyeCatcher);

    if (count == 0)
        count =  5;

    pxxd = (LPDESCFLD)MEreqmem(0, (count+2)*sizeof(DESCFLD), TRUE, NULL);
            /* allocate the descriptor field plus an element for
               a bookmark and a safety overflow element */
    if (pxxd == NULL)  /* no memory for fields! */
       { MEfree((PTR)pdesc);
         return NULL;
       }

    pdesc->CountAllocated = count;  /* Allocated number of elements 
                                       (not counting bookmark and safety) */
    pdesc->pcol  = pxxd;            /* Descriptor header -> descriptor fields*/

#ifdef _DEBUG
    pdesc->col1 = pxxd+1;           /* convenient debug pointer to elements */
    pdesc->col2 = pxxd+2;
    pdesc->col3 = pxxd+3;
    pdesc->col4 = pxxd+4;
#endif

    MEcopy("SQLCA   ", 8, pdesc->sqlca.SQLCAID);

    for (i=0; i < (count+2); i++, pxxd++) /* initialize the descriptor fields */
        {
         pxxd->ConciseType = pxxd->VerboseType = SQL_DEFAULT;
        }

                                    /* finish initializing the header */
    pdesc->AllocType     = AllocType;
                            /* automatically allocated (SQL_DESC_ALLOC_AUTO) or
                             explicitly user allocated (SQL_DESC_ALLOC_USER) */
    pdesc->BindType      = SQL_BIND_BY_COLUMN;
    pdesc->ArraySize     = 1;       /* SQLFetch/SQLFetchScroll row array size */

    pdesc->pdbc          = pdbc;    /* set the owner DBC of the DESC */

    if (AllocType == SQL_DESC_ALLOC_USER)
       {                         /* if user allocated, link to DBC-DESC list */
        /*
        **  Link DESC into DBC-->DESCRIPTOR chain
        */ 
        if (pdbc->pdescFirst == NULL)
            pdbc->pdescFirst = pdesc;
        else
           {
            pdescPrior = pdbc->pdescFirst; /* find the last in list */
            while (pdescPrior->pdbcNext != NULL)
                   pdescPrior = pdescPrior->pdbcNext;
            pdescPrior->pdbcNext = pdesc; /* old last now -> new last */
           }
       }

    return pdesc;
}

/*
**  CheckDescField
**
**  A consistency check is performed automatically whenever an application
**  sets the SQL_DESC_DATA_PTR of an APD, ARD, or IPD.
**
**  On entry: pdesc-->descriptor block
**
**  Returns:  SQL_SUCCESS
**            SQL_ERROR and HY021 (Inconsistent descriptor information)
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  12/15/2000  Dave Thole          Created 
*/

RETCODE CheckDescField(LPDESC pdesc, LPDESCFLD  papd)
{
    if ((papd->DatetimeIntervalCode == SQL_CODE_DATE  ||
         papd->DatetimeIntervalCode == SQL_CODE_TIME  ||
         papd->DatetimeIntervalCode == SQL_CODE_TIMESTAMP)  &&
           papd->VerboseType != SQL_DATETIME)
         return(ErrState(SQL_HY021,pdesc));

    return SQL_SUCCESS;
}


/*
**  FindDescIRDField
**
**  Find the implementation row descriptor (IRD) field (DESCFLD) using index.
**  User desc fields are relative to 1; bookmark is at 0.
**
**  On entry: pstmt  -->STMT block.
**            icol    = column number.
**            ppird  -->where to return address of 
**                      implementation row descriptor (IRD) field (DESCFLD).
**
**  Returns:  SQL_SUCCESS           If column's SQLVAR found.
**            SQL_ERROR             If not.
*/
RETCODE FindDescIRDField(LPSTMT pstmt, UWORD icol, LPDESCFLD * ppird)
{
    LPDESC    pIRD = pstmt->pIRD;   /* Implementation Row Descriptor */

    TRACE_INTL(pstmt,"FindDescIRDField");
    if (pIRD->Count == 0)
        return ErrState (SQL_24000, pstmt);

    if (icol > pIRD->Count)
        return ErrState (SQL_07009, pstmt);  /* invalid descriptor index */

    *ppird = pIRD->pcol + icol;

    return (SQL_SUCCESS);
}

/*
**  GetDescBoundCount
**
**  For a given descriptor, return the
**  count of "bound" parameters or result columns.
**
*/
UWORD GetDescBoundCount(LPDESC pdesc)
{
    LPDESCFLD pxxd = pdesc->pcol;  /* ->1st descriptor field including bookmark */
    i2        Count = 0;
    i2        i;

    for (i=0;  i<=pdesc->Count; i++, pxxd++)
        {
         if (pxxd->fStatus & COL_BOUND)
             Count++;
        }

    return Count;
}


/*
**  GetDescCaseSensitive
**
**  For a given data type in the DESC field, return the
**  CaseSensitive attr.  TRUE for character based types.
**
*/
SQLSMALLINT GetDescCaseSensitive(LPDESCFLD pxxd)
{
    SQLSMALLINT CaseSensitive;

    switch (pxxd->ConciseType)
    {
    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
        CaseSensitive = TRUE;                break;

    default:
        CaseSensitive = FALSE;               break;
    }  /* end switch */

    return CaseSensitive;
}


/*
**  GetDescDisplaySize
**
**  For a given data type in the DESC field, return the
**  DisplaySize as defined in ODBC Reference Appendix D: Data Types.
**
*/
SQLINTEGER GetDescDisplaySize(LPDESCFLD pxxd)
{
    SQLINTEGER i = 0;

    switch (pxxd->ConciseType)
    {

    case SQL_LONGVARBINARY:
    case SQL_LONGVARCHAR:
# ifdef IIAPI_NCHA_TYPE
    case SQL_WLONGVARCHAR:
# endif /* IIAPI_NCHA_TYPE */
        i = MAXI4;              break;

    case SQL_CHAR:
    case SQL_VARCHAR:
# ifdef IIAPI_NCHA_TYPE
    case SQL_WCHAR:   
    case SQL_WVARCHAR:   
# endif /* IIAPI_NCHA_TYPE */
        i = pxxd->Precision;       break;

    case SQL_DECIMAL:
    case SQL_NUMERIC:
        i = pxxd->Precision+2;  break;

    case SQL_BIT:
        i = 1;                  break;

    case SQL_TINYINT:
        i = 4;                  break;

    case SQL_SMALLINT:
        i = 6;                  break;

    case SQL_INTEGER:
        i = 11;                 break;

    case SQL_BIGINT:
        i = 20;                 break;

    case SQL_REAL:
        i = 14;                 break;

    case SQL_FLOAT:
    case SQL_DOUBLE:
        i = 24;                 break;

    case SQL_VARBINARY:
    case SQL_BINARY:
        i = pxxd->Length*2;     break;

    case SQL_GUID:
        i = 36;                 break;

    case SQL_TYPE_DATE:
        i = ODBC_DATE_OUTLENGTH; break;

    case SQL_TYPE_TIME:
        i =  ODBC_TMTZ_OUTLENGTH;  break;

    case SQL_TYPE_TIMESTAMP:
        i = ODBC_TSTZ_OUTLENGTH;   break;

    case SQL_INTERVAL_YEAR: 
    case SQL_INTERVAL_MONTH: 
    case SQL_INTERVAL_YEAR_TO_MONTH: 
        i = ODBC_INTYM_OUTLENGTH; break;

    case SQL_INTERVAL_DAY:
    case SQL_INTERVAL_HOUR:
    case SQL_INTERVAL_MINUTE:
    case SQL_INTERVAL_SECOND:
    case SQL_INTERVAL_DAY_TO_MINUTE:
    case SQL_INTERVAL_DAY_TO_HOUR:
    case SQL_INTERVAL_DAY_TO_SECOND:
    case SQL_INTERVAL_HOUR_TO_MINUTE:
    case SQL_INTERVAL_HOUR_TO_SECOND:
    case SQL_INTERVAL_MINUTE_TO_SECOND:
        i = ODBC_INTDS_OUTLENGTH; break;

    default:
        i = SQL_NO_TOTAL;       break;
    }  /* end switch */

    return i;
}


/*
**  GetDescLiteralPrefix
**
**  For a given data type in the DESC field, return the
**  LiteralPrefix for a literal of this type.
**
*/
char * GetDescLiteralPrefix(LPDESCFLD pxxd)
{

    switch (pxxd->VerboseType)
    {
    case SQL_LONGVARBINARY:
    case SQL_VARBINARY:
    case SQL_BINARY:
        return(LiteralPrefixSuffix+0);        /*  "X'"  */

    case SQL_LONGVARCHAR:
    case SQL_CHAR:
    case SQL_DATETIME:
    case SQL_TYPE_TIMESTAMP:
    case SQL_INTERVAL_YEAR: 
    case SQL_INTERVAL_MONTH: 
    case SQL_INTERVAL_YEAR_TO_MONTH: 
    case SQL_INTERVAL_DAY:
    case SQL_INTERVAL_HOUR:
    case SQL_INTERVAL_MINUTE:
    case SQL_INTERVAL_SECOND:
    case SQL_INTERVAL_DAY_TO_MINUTE:
    case SQL_INTERVAL_DAY_TO_HOUR:
    case SQL_INTERVAL_DAY_TO_SECOND:
    case SQL_INTERVAL_HOUR_TO_MINUTE:
    case SQL_INTERVAL_HOUR_TO_SECOND:
    case SQL_INTERVAL_MINUTE_TO_SECOND:
    case SQL_GUID:
    case SQL_VARCHAR:
        return(LiteralPrefixSuffix+1);        /*  "'"  */

    default:
        break;
    }

        return(LiteralPrefixSuffix+2);        /*  ""  */
}


/*
**  GetDescLiteralSuffix
**
**  For a given data type in the DESC field, return the
**  LiteralSuffix for a literal of this type.
**
*/
char * GetDescLiteralSuffix(LPDESCFLD pxxd)
{

    switch (pxxd->VerboseType)
    {
    case SQL_LONGVARBINARY:
    case SQL_VARBINARY:
    case SQL_BINARY:
    case SQL_LONGVARCHAR:
    case SQL_CHAR:
    case SQL_DATETIME:
    case SQL_TYPE_TIMESTAMP:
    case SQL_INTERVAL_YEAR: 
    case SQL_INTERVAL_MONTH: 
    case SQL_INTERVAL_YEAR_TO_MONTH: 
    case SQL_INTERVAL_DAY:
    case SQL_INTERVAL_HOUR:
    case SQL_INTERVAL_MINUTE:
    case SQL_INTERVAL_SECOND:
    case SQL_INTERVAL_DAY_TO_MINUTE:
    case SQL_INTERVAL_DAY_TO_HOUR:
    case SQL_INTERVAL_DAY_TO_SECOND:
    case SQL_INTERVAL_HOUR_TO_MINUTE:
    case SQL_INTERVAL_HOUR_TO_SECOND:
    case SQL_INTERVAL_MINUTE_TO_SECOND:
    case SQL_GUID:
    case SQL_VARCHAR:
        return(LiteralPrefixSuffix+1);        /*  "'"  */

    default:
        break;
    }

    return(LiteralPrefixSuffix+2);            /*  ""  */
}


/*
**  GetDescSearchable
**
**  For a given data type in the DESC field, return the
**  Unsigned attr.  SQL_PRED_NONE for long varchar and long byte.
**                  SQL_PRED_SEARCHABLE for varchar and char.
**                  SQL_PRED_BASIC for all others.
**
*/
SQLSMALLINT GetDescSearchable(LPDESCFLD pxxd)
{
    SQLSMALLINT Searchable;

    switch (pxxd->ConciseType)
    {

    case SQL_LONGVARBINARY:
    case SQL_LONGVARCHAR:
        Searchable = SQL_PRED_NONE;       break;

    case SQL_CHAR:
    case SQL_VARCHAR:
        Searchable = SQL_PRED_SEARCHABLE; break;

    default:
        Searchable = SQL_PRED_BASIC;      break;
    }  /* end switch */

    return Searchable;
}


/*
**  GetDescRadix
**
**  For a given data type in the DESC field, return the
**  NumPrecRadix attr.
**
*/
SQLSMALLINT GetDescRadix(LPDESCFLD pxxd)
{
    SQLSMALLINT radix;

    switch (pxxd->ConciseType)
    {
    case SQL_FLOAT:
    case SQL_REAL:
    case SQL_DOUBLE:
        radix = 2;              break;

    case SQL_TINYINT:
    case SQL_DECIMAL:
    case SQL_NUMERIC:
    case SQL_SMALLINT:
    case SQL_INTEGER:
    case SQL_BIGINT:
        radix = 10;             break;

    default:
        radix = 0;              break;
    }  /* end switch */

    return radix;
}


/*
**  GetDescTypeName
**
**  For a given data type in the DESC field, return the
**  TypeName attr.
**
*/
char* GetDescTypeName(LPDESCFLD pxxd)
{
    CHAR * pName;

	switch (pxxd->fIngApiType)
	{
	case IIAPI_INT_TYPE:
		 if (pxxd->OctetLength == 2)
		     pName = "SMALLINT"; 
		 else
		 if (pxxd->OctetLength == 1)
			 pName = "INTEGER1";
		 else
			 pName = "INTEGER";
		 break;
	
    case IIAPI_CHA_TYPE:
	     pName = "CHAR"; 
		 break;

	case IIAPI_CHR_TYPE:
/*		 pName = "C";    */   /* use CHAR since C is not in SQLGetTypeInfo */
	     pName = "CHAR"; 
	     break;

	case IIAPI_VCH_TYPE:
		 pName = "VARCHAR"; 
	     break;

	case IIAPI_TXT_TYPE:
/*		 pName = "TEXT"; */   /* use VARCHAR as defined in SQLGetTypeInfo */
		 pName = "VARCHAR"; 
		 break;
	
	case IIAPI_DTE_TYPE:
		 pName = "DATE";
		 break;
	
	case IIAPI_DEC_TYPE:	    
		 pName = "DECIMAL";
		 break;

    case IIAPI_FLT_TYPE:
		 if (pxxd->OctetLength <= 4)
			pName = "REAL";   /* use REAL as defined in SQLGetTypeInfo */
		 else
			pName = "FLOAT";
		 break;

	case IIAPI_BYTE_TYPE:
		 pName = "BYTE";  
		 break;

	case IIAPI_VBYTE_TYPE:
		 pName = "BYTE VARYING"; 
		 break;

	case IIAPI_LVCH_TYPE:
		 pName = "LONG VARCHAR";
		 break;

    case IIAPI_LTXT_TYPE:		
/*	     pName = "LONG TEXT"; */ /* use VARCHAR as defined in SQLGetTypeInfo */
/*		 pName = "LONG VARCHAR"; */   /* LONG TEXT is treated as VARCHAR, not LONG*/
		 pName = "VARCHAR"; 
		 break;
	
	case IIAPI_LBYTE_TYPE:
		 pName = "LONG BYTE";  
		 break;

   	case IIAPI_MNY_TYPE:
		 pName = "MONEY"; 
		 break;

	case IIAPI_NCHA_TYPE:
		 pName = "NCHAR";  
		 break;

	case IIAPI_NVCH_TYPE:
		 pName = "NVARCHAR"; 
		 break;

	case IIAPI_LNVCH_TYPE:
		 pName = "LONG NVARCHAR";
		 break;

        case IIAPI_BOOL_TYPE:
                 pName = "BOOLEAN";
                 break;

	case IIAPI_LOGKEY_TYPE:
/*		 pName = "OBJECT_KEY"; */
	     pName = "CHAR";      /* use CHAR as defined in SQLGetTypeInfo */
		 break;

	case IIAPI_TABKEY_TYPE:
/*		 pName = "TABLE_KEY";  */
	     pName = "CHAR";      /* use CHAR as defined in SQLGetTypeInfo */
		 break;
	
        case IIAPI_DATE_TYPE:
             pName = "INGRESDATE";
             break;

        case IIAPI_TS_TYPE:
             pName = "TIMESTAMP WITH LOCAL TIME ZONE";
             break;

        case IIAPI_TSWO_TYPE:
             pName = "TIMESTAMP WITHOUT TIME ZONE";
             break;

        case IIAPI_TSTZ_TYPE:
             pName = "TIMESTAMP WITH TIME ZONE";
             break;

        case IIAPI_TIME_TYPE:
             pName = "TIME WITH LOCAL TIME ZONE"; 
             break;

        case IIAPI_TMTZ_TYPE:
             pName = "TIME WITH TIME ZONE";
             break;

        case IIAPI_TMWO_TYPE:
             pName = "TIME WITH TIME ZONE";
             break;

        case IIAPI_INTDS_TYPE:
             pName = "INTERVAL DAY TO SECOND";
             break;

        case IIAPI_INTYM_TYPE:
             pName = "INTERVAL YEAR TO MONTH";
             break;
             
	default:
		return(NULL); 
	}
    return (pName);
}


/*
**  GetDescUnsigned
**
**  For a given data type in the DESC field, return the
**  Unsigned attr.  TRUE if column type is unsigned or non-numeric.
**
*/
SQLSMALLINT GetDescUnsigned(LPDESCFLD pxxd)
{
    SQLSMALLINT flag;

    switch (pxxd->ConciseType)
    {
    case SQL_TINYINT:
    case SQL_DECIMAL:
    case SQL_SMALLINT:
    case SQL_INTEGER:
    case SQL_BIGINT:
    case SQL_FLOAT:
    case SQL_REAL:
    case SQL_DOUBLE:
        flag = FALSE;              break;

    default:
        flag = TRUE;               break;
    }  /* end switch */

    return flag;
}


/*
**  FreeDescriptor
**
**  Free and deallocate a Descriptor
**
**  Free explicityly allocated descriptor and all statement handles
**  to which the free descriptor applied automatically revert to the 
**  descriptors implicitly allocated for them.    ???? need to add ????
**
**  On entry: pdesc-->descriptor block
**
**  Returns:  SQL_SUCCESS always
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  09/14/2000  Dave Thole          Created 
**  20-nov-2993 (loera01)
**      If a column descriptor still has a DataPtr allocated,
**      free it.
*/

RETCODE FreeDescriptor(LPDESC pdesc)
{
    LPDESC    priordesc;
    LPDBC     pdbc = pdesc->pdbc;
    LPDESCFLD pxxd = pdesc->pcol + 1;  /* pxxd -> 1st desc fld after bookmark */
    i2        i;

    for (i=0; i<pdesc->Count; i++,pxxd++)  /* walk all the literal desc field */
        if (pxxd->DataPtr  &&  (pxxd->fStatus & COL_DATAPTR_ALLOCED))
           {MEfree((PTR)pxxd->DataPtr);   /* free the work buffer */
            pxxd->DataPtr = NULL;
            pxxd->fStatus &= ~COL_DATAPTR_ALLOCED;
           }

    if (pdesc->AllocType == SQL_DESC_ALLOC_USER)  /* only explicitly
           allocated descriptors are in the DBC-DESC list */
       {
       /* locate descriptor in DBC-DESC list so we can delink it from list */
       if (pdbc->pdescFirst == pdesc)         /* if pdesc is first in list */
        pdbc->pdescFirst = pdesc->pdbcNext;             /* delink pdesc */
       else
          {             /* search list for prior of pdesc */
           priordesc = pdbc->pdescFirst;
           while (priordesc)
              { if (priordesc->pdbcNext == pdesc)
                   {priordesc->pdbcNext =  pdesc->pdbcNext;  /* delink pdesc */
                    break;
                   }
                priordesc = pdesc->pdbcNext;  /* try next in DBC-DESC list*/
              }
          }   /* end search of list for prior of pdesc */
       }  /* endif (pdesc->AllocType == SQL_DESC_ALLOC_USER) */

    MEfree((PTR)(pdesc->pcol));   /* free the descriptor field array */
    MEfree((PTR)pdesc);           /* free the descriptor header */

    return SQL_SUCCESS;
}


/*
**  ReallocDescriptor
**
**  Re-allocate a Descriptor to increase the number of fields
**
**  On entry: pdesc-->descriptor block
**            newcount = new count of user fields (columns);
**
**  Returns:  pdesc-->descriptor block.
**            NULL if could not re-allocate descriptor
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  09/14/2000  Dave Thole          Created 
*/

LPDESC ReallocDescriptor(LPDESC pdesc, i2 newcount)
{
    LPDESCFLD  pold;
    LPDESCFLD  pnew;

    newcount += SECONDARY_DESCFLD_ALLOCATION;  /* add extra to avoid Realloc */

    pold = pdesc->pcol;    /* pold -> old descriptor field array */
    pnew =                 /* pnew -> new descriptor field array */
          (LPDESCFLD)MEreqmem(0, (newcount+2)*sizeof(DESCFLD), TRUE, NULL);
                           /* include space for bookmark element and a safety*/
    if (pnew == NULL)
       return NULL;        /* no memory! */

    MEcopy((PTR)pold, (size_t)(pdesc->Count+1)*sizeof(DESCFLD), (PTR)pnew);
                           /* copy bookmark and user fields */

    pdesc->pcol = pnew;    /* Descriptor header -> new fields */
    pdesc->CountAllocated = newcount; /* set new count of allocated DESCFLDs
                                         not counting bookmark and a safety */

    MEfree((PTR)pold);            /* free the old descriptor field array */

    ResetDescriptorFields(pdesc, pdesc->Count+1, newcount);
                                    /* initialize the new descriptor fields */

#ifdef _DEBUG
    pdesc->col1 = pdesc->pcol+1;  /* convenient debug pointers to elements */
    pdesc->col2 = pdesc->pcol+2;
    pdesc->col3 = pdesc->pcol+3;
    pdesc->col4 = pdesc->pcol+4;
#endif

    return pdesc;
}


/*
**  ResetDescriptorFields
**
**  Reset and clear descriptor field fields from startcount to endcount inclusive.
**  Just zero them out and reset back to defaults.
**
**  On entry: pdesc-->descriptor block
**            startindex = starting index of fields (1 is first user field)
**            endindex   = ending index of fields
**
**  Returns:  nothing
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  12/18/2000  Dave Thole          Created 
*/

void ResetDescriptorFields(LPDESC pdesc, i4 startindex, i4 endindex)
{
    LPDESCFLD  pxxd;
    i4         i;

    if (startindex > endindex)
       return;

    if (endindex > pdesc->CountAllocated)    /* safety check */
        endindex = pdesc->CountAllocated;    /*  to avoid array overrun */

    pxxd = pdesc->pcol + startindex;         /* pxxd->descriptor field */

    for (i=startindex; i<=endindex; i++,pxxd++)   /* clear it out inclusively */
        {
         if (pxxd->DataPtr && (pxxd->fStatus & COL_DATAPTR_ALLOCED))
             MEfree(pxxd->DataPtr);
         MEfill(sizeof(DESCFLD), 0, pxxd);   /* zero everything */
         pxxd->ConciseType = pxxd->VerboseType = SQL_DEFAULT;
                                             /* re-init defaults */
        }

    return;
}


/*
**  SetDescDefaultsFromType
**
**  Set precision, scale, nullability defaults based on Ingres type and length.
**
**  On entry: pdbc-->DBC block to get the flags or NULL
**            pird-->implementation row descriptor
**            endindex   = ending index of fields
**
**  Returns:  nothing
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**  12/18/2000  Dave Thole          Created 
*/

void SetDescDefaultsFromType(LPDBC pdbc, LPDESCFLD pird)
{
    pird->DatetimeIntervalCode = 0;

    if (pird->fIngApiType != IIAPI_DEC_TYPE  &&
        pird->fIngApiType != IIAPI_MNY_TYPE)
        pird->Scale       = 0;

    pird->IsoTimePrecision = 0;
    
    switch(pird->fIngApiType)
    {
    case IIAPI_CHA_TYPE:
    case IIAPI_CHR_TYPE:
        pird->ConciseType = SQL_CHAR;
        pird->Precision = pird->Length = pird->OctetLength;
        pird->VerboseType = pird->ConciseType;    
        break;

    case IIAPI_VCH_TYPE:
    case IIAPI_TXT_TYPE:
    case IIAPI_LTXT_TYPE:
        pird->ConciseType = SQL_VARCHAR;
        pird->Precision   = pird->Length = pird->OctetLength - 2;
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_INT_TYPE:

        if (pird->OctetLength==4)
        { 
            pird->ConciseType      = SQL_INTEGER;
            pird->Length = pird->Precision = 10;
        }
        else if (pird->OctetLength==2)
        { 
            pird->ConciseType      = SQL_SMALLINT;
            pird->Length = pird->Precision = 5;
        }
        else if (pird->OctetLength==1)
        { 
            pird->ConciseType      = SQL_TINYINT;
            pird->Length = pird->Precision = 3;
        }
        else if (pird->OctetLength==8)
        { 
            pird->ConciseType      = SQL_BIGINT;
            pird->Length = pird->Precision = 19;
        }
        else /* should never happen */
        { 
            pird->ConciseType      = SQL_INTEGER;
            pird->Length = pird->Precision = 10;
        }
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_DEC_TYPE:
        pird->ConciseType = SQL_DECIMAL;
        pird->VerboseType = pird->ConciseType;    
        pird->Length = pird->Precision;
    
        break;

    case IIAPI_MNY_TYPE:
        pird->ConciseType = SQL_DECIMAL; 
        pird->Precision = pird->Length  = 14;  /* +-999,999,999,999.99 */
        pird->Scale       = 2;
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_FLT_TYPE:
        if (pird->OctetLength <= 4)
        {
            pird->ConciseType      = SQL_REAL;
            pird->Length = pird->Precision =  7;
        }
        else 
        {
            pird->ConciseType = SQL_FLOAT;
            pird->Length = pird->Precision = 15;
        }
        pird->VerboseType = pird->ConciseType;    
        
        break;

    case IIAPI_BOOL_TYPE:
        pird->ConciseType = SQL_BIT;
        pird->Precision = pird->Length = pird->OctetLength = 1;
        pird->VerboseType = pird->ConciseType;
        break;

    case IIAPI_BYTE_TYPE:
        pird->ConciseType = SQL_BINARY; 
        pird->Precision = pird->Length = pird->OctetLength;
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_VBYTE_TYPE:
        pird->ConciseType = SQL_VARBINARY;
        pird->Precision = pird->Length = pird->OctetLength - 2;
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_LVCH_TYPE:
        pird->ConciseType = SQL_LONGVARCHAR;
        pird->Precision  = pird->Length = MAXI4;
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_LBYTE_TYPE:
        pird->ConciseType = SQL_LONGVARBINARY; 
        pird->Precision   = pird->Length = MAXI4;
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_NCHA_TYPE:
        pird->ConciseType = SQL_WCHAR;
        pird->Precision   = pird->Length = 
            pird->OctetLength / sizeof(SQLWCHAR);
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_NVCH_TYPE:
        pird->ConciseType = SQL_WVARCHAR;
        pird->Precision   = pird->Length = (pird->OctetLength 
            / sizeof(SQLWCHAR)) - sizeof(i2);
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_LNVCH_TYPE:
        pird->ConciseType = SQL_WLONGVARCHAR;
        pird->Precision   = pird->Length = MAXI4/2;
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_LOGKEY_TYPE:
    case IIAPI_TABKEY_TYPE:
        pird->ConciseType = SQL_BINARY;
        pird->Precision = pird->Length = pird->OctetLength;
        pird->VerboseType = pird->ConciseType;    

        break;

    case IIAPI_HNDL_TYPE:
        pird->ConciseType = SQL_INTEGER;
        pird->Precision = pird->Length = 10;
        pird->VerboseType = pird->ConciseType;  
    
        break;

    case IIAPI_DATE_TYPE:
        pird->ConciseType = SQL_TYPE_DATE;
        pird->VerboseType = SQL_DATETIME;
        pird->DatetimeIntervalCode = SQL_CODE_DATE;
        pird->Precision = pird->OctetLength =
             pird->Length = ODBC_DATE_OUTLENGTH;

        if (pdbc == NULL)
            break;

        if (pdbc->fOptions & OPT_BLANKDATEisNULL ||  /* if return blank date */
            pdbc->fOptions & OPT_DATE1582isNULL)     /* or return 1582 */
           pird->Nullable = SQL_NULLABLE;    /* as NULL, then set descriptor */

        break;

    case IIAPI_DTE_TYPE:
        pird->ConciseType = SQL_TYPE_TIMESTAMP;
        pird->VerboseType = SQL_DATETIME;
        pird->DatetimeIntervalCode = SQL_CODE_TIMESTAMP;
        pird->IsoTimePrecision = 0;
        pird->Precision   = 19;
        pird->OctetLength =
             pird->Length      = ODBC_DTE_OUTLENGTH;

        if (pdbc == NULL)
            break;

        if (pdbc->fOptions & OPT_BLANKDATEisNULL ||  /* if return blank date */
            pdbc->fOptions & OPT_DATE1582isNULL)     /* or return 1582 */
           pird->Nullable = SQL_NULLABLE;    /* as NULL, then set descriptor */

        break;

    case IIAPI_TS_TYPE:
    case IIAPI_TSWO_TYPE:
    case IIAPI_TSTZ_TYPE:
        /*
        ** Note that the fetched precision is the number of digits to
        ** right of the decimal point.  This is the ISO standard definition
        ** for ISO date/time types.  For decimal types, precision is
        ** the number of significant digits.  
        ** The precision for ODBC gets overridden after IsoTimePrecision is
        ** filled, since precision takes on a different meaning in ODBC
        ** 2.0.  In ODBC 2.0, precision for date/time is the number of
        ** significant digits.       
        */
        pird->IsoTimePrecision = pird->Scale = (i2) pird->Precision;
        pird->Precision = pird->Length = 20 + pird->IsoTimePrecision;
        pird->ConciseType = SQL_TYPE_TIMESTAMP;
        pird->VerboseType = SQL_DATETIME;
        pird->DatetimeIntervalCode = SQL_CODE_TIMESTAMP;

        if (pdbc == NULL)
            break;

        if (pdbc->fOptions & OPT_BLANKDATEisNULL ||  /* if return blank date */
            pdbc->fOptions & OPT_DATE1582isNULL)     /* or return 1582 */
           pird->Nullable = SQL_NULLABLE;    /* as NULL, then set descriptor */

        break;

    case IIAPI_TMWO_TYPE:
    case IIAPI_TIME_TYPE:
    case IIAPI_TMTZ_TYPE:
        pird->IsoTimePrecision = pird->Scale = (i2) pird->Precision;
        pird->Precision = pird->Length = 9 + pird->IsoTimePrecision;
        pird->ConciseType = SQL_TYPE_TIME;
        pird->VerboseType = SQL_DATETIME;
        pird->DatetimeIntervalCode = SQL_CODE_TIME;

        if (pdbc == NULL)
            break;

        if (pdbc->fOptions & OPT_BLANKDATEisNULL ||  /* if return blank date */
            pdbc->fOptions & OPT_DATE1582isNULL)     /* or return 1582 */
           pird->Nullable = SQL_NULLABLE;    /* as NULL, then set descriptor */

        break;
    
    case IIAPI_INTYM_TYPE:
        pird->ConciseType = SQL_INTERVAL_YEAR_TO_MONTH;
        pird->VerboseType = SQL_INTERVAL;
        pird->DatetimeIntervalCode = SQL_CODE_YEAR_TO_MONTH;
        pird->Precision   = 
        pird->OctetLength =
        pird->Length      = ODBC_INTYM_OUTLENGTH; 
        break;
    
    case IIAPI_INTDS_TYPE:
        pird->ConciseType = SQL_INTERVAL_DAY_TO_SECOND;
        pird->VerboseType = SQL_INTERVAL;
        pird->DatetimeIntervalCode = SQL_CODE_DAY_TO_SECOND;
        pird->IsoTimePrecision = pird->Scale = pird->Precision;
        pird->Precision = pird->Length = 10 + pird->IsoTimePrecision;
        pird->OctetLength = ODBC_INTDS_OUTLENGTH; 
    
        break;
    
    default:
        if (pird->Precision > 0)
            pird->Length = pird->Precision;
        break;

    }  /* end switch(pird->fIngApiType) */
}
