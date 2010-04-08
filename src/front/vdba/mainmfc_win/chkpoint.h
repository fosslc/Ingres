/*****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : chkpoint.h, header file.
**
**
**    Project  : Ingres II / Visual DBA.
**    Author   : Schalk Philippe
**
**    Purpose  : data definition for the CaCheckPoint and CaCheckPointList
**               class.
*******************************************************************************/
#if !defined(AFX_CHKPOINT_H__FDEC4865_D1F0_11D5_B656_00C04F1790C3__INCLUDED_)
#define AFX_CHKPOINT_H__FDEC4865_D1F0_11D5_B656_00C04F1790C3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CaCheckPoint : public CObject
{
public:
	CaCheckPoint(): m_csDate(""), m_csCkp_sequence(""), m_csFirst_jnl("") ,
	                m_csLast_jnl(""), m_csValid(""), m_csMode(""){}
	CaCheckPoint(LPCTSTR lpDate,LPCTSTR lpCkpSeq,LPCTSTR lpFirstjnl,LPCTSTR lpLastjnl,
	             LPCTSTR lpValid,LPCTSTR lpMode): m_csDate(lpDate), m_csCkp_sequence(lpCkpSeq),
	                                              m_csFirst_jnl(lpFirstjnl) , m_csLast_jnl(lpLastjnl),
	                                              m_csValid(lpValid), m_csMode(lpMode){}
	virtual ~CaCheckPoint(){}

	LPCTSTR GetCheckSequence() {return m_csCkp_sequence;}
	LPCTSTR GetDate ()         {return m_csDate;}
	LPCTSTR GetFirstJnl ()     {return m_csFirst_jnl;}
	LPCTSTR GetLastJnl ()      {return m_csLast_jnl;}
	LPCTSTR GetValid ()        {return m_csValid;}
	LPCTSTR GetMode ()         {return m_csMode;}

protected:
	CString m_csDate;
	CString m_csCkp_sequence;
	CString m_csFirst_jnl;
	CString m_csLast_jnl;
	CString m_csValid;
	CString m_csMode;

};

class CaCheckPointList : public CObject
{
public:
	CaCheckPointList() ;
	void CaCheckPointList::AddCheckPoint(CString csDate, CString csCkpSequence,
	                                     CString csFirstJnl, CString csLastJnl,
	                                     CString csValid, CString csMode);
	virtual ~CaCheckPointList();
	CaCheckPoint* CaCheckPointList::Find(LPCTSTR lpCheckPointNum);
	int RetrieveCheckPointList(); // launch remote command and fill m_CheckPointList
	
	CTypedPtrList<CPtrList, CaCheckPoint*>& GetCheckPointList() {return m_CheckPointList;}

	CString m_csVnode_Name;
	CString m_csDB_Name;
	CString m_csDB_Owner;

protected:
	CString m_csStatus;
	CString m_csCurrent_CheckPoint_Sequence;
	CString m_csCurrent_Journal_Sequence;

	CString m_csField1;
	CString m_csField2;
	CString m_csField3;
	CString m_csField4;
	CString m_csField5;
	CString m_csField6;
	CString m_csField7;

	CTypedPtrList<CPtrList, CaCheckPoint*> m_CheckPointList;

};

#endif // !defined(AFX_CHKPOINT_H__FDEC4865_D1F0_11D5_B656_00C04F1790C3__INCLUDED_)
