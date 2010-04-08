/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : icegrs.h                                                              //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    classes for ice grants management in right pane of dom                           //
//                                                                                     //
****************************************************************************************/
#ifndef ICE_GRANTS_HEADER
#define COLUMNS_HEADER

#include "multflag.h"   // Base classes

//////////////////////////////////////////////////////////////////
// No Intermediary base class

//////////////////////////////////////////////////////////////////
// Classes for Business Unit Role Grants

#define NB_ICE_BUNIT_ROLE_COLUMNS  3

#define ICE_GRANT_TYPE_ROLE_EXECDOC    1
#define ICE_GRANT_TYPE_ROLE_CREATEDOC  2
#define ICE_GRANT_TYPE_ROLE_READDOC    3

class CuIceBunitRoleGrant: public CuMultFlag
{
  DECLARE_SERIAL (CuIceBunitRoleGrant)

public:
  CuIceBunitRoleGrant(LPCTSTR name, BOOL bExecDoc, BOOL bCreateDoc, BOOL bReadDoc);
  CuIceBunitRoleGrant(const CuIceBunitRoleGrant* pRefRoleGrant);
  CuIceBunitRoleGrant(LPCTSTR name);    // Error/No item
  virtual ~CuIceBunitRoleGrant() {}

// for serialisation
  CuIceBunitRoleGrant();
  virtual void Serialize (CArchive& ar);

  virtual int IndexFromFlagType(int FlagType);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuIceBunitRoleGrant(this); }

  // No sort for columns

private:
};

class CuArrayIceBunitRoleGrants: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayIceBunitRoleGrants)

public:

// operations
  CuIceBunitRoleGrant* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};

//////////////////////////////////////////////////////////////////
// Classes for Business Unit User Grants

#define NB_ICE_BUNIT_USER_COLUMNS  3

#define ICE_GRANT_TYPE_USER_UNITREAD   1
#define ICE_GRANT_TYPE_USER_CREATEDOC  2
#define ICE_GRANT_TYPE_USER_READDOC    3

class CuIceBunitUserGrant: public CuMultFlag
{
  DECLARE_SERIAL (CuIceBunitUserGrant)

public:
  CuIceBunitUserGrant(LPCTSTR name, BOOL bExecDoc, BOOL bCreateDoc, BOOL bReadDoc);
  CuIceBunitUserGrant(const CuIceBunitUserGrant* pRefUserGrant);
  CuIceBunitUserGrant(LPCTSTR name);    // Error/No item
  virtual ~CuIceBunitUserGrant() {}

// for serialisation
  CuIceBunitUserGrant();
  virtual void Serialize (CArchive& ar);

  virtual int IndexFromFlagType(int FlagType);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuIceBunitUserGrant(this); }

  // No sort for columns

private:
};

class CuArrayIceBunitUserGrants: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayIceBunitUserGrants)

public:

// operations
  CuIceBunitUserGrant* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


//////////////////////////////////////////////////////////////////
// Classes for Access definition, common to roles and users, for a facet as well as a page

#define NB_ICE_FACETNPAGE_ACCESSDEF_COLUMNS  4

#define ICE_GRANT_FACETNPAGE_ACCESSDEF_EXECUTE 1
#define ICE_GRANT_FACETNPAGE_ACCESSDEF_READ    2
#define ICE_GRANT_FACETNPAGE_ACCESSDEF_UPDATE  3
#define ICE_GRANT_FACETNPAGE_ACCESSDEF_DELETE  4

class CuIceFacetNPageAccessdef: public CuMultFlag
{
  DECLARE_SERIAL (CuIceFacetNPageAccessdef)

public:
  CuIceFacetNPageAccessdef(LPCTSTR name, BOOL bExecute, BOOL bRead, BOOL bUpdate, BOOL bDelete);
  CuIceFacetNPageAccessdef(const CuIceFacetNPageAccessdef* pRefFacetNPageAccessdef);
  CuIceFacetNPageAccessdef(LPCTSTR name);    // Error/No item
  virtual ~CuIceFacetNPageAccessdef() {}

// for serialisation
  CuIceFacetNPageAccessdef();
  virtual void Serialize (CArchive& ar);

  virtual int IndexFromFlagType(int FlagType);

  // Duplication
  virtual CuMultFlag* Duplicate() { return new CuIceFacetNPageAccessdef(this); }

  // No sort for columns

private:
};

class CuArrayIceFacetNPageAccessdefs: public CuArrayMultFlags
{
  DECLARE_SERIAL (CuArrayIceFacetNPageAccessdefs)

public:

// operations
  CuIceFacetNPageAccessdef* operator [] (int index);
  virtual void Add(CuMultFlag* pNewMultFlag);   // input parameter has been allocated by new
  virtual void Add(CuMultFlag& newMultFlag);    // input parameter has been allocated on the stack

// for serialisation
  virtual void Serialize (CArchive& ar);
};


#endif  // COLUMNS_HEADER

