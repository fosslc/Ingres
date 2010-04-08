/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : maindoc.cpp , Implementation File
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01) 
**    Purpose  : Main document for SDI
**
** History:
** 04-Dec-1998 (uk$so01)
**    Created
** 03-Feb-2000 (uk$so01)
**    (BUG #100327)
**    Move the data files of ivm to the directory II_SYSTEM\ingres\files.
** 31-May-2000 (uk$so01)
**    bug 99242 Handle DBCS
**/

#include "stdafx.h"
#include "ivm.h"
#include "maindoc.h"
#include "readflg.h"
#include "wmusrmsg.h"
#include "ivmdml.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CdMainDoc

IMPLEMENT_DYNCREATE(CdMainDoc, CDocument)

BEGIN_MESSAGE_MAP(CdMainDoc, CDocument)
	//{{AFX_MSG_MAP(CdMainDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdMainDoc construction/destruction

CdMainDoc::CdMainDoc()
{
	m_strArchiveFile = theApp.m_strArchiveEventFile;
	m_hwndLeftView = NULL;
	m_hwndRightView = NULL;
	m_hwndMainFrame = NULL;
	m_hwndMainDlg = NULL;
}

CdMainDoc::~CdMainDoc()
{
}

BOOL CdMainDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CdMainDoc serialization


void CdMainDoc::Serialize(CArchive& ar)
{
	try
	{
		CaReadFlag* pRFlag = theApp.GetReadFlag();

		if (ar.IsStoring())
		{
			ar << theApp.m_strIVMEVENTSerializeVersionNumber;
			SerializeLogFileCheck(ar);
		}
		else
		{
			CString strCheck;
			ar >> strCheck;
			SerializeLogFileCheck(ar);
			BOOL bRq = theApp.GetMarkAllAsUnread_flag();
			if (theApp.m_strIVMEVENTSerializeVersionNumber.CompareNoCase (strCheck) != 0)
			{
				TRACE0 ("CdMainDoc::Serialize: nothing is loaded because of wrong version number !!! \n");
				IVM_PostModelessMessageBox(IDS_W_MARKERCHANGED_READMSG,m_hwndMainFrame);
				return;
			}

			if (bRq)
			{
				TRACE0 ("Not read event flags\n");
				theApp.SetMarkAllAsUnread_flag(FALSE);
				IVM_PostModelessMessageBox(IDS_W_LOGFILE_CHGD_ALL_UNREAD,m_hwndMainFrame);
				return;
			}
		}
		if (pRFlag)
			pRFlag->Serialize (ar);
	}
	catch (CArchiveException* e)
	{
		IVM_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch(...)
	{
		AfxMessageBox (_T("System errror (CdMainDoc::Serialize): \nCannot serialize data."));
	}
}

/////////////////////////////////////////////////////////////////////////////
// CdMainDoc diagnostics

#ifdef _DEBUG
void CdMainDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CdMainDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdMainDoc commands

void CdMainDoc::LoadEvents()
{
	try
	{
		CFile f;
		if(f.Open (m_strArchiveFile, CFile::modeRead))
		{
			CArchive ar(&f, CArchive::load);
			Serialize (ar);
		}
	}
	catch(...)
	{
		AfxMessageBox (_T("System error (CdMainDoc::LoadEvents): Cannot load events"));
	}
}


void CdMainDoc::StoreEvents()
{
	try
	{
		CFile f;
		if(f.Open (m_strArchiveFile, CFile::modeCreate|CFile::modeWrite))
		{
			CArchive ar(&f, CArchive::store);
			Serialize (ar);
		}
	}
	catch(...)
	{
		AfxMessageBox (_T("System error (CdMainDoc::StoreEvents): Cannot load events"));
	}
}
