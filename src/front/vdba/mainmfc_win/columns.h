/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : columns.h                                                             //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    classes for table columns management in right pane of dom                        //
//                                                                                     //
//  16-Nov-2000 (schph01)
//    sir 102821 Comment on table and columns.
// 20-Aug-2008 (whiro01)
//    Remove redundant <afx...> include which is already in "stdafx.h"
****************************************************************************************/
#ifndef COLUMNS_HEADER
#define COLUMNS_HEADER

#include "multflag.h"   // Base classes

//////////////////////////////////////////////////////////////////
// No Intermediary base class

//////////////////////////////////////////////////////////////////
// Classes for Table Columns

#define NBTBLCOLCOLUMNS  2

#define FLAG_COL_NULLABLE    1
#define FLAG_COL_WITHDEFAULT 2

class CuTblColumn: public CuMultFlag
{
  DECLARE_SERIAL (CuTblColumn)

public:
  CuTblColumn(LPCTSTR name, LPCTSTR type, int primKeyRank, BOOL bNullable, BOOL bWithDefault, LPCTSTR defSpec,LPCTSTR ColComment);
  CuTblColumn(const CuTblColumn* pRefColumn);
  CuTblColumn(LPCTSTR name);    // Error/No item
  virtual ~CuTblColumn() {}

// for serialisation
  CuTblColumn();
  virtual void Serialize (CArchive& ar);

// access to members
  CString GetType()         { return m_csType; }
  int     GetPrimKeyRank()  { return m_primKeyRank; }
  BOOL    IsNullable()      { return m_aFlags[IndexFromFlagType(FLAG_COL_NULLABLE)]; }
  BOOL    HasDefault()      { return m_aFlags[IndexFromFlagType(FLAG_COL_WITHDEFAULT)]; }
  CString GetDefSpec()      { return m_csDefSpec; }
  CString GetColumnComment(){ return m_csColumnComment; }

  virtual int IndexFromFlagType(int FlagType);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuTblColumn(this); }

  // No sort for columns

private:
  CString m_csType;
  int     m_primKeyRank;
  CString m_csDefSpec;
  CString m_csColumnComment;
};

class CuArrayTblColumns: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayTblColumns)

public:

// operations
  CuTblColumn* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Table Constraints
//
// We could use a CTypedPtrArray < CObArray, CuTblConstraint >,
// but this leads to extra code in copy, destructor, etc...
// So let's derive from CuMultFlag/CuArrayMultFlags

#define DUMMYNBCONSTRAINTCOL  1

class CuTblConstraint: public CuMultFlag
{
  DECLARE_SERIAL (CuTblConstraint)

public:
  CuTblConstraint(char cType, LPCTSTR text, LPCTSTR lpszIndex);
  CuTblConstraint(const CuTblConstraint* pRefConstraint);
  CuTblConstraint(LPCTSTR name);    // Error/No item
  virtual ~CuTblConstraint() {}

// for serialisation
  CuTblConstraint();
  virtual void Serialize (CArchive& ar);

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuTblConstraint(this); }

// access to members
  TCHAR   GetType()         { return m_cType; }
  CString GetText()         { return m_csText; }
  CString GetIndex()        { return m_strIndex; }
  CString CaptionFromType();

private:
  TCHAR   m_cType;
  CString m_csText;
  CString m_strIndex;
};

class CuArrayTblConstraints: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayTblConstraints)

public:

// operations
  CuTblConstraint* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


//////////////////////////////////////////////////////////////////
// Classes for Star Table Columns

class CuStarTblColumn: public CuMultFlag
{
  DECLARE_SERIAL (CuStarTblColumn)

public:
  CuStarTblColumn(LPCTSTR name, LPCTSTR type, BOOL bNullable, BOOL bWithDefault, LPCTSTR ldbColName);
  CuStarTblColumn(const CuStarTblColumn* pRefColumn);
  CuStarTblColumn(LPCTSTR name);    // Error/No item
  virtual ~CuStarTblColumn() {}

// for serialisation
  CuStarTblColumn();
  virtual void Serialize (CArchive& ar);

// access to members
  CString GetType()         { return m_csType; }
  BOOL    IsNullable()      { return m_aFlags[IndexFromFlagType(FLAG_COL_NULLABLE)]; }
  BOOL    HasDefault()      { return m_aFlags[IndexFromFlagType(FLAG_COL_WITHDEFAULT)]; }
  CString GetLdbColName()   { return m_csLdbName; }

  virtual int IndexFromFlagType(int FlagType);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuStarTblColumn(this); }

  // No sort for columns

private:
  CString m_csType;
  CString m_csLdbName;
};

class CuArrayStarTblColumns: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayStarTblColumns)

public:

// operations
  CuStarTblColumn* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

class CuOidtTblConstraint: public CuMultFlag
{
  DECLARE_SERIAL (CuOidtTblConstraint)

public:
  CuOidtTblConstraint(LPCTSTR name, LPCTSTR parentTable, LPCTSTR deleteMode, LPCTSTR columns);
  CuOidtTblConstraint(const CuOidtTblConstraint* pRefConstraint);
  CuOidtTblConstraint(LPCTSTR name);    // Error/No item
  virtual ~CuOidtTblConstraint() {}

// for serialisation
  CuOidtTblConstraint();
  virtual void Serialize (CArchive& ar);

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuOidtTblConstraint(this); }

// access to members
  CString GetParentTable()  { return m_csParentTbl; }
  CString GetDeleteMode()   { return m_csDeleteMode; }
  CString GetColumns()      { return m_csColumns; }

private:
  CString m_csParentTbl;
  CString m_csDeleteMode;
  CString m_csColumns;
};

class CuArrayOidtTblConstraints: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayOidtTblConstraints)

public:

// operations
  CuOidtTblConstraint* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


#endif  // COLUMNS_HEADER

