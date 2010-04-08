/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : monrepli.h, Header file 
**    Project  : INGRES II/ Monitoring.
**    Author   : schph01
**    Purpose  : 
**
** History:
**
** xx-Feb-1997 (schph01)
**    Created
** 20-May-1999 (schph01)
**    Remove special management for -NQT -QIT -SGL flags,
**    because the parameters QIT and SGL are not mutually exclusive.
**
** 28-Jan-2000 (schph01)
**    #100201 Change 444735
**    Change the ingres service name.
** 30-Jan-2000 (schph01)
**    #100201 
**    Manage the new name for the Replicator service :
**    "Ingres_Replicator_<installation identifier>_<server number>".
**    and also for the Ingres service.
**    "Ingres_Database_<installation identifier>"
** 01-Feb-2000 (schph01)
**    #100277
**    Display the message returned by rmcmd when the launch of replicator
**    server failed.
** 23-May-2000 (schph01)
**    #99242 (cleanup of Vdba_DuplicateStringWithCRLF function for DBCS 
**    compliance)
** 20-Sep-2001 (noifr01)
**   (bug 105368) casted to (LPTSTR)(LPCTSTR) before 3 calls to wsprintf()
** 17-Oct-2008 (drivi01)
**   Changing password for replicator service requires elevated
**   privileges on Vista.  B/c ocx COM objects can not be elevated
**   the functionality is being moved to repinst.exe.
**   From now on, on Vista, to change replicator service password
**   we will be calling "repinst config <repsrv_num> -username:<username>
**   -password:<password>" using ShellExecute to allow for elevations.
**   Added custom dialog to replace a few AfxMessageBox calls on Vista
**   b/c elevated button needed to be painted for elevation requests.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "regsvice.h"
#include "monrepli.h"
#include "xdlgacco.h"
#include "regsvice.h"
#include "ipmdoc.h"
#include "vnodedml.h"
#include "ingobdml.h"
#include "libmon.h"
#include "messagebox.h"
#include <gvcl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static BOOL FillServiceConfig (CaServiceConfig& cf)
{
	CxDlgAccountPassword dlg;
	if (dlg.DoModal() == IDOK)
	{
		cf.m_strAccount  = dlg.m_strAccount;
		cf.m_strPassword = dlg.m_strPassword;
		return TRUE;
	}
	return FALSE;
}

static void GenerateVnodeName_DbName (LPREPLICSERVERDATAMIN lpRepSvrDta, CString *cTempo)
{
	if(lstrcmpi((LPCTSTR)lpRepSvrDta->RunNode, (LPCTSTR)lpRepSvrDta->LocalDBNode))
		cTempo->Format(_T("%s::%s"), (LPTSTR)lpRepSvrDta->LocalDBNode, (LPTSTR)lpRepSvrDta->LocalDBName);
	else
		*cTempo = lpRepSvrDta->LocalDBName;
}

void VerifyAndUpdateVnodeName (CString* rcsRunNode)
{
	BOOL bExistLocalNode = FALSE;
	BOOL bFound = FALSE;
	CTypedPtrList< CObList, CaDBObject* > lNew;
	BOOL bOk = (VNODE_QueryNode (lNew) == NOERROR)? TRUE: FALSE;
	while (bOk && !lNew.IsEmpty())
	{
		CaNode* pNode = (CaNode*)lNew.RemoveHead();
		if (pNode->IsLocalNode())
			bExistLocalNode = TRUE;
		if (rcsRunNode->CompareNoCase (pNode->GetName()) == 0)
			bFound = TRUE;
		delete pNode;
	}

	CString strError;

	if (bFound == FALSE && bExistLocalNode == TRUE) {
		CString strLoc = LIBMON_getLocalHostName();
		if ( strLoc.CompareNoCase(_T("UNKNOWN LOCAL HOST NAME")) != 0) {
			if ( rcsRunNode->CompareNoCase(strLoc) == 0) {
				rcsRunNode->LoadString(IDS_I_LOCALNODE);
			}
			else	{
				//"Vnode name %s not found. Please create it,\n"
				//"if you want to manage this replicator server."
				strError.Format(IDS_F_VNODE_NOT_FOUND,(LPCTSTR)(*rcsRunNode));
				MessageBox(GetFocus(),strError , NULL,MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
				rcsRunNode->Empty();
			}
		}
		else {
			strError.LoadString(IDS_E_LOCAL_HOST_NAME);//"Local Host name not found."
			MessageBox(GetFocus(),strError , NULL,MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
			rcsRunNode->Empty();
		}
	}
}


/*
** Read RunRepl.opt file and fill the class CaReplicationItemList.
*/
int GetReplicServerParams (CdIpmDoc* pIpmDoc, LPREPLICSERVERDATAMIN lpReplicServerDta , CaReplicationItemList *alist)
{
	TCHAR  tchszCommandLine[200];
	TCHAR* tchszCurLine;
	int ires;

	// Get current vnode name.
	CaConnectInfo& connect = pIpmDoc->GetConnectionInfo();
	CString strConnectedNode = connect.GetNode(); // The current connected node

	CString LocalDBNameOnServerNode;
	CString strError;
	CString strVName = lpReplicServerDta->RunNode;

	// Verify if vnode name exist before start the remote command.
	VerifyAndUpdateVnodeName (&strVName);
	if (strVName.IsEmpty())
		return RES_ERR;

	GenerateVnodeName_DbName (lpReplicServerDta ,&LocalDBNameOnServerNode);
	_stprintf(
		tchszCommandLine,
		_T("dddispf %s 2 %d"), 
		(LPTSTR)(LPCTSTR)LocalDBNameOnServerNode ,
		lpReplicServerDta->serverno);
	ires=RES_SUCCESS;
	if (!LIBMON_ExecRmcmdInBackground((LPTSTR)(LPCTSTR)strConnectedNode , tchszCommandLine, _T(""))) {
		alist->MyRemoveAll();
		ires=RES_ERR;
	}

	for (tchszCurLine=LIBMON_GetFirstTraceLine();tchszCurLine;tchszCurLine = LIBMON_GetNextSignificantTraceLine())
		alist->fillReplicServerParam(tchszCurLine);

	alist->VerifyValueForFlag();

	return RES_SUCCESS;
}

int SetReplicServerParams (CdIpmDoc* pIpmDoc, LPREPLICSERVERDATAMIN lpReplicServerDta , CaReplicationItemList *pList )
{
	TCHAR  tchszCommandLine[200];
	TCHAR* tchszCurLine;
	CString LocalDBNameOnServerNode;
	int ires;
	CString InputString=_T("");

	CString strVName = lpReplicServerDta->RunNode;
	// Verify if vnode name exist before start the remote command.
	VerifyAndUpdateVnodeName (&strVName);
	if (strVName.IsEmpty())
		return RES_ERR;

	GenerateVnodeName_DbName (lpReplicServerDta ,&LocalDBNameOnServerNode);
	_stprintf(tchszCommandLine, _T("ddstoref %s %d"), (LPTSTR)(LPCTSTR) LocalDBNameOnServerNode, lpReplicServerDta->serverno);

	// generate input to be sent to rmcmd
	CString strFormatFlag;
	CString strContent;
	BOOL bDefaultValue;
	POSITION pos;

	pos = pList->GetHeadPosition();
	while (pos != NULL) {
		CaReplicationItem* obj = pList->GetNext(pos);
		strFormatFlag.Empty();
		strContent = obj->GetFlagContent(bDefaultValue);
		if ( obj->IsMandatory() || obj->IsDefaultMandatoryInFile() ) {
			if (obj->GetType() != CaReplicationItem::REP_BOOLEAN)
				if (!strContent.IsEmpty())
					strFormatFlag = obj->GetFlagName() + strContent;
				else {
					CString csError;
					csError.Format(IDS_F_NO_VALUE,(LPCTSTR)obj->GetFlagName());//"Flag %s has no value and is not used."
					AfxMessageBox(csError, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
				}
			else {
				if ( strContent.Compare(_T("TRUE")) == 0)
					strFormatFlag = obj->GetFlagName();
			}
		}
		else {
			if ( bDefaultValue == FALSE) {
				if (obj->GetType() != CaReplicationItem::REP_BOOLEAN) {
					if ( !strContent.IsEmpty() )
						strFormatFlag = obj->GetFlagName() + strContent;
				}
				else {
					if ( strContent.Compare(_T("TRUE")) == 0)
						strFormatFlag = obj->GetFlagName();
					if (strFormatFlag.Compare(_T("-TPC")) == 0)
						strFormatFlag += _T("0"); // Two phase commit is turned off.
				}
			}
		}
		if (!strFormatFlag.IsEmpty())
			InputString += (strFormatFlag + _T("\n"));
		}
	/* empty line in order to stop the ddstoref command*/
	InputString += _T("\n");

	ires=RES_SUCCESS;
	if (!LIBMON_ExecRmcmdInBackground((TCHAR *)lpReplicServerDta->RunNode, tchszCommandLine, (LPTSTR)(LPCTSTR)InputString)) {
		ires=RES_ERR;
	}
	// display error message lines (starting with a '*' character) from output
	for (tchszCurLine=LIBMON_GetFirstTraceLine();tchszCurLine;tchszCurLine = LIBMON_GetNextSignificantTraceLine()) {
		if (tchszCurLine[0]==_T('*')) {
			tchszCurLine++;
			AfxMessageBox(tchszCurLine, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
				ires=RES_ERR;
		}
	}
	return ires;
}



void GetReplicStartLog (LPREPLICSERVERDATAMIN lpReplicServerDta, CString *csAllLines )
{
	TCHAR tchszCommandLine[200];
	CString LocalDBNameOnServerNode;
	CString strVName = lpReplicServerDta->RunNode;

	// Verify if vnode name exist before start the remote command.
	VerifyAndUpdateVnodeName (&strVName);
	if (strVName.IsEmpty())
		return ;

	GenerateVnodeName_DbName (lpReplicServerDta ,&LocalDBNameOnServerNode);

	// Get the current user name   
	_stprintf(tchszCommandLine, _T("dddispf %s 1 %d"), (LPCTSTR)LocalDBNameOnServerNode , lpReplicServerDta->serverno);

	// read Replicat.log
	LIBMON_ExecRmcmdInBackground((TCHAR*)lpReplicServerDta->RunNode, tchszCommandLine, _T(""));
	*csAllLines = (LPCTSTR)LIBMON_GetTraceOutputBuf();
	LIBMON_FreeTraceOutputBuf();
}

int StartReplicServer (int hdl,  LPREPLICSERVERDATAMIN lpReplicServerDta )
{
	const LPCTSTR lpszIngresServiceName = _T("Ingres_Database_%s");
	const LPCTSTR lpszReplicServiceTempl = _T("Ingres_Replicator_%s_%d");
	int ires = RES_SUCCESS;
	TCHAR  tchszCommandLine[200];
	TCHAR* tchszCurLine;
	CString strVName = lpReplicServerDta->RunNode;
	CString msg;

#if !defined (MAINWIN)
	CString strLoc = LIBMON_getLocalHostName();
	if ( strVName.CompareNoCase(strLoc) == 0)
	{
		CString strReplicServiceName;
		CString strIngresServiceName;
		CString strII = INGRESII_QueryInstallationID(FALSE);

		if( strII.IsEmpty() )
		{
			AfxMessageBox (IDS_E_INSTALLATION_NOT_FOUND);
			return RES_ERR;
		}
		strReplicServiceName.Format(lpszReplicServiceTempl, (LPCTSTR)strII, lpReplicServerDta->serverno);
		strIngresServiceName.Format(lpszIngresServiceName,  (LPCTSTR)strII);

		if (!IsServiceInstalled ((LPCTSTR)strReplicServiceName)) 
		{
			// search max server no on local (run) node
			RESOURCEDATAMIN  ResDtaMin;
			REPLICSERVERDATAMIN ReplicSvrDta;
			int i,iservermax=0;
			LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName ( hdl );
			FillResStructFromDB(&ResDtaMin, lpReplicServerDta->ParentDataBaseName);

			/* 23-Dec-98 :  fixed "error system 3" when exiting the product, in the case       */
			/* VDBA detected that the replicator services were not installed, and proposed     */
			/* to install them automatically.                                                  */
			/* implementation: Replaced GetFirstMonInfo-GetNextMonInfo loop                    */
			/* with DBAGetFirstObject-DBAGetNextObject loop.                                   */
			/* this avoids using the cache for getting the list of servers                     */
			/* (for calculating the argument to be passed to "repinst"), because of a          */
			/* restriction of the cache for repl.servers: cache entries are attached to a node,*/
			/* but for repl.servers, the given entry needs to be connected to other nodes.     */
			/* A special management is implemented for "Monitor windows" attached cache        */
			/* entries, but in the current case, there isn't a monitor window open on the      */
			/* used node (which can be different from the open Monitor Window from which       */
			/* this stuff has been called.                                                     */

			int ires =DBAGetFirstObject (
				(LPUCHAR)vnodeName,
				OT_MON_REPLIC_SERVER,
				1,
				(LPUCHAR *)&ResDtaMin,
				TRUE,
				(LPUCHAR)&ReplicSvrDta,
				NULL,NULL);
			while (ires==RES_SUCCESS) 
			{
				if (! lstrcmpi((LPCTSTR)lpReplicServerDta->RunNode, (LPCTSTR)ReplicSvrDta.RunNode)) {
					if (ReplicSvrDta.serverno>iservermax)
						iservermax=ReplicSvrDta.serverno;
				}
				ires=DBAGetNextObject((LPUCHAR)&ReplicSvrDta,NULL,NULL);
			}
			if ( iservermax == 0 ) {
				//_T("Cannot retrieve number of server(s) - Install services Aborted."
				AfxMessageBox(IDS_E_NB_SERVERS);
				return RES_ERR;
			}
			//"The Ingres Replicator %d service is not installed.\n"
			//"Do you want to invoke 'repinst %d' in the background, "
			//"in order to install all Replicator services required by your replication scheme?"
			msg.Format(IDS_F_SERVICE_NOT_INSTALL,lpReplicServerDta->serverno,iservermax);
			int iansw;
			if (GVvista())
			{
				CMessageBox imb(NULL, msg);
				iansw=imb.DoModal();
			}
			else
				iansw = AfxMessageBox (msg, MB_YESNO|MB_ICONQUESTION);
			if (iansw == IDNO)
				return RES_ERR;
			CString Cmd;
			Cmd.Format(_T("repinst %d"),iservermax);
			if (LIBMON_Mysystem((LPTSTR)(LPCTSTR)Cmd)<0) {
				AfxMessageBox (IDS_E_REPINST_UTIL);//"Failure in invoking the repinst utility"
				return RES_ERR;
			}
			if (!IsServiceInstalled ((LPCTSTR)strReplicServiceName)) {
				AfxMessageBox (IDS_E_REPINST_INSTALL);//"The service has not been installed properly by the repinst utility"
				return RES_ERR;
			}
			//"DBA passwords are required for all services that have been created\n"
			//"Do you want to enter them now?\n"
			//"(If not, you will need to use the Control Panel Services option.)\n"
			//"(Some other settings for these services can only be changed through the control panel.)
			if (GVvista())
			{
				CString str;
				str.Format(IDS_I_PASSWORD_REQUIRED);
				CMessageBox imb(NULL, str);
				iansw = imb.DoModal();	
			}
			else
				iansw = AfxMessageBox (IDS_I_PASSWORD_REQUIRED, MB_YESNO|MB_ICONQUESTION);
			if (iansw == IDNO)
				return RES_ERR;
			// ask for user/password
			CaServiceConfig ServiceParms;
			if (!FillServiceConfig  (ServiceParms))
				return RES_ERR;
			BOOL bOK=TRUE;
			for (i=1;i<=iservermax;i++) {
				strReplicServiceName.Format(lpszReplicServiceTempl, (LPCTSTR)strII, i);
				if (GVvista())
				{
				     CString Cmd;
				     Cmd.Format(_T("repinst config %d -username:%s -password:%s"), iservermax, 
						ServiceParms.m_strAccount, ServiceParms.m_strPassword);
				     if (LIBMON_Mysystem((LPTSTR)(LPCTSTR)Cmd)<0)
				     {
				     	//"Failure in user/password setup for the 'Ingres Replicator %d' service"
					msg.Format(IDS_E_USER_PASSWORD_NB,i);
					AfxMessageBox (msg);
					bOK=FALSE;
				     }
				}
				else
				if (!SetServicePassword ((LPCTSTR) strReplicServiceName, ServiceParms)) {
					//"Failure in user/password setup for the 'Ingres Replicator %d' service"
					msg.Format(IDS_E_USER_PASSWORD_NB,i);
					AfxMessageBox (msg);
					bOK=FALSE;
				}
			}
			if (!bOK)
				return RES_ERR;
			if (!IsServiceRunning(strIngresServiceName)) {
				//"Ingres is currently not started as a service.\n"
				//"You must close all connections in VDBA (or Exit VDBA), "
				//"then stop Ingres, and restart it as a Service if you want "
				//"to start a replicator server on the local installation.\n"
				//"You must also make sure that the user/password for the Ingres service is correct.\n"
				//"Do you want to apply the user/password that you just entered for the replicator services?"
				iansw = AfxMessageBox (IDS_I_INGRES_NOT_STARTED, MB_YESNO|MB_ICONQUESTION);
				if (iansw == IDYES) {
					if (GVvista())
					{
					     CString Cmd;
					     Cmd.Format(_T("repinst config %d -username:%s -password:%s"), iservermax, 
							ServiceParms.m_strAccount, ServiceParms.m_strPassword);
					     if (LIBMON_Mysystem((LPTSTR)(LPCTSTR)Cmd)<0)
					     {
						//"Failure in user/password setup for the 'Ingres Replicator %d' service"
						msg.Format(IDS_E_USER_PASSWORD_NB,i);
						AfxMessageBox (msg);
						bOK=FALSE;
					     }
					}
					else
					if (!SetServicePassword ((LPCTSTR) strReplicServiceName, ServiceParms)) {
						//"Failure in user/password setup for the 'Ingres Replicator %d' service"
						msg.Format(IDS_E_USER_PASSWORD_NB,i);
						AfxMessageBox (msg);
						bOK=FALSE;
					}
				}
				return RES_ERR;
			}
		}
		if (!IsServiceRunning(strIngresServiceName)) {
			//"Ingres is currently not started as a service.\n"
			//"You must close all connections in VDBA (or Exit VDBA), "
			//"then stop Ingres, and restart it as a Service if you want "
			//"to start a replicator server on the local installation.\n"
			//"You must also make sure that the user/password for the Ingres service is correct.");
			AfxMessageBox (IDS_E_INGRES_NOT_STARTED);
			return RES_ERR;
		}
	}
#endif // MAINWIN

	// Verify if vnode name exist before start the remote command.
	VerifyAndUpdateVnodeName (&strVName);
	if (strVName.IsEmpty())
		return RES_ERR;

	_stprintf(tchszCommandLine, _T("ddstart %d"), lpReplicServerDta->serverno);

	if (!LIBMON_ExecRmcmdInBackground((TCHAR *)lpReplicServerDta->RunNode, tchszCommandLine, _T("")))
		return RES_ERR;

	// check for error (output line starting with a '*' character )
	ires=RES_SUCCESS;
	tchszCurLine=LIBMON_GetFirstTraceLine();
	while (tchszCurLine) {
		if (tchszCurLine[0]==_T('*'))
			CMnext(tchszCurLine);
		if (_tcslen(tchszCurLine)) {
			CString Msg;
			/* "Error from RMCMD server on '%s' :\n%s" */
			Msg.Format(IDS_E_ERROR_RMCMD, (LPCTSTR)strVName, tchszCurLine);
			AfxMessageBox(Msg, MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
			ires=RES_ERR;
		}
		tchszCurLine = LIBMON_GetNextSignificantTraceLine();
	}

	return ires;
}

int StopReplicServer (int hdl, LPREPLICSERVERDATAMIN lpReplicServerDta )
{
	ERRORPARAMETERS ErrorParam;
	memset(&ErrorParam,NULL,sizeof(ERRORPARAMETERS));
	int iret;
	CString strMsg,strMsg2,strMsg3, strVName = lpReplicServerDta->RunNode;

	strMsg.Empty();
	strMsg2.Empty();
	strMsg3.Empty();
	// Verify if vnode name exist before start the remote command.
	VerifyAndUpdateVnodeName (&strVName);
	if (strVName.IsEmpty())
		return 0;

	iret = LIBMON_MonitorReplicatorStopSvr ( hdl ,
	                                         lpReplicServerDta->serverno ,
	                                         lpReplicServerDta->LocalDBName,
	                                         &ErrorParam);
	if ( iret != RES_SUCCESS )
	{
		if( ErrorParam.iErrorID == RES_E_OPENING_SESSION )
			strMsg.Format(IDS_F_OPENING_SESSION,ErrorParam.tcParam1);
		else
		{
			if ( ErrorParam.iErrorID & MASK_RES_E_RELEASE_SESSION )
				strMsg.LoadString(IDS_ERR_RELEASE_SESSION);

			ErrorParam.iErrorID &= ~MASK_RES_E_RELEASE_SESSION;
			if( ErrorParam.iErrorID == RES_E_SEND_EXECUTE_IMMEDIAT )
			{
				if (!strMsg.IsEmpty())
				{
					strMsg3.Format(IDS_E_SEND_EXECUTE_IMMEDIAT,ErrorParam.iParam1);
					strMsg += _T("\n");
					strMsg += strMsg3;
				}
				else
					strMsg.Format(IDS_E_SEND_EXECUTE_IMMEDIAT,ErrorParam.iParam1);
			}
		}
		//"Failure in stopping the replicator server."
		strMsg2.LoadString(IDS_E_STOP_REP_SVR);
		if (!strMsg.IsEmpty())
		{
			strMsg2 += _T("\n");
			strMsg2 += strMsg;
		}
		AfxMessageBox(strMsg2, NULL,MB_ICONEXCLAMATION | MB_OK | MB_TASKMODAL);
	}
	return iret;
}

