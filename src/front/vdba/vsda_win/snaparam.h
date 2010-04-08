/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : snaparam.h, header File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store parameters.
**           
** History:
**
** 19-Nov-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
*/

#if !defined (_VSD_STORE_PARAM_HEADER)
#define _VSD_STORE_PARAM_HEADER
#include "qryinfo.h"
#include "compfile.h"
#include "tkwait.h"

//
// Object: CaVsdStoreSchema 
// ************************************************************************************************
class CmtSessionManager;
class CaVsdStoreSchema: public CaLLQueryInfo
{
	DECLARE_SERIAL (CaVsdStoreSchema)
public:
	CaVsdStoreSchema():CaLLQueryInfo(){}
	CaVsdStoreSchema(CmtSessionManager* pMgr, LPCTSTR lpszFile, LPCTSTR lpszNode, LPCTSTR lpszDb, LPCTSTR lpszUser)
		:CaLLQueryInfo(-1, lpszNode, lpszDb)
	{
		m_pSessionManager = pMgr;
		m_strFile = lpszFile;
		m_strSchema = lpszUser;
		m_hWndAnimateDlg = NULL;
	}
	CaVsdStoreSchema(int nObjType, CmtSessionManager* pMgr, LPCTSTR lpszFile, LPCTSTR lpszNode, LPCTSTR lpszDb, LPCTSTR lpszUser)
		:CaLLQueryInfo(nObjType, lpszNode, lpszDb)
	{
		m_pSessionManager = pMgr;
		m_strFile = lpszFile;
		m_strSchema = lpszUser;
		m_hWndAnimateDlg = NULL;
	}
	virtual ~CaVsdStoreSchema(){}
	virtual void Serialize (CArchive& ar);
	void SetHwndAnimate(HWND hWndTaskDlg){m_hWndAnimateDlg = hWndTaskDlg;}
	HWND GetHwndAnimate(){return m_hWndAnimateDlg;}

	CString GetSchema(){return m_strSchema;}
	CString GetDate(){return m_strDate;}

	void ShowAnimateTextInfo(LPCTSTR lpszText);
public:
	CmtSessionManager* m_pSessionManager;
	CString m_strFile;
	CString m_strSchema;
protected:
	CString m_strDate;
	int     m_nVersion;
	HWND    m_hWndAnimateDlg;
};

#endif // _VSD_STORE_PARAM_HEADER