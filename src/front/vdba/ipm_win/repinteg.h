/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
** Source   : repinteg.h, Header File
** Project  : Ingres II/ (Vdba Monitoring)
** Author   : Philippe SCHALK
** Purpose  : The data for Replication : Integrity Object
** 
** History:
**
** xx-Jan-1998 (Philippe SCHALK)
**    Created.
*/

#if !defined (REPINTEGRITY_HEADER)
#define REPINTEGRITY_HEADER

class CaReplicIntegrityRegTable: public CObject
{
	DECLARE_SERIAL (CaReplicIntegrityRegTable)
public:
	int GetCdds_No();
	CaReplicIntegrityRegTable()
		:m_strTable_Name(""),m_strOwner_name(""),m_strLookupTable_Name(""),m_nTable_no (-1), m_bValid(TRUE)	{}
	
	CaReplicIntegrityRegTable(LPCTSTR lpszTbl_Name,LPCTSTR lpszOwner_name,LPCTSTR lpszLookupTable_Name,
							  LPCTSTR lpszDbName  ,LPCTSTR lpszVnodeName ,int iNodeHandle,int iRepVer,
							  int iTbl_no,int iCddsNo);
	
	CaReplicIntegrityRegTable(LPCTSTR lpszErrTxt)
		:m_strTable_Name(lpszErrTxt),m_strOwner_name(""),m_strLookupTable_Name(""),m_nTable_no (0), m_bValid(FALSE)	{}
	
	virtual ~CaReplicIntegrityRegTable(){}
	virtual void Serialize (CArchive& ar);

// Access to members
	BOOL IsValidTbl() { return m_bValid; }

	CString GetTable_Name() { return m_strTable_Name; }
	CString GetOwner_name() { return m_strOwner_name; }
	CString GetLookupTable_Name() { return m_strLookupTable_Name; }
	CString GetDbName()		{return m_strDbName;}
	CString GetVnodeName()	{return m_strVnodeName;}
	int		GetNodeHandle() {return m_nNodeHandle;}
	int		GetTable_no()	{return m_nTable_no;}
	int		GetReplicVersion()	{return m_nReplicVersion;}
private:
	int		m_nCdds_No;
	CString m_strDbName;
	CString m_strVnodeName;
	int		m_nNodeHandle;
	int		m_nReplicVersion;

	CString m_strTable_Name;
	CString m_strOwner_name;
	CString m_strLookupTable_Name;
	int		m_nTable_no;
	BOOL	m_bValid;
};


class CaReplicCommon: public CObject
{
	DECLARE_SERIAL (CaReplicCommon)

public:
	CaReplicCommon():m_strValue(_T("")),m_nNumberValue(-1){}
	CaReplicCommon(LPCTSTR lpszName,int iNo)
		:m_strValue(lpszName),m_nNumberValue(iNo){}
	virtual ~CaReplicCommon(){}
	virtual void Serialize (CArchive& ar);
// Access to members
	CString GetCommonString()	{ return m_strValue; }
	int		GetCommonNumber()	{ return m_nNumberValue; }
private:
	CString m_strValue;
	int		m_nNumberValue;
};



class CaReplicCommonList: public CTypedPtrList < CObList,CaReplicCommon * >
{
	DECLARE_SERIAL (CaReplicCommonList)
public:
	CaReplicCommonList(){}
	virtual ~CaReplicCommonList();
	void ClearList();
};


class CaReplicIntegrityRegTableList: public CTypedPtrList < CObList,CaReplicIntegrityRegTable * >
{
	DECLARE_SERIAL (CaReplicIntegrityRegTableList)
public:
	void ClearList();
	CaReplicIntegrityRegTableList(){}
	virtual ~CaReplicIntegrityRegTableList();
	//virtual void Serialize (CArchive& ar);
};
#endif