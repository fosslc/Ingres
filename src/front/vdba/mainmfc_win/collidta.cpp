/*****************************************************************************
**
** Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**
**  Project  : Visual DBA
**
**    Author   : UK Sotheavut
**
**    Source   : collidta.cpp, Implementation file
**
**    Purpose  : Modeless Dialog to display the conflict rows
**
**  History after 04-May-1999:
**
**    23-Jul-1999 (schph01)
**      bug #97682
**      Add variable m_nSvrTargetType in CaCollisionSequence::serialize()
**      method.
**    20-Sep-1999 (schph01)
**      bug #98655
**      Add variable m_nSequenceNo,m_nSourceDB et m_nTblNo in
**      CaCollisionSequence::serialize() method.
**    22-Sep-1999 (schph01)
**      bug #98838
**      Remove method CaCollisionTransaction::Find (long nID) because it is
**      possible that it have several sequences with the same number.
**      Remove method CaCollision::Find (long nTransID, long nSequenceID) never
**      called in vdba,this method calling therefore CaCollisionTransaction::Find (long nID).
**    24-Sep-1999 (schph01)
**      SIR #98839
**      Add Methode CaCollisionTransaction::SequenceWithSameNumber()
**
******************************************************************************/
#include "stdafx.h"
#include "collidta.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




//
// COLLISION ITEM DATA:
// *******************
IMPLEMENT_SERIAL (CaCollisionItem, CObject, 1)
void CaCollisionItem::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_nImageType;
		ar << m_nID;
		ar << m_bResolved;
		ar << m_bExpanded;
		ar << m_bSelected;
	}
	else
	{
		ar >> m_nImageType;
		ar >> m_nID;
		ar >> m_bResolved;
		ar >> m_bExpanded;
		ar >> m_bSelected;
	}
}



//
// COLLISION COLUMN:
// *****************
IMPLEMENT_SERIAL (CaCollisionColumn, CObject, 1)
void CaCollisionColumn::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_nKey;
		ar << m_strColumn;
		ar << m_strSource;
		ar << m_strTarget;
		ar << m_bConflicted;
	}
	else
	{
		ar >> m_nKey;
		ar >> m_strColumn;
		ar >> m_strSource;
		ar >> m_strTarget;
		ar >> m_bConflicted;
	}
}




//
// COLLISION SEQUENCE:
// *******************
IMPLEMENT_SERIAL (CaCollisionSequence, CaCollisionItem, 4)

CaCollisionSequence::~CaCollisionSequence()
{
	while (!m_listColumns.IsEmpty())
		delete m_listColumns.RemoveTail();
}

void CaCollisionSequence::Serialize (CArchive& ar)
{
	CaCollisionItem::Serialize(ar);
	m_listColumns.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_nCDDS;
		ar << m_nTransaction;
		ar << m_nPrevTransaction;

		ar << m_strSourceTransTime;
		ar << m_strTargetTransTime;
		ar << m_strCollisionType;

		ar << m_strSourceVNode;
		ar << m_strTargetVNode;
		ar << m_strLocalVNode;
		ar << m_strSourceDatabase;
		ar << m_strTargetDatabase;
		ar << m_strLocalDatabase;

		ar << m_strTable;
		ar << m_strTableOwner;
		ar << m_nSvrTargetType;
		ar << m_nTblNo;
		ar << m_nSequenceNo;
		ar << m_nSourceDB;
	}
	else
	{
		ar >> m_nCDDS;
		ar >> m_nTransaction;
		ar >> m_nPrevTransaction;

		ar >> m_strSourceTransTime;
		ar >> m_strTargetTransTime;
		ar >> m_strCollisionType;

		ar >> m_strSourceVNode;
		ar >> m_strTargetVNode;
		ar >> m_strLocalVNode;
		ar >> m_strSourceDatabase;
		ar >> m_strTargetDatabase;
		ar >> m_strLocalDatabase;

		ar >> m_strTable;
		ar >> m_strTableOwner;
		ar >> m_nSvrTargetType;
		ar >> m_nTblNo;
		ar >> m_nSequenceNo;
		ar >> m_nSourceDB;
	}
}

void CaCollisionSequence::AddColumn(LPCTSTR lpszColumn, LPCTSTR lpszSource, LPCTSTR lpszTarget, int nKey, BOOL bConflicted)
{
	CaCollisionColumn* pColumn = new CaCollisionColumn (lpszColumn, lpszSource, lpszTarget, nKey);
	pColumn->SetConflicted (bConflicted);
	m_listColumns.AddTail(pColumn);
}



//
// COLLISION TRANSACTION:
// **********************
IMPLEMENT_SERIAL (CaCollisionTransaction, CaCollisionItem, 1)

CaCollisionTransaction::~CaCollisionTransaction()
{
	while (!m_listCollisionSequence.IsEmpty())
		delete m_listCollisionSequence.RemoveTail();
}

void CaCollisionTransaction::Serialize (CArchive& ar)
{
	CaCollisionItem::Serialize(ar);
	m_listCollisionSequence.Serialize (ar);
}

void CaCollisionTransaction::AddCollisionSequence (CaCollisionSequence* pNewSequence)
{
	ASSERT (pNewSequence);
	if (pNewSequence)
		m_listCollisionSequence.AddTail (pNewSequence);
}

BOOL CaCollisionTransaction::SequenceWithSameNumber(long CurSeqID)
{
	ASSERT(CurSeqID>=0);
	int iNbSeq = 0;
	CaCollisionSequence* pSequence = NULL;
	POSITION pos = m_listCollisionSequence.GetHeadPosition();

	while (pos != NULL)
	{
		pSequence = m_listCollisionSequence.GetNext (pos);
		if (pSequence && pSequence->GetID() == CurSeqID)
			iNbSeq++;
		if (iNbSeq == 2)
			return TRUE;
	}
	return FALSE;
}

//
// GLOBAL COLLISION:
// *****************
IMPLEMENT_SERIAL (CaCollision, CObject, 1)
CaCollision::~CaCollision()
{
	Cleanup();
}

void CaCollision::Cleanup()
{
	while (!m_listTransaction.IsEmpty())
		delete m_listTransaction.RemoveTail();
}

void CaCollision::Serialize (CArchive& ar)
{
	m_listTransaction.Serialize (ar);
}

void CaCollision::AddTransaction (long nTransID)
{
	if (!Find(nTransID))
	{
		CaCollisionTransaction* pNewTrans = new CaCollisionTransaction(nTransID);
		m_listTransaction.AddTail (pNewTrans);
	}
}

void CaCollision::AddCollisionSequence (long nTransID, CaCollisionSequence* pNewSequence)
{
	CaCollisionTransaction* pTrans = Find(nTransID);
	if (!pTrans)
	{
		CaCollisionTransaction* pTrans = new CaCollisionTransaction(nTransID);
		m_listTransaction.AddTail (pTrans);
	}
	
	if (pTrans)
		pTrans->AddCollisionSequence (pNewSequence);
}

CaCollisionTransaction* CaCollision::Find (long nTransID)
{
	CaCollisionTransaction* pTrans = NULL;
	POSITION pos = m_listTransaction.GetHeadPosition();

	while (pos != NULL)
	{
		pTrans = m_listTransaction.GetNext (pos);
		if (pTrans && pTrans->GetID() == nTransID)
			return pTrans;
	}
	return NULL;
}
