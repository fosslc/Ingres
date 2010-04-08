/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ipmaxvie.cpp : implementation File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : interface of the view CvIpm class  (MFC frame/view/doc).
**
** History:
**
** 20-Fev-2002 (uk$so01)
**    Created
**    SIR #106648, Integrate ipm.ocx.
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 17-Jul-2003 (uk$so01)
**    SIR #106648, Vdba-Split, ensure that the ingres sessions 
**    have the descriptions.
** 07-Nov-2003 (uk$so01)
**    SIR #106648. Additional fix the session conflict between the container and ocx.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "ipmaxdoc.h"
#include "ipmaxvie.h"
#include "ipmaxfrm.h"
#include "property.h"
#include "cmdline.h"
#define  IDC_IPMCTRL 100

extern "C"
{
	extern LPTSTR VdbaGetTemporaryFileName();
}
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CvIpm, CView)

BEGIN_MESSAGE_MAP(CvIpm, CView)
	//{{AFX_MSG_MAP(CvIpm)
	ON_WM_CREATE()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// Standard printing commands
#if defined (_PRINT_ENABLE_)
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
#endif
END_MESSAGE_MAP()


static const int IpmTreeSelChange   = 1;
static const int IpmPropertiesChange = 2;
BEGIN_EVENTSINK_MAP(CvIpm, CView)
	ON_EVENT(CvIpm, IDC_IPMCTRL, IpmTreeSelChange ,  OnIpmTreeSelChange, VTS_NONE)
	ON_EVENT(CvIpm, IDC_IPMCTRL, IpmPropertiesChange, OnPropertyChange, VTS_NONE)
END_EVENTSINK_MAP()

/////////////////////////////////////////////////////////////////////////////
// CvIpm construction/destruction

CvIpm::CvIpm()
{
}

CvIpm::~CvIpm()
{
}

BOOL CvIpm::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CvIpm drawing

void CvIpm::OnDraw(CDC* pDC)
{
	CdIpm* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

/////////////////////////////////////////////////////////////////////////////
// CvIpm printing

BOOL CvIpm::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CvIpm::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CvIpm::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CvIpm diagnostics

#ifdef _DEBUG
void CvIpm::AssertValid() const
{
	CView::AssertValid();
}

void CvIpm::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CdIpm* CvIpm::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CdIpm)));
	return (CdIpm*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CvIpm message handlers

int CvIpm::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CdIpm* pIpmDoc = GetDocument();
	CfIpm* pFrame = (CfIpm*)GetParentFrame();
	pFrame->SetIpmDoc(pIpmDoc);

	CRect r;
	GetClientRect (r);
	m_cIpm.Create (
		NULL,
		NULL,
		WS_CHILD,
		r,
		this,
		IDC_IPMCTRL);
	m_cIpm.ShowWindow(SW_SHOW);

	if (!theApp.m_bipmStreamFile)
	{
		LoadIpmPreferences(m_cIpm.GetControlUnknown());
	}
	//
	// Load the global preferences:
	if (theApp.m_bipmStreamFile)
	{
		IPersistStreamInit* pPersistStreammInit = NULL;
		IUnknown* pUnk = m_cIpm.GetControlUnknown();
		HRESULT hr = pUnk->QueryInterface(IID_IPersistStreamInit, (LPVOID*)&pPersistStreammInit);
		if(SUCCEEDED(hr) && pPersistStreammInit)
		{
			theApp.m_ipmStreamFile.SeekToBegin();
			IStream* pStream = theApp.m_ipmStreamFile.GetStream();
			hr = pPersistStreammInit->Load(pStream);
			pPersistStreammInit->Release();
		}
	}

	m_cIpm.SetErrorlogFile(VdbaGetTemporaryFileName());
	m_cIpm.SetSessionStart(2000);
	m_cIpm.SetSessionDescription(_T("Ingres Visual DBA invokes Visual Performance Monitor Control (ipm.ocx)"));
	m_cIpm.SetHelpFile(VDBA_GetHelpFileName());

	return 0;
}

void CvIpm::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	if (IsWindow(m_cIpm.m_hWnd))
	{
		CRect r;
		GetClientRect (r);
		m_cIpm.MoveWindow(r);
	}
}

void CvIpm::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	int nHint = (int)lHint;
}


void CvIpm::OnIpmTreeSelChange() 
{
	TRACE0("CvIpm::OnIpmTreeSelChange\n");
	CfIpm* pFrame = (CfIpm*)GetParentFrame();
	ASSERT(pFrame);
	if (!pFrame)
		return;
	pFrame->DisableExpresRefresh();
}

void CvIpm::OnPropertyChange() 
{
	theApp.PropertyChange(TRUE);
}

void CvIpm::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	CdIpm* pIpmDoc = GetDocument();
	CfIpm* pFrame = (CfIpm*)GetParentFrame();

	BOOL bOK = TRUE;
	short arrayFilter[5];
	CaIpmFrameFilter afilter;
	pFrame->GetFilter(&afilter);
	CArray <short, short>& filter = afilter.GetFilter();
	int i, nSize = filter.GetSize();
	for (i=0; i<min (5, nSize); i++)
		arrayFilter[i] = filter[i];

	if (pIpmDoc->IsLoadedDoc())
	{
		arrayFilter[5] = 0;
		pFrame->SetWindowPlacement(&(pIpmDoc->GetWindowPlacement()));
		CDockState& ToolbarsState = pIpmDoc->GetToolbarState();
		pFrame->SetDockState(ToolbarsState);
		pFrame->RecalcLayout();
		pIpmDoc->SetConnectState(bOK);
		COleStreamFile& ipmStream = pIpmDoc->GetIpmStreamFile();
		IStream* pStream = ipmStream.Detach();
		if (pStream)
		{
			m_cIpm.Loading((LPUNKNOWN)(pStream));
			pStream->Release();
			m_cIpm.Invalidate();
		}
		return;
	}

	m_cIpm.UpdateFilters(arrayFilter, 5);
	if (IsUnicenterDriven())
	{
		CuWindowDesc* pDescriptor = GetAutomatedWindowDescriptor();
		ASSERT (pDescriptor);
		if (pDescriptor)
		{
			ASSERT (pDescriptor->GetType() == TYPEDOC_MONITOR);
			BOOL bHasObjDesc = pDescriptor->HasObjDesc();
			BOOL bHasSingleItem = pDescriptor->HasSingleTreeItem();
			CuIpmObjDesc* pObjDesc = pDescriptor->GetIpmObjDescriptor();
			ASSERT (pObjDesc);
			if (pObjDesc)
			{
				CString strKey = _T("SERVER");
				switch (pObjDesc->GetObjType())
				{
				case OT_MON_SERVER:
					strKey = _T("SERVER");
					break;
				case OT_MON_SESSION:
					strKey = _T("SESSION");
					break;
				case OT_MON_LOGINFO:
					strKey = _T("LOGINFO");
					break;
				case OT_MON_REPLIC_SERVER:
					strKey = _T("REPLICSERVER");
					break;
				default:
					bOK = FALSE;
					break;
				}

				if (bOK)
				{
					VARIANT varSafeArray;
					VariantInit(&varSafeArray);
					varSafeArray.vt = VT_ARRAY;
					varSafeArray.parray = NULL;

					SAFEARRAYBOUND aBound;
					aBound.lLbound   = 0;
					aBound.cElements = 2;
					long   lIndex    = 0;

					varSafeArray.parray = SafeArrayCreate (VT_VARIANT, 1, &aBound);
					int nLevel = pObjDesc->GetLevel();
					VARIANT obj;
					VariantInit(&obj);
					obj.vt = VT_BSTR;
					switch (nLevel)
					{
					case 0:
						obj.bstrVal = pObjDesc->GetObjectName().AllocSysString();
						SafeArrayPutElement(varSafeArray.parray, &lIndex, &obj);
						VariantClear (&obj);
						break;
					case 1:
						obj.bstrVal = pObjDesc->GetParent0Name().AllocSysString();
						SafeArrayPutElement(varSafeArray.parray, &lIndex, &obj);
						SysFreeString(obj.bstrVal);
						lIndex = 1;
						obj.bstrVal = pObjDesc->GetObjectName().AllocSysString();
						SafeArrayPutElement(varSafeArray.parray, &lIndex, &obj);
						VariantClear (&obj);
						break;
					case 2:
						obj.bstrVal = pObjDesc->GetParent0Name().AllocSysString();
						SafeArrayPutElement(varSafeArray.parray, &lIndex, &obj);
						SysFreeString(obj.bstrVal);
						lIndex = 1;
						obj.bstrVal = pObjDesc->GetParent1Name().AllocSysString();
						SafeArrayPutElement(varSafeArray.parray, &lIndex, &obj);
						SysFreeString(obj.bstrVal);
						lIndex = 2;
						obj.bstrVal = pObjDesc->GetObjectName().AllocSysString();
						SafeArrayPutElement(varSafeArray.parray, &lIndex, &obj);
						break;
					default:
						break;
					}

					bOK = m_cIpm.SelectItem(pIpmDoc->GetNode(), pIpmDoc->GetServer(), pIpmDoc->GetUser(), strKey, &varSafeArray, 1);
					VariantClear (&varSafeArray);
				}
			}
		}
	}
	else
	{
		bOK = FALSE;
	}
	if (!bOK)
		bOK = m_cIpm.Initiate(pIpmDoc->GetNode(), pIpmDoc->GetServer(), pIpmDoc->GetUser(), NULL);
	pIpmDoc->SetConnectState(bOK);
}
