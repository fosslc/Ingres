/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : diskinfo.h, header file 
**    Project  : OpenIngres Visual DBA
**    Author   : Emmanuel Blattes
**    Purpose  : The data type of Statistic bar. (Disk statistic of occupation)
**
** History:
**
** 15-May-1998 (Emmanuel Blattes)
**    Created
** 04-Jul-2002 (uk$so01)
**    Fix BUG #108183, Graphic in Pages Tab of DOM/Table keeps flickering.
*/

#ifndef STATIC_CELL_ITEM_HEADER
#define STATIC_CELL_ITEM_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StCellIt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuStaticCellItem window

class CuStaticCellItem : public CStatic
{
// Construction
public:
	CuStaticCellItem();

// Attributes
public:
  void SetItemType(BYTE itemType) { m_ItemType = itemType; InvalidateRect(NULL); } // PAGE_HEADER, PAGE_MAP, etc.

private:
  BYTE m_ItemType;    // PAGE_HEADER, PAGE_MAP, etc.

// Operations
public:
	void SetPalette(CPalette* p) {m_pPalette = p;}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuStaticCellItem)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CuStaticCellItem();

	// Generated message map functions
protected:
	CPalette* m_pPalette;
	//{{AFX_MSG(CuStaticCellItem)
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // STATIC_CELL_ITEM_HEADER
