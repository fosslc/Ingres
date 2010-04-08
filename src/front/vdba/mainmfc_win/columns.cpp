/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : columns.cpp                                                            //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    classes for table columns management in right pane of dom                        //
//                                                                                     //
//  16-Nov-2000 (schph01)
//    sir 102821 Comment on table and columns.
****************************************************************************************/

#include "stdafx.h"

#include "columns.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// CuTblColumn

IMPLEMENT_SERIAL (CuTblColumn, CuMultFlag, 1)

CuTblColumn::CuTblColumn()
: CuMultFlag()
{
  InitialAllocateFlags(NBTBLCOLCOLUMNS);

  m_csType        = "";
  m_primKeyRank   = -1;
  m_csDefSpec     = "";
  m_csColumnComment= "";
}

CuTblColumn::CuTblColumn(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NBTBLCOLCOLUMNS);

  m_csType        = "";
  m_primKeyRank   = -1;
  m_csDefSpec     = "";
  m_csColumnComment= "";
}

CuTblColumn::CuTblColumn(LPCTSTR name, LPCTSTR type, int primKeyRank, BOOL bNullable, BOOL bWithDefault, LPCTSTR defSpec,LPCTSTR ColComment)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NBTBLCOLCOLUMNS);

  m_csType        = type;
  m_primKeyRank   = primKeyRank;
  if (bWithDefault) {
    ASSERT(defSpec);
    m_csDefSpec     = defSpec;
  }
  else
    m_csDefSpec = "";

  m_csColumnComment = ColComment;

  // Immediate update of several flags
  if (bNullable)
    m_aFlags[IndexFromFlagType(FLAG_COL_NULLABLE)] = TRUE;
  if (bWithDefault)
    m_aFlags[IndexFromFlagType(FLAG_COL_WITHDEFAULT)] = TRUE;
}

CuTblColumn::CuTblColumn(const CuTblColumn* pRefTblColumn)
 :CuMultFlag(pRefTblColumn)
{
  // Flags array already duplicated by base class

  m_csType        = pRefTblColumn->m_csType        ;
  m_primKeyRank   = pRefTblColumn->m_primKeyRank   ;
  m_csDefSpec     = pRefTblColumn->m_csDefSpec     ;
  m_csColumnComment = pRefTblColumn->m_csColumnComment;
}

void CuTblColumn::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csType;
    ar << m_primKeyRank;
    ar << m_csDefSpec;
    ar << m_csColumnComment;
  }
  else {
    ar >> m_csType;
    ar >> m_primKeyRank;
    ar >> m_csDefSpec;
    ar >> m_csColumnComment;
  }
}

int CuTblColumn::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case FLAG_COL_NULLABLE:
      return 0;
    case FLAG_COL_WITHDEFAULT:
      return 1;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayTblColumns

IMPLEMENT_SERIAL (CuArrayTblColumns, CuArrayMultFlags, 1)

void CuArrayTblColumns::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuTblColumn* CuArrayTblColumns::operator [] (int index)
{
  CuTblColumn* pTblColumn;
  pTblColumn = (CuTblColumn*)m_aFlagObjects[index];
  ASSERT (pTblColumn);
  ASSERT (pTblColumn->IsKindOf(RUNTIME_CLASS(CuTblColumn)));
  return pTblColumn;
}

void CuArrayTblColumns::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuTblColumn)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayTblColumns::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuTblColumn)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuTblConstraint


IMPLEMENT_SERIAL (CuTblConstraint, CuMultFlag, 1)

CuTblConstraint::CuTblConstraint()
: CuMultFlag()
{
  InitialAllocateFlags(DUMMYNBCONSTRAINTCOL);

  m_cType  = _T('\0');
  m_strIndex  = _T("");
}

CuTblConstraint::CuTblConstraint(TCHAR cType, LPCTSTR text, LPCTSTR lpszIndex)
:CuMultFlag(text, FALSE)
{
  InitialAllocateFlags(DUMMYNBCONSTRAINTCOL);

  m_cType  = cType;
  m_strIndex  = lpszIndex;
}

CuTblConstraint::CuTblConstraint(LPCTSTR text)
:CuMultFlag(text, TRUE)  // error/No item
{
  InitialAllocateFlags(DUMMYNBCONSTRAINTCOL);
  m_strIndex = _T("");
  m_cType  = _T('\0');
}

CuTblConstraint::CuTblConstraint(const CuTblConstraint* pRefConstraint)
 :CuMultFlag(pRefConstraint)
{
  // Flags array already duplicated by base class

  m_cType  = pRefConstraint->m_cType;
  m_strIndex = pRefConstraint->m_strIndex;
}

void CuTblConstraint::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_cType;
    ar << m_strIndex;
  }
  else {
    ar >> m_cType;
    ar >> m_strIndex;
  }
}

CString CuTblConstraint::CaptionFromType()
{
  switch (m_cType) {
    case _T('r'):
    case _T('R'):
      return _T("Reference (Foreign Key)");
      break;

    case _T('p'):
    case _T('P'):
      return _T("Primary Key");
      break;

    case _T('u'):
    case _T('U'):
      return _T("Unique");
      break;

    case _T('c'):
    case _T('C'):
      return _T("Check");
      break;

    default:
      return CString(m_cType);
      break;
  }
  ASSERT (FALSE);
  return _T("Error");
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayTblConstraints

IMPLEMENT_SERIAL (CuArrayTblConstraints, CuArrayMultFlags, 1)

void CuArrayTblConstraints::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuTblConstraint* CuArrayTblConstraints::operator [] (int index)
{
  CuTblConstraint* pTblConstraint;
  pTblConstraint = (CuTblConstraint*)m_aFlagObjects[index];
  ASSERT (pTblConstraint);
  ASSERT (pTblConstraint->IsKindOf(RUNTIME_CLASS(CuTblConstraint)));
  return pTblConstraint;
}

void CuArrayTblConstraints::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuTblConstraint)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayTblConstraints::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuTblConstraint)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuStarTblColumn

IMPLEMENT_SERIAL (CuStarTblColumn, CuMultFlag, 1)

CuStarTblColumn::CuStarTblColumn()
: CuMultFlag()
{
  InitialAllocateFlags(NBTBLCOLCOLUMNS);

  m_csType        = "";
  m_csLdbName     = "";
}

CuStarTblColumn::CuStarTblColumn(LPCTSTR name)
  : CuMultFlag(name, TRUE)
{
  InitialAllocateFlags(NBTBLCOLCOLUMNS);

  m_csType        = "";
  m_csLdbName     = "";
}

CuStarTblColumn::CuStarTblColumn(LPCTSTR name, LPCTSTR type, BOOL bNullable, BOOL bWithDefault, LPCTSTR ldbColName)
  : CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(NBTBLCOLCOLUMNS);

  m_csType        = type;
  m_csLdbName     = ldbColName;

  // Immediate update of several flags
  if (bNullable)
    m_aFlags[IndexFromFlagType(FLAG_COL_NULLABLE)] = TRUE;
  if (bWithDefault)
    m_aFlags[IndexFromFlagType(FLAG_COL_WITHDEFAULT)] = TRUE;
}

CuStarTblColumn::CuStarTblColumn(const CuStarTblColumn* pRefStarTblColumn)
 :CuMultFlag(pRefStarTblColumn)
{
  // Flags array already duplicated by base class

  m_csType        = pRefStarTblColumn->m_csType        ;
  m_csLdbName     = pRefStarTblColumn->m_csLdbName     ;
}

void CuStarTblColumn::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csType;
    ar << m_csLdbName;
  }
  else {
    ar >> m_csType;
    ar >> m_csLdbName;
  }
}

int CuStarTblColumn::IndexFromFlagType(int flagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  switch (flagType) {
    case FLAG_COL_NULLABLE:
      return 0;
    case FLAG_COL_WITHDEFAULT:
      return 1;
    default:
      ASSERT (FALSE);
      return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayStarTblColumns

IMPLEMENT_SERIAL (CuArrayStarTblColumns, CuArrayMultFlags, 1)

void CuArrayStarTblColumns::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuStarTblColumn* CuArrayStarTblColumns::operator [] (int index)
{
  CuStarTblColumn* pStarTblColumn;
  pStarTblColumn = (CuStarTblColumn*)m_aFlagObjects[index];
  ASSERT (pStarTblColumn);
  ASSERT (pStarTblColumn->IsKindOf(RUNTIME_CLASS(CuStarTblColumn)));
  return pStarTblColumn;
}

void CuArrayStarTblColumns::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuStarTblColumn)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayStarTblColumns::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuStarTblColumn)));
  CuArrayMultFlags::Add(newMultFlag);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuOidtOidtTblConstraint


IMPLEMENT_SERIAL (CuOidtTblConstraint, CuMultFlag, 1)

CuOidtTblConstraint::CuOidtTblConstraint()
: CuMultFlag()
{
  InitialAllocateFlags(DUMMYNBCONSTRAINTCOL);

  m_csParentTbl   = _T("");
  m_csDeleteMode  = _T("");
  m_csColumns     = _T("");
}

CuOidtTblConstraint::CuOidtTblConstraint(LPCTSTR name, LPCTSTR parentTable, LPCTSTR deleteMode, LPCTSTR columns)
:CuMultFlag(name, FALSE)
{
  InitialAllocateFlags(DUMMYNBCONSTRAINTCOL);

  m_csParentTbl   = parentTable;
  m_csDeleteMode  = deleteMode;
  m_csColumns     = columns;
}

CuOidtTblConstraint::CuOidtTblConstraint(LPCTSTR name)
:CuMultFlag(name, TRUE)  // error/No item
{
  InitialAllocateFlags(DUMMYNBCONSTRAINTCOL);

  m_csParentTbl   = _T("");
  m_csDeleteMode  = _T("");
  m_csColumns     = _T("");
}

CuOidtTblConstraint::CuOidtTblConstraint(const CuOidtTblConstraint* pRefConstraint)
 :CuMultFlag(pRefConstraint)
{
  // Flags array already duplicated by base class
  m_csParentTbl   = pRefConstraint->m_csParentTbl ;
  m_csDeleteMode  = pRefConstraint->m_csDeleteMode;
  m_csColumns     = pRefConstraint->m_csColumns   ;
}

void CuOidtTblConstraint::Serialize (CArchive& ar)
{
  CuMultFlag::Serialize(ar);

  if (ar.IsStoring()) {
    ar << m_csParentTbl;
    ar << m_csDeleteMode;
    ar << m_csColumns;
  }
  else {
    ar >> m_csParentTbl;
    ar >> m_csDeleteMode;
    ar >> m_csColumns;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayOidtTblConstraints

IMPLEMENT_SERIAL (CuArrayOidtTblConstraints, CuArrayMultFlags, 1)

void CuArrayOidtTblConstraints::Serialize (CArchive& ar)
{
  CuArrayMultFlags::Serialize (ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

CuOidtTblConstraint* CuArrayOidtTblConstraints::operator [] (int index)
{
  CuOidtTblConstraint* pOidtTblConstraint;
  pOidtTblConstraint = (CuOidtTblConstraint*)m_aFlagObjects[index];
  ASSERT (pOidtTblConstraint);
  ASSERT (pOidtTblConstraint->IsKindOf(RUNTIME_CLASS(CuOidtTblConstraint)));
  return pOidtTblConstraint;
}

void CuArrayOidtTblConstraints::Add(CuMultFlag* pNewMultFlag)
{
  ASSERT (pNewMultFlag->IsKindOf(RUNTIME_CLASS(CuOidtTblConstraint)));
  CuArrayMultFlags::Add(pNewMultFlag);
}

void CuArrayOidtTblConstraints::Add(CuMultFlag& newMultFlag)
{
  ASSERT (newMultFlag.IsKindOf(RUNTIME_CLASS(CuOidtTblConstraint)));
  CuArrayMultFlags::Add(newMultFlag);
}
