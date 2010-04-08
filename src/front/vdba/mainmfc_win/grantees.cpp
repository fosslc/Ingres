/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : Grantees.cpp                                                          //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    classes for grantees management in right pane of dom                             //
//                                                                                     //
**  26-Mar-2001 (noifr01)
**     (sir 104270) removal of code for managing Ingres/Desktop
**  26-Mar-2003 (noifr01, schph01)
**   (sir 107523) management of sequences
**  01-Dec-2004 (schph01)
**   (BUG 113511) replace Compare function by Collate according to the
**   LC_COLLATE category setting of the locale code page currently in
**   use.
****************************************************************************************/

#include "stdafx.h"

#include "grantees.h"
#include "resmfc.h"
#include "localter.h"     // LOCEXT_...

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
// CuGrantee

IMPLEMENT_SERIAL (CuGrantee, CuMultFlag, 1)

CuGrantee::CuGrantee()
  : CuMultFlag()
{
  m_granteeType = GRANTEE_TYPE_UNKNOWN;
}

CuGrantee::CuGrantee(LPCTSTR name, BOOL bSpecialItem)
  : CuMultFlag(name, bSpecialItem)
{
  m_granteeType = GRANTEE_TYPE_UNKNOWN;
}

CuGrantee::CuGrantee(const CuGrantee* pRefGrantee)
 :CuMultFlag(pRefGrantee)
{
  m_granteeType = pRefGrantee->m_granteeType;
}

void CuGrantee::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_granteeType;
  }
  else {
    int iGranteeType;
    ar >> iGranteeType;
    m_granteeType = (GranteeType)iGranteeType;
  }
}

void CuGrantee::SetGranteeType(int iobjectType)
{
  switch (iobjectType) {
    case OT_USER:
      m_granteeType = GRANTEE_TYPE_USER;
      break;
    case OT_GROUP:
      m_granteeType = GRANTEE_TYPE_GROUP;
      break;
    case OT_ROLE:
      m_granteeType = GRANTEE_TYPE_ROLE;
      break;
    case OT_ERROR:
      m_granteeType = GRANTEE_TYPE_ERROR;
      break;
    default:
      ASSERT (FALSE);
  }
}

int CALLBACK CuGrantee::CompareTypes(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuGrantee* pGrantee1 = (CuGrantee*) lParam1;
  CuGrantee* pGrantee2 = (CuGrantee*) lParam2;
  ASSERT (pGrantee1);
  ASSERT (pGrantee2);
  ASSERT (pGrantee1->IsKindOf(RUNTIME_CLASS(CuGrantee)));
  ASSERT (pGrantee2->IsKindOf(RUNTIME_CLASS(CuGrantee)));

  // test grantee types
  GranteeType t1 = pGrantee1->GetGranteeType();
  GranteeType t2 = pGrantee2->GetGranteeType();
  if (t1 < t2)
    return -1;
  if (t1 > t2)
    return +1;

  return 0;

  /* No secondary order sort
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // types equality ---> test names
  int retval = CuGrantee::CompareNames(lParam1, lParam2, 1L);
  return retval;
  */
}

int CALLBACK CuGrantee::CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuGrantee* pGrantee1 = (CuGrantee*) lParam1;
  CuGrantee* pGrantee2 = (CuGrantee*) lParam2;
  ASSERT (pGrantee1);
  ASSERT (pGrantee2);
  ASSERT (pGrantee1->IsKindOf(RUNTIME_CLASS(CuGrantee)));
  ASSERT (pGrantee2->IsKindOf(RUNTIME_CLASS(CuGrantee)));

  // test names
  CString cs1 = pGrantee1->GetStrName();
  CString cs2 = pGrantee2->GetStrName();
  int retval = cs1.Collate(cs2);
  if (retval)
    return retval;

  /* No secondary order sort
  // Anti-deadloop test
  if (lParamSort)
    return 0;
  // names  equality ---> test types
  retval = CuGrantee::CompareTypes(lParam1, lParam2, 1L);
  */

  return retval;
}

BOOL CuGrantee::IsSimilar(CuMultFlag* pSearchedObject)
{
  CuGrantee* pSearchedGrantee = (CuGrantee*)pSearchedObject;
  ASSERT (pSearchedGrantee);
  ASSERT (pSearchedGrantee->IsKindOf(RUNTIME_CLASS(CuGrantee)));

  if (GetStrName() == pSearchedGrantee->GetStrName()) {
    ASSERT (GetGranteeType() == pSearchedGrantee->GetGranteeType());
    return TRUE;
  }
  else
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuTableGrantee

IMPLEMENT_SERIAL (CuTableGrantee, CuGrantee, 1)

CuTableGrantee::CuTableGrantee()
: CuGrantee()
{
  InitialAllocateFlags(NBTBLGRANTEES);
}

CuTableGrantee::CuTableGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType)
: CuGrantee(name, bSpecialItem)
{
  InitialAllocateFlags(NBTBLGRANTEES);

  if (!bSpecialItem) {
    ASSERT (grantType);
    m_aFlags[IndexFromFlagType(grantType)] = TRUE;
  }
}

CuTableGrantee::CuTableGrantee(const CuTableGrantee* pRefTableGrantee)
:CuGrantee((CuGrantee*)pRefTableGrantee)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefTableGrantee->m_nbFlags == NBTBLGRANTEES);
}

void CuTableGrantee::Serialize (CArchive& ar)
{
  CuGrantee::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuTableGrantee::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OT_TABLEGRANT_SEL_USER:
      return 0;
    case OT_TABLEGRANT_INS_USER:
      return 1;
    case OT_TABLEGRANT_UPD_USER:
      return 2;
    case OT_TABLEGRANT_DEL_USER:
      return 3;
    case OT_TABLEGRANT_REF_USER:
      return 4;
    case OT_TABLEGRANT_CPI_USER:
      return 5;
    case OT_TABLEGRANT_CPF_USER:
      return 6;
    case OT_TABLEGRANT_ALL_USER:
      return 7;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayTableGrantees

IMPLEMENT_SERIAL (CuArrayTableGrantees, CuArrayMultFlags, 2)

void CuArrayTableGrantees::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuTableGrantee* CuArrayTableGrantees::operator [] (int index)
{
  CuTableGrantee* pGrantee;
  pGrantee = (CuTableGrantee*)m_aFlagObjects[index];
  ASSERT (pGrantee);
  ASSERT (pGrantee->IsKindOf(RUNTIME_CLASS(CuTableGrantee)));
  return pGrantee;
}

void CuArrayTableGrantees::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuTableGrantee)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayTableGrantees::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuTableGrantee)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuDbGrantee

IMPLEMENT_SERIAL (CuDbGrantee, CuGrantee, 2)

CuDbGrantee::CuDbGrantee()
: CuGrantee()
{
  InitialAllocateFlags(NBDBGRANTEES);
}

CuDbGrantee::CuDbGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType)
: CuGrantee(name, bSpecialItem)
{
  InitialAllocateFlags(NBDBGRANTEES);

  if (!bSpecialItem) {
    ASSERT (grantType);
    m_aFlags[IndexFromFlagType(grantType)] = TRUE;
  }
}

CuDbGrantee::CuDbGrantee(const CuDbGrantee* pRefDbGrantee)
:CuGrantee((CuGrantee*)pRefDbGrantee)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefDbGrantee->m_nbFlags == NBDBGRANTEES);
}

void CuDbGrantee::Serialize (CArchive& ar)
{
  CuGrantee::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuDbGrantee::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OT_DBGRANT_ACCESY_USER:
      return 0;
    case OT_DBGRANT_ACCESN_USER:
      return 1;
    case OT_DBGRANT_CREPRY_USER:
      return 2;
    case OT_DBGRANT_CREPRN_USER:
      return 3;
    case OT_DBGRANT_CRETBY_USER:
      return 4;
    case OT_DBGRANT_CRETBN_USER:
      return 5;
    case OT_DBGRANT_DBADMY_USER:
      return 6;
    case OT_DBGRANT_DBADMN_USER:
      return 7;
    case OT_DBGRANT_LKMODY_USER:
      return 8;
    case OT_DBGRANT_LKMODN_USER:
      return 9;
    case OT_DBGRANT_QRYIOY_USER:
      return 10;
    case OT_DBGRANT_QRYION_USER:
      return 11;
    case OT_DBGRANT_QRYRWY_USER:
      return 12;
    case OT_DBGRANT_QRYRWN_USER:
      return 13;
    case OT_DBGRANT_UPDSCY_USER:
      return 14;
    case OT_DBGRANT_UPDSCN_USER:
      return 15;

    case OT_DBGRANT_SELSCY_USER:
      return 16;
    case OT_DBGRANT_SELSCN_USER:
      return 17;
    case OT_DBGRANT_CNCTLY_USER:
      return 18;
    case OT_DBGRANT_CNCTLN_USER:
      return 19;
    case OT_DBGRANT_IDLTLY_USER:
      return 20;
    case OT_DBGRANT_IDLTLN_USER:
      return 21;
    case OT_DBGRANT_SESPRY_USER:
      return 22;
    case OT_DBGRANT_SESPRN_USER:
      return 23;
    case OT_DBGRANT_TBLSTY_USER:
      return 24;
    case OT_DBGRANT_TBLSTN_USER:
      return 25;

    case OT_DBGRANT_QRYCPY_USER:
      return 26;
    case OT_DBGRANT_QRYCPN_USER:
      return 27;
    case OT_DBGRANT_QRYPGY_USER:
      return 28;
    case OT_DBGRANT_QRYPGN_USER:
      return 29;
    case OT_DBGRANT_QRYCOY_USER:
      return 30;
    case OT_DBGRANT_QRYCON_USER:
      return 31;
    case OT_DBGRANT_SEQCRY_USER:
      return 32;
    case OT_DBGRANT_SEQCRN_USER:
      return 33;

    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayDbGrantees

IMPLEMENT_SERIAL (CuArrayDbGrantees, CuArrayMultFlags, 2)

void CuArrayDbGrantees::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuDbGrantee* CuArrayDbGrantees::operator [] (int index)
{
  CuDbGrantee* pGrantee;
  pGrantee = (CuDbGrantee*)m_aFlagObjects[index];
  ASSERT (pGrantee);
  ASSERT (pGrantee->IsKindOf(RUNTIME_CLASS(CuDbGrantee)));
  return pGrantee;
}

void CuArrayDbGrantees::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuDbGrantee)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayDbGrantees::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuDbGrantee)));
  CuArrayMultFlags::Add(newMultFlag);
}



/////////////////////////////////////////////////////////////////////////////////////////
// CuViewGrantee

IMPLEMENT_SERIAL (CuViewGrantee, CuGrantee, 1)

CuViewGrantee::CuViewGrantee()
: CuGrantee()
{
  InitialAllocateFlags(NBVIEWGRANTEES);
}

CuViewGrantee::CuViewGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType)
: CuGrantee(name, bSpecialItem)
{
  InitialAllocateFlags(NBVIEWGRANTEES);

  if (!bSpecialItem) {
    ASSERT (grantType);
    m_aFlags[IndexFromFlagType(grantType)] = TRUE;
  }
}

CuViewGrantee::CuViewGrantee(const CuViewGrantee* pRefViewGrantee)
:CuGrantee((CuGrantee*)pRefViewGrantee)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefViewGrantee->m_nbFlags == NBVIEWGRANTEES);
}

void CuViewGrantee::Serialize (CArchive& ar)
{
  CuGrantee::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuViewGrantee::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OT_VIEWGRANT_SEL_USER:
      return 0;
    case OT_VIEWGRANT_INS_USER:
      return 1;
    case OT_VIEWGRANT_UPD_USER:
      return 2;
    case OT_VIEWGRANT_DEL_USER:
      return 3;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayViewGrantees

IMPLEMENT_SERIAL (CuArrayViewGrantees, CuArrayMultFlags, 2)

void CuArrayViewGrantees::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuViewGrantee* CuArrayViewGrantees::operator [] (int index)
{
  CuViewGrantee* pGrantee;
  pGrantee = (CuViewGrantee*)m_aFlagObjects[index];
  ASSERT (pGrantee);
  ASSERT (pGrantee->IsKindOf(RUNTIME_CLASS(CuViewGrantee)));
  return pGrantee;
}

void CuArrayViewGrantees::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuViewGrantee)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayViewGrantees::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuViewGrantee)));
  CuArrayMultFlags::Add(newMultFlag);
}



/////////////////////////////////////////////////////////////////////////////////////////
// CuDbEventGrantee

IMPLEMENT_SERIAL (CuDbEventGrantee, CuGrantee, 1)

CuDbEventGrantee::CuDbEventGrantee()
: CuGrantee()
{
  InitialAllocateFlags(NBDBEVGRANTEES);
}

CuDbEventGrantee::CuDbEventGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType)
: CuGrantee(name, bSpecialItem)
{
  InitialAllocateFlags(NBDBEVGRANTEES);

  if (!bSpecialItem) {
    ASSERT (grantType);
    m_aFlags[IndexFromFlagType(grantType)] = TRUE;
  }
}

CuDbEventGrantee::CuDbEventGrantee(const CuDbEventGrantee* pRefDbEventGrantee)
:CuGrantee((CuGrantee*)pRefDbEventGrantee)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefDbEventGrantee->m_nbFlags == NBDBEVGRANTEES);
}

void CuDbEventGrantee::Serialize (CArchive& ar)
{
  CuGrantee::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuDbEventGrantee::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OT_DBEGRANT_RAISE_USER:
      return 0;
    case OT_DBEGRANT_REGTR_USER:
      return 1;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayDbEventGrantees

IMPLEMENT_SERIAL (CuArrayDbEventGrantees, CuArrayMultFlags, 1)

void CuArrayDbEventGrantees::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuDbEventGrantee* CuArrayDbEventGrantees::operator [] (int index)
{
  CuDbEventGrantee* pGrantee;
  pGrantee = (CuDbEventGrantee*)m_aFlagObjects[index];
  ASSERT (pGrantee);
  ASSERT (pGrantee->IsKindOf(RUNTIME_CLASS(CuDbEventGrantee)));
  return pGrantee;
}

void CuArrayDbEventGrantees::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuDbEventGrantee)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayDbEventGrantees::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuDbEventGrantee)));
  CuArrayMultFlags::Add(newMultFlag);
}



/////////////////////////////////////////////////////////////////////////////////////////
// CuProcGrantee

IMPLEMENT_SERIAL (CuProcGrantee, CuGrantee, 1)

CuProcGrantee::CuProcGrantee()
: CuGrantee()
{
  InitialAllocateFlags(NBPROCGRANTEES);
}

CuProcGrantee::CuProcGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType)
: CuGrantee(name, bSpecialItem)
{
  InitialAllocateFlags(NBPROCGRANTEES);

  if (!bSpecialItem) {
    ASSERT (grantType);
    m_aFlags[IndexFromFlagType(grantType)] = TRUE;
  }
}

CuProcGrantee::CuProcGrantee(const CuProcGrantee* pRefProcGrantee)
:CuGrantee((CuGrantee*)pRefProcGrantee)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefProcGrantee->m_nbFlags == NBPROCGRANTEES);
}

void CuProcGrantee::Serialize (CArchive& ar)
{
  CuGrantee::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuProcGrantee::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OT_PROCGRANT_EXEC_USER:
      return 0;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayProcGrantees

IMPLEMENT_SERIAL (CuArrayProcGrantees, CuArrayMultFlags, 1)

void CuArrayProcGrantees::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuProcGrantee* CuArrayProcGrantees::operator [] (int index)
{
  CuProcGrantee* pGrantee;
  pGrantee = (CuProcGrantee*)m_aFlagObjects[index];
  ASSERT (pGrantee);
  ASSERT (pGrantee->IsKindOf(RUNTIME_CLASS(CuProcGrantee)));
  return pGrantee;
}

void CuArrayProcGrantees::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuProcGrantee)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayProcGrantees::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuProcGrantee)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuSeqGrantee

IMPLEMENT_SERIAL (CuSeqGrantee, CuGrantee, 1)

CuSeqGrantee::CuSeqGrantee()
: CuGrantee()
{
  InitialAllocateFlags(NBPROCGRANTEES);
}

CuSeqGrantee::CuSeqGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType)
: CuGrantee(name, bSpecialItem)
{
  InitialAllocateFlags(NBPROCGRANTEES);

  if (!bSpecialItem) {
    ASSERT (grantType);
    m_aFlags[IndexFromFlagType(grantType)] = TRUE;
  }
}

CuSeqGrantee::CuSeqGrantee(const CuSeqGrantee* pRefSeqGrantee)
:CuGrantee((CuGrantee*)pRefSeqGrantee)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefSeqGrantee->m_nbFlags == NBPROCGRANTEES);
}

void CuSeqGrantee::Serialize (CArchive& ar)
{
  CuGrantee::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuSeqGrantee::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case OT_SEQUGRANT_NEXT_USER:
      return 0;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArraySeqGrantees

IMPLEMENT_SERIAL (CuArraySeqGrantees, CuArrayMultFlags, 1)

void CuArraySeqGrantees::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuSeqGrantee* CuArraySeqGrantees::operator [] (int index)
{
  CuSeqGrantee* pGrantee;
  pGrantee = (CuSeqGrantee*)m_aFlagObjects[index];
  ASSERT (pGrantee);
  ASSERT (pGrantee->IsKindOf(RUNTIME_CLASS(CuSeqGrantee)));
  return pGrantee;
}

void CuArraySeqGrantees::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuSeqGrantee)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArraySeqGrantees::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuSeqGrantee)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuAltLoc

IMPLEMENT_SERIAL (CuAltLoc, CuGrantee, 1)

CuAltLoc::CuAltLoc()
: CuGrantee()
{
  InitialAllocateFlags(NBALTLOC);
}

CuAltLoc::CuAltLoc(LPCTSTR name, BOOL bSpecialItem, int grantType)
: CuGrantee(name, bSpecialItem)
{
  InitialAllocateFlags(NBALTLOC);

  if (!bSpecialItem) {
    ASSERT (grantType);
    m_aFlags[IndexFromFlagType(grantType)] = TRUE;
  }
}

CuAltLoc::CuAltLoc(const CuAltLoc* pRefAltLoc)
:CuGrantee((CuGrantee*)pRefAltLoc)
{
  // Flags array copy already done in base class
  // We just need to post - assert on sizes
  ASSERT (pRefAltLoc->m_nbFlags == NBALTLOC);
}

void CuAltLoc::Serialize (CArchive& ar)
{
  CuGrantee::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

int CuAltLoc::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case LOCEXT_DATABASE:
      return 0;
    case LOCEXT_WORK:
      return 1;
    case LOCEXT_AUXILIARY:
      return 2;
      ASSERT (FALSE);
      return -1;
  }
  ASSERT (FALSE);
  return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayAltLocs

IMPLEMENT_SERIAL (CuArrayAltLocs, CuArrayMultFlags, 1)

void CuArrayAltLocs::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuAltLoc* CuArrayAltLocs::operator [] (int index)
{
  CuAltLoc* pGrantee;
  pGrantee = (CuAltLoc*)m_aFlagObjects[index];
  ASSERT (pGrantee);
  ASSERT (pGrantee->IsKindOf(RUNTIME_CLASS(CuAltLoc)));
  return pGrantee;
}

void CuArrayAltLocs::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuAltLoc)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayAltLocs::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuAltLoc)));
  CuArrayMultFlags::Add(newMultFlag);
}





