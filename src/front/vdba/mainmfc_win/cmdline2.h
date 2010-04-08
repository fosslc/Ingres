/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: cmdline2.h - header file
**  Author: Emmanuel Blattes
**
**  Description:
**  Definition of CuDomObjDesc_xxx.
**
**  History (after 26-Sep-2000)
**
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Handle PROFILE, SYNONYM, RULE, INTEGRITY command line. 
**    (Synonym is avalable only for the sub-branch of database)
** 20-Aug-2008 (whiro01)
**    Remove redundant <afx...> include which is already in "stdafx.h"
*/

#ifndef CMDLINE2_INCLUDED
#define CMDLINE2_INCLUDED

#include "domfast.h"    // CuDomTreeFastItem
//UKS#include "ipmfast.h"    // CuIpmTreeFastItem
#include "ipmaxdoc.h"     // CdIpm

// for LPDOMDATA
extern "C" {
#include "dba.h"
#include "main.h"
#include "dom.h"
}

class CTreeItem;
class CuIpmTreeFastItem: public CObject
{
public:
  CuIpmTreeFastItem(BOOL bStatic, int type, void* pFastStruct = NULL, BOOL bCheckName = FALSE, LPCTSTR lpsCheckName = NULL);
  virtual ~CuIpmTreeFastItem();

protected:
  // CuIpmTreeFastItem();    // serialization
  // DECLARE_SERIAL (CuIpmTreeFastItem)

public:
  BOOL    GetBStatic() { return m_bStatic; }
  int     GetType()    { return m_type; }
  void*   GetPStruct() { return m_pStruct; }
  BOOL    IsCheckNameNeeded()   { return m_bCheckName; }
  LPCTSTR GetCheckName() { return m_csCheckName; }

private:
  BOOL  m_bStatic;
  int   m_type;
  void* m_pStruct;
  BOOL  m_bCheckName;
  CString m_csCheckName;
};

inline CuIpmTreeFastItem::CuIpmTreeFastItem(BOOL bStatic, int type, void* pFastStruct, BOOL bCheckName, LPCTSTR lpsCheckName)
{
  m_bStatic = bStatic;
  m_type    = type;
  m_pStruct = pFastStruct;
  m_bCheckName = bCheckName;
  if (bCheckName) {
    ASSERT (lpsCheckName);
    if (lpsCheckName)
      m_csCheckName = lpsCheckName;
    else
      m_csCheckName = _T("ERRONEOUS");
  }
}

inline CuIpmTreeFastItem::~CuIpmTreeFastItem()
{
}

BOOL ExpandUpToSearchedItem(CWnd* pPropWnd, CTypedPtrList<CObList, CuIpmTreeFastItem*>& rIpmTreeFastItemList, BOOL bAutomated = FALSE);
BOOL IpmCurSelHasStaticChildren(CWnd* pPropWnd);

/////////////////////////////////////////////////////////////////////////////////////////
// CuObjDesc: anchor base class for CuDomObjDesc and CuIpmObjDesc

class CuObjDesc: public CObject
{
  DECLARE_DYNAMIC (CuObjDesc)
public:
  CuObjDesc(LPCTSTR objName, int objType, int level = 0, int parent0type = 0, int parent1type = 0);
  virtual ~CuObjDesc() {}
  int GetLevel(){return m_level;}
  int GetObjType(){return m_objType;}
  CString GetObjectName() {return m_csObjName;}
  CString GetParent0Name(){return m_csParent0;}
  CString GetParent1Name(){return m_csParent1;}
// Attributes
private:
  CString m_csFullObjName;
  BOOL    m_bParsed;

protected:
  int     m_level;
  int     m_objType;               //  OT_xxx
  int     m_parent0type;
  int     m_parent1type;
  CString m_csObjName;
  CString m_csParent0;
  CString m_csParent1;

  // Operations
private:
  BOOL ParseLevel1ObjName(CString csObjId, CString& rcsParent, CString& rcsObj);
  BOOL ParseLevel2ObjName(CString csObjId, CString& rcsParent0, CString& rcsParent1, CString& rcsObj);
  BOOL ParseLevel1ObjName() { return ParseLevel1ObjName(m_csFullObjName, m_csParent0, m_csObjName); }
  BOOL ParseLevel2ObjName() { return ParseLevel2ObjName(m_csFullObjName, m_csParent0, m_csParent1, m_csObjName); }

protected:
  virtual BOOL IsStaticType(int type) { ASSERT (FALSE); return FALSE; }
  BOOL IsParsed() { return m_bParsed; }
  
public:
  BOOL ParseObjectDescriptor();
};

/////////////////////////////////////////////////////////////////////////////////////////
// CuDomObjDesc: derived from CuObjDesc

class CuDomObjDesc: public CuObjDesc
{
  DECLARE_DYNAMIC (CuDomObjDesc)

// Construction
public:
  CuDomObjDesc(LPCTSTR objName, int objType, int level = 0, int parent0type = 0, int parent1type = 0);
  virtual ~CuDomObjDesc() {}

// operations

private:
  BOOL CheckObjectExistence(int hNode, LPCTSTR objName, int objType, int level, LPCTSTR parent0 = NULL, LPCTSTR parent1 = NULL);
  BOOL GetObjectLimitedInfo(int hNode, LPCTSTR objName, int objType, int level, LPCTSTR parent0, LPCTSTR parent1, BOOL bFillBuffers = FALSE, LPUCHAR lpResBufObj = NULL, LPUCHAR lpResBufOwner = NULL, LPUCHAR lpResBufComplim = NULL);
  virtual BOOL NeedsDummySubItem() {return TRUE; }   // virtualize only if necessary
  virtual BOOL IsStaticType(int type);

public:
  BOOL CheckLowLevel(int hNode);
  virtual BOOL CreateRootTreeItem(LPDOMDATA lpDomData); // virtualize only if necessary
  BOOL BuildFastItemList(CTypedPtrList<CObList, CuDomTreeFastItem*>& rDomTreeFastItemList); //never virtualized at this time
};

/////////////////////////////////////////////////////////////////////////////////////////
// Derived classes for dom items - almost empty

class CuDomObjDesc_DATABASE: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_DATABASE(LPCTSTR objName) : CuDomObjDesc(objName, OT_DATABASE, 0) {}
  virtual ~CuDomObjDesc_DATABASE() {}
};

class CuDomObjDesc_TABLE: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_TABLE(LPCTSTR objName) : CuDomObjDesc(objName, OT_TABLE, 1, OT_DATABASE) {}
  virtual ~CuDomObjDesc_TABLE() {}
};

class CuDomObjDesc_VIEW: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_VIEW(LPCTSTR objName) : CuDomObjDesc(objName, OT_VIEW, 1, OT_DATABASE) {}
  virtual ~CuDomObjDesc_VIEW() {}
};

class CuDomObjDesc_PROCEDURE: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_PROCEDURE(LPCTSTR objName) : CuDomObjDesc(objName, OT_PROCEDURE, 1, OT_DATABASE) {}
  virtual ~CuDomObjDesc_PROCEDURE() {}
};

class CuDomObjDesc_DBEVENT: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_DBEVENT(LPCTSTR objName) : CuDomObjDesc(objName, OT_DBEVENT, 1, OT_DATABASE) {}
  virtual ~CuDomObjDesc_DBEVENT() {}
};

class CuDomObjDesc_USER: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_USER(LPCTSTR objName) : CuDomObjDesc(objName, OT_USER, 0) {}
  virtual ~CuDomObjDesc_USER() {}
};

class CuDomObjDesc_GROUP: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_GROUP(LPCTSTR objName) : CuDomObjDesc(objName, OT_GROUP, 0) {}
  virtual ~CuDomObjDesc_GROUP() {}
};

class CuDomObjDesc_ROLE: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_ROLE(LPCTSTR objName) : CuDomObjDesc(objName, OT_ROLE, 0) {}
  virtual ~CuDomObjDesc_ROLE() {}
};

class CuDomObjDesc_PROFILE: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_PROFILE(LPCTSTR objName) : CuDomObjDesc(objName, OT_PROFILE, 0) {}
  virtual ~CuDomObjDesc_PROFILE() {}
};

class CuDomObjDesc_SYNONYM: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_SYNONYM(LPCTSTR objName) : CuDomObjDesc(objName, OT_SYNONYM, 1, OT_DATABASE) {}
 virtual ~CuDomObjDesc_SYNONYM() {}
};

class CuDomObjDesc_LOCATION: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_LOCATION(LPCTSTR objName) : CuDomObjDesc(objName, OT_LOCATION, 0) {}
  virtual ~CuDomObjDesc_LOCATION() {}
};

class CuDomObjDesc_INDEX: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_INDEX(LPCTSTR objName) : CuDomObjDesc(objName, OT_INDEX, 2, OT_DATABASE, OT_TABLE) {}
  virtual ~CuDomObjDesc_INDEX() {}
};

class CuDomObjDesc_RULE: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_RULE(LPCTSTR objName) : CuDomObjDesc(objName, OT_RULE, 2, OT_DATABASE, OT_TABLE) {}
  virtual ~CuDomObjDesc_RULE() {}
};

class CuDomObjDesc_INTEGRITY: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_INTEGRITY(LPCTSTR objName) : CuDomObjDesc(objName, OT_INTEGRITY, 2, OT_DATABASE, OT_TABLE) {}
  virtual ~CuDomObjDesc_INTEGRITY() {}
};


class CuDomObjDesc_STATIC_INSTALL_SECURITY: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_STATIC_INSTALL_SECURITY(LPCTSTR objName) : CuDomObjDesc(_T("Security Auditing"), OT_STATIC_INSTALL_SECURITY, 1, OT_STATIC_INSTALL) {}
  virtual ~CuDomObjDesc_STATIC_INSTALL_SECURITY() {}
  virtual BOOL NeedsDummySubItem() {return FALSE; }
};

class CuDomObjDesc_STATIC_INSTALL_GRANTEES: public CuDomObjDesc
{
// Construction
public:
  CuDomObjDesc_STATIC_INSTALL_GRANTEES(LPCTSTR objName) : CuDomObjDesc(_T("Installation Grantees"), OT_STATIC_INSTALL_GRANTEES, 1, OT_STATIC_INSTALL) {}
  virtual ~CuDomObjDesc_STATIC_INSTALL_GRANTEES() {}
};

/////////////////////////////////////////////////////////////////////////////////////////
// CuIpmObjDesc: derived from CuObjDesc

class CuIpmObjDesc: public CuObjDesc
{
  DECLARE_DYNAMIC (CuIpmObjDesc)

// Construction
public:
  CuIpmObjDesc(LPCTSTR objName, int objType, int level = 0, int parent0type = 0, int parent1type = 0);
  virtual ~CuIpmObjDesc();

private:
  void* m_lpDataObj;      // for fast access
  void* m_lpDataParent;  // for fast access

private:
  virtual BOOL IsStaticType(int type);
  virtual BOOL ObjectNeedsNameCheck() { return FALSE; }   // virtualize only if necessary
  virtual BOOL ObjectHasStaticBranch() { return TRUE; }   // virtualize only if necessary

  virtual BOOL StaticParentNeedsNameCheck() { return FALSE; } // virtualize only if necessary
  virtual CString GetStaticParentNameCheck() { ASSERT (FALSE); return _T(""); } // virtualize only if necessary

protected:
  CString GetDisplayName(void* pstruct);

public:
  //
  // for single item on root branch - make virtual if necessary
  //
  CTreeItem* CreateRootTreeItem(CdIpm* pDoc);
  virtual CTreeItem* CreateRootBranch(CdIpm* pDoc, void* pStructReq) { ASSERT(FALSE); return NULL; }

  //
  // for expansion up to requested item
  //
  BOOL  BuildFastItemList(CTypedPtrList<CObList, CuIpmTreeFastItem*>&  rIpmTreeFastItemList);
  virtual BOOL ObjectNameFits(void* pStructReq) { ASSERT(FALSE); return FALSE; }
  virtual BOOL ParentNameFits(void* pStructReq) { ASSERT(FALSE); return FALSE; }
};

/////////////////////////////////////////////////////////////////////////////////////////
// Derived classes for monitor items

class CuIpmObjDesc_SERVER: public CuIpmObjDesc
{
// Construction
public:
  CuIpmObjDesc_SERVER(LPCTSTR objName) : CuIpmObjDesc(objName, OT_MON_SERVER, 0) {}
  virtual ~CuIpmObjDesc_SERVER() {}
  virtual BOOL ObjectNameFits(void* pStructReq);
  virtual CTreeItem* CreateRootBranch(CdIpm* pDoc, void* pStructReq);
};

class CuIpmObjDesc_SESSION: public CuIpmObjDesc
{
// Construction
public:
  CuIpmObjDesc_SESSION(LPCTSTR objName) : CuIpmObjDesc(objName, OT_MON_SESSION, 1, OT_MON_SERVER) {}
  virtual ~CuIpmObjDesc_SESSION() {}
  virtual BOOL ObjectNameFits(void* pStructReq);
  virtual BOOL ParentNameFits(void* pStructReq);

  virtual CTreeItem* CreateRootBranch(CdIpm* pDoc, void* pStructReq);
};

class CuIpmObjDesc_LOGINFO: public CuIpmObjDesc
{
// Construction
public:
  CuIpmObjDesc_LOGINFO(LPCTSTR objName) : CuIpmObjDesc(objName, OT_MON_LOGINFO) {}
  virtual ~CuIpmObjDesc_LOGINFO() {}
  virtual BOOL ObjectNeedsNameCheck() { return TRUE; }
  virtual BOOL ObjectNameFits(void* pStructReq) { ASSERT (!pStructReq); return TRUE; }
  virtual CTreeItem* CreateRootBranch(CdIpm* pDoc, void* pStructReq);
};

class CuIpmObjDesc_REPLICSERVER: public CuIpmObjDesc
{
// Construction
public:
  CuIpmObjDesc_REPLICSERVER(LPCTSTR objName) : CuIpmObjDesc(objName, OT_MON_REPLIC_SERVER, 1, OT_DATABASE) {}
  virtual ~CuIpmObjDesc_REPLICSERVER() {}
  virtual BOOL ObjectNameFits(void* pStructReq);
  virtual BOOL ParentNameFits(void* pStructReq);
  virtual BOOL ObjectHasStaticBranch() { return FALSE; }
  virtual BOOL StaticParentNeedsNameCheck() { return TRUE; }
  virtual CString GetStaticParentNameCheck() { return _T("Replication"); }

  virtual CTreeItem* CreateRootBranch(CdIpm* pDoc, void* pStructReq);
};

#endif	// CMDLINE2_INCLUDED
