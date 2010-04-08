/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : DomSeri.cpp, implementation File 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : Emmanuel Blattes 
**
**    Purpose  : Serialization of DOM right pane property pages (modeless dialog)
**
** History:
**
** xx-Dec-1997 (Emmanuel Blattes)
**    Created.
** 24-Oct-2000 (schph01)
**    Sir 102821 comment on table and columns
** 09-May-2001 (schph01)
**    SIR 102509 serialize new parameter in CuDomPropDataLocation class to manage the
**    raw data location.
** 15-Jun-2001(schph01)
**    SIR 104991 serialize new parameters (m_bGrantUsr4Rmcmd ,m_bPartialGrantDefined )
**    in CuDomPropDataUser class.
** 21-Jun-2001 (schph01)
**    (SIR 103694) STEP 2 support of UNICODE datatypes. Serialize new parameters
**    m_bUnicodeDB in CuDomPropDataDb Class
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate (ipm and sqlquery).ocx.
** 10-Oct-2002 (noifr01)
**    SIR #106648, removed double end of comment, could generate build
**    problems on certain platforms
** 02-Apr-2003 (schph01)
**    SIR #107523, Add class to serialize the Sequences.
** 10-Jun-2004 (schph01)
**    SIR #112460, Add "Catalogs Page Size" information in Database
**    detail right pane.
** 20-Sep-2004 (schph01)
**    Bug 113084 Change the initialization of m_csFeCatalogs in
**    CuDomPropDataDb class, to avoid system message when the DBMS connection
**    failed
**  06-Sep-2005) (drivi01)
**    Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**    two available unicode normalization options, added group box with
**    two checkboxes corresponding to each normalization, including all
**    expected functionlity.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "domseri.h"
#include "a2stream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define IO_BUFFERSIZE  1024


//
// LOCATION
//

IMPLEMENT_SERIAL (CuDomPropDataLocation, CuIpmPropertyData, 4)

CuDomPropDataLocation::CuDomPropDataLocation()
    :CuIpmPropertyData("CuDomPropDataLocation")
{
  m_Area        = "";
  m_bDatabase   = FALSE;
  m_bWork       = FALSE;
  m_bJournal    = FALSE;
  m_bCheckPoint = FALSE;
  m_bDump       = FALSE;
}

void CuDomPropDataLocation::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataLocation::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_Area;
    ar << m_bDatabase;
    ar << m_bWork;
    ar << m_bJournal;
    ar << m_bCheckPoint;
    ar << m_bDump;
    ar << m_iRawPct;
  }
  else {
    ar >> m_Area;
    ar >> m_bDatabase;
    ar >> m_bWork;
    ar >> m_bJournal;
    ar >> m_bCheckPoint;
    ar >> m_bDump;
    ar >> m_iRawPct;
  }
  m_refreshParams.Serialize(ar);
}

//
// USER
//

IMPLEMENT_SERIAL (CuDomPropDataUser, CuIpmPropertyData, 3)

CuDomPropDataUser::CuDomPropDataUser()
    :CuIpmPropertyData("CuDomPropDataUser")
{
  m_csDefGroup       = "";
  m_csDefProfile     = "";
  m_csExpireDate     = "";
  m_csLimSecureLabel = "";

  memset (m_aPrivilege,        FALSE, sizeof(m_aPrivilege));
  memset (m_aDefaultPrivilege, FALSE, sizeof(m_aDefaultPrivilege));
  memset (m_aSecAudit,         FALSE, sizeof(m_aSecAudit));

  m_bGrantUsr4Rmcmd      = FALSE;
  m_bPartialGrantDefined = FALSE;
    // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataUser::CuDomPropDataUser(const CuDomPropDataUser & refPropDataUser)
    :CuIpmPropertyData("CuDomPropDataUser")
{
  m_csDefGroup       = refPropDataUser.m_csDefGroup      ;
  m_csDefProfile     = refPropDataUser.m_csDefProfile    ;
  m_csExpireDate     = refPropDataUser.m_csExpireDate    ;
  m_csLimSecureLabel = refPropDataUser.m_csLimSecureLabel;

  memcpy (m_aPrivilege,        refPropDataUser.m_aPrivilege,        sizeof(m_aPrivilege));
  memcpy (m_aDefaultPrivilege, refPropDataUser.m_aDefaultPrivilege, sizeof(m_aDefaultPrivilege));
  memcpy (m_aSecAudit,         refPropDataUser.m_aSecAudit,         sizeof(m_aSecAudit));

  m_acsGroupsContainingUser.Copy(refPropDataUser.m_acsGroupsContainingUser);
  m_acsSchemaDatabases.Copy(refPropDataUser.m_acsSchemaDatabases);

  m_refreshParams = refPropDataUser.m_refreshParams;

  m_bGrantUsr4Rmcmd      = refPropDataUser.m_bGrantUsr4Rmcmd;
  m_bPartialGrantDefined = refPropDataUser.m_bPartialGrantDefined;
}

// '=' operator overloading
CuDomPropDataUser CuDomPropDataUser::operator = (const CuDomPropDataUser & refPropDataUser)
{
  if (this == &refPropDataUser)
    ASSERT (FALSE);

  m_csDefGroup       = refPropDataUser.m_csDefGroup      ;
  m_csDefProfile     = refPropDataUser.m_csDefProfile    ;
  m_csExpireDate     = refPropDataUser.m_csExpireDate    ;
  m_csLimSecureLabel = refPropDataUser.m_csLimSecureLabel;

  memcpy (m_aPrivilege,        refPropDataUser.m_aPrivilege,        sizeof(m_aPrivilege));
  memcpy (m_aDefaultPrivilege, refPropDataUser.m_aDefaultPrivilege, sizeof(m_aDefaultPrivilege));
  memcpy (m_aSecAudit,         refPropDataUser.m_aSecAudit,         sizeof(m_aSecAudit));

  m_acsGroupsContainingUser.Copy(refPropDataUser.m_acsGroupsContainingUser);
  m_acsSchemaDatabases.Copy(refPropDataUser.m_acsSchemaDatabases);

  m_refreshParams = refPropDataUser.m_refreshParams;

  m_bGrantUsr4Rmcmd      = refPropDataUser.m_bGrantUsr4Rmcmd;
  m_bPartialGrantDefined = refPropDataUser.m_bPartialGrantDefined;

  return *this;
}

void CuDomPropDataUser::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataUser::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csDefGroup      ;
    ar << m_csDefProfile    ;
    ar << m_csExpireDate    ;
    ar << m_csLimSecureLabel;
    ar << m_bGrantUsr4Rmcmd;
    ar << m_bPartialGrantDefined;

    ar.Write (&m_aPrivilege,        sizeof(m_aPrivilege));
    ar.Write (&m_aDefaultPrivilege, sizeof(m_aDefaultPrivilege));
    ar.Write (&m_aSecAudit,         sizeof(m_aSecAudit));
  }
  else {
    ar >> m_csDefGroup      ;
    ar >> m_csDefProfile    ;
    ar >> m_csExpireDate    ;
    ar >> m_csLimSecureLabel;
    ar >> m_bGrantUsr4Rmcmd;
    ar >> m_bPartialGrantDefined;

    ar.Read (&m_aPrivilege,        sizeof(m_aPrivilege));
    ar.Read (&m_aDefaultPrivilege, sizeof(m_aDefaultPrivilege));
    ar.Read (&m_aSecAudit,         sizeof(m_aSecAudit));
  }

  m_acsGroupsContainingUser.Serialize(ar);
  m_acsSchemaDatabases.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// ROLE
//

IMPLEMENT_SERIAL (CuDomPropDataRole, CuIpmPropertyData, 2)

CuDomPropDataRole::CuDomPropDataRole()
    :CuIpmPropertyData("CuDomPropDataRole")
{
  m_csLimSecureLabel = "";

  memset (m_aPrivilege,        FALSE, sizeof(m_aPrivilege));
  memset (m_aSecAudit,         FALSE, sizeof(m_aSecAudit));

  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataRole::CuDomPropDataRole(const CuDomPropDataRole & refPropDataRole)
    :CuIpmPropertyData("CuDomPropDataRole")
{
  m_csLimSecureLabel = refPropDataRole.m_csLimSecureLabel;

  memcpy (m_aPrivilege,        refPropDataRole.m_aPrivilege,        sizeof(m_aPrivilege));
  memcpy (m_aSecAudit,         refPropDataRole.m_aSecAudit,         sizeof(m_aSecAudit));

  m_acsGranteesOnRole.Copy(refPropDataRole.m_acsGranteesOnRole);

  m_refreshParams = refPropDataRole.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataRole CuDomPropDataRole::operator = (const CuDomPropDataRole & refPropDataRole)
{
  if (this == &refPropDataRole)
    ASSERT (FALSE);

  m_csLimSecureLabel = refPropDataRole.m_csLimSecureLabel;

  memcpy (m_aPrivilege,        refPropDataRole.m_aPrivilege,        sizeof(m_aPrivilege));
  memcpy (m_aSecAudit,         refPropDataRole.m_aSecAudit,         sizeof(m_aSecAudit));

  m_acsGranteesOnRole.Copy(refPropDataRole.m_acsGranteesOnRole);

  m_refreshParams = refPropDataRole.m_refreshParams;

  return *this;
}

void CuDomPropDataRole::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataRole::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csLimSecureLabel;

    ar.Write (&m_aPrivilege,        sizeof(m_aPrivilege));
    ar.Write (&m_aSecAudit,         sizeof(m_aSecAudit));
  }
  else {
    ar >> m_csLimSecureLabel;

    ar.Read (&m_aPrivilege,        sizeof(m_aPrivilege));
    ar.Read (&m_aSecAudit,         sizeof(m_aSecAudit));
  }

  m_acsGranteesOnRole.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// PROFILE
//

IMPLEMENT_SERIAL (CuDomPropDataProfile, CuIpmPropertyData, 2)

CuDomPropDataProfile::CuDomPropDataProfile()
    :CuIpmPropertyData("CuDomPropDataProfile")
{
  m_csDefGroup       = "";
  m_csExpireDate     = "";
  m_csLimSecureLabel = "";

  memset (m_aPrivilege,        FALSE, sizeof(m_aPrivilege));
  memset (m_aDefaultPrivilege, FALSE, sizeof(m_aDefaultPrivilege));
  memset (m_aSecAudit,         FALSE, sizeof(m_aSecAudit));

  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataProfile::CuDomPropDataProfile(const CuDomPropDataProfile & refPropDataProfile)
    :CuIpmPropertyData("CuDomPropDataProfile")
{
  m_csDefGroup       = refPropDataProfile.m_csDefGroup      ;
  m_csExpireDate     = refPropDataProfile.m_csExpireDate    ;
  m_csLimSecureLabel = refPropDataProfile.m_csLimSecureLabel;

  memcpy (m_aPrivilege,        refPropDataProfile.m_aPrivilege,        sizeof(m_aPrivilege));
  memcpy (m_aDefaultPrivilege, refPropDataProfile.m_aDefaultPrivilege, sizeof(m_aDefaultPrivilege));
  memcpy (m_aSecAudit,         refPropDataProfile.m_aSecAudit,         sizeof(m_aSecAudit));

  m_refreshParams = refPropDataProfile.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataProfile CuDomPropDataProfile::operator = (const CuDomPropDataProfile & refPropDataProfile)
{
  if (this == &refPropDataProfile)
    ASSERT (FALSE);

  m_csDefGroup       = refPropDataProfile.m_csDefGroup      ;
  m_csExpireDate     = refPropDataProfile.m_csExpireDate    ;
  m_csLimSecureLabel = refPropDataProfile.m_csLimSecureLabel;

  memcpy (m_aPrivilege,        refPropDataProfile.m_aPrivilege,        sizeof(m_aPrivilege));
  memcpy (m_aDefaultPrivilege, refPropDataProfile.m_aDefaultPrivilege, sizeof(m_aDefaultPrivilege));
  memcpy (m_aSecAudit,         refPropDataProfile.m_aSecAudit,         sizeof(m_aSecAudit));

  m_refreshParams = refPropDataProfile.m_refreshParams;

  return *this;
}

void CuDomPropDataProfile::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataProfile::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csDefGroup      ;
    ar << m_csExpireDate    ;
    ar << m_csLimSecureLabel;

    ar.Write (&m_aPrivilege,        sizeof(m_aPrivilege));
    ar.Write (&m_aDefaultPrivilege, sizeof(m_aDefaultPrivilege));
    ar.Write (&m_aSecAudit,         sizeof(m_aSecAudit));
  }
  else {
    ar >> m_csDefGroup      ;
    ar >> m_csExpireDate    ;
    ar >> m_csLimSecureLabel;

    ar.Read (&m_aPrivilege,        sizeof(m_aPrivilege));
    ar.Read (&m_aDefaultPrivilege, sizeof(m_aDefaultPrivilege));
    ar.Read (&m_aSecAudit,         sizeof(m_aSecAudit));
  }
  m_refreshParams.Serialize(ar);
}

//
// INDEX
//

IMPLEMENT_SERIAL (CuDomPropDataIndex, CuIpmPropertyData, 4)

CuDomPropDataIndex::CuDomPropDataIndex()
    :CuIpmPropertyData("CuDomPropDataIndex")
{
  // nothing to do for Arrays
  m_csSchema = "";
  m_csParent = "";

  m_csStorage = "";
  m_bStruct1  = FALSE;
  m_csCaptStruct1 = "";
  m_csStruct1 = "";
  m_bStruct2  = FALSE;
  m_csCaptStruct2 = "";
  m_csStruct2 = "";
  m_bStruct3  = FALSE;
  m_csCaptStruct3 = "";
  m_csStruct3 = "";
  m_bStruct4  = FALSE;
  m_csCaptStruct4 = "";
  m_csStruct4 = "";

  m_csUnique = "";
  m_csPgSize = "";
  m_csAlloc = "";
  m_csExtend = "";
  m_csAllocatedPages = "";
  m_csFillFact = "";
  m_bPersists = FALSE;
  m_bCompression = FALSE;
  m_bCompress = FALSE;
  m_csCompress = "";

  m_bEnterprise = FALSE;
}

// copy constructor
CuDomPropDataIndex::CuDomPropDataIndex(const CuDomPropDataIndex & refPropDataIndex)
    :CuIpmPropertyData("CuDomPropDataIndex")
{
  m_acsIdxColumns.Copy(refPropDataIndex.m_acsIdxColumns);
  m_awIdxColumns.Copy(refPropDataIndex.m_awIdxColumns);
  m_acsLocations.Copy(refPropDataIndex.m_acsLocations);

  m_csSchema      = refPropDataIndex.m_csSchema;
  m_csParent      = refPropDataIndex.m_csParent;

  m_csStorage     = refPropDataIndex.m_csStorage;
  m_bStruct1      = refPropDataIndex.m_bStruct1;
  m_csCaptStruct1 = refPropDataIndex.m_csCaptStruct1;
  m_csStruct1     = refPropDataIndex.m_csStruct1;
  m_bStruct2      = refPropDataIndex.m_bStruct2;
  m_csCaptStruct2 = refPropDataIndex.m_csCaptStruct2;
  m_csStruct2     = refPropDataIndex.m_csStruct2;
  m_bStruct3      = refPropDataIndex.m_bStruct3;
  m_csCaptStruct3 = refPropDataIndex.m_csCaptStruct3;
  m_csStruct3     = refPropDataIndex.m_csStruct3;
  m_bStruct4      = refPropDataIndex.m_bStruct4;
  m_csCaptStruct4 = refPropDataIndex.m_csCaptStruct4;
  m_csStruct4     = refPropDataIndex.m_csStruct4;

  m_csUnique      = refPropDataIndex.m_csUnique;
  m_csPgSize      = refPropDataIndex.m_csPgSize;
  m_csAlloc       = refPropDataIndex.m_csAlloc;
  m_csExtend      = refPropDataIndex.m_csExtend;
  m_csAllocatedPages = refPropDataIndex.m_csAllocatedPages;
  m_csFillFact    = refPropDataIndex.m_csFillFact;
  m_bPersists     = refPropDataIndex.m_bPersists;
  m_bCompression  = refPropDataIndex.m_bCompression;
  m_bCompress     = refPropDataIndex.m_bCompress;
  m_csCompress    = refPropDataIndex.m_csCompress;

  m_bEnterprise   = refPropDataIndex.m_bEnterprise;

  m_refreshParams = refPropDataIndex.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIndex CuDomPropDataIndex::operator = (const CuDomPropDataIndex & refPropDataIndex)
{
  if (this == &refPropDataIndex)
    ASSERT (FALSE);

  m_acsIdxColumns.Copy(refPropDataIndex.m_acsIdxColumns);
  m_awIdxColumns.Copy(refPropDataIndex.m_awIdxColumns);
  m_acsLocations.Copy(refPropDataIndex.m_acsLocations);

  m_csSchema      = refPropDataIndex.m_csSchema;
  m_csParent      = refPropDataIndex.m_csParent;

  m_csStorage     = refPropDataIndex.m_csStorage;
  m_bStruct1      = refPropDataIndex.m_bStruct1;
  m_csCaptStruct1 = refPropDataIndex.m_csCaptStruct1;
  m_csStruct1     = refPropDataIndex.m_csStruct1;
  m_bStruct2      = refPropDataIndex.m_bStruct2;
  m_csCaptStruct2 = refPropDataIndex.m_csCaptStruct2;
  m_csStruct2     = refPropDataIndex.m_csStruct2;
  m_bStruct3      = refPropDataIndex.m_bStruct3;
  m_csCaptStruct3 = refPropDataIndex.m_csCaptStruct3;
  m_csStruct3     = refPropDataIndex.m_csStruct3;
  m_bStruct4      = refPropDataIndex.m_bStruct4;
  m_csCaptStruct4 = refPropDataIndex.m_csCaptStruct4;
  m_csStruct4     = refPropDataIndex.m_csStruct4;

  m_csUnique      = refPropDataIndex.m_csUnique;
  m_csPgSize      = refPropDataIndex.m_csPgSize;
  m_csAlloc       = refPropDataIndex.m_csAlloc;
  m_csExtend      = refPropDataIndex.m_csExtend;
  m_csAllocatedPages = refPropDataIndex.m_csAllocatedPages;
  m_csFillFact    = refPropDataIndex.m_csFillFact;
  m_bPersists     = refPropDataIndex.m_bPersists;
  m_bCompression  = refPropDataIndex.m_bCompression;
  m_bCompress     = refPropDataIndex.m_bCompress;
  m_csCompress    = refPropDataIndex.m_csCompress;

  m_bEnterprise   = refPropDataIndex.m_bEnterprise;

  m_refreshParams = refPropDataIndex.m_refreshParams;

  return *this;
}

void CuDomPropDataIndex::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIndex::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  m_acsIdxColumns.Serialize(ar);
  m_awIdxColumns.Serialize(ar);
  m_acsLocations.Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csSchema;
    ar << m_csParent;
    
    ar << m_csStorage;
    ar << m_bStruct1;
    ar << m_csCaptStruct1;
    ar << m_csStruct1;
    ar << m_bStruct2;
    ar << m_csCaptStruct2;
    ar << m_csStruct2;
    ar << m_bStruct3;
    ar << m_csCaptStruct3;
    ar << m_csStruct3;
    ar << m_bStruct4;
    ar << m_csCaptStruct4;
    ar << m_csStruct4;
    
    ar << m_csUnique;
    ar << m_csPgSize;
    ar << m_csAlloc;
    ar << m_csExtend;
    ar << m_csAllocatedPages;
    ar << m_csFillFact;
    ar << m_bPersists;
    ar << m_bCompression; 
    ar << m_bCompress;
    ar << m_csCompress;

    ar << m_bEnterprise;
  }
  else {
    ar >> m_csSchema;
    ar >> m_csParent;
    
    ar >> m_csStorage;
    ar >> m_bStruct1;
    ar >> m_csCaptStruct1;
    ar >> m_csStruct1;
    ar >> m_bStruct2;
    ar >> m_csCaptStruct2;
    ar >> m_csStruct2;
    ar >> m_bStruct3;
    ar >> m_csCaptStruct3;
    ar >> m_csStruct3;
    ar >> m_bStruct4;
    ar >> m_csCaptStruct4;
    ar >> m_csStruct4;
    
    ar >> m_csUnique;
    ar >> m_csPgSize;
    ar >> m_csAlloc;
    ar >> m_csExtend;
    ar >> m_csAllocatedPages;
    ar >> m_csFillFact;
    ar >> m_bPersists;
    ar >> m_bCompression; 
    ar >> m_bCompress;
    ar >> m_csCompress;

    ar >> m_bEnterprise;
  }
  m_refreshParams.Serialize(ar);
}

//
// INTEGRITY
//

IMPLEMENT_SERIAL (CuDomPropDataIntegrity, CuIpmPropertyData, 2)

CuDomPropDataIntegrity::CuDomPropDataIntegrity()
    :CuIpmPropertyData("CuDomPropDataIntegrity")
{
  // nothing to do for Arrays

  m_lNumber = -1L;
  m_csText  = "";
}

// copy constructor
CuDomPropDataIntegrity::CuDomPropDataIntegrity(const CuDomPropDataIntegrity & refPropDataIntegrity)
    :CuIpmPropertyData("CuDomPropDataIntegrity")
{
  ASSERT (refPropDataIntegrity.m_lNumber != -1L);

  m_lNumber = refPropDataIntegrity.m_lNumber;
  m_csText  = refPropDataIntegrity.m_csText;

  m_refreshParams = refPropDataIntegrity.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIntegrity CuDomPropDataIntegrity::operator = (const CuDomPropDataIntegrity & refPropDataIntegrity)
{
  if (this == &refPropDataIntegrity)
    ASSERT (FALSE);

  ASSERT (refPropDataIntegrity.m_lNumber != -1L);

  m_lNumber = refPropDataIntegrity.m_lNumber;
  m_csText  = refPropDataIntegrity.m_csText;

  m_refreshParams = refPropDataIntegrity.m_refreshParams;

  return *this;
}

void CuDomPropDataIntegrity::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIntegrity::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_lNumber;
    ar << m_csText;
  }
  else {
    ar >> m_lNumber;
    ar >> m_csText;
  }
  m_refreshParams.Serialize(ar);
}

//
// RULE
//

IMPLEMENT_SERIAL (CuDomPropDataRule, CuIpmPropertyData, 2)

CuDomPropDataRule::CuDomPropDataRule()
    :CuIpmPropertyData("CuDomPropDataRule")
{
  // nothing to do for Arrays

  m_csProc = "";
  m_csText = "";
}

// copy constructor
CuDomPropDataRule::CuDomPropDataRule(const CuDomPropDataRule & refPropDataRule)
    :CuIpmPropertyData("CuDomPropDataRule")
{
  m_csProc = refPropDataRule.m_csProc;
  m_csText = refPropDataRule.m_csText;

  m_refreshParams = refPropDataRule.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataRule CuDomPropDataRule::operator = (const CuDomPropDataRule & refPropDataRule)
{
  if (this == &refPropDataRule)
    ASSERT (FALSE);

  m_csProc = refPropDataRule.m_csProc;
  m_csText = refPropDataRule.m_csText;

  m_refreshParams = refPropDataRule.m_refreshParams;

  return *this;
}

void CuDomPropDataRule::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataRule::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csProc;
    ar << m_csText;
  }
  else {
    ar >> m_csProc;
    ar >> m_csText;
  }
  m_refreshParams.Serialize(ar);
}

//
// PROCEDURE
//

IMPLEMENT_SERIAL (CuDomPropDataProcedure, CuIpmPropertyData, 2)

CuDomPropDataProcedure::CuDomPropDataProcedure()
    :CuIpmPropertyData("CuDomPropDataProcedure")
{
  m_csText = "";

  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataProcedure::CuDomPropDataProcedure(const CuDomPropDataProcedure & refPropDataProcedure)
    :CuIpmPropertyData("CuDomPropDataProcedure")
{
  m_csText = refPropDataProcedure.m_csText;
  m_acsRulesExecutingProcedure.Copy(refPropDataProcedure.m_acsRulesExecutingProcedure);

  m_refreshParams = refPropDataProcedure.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataProcedure CuDomPropDataProcedure::operator = (const CuDomPropDataProcedure & refPropDataProcedure)
{
  if (this == &refPropDataProcedure)
    ASSERT (FALSE);

  m_csText = refPropDataProcedure.m_csText;
  m_acsRulesExecutingProcedure.Copy(refPropDataProcedure.m_acsRulesExecutingProcedure);

  m_refreshParams = refPropDataProcedure.m_refreshParams;

  return *this;
}

void CuDomPropDataProcedure::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataProcedure::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csText;
  }
  else {
    ar >> m_csText;
  }

  m_acsRulesExecutingProcedure.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// SEQUENCE
//

IMPLEMENT_SERIAL (CuDomPropDataSequence, CuIpmPropertyData, 2)

CuDomPropDataSequence::CuDomPropDataSequence()
    :CuIpmPropertyData("CuDomPropDataSequence")
{
  m_csNameSeq = _T("");
  m_csOwnerSeq = _T("");
  m_bDecimalType = FALSE;
  m_csPrecSeq = _T("");
  m_csStartWith = _T("");
  m_csIncrementBy = _T("");
  m_csMaxValue = _T("");
  m_csMinValue = _T("");
  m_bCache = FALSE;
  m_csCacheSize = _T("");
  m_bCycle = FALSE;
}

// copy constructor
CuDomPropDataSequence::CuDomPropDataSequence(const CuDomPropDataSequence & refPropDataSequence)
    :CuIpmPropertyData("CuDomPropDataSequence")
{
  m_csNameSeq     = refPropDataSequence.m_csNameSeq    ;
  m_csOwnerSeq    = refPropDataSequence.m_csOwnerSeq   ;
  m_bDecimalType  = refPropDataSequence.m_bDecimalType ;
  m_csPrecSeq     = refPropDataSequence.m_csPrecSeq    ;
  m_csStartWith   = refPropDataSequence.m_csStartWith  ;
  m_csIncrementBy = refPropDataSequence.m_csIncrementBy;
  m_csMaxValue    = refPropDataSequence.m_csMaxValue   ;
  m_csMinValue    = refPropDataSequence.m_csMinValue   ;
  m_bCache        = refPropDataSequence.m_bCache       ;
  m_csCacheSize   = refPropDataSequence.m_csCacheSize  ;
  m_bCycle        = refPropDataSequence.m_bCycle       ;

  m_refreshParams = refPropDataSequence.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataSequence CuDomPropDataSequence::operator = (const CuDomPropDataSequence & refPropDataSequence)
{
  if (this == &refPropDataSequence)
    ASSERT (FALSE);

  m_csNameSeq     = refPropDataSequence.m_csNameSeq    ;
  m_csOwnerSeq    = refPropDataSequence.m_csOwnerSeq   ;
  m_bDecimalType  = refPropDataSequence.m_bDecimalType ;
  m_csPrecSeq     = refPropDataSequence.m_csPrecSeq    ;
  m_csStartWith   = refPropDataSequence.m_csStartWith  ;
  m_csIncrementBy = refPropDataSequence.m_csIncrementBy;
  m_csMaxValue    = refPropDataSequence.m_csMaxValue   ;
  m_csMinValue    = refPropDataSequence.m_csMinValue   ;
  m_bCache        = refPropDataSequence.m_bCache       ;
  m_csCacheSize   = refPropDataSequence.m_csCacheSize  ;
  m_bCycle        = refPropDataSequence.m_bCycle       ;

  return *this;
}

void CuDomPropDataSequence::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataSequence::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csNameSeq    ;
    ar << m_csOwnerSeq   ;
    ar << m_bDecimalType ;
    ar << m_csPrecSeq    ;
    ar << m_csStartWith  ;
    ar << m_csIncrementBy;
    ar << m_csMaxValue   ;
    ar << m_csMinValue   ;
    ar << m_bCache       ;
    ar << m_csCacheSize  ;
    ar << m_bCycle       ;
  }
  else {
    ar >> m_csNameSeq    ;
    ar >> m_csOwnerSeq   ;
    ar >> m_bDecimalType ;
    ar >> m_csPrecSeq    ;
    ar >> m_csStartWith  ;
    ar >> m_csIncrementBy;
    ar >> m_csMaxValue   ;
    ar >> m_csMinValue   ;
    ar >> m_bCache       ;
    ar >> m_csCacheSize  ;
    ar >> m_bCycle       ;
  }

  m_refreshParams.Serialize(ar);
}

//
// GROUP
//

IMPLEMENT_SERIAL (CuDomPropDataGroup, CuIpmPropertyData, 2)

CuDomPropDataGroup::CuDomPropDataGroup()
    :CuIpmPropertyData("CuDomPropDataGroup")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataGroup::CuDomPropDataGroup(const CuDomPropDataGroup & refPropDataGroup)
    :CuIpmPropertyData("CuDomPropDataGroup")
{
  m_acsUsersInGroup.Copy(refPropDataGroup.m_acsUsersInGroup);
  m_objType = refPropDataGroup.m_objType;
}

// '=' operator overloading
CuDomPropDataGroup CuDomPropDataGroup::operator = (const CuDomPropDataGroup & refPropDataGroup)
{
  if (this == &refPropDataGroup)
    ASSERT (FALSE);

  m_acsUsersInGroup.Copy(refPropDataGroup.m_acsUsersInGroup);
  m_objType = refPropDataGroup.m_objType;

  return *this;
}

void CuDomPropDataGroup::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataGroup::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_acsUsersInGroup.Serialize(ar);
}

//
// DB ALARMS
//

IMPLEMENT_SERIAL (CuDomPropDataDbAlarms, CuIpmPropertyData, 1)

CuDomPropDataDbAlarms::CuDomPropDataDbAlarms()
    :CuIpmPropertyData("CuDomPropDataDbAlarms")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataDbAlarms::CuDomPropDataDbAlarms(const CuDomPropDataDbAlarms & refPropDataDbAlarms)
    :CuIpmPropertyData("CuDomPropDataDbAlarms")
{
  m_uaDbAlarms.Copy(refPropDataDbAlarms.m_uaDbAlarms);
}

// '=' operator overloading
CuDomPropDataDbAlarms CuDomPropDataDbAlarms::operator = (const CuDomPropDataDbAlarms & refPropDataDbAlarms)
{
  if (this == &refPropDataDbAlarms)
    ASSERT (FALSE);

  m_uaDbAlarms.Copy(refPropDataDbAlarms.m_uaDbAlarms);

  return *this;
}

void CuDomPropDataDbAlarms::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbAlarms::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaDbAlarms.Serialize(ar);
}

//
// TABLE ALARMS
//

IMPLEMENT_SERIAL (CuDomPropDataTableAlarms, CuIpmPropertyData, 1)

CuDomPropDataTableAlarms::CuDomPropDataTableAlarms()
    :CuIpmPropertyData("CuDomPropDataTableAlarms")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataTableAlarms::CuDomPropDataTableAlarms(const CuDomPropDataTableAlarms & refPropDataTableAlarms)
    :CuIpmPropertyData("CuDomPropDataTableAlarms")
{
  m_uaTableAlarms.Copy(refPropDataTableAlarms.m_uaTableAlarms);
}

// '=' operator overloading
CuDomPropDataTableAlarms CuDomPropDataTableAlarms::operator = (const CuDomPropDataTableAlarms & refPropDataTableAlarms)
{
  if (this == &refPropDataTableAlarms)
    ASSERT (FALSE);

  m_uaTableAlarms.Copy(refPropDataTableAlarms.m_uaTableAlarms);

  return *this;
}

void CuDomPropDataTableAlarms::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTableAlarms::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaTableAlarms.Serialize(ar);
}

//
// TABLE GRANTEES
//

IMPLEMENT_SERIAL (CuDomPropDataTableGrantees, CuIpmPropertyData, 1)

CuDomPropDataTableGrantees::CuDomPropDataTableGrantees()
    :CuIpmPropertyData("CuDomPropDataTableGrantees")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataTableGrantees::CuDomPropDataTableGrantees(const CuDomPropDataTableGrantees & refPropDataTableGrantees)
    :CuIpmPropertyData("CuDomPropDataTableGrantees")
{
  m_uaTableGrantees.Copy(refPropDataTableGrantees.m_uaTableGrantees);
}

// '=' operator overloading
CuDomPropDataTableGrantees CuDomPropDataTableGrantees::operator = (const CuDomPropDataTableGrantees & refPropDataTableGrantees)
{
  if (this == &refPropDataTableGrantees)
    ASSERT (FALSE);

  m_uaTableGrantees.Copy(refPropDataTableGrantees.m_uaTableGrantees);

  return *this;
}

void CuDomPropDataTableGrantees::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTableGrantees::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaTableGrantees.Serialize(ar);
}

//
// DB GRANTEES
//

IMPLEMENT_SERIAL (CuDomPropDataDbGrantees, CuIpmPropertyData, 1)

CuDomPropDataDbGrantees::CuDomPropDataDbGrantees()
    :CuIpmPropertyData("CuDomPropDataDbGrantees")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataDbGrantees::CuDomPropDataDbGrantees(const CuDomPropDataDbGrantees & refPropDataDbGrantees)
    :CuIpmPropertyData("CuDomPropDataDbGrantees")
{
  m_uaDbGrantees.Copy(refPropDataDbGrantees.m_uaDbGrantees);
}

// '=' operator overloading
CuDomPropDataDbGrantees CuDomPropDataDbGrantees::operator = (const CuDomPropDataDbGrantees & refPropDataDbGrantees)
{
  if (this == &refPropDataDbGrantees)
    ASSERT (FALSE);

  m_uaDbGrantees.Copy(refPropDataDbGrantees.m_uaDbGrantees);

  return *this;
}

void CuDomPropDataDbGrantees::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbGrantees::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaDbGrantees.Serialize(ar);
}

//
// DB
//

IMPLEMENT_SERIAL (CuDomPropDataDb, CuIpmPropertyData, 4)

CuDomPropDataDb::CuDomPropDataDb(LPCTSTR MarkerString)
    :CuIpmPropertyData(MarkerString)
{
  // nothing to do for Arrays

  m_csOwner  = _T("");
  m_csLocDb  = _T("");
  m_csLocChk = _T("");
  m_csLocJnl = _T("");
  m_csLocDmp = _T("");
  m_csLocSrt = _T("");

  m_bPrivate     = FALSE;
  m_csFeCatalogs = _T("NNNN");
  m_bReadOnly = FALSE;
  m_bUnicodeDB = 0;
  m_csCatalogsPageSize = _T("");

}

// copy constructor
CuDomPropDataDb::CuDomPropDataDb(const CuDomPropDataDb & refPropDataDb)
    :CuIpmPropertyData("CuDomPropDataDb")
{
  m_csOwner  = refPropDataDb.m_csOwner ;
  m_csLocDb  = refPropDataDb.m_csLocDb ;
  m_csLocChk = refPropDataDb.m_csLocChk;
  m_csLocJnl = refPropDataDb.m_csLocJnl;
  m_csLocDmp = refPropDataDb.m_csLocDmp;
  m_csLocSrt = refPropDataDb.m_csLocSrt;

  m_bPrivate     = refPropDataDb.m_bPrivate;
  m_csFeCatalogs = refPropDataDb.m_csFeCatalogs;
  m_uaAltLocs.Copy(refPropDataDb.m_uaAltLocs);
  m_bReadOnly    = refPropDataDb.m_bReadOnly;
  m_refreshParams = refPropDataDb.m_refreshParams;
  m_bUnicodeDB = refPropDataDb.m_bUnicodeDB;
  m_csCatalogsPageSize = refPropDataDb.m_csCatalogsPageSize;
}

// '=' operator overloading
CuDomPropDataDb CuDomPropDataDb::operator = (const CuDomPropDataDb & refPropDataDb)
{
  if (this == &refPropDataDb)
    ASSERT (FALSE);

  m_csOwner  = refPropDataDb.m_csOwner ;
  m_csLocDb  = refPropDataDb.m_csLocDb ;
  m_csLocChk = refPropDataDb.m_csLocChk;
  m_csLocJnl = refPropDataDb.m_csLocJnl;
  m_csLocDmp = refPropDataDb.m_csLocDmp;
  m_csLocSrt = refPropDataDb.m_csLocSrt;

  m_bPrivate     = refPropDataDb.m_bPrivate;
  m_csFeCatalogs = refPropDataDb.m_csFeCatalogs;
  m_uaAltLocs.Copy(refPropDataDb.m_uaAltLocs);
  m_bReadOnly    = refPropDataDb.m_bReadOnly;
  m_bUnicodeDB = refPropDataDb.m_bUnicodeDB;
  m_csCatalogsPageSize = refPropDataDb.m_csCatalogsPageSize;

  m_refreshParams = refPropDataDb.m_refreshParams;

  return *this;
}

void CuDomPropDataDb::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDb::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csOwner ;
    ar << m_csLocDb ;
    ar << m_csLocChk;
    ar << m_csLocJnl;
    ar << m_csLocDmp;
    ar << m_csLocSrt;
    ar << m_bPrivate;
    ar << m_csFeCatalogs;
    ar << m_bReadOnly;
    ar << m_bUnicodeDB;
    ar << m_csCatalogsPageSize;
  }
  else {
    ar >> m_csOwner ;
    ar >> m_csLocDb ;
    ar >> m_csLocChk;
    ar >> m_csLocJnl;
    ar >> m_csLocDmp;
    ar >> m_csLocSrt;
    ar >> m_bPrivate;
    ar >> m_csFeCatalogs;
    ar >> m_bReadOnly;
    ar >> m_bUnicodeDB;
    ar >> m_csCatalogsPageSize;
  }
  m_uaAltLocs.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// TABLE
//

IMPLEMENT_SERIAL (CuDomPropDataTable, CuIpmPropertyData, 9)

CuDomPropDataTable::CuDomPropDataTable()
    :CuIpmPropertyData("CuDomPropDataTable")
{
  m_szSchema      = "";
  m_cJournaling   = _T('N');
  m_bReadOnly     = FALSE;
  m_nStructure    = -1;
  m_nEstimNbRows  = 0;
  m_nFillFact     = 0;
  m_nMinPages     = 0;
  m_nMaxPages     = 0;
  m_nLeafFill     = 0;
  m_nNonLeafFill  = 0;
  m_lPgAlloc      = 0L;
  m_lExtend       = 0L;
  m_lAllocatedPages = 0L;
  m_lPgSize       = 0L;

  m_bExpire       = FALSE;
  m_csExpire      = _T("");

  m_nUnique       = -1;
  m_csTableComment= _T("");

  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataTable::CuDomPropDataTable(const CuDomPropDataTable & refPropDataTable)
    :CuIpmPropertyData("CuDomPropDataTable")
{
  m_szSchema      = refPropDataTable.m_szSchema     ;
  m_csTableComment= refPropDataTable.m_csTableComment ;
  m_cJournaling   = refPropDataTable.m_cJournaling  ;
  m_bDupRows      = refPropDataTable.m_bDupRows     ;
  m_bReadOnly     = refPropDataTable.m_bReadOnly    ;
  m_nStructure    = refPropDataTable.m_nStructure   ;
  m_nEstimNbRows  = refPropDataTable.m_nEstimNbRows ;
  m_nFillFact     = refPropDataTable.m_nFillFact    ;
  m_nMinPages     = refPropDataTable.m_nMinPages    ;
  m_nMaxPages     = refPropDataTable.m_nMaxPages    ;
  m_nLeafFill     = refPropDataTable.m_nLeafFill    ;
  m_nNonLeafFill  = refPropDataTable.m_nNonLeafFill ;
  m_lPgAlloc      = refPropDataTable.m_lPgAlloc     ;
  m_lExtend       = refPropDataTable.m_lExtend      ;
  m_lAllocatedPages = refPropDataTable.m_lAllocatedPages;
  m_lPgSize       = refPropDataTable.m_lPgSize      ;

  m_uaConstraints.Copy(refPropDataTable.m_uaConstraints);

  m_bExpire       = refPropDataTable.m_bExpire;
  m_csExpire      = refPropDataTable.m_csExpire;

  m_nUnique       = refPropDataTable.m_nUnique;

  m_refreshParams = refPropDataTable.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataTable CuDomPropDataTable::operator = (const CuDomPropDataTable & refPropDataTable)
{
  if (this == &refPropDataTable)
    ASSERT (FALSE);

  m_szSchema      = refPropDataTable.m_szSchema     ;
  m_csTableComment= refPropDataTable.m_csTableComment ;
  m_cJournaling   = refPropDataTable.m_cJournaling  ;
  m_bDupRows      = refPropDataTable.m_bDupRows     ;
  m_bReadOnly     = refPropDataTable.m_bReadOnly    ;
  m_nStructure    = refPropDataTable.m_nStructure   ;
  m_nEstimNbRows  = refPropDataTable.m_nEstimNbRows ;
  m_nFillFact     = refPropDataTable.m_nFillFact    ;
  m_nMinPages     = refPropDataTable.m_nMinPages    ;
  m_nMaxPages     = refPropDataTable.m_nMaxPages    ;
  m_nLeafFill     = refPropDataTable.m_nLeafFill    ;
  m_nNonLeafFill  = refPropDataTable.m_nNonLeafFill ;
  m_lPgAlloc      = refPropDataTable.m_lPgAlloc     ;
  m_lExtend       = refPropDataTable.m_lExtend      ;
  m_lAllocatedPages = refPropDataTable.m_lAllocatedPages;
  m_lPgSize       = refPropDataTable.m_lPgSize      ;

  m_uaConstraints.Copy(refPropDataTable.m_uaConstraints);

  m_bExpire       = refPropDataTable.m_bExpire;
  m_csExpire      = refPropDataTable.m_csExpire;

  m_nUnique       = refPropDataTable.m_nUnique;

  m_refreshParams = refPropDataTable.m_refreshParams;

  return *this;
}

void CuDomPropDataTable::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTable::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_szSchema     ;
    ar << m_cJournaling  ;
    ar << m_bDupRows     ;
    ar << m_bReadOnly    ;
    ar << m_nStructure   ;
    ar << m_nEstimNbRows ;
    ar << m_nFillFact    ;
    ar << m_nMinPages    ;
    ar << m_nMaxPages    ;
    ar << m_nLeafFill    ;
    ar << m_nNonLeafFill ;
    ar << m_lPgAlloc     ;
    ar << m_lExtend      ;
    ar << m_lAllocatedPages;
    ar << m_lPgSize      ;
    ar << m_bExpire;
    ar << m_csExpire;
    ar << m_nUnique;
    ar << m_csTableComment;
  }
  else {
    ar >> m_szSchema     ;
    ar >> m_cJournaling  ;
    ar >> m_bDupRows     ;
    ar >> m_bReadOnly    ;
    ar >> m_nStructure   ;
    ar >> m_nEstimNbRows ;
    ar >> m_nFillFact    ;
    ar >> m_nMinPages    ;
    ar >> m_nMaxPages    ;
    ar >> m_nLeafFill    ;
    ar >> m_nNonLeafFill ;
    ar >> m_lPgAlloc     ;
    ar >> m_lExtend      ;
    ar >> m_lAllocatedPages;
    ar >> m_lPgSize      ;
    ar >> m_bExpire;
    ar >> m_csExpire;
    ar >> m_nUnique;
    ar >> m_csTableComment;
  }

  m_uaConstraints.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// VIEW GRANTEES
//

IMPLEMENT_SERIAL (CuDomPropDataViewGrantees, CuIpmPropertyData, 1)

CuDomPropDataViewGrantees::CuDomPropDataViewGrantees()
    :CuIpmPropertyData("CuDomPropDataViewGrantees")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataViewGrantees::CuDomPropDataViewGrantees(const CuDomPropDataViewGrantees & refPropDataViewGrantees)
    :CuIpmPropertyData("CuDomPropDataViewGrantees")
{
  m_uaViewGrantees.Copy(refPropDataViewGrantees.m_uaViewGrantees);
}

// '=' operator overloading
CuDomPropDataViewGrantees CuDomPropDataViewGrantees::operator = (const CuDomPropDataViewGrantees & refPropDataViewGrantees)
{
  if (this == &refPropDataViewGrantees)
    ASSERT (FALSE);

  m_uaViewGrantees.Copy(refPropDataViewGrantees.m_uaViewGrantees);

  return *this;
}

void CuDomPropDataViewGrantees::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataViewGrantees::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaViewGrantees.Serialize(ar);
}

//
// VIEW
//

IMPLEMENT_SERIAL (CuDomPropDataView, CuIpmPropertyData, 5)

CuDomPropDataView::CuDomPropDataView()
    :CuIpmPropertyData("CuDomPropDataView")
{
  m_csText = "";
  m_csComment = "";
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataView::CuDomPropDataView(const CuDomPropDataView & refPropDataView)
    :CuIpmPropertyData("CuDomPropDataView")
{
  m_csText = refPropDataView.m_csText;
  m_csComment = refPropDataView.m_csComment;
  m_acsViewComponents.Copy(refPropDataView.m_acsViewComponents);
  m_acsSchema.Copy(refPropDataView.m_acsSchema);
  m_awCompType.Copy(refPropDataView.m_awCompType);

  m_refreshParams = refPropDataView.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataView CuDomPropDataView::operator = (const CuDomPropDataView & refPropDataView)
{
  if (this == &refPropDataView)
    ASSERT (FALSE);

  m_csText = refPropDataView.m_csText;
  m_csComment = refPropDataView.m_csComment;
  m_acsViewComponents.Copy(refPropDataView.m_acsViewComponents);
  m_acsSchema.Copy(refPropDataView.m_acsSchema);
  m_awCompType.Copy(refPropDataView.m_awCompType);

  m_refreshParams = refPropDataView.m_refreshParams;

  return *this;
}

void CuDomPropDataView::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataView::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csText;
    ar << m_csComment;
  }
  else {
    ar >> m_csText;
    ar >> m_csComment;
  }

  m_acsViewComponents.Serialize(ar);
  m_acsSchema.Serialize(ar);
  m_awCompType.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// CDDS
//

IMPLEMENT_SERIAL (CuDomPropDataCdds, CuIpmPropertyData, 3)

CuDomPropDataCdds::CuDomPropDataCdds()
    :CuIpmPropertyData("CuDomPropDataCdds")
{
  m_no             = -1;
  m_csName         = "";
  m_collisionMode  = -1;
  m_errorMode      = -1;

  // nothing to do for CStringArrays and cuArrays
}

// copy constructor
CuDomPropDataCdds::CuDomPropDataCdds(const CuDomPropDataCdds & refPropDataCdds)
    :CuIpmPropertyData("CuDomPropDataCdds")
{
  m_no             = refPropDataCdds.m_no            ;
  m_csName         = refPropDataCdds.m_csName        ;
  m_collisionMode  = refPropDataCdds.m_collisionMode ;
  m_errorMode      = refPropDataCdds.m_errorMode     ;

  m_uaPropagationPaths.Copy(refPropDataCdds.m_uaPropagationPaths);
  m_uaDbInfos.Copy(refPropDataCdds.m_uaDbInfos);

  m_refreshParams = refPropDataCdds.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataCdds CuDomPropDataCdds::operator = (const CuDomPropDataCdds & refPropDataCdds)
{
  if (this == &refPropDataCdds)
    ASSERT (FALSE);

  m_no             = refPropDataCdds.m_no            ;
  m_csName         = refPropDataCdds.m_csName        ;
  m_collisionMode  = refPropDataCdds.m_collisionMode ;
  m_errorMode      = refPropDataCdds.m_errorMode     ;

  m_uaPropagationPaths.Copy(refPropDataCdds.m_uaPropagationPaths);
  m_uaDbInfos.Copy(refPropDataCdds.m_uaDbInfos);

  m_refreshParams = refPropDataCdds.m_refreshParams;

  return *this;
}

void CuDomPropDataCdds::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataCdds::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_no;
    ar << m_csName;
    ar << m_collisionMode;
    ar << m_errorMode;
  }
  else {
    ar >> m_no;
    ar >> m_csName;
    ar >> m_collisionMode;
    ar >> m_errorMode;
  }

  m_uaPropagationPaths.Serialize(ar);
  m_uaDbInfos.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// CDDS TABLES
//

IMPLEMENT_SERIAL (CuDomPropDataCddsTables, CuIpmPropertyData, 2)

CuDomPropDataCddsTables::CuDomPropDataCddsTables()
    :CuIpmPropertyData("CuDomPropDataCddsTables")
{
  m_nCurSel = -1;
  // nothing to do for CStringArrays and cuArrays
}

// copy constructor
CuDomPropDataCddsTables::CuDomPropDataCddsTables(const CuDomPropDataCddsTables & refPropDataCddsTables)
    :CuIpmPropertyData("CuDomPropDataCddsTables")
{
  m_uaCddsTables.Copy(refPropDataCddsTables.m_uaCddsTables);
  //m_uaCddsTableCols.Copy(refPropDataCddsTables.m_uaCddsTableCols);
  m_nCurSel = refPropDataCddsTables.m_nCurSel;

  m_refreshParams = refPropDataCddsTables.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataCddsTables CuDomPropDataCddsTables::operator = (const CuDomPropDataCddsTables & refPropDataCddsTables)
{
  if (this == &refPropDataCddsTables)
    ASSERT (FALSE);

  m_uaCddsTables.Copy(refPropDataCddsTables.m_uaCddsTables);
  //m_uaCddsTableCols.Copy(refPropDataCddsTables.m_uaCddsTableCols);
  m_nCurSel = refPropDataCddsTables.m_nCurSel;

  m_refreshParams = refPropDataCddsTables.m_refreshParams;

  return *this;
}

void CuDomPropDataCddsTables::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataCddsTables::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_nCurSel;
  }
  else {
    ar >> m_nCurSel;
  }

  m_uaCddsTables.Serialize(ar);
  //m_uaCddsTableCols.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// DBEVENT GRANTEES
//

IMPLEMENT_SERIAL (CuDomPropDataDbEventGrantees, CuIpmPropertyData, 1)

CuDomPropDataDbEventGrantees::CuDomPropDataDbEventGrantees()
    :CuIpmPropertyData("CuDomPropDataDbEventGrantees")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataDbEventGrantees::CuDomPropDataDbEventGrantees(const CuDomPropDataDbEventGrantees & refPropDataDbEventGrantees)
    :CuIpmPropertyData("CuDomPropDataDbEventGrantees")
{
  m_uaDbEventGrantees.Copy(refPropDataDbEventGrantees.m_uaDbEventGrantees);
}

// '=' operator overloading
CuDomPropDataDbEventGrantees CuDomPropDataDbEventGrantees::operator = (const CuDomPropDataDbEventGrantees & refPropDataDbEventGrantees)
{
  if (this == &refPropDataDbEventGrantees)
    ASSERT (FALSE);

  m_uaDbEventGrantees.Copy(refPropDataDbEventGrantees.m_uaDbEventGrantees);

  return *this;
}

void CuDomPropDataDbEventGrantees::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDbEventGrantees::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaDbEventGrantees.Serialize(ar);
}

//
// PROCEDURE GRANTEES
//

IMPLEMENT_SERIAL (CuDomPropDataProcGrantees, CuIpmPropertyData, 1)

CuDomPropDataProcGrantees::CuDomPropDataProcGrantees()
    :CuIpmPropertyData("CuDomPropDataProcGrantees")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataProcGrantees::CuDomPropDataProcGrantees(const CuDomPropDataProcGrantees & refPropDataProcGrantees)
    :CuIpmPropertyData("CuDomPropDataProcGrantees")
{
  m_uaProcGrantees.Copy(refPropDataProcGrantees.m_uaProcGrantees);
}

// '=' operator overloading
CuDomPropDataProcGrantees CuDomPropDataProcGrantees::operator = (const CuDomPropDataProcGrantees & refPropDataProcGrantees)
{
  if (this == &refPropDataProcGrantees)
    ASSERT (FALSE);

  m_uaProcGrantees.Copy(refPropDataProcGrantees.m_uaProcGrantees);

  return *this;
}

void CuDomPropDataProcGrantees::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataProcGrantees::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaProcGrantees.Serialize(ar);
}

//
// SEQUENCE GRANTEES
//

IMPLEMENT_SERIAL (CuDomPropDataSeqGrantees, CuIpmPropertyData, 1)

CuDomPropDataSeqGrantees::CuDomPropDataSeqGrantees()
    :CuIpmPropertyData("CuDomPropDataSeqGrantees")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataSeqGrantees::CuDomPropDataSeqGrantees(const CuDomPropDataSeqGrantees & refPropDataSeqGrantees)
    :CuIpmPropertyData("CuDomPropDataSeqGrantees")
{
  m_uaSeqGrantees.Copy(refPropDataSeqGrantees.m_uaSeqGrantees);
}

// '=' operator overloading
CuDomPropDataSeqGrantees CuDomPropDataSeqGrantees::operator = (const CuDomPropDataSeqGrantees & refPropDataSeqGrantees)
{
  if (this == &refPropDataSeqGrantees)
    ASSERT (FALSE);

  m_uaSeqGrantees.Copy(refPropDataSeqGrantees.m_uaSeqGrantees);

  return *this;
}

void CuDomPropDataSeqGrantees::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataSeqGrantees::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaSeqGrantees.Serialize(ar);
}


//
// TABLE COLUMNS
//

IMPLEMENT_SERIAL (CuDomPropDataTableColumns, CuIpmPropertyData, 4)

CuDomPropDataTableColumns::CuDomPropDataTableColumns()
    :CuIpmPropertyData("CuDomPropDataTableColumns")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataTableColumns::CuDomPropDataTableColumns(const CuDomPropDataTableColumns & refPropDataTableColumns)
    :CuIpmPropertyData("CuDomPropDataTableColumns")
{
  m_uaColumns.Copy(refPropDataTableColumns.m_uaColumns);

  m_refreshParams = refPropDataTableColumns.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataTableColumns CuDomPropDataTableColumns::operator = (const CuDomPropDataTableColumns & refPropDataTableColumns)
{
  if (this == &refPropDataTableColumns)
    ASSERT (FALSE);

  m_uaColumns.Copy(refPropDataTableColumns.m_uaColumns);

  m_refreshParams = refPropDataTableColumns.m_refreshParams;

  return *this;
}

void CuDomPropDataTableColumns::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTableColumns::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
  m_uaColumns.Serialize(ar);

  m_refreshParams.Serialize(ar);
}


//
// TABLE/VIEW ROWS
//

IMPLEMENT_SERIAL (CuDomPropDataTableRows, CuIpmPropertyData, 2)
CuDomPropDataTableRows::CuDomPropDataTableRows()
    :CuIpmPropertyData(_T("CuDomPropDataTableRows"))
{
}

void CuDomPropDataTableRows::NotifyLoad(CWnd* pDlg)
{
	pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTableRows::Serialize (CArchive& ar)
{
	CuIpmPropertyData::Serialize(ar);
	if (ar.IsStoring()) 
	{
		IStream* pStream = m_streamFile.GetStream();
		CArchiveFromIStream(ar, pStream);
	}
	else 
	{
		IStream* pStream = NULL;
		BOOL bOK = CArchiveToIStream  (ar, &pStream);
		if (bOK)
			m_streamFile.Attach(pStream);
	}
}


//
// TABLE/VIEW STATISTIC
//

IMPLEMENT_SERIAL (CuDomPropDataTableStatistic, CuIpmPropertyData, 2)
CuDomPropDataTableStatistic::CuDomPropDataTableStatistic()
    :CuIpmPropertyData("CuDomPropDataTableStatistic")
{
    m_nSelectColumn = -1;
    m_scrollPos = CSize (0, 0);

}

void CuDomPropDataTableStatistic::Serialize (CArchive& ar)
{
    CuIpmPropertyData::Serialize(ar);
    m_statisticData.Serialize(ar);
    if (ar.IsStoring()) 
    {
        ar << m_nSelectColumn;
        ar << m_scrollPos;
    }
    else 
    {
        ar >> m_nSelectColumn;
        ar >> m_scrollPos;
    }
}

void CuDomPropDataTableStatistic::NotifyLoad(CWnd* pDlg)
{
    pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

//
// STAR (Distributed) DATABASE - DDB
//

IMPLEMENT_SERIAL (CuDomPropDataDDb, CuDomPropDataDb, 2)

CuDomPropDataDDb::CuDomPropDataDDb()
    :CuDomPropDataDb("CuDomPropDataDDb")  // marker string different
{
  // nothing to do for Arrays

  m_csCoordinator  = _T("");
}

// copy constructor
CuDomPropDataDDb::CuDomPropDataDDb(const CuDomPropDataDDb & refPropDataDDb)
    :CuDomPropDataDb(refPropDataDDb)
{
  m_csCoordinator = refPropDataDDb.m_csCoordinator;
}

// '=' operator overloading
CuDomPropDataDDb CuDomPropDataDDb::operator = (const CuDomPropDataDDb & refPropDataDDb)
{
  // parent class operator overloading
  (*((CuDomPropDataDb*)this)) = refPropDataDDb;

  if (this == &refPropDataDDb)
    ASSERT (FALSE);

  m_csCoordinator = refPropDataDDb.m_csCoordinator;

  return *this;
}

void CuDomPropDataDDb::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataDDb::Serialize (CArchive& ar)
{
  CuDomPropDataDb::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csCoordinator;
  }
  else {
    ar >> m_csCoordinator;
  }
}

//
// STAR TABLE REGISTERED AS LINK
//

IMPLEMENT_SERIAL (CuDomPropDataStarTableLink, CuIpmPropertyData, 3)

CuDomPropDataStarTableLink::CuDomPropDataStarTableLink()
    :CuIpmPropertyData("CuDomPropDataStarTableLink")
{
  m_csLdbObjName  = "";
  m_csLdbObjOwner = "";
  m_csLdbDatabase = "";
  m_csLdbNode     = "";
  m_csLdbDbmsType = "";

  // nothing to do for m_uaColumns
}

// copy constructor
CuDomPropDataStarTableLink::CuDomPropDataStarTableLink(const CuDomPropDataStarTableLink & refPropDataStarTableLink)
    :CuIpmPropertyData("CuDomPropDataStarTableLink")
{
  m_csLdbObjName   = refPropDataStarTableLink.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataStarTableLink.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataStarTableLink.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataStarTableLink.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataStarTableLink.m_csLdbDbmsType  ;

  m_uaColumns.Copy(refPropDataStarTableLink.m_uaColumns);

  m_refreshParams = refPropDataStarTableLink.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataStarTableLink CuDomPropDataStarTableLink::operator = (const CuDomPropDataStarTableLink & refPropDataStarTableLink)
{
  if (this == &refPropDataStarTableLink)
    ASSERT (FALSE);

  m_csLdbObjName   = refPropDataStarTableLink.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataStarTableLink.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataStarTableLink.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataStarTableLink.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataStarTableLink.m_csLdbDbmsType  ;

  m_uaColumns.Copy(refPropDataStarTableLink.m_uaColumns);

  m_refreshParams = refPropDataStarTableLink.m_refreshParams;

  return *this;
}

void CuDomPropDataStarTableLink::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataStarTableLink::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csLdbObjName  ;
    ar << m_csLdbObjOwner ;
    ar << m_csLdbDatabase ;
    ar << m_csLdbNode     ;
    ar << m_csLdbDbmsType ;
  }
  else {
    ar >> m_csLdbObjName  ;
    ar >> m_csLdbObjOwner ;
    ar >> m_csLdbDatabase ;
    ar >> m_csLdbNode     ;
    ar >> m_csLdbDbmsType ;
  }

  m_uaColumns.Serialize(ar);

  m_refreshParams.Serialize(ar);
}

//
// STAR INDEX REGISTERED AS LINK: SOURCE
//

IMPLEMENT_SERIAL (CuDomPropDataIndexLinkSource, CuIpmPropertyData, 1)

CuDomPropDataIndexLinkSource::CuDomPropDataIndexLinkSource()
    :CuIpmPropertyData("CuDomPropDataIndexLinkSource")
{
  m_csLdbObjName  = "";
  m_csLdbObjOwner = "";
  m_csLdbDatabase = "";
  m_csLdbNode     = "";
  m_csLdbDbmsType = "";
}

// copy constructor
CuDomPropDataIndexLinkSource::CuDomPropDataIndexLinkSource(const CuDomPropDataIndexLinkSource & refPropDataIndexLinkSource)
    :CuIpmPropertyData("CuDomPropDataIndexLinkSource")
{
  m_csLdbObjName   = refPropDataIndexLinkSource.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataIndexLinkSource.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataIndexLinkSource.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataIndexLinkSource.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataIndexLinkSource.m_csLdbDbmsType  ;

  m_refreshParams = refPropDataIndexLinkSource.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIndexLinkSource CuDomPropDataIndexLinkSource::operator = (const CuDomPropDataIndexLinkSource & refPropDataIndexLinkSource)
{
  if (this == &refPropDataIndexLinkSource)
    ASSERT (FALSE);

  m_csLdbObjName   = refPropDataIndexLinkSource.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataIndexLinkSource.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataIndexLinkSource.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataIndexLinkSource.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataIndexLinkSource.m_csLdbDbmsType  ;

  m_refreshParams = refPropDataIndexLinkSource.m_refreshParams;

  return *this;
}

void CuDomPropDataIndexLinkSource::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIndexLinkSource::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csLdbObjName  ;
    ar << m_csLdbObjOwner ;
    ar << m_csLdbDatabase ;
    ar << m_csLdbNode     ;
    ar << m_csLdbDbmsType ;
  }
  else {
    ar >> m_csLdbObjName  ;
    ar >> m_csLdbObjOwner ;
    ar >> m_csLdbDatabase ;
    ar >> m_csLdbNode     ;
    ar >> m_csLdbDbmsType ;
  }

  m_refreshParams.Serialize(ar);
}

//
// STAR VIEW REGISTERED AS LINK: SOURCE
//

IMPLEMENT_SERIAL (CuDomPropDataViewLinkSource, CuIpmPropertyData, 1)

CuDomPropDataViewLinkSource::CuDomPropDataViewLinkSource()
    :CuIpmPropertyData("CuDomPropDataViewLinkSource")
{
  m_csLdbObjName  = "";
  m_csLdbObjOwner = "";
  m_csLdbDatabase = "";
  m_csLdbNode     = "";
  m_csLdbDbmsType = "";
}

// copy constructor
CuDomPropDataViewLinkSource::CuDomPropDataViewLinkSource(const CuDomPropDataViewLinkSource & refPropDataViewLinkSource)
    :CuIpmPropertyData("CuDomPropDataViewLinkSource")
{
  m_csLdbObjName   = refPropDataViewLinkSource.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataViewLinkSource.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataViewLinkSource.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataViewLinkSource.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataViewLinkSource.m_csLdbDbmsType  ;

  m_refreshParams = refPropDataViewLinkSource.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataViewLinkSource CuDomPropDataViewLinkSource::operator = (const CuDomPropDataViewLinkSource & refPropDataViewLinkSource)
{
  if (this == &refPropDataViewLinkSource)
    ASSERT (FALSE);

  m_csLdbObjName   = refPropDataViewLinkSource.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataViewLinkSource.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataViewLinkSource.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataViewLinkSource.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataViewLinkSource.m_csLdbDbmsType  ;

  m_refreshParams = refPropDataViewLinkSource.m_refreshParams;

  return *this;
}

void CuDomPropDataViewLinkSource::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataViewLinkSource::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csLdbObjName  ;
    ar << m_csLdbObjOwner ;
    ar << m_csLdbDatabase ;
    ar << m_csLdbNode     ;
    ar << m_csLdbDbmsType ;
  }
  else {
    ar >> m_csLdbObjName  ;
    ar >> m_csLdbObjOwner ;
    ar >> m_csLdbDatabase ;
    ar >> m_csLdbNode     ;
    ar >> m_csLdbDbmsType ;
  }

  m_refreshParams.Serialize(ar);
}

//
// STAR VIEW NATIVE: SOURCE
//

IMPLEMENT_SERIAL (CuDomPropDataViewNativeSource, CuIpmPropertyData, 1)

CuDomPropDataViewNativeSource::CuDomPropDataViewNativeSource()
    :CuIpmPropertyData("CuDomPropDataViewNativeSource")
{
  m_csLdbObjName  = "";
  m_csLdbObjOwner = "";
  m_csLdbDatabase = "";
  m_csLdbNode     = "";
  m_csLdbDbmsType = "";
}

// copy constructor
CuDomPropDataViewNativeSource::CuDomPropDataViewNativeSource(const CuDomPropDataViewNativeSource & refPropDataViewNativeSource)
    :CuIpmPropertyData("CuDomPropDataViewNativeSource")
{
  m_csLdbObjName   = refPropDataViewNativeSource.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataViewNativeSource.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataViewNativeSource.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataViewNativeSource.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataViewNativeSource.m_csLdbDbmsType  ;

  m_refreshParams = refPropDataViewNativeSource.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataViewNativeSource CuDomPropDataViewNativeSource::operator = (const CuDomPropDataViewNativeSource & refPropDataViewNativeSource)
{
  if (this == &refPropDataViewNativeSource)
    ASSERT (FALSE);

  m_csLdbObjName   = refPropDataViewNativeSource.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataViewNativeSource.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataViewNativeSource.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataViewNativeSource.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataViewNativeSource.m_csLdbDbmsType  ;

  m_refreshParams = refPropDataViewNativeSource.m_refreshParams;

  return *this;
}

void CuDomPropDataViewNativeSource::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataViewNativeSource::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csLdbObjName  ;
    ar << m_csLdbObjOwner ;
    ar << m_csLdbDatabase ;
    ar << m_csLdbNode     ;
    ar << m_csLdbDbmsType ;
  }
  else {
    ar >> m_csLdbObjName  ;
    ar >> m_csLdbObjOwner ;
    ar >> m_csLdbDatabase ;
    ar >> m_csLdbNode     ;
    ar >> m_csLdbDbmsType ;
  }

  m_refreshParams.Serialize(ar);
}

//
// STAR PROCEDURE REGISTERED AS LINK: SOURCE
//

IMPLEMENT_SERIAL (CuDomPropDataProcLinkSource, CuIpmPropertyData, 1)

CuDomPropDataProcLinkSource::CuDomPropDataProcLinkSource()
    :CuIpmPropertyData("CuDomPropDataProcLinkSource")
{
  m_csLdbObjName  = "";
  m_csLdbObjOwner = "";
  m_csLdbDatabase = "";
  m_csLdbNode     = "";
  m_csLdbDbmsType = "";
}

// copy constructor
CuDomPropDataProcLinkSource::CuDomPropDataProcLinkSource(const CuDomPropDataProcLinkSource & refPropDataProcLinkSource)
    :CuIpmPropertyData("CuDomPropDataProcLinkSource")
{
  m_csLdbObjName   = refPropDataProcLinkSource.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataProcLinkSource.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataProcLinkSource.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataProcLinkSource.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataProcLinkSource.m_csLdbDbmsType  ;

  m_refreshParams = refPropDataProcLinkSource.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataProcLinkSource CuDomPropDataProcLinkSource::operator = (const CuDomPropDataProcLinkSource & refPropDataProcLinkSource)
{
  if (this == &refPropDataProcLinkSource)
    ASSERT (FALSE);

  m_csLdbObjName   = refPropDataProcLinkSource.m_csLdbObjName   ;
  m_csLdbObjOwner  = refPropDataProcLinkSource.m_csLdbObjOwner  ;
  m_csLdbDatabase  = refPropDataProcLinkSource.m_csLdbDatabase  ;
  m_csLdbNode      = refPropDataProcLinkSource.m_csLdbNode      ;
  m_csLdbDbmsType  = refPropDataProcLinkSource.m_csLdbDbmsType  ;

  m_refreshParams = refPropDataProcLinkSource.m_refreshParams;

  return *this;
}

void CuDomPropDataProcLinkSource::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataProcLinkSource::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csLdbObjName  ;
    ar << m_csLdbObjOwner ;
    ar << m_csLdbDatabase ;
    ar << m_csLdbNode     ;
    ar << m_csLdbDbmsType ;
  }
  else {
    ar >> m_csLdbObjName  ;
    ar >> m_csLdbObjOwner ;
    ar >> m_csLdbDatabase ;
    ar >> m_csLdbNode     ;
    ar >> m_csLdbDbmsType ;
  }

  m_refreshParams.Serialize(ar);
}

//
// SHOW NO ROWS
//
IMPLEMENT_SERIAL (CuDomPropDataTableRowsNA, CuIpmPropertyData, 1)

CuDomPropDataTableRowsNA::CuDomPropDataTableRowsNA()
    :CuIpmPropertyData("CuDomPropDataTableRowsNA")
{
  // nothing to do for Arrays

  m_csDummy = "";
}

// copy constructor
CuDomPropDataTableRowsNA::CuDomPropDataTableRowsNA(const CuDomPropDataTableRowsNA & refPropDataTableRowsNA)
    :CuIpmPropertyData("CuDomPropDataTableRowsNA")
{
  m_csDummy = refPropDataTableRowsNA.m_csDummy;

}

// '=' operator overloading
CuDomPropDataTableRowsNA CuDomPropDataTableRowsNA::operator = (const CuDomPropDataTableRowsNA & refPropDataTableRowsNA)
{
  if (this == &refPropDataTableRowsNA)
    ASSERT (FALSE);

  m_csDummy = refPropDataTableRowsNA.m_csDummy;

  return *this;
}

void CuDomPropDataTableRowsNA::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataTableRowsNA::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csDummy;
  }
  else {
    ar >> m_csDummy;
  }
}



//
// DOM PROP: AUDIT ALL USERS
// ***************************************************************
IMPLEMENT_SERIAL (CuDomPropDataPageInstallationLevelAuditAllUsers, CuIpmPropertyData, 1)
CuDomPropDataPageInstallationLevelAuditAllUsers::CuDomPropDataPageInstallationLevelAuditAllUsers()
    :CuIpmPropertyData("CuDomPropDataPageInstallationLevelAuditAllUsers")
{

}

CuDomPropDataPageInstallationLevelAuditAllUsers::~CuDomPropDataPageInstallationLevelAuditAllUsers()
{
    while (!m_listTuple.IsEmpty())
        delete m_listTuple.RemoveHead();
}

void CuDomPropDataPageInstallationLevelAuditAllUsers::NotifyLoad (CWnd* pDlg)
{
    pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}


void CuDomPropDataPageInstallationLevelAuditAllUsers::Serialize  (CArchive& ar)
{
    m_listTuple.Serialize (ar);
}


//
// DOM PROP: ENABLED LEVEL
// ***************************************************************
IMPLEMENT_SERIAL (CaAuditEnabledLevel, CObject, 1)
void CaAuditEnabledLevel::Serialize  (CArchive& ar)
{
    if (ar.IsStoring()) 
    {
        ar << m_strAuditFlag;
        ar << m_bEnabled;
    }
    else 
    {
        ar >> m_strAuditFlag;
        ar >> m_bEnabled;
    }
}

IMPLEMENT_SERIAL (CuDomPropDataPageInstallationLevelEnabledLevel, CuIpmPropertyData, 1)
CuDomPropDataPageInstallationLevelEnabledLevel::CuDomPropDataPageInstallationLevelEnabledLevel()
    :CuIpmPropertyData("CuDomPropDataPageInstallationLevelEnabledLevel")
{

}

CuDomPropDataPageInstallationLevelEnabledLevel::~CuDomPropDataPageInstallationLevelEnabledLevel()
{
    while (!m_listTuple.IsEmpty())
        delete m_listTuple.RemoveHead();
}

void CuDomPropDataPageInstallationLevelEnabledLevel::NotifyLoad (CWnd* pDlg)
{
    pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)&m_listTuple);
}


void CuDomPropDataPageInstallationLevelEnabledLevel::Serialize  (CArchive& ar)
{
    m_listTuple.Serialize (ar);
}


//
// DOM PROP: AUDIT LOG
// ***************************************************************
IMPLEMENT_SERIAL (CuDomPropDataPageInstallationLevelAuditLog, CuIpmPropertyData, 1)
CuDomPropDataPageInstallationLevelAuditLog::CuDomPropDataPageInstallationLevelAuditLog()
    :CuIpmPropertyData("CuDomPropDataPageInstallationLevelAuditLog")
{
    m_pRowData = NULL;
}

CuDomPropDataPageInstallationLevelAuditLog::~CuDomPropDataPageInstallationLevelAuditLog()
{
    while (!m_listObjectName.IsEmpty())
        delete m_listObjectName.RemoveHead();
    if (m_pRowData)
        delete m_pRowData;
}

void CuDomPropDataPageInstallationLevelAuditLog::NotifyLoad (CWnd* pDlg)
{
    pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}


void CuDomPropDataPageInstallationLevelAuditLog::Serialize  (CArchive& ar)
{
    m_listEventType.Serialize (ar);
    m_listDatabase.Serialize (ar);
    m_listObjectType.Serialize (ar);
    m_listUser.Serialize (ar);
    m_listRealUser.Serialize (ar);
    m_listBegin.Serialize (ar);
    m_listEnd.Serialize (ar);
    m_listObjectName.Serialize (ar);

    if (ar.IsStoring()) 
    {
        ar << m_nSelEventType;
        ar << m_nSelDatabase;
        ar << m_nSelObjectType;
        ar << m_nSelObjectName;
        ar << m_nSelObjectName;
        ar << m_nSelUser;
        ar << m_nSelRealUser;
        
        ar << m_strBegin;
        ar << m_strEnd;
        ar << m_pRowData;
    }
    else 
    {
        ar >> m_nSelEventType;
        ar >> m_nSelDatabase;
        ar >> m_nSelObjectType;
        ar >> m_nSelObjectName;
        ar >> m_nSelObjectName;
        ar >> m_nSelUser;
        ar >> m_nSelRealUser;
        
        ar >> m_strBegin;
        ar >> m_strEnd;
        ar >> m_pRowData;
    }
}
