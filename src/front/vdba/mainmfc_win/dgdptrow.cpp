/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdptrow.cpp, Implementation file    (Modeless Dialog)
**
**
**    Project  : Visual DBA
**    Author   : UK Sotheavut (uk$so01)
**
**    Purpose  : It contains a list of rows resulting from the SELECT
**               statement.
**
** History after 04-May-1999:
**
** 17-June-1999 (schph01)
**    bug #97490: changed the "session type"  parameter used in GetSession()
**    (used for fetching the rows) , from SESSION_TYPE_CACHENOREADLOCK to 
**    SESSION_TYPE_INDEPENDENT (since this session isn't used for an 
**    immediate query and released immediatly, but is kept opened for later
**    "fetches" (upon action on the scrollbar for scrolling the rows))
** 04-Oct-1999 (noifr01)
**    bug #99017: (previous fix for bug #97490 has a side effect of a bad response 
**    time against gateways) Restored the session type to SESSION_TYPE_CACHENOREADLOCK  
**    and fixed differently bug #97490 by a minor change in the cache management
**    (in dbaginfo.sc) , sufficient for ensuring that SESSION_TYPE_CACHENOREADLOCK 
**    sessions can be used in the general case (not only for an immediate query and
**    session release)
** 13-Mar-2000 (noifr01)
**    bug 100577 : workaround'ed the problem where a schema starting with
**    a $ sign will result in an SQL error on 6.4 level or lower (gateways ):
**    don't prefix with the schema in that exact case, if the tablename
**    starts with ii (means a system catalog)
** 21-Mar-2000 (noifr01)
**    (bug #100951) specified the session to keep its default isolation level, for the
**    "rows" right pane of a table
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 18-Apr-2001 (noifr01)
**    (bug 104505) a numeric column was displayed as left-aligned (instead of
**    right-aligned)if it was the first displayed column
** 28-may-2001 (schph01)
**    BUG 104636 set with SetFloatDisplayFormat() method ,the current float parameters
**    defined in "SQL Test Preferences".
**  30-May-2001 (uk$so01)
**     SIR #104736, Integrate Ingres Import Assistant.
**  07-Jun-2001 (noifr01)
**   (sir 104881) left aligned remaining non-numeric columns that were still right-aligned
**  18-Jun-2001 (uk$so01)
**     BUG #104799, Deadlock on modify structure when there exist an opened 
**     cursor on the table.
**	26-Nov-2001 (hanje04)
**	27-Nov-2001 (hanje04) - BACKED OUT.
**	    BUG 106461
**	    'Fetching Rows' animation causing hang on UNIX. Disable'
**  10-Jun-2002 (schph01)
**     BUG 107944 Before fetching rows on Gateway verify that schema.table table
**     name format is supported.
**	31-Jul-2002 (hanch04)
**	    Replace CString format call with wsprintf because of 
**	    Mainwin bug with y-umlaut in a string
** 18-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
** 21-Oct-2002 (uk$so01)
**    BUG #107944, Manually integrate the change #457794
** 25-Mar-2003 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
**    Additional fix: DOM/RightPane/Table does not display the rows of table on the
**    remote node.
** 13-Jun-2003 (uk$so01)
**    SIR #106648, Take into account the STAR database for connection.
** 07-Nov-2003 (uk$so01)
**    SIR #106648. Additional fix the session conflict between the container and ocx.
** 02-Aug-2004 (uk$so01)
**    BUG 112766 VDBA/DOM Connect dom through the SERVER CLASS then click on the Row page
**    on the right pane, il results an error.
** 18-Nov-2004 (uk$so01)
**    BUG #113491 / ISSUE #13755178: Add the session description.
** 01-Dec-2004 (uk$so01)
**    VDBA BUG #113548 / ISSUE #13768610 
**    Fix the problem of serialization.
**  02-Jun-2010 (drivi01)
**    Removed BUFSIZE redefinition.  It's not needed here anymore.
**    the constant is defined in main.h now. and main.h is included
**    in dom.h.
**    Remove hard coded buffer size.
** 22-Jun-2010 (maspa05)
**    Bug 123847
**    Call GetConnected when destroying the "DOM/Table/Rows" tab. This checks 
**    for and disconnects any database sessions marked as "released".
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "dgdptrow.h"
#include "wmusrmsg.h"
#include "sqlquery.h"
#include "domseri.h"
#include "font.h"
#include "property.h"
//#define USES_DOM_FONT // Force the sql control to use the same font as Tree in left pane.

extern "C"
{
#include "monitor.h"
#include "dom.h"
#include "tree.h"
#include "main.h"
#include "getdata.h"
#include "domdata.h"
#include "dbaginfo.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CuDlgDomPropTableRows::CuDlgDomPropTableRows(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgDomPropTableRows::IDD, pParent)
{
	m_bSetComponent = FALSE;
	//{{AFX_DATA_INIT(CuDlgDomPropTableRows)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_bExecuted     = FALSE;
	m_strNode = _T("(local)");
	m_strServer = _T("");
	m_strUser = _T("");
	m_strDBName     = _T("");
	m_strTable = _T("");
	m_strTableOwner = _T("");
}


void CuDlgDomPropTableRows::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgDomPropTableRows)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgDomPropTableRows, CDialog)
	//{{AFX_MSG_MAP(CuDlgDomPropTableRows)
	ON_WM_DESTROY()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_MESSAGE (WM_SQL_CLOSE_CURSOR,         OnCloseCursor)
	ON_MESSAGE (WM_QUERY_UPDATING,           OnUpdateQuery)
	ON_MESSAGE (WM_USER_IPMPAGE_LOADING,     OnLoad)
	ON_MESSAGE (WM_USER_IPMPAGE_UPDATEING,   OnUpdateData)
	ON_MESSAGE (WM_USER_IPMPAGE_GETDATA,     OnGetData)
	ON_MESSAGE (WM_USER_DOMPAGE_QUERYTYPE,   OnQueryType)
	ON_MESSAGE (WM_USER_IPMPAGE_LEAVINGPAGE, OnChangeProperty)
	ON_MESSAGE (WM_QUERY_OPEN_CURSOR,        OnQueryOpenCursor)
	ON_MESSAGE (WM_CHANGE_FONT,              OnSettingChange) // Sent by UpdateFontDOMRightPane()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgDomPropTableRows message handlers

LONG CuDlgDomPropTableRows::OnUpdateQuery (WPARAM wParam, LPARAM lParam)
{
	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (pUps);
	if (pUps == NULL)
		return 0L;
	pUps->nIpmHint = -1;
	return 0L;
}

void CuDlgDomPropTableRows::PostNcDestroy() 
{
	delete this;
	CDialog::PostNcDestroy();
}

void CuDlgDomPropTableRows::OnDestroy() 
{

// GetConnected removes sessions marked as "released"

	short p = m_SqlQuery.GetConnected(m_strNode,NULL);

	CDialog::OnDestroy();
}



BOOL CuDlgDomPropTableRows::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CRect r;
	GetClientRect (r);

	if (!theApp.m_pUnknownSqlQuery)
	{
		CuSqlQueryControl ccl;
		HRESULT hError = CoCreateInstance(
			ccl.GetClsid(),
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IUnknown,
			(PVOID*)&(theApp.m_pUnknownSqlQuery));
		if (!SUCCEEDED(hError))
			theApp.m_pUnknownSqlQuery = NULL;
	}

	m_SqlQuery.Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		this,
		IDC_STATIC_01);
	if (!theApp.m_bsqlStreamFile)
	{
		LoadSqlQueryPreferences(m_SqlQuery.GetControlUnknown());
	}
	//
	// Load the global preferences:
	if (theApp.m_bsqlStreamFile)
	{
		IPersistStreamInit* pPersistStreammInit = NULL;
		IUnknown* pUnk = m_SqlQuery.GetControlUnknown();
		HRESULT hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
		if(SUCCEEDED(hr) && pPersistStreammInit)
		{
			theApp.m_sqlStreamFile.SeekToBegin();
			IStream* pStream = theApp.m_sqlStreamFile.GetStream();
			hr = pPersistStreammInit->Load(pStream);
			pPersistStreammInit->Release();
		}
	}
#if defined (USES_DOM_FONT)
	HFONT hFont = hFontBrowsers;
	if (hFont)
		SendMessage (WM_CHANGE_FONT, 0, (LPARAM)hFont);
#endif

	m_SqlQuery.SetSessionStart(1000);
	m_SqlQuery.SetSessionDescription(_T("Ingres Visual DBA (DOM/Table/Rows) invokes Visual SQL control (sqlquery.ocx)"));
	m_SqlQuery.CreateSelectResultPage(_T(""), _T(""), _T(""), _T(""), NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgDomPropTableRows::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	ResizeControls();
}


LONG CuDlgDomPropTableRows::OnGetData (WPARAM wParam, LPARAM lParam)
{
	CuDomPropDataTableRows* pData    = NULL;
	try
	{
		int nWhat = (int)wParam;
		if (wParam == 1) // Request for sql control
		{
			IUnknown* pUnknown = m_SqlQuery.m_hWnd? m_SqlQuery.GetControlUnknown(): NULL;
			return (LONG)pUnknown;
		}

		pData = new CuDomPropDataTableRows();
		COleStreamFile& streamfile = pData->GetStreamFile();
		IStream* pStream = NULL;
		m_SqlQuery.Storing((LPUNKNOWN*)&pStream);
		if(pStream)
		{
			streamfile.Attach(pStream);
		}
	}
	catch (...)
	{
		//
		// Cannot save the query result.
		AfxMessageBox (IDS_E_QUERY_RESULT);
		if (pData)
			delete pData;
		return NULL;
	}
	
	return (LONG)pData;
}


void CuDlgDomPropTableRows::ResizeControls()
{
	CRect rDlg;
	GetClientRect (rDlg);
	if (!IsWindow (m_SqlQuery.m_hWnd))
		return;
	m_SqlQuery.MoveWindow(rDlg);
}

LONG CuDlgDomPropTableRows::OnLoad(WPARAM wParam, LPARAM lParam) 
{
	LPCTSTR pClass = (LPCTSTR)wParam;
	ASSERT (lstrcmp (pClass, _T("CuDomPropDataTableRows")) == 0);
	CuDomPropDataTableRows* pData = (CuDomPropDataTableRows*)lParam;
	
	ASSERT (pData);
	if (!pData)
		return 0L;

	COleStreamFile& streamfile = pData->GetStreamFile();
	streamfile.SeekToBegin();
	IStream* pStream = streamfile.GetStream();
	if(pStream)
	{
		m_SqlQuery.Loading(pStream);
	}

	return 0L;
}




LONG CuDlgDomPropTableRows::OnQueryType(WPARAM wParam, LPARAM lParam)
{
	ASSERT (wParam == 0);
	ASSERT (lParam == 0);
	return DOMPAGE_SPECIAL;
}

//
// When this message is resulting from the selection changes of Tab Control
// in the right pane, the pUps->nIpmHint should be: -1.
LONG CuDlgDomPropTableRows::OnUpdateData (WPARAM wParam, LPARAM lParam)
{
	USES_CONVERSION;
	int iret = RES_ERR;
	CWaitCursor waitCursor;
	BOOL bCheck = FALSE;

	LPIPMUPDATEPARAMS pUps = (LPIPMUPDATEPARAMS)lParam;
	ASSERT (pUps != NULL);
	if (pUps == NULL)
		return 0L;

	switch (pUps->nIpmHint)
	{
	case 0:
		bCheck = FALSE;
		m_bExecuted = FALSE;
		break;
	case -1:
		bCheck = TRUE;
		break;
	default:
		//
		// nothing to change on the display
		return 0L;
	}

	try
	{
		if (!m_bSetComponent)
		{
			TCHAR tchszObject [MAXOBJECTNAME*2];
			LPTREERECORD  pRecord = (LPTREERECORD)pUps->pStruct;
			ASSERT (pRecord);
			if (!pRecord)
				return 0;
			m_strDBName    = A2T((char*)pRecord->extra);
			m_strTable = A2T((char*)StringWithoutOwner (pRecord->objName));
			m_strTableOwner = A2T((char*)pRecord->ownerName);
			/* workaround of problem (bug 100577), where a schema starting 
			   with a $ sign will result in an SQL error on 6.4 level or
			   lower (gateways ): don't prefix with the schema in that case, if the tablename
			   starts with ii (means a system catalog)
			   ( GetOIVers()== OIVERS_NOTOI means a server without OpenIngres 1.x or higher functionalities,
				 i.e. Ingres 6.4 or lower, AND "regular" gateways (as opposes to Ingres Desktop which has a 
				 very specific management) */
			if (GetOIVers() == OIVERS_NOTOI && pRecord->ownerName[0]=='$' && !x_strnicmp((char *)pRecord->objName,"II",2)) 
				_tcscpy(tchszObject,_T(""));
			else
			{
				GATEWAYCAPABILITIESINFO GWInfo;
				TABLEPARAMS TblInfo;
				memset (&GWInfo,_T('\0'),sizeof(GATEWAYCAPABILITIESINFO));
				memset (&TblInfo,_T('\0'),sizeof(TABLEPARAMS));
				if ( GetOIVers() == OIVERS_NOTOI &&
				     GetCapabilitiesInfo(&TblInfo, &GWInfo ) == RES_SUCCESS &&
				     !GWInfo.bOwnerTableSupported)
					_tcscpy(tchszObject,_T(""));
				else
				{
					_tcscpy(tchszObject,QuoteIfNeeded((LPCTSTR)pRecord->ownerName));
					_tcscat(tchszObject,_T("."));
				}
			}
			lstrcat(tchszObject,QuoteIfNeeded(RemoveDisplayQuotesIfAny((LPCTSTR)StringWithoutOwner(pRecord->objName))));
			m_strStatement.Format (_T("select * from %s"), tchszObject);
		}

		if (!m_bExecuted)
		{
			m_strNode = (LPTSTR)GetVirtNodeName (GetCurMdiNodeHandle());
			m_bExecuted = TRUE;
			m_SqlQuery.ShowWindow (SW_SHOW);
			UCHAR szGw[200];
			char* node = T2A((LPTSTR)(LPCTSTR)m_strNode);

			BOOL bHasGWSuffix = GetGWClassNameFromString((LPUCHAR)node, szGw);
			RemoveGWNameFromString((LPUCHAR)node);
			m_strServer = szGw;
			m_strNode = node;

			if ( IsStarDatabase(GetCurMdiNodeHandle(), (LPUCHAR)(LPCTSTR)m_strDBName))
			{
				UINT nFlag = DBFLAG_STARNATIVE;
				m_SqlQuery.CreateSelectResultPage4Star(m_strNode, m_strServer, m_strUser, m_strDBName, nFlag, m_strStatement);
			}
			else
				m_SqlQuery.CreateSelectResultPage(m_strNode, m_strServer, m_strUser, m_strDBName, m_strStatement);
			m_SqlQuery.Invalidate();
		}
	}
	catch (...)
	{
	}
	return 0L;
}


LONG CuDlgDomPropTableRows::OnChangeProperty (WPARAM wParam, LPARAM lParam)
{
	if ((int)lParam == 0)
	{
		m_bExecuted = FALSE;
	}
	else
	if ((int)lParam == 1)
	{
		//
		// Right pane Tab selection changes:
	}
	return 0L;
}


long CuDlgDomPropTableRows::OnQueryOpenCursor(WPARAM wParam, LPARAM lParam)
{
	/*UKS
	BOOL bExist = m_pCursor? TRUE: FALSE;
	FINDCURSOR* pFind = (FINDCURSOR*)lParam;
	REQUESTMDIINFO* pInfo = (REQUESTMDIINFO*)wParam;

	if (pFind && m_pCursor)
	{
		BOOL bCompareServer = FALSE;
		TCHAR tchszGateway[200];
		TCHAR tchszNode[MAXOBJECTNAME*4];
		CString strNode = (LPCTSTR)(LPTSTR)GetVirtNodeName (m_nHNode);
		GetGWClassNameFromString((LPUCHAR)(LPCTSTR)strNode, (LPUCHAR)tchszGateway);
		lstrcpy (tchszNode, strNode);
		RemoveGWNameFromString((LPUCHAR)tchszNode);
		RemoveConnectUserFromString((LPUCHAR)tchszNode);
		strNode = tchszNode;

		if (pFind->tchszServer[0] && lstrcmpi (pFind->tchszServer, _T("INGRES")) != 0)
			bCompareServer = TRUE;
		if (!bCompareServer && tchszGateway[0] && lstrcmpi (tchszGateway, _T("INGRES")) != 0)
			bCompareServer = TRUE;

		if (strNode.CompareNoCase (pFind->tchszNode) != 0)
			return (long)FALSE;
		//
		// Check the gateway only if it is not INGRES:
		if (bCompareServer && lstrcmpi (tchszGateway, pFind->tchszServer) != 0)
			return (long)FALSE;
		if (m_strDBName.CompareNoCase (pFind->tchszDatabase) != 0)
			return (long)FALSE;
		if (m_strTable.CompareNoCase (pFind->tchszTable) != 0)
			return (long)FALSE;
		if (m_strTableOwner.CompareNoCase (pFind->tchszTableOwner) != 0)
			return (long)FALSE;

		if (pFind->bCloseCursor)
			OnCloseCursor(wParam, lParam);
	}
	else
	{
		bExist = FALSE;
	}

	if (bExist && pInfo)
	{
		pInfo->hWndMDI = GetParentFrame()->m_hWnd;
		pInfo->hWnd = m_hWnd;
		pInfo->nInfo= OPEN_CURSOR_DOM;
	}
	*/
	return 0;
	//return (BOOL)bExist;
}

long CuDlgDomPropTableRows::OnSettingChange(WPARAM wParam, LPARAM lParam)
{
#if defined (USES_DOM_FONT)
	USES_CONVERSION;
	HFONT hfont = (HFONT)lParam;
	CFont* pFont = CFont::FromHandle(hfont);
	LOGFONT lf;
	memset (&lf, 0, sizeof(lf));
	if (pFont->GetLogFont(&lf) != 0)
	{
		FONTDESC desc;
		memset( &desc, 0, sizeof( desc ) );
		desc.cbSizeofstruct = sizeof( desc );
		desc.lpstrName = T2OLE(lf.lfFaceName);
		desc.sWeight = short( lf.lfWeight );
		desc.sCharset = lf.lfCharSet;
		desc.fItalic = lf.lfItalic;
		desc.fUnderline = lf.lfUnderline;
		desc.fStrikethrough = lf.lfStrikeOut;

		long lfHeight = lf.lfHeight;
		if (lfHeight < 0)
			lfHeight = -lfHeight;

		//
		// Calculate the point size:
		// Reference to: 
		//    -Article ID: Q74299 (Calculating the Logical Height and Point Size of a Font)
		//    -Article ID: Q74300 (Calculating the Point Size of a Font)
		CWindowDC dc(this);
		int ppi = dc.GetDeviceCaps(LOGPIXELSY);
		desc.cySize.Lo = lfHeight * 720000 / ppi;
		desc.cySize.Hi = 0;

		IFontDispPtr pFontDisp;
		HRESULT hResult = OleCreateFontIndirect( &desc, IID_IFontDisp, (void**)&pFontDisp );
		if( FAILED( hResult ) )
		{
			TRACE( "OleCreateFontIndirect() failed\n" );
			return 0;
		}
		if (pFontDisp)
			m_SqlQuery.SetFont(pFontDisp);
	}
#endif
	return 0;
}

LONG CuDlgDomPropTableRows::OnCloseCursor (WPARAM wParam, LPARAM lParam)
{
	ASSERT(FALSE);
	return 0;
}
