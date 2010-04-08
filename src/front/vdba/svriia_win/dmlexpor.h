/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlexpor.h: header file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manipulation of data to be exported
**
** History:
**
** 16-Jan-2001 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**    Created
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
**/

#if !defined (DMLEXPORT_DATA_HEADER)
#define DMLEXPORT_DATA_HEADER
#include "exportdt.h"

BOOL MatchColumn (CaColumn* pCol1, CaColumn* pCol2);
void ConfigureLayout (ExportedFileType expt, CaQueryResult& result, CTypedPtrList< CObList, CaColumn* >* pListLeader);
BOOL IEA_FetchRows (CaIEAInfo* pIEAInfo, HWND hwndCaller);
BOOL ExportToCsv(CaIEAInfo* pIEAInfo);
BOOL ExportToDbf(CaIEAInfo* pIEAInfo);
BOOL ExportToXml(CaIEAInfo* pIEAInfo);
BOOL ExportToFixedWidth(CaIEAInfo* pIEAInfo);
BOOL ExportToCopyStatement(CaIEAInfo* pIEAInfo);
BOOL ExportInSilent(CaIEAInfo& data);

#endif // DMLEXPORT_DATA_HEADER