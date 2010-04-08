/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qrecnitm.cpp, Implementation file
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : OLE Support for (CRichEditDoc)
**
** History:
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "qredoc.h"
#include "qreview.h"
#include "qrecnitm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_SERIAL(CuRichCtrlCntrItem, CRichEditCntrItem, 0)

CuRichCtrlCntrItem::CuRichCtrlCntrItem(REOBJECT* preo, CdSqlQueryRichEditDoc* pContainer)
	: CRichEditCntrItem(preo, pContainer)
{
	// TODO: add one-time construction code here
	
}

CuRichCtrlCntrItem::~CuRichCtrlCntrItem()
{
	// TODO: add cleanup code here
	
}

/////////////////////////////////////////////////////////////////////////////
// CuRichCtrlCntrItem diagnostics

#ifdef _DEBUG
void CuRichCtrlCntrItem::AssertValid() const
{
	CRichEditCntrItem::AssertValid();
}

void CuRichCtrlCntrItem::Dump(CDumpContext& dc) const
{
	CRichEditCntrItem::Dump(dc);
}
#endif

