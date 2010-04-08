/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*

/*
**    Source   : qepbtree.h, Header file    (Binary Tree) 
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

#if !defined (QEPBTREE_HEADER)
#define QEPBTREE_HEADER
#include "rcdepend.h"
#include "qepboxdg.h" // Qep Box
#include "qboxleaf.h" // Qep Box (Leaf)
#include "qboxstar.h" // Qep Box (Star)
#include "qboxstlf.h" // Qep Box (Star Leaf)
#include "prevboxn.h" // Qep Box (normal preview)
#include "prevboxs.h" // Qep Box (star preview)
//
// Add extra information of your node hear !!
class CaQepNodeInformation: public CObject
{
	DECLARE_SERIAL (CaQepNodeInformation)
public:
	CaQepNodeInformation();
	CaQepNodeInformation(QepNode node);
	void SetQepNode (QepNode node){m_qepNode = node;}
	QepNode GetQepNode (){return m_qepNode;}
	void SetDisplayMode (BOOL bPreview);

	virtual ~CaQepNodeInformation();
	virtual void Serialize (CArchive& ar);
	virtual void AllocateQepBox(BOOL bPreview = FALSE);
	virtual BOOL CreateQepBox(CWnd* pParent, BOOL bPreview = FALSE);
	BOOL  IsRoot() {return m_bRoot;}
	void  SetRoot(BOOL bRoot=TRUE) {m_bRoot = bRoot;}
	CWnd* GetQepBox(BOOL bPreview = FALSE) {return bPreview? m_pQepBoxPreview: m_pQepBox;}
	CRect GetQepBoxRect (BOOL bPreview){return bPreview? m_rcQepBoxPreview: m_rcQepBox;};

	//
	// Data member:

	UINT    m_nIcon;                   // ID of icon for Qep box.
	CString m_strNodeType;             // Type of Qep's node.  Ex: "Proj-rest".
	CString m_strNodeHeader;           // Description of node: Ex: "Sort on (col1)".
	CString m_strPage;                 // Total number of pages.
	CString m_strTuple;                // Total number of Tuples.
	CString m_strC;                    // CPU usage.       (Leaf node does not use this member)
	CString m_strD;                    // Disk I/O cost.   (Leaf node does not use this member)
	CTypedPtrList < CObList, CaStaticIndicatorData* > m_listPercent1; // Bar for CPU.
	CTypedPtrList < CObList, CaStaticIndicatorData* > m_listPercent2; // Bar for Disk I/O
	COLORREF m_crColorPreview;         // In the preview mode, give a the color indicator.
	int m_nDiskCost;
	int m_nCPUCost;
	int m_nNetCost;
	
	int m_iPosX;                       // "logical" horizontal position ( 2 is the minimum between 2 nodes on the same "line"
protected:
	QepNode m_qepNode;
	BOOL m_bFromSerialization;         // TRUE if data is being loaded.
	WINDOWPLACEMENT m_wndPlacement;    // It is not used if m_bFromSerialization = FALSE
	BOOL m_bPreview;
	BOOL m_bRoot;

	CWnd*   m_pQepBox;                 // Qep's Box.
	CWnd*   m_pQepBoxPreview;          // Qep's Box (Preview Mode);
public:
	CRect m_rcQepBox;
	CRect m_rcQepBoxPreview;

};

//
// Add extra information of your node hear !!
class CaQepNodeInformationStar: public CaQepNodeInformation
{
	DECLARE_SERIAL (CaQepNodeInformationStar)
public:
	CaQepNodeInformationStar();
	CaQepNodeInformationStar(QepNode node);

	virtual ~CaQepNodeInformationStar();
	virtual void Serialize (CArchive& ar);
	virtual void AllocateQepBox(BOOL bPreview = FALSE);
	virtual BOOL CreateQepBox(CWnd* pParent, BOOL bPreview = FALSE);

	CString m_strDatabase;            // Database for Star. Ex: "1->2 FRNASU02::XXXX"
	CString m_strNode;                // Node. Ex: "FRNASU02"

	CString m_strN;                   // Net cost.   (Leaf node does not use this member)
	CTypedPtrList < CObList, CaStaticIndicatorData* > m_listPercent3; // Bar for net.

};


class CaSqlQueryExecutionPlanBoxLink: public CObject
{
public:
	CaSqlQueryExecutionPlanBoxLink();
	CaSqlQueryExecutionPlanBoxLink(
		CaQepNodeInformation* pParent, 
		CaQepNodeInformation* pSon, 
		int nSon = 0);
	
	virtual ~CaSqlQueryExecutionPlanBoxLink(){};

	CWnd* GetParentBox(BOOL bPreview = FALSE) {return m_pQepParentNodeInfo? m_pQepParentNodeInfo->GetQepBox(bPreview): NULL;}
	CWnd* GetChildBox (BOOL bPreview = FALSE) {return m_pQepSonNodeInfo? m_pQepSonNodeInfo->GetQepBox(bPreview): NULL;}

public:
	//
	// Data member
	int m_nSon;
	CaQepNodeInformation* m_pQepParentNodeInfo;
	CaQepNodeInformation* m_pQepSonNodeInfo;
};


class CaSqlQueryExecutionPlanBinaryTree: public CObject
{
	DECLARE_SERIAL (CaSqlQueryExecutionPlanBinaryTree)
public:
	BOOL CalcChildrenPercentages(CaSqlQueryExecutionPlanBinaryTree* pRootNode);
	CaSqlQueryExecutionPlanBinaryTree();
	virtual ~CaSqlQueryExecutionPlanBinaryTree();
	virtual void Serialize (CArchive& ar);

	int GetCount(); // Return the number of nodes.


	int m_nID;                           // Unique ID to identify the node.
	int m_nSon;                          // 0: Root, 1: Left, 2: Right
	int m_nParentID;                     // If m_nSon = 0, it is not used.
	CaQepNodeInformation* m_pQepNodeInfo;// More inforation about node.
	                                     // It is a base class. For a star node it should be type-cast to
	                                     // the class CaQepNodeInformationStar before using.
	CaSqlQueryExecutionPlanBinaryTree* m_pQepNodeLeft;
	CaSqlQueryExecutionPlanBinaryTree* m_pQepNodeRight;


};

extern BOOL CalcSTARChildrenPercentages(CaSqlQueryExecutionPlanBinaryTree * pTreeNode,CaSqlQueryExecutionPlanBinaryTree * pRootNode);
extern BOOL UpdateSTARSiteStrings(CaSqlQueryExecutionPlanBinaryTree * pTreeNode,int inodeno, LPTSTR SiteString);

#endif
