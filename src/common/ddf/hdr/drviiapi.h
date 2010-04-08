/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: drviiapi.h - Database Driver for Ingres
**
** Description:
**  This file contains driver internal structures for the DDI.
**    These stuctures are Ingres dependent.
**
** History: 
**      02-mar-98 (marol01)
**          Created
**    29-Jul-1998 (fanra01)
**        Removed unused union in the OI_QUERY structure.
**    01-Apr-1999 (fanra01)
**        Add a pre-increment flag to handle blobs within a result row.
**        Group column must be pre-incremented following a regular type
**        and post incremented following a blob type or at the beginning
**        of the row.
**    16-Mar-2001 (fanra01)
**        Bug 104298
**        Add rowset and max memory size as parameters for block selects to
**        prepared statements.
**        Add fields, rowsReturned and currentRow, into the select structure
**        to maintain position of row set processing.
**/

#ifndef DDF_IIAPI_INCLUDED
#define DDF_IIAPI_INCLUDED

#include <ddfcom.h>
#include <iiapi.h>
#include <drviiutil.h>
#include <dbaccess.h>

typedef struct __OI_GROUP {
    i4      cColumns;           /* total no of columns to be fetched */
    i4      cColumnGroups;      /* number of structs in agcp */
    i4      *groupFirst;        /* table of the first property of the group */
    i4      cBlobs;             /* number of blob columns */
    i4      grppreinc;          /* Flag for moving to  next column group for
                                ** blobs.
                                ** This handles the case for blobs appearing
                                ** at the start or within a result set.
                                ** Pre-increment following regular types
                                ** otherwise post increment.
                                */
    char*   pRowBuffer;         /* ptr to start of row data */
    IIAPI_GETCOLPARM*   agcp;   /* structs passed to getColumns() */
} OI_GROUP;

typedef struct __OI_SESSION 
{
    i4        timeout;
    II_PTR    *connHandle;
    II_PTR    *tranHandle;
} OI_SESSION, *POI_SESSION;

typedef struct __OI_QUERY 
{
    POI_SESSION         session;
    u_i4                type;
    char                name[NUM_MAX_ADD_STR + 4];
    char                *text;
    POI_PARAM           params;
    u_i4                rowcount;
    II_INT2             numberOfParams;
    IIAPI_GETDESCRPARM  getDescrParm;
    IIAPI_QUERYPARM     queryParm;
    struct __OI_SELECT
    {
        II_INT2         status;
        OI_GROUP        *groups;
        i4              group_counter;
        i4              rowsReturned;
        i4              currentRow;
        i4              minrowset;
        i4              maxsetsize;
    } select;
} OI_QUERY, *POI_QUERY;

GSTATUS 
OIRepOIStruct (
    OI_Param **last,
    u_i4  type);

GSTATUS 
OIInitQuery    (
    POI_QUERY qry);

GSTATUS 
OI_RunExecute ( 
    POI_SESSION oisession,
    POI_QUERY oiquery );

GSTATUS 
OI_RunOpen    (
    POI_SESSION oisession,
    POI_QUERY oiquery );

#endif
