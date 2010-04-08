/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : property.cpp : implementation file
**    Project  : Vdba 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manage OLE Properties.
**
** History:
**
** 13-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
**    Created
** 29-Aug-2007 (drivi01)
**    On Vista, store iiguis.prf in %USERPROFILE% location.
**/


#include "stdafx.h"
#include "property.h"
#include "rcdepend.h"
#include "constdef.h"
#include "compfile.h"
#include "qsqldoc.h"
#include "ipmaxdoc.h"
#include "maindoc.h"
#include <compat.h>
#include <gv.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


void GetActiveControlUnknow(CPtrArray& arrayUnknown, UINT nCtrl)
{
	CDocument*   pDoc;
	POSITION pos, curTemplatePos = theApp.GetFirstDocTemplatePosition();
	while(curTemplatePos != NULL)
	{
		CDocTemplate* curTemplate = theApp.GetNextDocTemplate (curTemplatePos);
		pos = curTemplate->GetFirstDocPosition ();
		while (pos)
		{
			pDoc = curTemplate->GetNextDoc (pos);
			switch (nCtrl)
			{
			case AXCONTROL_SQL:
				if(pDoc->IsKindOf (RUNTIME_CLASS (CdSqlQuery)))
				{
					CdSqlQuery* pCtlDoc = (CdSqlQuery*)pDoc; // We assure that only one view attached to a document.
					IUnknown* pUnknown = pCtlDoc->GetSqlUnknown();
					if (pUnknown)
						arrayUnknown.Add(pUnknown);
				}
				else
				if (pDoc->IsKindOf (RUNTIME_CLASS (CMainMfcDoc)))
				{
					CMainMfcDoc* pCtlDoc = (CMainMfcDoc*)pDoc; // We assure that only one view attached to a document.
					IUnknown* pUnknown = pCtlDoc->GetSqlUnknown();
					if (pUnknown)
						arrayUnknown.Add(pUnknown);
				}
				break;
			case AXCONTROL_IPM:
				if(pDoc->IsKindOf (RUNTIME_CLASS (CdIpm)))
				{
					CdIpm* pCtlDoc = (CdIpm*)pDoc; // We assure that only one view attached to a document.
					IUnknown* pUnknown = pCtlDoc->GetIpmUnknown();
					if (pUnknown)
						arrayUnknown.Add(pUnknown);
				}
				break;
			default:
				break;
			}
		}
	}
}


//
// Load the persintent properties form file.
BOOL LoadStreamInit(COleStreamFile& sqlStreamInit, LPCTSTR lpszStreamName)
{
	IStorage* pActiveXStorage = NULL;
	HRESULT hr = NOERROR;
	BOOL bFound = FALSE;
	BOOL bOK = TRUE;
	try
	{
		DWORD grfMode1 = STGM_DIRECT|STGM_WRITE|STGM_READ|STGM_SHARE_EXCLUSIVE;
		TCHAR* pEnv;
		if (GVvista())
		pEnv = _tgetenv(_T("USERPROFILE"));
		else
		pEnv = _tgetenv(_T("II_SYSTEM"));
		if (!pEnv)
			return FALSE;

		CString strPath = pEnv;
		if (GVvista())
		strPath += consttchszIngConf;
		else
		strPath += consttchszIngVdba;
		strPath += _T("iiguis.prf");

		CaCompoundFile compoundFile(strPath);
		bFound = compoundFile.FindElement(NULL, _T("ActiveXRootStorage"), STGTY_STORAGE);
		if (bFound)
			pActiveXStorage = compoundFile.OpenStorage(NULL, _T("ActiveXRootStorage"), grfMode1);

		if (!pActiveXStorage)
			return FALSE;

		//
		// Get Stream from the .prf file and copy its content to the memory stream:
		STATSTG statstg ;
		COleStreamFile file;
		if( !file.OpenStream( pActiveXStorage, lpszStreamName, STGM_READ|STGM_SHARE_EXCLUSIVE ) )
		{
			pActiveXStorage->Release();
			return FALSE;
		}
		IStream* pStreamInit = file.GetStream();
		hr = pStreamInit->Stat(&statstg, STATFLAG_NONAME);
		if (FAILED (hr))
		{
			pActiveXStorage->Release();
			return FALSE;
		}

		//
		// Copy to the memory stream:
		IStream* pStreamMemory = sqlStreamInit.GetStream();
		hr = pStreamInit->CopyTo(pStreamMemory, statstg.cbSize, NULL, NULL);
		if (hr != S_OK)
		{
			pActiveXStorage->Release();
			return FALSE;
		}

		pActiveXStorage->Release();
		return TRUE;
	}
	catch (...)
	{

	}
	return FALSE;
}

void LoadSqlQueryPreferences(IUnknown* pUnk)
{
	HRESULT hr = NOERROR;
	IPersistStreamInit* pPersistStreammInit = NULL;
	BOOL bOK = FALSE;
	IStream* pOldStream = theApp.m_sqlStreamFile.Detach();
	if (pOldStream)
		pOldStream->Release();

	bOK = theApp.m_sqlStreamFile.CreateMemoryStream();
	if (bOK)
	{
		if (!pUnk)
			return;
		//
		// Load the propertis from file IIGUIS.PRF:
		OpenStreamInit (pUnk, _T("SqlQuery"));
		hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
		if(SUCCEEDED(hr) && pPersistStreammInit)
		{
			IStream* pStream = theApp.m_sqlStreamFile.GetStream();
			hr = pPersistStreammInit->Save(pStream, TRUE);
			pPersistStreammInit->Release();
			theApp.m_bsqlStreamFile = TRUE;
		}
	}
}

void LoadIpmPreferences(IUnknown* pUnk)
{
	HRESULT hr = NOERROR;
	IPersistStreamInit* pPersistStreammInit = NULL;
	BOOL bOK = FALSE;
	IStream* pOldStream = theApp.m_ipmStreamFile.Detach();
	if (pOldStream)
		pOldStream->Release();

	bOK = theApp.m_ipmStreamFile.CreateMemoryStream();
	if (bOK)
	{
		if (pUnk)
		{
			//
			// Load the propertis from file IIGUIS.PRF:
			OpenStreamInit (pUnk, _T("Ipm"));
			hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
			if(SUCCEEDED(hr) && pPersistStreammInit)
			{
				IStream* pStream = theApp.m_ipmStreamFile.GetStream();
				hr = pPersistStreammInit->Save(pStream, TRUE);
				pPersistStreammInit->Release();
				theApp.m_bipmStreamFile = TRUE;
			}
		}
	}
}

