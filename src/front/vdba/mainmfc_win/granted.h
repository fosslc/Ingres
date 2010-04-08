/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : Granted.h                                                             //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    classes for granted objects (cross-ref) management in right pane of dom          //
//                                                                                     //
****************************************************************************************/
#ifndef GRANTED_HEADER
#define GRANTED_HEADER

#include "multflag.h"   // Base classes


//////////////////////////////////////////////////////////////////
// Intermediary base class

class CuGranted: public CuMultFlag
{
  DECLARE_SERIAL (CuGranted)

public:
  CuGranted(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb);
  CuGranted(const CuGranted* pRefGranted);
  CuGranted(LPCTSTR name);  // special item
  virtual ~CuGranted() {}

// for serialisation
  CuGranted();
  virtual void Serialize (CArchive& ar);

// access to members
  CString GetOwnerName()  { return m_csOwner; }
  CString GetParentName() { return m_csParent; }

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  virtual BOOL IsSimilar(CuMultFlag* pSearchedObject);

  // Duplication
  virtual CuMultFlag* Duplicate() { ASSERT(FALSE); return 0; }

  // For sort
  static int CALLBACK CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareOwners(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareParents(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:
  CString m_csOwner;
  CString m_csParent;
};

//////////////////////////////////////////////////////////////////
// Classes for tables granted

#define NBTBLGRANTED  7

class CuGrantedTbl: public CuGranted
{
  DECLARE_SERIAL (CuGrantedTbl)

public:
  CuGrantedTbl(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType);
  CuGrantedTbl(LPCTSTR name);
  CuGrantedTbl(const CuGrantedTbl* pRefGranted);
  virtual ~CuGrantedTbl() {}

// for serialisation
  CuGrantedTbl();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuGrantedTbl(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayGrantedTbls: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayGrantedTbls)

public:

// operations
  CuGrantedTbl* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for databases granted

#define NBDBGRANTED  34

class CuGrantedDb: public CuGranted
{
  DECLARE_SERIAL (CuGrantedDb)

public:
  CuGrantedDb(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType);
  CuGrantedDb(LPCTSTR name);
  CuGrantedDb(const CuGrantedDb* pRefGranted);
  virtual ~CuGrantedDb() {}

// for serialisation
  CuGrantedDb();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuGrantedDb(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayGrantedDbs: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayGrantedDbs)

public:

// operations
  CuGrantedDb* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for views granted

#define NBVIEWGRANTED  4

class CuGrantedView: public CuGranted
{
  DECLARE_SERIAL (CuGrantedView)

public:
  CuGrantedView(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType);
  CuGrantedView(LPCTSTR name);
  CuGrantedView(const CuGrantedView* pRefGranted);
  virtual ~CuGrantedView() {}

// for serialisation
  CuGrantedView();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuGrantedView(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayGrantedViews: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayGrantedViews)

public:

// operations
  CuGrantedView* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for dbevents granted

#define NBDBEVGRANTED  2

class CuGrantedEvent: public CuGranted
{
  DECLARE_SERIAL (CuGrantedEvent)

public:
  CuGrantedEvent(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType);
  CuGrantedEvent(LPCTSTR name);
  CuGrantedEvent(const CuGrantedEvent* pRefGranted);
  virtual ~CuGrantedEvent() {}

// for serialisation
  CuGrantedEvent();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuGrantedEvent(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayGrantedEvents: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayGrantedEvents)

public:

// operations
  CuGrantedEvent* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


//////////////////////////////////////////////////////////////////
// Classes for procedures granted

#define NBPROCGRANTED  1

class CuGrantedProc: public CuGranted
{
  DECLARE_SERIAL (CuGrantedProc)

public:
  CuGrantedProc(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType);
  CuGrantedProc(LPCTSTR name);
  CuGrantedProc(const CuGrantedProc* pRefGranted);
  virtual ~CuGrantedProc() {}

// for serialisation
  CuGrantedProc();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuGrantedProc(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayGrantedProcs: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayGrantedProcs)

public:

// operations
  CuGrantedProc* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

#define NBROLEGRANTED  1

class CuGrantedRole: public CuGranted
{
  DECLARE_SERIAL (CuGrantedRole)

public:
  CuGrantedRole(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType);
  CuGrantedRole(LPCTSTR name);
  CuGrantedRole(const CuGrantedRole* pRefGranted);
  virtual ~CuGrantedRole() {}

// for serialisation
  CuGrantedRole();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuGrantedRole(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayGrantedRoles: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayGrantedRoles)

public:

// operations
  CuGrantedRole* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Sequences granted

#define NBSEQGRANTED  1

class CuGrantedSeq: public CuGranted
{
  DECLARE_SERIAL (CuGrantedSeq)

public:
  CuGrantedSeq(LPCTSTR name, LPCTSTR owner, LPCTSTR parentDb, int grantType);
  CuGrantedSeq(LPCTSTR name);
  CuGrantedSeq(const CuGrantedSeq* pRefGranted);
  virtual ~CuGrantedSeq() {}

// for serialisation
  CuGrantedSeq();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuGrantedSeq(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayGrantedSeqs: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayGrantedSeqs)

public:

// operations
  CuGrantedSeq* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

#endif  // GRANTED_HEADER
