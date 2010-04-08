/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**
**    Source   : Granted.cpp
**
**    Project  : CA-OpenIngres/VDBA
**
**    classes for granted objects (cross-ref) management in right pane of dom
**
** 01-Dec-2004 (schph01)
**    BUG #113511 replace Compare function by Collate according to the
**    LC_COLLATE category setting of the locale code page currently in
**    use.
**/
#include "stdafx.h"

#include "granted.h"

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
// CuGranted

IMPLEMENT_SERIAL (CuGranted, CuMultFlag, 1)

CuGranted::CuGranted()
  : CuMultFlag()
{
  m_csOwner  = _T("");
  m_csParent = _T("");
}

CuGranted::CuGranted(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  m_csOwner  = _T("");
  m_csParent = _T("");
}

CuGranted::CuGranted(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb)
  : CuMultFlag(name, FALSE)
{
  m_csOwner  = owner;
  m_csParent = parentDb;
}

CuGranted::CuGranted(const CuGranted* pRefGranted)
 :CuMultFlag(pRefGranted)
{
  m_csOwner  = pRefGranted->m_csOwner;
  m_csParent = pRefGranted->m_csParent;
}

void CuGranted::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csOwner;
    ar << m_csParent;
  }
  else {
    ar >> m_csOwner;
    ar >> m_csParent;
  }
}

int CALLBACK CuGranted::CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuGranted* pGranted1 = (CuGranted*) lParam1;
  CuGranted* pGranted2 = (CuGranted*) lParam2;
  ASSERT (pGranted1);
  ASSERT (pGranted2);
  ASSERT (pGranted1->IsKindOf(RUNTIME_CLASS(CuGranted)));
  ASSERT (pGranted2->IsKindOf(RUNTIME_CLASS(CuGranted)));

  // test names
  CString cs1 = pGranted1->GetStrName();
  CString cs2 = pGranted2->GetStrName();
  int retval = cs1.Collate(cs2);
  return retval;

  // No secondary order sort
}

int CALLBACK CuGranted::CompareOwners(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuGranted* pGranted1 = (CuGranted*) lParam1;
  CuGranted* pGranted2 = (CuGranted*) lParam2;
  ASSERT (pGranted1);
  ASSERT (pGranted2);
  ASSERT (pGranted1->IsKindOf(RUNTIME_CLASS(CuGranted)));
  ASSERT (pGranted2->IsKindOf(RUNTIME_CLASS(CuGranted)));

  CString cs1 = pGranted1->GetOwnerName();
  CString cs2 = pGranted2->GetOwnerName();
  int retval = cs1.Collate(cs2);
  return retval;

  // No secondary order sort
}

int CALLBACK CuGranted::CompareParents(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuGranted* pGranted1 = (CuGranted*) lParam1;
  CuGranted* pGranted2 = (CuGranted*) lParam2;
  ASSERT (pGranted1);
  ASSERT (pGranted2);
  ASSERT (pGranted1->IsKindOf(RUNTIME_CLASS(CuGranted)));
  ASSERT (pGranted2->IsKindOf(RUNTIME_CLASS(CuGranted)));

  CString cs1 = pGranted1->GetParentName();
  CString cs2 = pGranted2->GetParentName();
  int retval = cs1.Collate(cs2);
  return retval;

  // No secondary order sort
}

BOOL CuGranted::IsSimilar(CuMultFlag* pSearchedObject)
{
  CuGranted* pSearchedGranted = (CuGranted*)pSearchedObject;
  ASSERT (pSearchedGranted);
  ASSERT (pSearchedGranted->IsKindOf(RUNTIME_CLASS(CuGranted)));

  if (GetStrName() == pSearchedGranted->GetStrName()
      && GetOwnerName() == pSearchedGranted->GetOwnerName()
      && GetParentName() == pSearchedGranted->GetParentName() )
    return TRUE;
  else
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuGrantedTbl

IMPLEMENT_SERIAL (CuGrantedTbl, CuGranted, 1)

CuGrantedTbl::CuGrantedTbl()
: CuGranted()
{
  InitialAllocateFlags(NBTBLGRANTED);
}

CuGrantedTbl::CuGrantedTbl(LPCTSTR name)
: CuGranted(name)
{
  InitialAllocateFlags(NBTBLGRANTED);
}


CuGrantedTbl::CuGrantedTbl(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType)
: CuGranted(name, owner, parentDb)
{
  InitialAllocateFlags(NBTBLGRANTED);

  ASSERT (grantType);
  m_aFlags[IndexFromFlagType(grantType)] = TRUE;
}

CuGrantedTbl::CuGrantedTbl(const CuGrantedTbl* pRefGrantedTbl)
:CuGranted((CuGranted*)pRefGrantedTbl)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefGrantedTbl->m_nbFlags == NBTBLGRANTED);
}

void CuGrantedTbl::Serialize (CArchive& ar)
{
  CuGranted::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuGrantedTbl::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OTR_GRANTEE_SEL_TABLE:
      return 0;
    case OTR_GRANTEE_INS_TABLE:
      return 1;
    case OTR_GRANTEE_UPD_TABLE:
      return 2;
    case OTR_GRANTEE_DEL_TABLE:
      return 3;
    case OTR_GRANTEE_REF_TABLE:
      return 4;
    case OTR_GRANTEE_CPI_TABLE:
      return 5;
    case OTR_GRANTEE_CPF_TABLE:
      return 6;
    case OTR_GRANTEE_ALL_TABLE:
      return 7;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayGrantedTbls

IMPLEMENT_SERIAL (CuArrayGrantedTbls, CuArrayMultFlags, 2)

void CuArrayGrantedTbls::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuGrantedTbl* CuArrayGrantedTbls::operator [] (int index)
{
  CuGrantedTbl* pGranted;
  pGranted = (CuGrantedTbl*)m_aFlagObjects[index];
  ASSERT (pGranted);
  ASSERT (pGranted->IsKindOf(RUNTIME_CLASS(CuGrantedTbl)));
  return pGranted;
}

void CuArrayGrantedTbls::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuGrantedTbl)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayGrantedTbls::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuGrantedTbl)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuGrantedDb

IMPLEMENT_SERIAL (CuGrantedDb, CuGranted, 2)

CuGrantedDb::CuGrantedDb()
: CuGranted()
{
  InitialAllocateFlags(NBDBGRANTED);
}

CuGrantedDb::CuGrantedDb(LPCTSTR name)
: CuGranted(name)
{
  InitialAllocateFlags(NBDBGRANTED);
}


CuGrantedDb::CuGrantedDb(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType)
: CuGranted(name, owner, parentDb)
{
  InitialAllocateFlags(NBDBGRANTED);

  ASSERT (grantType);
  m_aFlags[IndexFromFlagType(grantType)] = TRUE;
}

CuGrantedDb::CuGrantedDb(const CuGrantedDb* pRefGrantedDb)
:CuGranted((CuGranted*)pRefGrantedDb)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefGrantedDb->m_nbFlags == NBDBGRANTED);
}

void CuGrantedDb::Serialize (CArchive& ar)
{
  CuGranted::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuGrantedDb::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OTR_DBGRANT_ACCESY_DB:
      return 0;
    case OTR_DBGRANT_ACCESN_DB:
      return 1;
    case OTR_DBGRANT_CREPRY_DB:
      return 2;
    case OTR_DBGRANT_CREPRN_DB:
      return 3;
    case OTR_DBGRANT_CRETBY_DB:
      return 4;
    case OTR_DBGRANT_CRETBN_DB:
      return 5;
    case OTR_DBGRANT_DBADMY_DB:
      return 6;
    case OTR_DBGRANT_DBADMN_DB:
      return 7;
    case OTR_DBGRANT_LKMODY_DB:
      return 8;
    case OTR_DBGRANT_LKMODN_DB:
      return 9;
    case OTR_DBGRANT_QRYIOY_DB:
      return 10;
    case OTR_DBGRANT_QRYION_DB:
      return 11;
    case OTR_DBGRANT_QRYRWY_DB:
      return 12;
    case OTR_DBGRANT_QRYRWN_DB:
      return 13;
    case OTR_DBGRANT_UPDSCY_DB:
      return 14;
    case OTR_DBGRANT_UPDSCN_DB:
      return 15;

    case OTR_DBGRANT_SELSCY_DB:
      return 16;
    case OTR_DBGRANT_SELSCN_DB:
      return 17;
    case OTR_DBGRANT_CNCTLY_DB:
      return 18;
    case OTR_DBGRANT_CNCTLN_DB:
      return 19;
    case OTR_DBGRANT_IDLTLY_DB:
      return 20;
    case OTR_DBGRANT_IDLTLN_DB:
      return 21;
    case OTR_DBGRANT_SESPRY_DB:
      return 22;
    case OTR_DBGRANT_SESPRN_DB:
      return 23;
    case OTR_DBGRANT_TBLSTY_DB:
      return 24;
    case OTR_DBGRANT_TBLSTN_DB:
      return 25;

    case OTR_DBGRANT_QRYCPY_DB:
      return 26;
    case OTR_DBGRANT_QRYCPN_DB:
      return 27;
    case OTR_DBGRANT_QRYPGY_DB:
      return 28;
    case OTR_DBGRANT_QRYPGN_DB:
      return 29;
    case OTR_DBGRANT_QRYCOY_DB:
      return 30;
    case OTR_DBGRANT_QRYCON_DB:
      return 31;
    case OTR_DBGRANT_SEQCRY_DB:
      return 32;
    case OTR_DBGRANT_SEQCRN_DB:
      return 33;

    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayGrantedDbs

IMPLEMENT_SERIAL (CuArrayGrantedDbs, CuArrayMultFlags, 2)

void CuArrayGrantedDbs::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuGrantedDb* CuArrayGrantedDbs::operator [] (int index)
{
  CuGrantedDb* pGranted;
  pGranted = (CuGrantedDb*)m_aFlagObjects[index];
  ASSERT (pGranted);
  ASSERT (pGranted->IsKindOf(RUNTIME_CLASS(CuGrantedDb)));
  return pGranted;
}

void CuArrayGrantedDbs::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuGrantedDb)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayGrantedDbs::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuGrantedDb)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuGrantedView

IMPLEMENT_SERIAL (CuGrantedView, CuGranted, 1)

CuGrantedView::CuGrantedView()
: CuGranted()
{
  InitialAllocateFlags(NBVIEWGRANTED);
}

CuGrantedView::CuGrantedView(LPCTSTR name)
: CuGranted(name)
{
  InitialAllocateFlags(NBVIEWGRANTED);
}


CuGrantedView::CuGrantedView(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType)
: CuGranted(name, owner, parentDb)
{
  InitialAllocateFlags(NBVIEWGRANTED);

  ASSERT (grantType);
  m_aFlags[IndexFromFlagType(grantType)] = TRUE;
}

CuGrantedView::CuGrantedView(const CuGrantedView* pRefGrantedView)
:CuGranted((CuGranted*)pRefGrantedView)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefGrantedView->m_nbFlags == NBVIEWGRANTED);
}

void CuGrantedView::Serialize (CArchive& ar)
{
  CuGranted::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuGrantedView::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OTR_GRANTEE_SEL_VIEW:
      return 0;
    case OTR_GRANTEE_INS_VIEW:
      return 1;
    case OTR_GRANTEE_UPD_VIEW:
      return 2;
    case OTR_GRANTEE_DEL_VIEW:
      return 3;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayGrantedViews

IMPLEMENT_SERIAL (CuArrayGrantedViews, CuArrayMultFlags, 2)

void CuArrayGrantedViews::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuGrantedView* CuArrayGrantedViews::operator [] (int index)
{
  CuGrantedView* pGranted;
  pGranted = (CuGrantedView*)m_aFlagObjects[index];
  ASSERT (pGranted);
  ASSERT (pGranted->IsKindOf(RUNTIME_CLASS(CuGrantedView)));
  return pGranted;
}

void CuArrayGrantedViews::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuGrantedView)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayGrantedViews::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuGrantedView)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuGrantedEvent

IMPLEMENT_SERIAL (CuGrantedEvent, CuGranted, 1)

CuGrantedEvent::CuGrantedEvent()
: CuGranted()
{
  InitialAllocateFlags(NBDBEVGRANTED);
}

CuGrantedEvent::CuGrantedEvent(LPCTSTR name)
: CuGranted(name)
{
  InitialAllocateFlags(NBDBEVGRANTED);
}


CuGrantedEvent::CuGrantedEvent(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType)
: CuGranted(name, owner, parentDb)
{
  InitialAllocateFlags(NBDBEVGRANTED);

  ASSERT (grantType);
  m_aFlags[IndexFromFlagType(grantType)] = TRUE;
}

CuGrantedEvent::CuGrantedEvent(const CuGrantedEvent* pRefGrantedEvent)
:CuGranted((CuGranted*)pRefGrantedEvent)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefGrantedEvent->m_nbFlags == NBDBEVGRANTED);
}

void CuGrantedEvent::Serialize (CArchive& ar)
{
  CuGranted::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuGrantedEvent::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OTR_GRANTEE_RAISE_DBEVENT:
      return 0;
    case OTR_GRANTEE_REGTR_DBEVENT:
      return 1;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayGrantedEvents

IMPLEMENT_SERIAL (CuArrayGrantedEvents, CuArrayMultFlags, 2)

void CuArrayGrantedEvents::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuGrantedEvent* CuArrayGrantedEvents::operator [] (int index)
{
  CuGrantedEvent* pGranted;
  pGranted = (CuGrantedEvent*)m_aFlagObjects[index];
  ASSERT (pGranted);
  ASSERT (pGranted->IsKindOf(RUNTIME_CLASS(CuGrantedEvent)));
  return pGranted;
}

void CuArrayGrantedEvents::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuGrantedEvent)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayGrantedEvents::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuGrantedEvent)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuGrantedProc

IMPLEMENT_SERIAL (CuGrantedProc, CuGranted, 1)

CuGrantedProc::CuGrantedProc()
: CuGranted()
{
  InitialAllocateFlags(NBPROCGRANTED);
}

CuGrantedProc::CuGrantedProc(LPCTSTR name)
: CuGranted(name)
{
  InitialAllocateFlags(NBPROCGRANTED);
}


CuGrantedProc::CuGrantedProc(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType)
: CuGranted(name, owner, parentDb)
{
  InitialAllocateFlags(NBPROCGRANTED);

  ASSERT (grantType);
  m_aFlags[IndexFromFlagType(grantType)] = TRUE;
}

CuGrantedProc::CuGrantedProc(const CuGrantedProc* pRefGrantedProc)
:CuGranted((CuGranted*)pRefGrantedProc)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefGrantedProc->m_nbFlags == NBPROCGRANTED);
}

void CuGrantedProc::Serialize (CArchive& ar)
{
  CuGranted::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuGrantedProc::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OTR_GRANTEE_EXEC_PROC:
      return 0;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayGrantedProcs

IMPLEMENT_SERIAL (CuArrayGrantedProcs, CuArrayMultFlags, 2)

void CuArrayGrantedProcs::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuGrantedProc* CuArrayGrantedProcs::operator [] (int index)
{
  CuGrantedProc* pGranted;
  pGranted = (CuGrantedProc*)m_aFlagObjects[index];
  ASSERT (pGranted);
  ASSERT (pGranted->IsKindOf(RUNTIME_CLASS(CuGrantedProc)));
  return pGranted;
}

void CuArrayGrantedProcs::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuGrantedProc)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayGrantedProcs::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuGrantedProc)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuGrantedSeq

IMPLEMENT_SERIAL (CuGrantedSeq, CuGranted, 1)

CuGrantedSeq::CuGrantedSeq()
: CuGranted()
{
  InitialAllocateFlags(NBSEQGRANTED);
}

CuGrantedSeq::CuGrantedSeq(LPCTSTR name)
: CuGranted(name)
{
  InitialAllocateFlags(NBSEQGRANTED);
}


CuGrantedSeq::CuGrantedSeq(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType)
: CuGranted(name, owner, parentDb)
{
  InitialAllocateFlags(NBSEQGRANTED);

  ASSERT (grantType);
  m_aFlags[IndexFromFlagType(grantType)] = TRUE;
}

CuGrantedSeq::CuGrantedSeq(const CuGrantedSeq* pRefGrantedSeq)
:CuGranted((CuGranted*)pRefGrantedSeq)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefGrantedSeq->m_nbFlags == NBSEQGRANTED);
}

void CuGrantedSeq::Serialize (CArchive& ar)
{
  CuGranted::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuGrantedSeq::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OTR_GRANTEE_NEXT_SEQU:
      return 0;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayGrantedSeqs

IMPLEMENT_SERIAL (CuArrayGrantedSeqs, CuArrayMultFlags, 2)

void CuArrayGrantedSeqs::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuGrantedSeq* CuArrayGrantedSeqs::operator [] (int index)
{
  CuGrantedSeq* pGranted;
  pGranted = (CuGrantedSeq*)m_aFlagObjects[index];
  ASSERT (pGranted);
  ASSERT (pGranted->IsKindOf(RUNTIME_CLASS(CuGrantedSeq)));
  return pGranted;
}

void CuArrayGrantedSeqs::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuGrantedSeq)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayGrantedSeqs::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuGrantedSeq)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuGrantedRole

IMPLEMENT_SERIAL (CuGrantedRole, CuGranted, 1)

CuGrantedRole::CuGrantedRole()
: CuGranted()
{
  InitialAllocateFlags(NBROLEGRANTED);
}

CuGrantedRole::CuGrantedRole(LPCTSTR name)
: CuGranted(name)
{
  InitialAllocateFlags(NBROLEGRANTED);
}


CuGrantedRole::CuGrantedRole(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType)
: CuGranted(name, owner, parentDb)
{
  InitialAllocateFlags(NBROLEGRANTED);

  ASSERT (grantType);
  m_aFlags[IndexFromFlagType(grantType)] = TRUE;
}

CuGrantedRole::CuGrantedRole(const CuGrantedRole* pRefGrantedRole)
:CuGranted((CuGranted*)pRefGrantedRole)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefGrantedRole->m_nbFlags == NBROLEGRANTED);
}

void CuGrantedRole::Serialize (CArchive& ar)
{
  CuGranted::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuGrantedRole::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OTR_GRANTEE_ROLE:
      return 0;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayGrantedRoles

IMPLEMENT_SERIAL (CuArrayGrantedRoles, CuArrayMultFlags, 2)

void CuArrayGrantedRoles::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuGrantedRole* CuArrayGrantedRoles::operator [] (int index)
{
  CuGrantedRole* pGranted;
  pGranted = (CuGrantedRole*)m_aFlagObjects[index];
  ASSERT (pGranted);
  ASSERT (pGranted->IsKindOf(RUNTIME_CLASS(CuGrantedRole)));
  return pGranted;
}

void CuArrayGrantedRoles::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuGrantedRole)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayGrantedRoles::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuGrantedRole)));
  CuArrayMultFlags::Add(newMultFlag);
}

