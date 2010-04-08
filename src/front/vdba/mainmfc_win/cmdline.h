/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: cmdline.h - header file
**
**  Description:
**  This is the header file for the CuWindowDesc class.
**
**  History:
**  15-feb-2000 (somsa01)
**      Removed extra commas from CmdObjType, AnalyseStep, and
**      AnalyseToken declarations.
**  26-Sep-2000 (uk$so01)
**     SIR #102711: Callable in context command line improvement (for Manage-IT)
**  31-may-2001 (zhayu01) SIR 104824 for EDBC
**     1. Added m_csTitleName, GetTitleName(), SetTitleName().
**     2. Added a new return parameter for ManageNodeDescToken().
**     3. Declare GetContextDrivenTitleName().
** 20-Aug-2008 (whiro01)
**    Remove redundant <afx...> include which is already in "stdafx.h"
** 10-Mar-2010 (drivi01) SIR 123397
**    Add functionality to bring up "Generate Statistics" dialog 
**    from command line by providing proper vnode/database/table flags
**    to Visual DBA utility.
**    example: vdba.exe /c dom (local) table junk/mytable1 GENSTATANDEXIT
*/

#ifndef CMDLINE_INCLUDED
#define CMDLINE_INCLUDED

#include "cmdline2.h"   // command line objects descriptions

/////////////////////////////////////////////////////////////////////////////
// Automated document management

class CuWindowDesc;
class CuCmdLineParse;

void SetUnicenterDriven(CuCmdLineParse* pGeneralDesc);
BOOL IsUnicenterDriven();
CuCmdLineParse* GetAutomatedGeneralDescriptor();
void ResetUnicenterDriven();

void SetAutomatedWindowDescriptor(CuWindowDesc* pDesc);
CuWindowDesc* GetAutomatedWindowDescriptor();
void ResetAutomatedWindowDescriptor();

BOOL FlagsSayNoCloseWindow();

extern CuWindowDesc* GetWinDescPtr();

#ifdef	EDBC
void GetContextDrivenTitleName(char *buffer);
#endif

/////////////////////////////////////////////////////////////////////////////
// command objects types

typedef enum {
  CMD_ERROR     = 0,

  // DOM object types
  CMD_DATABASE,
  CMD_TABLE,
  CMD_VIEW,
  CMD_PROCEDURE,
  CMD_DBEVENT,
  CMD_USER,
  CMD_GROUP,
  CMD_ROLE,
  CMD_PROFILE,
  CMD_SYNONYM,  // Only for the sub-branch of database
  CMD_LOCATION,
  CMD_INDEX,
  CMD_RULE,
  CMD_INTEGRITY,
  CMD_STATIC_INSTALL_SECURITY,
  CMD_STATIC_INSTALL_GRANTEES,

  // IPM object types
  CMD_MON_SERVER,
  CMD_MON_SESSION,
  CMD_STATIC_MON_LOGINFO,
  CMD_MON_REPLICSERVER

} CmdObjType;


typedef enum 
{
	OBJECT_NONE = 0, 
	OBJECT_ADD_EXIT, 
	OBJECT_ALTER_EXIT, 
	OBJECT_DROP_EXIT,
	OBJECT_OPTIMIZE_EXIT
} AADACTION;

/////////////////////////////////////////////////////////////////////////////
// CuWindowDesc object

class CuWindowDesc : public CObject
{
// Construction
public:

  CuWindowDesc(int type, LPCTSTR NodeName = _T("(local)"), LPCTSTR ServerClassName = _T(""), LPCTSTR UserName = _T(""), BOOL bHasObjDesc = FALSE, CmdObjType objType = CMD_ERROR , LPCTSTR ObjIdentifier = _T("") );
  virtual ~CuWindowDesc();

protected:
  CuWindowDesc() {}   // serialization
  DECLARE_SERIAL (CuWindowDesc)

// Attributes
private:
  int     m_type;               //  TYPEDOC_XXX
  CString m_csNodeName;
  CString m_csServerClassName;
  CString m_csUserName;
#ifdef	EDBC
  CString m_csTitleName;
#endif

  BOOL        m_bHasObjDesc;
  CmdObjType  m_objType;         // CMD_XXX
  CString     m_csObjIdentifier; // identification of object - details in fnn's doc
  CuObjDesc*  m_pObjDesc;        // Object Description

  CString m_csTabCaption;       // for tab caption
  int     m_splitbarPct;        // for splitbar percentage

  BOOL m_bHideDocToolbar;
  BOOL m_bNoMenu;
  BOOL m_bObjectActionsOnlyMenu;
  BOOL m_bSplitbarNotMoveable;
  BOOL m_bSingleTreeItem;
  BOOL m_bSpecifiedTabOnly;

  AADACTION m_aadAction;
  // Operations
public:
  void Serialize (CArchive& ar);

  void SetNodeName(CString csNodeName)                { m_csNodeName = csNodeName; }
  void SetServerClassName(CString csServerClassName)  { m_csServerClassName = csServerClassName; }
  void SetUserName(CString csUserName)                { m_csUserName = csUserName; }
#ifdef	EDBC
  void SetTitleName(CString csTitleName)                { m_csTitleName = csTitleName; }
#endif
  void SetObjType(CmdObjType objType)                 { m_bHasObjDesc = TRUE; m_objType = objType; }
  void SetObjIdentifier(CString csObjIdentifier)      { ASSERT (m_bHasObjDesc); m_csObjIdentifier = csObjIdentifier; }

  void SetTabCaption(CString csTabCaption)            { ASSERT (m_csTabCaption.IsEmpty()); m_csTabCaption = csTabCaption; }
  void SetSplitbarPercentage(int pct)                 { ASSERT (m_splitbarPct == -1); ASSERT (pct>=0 && pct <=100); m_splitbarPct = pct; }

  int     GetType()             { return m_type; }
  CString GetNodeName()         { return m_csNodeName; }
  CString GetServerClassName()  { return m_csServerClassName; }
  CString GetUserName()         { return m_csUserName; }
  CmdObjType GetObjType()		{ ASSERT (m_bHasObjDesc); return m_objType; }
#ifdef	EDBC
  CString GetTitleName()         { return m_csTitleName; }
#endif
  CString GetObjectIdentifier() { ASSERT (m_bHasObjDesc); return m_csObjIdentifier; }

  BOOL    HasObjDesc()              { return m_bHasObjDesc; }
  CuObjDesc* AllocateObjDescriptor(CmdObjType objType, LPCTSTR csObjId);

  void    CreateObjDescriptor();
  BOOL    ParseObjDescriptor();
  CuDomObjDesc* GetDomObjDescriptor()  { ASSERT (m_pObjDesc); ASSERT (m_pObjDesc->IsKindOf(RUNTIME_CLASS(CuDomObjDesc))); return (CuDomObjDesc*)m_pObjDesc; }
  CuIpmObjDesc* GetIpmObjDescriptor()  { ASSERT (m_pObjDesc); ASSERT (m_pObjDesc->IsKindOf(RUNTIME_CLASS(CuIpmObjDesc))); return (CuIpmObjDesc*)m_pObjDesc; }

  BOOL    HasTabCaption()         { return !m_csTabCaption.IsEmpty(); }
  CString GetTabCaption()         { return m_csTabCaption; }
  BOOL    HasSplitbarPercentage() { return (BOOL)(m_splitbarPct != -1); }
  int     GetSplitbarPercentage() { return m_splitbarPct; }

  BOOL DoWeHideDocToolbar()       { return m_bHideDocToolbar; }
  void SetNoDocToolbar()          { m_bHideDocToolbar = TRUE; }

  BOOL HasNoMenu()                  { return m_bNoMenu; }
  void SetNoMenu()                  { m_bNoMenu = TRUE; }
  BOOL HasObjectActionsOnlyMenu()   { return m_bObjectActionsOnlyMenu; }
  void SetObjectActionsOnlyMenu()   { m_bObjectActionsOnlyMenu = TRUE; }

  BOOL IsSplitbarNotMoveable()    { return m_bSplitbarNotMoveable; }
  void SetSplitbarNotMoveable()   { m_bSplitbarNotMoveable = TRUE; }

  BOOL HasSingleTreeItem()        { return m_bSingleTreeItem; }
  void SetSingleTreeItem()        { m_bSingleTreeItem = TRUE; }

  BOOL HasSpecifiedTabOnly()      { return m_bSpecifiedTabOnly; }
  void SetSpecifiedTabOnly()      { m_bSpecifiedTabOnly = TRUE; }

  void SetAAD (AADACTION aad){m_aadAction = aad;}
  AADACTION GetAAD (){return m_aadAction;}

};

/////////////////////////////////////////////////////////////////////////////
// CuCmdLineParse object

typedef enum {
  STEP_BEGIN = 1,
  STEP_TERMINATED,      // special
  STEP_AFTERPARAM,
  STEP_AFTERWINDESC,
  STEP_AFTERSEPARATOR
} AnalyseStep;

typedef enum {
  TOKEN_ERROR = 1,
  TOKEN_NOTHING,
  TOKEN_PARAMETER,
  TOKEN_WINDOWDESC,
  TOKEN_SEPARATOR
} AnalyseToken;

class CuCmdLineParse : public CObject
{
// Construction
public:
  CuCmdLineParse(LPCTSTR cmdLine);
  virtual ~CuCmdLineParse();
  void ProcessCmdLine();
  void SetHideApp(BOOL bSet){m_bHideApp = bSet;}
  BOOL GetHideApp(){return m_bHideApp;}

protected:
  CuCmdLineParse() {}   // serialization
  DECLARE_SERIAL (CuCmdLineParse)

private:
  void ParseCmdLineIntoArgArray();
  void AnalyseArgArray();
  BOOL Analyse();

  BOOL Analyse(int& rCurArgsIndex, AnalyseStep& rStep, AnalyseToken& rToken);
  AnalyseToken AnalyseNextToken(int &rCurArgsIndex);

  BOOL IsSeparatorToken     (int& rCurArgsIndex);
  BOOL ManageSeparatorToken (int& rCurArgsIndex);

  BOOL IsParameterToken     (int& rCurArgsIndex);
  BOOL ManageParameterToken (int& rCurArgsIndex);

  BOOL IsWindowDescToken    (int& rCurArgsIndex);
  BOOL ManageWindowDescToken(int& rCurArgsIndex);

  BOOL ManageWindowOptionToken(int& rCurArgsIndex, CuWindowDesc* pWindowDesc);
  BOOL IsWindowOptionToken(int& rCurArgsIndex);

  BOOL IsNodeDescToken    (int& rCurArgsIndex);
#ifdef	EDBC
  BOOL ManageNodeDescToken(int& rCurArgsIndex, CString& rCsNodeName, CString& rCsServerClassName, CString& rCsUserName, CString& rCsTitleName);
#else
  BOOL ManageNodeDescToken(int& rCurArgsIndex, CString& rCsNodeName, CString& rCsServerClassName, CString& rCsUserName);
#endif
  BOOL IsServerClass      (int& rCurArgsIndex);
  BOOL IsUserName         (int& rCurArgsIndex);

  BOOL IsObjTypeToken    (int& rCurArgsIndex);
  BOOL ManageObjTypeToken(int& rCurArgsIndex, CmdObjType& rObjType, int docType);

  BOOL IsObjIdToken    (int& rCurArgsIndex);
  BOOL ManageObjIdToken(int& rCurArgsIndex, CString& rcsObjId);

  BOOL IsObjOptionToken     (int& rCurArgsIndex);
  BOOL ManageObjOptionToken (int& rCurArgsIndex, CuWindowDesc* pWindowDesc);

// Attributes
private:
  CString m_csCmdLine;
  BOOL    m_bParsed;

  // intermediary
  CStringArray m_argArray;
  int          m_nbArgs;

  // output
  BOOL    m_bUnicentered;   // has '/c' option
  BOOL    m_bSyntaxError;   // if has /c option: syntax error found?
  int     m_indexOfError;   // if error: where is error (---> near ...)

  BOOL    m_bMaxApp;
  BOOL    m_bMaxWin;
  BOOL    m_bHideNodes;
  BOOL    m_bNoSplashScreen;
  BOOL    m_bNoMainToolbar;
  BOOL    m_bNoSaveOnExit;
  BOOL    m_bHideApp;

  CTypedPtrList<CObList, CuWindowDesc*> m_WindowDescList;

// Operations
public:
  void Serialize (CArchive& ar);

  BOOL IsUnicenterDriven() { ASSERT (m_bParsed); return m_bUnicentered; }
  BOOL SyntaxError()       { ASSERT (m_bParsed); return m_bSyntaxError; }
  LPCTSTR GetErrorToken()  { ASSERT (m_bParsed); return m_argArray[m_indexOfError]; }

  BOOL DoWeMaximizeApp()   { ASSERT (m_bParsed); return m_bMaxApp; }
  BOOL DoWeMaximizeWin()   { ASSERT (m_bParsed); return m_bMaxWin; }
  BOOL DoWeHideNodes()     { ASSERT (m_bParsed); return m_bHideNodes; }
  BOOL DoWeHideSplash()    { ASSERT (m_bParsed); return m_bNoSplashScreen; }
  BOOL DoWeHideMainToolbar() { ASSERT (m_bParsed); return m_bNoMainToolbar; }
  BOOL DoWeExitWithoutSave() { ASSERT (m_bParsed); return m_bNoSaveOnExit; }

  CTypedPtrList<CObList, CuWindowDesc*>* GetPWindowDescSerialList() { ASSERT (m_bParsed); return &m_WindowDescList; }
};

#endif // CMDLINE_INCLUDED
