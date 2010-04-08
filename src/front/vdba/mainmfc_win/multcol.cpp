/****************************************************************************************
**
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : MultCol.cpp
**    Project : Ingres Visual DBA
**
**    Purpose  :Utility classes for multicolumn dom detail windows:
**                       Name Plus Owner, Name Plus Integer value, etc.
**
** 22-Nov-2004 (schph01)
**    BUG #113511 replace Compare function by Collate according to the
**    LC_COLLATE category setting of the locale code page currently in
**    use.
****************************************************************************************/

#include "stdafx.h"

#include "multcol.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// CuNameOnly

IMPLEMENT_SERIAL (CuNameOnly, CuMultFlag, 1)

CuNameOnly::CuNameOnly()
: CuMultFlag()
{
  InitialAllocateFlags(DUMMY_NB_NAMEONLY_COLUMNS);
}

CuNameOnly::CuNameOnly(LPCTSTR name, BOOL bSpecial)
  : CuMultFlag(name, bSpecial)
{
  InitialAllocateFlags(DUMMY_NB_NAMEONLY_COLUMNS);
}

CuNameOnly::CuNameOnly(const CuNameOnly* pRefNameOnly)
 :CuMultFlag(pRefNameOnly)
{
  // Flags array already duplicated by base class
}

void CuNameOnly::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CALLBACK CuNameOnly::CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuNameOnly* pNameOnly1 = (CuNameOnly*) lParam1;
  CuNameOnly* pNameOnly2 = (CuNameOnly*) lParam2;
  ASSERT (pNameOnly1);
  ASSERT (pNameOnly2);
  ASSERT (pNameOnly1->IsKindOf(RUNTIME_CLASS(CuNameOnly)));
  ASSERT (pNameOnly2->IsKindOf(RUNTIME_CLASS(CuNameOnly)));

  // test names
  CString cs1 = pNameOnly1->GetStrName();
  CString cs2 = pNameOnly2->GetStrName();
  int retval = cs1.Collate(cs2);
  return retval;

  // No secondary order sort
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayNameOnly

IMPLEMENT_SERIAL (CuArrayNameOnly, CuArrayMultFlags, 1)

void CuArrayNameOnly::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuNameOnly* CuArrayNameOnly::operator [] (int index)
{
  CuNameOnly* pNameOnly;
  pNameOnly = (CuNameOnly*)m_aFlagObjects[index];
  ASSERT (pNameOnly);
  ASSERT (pNameOnly->IsKindOf(RUNTIME_CLASS(CuNameOnly)));
  return pNameOnly;
}

void CuArrayNameOnly::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuNameOnly)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayNameOnly::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuNameOnly)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuNameWithNumber

IMPLEMENT_SERIAL (CuNameWithNumber, CuMultFlag, 1)

CuNameWithNumber::CuNameWithNumber()
: CuMultFlag()
{
  InitialAllocateFlags(DUMMY_NB_NAMEWITHNUMBER_COLUMNS);

  m_number = -1;
}

CuNameWithNumber::CuNameWithNumber(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(DUMMY_NB_NAMEWITHNUMBER_COLUMNS);

  m_number = -1;
}

CuNameWithNumber::CuNameWithNumber(LPCTSTR name, int number)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(DUMMY_NB_NAMEWITHNUMBER_COLUMNS);

  m_number = number;
}

CuNameWithNumber::CuNameWithNumber(const CuNameWithNumber* pRefNameWithNumber)
 :CuMultFlag(pRefNameWithNumber)
{
  // Flags array already duplicated by base class

  m_number = pRefNameWithNumber->m_number;
}

void CuNameWithNumber::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_number;
  }
  else {
    ar >> m_number;
  }
}

int CALLBACK CuNameWithNumber::CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuNameWithNumber* pNameWithNumber1 = (CuNameWithNumber*) lParam1;
  CuNameWithNumber* pNameWithNumber2 = (CuNameWithNumber*) lParam2;
  ASSERT (pNameWithNumber1);
  ASSERT (pNameWithNumber2);
  ASSERT (pNameWithNumber1->IsKindOf(RUNTIME_CLASS(CuNameWithNumber)));
  ASSERT (pNameWithNumber2->IsKindOf(RUNTIME_CLASS(CuNameWithNumber)));

  // test names
  CString cs1 = pNameWithNumber1->GetStrName();
  CString cs2 = pNameWithNumber2->GetStrName();
  int retval = cs1.Collate(cs2);
  if (retval)
    return retval;

  /* No secondary order sort
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // names  equality ---> test numbers
  retval = CuNameWithNumber::CompareNumbers(lParam1, lParam2, 1L);
  */

  return retval;
}

int CALLBACK CuNameWithNumber::CompareNumbers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuNameWithNumber* pNameWithNumber1 = (CuNameWithNumber*) lParam1;
  CuNameWithNumber* pNameWithNumber2 = (CuNameWithNumber*) lParam2;
  ASSERT (pNameWithNumber1);
  ASSERT (pNameWithNumber2);
  ASSERT (pNameWithNumber1->IsKindOf(RUNTIME_CLASS(CuNameWithNumber)));
  ASSERT (pNameWithNumber2->IsKindOf(RUNTIME_CLASS(CuNameWithNumber)));

  // test numbers
  int i1 = pNameWithNumber1->GetNumber();
  int i2 = pNameWithNumber2->GetNumber();
  if (i1<i2)
    return -1;
  if (i1 > i2)
    return +1;

  return 0;
  /* No secondary order sort
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // numbers equality ---> test names
  int retval = CuNameWithNumber::CompareNames(lParam1, lParam2, 1L);
  return retval;
  */
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayNameWithNumber

IMPLEMENT_SERIAL (CuArrayNameWithNumber, CuArrayMultFlags, 1)

void CuArrayNameWithNumber::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuNameWithNumber* CuArrayNameWithNumber::operator [] (int index)
{
  CuNameWithNumber* pNameWithNumber;
  pNameWithNumber = (CuNameWithNumber*)m_aFlagObjects[index];
  ASSERT (pNameWithNumber);
  ASSERT (pNameWithNumber->IsKindOf(RUNTIME_CLASS(CuNameWithNumber)));
  return pNameWithNumber;
}

void CuArrayNameWithNumber::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuNameWithNumber)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayNameWithNumber::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuNameWithNumber)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuNameWithOwner

IMPLEMENT_SERIAL (CuNameWithOwner, CuMultFlag, 1)

CuNameWithOwner::CuNameWithOwner()
: CuMultFlag()
{
  InitialAllocateFlags(NB_NAMEWITHOWNER_COLUMNS);

  m_csOwner = _T("");
}

CuNameWithOwner::CuNameWithOwner(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NB_NAMEWITHOWNER_COLUMNS);

  m_csOwner = _T("");
}

CuNameWithOwner::CuNameWithOwner(LPCTSTR name, LPCTSTR owner, BOOL bFlag)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NB_NAMEWITHOWNER_COLUMNS);

  m_aFlags[IndexFromFlagType(FLAG_STARTYPE)] = bFlag;

  m_csOwner = owner;
}

CuNameWithOwner::CuNameWithOwner(const CuNameWithOwner* pRefNameWithOwner)
 :CuMultFlag(pRefNameWithOwner)
{
  // Flags array already duplicated by base class

  m_csOwner = pRefNameWithOwner->m_csOwner;
}

void CuNameWithOwner::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csOwner;
  }
  else {
    ar >> m_csOwner;
  }
}

int CuNameWithOwner::IndexFromFlagType(int FlagType)
{
  ASSERT (FlagType == FLAG_STARTYPE);

  if (FlagType == FLAG_STARTYPE)
    return 0;
  else
    return -1;
}

int CALLBACK CuNameWithOwner::CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuNameWithOwner* pNameWithOwner1 = (CuNameWithOwner*) lParam1;
  CuNameWithOwner* pNameWithOwner2 = (CuNameWithOwner*) lParam2;
  ASSERT (pNameWithOwner1);
  ASSERT (pNameWithOwner2);
  ASSERT (pNameWithOwner1->IsKindOf(RUNTIME_CLASS(CuNameWithOwner)));
  ASSERT (pNameWithOwner2->IsKindOf(RUNTIME_CLASS(CuNameWithOwner)));

  // test names
  CString cs1 = pNameWithOwner1->GetStrName();
  CString cs2 = pNameWithOwner2->GetStrName();
  int retval = cs1.Collate(cs2);
  if (retval)
    return retval;

  /* No secondary order sort
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // names  equality ---> test owners
  retval = CuNameWithOwner::CompareOwners(lParam1, lParam2, 1L);
  */

  return retval;
}

int CALLBACK CuNameWithOwner::CompareOwners(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuNameWithOwner* pNameWithOwner1 = (CuNameWithOwner*) lParam1;
  CuNameWithOwner* pNameWithOwner2 = (CuNameWithOwner*) lParam2;
  ASSERT (pNameWithOwner1);
  ASSERT (pNameWithOwner2);
  ASSERT (pNameWithOwner1->IsKindOf(RUNTIME_CLASS(CuNameWithOwner)));
  ASSERT (pNameWithOwner2->IsKindOf(RUNTIME_CLASS(CuNameWithOwner)));

  // test owners
  CString cs1 = pNameWithOwner1->GetOwnerName();
  CString cs2 = pNameWithOwner2->GetOwnerName();
  int retval = cs1.Collate(cs2);
  if (retval)
    return retval;

  /* No secondary order sort
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // types equality ---> test names
  retval = CuNameWithOwner::CompareNames(lParam1, lParam2, 1L);
  */

  return retval;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayNameWithOwner

IMPLEMENT_SERIAL (CuArrayNameWithOwner, CuArrayMultFlags, 1)

void CuArrayNameWithOwner::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuNameWithOwner* CuArrayNameWithOwner::operator [] (int index)
{
  CuNameWithOwner* pNameWithOwner;
  pNameWithOwner = (CuNameWithOwner*)m_aFlagObjects[index];
  ASSERT (pNameWithOwner);
  ASSERT (pNameWithOwner->IsKindOf(RUNTIME_CLASS(CuNameWithOwner)));
  return pNameWithOwner;
}

void CuArrayNameWithOwner::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuNameWithOwner)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayNameWithOwner::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuNameWithOwner)));
  CuArrayMultFlags::Add(newMultFlag);
}

