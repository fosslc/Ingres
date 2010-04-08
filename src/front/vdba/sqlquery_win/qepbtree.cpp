/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*

/*
**    Source   : qepbtree.cpp, Implementation file    (Binary Tree) 
**    Project  : CA-OpenIngres Visual DBA.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Binary Tree data for Qep
**
** xx-Oct-1997 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/


#include "stdafx.h"
#include "rcdepend.h"
#include "qepbtree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Implementation of Node Information
IMPLEMENT_SERIAL (CaQepNodeInformation, CObject, 1)
CaQepNodeInformation::CaQepNodeInformation()
	: m_pQepBox(NULL), m_pQepBoxPreview(NULL), m_bFromSerialization(FALSE)
{
	m_bRoot    = FALSE;
	m_qepNode  = QEP_NODE_INTERNAL;
	m_rcQepBox = CRect (0, 0, 0, 0);
	m_nCPUCost = 0;
	m_nDiskCost= 0;
	m_nNetCost = 0;
	m_crColorPreview = GetSysColor (COLOR_MENU);
	m_rcQepBoxPreview= CRect (0, 0, 0, 0);
}


CaQepNodeInformation::CaQepNodeInformation(QepNode node)
	:m_pQepBox(NULL),  m_pQepBoxPreview(NULL), m_bFromSerialization(FALSE)
{
	m_bRoot   = FALSE;
	m_qepNode = node;
	m_rcQepBox= CRect (0, 0, 0, 0);
	m_nCPUCost = 0;
	m_nDiskCost= 0;
	m_nNetCost = 0;
	m_crColorPreview = GetSysColor (COLOR_MENU);
	m_rcQepBoxPreview = CRect (0, 0, 0, 0);
}

CaQepNodeInformation::~CaQepNodeInformation()
{
	while (!m_listPercent1.IsEmpty())
		delete m_listPercent1.RemoveHead();
	while (!m_listPercent2.IsEmpty())
		delete m_listPercent2.RemoveHead();
}


void CaQepNodeInformation::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_bPreview;
		CWnd* pBox = GetQepBox(m_bPreview);
		if (pBox)
		{
			CWnd* pParent = pBox->GetParent();
			if (m_bPreview)
			{
				pBox->GetWindowRect (m_rcQepBoxPreview);
				pParent->ScreenToClient (m_rcQepBoxPreview);
			}
			else
			{
				pBox->GetWindowRect (m_rcQepBox);
				pParent->ScreenToClient (m_rcQepBox);
			}
		}
		ar << TRUE; // m_bFromSerialization;
		ar.Write(&m_wndPlacement, sizeof(m_wndPlacement));

		ar << m_nIcon;
		ar << m_strNodeType;
		ar << m_strNodeHeader;
		ar << m_strPage;
		ar << m_strTuple;
		ar << m_strC;
		ar << m_strD;
		ar << m_qepNode;
		ar << m_bRoot;
		ar << m_rcQepBox;
		ar << m_rcQepBoxPreview;
		ar << m_crColorPreview;
		ar << m_iPosX;
	}
	else
	{
		int iNode;
		ar >> m_bPreview;
		ar >> m_bFromSerialization; // TRUE
		ar.Read(&m_wndPlacement, sizeof(m_wndPlacement));

		ar >> m_nIcon;
		ar >> m_strNodeType;
		ar >> m_strNodeHeader;
		ar >> m_strPage;
		ar >> m_strTuple;
		ar >> m_strC;
		ar >> m_strD;
		ar >> iNode; m_qepNode = QepNode(iNode);
		ar >> m_bRoot;
		ar >> m_rcQepBox;
		ar >> m_rcQepBoxPreview;
		ar >> m_crColorPreview;
		ar >> m_iPosX;
	}
	m_listPercent1.Serialize (ar);
	m_listPercent2.Serialize (ar);
}

void CaQepNodeInformation::SetDisplayMode (BOOL bPreview)
{
	m_bPreview = bPreview;
	if (bPreview)
	{
		if (m_pQepBox && IsWindow (m_pQepBox->m_hWnd) && m_pQepBox->IsWindowVisible())
			m_pQepBox->ShowWindow (SW_HIDE);
	}
	else
	{
		if (m_pQepBoxPreview && IsWindow (m_pQepBoxPreview->m_hWnd) && m_pQepBoxPreview->IsWindowVisible())
			m_pQepBoxPreview->ShowWindow (SW_HIDE);
	}
}


void CaQepNodeInformation::AllocateQepBox(BOOL bPreview)
{

	if (m_qepNode == QEP_NODE_LEAF)
	{
		if (bPreview)
		{
			CuDlgQueryExecutionPlanBoxPreview* pBox = new CuDlgQueryExecutionPlanBoxPreview ();
			pBox->SetDisplayIndicator (FALSE);
			m_pQepBoxPreview = (CWnd*)pBox;
		}
		else
		{
			CuDlgQueryExecutionPlanBoxLeaf* pBox = new CuDlgQueryExecutionPlanBoxLeaf ();
			m_pQepBox = (CWnd*)pBox;
		}
	}
	else
	{
		if (bPreview)
		{
			CuDlgQueryExecutionPlanBoxPreview* pBox = new CuDlgQueryExecutionPlanBoxPreview ();
			pBox->SetDisplayIndicator (TRUE);
			m_pQepBoxPreview = (CWnd*)pBox;
		}
		else
		{
			CuDlgQueryExecutionPlanBox* pBox = new CuDlgQueryExecutionPlanBox ();
			m_pQepBox = (CWnd*)pBox;
		}
	}
}

BOOL CaQepNodeInformation::CreateQepBox(CWnd* pParent, BOOL bPreview)
{
	if (bPreview)
	{
		ASSERT (m_pQepBoxPreview && pParent);
		if (!(m_pQepBoxPreview && pParent))
			return FALSE;
		return ((CuDlgQueryExecutionPlanBoxPreview*)m_pQepBoxPreview)->Create (IDD_QEPBOX_PREVIEW, pParent);
	}
	else
	{
		ASSERT (m_pQepBox && pParent);
		if (!(m_pQepBox && pParent))
			return FALSE;
		if (m_qepNode == QEP_NODE_LEAF)
			return ((CuDlgQueryExecutionPlanBoxLeaf*)m_pQepBox)->Create (IDD_QEPBOX_LEAF, pParent);
		else
			return ((CuDlgQueryExecutionPlanBox*)m_pQepBox)->Create (IDD_QEPBOX, pParent);
	}
	return TRUE;
}
	

//
// Implementation of Node Information for star
IMPLEMENT_SERIAL (CaQepNodeInformationStar, CaQepNodeInformation, 1)
CaQepNodeInformationStar::CaQepNodeInformationStar(): CaQepNodeInformation()
{

}


CaQepNodeInformationStar::CaQepNodeInformationStar(QepNode node): CaQepNodeInformation(node)
{

}

CaQepNodeInformationStar::~CaQepNodeInformationStar()
{
	while (!m_listPercent3.IsEmpty())
		delete m_listPercent3.RemoveHead();
}


void CaQepNodeInformationStar::Serialize (CArchive& ar)
{
	CaQepNodeInformation::Serialize (ar);
	if (ar.IsStoring())
	{
		ar << m_strDatabase;
		ar << m_strN;
		ar << m_strNode;
	}
	else
	{
		ar >> m_strDatabase;
		ar >> m_strN;
		ar >> m_strNode;
	}
	m_listPercent3.Serialize (ar);
}

void CaQepNodeInformationStar::AllocateQepBox(BOOL bPreview)
{

	if (m_qepNode == QEP_NODE_LEAF)
	{
		if (bPreview)
		{
			CuDlgQueryExecutionPlanBoxStarPreview* pBox = new CuDlgQueryExecutionPlanBoxStarPreview ();
			pBox->SetDisplayIndicator (FALSE);
			m_pQepBoxPreview = (CWnd*)pBox;
		}
		else
		{
			CuDlgQueryExecutionPlanBoxStarLeaf* pBox = new CuDlgQueryExecutionPlanBoxStarLeaf ();
			m_pQepBox = (CWnd*)pBox;
		}
	}
	else
	{
		if (bPreview)
		{
			CuDlgQueryExecutionPlanBoxStarPreview* pBox = new CuDlgQueryExecutionPlanBoxStarPreview ();
			pBox->SetDisplayIndicator (TRUE);
			m_pQepBoxPreview = (CWnd*)pBox;
		}
		else
		{
			CuDlgQueryExecutionPlanBoxStar* pBox = new CuDlgQueryExecutionPlanBoxStar ();
			m_pQepBox = (CWnd*)pBox;
		}
	}
}

BOOL CaQepNodeInformationStar::CreateQepBox(CWnd* pParent, BOOL bPreview)
{
	if (bPreview)
	{
		ASSERT (m_pQepBoxPreview && pParent);
		if (!(m_pQepBoxPreview && pParent))
			return FALSE;
		return ((CuDlgQueryExecutionPlanBoxStarPreview*)m_pQepBoxPreview)->Create (IDD_QEPBOX_STAR_PREVIEW, pParent);
	}
	else
	{
		ASSERT (m_pQepBox && pParent);
		if (!(m_pQepBox && pParent))
			return FALSE;
		if (m_qepNode == QEP_NODE_LEAF)
			return ((CuDlgQueryExecutionPlanBoxStarLeaf*)m_pQepBox)->Create (IDD_QEPBOX_STAR_LEAF, pParent);
		else
			return ((CuDlgQueryExecutionPlanBoxStar*)m_pQepBox)->Create (IDD_QEPBOX_STAR, pParent);
	}
	return TRUE;
}



CaSqlQueryExecutionPlanBoxLink::CaSqlQueryExecutionPlanBoxLink()
{
	m_pQepParentNodeInfo  = NULL;
	m_pQepSonNodeInfo     = NULL;
	m_nSon = 0;
}

	
CaSqlQueryExecutionPlanBoxLink::CaSqlQueryExecutionPlanBoxLink(CaQepNodeInformation* pParent, CaQepNodeInformation* pSon, int nSon)
{
	m_pQepParentNodeInfo  = pParent;
	m_pQepSonNodeInfo     = pSon;
	m_nSon                = nSon;
}
	
//
// Implementation of Binary Tree.
IMPLEMENT_SERIAL (CaSqlQueryExecutionPlanBinaryTree, CObject, 1)
CaSqlQueryExecutionPlanBinaryTree::CaSqlQueryExecutionPlanBinaryTree()
{
	m_nID           = 0;
	m_nSon          = 0;
	m_pQepNodeLeft  = NULL;
	m_pQepNodeRight = NULL;
	m_pQepNodeInfo  = NULL;
}

CaSqlQueryExecutionPlanBinaryTree::~CaSqlQueryExecutionPlanBinaryTree()
{
	if (m_pQepNodeInfo)
		delete m_pQepNodeInfo;
}

void CaSqlQueryExecutionPlanBinaryTree::Serialize (CArchive& ar)
{
	if (ar.IsStoring())
	{
		ar << m_nID;
		ar << m_nSon;
		ar << m_nParentID;
		ar << m_pQepNodeInfo;
	}
	else
	{
		ar >> m_nID;
		ar >> m_nSon;
		ar >> m_nParentID;
		ar >> m_pQepNodeInfo;
	}
}

static void GetNodeCount(CaSqlQueryExecutionPlanBinaryTree* pNode, int& nCount)
{
	if (pNode)
	{
		nCount++;
		if (pNode->m_pQepNodeLeft)
			GetNodeCount (pNode->m_pQepNodeLeft, nCount);
		if (pNode->m_pQepNodeRight)
			GetNodeCount (pNode->m_pQepNodeRight, nCount);
	}
}

int CaSqlQueryExecutionPlanBinaryTree::GetCount()
{
	int nCount = 0;
	GetNodeCount(this, nCount);
	return nCount;
}

BOOL CaSqlQueryExecutionPlanBinaryTree::CalcChildrenPercentages(CaSqlQueryExecutionPlanBinaryTree * pRootNode)
{
	int dtot,ctot;
	int dcur,ccur;
	int dprev,cprev;
	double d1,d2,d3;

	if (pRootNode == NULL) {
		ASSERT (!m_nSon);
		pRootNode=this;
	}
	dtot = pRootNode->m_pQepNodeInfo->m_nDiskCost;
	ctot = pRootNode->m_pQepNodeInfo->m_nCPUCost;

	dcur = m_pQepNodeInfo->m_nDiskCost;
	ccur = m_pQepNodeInfo->m_nCPUCost;

	dprev=0;
	cprev=0;
	if (m_pQepNodeLeft) {
		dprev+=m_pQepNodeLeft->m_pQepNodeInfo->m_nDiskCost;
		cprev+=m_pQepNodeLeft->m_pQepNodeInfo->m_nCPUCost;
		m_pQepNodeLeft->CalcChildrenPercentages(pRootNode);
	}
	if (m_pQepNodeRight) {
		dprev+=m_pQepNodeRight->m_pQepNodeInfo->m_nDiskCost;
		cprev+=m_pQepNodeRight->m_pQepNodeInfo->m_nCPUCost;
		m_pQepNodeRight->CalcChildrenPercentages(pRootNode);
	}
	if (!m_pQepNodeLeft && !m_pQepNodeRight)
		m_pQepNodeInfo->m_nIcon        = IDR_QEP_TABLE;        // Icon

	m_pQepNodeInfo->m_strD.Format (_T("%d  (%d)"), dcur, dprev);              // Disk I/O cost.
	m_pQepNodeInfo->m_strC.Format (_T("%d  (%d)"), ccur, cprev);              // CPU usage.         

	d1=dtot;
	d2=dcur;
	d3=dprev;

	if (d1<.5) {
		d1=100.0;
		d2=0.0;
		d3=0.0;
	}
	else {
		d2=(d2-d3)*100.0/d1;
		d3=d3*100.0/d1;
		d1=100.0-d2-d3;
	}
	m_pQepNodeInfo->m_listPercent1.AddTail (new CaStaticIndicatorData (d1 , RGB (192, 192, 192)));
	m_pQepNodeInfo->m_listPercent1.AddTail (new CaStaticIndicatorData (d2,  RGB (0,   0,   255)));
	m_pQepNodeInfo->m_listPercent1.AddTail (new CaStaticIndicatorData (d3,  RGB (0,   255, 255)));

	d1=ctot;
	d2=ccur;
	d3=cprev;
	if (d1<.5) {
		d1=100.0;
		d2=0.0;
		d3=0.0;
	}
	else {
		d2=(d2-d3)*100.0/d1;
		d3=d3*100.0/d1;
		d1=100.0-d2-d3;
	}
	m_pQepNodeInfo->m_listPercent2.AddTail (new CaStaticIndicatorData (d1, RGB (192, 192, 192)));
	m_pQepNodeInfo->m_listPercent2.AddTail (new CaStaticIndicatorData (d2, RGB (0,   128, 0 )));
	m_pQepNodeInfo->m_listPercent2.AddTail (new CaStaticIndicatorData (d3, RGB (0,   255, 0)));

	return TRUE;
}

BOOL CalcSTARChildrenPercentages(CaSqlQueryExecutionPlanBinaryTree * pTreeNode,CaSqlQueryExecutionPlanBinaryTree * pRootNode)
{
	int ntot;
	int ncur;
	int nprev;
	double d1,d2,d3;
	CaQepNodeInformationStar * pStarNode = (CaQepNodeInformationStar *)pTreeNode->m_pQepNodeInfo;
	
	if (pRootNode == NULL) {
		ASSERT (!pTreeNode->m_nSon);
		pRootNode=pTreeNode;
	}
	ntot = pRootNode->m_pQepNodeInfo->m_nNetCost;
	ncur = pTreeNode->m_pQepNodeInfo->m_nNetCost;
	nprev=0;
	if (pTreeNode->m_pQepNodeLeft) {
		nprev+=pTreeNode->m_pQepNodeLeft->m_pQepNodeInfo->m_nNetCost;
		CalcSTARChildrenPercentages(pTreeNode->m_pQepNodeLeft,pRootNode);
	}
	if (pTreeNode->m_pQepNodeRight) {
		nprev+=pTreeNode->m_pQepNodeRight->m_pQepNodeInfo->m_nNetCost;
		CalcSTARChildrenPercentages(pTreeNode->m_pQepNodeRight,pRootNode);
	}
	if (!pTreeNode->m_pQepNodeLeft && !pTreeNode->m_pQepNodeRight)
		pTreeNode->m_pQepNodeInfo->m_nIcon        = IDR_QEP_TABLE;        // Icon

	pStarNode->m_strN.Format(_T("%d  (%d)"), ncur, nprev);  
	
	d1=ntot;
	d2=ncur;
	d3=nprev;
	
	if (d1<.5) {
		d1=100.0;
		d2=0.0;
		d3=0.0;
	}
	else {
		d2=(d2-d3)*100.0/d1;
		d3=d3*100.0/d1;
		d1=100.0-d2-d3;
	}
	pStarNode->m_listPercent3.AddTail (new CaStaticIndicatorData (d1 , RGB (192, 192, 192)));
	pStarNode->m_listPercent3.AddTail (new CaStaticIndicatorData (d2,  RGB (255, 255, 0  )));
	pStarNode->m_listPercent3.AddTail (new CaStaticIndicatorData (d3,  RGB (255, 255, 255)));
	
	return TRUE;
}

BOOL UpdateSTARSiteStrings(CaSqlQueryExecutionPlanBinaryTree * pTreeNode,int inodeno, LPTSTR SiteString)
{
	CaQepNodeInformationStar * pStarNode = (CaQepNodeInformationStar *)pTreeNode->m_pQepNodeInfo;

	static TCHAR *s1=_T("Site ID ");
	static TCHAR *s2=_T(" ->");
	TCHAR buf[200];
	TCHAR* pc;
	
	lstrcpy(buf,pStarNode->m_strDatabase.GetBuffer(0)); 
	if (!_tcsnicmp(buf,s1, _tcslen(s1))) {            // "Site ID" string is string not yet transformed
		pc=buf+_tcslen(s1);
		int i1=_ttoi(pc);
		if (i1==inodeno)
			pStarNode->m_strDatabase=SiteString;
	}
	
	_tcscpy(buf, pStarNode->m_strNode.GetBuffer(0));
	if (buf[0] && _tcsnicmp(buf,s2,_tcslen(s2))) {  // not found yet
		pc=buf+_tcslen(s2);
		int i1=_ttoi(pc);
		if (i1==inodeno) {
			pStarNode->m_strNode = s2;
			pStarNode->m_strNode += SiteString;
		}
	}
	
	if (pTreeNode->m_pQepNodeLeft) 
		UpdateSTARSiteStrings(pTreeNode->m_pQepNodeLeft,inodeno,SiteString);
	if (pTreeNode->m_pQepNodeRight) 
		UpdateSTARSiteStrings(pTreeNode->m_pQepNodeRight,inodeno,SiteString);
	return TRUE;
}
