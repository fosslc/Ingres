/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : repitem.h, Header File
**    Project  : VDBA / Monitoring
**    Author   : Philippe SCHALK
**    Purpose  : The data for Replication Object
**
** History:
**
** xx-Feb-1997 (Philippe SCHALK)
**    Created
*/


#if !defined (REPITEM_HEADER)
#define REPITEM_HEADER


class CaReplicationItem: public CObject
{
	DECLARE_SERIAL (CaReplicationItem)
public:
	CaReplicationItem(): m_strDescription(_T("")), m_strFlagContent(_T("")), m_strUnit(_T("")), m_bIsDefault(FALSE), m_nType(REP_STRING)
		{
			m_bDisplay = FALSE;
			m_nMin     = -1;
			m_nMax     = -1;
			m_bIsDefaultMandatoryInFile = FALSE;
			m_bIsMandatory = FALSE;
		}
	CaReplicationItem(
		LPCTSTR lpszDescription, 
		LPCTSTR lpszFlagContent, 
		LPCTSTR lpszUnit, 
		LPCTSTR lpszFlagName, 
		int nType = CaReplicationItem::REP_STRING,
		LPCTSTR lpszDefault = NULL)
		:m_strDescription(lpszDescription), m_strFlagContent(lpszFlagContent), m_strUnit(lpszUnit)
		{
			m_strDefault  = lpszDefault? lpszDefault: _T("");
			m_strFlagName = lpszFlagName;
			m_bIsDefault  = FALSE;
			m_nType       = nType;
			m_bDisplay    = TRUE;
			m_nMin        = -1;
			m_nMax        = -1;
			m_bIsDefaultMandatoryInFile = FALSE;
			m_bIsMandatory = FALSE;
			m_bValueModifyByUser = FALSE;
			m_bReadOnlyFlag = FALSE;      
			if (m_nType == REP_BOOLEAN)
				ASSERT (m_strFlagContent == _T("FALSE") || m_strFlagContent == _T("TRUE"));
		}
	virtual ~CaReplicationItem(){}
	virtual void Serialize (CArchive& ar);

	//
	// Operations
	// ----------
	BOOL IsDefault(){return m_bIsDefault;}
	BOOL IsDisplay(){return m_bDisplay;}
	BOOL IsMandatory () {return m_bIsMandatory;}
	BOOL IsDefaultMandatoryInFile() {return m_bIsDefaultMandatoryInFile;}
	BOOL IsValueModifyByUser() {return m_bValueModifyByUser;}
	BOOL IsReadOnlyFlag () {return m_bReadOnlyFlag;}
	//
	// Return the value. (Flag value of the col 1)
	// The boolean bDefault is set to TRUE if the value returning by this function is a default value.
	CString GetFlagContent(BOOL& bDefault) {bDefault = m_bIsDefault; return m_strFlagContent; }
	int GetType(){return m_nType;} 
	CString GetFlagName() {return m_strFlagName;}
	CString GetDescription() {return m_strDescription;}
	CString GetDefaultValue(){return m_strDefault;}
	CString GetUnit(){return m_strUnit;}
	
	void GetMinMax(int& nMin, int& nMax){ASSERT (m_nType == REP_NUMBER); nMin = m_nMin; nMax = m_nMax;}
	
	void SetDisplay (BOOL bDisplay = TRUE){m_bDisplay = bDisplay;}
	void SetFlagContent(LPCTSTR lpszNewValue);
	void SetFlagName(LPCTSTR lpszNewFlagName);
	void SetToDefault(){m_strFlagContent = m_strDefault; m_bIsDefault = TRUE;}
	void SetMinMax (int nMin, int nMax){ASSERT (m_nType == REP_NUMBER); m_nMin = nMin; m_nMax = nMax;}
	void SetMandatory () {m_bIsMandatory = TRUE;}
	void SetDefaultMandatoryInFile() {m_bIsDefaultMandatoryInFile = TRUE;}
	void SetValueModifyByUser(BOOL bIsChange = FALSE) {m_bValueModifyByUser = bIsChange;}
	void SetReadOnlyFlag (BOOL bIsEditable = TRUE) {m_bReadOnlyFlag = bIsEditable;}
	//
	// Data member.
	enum {REP_BOOLEAN, REP_NUMBER, REP_STRING};

protected:
	CString m_strDescription;    // Main Item (col 0)
	CString m_strFlagContent;    // Value     (col 1), can be mandatory
	                             // For the boolean, the value is eigther "TRUE" or "FALSE".
	CString m_strUnit;           // Unit      (col 2)
	BOOL    m_bIsDefault;        // TRUE if m_strFlagContent represents the default value.
	BOOL    m_bDisplay;          // TRUE (Default) if this item is to be used (shown in dialog box) 
	int     m_nType;             // Indicate the type of the object.
	int     m_nMin;
	int     m_nMax;

	CString m_strFlagName;       // Internal use only
	CString m_strDefault;        // Internal use only
	BOOL    m_bIsMandatory;         // Flag mandatory 
	BOOL    m_bIsDefaultMandatoryInFile;    
	BOOL    m_bValueModifyByUser;   // FALSE (default). TRUE when the user modify this item.
	BOOL    m_bReadOnlyFlag;        // FALSE (default) this item is not read only.

};

class CaReplicationItemList: public CTypedPtrList < CObList, CaReplicationItem* >
{
public:
	CaReplicationItemList(){}
	virtual ~CaReplicationItemList();
	void DefineAllFlagsWithDefaultValues(int hdl,CString ServerNo, CString dbName, CString dbOwner);
	POSITION GetPosOfFlag(CString strFlag);
	BOOL ManageBooleanValue( CString strFlag );
	BOOL SetAllFlagsToDefault();
	void MyRemoveAll();
	void VerifyValueForFlag();
	int fillReplicServerParam( LPCTSTR RetrievedLine );
	BOOL GetModifyEdit();
	void SetModifyEdit();
};


inline void CaReplicationItem::SetFlagContent(LPCTSTR lpszNewValue)
{
	m_strFlagContent = lpszNewValue? lpszNewValue:  _T("");
	m_bIsDefault = FALSE;
	if (m_nType == REP_BOOLEAN)
		ASSERT (m_strFlagContent == _T("FALSE") || m_strFlagContent == _T("TRUE"));
}

inline void CaReplicationItem::SetFlagName(LPCTSTR lpszNewFlagName)
{
	m_strFlagName = lpszNewFlagName? lpszNewFlagName:  _T("");
}



#endif

