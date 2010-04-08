/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : snaparam.cpp, implementation File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store parameters.
**           
** History:
**
** 18-Nov-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#include "stdafx.h"
#include "snaparam.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




//
// Object: CaVsdStoreSchema 
// ************************************************************************************************
IMPLEMENT_SERIAL (CaVsdStoreSchema, CaLLQueryInfo, 1)
void CaVsdStoreSchema::Serialize (CArchive& ar)
{
	ASSERT(FALSE);
}

void CaVsdStoreSchema::ShowAnimateTextInfo(LPCTSTR lpszText)
{
	if (m_hWndAnimateDlg)
	{
		LPTSTR lpszMsg = new TCHAR[_tcslen(lpszText) + 1];
		lstrcpy (lpszMsg, lpszText);
		::PostMessage (m_hWndAnimateDlg, WM_EXECUTE_TASK, W_EXTRA_TEXTINFO, (LPARAM)lpszMsg);
	}
}

