/********************************************************************
//
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
//
//    Project : CA/OpenIngres Visual DBA
//
//    Source : error.h
//    system errors
//
**  26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
********************************************************************/

#define ERR_OPEN_NODE        1
#define ERR_CLOSE_NODE       2
#define ERR_UPDATEDOMDATA    3
#define ERR_UPDATEDOMDATA1   4
#define ERR_ALLOCMEM         5
#define ERR_DOMDATA          6
#define ERR_DOMGETDATA       7
#define ERR_PARENTNOEXIST    8
#define ERR_LEVELNUMBER      9
#define ERR_ADDOBJECT       10
#define ERR_OBJECTNOEXISTS  11
#define ERR_FREE            12
#define ERR_LIST            13
#define ERR_ISDOMDATANEEDED 14
#define ERR_SESSION         15
#define ERR_TIME            16
#define ERR_SETTINGS        17
#define ERR_REFRESH_WIN     18
#define ERR_RMCLI           19
#define ERR_BKRF            20
#define ERR_PROPSCACHE      21
#define ERR_PROPAGATE       22
#define ERR_WILDCARD        23
#define ERR_REPLICTYPE      24
#define ERR_REPLICCDDS      25
#define ERR_GW              26

#define ERR_INVALIDPARAMS   28
#define ERR_MONITORTYPE     29
#define ERR_MONITOREQUIV    30
#define ERR_FILE            31
#define ERR_REGEVENTS       32
#define ERR_QEP             33
#define ERR_INGRESPAGES     34
#define ERR_DBCS			35
#define ERR_OLDCODE			36

/////////////////////////////////////////////////////
//   Error defines for SQL tools From 1100 to 1199
#define ERR_NOTSELECTSTM   1100
#define ERR_INVPARAMS      1101
#define ERR_INVFORMAT      1102
/////////////////////////////////////////////////////

extern int  gMemErrFlag;
// values for gMemErrFlag - "out of memory" error management
#define MEMERR_STANDARD   0
#define MEMERR_NOMESSAGE  1
#define MEMERR_HAPPENED   2

int myerror(int ierr);

