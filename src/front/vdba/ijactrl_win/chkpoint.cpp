/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/
/*
**
**    Source   : chkpoint.cpp, implementation file
**
**
**    Project  : Ingres Journal Analyser
**    Author   : Schalk Philippe
**
**    Purpose  : implementation of the CaCheckPoint and CaCheckPointList class.
**
** History:
**
** 02-Dec-2003 (schph01)
**    SIR #111389 add in project for displayed the checkpoint list.
**/
#include "stdafx.h"
#include "chkpoint.h"
#include "remotecm.h"

//////////////////////////////////////////////////////////////////////
// CaCheckPointList Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CaCheckPointList::CaCheckPointList()
{
	m_csVnode_Name                  = _T("");
	m_csDB_Name                     = _T("");
	m_csCurrent_CheckPoint_Sequence = _T("");
	m_csCurrent_Journal_Sequence    = _T("");

	// initialize fields researched.
	m_csField1 = _T("    Status   : ");
	m_csField2 = _T("    Checkpoint sequence : ");
	m_csField3 = _T("----Checkpoint History for Journal----------------------------------------------");
	m_csField4 = _T("    Date                      Ckp_sequence  First_jnl   Last_jnl  valid  mode");
    m_csField5 = _T("    ----------------------------------------------------------------------------");
	m_csField6 = _T("----Checkpoint History for Dump");
	m_csField7 = _T("    None");

}

CaCheckPointList::~CaCheckPointList()
{
	while (!m_CheckPointList.IsEmpty())
		delete m_CheckPointList.RemoveTail();
}

CaCheckPoint* CaCheckPointList::Find(LPCTSTR lpCheckPointNum)
{
	CaCheckPoint* pCheckPt = NULL;
	POSITION pos = m_CheckPointList.GetHeadPosition();

	while (pos != NULL)
	{
		pCheckPt = m_CheckPointList.GetNext (pos);
		if (pCheckPt && pCheckPt->GetCheckSequence() == lpCheckPointNum)
			return pCheckPt;
	}
	return NULL;
}

void CaCheckPointList::AddCheckPoint(CString csDate, CString csCkpSequence,
                                     CString csFirstJnl, CString csLastJnl,
                                     CString csValid, CString csMode)
{
	if (!Find(csCkpSequence))
	{
		CaCheckPoint* pNewChecPt = new CaCheckPoint( csDate,csCkpSequence, csFirstJnl,
													csLastJnl, csValid, csMode);
		m_CheckPointList.AddTail (pNewChecPt);
	}
}


int CaCheckPointList::RetrieveCheckPointList()
{
	char szD1[10], szD2[10], szD3[10], szD4[10], szD5[10], szD6[10],szD7[10],szD8[10],szD9[10], szD10[10];
	char * CurLine, *pc;
	int iNb;
	CString csCommandLine ,csDate;
	CWaitCursor wait;
	if ( m_csDB_Name.IsEmpty())
		return FALSE;
	csCommandLine.Format("infodb %s -u%s ",m_csDB_Name,m_csDB_Owner);

	if (!ExecRmcmdInBackground((LPTSTR)(LPCTSTR)m_csVnode_Name , (LPTSTR)(LPCTSTR)csCommandLine, ""))
		return FALSE;

	CurLine=GetFirstTraceLine();
	if (!CurLine)
		return FALSE;
	while (CurLine && _tcsncicmp(CurLine,m_csField1,m_csField1.GetLength())) 
		CurLine = GetNextTraceLine();
	if (!CurLine)
		return FALSE;
	pc = CurLine + m_csField1.GetLength();
	m_csStatus = pc;

	while (CurLine && _tcsncicmp(CurLine,m_csField2,m_csField2.GetLength())) 
		CurLine = GetNextTraceLine();
	if (!CurLine)
		return FALSE;
	int iChkpnt,iJnlSeq,ires;
	char * szJnlInf = _T("    Checkpoint sequence : %d    Journal sequence : %d");
	pc = CurLine + m_csField2.GetLength();
	ires = sscanf(CurLine,szJnlInf,&iChkpnt,&iJnlSeq);
	if (ires == 2) {
		m_csCurrent_CheckPoint_Sequence.Format ("%d", iChkpnt);
		m_csCurrent_Journal_Sequence.Format("%d",iJnlSeq);
	}
	while (CurLine && _tcsncicmp(CurLine,m_csField3,m_csField3.GetLength())) 
		CurLine = GetNextTraceLine();
	if (!CurLine)
		return FALSE;
	CurLine = GetNextTraceLine();
	if (!CurLine || _tcsncicmp(CurLine,m_csField4,m_csField4.GetLength()) != 0)
		return FALSE;
	CurLine = GetNextTraceLine();
	if (!CurLine || _tcsncicmp(CurLine,m_csField5,m_csField5.GetLength()) != 0)
		return FALSE;
	CurLine = GetNextTraceLine();
	if (!CurLine || !_tcsncicmp(CurLine,m_csField7,m_csField7.GetLength()) != 0)
		return FALSE;
	while (CurLine && _tcsncicmp(CurLine,m_csField6,m_csField6.GetLength()) != 0) 
	{
		iNb = sscanf(CurLine,"%s %s %s %s %s %s %s %s %s %s",szD1, szD2, szD3,
		                                                     szD4, szD5, szD6,
		                                                     szD7,szD8,szD9, szD10);
		if (iNb != 10)
			return FALSE;
		csDate.Format("%s %s %s %s %s",szD1, szD2, szD3, szD4, szD5);
		AddCheckPoint(csDate, szD6,szD7,szD8,szD9, szD10);
		CurLine = GetNextTraceLine();
	}

	return TRUE;
}
