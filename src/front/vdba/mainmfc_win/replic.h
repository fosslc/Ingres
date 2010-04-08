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

#ifndef REPLIC_HEADER
#define REPLIC_HEADER

#include "multflag.h"   // Base classes

//////////////////////////////////////////////////////////////////
// No Intermediary base class

//////////////////////////////////////////////////////////////////
// Classes for Cdds Paths
// We could use a CTypedPtrArray < CObArray, CuCddsPath >,
// but this leads to extra code in copy, destructor, etc...
// So let's derive from CuMultFlag/CuArrayMultFlags

#define DUMMY_NBCDDSPATHCOLUMNS 1

class CuCddsPath: public CuMultFlag
{
  DECLARE_SERIAL (CuCddsPath)

public:
  CuCddsPath(LPCTSTR originalName, LPCTSTR localName, LPCTSTR targetName);
  CuCddsPath(const CuCddsPath* pRefCddsPath);
  CuCddsPath(LPCTSTR originalName);    // Error/No item
  virtual ~CuCddsPath() {}

// for serialisation
  CuCddsPath();
  virtual void Serialize (CArchive& ar);

// access to members
  CString GetLocalName()         { return m_csLocal; }
  CString GetTargetName()        { return m_csTarget; }

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuCddsPath(this); }

  // No sort for columns

private:
  CString m_csLocal;
  CString m_csTarget;
};

class CuArrayCddsPaths: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayCddsPaths)

public:

// operations
  CuCddsPath* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Cdds Database information
//
// We could use a CTypedPtrArray < CObArray, CuCddsDbInfo >,
// but this leads to extra code in copy, destructor, etc...
// So let's derive from CuMultFlag/CuArrayMultFlags

#define DUMMY_NBCDDSDBINFOCOLUMNS 1

class CuCddsDbInfo: public CuMultFlag
{
  DECLARE_SERIAL (CuCddsDbInfo)

public:
  CuCddsDbInfo(LPCTSTR vnodeName, LPCTSTR dbName, int no, int type, int serverNo);
  CuCddsDbInfo(const CuCddsDbInfo* pRefCddsDbInfo);
  CuCddsDbInfo(LPCTSTR name);    // Error/No item
  virtual ~CuCddsDbInfo() {}

// for serialisation
  CuCddsDbInfo();
  virtual void Serialize (CArchive& ar);

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuCddsDbInfo(this); }

// access to members
  int     GetNumber()       { return m_no; }
  CString GetDbName()       { return m_csDbName; }
  int     GetType()         { return m_type; }
  int     GetServerNo()     { return m_srvNo; }

// extra methods
  CString GetTypeString();

private:
  int     m_no;
  CString m_csDbName;
  int     m_type;
  int     m_srvNo;
};

class CuArrayCddsDbInfos: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayCddsDbInfos)

public:

// operations
  CuCddsDbInfo* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);

// Specific for this class
  CuCddsDbInfo* FindFromDbNo(int dbNo);
};


//////////////////////////////////////////////////////////////////
// Classes for Cdds Columns of a registered table

#define NB_CDDS_TBLCOL_COLUMNS 1

#define FLAG_CDDSTBLCOL_REPLIC 1

class CuCddsTableCol: public CuMultFlag
{
  DECLARE_SERIAL (CuCddsTableCol)

public:
  CuCddsTableCol(LPCTSTR name, int repKey, BOOL bReplicated);
  CuCddsTableCol(const CuCddsTableCol* pRefCddsTableCol);
  CuCddsTableCol(LPCTSTR name);    // Error/No item
  virtual ~CuCddsTableCol() {}

// for serialisation
  CuCddsTableCol();
  virtual void Serialize (CArchive& ar);

// access to members
  int     GetRepKey()       { ASSERT (m_iRepKey != -1); return m_iRepKey; }
  BOOL    IsReplicated()    { return m_aFlags[IndexFromFlagType(FLAG_CDDSTBLCOL_REPLIC)]; }

  virtual int IndexFromFlagType(int FlagType);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuCddsTableCol(this); }

  // No sort for columns

private:
  int     m_iRepKey;
};

class CuArrayCddsTableCols: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayCddsTableCols)

public:

// operations
  CuCddsTableCol* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


//////////////////////////////////////////////////////////////////
// Classes for Cdds Registered Tables

#define NB_CDDS_TBL_COLUMNS 2

#define FLAG_CDDSTBL_SUPPORT    1
#define FLAG_CDDSTBL_ACTIVATED  2

class CuCddsTable: public CuMultFlag
{
  DECLARE_SERIAL (CuCddsTable)

public:
  CuCddsTable(LPCTSTR name, LPCTSTR schema, BOOL bSupport, BOOL bActivated, LPCTSTR lookup, LPCTSTR priority);
  CuCddsTable(const CuCddsTable* pRefCddsTable);
  CuCddsTable(LPCTSTR name);    // Error/No item
  virtual ~CuCddsTable() {}

// for serialisation
  CuCddsTable();
  virtual void Serialize (CArchive& ar);

// access to members
  CString GetSchema()       { return m_csSchema; }
  BOOL    IsSupport()       { return m_aFlags[IndexFromFlagType(FLAG_CDDSTBL_SUPPORT)]; }
  BOOL    IsActivated()     { return m_aFlags[IndexFromFlagType(FLAG_CDDSTBL_ACTIVATED)]; }
  CString GetLookup()       { return m_csLookup; }
  CString GetPriority()     { return m_csPriority; }

  virtual int IndexFromFlagType(int FlagType);

  // Special methods for list of columns
  CuArrayCddsTableCols& GetACols() { return m_aCols; }
  void AddColumnInfo(CuCddsTableCol& refCddsColInfo) { m_aCols.Add(refCddsColInfo); }

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuCddsTable(this); }

  // No sort for columns

private:
  CString m_csSchema;
  CString m_csLookup;
  CString m_csPriority;
  CuArrayCddsTableCols  m_aCols;
};

class CuArrayCddsTables: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayCddsTables)

public:

// operations
  CuCddsTable* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

#endif  // REPLIC_HEADER

