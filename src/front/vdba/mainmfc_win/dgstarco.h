/****************************************************************************************
//                                                                                     //
// Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : dgstarco.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II/Vdba.                                                       //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Star Database (Component Pane)                                //
****************************************************************************************/
#if !defined(AFX_DGSTARCO_H__09BFF514_EB0A_11D1_A27C_00609725DDAF__INCLUDED_)
#define AFX_DGSTARCO_H__09BFF514_EB0A_11D1_A27C_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "domseri.h"

class CaStarDbComponent: public CObject
{
	DECLARE_SERIAL (CaStarDbComponent)
public:
	CaStarDbComponent()
		:m_strVNode(_T("")), m_strDatabase(_T("")), m_strServer(_T("")), 
		 m_strObject(_T("")), m_strObjectOwner(_T(""))
	{
		m_nType         = OT_UNKNOWN;
	};
	CaStarDbComponent(const CaStarDbComponent& c);
	CaStarDbComponent operator = (const CaStarDbComponent& c);
	CaStarDbComponent(STARDBCOMPONENTPARAMS* pParams);

	void Copy (const CaStarDbComponent& c);
	virtual ~CaStarDbComponent(){}
	virtual void Serialize (CArchive& ar);

	//
	// Data members
	int     m_nType;          // Object Type (OT_TABLE, OT_VIEW, OT_PROCEDURE, OT_INDEX)
	CString m_strVNode;       // VNode Name
	CString m_strDatabase;    // Database Name
	CString m_strServer;      // Server Class
	CString m_strObject;      // Database Object
	CString m_strObjectOwner; // Owner of  Database Object
};

class CaDomPropDataStarDb: public CuIpmPropertyData
{
	DECLARE_SERIAL (CaDomPropDataStarDb)
public:

	CaDomPropDataStarDb(): CuIpmPropertyData(_T("CaDomPropDataStarDb")){};
	CaDomPropDataStarDb(const CaDomPropDataStarDb& c);
	CaDomPropDataStarDb operator = (const CaDomPropDataStarDb& c);
	void Copy (const CaDomPropDataStarDb& c);

	virtual ~CaDomPropDataStarDb();
	virtual void Serialize (CArchive& ar);
	void Cleanup();
	virtual void NotifyLoad (CWnd* pDlg);  

	//
	// Data members

	CTypedPtrList<CObList, CaStarDbComponent*> m_listComponent;
	CTypedPtrList<CObList, CStringList*> m_listExpandedNode;

};

class CuDlgDomPropStarDbComponent : public CDialog
{
public:
	CuDlgDomPropStarDbComponent(CWnd* pParent = NULL);
	void OnCancel() {return;}
	void OnOK()     {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDomPropStarDbComponent)
	enum { IDD = IDD_DOMPROP_STARDB_COMPONENT };
	CTreeCtrl	m_cTree1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropStarDbComponent)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	void UpdateTree();
	void AddItem (CaStarDbComponent* pC);
	void StoreExpandedState();
	void RestoreExpandedState();
	HTREEITEM FindItem (CStringList* pStrList);


	HTREEITEM FindNode    (LPCTSTR lpszNode,     HTREEITEM hRoot);
	HTREEITEM FindObject  (LPCTSTR lpszObject,   HTREEITEM hRootDatabase);
	void ResetDisplay();

	int m_nHNode;
	CImageList m_ImageList;
	CaDomPropDataStarDb* m_pData;
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropStarDbComponent)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnItemexpandedTree1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOutofmemoryTree1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad          (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnChangeFont    (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGSTARCO_H__09BFF514_EB0A_11D1_A27C_00609725DDAF__INCLUDED_)
