/*
** Copyright (c) 2010 Ingres Corporation 
*/ 

#include <compat.h>
#include <mu.h>
#include <qu.h>
#include <st.h>
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <iiodbc.h>
#include <iiodbcfn.h>
#include "odbclocal.h"
#include "tracefn.h"

/* 
** Name: descript.c - Descriptor functions for the ODBC CLI.
** 
** Description: 
** 		This file defines: 
** 
**              SQLColAttributes - Get column Attributes (ODBC 2.0 version).
**              SQLColAttribute - Get column Attributes.
**              SQLCopyDesc - Copy column descriptor.
**              SQLGetDescField - Get a descriptor item.
**              SQLGetDescRec - Get a column descriptor.
**              SQLSetDescFielda - Set a descriptor item.
**              SQLSetDescRec - Set a column descriptor.
**
** History:
**    30-Apr-04 (loera01)
**        Created.
**    12-May-04 (loera01)
**        In SQLColAttributes, map deprecated field identifiers to their
**        ODBC 3.0 counterparts before calling SQLColAttribute().
**    18-May-04 (loera01)
**        In SQLColAttributes(), remove extraneous argument to 
**        SQLColAttribute().
**    04-Oct-04 (hweho01)
**        Avoid compiler error on AIX platform, put include
**        files of sql.h and sqlext.h after st.h.
**    15-Jul-2005 (hanje04)
**        Include iiodbcfn.h and tracefn.h which are the new home for the 
**        ODBC CLI function pointer definitions.
**    03-Sep-2010 (Ralph Loen) Bug 124348
**        Replaced SQLINTEGER, SQLUINTEGER and SQLPOINTER arguments with
**        SQLLEN, SQLULEN and SQLLEN * for compatibility with 64-bit
**        platforms.
*/ 

/*
** Name: 	SQLColAttributes - Get column attributes
** 
** Description: 
**	        SQLColAttributes returns descriptor information for a 
**              column in a result set.  This is the ODBC 2.0 version of 
**              SQLColAttribute().
** Inputs:
**              StatementHandle - Statement handle.
**              ColumnNumber - The record number in the row descriptor.
**              FieldIdentifier - Field type to be returned.
**              BufferLength - Maximum length of returned attribute.
**
** Outputs: 
**              ValuePtrParm - Returned field attribute.
**              StringLengthPtr - Returned attribute length.
**              pfDesc - Returned column attribute.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLCHAR messageText[512];
SQLCHAR sqlState[6];

SQLRETURN  SQL_API SQLColAttributes (
    SQLHSTMT     StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier,
    SQLPOINTER   ValuePtrParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLLEN      *pfDesc)
{
    RETCODE rc, traceRet = 1;
    pSTMT pstmt = (pSTMT)StatementHandle;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLColAttributes(StatementHandle,
        ColumnNumber, FieldIdentifier, ValuePtrParm, BufferLength,
        StringLengthPtr, pfDesc), traceRet);

    if (validHandle(StatementHandle, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(pstmt, SQL_HANDLE_STMT);

    switch(FieldIdentifier)
    {
        case SQL_COLUMN_NULLABLE:
            FieldIdentifier = SQL_DESC_NULLABLE;
            break;

        case SQL_COLUMN_COUNT:
            FieldIdentifier = SQL_DESC_COUNT;
            break;

        case SQL_COLUMN_NAME:
            FieldIdentifier = SQL_DESC_NAME;
            break;

    }

    rc =  IIColAttribute(
        pstmt->hdr.driverHandle,
        ColumnNumber,
        FieldIdentifier,
        ValuePtrParm,
        BufferLength,
        StringLengthPtr,
        (SQLPOINTER)pfDesc);

    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
         pstmt->hdr.driverError = TRUE;
         pstmt->errHdr.rc = rc;
    }

    releaseLock(SQL_HANDLE_STMT, pstmt);

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLColAttribute - Get column attributes
** 
** Description: 
**	        SQLColAttribute returns descriptor information for a 
**              column in a result set.  This is the ODBC 3.x (non-deprecated)
**              version.
** Inputs:
**              StatementHandle - Statement handle.
**              ColumnNumber - The record number in the row descriptor.
**              FieldIdentifier - Field type to be returned.
**              BufferLength - Maximum length of returned attribute.
**
** Outputs: 
**              ValuePtrParm - Returned field attribute.
**              StringLengthPtr - Returned attribute length.
**              pfDesc - Returned column attribute.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 


SQLRETURN  SQL_API SQLColAttribute (
    SQLHSTMT     StatementHandle,
    SQLUSMALLINT ColumnNumber,
    SQLUSMALLINT FieldIdentifier,
    SQLPOINTER   ValuePtrParm,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLLEN      *NumericAttributePtr)
{
    pSTMT pstmt = (pSTMT)StatementHandle;
    RETCODE rc, traceRet = 1;;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLColAttribute(StatementHandle,
         ColumnNumber, FieldIdentifier, ValuePtrParm, BufferLength, 
		 StringLengthPtr, NumericAttributePtr), traceRet);

    if (validHandle(pstmt, SQL_HANDLE_STMT) != SQL_SUCCESS)
    {
        ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    resetErrorBuff(pstmt, SQL_HANDLE_STMT);

    rc = IIColAttribute(pstmt->hdr.driverHandle,
        ColumnNumber,
        FieldIdentifier,
        ValuePtrParm,
        BufferLength,
        StringLengthPtr,
        NumericAttributePtr);


    applyLock(SQL_HANDLE_STMT, pstmt);
    if (rc != SQL_SUCCESS)
    {
        pstmt->hdr.driverError = TRUE;
        pstmt->errHdr.rc = rc;
    }
    releaseLock(SQL_HANDLE_STMT, pstmt);

    ODBC_TRACE(ODBC_TR_TRACE,IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLCopyDesc - Copy a descriptor.
** 
** Description: 
**              SQLCopyDesc copies descriptor information from one 
**              descriptor handle to another.
**
** Inputs:
**              SourceDescHandle - Source descriptor handle.
**
** Outputs: 
**              TargetDescHandle - Destination descriptor handle.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 
SQLRETURN  SQL_API SQLCopyDesc (
    SQLHDESC SourceDescHandle,
    SQLHDESC TargetDescHandle)
{
    pDESC pdesc;

    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLCopyDesc(SourceDescHandle, 
        TargetDescHandle), traceRet);

    if (!SourceDescHandle)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    if (!TargetDescHandle)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    pdesc = getDescHandle(SourceDescHandle);
    if (pdesc)
    {
        if (validHandle(pdesc, SQL_HANDLE_DESC) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        resetErrorBuff(pdesc, SQL_HANDLE_DESC);
        SourceDescHandle = pdesc->hdr.driverHandle;
    }

    rc = IICopyDesc(SourceDescHandle, 
         TargetDescHandle);


    if (pdesc)
        applyLock(SQL_HANDLE_DESC, pdesc);
    if (rc != SQL_SUCCESS )
    {
        if (pdesc != NULL)
        {
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc;
        }
    }
        
    if (pdesc)
        releaseLock(SQL_HANDLE_DESC, pdesc);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetDescField - Get a descriptor field.
** 
** Description: 
**              SQLGetDescField returns the current setting or value of a 
**              single field of a descriptor record.
** Inputs:
**              DescriptorHandle - Descriptor handle.
**              RecNumber - Descriptor record.
**              FieldIdentifier - Field whose value is to be returned.
**              BufferLength - Maximum buffer length.
**
** Outputs: 
**              ValuePtr - Returned descriptor information.
**              StringLengthPtr - Returned buffer length.
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN  SQL_API SQLGetDescField (
    SQLHDESC    DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER  ValuePtr,
    SQLINTEGER  BufferLength,
    SQLINTEGER *StringLengthPtr)
{
    pDESC pdesc = NULL;
    RETCODE rc, traceRet = 1;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetDescField(DescriptorHandle,
         RecNumber, FieldIdentifier, ValuePtr, BufferLength,
         StringLengthPtr), traceRet);

    if (!DescriptorHandle)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    pdesc = getDescHandle(DescriptorHandle);
    if (pdesc)
    {
        if (validHandle(pdesc, SQL_HANDLE_DESC) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, 
                SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        resetErrorBuff(pdesc, SQL_HANDLE_DESC);
        DescriptorHandle = pdesc->hdr.driverHandle;
    }

    rc = IIGetDescField(DescriptorHandle,
       RecNumber,
       FieldIdentifier,
       ValuePtr,
       BufferLength,
       StringLengthPtr);


    if (pdesc)
        applyLock(SQL_HANDLE_DESC, pdesc);
    if (rc != SQL_SUCCESS )
    {
        if (pdesc != NULL)
        {
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc;
        }
    }
    if (pdesc)
        releaseLock(SQL_HANDLE_DESC, pdesc);

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLGetDescRec - Return descriptor metadata
** 
** Description: 
**              SQLGetDescRec returns the current settings or values of 
**              multiple fields of a descriptor record. The fields returned 
**              describe the name, data type, and storage of column or 
**              parameter data.
** Inputs:
**              DescriptorHandle - Descriptor handle.
**              RecNumber - Descriptor record number.
**              BufferLength - Maximum length of the Name buffer.
**
** Outputs: 
**              Name - SQL_DESC_NAME field buffer.
**              StringLengthPtr - Length of the Name Buffer.
**              TypePtr - SQL_DESC_TYPE field buffer.
**              SubTypePtr - SQL_DESC_DATETIME_INTERVAL_CODE field buffer.
**              LengthPtr - SQL_DESC_OCTET_LENGTH field buffer.
**              PrecisionPtr - SQL_DESC_PRECISION field buffer.
**              ScalePtr - SQL_DESC_SCALE field buffer.
**              NullablePtr - SQL_DESC_NULLABLE field buffer.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 
SQLRETURN  SQL_API SQLGetDescRec (
    SQLHDESC     DescriptorHandle,
    SQLSMALLINT  RecNumber,
    SQLCHAR     *Name,
    SQLSMALLINT  BufferLength,
    SQLSMALLINT *StringLengthPtr,
    SQLSMALLINT *TypePtr,
    SQLSMALLINT *SubTypePtr,
    SQLLEN      *LengthPtr,
    SQLSMALLINT *PrecisionPtr,
    SQLSMALLINT *ScalePtr,
    SQLSMALLINT *NullablePtr)
{
    RETCODE rc, traceRet = 1;
    pDESC pdesc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLGetDescRec( DescriptorHandle, 
          RecNumber, Name, BufferLength, StringLengthPtr, TypePtr, SubTypePtr,
          LengthPtr, PrecisionPtr, ScalePtr, NullablePtr), traceRet);

    if (!DescriptorHandle)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    pdesc = getDescHandle(DescriptorHandle);
    if (pdesc)
    {
        if (validHandle(pdesc, SQL_HANDLE_DESC) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        resetErrorBuff(pdesc, SQL_HANDLE_DESC);
        DescriptorHandle = pdesc->hdr.driverHandle;
    }

    rc = IIGetDescRec(DescriptorHandle,
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

    if (rc != SQL_SUCCESS )
    {
        if (pdesc != NULL)
        {

            applyLock(SQL_HANDLE_DESC, pdesc);
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc;
            releaseLock(SQL_HANDLE_DESC, pdesc);
        }
    }

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	SQLSetDescField - Set a descriptor field.
** 
** Description: 
**              SQLSetDescField sets the value of a single field of a 
**              descriptor record.
**
** Inputs:
**              DescriptorHandle - Descriptor handle.
**              RecNumber - Descriptor record number.
**              FieldIdentifier - Descriptor field to be set.
**              ValuePtr - Descriptor field buffer
**              BufferLength - Length of descriptor field buffer.
**
**
** Outputs: 
**              None.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

SQLRETURN  SQL_API SQLSetDescField (
    SQLHDESC    DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT FieldIdentifier,
    SQLPOINTER  ValuePtr,
    SQLINTEGER  BufferLength)
{
    RETCODE rc, traceRet = 1;
    pDESC pdesc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetDescField(DescriptorHandle,
         RecNumber, FieldIdentifier, ValuePtr, BufferLength), traceRet);

    if (!DescriptorHandle)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    pdesc = getDescHandle(DescriptorHandle);
    if (pdesc)
    {
        if (validHandle(pdesc, SQL_HANDLE_DESC) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        resetErrorBuff(pdesc, SQL_HANDLE_DESC);
        DescriptorHandle = pdesc->hdr.driverHandle;
    }

    rc = IISetDescField(DescriptorHandle,
       RecNumber,
       FieldIdentifier,
       ValuePtr,
       BufferLength);

    if (rc != SQL_SUCCESS )
    {
        if (pdesc != NULL)
        {

            applyLock(SQL_HANDLE_DESC, pdesc);
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc;
            releaseLock(SQL_HANDLE_DESC, pdesc);
        }
    }

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;

}

/*
** Name: 	SQLSetDesRec - SetDescriptor data.
** 
** Description: 
**              The SQLSetDescRec function sets multiple descriptor fields 
**              that affect the data type and buffer bound to a column or 
**              parameter data.
**
** Inputs:
**
**              DescriptorHandle - Descriptor handle.
**              RecNumber - Descriptor record number.
**              VerboseType - SQL_DESC_TYPE buffer.
**              SubType - SQL_DESC_DATETIME_INTERVAL_CODE buffer.
**              Length - SQL_DESC_OCTET_LENGTH buffer.
**              Precision - SQL_DESC_PRECISION buffer.
**              Scale - SQL_DESC_SCALE buffer.
**
** Outputs: 
**              DataPtr - SQL_DESC_DATA_PTR buffer.
**              StringLengthPtr - SQL_DESC_OCTET_LENGTH_PTR field pointer.
**              IndicatorPtr - SQL_DESC_INDICATOR_PTR field pointer.
**
** Returns: 
**              SQL_SUCCESS
**              SQL_SUCCESS_WITH_INFO
**              SQL_ERROR
**              SQL_INVALID_HANDLE
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 
SQLRETURN  SQL_API SQLSetDescRec (
    SQLHDESC    DescriptorHandle,
    SQLSMALLINT RecNumber,
    SQLSMALLINT VerboseType,
    SQLSMALLINT SubType,
    SQLLEN      Length,
    SQLSMALLINT Precision,
    SQLSMALLINT Scale,
    SQLPOINTER  DataPtr,
    SQLLEN  *StringLengthPtr,
    SQLLEN  *IndicatorPtr)
{
    RETCODE rc, traceRet = 1;
    pDESC pdesc;

    ODBC_TRACE_ENTRY(ODBC_TR_TRACE, IITraceSQLSetDescRec (DescriptorHandle,
       RecNumber, VerboseType, SubType, Length, Precision, Scale,
       DataPtr, StringLengthPtr, IndicatorPtr), traceRet);

    if (!DescriptorHandle)
    {
        ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
        return SQL_INVALID_HANDLE;
    }

    pdesc = getDescHandle(DescriptorHandle);
    if (pdesc)
    {
        if (validHandle(pdesc, SQL_HANDLE_DESC) != SQL_SUCCESS)
        {
            ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, SQL_INVALID_HANDLE));
            return SQL_INVALID_HANDLE;
        }

        resetErrorBuff(pdesc, SQL_HANDLE_DESC);
        DescriptorHandle = pdesc->hdr.driverHandle;
    }

    rc = IISetDescRec(DescriptorHandle,
       RecNumber,
	   VerboseType,
	   SubType,
	   Length,
	   Precision,
	   Scale,
	   DataPtr,
	   StringLengthPtr,
	   IndicatorPtr);
       
    if (rc != SQL_SUCCESS )
    {
        if (pdesc != NULL)
        {
            applyLock(SQL_HANDLE_DESC, pdesc);
            pdesc->hdr.driverError = TRUE;
            pdesc->errHdr.rc = rc;
            releaseLock(SQL_HANDLE_DESC, pdesc);
        }
    }

    ODBC_TRACE(ODBC_TR_TRACE, IITraceReturn(traceRet, rc));
    return rc;
}

/*
** Name: 	getDescHandle - get an ODBC driver descriptor handle.
** 
** Description: 
**              Get a driver descriptor handle from the CLI cache.  The
**              CLI may allocate external descriptor handles, but these
**              are associated with actual driver descriptor handles
**              that represent a tuple descriptor.
**
** Inputs:
**              handle - CLI descriptor handle.
**
** Outputs: 
**              None.
**
** Returns: 
**              Driver descriptor handle or NULL.
**
** Exceptions: 
**       
**              N/A 
** 
** Side Effects: 
** 
**              None.
**
** History: 
**    14-jun-04 (loera01)
**      Created.
*/ 

pDESC getDescHandle( SQLHANDLE handle )
{
    QUEUE *q;
    pDESC pdesc;

    if (handle)
    {
        for (q = IIodbc_cb.desc_q.q_prev; q != &IIodbc_cb.desc_q; q = q->q_prev)
        {
            pdesc = (pDESC)q;
            if (pdesc->hdr.driverHandle == handle)
                return pdesc;
        }
    }
    return NULL;
}
