/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : icegrs.cpp                                                            //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    classes for ice grants management in right pane of dom                           //
//                                                                                     //
****************************************************************************************/

#include "stdafx.h"

#include "icegrs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// CuIceBunitRoleGrant

IMPLEMENT_SERIAL (CuIceBunitRoleGrant, CuMultFlag, 1)

CuIceBunitRoleGrant::CuIceBunitRoleGrant()
: CuMultFlag()
{
  InitialAllocateFlags(NB_ICE_BUNIT_ROLE_COLUMNS);
}

CuIceBunitRoleGrant::CuIceBunitRoleGrant(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NB_ICE_BUNIT_ROLE_COLUMNS);
}

CuIceBunitRoleGrant::CuIceBunitRoleGrant(LPCTSTR name, BOOL bExecDoc, BOOL bCreateDoc, BOOL bReadDoc)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NB_ICE_BUNIT_ROLE_COLUMNS);

  // Immediate update of several flags
  if (bExecDoc)
    m_aFlags[IndexFromFlagType(ICE_GRANT_TYPE_ROLE_EXECDOC)] = TRUE;
  if (bCreateDoc)
    m_aFlags[IndexFromFlagType(ICE_GRANT_TYPE_ROLE_CREATEDOC)] = TRUE;
  if (bReadDoc)
    m_aFlags[IndexFromFlagType(ICE_GRANT_TYPE_ROLE_READDOC)] = TRUE;
}

CuIceBunitRoleGrant::CuIceBunitRoleGrant(const CuIceBunitRoleGrant* pRefIceBunitRoleGrant)
 :CuMultFlag(pRefIceBunitRoleGrant)
{
  // Flags array already duplicated by base class
}

void CuIceBunitRoleGrant::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuIceBunitRoleGrant::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case ICE_GRANT_TYPE_ROLE_EXECDOC:
      return 0;
    case ICE_GRANT_TYPE_ROLE_CREATEDOC:
      return 1;
    case ICE_GRANT_TYPE_ROLE_READDOC:
      return 2;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayIceBunitRoleGrants

IMPLEMENT_SERIAL (CuArrayIceBunitRoleGrants, CuArrayMultFlags, 1)

void CuArrayIceBunitRoleGrants::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuIceBunitRoleGrant* CuArrayIceBunitRoleGrants::operator [] (int index)
{
  CuIceBunitRoleGrant* pIceBunitRoleGrant;
  pIceBunitRoleGrant = (CuIceBunitRoleGrant*)m_aFlagObjects[index];
  ASSERT (pIceBunitRoleGrant);
  ASSERT (pIceBunitRoleGrant->IsKindOf(RUNTIME_CLASS(CuIceBunitRoleGrant)));
  return pIceBunitRoleGrant;
}

void CuArrayIceBunitRoleGrants::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuIceBunitRoleGrant)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayIceBunitRoleGrants::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuIceBunitRoleGrant)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuIceBunitUserGrant

IMPLEMENT_SERIAL (CuIceBunitUserGrant, CuMultFlag, 1)

CuIceBunitUserGrant::CuIceBunitUserGrant()
: CuMultFlag()
{
  InitialAllocateFlags(NB_ICE_BUNIT_USER_COLUMNS);
}

CuIceBunitUserGrant::CuIceBunitUserGrant(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NB_ICE_BUNIT_USER_COLUMNS);
}

CuIceBunitUserGrant::CuIceBunitUserGrant(LPCTSTR name, BOOL bExecDoc, BOOL bCreateDoc, BOOL bReadDoc)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NB_ICE_BUNIT_USER_COLUMNS);

  // Immediate update of several flags
  if (bExecDoc)
    m_aFlags[IndexFromFlagType(ICE_GRANT_TYPE_USER_UNITREAD)] = TRUE;
  if (bCreateDoc)
    m_aFlags[IndexFromFlagType(ICE_GRANT_TYPE_USER_CREATEDOC)] = TRUE;
  if (bReadDoc)
    m_aFlags[IndexFromFlagType(ICE_GRANT_TYPE_USER_READDOC)] = TRUE;
}

CuIceBunitUserGrant::CuIceBunitUserGrant(const CuIceBunitUserGrant* pRefIceBunitUserGrant)
 :CuMultFlag(pRefIceBunitUserGrant)
{
  // Flags array already duplicated by base class
}

void CuIceBunitUserGrant::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuIceBunitUserGrant::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case ICE_GRANT_TYPE_USER_UNITREAD:
      return 0;
    case ICE_GRANT_TYPE_USER_CREATEDOC:
      return 1;
    case ICE_GRANT_TYPE_USER_READDOC:
      return 2;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayIceBunitUserGrants

IMPLEMENT_SERIAL (CuArrayIceBunitUserGrants, CuArrayMultFlags, 1)

void CuArrayIceBunitUserGrants::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuIceBunitUserGrant* CuArrayIceBunitUserGrants::operator [] (int index)
{
  CuIceBunitUserGrant* pIceBunitUserGrant;
  pIceBunitUserGrant = (CuIceBunitUserGrant*)m_aFlagObjects[index];
  ASSERT (pIceBunitUserGrant);
  ASSERT (pIceBunitUserGrant->IsKindOf(RUNTIME_CLASS(CuIceBunitUserGrant)));
  return pIceBunitUserGrant;
}

void CuArrayIceBunitUserGrants::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuIceBunitUserGrant)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayIceBunitUserGrants::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuIceBunitUserGrant)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuIceFacetNPageAccessdef

IMPLEMENT_SERIAL (CuIceFacetNPageAccessdef, CuMultFlag, 1)

CuIceFacetNPageAccessdef::CuIceFacetNPageAccessdef()
: CuMultFlag()
{
  InitialAllocateFlags(NB_ICE_FACETNPAGE_ACCESSDEF_COLUMNS);
}

CuIceFacetNPageAccessdef::CuIceFacetNPageAccessdef(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NB_ICE_FACETNPAGE_ACCESSDEF_COLUMNS);
}

CuIceFacetNPageAccessdef::CuIceFacetNPageAccessdef(LPCTSTR name, BOOL bExecute, BOOL bRead, BOOL bUpdate, BOOL bDelete)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NB_ICE_FACETNPAGE_ACCESSDEF_COLUMNS);

  // Immediate update of several flags
  if (bExecute)
    m_aFlags[IndexFromFlagType(ICE_GRANT_FACETNPAGE_ACCESSDEF_EXECUTE)] = TRUE;
  if (bRead)
    m_aFlags[IndexFromFlagType(ICE_GRANT_FACETNPAGE_ACCESSDEF_READ)] = TRUE;
  if (bUpdate)
    m_aFlags[IndexFromFlagType(ICE_GRANT_FACETNPAGE_ACCESSDEF_UPDATE)] = TRUE;
  if (bDelete)
    m_aFlags[IndexFromFlagType(ICE_GRANT_FACETNPAGE_ACCESSDEF_DELETE)] = TRUE;
}

CuIceFacetNPageAccessdef::CuIceFacetNPageAccessdef(const CuIceFacetNPageAccessdef* pRefIceFacetNPageAccessdef)
 :CuMultFlag(pRefIceFacetNPageAccessdef)
{
  // Flags array already duplicated by base class
}

void CuIceFacetNPageAccessdef::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuIceFacetNPageAccessdef::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case ICE_GRANT_FACETNPAGE_ACCESSDEF_EXECUTE:
      return 0;
    case ICE_GRANT_FACETNPAGE_ACCESSDEF_READ   :
      return 1;
    case ICE_GRANT_FACETNPAGE_ACCESSDEF_UPDATE :
      return 2;
    case ICE_GRANT_FACETNPAGE_ACCESSDEF_DELETE :
      return 3;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayIceFacetNPageAccessdefs

IMPLEMENT_SERIAL (CuArrayIceFacetNPageAccessdefs, CuArrayMultFlags, 1)

void CuArrayIceFacetNPageAccessdefs::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuIceFacetNPageAccessdef* CuArrayIceFacetNPageAccessdefs::operator [] (int index)
{
  CuIceFacetNPageAccessdef* pIceFacetNPageAccessdef;
  pIceFacetNPageAccessdef = (CuIceFacetNPageAccessdef*)m_aFlagObjects[index];
  ASSERT (pIceFacetNPageAccessdef);
  ASSERT (pIceFacetNPageAccessdef->IsKindOf(RUNTIME_CLASS(CuIceFacetNPageAccessdef)));
  return pIceFacetNPageAccessdef;
}

void CuArrayIceFacetNPageAccessdefs::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuIceFacetNPageAccessdef)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayIceFacetNPageAccessdefs::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuIceFacetNPageAccessdef)));
  CuArrayMultFlags::Add(newMultFlag);
}

