/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : MultCol.h                                                             //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
// Utility classes for multicolumn dom detail windows:                                 //
//    Name Only,                                                                       //
//    Name With Owner,                                                                 //
//    Name With Integer value,                                                         //
//    etc.                                                                             //
//                                                                                     //
****************************************************************************************/

#ifndef MULTCOL_HEADER
#define MULTCOL_HEADER

#include "multflag.h"   // Base classes

//////////////////////////////////////////////////////////////////
// Classes for Object having "Name Only"

#define DUMMY_NB_NAMEONLY_COLUMNS 1

class CuNameOnly: public CuMultFlag
{
  DECLARE_SERIAL (CuNameOnly)

public:
  CuNameOnly(LPCTSTR name, BOOL bSpecial);    // bSpecial TRUE ---> Error/No item
  CuNameOnly(const CuNameOnly* pRefNameOnly);
  virtual ~CuNameOnly() {}

// for serialisation
  CuNameOnly();
  virtual void Serialize (CArchive& ar);

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuNameOnly(this); }

  // For sort
  static int CALLBACK CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:
  // No members
};

class CuArrayNameOnly: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayNameOnly)

public:

// operations
  CuNameOnly* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Object having "Name With Number"

#define DUMMY_NB_NAMEWITHNUMBER_COLUMNS 1

class CuNameWithNumber: public CuMultFlag
{
  DECLARE_SERIAL (CuNameWithNumber)

public:
  CuNameWithNumber(LPCTSTR name, int  number);
  CuNameWithNumber(const CuNameWithNumber* pRefNameWithNumber);
  CuNameWithNumber(LPCTSTR name);    // Error/No item
  virtual ~CuNameWithNumber() {}

// for serialisation
  CuNameWithNumber();
  virtual void Serialize (CArchive& ar);

// access to members
  int GetNumber() { return m_number; }

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuNameWithNumber(this); }

  // For sort
  static int CALLBACK CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareNumbers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:
  int m_number;
};

class CuArrayNameWithNumber: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayNameWithNumber)

public:

// operations
  CuNameWithNumber* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Object having "Name With Owner"
// Modified Feb. 18, 98 to manage a flag which will be used for star objects

#define NB_NAMEWITHOWNER_COLUMNS 1
#define FLAG_STARTYPE          100

class CuNameWithOwner: public CuMultFlag
{
  DECLARE_SERIAL (CuNameWithOwner)

public:
  CuNameWithOwner(LPCTSTR name, LPCTSTR owner, BOOL bFlag = FALSE);
  CuNameWithOwner(const CuNameWithOwner* pRefNameWithOwner);
  CuNameWithOwner(LPCTSTR originalName);    // Error/No item
  virtual ~CuNameWithOwner() {}

// for serialisation
  CuNameWithOwner();
  virtual void Serialize (CArchive& ar);

// access to members
  CString GetOwnerName()         { return m_csOwner; }

  virtual int IndexFromFlagType(int FlagType);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuNameWithOwner(this); }

  // For sort
  static int CALLBACK CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareOwners(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:
  CString m_csOwner;
};

class CuArrayNameWithOwner: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayNameWithOwner)

public:

// operations
  CuNameWithOwner* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


#endif  // MULTCOL_HEADER
