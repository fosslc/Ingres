/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : MultFlag.cpp                                                          //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    base classes for lists of objects with multiple flags                            //
//                                                                                     //
// History:                                                                            //
//   09-Jun-2010 (drivi01)                                                             //
//     Update constructor for CuMultFlag to take additional parameter                  //
//     for table type.  Update Duplicate option to also duplicate table                //
//     type and initialize table type in the constructor.                              //
****************************************************************************************/

#include "stdafx.h"

#include "multflag.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////
// Specialized list ctrl, with checkmarks in relevant columns
// Line selection masqued out, checkmarks not editable

BEGIN_MESSAGE_MAP(CuListCtrlCheckmarks, CuListCtrl)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

void CuListCtrlCheckmarks::OnLButtonDown(UINT nFlags, CPoint point) 
{
  // Prevent any selection by not calling base class method
  //CListCtrl::OnLButtonDown(nFlags, point);
}

void CuListCtrlCheckmarks::OnRButtonDown(UINT nFlags, CPoint point) 
{
  // Prevent any selection by not calling base class method
  //CListCtrl::OnRButtonDown(nFlags, point);
}

void InitializeColumns(CuListCtrlCheckmarks& refListCtrl, CHECKMARK_COL_DESCRIBE aColumns[], int nbColumns)
{
  LV_COLUMN lvcolumn;
  int i;
  memset (&lvcolumn, 0, sizeof (LV_COLUMN));
  lvcolumn.mask = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
  for (i=0; i<nbColumns; i++)
  {
      CString strHeader;
      #ifdef USE_RESOURCE_STRING
      strHeader.LoadString (aColumns[i].stringId);
      #else
      strHeader = aColumns[i].szCaption;
      #endif
      if (aColumns[i].bCheckmark)
        lvcolumn.fmt = LVCFMT_CENTER;
      else
        lvcolumn.fmt = LVCFMT_LEFT;
      lvcolumn.pszText = (LPTSTR)(LPCTSTR)strHeader;
      lvcolumn.iSubItem = i;
      if (aColumns[i].CustomWidth == -1) {
        lvcolumn.cx = refListCtrl.GetStringWidth(lvcolumn.pszText);
        lvcolumn.cx += 15;  // fixed margin for sides
        //lvcolumn.cx += ( (lvcolumn.cx + 4) / 5 ); // add 20% rounded up
      }
      else
        lvcolumn.cx = aColumns[i].CustomWidth;
      if (aColumns[i].bCheckmark)
        refListCtrl.InsertColumn (i, &lvcolumn, TRUE);  // "Checkboxable"
      else
        refListCtrl.InsertColumn (i, &lvcolumn);    // Not checkboxable
  }
}


//////////////////////////////////////////////////////////////////
// Specialized list ctrl, with checkmarks in relevant columns
// Line selection possible, but checkmarks not editables
BEGIN_MESSAGE_MAP(CuListCtrlCheckmarksWithSel, CuListCtrlCheckmarks)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

void CuListCtrlCheckmarksWithSel::OnLButtonDown(UINT nFlags, CPoint point) 
{
  // Simply authorize selection
  CListCtrl::OnLButtonDown(nFlags, point);
}

void CuListCtrlCheckmarksWithSel::OnRButtonDown(UINT nFlags, CPoint point) 
{
  // Simply authorize selection
  CListCtrl::OnRButtonDown(nFlags, point);
}

/////////////////////////////////////////////////////////////////////////////////////////
// CuMultFlag

IMPLEMENT_SERIAL (CuMultFlag, CObject, 1)

CuMultFlag::CuMultFlag()
{
  m_bSpecialItem        = FALSE;
  m_strName             = "";

  m_nbFlags = 0;
  m_aFlags = NULL;
  m_aValues = NULL;         // For numeric values associated with flags
}

CuMultFlag::CuMultFlag(LPCTSTR name, BOOL bSpecialItem, int tbltype)
{
  m_bSpecialItem        = bSpecialItem;
  m_iTblType		= tbltype;

  // Name can be NULL! (unnamed object)
  if (name[0])
    m_strName = name;
  else
    m_strName = "";
  m_nbFlags = 0;
  m_aFlags = NULL;
  m_aValues = NULL;         // For numeric values associated with flags
}

CuMultFlag::~CuMultFlag()
{
  // Nothing has been allocated if special item
  if (!m_bSpecialItem) {
    ASSERT (m_nbFlags > 0);
    ASSERT (m_aFlags);
    ASSERT (m_aValues);
  }

  if (m_aFlags)
    delete m_aFlags;
  if (m_aValues)
    delete m_aValues;
}

CuMultFlag::CuMultFlag(const CuMultFlag* pRefFlag)
{
  ASSERT (pRefFlag->m_nbFlags > 0);
  ASSERT (pRefFlag->m_aFlags);
  ASSERT (pRefFlag->m_aValues);

  m_bSpecialItem        = pRefFlag->m_bSpecialItem      ;
  m_strName             = pRefFlag->m_strName           ;
  m_iTblType			= pRefFlag->m_iTblType			;

  m_nbFlags = pRefFlag->m_nbFlags;
  m_aFlags = new BOOL[m_nbFlags];
  memset (m_aFlags, 0, m_nbFlags*sizeof(BOOL));
  memcpy(m_aFlags, pRefFlag->m_aFlags, m_nbFlags*sizeof(BOOL));
  m_aValues = new DWORD[m_nbFlags];
  memset (m_aValues, 0, m_nbFlags*sizeof(DWORD));
  memcpy(m_aValues, pRefFlag->m_aValues, m_nbFlags*sizeof(DWORD));
}

void CuMultFlag::Serialize (CArchive& ar)
{
  CObject::Serialize(ar);

  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);
  ASSERT (m_aValues);

  if (ar.IsStoring()) {
    ar << m_bSpecialItem;
    ar << m_strName;

    ar << m_nbFlags;
    ar.Write(m_aFlags, m_nbFlags*sizeof(BOOL));
    ar.Write(m_aValues, m_nbFlags*sizeof(DWORD));
  }
  else {
    ar >> m_bSpecialItem;
    ar >> m_strName;

    ar >> m_nbFlags;
    ar.Read(m_aFlags, m_nbFlags*sizeof(BOOL));
    ar.Read(m_aValues, m_nbFlags*sizeof(DWORD));
  }
}

void CuMultFlag::InitialAllocateFlags(int size)
{
  ASSERT (size > 0);
  m_nbFlags = size;
  m_aFlags = new BOOL[m_nbFlags];
  memset (m_aFlags, 0, m_nbFlags*sizeof(BOOL));
  m_aValues = new DWORD[m_nbFlags];
  memset (m_aValues, 0, m_nbFlags*sizeof(DWORD));
}

BOOL CuMultFlag::GetFlag(int FlagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);
  ASSERT (m_aValues);

  ASSERT (!m_bSpecialItem);

  int index = IndexFromFlagType(FlagType);
  ASSERT (index >= 0);
  ASSERT (index < m_nbFlags);
  if (index >= 0 && index < m_nbFlags)
    return m_aFlags[index];
  else
    return FALSE;   // Erroneous type defaulted to FALSE
}

BOOL CuMultFlag::SetValue(int FlagType, DWORD dwVal)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);
  ASSERT (m_aValues);

  ASSERT (!m_bSpecialItem);

  int index = IndexFromFlagType(FlagType);
  ASSERT (index >= 0);
  ASSERT (index < m_nbFlags);
  ASSERT (m_aFlags[index]);
  if (m_aFlags[index]) {
    m_aValues[index] = dwVal;
    return TRUE;
  }
  return FALSE;
}

DWORD CuMultFlag::GetValue(int FlagType)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);
  ASSERT (m_aValues);

  ASSERT (!m_bSpecialItem);

  int index = IndexFromFlagType(FlagType);
  ASSERT (index >= 0);
  ASSERT (index < m_nbFlags);
  if (index >= 0 && index < m_nbFlags)
    return m_aValues[index];
  else
    return 0;   // Erroneous type defaulted to 0
}

void CuMultFlag::MergeFlags(CuMultFlag *pNewFlag)
{
  ASSERT (m_nbFlags > 0);
  ASSERT (m_aFlags);

  ASSERT (!m_bSpecialItem);

  for (int index = 0; index < m_nbFlags; index++ ) {
    m_aFlags[index] = m_aFlags[index] || pNewFlag->m_aFlags[index];

    // value: give priority to incoming flags
    if (pNewFlag->m_aValues[index])
      m_aValues[index] = pNewFlag->m_aValues[index];
  }
}

BOOL CuMultFlag::IsSimilar(CuMultFlag *pSearchedObject)
{
  if (GetStrName() == pSearchedObject->GetStrName())
    return TRUE;
  else
    return FALSE;
}

int CALLBACK CuMultFlag::CompareFlags(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  CuMultFlag* pMultFlag1 = (CuMultFlag*) lParam1;
  CuMultFlag* pMultFlag2 = (CuMultFlag*) lParam2;
  ASSERT (pMultFlag1);
  ASSERT (pMultFlag2);
  ASSERT (pMultFlag1->IsKindOf(RUNTIME_CLASS(CuMultFlag)));
  ASSERT (pMultFlag2->IsKindOf(RUNTIME_CLASS(CuMultFlag)));

  ASSERT (pMultFlag1->m_nbFlags > 0);
  ASSERT (pMultFlag1->m_nbFlags == pMultFlag2->m_nbFlags);

  // lparamSort contains index in array of flags
  ASSERT (lParamSort >= 0);
  ASSERT (lParamSort < pMultFlag1->m_nbFlags);
  BOOL b1 = pMultFlag1->m_aFlags[lParamSort];
  BOOL b2 = pMultFlag2->m_aFlags[lParamSort];

  if (b1 && !b2)
    return -1;      // Checked before unchecked
  if (!b1 && b2)
    return +1;      // Unchecked after checked

  // Same state: if eligible, compare Value, return biggest
  if (!b1 && !b2)
    return 0;

  DWORD dw1 = pMultFlag1->m_aValues[lParamSort];
  DWORD dw2 = pMultFlag2->m_aValues[lParamSort];
  if (dw1 > dw2)
    return -1;
  if (dw2 > dw1)
    return +1;
  // same state and same value
  return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
// CuArrayMultFlags

IMPLEMENT_SERIAL (CuArrayMultFlags, CObject, 1)

CuArrayMultFlags::CuArrayMultFlags()
{
  m_aFlagObjects.SetSize(10, 10);
  m_aFlagObjects.RemoveAll();     // otherwise initial items not initialized, leads to gpfs
}

CuArrayMultFlags::~CuArrayMultFlags()
{
  RemoveAll();
}

void CuArrayMultFlags::Serialize (CArchive& ar)
{
  CObject::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
  m_aFlagObjects.Serialize(ar);
}

//
// operations on list
//

int  CuArrayMultFlags::GetCount()
{
  return m_aFlagObjects.GetSize();
}

CuMultFlag* CuArrayMultFlags::operator [] (int index)
{
  CuMultFlag* pFlag;
  pFlag = (CuMultFlag*)m_aFlagObjects[index];
  ASSERT (pFlag);
  ASSERT (pFlag->IsKindOf(RUNTIME_CLASS(CuMultFlag)));
  return pFlag;
}

void CuArrayMultFlags::RemoveAll()
{
  int size = m_aFlagObjects.GetSize();
  for (int cpt=0; cpt<size; cpt++)
    delete m_aFlagObjects[cpt];
  m_aFlagObjects.RemoveAll();  // Redundant
}

void CuArrayMultFlags::Add(CuMultFlag* pNewFlag)
{
  // ASSUMPTION : received parameter has been allocated by new
  // ---> we don't duplicate it, and it is assumed NOT TO BE DELETED by caller
  ASSERT (pNewFlag);
  m_aFlagObjects.Add(pNewFlag);
}

void CuArrayMultFlags::Add(CuMultFlag& newFlag)
{
  // ASSUMPTION : received parameter has been allocated on the stack
  // ---> we NEED TO DUPLICATE it since the original will be deleted from the stack
  // when the caller leaves
  CuMultFlag *pDupFlag = newFlag.Duplicate();
  ASSERT (pDupFlag);
  if (pDupFlag)
    m_aFlagObjects.Add(pDupFlag);
}

void CuArrayMultFlags::Copy(const CuArrayMultFlags& refListFlags)
{
  RemoveAll();  // in case

  // Cannot use copy,
  // since it would only copy the pointers
  int size = refListFlags.m_aFlagObjects.GetSize();
  for (int cpt=0; cpt<size; cpt++) {
    CuMultFlag* pRefFlag = (CuMultFlag*)refListFlags.m_aFlagObjects[cpt];
    ASSERT (pRefFlag);
    CuMultFlag* pFlag = pRefFlag->Duplicate();   // allocates with new
    Add(pFlag);                                // stores the received ptr, so we don't need to delete pFlag afterwards
  }
}

//
// Operations on items
//

CuMultFlag* CuArrayMultFlags::Find(CuMultFlag* pSearchedFlag)
{
  int size = m_aFlagObjects.GetSize();
  for (int cpt=0; cpt<size; cpt++) {
    CuMultFlag* pFlag = (CuMultFlag*)m_aFlagObjects[cpt];
    ASSERT (pFlag);
    ASSERT (pFlag->IsKindOf(RUNTIME_CLASS(CuMultFlag)));  // CuMultFlag or derived from CuMultFlag
    if (pFlag->IsSimilar(pSearchedFlag))
      return pFlag;
  }
  return 0;   // Not found
}

void CuArrayMultFlags::Merge(CuMultFlag* pFoundFlag, CuMultFlag* pNewFlag)
{
  ASSERT (pFoundFlag->IsSimilar(pNewFlag));
  pFoundFlag->MergeFlags(pNewFlag);
}

