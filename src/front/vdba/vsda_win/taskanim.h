/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : taskanim.h, Header file 
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Dialog that run thread with animation, allow interupt task 
**               (Parameters and execute task)
** History:
**
** 26-Nov-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    created
** 18-Aug-2004 (uk$so01)
**    BUG #112841 / ISSUE #13623161, Allow user to cancel the Comparison
**    operation.
*/

#if !defined (TASKANIM_HEADER)
#define TASKANIM_HEADER
#include "tkwait.h"
class CaVsdStoreSchema;
class CaVsdRefreshTreeInfo;
class CaVsdRoot;
//
// RUN THE STORE SCHEMA OR INSTALLATION ....
// ************************************************************************************************
class CaExecParamInstallation: public CaExecParam
{
public:
	CaExecParamInstallation(CaVsdStoreSchema* pStoreSchema);
	virtual ~CaExecParamInstallation();
	virtual int  Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);
	BOOL IsInterrupted(){return m_bInterrupted;}
	CString GetFailMessage(){return m_strException;}

private:
	CaVsdStoreSchema* m_pStoreSchema;


	CString m_strMsgBoxTitle;
	CString m_strException;
	CString m_strMsgFailed;
	CString m_strInterruptOperation;
	BOOL m_bOutOfMemory;
	BOOL m_bInterrupted;
};


//
// RUN THE COMPARAISON OF SCHEMA OR INSTALLATION ....
// ************************************************************************************************
class CaExecParamComparing: public CaExecParam
{
public:
	CaExecParamComparing(CaVsdRefreshTreeInfo* pPopulateParamp, CaVsdRoot* pTree, BOOL bInstallation);
	virtual ~CaExecParamComparing();
	virtual int  Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);
	BOOL IsInterrupted(){return m_bInterrupted;}
	CString GetFailMessage(){return m_strException;}
	BOOL IsPopulated(){return m_bPopulated;}

private:
	CaVsdRefreshTreeInfo* m_pPopulateParamp;
	CaVsdRoot* m_pTreeData;
	BOOL m_bInstallation;
	BOOL m_bPopulated;

	CString m_strMsgBoxTitle;
	CString m_strException;
	CString m_strMsgFailed;
	CString m_strInterruptDetection;
	BOOL m_bOutOfMemory;
	BOOL m_bInterrupted;
};


#endif // TASKANIM_HEADER