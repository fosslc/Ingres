/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : cbfprdta.cpp, Implementation File 
**    Project  : OpenIngres Configuration Manager (Origin IPM Project)
**    Author   : UK Sotheavut
**    Purpose  : The implementation of serialization of CBF Page (modeless dialog)
**
** History:
**
** xx-Sep-1997: (uk$so01) 
**    created
** 06-Jun-2000: (uk$so01) 
**    (BUG #99242)
**    Cleanup code for DBCS compliant
**
**/

#include "stdafx.h"
#include "cbfprdta.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_SERIAL (CuCbfPropertyData, CObject, 1)
CuCbfPropertyData::CuCbfPropertyData(){m_strClassName = _T("");}
CuCbfPropertyData::CuCbfPropertyData(LPCTSTR lpszClassName){m_strClassName = lpszClassName;}

void CuCbfPropertyData::NotifyLoad (CWnd* pDlg){}  
void CuCbfPropertyData::Serialize (CArchive& ar){}

