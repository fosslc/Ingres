/*
** Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Source : cmdline.cpp
**  Defines the class behaviors for the application
**  Project : Visual DBA
**  Author :  Emmanuel Blattes
**
**  History after 25-May-2000
**
**  26-May-2000 (noifr01)
**     (bug 99242) checked DBCS compliance : should be OK since the misc
**     characters handling that could seem not compliant, are only associated
**     to known single byte characters. No change done.
**  26-Sep-2000 (uk$so01)
**    SIR #102711: Callable in context command line improvement (for Manage-IT)
**  31-may-2001 (zhayu01) SIR 104824 for EDBC
**	  1. Handle the title name.
**	  2. Add a new function GetContextDrivenTitleName().
**  07-Dec-2001 (loera01)
**        Made temporary char string const to compile on Solaris.
**  22-may-2002 (somsa01)
**     Corrected cross-integration.
**  10-Mar-2010 (drivi01) SIR 123397
**     Add functionality to bring up "Generate Statistics" dialog 
**     from command line by providing proper vnode/database/table flags
**     to Visual DBA utility.
**     example: vdba.exe /c dom (local) table junk/mytable1 GENSTATANDEXIT
*/

#include "stdafx.h"
#include "mainmfc.h"

#include "cmdline.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" {
#include "dba.h"
#include "domdata.h"
#include "main.h"
#include "dbaginfo.h"
};

/////////////////////////////////////////////////////////////////////////////
// Automated document management

static BOOL bUnicenterDriven = FALSE;
static CuCmdLineParse* pUnicenterGeneralDesc = NULL;
static CuWindowDesc* pUnicenterWindowDesc = NULL;

void SetUnicenterDriven(CuCmdLineParse* pGeneralDesc) { ASSERT (!bUnicenterDriven); bUnicenterDriven = TRUE; ASSERT (pGeneralDesc); pUnicenterGeneralDesc = pGeneralDesc; }
void ResetUnicenterDriven() { bUnicenterDriven = FALSE; pUnicenterGeneralDesc = NULL; pUnicenterWindowDesc = NULL; }
BOOL IsUnicenterDriven() { return bUnicenterDriven; }
CuCmdLineParse* GetAutomatedGeneralDescriptor() { return pUnicenterGeneralDesc; }

void SetAutomatedWindowDescriptor(CuWindowDesc* pDesc) { ASSERT (pDesc); pUnicenterWindowDesc = pDesc; }
CuWindowDesc* GetAutomatedWindowDescriptor() { return pUnicenterWindowDesc; }
void ResetAutomatedWindowDescriptor() { pUnicenterWindowDesc = NULL; }


/////////////////////////////////////////////////////////////////////////////////////////
// Extern "C" interface

CuWindowDesc* GetWinDescPtr()
{
  if (!IsUnicenterDriven())
    return NULL;
  CuCmdLineParse* pCmdLineParse = GetAutomatedGeneralDescriptor();
  ASSERT (pCmdLineParse);
  if (!pCmdLineParse)
    return NULL;
  BOOL bHideNodes = pCmdLineParse->DoWeHideNodes();
  if (!bHideNodes)
    return NULL;
  CTypedPtrList<CObList, CuWindowDesc*>* pWinDescList = pCmdLineParse->GetPWindowDescSerialList();
  ASSERT (pWinDescList);
  if (!pWinDescList)
    return NULL;
  if (pWinDescList->IsEmpty())
    return NULL;
  POSITION pos = pWinDescList->GetHeadPosition();
  CuWindowDesc* pDesc = pWinDescList->GetNext(pos);
  if (pos)
    return NULL;     // more than one doc
  if (!pDesc->HasNoMenu() && !pDesc->HasObjectActionsOnlyMenu())
    return NULL;

  return pDesc;
}

BOOL GetContextDrivenNodeName(char* buffer)
{
  ASSERT (buffer);
  buffer[0] = '\0';

  CuWindowDesc* pDesc = GetWinDescPtr();
  if (!pDesc)
    return FALSE;
  CString csName = pDesc->GetNodeName();
  x_strcpy(buffer, (LPCSTR)(LPCTSTR)csName);
  return TRUE;
}

#ifdef	EDBC
void GetContextDrivenTitleName(char* buffer)
{
  ASSERT (buffer);
  buffer[0] = '\0';

  CuWindowDesc* pDesc = GetWinDescPtr();
  if (!pDesc)
    return;
  CString csName = pDesc->GetTitleName();
  x_strcpy(buffer, (LPCSTR)(LPCTSTR)csName);
  return;
}
#endif

BOOL GetContextDrivenServerClassName(char* buffer)
{
  ASSERT (buffer);
  buffer[0] = '\0';

  CuWindowDesc* pDesc = GetWinDescPtr();
  if (!pDesc)
    return FALSE;
  CString csName = pDesc->GetServerClassName();
  if (csName.IsEmpty()) {
//    ASSERT (FALSE);         // Function expected not to be called if no server class was specified!
    return FALSE;
  }
  x_strcpy(buffer, (LPCSTR)(LPCTSTR)csName);
  return TRUE;
}

BOOL IsInvokedInContextWithOneWinExactly()
{
  CuWindowDesc* pDesc = GetWinDescPtr();
  if (!pDesc)
    return FALSE;
  return TRUE;
}



/////////////////////////////////////////////////////////////////////////////////////////
// cpp interface

BOOL FlagsSayNoCloseWindow()
{
  if (GetWinDescPtr())
	  return TRUE;
  else
	  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuWindowDesc Class

// declaration for serialization
IMPLEMENT_SERIAL ( CuWindowDesc, CObject, 1)

CuWindowDesc::CuWindowDesc(int type, LPCTSTR NodeName, LPCTSTR ServerClassName, LPCTSTR UserName, BOOL bHasObjDesc, CmdObjType objType, LPCTSTR ObjIdentifier)
{
  m_type              = type            ;
  m_csNodeName        = NodeName        ;
  m_csServerClassName = ServerClassName ;
  m_csUserName        = UserName        ;

  m_bHasObjDesc       = bHasObjDesc     ;
  m_objType           = objType         ;
  m_csObjIdentifier   = ObjIdentifier   ;
  m_pObjDesc          = NULL;

  m_csTabCaption      = _T("");
  m_splitbarPct       = -1;

  m_bHideDocToolbar         = FALSE;
  m_bNoMenu                 = FALSE;
  m_bObjectActionsOnlyMenu  = FALSE;
  m_bSplitbarNotMoveable    = FALSE;
  m_bSingleTreeItem         = FALSE;
  m_bSpecifiedTabOnly       = FALSE;
  m_aadAction = OBJECT_NONE;
}

CuWindowDesc::~CuWindowDesc()
{
  if (m_pObjDesc)
    delete m_pObjDesc;
}


void CuWindowDesc::Serialize (CArchive& ar)
{
  // Should not be used!
  ASSERT (FALSE);

  if (ar.IsStoring()) {
  }
  else {
  }
}

void CuWindowDesc::CreateObjDescriptor()
{
  ASSERT (!m_pObjDesc);
  ASSERT (HasObjDesc());

  m_pObjDesc = AllocateObjDescriptor(m_objType, m_csObjIdentifier);
}

// NOTE : ObjTypeFromKeyword() TO BE UPDATED FOR EVERY NEW TYPE ADDED HERE
CuObjDesc* CuWindowDesc::AllocateObjDescriptor(CmdObjType objType, LPCTSTR csObjId)
{
  switch (objType) {
    // DOM object types
    case CMD_DATABASE:
      return new CuDomObjDesc_DATABASE(csObjId);
    case CMD_TABLE:
      return new CuDomObjDesc_TABLE(csObjId);
    case CMD_VIEW:
      return new CuDomObjDesc_VIEW(csObjId);
    case CMD_PROCEDURE:
      return new CuDomObjDesc_PROCEDURE(csObjId);
    case CMD_DBEVENT:
      return new CuDomObjDesc_DBEVENT(csObjId);
    case CMD_USER:
      return new CuDomObjDesc_USER(csObjId);
    case CMD_GROUP:
      return new CuDomObjDesc_GROUP(csObjId);
    case CMD_ROLE:
      return new CuDomObjDesc_ROLE(csObjId);
    case CMD_PROFILE:
      return new CuDomObjDesc_PROFILE(csObjId);
    case CMD_SYNONYM:
      return new CuDomObjDesc_SYNONYM(csObjId);
    case CMD_LOCATION:
      return new CuDomObjDesc_LOCATION(csObjId);
    case CMD_INDEX:
      return new CuDomObjDesc_INDEX(csObjId);
    case CMD_RULE:
      return new CuDomObjDesc_RULE(csObjId);
    case CMD_INTEGRITY:
      return new CuDomObjDesc_INTEGRITY(csObjId);
    case CMD_STATIC_INSTALL_SECURITY:
      return new CuDomObjDesc_STATIC_INSTALL_SECURITY(csObjId);
    case CMD_STATIC_INSTALL_GRANTEES:
      return new CuDomObjDesc_STATIC_INSTALL_GRANTEES(csObjId);

    // IPM object types
    case CMD_MON_SERVER:
      return new CuIpmObjDesc_SERVER(csObjId);
    case CMD_MON_SESSION:
      return new CuIpmObjDesc_SESSION(csObjId);
    case CMD_STATIC_MON_LOGINFO:
      return new CuIpmObjDesc_LOGINFO(csObjId);
    case CMD_MON_REPLICSERVER:
      return new CuIpmObjDesc_REPLICSERVER(csObjId);

    default:
      ASSERT (FALSE);
      return NULL;
  }
}

BOOL CuWindowDesc::ParseObjDescriptor()
{
  ASSERT (HasObjDesc());
  ASSERT (m_pObjDesc);
  return m_pObjDesc->ParseObjectDescriptor();
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuCmdLineParse Class

// forward definitions:
static BOOL AdjustStep(AnalyseStep& rStep, AnalyseToken token);
static int  DocTypeFromKeyword(CString csToken);
static CmdObjType ObjTypeFromKeyword(CString csToken);
static CmdObjType ObjTypeFromKeyword(CString csToken, BOOL& rBDom);

// declaration for serialization
IMPLEMENT_SERIAL ( CuCmdLineParse, CObject, 1)

CuCmdLineParse::CuCmdLineParse(LPCTSTR cmdLine)
{
  m_csCmdLine    = cmdLine;
  m_csCmdLine.TrimLeft();
  m_csCmdLine.TrimRight();

  m_bParsed      = FALSE;
  m_nbArgs       = 0;

  m_bUnicentered = FALSE;
  m_bSyntaxError = TRUE;
  m_indexOfError = -1;

  // Default values
  m_bMaxApp     = FALSE;
  m_bMaxWin     = FALSE;
  m_bHideNodes  = FALSE;
  m_bNoSplashScreen = FALSE;
  m_bNoMainToolbar = FALSE;
  m_bNoSaveOnExit = FALSE;
  m_bHideApp = FALSE;
}

CuCmdLineParse::~CuCmdLineParse()
{
  while (!m_WindowDescList.IsEmpty())
    delete m_WindowDescList.RemoveHead();
}


void CuCmdLineParse::Serialize (CArchive& ar)
{
  // Should not be used!
  ASSERT (FALSE);

  if (ar.IsStoring()) {
  }
  else {
  }
}

void CuCmdLineParse::ProcessCmdLine()
{
  if (m_csCmdLine.IsEmpty()) {
    m_bParsed   = TRUE;
    return;
  }

  // Parse cmdline into array of arguments
  ParseCmdLineIntoArgArray();

  // analyse parsed array
  AnalyseArgArray();
}

void CuCmdLineParse::ParseCmdLineIntoArgArray()
{
  CString csLine = m_csCmdLine;
  int pos = 0;
  CString csArg;

  do {
	  if (csLine[0]=='\"') { // manage quoted strings as single parameter
		pos = csLine.Mid(1).Find('\"');
		csLine+=_T(" ");
		if (pos!=-1) {
			TCHAR tch = csLine[1+pos+1];
			if (tch==' ' || tch==',') {
				CString cs1= csLine.Mid(1,pos);
				m_argArray.Add(cs1);
				pos = 1+pos+1;
				if (tch==',') {
					CString csComma = _T(",");
					m_argArray.Add(csComma);
					pos++;
				}
				goto cont;
			}
		}
	}
    pos = csLine.FindOneOf(_T(" ,"));   // Search the first, either space and comma
    if (pos != -1) {
      // store left string
      csArg = csLine.Left(pos);
      if (!csArg.IsEmpty())
        m_argArray.Add(csArg);    // can be empty if comma found at first position!

      // if comma, store it as a keyword
      if (csLine[pos] == _T(',')) {
        CString csComma = _T(",");
        m_argArray.Add(csComma);
        pos++;
      }

cont:
      // stop here if at the end of the string
      if (pos >= csLine.GetLength())
        break;

      // Keep remainder
      csLine = csLine.Mid(pos);
      csLine.TrimLeft();

      // Special if remaider was only made of spaces
      if (csLine.IsEmpty())
        break;
    }
    else {
      // Last one
      m_argArray.Add(csLine);
    }
  } while (pos != -1);


  m_nbArgs = m_argArray.GetUpperBound() + 1;
  m_bParsed   = TRUE;
}

void CuCmdLineParse::AnalyseArgArray()
{
  ASSERT (m_bParsed);

  // No arguments : fine, not unicentered
  if (m_nbArgs == 0)
    return;

  // First argument not '/c' : fine, not unicentered
  if (m_argArray[0].CompareNoCase(_T("/c")) != 0)
    return;

  // Unicentered!
  m_bUnicentered = TRUE;

  // Analyse 
  BOOL bSuccess = Analyse();
  if (bSuccess) {
    // All is well : reset syntax error
    m_bSyntaxError = FALSE;
  }
  else {
    ASSERT (m_indexOfError < m_nbArgs);
    if (m_indexOfError >= m_nbArgs)
      m_indexOfError = m_nbArgs;
  }
}

BOOL CuCmdLineParse::Analyse()
{
  // Loop on arguments
  AnalyseStep step = STEP_BEGIN;
  AnalyseToken token = TOKEN_ERROR;
  int curArgsIndex = 1;

  while (1) {
    AnalyseToken token = AnalyseNextToken(curArgsIndex);
    if (token == TOKEN_ERROR) {
      m_indexOfError = curArgsIndex;
      return FALSE;
    }
    if (token == TOKEN_NOTHING)
      return TRUE;
    if (!AdjustStep(step, token)) {
      if (curArgsIndex >= m_nbArgs)
        curArgsIndex--;
      m_indexOfError = curArgsIndex;
      return FALSE;
    }
  }
}

// 1) Says whether the analysed token was acceptable according to step we were in
// 2) updates the step
static BOOL AdjustStep(AnalyseStep& rStep, AnalyseToken token)
{
  ASSERT (rStep != STEP_TERMINATED);
  switch (rStep) {
    case STEP_BEGIN:
    case STEP_AFTERPARAM:
      // Accept nothing, param, or windowdesc
      if (token == TOKEN_NOTHING) {
        rStep = STEP_TERMINATED;
        return TRUE;
      }
      else if (token == TOKEN_PARAMETER) {
        rStep = STEP_AFTERPARAM;
        return TRUE;
      }
      else if (token == TOKEN_WINDOWDESC) {
        rStep = STEP_AFTERWINDESC;
        return TRUE;
      }
      else
        return FALSE;
      break;
    case STEP_AFTERWINDESC:
      // Accept nothing or separator
      if (token == TOKEN_NOTHING) {
        rStep = STEP_TERMINATED;
        return TRUE;
      }
      else if (token == TOKEN_SEPARATOR) {
        rStep = STEP_AFTERSEPARATOR;
        return TRUE;
      }
      else
        return FALSE;
      break;
    case STEP_AFTERSEPARATOR:
      // Accept windowdesc
      if (token == TOKEN_WINDOWDESC) {
        rStep = STEP_AFTERWINDESC;
        return TRUE;
      }
      else
        return FALSE;
      break;
  }

  // Manage forgotten case
  ASSERT (FALSE);
  return FALSE;
}

AnalyseToken CuCmdLineParse::AnalyseNextToken(int &rCurArgsIndex)
{
  ASSERT (rCurArgsIndex <= m_nbArgs);

  // In search of termination
  if (rCurArgsIndex >= m_nbArgs)
    return TOKEN_NOTHING;

  // in search of separator
  if (IsSeparatorToken(rCurArgsIndex))
    if (ManageSeparatorToken(rCurArgsIndex))
      return TOKEN_SEPARATOR;
    else
      return TOKEN_ERROR;

  // in search of parameter
  if (IsParameterToken(rCurArgsIndex))
    if (ManageParameterToken(rCurArgsIndex))
      return TOKEN_PARAMETER;
    else
      return TOKEN_ERROR;

  // in search of window descriptor
  if (IsWindowDescToken(rCurArgsIndex))
    if (ManageWindowDescToken(rCurArgsIndex))
      return TOKEN_WINDOWDESC;
    else
      return TOKEN_ERROR;

  // Unknown
  return TOKEN_ERROR;
}

BOOL CuCmdLineParse::IsSeparatorToken(int& rCurArgsIndex)
{
  CString csToken = m_argArray[rCurArgsIndex];
  if (csToken.CompareNoCase(_T(",")) == 0)
    return TRUE;
  return FALSE;
}

BOOL CuCmdLineParse::ManageSeparatorToken(int& rCurArgsIndex)
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  CString csToken = m_argArray[rCurArgsIndex];
  if (csToken.CompareNoCase(_T(",")) == 0) {
    rCurArgsIndex++;
    return TRUE;
  }
  return FALSE;
}

BOOL CuCmdLineParse::IsParameterToken(int& rCurArgsIndex)
{
  CString csToken = m_argArray[rCurArgsIndex];
  if (csToken.CompareNoCase(_T("MAXAPP")) == 0
      || csToken.CompareNoCase(_T("MAXWIN")) == 0
      || csToken.CompareNoCase(_T("NONODESWINDOW")) == 0
      || csToken.CompareNoCase(_T("NOSPLASHSCREEN")) == 0
      || csToken.CompareNoCase(_T("NOMAINTOOLBAR")) == 0
      || csToken.CompareNoCase(_T("NOSAVEONEXIT")) == 0
    )
    return TRUE;
  return FALSE;
}

BOOL CuCmdLineParse::ManageParameterToken(int& rCurArgsIndex)
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  CString csToken = m_argArray[rCurArgsIndex];

  // Maxapp
  if (csToken.CompareNoCase(_T("MAXAPP")) == 0) {
    if (m_bMaxApp)
      return FALSE;     // already set: unacceptable
    m_bMaxApp = TRUE;
    rCurArgsIndex++;
    return TRUE;
  }

  // Maxwin
  if (csToken.CompareNoCase(_T("MAXWIN")) == 0) {
    if (m_bMaxWin)
      return FALSE;     // already set: unacceptable
    m_bMaxWin = TRUE;
    rCurArgsIndex++;
    return TRUE;
  }

  // NoNodesWindow
  if (csToken.CompareNoCase(_T("NONODESWINDOW")) == 0) {
    if (m_bHideNodes)
      return FALSE;     // already set: unacceptable
    m_bHideNodes = TRUE;
    rCurArgsIndex++;
    return TRUE;
  }

  // NoSplashScreen
  if (csToken.CompareNoCase(_T("NOSPLASHSCREEN")) == 0) {
    if (m_bNoSplashScreen)
      return FALSE;     // already set: unacceptable
    m_bNoSplashScreen = TRUE;
    rCurArgsIndex++;
    return TRUE;
  }

  // No main toolbar
  if (csToken.CompareNoCase(_T("NOMAINTOOLBAR")) == 0) {
    if (m_bNoMainToolbar)
      return FALSE;     // already set: unacceptable
    m_bNoMainToolbar = TRUE;
    rCurArgsIndex++;
    return TRUE;
  }

  // No save on exit
  if (csToken.CompareNoCase(_T("NOSAVEONEXIT")) == 0) {
    if (m_bNoSaveOnExit)
      return FALSE;     // already set: unacceptable
    m_bNoSaveOnExit = TRUE;
    rCurArgsIndex++;
    return TRUE;
  }

  // not a keyword
  return FALSE;
}

static int DocTypeFromKeyword(CString csToken)
{
  if (csToken.CompareNoCase(_T("DOM")) == 0 )
    return TYPEDOC_DOM;
  if ( csToken.CompareNoCase(_T("SQL")) == 0 )
    return TYPEDOC_SQLACT;
  if ( csToken.CompareNoCase(_T("MONITOR")) == 0 )
    return TYPEDOC_MONITOR;
  if ( csToken.CompareNoCase(_T("DBEVENT")) == 0 )
    return TYPEDOC_DBEVENT;
  return TYPEDOC_UNKNOWN;
}

static CmdObjType ObjTypeFromKeyword(CString csToken, BOOL& rBDom)
{
  // DOM object types
  rBDom = TRUE;
  if (csToken.CompareNoCase(_T("DATABASE")) == 0 )
    return CMD_DATABASE;
  if (csToken.CompareNoCase(_T("TABLE")) == 0 )
    return CMD_TABLE;
  if (csToken.CompareNoCase(_T("VIEW")) == 0 )
    return CMD_VIEW;
  if (csToken.CompareNoCase(_T("PROCEDURE")) == 0 )
    return CMD_PROCEDURE;
  if (csToken.CompareNoCase(_T("EVENT")) == 0 )
    return CMD_DBEVENT;
  if (csToken.CompareNoCase(_T("USER")) == 0 )
    return CMD_USER;
  if (csToken.CompareNoCase(_T("GROUP")) == 0 )
    return CMD_GROUP;
  if (csToken.CompareNoCase(_T("ROLE")) == 0 )
    return CMD_ROLE;
  if (csToken.CompareNoCase(_T("PROFILE")) == 0 )
    return CMD_PROFILE;
  if (csToken.CompareNoCase(_T("SYNONYM")) == 0 )
    return CMD_SYNONYM;
  if (csToken.CompareNoCase(_T("LOCATION")) == 0 )
    return CMD_LOCATION;
  if (csToken.CompareNoCase(_T("INDEX")) == 0 )
    return CMD_INDEX;
  if (csToken.CompareNoCase(_T("RULE")) == 0 )
    return CMD_RULE;
  if (csToken.CompareNoCase(_T("INTEGRITY")) == 0 )
    return CMD_INTEGRITY;
  if (csToken.CompareNoCase(_T("INSTALL_SECURITY_AUDITING")) == 0 )
    return CMD_STATIC_INSTALL_SECURITY;
  if (csToken.CompareNoCase(_T("INSTALL_GRANTEES")) == 0 )
    return CMD_STATIC_INSTALL_GRANTEES;

  // IPM object types
  rBDom = FALSE;
  if (csToken.CompareNoCase(_T("SERVER")) == 0 )
    return CMD_MON_SERVER;
  if (csToken.CompareNoCase(_T("SESSION")) == 0 )
    return CMD_MON_SESSION;
  if (csToken.CompareNoCase(_T("LOGINFO")) == 0 )
    return CMD_STATIC_MON_LOGINFO;
  if (csToken.CompareNoCase(_T("REPLICSERVER")) == 0 )
    return CMD_MON_REPLICSERVER;

  return CMD_ERROR;
}

static CmdObjType ObjTypeFromKeyword(CString csToken)
{
  BOOL bDummy;
  return ObjTypeFromKeyword(csToken, bDummy);
}

BOOL CuCmdLineParse::IsWindowDescToken(int& rCurArgsIndex)
{
  CString csToken = m_argArray[rCurArgsIndex];
  int docType = DocTypeFromKeyword(csToken);
  if (docType == TYPEDOC_UNKNOWN)
    return FALSE;
  else
    return TRUE;
}

BOOL CuCmdLineParse::IsWindowOptionToken(int& rCurArgsIndex)
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  CString csToken = m_argArray[rCurArgsIndex];
  if (csToken.CompareNoCase(_T("NOMENU")) == 0)
    return TRUE;
  else
    return FALSE;
}

BOOL CuCmdLineParse::ManageWindowOptionToken(int& rCurArgsIndex, CuWindowDesc* pWindowDesc)
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  CString csToken = m_argArray[rCurArgsIndex];

  // No menu
  if (csToken.CompareNoCase(_T("NOMENU")) == 0) {
    if (pWindowDesc->HasNoMenu() || pWindowDesc->HasObjectActionsOnlyMenu() )
      return FALSE;     // already set: unacceptable
    pWindowDesc->SetNoMenu();
    rCurArgsIndex++;
    return TRUE;
  }

  // undetected window option token
  ASSERT (FALSE);
  return FALSE;
}

BOOL CuCmdLineParse::ManageWindowDescToken(int& rCurArgsIndex)
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  CString csToken;

  // First string must be window type
  csToken = m_argArray[rCurArgsIndex];
  int docType = DocTypeFromKeyword(csToken);
  if (docType == TYPEDOC_UNKNOWN)
    return FALSE;
  theApp.SetOneWindowType(docType);
  CuWindowDesc* pWindowDesc = new CuWindowDesc(docType);
  m_WindowDescList.AddTail(pWindowDesc);

  rCurArgsIndex++;
  if (rCurArgsIndex >= m_nbArgs)
    return TRUE;  // this is the end
  if (IsSeparatorToken(rCurArgsIndex))
    return TRUE;
  if (IsWindowDescToken(rCurArgsIndex))
    return FALSE;
  if (IsWindowOptionToken(rCurArgsIndex)) {
    if (!ManageWindowOptionToken(rCurArgsIndex, pWindowDesc))
      return FALSE;
  }
  // test whether the end has been reached
  if (rCurArgsIndex >= m_nbArgs)
    return TRUE;
  if (IsSeparatorToken(rCurArgsIndex))
    return TRUE;
  if (IsWindowDescToken(rCurArgsIndex))
    return FALSE;
  if (!IsObjTypeToken(rCurArgsIndex)) {
    if (IsNodeDescToken(rCurArgsIndex)) {
      CString csNodeName, csServerClassName, csUserName;
#ifdef	EDBC
      CString csTitleName;
#endif
#ifdef	EDBC
      if (!ManageNodeDescToken(rCurArgsIndex, csNodeName, csServerClassName, csUserName, csTitleName))
#else
      if (!ManageNodeDescToken(rCurArgsIndex, csNodeName, csServerClassName, csUserName))
#endif
        return FALSE;
#ifdef	EDBC
      pWindowDesc->SetTitleName(csTitleName);
#endif
      pWindowDesc->SetNodeName(csNodeName);
      pWindowDesc->SetServerClassName(csServerClassName);
      pWindowDesc->SetUserName(csUserName);
    }
    else
      return FALSE;
  }

  if (!IsObjTypeToken(rCurArgsIndex))
    return TRUE;
  CmdObjType objType = CMD_ERROR;
  if (ManageObjTypeToken(rCurArgsIndex, objType, docType))
    pWindowDesc->SetObjType(objType);
  else
    return FALSE;
  // Must be followed by obj id
  if (!IsObjIdToken(rCurArgsIndex))
    return FALSE;
  CString csObjId;
  if (ManageObjIdToken(rCurArgsIndex, csObjId)) {
    pWindowDesc->SetObjIdentifier(csObjId);
    pWindowDesc->CreateObjDescriptor();
    if (!pWindowDesc->ParseObjDescriptor())
      return FALSE;
  }
  else
    return FALSE;

  // Can be followed by one or more object options
  while (1) {
    // test whether the end has been reached
    if (rCurArgsIndex >= m_nbArgs)
      return TRUE;
    if (IsObjOptionToken(rCurArgsIndex)) {
      if (!ManageObjOptionToken(rCurArgsIndex, pWindowDesc))
        return FALSE;
    }
    else
      break;
  } // end of while(1)

  // All is well!
  return TRUE;

}

BOOL CuCmdLineParse::IsNodeDescToken    (int& rCurArgsIndex)
{
  // In search of termination
  if (rCurArgsIndex >= m_nbArgs)
    return FALSE;

  // Have to return true since can be nothing - Syntax is questionable
  return TRUE;
}

BOOL CuCmdLineParse::IsServerClass(int& rCurArgsIndex)
{
  // In search of termination
  if (rCurArgsIndex >= m_nbArgs)
    return FALSE;

  CString csToken = m_argArray[rCurArgsIndex];
  if (csToken[0] == _T('/'))
    if (csToken.GetLength() > 1)
      return TRUE;
    else
      return FALSE;
  else
    return FALSE;
}

BOOL CuCmdLineParse::IsUserName(int& rCurArgsIndex)
{
  // In search of termination
  if (rCurArgsIndex >= m_nbArgs)
    return FALSE;

  // check against -u
  CString csToken = m_argArray[rCurArgsIndex];
  if (csToken[0] == _T('-') && csToken[1] == _T('u') )
    if (csToken.GetLength() > 2)
      return TRUE;
    else
      return FALSE;
  else
    return FALSE;
}

#ifdef	EDBC
BOOL CuCmdLineParse::ManageNodeDescToken(int& rCurArgsIndex, CString& rCsNodeName, CString& rCsServerClassName, CString& rCsUserName, CString& rCsTitleName)
#else
BOOL CuCmdLineParse::ManageNodeDescToken(int& rCurArgsIndex, CString& rCsNodeName, CString& rCsServerClassName, CString& rCsUserName)
#endif
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  BOOL bServerClass, bUserName;
  bServerClass = IsServerClass(rCurArgsIndex);
  bUserName    = IsUserName(rCurArgsIndex);
  if (!bServerClass && !bUserName) {
    // assumed to be node name
    CString csToken = m_argArray[rCurArgsIndex];
    rCurArgsIndex++;
    rCsNodeName = csToken;
#ifdef	EDBC
	/*
	** If the rCsNodeName is a connection string, the next token must be
	** the server name and should be saved as the title name.
	*/
	if (!strncmp((LPCTSTR) rCsNodeName, "@",1) &&
			strstr((LPCTSTR) rCsNodeName, ",") && 
				rCurArgsIndex < m_nbArgs)
	{
		rCsTitleName = m_argArray[rCurArgsIndex];
		rCurArgsIndex++;
		/*
		** Set the server class name if there is one.
		*/
		const char	*p;
		if ((p = strstr((LPCTSTR) rCsTitleName, " /")))
		{
			p += 2;
			rCsServerClassName = _T(p);
		}
	}
	else
	{
		rCsTitleName = _T("");
	}
#endif
  }
  else
    rCsNodeName = _T("(local)");

  // after potential node name
  bServerClass = IsServerClass(rCurArgsIndex);
  bUserName    = IsUserName(rCurArgsIndex);
  if (bServerClass || bUserName) {
    CString csToken = m_argArray[rCurArgsIndex];
    rCurArgsIndex++;
    if (bServerClass)
      rCsServerClassName = csToken.Mid(1);
    else
      rCsUserName = csToken.Mid(2);
  }
  else
    return TRUE;    // finished

  // after first parameter
  bServerClass = IsServerClass(rCurArgsIndex);
  bUserName    = IsUserName(rCurArgsIndex);
  if (bServerClass || bUserName) {
    CString csToken = m_argArray[rCurArgsIndex];
    rCurArgsIndex++;
    if (bServerClass)
      rCsServerClassName = csToken.Mid(1);
    else
      rCsUserName = csToken.Mid(2);
  }
  // in any case, we are finished
  return TRUE;    // finished
}

BOOL CuCmdLineParse::IsObjTypeToken    (int& rCurArgsIndex)
{
  // In search of termination
  if (rCurArgsIndex >= m_nbArgs)
    return FALSE;

  CString csToken = m_argArray[rCurArgsIndex];
  CmdObjType objType = ObjTypeFromKeyword(csToken);
  if (objType == CMD_ERROR)
    return FALSE;
  return TRUE;
}

BOOL CuCmdLineParse::ManageObjTypeToken(int& rCurArgsIndex, CmdObjType& rObjType, int docType)
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  CString csToken = m_argArray[rCurArgsIndex];
  BOOL bDomType;
  CmdObjType objType = ObjTypeFromKeyword(csToken, bDomType);
  ASSERT (objType != CMD_ERROR);
  if (objType == CMD_ERROR)
    return FALSE;
  rObjType = objType;
  rCurArgsIndex++;

  // check object type against doc type
  if (docType == TYPEDOC_DOM && bDomType)
    return TRUE;
  if (docType == TYPEDOC_MONITOR && !bDomType)
    return TRUE;
	if (docType == TYPEDOC_SQLACT || docType == TYPEDOC_DBEVENT)
	{
		if (bDomType &&  objType == CMD_DATABASE)
			return TRUE;
	}

  return FALSE;
}

BOOL CuCmdLineParse::IsObjIdToken    (int& rCurArgsIndex)
{
  // In search of termination
  if (rCurArgsIndex >= m_nbArgs)
    return FALSE;

  // Do not accept separator
  if (IsSeparatorToken(rCurArgsIndex))
    return FALSE;

  // Do not accept obj options
  if (IsObjOptionToken(rCurArgsIndex))
    return FALSE;

  // accept any other string
  return TRUE;
}

BOOL CuCmdLineParse::ManageObjIdToken(int& rCurArgsIndex, CString& rcsObjId)
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  CString csToken = m_argArray[rCurArgsIndex];
  rcsObjId = csToken;
  rCurArgsIndex++;
  return TRUE;
}


BOOL CuCmdLineParse::IsObjOptionToken(int& rCurArgsIndex)
{
  CString csToken = m_argArray[rCurArgsIndex];
  if (csToken.CompareNoCase(_T("NODOCTOOLBAR")) == 0
      || csToken.CompareNoCase(_T("NOMENU")) == 0
      || csToken.CompareNoCase(_T("OBJECTACTIONSONLYMENU")) == 0
      || csToken.CompareNoCase(_T("SPLITBARNOTMOVEABLE")) == 0
      || csToken.CompareNoCase(_T("SINGLETREEITEM")) == 0
      || csToken.CompareNoCase(_T("SPECIFIEDTABONLY")) == 0
      || csToken.CompareNoCase(_T("CREATEANDEXIT")) == 0
      || csToken.CompareNoCase(_T("ALTERANDEXIT")) == 0
      || csToken.CompareNoCase(_T("DROPANDEXIT")) == 0
	  || csToken.CompareNoCase(_T("GENSTATANDEXIT")) == 0
    )
    return TRUE;
  if (csToken[0] == _T('-') && csToken[1] == _T('t') && csToken.GetLength() > 2)
      return TRUE;
  if (csToken[0] == _T('-') && csToken[1] == _T('p') && csToken.GetLength() > 2)
      return TRUE;
  return FALSE;
}

BOOL CuCmdLineParse::ManageObjOptionToken(int& rCurArgsIndex, CuWindowDesc* pWindowDesc)
{
  // Check not too far
  ASSERT (rCurArgsIndex < m_nbArgs);

  CString csToken = m_argArray[rCurArgsIndex];

  // No doc toolbar
  if (csToken.CompareNoCase(_T("NODOCTOOLBAR")) == 0) {
    if (pWindowDesc->DoWeHideDocToolbar())
      return FALSE;     // already set: unacceptable
    pWindowDesc->SetNoDocToolbar();
    rCurArgsIndex++;
    return TRUE;
  }

  // No menu
  if (csToken.CompareNoCase(_T("NOMENU")) == 0) {
    if (pWindowDesc->HasNoMenu() || pWindowDesc->HasObjectActionsOnlyMenu() )
      return FALSE;     // already set: unacceptable
    pWindowDesc->SetNoMenu();
    rCurArgsIndex++;
    return TRUE;
  }

  // Object actions only menu
  if (csToken.CompareNoCase(_T("OBJECTACTIONSONLYMENU")) == 0) {
    if (pWindowDesc->HasNoMenu() || pWindowDesc->HasObjectActionsOnlyMenu() )
      return FALSE;     // already set: unacceptable
    pWindowDesc->SetObjectActionsOnlyMenu();
    rCurArgsIndex++;
    return TRUE;
  }

  // Splitbar not moveable
  if (csToken.CompareNoCase(_T("SPLITBARNOTMOVEABLE")) == 0) {
    if (pWindowDesc->IsSplitbarNotMoveable())
      return FALSE;     // already set: unacceptable
    pWindowDesc->SetSplitbarNotMoveable();
    rCurArgsIndex++;
    return TRUE;
  }

  // Single tree item
  if (csToken.CompareNoCase(_T("SINGLETREEITEM")) == 0) {
    if (pWindowDesc->HasSingleTreeItem())
      return FALSE;     // already set: unacceptable
    pWindowDesc->SetSingleTreeItem();
    rCurArgsIndex++;
    return TRUE;
  }

  // Specified tab only
  if (csToken.CompareNoCase(_T("SPECIFIEDTABONLY")) == 0) {
    if (pWindowDesc->HasSpecifiedTabOnly())
      return FALSE;     // already set: unacceptable
    pWindowDesc->SetSpecifiedTabOnly();
    rCurArgsIndex++;
    return TRUE;
  }

	if (csToken.CompareNoCase(_T("CREATEANDEXIT")) == 0) 
	{
		SetHideApp(TRUE);
		pWindowDesc->SetAAD (OBJECT_ADD_EXIT);
		rCurArgsIndex++;
		return TRUE;
	}

	if (csToken.CompareNoCase(_T("ALTERANDEXIT")) == 0) 
	{
		SetHideApp(TRUE);
		pWindowDesc->SetAAD (OBJECT_ALTER_EXIT);
		rCurArgsIndex++;
		return TRUE;
	}

	if (csToken.CompareNoCase(_T("DROPANDEXIT")) == 0) 
	{
		SetHideApp(TRUE);
		pWindowDesc->SetAAD (OBJECT_DROP_EXIT);
		rCurArgsIndex++;
		return TRUE;
	}
	if (csToken.CompareNoCase(_T("GENSTATANDEXIT")) == 0)
	{
		SetHideApp(TRUE);
		pWindowDesc->SetAAD (OBJECT_OPTIMIZE_EXIT);
		rCurArgsIndex++;
		return TRUE;
	}

  // tab caption
  if (csToken[0] == _T('-') && csToken[1] == _T('t') && csToken.GetLength() > 2) {
    if (pWindowDesc->HasTabCaption())
      return FALSE;     // already set: unacceptable
    pWindowDesc->SetTabCaption(csToken.Mid(2));
    rCurArgsIndex++;
    return TRUE;
  }

  // Percentage
  if (csToken[0] == _T('-') && csToken[1] == _T('p') && csToken.GetLength() > 2) {
    if (pWindowDesc->HasSplitbarPercentage())
      return FALSE;     // already set: unacceptable
    CString csPct = csToken.Mid(2);
    int pct = atoi((LPCTSTR)csPct);     // cast in provision for unix port
    if (pct < 0 || pct > 100)
      return FALSE;   // unacceptable value
    pWindowDesc->SetSplitbarPercentage(pct);
    rCurArgsIndex++;
    return TRUE;
  }

  // not a keyword
  return FALSE;
}

