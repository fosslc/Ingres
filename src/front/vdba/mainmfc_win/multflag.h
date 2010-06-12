/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : MultFlag.h                                                            //
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
#ifndef MULTFLAG_HEADER
#define MULTFLAG_HEADER

#include "calsctrl.h"  // CuListCtrl

//////////////////////////////////////////////////////////////////
// Specialized list ctrl, with checkmarks in relevant columns
// Line selection masqued out, checkmarks not editable

class CuListCtrlCheckmarks: public CuListCtrl
{
public:
  CuListCtrlCheckmarks() : CuListCtrl() {}
protected:
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
  DECLARE_MESSAGE_MAP()
};

// structure to describe columns
typedef struct sCheckmarkColDescribe {
  #ifdef USE_RESOURCE_STRING
    UINT stringId;
  #else
    TCHAR szCaption[32];
  #endif
  BOOL  bCheckmark;     // False means text column (obj name, ...)
  int   CustomWidth;    // -1 means it has to be calculated at runtime
} CHECKMARK_COL_DESCRIBE, FAR *LPCHECKMARK_COL_DESCRIBE;

// function to fill columns starting from their description
void InitializeColumns(CuListCtrlCheckmarks& refListCtrl, CHECKMARK_COL_DESCRIBE aColumns[], int nbColumns);


//////////////////////////////////////////////////////////////////
// Specialized list ctrl, with checkmarks in relevant columns
// Line selection possible, but checkmarks not editables

class CuListCtrlCheckmarksWithSel: public CuListCtrlCheckmarks
{
public:
  CuListCtrlCheckmarksWithSel() : CuListCtrlCheckmarks() {}
protected:
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
  DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////
// Multiple-flag Object

class CuMultFlag: public CObject
{
  DECLARE_SERIAL (CuMultFlag)

public:
  CuMultFlag(LPCTSTR name, BOOL bSpecialItem, int tbltype = 0);
  CuMultFlag(const CuMultFlag* pRefFlag);
  virtual ~CuMultFlag();

// for serialisation
  CuMultFlag();
  virtual void Serialize (CArchive& ar);

// access to members
  BOOL    IsSpecialItem()         { return m_bSpecialItem; }
  int	  GetTableType()	  { return m_iTblType; }
  CString GetStrName()            { return m_strName; } 
  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

// flags array management
  void    InitialAllocateFlags(int size);
  void    MergeFlags(CuMultFlag *pNewFlag);
  BOOL    GetFlag(int FlagType);

  /* void    SetFlag(int FlagType, BOOL bSet); */

  // complimentary for associated values
  BOOL    SetValue(int FlagType, DWORD dwVal);
  DWORD   GetValue(int FlagType);

  virtual BOOL IsSimilar(CuMultFlag *pSearchedObject);

  // Duplication
  virtual CuMultFlag* Duplicate() { ASSERT(FALSE); return 0; }

  // For sort
  static int CALLBACK CompareFlags(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:
  BOOL    m_bSpecialItem;     // TRUE if error/none, FALSE if regular
  int	  m_iTblType;

protected:
  CString m_strName;          // Name

protected:
  int     m_nbFlags;
  BOOL    *m_aFlags;
  DWORD   *m_aValues;         // For numeric values associated with flags
};

//////////////////////////////////////////////////////////////////
// list of Multiple-flag Objects

class CuArrayMultFlags: public CObject
{
  DECLARE_SERIAL (CuArrayMultFlags)

public:
  CuArrayMultFlags();
  virtual ~CuArrayMultFlags();

// operations on list
  int  GetCount();
  CuMultFlag* operator [] (int index);
  void RemoveAll();
  void InitialSort();
  void Copy(const CuArrayMultFlags& refListFlags);

  // add to list : virtualized methods so the derived list class will test
  // the runtime class of received parameters in debug version
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

  // Operations on items
  CuMultFlag* Find(CuMultFlag* pSearchedFlag);
  void Merge(CuMultFlag* pFoundFlag, CuMultFlag* pNewFlag);

// for serialisation
  virtual void Serialize (CArchive& ar);

protected:
  CObArray  m_aFlagObjects;
};

#endif  // MULTFLAG_HEADER

