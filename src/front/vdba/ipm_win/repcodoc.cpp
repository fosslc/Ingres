/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : epcodoc.cpp, Implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Document for Frame/View the page of Replication Page
**
** History:
**
** xx-May-1998 (uk$so01)
**    Created
*/

#include "stdafx.h"
#include "repcodoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CdReplicationPageCollisionDoc ::CdReplicationPageCollisionDoc ()
{
	m_pCollisionData = NULL;
}

BOOL CdReplicationPageCollisionDoc ::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	return TRUE;
}

CdReplicationPageCollisionDoc ::~CdReplicationPageCollisionDoc ()
{
}


BEGIN_MESSAGE_MAP(CdReplicationPageCollisionDoc , CDocument)
	//{{AFX_MSG_MAP(CdReplicationPageCollisionDoc )
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdReplicationPageCollisionDoc  diagnostics

#ifdef _DEBUG
void CdReplicationPageCollisionDoc ::AssertValid() const
{
	CDocument::AssertValid();
}

void CdReplicationPageCollisionDoc ::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdReplicationPageCollisionDoc  serialization

void CdReplicationPageCollisionDoc ::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CdReplicationPageCollisionDoc  commands
