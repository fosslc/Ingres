/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : replic.cpp                                                            //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
// Classes for replicator multicolumn management:                                      //
//    cdds propagation paths and database information,                                 //
//    columns for a registered table,                                                  //
//    etc.                                                                             //
//                                                                                     //
****************************************************************************************/

#include "stdafx.h"

#include "replic.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" {
  #include "dba.h"    // REPLIC_XYZ
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuCddsPath

IMPLEMENT_SERIAL (CuCddsPath, CuMultFlag, 1)

CuCddsPath::CuCddsPath()
: CuMultFlag()
{
  InitialAllocateFlags(DUMMY_NBCDDSPATHCOLUMNS);

  m_csLocal       = _T("");
  m_csTarget      = _T("");
}

CuCddsPath::CuCddsPath(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(DUMMY_NBCDDSPATHCOLUMNS);

  m_csLocal       = _T("");
  m_csTarget      = _T("");
}

CuCddsPath::CuCddsPath(LPCTSTR originalName, LPCTSTR localName, LPCTSTR targetName)
  : CuMultFlag(originalName, FALSE)
{
  InitialAllocateFlags(DUMMY_NBCDDSPATHCOLUMNS);

  m_csLocal       = localName;
  m_csTarget      = targetName;
}

CuCddsPath::CuCddsPath(const CuCddsPath* pRefCddsPath)
 :CuMultFlag(pRefCddsPath)
{
  // Flags array already duplicated by base class

  m_csLocal       = pRefCddsPath->m_csLocal;
  m_csTarget      = pRefCddsPath->m_csTarget;
}

void CuCddsPath::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csLocal;
    ar << m_csTarget;
  }
  else {
    ar >> m_csLocal;
    ar >> m_csTarget;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayCddsPaths

IMPLEMENT_SERIAL (CuArrayCddsPaths, CuArrayMultFlags, 1)

void CuArrayCddsPaths::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuCddsPath* CuArrayCddsPaths::operator [] (int index)
{
  CuCddsPath* pCddsPath;
  pCddsPath = (CuCddsPath*)m_aFlagObjects[index];
  ASSERT (pCddsPath);
  ASSERT (pCddsPath->IsKindOf(RUNTIME_CLASS(CuCddsPath)));
  return pCddsPath;
}

void CuArrayCddsPaths::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuCddsPath)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayCddsPaths::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuCddsPath)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuCddsDbInfo


IMPLEMENT_SERIAL (CuCddsDbInfo, CuMultFlag, 1)

CuCddsDbInfo::CuCddsDbInfo()
: CuMultFlag()
{
  InitialAllocateFlags(DUMMY_NBCDDSDBINFOCOLUMNS);

  m_no        = -1;
  m_csDbName  = _T("");
  m_type      = -1;
  m_srvNo     = -1;
}

CuCddsDbInfo::CuCddsDbInfo(LPCTSTR vnodeName, LPCTSTR dbName, int no, int type, int serverNo)
:CuMultFlag(vnodeName, FALSE)
{
  InitialAllocateFlags(DUMMY_NBCDDSDBINFOCOLUMNS);

  m_no        = no;
  m_csDbName  = dbName;
  m_type      = type;
  m_srvNo     = serverNo;
}

CuCddsDbInfo::CuCddsDbInfo(LPCTSTR vnodeName)
:CuMultFlag(vnodeName, TRUE)  // error/No item
{
  InitialAllocateFlags(DUMMY_NBCDDSDBINFOCOLUMNS);

  m_no        = -1;
  m_csDbName  = _T("");
  m_type      = -1;
  m_srvNo     = -1;
}

CuCddsDbInfo::CuCddsDbInfo(const CuCddsDbInfo* pRefCddsDbInfo)
 :CuMultFlag(pRefCddsDbInfo)
{
  // Flags array already duplicated by base class

  m_no        = pRefCddsDbInfo->m_no;
  m_csDbName  = pRefCddsDbInfo->m_csDbName;
  m_type      = pRefCddsDbInfo->m_type;
  m_srvNo     = pRefCddsDbInfo->m_srvNo;
}

void CuCddsDbInfo::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_no;
    ar << m_csDbName;
    ar << m_type;
    ar << m_srvNo;
  }
  else {
    ar >> m_no;
    ar >> m_csDbName;
    ar >> m_type;
    ar >> m_srvNo;
  }
}

CString CuCddsDbInfo::GetTypeString()
{
  ASSERT (m_type != -1);

  switch (m_type) {
    case REPLIC_FULLPEER :
      return _T("Full Peer");
      break;
    case REPLIC_PROT_RO  :
      return _T("Protected Read-only");
      break;
    case REPLIC_UNPROT_RO:
      return _T("Unprotected Read-only");
      break;
    case REPLIC_TYPE_UNDEFINED:
      return _T("Undefined");
      break;
    default:
      ASSERT (FALSE);
      return _T("Error");
      break;
  }
  ASSERT (FALSE);
  return _T("Error");
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayCddsDbInfos

IMPLEMENT_SERIAL (CuArrayCddsDbInfos, CuArrayMultFlags, 1)

void CuArrayCddsDbInfos::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuCddsDbInfo* CuArrayCddsDbInfos::operator [] (int index)
{
  CuCddsDbInfo* pCddsDbInfo;
  pCddsDbInfo = (CuCddsDbInfo*)m_aFlagObjects[index];
  ASSERT (pCddsDbInfo);
  ASSERT (pCddsDbInfo->IsKindOf(RUNTIME_CLASS(CuCddsDbInfo)));
  return pCddsDbInfo;
}

void CuArrayCddsDbInfos::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuCddsDbInfo)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayCddsDbInfos::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuCddsDbInfo)));
  CuArrayMultFlags::Add(newMultFlag);
}

CuCddsDbInfo* CuArrayCddsDbInfos::FindFromDbNo(int dbNo)
{
  ASSERT (dbNo >= 0);

  int size = m_aFlagObjects.GetSize();
  for (int cpt=0; cpt<size; cpt++) {
    CuCddsDbInfo* pCddsDbInfo = (CuCddsDbInfo*)m_aFlagObjects[cpt];
    ASSERT (pCddsDbInfo);
    ASSERT (pCddsDbInfo->IsKindOf(RUNTIME_CLASS(CuCddsDbInfo)));  // CuCddsDbInfo or derived from CuCddsDbInfo
    if (pCddsDbInfo->GetNumber() == dbNo)
      return pCddsDbInfo;
  }
  ASSERT (FALSE);
  return 0;   // Not found
}


/////////////////////////////////////////////////////////////////////////////////////////
// CuCddsTable

IMPLEMENT_SERIAL (CuCddsTable, CuMultFlag, 1)

CuCddsTable::CuCddsTable()
: CuMultFlag()
{
  InitialAllocateFlags(NB_CDDS_TBL_COLUMNS);

  m_csSchema    = _T("");
  m_csLookup    = _T("");
  m_csPriority  = _T("");
  // nothing for m_aCols
}

CuCddsTable::CuCddsTable(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NB_CDDS_TBL_COLUMNS);

  m_csSchema    = _T("");
  m_csLookup    = _T("");
  m_csPriority  = _T("");
  // nothing for m_aCols
}

CuCddsTable::CuCddsTable(LPCTSTR name, LPCTSTR schema, BOOL bSupport, BOOL bActivated, LPCTSTR lookup, LPCTSTR priority)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NB_CDDS_TBL_COLUMNS);

  m_csSchema    = schema;
  m_csLookup    = lookup;
  m_csPriority  = priority;
  // nothing for m_aCols

  // Immediate update of several flags
  if (bSupport)
    m_aFlags[IndexFromFlagType(FLAG_CDDSTBL_SUPPORT)] = TRUE;
  if (bActivated)
    m_aFlags[IndexFromFlagType(FLAG_CDDSTBL_ACTIVATED)] = TRUE;
}

CuCddsTable::CuCddsTable(const CuCddsTable* pRefCddsTable)
 :CuMultFlag(pRefCddsTable)
{
  // Flags array already duplicated by base class

  m_csSchema    = pRefCddsTable->m_csSchema  ;
  m_csLookup    = pRefCddsTable->m_csLookup  ;
  m_csPriority  = pRefCddsTable->m_csPriority;
  m_aCols.Copy(pRefCddsTable->m_aCols);
}

void CuCddsTable::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csSchema  ;
    ar << m_csLookup  ;
    ar << m_csPriority;
  }
  else {
    ar >> m_csSchema  ;
    ar >> m_csLookup  ;
    ar >> m_csPriority;
  }
  m_aCols.Serialize(ar);
}

int CuCddsTable::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case FLAG_CDDSTBL_SUPPORT:
      return 0;
    case FLAG_CDDSTBL_ACTIVATED:
      return 1;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayCddsTables

IMPLEMENT_SERIAL (CuArrayCddsTables, CuArrayMultFlags, 1)

void CuArrayCddsTables::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuCddsTable* CuArrayCddsTables::operator [] (int index)
{
  CuCddsTable* pCddsTable;
  pCddsTable = (CuCddsTable*)m_aFlagObjects[index];
  ASSERT (pCddsTable);
  ASSERT (pCddsTable->IsKindOf(RUNTIME_CLASS(CuCddsTable)));
  return pCddsTable;
}

void CuArrayCddsTables::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuCddsTable)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayCddsTables::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuCddsTable)));
  CuArrayMultFlags::Add(newMultFlag);
}


/////////////////////////////////////////////////////////////////////////////////////////
// CuCddsTableCol

IMPLEMENT_SERIAL (CuCddsTableCol, CuMultFlag, 1)

CuCddsTableCol::CuCddsTableCol()
: CuMultFlag()
{
  InitialAllocateFlags(NB_CDDS_TBLCOL_COLUMNS);

  m_iRepKey     = -1;
}

CuCddsTableCol::CuCddsTableCol(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NB_CDDS_TBLCOL_COLUMNS);

  m_iRepKey     = -1;
}

CuCddsTableCol::CuCddsTableCol(LPCTSTR name, int repKey, BOOL bReplicated)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NB_CDDS_TBLCOL_COLUMNS);

  m_iRepKey     = repKey;

  // Immediate update of flag(s)
  if (bReplicated)
    m_aFlags[IndexFromFlagType(FLAG_CDDSTBLCOL_REPLIC)] = TRUE;
}

CuCddsTableCol::CuCddsTableCol(const CuCddsTableCol* pRefCddsTableCol)
 :CuMultFlag(pRefCddsTableCol)
{
  // Flags array already duplicated by base class

  m_iRepKey     = pRefCddsTableCol->m_iRepKey;
}

void CuCddsTableCol::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_iRepKey;
  }
  else {
    ar >> m_iRepKey;
  }
}

int CuCddsTableCol::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case FLAG_CDDSTBLCOL_REPLIC:
      return 0;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayCddsTableCols

IMPLEMENT_SERIAL (CuArrayCddsTableCols, CuArrayMultFlags, 1)

void CuArrayCddsTableCols::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuCddsTableCol* CuArrayCddsTableCols::operator [] (int index)
{
  CuCddsTableCol* pCddsTableCol;
  pCddsTableCol = (CuCddsTableCol*)m_aFlagObjects[index];
  ASSERT (pCddsTableCol);
  ASSERT (pCddsTableCol->IsKindOf(RUNTIME_CLASS(CuCddsTableCol)));
  return pCddsTableCol;
}

void CuArrayCddsTableCols::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuCddsTableCol)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayCddsTableCols::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuCddsTableCol)));
  CuArrayMultFlags::Add(newMultFlag);
}

