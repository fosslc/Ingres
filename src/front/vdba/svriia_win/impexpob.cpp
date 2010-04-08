/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : impexpob.cpp , implementation of the CoImportExport class
**    Project  : COM Server
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Com Object of Import & Export Assistant
**
** History:
**	15-Oct-2000 (uk$so01)
**	    Created
**	30-Jan-2002 (uk$so01)
**	    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**	17-Jul-2003 (uk$so01)
**	    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**	    have the descriptions.
**	22-Aug-2003 (uk$so01)
**	    SIR #106648, Add silent mode in IEA. 
**	30-Jan-2004 (uk$so01)
**	    SIR #111701, Use Compiled HTML Help (.chm file)
**	02-feb-2004 (somsa01)
**	    Changed CFile::WriteHuge()/ReadHuge() to CFile::Write()/Read(), as
**	    WriteHuge()/ReadHuge() is obsolete and in WIN32 programming
**	    Write()/Read() can also write more than 64K-1 bytes of data.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "server.h"
#include "impexpob.h"
#include "iapsheet.h"
#include "eapsheet.h"
#include "dmlexpor.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CoImportExport::CoImportExport(IUnknown* pUnkOuter, CaComServer* pServer)
{
	m_ImpImportAssistant.Init(this, pUnkOuter);
	m_ImpExportAssistant.Init(this, pUnkOuter);
	m_cRefs = 0;
	m_pUnkOuter = pUnkOuter; // No AddRef necessary if non-NULL, as we're nested.
	m_pServer   = pServer;   // Assign the pointer to the server control object.
}



CoImportExport::~CoImportExport(void)
{
}


STDMETHODIMP CoImportExport::QueryInterface(REFIID riid, LPVOID* ppv)
{
	HRESULT hError = E_NOINTERFACE;
	CmtOwnMutex mBlock (this);
	if (!mBlock.IsLocked())
		return hError;

	*ppv = NULL;
	if (IID_IUnknown == riid)
	{
		*ppv = this;
	}
	else 
	if (IID_IImportAssistant == riid)
	{
		*ppv = &m_ImpImportAssistant;
	}
	else
	if (IID_IExportAssistant == riid)
	{
		*ppv = &m_ImpExportAssistant;
	}

	if (NULL != *ppv)
	{
		// We've handed out a pointer to the interface so obey the COM rules
		// and AddRef the reference count.
		((LPUNKNOWN)*ppv)->AddRef();
		hError = NOERROR;
	}

	mBlock.UnLock();
	return hError;
}



STDMETHODIMP_(ULONG) CoImportExport::AddRef(void)
{
	ULONG cRefs;
	CmtOwnMutex mBlock (this);
	if (!mBlock.IsLocked())
		return m_cRefs;

	cRefs = ++m_cRefs;
	mBlock.UnLock();

	return cRefs;
}



STDMETHODIMP_(ULONG) CoImportExport::Release(void)
{
	ULONG cRefs;
	CaComServer* pServer;

	CmtOwnMutex mBlock (this);
	if (!mBlock.IsLocked())
		return m_cRefs;

	cRefs = --m_cRefs;
	if (0 == cRefs)
	{
		//
		// Save a local copy of the pointer to the server control object.
		pServer = m_pServer;
		//
		// We artificially bump the main ref count to prevent reentrancy
		// via the main object destructor.  Not really needed in this
		// CoImportExport but a good practice because we are aggregatable and
		// may at some point in the future add something entertaining like
		// some Releases to the CoImportExport destructor. We relinquish thread
		// ownership of this object before deleting it--a good practice.
		m_cRefs++;
		mBlock.UnLock();
		delete this;

		//
		// We've reached a zero reference count for this COM object.
		// So we tell the server housing to decrement its global object
		// count so that the server will be unloaded if appropriate.
		if (NULL != pServer)
			pServer->ObjectsDown();
	}

	return cRefs;
}





// ************************************************************************************************
//                           CoImpImportAssistant 
// ************************************************************************************************
CoImportExport::CoImpImportAssistant::CoImpImportAssistant()
{
}

CoImportExport::CoImpImportAssistant::CoImpImportAssistant(CoImportExport* pBackObj, IUnknown* pUnkOuter)
{
	m_cRefI = 0;            // Init the Interface Ref Count (used for debugging only).
	m_pBackObj = pBackObj;  // Init the Back Object Pointer to point to the parent object.

	//
	// Init the CoImpIQueryObject interface's delegating Unknown pointer.  We use
	// the Back Object pointer for IUnknown delegation here if we are not
	// being aggregated.  If we are being aggregated we use the supplied
	// pUnkOuter for IUnknown delegation.  In either case the pointer
	// assignment requires no AddRef because the CoImpIQueryObject lifetime is
	// guaranteed by the lifetime of the parent object in which
	// CoImpIQueryObject is nested.
	if (NULL == pUnkOuter)
	{
		m_pUnkOuter = pBackObj;
	}
	else
	{
		m_pUnkOuter = pUnkOuter;
	}
}



CoImportExport::CoImpImportAssistant::~CoImpImportAssistant(void)
{
}



STDMETHODIMP CoImportExport::CoImpImportAssistant::QueryInterface(REFIID riid, LPVOID* ppv)
{
	//
	// Delegate this call to the outer object's QueryInterface.
	return m_pUnkOuter->QueryInterface(riid, ppv);
}



STDMETHODIMP_(ULONG) CoImportExport::CoImpImportAssistant::AddRef(void)
{
	//
	// Increment the Interface Reference Count.
	CmtOwnMutex mBlock (this);
	++m_cRefI;

	//
	// Delegate this call to the outer object's AddRef.
	return m_pUnkOuter->AddRef();
}


STDMETHODIMP_(ULONG) CoImportExport::CoImpImportAssistant::Release(void)
{
	//
	// Decrement the Interface Reference Count.
	CmtOwnMutex mBlock (this);
	--m_cRefI;

	//
	// Delegate this call to the outer object's Release.
	return m_pUnkOuter->Release();
}





STDMETHODIMP CoImportExport::CoImpImportAssistant::ImportAssistant(HWND hwndParent)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWnd* pWnd = hwndParent? CWnd::FromHandle(hwndParent): NULL;

	HRESULT hErr = NOERROR;
	BOOL bOk = FALSE;
	try
	{
		theApp.m_strHelpFile = _T("iia.chm");
		CmtSessionManager& smgr = theApp.GetSessionManager();
		smgr.SetDescription(_T("Ingres Import Assistant"));
		//
		// Unpack the CaLowLevelQueryInfomation:
		CuIIAPropertySheet propSheet (pWnd);
		int nRes = propSheet.DoModal();
		hErr = (nRes == IDCANCEL)? 	E_FAIL: NOERROR;

		return hErr;
	}
	catch (...)
	{
		hErr = E_FAIL;
	}

	return hErr;
}


STDMETHODIMP CoImportExport::CoImpImportAssistant::ImportAssistant2(HWND hwndParent, IIASTRUCT* pStruct)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWnd* pWnd = hwndParent? CWnd::FromHandle(hwndParent): NULL;
	USES_CONVERSION;

	HRESULT hErr = NOERROR;
	BOOL bOk = FALSE;
	try
	{
		theApp.m_strHelpFile = _T("iia.chm");
		if (pStruct)
		{
			CmtSessionManager& smgr = theApp.GetSessionManager();
			smgr.SetDescription(W2CT(pStruct->wczSessionDescription));
		}

		//
		// Unpack the CaLowLevelQueryInfomation:
		BOOL bOne = TRUE;
		BOOL bLoop = TRUE;
		while (bLoop)
		{
			CuIIAPropertySheet propSheet (pWnd);
			propSheet.SetInputParam (pStruct);
			int nRet = propSheet.DoModal();

			if (!pStruct->bLoop || nRet == IDCANCEL)
				bLoop = FALSE;
			if (bLoop) {
				//
				// Import another file ?
				CString strMsg;
				strMsg.LoadString (IDS_IMPORT_ANOTHER_FILE);

				int nAnswer = AfxMessageBox (strMsg, MB_ICONQUESTION|MB_YESNO);
				if (nAnswer != IDYES)
					bLoop = FALSE;
			}

			if (bOne && nRet == IDCANCEL)
			{
				hErr = E_FAIL;
			}
			bOne = FALSE;

		}

		return hErr;
	}
	catch (...)
	{
		hErr = E_FAIL;
	}

	return hErr;
}


STDMETHODIMP CoImportExport::CoImpImportAssistant::AboutBox(HWND hwndParent)
{
	HRESULT hErr = NOERROR;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	AfxMessageBox(_T("Not available yet"));
	return hErr;
}



// ************************************************************************************************
//                           CoImpExportAssistant 
// ************************************************************************************************
CoImportExport::CoImpExportAssistant::CoImpExportAssistant()
{
}

CoImportExport::CoImpExportAssistant::CoImpExportAssistant(CoImportExport* pBackObj, IUnknown* pUnkOuter)
{
	m_cRefI = 0;           // Init the Interface Ref Count (used for debugging only).
	m_pBackObj = pBackObj; // Init the Back Object Pointer to point to the parent object.

	// Init the CoImpIAad interface's delegating Unknown pointer.  We use
	// the Back Object pointer for IUnknown delegation here if we are not
	// being aggregated.  If we are being aggregated we use the supplied
	// pUnkOuter for IUnknown delegation.  In either case the pointer
	// assignment requires no AddRef because the CoAad lifetime is
	// guaranteed by the lifetime of the parent object in which
	// CoImpIAad is nested.
	if (NULL == pUnkOuter)
	{
		m_pUnkOuter = pBackObj;
	}
	else
	{
		m_pUnkOuter = pUnkOuter;
	}
}


CoImportExport::CoImpExportAssistant::~CoImpExportAssistant(void)
{

}


STDMETHODIMP CoImportExport::CoImpExportAssistant::QueryInterface(REFIID riid, LPVOID* ppv)
{

	// Delegate this call to the outer object's QueryInterface.
	return m_pUnkOuter->QueryInterface(riid, ppv);
}



STDMETHODIMP_(ULONG) CoImportExport::CoImpExportAssistant::AddRef(void)
{
	//
	// Increment the Interface Reference Count.
	CmtOwnMutex mBlock (this);
	++m_cRefI;

	//
	// Delegate this call to the outer object's AddRef.
	return m_pUnkOuter->AddRef();
}


STDMETHODIMP_(ULONG) CoImportExport::CoImpExportAssistant::Release(void)
{
	//
	// Decrement the Interface Reference Count.
	CmtOwnMutex mBlock (this);
	--m_cRefI;

	//
	// Delegate this call to the outer object's Release.
	return m_pUnkOuter->Release();
}


void CoImportExport::CoImpExportAssistant::UpdateTitle()
{
	CString strTitle;
	strTitle.LoadString(IDS_TITLE_EXPORT);
	free((void*)theApp.m_pszAppName);
	theApp.m_pszAppName =_tcsdup(strTitle);
}


//
// Parameters:
//
// [in ] pQueryInfo: Data of CaLLAddAlterDrop
// [out] pStream   : if not NULL, it packs a CString indicates an error text.
// COMMENT: In case of error, if pStream is not NULL it might contain
//          error text. Caller must call Release of the return IStrem interface
STDMETHODIMP CoImportExport::CoImpExportAssistant::ExportAssistant (HWND hwndParent)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWnd* pWnd = hwndParent? CWnd::FromHandle(hwndParent): NULL;

	HRESULT hErr = NOERROR;
	BOOL bOk = FALSE;
	try
	{
		theApp.m_strHelpFile = _T("iea.chm");
		CmtSessionManager& smgr = theApp.GetSessionManager();
		smgr.SetDescription(_T("Ingres Export Assistant"));

		UpdateTitle();
		//
		// Unpack the CaLowLevelQueryInfomation:
		CuIEAPropertySheet propSheet (pWnd);
		int nRes = propSheet.DoModal();
		hErr = (nRes == IDCANCEL)? 	E_FAIL: NOERROR;

		return hErr;
	}
	catch (...)
	{
		hErr = E_FAIL;
	}

	return hErr;
}

STDMETHODIMP CoImportExport::CoImpExportAssistant::ExportAssistant2(HWND hwndParent, IEASTRUCT* pStruct)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWnd* pWnd = hwndParent? CWnd::FromHandle(hwndParent): NULL;
	USES_CONVERSION;

	HRESULT hErr = NOERROR;
	BOOL bOk = FALSE;
	BOOL bSilent = FALSE;
	CString strLogFile = _T("");
	CString strExceptionError = _T("");
	try
	{
		theApp.m_strHelpFile = _T("iea.chm");
		if (pStruct)
		{
			CmtSessionManager& smgr = theApp.GetSessionManager();
			smgr.SetDescription(W2CT(pStruct->wczSessionDescription));
			bSilent = pStruct->bSilent;
			strLogFile = W2CT(pStruct->wczLogFile);
		}

		if (!bSilent)
		{
			UpdateTitle();
			//
			// Unpack the CaLowLevelQueryInfomation:
			BOOL bOne = TRUE;
			BOOL bLoop = TRUE;
			while (bLoop)
			{
				CuIEAPropertySheet propSheet (pWnd);

				propSheet.SetInputParam (pStruct);
				int nRet = propSheet.DoModal();

				if (!pStruct->bLoop || nRet == IDCANCEL)
					bLoop = FALSE;
				if (bLoop) 
				{
					//
					// Export another file ?
					int nAnswer = AfxMessageBox (IDS_MSG_EXPORT_ANOTHER_FILE, MB_ICONQUESTION|MB_YESNO);
					if (nAnswer != IDYES)
						bLoop = FALSE;
				}

				if (bOne && nRet == IDCANCEL)
				{
					hErr = E_FAIL;
				}
				bOne = FALSE;
			}
		}
		else
		{
			// -silent -o -logfilename="D:\Development\Ingres30\Front\Vdba\App\log.txt" "D:\Development\Ingres30\Front\Vdba\App\ii_exparam.ii_exp"
			TCHAR tchszText  [_MAX_PATH];

			CStringList listFile;
			CString strFile = W2CT(pStruct->wczParamFile);
			if (strFile.IsEmpty())
			{
				strFile = W2CT(pStruct->wczListFile);
				if (strFile.IsEmpty())
					throw (int)2;

				FILE* in;
				in = _tfopen((LPCTSTR)strFile, _T("r"));
				if (in == NULL)
					throw (int)2;

				CString strText = _T("");
				tchszText[0] = _T('\0');
				while ( !feof(in) ) 
				{
					TCHAR* pLine = _fgetts(tchszText, _MAX_PATH, in);
					if (!pLine)
						break;
					if (!tchszText[0])
						continue;
					strText = tchszText;
					strText.TrimLeft();
					strText.TrimRight();
					if (!strText.IsEmpty() && _taccess(strText, 0) != -1)
					{
						listFile.AddTail(strText);
					}
				}

				fclose(in);
			}
			else
				listFile.AddTail(strFile);

			//
			// If specified logfile, first create and make it empty:
			if (!strLogFile.IsEmpty())
			{
				if (_taccess(strLogFile, 0) == -1)
				{
					CFile f (strLogFile, CFile::modeCreate|CFile::modeWrite);
					f.Close();
				}
			}

			POSITION pos = listFile.GetHeadPosition();
			while (pos != NULL)
			{
				CaIEAInfo data;
				data.SetBatchMode (bSilent, pStruct->bOverWrite, W2CT(pStruct->wczLogFile));
				CString strParamFile = listFile.GetNext(pos);
				if (_taccess(strParamFile, 0) != -1)
				{
					data.LoadData (strParamFile);
					data.UpdateFromLoadData();
					BOOL bOK = ExportInSilent(data);
				}
				else
				if (!strLogFile.IsEmpty())
				{
					CString strMsg;
					AfxFormatString1 (strMsg, IDS_LOADPARAM_ERR_FILE_NOT_EXIST, (LPCTSTR)strParamFile);
					CFile f (strLogFile, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite);
					f.SeekToEnd();
					f.Write((LPCTSTR)strMsg, strMsg.GetLength());
					f.Close();
				}
			}
		}

		return hErr;
	}
	catch (int nError)
	{
		switch(nError)
		{
		case 1: // invalid -l<list files>
			break;
		case 2: // cannot open the specified file
			break;
		default:
			break;
		}
		hErr = E_FAIL;
	}
	catch (CArchiveException* e)
	{
		CString strErr;
		MSGBOX_ArchiveExceptionMessage (e, strErr);
		strExceptionError += strErr;
		e->Delete();
	}
	catch (CFileException* e)
	{
		CString strErr;
		MSGBOX_FileExceptionMessage (e, strErr);
		strExceptionError += strErr;
		e->Delete();
	}
	catch (...)
	{
		hErr = E_FAIL;
	}
	if (!strExceptionError.IsEmpty())
	{
		if (bSilent)
		{
			if (!strLogFile.IsEmpty())
			{
				CFile f (strLogFile, CFile::modeCreate|CFile::modeNoTruncate|CFile::modeWrite);
				f.SeekToEnd();
				f.Write((LPCTSTR)strExceptionError, strExceptionError.GetLength());
				f.Close();
			}
		}
		else
		{
			AfxMessageBox (strExceptionError);
		}
	}

	return hErr;
}


STDMETHODIMP CoImportExport::CoImpExportAssistant::AboutBox(HWND hwndParent)
{
	HRESULT hErr = NOERROR;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	UpdateTitle();

	AfxMessageBox(_T("Not available yet"));
	return hErr;
}
