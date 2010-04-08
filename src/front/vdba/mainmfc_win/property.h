/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : property.h : header file
**    Project  : Vdba 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manage OLE Properties.
**
** History:
**
** 13-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
**    Created
**/


#if !defined(OLE_PROPERTIES_MANAGER_HEADER)
#define OLE_PROPERTIES_MANAGER_HEADER
#include "compfile.h"

void LoadSqlQueryPreferences(IUnknown* pUnk); // Initialize the GLobal StreamInit of SqlQuery
void LoadIpmPreferences(IUnknown* pUnk);      // Initialize the GLobal StreamInit of Ipm

#define AXCONTROL_SQL  1
#define AXCONTROL_IPM  2
void GetActiveControlUnknow(CPtrArray& arrayUnknown, UINT nCtrl);


#endif // OLE_PROPERTIES_MANAGER_HEADER