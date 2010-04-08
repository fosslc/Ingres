/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : repcodoc.h, header file  
**    Project  : INGRES II/ Monitoring.
**    Author   : Philippe SCHALK 
**    Purpose  : The data for Replication Object
**
** History:
**
** xx-May-1997 (Philippe SCHALK )
**    Created
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "repevent.h"
#include "xdgreque.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL (CaReplicationRaiseDbevent, CObject, 1)
void CaReplicationRaiseDbevent::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_strAction;
		ar << m_strEvent;
		ar << m_strServerFlag;
		ar << m_bAskQuestion; 
		ar << m_strQuestionText;
		ar << m_strQuestionCaption;
		ar << m_nServerNumber;
	}
	else
	{
		ar >> m_strAction;
		ar >> m_strEvent;
		ar >> m_strServerFlag;
		ar >> m_bAskQuestion; 
		ar >> m_strQuestionText;
		ar >> m_strQuestionCaption;
		ar >> m_nServerNumber;
	}
}


CaReplicRaiseDbeventList::~CaReplicRaiseDbeventList()
{
	while (!IsEmpty())
		delete RemoveHead();
}

void CaReplicRaiseDbeventList::DefineAllDbeventWithDefaultValues(int iHandle, CString csDataBaseName, int nSvrNumber)
{
	CString csTempo, csTmp1,csTmp2, csNumber, csDescription, csFlagName,csDescription2,csFlagName2;
	CString csDdSetSvr = _T("dd_set_server");
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_STOP_SVR,"dd_stop_server","",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_START_REPLIC,"dd_go_server","",nSvrNumber	) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_ACTIVE_ALL,csDdSetSvr,"CLQ",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_CONNECT_TIME_OUT			,csDdSetSvr,"CTO0",nSvrNumber ) ); // 30 minutes = CTO1800
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_CTO_30						,csDdSetSvr,"CTO1800", nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_CTO_1 						,csDdSetSvr,"CTO60", nSvrNumber ) );
	csTmp1.LoadString(IDS_TEXT_EV_NB_SECONDS);
	csTmp2.LoadString(IDS_TITLE_EV_CTO);
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_CTO_ASK						,csDdSetSvr,"CTO" , nSvrNumber ,TRUE,(LPCTSTR) csTmp1,(LPCTSTR) csTmp2,32000,1 ));

	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_SHD_SVR_100_ERRORS			,csDdSetSvr,"EMX100",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_SHD_SVR_200_ERRORS			,csDdSetSvr,"EMX200",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_SHD_SVR_50_ERRORS			,csDdSetSvr,"EMX50" ,nSvrNumber ) );
	csTmp1.LoadString(IDS_TEXT_SHD_SVR);
	csTmp2.LoadString(IDS_TITLE_SHD_SVR);
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_SHD_SVR_N_ERRORS,csDdSetSvr  ,"EMX"   ,nSvrNumber,TRUE ,(LPCTSTR) csTmp1,(LPCTSTR) csTmp2,32000,1) );

	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_TIMEOUT						,csDdSetSvr,"EVT0",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_TIMEOUT_30					,csDdSetSvr,"EVT30",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_TIMEOUT_1					,csDdSetSvr,"EVT3600",nSvrNumber ) );
	csTmp1.LoadString(IDS_TEXT_EV_NB_SECONDS);
	csTmp2.LoadString(IDS_TITLE_EV_PRN);
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_TIMEOUT_N,csDdSetSvr,"EVT" ,nSvrNumber ,TRUE ,(LPCTSTR) csTmp1,(LPCTSTR) csTmp2,9999,1 ) );


	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_NO_LOG_INF					,csDdSetSvr,"LGL0",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_LOG_ERROR_INF				,csDdSetSvr,"LGL1",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_LOG_ERR_IMP					,csDdSetSvr,"LGL2",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_LOG_ERR_WARNING				,csDdSetSvr,"LGL3",nSvrNumber ) );
	// LGL4 and LGL5 they are intended for internal,support use. PS 03-15-99
	//AddTail( new CaReplicationRaiseDbevent("Log the flow of the replication server."					,csDdSetSvr,"LGL4",nSvrNumber ) );
	//AddTail( new CaReplicationRaiseDbevent("Log debug messages."										,csDdSetSvr,"LGL5",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_SEND_MAIL					,csDdSetSvr,"MLE",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_SEND_DB_EV					,csDdSetSvr,"MON",nSvrNumber ) );
	//NEP is intended for internal,support use. PS 03-15-99
	//AddTail( new CaReplicationRaiseDbevent("Do not write the query execution plan out to the log."		,csDdSetSvr,"NEP",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_NO_MAIL		 				,csDdSetSvr,"NML",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_NO_DB_EVENT					,csDdSetSvr,"NMO",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_UNQUIET_REPL				,csDdSetSvr,"NQT",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_PRINT_SESS 				,csDdSetSvr,"NSE",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_ENABLE_RULE					,csDdSetSvr,"NSR",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_CHECK_CLOSED				,csDdSetSvr,"ORT100",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_CHECK_CLOSED50				,csDdSetSvr,"ORT50",nSvrNumber ) );
	csTmp1.LoadString(IDS_TEXT_EV_REPL_CYCLES);
	csTmp2.LoadString(IDS_TITLE_EV_CLOSED_DB);
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_CHECK_CLOSEDN				,csDdSetSvr,"ORT",nSvrNumber,TRUE,(LPCTSTR) csTmp1,(LPCTSTR) csTmp2, 32000,1 ) );

	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_NO_PRINT_LOG				,csDdSetSvr,"PTL0",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_PRINT_STANDARD				,csDdSetSvr,"PTL1",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_PRINT_ERR_INF				,csDdSetSvr,"PTL2",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_EV_PRINT_WARNING				,csDdSetSvr,"PTL3",nSvrNumber ) );
	// PTL4 and PTL5 they are intended for internal,support use. PS 03-15-99
	//AddTail( new CaReplicationRaiseDbevent("Print the flow of the replication server."					,csDdSetSvr,"PTL4",nSvrNumber ) );
	//AddTail( new CaReplicationRaiseDbevent("Print Debugging messages to standard output."				,csDdSetSvr,"PTL5",nSvrNumber ) );

	CStringList cslAllInfo;
	POSITION pos;
	IPM_GetCddsNumber (iHandle, csDataBaseName, cslAllInfo);
	
	pos = cslAllInfo.GetHeadPosition();
	if ( pos != NULL )
	{
		csDescription.LoadString(IDS_I_QUIET_CDDS);  //"Quiet CDDS number : "
		csDescription2.LoadString(IDS_I_UNQUIET_CDDS);//"Unquiet CDDS number : "
	}
	while ( pos != NULL ) {
		CString csDescTmp,csDescTmp2;
		csTempo = cslAllInfo.GetNext(pos);
		csFlagName.Empty();
		csFlagName = "QCD" + csTempo;
		csDescTmp = csDescription + csTempo;
		AddTail( new CaReplicationRaiseDbevent(csDescTmp,csDdSetSvr,csFlagName,nSvrNumber ) );
		csDescTmp2  = csDescription2 + csTempo;
		csFlagName2 = "UCD" + csTempo;
		AddTail( new CaReplicationRaiseDbevent(csDescTmp2,csDdSetSvr,csFlagName2,nSvrNumber ) );
	}

	cslAllInfo.RemoveAll();
	// Get All database for this server and add in the list.
	IPM_GetDatabaseConnection(iHandle, csDataBaseName, cslAllInfo);

	pos = cslAllInfo.GetHeadPosition();
	if ( pos != NULL )
	{
		csDescription.LoadString(IDS_I_QUIET_DB);  //"Quiet Database number : "
		csDescription2.LoadString(IDS_I_UNQUIET_DB);//"Unquiet Database number : "
	}
	while ( pos != NULL ) {
		CString csDescTmp,csDescTmp2;
		csTempo = cslAllInfo.GetNext(pos);
		csNumber = csTempo.SpanExcluding(" ");
		//	 -QCB Quiet Database
		csFlagName = "QDB" + csNumber;
		csDescTmp = csDescription + csTempo;
		AddTail( new CaReplicationRaiseDbevent(csDescTmp,csDdSetSvr,csFlagName,nSvrNumber ) );
		//	-UCB  Unquiet Database
		csFlagName2 = "UDB" + csNumber;
		csDescTmp2 = csDescription2 + csTempo;
		AddTail( new CaReplicationRaiseDbevent(csDescTmp2,csDdSetSvr,csFlagName2,nSvrNumber ) );
	}

	//AddTail( new CaReplicationRaiseDbevent("Quiet Database number n",csDdSetSvr,"QDB",nSvrNumber ) );
	//AddTail( new CaReplicationRaiseDbevent("Unquiet CDDS Number n",csDdSetSvr,"UCDn",nSvrNumber ) );
	//AddTail( new CaReplicationRaiseDbevent("Unquiet Database number n",csDdSetSvr,"UDBn",nSvrNumber ) );

	// QEP is intended for internal,support use. PS 03-15-99
	//AddTail( new CaReplicationRaiseDbevent("Set Query Execution Plan Flag to write to the print.log.",csDdSetSvr,"QEP",nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_QUIET_SVR			,csDdSetSvr,"QIT"		,nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_SKIP_INDEX			,csDdSetSvr,"SCR"		,nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_PRINT_SESS_NUM		,csDdSetSvr,"SES"		,nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_CONN_TIMEOUT120	,csDdSetSvr,"TOT120"	,nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_CONN_TIMEOUT5		,csDdSetSvr,"TOT300"	,nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_CONN_TIMEOUT60		,csDdSetSvr,"TOT60"	,nSvrNumber ) );
	csTmp1.LoadString(IDS_TEXT_EV_NB_SECONDS);
	csTmp2.LoadString(IDS_TITLE_CONN_TIMEOUT);
	AddTail( new CaReplicationRaiseDbevent(IDS_I_CONN_TIMEOUTN		,csDdSetSvr,"TOT"		,nSvrNumber ,TRUE,(LPCTSTR) csTmp1,(LPCTSTR) csTmp2,999,1) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_TWO_PHASE_COMMIT_OFF,csDdSetSvr,"TPC0" 	,nSvrNumber ) );
	AddTail( new CaReplicationRaiseDbevent(IDS_I_XTWO_PHASE_COMMIT_ON,csDdSetSvr,"TPC1" 	,nSvrNumber ) );
}

//	
//	output: CString filled
//	return :	IDOK	 filled OK.
//				IDCANCEL filled canceled.
int CaReplicationRaiseDbevent::AskQuestion4FilledServerFlag(CString *SvrFlag)
{
	int Ret = -1;
	CxDlgRaiseEventQuestion RaiseEventAnswerDlg;
	RaiseEventAnswerDlg.m_strCaption  = GetQuestionCaption();
	RaiseEventAnswerDlg.m_strQuestion = GetQuestionText();
	RaiseEventAnswerDlg.m_iMin = m_iMinValue;
	RaiseEventAnswerDlg.m_iMax = m_iMaxValue;
	Ret = RaiseEventAnswerDlg.DoModal();
	if (Ret == IDOK)
		SvrFlag->Format(_T("%s%d"), GetServerFlag(), RaiseEventAnswerDlg.m_NewValue);
	return Ret; 
}



