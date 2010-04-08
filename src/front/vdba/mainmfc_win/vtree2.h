/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vtree2.h, Header file 
**    Project  : IngresII / Vdba.
**    Authors  : UK Sotheavut / Emmanuel Blattes 
**    Purpose  : Manipulate the Virtual Node Tree ItemData to respond to the
**               method like: Add, Alter, Drop,
**
** History (after 26-Sep-2000)
** 
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Select the input database if specified.
*/


#ifndef VTREE_NODES_INCLUDED
#define VTREE_NODES_INCLUDED

#include "vtree1.h"    // base class

/////////////////////////////////////////////////////////////////////////////
// CTreeItemNodes object

class CTreeItemNodes : public CTreeItemAAD
{
// Construction
public:
  // Constructor for non-static branches
  CTreeItemNodes(CTreeGlobalData *pTree, int type, SubBK subBranchKind, BOOL bNeedsTypeSolve, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);

  // Constructor for static branches
  CTreeItemNodes(CTreeGlobalData *pTree, int type, UINT nameId, SubBK subBranchKind, UINT imageId, CTreeItemData *pItemData, int childtype=CHILDTYPE_UNDEF, UINT noChildStringId=0);

  virtual ~CTreeItemNodes();

// Construction
protected:
  CTreeItemNodes();   // serialization
  DECLARE_SERIAL (CTreeItemNodes)

// Construction
private:

// Attributes
private:

// Attributes
protected:

// Attributes
public:

// Operations
public:
  void Serialize (CArchive& ar);

  // reminder : UiUpdate must be managed
  // by derivation of virtual BOOL IsEnabled(UINT idMenu)

  // special methods for UiCommand, not virtualized
	BOOL TreeConnect();           // will call virtual Connect()
	BOOL TreeSqlTest(LPCTSTR lpszDatabase=NULL);           // will call virtual SqlTest()
	BOOL TreeMonitor();           // will call virtual Monitor()
	BOOL TreeDbevent(LPCTSTR lpszDatabase=NULL);           // will call virtual Dbevent()
	BOOL TreeDisconnect();        // will call virtual Disconnect()
	BOOL TreeCloseWin();          // will call virtual CloseWin()
	BOOL TreeActivateWin();       // will call virtual ActivateWin()
	BOOL TreeScratchpad();        // will call virtual Scratchpad()

  // special methods for UiCommand, virtualized, must be derived
	virtual BOOL Connect()        { ASSERT (FALSE); return FALSE; }
	virtual BOOL SqlTest(LPCTSTR lpszDatabase = NULL){ ASSERT (FALSE); return FALSE; }
	virtual BOOL Monitor()        { ASSERT (FALSE); return FALSE; }
	virtual BOOL Dbevent(LPCTSTR lpszDatabase = NULL){ ASSERT (FALSE); return FALSE; }
	virtual BOOL Disconnect()     { ASSERT (FALSE); return FALSE; }
	virtual BOOL CloseWin()       { ASSERT (FALSE); return FALSE; }
	virtual BOOL ActivateWin()    { ASSERT (FALSE); return FALSE; }
	virtual BOOL Scratchpad()     { ASSERT (FALSE); return FALSE; }

  // Direct virtual for node test
	virtual BOOL TestNode()       { ASSERT (FALSE); return FALSE; }

  // for install password
	virtual BOOL InstallPassword() { ASSERT (FALSE); return FALSE; }

  // special methods for branch refresh
  BOOL RefreshDisplayedOpenWindowsList();

  // specially redefined methods for UiCommand, not virtualized,
  // To have a custom behaviour for branches refresh
	BOOL TreeAdd();         // will call virtual Add()
	BOOL TreeAlter();       // will call virtual Alter()
	BOOL TreeDrop();        // will call virtual Delete()

// Operations
private:
};

/////////////////////////////////////////////////////////////////////////////

#endif // VTREE_NODES_INCLUDED
