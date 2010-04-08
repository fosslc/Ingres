/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vnitem.h, Header file 
**    Project  : CA-OpenIngres/Monitoring.
**    Authors  : UK Sotheavut / Emmanuel Blattes 
**    Purpose  : Manipulate the Virtual Node Tree ItemData to respond to the
**               method like: Add, Alter, Drop,
**
** History (after 26-Sep-2000)
** 
** 26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**    Select the input database if specified.
** 26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 19-Feb-2002 (uk$so01)
**    SIR #106648, Integrate SQL Test ActiveX Control
*/

#ifndef VNITEM_HEADER
#define VNITEM_HEADER
#include "vtree2.h"
#include "rcdepend.h"

// Overloading magic
extern CString BuildFullNodeName(LPNODESVRCLASSDATAMIN pServerDataMin);
extern CString BuildFullNodeName(LPNODEUSERDATAMIN pUserDataMin);

extern CString GetMonitorTemplateString();
extern CString GetDbEventTemplateString();
extern CString GetSqlActTemplateString();
class CdSqlQuery;
class CdIpm;
CdSqlQuery* CreateSqlTest(CString& strNode, CString& strServer, CString& strUser, CString& strDatabase);
CdIpm* CreateMonitor(CString& strNode, CString& strServer, CString& strUser);

class CuTreeServerStatic: public CTreeItemNodes
{
public:
    CuTreeServerStatic (CTreeGlobalData* pTreeGlobalData);
    virtual ~CuTreeServerStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Add();
    virtual BOOL InstallPassword();

protected:
    CuTreeServerStatic();
    DECLARE_SERIAL (CuTreeServerStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};



class CuTreeServer: public CTreeItemNodes
{
public:
    CuTreeServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeServer(){};

    BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Connect();
	  virtual BOOL Monitor();
    virtual BOOL Dbevent(LPCTSTR lpszDatabase = NULL);
	  virtual BOOL SqlTest(LPCTSTR lpszDatabase = NULL);
    virtual BOOL Drop();
	  virtual BOOL Alter();
	  virtual BOOL Add();
	  virtual BOOL Disconnect();
	  virtual BOOL Scratchpad();

    virtual BOOL TestNode();
    virtual BOOL InstallPassword();

    virtual UINT GetCustomImageId();

    // special methods
    BOOL AtLeastOneWindowOnNode();
    void UpdateOpenedWindowsList();
    BOOL CleanServersAndUsersBranches();

    // custom refresh of related branches
    virtual BOOL RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because);

protected:
    CuTreeServer();
    DECLARE_SERIAL (CuTreeServer)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};



class CuTreeOpenwinStatic: public CTreeItemNodes
{
public:
    CuTreeOpenwinStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeOpenwinStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
protected:
    CuTreeOpenwinStatic();
    DECLARE_SERIAL (CuTreeOpenwinStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


class CuTreeOpenwin: public CTreeItemNodes
{
public:
    CuTreeOpenwin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeOpenwin(){};

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL ActivateWin();
	  virtual BOOL CloseWin();

    // special methods
    void UpdateOpenedWindowsList();

protected:
    CuTreeOpenwin();
    DECLARE_SERIAL (CuTreeOpenwin)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


class CuTreeAdvancedStatic: public CTreeItemNodes
{
public:
    CuTreeAdvancedStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeAdvancedStatic(){};

    BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
protected:
    CuTreeAdvancedStatic();
    DECLARE_SERIAL (CuTreeAdvancedStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


class CuTreeLoginStatic: public CTreeItemNodes
{
public:
    CuTreeLoginStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeLoginStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	  virtual BOOL IsEnabled(UINT idMenu);
  	virtual BOOL Add();

    virtual void MinFillpStructReq(void *pStructReq);
    BOOL AtLeastOneWindowOnNode();

protected:
    CuTreeLoginStatic();
    DECLARE_SERIAL (CuTreeLoginStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};

class CuTreeLogin: public CTreeItemNodes
{
public:
	virtual UINT GetCustomImageId();
    CuTreeLogin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeLogin(){};
  	virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Add();
    virtual BOOL Drop();
    virtual BOOL Alter();
    BOOL AtLeastOneWindowOnNode();

protected:
    CuTreeLogin();
    DECLARE_SERIAL (CuTreeLogin)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


class CuTreeConnectionStatic: public CTreeItemNodes
{
public:
    CuTreeConnectionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeConnectionStatic(){};
  	virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Add();

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual void MinFillpStructReq(void *pStructReq);
    BOOL AtLeastOneWindowOnNode();

protected:
    CuTreeConnectionStatic();
    DECLARE_SERIAL (CuTreeConnectionStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};

class CuTreeConnection: public CTreeItemNodes
{
public:
    CuTreeConnection (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeConnection(){};

    virtual UINT GetCustomImageId();
  	virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Add();
    virtual BOOL Drop();
    virtual BOOL Alter();
    BOOL AtLeastOneWindowOnNode();

protected:
    CuTreeConnection();
    DECLARE_SERIAL (CuTreeConnection)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};

class CuTreeSvrClassStatic: public CTreeItemNodes
{
public:
    CuTreeSvrClassStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeSvrClassStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

	  virtual BOOL IsEnabled(UINT idMenu);

    virtual void UpdateParentBranchesAfterExpand(HTREEITEM hItem);

protected:
    CuTreeSvrClassStatic();
    DECLARE_SERIAL (CuTreeSvrClassStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};



class CuTreeSvrClass: public CTreeItemNodes
{
public:
    CuTreeSvrClass (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeSvrClass(){};

    BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Connect();
	  virtual BOOL Monitor();
    virtual BOOL Dbevent(LPCTSTR lpszDatabase = NULL);
	  virtual BOOL SqlTest(LPCTSTR lpszDatabase = NULL);
	  virtual BOOL Disconnect();
	  virtual BOOL Scratchpad();

    virtual UINT GetCustomImageId();

    // special methods
    BOOL AtLeastOneWindowOnNode();
    void UpdateOpenedWindowsList();
    BOOL CleanUsersBranch();

    // custom refresh of related branches
    virtual BOOL RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because);

protected:
    CuTreeSvrClass();
    DECLARE_SERIAL (CuTreeSvrClass)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};



class CuTreeSvrOpenwinStatic: public CTreeItemNodes
{
public:
    CuTreeSvrOpenwinStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeSvrOpenwinStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
protected:
    CuTreeSvrOpenwinStatic();
    DECLARE_SERIAL (CuTreeSvrOpenwinStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


class CuTreeSvrOpenwin: public CTreeItemNodes
{
public:
    CuTreeSvrOpenwin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeSvrOpenwin(){};

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL ActivateWin();
	  virtual BOOL CloseWin();

    // special methods
    void UpdateOpenedWindowsList();

protected:
    CuTreeSvrOpenwin();
    DECLARE_SERIAL (CuTreeSvrOpenwin)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};

//
/* New as of May 4, 98: for "users" connection aliasing : new set of branches */
//

class CuTreeUserStatic: public CTreeItemNodes
{
public:
    CuTreeUserStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeUserStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

	  virtual BOOL IsEnabled(UINT idMenu);

    // special methods since need to frame GetFirst/NextMonInfo(OT_USER) with Open/CloseNodeStruct
    // Take advantage of already designed StarOpen/CloseNodeStruct methods
    virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
    virtual void StarCloseNodeStruct(int &hNode);

    // Special method for RES_NOT_OI
    virtual UINT GetStringId_ResNotOi() { return IDS_USER_NOTOI; }

    virtual void UpdateParentBranchesAfterExpand(HTREEITEM hItem);

protected:
    CuTreeUserStatic();
    DECLARE_SERIAL (CuTreeUserStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};



class CuTreeUser: public CTreeItemNodes
{
public:
    CuTreeUser (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeUser(){};

    BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Connect();
	  virtual BOOL Monitor();
    virtual BOOL Dbevent(LPCTSTR lpszDatabase = NULL);
	  virtual BOOL SqlTest(LPCTSTR lpszDatabase = NULL);
	  virtual BOOL Disconnect();
	  virtual BOOL Scratchpad();

    virtual UINT GetCustomImageId();

    // special methods
    BOOL AtLeastOneWindowOnNode();
    void UpdateOpenedWindowsList();

    // custom refresh of related branches
    virtual BOOL RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because);

protected:
    CuTreeUser();
    DECLARE_SERIAL (CuTreeUser)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};

class CuTreeUsrOpenwinStatic: public CTreeItemNodes
{
public:
    CuTreeUsrOpenwinStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeUsrOpenwinStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
protected:
    CuTreeUsrOpenwinStatic();
    DECLARE_SERIAL (CuTreeUsrOpenwinStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


class CuTreeUsrOpenwin: public CTreeItemNodes
{
public:
    CuTreeUsrOpenwin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeUsrOpenwin(){};

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL ActivateWin();
	  virtual BOOL CloseWin();

    // special methods
    void UpdateOpenedWindowsList();

protected:
    CuTreeUsrOpenwin();
    DECLARE_SERIAL (CuTreeUsrOpenwin)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};

//
/* New as of May 6, 98: for "users on serverclass" connection aliasing : new set of branches */
//

class CuTreeSvrUserStatic: public CTreeItemNodes
{
public:
    CuTreeSvrUserStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeSvrUserStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

	  virtual BOOL IsEnabled(UINT idMenu);

    // special methods since need to frame GetFirst/NextMonInfo(OT_USER) with Open/CloseNodeStruct
    // Take advantage of already designed StarOpen/CloseNodeStruct methods
    virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
    virtual void StarCloseNodeStruct(int &hNode);

    // Special method for RES_NOT_OI
    virtual UINT GetStringId_ResNotOi() { return IDS_USER_NOTOI; }

    virtual void UpdateParentBranchesAfterExpand(HTREEITEM hItem);

protected:
    CuTreeSvrUserStatic();
    DECLARE_SERIAL (CuTreeSvrUserStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};



class CuTreeSvrUser: public CTreeItemNodes
{
public:
    CuTreeSvrUser (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeSvrUser(){};

    BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Connect();
	  virtual BOOL Monitor();
    virtual BOOL Dbevent(LPCTSTR lpszDatabase = NULL);
	  virtual BOOL SqlTest(LPCTSTR lpszDatabase = NULL);
	  virtual BOOL Disconnect();
	  virtual BOOL Scratchpad();

    virtual UINT GetCustomImageId();

    // special methods
    BOOL AtLeastOneWindowOnNode();
    void UpdateOpenedWindowsList();

    // custom refresh of related branches
    virtual BOOL RefreshRelatedBranches(HTREEITEM hItem, RefreshCause because);

protected:
    CuTreeSvrUser();
    DECLARE_SERIAL (CuTreeSvrUser)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};

class CuTreeSvrUsrOpenwinStatic: public CTreeItemNodes
{
public:
    CuTreeSvrUsrOpenwinStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeSvrUsrOpenwinStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
protected:
    CuTreeSvrUsrOpenwinStatic();
    DECLARE_SERIAL (CuTreeSvrUsrOpenwinStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


class CuTreeSvrUsrOpenwin: public CTreeItemNodes
{
public:
    CuTreeSvrUsrOpenwin (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeSvrUsrOpenwin(){};

	  virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL ActivateWin();
	  virtual BOOL CloseWin();

    // special methods
    void UpdateOpenedWindowsList();

protected:
    CuTreeSvrUsrOpenwin();
    DECLARE_SERIAL (CuTreeSvrUsrOpenwin)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


class CuTreeOtherAttrStatic: public CTreeItemNodes
{
public:
    CuTreeOtherAttrStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeOtherAttrStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	  virtual BOOL IsEnabled(UINT idMenu);
  	virtual BOOL Add();

    virtual void MinFillpStructReq(void *pStructReq);
    BOOL AtLeastOneWindowOnNode();

protected:
    CuTreeOtherAttrStatic();
    DECLARE_SERIAL (CuTreeOtherAttrStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};

class CuTreeOtherAttr: public CTreeItemNodes
{
public:
	virtual UINT GetCustomImageId();
    CuTreeOtherAttr (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeOtherAttr(){};
  	virtual BOOL IsEnabled(UINT idMenu);
	  virtual BOOL Add();
    virtual BOOL Drop();
    virtual BOOL Alter();
    BOOL AtLeastOneWindowOnNode();

protected:
    CuTreeOtherAttr();
    DECLARE_SERIAL (CuTreeOtherAttr)
public:
  void Serialize (CArchive& ar) { CTreeItemNodes::Serialize(ar); }
};


#endif
