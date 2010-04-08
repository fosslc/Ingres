/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : Grantees.h
**
**  Project  : CA-OpenIngres/VDBA
**
**  classes for grantees management in right pane of dom
**
**  History:
**  15-feb-2000 (somsa01)
**    Removed extra comma from GranteeType declaration.
**  26-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 02-Apr-2003 (noifr0,schph01)
**   (sir 107523) management of sequences
** 20-Aug-2008 (whiro01)
**    Remove redundant <afx...> include which is already in "stdafx.h"
*/

#ifndef GRANTEES_HEADER
#define GRANTEES_HEADER

#include "multflag.h"   // Base classes


//////////////////////////////////////////////////////////////////
// Intermediary base class

// Grantee type
typedef enum
{   
  GRANTEE_TYPE_UNKNOWN = 1,
  GRANTEE_TYPE_ERROR,
  GRANTEE_TYPE_USER,
  GRANTEE_TYPE_GROUP,
  GRANTEE_TYPE_ROLE
} GranteeType;

class CuGrantee: public CuMultFlag
{
  DECLARE_SERIAL (CuGrantee)

public:
  CuGrantee(LPCTSTR name, BOOL bSpecialItem);
  CuGrantee(const CuGrantee* pRefGrantee);
  virtual ~CuGrantee() {}

// for serialisation
  CuGrantee();
  virtual void Serialize (CArchive& ar);

// access to members
  GranteeType GetGranteeType()    { return m_granteeType; }
  void        SetGranteeType(int iobjectType);

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  virtual BOOL IsSimilar(CuMultFlag* pSearchedObject);

  // Duplication
  virtual CuMultFlag* Duplicate() { ASSERT(FALSE); return 0; }

  // For sort
  static int CALLBACK CompareTypes(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:
  GranteeType m_granteeType;    // User/Group/Role/Unknown
};

//////////////////////////////////////////////////////////////////
// Classes for table grantees

#define NBTBLGRANTEES  8

class CuTableGrantee: public CuGrantee
{
  DECLARE_SERIAL (CuTableGrantee)

public:
  CuTableGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType = 0);
  CuTableGrantee(const CuTableGrantee* pRefDbAlarm);
  virtual ~CuTableGrantee() {}

// for serialisation
  CuTableGrantee();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuTableGrantee(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayTableGrantees: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayTableGrantees)

public:

// operations
  CuTableGrantee* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for database grantees

#define NBDBGRANTEES  34

class CuDbGrantee: public CuGrantee
{
  DECLARE_SERIAL (CuDbGrantee)

public:
  CuDbGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType = 0);
  CuDbGrantee(const CuDbGrantee* pRefDbAlarm);
  virtual ~CuDbGrantee() {}

// for serialisation
  CuDbGrantee();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuDbGrantee(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayDbGrantees: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayDbGrantees)

public:

// operations
  CuDbGrantee* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for view grantees

#define NBVIEWGRANTEES  4

class CuViewGrantee: public CuGrantee
{
  DECLARE_SERIAL (CuViewGrantee)

public:
  CuViewGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType = 0);
  CuViewGrantee(const CuViewGrantee* pRefViewAlarm);
  virtual ~CuViewGrantee() {}

// for serialisation
  CuViewGrantee();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuViewGrantee(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayViewGrantees: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayViewGrantees)

public:

// operations
  CuViewGrantee* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for dbevent grantees

#define NBDBEVGRANTEES  2

class CuDbEventGrantee: public CuGrantee
{
  DECLARE_SERIAL (CuDbEventGrantee)

public:
  CuDbEventGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType = 0);
  CuDbEventGrantee(const CuDbEventGrantee* pRefDbEventAlarm);
  virtual ~CuDbEventGrantee() {}

// for serialisation
  CuDbEventGrantee();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuDbEventGrantee(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayDbEventGrantees: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayDbEventGrantees)

public:

// operations
  CuDbEventGrantee* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


//////////////////////////////////////////////////////////////////
// Classes for procedure grantees

#define NBPROCGRANTEES  1

class CuProcGrantee: public CuGrantee
{
  DECLARE_SERIAL (CuProcGrantee)

public:
  CuProcGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType = 0);
  CuProcGrantee(const CuProcGrantee* pRefProcAlarm);
  virtual ~CuProcGrantee() {}

// for serialisation
  CuProcGrantee();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuProcGrantee(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayProcGrantees: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayProcGrantees)

public:

// operations
  CuProcGrantee* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Sequence grantees

#define NBPROCGRANTEES  1

class CuSeqGrantee: public CuGrantee
{
  DECLARE_SERIAL (CuSeqGrantee)

public:
  CuSeqGrantee(LPCTSTR name, BOOL bSpecialItem, int grantType = 0);
  CuSeqGrantee(const CuSeqGrantee* pRefProcAlarm);
  virtual ~CuSeqGrantee() {}

// for serialisation
  CuSeqGrantee();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuSeqGrantee(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArraySeqGrantees: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArraySeqGrantees)

public:

// operations
  CuSeqGrantee* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Alternate Locations

#define NBALTLOC  3

class CuAltLoc: public CuGrantee
{
  DECLARE_SERIAL (CuAltLoc)

public:
  CuAltLoc(LPCTSTR name, BOOL bSpecialItem, int grantType = 0);
  CuAltLoc(const CuAltLoc* pRefViewAlarm);
  virtual ~CuAltLoc() {}

// for serialisation
  CuAltLoc();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuAltLoc(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayAltLocs: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayAltLocs)

public:

// operations
  CuAltLoc* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};




#endif  // GRANTEES_HEADER

