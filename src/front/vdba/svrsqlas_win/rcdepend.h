/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : rcdepend.h, Header File.
**    Project  : COM Server SQL Assistant
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Depend on Application and Resource.
**
** History:
**
** 25-Oct-2000 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
** 19-Dec-2001 (uk$so01)
**    SIR #106648, Split vdba into the small component ActiveX/COM
**/



#if !defined (APPLICATION_AND_RESOURCE_DEPENDEND_HEADER)
#define APPLICATION_AND_RESOURCE_DEPENDEND_HEADER
#include "svrsqlas.h"
#include "rctools.h"
extern CSvrsqlasApp theApp;

#define IDB_SQL_MAIN       IDB_IMPORTAS_1
#define IDB_SQL_SELECT1    IDB_IMPORTAS_1
#define IDB_SQL_SELECT2    IDB_IMPORTAS_1
#define IDB_SQL_SELECT3    IDB_IMPORTAS_1
#define IDB_SQL_UPDATE1    IDB_IMPORTAS_1
#define IDB_SQL_UPDATE2    IDB_IMPORTAS_1
#define IDB_SQL_UPDATE3    IDB_IMPORTAS_1
#define IDB_SQL_INSERT1    IDB_IMPORTAS_1
#define IDB_SQL_INSERT2    IDB_IMPORTAS_1
#define IDB_SQL_INSERT3    IDB_IMPORTAS_1
#define IDB_SQL_DELETE1    IDB_IMPORTAS_1

#endif // APPLICATION_AND_RESOURCE_DEPENDEND_HEADER
