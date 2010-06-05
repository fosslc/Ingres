/*
** Copyright (c) 2006 Ingres Corporation
*/

# if defined(dgi_us5)
#include <compat.h>
#include <stdlib.h>
# else
#include <stdlib.h>
#include <compat.h>
# endif /* dgi_us5 */

#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <me.h>
#include <st.h>
#include <er.h>
#include <erodbc.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */
#include "idmsocvt.h"               /* conversion functions */

#ifdef ODBC_UNICODE_SUPPORT_IS_DISABLED
   /* Merant 3.11 calls the SQLxxxxW with ASCII strings!
      Disable the SQLxxxxW entry points.  */
#define SQLColAttributeW       IIODXXXColAttributeW
#define SQLColAttributesW      IIODXXXColAttributesW
#define SQLConnectW            IIODXXXConnectW
#define SQLDescribeColW        IIODXXXDescribeColW
#define SQLErrorW              IIODXXXErrorW
#define SQLExecDirectW         IIODXXXExecDirectW
#define SQLGetConnectAttrW     IIODXXXGetConnectAttrW
#define SQLGetCursorNameW      IIODXXXGetCursorNameW
#define SQLSetDescFieldW       IIODXXXSetDescFieldW
#define SQLGetDescFieldW       IIODXXXGetDescFieldW
#define SQLGetDescRecW         IIODXXXGetDescRecW
#define SQLGetDiagFieldW       IIODXXXGetDiagFieldW
#define SQLGetDiagRecW         IIODXXXGetDiagRecW
#define SQLPrepareW            IIODXXXPrepareW
#define SQLSetConnectAttrW     IIODXXXSetConnectAttrW
#define SQLSetCursorNameW      IIODXXXSetCursorNameW
#define SQLColumnsW            IIODXXXColumnsW
#define SQLGetConnectOptionW   IIODXXXGetConnectOptionW
#define SQLGetInfoW            IIODXXXGetInfoW
#define SQLGetTypeInfoW        IIODXXXGetTypeInfoW
#define SQLSetConnectOptionW   IIODXXXSetConnectOptionW
#define SQLSpecialColumnsW     IIODXXXSpecialColumnsW
#define SQLStatisticsW         IIODXXXStatisticsW
#define SQLTablesW             IIODXXXTablesW
#define SQLDataSourcesW        IIODXXXDataSourcesW
#define SQLDriverConnectW      IIODXXXDriverConnectW
#define SQLBrowseConnectW      IIODXXXBrowseConnectW
#define SQLColumnPrivilegesW   IIODXXXColumnPrivilegesW
#define SQLGetStmtAttrW        IIODXXXGetStmtAttrW
#define SQLSetStmtAttrW        IIODXXXSetStmtAttrW
#define SQLForeignKeysW        IIODXXXForeignKeysW
#define SQLNativeSqlW          IIODXXXNativeSqlW
#define SQLPrimaryKeysW        IIODXXXPrimaryKeysW
#define SQLProcedureColumnsW   IIODXXXProcedureColumnsW
#define SQLProceduresW         IIODXXXProceduresW
#define SQLTablePrivilegesW    IIODXXXTablePrivilegesW
#define SQLDriversW            IIODXXXDriversW
#endif /* ODBC_UNICODE_SUPPORT_IS_DISABLED */

#define CVT_UNICODE_WORKBUFFER_SIZE   1000

/*
** Name: UNICODE.C
**
** Description:
**	Wide (Unicode) character routines for ODBC driver.
**
** History:
**	14-feb-2001 (thoda04)
**	    Initial coding
**	19-jun-2001 (thoda04)
**	    return null terminator at end of SQLSTATE
**	03-jul-2001 (loera01)
**	    Include stdlib.h.
**	13-jul-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	18-jul-2001 (thoda04)
**	    Assume that ConvertUCS2ToUCS4 UCS4 parm is not aligned
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**	09-May-2002 (xu$we01)
**	    Fixed the compilation warnings by moving the included header
**	    file <me.h> under <compat.h> and <stdlib.h> under <me.h>. 	     
**	29-may-2002 (somsa01)
**	    Corrected failed compilation due to last change.
**      01-may-2002 (wanfr01) Moved stdlib.h after compat.h to avoid 
**          dgi_us5 compilation issues.  (bug 107793)
**      26-Aug-2002 (hweho01)
**          Cleaned up compiler warnings for rs4_us5 platform. 
**	12-Sep-2002 (inifa01 for Somsa01)
**	    Fixed compile failure for HP due to order of <compat.h> and 
**	    <stdlib.h>
**      22-aug-2002 (loera01) Bug 108364
**          Make sure a length is provided to SQLGetConnectAttr_InternalCall()
**          if invoked from SQLGetConnectOptionW().
**      04-nov-2002 (loera01)
**          Added check for a non-null pointer before invoking MEfree() on the
**          pwork temporary buffer. 
**	11-nov-2002 (abbjo03)
**	    Remove include of bzarch.h.
**      17-jan-2003 (loera01) Bug 109470 
**          If the platform is WIN32, use the native version of 
**          MultiByteToWideChar() and WideCharToMultiByte().
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**      19-mar-2003 (loera01)
**          Disabled mbtowc() on non-Win32 platforms.  To be replaced with
**          API conversion routine when the ASCII/Unicode project is
**          completed.  Instead, passed through the ASCII value unchanged.
**      2-Apr-2003 (hanal04) Bug 109960
**          Rewrite change for 107793 so that it only affects dgi_us5. Existing
**          change break axp.osf build.
**     28-Jul-2004 (Ralph.Loen@ca.com)
**          Replaced WideCharToMultiByte() and MultiByteToWideChar() with
**          IIapi_convertData().
**     30-Sep-2004 (thoda04) Bug 113159
**          Don't try to convert empty string to/from WCHAR in
**          ConvertCharToWChar and ConvertWCharToChar using IIapi_convertData().
**     26-Oct-2004 (Ralph.Loen@ca.com) Bug 113320
**          In ConvertCharToWChar(), when copying a truncated Unicode string
**          to the output buffer, the character length was used instead of
**          the truncated length.
**     14-Oct-2005 (Ralph.Loen@ca.com) Bug 114083
**          When calling ConvertWCharToChar(), be sure to check whether or
**          not a valid handle is provided before calling ErrState().
**     09-Nov-2004 (loera01) Bug 115517
**          In ConvertWCharToChar(), use the maximum length of the target
**          string as the destination length in IIapi_convertData().  Return
**          the result as a varchar so the result will be null-terminated.
**     01-dec-2005 (loera01) Bug 115358
**          Added argument to SQLPrepare_InternalCall() so that the statement
**          handle buffers aren't freed when a statement is re-prepared.
**     09-mar-2006 (weife01) Bug 115825
**          In ConvertCharToWChar(), delete the cast for cbWideSqlStr to 
**          prevent overflow when the length is long. 
**     16-mar-2006 (Ralph Loen) Bug 115833
**          In ConvertWCharToChar(), replaced allocation of temporary
**          UCS2 buffer with ConvertUCS4ToUCS2() to avoid segmentation
**          error.
**     21-mar-2006 (Ralph Loen) Bug 115833
**          Use a separate UCS2 buffer when invoking ConvertUCS4ToUCS2(),
**          otherwise failed conversions won't display correctly.
**     21-June-2006 (Ralph Loen) Bug 116278
**          In ConvertWCharToChar(), allocate the proper amount of memory
**          for the internal UCS2 buffer when converting from UCS4 to UCS2.
**     03-Jul-2006 (Ralph Loen) SIR 116326
**          Add support for SQLBrowseConnectW().
**     14-Jul-2006 (Ralph Loen) SIR 116385
**          Add support for SQLTablePrivilegesW().
**     21-jul-06 (Ralph Loen)  SIR 116410
**          Add support for SQLColumnPrivilegesW().
**     10-Aug-2006 (Ralph Loen) SIR 116477
**          Add support for ISO dates, times, and intervals (first phase).
**     12-Oct-2006 (weife01) Bug 116839
**          In ConvertWCharToChar(), memory leak was introduced when fixing
**          bug 115517. 
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**     10-Nov-2009 (Ralph Loen) Bug 122866
**          Remove rePrepare argument from SQLPrepare_InternalCall().
**    19-Nov-2009 (Ralph Loen) Bug 122941
**          Re-instate rePrepare argument from SQLPrepare_InternalCall().
**     03-Dec-2009 (Ralph Loen) Bug 123001 
**          In In ConvertWCharToChar() and ConvertCharToWChar(), set the
**          fCvtError field in the statement handle if conversion errors
**          are detected.  Set the sqlstate to 22018 (error in assignment)
**          rather than the custom HY001 with the message "Invalid memory
**          in work area".
**     05-Mar-2010 (Ralph Loen) SIR 123378
**          Resized catalog work buffers to OBJECT_NAME_SIZE.  Replaced
**          references to CATALOG_NAME_WORK_NAME_LEN to OBJECT_NAME_SIZE
**          for consistency.
**     14-Apr-2010 (Ralph Loen) Bug 123558
**          In ExecDirectOrPrepareW(), if the query string is greater than
**          32K UCS2 or 16K UCS4 characters, convert to multi-byte in
**          segments.
*/

/*
**  ConvertCharToWChar
**
**  Convert a character string to wide (Unicode) string, possibly truncated.
**
**  On entry: lpv          -->DBC or STMT block or NULL for errors.
**            szValue      -->ANSI string to convert from.
**            cbValue       = length of input buffer or SQL_NTS if null-terminated.
**            rgbWideValue -->where to return wide (Unicode) string.
**            cbWideValueMax= length (in SQLWCHAR chars) of output buffer.
**            pcbValue      = where to return length (in SQLWCHAR chars) of string
**                            (not counting the null-termination character).
**            pdwValue      = where to return length (in SQLWCHAR chars) of string
**                            (not counting the null-termination character).
**            plenValue     = where to return length (in SQLWCHAR chars) of string
**                            (not counting the null-termination character).
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
*/
RETCODE ConvertCharToWChar(
    LPVOID     lpv,
    char     * szValue,
    SQLINTEGER     cbValue,
    SQLWCHAR * rgbWideValue,
    SQLINTEGER     cbWideValueMax,
    SWORD    * pcbValue,
    SDWORD   * pdwValue,
    SQLINTEGER   * plenValue)
{
    SQLWCHAR *szWideValue = NULL;
    SQLINTEGER     cbWideValue = 0;
    RETCODE    rc    = SQL_SUCCESS;
    IIAPI_CONVERTPARM   cv;
    BOOL isUcs4 = (sizeof(SQLWCHAR) == 4 ? TRUE : FALSE);
    i2 i; 
    i2 *ucs2buf = NULL;

    if (cbValue == SQL_NTS)              /* if length NTS, get length */
    {
        if (szValue != NULL)
            cbValue = (SQLINTEGER)STlength(szValue);
        else
            cbValue = 0;
    }

    if (szValue == NULL  ||  cbValue <= 0)  /* if no input string, return 0 now*/
    {
        if (rgbWideValue  &&  cbWideValueMax>0)  /* room in buffer for null-term? */
            *rgbWideValue = 0;
        if (pcbValue)
            *pcbValue = 0;
        if (pdwValue)
            *pdwValue = 0;
        if (plenValue)
            *plenValue = 0;
        return SQL_SUCCESS;
    }

    if (isUcs4)       
    {
        cbWideValue = cbValue * 2;
        ucs2buf = (i2 *)MEreqmem(0, cbWideValue+8, TRUE, NULL);
        if (ucs2buf == NULL)  /* no memory!? */
        {
            if (lpv)
            {
                rc = ErrState (SQL_HY001, lpv, F_OD0015_IDS_WORKAREA);
                return rc;
            }
            else
                return SQL_ERROR;
        }
    }
    else
    {
        cbWideValue = cbValue * sizeof(SQLWCHAR);
        szWideValue = (SQLWCHAR *)MEreqmem(0, cbWideValue+8, TRUE, NULL);
        if (szWideValue == NULL)  /* no memory!? */
        {
            if (lpv)
            {
                rc = ErrState (SQL_HY001, lpv, F_OD0015_IDS_WORKAREA);
                return rc;
            }
            else
                return SQL_ERROR;
        }
    }

    /* szWideValue -> target buffer to hold Unicode string
    cbWideValue  = target buffer length in bytes */
    
    cv.cv_srcDesc.ds_dataType   = IIAPI_CHA_TYPE;
    cv.cv_srcDesc.ds_nullable   = FALSE;
    cv.cv_srcDesc.ds_length     = (II_UINT2) cbValue;
    cv.cv_srcDesc.ds_precision  = 0;
    cv.cv_srcDesc.ds_scale      = 0;
    cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
    cv.cv_srcDesc.ds_columnName = NULL;
    cv.cv_srcValue.dv_null      = FALSE;
    cv.cv_srcValue.dv_length    = (II_UINT2) cbValue;
    cv.cv_srcValue.dv_value     = szValue;

    cv.cv_dstDesc.ds_dataType   = IIAPI_NCHA_TYPE;
    cv.cv_dstDesc.ds_nullable   = FALSE;
    cv.cv_dstDesc.ds_length     = (II_UINT2)cbWideValue;
    cv.cv_dstDesc.ds_precision  = 0;
    cv.cv_dstDesc.ds_scale      = 0;
    cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
    cv.cv_dstDesc.ds_columnName = NULL;
    cv.cv_dstValue.dv_null      = FALSE;
    cv.cv_dstValue.dv_length    = (II_UINT2)cbWideValue;
    if (isUcs4)
        cv.cv_dstValue.dv_value     = ucs2buf;
    else
        cv.cv_dstValue.dv_value     = szWideValue;

    IIapi_convertData(&cv);  
    if (cv.cv_status != IIAPI_ST_SUCCESS)
    {
        if (ucs2buf)
            MEfree((PTR)ucs2buf);
        if (szWideValue)
            MEfree((PTR)szWideValue);
        if (lpv)
        {
             rc = ErrState (SQL_22018, lpv);
             if (STcompare (((LPSTMT)lpv)->szEyeCatcher, "STMT*") == 0)
             {
                 ((LPSTMT)lpv)->fCvtError |= CVT_INVALID;
                 return rc;
             }
        }
        else
            return SQL_ERROR;
    }

 /* szWideValue -> Unicode string
    cbWideValue  = Unicode string length in characters */

    if (pcbValue)   /* if SWORD destination count field present */
       *pcbValue = (SWORD)cbValue;
                                 /* return length in SQLWCHAR chars 
                                    excluding null-term char*/
    if (pdwValue)   /* if SDWORD destination count field present */
       *pdwValue = (SDWORD)cbValue;
                                 /* return length in SQLWCHAR chars 
                                    excluding null-term char*/
    if (plenValue)  /* if SQLINTEGER destination count field present */
       *plenValue = cbValue;
                                 /* return length in SQLWCHAR chars 
                                    excluding null-term char*/

    if (rgbWideValue)   /* if destination buffer present */
    {
        if (cbValue < cbWideValueMax) /* fits well into the target buffer*/
        {
            if (isUcs4)
            {
                for (i = 0; i < cbValue; i++)
                    rgbWideValue[i] = ucs2buf[i]; 
            }
            else
                memcpy (rgbWideValue, szWideValue, cbWideValue);

            *(rgbWideValue+cbValue)=0;  /* null terminate */
        }
        else
        {   /* need to truncate some */
            if (cbWideValueMax > 0)
            {
                cbValue = cbWideValueMax - 1;
                if (isUcs4)
                {
                    for (i = 0; i < cbValue; i++)
                        rgbWideValue[i] = ucs2buf[i]; 
                }
                else
                    memcpy (rgbWideValue, szWideValue, cbValue*sizeof(SQLWCHAR));

                *(rgbWideValue + cbValue) = 0;
            }
            rc = SQL_SUCCESS_WITH_INFO;
            if (lpv) 
                ErrState (SQL_01004, lpv);
        }
    }
    if (szWideValue)
        MEfree((PTR)szWideValue);      /* free work area for work string */

    return rc;
}


/*
**  ConvertWCharToChar
**
**  Convert a wide (Unicode) string to character string, possibly truncated.
**
**  On entry: lpv          -->DBC or STMT block or NULL for errors.
**            szWideValue  -->Wide (Unicode) string to convert.
**            cbWideValue   = length in char of input buffer
                                 or SQL_NTS if null-terminated.
**            rgbValue     -->where to return null-terminated ANSI string.
**            cbValueMax    = length of output buffer.
**            pcbValue      = where to return length (SWORD) of ANSI string
**                            (not couting the null-termination character).
**            pdwValue      = where to return length (SDWORD) of ANSI string
**                            (not couting the null-termination character).
**            plenValue     = where to return length (SQLINTEGER) of ANSI string
**                            (not couting the null-termination character).
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
*/
RETCODE ConvertWCharToChar(
    LPVOID     lpv,
    SQLWCHAR * szWideValue,
    SQLINTEGER     cbWideValue,
    char     * rgbValue,
    SQLINTEGER     cbValueMax,
    SWORD    * pcbValue,
    SDWORD   * pdwValue,
    SQLINTEGER   * plenValue)
{
    u_i2     cbValue;
    char     * szValue = NULL, *szData = NULL;
    RETCODE    rc = SQL_SUCCESS;
    IIAPI_CONVERTPARM   cv;
    BOOL isUcs4 = (sizeof(SQLWCHAR) == 4 ? TRUE : FALSE);
    u_i2 *ucs2buf = NULL;

    if (cbWideValue == SQL_NTS)              /* if length NTS, get length */
    {
        if (szWideValue)
            cbWideValue = (SQLINTEGER)wcslen(szWideValue); /* length in chars */
        else
            cbWideValue = 0;
    }

    if (szWideValue == NULL  ||  rgbValue == NULL  ||
        cbWideValue <= 0)  /* if no input string, return 0 now*/
       {
        if (rgbValue  &&  cbValueMax>0)  /* room in buffer? */
           *rgbValue = '\0';
        if (pcbValue)
           *pcbValue = 0;
        if (pdwValue)
           *pdwValue = 0;
        if (plenValue)
           *plenValue = 0;
        return SQL_SUCCESS;
       }

    if (isUcs4)
    {
        ucs2buf = (u_i2 *)MEreqmem(0, ((cbWideValue+1)*sizeof(u_i2)), TRUE, NULL);
        if (ucs2buf == NULL)  /* no memory!? */
        {
            if (lpv)
            {
                rc = ErrState (SQL_HY001, lpv, F_OD0015_IDS_WORKAREA);
                return rc;
            }
            else
                return SQL_ERROR;
        }
        ConvertUCS4ToUCS2((u_i4*)szWideValue, (u_i2*)ucs2buf, cbWideValue);
    }

    cbWideValue *= sizeof(SQLWCHAR); 

    szValue = MEreqmem(0, cbValueMax+2, TRUE, NULL);

    if (szValue == NULL)  /* no memory!? */
    {
        if (lpv)
        {
            rc = ErrState (SQL_HY001, lpv, F_OD0015_IDS_WORKAREA);
            return rc;
        }
        else
            return SQL_ERROR;
    }

 /* szValue -> target buffer to hold character string
    cbValue  = target buffer length in bytes */

    if (cbWideValue)     /* convert the Unicode string to character */
    {
        cv.cv_srcDesc.ds_dataType   = IIAPI_NCHA_TYPE;
        cv.cv_srcDesc.ds_nullable   = FALSE;
        cv.cv_srcDesc.ds_length     = (II_UINT2) cbWideValue;
        cv.cv_srcDesc.ds_precision  = 0;
        cv.cv_srcDesc.ds_scale      = 0;
        cv.cv_srcDesc.ds_columnType = IIAPI_COL_TUPLE;
        cv.cv_srcDesc.ds_columnName = NULL;
        cv.cv_srcValue.dv_null      = FALSE;
        if (isUcs4)
            cv.cv_srcValue.dv_value     = ucs2buf;
        else
            cv.cv_srcValue.dv_value     = szWideValue;
        cv.cv_srcValue.dv_length    = (II_UINT2) cbWideValue;
        cv.cv_dstDesc.ds_dataType   = IIAPI_VCH_TYPE;
        cv.cv_dstDesc.ds_nullable   = FALSE;
        cv.cv_dstDesc.ds_length     = (II_UINT2)cbValueMax;
        cv.cv_dstDesc.ds_precision  = 0;
        cv.cv_dstDesc.ds_scale      = 0;
        cv.cv_dstDesc.ds_columnType = IIAPI_COL_TUPLE;
        cv.cv_dstDesc.ds_columnName = NULL;
        cv.cv_dstValue.dv_null      = FALSE;
        cv.cv_dstValue.dv_length    = (II_UINT2)cbValueMax;
        cv.cv_dstValue.dv_value     = szValue;

        IIapi_convertData(&cv);  
        if (cv.cv_status != IIAPI_ST_SUCCESS)
        {
            if (szValue)
                MEfree((PTR)szValue);
            if (ucs2buf)
                MEfree((PTR)ucs2buf);
            if (lpv)
            {
                 rc = ErrState (SQL_22018, lpv);
                 if (STcompare (((LPSTMT)lpv)->szEyeCatcher, "STMT*") == 0)
                 {
                     ((LPSTMT)lpv)->fCvtError |= CVT_INVALID;
                     return rc;
                 }
            }
            else
                return SQL_ERROR;
        }
    }

    cbValue = *(u_i2 *)szValue;
    szData = szValue + sizeof(u_i2);
    

 /* szData -> character string
    cbValue  = character string length in bytes */

    if (pcbValue)   /* if destination count field (SWORD) present */
       *pcbValue = (SWORD)cbValue;
                    /* number of character excluding null-term char*/
    if (pdwValue)   /* if destination count field (SDWORD) present */
       *pdwValue = (SDWORD)cbValue;
                    /* number of character excluding null-term char*/
    if (plenValue)  /* if destination count field (SQLINTEGER) present */
       *plenValue = cbValue;
                    /* number of character excluding null-term char*/

    if (rgbValue)   /* if destination buffer present */
        if (cbValue < cbValueMax) /* fits well into the target buffer*/
        {
            memcpy (rgbValue, szData, cbValue);
            *(rgbValue+cbValue)=0;  /* null terminate */
        }
        else
        {   /* need to truncate some */
            if (cbValueMax > 0)
               {cbValue = (u_i2)cbValueMax - 1;
                memcpy (rgbValue, szData, cbValue);
                *(rgbValue + cbValue) = '\0';
               }
            rc = SQL_SUCCESS_WITH_INFO;
            if (lpv) ErrState (SQL_01004, lpv);
        }

    if (szValue)
        MEfree((PTR)szValue); /* free work area for work string */

    if (ucs2buf)
        MEfree((PTR)ucs2buf);

    return rc;
}

/*
**  ConvertUCS2ToUCS4
**
**  Convert a character string to wide (Unicode) string, possibly truncated.
**
**  On entry: p2  -->UCS2 Unicode string
**            p4  -->UCS4 Unicode string
**            len  = length of string in characters.
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
*/
RETCODE ConvertUCS2ToUCS4(
    u_i2     * p2,
    u_i4     * p4,
    SQLINTEGER     len)
{
    SQLINTEGER     i;
    u_i4       sizeofSQLWCHAR = sizeof(SQLWCHAR);
    u_i4       tempui4;

    if (sizeofSQLWCHAR != 4)  /* return if not needed */
        return SQL_SUCCESS;

    p2 = p2 + len - 1;    /* assume that p2 and p4 point to same buffer */
    p4 = p4 + len - 1;    /* and process right to left */

    for (i=0; i < len; i++, p2--, p4--)
        {
 /*      *p4 = *p2;               */
         tempui4 = *p2;
         I4ASSIGN_MACRO(tempui4, *p4);
        }

    return SQL_SUCCESS;
}


/*
**  ConvertUCS4ToUCS2
**
**  Convert a character string to wide (Unicode) string, possibly truncated.
**
**  On entry: p4  -->UCS4 Unicode string
**            p2  -->UCS2 Unicode string
**            len  = length of string in characters.
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
*/
RETCODE ConvertUCS4ToUCS2(
    u_i4     * p4,
    u_i2     * p2,
    SQLINTEGER     len)
{
    SQLINTEGER     i;
    u_i4       sizeofSQLWCHAR = sizeof(SQLWCHAR);

    if (sizeofSQLWCHAR != 4)  /* return if not needed */
        return SQL_SUCCESS;

        /* assume that p2 and p4 point to same buffer */
        /* and process left to right */

    for (i=0; i < len; i++, p2++, p4++)
        *p2 = (u_i2)(*p4);

    return SQL_SUCCESS;
}


/*
**  GetWChar
**
**  Return a wide (Unicode) character string, possibly truncated.
**
**  On entry: pstmt     -->statement block or NULL.
**            szValue   -->string to copy.
**            cbValue    = length of input buffer or SQL_NTS if null-terminated.
**            rgbValue  -->where to return string.
**            cbValueMax = length of output buffer.
**            pcbValue   = where to return length of string.
**
**  On exit:  Data is copied to buffer, possibly truncated, null terminated.
**
**  Returns:  SQL_SUCCESS           String returned successfully.
**            SQL_SUCCESS_WITH_INFO String truncated, buffer too short.
**
**  Note:     Do not use to directly return length to caller if
**            a double word is requred for pcbValue (e.g SQLGetData).
*/
RETCODE GetWChar(
    LPVOID        lpv,
    SQLWCHAR   *  szValue,
    SQLINTEGER        cbValue,
    SQLWCHAR   *  rgbValue,
    SQLINTEGER        cbValueMax,
    SQLINTEGER     *  pcbValue)
{
    RETCODE rc = SQL_SUCCESS;

    if (cbValue == SQL_NTS)              /* if length unknown, get length */
        cbValue =  (SQLINTEGER)wcslen (szValue);

    if (pcbValue)
       *pcbValue = cbValue;  /* number of character excluding null-term char*/

    if (rgbValue)
        if (cbValue < cbValueMax)
           {memcpy (rgbValue, szValue, cbValue*sizeof(SQLWCHAR));
            *(rgbValue+cbValue)=0;  /* null terminate */
           }
        else
        {
            if (cbValueMax > 0)
               {cbValue = cbValueMax - 1;
                memcpy (rgbValue, szValue, cbValue*sizeof(SQLWCHAR));
                *(rgbValue + cbValue) = '\0';
               }
            rc = SQL_SUCCESS_WITH_INFO;
            if (lpv) ErrState (SQL_01004, lpv);
        }

    return rc;
}

static void CatalogNameConversion(
    SQLWCHAR        *szWideName,
    SQLSMALLINT      cbWideName,   /* count of chars */
    SQLCHAR        **szName,
    SQLSMALLINT     *cbName)       /* count of chars */
{
    RETCODE rc;
    if (szWideName)
    {
        rc = ConvertWCharToChar(NULL,
           szWideName, (SQLINTEGER)cbWideName,              /* source */
              (char*)*szName, OBJECT_NAME_SIZE,  /* target */
                       cbName, NULL, NULL);
        if (rc != SQL_SUCCESS)
        {
             *szName = NULL;
             *cbName = 0;
        }
    }
    else
    {
        *szName = NULL;
        *cbName = 0;
    }

    if (cbWideName == SQL_NTS)   /* if original passed null-terminated */
        *cbName     = SQL_NTS;   /* then pass null-terminated string */

    return;
}


static SQLRETURN   CatalogFunction(
    SQLINTEGER       fFunction,
    SQLHSTMT         hstmt,
    SQLWCHAR        *szWideName1,
    SQLSMALLINT      cbWideName1,
    SQLWCHAR        *szWideName2,
    SQLSMALLINT      cbWideName2,
    SQLWCHAR        *szWideName3,
    SQLSMALLINT      cbWideName3,
    SQLWCHAR        *szWideName4,
    SQLSMALLINT      cbWideName4,
    SQLWCHAR        *szWideName5,
    SQLSMALLINT      cbWideName5,
    SQLWCHAR        *szWideName6,
    SQLSMALLINT      cbWideName6,
    SQLUSMALLINT     field1,
    SQLUSMALLINT     field2,
    SQLUSMALLINT     field3)
{
    LPSTMT           pstmt = (LPSTMT)hstmt;
    SQLRETURN        rc = SQL_ERROR;
    SQLCHAR          szName1Buf[OBJECT_NAME_SIZE];
    SQLCHAR          *szName1 = szName1Buf;
    SQLSMALLINT      cbName1 = 0;
    SQLCHAR          szName2Buf[OBJECT_NAME_SIZE];
    SQLCHAR         *szName2 = szName2Buf;
    SQLSMALLINT      cbName2 = 0;
    SQLCHAR          szName3Buf[OBJECT_NAME_SIZE];
    SQLCHAR         *szName3 = szName3Buf;
    SQLSMALLINT      cbName3 = 0;
    SQLCHAR          szName4Buf[OBJECT_NAME_SIZE];
    SQLCHAR         *szName4 = szName4Buf;
    SQLSMALLINT      cbName4 = 0;
    SQLCHAR          szName5Buf[OBJECT_NAME_SIZE];
    SQLCHAR         *szName5 = szName5Buf;
    SQLSMALLINT      cbName5 = 0;
    SQLCHAR          szName6Buf[OBJECT_NAME_SIZE];
    SQLCHAR         *szName6 = szName6Buf;
    SQLSMALLINT      cbName6 = 0;

    if (pstmt->pdbcOwner->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
       {
        szName1 = (SQLCHAR*)szWideName1;        cbName1 = cbWideName1;
        szName2 = (SQLCHAR*)szWideName2;        cbName2 = cbWideName2;
        szName3 = (SQLCHAR*)szWideName3;        cbName3 = cbWideName3;
        szName4 = (SQLCHAR*)szWideName4;        cbName4 = cbWideName4;
        szName5 = (SQLCHAR*)szWideName5;        cbName5 = cbWideName5;
        szName6 = (SQLCHAR*)szWideName6;        cbName6 = cbWideName6;
       }
    else
       {
       /* convert the wide (Unicode) strings to ANSI strings */
        CatalogNameConversion(szWideName1, cbWideName1, &szName1, &cbName1);
        CatalogNameConversion(szWideName2, cbWideName2, &szName2, &cbName2);
        CatalogNameConversion(szWideName3, cbWideName3, &szName3, &cbName3);
        CatalogNameConversion(szWideName4, cbWideName4, &szName4, &cbName4);
        if (fFunction==SQL_API_SQLFOREIGNKEYS)  /* only do if SQLForeignKeys */
        {                                       /* else don't waste the time */
        CatalogNameConversion(szWideName5, cbWideName5, &szName5, &cbName5);
        CatalogNameConversion(szWideName6, cbWideName6, &szName6, &cbName6);
        }
       }

    switch(fFunction)
    {
    case SQL_API_SQLSETCURSORNAME:

            rc = SQLSetCursorName_InternalCall( hstmt,
                                   szName1, cbName1);
            break;

    case SQL_API_SQLCOLUMNS:

            rc = SQLColumns_InternalCall( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3,
                                   szName4, cbName4);
            break;

    case SQL_API_SQLSPECIALCOLUMNS:

            rc = SQLSpecialColumns_InternalCall(hstmt,
                                   field1,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3,
                                   field2,
                                   field3);
            break;

    case SQL_API_SQLSTATISTICS:

            rc = SQLStatistics_InternalCall( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3,
                                   field2,
                                   field3);
            break;

    case SQL_API_SQLTABLES:

            rc = SQLTables_InternalCall( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3,
                                   szName4, cbName4);
            break;

    case SQL_API_SQLFOREIGNKEYS:

            rc = SQLForeignKeys_InternalCall( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3,
                                   szName4, cbName4,
                                   szName5, cbName5,
                                   szName6, cbName6);
            break;

    case SQL_API_SQLPRIMARYKEYS:

            rc = SQLPrimaryKeys_InternalCall( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3);
            break;

    case SQL_API_SQLPROCEDURECOLUMNS:

            rc = SQLProcedureColumns_Internal( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3,
                                   szName4, cbName4);
            break;

    case SQL_API_SQLPROCEDURES:

            rc = SQLProcedures_InternalCall( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3);
            break;

   case SQL_API_SQLTABLEPRIVILEGES:
    
           rc = SQLTablePrivileges_InternalCall( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3);
           break;

   case SQL_API_SQLCOLUMNPRIVILEGES:
    
           rc = SQLColumnPrivileges_InternalCall( hstmt,
                                   szName1, cbName1,
                                   szName2, cbName2,
                                   szName3, cbName3,
								   szName4, cbName4);
           break;



    }  /* end switch */

    return rc;
}



SQLRETURN SQL_API SQLColAttributeW(
    SQLHSTMT         hstmt,
    SQLUSMALLINT     ColumnNumber,
    SQLUSMALLINT     FieldIdentifier,
    SQLPOINTER       ValuePtr,
    SQLSMALLINT      BufferLength,       /*   count of bytes */
    SQLSMALLINT     *StringLengthPtr,    /* ->count of bytes */
    SQLPOINTER       NumericAttributePtr) 
{
    LPSTMT           pstmt = (LPSTMT)hstmt;
    SQLRETURN        rc, rc2;
    SQLPOINTER       ValuePtrOrig   = ValuePtr;
    SQLSMALLINT      BufferLengthOrig = BufferLength;
    short            bNeedConversion=FALSE;
    char             DescWk[OBJECT_NAME_SIZE];

    if (pstmt->pdbcOwner->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLColAttribute_InternalCall(hstmt,
                                      ColumnNumber, FieldIdentifier,
                                      ValuePtr, BufferLength,
                                      StringLengthPtr, NumericAttributePtr);

    if (FieldIdentifier == SQL_DESC_BASE_COLUMN_NAME ||
        FieldIdentifier == SQL_DESC_BASE_TABLE_NAME  ||
        FieldIdentifier == SQL_DESC_CATALOG_NAME     ||
        FieldIdentifier == SQL_DESC_LABEL            ||
        FieldIdentifier == SQL_DESC_LITERAL_PREFIX   ||
        FieldIdentifier == SQL_DESC_LITERAL_SUFFIX   ||
        FieldIdentifier == SQL_DESC_LOCAL_TYPE_NAME  ||
        FieldIdentifier == SQL_DESC_NAME             ||
        FieldIdentifier == SQL_DESC_SCHEMA_NAME      ||
        FieldIdentifier == SQL_DESC_TABLE_NAME       ||
        FieldIdentifier == SQL_DESC_TYPE_NAME)
           {
            bNeedConversion=TRUE;
            ValuePtr   = DescWk;
            BufferLength = sizeof(DescWk);
           }

    rc = SQLColAttribute_InternalCall(hstmt, ColumnNumber, FieldIdentifier,
                                      ValuePtr, BufferLength,
                                      StringLengthPtr, NumericAttributePtr);

    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    if (bNeedConversion==FALSE)  /* if not a name, we're all done */
        return rc;

    rc2 = ConvertCharToWChar(pstmt,    /* move name back as wide (Unicode)*/
                         ValuePtr,   SQL_NTS,                 /* source */
                         ValuePtrOrig, (SWORD)(BufferLengthOrig/sizeof(SQLWCHAR)),
                                                             /* target */
                         StringLengthPtr, NULL, NULL);
    if (rc == SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    if (StringLengthPtr && (rc == SQL_SUCCESS  ||
                            rc == SQL_SUCCESS_WITH_INFO))
       *StringLengthPtr = (SQLSMALLINT)(*StringLengthPtr * sizeof(SQLWCHAR));
                                       /* return length as byte length */

    return rc;
}


SQLRETURN SQL_API SQLConnectW(
    SQLHDBC          hdbc,
    SQLWCHAR        *szWideDSN,
    SQLSMALLINT      cbWideDSN,      /*   count of chars */
    SQLWCHAR        *szWideUID,
    SQLSMALLINT      cbWideUID,      /*   count of chars */
    SQLWCHAR        *szWidePWD,
    SQLSMALLINT      cbWidePWD)      /*   count of chars */
{
    LPDBC       pdbc = (LPDBC)hdbc;
    SQLCHAR     szDSN[128]="";
    SWORD       cbDSN = 0;
    SQLCHAR     szUID[128]="";
    SWORD       cbUID = 0;
    SQLCHAR     szPWD[128]="";
    SWORD       cbPWD = 0;
    SWORD       cbWideDSNorig = cbWideDSN;
    SWORD       cbWideUIDorig = cbWideUID;
    SWORD       cbWidePWDorig = cbWidePWD;
    RETCODE     rc;

    if (pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLConnect_InternalCall(pdbc,
                                       (SQLCHAR*)szWideDSN, cbWideDSN,
                                       (SQLCHAR*)szWideUID, cbWideUID,
                                       (SQLCHAR*)szWidePWD, cbWidePWD);

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    ErrResetDbc (pdbc);
    ResetDbc (pdbc);

    rc = ConvertWCharToChar(pdbc,
                        szWideDSN, cbWideDSN,         /* source */
                        (char*)szDSN, sizeof(szDSN),  /* target */
                        &cbDSN, NULL, NULL);
    if (rc != SQL_SUCCESS)
       {UnlockDbc(pdbc);
        return rc;
       }

    rc = ConvertWCharToChar(pdbc,
                        szWideUID, cbWideUID,         /* source */
                        (char*)szUID, sizeof(szUID),  /* target */
                        &cbUID, NULL, NULL);
    if (rc != SQL_SUCCESS)
       {UnlockDbc(pdbc);
        return rc;
       }

    rc = ConvertWCharToChar(pdbc,
                        szWidePWD, cbWidePWD,         /* source */
                        (char*)szPWD, sizeof(szPWD),  /* target */
                        &cbPWD, NULL, NULL);
    if (rc != SQL_SUCCESS)
       {UnlockDbc(pdbc);
        return rc;
       }

    UnlockDbc(pdbc);

    if (cbWideDSNorig == SQL_NTS)    /* if user passed NTS, we pass NTS */
        cbDSN          = SQL_NTS;
    if (cbWideUIDorig == SQL_NTS)
        cbUID          = SQL_NTS;
    if (cbWidePWDorig == SQL_NTS)
        cbPWD          = SQL_NTS;

    rc = SQLConnect_InternalCall(pdbc, szDSN, cbDSN, szUID, cbUID, szPWD, cbPWD);

    return(rc);
}


SQLRETURN SQL_API SQLDescribeColW(
    SQLHSTMT         hstmt,
    SQLUSMALLINT     icol,
    SQLWCHAR        *szWideColName,
    SQLSMALLINT      cbWideColNameMax,   /*   count of chars */
    SQLSMALLINT     *pcbWideColName,     /* ->count of chars */
    SQLSMALLINT     *pfSqlType,
    SQLUINTEGER         *pcbColDef,          /* ->ColumnSize in chars; may cause MS KB Q249803*/
    SQLSMALLINT     *pibScale,
    SQLSMALLINT     *pfNullable)
{
    LPSTMT           pstmt = (LPSTMT)hstmt;
    SQLCHAR          szColNameWk[OBJECT_NAME_SIZE]="";
    SQLRETURN        rc, rc2;

    if (pstmt->pdbcOwner->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLDescribeCol_InternalCall(hstmt, icol,
                        (SQLCHAR*)szWideColName, cbWideColNameMax, pcbWideColName,
                        pfSqlType, pcbColDef, pibScale, pfNullable);

    rc = SQLDescribeCol_InternalCall(hstmt, icol,
                        szColNameWk, sizeof(szColNameWk), NULL,
                        pfSqlType, pcbColDef, pibScale, pfNullable);
    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    rc2 = ConvertCharToWChar(pstmt,    /* move column name back as wide (Unicode)*/
                         (char*)szColNameWk,   SQL_NTS,     /* source */
                         szWideColName, cbWideColNameMax,   /* target */
                         pcbWideColName, NULL, NULL);
    if (rc == SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    return rc;
}


SQLRETURN SQL_API SQLErrorW(
    SQLHENV          henv,
    SQLHDBC          hdbc,
    SQLHSTMT         hstmt,
    SQLWCHAR        *szSqlState,
    SQLINTEGER      *pfNativeError,
    SQLWCHAR        *szErrorMsg,
    SQLSMALLINT      cbErrorMsgMax, /* ->count of chars */
    SQLSMALLINT     *pcbErrorMsg)   /* ->count of chars */
{
    SQLRETURN    rc, rc2 = SQL_SUCCESS;
    SQLCHAR      szSqlStateWk    [SQL_SQLSTATE_SIZE+1]="";
    SQLWCHAR     szWideSqlStateWk[SQL_SQLSTATE_SIZE+1]={0};
    SQLCHAR      szErrorMsgWk[512];

    if ((henv  &&  ((LPENV )henv)                       ->isMerant311) ||
        (hdbc  &&  ((LPDBC )hdbc)            ->penvOwner->isMerant311) ||
        (hstmt &&  ((LPSTMT)hstmt)->pdbcOwner->penvOwner->isMerant311))
                              /* call ASCII entry point if Merant 3.11*/
        return SQLError_InternalCall(henv, hdbc, hstmt,
                  (SQLCHAR*)szSqlState, pfNativeError,
                  (SQLCHAR*)szErrorMsg, cbErrorMsgMax, pcbErrorMsg);

/*
   {    char s[80]= "cbErrorMsgMax = ";
        char t[80];
        itoa((int)cbErrorMsgMax, t, (int)10);
           strcat(s,t); 
        MessageBox(NULL, s, "SQLErrorW", MB_ICONSTOP|MB_OK);
    }
*/
    rc = SQLError_InternalCall(henv, hdbc, hstmt,
                  szSqlStateWk, pfNativeError,
                  szErrorMsgWk, sizeof(szErrorMsgWk), pcbErrorMsg);
    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    if (szSqlState)          /* move SQLSTATE back */
       {
        rc2 = ConvertCharToWChar(NULL,
             (CHAR*)szSqlStateWk,    SQL_SQLSTATE_SIZE,     /* source */
             szWideSqlStateWk,                       /* target */
             sizeof(szWideSqlStateWk)/sizeof(SQLWCHAR),
             NULL, NULL, NULL);
        memcpy(szSqlState, szWideSqlStateWk, SQL_SQLSTATE_SIZE*sizeof(SQLWCHAR));
       }

    if (rc2 == SQL_SUCCESS)
        rc2 = ConvertCharToWChar(NULL,   /* move Error Message Text back */
            (CHAR*)szErrorMsgWk,    SQL_NTS     ,  /* source */
            szErrorMsg,      cbErrorMsgMax,        /* target */
            pcbErrorMsg, NULL, NULL);

    if (rc == SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    return rc;
}


#define EXEC_DIRECT_UNICODE_WORKBUFFER_SIZE   1000
#define MAX_UNICODE_CHAR_SIZE 64000 / sizeof(SQLWCHAR) 

/* common code for SQLExecDirect and SQLPrepare */
static SQLRETURN SQL_API ExecDirectOrPrepareW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szWideSqlStr,
    SQLINTEGER           cbWideSqlStr,     /*   count of chars */
    SQLINTEGER       fFunction)
{
    LPSTMT     pstmt= (LPSTMT)hstmt;
    LPDBC      pdbc;
    SQLCHAR     * szValue;
    SQLINTEGER     cbValue, retLen;
    char     * pwork = NULL;
    RETCODE    rc = SQL_SUCCESS;
    SQLINTEGER     cbWideSqlStrOrig=cbWideSqlStr; /*   save the original count */
    char       workbuffer[EXEC_DIRECT_UNICODE_WORKBUFFER_SIZE];
    SQLWCHAR   saveWChar;
    SQLWCHAR   *pWSqlStr = szWideSqlStr;

    if (!LockStmt (pstmt)) return SQL_INVALID_HANDLE;

    ErrResetStmt (pstmt);
    pdbc = pstmt->pdbcOwner;

    if (cbWideSqlStr == SQL_NTS)          /* if length NTS, get length */
        cbWideSqlStr = (SQLINTEGER)wcslen(szWideSqlStr);

    cbValue = cbWideSqlStr*sizeof(SQLWCHAR); /* min byte length of buffer needed */

    if (cbValue < EXEC_DIRECT_UNICODE_WORKBUFFER_SIZE)
       { szValue = workbuffer;   /* use the stack work buffer */
         cbValue = EXEC_DIRECT_UNICODE_WORKBUFFER_SIZE;
       }
    else
        {
         cbValue += 4;  /* add a safety margin */
         szValue =
           pwork = MEreqmem(0, cbValue+4, TRUE, NULL);
                     /* allocate work area for string plus another safety */
         if (pwork == NULL)  /* no memory!? */
            {
             if (pstmt)
             {
                 rc = ErrState (SQL_HY001, pstmt, F_OD0015_IDS_WORKAREA);
                 UnlockStmt (pstmt);
                 return rc;
             }
             else
                 return SQL_ERROR;
            }
            szValue = pwork;
            szValue[0] = '\0';
        }

    /*
    ** The Unicode string szWideSqlStr is passed as an IIAPI_DATAVALUE 
    ** when ConvertWCharToChar() converts to multibyte.  Because of this,
    ** the byte length cannot be greater than approximately 64K.  This works
    ** out to 32K UCS2 characters or 16K UCS4 characters.  
    ** If szWideSqlStr doesn't fit, convert in 64K-byte segments.
    */
    if (cbWideSqlStr > MAX_UNICODE_CHAR_SIZE)
    {
        SQLINTEGER len = MAX_UNICODE_CHAR_SIZE;

        while (cbWideSqlStr)
        {
            rc = ConvertWCharToChar(pdbc,
                pWSqlStr, len,  /* source */
                szValue, MAX_UNICODE_CHAR_SIZE + 2, /* target */
                NULL, NULL, &retLen);
    
            if (rc != SQL_SUCCESS)
            {
                UnlockStmt (pstmt);
                return rc;
            }
            cbWideSqlStr -= len;
            pWSqlStr += len;
            szValue += retLen;
            len = min(cbWideSqlStr, MAX_UNICODE_CHAR_SIZE);
        }
        szValue = pwork;
    }
    else
    {
        rc = ConvertWCharToChar(pdbc,
            szWideSqlStr,        cbWideSqlStr,  /* source */
            szValue,             cbValue,       /* target */
            NULL, NULL, &retLen);
        if (rc != SQL_SUCCESS)
        {
            UnlockStmt (pstmt);
            return rc;
        }
    }

    if (cbWideSqlStrOrig == SQL_NTS) /* if user passed NTS, we pass NTS */
        cbValue           = SQL_NTS;
	else
		cbValue = cbWideSqlStrOrig;

    UnlockStmt (pstmt);

    if (fFunction == SQL_API_SQLEXECDIRECT)
        rc = SQLExecDirect_InternalCall(pstmt, (SQLCHAR*)szValue, cbValue);
    else 
    if (fFunction == SQL_API_SQLPREPARE)
        rc = SQLPrepare_InternalCall(hstmt, (SQLCHAR*)szValue, 
            (SDWORD)cbValue, FALSE);

    if (pwork)
        MEfree((PTR)pwork);                 /* free work area for work string */

    return rc;
}

SQLRETURN SQL_API SQLExecDirectW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szWideSqlStr,
    SQLINTEGER       cbWideSqlStr)     /*   count of chars */
{
    LPSTMT           pstmt = (LPSTMT)hstmt;

    if (pstmt->pdbcOwner->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLExecDirect_InternalCall(pstmt,
                                      (SQLCHAR*)szWideSqlStr, cbWideSqlStr);

    return ExecDirectOrPrepareW(hstmt, szWideSqlStr, cbWideSqlStr,
                                SQL_API_SQLEXECDIRECT);  /* call common code */
}


SQLRETURN SQL_API SQLGetConnectAttrW(
    SQLHDBC          hdbc,
    SQLINTEGER       fAttribute,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValueMax,    /*   count of bytes */
    SQLINTEGER      *pcbValue)      /* ->count of bytes */
{
    LPDBC            pdbc = (LPDBC)hdbc;
    SQLRETURN        rc, rc2;
    SQLPOINTER       rgbValueOrig   = rgbValue;
    SQLINTEGER       cbValueMaxOrig = cbValueMax;
    short            bNeedConversion=FALSE;
    char             DescWk[OBJECT_NAME_SIZE];

    if (pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLGetConnectAttr_InternalCall(hdbc, fAttribute,
                                        rgbValue, cbValueMax, pcbValue);

    if (fAttribute == SQL_ATTR_CURRENT_CATALOG  ||
        fAttribute == SQL_ATTR_TRACEFILE        ||
        fAttribute == SQL_ATTR_TRANSLATE_LIB)
           {
            bNeedConversion=TRUE;
            rgbValue   = DescWk;
            cbValueMax = sizeof(DescWk);
           }

    rc = SQLGetConnectAttr_InternalCall(hdbc, fAttribute,
                                        rgbValue, cbValueMax, pcbValue);

    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    if (bNeedConversion==FALSE)  /* if not a name, we're all done */
        return rc;

    rc2 = ConvertCharToWChar(pdbc,    /* move name back as wide (Unicode)*/
        rgbValue,   SQL_NTS,                /* source */
        rgbValueOrig, (SWORD)(cbValueMaxOrig/sizeof(SQLWCHAR)), /* target */
        NULL, pcbValue, NULL);
    if (rc == SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    if (pcbValue && (rc == SQL_SUCCESS  ||
                     rc == SQL_SUCCESS_WITH_INFO))
       *pcbValue = *pcbValue * sizeof(SQLWCHAR);
                                       /* return length as byte length */

    return rc;
}


SQLRETURN SQL_API SQLGetCursorNameW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCursor,
    SQLSMALLINT      cbCursorMax,   /*   count of chars; bytes in SQLGetCursorNameA */
    SQLSMALLINT     *pcbCursor)     /* ->count of chars */
{
    LPSTMT           pstmt = (LPSTMT)hstmt;
    char             szNameWk[OBJECT_NAME_SIZE];
    SQLRETURN        rc, rc2;

    /* call ASCII entry point if Merant 3.11 Driver Manager */
    if (pstmt->pdbcOwner->penvOwner->isMerant311) 
        return SQLGetCursorName_InternalCall(hstmt,
                                       (SQLCHAR*)szCursor, cbCursorMax,
                                       pcbCursor);

    rc = SQLGetCursorName_InternalCall(hstmt,
                                       (SQLCHAR*)szNameWk, sizeof(szNameWk),
                                       pcbCursor);
    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    rc2 = ConvertCharToWChar(pstmt,    /* move column name back as wide (Unicode)*/
                         szNameWk,   SQL_NTS,       /* source */
                         szCursor,   cbCursorMax,   /* target */
                         pcbCursor, NULL, NULL);
    if (rc == SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    return rc;
}


#if (ODBCVER >= 0x0300)
SQLRETURN  SQL_API SQLSetDescFieldW(
    SQLHDESC         DescriptorHandle,
    SQLSMALLINT      RecNumber,
    SQLSMALLINT      FieldIdentifier,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValue)     /* count of bytes */
{
    LPDESC           pdesc = (LPDESC)DescriptorHandle;
    SQLRETURN        rc;
    SQLCHAR          szName1Buf[OBJECT_NAME_SIZE];
    SQLCHAR          *szName = szName1Buf;
    SQLSMALLINT      cbName = 0;

    if (pdesc->pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLSetDescField_InternalCall(DescriptorHandle, RecNumber,
                                      FieldIdentifier, rgbValue, cbValue);

    if (FieldIdentifier == SQL_DESC_NAME)
       {
       if (cbValue > 0)     /* convert count of bytes  to count of chars */
           cbValue /= sizeof(SQLWCHAR);
       CatalogNameConversion(rgbValue, (SQLSMALLINT)cbValue, &szName, &cbName);
       if (szName)
           rgbValue = szName;
       cbValue  = cbName;
       }

    rc = SQLSetDescField_InternalCall(DescriptorHandle, RecNumber,
                                      FieldIdentifier, rgbValue, cbValue);
    return(rc);
}


SQLRETURN SQL_API SQLGetDescFieldW(
    SQLHDESC         hdesc,
    SQLSMALLINT      iRecord,
    SQLSMALLINT      iField,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValueMax,    /*   count of bytes */
    SQLINTEGER      *pcbValue)      /* ->count of bytes */
{
    LPDESC           pdesc = (LPDESC)hdesc;
    SQLRETURN        rc, rc2;
    SQLPOINTER       rgbValueOrig   = rgbValue;
    SQLINTEGER       cbValueMaxOrig = cbValueMax;
    short            bNeedConversion=FALSE;
    char             DescWk[OBJECT_NAME_SIZE];

    if (pdesc->pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLGetDescField_InternalCall(hdesc, iRecord, iField,
                                      rgbValue, cbValueMax, pcbValue);

    if (iField == SQL_DESC_BASE_COLUMN_NAME ||
        iField == SQL_DESC_BASE_TABLE_NAME  ||
        iField == SQL_DESC_CATALOG_NAME     ||
        iField == SQL_DESC_LABEL            ||
        iField == SQL_DESC_LITERAL_PREFIX   ||
        iField == SQL_DESC_LITERAL_SUFFIX   ||
        iField == SQL_DESC_LOCAL_TYPE_NAME  ||
        iField == SQL_DESC_NAME             ||
        iField == SQL_DESC_SCHEMA_NAME      ||
        iField == SQL_DESC_TABLE_NAME       ||
        iField == SQL_DESC_TYPE_NAME)
           {
            bNeedConversion=TRUE;
            rgbValue   = DescWk;
            cbValueMax = sizeof(DescWk);
           }

    rc = SQLGetDescField_InternalCall(hdesc, iRecord, iField,
                                      rgbValue, cbValueMax, pcbValue);

    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    if (bNeedConversion==FALSE)  /* if not a name, we're all done */
        return rc;

    rc2 = ConvertCharToWChar(pdesc,   /* move name back as wide (Unicode)*/
                         rgbValue,   SQL_NTS,                /* source */
                         rgbValueOrig, (SWORD)(cbValueMaxOrig/sizeof(SQLWCHAR)),
                                                             /* target */
                         NULL, pcbValue, NULL);
    if (rc == SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    if (pcbValue && (rc == SQL_SUCCESS  ||
                     rc == SQL_SUCCESS_WITH_INFO))
       *pcbValue = *pcbValue * sizeof(SQLWCHAR);
                                       /* return length as byte length */

    return rc;
}


SQLRETURN SQL_API SQLGetDescRecW(
    SQLHDESC         hdesc,
    SQLSMALLINT      iRecord,
    SQLWCHAR        *szWideColName,
    SQLSMALLINT      cbWideColNameMax, /*   count of chars; bytes in SQLGetDescRecA */
    SQLSMALLINT     *pcbWideColName,   /* ->count of chars */
    SQLSMALLINT     *pfType,
    SQLSMALLINT     *pfSubType,
    SQLINTEGER          *pLength,
    SQLSMALLINT     *pPrecision, 
    SQLSMALLINT     *pScale,
    SQLSMALLINT     *pNullable)
{
    LPDESC           pdesc = (LPDESC)hdesc;
    char             szColNameWk[OBJECT_NAME_SIZE]="";
    SQLRETURN        rc, rc2;

    if (pdesc->pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLGetDescRec_InternalCall(hdesc, iRecord,
                       (SQLCHAR*)szWideColName, cbWideColNameMax, pcbWideColName,
                       pfType, pfSubType, pLength, pPrecision, pScale, pNullable);

    rc = SQLGetDescRec_InternalCall(hdesc, iRecord,
                       (SQLCHAR*)szColNameWk, sizeof(szColNameWk), NULL,
                       pfType, pfSubType, pLength, pPrecision, pScale, pNullable);

    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    rc2 = ConvertCharToWChar(pdesc,    /* move column name back as wide (Unicode)*/
                         szColNameWk,   SQL_NTS,            /* source */
                         szWideColName, cbWideColNameMax,   /* target */
                         pcbWideColName, NULL, NULL);
    if (rc == SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    return rc;
}




SQLRETURN SQL_API SQLGetDiagFieldW(
    SQLSMALLINT      fHandleType,
    SQLHANDLE        handle,
    SQLSMALLINT      iRecord,
    SQLSMALLINT      fDiagField,
    SQLPOINTER       rgbDiagInfo,
    SQLSMALLINT      cbDiagInfoMax,  /*   count of bytes */
    SQLSMALLINT     *pcbDiagInfo)    /* ->count of bytes */
{
    SQLRETURN        rc, rc2;
    SQLPOINTER       rgbDiagInfoOrig   = rgbDiagInfo;
    SQLSMALLINT      cbDiagInfoMaxOrig = cbDiagInfoMax;
    short            bNeedConversion=FALSE;
    char             DescWk[512];

    if ((fHandleType==SQL_HANDLE_ENV  &&  ((LPENV )handle)                      ->isMerant311) ||
        (fHandleType==SQL_HANDLE_DBC  &&  ((LPDBC )handle)           ->penvOwner->isMerant311) ||
        (fHandleType==SQL_HANDLE_STMT &&  ((LPSTMT)handle)->pdbcOwner->penvOwner->isMerant311) ||
        (fHandleType==SQL_HANDLE_DESC &&  ((LPDESC)handle)     ->pdbc->penvOwner->isMerant311))
                              /* call ASCII entry point if Merant 3.11*/
        return SQLGetDiagField_InternalCall(fHandleType, handle, iRecord, fDiagField,
                         rgbDiagInfo, cbDiagInfoMax, pcbDiagInfo);

    if (fDiagField == SQL_DIAG_DYNAMIC_FUNCTION ||
        fDiagField == SQL_DIAG_CLASS_ORIGIN     ||
        fDiagField == SQL_DIAG_CONNECTION_NAME  ||
        fDiagField == SQL_DIAG_MESSAGE_TEXT     ||
        fDiagField == SQL_DIAG_SERVER_NAME      ||
        fDiagField == SQL_DIAG_SQLSTATE         ||
        fDiagField == SQL_DIAG_SUBCLASS_ORIGIN)
           {
            bNeedConversion=TRUE;
            rgbDiagInfo   = DescWk;
            cbDiagInfoMax = sizeof(DescWk);
           }

    rc = SQLGetDiagField_InternalCall(fHandleType, handle, iRecord, fDiagField,
                         rgbDiagInfo, cbDiagInfoMax, pcbDiagInfo);

    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    if (bNeedConversion==FALSE)  /* if not a name, we're all done */
        return rc;

    rc2 = ConvertCharToWChar(NULL,    /* move name back as wide (Unicode)*/
                         rgbDiagInfo,   SQL_NTS,             /* source */
                         rgbDiagInfoOrig, (SWORD)(cbDiagInfoMaxOrig/sizeof(SQLWCHAR)),
                                                             /* target */
                         pcbDiagInfo, NULL, NULL);
    if (rc == SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    if (pcbDiagInfo && (rc == SQL_SUCCESS  ||  rc == SQL_SUCCESS_WITH_INFO))
       *pcbDiagInfo = (SQLSMALLINT)(*pcbDiagInfo * sizeof(SQLWCHAR));
                             /* return length as byte length */

    return rc;
}


SQLRETURN SQL_API SQLGetDiagRecW(
    SQLSMALLINT      fHandleType,
    SQLHANDLE        handle,
    SQLSMALLINT      iRecord,
    SQLWCHAR        *szSqlState,
    SQLINTEGER      *pfNativeError,
    SQLWCHAR        *szErrorMsg,
    SQLSMALLINT      cbErrorMsgMax,  /*   count of chars */
    SQLSMALLINT     *pcbErrorMsg)    /* ->count of chars */
{
    SQLRETURN    rc, rc2 = SQL_SUCCESS;
    char         szSqlStateWk    [SQL_SQLSTATE_SIZE+1]="";
    SQLWCHAR     szWideSqlStateWk[SQL_SQLSTATE_SIZE+1]={0};
    char         szErrorMsgWk[512];

    if ((fHandleType==SQL_HANDLE_ENV  &&  ((LPENV )handle)                      ->isMerant311) ||
        (fHandleType==SQL_HANDLE_DBC  &&  ((LPDBC )handle)           ->penvOwner->isMerant311) ||
        (fHandleType==SQL_HANDLE_STMT &&  ((LPSTMT)handle)->pdbcOwner->penvOwner->isMerant311) ||
        (fHandleType==SQL_HANDLE_DESC &&  ((LPDESC)handle)     ->pdbc->penvOwner->isMerant311))
                              /* call ASCII entry point if Merant 3.11*/
        return SQLGetDiagRec_InternalCall(fHandleType, handle, iRecord,
                       (SQLCHAR*)szSqlState, pfNativeError,
                       (SQLCHAR*)szErrorMsg, cbErrorMsgMax, pcbErrorMsg);

    rc = SQLGetDiagRec_InternalCall(fHandleType, handle, iRecord,
                       (SQLCHAR*)szSqlStateWk, pfNativeError,
                       (SQLCHAR*)szErrorMsgWk, sizeof(szErrorMsgWk), pcbErrorMsg);
    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    if (szSqlState)          /* move SQLSTATE back */
       {
        rc2 = ConvertCharToWChar(NULL,
             szSqlStateWk,    sizeof(szSqlStateWk),                      /* source */
             szWideSqlStateWk,sizeof(szWideSqlStateWk)/sizeof(SQLWCHAR), /* target */
             NULL, NULL, NULL);
        if (SQL_SUCCEEDED(rc2))
            memcpy(szSqlState, szWideSqlStateWk, (SQL_SQLSTATE_SIZE+1)*sizeof(SQLWCHAR));
       }

    if (SQL_SUCCEEDED(rc2))
        rc2 = ConvertCharToWChar(NULL,   /* move Error Message Text back */
         szErrorMsgWk,    SQL_NTS     ,  /* source */
         szErrorMsg,      cbErrorMsgMax, /* target */
         pcbErrorMsg, NULL, NULL);

    if (rc2 != SQL_SUCCESS)   /* upgrade the msg level */
        rc =  rc2;

    return rc;
}



#endif


SQLRETURN SQL_API SQLPrepareW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szWideSqlStr,
    SQLINTEGER       cbWideSqlStr)   /*   count of chars */
{
    LPSTMT           pstmt = (LPSTMT)hstmt;

    if (pstmt->pdbcOwner->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLPrepare_InternalCall(hstmt,
                                      (SQLCHAR*)szWideSqlStr, cbWideSqlStr,
                                       FALSE);

    return ExecDirectOrPrepareW(hstmt, szWideSqlStr, cbWideSqlStr,
                                SQL_API_SQLPREPARE);  /* call common code */
}


SQLRETURN SQL_API SQLSetConnectAttrW(
    SQLHDBC          hdbc,
    SQLINTEGER       fAttribute,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValue)        /*   count of bytes */
{
    LPDBC            pdbc = (LPDBC)hdbc;
    SQLRETURN        rc;
    SQLCHAR          szName1Buf[OBJECT_NAME_SIZE];
    SQLCHAR         *szName1 = szName1Buf;
    SQLSMALLINT      cbName1 = 0;

    if (pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLSetConnectAttr_InternalCall(hdbc,
                                         fAttribute, rgbValue, cbValue);

    if (fAttribute == SQL_ATTR_CURRENT_CATALOG  ||
        fAttribute == SQL_ATTR_TRACEFILE        ||
        fAttribute == SQL_ATTR_TRANSLATE_LIB)
       {
       if (cbValue > 0)     /* convert count of bytes  to count of chars */
           cbValue /= sizeof(SQLWCHAR);
       CatalogNameConversion(rgbValue, (SQLSMALLINT)cbValue, &szName1, &cbName1);
       if (szName1)
           rgbValue = szName1;
       cbValue  = cbName1;
       }

    rc = SQLSetConnectAttr_InternalCall(hdbc, fAttribute, rgbValue, cbValue);
    return(rc);
}

SQLRETURN SQL_API SQLSetCursorNameW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCursor,
    SQLSMALLINT      cbCursor)       /*   count of chars */
{
    return(CatalogFunction(
                     SQL_API_SQLSETCURSORNAME,
                     hstmt,
                     szCursor,  cbCursor,
                     NULL,0,  NULL,0,  NULL,0,  NULL,0,  NULL,0,
                     0,0,0));
}


SQLRETURN SQL_API SQLColumnsW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLWCHAR        *szColumnName,
    SQLSMALLINT      cbColumnName)
{
    return(CatalogFunction(
                     SQL_API_SQLCOLUMNS,
                     hstmt,
                     szCatalogName,  cbCatalogName,
                     szSchemaName,   cbSchemaName,
                     szTableName,    cbTableName,
                     szColumnName,   cbColumnName,
                     NULL,0,  NULL,0,
                     0,0,0));
}


SQLRETURN SQL_API SQLGetConnectOptionW(
    SQLHDBC          hdbc,
    SQLUSMALLINT     fOption,
    SQLPOINTER       pvParam)
{
    SQLINTEGER StringLength;
    SQLINTEGER BufferLength=GetAttrLength(fOption);

    return (SQLGetConnectAttr_InternalCall(hdbc, fOption, pvParam, 
        BufferLength, &StringLength));
}


SQLRETURN SQL_API SQLGetInfoW(
    SQLHDBC          hdbc,
    SQLUSMALLINT     fInfoType,
    SQLPOINTER       rgbInfoValue,
    SQLSMALLINT      cbInfoValueMax, /*   count of bytes */
    SQLSMALLINT     *pcbInfoValue)   /* ->count of bytes */
{
    LPDBC   pdbc = (LPDBC)hdbc;
    char    szInfoValue[100]="0";
    RETCODE rc, rc2;

    /* if Unicode entry point of SQLGetInfoW was called with 
       a buffer size of only 6 bytes then we know it's old Merant 3.11 DM */
    if (fInfoType == SQL_DRIVER_ODBC_VER  &&  cbInfoValueMax == 6)
        pdbc->penvOwner->isMerant311 = TRUE;

    if (pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLGetInfo_InternalCall(pdbc, fInfoType,
                        rgbInfoValue, cbInfoValueMax, pcbInfoValue);

    switch (fInfoType)
    {
    case SQL_ACCESSIBLE_PROCEDURES:  /* marshall the string values */
    case SQL_COLUMN_ALIAS:
    case SQL_COLLATION_SEQ:
    case SQL_DRIVER_VER:
    case SQL_EXPRESSIONS_IN_ORDERBY:
    case SQL_IDENTIFIER_QUOTE_CHAR:
    case SQL_KEYWORDS:
    case SQL_LIKE_ESCAPE_CLAUSE:
    case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
    case SQL_MULT_RESULT_SETS:
    case SQL_MULTIPLE_ACTIVE_TXN:
    case SQL_NEED_LONG_DATA_LEN:
    case SQL_ODBC_SQL_OPT_IEF:
    case SQL_ORDER_BY_COLUMNS_IN_SELECT:
    case SQL_OWNER_TERM:
    case SQL_PROCEDURE_TERM:
    case SQL_PROCEDURES:
    case SQL_QUALIFIER_NAME_SEPARATOR:
    case SQL_QUALIFIER_TERM:
    case SQL_ROW_UPDATES:
    case SQL_SEARCH_PATTERN_ESCAPE:
    case SQL_SPECIAL_CHARACTERS:
    case SQL_TABLE_TERM:
    case SQL_OUTER_JOINS:
    case SQL_DRIVER_ODBC_VER:
    case SQL_DRIVER_NAME:
    case SQL_DATA_SOURCE_NAME:
    case SQL_SERVER_NAME:
    case SQL_DATABASE_NAME:
    case SQL_DBMS_NAME:
    case SQL_DBMS_VER:
    case SQL_ACCESSIBLE_TABLES:
    case SQL_DATA_SOURCE_READ_ONLY:
    case SQL_USER_NAME:
    case SQL_XOPEN_CLI_YEAR:

        rc = SQLGetInfo_InternalCall(pdbc, fInfoType,
                        szInfoValue, sizeof(szInfoValue), pcbInfoValue);

        if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
            return rc;

        rc2 = ConvertCharToWChar(pdbc,
                 szInfoValue, SQL_NTS,                                  /* source */
                 rgbInfoValue,(SWORD)(cbInfoValueMax/sizeof(SQLWCHAR)), /* target */
                 pcbInfoValue, NULL, NULL);

        if (rc2 != SQL_SUCCESS)
            rc = rc2;

        if (pcbInfoValue && (rc == SQL_SUCCESS  ||  rc == SQL_SUCCESS_WITH_INFO))
           *pcbInfoValue = (SQLSMALLINT)(*pcbInfoValue * sizeof(SQLWCHAR));
                                             /* return length as byte length */

        break;

    default:
        rc = SQLGetInfo_InternalCall(pdbc, fInfoType,
                          rgbInfoValue, cbInfoValueMax, pcbInfoValue);
    }   /* end switch */

    return rc;
}


SQLRETURN SQL_API SQLGetTypeInfoW(
    SQLHSTMT         hstmt,
    SQLSMALLINT      DataType)
{
    return(SQLGetTypeInfo_InternalCall(hstmt, DataType));
}


SQLRETURN SQL_API SQLSetConnectOptionW(
    SQLHDBC          hdbc,
    SQLUSMALLINT     fOption,
    SQLUINTEGER          vParam)
{
    SQLINTEGER StringLength = SQL_NTS;

    return(SQLSetConnectAttr_InternalCall(hdbc, fOption, 
                 (SQLPOINTER)(SCALARP)vParam, StringLength));
}



SQLRETURN SQL_API SQLSpecialColumnsW(
    SQLHSTMT         hstmt,
    SQLUSMALLINT     fColType,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLUSMALLINT     fScope,
    SQLUSMALLINT     fNullable)
{
    return(CatalogFunction(
                     SQL_API_SQLSPECIALCOLUMNS,
                     hstmt,
                     szCatalogName,  cbCatalogName,
                     szSchemaName,   cbSchemaName,
                     szTableName,    cbTableName,
                     NULL,0,  NULL,0,  NULL,0,
                     fColType, fScope, fNullable));
}


SQLRETURN SQL_API SQLStatisticsW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLUSMALLINT     fUnique,
    SQLUSMALLINT     fAccuracy)
{
    return(CatalogFunction(
                     SQL_API_SQLSTATISTICS,
                     hstmt,
                     szCatalogName,  cbCatalogName,
                     szSchemaName,   cbSchemaName,
                     szTableName,    cbTableName,
                     NULL,0,  NULL,0,  NULL,0,
                     0, fUnique, fAccuracy));
}


SQLRETURN SQL_API SQLTablesW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLWCHAR        *szTableType,
    SQLSMALLINT      cbTableType)
{
    return(CatalogFunction(
                     SQL_API_SQLTABLES,
                     hstmt,
                     szCatalogName,  cbCatalogName,
                     szSchemaName,   cbSchemaName,
                     szTableName,    cbTableName,
                     szTableType,    cbTableType,
                     NULL,0,  NULL,0,
                     0,0,0));
}


SQLRETURN SQL_API SQLDataSourcesW(
    SQLHENV          henv,
    SQLUSMALLINT     fDirection,
    SQLWCHAR        *szDSN,
    SQLSMALLINT      cbDSNMax,
    SQLSMALLINT     *pcbDSN,
    SQLWCHAR        *szDescription,
    SQLSMALLINT      cbDescriptionMax,
    SQLSMALLINT     *pcbDescription);


SQLRETURN SQL_API SQLDriverConnectW(
    SQLHDBC          hdbc,
    SQLHWND          hwnd,
    SQLWCHAR        *szWideConnStrIn,
    SQLSMALLINT      cbWideConnStrIn,     /*   count of chars; bytes in SQLDriverConnectA */
    SQLWCHAR        *szWideConnStrOut,
    SQLSMALLINT      cbWideConnStrOutMax, /*   count of chars; bytes in SQLDriverConnectA */
    SQLSMALLINT     *pcbWideConnStrOut,   /* ->count of chars */
    SQLUSMALLINT     fDriverCompletion)
{
    LPDBC       pdbc=(LPDBC)hdbc;
    CHAR        szConnStrIn[1096];
    SWORD       cbConnStrIn;
    CHAR        szConnStrOut[1096];
    SWORD       cbConnStrOut;
    SWORD       cbWideConnStrInOrig = cbWideConnStrIn;
    RETCODE     rc, rc2;


    if (pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLDriverConnect_InternalCall(hdbc, hwnd,
                                      (SQLCHAR*)szWideConnStrIn,  cbWideConnStrIn,
                                      (SQLCHAR*)szWideConnStrOut, cbWideConnStrOutMax,
                                      pcbWideConnStrOut, fDriverCompletion);

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    ErrResetDbc (pdbc);
    ResetDbc (pdbc);

    rc = ConvertWCharToChar(pdbc,
                        szWideConnStrIn, cbWideConnStrIn,  /* source */
                        szConnStrIn, sizeof(szConnStrIn),  /* target */
                        &cbConnStrIn, NULL, NULL);
    if (rc != SQL_SUCCESS)
       return rc;

    if (cbWideConnStrInOrig == SQL_NTS)    /* if user passed NTS, we pass NTS */
        cbConnStrIn          = SQL_NTS;

    UnlockDbc(pdbc);

    rc = SQLDriverConnect_InternalCall(hdbc, hwnd,
                                      (SQLCHAR*)szConnStrIn, cbConnStrIn,
                                      (SQLCHAR*)szConnStrOut, sizeof(szConnStrOut),
                                      &cbConnStrOut, fDriverCompletion);
    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    rc2 = ConvertCharToWChar(pdbc,
                         szConnStrOut, SQL_NTS,                 /* source */
                         szWideConnStrOut,cbWideConnStrOutMax,  /* target */
                         pcbWideConnStrOut, NULL, NULL);
    if (rc2 != SQL_SUCCESS)
        rc = rc2;

    return(rc);
}

SQLRETURN SQL_API SQLBrowseConnectW(
    SQLHDBC          hdbc,
    SQLWCHAR        *szWideConnStrIn,
    SQLSMALLINT      cbWideConnStrIn,
    SQLWCHAR        *szWideConnStrOut,
    SQLSMALLINT      cbWideConnStrOutMax,
    SQLSMALLINT     *pcbWideConnStrOut)
{
    LPDBC       pdbc=(LPDBC)hdbc;
    CHAR        szConnStrIn[1096];
    SWORD       cbConnStrIn;
    CHAR        szConnStrOut[1096];
    SWORD       cbConnStrOut;
    SWORD       cbWideConnStrInOrig = cbWideConnStrIn;
    RETCODE     rc, rc2;

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    ErrResetDbc (pdbc);
    ResetDbc (pdbc);

    rc = ConvertWCharToChar(pdbc,
                        szWideConnStrIn, cbWideConnStrIn,  /* source */
                        szConnStrIn, sizeof(szConnStrIn),  /* target */
                        &cbConnStrIn, NULL, NULL);
    if (rc != SQL_SUCCESS)
       return rc;

    if (cbWideConnStrInOrig == SQL_NTS)    /* if user passed NTS, we pass NTS */
        cbConnStrIn          = SQL_NTS;

    UnlockDbc(pdbc);

    rc = SQLBrowseConnect_InternalCall(hdbc, (SQLCHAR*)szConnStrIn, 
        cbConnStrIn, (SQLCHAR*)szConnStrOut, sizeof(szConnStrOut),
        &cbConnStrOut);

    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO && rc != SQL_NEED_DATA)
        return rc;

    rc2 = ConvertCharToWChar(pdbc,
                         szConnStrOut, SQL_NTS,                 /* source */
                         szWideConnStrOut,cbWideConnStrOutMax,  /* target */
                         pcbWideConnStrOut, NULL, NULL);
    if (rc2 != SQL_SUCCESS)
        rc = rc2;

    return(rc);
}

SQLRETURN SQL_API SQLColumnPrivilegesW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName,
    SQLWCHAR        *szColumnName,
    SQLSMALLINT      cbColumnName)
{
    return(CatalogFunction(
                     SQL_API_SQLCOLUMNPRIVILEGES,
                     hstmt,
                     szCatalogName, cbCatalogName,
                     szSchemaName,  cbSchemaName,
                     szTableName,    cbTableName,
                     szColumnName,  cbColumnName,  
                     NULL,0,  NULL,0,
                     0,0,0));
}

SQLRETURN SQL_API SQLGetStmtAttrW(
    SQLHSTMT         hstmt,
    SQLINTEGER       fAttribute,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValueMax,      /*   count of bytes */
    SQLINTEGER      *pcbValue)        /* ->count of bytes */
{
    return (SQLGetStmtAttr_InternalCall(hstmt, fAttribute,
                                        rgbValue, cbValueMax, pcbValue));
}


SQLRETURN SQL_API SQLSetStmtAttrW(
    SQLHSTMT         hstmt,
    SQLINTEGER       fAttribute,
    SQLPOINTER       rgbValue,
    SQLINTEGER       cbValueMax)
{
    return (SQLSetStmtAttr_InternalCall(hstmt, fAttribute,
                                        rgbValue, cbValueMax));
}


SQLRETURN SQL_API SQLForeignKeysW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szPkCatalogName,
    SQLSMALLINT      cbPkCatalogName, /*   count of chars; bytes in SQLForeignKeysA */
    SQLWCHAR        *szPkSchemaName,
    SQLSMALLINT      cbPkSchemaName,  /*   count of chars; bytes in SQLForeignKeysA */
    SQLWCHAR        *szPkTableName,
    SQLSMALLINT      cbPkTableName,
    SQLWCHAR        *szFkCatalogName,
    SQLSMALLINT      cbFkCatalogName,
    SQLWCHAR        *szFkSchemaName,
    SQLSMALLINT      cbFkSchemaName,
    SQLWCHAR        *szFkTableName,
    SQLSMALLINT      cbFkTableName)
{
    return(CatalogFunction(
                     SQL_API_SQLFOREIGNKEYS,
                     hstmt,
                     szPkCatalogName, cbPkCatalogName,
                     szPkSchemaName,  cbPkSchemaName,
                     szPkTableName,   cbPkTableName,
                     szFkCatalogName, cbFkCatalogName,
                     szFkSchemaName,  cbFkSchemaName,
                     szFkTableName,   cbFkTableName,
                     0,0,0));
}



SQLRETURN SQL_API SQLNativeSqlW(
    SQLHDBC          hdbc,
    SQLWCHAR        *szWideSqlStrIn,
    SQLINTEGER       cbWideSqlStrIn,      /*   count of chars */
    SQLWCHAR        *szWideSqlStrOut,
    SQLINTEGER       cbWideSqlStrMax,     /*   count of chars; bytes in SQLNativeSqlA */
    SQLINTEGER      *pcbWideSqlStr)       /* ->count of chars */
{
    LPDBC       pdbc=(LPDBC)hdbc;
    CHAR        szSqlStrIn[5000];
    SQLINTEGER  cbSqlStrIn;
    CHAR        szSqlStrOut[5000];
    SQLINTEGER  cbWideSqlStrInOrig = cbWideSqlStrIn;
    RETCODE     rc, rc2;

    if (pdbc->penvOwner->isMerant311)
                              /* call ASCII entry point if Merant 3.11*/
        return SQLNativeSql_InternalCall(hdbc,
                            (SQLCHAR*)szWideSqlStrIn,  cbWideSqlStrIn,
                            (SQLCHAR*)szWideSqlStrOut, cbWideSqlStrMax,
                            pcbWideSqlStr);

    if (!LockDbc (pdbc)) return SQL_INVALID_HANDLE;

    ErrResetDbc (pdbc);

    rc = ConvertWCharToChar(pdbc,
                        szWideSqlStrIn, cbWideSqlStrIn,  /* source */
                        szSqlStrIn, sizeof(szSqlStrIn),  /* target */
                        NULL, &cbSqlStrIn, NULL);
    if (rc != SQL_SUCCESS)
       return rc;

    if (cbWideSqlStrInOrig == SQL_NTS)    /* if user passed NTS, we pass NTS */
        cbSqlStrIn          = SQL_NTS;

    UnlockDbc(pdbc);

    rc = SQLNativeSql_InternalCall(hdbc,
                            (SQLCHAR*)szSqlStrIn, cbSqlStrIn,
                            (SQLCHAR*)szSqlStrOut, sizeof(szSqlStrOut),
                            pcbWideSqlStr);
    if (rc != SQL_SUCCESS  &&  rc != SQL_SUCCESS_WITH_INFO)
        return rc;

    rc2 = ConvertCharToWChar(pdbc,
                         szSqlStrOut, SQL_NTS,             /* source */
                         szWideSqlStrOut,cbWideSqlStrMax,  /* target */
                         NULL, pcbWideSqlStr, NULL);
    if (rc2 != SQL_SUCCESS)
        rc = rc2;

    return(rc);
}




SQLRETURN SQL_API SQLPrimaryKeysW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,   /*   count of chars */
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,    /*   count of chars */
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName)     /*   count of chars */
{
    return(CatalogFunction(
                     SQL_API_SQLPRIMARYKEYS,
                     hstmt,
                     szCatalogName, cbCatalogName,
                     szSchemaName,  cbSchemaName,
                     szTableName,   cbTableName,
                     NULL,0,  NULL,0,  NULL,0,
                     0,0,0));
}

SQLRETURN SQL_API SQLProcedureColumnsW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szProcName,
    SQLSMALLINT      cbProcName,
    SQLWCHAR        *szColumnName,
    SQLSMALLINT      cbColumnName)
{
    LPSTMT           pstmt = (LPSTMT)hstmt;

    return(CatalogFunction(
                     SQL_API_SQLPROCEDURECOLUMNS,
                     hstmt,
                     szCatalogName, cbCatalogName,
                     szSchemaName,  cbSchemaName,
                     szProcName,    cbProcName,
                     szColumnName,  cbColumnName,
                     NULL,0,  NULL,0,
                     0,0,0));
}


SQLRETURN SQL_API SQLProceduresW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,   /*   count of chars; bytes in SQLProceduresA */
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,    /*   count of chars; bytes in SQLProceduresA */
    SQLWCHAR        *szProcName,
    SQLSMALLINT      cbProcName)      /*   count of chars; bytes in SQLProceduresA */
{
    return(CatalogFunction(
                     SQL_API_SQLPROCEDURES,
                     hstmt,
                     szCatalogName, cbCatalogName,
                     szSchemaName,  cbSchemaName,
                     szProcName,    cbProcName,
                     NULL,0,  NULL,0,  NULL,0,
                     0,0,0));
}



SQLRETURN SQL_API SQLTablePrivilegesW(
    SQLHSTMT         hstmt,
    SQLWCHAR        *szCatalogName,
    SQLSMALLINT      cbCatalogName,
    SQLWCHAR        *szSchemaName,
    SQLSMALLINT      cbSchemaName,
    SQLWCHAR        *szTableName,
    SQLSMALLINT      cbTableName)
{
    return(CatalogFunction(
                     SQL_API_SQLTABLEPRIVILEGES,
                     hstmt,
                     szCatalogName, cbCatalogName,
                     szSchemaName,  cbSchemaName,
                     szTableName,    cbTableName,
                     NULL,0,  NULL,0,  NULL,0,
                     0,0,0));
}

SQLRETURN SQL_API SQLDriversW(
    SQLHENV          henv,
    SQLUSMALLINT     fDirection,
    SQLWCHAR        *szDriverDesc,
    SQLSMALLINT      cbDriverDescMax,
    SQLSMALLINT     *pcbDriverDesc,
    SQLWCHAR        *szDriverAttributes,
    SQLSMALLINT      cbDrvrAttrMax,
    SQLSMALLINT     *pcbDrvrAttr);

