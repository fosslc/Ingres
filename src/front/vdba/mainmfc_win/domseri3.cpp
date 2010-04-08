/**************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Source   : DomSeri3.cpp, Code File
**             Serialisation Classes for ICE
**
**  Project  : Ingres Visual DBA
**  Author   : Emmanuel Blattes
**
**  Purpose  : Serialization of DOM right pane property pages (modeless dialog)
**
**  History after 01-01-2000
**   03-Mar-2000 (noifr01)
**    (bug 100713) added a warning when a facet or page can't be saved / loaded
**    because of its size. Increased the max size from 30000 to 120000 bytes
**   11-May-2000 (noifr01)
**    (bug 101492) implemented emergency workaround: archive ICE facets and pages
**    byte per byte (casted as integers), rather than using the Write() method
**    for the whole object at once, which was resulting in problems with 
**    compression
*******************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "domseri3.h"


extern "C" {
#include "dba.h"
#include "esltools.h"
#include "dbafile.h"
};

#define MAXICEDOCSIZE4SAVE 120000
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceDbuser, CuIpmPropertyData, 2)

CuDomPropDataIceDbuser::CuDomPropDataIceDbuser()
    :CuIpmPropertyData("CuDomPropDataIceDbuser")
{
  m_csUserAlias = _T("");
  m_csComment   = _T("");
  m_csUserName  = _T("");
}

// copy constructor
CuDomPropDataIceDbuser::CuDomPropDataIceDbuser(const CuDomPropDataIceDbuser & refPropDataIceDbuser)
    :CuIpmPropertyData("CuDomPropDataIceDbuser")
{
  m_csUserAlias = refPropDataIceDbuser.m_csUserAlias;
  m_csComment   = refPropDataIceDbuser.m_csComment;
  m_csUserName  = refPropDataIceDbuser.m_csUserName;

  m_refreshParams = refPropDataIceDbuser.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceDbuser CuDomPropDataIceDbuser::operator = (const CuDomPropDataIceDbuser & refPropDataIceDbuser)
{
  if (this == &refPropDataIceDbuser)
    ASSERT (FALSE);

  m_csUserAlias = refPropDataIceDbuser.m_csUserAlias;
  m_csComment   = refPropDataIceDbuser.m_csComment;
  m_csUserName  = refPropDataIceDbuser.m_csUserName;

  m_refreshParams = refPropDataIceDbuser.m_refreshParams;

  return *this;
}

void CuDomPropDataIceDbuser::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceDbuser::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csUserAlias;
    ar << m_csComment;
    ar << m_csUserName;
  }
  else {
    ar >> m_csUserAlias;
    ar >> m_csComment;
    ar >> m_csUserName;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceDbconnection, CuIpmPropertyData, 1)

CuDomPropDataIceDbconnection::CuDomPropDataIceDbconnection()
    :CuIpmPropertyData("CuDomPropDataIceDbconnection")
{
  m_csDBName  = _T("");
  m_csDBUser  = _T("");
  m_csComment = _T("");
}

// copy constructor
CuDomPropDataIceDbconnection::CuDomPropDataIceDbconnection(const CuDomPropDataIceDbconnection & refPropDataIceDbconnection)
    :CuIpmPropertyData("CuDomPropDataIceDbconnection")
{
  m_csDBName  = refPropDataIceDbconnection.m_csDBName;
  m_csDBUser  = refPropDataIceDbconnection.m_csDBUser;
  m_csComment = refPropDataIceDbconnection.m_csComment;

  m_refreshParams = refPropDataIceDbconnection.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceDbconnection CuDomPropDataIceDbconnection::operator = (const CuDomPropDataIceDbconnection & refPropDataIceDbconnection)
{
  if (this == &refPropDataIceDbconnection)
    ASSERT (FALSE);

  m_csDBName  = refPropDataIceDbconnection.m_csDBName;
  m_csDBUser  = refPropDataIceDbconnection.m_csDBUser;
  m_csComment = refPropDataIceDbconnection.m_csComment;

  m_refreshParams = refPropDataIceDbconnection.m_refreshParams;

  return *this;
}

void CuDomPropDataIceDbconnection::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceDbconnection::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csDBName;
    ar << m_csDBUser;
    ar << m_csComment;
  }
  else {
    ar >> m_csDBName;
    ar >> m_csDBUser;
    ar >> m_csComment;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceRole, CuIpmPropertyData, 1)

CuDomPropDataIceRole::CuDomPropDataIceRole()
    :CuIpmPropertyData("CuDomPropDataIceRole")
{
  m_csComment = "";
}

// copy constructor
CuDomPropDataIceRole::CuDomPropDataIceRole(const CuDomPropDataIceRole & refPropDataIceRole)
    :CuIpmPropertyData("CuDomPropDataIceRole")
{
  m_csComment = refPropDataIceRole.m_csComment;

  m_refreshParams = refPropDataIceRole.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceRole CuDomPropDataIceRole::operator = (const CuDomPropDataIceRole & refPropDataIceRole)
{
  if (this == &refPropDataIceRole)
    ASSERT (FALSE);

  m_csComment = refPropDataIceRole.m_csComment;

  m_refreshParams = refPropDataIceRole.m_refreshParams;

  return *this;
}

void CuDomPropDataIceRole::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceRole::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csComment;
  }
  else {
    ar >> m_csComment;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceWebuser, CuIpmPropertyData, 2)

CuDomPropDataIceWebuser::CuDomPropDataIceWebuser()
    :CuIpmPropertyData("CuDomPropDataIceWebuser")
{
  m_csDefDBUser   = _T("");
  m_csComment     = _T("");
  m_bAdminPriv    = FALSE;
  m_bSecurityPriv = FALSE;
  m_bUnitMgrPriv  = FALSE;
  m_bMonitorPriv  = FALSE;
  m_ltimeoutms    = 0L;
  m_bRepositoryPsw = FALSE;
}

// copy constructor
CuDomPropDataIceWebuser::CuDomPropDataIceWebuser(const CuDomPropDataIceWebuser & refPropDataIceWebuser)
    :CuIpmPropertyData("CuDomPropDataIceWebuser")
{
  m_csDefDBUser   = refPropDataIceWebuser.m_csDefDBUser   ;
  m_csComment     = refPropDataIceWebuser.m_csComment     ;
  m_bAdminPriv    = refPropDataIceWebuser.m_bAdminPriv    ;
  m_bSecurityPriv = refPropDataIceWebuser.m_bSecurityPriv ;
  m_bUnitMgrPriv  = refPropDataIceWebuser.m_bUnitMgrPriv  ;
  m_bMonitorPriv  = refPropDataIceWebuser.m_bMonitorPriv  ;
  m_ltimeoutms    = refPropDataIceWebuser.m_ltimeoutms    ;
  m_bRepositoryPsw =refPropDataIceWebuser.m_bRepositoryPsw;

  m_refreshParams = refPropDataIceWebuser.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceWebuser CuDomPropDataIceWebuser::operator = (const CuDomPropDataIceWebuser & refPropDataIceWebuser)
{
  if (this == &refPropDataIceWebuser)
    ASSERT (FALSE);

  m_csDefDBUser   = refPropDataIceWebuser.m_csDefDBUser   ;
  m_csComment     = refPropDataIceWebuser.m_csComment     ;
  m_bAdminPriv    = refPropDataIceWebuser.m_bAdminPriv    ;
  m_bSecurityPriv = refPropDataIceWebuser.m_bSecurityPriv ;
  m_bUnitMgrPriv  = refPropDataIceWebuser.m_bUnitMgrPriv  ;
  m_bMonitorPriv  = refPropDataIceWebuser.m_bMonitorPriv  ;
  m_ltimeoutms    = refPropDataIceWebuser.m_ltimeoutms    ;
  m_bRepositoryPsw =refPropDataIceWebuser.m_bRepositoryPsw;

  m_refreshParams = refPropDataIceWebuser.m_refreshParams;

  return *this;
}

void CuDomPropDataIceWebuser::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceWebuser::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csDefDBUser   ;
    ar << m_csComment     ;
    ar << m_bAdminPriv    ;
    ar << m_bSecurityPriv ;
    ar << m_bUnitMgrPriv  ;
    ar << m_bMonitorPriv  ;
    ar << m_ltimeoutms    ;
    ar << m_bRepositoryPsw;
  }
  else {
    ar >> m_csDefDBUser   ;
    ar >> m_csComment     ;
    ar >> m_bAdminPriv    ;
    ar >> m_bSecurityPriv ;
    ar >> m_bUnitMgrPriv  ;
    ar >> m_bMonitorPriv  ;
    ar >> m_ltimeoutms    ;
    ar >> m_bRepositoryPsw;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceProfile, CuIpmPropertyData, 1)

CuDomPropDataIceProfile::CuDomPropDataIceProfile()
    :CuIpmPropertyData("CuDomPropDataIceProfile")
{
  m_csDefProfile  = _T("");
  m_bAdminPriv    = FALSE;
  m_bSecurityPriv = FALSE;
  m_bUnitMgrPriv  = FALSE;
  m_bMonitorPriv  = FALSE;
  m_ltimeoutms    = 0L;
}

// copy constructor
CuDomPropDataIceProfile::CuDomPropDataIceProfile(const CuDomPropDataIceProfile & refPropDataIceProfile)
    :CuIpmPropertyData("CuDomPropDataIceProfile")
{
  m_csDefProfile  = refPropDataIceProfile.m_csDefProfile  ;
  m_bAdminPriv    = refPropDataIceProfile.m_bAdminPriv    ;
  m_bSecurityPriv = refPropDataIceProfile.m_bSecurityPriv ;
  m_bUnitMgrPriv  = refPropDataIceProfile.m_bUnitMgrPriv  ;
  m_bMonitorPriv  = refPropDataIceProfile.m_bMonitorPriv  ;
  m_ltimeoutms    = refPropDataIceProfile.m_ltimeoutms    ;

  m_refreshParams = refPropDataIceProfile.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceProfile CuDomPropDataIceProfile::operator = (const CuDomPropDataIceProfile & refPropDataIceProfile)
{
  if (this == &refPropDataIceProfile)
    ASSERT (FALSE);

  m_csDefProfile  = refPropDataIceProfile.m_csDefProfile  ;
  m_bAdminPriv    = refPropDataIceProfile.m_bAdminPriv    ;
  m_bSecurityPriv = refPropDataIceProfile.m_bSecurityPriv ;
  m_bUnitMgrPriv  = refPropDataIceProfile.m_bUnitMgrPriv  ;
  m_bMonitorPriv  = refPropDataIceProfile.m_bMonitorPriv  ;
  m_ltimeoutms    = refPropDataIceProfile.m_ltimeoutms    ;

  m_refreshParams = refPropDataIceProfile.m_refreshParams;

  return *this;
}

void CuDomPropDataIceProfile::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceProfile::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csDefProfile  ;
    ar << m_bAdminPriv    ;
    ar << m_bSecurityPriv ;
    ar << m_bUnitMgrPriv  ;
    ar << m_bMonitorPriv  ;
    ar << m_ltimeoutms    ;
  }
  else {
    ar >> m_csDefProfile  ;
    ar >> m_bAdminPriv    ;
    ar >> m_bSecurityPriv ;
    ar >> m_bUnitMgrPriv  ;
    ar >> m_bMonitorPriv  ;
    ar >> m_ltimeoutms    ;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceLocation, CuIpmPropertyData, 1)

CuDomPropDataIceLocation::CuDomPropDataIceLocation()
    :CuIpmPropertyData("CuDomPropDataIceLocation")
{
  m_bIce          = FALSE;
  m_bPublic       = FALSE;
  m_csPath        = _T("");
  m_csExtensions  = _T("");
  m_csComment     = _T("");
}

// copy constructor
CuDomPropDataIceLocation::CuDomPropDataIceLocation(const CuDomPropDataIceLocation & refPropDataIceLocation)
    :CuIpmPropertyData("CuDomPropDataIceLocation")
{
  m_bIce         = refPropDataIceLocation.m_bIce         ;
  m_bPublic      = refPropDataIceLocation.m_bPublic      ;
  m_csPath       = refPropDataIceLocation.m_csPath       ;
  m_csExtensions = refPropDataIceLocation.m_csExtensions ;
  m_csComment    = refPropDataIceLocation.m_csComment    ;

  m_refreshParams = refPropDataIceLocation.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceLocation CuDomPropDataIceLocation::operator = (const CuDomPropDataIceLocation & refPropDataIceLocation)
{
  if (this == &refPropDataIceLocation)
    ASSERT (FALSE);

  m_bIce         = refPropDataIceLocation.m_bIce         ;
  m_bPublic      = refPropDataIceLocation.m_bPublic      ;
  m_csPath       = refPropDataIceLocation.m_csPath       ;
  m_csExtensions = refPropDataIceLocation.m_csExtensions ;
  m_csComment    = refPropDataIceLocation.m_csComment    ;

  m_refreshParams = refPropDataIceLocation.m_refreshParams;

  return *this;
}

void CuDomPropDataIceLocation::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceLocation::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_bIce        ;
    ar << m_bPublic     ;
    ar << m_csPath      ;
    ar << m_csExtensions;
    ar << m_csComment   ;
  }
  else {
    ar >> m_bIce        ;
    ar >> m_bPublic     ;
    ar >> m_csPath      ;
    ar >> m_csExtensions;
    ar >> m_csComment   ;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceVariable, CuIpmPropertyData, 1)

CuDomPropDataIceVariable::CuDomPropDataIceVariable()
    :CuIpmPropertyData("CuDomPropDataIceVariable")
{
  m_csVariableValue  = _T("");
}

// copy constructor
CuDomPropDataIceVariable::CuDomPropDataIceVariable(const CuDomPropDataIceVariable & refPropDataIceVariable)
    :CuIpmPropertyData("CuDomPropDataIceVariable")
{
  m_csVariableValue  = refPropDataIceVariable.m_csVariableValue  ;

  m_refreshParams = refPropDataIceVariable.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceVariable CuDomPropDataIceVariable::operator = (const CuDomPropDataIceVariable & refPropDataIceVariable)
{
  if (this == &refPropDataIceVariable)
    ASSERT (FALSE);

  m_csVariableValue  = refPropDataIceVariable.m_csVariableValue  ;

  m_refreshParams = refPropDataIceVariable.m_refreshParams;

  return *this;
}

void CuDomPropDataIceVariable::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceVariable::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csVariableValue  ;
  }
  else {
    ar >> m_csVariableValue  ;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceBunit, CuIpmPropertyData, 1)

CuDomPropDataIceBunit::CuDomPropDataIceBunit()
    :CuIpmPropertyData("CuDomPropDataIceBunit")
{
  m_csOwner  = _T("");
}

// copy constructor
CuDomPropDataIceBunit::CuDomPropDataIceBunit(const CuDomPropDataIceBunit & refPropDataIceBunit)
    :CuIpmPropertyData("CuDomPropDataIceBunit")
{
  m_csOwner  = refPropDataIceBunit.m_csOwner  ;

  m_refreshParams = refPropDataIceBunit.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceBunit CuDomPropDataIceBunit::operator = (const CuDomPropDataIceBunit & refPropDataIceBunit)
{
  if (this == &refPropDataIceBunit)
    ASSERT (FALSE);

  m_csOwner  = refPropDataIceBunit.m_csOwner  ;

  m_refreshParams = refPropDataIceBunit.m_refreshParams;

  return *this;
}

void CuDomPropDataIceBunit::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceBunit::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csOwner  ;
  }
  else {
    ar >> m_csOwner  ;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceSecRoles, CuIpmPropertyData, 1)

CuDomPropDataIceSecRoles::CuDomPropDataIceSecRoles()
    :CuIpmPropertyData("CuDomPropDataIceSecRoles")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceSecRoles::CuDomPropDataIceSecRoles(const CuDomPropDataIceSecRoles & refPropDataIceSecRoles)
    :CuIpmPropertyData("CuDomPropDataIceSecRoles")
{
  m_uaIceSecRoles.Copy(refPropDataIceSecRoles.m_uaIceSecRoles);
  m_objType = refPropDataIceSecRoles.m_objType;
}

// '=' operator overloading
CuDomPropDataIceSecRoles CuDomPropDataIceSecRoles::operator = (const CuDomPropDataIceSecRoles & refPropDataIceSecRoles)
{
  if (this == &refPropDataIceSecRoles)
    ASSERT (FALSE);

  m_uaIceSecRoles.Copy(refPropDataIceSecRoles.m_uaIceSecRoles);
  m_objType = refPropDataIceSecRoles.m_objType;

  return *this;
}

void CuDomPropDataIceSecRoles::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceSecRoles::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceSecRoles.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceSecDbusers, CuIpmPropertyData, 1)

CuDomPropDataIceSecDbusers::CuDomPropDataIceSecDbusers()
    :CuIpmPropertyData("CuDomPropDataIceSecDbusers")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceSecDbusers::CuDomPropDataIceSecDbusers(const CuDomPropDataIceSecDbusers & refPropDataIceSecDbusers)
    :CuIpmPropertyData("CuDomPropDataIceSecDbusers")
{
  m_uaIceSecDbusers.Copy(refPropDataIceSecDbusers.m_uaIceSecDbusers);
  m_objType = refPropDataIceSecDbusers.m_objType;
}

// '=' operator overloading
CuDomPropDataIceSecDbusers CuDomPropDataIceSecDbusers::operator = (const CuDomPropDataIceSecDbusers & refPropDataIceSecDbusers)
{
  if (this == &refPropDataIceSecDbusers)
    ASSERT (FALSE);

  m_uaIceSecDbusers.Copy(refPropDataIceSecDbusers.m_uaIceSecDbusers);
  m_objType = refPropDataIceSecDbusers.m_objType;

  return *this;
}

void CuDomPropDataIceSecDbusers::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceSecDbusers::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceSecDbusers.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceSecDbconns, CuIpmPropertyData, 1)

CuDomPropDataIceSecDbconns::CuDomPropDataIceSecDbconns()
    :CuIpmPropertyData("CuDomPropDataIceSecDbconns")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceSecDbconns::CuDomPropDataIceSecDbconns(const CuDomPropDataIceSecDbconns & refPropDataIceSecDbconns)
    :CuIpmPropertyData("CuDomPropDataIceSecDbconns")
{
  m_uaIceSecDbconns.Copy(refPropDataIceSecDbconns.m_uaIceSecDbconns);
  m_objType = refPropDataIceSecDbconns.m_objType;
}

// '=' operator overloading
CuDomPropDataIceSecDbconns CuDomPropDataIceSecDbconns::operator = (const CuDomPropDataIceSecDbconns & refPropDataIceSecDbconns)
{
  if (this == &refPropDataIceSecDbconns)
    ASSERT (FALSE);

  m_uaIceSecDbconns.Copy(refPropDataIceSecDbconns.m_uaIceSecDbconns);
  m_objType = refPropDataIceSecDbconns.m_objType;

  return *this;
}

void CuDomPropDataIceSecDbconns::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceSecDbconns::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceSecDbconns.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceSecWebusers, CuIpmPropertyData, 1)

CuDomPropDataIceSecWebusers::CuDomPropDataIceSecWebusers()
    :CuIpmPropertyData("CuDomPropDataIceSecWebusers")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceSecWebusers::CuDomPropDataIceSecWebusers(const CuDomPropDataIceSecWebusers & refPropDataIceSecWebusers)
    :CuIpmPropertyData("CuDomPropDataIceSecWebusers")
{
  m_uaIceSecWebusers.Copy(refPropDataIceSecWebusers.m_uaIceSecWebusers);
  m_objType = refPropDataIceSecWebusers.m_objType;
}

// '=' operator overloading
CuDomPropDataIceSecWebusers CuDomPropDataIceSecWebusers::operator = (const CuDomPropDataIceSecWebusers & refPropDataIceSecWebusers)
{
  if (this == &refPropDataIceSecWebusers)
    ASSERT (FALSE);

  m_uaIceSecWebusers.Copy(refPropDataIceSecWebusers.m_uaIceSecWebusers);
  m_objType = refPropDataIceSecWebusers.m_objType;

  return *this;
}

void CuDomPropDataIceSecWebusers::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceSecWebusers::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceSecWebusers.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceSecProfiles, CuIpmPropertyData, 1)

CuDomPropDataIceSecProfiles::CuDomPropDataIceSecProfiles()
    :CuIpmPropertyData("CuDomPropDataIceSecProfiles")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceSecProfiles::CuDomPropDataIceSecProfiles(const CuDomPropDataIceSecProfiles & refPropDataIceSecProfiles)
    :CuIpmPropertyData("CuDomPropDataIceSecProfiles")
{
  m_uaIceSecProfiles.Copy(refPropDataIceSecProfiles.m_uaIceSecProfiles);
  m_objType = refPropDataIceSecProfiles.m_objType;
}

// '=' operator overloading
CuDomPropDataIceSecProfiles CuDomPropDataIceSecProfiles::operator = (const CuDomPropDataIceSecProfiles & refPropDataIceSecProfiles)
{
  if (this == &refPropDataIceSecProfiles)
    ASSERT (FALSE);

  m_uaIceSecProfiles.Copy(refPropDataIceSecProfiles.m_uaIceSecProfiles);
  m_objType = refPropDataIceSecProfiles.m_objType;

  return *this;
}

void CuDomPropDataIceSecProfiles::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceSecProfiles::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceSecProfiles.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceWebuserRoles, CuIpmPropertyData, 1)

CuDomPropDataIceWebuserRoles::CuDomPropDataIceWebuserRoles()
    :CuIpmPropertyData("CuDomPropDataIceWebuserRoles")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceWebuserRoles::CuDomPropDataIceWebuserRoles(const CuDomPropDataIceWebuserRoles & refPropDataIceWebuserRoles)
    :CuIpmPropertyData("CuDomPropDataIceWebuserRoles")
{
  m_uaIceWebuserRoles.Copy(refPropDataIceWebuserRoles.m_uaIceWebuserRoles);
  m_objType = refPropDataIceWebuserRoles.m_objType;
}

// '=' operator overloading
CuDomPropDataIceWebuserRoles CuDomPropDataIceWebuserRoles::operator = (const CuDomPropDataIceWebuserRoles & refPropDataIceWebuserRoles)
{
  if (this == &refPropDataIceWebuserRoles)
    ASSERT (FALSE);

  m_uaIceWebuserRoles.Copy(refPropDataIceWebuserRoles.m_uaIceWebuserRoles);
  m_objType = refPropDataIceWebuserRoles.m_objType;

  return *this;
}

void CuDomPropDataIceWebuserRoles::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceWebuserRoles::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceWebuserRoles.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceWebuserDbconns, CuIpmPropertyData, 1)

CuDomPropDataIceWebuserDbconns::CuDomPropDataIceWebuserDbconns()
    :CuIpmPropertyData("CuDomPropDataIceWebuserDbconns")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceWebuserDbconns::CuDomPropDataIceWebuserDbconns(const CuDomPropDataIceWebuserDbconns & refPropDataIceWebuserDbconns)
    :CuIpmPropertyData("CuDomPropDataIceWebuserDbconns")
{
  m_uaIceWebuserDbconns.Copy(refPropDataIceWebuserDbconns.m_uaIceWebuserDbconns);
  m_objType = refPropDataIceWebuserDbconns.m_objType;
}

// '=' operator overloading
CuDomPropDataIceWebuserDbconns CuDomPropDataIceWebuserDbconns::operator = (const CuDomPropDataIceWebuserDbconns & refPropDataIceWebuserDbconns)
{
  if (this == &refPropDataIceWebuserDbconns)
    ASSERT (FALSE);

  m_uaIceWebuserDbconns.Copy(refPropDataIceWebuserDbconns.m_uaIceWebuserDbconns);
  m_objType = refPropDataIceWebuserDbconns.m_objType;

  return *this;
}

void CuDomPropDataIceWebuserDbconns::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceWebuserDbconns::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceWebuserDbconns.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceProfileRoles, CuIpmPropertyData, 1)

CuDomPropDataIceProfileRoles::CuDomPropDataIceProfileRoles()
    :CuIpmPropertyData("CuDomPropDataIceProfileRoles")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceProfileRoles::CuDomPropDataIceProfileRoles(const CuDomPropDataIceProfileRoles & refPropDataIceProfileRoles)
    :CuIpmPropertyData("CuDomPropDataIceProfileRoles")
{
  m_uaIceProfileRoles.Copy(refPropDataIceProfileRoles.m_uaIceProfileRoles);
  m_objType = refPropDataIceProfileRoles.m_objType;
}

// '=' operator overloading
CuDomPropDataIceProfileRoles CuDomPropDataIceProfileRoles::operator = (const CuDomPropDataIceProfileRoles & refPropDataIceProfileRoles)
{
  if (this == &refPropDataIceProfileRoles)
    ASSERT (FALSE);

  m_uaIceProfileRoles.Copy(refPropDataIceProfileRoles.m_uaIceProfileRoles);
  m_objType = refPropDataIceProfileRoles.m_objType;

  return *this;
}

void CuDomPropDataIceProfileRoles::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceProfileRoles::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceProfileRoles.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceProfileDbconns, CuIpmPropertyData, 1)

CuDomPropDataIceProfileDbconns::CuDomPropDataIceProfileDbconns()
    :CuIpmPropertyData("CuDomPropDataIceProfileDbconns")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceProfileDbconns::CuDomPropDataIceProfileDbconns(const CuDomPropDataIceProfileDbconns & refPropDataIceProfileDbconns)
    :CuIpmPropertyData("CuDomPropDataIceProfileDbconns")
{
  m_uaIceProfileDbconns.Copy(refPropDataIceProfileDbconns.m_uaIceProfileDbconns);
  m_objType = refPropDataIceProfileDbconns.m_objType;
}

// '=' operator overloading
CuDomPropDataIceProfileDbconns CuDomPropDataIceProfileDbconns::operator = (const CuDomPropDataIceProfileDbconns & refPropDataIceProfileDbconns)
{
  if (this == &refPropDataIceProfileDbconns)
    ASSERT (FALSE);

  m_uaIceProfileDbconns.Copy(refPropDataIceProfileDbconns.m_uaIceProfileDbconns);
  m_objType = refPropDataIceProfileDbconns.m_objType;

  return *this;
}

void CuDomPropDataIceProfileDbconns::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceProfileDbconns::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceProfileDbconns.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceServerApplications, CuIpmPropertyData, 1)

CuDomPropDataIceServerApplications::CuDomPropDataIceServerApplications()
    :CuIpmPropertyData("CuDomPropDataIceServerApplications")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceServerApplications::CuDomPropDataIceServerApplications(const CuDomPropDataIceServerApplications & refPropDataIceServerApplications)
    :CuIpmPropertyData("CuDomPropDataIceServerApplications")
{
  m_uaIceServerApplications.Copy(refPropDataIceServerApplications.m_uaIceServerApplications);
  m_objType = refPropDataIceServerApplications.m_objType;
}

// '=' operator overloading
CuDomPropDataIceServerApplications CuDomPropDataIceServerApplications::operator = (const CuDomPropDataIceServerApplications & refPropDataIceServerApplications)
{
  if (this == &refPropDataIceServerApplications)
    ASSERT (FALSE);

  m_uaIceServerApplications.Copy(refPropDataIceServerApplications.m_uaIceServerApplications);
  m_objType = refPropDataIceServerApplications.m_objType;

  return *this;
}

void CuDomPropDataIceServerApplications::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceServerApplications::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceServerApplications.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceServerLocations, CuIpmPropertyData, 1)

CuDomPropDataIceServerLocations::CuDomPropDataIceServerLocations()
    :CuIpmPropertyData("CuDomPropDataIceServerLocations")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceServerLocations::CuDomPropDataIceServerLocations(const CuDomPropDataIceServerLocations & refPropDataIceServerLocations)
    :CuIpmPropertyData("CuDomPropDataIceServerLocations")
{
  m_uaIceServerLocations.Copy(refPropDataIceServerLocations.m_uaIceServerLocations);
  m_objType = refPropDataIceServerLocations.m_objType;
}

// '=' operator overloading
CuDomPropDataIceServerLocations CuDomPropDataIceServerLocations::operator = (const CuDomPropDataIceServerLocations & refPropDataIceServerLocations)
{
  if (this == &refPropDataIceServerLocations)
    ASSERT (FALSE);

  m_uaIceServerLocations.Copy(refPropDataIceServerLocations.m_uaIceServerLocations);
  m_objType = refPropDataIceServerLocations.m_objType;

  return *this;
}

void CuDomPropDataIceServerLocations::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceServerLocations::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceServerLocations.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceServerVariables, CuIpmPropertyData, 1)

CuDomPropDataIceServerVariables::CuDomPropDataIceServerVariables()
    :CuIpmPropertyData("CuDomPropDataIceServerVariables")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceServerVariables::CuDomPropDataIceServerVariables(const CuDomPropDataIceServerVariables & refPropDataIceServerVariables)
    :CuIpmPropertyData("CuDomPropDataIceServerVariables")
{
  m_uaIceServerVariables.Copy(refPropDataIceServerVariables.m_uaIceServerVariables);
  m_objType = refPropDataIceServerVariables.m_objType;
}

// '=' operator overloading
CuDomPropDataIceServerVariables CuDomPropDataIceServerVariables::operator = (const CuDomPropDataIceServerVariables & refPropDataIceServerVariables)
{
  if (this == &refPropDataIceServerVariables)
    ASSERT (FALSE);

  m_uaIceServerVariables.Copy(refPropDataIceServerVariables.m_uaIceServerVariables);
  m_objType = refPropDataIceServerVariables.m_objType;

  return *this;
}

void CuDomPropDataIceServerVariables::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceServerVariables::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceServerVariables.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceFacetAndPage, CuIpmPropertyData, 1)

CuDomPropDataIceFacetAndPage::CuDomPropDataIceFacetAndPage()
    :CuIpmPropertyData("CuDomPropDataIceFacetAndPage")
{
  m_objType  = 0;

  m_csNamePlusExt = _T("");
  m_bPublic = FALSE;
  m_bStorageInsideRepository;
  m_bPreCache = FALSE;
  m_bPermCache = FALSE;
  m_bSessionCache = FALSE;
  m_csPath = _T("");
  m_csLocation = _T("");
  m_csFilename = _T("");
  m_csTempFile = _T(""); 

}

// copy constructor
CuDomPropDataIceFacetAndPage::CuDomPropDataIceFacetAndPage(const CuDomPropDataIceFacetAndPage & refPropDataIceFacetAndPage)
    :CuIpmPropertyData("CuDomPropDataIceFacetAndPage")
{
  m_objType  = refPropDataIceFacetAndPage.m_objType  ;

  m_csNamePlusExt             = refPropDataIceFacetAndPage.m_csNamePlusExt;
  m_bPublic                   = refPropDataIceFacetAndPage.m_bPublic;
  m_bStorageInsideRepository  = refPropDataIceFacetAndPage.m_bStorageInsideRepository;
  m_bPreCache                 = refPropDataIceFacetAndPage.m_bPreCache;
  m_bPermCache                = refPropDataIceFacetAndPage.m_bPermCache;
  m_bSessionCache             = refPropDataIceFacetAndPage.m_bSessionCache;
  m_csPath                    = refPropDataIceFacetAndPage.m_csPath;
  m_csLocation                = refPropDataIceFacetAndPage.m_csLocation;
  m_csFilename                = refPropDataIceFacetAndPage.m_csFilename;

  m_refreshParams = refPropDataIceFacetAndPage.m_refreshParams;

  m_csTempFile = _T("");

  // copy tempfile if any
  	if (refPropDataIceFacetAndPage.m_csTempFile.Compare(_T(""))!=0) {
		char buffer[_MAX_PATH];
		int iret=ESL_GetTempFileName((LPUCHAR) buffer, sizeof(buffer));
		if (iret==RES_SUCCESS) {
			BOOL bRes = CopyFile( (LPCTSTR) refPropDataIceFacetAndPage.m_csTempFile, 
								  (LPCTSTR) buffer,
								  FALSE);
			if (bRes) 
				m_csTempFile = (LPCTSTR) buffer;
		}
	}

}
void CuDomPropDataIceFacetAndPage::ClearTempFileIfAny()
{
	if (m_csTempFile.Compare(_T(""))!=0) {
		DeleteFile((LPCTSTR)m_csTempFile);
		m_csTempFile = _T("");
	}
}
// '=' operator overloading
CuDomPropDataIceFacetAndPage CuDomPropDataIceFacetAndPage::operator = (const CuDomPropDataIceFacetAndPage & refPropDataIceFacetAndPage)
{
  if (this == &refPropDataIceFacetAndPage)
    ASSERT (FALSE);

  m_objType  = refPropDataIceFacetAndPage.m_objType  ;


  m_csNamePlusExt             = refPropDataIceFacetAndPage.m_csNamePlusExt;
  m_bPublic                   = refPropDataIceFacetAndPage.m_bPublic;
  m_bStorageInsideRepository  = refPropDataIceFacetAndPage.m_bStorageInsideRepository;
  m_bPreCache                 = refPropDataIceFacetAndPage.m_bPreCache;
  m_bPermCache                = refPropDataIceFacetAndPage.m_bPermCache;
  m_bSessionCache             = refPropDataIceFacetAndPage.m_bSessionCache;
  m_csPath                    = refPropDataIceFacetAndPage.m_csPath;
  m_csLocation                = refPropDataIceFacetAndPage.m_csLocation;
  m_csFilename                = refPropDataIceFacetAndPage.m_csFilename;

  m_refreshParams = refPropDataIceFacetAndPage.m_refreshParams;

  m_csTempFile = _T("");

  // copy tempfile if any
  	if (refPropDataIceFacetAndPage.m_csTempFile.Compare(_T(""))!=0) {
		char buffer[_MAX_PATH];
		int iret=ESL_GetTempFileName((LPUCHAR) buffer, sizeof(buffer));
		if (iret==RES_SUCCESS) {
			BOOL bRes = CopyFile( (LPCTSTR) refPropDataIceFacetAndPage.m_csTempFile, 
								  (LPCTSTR) buffer,
								  FALSE);
			if (bRes) 
				m_csTempFile = (LPCTSTR) buffer;
		}
	}
  return *this;
}

void CuDomPropDataIceFacetAndPage::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceFacetAndPage::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType  ;
    ar << m_csNamePlusExt;
    ar << m_bPublic;
    ar << m_bStorageInsideRepository;
    ar << m_bPreCache;
    ar << m_bPermCache;
    ar << m_bSessionCache;
    ar << m_csPath;
    ar << m_csLocation;
    ar << m_csFilename;
	if (m_csTempFile.Compare(_T(""))!=0) {
		char buf[MAXICEDOCSIZE4SAVE];
		int len;
		int iret = ESL_FillBufferFromFile((LPUCHAR)(LPCTSTR)m_csTempFile,
										  (LPUCHAR)buf,sizeof(buf), &len, FALSE);
		if (iret == RES_SUCCESS && len < MAXICEDOCSIZE4SAVE) {
			ar << len;
			/*ar.Write(buf, len);*/
			for (int ii=0;ii<len;ii++) { /* workaround to fix bug 101492 */
				int jj = (int) buf[ii];
				ar <<jj;
			}
		}
		else {
			AfxMessageBox(IDS_CANNOT_SAVE_BIG_FACETPAGE);
			ar << (-1);
		}
	}
	else 
		ar << 0;
  }
  else {
	  int itempfilelen;
    ar >> m_objType  ;
    ar >> m_csNamePlusExt;
    ar >> m_bPublic;
    ar >> m_bStorageInsideRepository;
    ar >> m_bPreCache;
    ar >> m_bPermCache;
    ar >> m_bSessionCache;
    ar >> m_csPath;
    ar >> m_csLocation;
    ar >> m_csFilename;

	m_csTempFile = _T("");

	ar >> itempfilelen;
	if (itempfilelen == (-1)) 
		AfxMessageBox(IDS_CANNOT_LOAD_BIG_FACET_OR_PAGE);

	if (itempfilelen > 0) {
		char buf[MAXICEDOCSIZE4SAVE];
		char FileNamebuffer[_MAX_PATH];

		/* ar.Read(buf,itempfilelen);*/
		for (int ii=0;ii<itempfilelen;ii++) { /* workaround to fix bug 101492 */
			int jj;
			ar >>jj;
			buf[ii] = (char)jj;
		}

		int iret=ESL_GetTempFileName((LPUCHAR) FileNamebuffer, sizeof(FileNamebuffer));
		if (iret==RES_SUCCESS) {
			iret = ESL_FillNewFileFromBuffer((LPUCHAR)FileNamebuffer,(LPUCHAR) buf, itempfilelen, FALSE);
			if (iret ==RES_SUCCESS)
				m_csTempFile = (LPCTSTR) FileNamebuffer;
		}
	}
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceAccessDef, CuIpmPropertyData, 1)

CuDomPropDataIceAccessDef::CuDomPropDataIceAccessDef()
    :CuIpmPropertyData("CuDomPropDataIceAccessDef")
{
  m_objType  = 0;

  m_bExecute = 0;
  m_bRead    = 0;
  m_bUpdate  = 0;
  m_bDelete  = 0;
}

// copy constructor
CuDomPropDataIceAccessDef::CuDomPropDataIceAccessDef(const CuDomPropDataIceAccessDef & refPropDataIceAccessDef)
    :CuIpmPropertyData("CuDomPropDataIceAccessDef")
{
  m_objType  = refPropDataIceAccessDef.m_objType  ;

  m_bExecute = refPropDataIceAccessDef.m_bExecute;
  m_bRead    = refPropDataIceAccessDef.m_bRead   ;
  m_bUpdate  = refPropDataIceAccessDef.m_bUpdate ;
  m_bDelete  = refPropDataIceAccessDef.m_bDelete ;

  m_refreshParams = refPropDataIceAccessDef.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceAccessDef CuDomPropDataIceAccessDef::operator = (const CuDomPropDataIceAccessDef & refPropDataIceAccessDef)
{
  if (this == &refPropDataIceAccessDef)
    ASSERT (FALSE);

  m_objType  = refPropDataIceAccessDef.m_objType  ;

  m_bExecute = refPropDataIceAccessDef.m_bExecute;
  m_bRead    = refPropDataIceAccessDef.m_bRead   ;
  m_bUpdate  = refPropDataIceAccessDef.m_bUpdate ;
  m_bDelete  = refPropDataIceAccessDef.m_bDelete ;

  m_refreshParams = refPropDataIceAccessDef.m_refreshParams;

  return *this;
}

void CuDomPropDataIceAccessDef::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceAccessDef::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_bExecute;
    ar << m_bRead   ;
    ar << m_bUpdate ;
    ar << m_bDelete ;
  }
  else {
    ar >> m_bExecute;
    ar >> m_bRead   ;
    ar >> m_bUpdate ;
    ar >> m_bDelete ;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceBunits, CuIpmPropertyData, 1)

CuDomPropDataIceBunits::CuDomPropDataIceBunits()
    :CuIpmPropertyData("CuDomPropDataIceBunits")
{
  // nothing to do for CStringArrays
}

// copy constructor
CuDomPropDataIceBunits::CuDomPropDataIceBunits(const CuDomPropDataIceBunits & refPropDataIceBunits)
    :CuIpmPropertyData("CuDomPropDataIceBunits")
{
  m_uaIceBunits.Copy(refPropDataIceBunits.m_uaIceBunits);
}

// '=' operator overloading
CuDomPropDataIceBunits CuDomPropDataIceBunits::operator = (const CuDomPropDataIceBunits & refPropDataIceBunits)
{
  if (this == &refPropDataIceBunits)
    ASSERT (FALSE);

  m_uaIceBunits.Copy(refPropDataIceBunits.m_uaIceBunits);

  return *this;
}

void CuDomPropDataIceBunits::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceBunits::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }

  m_uaIceBunits.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceSecDbuser, CuIpmPropertyData, 1)

CuDomPropDataIceSecDbuser::CuDomPropDataIceSecDbuser()
    :CuIpmPropertyData("CuDomPropDataIceSecDbuser")
{
  m_bUnitRead  = 0;
  m_bCreateDoc = 0;
  m_bReadDoc   = 0;
}

// copy constructor
CuDomPropDataIceSecDbuser::CuDomPropDataIceSecDbuser(const CuDomPropDataIceSecDbuser & refPropDataIceSecDbuser)
    :CuIpmPropertyData("CuDomPropDataIceSecDbuser")
{
  m_bUnitRead  = refPropDataIceSecDbuser.m_bUnitRead  ;
  m_bCreateDoc = refPropDataIceSecDbuser.m_bCreateDoc ;
  m_bReadDoc   = refPropDataIceSecDbuser.m_bReadDoc   ;

  m_refreshParams = refPropDataIceSecDbuser.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceSecDbuser CuDomPropDataIceSecDbuser::operator = (const CuDomPropDataIceSecDbuser & refPropDataIceSecDbuser)
{
  if (this == &refPropDataIceSecDbuser)
    ASSERT (FALSE);

  m_bUnitRead  = refPropDataIceSecDbuser.m_bUnitRead  ;
  m_bCreateDoc = refPropDataIceSecDbuser.m_bCreateDoc ;
  m_bReadDoc   = refPropDataIceSecDbuser.m_bReadDoc   ;

  m_refreshParams = refPropDataIceSecDbuser.m_refreshParams;

  return *this;
}

void CuDomPropDataIceSecDbuser::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceSecDbuser::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_bUnitRead ;
    ar << m_bCreateDoc;
    ar << m_bReadDoc  ;
  }
  else {
    ar >> m_bUnitRead ;
    ar >> m_bCreateDoc;
    ar >> m_bReadDoc  ;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceSecRole, CuIpmPropertyData, 1)

CuDomPropDataIceSecRole::CuDomPropDataIceSecRole()
    :CuIpmPropertyData("CuDomPropDataIceSecRole")
{
  m_bExecDoc  = 0;
  m_bCreateDoc = 0;
  m_bReadDoc   = 0;
}

// copy constructor
CuDomPropDataIceSecRole::CuDomPropDataIceSecRole(const CuDomPropDataIceSecRole & refPropDataIceSecRole)
    :CuIpmPropertyData("CuDomPropDataIceSecRole")
{
  m_bExecDoc  = refPropDataIceSecRole.m_bExecDoc  ;
  m_bCreateDoc = refPropDataIceSecRole.m_bCreateDoc ;
  m_bReadDoc   = refPropDataIceSecRole.m_bReadDoc   ;

  m_refreshParams = refPropDataIceSecRole.m_refreshParams;
}

// '=' operator overloading
CuDomPropDataIceSecRole CuDomPropDataIceSecRole::operator = (const CuDomPropDataIceSecRole & refPropDataIceSecRole)
{
  if (this == &refPropDataIceSecRole)
    ASSERT (FALSE);

  m_bExecDoc  = refPropDataIceSecRole.m_bExecDoc  ;
  m_bCreateDoc = refPropDataIceSecRole.m_bCreateDoc ;
  m_bReadDoc   = refPropDataIceSecRole.m_bReadDoc   ;

  m_refreshParams = refPropDataIceSecRole.m_refreshParams;

  return *this;
}

void CuDomPropDataIceSecRole::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceSecRole::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_bExecDoc ;
    ar << m_bCreateDoc;
    ar << m_bReadDoc  ;
  }
  else {
    ar >> m_bExecDoc ;
    ar >> m_bCreateDoc;
    ar >> m_bReadDoc  ;
  }
  m_refreshParams.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceFacets, CuIpmPropertyData, 1)

CuDomPropDataIceFacets::CuDomPropDataIceFacets()
    :CuIpmPropertyData("CuDomPropDataIceFacets")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceFacets::CuDomPropDataIceFacets(const CuDomPropDataIceFacets & refPropDataIceFacets)
    :CuIpmPropertyData("CuDomPropDataIceFacets")
{
  m_uaIceFacets.Copy(refPropDataIceFacets.m_uaIceFacets);
  m_objType = refPropDataIceFacets.m_objType;
}

// '=' operator overloading
CuDomPropDataIceFacets CuDomPropDataIceFacets::operator = (const CuDomPropDataIceFacets & refPropDataIceFacets)
{
  if (this == &refPropDataIceFacets)
    ASSERT (FALSE);

  m_uaIceFacets.Copy(refPropDataIceFacets.m_uaIceFacets);
  m_objType = refPropDataIceFacets.m_objType;

  return *this;
}

void CuDomPropDataIceFacets::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceFacets::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceFacets.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIcePages, CuIpmPropertyData, 1)

CuDomPropDataIcePages::CuDomPropDataIcePages()
    :CuIpmPropertyData("CuDomPropDataIcePages")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIcePages::CuDomPropDataIcePages(const CuDomPropDataIcePages & refPropDataIcePages)
    :CuIpmPropertyData("CuDomPropDataIcePages")
{
  m_uaIcePages.Copy(refPropDataIcePages.m_uaIcePages);
  m_objType = refPropDataIcePages.m_objType;
}

// '=' operator overloading
CuDomPropDataIcePages CuDomPropDataIcePages::operator = (const CuDomPropDataIcePages & refPropDataIcePages)
{
  if (this == &refPropDataIcePages)
    ASSERT (FALSE);

  m_uaIcePages.Copy(refPropDataIcePages.m_uaIcePages);
  m_objType = refPropDataIcePages.m_objType;

  return *this;
}

void CuDomPropDataIcePages::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIcePages::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIcePages.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceLocations, CuIpmPropertyData, 1)

CuDomPropDataIceLocations::CuDomPropDataIceLocations()
    :CuIpmPropertyData("CuDomPropDataIceLocations")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceLocations::CuDomPropDataIceLocations(const CuDomPropDataIceLocations & refPropDataIceLocations)
    :CuIpmPropertyData("CuDomPropDataIceLocations")
{
  m_uaIceLocations.Copy(refPropDataIceLocations.m_uaIceLocations);
  m_objType = refPropDataIceLocations.m_objType;
}

// '=' operator overloading
CuDomPropDataIceLocations CuDomPropDataIceLocations::operator = (const CuDomPropDataIceLocations & refPropDataIceLocations)
{
  if (this == &refPropDataIceLocations)
    ASSERT (FALSE);

  m_uaIceLocations.Copy(refPropDataIceLocations.m_uaIceLocations);
  m_objType = refPropDataIceLocations.m_objType;

  return *this;
}

void CuDomPropDataIceLocations::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceLocations::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceLocations.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceBunitRoles, CuIpmPropertyData, 1)

CuDomPropDataIceBunitRoles::CuDomPropDataIceBunitRoles()
    :CuIpmPropertyData("CuDomPropDataIceBunitRoles")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceBunitRoles::CuDomPropDataIceBunitRoles(const CuDomPropDataIceBunitRoles & refPropDataIceBunitRoles)
    :CuIpmPropertyData("CuDomPropDataIceBunitRoles")
{
  m_uaIceBunitRoles.Copy(refPropDataIceBunitRoles.m_uaIceBunitRoles);
  m_objType = refPropDataIceBunitRoles.m_objType;
}

// '=' operator overloading
CuDomPropDataIceBunitRoles CuDomPropDataIceBunitRoles::operator = (const CuDomPropDataIceBunitRoles & refPropDataIceBunitRoles)
{
  if (this == &refPropDataIceBunitRoles)
    ASSERT (FALSE);

  m_uaIceBunitRoles.Copy(refPropDataIceBunitRoles.m_uaIceBunitRoles);
  m_objType = refPropDataIceBunitRoles.m_objType;

  return *this;
}

void CuDomPropDataIceBunitRoles::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceBunitRoles::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceBunitRoles.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceBunitUsers, CuIpmPropertyData, 1)

CuDomPropDataIceBunitUsers::CuDomPropDataIceBunitUsers()
    :CuIpmPropertyData("CuDomPropDataIceBunitUsers")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceBunitUsers::CuDomPropDataIceBunitUsers(const CuDomPropDataIceBunitUsers & refPropDataIceBunitUsers)
    :CuIpmPropertyData("CuDomPropDataIceBunitUsers")
{
  m_uaIceBunitUsers.Copy(refPropDataIceBunitUsers.m_uaIceBunitUsers);
  m_objType = refPropDataIceBunitUsers.m_objType;
}

// '=' operator overloading
CuDomPropDataIceBunitUsers CuDomPropDataIceBunitUsers::operator = (const CuDomPropDataIceBunitUsers & refPropDataIceBunitUsers)
{
  if (this == &refPropDataIceBunitUsers)
    ASSERT (FALSE);

  m_uaIceBunitUsers.Copy(refPropDataIceBunitUsers.m_uaIceBunitUsers);
  m_objType = refPropDataIceBunitUsers.m_objType;

  return *this;
}

void CuDomPropDataIceBunitUsers::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceBunitUsers::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceBunitUsers.Serialize(ar);
}

//
// ICE
//
IMPLEMENT_SERIAL (CuDomPropDataIceFacetNPageAccessdef, CuIpmPropertyData, 1)

CuDomPropDataIceFacetNPageAccessdef::CuDomPropDataIceFacetNPageAccessdef()
    :CuIpmPropertyData("CuDomPropDataIceFacetNPageAccessdef")
{
  // nothing to do for CStringArrays
  m_objType = 0;
}

// copy constructor
CuDomPropDataIceFacetNPageAccessdef::CuDomPropDataIceFacetNPageAccessdef(const CuDomPropDataIceFacetNPageAccessdef & refPropDataIceFacetNPageAccessdef)
    :CuIpmPropertyData("CuDomPropDataIceFacetNPageAccessdef")
{
  m_uaIceFacetNPageAccessdef.Copy(refPropDataIceFacetNPageAccessdef.m_uaIceFacetNPageAccessdef);
  m_objType = refPropDataIceFacetNPageAccessdef.m_objType;
}

// '=' operator overloading
CuDomPropDataIceFacetNPageAccessdef CuDomPropDataIceFacetNPageAccessdef::operator = (const CuDomPropDataIceFacetNPageAccessdef & refPropDataIceFacetNPageAccessdef)
{
  if (this == &refPropDataIceFacetNPageAccessdef)
    ASSERT (FALSE);

  m_uaIceFacetNPageAccessdef.Copy(refPropDataIceFacetNPageAccessdef.m_uaIceFacetNPageAccessdef);
  m_objType = refPropDataIceFacetNPageAccessdef.m_objType;

  return *this;
}

void CuDomPropDataIceFacetNPageAccessdef::NotifyLoad(CWnd* pDlg)
{
  pDlg->SendMessage (WM_USER_IPMPAGE_LOADING, (WPARAM)(LPCTSTR)m_strClassName, (LPARAM)this);
}

void CuDomPropDataIceFacetNPageAccessdef::Serialize (CArchive& ar)
{
  CuIpmPropertyData::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_objType;
  }
  else {
    ar >> m_objType;
  }

  m_uaIceFacetNPageAccessdef.Serialize(ar);
}

