/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vtreedat.cpp : implementation file
**    Project  : INGRESII / Monitoring.
**    Author   : Emmanuel Blattes 
**    Purpose  : Tree data
**
** History:
**
** 07-Feb-1997 (Emmanuel Blattes)
**    Created
*/

#include "stdafx.h"
#include "vtreedat.h"
#include "ipmdoc.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// declaration for serialization
IMPLEMENT_SERIAL ( CTreeItemData, CObject, 3)

// "normal" constructor
CTreeItemData::CTreeItemData(void *pstruct, int structsize, CdIpmDoc* pDoc, CString name)
{
	ASSERT (pstruct);
	ASSERT (structsize);

	m_name = name;
	m_size = structsize;
	m_pstruct = NULL;
	if (pstruct && structsize) {
		m_pstruct = new char[structsize];
		ASSERT(m_pstruct);
		if (m_pstruct)
			memcpy(m_pstruct, pstruct, structsize);
	}
	m_pIpmDoc = pDoc;
}

// copy constructor
CTreeItemData::CTreeItemData(const CTreeItemData &itemData)
{
	m_size = itemData.m_size;
	m_pstruct = new char[m_size];
	ASSERT(m_pstruct);
	if (m_pstruct)
		memcpy(m_pstruct, itemData.m_pstruct, m_size);
	m_pIpmDoc = itemData.m_pIpmDoc;
	m_name  = itemData.m_name;
}

// assignment operator
CTreeItemData &CTreeItemData::operator=( CTreeItemData &itemData )
{
	m_size = itemData.m_size;
	m_pstruct = new char[m_size];
	ASSERT(m_pstruct);
	if (m_pstruct)
		memcpy(m_pstruct, itemData.m_pstruct, m_size);
	m_pIpmDoc = itemData.m_pIpmDoc;
	m_name  = itemData.m_name;

	return *this;  // Assignment operator returns left side.
}

// serialization constructor
CTreeItemData::CTreeItemData()
{
}

// destructor
CTreeItemData::~CTreeItemData()
{
	if (m_pstruct)
		delete m_pstruct;
}

// serialization method
void CTreeItemData::Serialize (CArchive& ar)
{
	if (ar.IsStoring()) 
	{
		ar << m_size;
		ar.Write(m_pstruct, m_size);
		ar << m_name;
	}
	else 
	{
		ar >> m_size;
		m_pstruct = new char[m_size];   // will throw a memory exception if out of memory
		ar.Read(m_pstruct, m_size);
		ar >> m_name;
	}
}


CString CTreeItemData::GetItemName(int objType)
{
	return m_name;
}

// replace structure "as is" - no delete/new to keep same address,
// which is needed for right pane refresh in monitor window
BOOL CTreeItemData::SetNewStructure(void* newpstruct)
{
	ASSERT (newpstruct);
	ASSERT (m_size);
	ASSERT(m_pstruct);

	if (!newpstruct)
		return FALSE;
	if (!m_size)
		return FALSE;
	if (!m_pstruct)
		return FALSE;

	memcpy(m_pstruct, newpstruct, m_size);
	return TRUE;
}

