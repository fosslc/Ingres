/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qepdoc.cpp, Implementation file.
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Document, it contains the list of QEP Boxes and its links
**               (QEP Boxes and the links constitute the binary tree).
** History:
** 
** xx-Oct-2001 (uk$so01)
**    Created.
** 16-feb-2000 (somsa01)
**    In QEPBoxesInitPosition(), typecasted use of pow(), as platforms
**    such as HP have two versions of it.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#include "stdafx.h"
#include <math.h>
#include "rcdepend.h"
#include "qepdoc.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL (CaSqlQueryExecutionPlanData, CObject, 1)

static void DestroyQEPBinaryTree (
	CaSqlQueryExecutionPlanBinaryTree* pTree, 
	CaSqlQueryExecutionPlanBinaryTree* pParentNode = NULL);

void DestroyQEPBinaryTree (
	CaSqlQueryExecutionPlanBinaryTree* pTree, 
	CaSqlQueryExecutionPlanBinaryTree* pParentNode)
{
	if (pTree) // Terminasion condition
	{
		DestroyQEPBinaryTree (pTree->m_pQepNodeLeft, pTree);
		DestroyQEPBinaryTree (pTree->m_pQepNodeRight, pTree);
		//
		// Post-order treatment:
		if (!(pTree->m_pQepNodeLeft || pTree->m_pQepNodeRight))
		{
			if (pParentNode)
			{
				if (pTree->m_nSon == 1)
					pParentNode->m_pQepNodeLeft  = NULL;
				if (pTree->m_nSon == 2)
					pParentNode->m_pQepNodeRight = NULL;
			}
			delete pTree;
		}
	}
}

CaSqlQueryExecutionPlanData::CaSqlQueryExecutionPlanData(): m_qepType(QEP_NORMAL)
{
	m_strHeader = _T("");
	m_pQepBinaryTree = NULL;
	m_bPreview = FALSE;
	m_xScroll = 0;
	m_yScroll = 0;
	m_xScrollPreview = 0;
	m_yScrollPreview = 0;
	m_bCalDrawAreaPreview = FALSE;
	m_bCalDrawArea        = FALSE;
}
	
CaSqlQueryExecutionPlanData::CaSqlQueryExecutionPlanData(QepType qeptype, LPCTSTR lpszHeader, CaSqlQueryExecutionPlanBinaryTree* pTree)
	:m_qepType (qeptype)
{
	m_strHeader = lpszHeader? lpszHeader: _T("");
	m_pQepBinaryTree = pTree; 
	m_bPreview = FALSE;
	m_xScroll = 0;
	m_yScroll = 0;
	m_xScrollPreview = 0;
	m_yScrollPreview = 0;
	m_bCalDrawAreaPreview = FALSE;
	m_bCalDrawArea        = FALSE;
}

CaSqlQueryExecutionPlanData::~CaSqlQueryExecutionPlanData()
{
	TRACE0 ("CaSqlQueryExecutionPlanData::~CaSqlQueryExecutionPlanData()...\n");
	DestroyQEPBinaryTree (m_pQepBinaryTree);
	m_pQepBinaryTree = NULL;
	while (!m_listLink.IsEmpty())
		delete m_listLink.RemoveHead();
}

void CaSqlQueryExecutionPlanData::StoreBinaryTree  (CaSqlQueryExecutionPlanBinaryTree* pNode, CArchive& ar)
{
	if (pNode)
	{
		//
		// Save data
		ar << pNode->m_nID;            // Unique ID to identify the node.
		ar << pNode->m_nSon;           // 0: Root, 1: Left, 2: Right
		ar << pNode->m_nParentID;      // If m_nSon = 0, it is not used.
		ar << pNode->m_pQepNodeInfo;   // More inforation about node.
		if (pNode->m_pQepNodeLeft)
			StoreBinaryTree (pNode->m_pQepNodeLeft, ar);
		if (pNode->m_pQepNodeRight)
			StoreBinaryTree (pNode->m_pQepNodeRight, ar);
	}
}


void CaSqlQueryExecutionPlanData::RestoreBinaryTree(int nCount, CArchive& ar)
{
	int i, nID, nSon, nParentID;

	for (i=0; i<nCount; i++)
	{
		ar >> nID;            // Unique ID to identify the node.
		ar >> nSon;           // 0: Root, 1: Left, 2: Right
		ar >> nParentID;      // If m_nSon = 0, it is not used.

		if (m_qepType == QEP_STAR)
		{
			CaQepNodeInformationStar* pNodeInfo = NULL;
			ar >> pNodeInfo;
			if (i==0)
				AddQEPRootNode (nID, pNodeInfo);
			else
				AddQEPNode (nID, nParentID, nSon, pNodeInfo);
		}
		else
		{
			CaQepNodeInformation* pNodeInfo = NULL;
			ar >> pNodeInfo;
			if (i==0)
				AddQEPRootNode (nID, pNodeInfo);
			else
				AddQEPNode (nID, nParentID, nSon, pNodeInfo);
		}
	}
}

void CaSqlQueryExecutionPlanData::Serialize(CArchive& ar)
{
	m_strlistAggregate.Serialize (ar);
	m_strlistExpression.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_qepType;                // Indicate the type of Qep (Normal, Star);   
		ar << m_strHeader;              // Header of Qep. Ex: Query Plan ... Main Query.         
		ar << m_strGenerateTable;       // Header. Name of table.
		ar << m_bTimeOut; 
		ar << m_bLargeTemporaries;
		ar << m_bFloatIntegerException;

		ar << m_sizeQepBox;
		ar << m_sizeQepBoxPreview;
		ar << m_sizeDrawingArea;
		ar << m_sizeDrawingAreaPreview;
		ar << m_pQepBinaryTree->GetCount();
		ar << m_bPreview;
		ar << m_xScroll;
		ar << m_yScroll;
		ar << m_xScrollPreview;
		ar << m_yScrollPreview;
		ar << m_bCalDrawAreaPreview;
		ar << m_bCalDrawArea;
		//
		// Store the binary tree nodes:
		StoreBinaryTree  (m_pQepBinaryTree, ar);
	}
	else
	{
		int nCount, iMode;
		ar >> iMode; 
		m_qepType = QepType(iMode);     // Indicate the type of Qep (Normal, Star);   
		ar >> m_strHeader;              // Header of Qep. Ex: Query Plan ... Main Query.         
		ar >> m_strGenerateTable;       // Header. Name of table.
		ar >> m_bTimeOut; 
		ar >> m_bLargeTemporaries;
		ar >> m_bFloatIntegerException;

		ar >> m_sizeQepBox;
		ar >> m_sizeQepBoxPreview;
		ar >> m_sizeDrawingArea;
		ar >> m_sizeDrawingAreaPreview;
		ar >> nCount;
		ar >> m_bPreview;
		ar >> m_xScroll;
		ar >> m_yScroll;
		ar >> m_xScrollPreview;
		ar >> m_yScrollPreview;
		ar >> m_bCalDrawAreaPreview;
		ar >> m_bCalDrawArea;
		//
		// Restore the binary tree nodes:
		RestoreBinaryTree  (nCount, ar);
	}
}


CaSqlQueryExecutionPlanBinaryTree* CaSqlQueryExecutionPlanData::AddQEPRootNode (int nodeID, CaQepNodeInformation* pNodeInfo)
{
	ASSERT (m_pQepBinaryTree == NULL);
	try
	{
		m_pQepBinaryTree = new CaSqlQueryExecutionPlanBinaryTree();
		if (pNodeInfo)
			pNodeInfo->SetRoot();
		m_pQepBinaryTree->m_pQepNodeInfo = pNodeInfo;
		m_pQepBinaryTree->m_nID  = nodeID;
		m_pQepBinaryTree->m_nSon = 0;
		m_pQepBinaryTree->m_nParentID = -1;
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaSqlQueryExecutionPlanData::AddQEPRootNode(1): Unknown error "));
	}
	return m_pQepBinaryTree;
}


CaSqlQueryExecutionPlanBinaryTree* CaSqlQueryExecutionPlanData::AddQEPNode (int nodeID, int ParentID, int nSon, CaQepNodeInformation* pNodeInfo)
{
	CaSqlQueryExecutionPlanBinaryTree* pNode   = NULL;
	CaSqlQueryExecutionPlanBinaryTree* pNodeID = NULL;

	pNodeID = FindNodeID (m_pQepBinaryTree, ParentID);
	ASSERT (pNodeID);
	if (!pNodeID)
		return NULL;
	ASSERT (nSon != 0);
	try
	{
		pNode = new CaSqlQueryExecutionPlanBinaryTree ();
		pNode->m_nID = nodeID;
		if (nSon == 1)
			pNodeID->m_pQepNodeLeft  = pNode;
		else
			pNodeID->m_pQepNodeRight = pNode;

		pNode->m_pQepNodeInfo = pNodeInfo;
		pNode->m_nSon      = nSon;
		pNode->m_nParentID = ParentID;
		AddLink (pNodeID->m_pQepNodeInfo, pNode->m_pQepNodeInfo, nSon);
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaSqlQueryExecutionPlanData::AddQEPRootNode(2): Unknown error "));
	}
	return pNode;
}



void CaSqlQueryExecutionPlanData::AddLink (int nParentID, int nSonID, int nSon)
{
	CaSqlQueryExecutionPlanBoxLink* pLink = NULL;
	CaSqlQueryExecutionPlanBinaryTree* pParentID = FindNodeID (m_pQepBinaryTree, nParentID);
	CaSqlQueryExecutionPlanBinaryTree* pSonID    = FindNodeID (m_pQepBinaryTree, nSonID);
	ASSERT (pParentID && pSonID);

	pLink = new CaSqlQueryExecutionPlanBoxLink (pParentID->m_pQepNodeInfo, pSonID->m_pQepNodeInfo, nSon);
	m_listLink.AddTail (pLink);
}

void CaSqlQueryExecutionPlanData::AddLink (CaQepNodeInformation* pParent, CaQepNodeInformation* pSon, int nSon)
{
	CaSqlQueryExecutionPlanBoxLink* pLink = NULL;
	pLink = new CaSqlQueryExecutionPlanBoxLink (pParent, pSon, nSon);
	m_listLink.AddTail (pLink);
}



CaSqlQueryExecutionPlanBinaryTree* CaSqlQueryExecutionPlanData::FindNodeID (CaSqlQueryExecutionPlanBinaryTree* pTree, int nodeID)
{
	CaSqlQueryExecutionPlanBinaryTree* pRet;
	if (pTree)
	{
		if (pTree->m_nID == nodeID)
			return pTree;
		pRet = FindNodeID (pTree->m_pQepNodeLeft,  nodeID);
		if (pRet)
			return pRet;
		pRet = FindNodeID (pTree->m_pQepNodeRight, nodeID);
		if (pRet)
			return pRet;
	}
	return NULL;
}



void CaSqlQueryExecutionPlanData::QEPBoxesInitPosition (
	CaSqlQueryExecutionPlanBinaryTree* pNode, 
	CdQueryExecutionPlanDoc* pDoc,
	CaSqlQueryExecutionPlanBinaryTree* pParent,
	int level)
{
	CPoint center;
	CSize  sizeBox;
	CSize  sizeArea;
	CSize  sizePad;
	int x1,y1, x2, y2;
	if (pNode)
	{
		CaQepNodeInformation* pNodeInfo = pNode->m_pQepNodeInfo;
		if (pNode->m_pQepNodeLeft == NULL && pNode->m_pQepNodeRight == NULL)
			pNodeInfo->SetQepNode (QEP_NODE_LEAF);
		if (m_bPreview)
		{
			sizeBox = m_sizeQepBoxPreview;
			sizeArea= m_sizeDrawingAreaPreview;
			sizePad = pDoc->m_sizeQepBoxPaddingPreview;
		}
		else
		{
			sizeBox = m_sizeQepBox;
			sizeArea= m_sizeDrawingArea;
			sizePad = pDoc->m_sizeQepBoxPadding;
		}

		switch (pNode->m_nSon)
		{
		case 0:
			center = CPoint (sizeArea.cx / 2, pDoc->m_cyHeader + sizeBox.cy/2);
			x1 = center.x-sizeBox.cx/2;
			y1 = center.y-sizeBox.cy/2;
			x2 = center.x+sizeBox.cx/2;
			y2 = center.y+sizeBox.cy/2;
			if (m_bPreview)
				pNodeInfo->m_rcQepBoxPreview = CRect (x1, y1, x2, y2);
			else
				pNodeInfo->m_rcQepBox = CRect (x1, y1, x2, y2);
			break;
		case 1:
			ASSERT (pParent);
			if (m_bPreview)
			{
				center.x = pParent->m_pQepNodeInfo->m_rcQepBoxPreview.left + pParent->m_pQepNodeInfo->m_rcQepBoxPreview.Width()/2 
				         - sizeArea.cx/(int)(2*(double)pow ((double)2, level));
				center.y = pParent->m_pQepNodeInfo->m_rcQepBoxPreview.bottom + sizePad.cy + sizeBox.cy/2;
			}
			else
			{
				center.x = pParent->m_pQepNodeInfo->m_rcQepBox.left + pParent->m_pQepNodeInfo->m_rcQepBox.Width()/2 
				         - sizeArea.cx/(int)(2* (double)pow ((double)2, level));
				center.y = pParent->m_pQepNodeInfo->m_rcQepBox.bottom + sizePad.cy + sizeBox.cy/2;
			}
			x1 = center.x-sizeBox.cx/2;
			y1 = center.y-sizeBox.cy/2;
			x2 = center.x+sizeBox.cx/2;
			y2 = center.y+sizeBox.cy/2;
			if (m_bPreview)
				pNodeInfo->m_rcQepBoxPreview = CRect (x1, y1, x2, y2);
			else
				pNodeInfo->m_rcQepBox = CRect (x1, y1, x2, y2);
			break;
		case 2:
			ASSERT (pParent);
			if (m_bPreview)
			{
				center.x = pParent->m_pQepNodeInfo->m_rcQepBoxPreview.left + pParent->m_pQepNodeInfo->m_rcQepBoxPreview.Width()/2 
				         + sizeArea.cx/(int)(2* (double)pow ((double)2, level));
				center.y = pParent->m_pQepNodeInfo->m_rcQepBoxPreview.bottom + sizePad.cy + sizeBox.cy/2;
			}
			else
			{
				center.x = pParent->m_pQepNodeInfo->m_rcQepBox.left + pParent->m_pQepNodeInfo->m_rcQepBox.Width()/2 
				         + sizeArea.cx/(int)(2* (double)pow ((double)2, level));
				center.y = pParent->m_pQepNodeInfo->m_rcQepBox.bottom + sizePad.cy + sizeBox.cy/2;
			}
			x1 = center.x-sizeBox.cx/2;
			y1 = center.y-sizeBox.cy/2;
			x2 = center.x+sizeBox.cx/2;
			y2 = center.y+sizeBox.cy/2;
			if (m_bPreview)
				pNodeInfo->m_rcQepBoxPreview = CRect (x1, y1, x2, y2);
			else
				pNodeInfo->m_rcQepBox = CRect (x1, y1, x2, y2);
			break;
		default:
			break;
		}
		QEPBoxesInitPosition (pNode->m_pQepNodeLeft,  pDoc, pNode, level + 1);
		QEPBoxesInitPosition (pNode->m_pQepNodeRight, pDoc, pNode, level + 1);
	}
}





void CaSqlQueryExecutionPlanData::GetMaxPosition (CaSqlQueryExecutionPlanBinaryTree* pTree, int& nMax)
{
	if (pTree)
	{
		if (m_bPreview)
			nMax = max (pTree->m_pQepNodeInfo->m_rcQepBoxPreview.right, nMax);
		else
			nMax = max (pTree->m_pQepNodeInfo->m_rcQepBox.right, nMax);
		GetMaxPosition (pTree->m_pQepNodeLeft, nMax);
		GetMaxPosition (pTree->m_pQepNodeRight, nMax);
	}
}

void CaSqlQueryExecutionPlanData::QEPBoxesChangePosition (CaSqlQueryExecutionPlanBinaryTree* pNode, CdQueryExecutionPlanDoc* pDoc)
{
	if (pNode)
	{
		CSize sizeBox;
		CSize sizePad;
		if (m_bPreview)
		{
			sizeBox = m_sizeQepBoxPreview;
			sizePad = pDoc->m_sizeQepBoxPaddingPreview;
		}
		else
		{
			sizeBox = m_sizeQepBox;
			sizePad = pDoc->m_sizeQepBoxPadding;
		}

		if ((pNode->m_pQepNodeInfo->m_iPosX % 2) == 0)
		{
			if (m_bPreview)
			{
				pNode->m_pQepNodeInfo->m_rcQepBoxPreview.left = 2 + pNode->m_pQepNodeInfo->m_iPosX*(sizeBox.cx + sizePad.cx)/2;
				pNode->m_pQepNodeInfo->m_rcQepBoxPreview.right= pNode->m_pQepNodeInfo->m_rcQepBoxPreview.left + sizeBox.cx;
			}
			else
			{
				pNode->m_pQepNodeInfo->m_rcQepBox.left = 2 + pNode->m_pQepNodeInfo->m_iPosX*(sizeBox.cx + sizePad.cx)/2;
				pNode->m_pQepNodeInfo->m_rcQepBox.right= pNode->m_pQepNodeInfo->m_rcQepBox.left + sizeBox.cx;
			}
		}
		else
		{
			if (m_bPreview)
			{
				pNode->m_pQepNodeInfo->m_rcQepBoxPreview.left = 
					2 + (sizeBox.cx+sizePad.cx)/2 + (pNode->m_pQepNodeInfo->m_iPosX-1)*(sizeBox.cx + sizePad.cx)/2;
				pNode->m_pQepNodeInfo->m_rcQepBoxPreview.right= pNode->m_pQepNodeInfo->m_rcQepBoxPreview.left + sizeBox.cx;
			}
			else
			{
				pNode->m_pQepNodeInfo->m_rcQepBox.left = 
					2 + (sizeBox.cx+sizePad.cx)/2 + (pNode->m_pQepNodeInfo->m_iPosX-1)*(sizeBox.cx + sizePad.cx)/2;
				pNode->m_pQepNodeInfo->m_rcQepBox.right= pNode->m_pQepNodeInfo->m_rcQepBox.left + sizeBox.cx;
			}
		}

		QEPBoxesChangePosition (pNode->m_pQepNodeLeft, pDoc);
		QEPBoxesChangePosition (pNode->m_pQepNodeRight, pDoc);
	}
}

void CaSqlQueryExecutionPlanData::MemoryQepBoxPositions (CaSqlQueryExecutionPlanBinaryTree* pTree)
{
	if (pTree)
	{
		CaQepNodeInformation* pNodeInfo = pTree->m_pQepNodeInfo;
		if (pNodeInfo)
		{
			CWnd* pBox = pNodeInfo->GetQepBox (m_bPreview);
			//
			// The Qep box might not have been created yet. 
			if (pBox)
			{
				CWnd* pParent = pBox->GetParent();
				if (m_bPreview)
				{
					pBox->GetWindowRect (pNodeInfo->m_rcQepBoxPreview);
					pParent->ScreenToClient (pNodeInfo->m_rcQepBoxPreview);
				}
				else
				{
					pBox->GetWindowRect (pNodeInfo->m_rcQepBox);
					pParent->ScreenToClient (pNodeInfo->m_rcQepBox);
				}
			}
		}
		MemoryQepBoxPositions (pTree->m_pQepNodeLeft);
		MemoryQepBoxPositions (pTree->m_pQepNodeRight);
	}
}

void CaSqlQueryExecutionPlanData::SetDisplayMode (BOOL bModePreview)
{
	//
	// Memory the position of Qep boxes (normal mode) before switching 
	// to the preview mode:
	MemoryQepBoxPositions (m_pQepBinaryTree);
	m_bPreview = bModePreview;
}

static  CaQepNodeInformation* GetNodeInfoFromPreview (CaSqlQueryExecutionPlanBinaryTree* pTree, CWnd* pPreviewBox)
{
	if (pTree)
	{
		CaQepNodeInformation* pNodeInfo = pTree->m_pQepNodeInfo;
		if (pNodeInfo)
		{
			CWnd* pBox = pNodeInfo->GetQepBox (TRUE); // Mode Preview
			ASSERT (pBox);
			if (pBox == pPreviewBox)
				return pNodeInfo;
		}
		CaQepNodeInformation* pFound = GetNodeInfoFromPreview (pTree->m_pQepNodeLeft, pPreviewBox);
		if (pFound)
			return pFound;
		else
			return GetNodeInfoFromPreview (pTree->m_pQepNodeRight, pPreviewBox);
	}
	return NULL;
}


CaQepNodeInformation* CaSqlQueryExecutionPlanData::FindNodeInfo (CWnd* pPreviewBox)
{
	return GetNodeInfoFromPreview (m_pQepBinaryTree, pPreviewBox);
}










/////////////////////////////////////////////////////////////////////////////
// CdQueryExecutionPlanDoc
IMPLEMENT_SERIAL (CdQueryExecutionPlanDoc, CDocument, 1)

BEGIN_MESSAGE_MAP(CdQueryExecutionPlanDoc, CDocument)
	//{{AFX_MSG_MAP(CdQueryExecutionPlanDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CdQueryExecutionPlanDoc construction/destruction

CdQueryExecutionPlanDoc::CdQueryExecutionPlanDoc()
{
	m_cyHeader = 5;
	m_sizeQepBoxPadding.cx = 16;
	m_sizeQepBoxPadding.cy = 20; //26;
	m_sizeQepBoxPaddingPreview.cx =  8;
	m_sizeQepBoxPaddingPreview.cy = 16;
	m_bFromSerialize       = FALSE;
}

CdQueryExecutionPlanDoc::~CdQueryExecutionPlanDoc()
{
	TRACE0 ("CdQueryExecutionPlanDoc::~CdQueryExecutionPlanDoc()...\n");
	while (!m_listQepData.IsEmpty())
		delete m_listQepData.RemoveHead();
}

BOOL CdQueryExecutionPlanDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CdQueryExecutionPlanDoc serialization

void CdQueryExecutionPlanDoc::Serialize(CArchive& ar)
{
	m_listQepData.Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_cyHeader;
		ar << m_sizeQepBoxPadding;
		ar << m_sizeQepBoxPaddingPreview;
	}
	else
	{	
		ar >> m_cyHeader;
		ar >> m_sizeQepBoxPadding;
		ar >> m_sizeQepBoxPaddingPreview;
		m_bFromSerialize = TRUE;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CdQueryExecutionPlanDoc diagnostics

#ifdef _DEBUG
void CdQueryExecutionPlanDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CdQueryExecutionPlanDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CdQueryExecutionPlanDoc commands

