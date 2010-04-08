/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qepdoc.h, header file 
**    Project  : OpenIngres Visual DBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Document, it contains the list of QEP Boxes and its links
**               (QEP Boxes and the links constitute the binary tree).
** History:
** 
** xx-Oct-2001 (uk$so01)
**    Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined(AFX_QEPDOC_H__DC4E108C_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
#define AFX_QEPDOC_H__DC4E108C_46D7_11D1_A20A_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "qepboxdg.h" // Qep Box
#include "qboxleaf.h" // Qep Box (Leaf)
#include "qboxstar.h" // Qep Box (Star)
#include "qboxstlf.h" // Qep Box (Star Leaf)
#include "qepbtree.h" // Binary Tree.

class CdQueryExecutionPlanDoc;
class CaSqlQueryExecutionPlanData: public CObject
{
	DECLARE_SERIAL (CaSqlQueryExecutionPlanData)
public:
	CaSqlQueryExecutionPlanData();
	CaSqlQueryExecutionPlanData(QepType qeptype, LPCTSTR lpszHeader = NULL, CaSqlQueryExecutionPlanBinaryTree* pTree = NULL);
	virtual ~CaSqlQueryExecutionPlanData();
	virtual void Serialize(CArchive& ar);
	
	CaSqlQueryExecutionPlanBinaryTree* AddQEPRootNode (int nodeID, CaQepNodeInformation* pNodeInfo = NULL);
	CaSqlQueryExecutionPlanBinaryTree* AddQEPNode     (int nodeID, int ParentID, int nSon, CaQepNodeInformation* pNodeInfo = NULL);

	void AddLink (int nParentID, int nSonID, int nSon);
	void AddLink (CaQepNodeInformation* pParent, CaQepNodeInformation* pSon, int nSon);

	void QEPBoxesInitPosition (
		CaSqlQueryExecutionPlanBinaryTree* pNode, 
		CdQueryExecutionPlanDoc* pDoc,
		CaSqlQueryExecutionPlanBinaryTree* pParent = NULL,
		int level = 0);
	CaSqlQueryExecutionPlanBinaryTree* FindNodeID (CaSqlQueryExecutionPlanBinaryTree* pTree, int nodeID);

	void GetMaxPosition (CaSqlQueryExecutionPlanBinaryTree* pTree, int& nMax);
	void QEPBoxesChangePosition (
		CaSqlQueryExecutionPlanBinaryTree* pNode, 
		CdQueryExecutionPlanDoc* pDoc);
	void SetDisplayMode (BOOL bModePreview = TRUE);
	BOOL GetDisplayMode (){return m_bPreview;}
	CaQepNodeInformation* FindNodeInfo (CWnd* pPreviewBox);

	void SetCalDrawAreaPreview(BOOL bCalculated = TRUE){m_bCalDrawAreaPreview = bCalculated;}
	void SetCalDrawArea(BOOL bCalculated = TRUE){m_bCalDrawArea = bCalculated;}
	BOOL IsCalDrawAreaPreview(){return m_bCalDrawAreaPreview;}
	BOOL IsCalDrawArea(){return m_bCalDrawArea;}

	//
	// Data member.
	QepType m_qepType;                // Indicate the type of Qep (Normal, Star)
	CString m_strHeader;              // Header of Qep. Ex: Query Plan ... Main Query.
	CString m_strGenerateTable;       // Header. Name of table.
	BOOL    m_bTimeOut;               // Header.
	BOOL    m_bLargeTemporaries;      // Header.
	BOOL    m_bFloatIntegerException; // Header.
	CStringList m_strlistAggregate;   // Header.
	CStringList m_strlistExpression;  // Header.

	//
	// Qep's Tree.
	CaSqlQueryExecutionPlanBinaryTree* m_pQepBinaryTree;

	//
	// Internal Use.
	// -------------
	// For the visual design, the binary tree has a list of links, that is, the lines drawn
	// from the parent to the children (left and/or right)
	CTypedPtrList< CObList, CaSqlQueryExecutionPlanBoxLink* > m_listLink;
	CSize m_sizeQepBox;               // Size of Qep's Box
	CSize m_sizeQepBoxPreview;        // Size of Qep's Box (preview mode)
	CSize m_sizeDrawingArea;          // Client size of SCrollView for displaying the Qep's tree.
	CSize m_sizeDrawingAreaPreview;   // Client size of SCrollView for displaying the Qep's tree. (Preview)
	int m_xScroll;
	int m_yScroll;
	int m_xScrollPreview;
	int m_yScrollPreview;

protected:
	BOOL m_bCalDrawAreaPreview;  // Indicate if the Drawing area has been calculated for the Preview mode.
	BOOL m_bCalDrawArea;         // Indicate if the Drawing area has been calculated for the Normal mode.
	BOOL m_bPreview;             // Mode preview (if TRUE)
	void StoreBinaryTree  (CaSqlQueryExecutionPlanBinaryTree* pNode, CArchive& ar);
	void RestoreBinaryTree(int nCount, CArchive& ar);
	void MemoryQepBoxPositions (CaSqlQueryExecutionPlanBinaryTree* pTree);
};


class CdQueryExecutionPlanDoc : public CDocument
{
	DECLARE_SERIAL (CdQueryExecutionPlanDoc)
public:
	CdQueryExecutionPlanDoc();

public:
	BOOL  m_bFromSerialize;
	int   m_cyHeader;                 // Height  taken to draw the header.
	CSize m_sizeQepBoxPadding;        // Spacing between two Qep box (leaf level)
	CSize m_sizeQepBoxPaddingPreview; // Spacing between two Qep box Preview (leaf level)

	//
	// For a statement of QEP, there may be more than one binary tree data.
	// So a document conatains a list of the binary tree.
	CTypedPtrList< CObList, CaSqlQueryExecutionPlanData* > m_listQepData;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CdQueryExecutionPlanDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

public:
	virtual ~CdQueryExecutionPlanDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

	// Generated message map functions
protected:
	//{{AFX_MSG(CdQueryExecutionPlanDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QEPDOC_H__DC4E108C_46D7_11D1_A20A_00609725DDAF__INCLUDED_)
