/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : alarms.cpp
**
**    Project  : CA-OpenIngres/VDBA
**
**    classes for alarms management in right pane of dom
**
** 01-Dec-2004 (schph01)
**    BUG #113511 replace Compare function by Collate according to the
**    LC_COLLATE category setting of the locale code page currently in
**    use.
**/

#include "stdafx.h"

#include "alarms.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "dbaset.h"
  #include "monitor.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////////////////
// CuAlarm

IMPLEMENT_SERIAL (CuAlarm, CuMultFlag, 2)

CuAlarm::CuAlarm()
  : CuMultFlag()
{
  m_number              = -1L;
  m_strAlarmee          = "";
}

CuAlarm::CuAlarm(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  m_number              = -1L;
  m_strAlarmee          = "";
}

CuAlarm::CuAlarm(long number, LPCTSTR name, LPCTSTR alarmee)
  : CuMultFlag(name, FALSE)
{
  m_number  = number;
  m_strAlarmee = alarmee;
}

CuAlarm::CuAlarm(const CuAlarm* pRefAlarm)
 :CuMultFlag(pRefAlarm)
{
  m_number              = pRefAlarm->m_number            ;
  m_strAlarmee          = pRefAlarm->m_strAlarmee        ;
}

void CuAlarm::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_number;
    ar << m_strAlarmee;
  }
  else {
    ar >> m_number;
    ar >> m_strAlarmee;
  }
}

int CALLBACK CuAlarm::CompareNumbers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuAlarm* pAlarm1 = (CuAlarm*) lParam1;
  CuAlarm* pAlarm2 = (CuAlarm*) lParam2;
  ASSERT (pAlarm1);
  ASSERT (pAlarm2);
  ASSERT (pAlarm1->IsKindOf(RUNTIME_CLASS(CuAlarm)));
  ASSERT (pAlarm2->IsKindOf(RUNTIME_CLASS(CuAlarm)));

  // test numbers
  long l1 = pAlarm1->GetNumber();
  long l2 = pAlarm2->GetNumber();
  if (l1 < l2)
    return -1;
  if (l1 > l2)
    return +1;

  return 0;
  /* no secondary order code
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // numbers equality ---> test names
  int retval = CuAlarm::CompareNames(lParam1, lParam2, 1L);
  if (retval)
    return retval;
  // names equality ---> compare alarmees
  retval = CuAlarm::CompareAlarmees(lParam1, lParam2, 1L);
  return retval;
  */
}

int CALLBACK CuAlarm::CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuAlarm* pAlarm1 = (CuAlarm*) lParam1;
  CuAlarm* pAlarm2 = (CuAlarm*) lParam2;
  ASSERT (pAlarm1);
  ASSERT (pAlarm2);
  ASSERT (pAlarm1->IsKindOf(RUNTIME_CLASS(CuAlarm)));
  ASSERT (pAlarm2->IsKindOf(RUNTIME_CLASS(CuAlarm)));

  // test names
  CString cs1 = pAlarm1->GetStrName();
  CString cs2 = pAlarm2->GetStrName();
  int retval = cs1.Collate(cs2);
  if (retval)
    return retval;

  /* no secondary order code
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // strings equality ---> test numbers
  retval = CuAlarm::CompareNumbers(lParam1, lParam2, 1L);
  if (retval)
    return retval;
  // numbers equality ---> compare alarmees
  retval = CuAlarm::CompareAlarmees(lParam1, lParam2, 1L);
  */

  return retval;
}

int CALLBACK CuAlarm::CompareAlarmees(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuAlarm* pAlarm1 = (CuAlarm*) lParam1;
  CuAlarm* pAlarm2 = (CuAlarm*) lParam2;
  ASSERT (pAlarm1);
  ASSERT (pAlarm2);
  ASSERT (pAlarm1->IsKindOf(RUNTIME_CLASS(CuAlarm)));
  ASSERT (pAlarm2->IsKindOf(RUNTIME_CLASS(CuAlarm)));

  // test alarmees
  CString cs1 = pAlarm1->GetStrAlarmee();
  CString cs2 = pAlarm2->GetStrAlarmee();
  int retval = cs1.Collate(cs2);
  if (retval)
    return retval;

  /* no secondary order code
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // strings equality ---> test numbers
  retval = CuAlarm::CompareNumbers(lParam1, lParam2, 1L);
  if (retval)
    return retval;
  // numbers equality ---> compare names
  retval = CuAlarm::CompareNames(lParam1, lParam2, 1L);
  */

  return retval;
}

BOOL CuAlarm::IsSimilar(CuMultFlag* pSearchedObject)
{
  CuAlarm* pSearchedAlarm = (CuAlarm*)pSearchedObject;
  ASSERT (pSearchedAlarm);
  ASSERT (pSearchedAlarm->IsKindOf(RUNTIME_CLASS(CuAlarm)));

  if (GetNumber() == pSearchedAlarm->GetNumber()) {
    ASSERT (GetStrName() == pSearchedAlarm->GetStrName());
    ASSERT (GetStrAlarmee() == pSearchedAlarm->GetStrAlarmee());
    return TRUE;
  }
  else
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuDbAlarm

IMPLEMENT_SERIAL (CuDbAlarm, CuAlarm, 1)

CuDbAlarm::CuDbAlarm()
: CuAlarm()
{
  InitialAllocateFlags(NBDBALARMS);
}

CuDbAlarm::CuDbAlarm(LPCTSTR name)
: CuAlarm(name)
{
  InitialAllocateFlags(NBDBALARMS);
}

CuDbAlarm::CuDbAlarm(long number, LPCTSTR name, LPCTSTR alarmee, int alarmType)
: CuAlarm(number, name, alarmee)
{
  InitialAllocateFlags(NBDBALARMS);

  m_aFlags[IndexFromFlagType(alarmType)] = TRUE;
}

CuDbAlarm::CuDbAlarm(const CuDbAlarm* pRefDbAlarm)
:CuAlarm((CuAlarm*)pRefDbAlarm)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefDbAlarm->m_nbFlags == NBDBALARMS);
}

void CuDbAlarm::Serialize (CArchive& ar)
{
  CuAlarm::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuDbAlarm::IndexFromFlagType(int alarmType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (alarmType) {
    case OT_S_ALARM_CO_SUCCESS_USER:
      return 0;
    case OT_S_ALARM_CO_FAILURE_USER:
      return 1;
    case OT_S_ALARM_DI_SUCCESS_USER:
      return 2;
    case OT_S_ALARM_DI_FAILURE_USER:
      return 3;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayDbAlarms

IMPLEMENT_SERIAL (CuArrayDbAlarms, CuArrayMultFlags, 2)

void CuArrayDbAlarms::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuDbAlarm* CuArrayDbAlarms::operator [] (int index)
{
  CuDbAlarm* pAlarm;
  pAlarm = (CuDbAlarm*)m_aFlagObjects[index];
  ASSERT (pAlarm);
  ASSERT (pAlarm->IsKindOf(RUNTIME_CLASS(CuDbAlarm)));
  return pAlarm;
}

void CuArrayDbAlarms::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuDbAlarm)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayDbAlarms::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuDbAlarm)));
  CuArrayMultFlags::Add(newMultFlag);
}


/////////////////////////////////////////////////////////////////////////////////////////
// CuTableAlarm

IMPLEMENT_SERIAL (CuTableAlarm, CuAlarm, 1)

CuTableAlarm::CuTableAlarm()
: CuAlarm()
{
  InitialAllocateFlags(NBTBLALARMS);
}

CuTableAlarm::CuTableAlarm(LPCTSTR name)
: CuAlarm(name)
{
  InitialAllocateFlags(NBTBLALARMS);
}

CuTableAlarm::CuTableAlarm(long number, LPCTSTR name, LPCTSTR alarmee, int alarmType)
: CuAlarm(number, name, alarmee)
{
  InitialAllocateFlags(NBTBLALARMS);

  m_aFlags[IndexFromFlagType(alarmType)] = TRUE;
}

CuTableAlarm::CuTableAlarm(const CuTableAlarm* pRefTableAlarm)
:CuAlarm((CuAlarm*)pRefTableAlarm)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefTableAlarm->m_nbFlags == NBTBLALARMS);
}

void CuTableAlarm::Serialize (CArchive& ar)
{
  CuAlarm::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuTableAlarm::IndexFromFlagType(int alarmType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (alarmType) {
    case OT_S_ALARM_SELSUCCESS_USER:
      return 0;
    case OT_S_ALARM_SELFAILURE_USER:
      return 1;
    case OT_S_ALARM_DELSUCCESS_USER:
      return 2;
    case OT_S_ALARM_DELFAILURE_USER:
      return 3;
    case OT_S_ALARM_INSSUCCESS_USER:
      return 4;
    case OT_S_ALARM_INSFAILURE_USER:
      return 5;
    case OT_S_ALARM_UPDSUCCESS_USER:
      return 6;
    case OT_S_ALARM_UPDFAILURE_USER:
      return 7;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayTableAlarms

IMPLEMENT_SERIAL (CuArrayTableAlarms, CuArrayMultFlags, 2)

void CuArrayTableAlarms::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuTableAlarm* CuArrayTableAlarms::operator [] (int index)
{
  CuTableAlarm* pAlarm;
  pAlarm = (CuTableAlarm*)m_aFlagObjects[index];
  ASSERT (pAlarm);
  ASSERT (pAlarm->IsKindOf(RUNTIME_CLASS(CuTableAlarm)));
  return pAlarm;
}


void CuArrayTableAlarms::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuTableAlarm)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayTableAlarms::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuTableAlarm)));
  CuArrayMultFlags::Add(newMultFlag);
}


/////////////////////////////////////////////////////////////////////////////////////////
// CuUserXAlarm

IMPLEMENT_SERIAL (CuUserXAlarm, CuMultFlag, 1)

CuUserXAlarm::CuUserXAlarm()
: CuMultFlag()
{
  InitialAllocateFlags(NBUSERXTBLALARMS);

  m_number      = -1;
  m_strDbName   = _T("");
  m_strTblName  = _T("");
}

CuUserXAlarm::CuUserXAlarm(LPCTSTR name)
: CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NBUSERXTBLALARMS);

  m_number      = -1;
  m_strDbName   = _T("");
  m_strTblName  = _T("");
}

CuUserXAlarm::CuUserXAlarm(long number, LPCTSTR name, LPCTSTR dbName, LPCTSTR tblName, int alarmType)
: CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NBUSERXTBLALARMS);

  m_aFlags[IndexFromFlagType(alarmType)] = TRUE;

  m_number      = number;
  m_strDbName   = dbName;
  m_strTblName  = tblName;
}

CuUserXAlarm::CuUserXAlarm(const CuUserXAlarm* pRefUserXAlarm)
:CuMultFlag((CuMultFlag*)pRefUserXAlarm)
{
  // Flags array copy already done in base class

  ASSERT (pRefUserXAlarm->m_nbFlags == NBUSERXTBLALARMS);

  m_number      = pRefUserXAlarm->m_number;
  m_strDbName   = pRefUserXAlarm->m_strDbName;
  m_strTblName  = pRefUserXAlarm->m_strTblName;
}

void CuUserXAlarm::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_number;
    ar << m_strDbName;
    ar << m_strTblName;
  }
  else {
    ar >> m_number;
    ar >> m_strDbName;
    ar >> m_strTblName;
  }
}

int CuUserXAlarm::IndexFromFlagType(int alarmType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (alarmType) {
    case OTR_ALARM_SELSUCCESS_TABLE:
      return 0;
    case OTR_ALARM_SELFAILURE_TABLE:
      return 1;
    case OTR_ALARM_DELSUCCESS_TABLE:
      return 2;
    case OTR_ALARM_DELFAILURE_TABLE:
      return 3;
    case OTR_ALARM_INSSUCCESS_TABLE:
      return 4;
    case OTR_ALARM_INSFAILURE_TABLE:
      return 5;
    case OTR_ALARM_UPDSUCCESS_TABLE:
      return 6;
    case OTR_ALARM_UPDFAILURE_TABLE:
      return 7;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

BOOL CuUserXAlarm::IsSimilar(CuMultFlag* pSearchedObject)
{
  CuUserXAlarm* pSearchedAlarm  = (CuUserXAlarm*)pSearchedObject;
  ASSERT (pSearchedAlarm);
  ASSERT (pSearchedAlarm->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));

  if (GetNumber() == pSearchedAlarm->GetNumber()
    && GetStrDbName() == pSearchedAlarm->GetStrDbName()
    && GetStrTblName() == pSearchedAlarm->GetStrTblName()
    )
    return TRUE;
  else
    return FALSE;
}


int CALLBACK CuUserXAlarm::CompareNumbers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuUserXAlarm* pAlarm1 = (CuUserXAlarm*) lParam1;
  CuUserXAlarm* pAlarm2 = (CuUserXAlarm*) lParam2;
  ASSERT (pAlarm1);
  ASSERT (pAlarm2);
  ASSERT (pAlarm1->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));
  ASSERT (pAlarm2->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));

  // test numbers
  long l1 = pAlarm1->GetNumber();
  long l2 = pAlarm2->GetNumber();
  if (l1 < l2)
    return -1;
  if (l1 > l2)
    return +1;
  return 0;

  // No secondary order sort
}

int CALLBACK CuUserXAlarm::CompareDbNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuUserXAlarm* pAlarm1 = (CuUserXAlarm*) lParam1;
  CuUserXAlarm* pAlarm2 = (CuUserXAlarm*) lParam2;
  ASSERT (pAlarm1);
  ASSERT (pAlarm2);
  ASSERT (pAlarm1->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));
  ASSERT (pAlarm2->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));

  // test names
  CString cs1 = pAlarm1->GetStrDbName();
  CString cs2 = pAlarm2->GetStrDbName();
  int retval = cs1.Collate(cs2);
  return retval;

  // No secondary order sort
}

int CALLBACK CuUserXAlarm::CompareTblNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuUserXAlarm* pAlarm1 = (CuUserXAlarm*) lParam1;
  CuUserXAlarm* pAlarm2 = (CuUserXAlarm*) lParam2;
  ASSERT (pAlarm1);
  ASSERT (pAlarm2);
  ASSERT (pAlarm1->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));
  ASSERT (pAlarm2->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));

  // test alarmees
  CString cs1 = pAlarm1->GetStrTblName();
  CString cs2 = pAlarm2->GetStrTblName();
  int retval = cs1.Collate(cs2);
  return retval;

  // No secondary order sort
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayUserXAlarms

IMPLEMENT_SERIAL (CuArrayUserXAlarms, CuArrayMultFlags, 2)

void CuArrayUserXAlarms::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuUserXAlarm* CuArrayUserXAlarms::operator [] (int index)
{
  CuUserXAlarm* pAlarm;
  pAlarm = (CuUserXAlarm*)m_aFlagObjects[index];
  ASSERT (pAlarm);
  ASSERT (pAlarm->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));
  return pAlarm;
}


void CuArrayUserXAlarms::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayUserXAlarms::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuUserXAlarm)));
  CuArrayMultFlags::Add(newMultFlag);
}

