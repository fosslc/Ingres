/****************************************************************************************
//                                                                                     //
//  Copyright (C) Jan, 1998 Computer Associates International, Inc.                    //
//                                                                                     //
//    Source   : alarms.h                                                              //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    classes for alarms management in right pane of dom                               //
//                                                                                     //
****************************************************************************************/
#ifndef ALARMS_HEADER
#define ALARMS_HEADER

#include "multflag.h"   // Base classes


//////////////////////////////////////////////////////////////////
// Intermediary base class

class CuAlarm: public CuMultFlag
{
  DECLARE_SERIAL (CuAlarm)

public:
  CuAlarm(long number, LPCTSTR name, LPCTSTR alarmee);
  CuAlarm(const CuAlarm* pRefAlarm);
  CuAlarm(LPCTSTR name);    // for error/none cases
  virtual ~CuAlarm() {}

// for serialisation
  CuAlarm();
  virtual void Serialize (CArchive& ar);

// access to members
  long    GetNumber()             { return m_number; }
  CString GetStrAlarmee()         { return m_strAlarmee; }

  virtual int IndexFromFlagType(int FlagType) { ASSERT (FALSE); return -1; }

  virtual BOOL IsSimilar(CuMultFlag* pSearchedObject);

  // Duplication
  virtual CuMultFlag* Duplicate() { ASSERT(FALSE); return 0; }

  // For sort
  static int CALLBACK CompareNumbers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareAlarmees(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:
  long    m_number;           // alarm number
  CString m_strAlarmee;       // alarmee name
};

//////////////////////////////////////////////////////////////////
// Classes for Database alarms

#define NBDBALARMS  4

class CuDbAlarm: public CuAlarm
{
  DECLARE_SERIAL (CuDbAlarm)

public:
  CuDbAlarm(long number, LPCTSTR name, LPCTSTR alarmee, int alarmType);
  CuDbAlarm(const CuDbAlarm* pRefDbAlarm);
  CuDbAlarm(LPCTSTR name);
  virtual ~CuDbAlarm() {}

// for serialisation
  CuDbAlarm();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuDbAlarm(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayDbAlarms: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayDbAlarms)

public:

// operations
  CuDbAlarm* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Table alarms

#define NBTBLALARMS  8

class CuTableAlarm: public CuAlarm
{
  DECLARE_SERIAL (CuTableAlarm)

public:
  CuTableAlarm(long number, LPCTSTR name, LPCTSTR alarmee, int alarmType);
  CuTableAlarm(const CuTableAlarm* pRefTableAlarm);
  CuTableAlarm(LPCTSTR name);
  virtual ~CuTableAlarm() {}

// for serialisation
  CuTableAlarm();
  virtual void Serialize (CArchive& ar);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuTableAlarm(this); }

// access to members
  virtual int IndexFromFlagType(int FlagType);
};

class CuArrayTableAlarms: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayTableAlarms)

public:

// operations
  CuTableAlarm* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for User XAlarms

#define NBUSERXTBLALARMS  8

class CuUserXAlarm: public CuMultFlag
{
  DECLARE_SERIAL (CuUserXAlarm)

public:
  CuUserXAlarm(long number, LPCTSTR name, LPCTSTR dbName, LPCTSTR tblName, int alarmType);
  CuUserXAlarm(const CuUserXAlarm* pRefUserXAlarm);
  CuUserXAlarm(LPCTSTR name);   // Error / No Item
  virtual ~CuUserXAlarm() {}

// for serialisation
  CuUserXAlarm();
  virtual void Serialize (CArchive& ar);

  virtual BOOL IsSimilar(CuMultFlag* pSearchedObject);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuUserXAlarm(this); }

  // For sort
  static int CALLBACK CompareNumbers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareDbNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
  static int CALLBACK CompareTblNames(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

// access to members
  virtual int IndexFromFlagType(int FlagType);
  long    GetNumber()          { return m_number; }
  CString GetStrDbName()       { return m_strDbName; }
  CString GetStrTblName()      { return m_strTblName; }

private:
  long    m_number;           // alarm number
  CString m_strDbName;
  CString m_strTblName;
};

class CuArrayUserXAlarms: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayUserXAlarms)

public:

// operations
  CuUserXAlarm* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


#endif  // ALARMS_HEADER

