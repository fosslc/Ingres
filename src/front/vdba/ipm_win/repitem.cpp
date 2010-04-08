/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : repitem.cpp, Implementation File
**    Project  : Visual DBA.
**    Author   : Philippe SCHALK
**
**    Purpose  : The data for Replication Object
**
** History:
**
** xx-Feb-1997 (schph01)
**    Created
** 20-May-1999 (schph01)
**    Display -CTO flag.
**    Remove management of special flag -NQTQITSGL and replace by adding the
**    Booleans flags -QIT and -SGL.
**    Change -TPC integer flag in boolean.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "repitem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_SERIAL (CaReplicationItem, CObject, 1)
void CaReplicationItem::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strDescription;
		ar << m_strFlagContent;
		ar << m_strUnit;
		ar << m_bIsDefault;
		ar << m_bDisplay;
		ar << m_nType;
		ar << m_nMin;
		ar << m_nMax;
		ar << m_strFlagName;
		ar << m_strDefault;
		ar << m_bIsMandatory;
		ar << m_bIsDefaultMandatoryInFile;
		ar << m_bValueModifyByUser;
		ar << m_bReadOnlyFlag;
	}
	else
	{
		ar >> m_strDescription;
		ar >> m_strFlagContent;
		ar >> m_strUnit;
		ar >> m_bIsDefault;
		ar >> m_bDisplay;
		ar >> m_nType;
		ar >> m_nMin;
		ar >> m_nMax;
		ar >> m_strFlagName;
		ar >> m_strDefault;
		ar >> m_bIsMandatory;
		ar >> m_bIsDefaultMandatoryInFile;
		ar >> m_bValueModifyByUser;
		ar >> m_bReadOnlyFlag;

	}
}


CaReplicationItemList::~CaReplicationItemList()
{
	while (!IsEmpty())
		delete RemoveHead();
}
static CString RCGetString(UINT nIDS)
{
	CString str;
	str.LoadString(nIDS);
	return str;
}

void CaReplicationItemList::DefineAllFlagsWithDefaultValues(int hdl,CString ServerNo, CString dbName, CString dbOwner)
{
	CString csTempo, csNumber, csDescription, csFlagName;

	//  -SVR  |Server Number
	CString strText1, strText2;
	strText1.LoadString(IDS_I_SVR_FLAG_SVR_NUM);
	strText2.LoadString(IDS_I_SVR_FLAG_INTEGER);
	CaReplicationItem* pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_SVR_NUM),
		_T(""), 
		RCGetString(IDS_I_SVR_FLAG_INTEGER),
		_T("-SVR"),
		CaReplicationItem::REP_NUMBER,
		ServerNo);
	pItem->SetMinMax (1 ,999);
	pItem->SetMandatory ();
	pItem->SetDefaultMandatoryInFile();
	pItem->SetReadOnlyFlag ();
	AddTail (pItem);

	//  -IDB  |Local DB Name
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_LOC_DB),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_STRING),
		_T("-IDB"),
		CaReplicationItem::REP_STRING, 
		dbName);

	pItem->SetMandatory ();
	pItem->SetDefaultMandatoryInFile();
	pItem->SetReadOnlyFlag ();
	AddTail ( pItem );

	//  -OWN  |Local Database Owner
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_LOC_DBOWNER),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_STRING),
		_T("-OWN"),
		CaReplicationItem::REP_STRING, 
		dbOwner);
	pItem->SetDefaultMandatoryInFile();
	pItem->SetReadOnlyFlag ();
	AddTail ( pItem );

	//  -NMO  |No Monitor Events   
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_SEND_NOTI),
		_T("FALSE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-NMO"),
		CaReplicationItem::REP_BOOLEAN,
		_T("FALSE"));
	pItem->SetDisplay( TRUE );
	//pItem->SetDefaultMandatoryInFile();
	AddTail ( pItem );

	//  -MON  |Send Monitor Events
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_SEND_MON),
		_T("TRUE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-MON"),
		CaReplicationItem::REP_BOOLEAN,
		_T("TRUE")); 
	pItem->SetDisplay( FALSE );
	//pItem->SetDefaultMandatoryInFile(); // 5/01/98 already default for the Repserver
	AddTail ( pItem );

	//  -PTL  |
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_PRINT_LEV),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_INTEGER),
		_T("-PTL"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	pItem->SetMinMax (0 ,3);
	//pItem->SetDefaultMandatoryInFile();// 5/01/98 already default for the Repserver
	AddTail ( pItem );

	//  -LGL  |Log Level
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_LOG_LEVEL),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_INTEGER),
		_T("-LGL"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	pItem->SetMinMax (0 ,3);
	//pItem->SetDefaultMandatoryInFile();// 5/01/98 already default for the Repserver
	AddTail ( pItem );

	//  -TOT  |Time Out
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_TIME_OUT),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_SECONDS),
		_T("-TOT"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	pItem->SetMinMax (1  ,999);
	//pItem->SetDefaultMandatoryInFile();// 5/01/98 already default for the Repserver
	AddTail ( pItem );

	//  -CTO  |Connection Timeout  
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_CONN_TIMEOUT),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_SECONDS),
		_T("-CTO"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	pItem->SetMinMax (0  ,-1);
	//pItem->SetDisplay( FALSE );
	AddTail ( pItem );

	//  -EMX  |Maximum Error       
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_MAX_ERR),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_INTEGER),
		_T("-EMX"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	pItem->SetMinMax (0  ,-1);
	AddTail ( pItem );

	//  -EVT  |Event Time Out
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_EVT_TIME),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_SECONDS),
		_T("-EVT"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	pItem->SetMinMax (1  ,9999);
	AddTail ( pItem );

	//  -MLE  |Error Mail
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_ERR_MAIL),
		_T("TRUE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-MLE"),
		CaReplicationItem::REP_BOOLEAN,
		_T(""));
	pItem->SetDisplay( FALSE );
	AddTail ( pItem );

	//  -NML  |No Error Mail
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_NO_ERR_MAIL),
		_T("FALSE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-NML"),
		CaReplicationItem::REP_BOOLEAN,
		_T("FALSE"));
	AddTail ( pItem );

	//  -NSR  |Don't Skip Rule Chk 
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_NO_SKIP_RULE),
		_T("TRUE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-NSR"),
		CaReplicationItem::REP_BOOLEAN,
		_T("TRUE"));
	pItem->SetDisplay( FALSE );
	AddTail ( pItem );

	//  -SCR  |Skip Rule Check     
	pItem =   new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_SKIP_RULE),
		_T("FALSE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-SCR"),
		CaReplicationItem::REP_BOOLEAN,
		_T("FALSE"));
	AddTail ( pItem );

	//  -ORT  |Open Retry          
	pItem =   new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_OPEN_RETRY),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_REPL_CYCLES),
		_T("-ORT"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	AddTail ( pItem );

	//  -QBM  |Absolute Memory     
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_ABS_MEM),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_NB_RECORDS),
		_T("-QBM"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	pItem->SetMinMax (1  ,999999);
	AddTail ( pItem );

	//  -QBT  |Queue Break Transact
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_QUEUE_BREAK),
		_T(""),
		RCGetString(IDS_I_SVR_FLAG_NB_RECORDS),
		_T("-QBT"),
		CaReplicationItem::REP_NUMBER,
		_T(""));
	pItem->SetMinMax (1  ,999999);
	AddTail ( pItem );

	//  -NQTQITSGL this flag manage -QIT(0) -SGL(1) -NQT(2)
	//pItem = new  CaReplicationItem(VDBA_MfcResourceString(IDS_I_SVR_FLAG_QUIET_ONE),"",VDBA_MfcResourceString(IDS_I_SVR_FLAG_QUIET_TWO),"-NQTQITSGL",CaReplicationItem::REP_NUMBER,"");
	//pItem->SetDisplay( FALSE );
	//pItem->SetMinMax (0  ,2);
	//AddTail ( pItem );

	//  -NQT  |Unquiet Server
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_UNQUIET_SVR),
		_T("FALSE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-NQT"),
		CaReplicationItem::REP_BOOLEAN,
		_T(""));
	pItem->SetDisplay( FALSE );
	AddTail ( pItem );

	//  -QIT  |Quiet Server        
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_QUIET_SVR),
		_T("FALSE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-QIT"),
		CaReplicationItem::REP_BOOLEAN,
		_T("FALSE"));
	//pItem->SetDisplay( FALSE );
	AddTail ( pItem );

	//  -SGL  |Single Pass         
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_SINGLE_PASS),
		_T("FALSE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-SGL"),
		CaReplicationItem::REP_BOOLEAN,
		_T("FALSE"));
	//pItem->SetDisplay( FALSE );
	AddTail ( pItem );

	//  -TPC  |Two-Phase Commit    
	pItem = new  CaReplicationItem(
		RCGetString(IDS_I_SVR_FLAG_TWO_PHASE),
		_T("FALSE"),
		RCGetString(IDS_I_SVR_FLAG_BOOLEAN),
		_T("-TPC"),
		CaReplicationItem::REP_BOOLEAN,
		_T("FALSE"));
	AddTail ( pItem );
}


POSITION CaReplicationItemList::GetPosOfFlag(CString strFlag)
{
	POSITION pos = GetHeadPosition();
	while (pos != NULL) {
		CaReplicationItem* obj = GetAt (pos);
		if ( obj->GetFlagName().Compare(strFlag.Left(4)) == 0 )
			return pos;
		else {
			int ret,ret1;
			ret = obj->GetFlagName().Find(_T("-QCD"));
			ret1 = obj->GetFlagName().Find(_T("-QDB"));
			if ( ret != -1 || ret1 != -1 )  {
				if ( obj->GetFlagName().Compare(strFlag) == 0 )
					return pos;
			}
		}
		GetNext(pos);
	}
	return NULL;
}

BOOL CaReplicationItemList::ManageBooleanValue( CString strFlag )
{
	CaReplicationItem* obj1;
	POSITION pos = NULL;

	if (strFlag.Compare(_T("-MON")) == 0 || strFlag.Compare(_T("-NMO")) == 0)   {
		pos = GetPosOfFlag(_T("-NMO"));
		if (pos != NULL)   {
			obj1 = GetAt (pos);
			if (strFlag.Compare(_T("-MON")) == 0) 
				obj1->SetFlagContent(_T("FALSE")); // -NMO == FALSE
			else
				obj1->SetFlagContent(_T("TRUE")); // -NMO == TRUE
		}
	}
	else if (strFlag.Compare(_T("-MLE")) == 0 || strFlag.Compare(_T("-NML")) == 0)  {
		pos = GetPosOfFlag(_T("-NML"));
		if (pos != NULL)   {
			obj1 = GetAt (pos);
			if (strFlag.Compare(_T("-MLE")) == 0) 
				obj1->SetFlagContent(_T("FALSE")); // -NML == FALSE
			else
				obj1->SetFlagContent(_T("TRUE")); // -NML == TRUE
		}
	}
	else if (strFlag.Compare(_T("-QIT")) == 0 || strFlag.Compare(_T("-SGL")) == 0 )  {
		pos = GetPosOfFlag(strFlag);//"-NQTQITSGL"
		if (pos != NULL)   {
			obj1 = GetAt (pos);
			if (strFlag.Compare(_T("-QIT")) == 0) 
				obj1->SetFlagContent(_T("TRUE"));
			if (strFlag.Compare(_T("-SGL")) == 0)
				obj1->SetFlagContent(_T("TRUE"));
		}
	} 
	else if (strFlag.Compare(_T("-SCR")) == 0 || strFlag.Compare(_T("-NSR")) == 0) {
		pos = GetPosOfFlag(_T("-SCR"));
		if (pos != NULL)   {
			obj1 = GetAt (pos);
			if (strFlag.Compare(_T("-NSR")) == 0) 
				obj1->SetFlagContent(_T("FALSE")); // -SCR == FALSE
			else
				obj1->SetFlagContent(_T("TRUE"));  // -SCR == TRUE
		}
	}
	else if ( strFlag.Find(_T("-QCD")) != -1 || strFlag.Find(_T("-QDB")) != -1 )  {
				pos = GetPosOfFlag(strFlag);
				if (pos != NULL)   {
					obj1 = GetAt (pos);
					obj1->SetFlagContent(_T("TRUE"));  
				}
			}
	else if (strFlag.Compare(_T("-NEP")) == 0) {
		pos = GetPosOfFlag(strFlag);
		if (pos != NULL)   {
			obj1 = GetAt (pos);
			obj1->SetFlagContent(_T("TRUE"));  
		}
	}
	else if (strFlag.Compare(_T("-TPC0")) == 0)    {
		pos = GetPosOfFlag(strFlag);
		if (pos != NULL)   {
			obj1 = GetAt (pos);
			obj1->SetFlagContent(_T("TRUE"));
		}
	}

	if (pos)
		return TRUE;
	else
		return FALSE;
}

BOOL CaReplicationItemList::SetAllFlagsToDefault()
{
	POSITION pos = GetHeadPosition();

	while (pos != NULL) {
		CaReplicationItem* obj = GetNext(pos);
		obj->SetToDefault();
		obj->SetValueModifyByUser(TRUE);
	}
	return TRUE;
}

void CaReplicationItemList::MyRemoveAll()
{
	CaReplicationItem* pItem ;
	while (!IsEmpty()) {
		pItem = RemoveHead();
		 delete pItem;
	}
}

void CaReplicationItemList::VerifyValueForFlag()
{
	POSITION pos = GetHeadPosition();
	BOOL bDefault;
	int answ;
	CString Tempo,strValue,strErrorMsg,strDefault;
	while (pos != NULL) {

		CaReplicationItem* obj = GetNext(pos);
		Tempo = obj->GetFlagContent( bDefault );
		strDefault = obj->GetDefaultValue();
		if (Tempo.IsEmpty() == TRUE && ( obj->IsMandatory () || obj->IsDefaultMandatoryInFile())) {
			//"%s (mandatory) parameter not found in runrepl.opt. Will be set to Default."
			strErrorMsg.Format(IDS_F_REP_FLAG_DEFAULT,(LPCTSTR)obj->GetFlagName());
			AfxMessageBox(strErrorMsg, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
			strValue = obj->GetDefaultValue();
			obj->SetFlagContent(strValue);
			obj->SetValueModifyByUser(TRUE);
		}
		if ( obj->IsReadOnlyFlag() && Tempo.Compare(strDefault) !=0 && ( obj->IsMandatory () || obj->IsDefaultMandatoryInFile()))    {
			//"Inconsistent value for %s parameter.\n"
			//"Value in runrepl.opt file  : %s\n"
			//"Required Value : %s\n"
			//"Apply required value?"
			strErrorMsg.Format(
				IDS_F_REP_FLAG_INCONSISTENT, 
				(LPCTSTR)obj->GetDescription(),
				(LPCTSTR)Tempo,
				(LPCTSTR)strDefault);
			answ = AfxMessageBox(strErrorMsg, MB_ICONEXCLAMATION | MB_YESNO | MB_TASKMODAL);
			if ( answ == IDYES ){
				strValue = obj->GetDefaultValue();
				obj->SetFlagContent(strValue);
				obj->SetValueModifyByUser(TRUE);
			}
			else {
				// remove all flags in the list, no modication available.
				MyRemoveAll();
				return;
			}
		}
	}
}

void CaReplicationItemList::SetModifyEdit()
{
	CString Tempo,strValue,strErrorMsg;
	POSITION pos = GetHeadPosition();

	while (pos != NULL) {
		CaReplicationItem* obj = GetNext(pos);
		obj->SetValueModifyByUser(FALSE);
	}
}

BOOL CaReplicationItemList::GetModifyEdit()
{
	CString Tempo,strValue,strErrorMsg;
	POSITION pos = GetHeadPosition();

	while (pos != NULL) {
		CaReplicationItem* obj = GetNext(pos);
		if (obj->IsValueModifyByUser() == TRUE)
			return TRUE;
	}
	return FALSE;
}

int CaReplicationItemList::fillReplicServerParam( LPCTSTR RetrievedLine)
{
	CString Tempo;
	CString strErrorMsg;

	if (RetrievedLine[0] != _T('\0')) {
		Tempo = RetrievedLine;
		POSITION pos = GetPosOfFlag(Tempo);
		if (pos != NULL) {
			CaReplicationItem* obj = GetAt(pos);
			if (obj->GetType() != CaReplicationItem::REP_BOOLEAN)   {
				CString strValue = Tempo.Mid(4,Tempo.GetLength()-4);
				if (!strValue.IsEmpty())
					obj->SetFlagContent(strValue);
				else    {
					//"%s :No value for this Parameter / will be set to default value"
					strErrorMsg.Format(IDS_F_REP_FLAG_VALUE,(LPCTSTR)Tempo);
					AfxMessageBox(strErrorMsg, MB_ICONEXCLAMATION | MB_OK);
					strValue = obj->GetDefaultValue();
					obj->SetFlagContent(strValue);
				}
			}
			else {
				if ( !ManageBooleanValue(Tempo) ){
					//"%s : Unknown Parameter"
					strErrorMsg.Format(IDS_F_REP_FLAG_UNKNOWN,(LPCTSTR)Tempo);
					AfxMessageBox(strErrorMsg, MB_ICONEXCLAMATION | MB_OK);
				}
			}
		}
		else {
			// Flag not found
			if ( Tempo[0] == _T('*') ) {
				strErrorMsg = Tempo.Mid(1); // Error send by the remote command DDdispf
				if ( Tempo[1] == _T('*') ) {
					strErrorMsg = Tempo.Mid(2);
					CString strMsg; // T("All Parameters will be set to default values.\n")
					strMsg.LoadString (IDS_MSG_ALLPARAMS_SET_2_DEFAULT);
					strErrorMsg += strMsg;
					SetAllFlagsToDefault();
				}
			}
			else
				//"%s : Unknown Parameter"
				strErrorMsg.Format(IDS_F_REP_FLAG_UNKNOWN,(LPCTSTR)Tempo);
			AfxMessageBox(strErrorMsg, MB_ICONEXCLAMATION | MB_OK);
		}
	}
	return TRUE;
}

