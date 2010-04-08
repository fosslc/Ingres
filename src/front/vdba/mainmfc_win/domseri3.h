/****************************************************************************************
//                                                                                     //
//  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : DomSeri3.h, Header File                                               //
//               Serialisation Classes for ICE                                         //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Author   : Emmanuel Blattes                                                      //
//                                                                                     //
//    Purpose  : Serialization of DOM right pane property pages (modeless dialog)      //
****************************************************************************************/

#ifndef DOMSERIAL3_HEADER
#define DOMSERIAL3_HEADER

#include "ipmprdta.h"
#include "domrfsh.h"    // CuDomRefreshParams
#include "multcol.h"    // Utility classes for multicolumn dom detail windows
#include "icegrs.h"     // Grants classes for ice

//
// ICE
//
class CuDomPropDataIceDbuser: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceDbuser will have access to private/protected member variables
  friend class CuDlgDomPropIceDbuser;

    DECLARE_SERIAL (CuDomPropDataIceDbuser)
public:
    CuDomPropDataIceDbuser();
    virtual ~CuDomPropDataIceDbuser(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceDbuser(const CuDomPropDataIceDbuser & refPropDataIceDbuser);
    CuDomPropDataIceDbuser operator = (const CuDomPropDataIceDbuser & refPropDataIceDbuser);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csUserAlias;
  CString m_csComment;
  CString m_csUserName;
};

class CuDomPropDataIceDbconnection: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceDbconnection will have access to private/protected member variables
  friend class CuDlgDomPropIceDbconnection;

    DECLARE_SERIAL (CuDomPropDataIceDbconnection)
public:
    CuDomPropDataIceDbconnection();
    virtual ~CuDomPropDataIceDbconnection(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceDbconnection(const CuDomPropDataIceDbconnection & refPropDataIceDbconnection);
    CuDomPropDataIceDbconnection operator = (const CuDomPropDataIceDbconnection & refPropDataIceDbconnection);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csDBName;
  CString m_csDBUser;
  CString m_csComment;
};

class CuDomPropDataIceRole: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceRole will have access to private/protected member variables
  friend class CuDlgDomPropIceRole;

    DECLARE_SERIAL (CuDomPropDataIceRole)
public:
    CuDomPropDataIceRole();
    virtual ~CuDomPropDataIceRole(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceRole(const CuDomPropDataIceRole & refPropDataIceRole);
    CuDomPropDataIceRole operator = (const CuDomPropDataIceRole & refPropDataIceRole);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csComment;
};

class CuDomPropDataIceWebuser: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceWebuser will have access to private/protected member variables
  friend class CuDlgDomPropIceWebuser;

    DECLARE_SERIAL (CuDomPropDataIceWebuser)
public:
    CuDomPropDataIceWebuser();
    virtual ~CuDomPropDataIceWebuser(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceWebuser(const CuDomPropDataIceWebuser & refPropDataIceWebuser);
    CuDomPropDataIceWebuser operator = (const CuDomPropDataIceWebuser & refPropDataIceWebuser);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csDefDBUser;
  CString m_csComment;
  BOOL m_bAdminPriv;
  BOOL m_bSecurityPriv;
  BOOL m_bUnitMgrPriv;
  BOOL m_bMonitorPriv;
  long m_ltimeoutms;
  BOOL m_bRepositoryPsw;
};

class CuDomPropDataIceProfile: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceProfile will have access to private/protected member variables
  friend class CuDlgDomPropIceProfile;

    DECLARE_SERIAL (CuDomPropDataIceProfile)
public:
    CuDomPropDataIceProfile();
    virtual ~CuDomPropDataIceProfile(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceProfile(const CuDomPropDataIceProfile & refPropDataIceProfile);
    CuDomPropDataIceProfile operator = (const CuDomPropDataIceProfile & refPropDataIceProfile);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csDefProfile;
  BOOL m_bAdminPriv;
  BOOL m_bSecurityPriv;
  BOOL m_bUnitMgrPriv;
  BOOL m_bMonitorPriv;
  long m_ltimeoutms;
};

class CuDomPropDataIceLocation: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceLocation will have access to private/protected member variables
  friend class CuDlgDomPropIceLocation;

    DECLARE_SERIAL (CuDomPropDataIceLocation)
public:
    CuDomPropDataIceLocation();
    virtual ~CuDomPropDataIceLocation(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceLocation(const CuDomPropDataIceLocation & refPropDataIceLocation);
    CuDomPropDataIceLocation operator = (const CuDomPropDataIceLocation & refPropDataIceLocation);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  BOOL    m_bIce;
  BOOL    m_bPublic;
  CString m_csPath;
  CString m_csExtensions;
  CString m_csComment;
};

class CuDomPropDataIceVariable: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceVariable will have access to private/protected member variables
  friend class CuDlgDomPropIceVariable;

    DECLARE_SERIAL (CuDomPropDataIceVariable)
public:
    CuDomPropDataIceVariable();
    virtual ~CuDomPropDataIceVariable(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceVariable(const CuDomPropDataIceVariable & refPropDataIceVariable);
    CuDomPropDataIceVariable operator = (const CuDomPropDataIceVariable & refPropDataIceVariable);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csVariableValue;
};

class CuDomPropDataIceBunit: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceBunit will have access to private/protected member variables
  friend class CuDlgDomPropIceBunit;

    DECLARE_SERIAL (CuDomPropDataIceBunit)
public:
    CuDomPropDataIceBunit();
    virtual ~CuDomPropDataIceBunit(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceBunit(const CuDomPropDataIceBunit & refPropDataIceBunit);
    CuDomPropDataIceBunit operator = (const CuDomPropDataIceBunit & refPropDataIceBunit);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csOwner;
};

class CuDomPropDataIceSecRoles: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceSecRoles will have access to private/protected member variables
  friend class CuDlgDomPropIceSecRoles;

    DECLARE_SERIAL (CuDomPropDataIceSecRoles)
public:
    CuDomPropDataIceSecRoles();
    virtual ~CuDomPropDataIceSecRoles(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceSecRoles(const CuDomPropDataIceSecRoles & refPropDataIceSecRoles);
    CuDomPropDataIceSecRoles operator = (const CuDomPropDataIceSecRoles & refPropDataIceSecRoles);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceSecRoles;
  int m_objType;
};

class CuDomPropDataIceSecDbusers: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceSecDbusers will have access to private/protected member variables
  friend class CuDlgDomPropIceSecDbusers;

    DECLARE_SERIAL (CuDomPropDataIceSecDbusers)
public:
    CuDomPropDataIceSecDbusers();
    virtual ~CuDomPropDataIceSecDbusers(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceSecDbusers(const CuDomPropDataIceSecDbusers & refPropDataIceSecDbusers);
    CuDomPropDataIceSecDbusers operator = (const CuDomPropDataIceSecDbusers & refPropDataIceSecDbusers);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceSecDbusers;
  int m_objType;
};

class CuDomPropDataIceSecDbconns: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceSecDbconns will have access to private/protected member variables
  friend class CuDlgDomPropIceSecDbconns;

    DECLARE_SERIAL (CuDomPropDataIceSecDbconns)
public:
    CuDomPropDataIceSecDbconns();
    virtual ~CuDomPropDataIceSecDbconns(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceSecDbconns(const CuDomPropDataIceSecDbconns & refPropDataIceSecDbconns);
    CuDomPropDataIceSecDbconns operator = (const CuDomPropDataIceSecDbconns & refPropDataIceSecDbconns);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceSecDbconns;
  int m_objType;
};

class CuDomPropDataIceSecWebusers: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceSecWebusers will have access to private/protected member variables
  friend class CuDlgDomPropIceSecWebusers;

    DECLARE_SERIAL (CuDomPropDataIceSecWebusers)
public:
    CuDomPropDataIceSecWebusers();
    virtual ~CuDomPropDataIceSecWebusers(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceSecWebusers(const CuDomPropDataIceSecWebusers & refPropDataIceSecWebusers);
    CuDomPropDataIceSecWebusers operator = (const CuDomPropDataIceSecWebusers & refPropDataIceSecWebusers);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceSecWebusers;
  int m_objType;
};

class CuDomPropDataIceSecProfiles: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceSecProfiles will have access to private/protected member variables
  friend class CuDlgDomPropIceSecProfiles;

    DECLARE_SERIAL (CuDomPropDataIceSecProfiles)
public:
    CuDomPropDataIceSecProfiles();
    virtual ~CuDomPropDataIceSecProfiles(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceSecProfiles(const CuDomPropDataIceSecProfiles & refPropDataIceSecProfiles);
    CuDomPropDataIceSecProfiles operator = (const CuDomPropDataIceSecProfiles & refPropDataIceSecProfiles);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceSecProfiles;
  int m_objType;
};

class CuDomPropDataIceWebuserRoles: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceWebuserRoles will have access to private/protected member variables
  friend class CuDlgDomPropIceWebuserRoles;

    DECLARE_SERIAL (CuDomPropDataIceWebuserRoles)
public:
    CuDomPropDataIceWebuserRoles();
    virtual ~CuDomPropDataIceWebuserRoles(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceWebuserRoles(const CuDomPropDataIceWebuserRoles & refPropDataIceWebuserRoles);
    CuDomPropDataIceWebuserRoles operator = (const CuDomPropDataIceWebuserRoles & refPropDataIceWebuserRoles);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceWebuserRoles;
  int m_objType;
};

class CuDomPropDataIceWebuserDbconns: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceWebuserDbconns will have access to private/protected member variables
  friend class CuDlgDomPropIceWebuserDbconns;

    DECLARE_SERIAL (CuDomPropDataIceWebuserDbconns)
public:
    CuDomPropDataIceWebuserDbconns();
    virtual ~CuDomPropDataIceWebuserDbconns(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceWebuserDbconns(const CuDomPropDataIceWebuserDbconns & refPropDataIceWebuserDbconns);
    CuDomPropDataIceWebuserDbconns operator = (const CuDomPropDataIceWebuserDbconns & refPropDataIceWebuserDbconns);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceWebuserDbconns;
  int m_objType;
};

class CuDomPropDataIceProfileRoles: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceProfileRoles will have access to private/protected member variables
  friend class CuDlgDomPropIceProfileRoles;

    DECLARE_SERIAL (CuDomPropDataIceProfileRoles)
public:
    CuDomPropDataIceProfileRoles();
    virtual ~CuDomPropDataIceProfileRoles(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceProfileRoles(const CuDomPropDataIceProfileRoles & refPropDataIceProfileRoles);
    CuDomPropDataIceProfileRoles operator = (const CuDomPropDataIceProfileRoles & refPropDataIceProfileRoles);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceProfileRoles;
  int m_objType;
};

class CuDomPropDataIceProfileDbconns: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceProfileDbconns will have access to private/protected member variables
  friend class CuDlgDomPropIceProfileDbconns;

    DECLARE_SERIAL (CuDomPropDataIceProfileDbconns)
public:
    CuDomPropDataIceProfileDbconns();
    virtual ~CuDomPropDataIceProfileDbconns(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceProfileDbconns(const CuDomPropDataIceProfileDbconns & refPropDataIceProfileDbconns);
    CuDomPropDataIceProfileDbconns operator = (const CuDomPropDataIceProfileDbconns & refPropDataIceProfileDbconns);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceProfileDbconns;
  int m_objType;
};

class CuDomPropDataIceServerApplications: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceServerApplications will have access to private/protected member variables
  friend class CuDlgDomPropIceServerApplications;

    DECLARE_SERIAL (CuDomPropDataIceServerApplications)
public:
    CuDomPropDataIceServerApplications();
    virtual ~CuDomPropDataIceServerApplications(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceServerApplications(const CuDomPropDataIceServerApplications & refPropDataIceServerApplications);
    CuDomPropDataIceServerApplications operator = (const CuDomPropDataIceServerApplications & refPropDataIceServerApplications);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceServerApplications;
  int m_objType;
};

class CuDomPropDataIceServerLocations: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceServerLocations will have access to private/protected member variables
  friend class CuDlgDomPropIceServerLocations;

    DECLARE_SERIAL (CuDomPropDataIceServerLocations)
public:
    CuDomPropDataIceServerLocations();
    virtual ~CuDomPropDataIceServerLocations(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceServerLocations(const CuDomPropDataIceServerLocations & refPropDataIceServerLocations);
    CuDomPropDataIceServerLocations operator = (const CuDomPropDataIceServerLocations & refPropDataIceServerLocations);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceServerLocations;
  int m_objType;
};

class CuDomPropDataIceServerVariables: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceServerVariables will have access to private/protected member variables
  friend class CuDlgDomPropIceServerVariables;

    DECLARE_SERIAL (CuDomPropDataIceServerVariables)
public:
    CuDomPropDataIceServerVariables();
    virtual ~CuDomPropDataIceServerVariables(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceServerVariables(const CuDomPropDataIceServerVariables & refPropDataIceServerVariables);
    CuDomPropDataIceServerVariables operator = (const CuDomPropDataIceServerVariables & refPropDataIceServerVariables);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceServerVariables;
  int m_objType;
};

class CuDomPropDataIceFacetAndPage: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceFacetAndPage will have access to private/protected member variables
  friend class CuDlgDomPropIceFacetAndPage;
  friend class CuDlgDomPropIceHtml;

    DECLARE_SERIAL (CuDomPropDataIceFacetAndPage)
public:
    CuDomPropDataIceFacetAndPage();
	void ClearTempFileIfAny();
    virtual ~CuDomPropDataIceFacetAndPage(){ClearTempFileIfAny();};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceFacetAndPage(const CuDomPropDataIceFacetAndPage & refPropDataIceFacetAndPage);
    CuDomPropDataIceFacetAndPage operator = (const CuDomPropDataIceFacetAndPage & refPropDataIceFacetAndPage);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  int m_objType;    // Facet or Page ?

  // Common info
  CString m_csNamePlusExt;
  BOOL    m_bPublic;

  // Storage type
  BOOL    m_bStorageInsideRepository;

  // specific for Inside repository
  BOOL    m_bPreCache;
  BOOL    m_bPermCache;
  BOOL    m_bSessionCache;
  CString m_csPath;

  // specific for File system
  CString m_csLocation;
  CString m_csFilename;

  // name of temporary file when displaying file content
  CString m_csTempFile;
};

class CuDomPropDataIceAccessDef: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceAccessDef will have access to private/protected member variables
  friend class CuDlgDomPropIceAccessDef;

    DECLARE_SERIAL (CuDomPropDataIceAccessDef)
public:
    CuDomPropDataIceAccessDef();
    virtual ~CuDomPropDataIceAccessDef(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceAccessDef(const CuDomPropDataIceAccessDef & refPropDataIceAccessDef);
    CuDomPropDataIceAccessDef operator = (const CuDomPropDataIceAccessDef & refPropDataIceAccessDef);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  int m_objType;    // Facet or Page ?

  BOOL m_bExecute;
  BOOL m_bRead;
  BOOL m_bUpdate;
  BOOL m_bDelete;
};

class CuDomPropDataIceBunits: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceBunits will have access to private/protected member variables
  friend class CuDlgDomPropIceBunits;

    DECLARE_SERIAL (CuDomPropDataIceBunits)
public:
    CuDomPropDataIceBunits();
    virtual ~CuDomPropDataIceBunits(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceBunits(const CuDomPropDataIceBunits & refPropDataIceBunits);
    CuDomPropDataIceBunits operator = (const CuDomPropDataIceBunits & refPropDataIceBunits);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceBunits;
};

class CuDomPropDataIceSecDbuser: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceSecDbuser will have access to private/protected member variables
  friend class CuDlgDomPropIceSecDbuser;

    DECLARE_SERIAL (CuDomPropDataIceSecDbuser)
public:
    CuDomPropDataIceSecDbuser();
    virtual ~CuDomPropDataIceSecDbuser(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceSecDbuser(const CuDomPropDataIceSecDbuser & refPropDataIceSecDbuser);
    CuDomPropDataIceSecDbuser operator = (const CuDomPropDataIceSecDbuser & refPropDataIceSecDbuser);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  BOOL m_bUnitRead;
  BOOL m_bCreateDoc;
  BOOL m_bReadDoc;
};

class CuDomPropDataIceSecRole: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceSecRole will have access to private/protected member variables
  friend class CuDlgDomPropIceSecRole;

    DECLARE_SERIAL (CuDomPropDataIceSecRole)
public:
    CuDomPropDataIceSecRole();
    virtual ~CuDomPropDataIceSecRole(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceSecRole(const CuDomPropDataIceSecRole & refPropDataIceSecRole);
    CuDomPropDataIceSecRole operator = (const CuDomPropDataIceSecRole & refPropDataIceSecRole);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  BOOL m_bExecDoc;
  BOOL m_bCreateDoc;
  BOOL m_bReadDoc;
};

class CuDomPropDataIceFacets: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceFacets will have access to private/protected member variables
  friend class CuDlgDomPropIceFacets;

    DECLARE_SERIAL (CuDomPropDataIceFacets)
public:
    CuDomPropDataIceFacets();
    virtual ~CuDomPropDataIceFacets(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceFacets(const CuDomPropDataIceFacets & refPropDataIceFacets);
    CuDomPropDataIceFacets operator = (const CuDomPropDataIceFacets & refPropDataIceFacets);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceFacets;
  int m_objType;
};

class CuDomPropDataIcePages: public CuIpmPropertyData
{
  // Only CuDlgDomPropIcePages will have access to private/protected member variables
  friend class CuDlgDomPropIcePages;

    DECLARE_SERIAL (CuDomPropDataIcePages)
public:
    CuDomPropDataIcePages();
    virtual ~CuDomPropDataIcePages(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIcePages(const CuDomPropDataIcePages & refPropDataIcePages);
    CuDomPropDataIcePages operator = (const CuDomPropDataIcePages & refPropDataIcePages);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIcePages;
  int m_objType;
};

class CuDomPropDataIceLocations: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceLocations will have access to private/protected member variables
  friend class CuDlgDomPropIceLocations;

    DECLARE_SERIAL (CuDomPropDataIceLocations)
public:
    CuDomPropDataIceLocations();
    virtual ~CuDomPropDataIceLocations(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceLocations(const CuDomPropDataIceLocations & refPropDataIceLocations);
    CuDomPropDataIceLocations operator = (const CuDomPropDataIceLocations & refPropDataIceLocations);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaIceLocations;
  int m_objType;
};

class CuDomPropDataIceBunitRoles: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceBunitRoles will have access to private/protected member variables
  friend class CuDlgDomPropIceBunitRoles;

    DECLARE_SERIAL (CuDomPropDataIceBunitRoles)
public:
    CuDomPropDataIceBunitRoles();
    virtual ~CuDomPropDataIceBunitRoles(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceBunitRoles(const CuDomPropDataIceBunitRoles & refPropDataIceBunitRoles);
    CuDomPropDataIceBunitRoles operator = (const CuDomPropDataIceBunitRoles & refPropDataIceBunitRoles);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayIceBunitRoleGrants m_uaIceBunitRoles;
  //CuArrayNameOnly m_uaIceBunitRoles;
  int m_objType;
};

class CuDomPropDataIceBunitUsers: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceBunitUsers will have access to private/protected member variables
  friend class CuDlgDomPropIceBunitUsers;

    DECLARE_SERIAL (CuDomPropDataIceBunitUsers)
public:
    CuDomPropDataIceBunitUsers();
    virtual ~CuDomPropDataIceBunitUsers(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceBunitUsers(const CuDomPropDataIceBunitUsers & refPropDataIceBunitUsers);
    CuDomPropDataIceBunitUsers operator = (const CuDomPropDataIceBunitUsers & refPropDataIceBunitUsers);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayIceBunitUserGrants m_uaIceBunitUsers;
  //CuArrayNameOnly m_uaIceBunitUsers;
  int m_objType;
};

class CuDomPropDataIceFacetNPageAccessdef: public CuIpmPropertyData
{
  // Only CuDlgDomPropIceFacetNPageRoles and CuDlgDomPropIceFacetNPageUsers will have access to private/protected member variables
  friend class CuDlgDomPropIceFacetNPageRoles;
  friend class CuDlgDomPropIceFacetNPageUsers;

    DECLARE_SERIAL (CuDomPropDataIceFacetNPageAccessdef)
public:
    CuDomPropDataIceFacetNPageAccessdef();
    virtual ~CuDomPropDataIceFacetNPageAccessdef(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIceFacetNPageAccessdef(const CuDomPropDataIceFacetNPageAccessdef & refPropDataIceFacetNPageAccessdef);
    CuDomPropDataIceFacetNPageAccessdef operator = (const CuDomPropDataIceFacetNPageAccessdef & refPropDataIceFacetNPageAccessdef);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayIceFacetNPageAccessdefs m_uaIceFacetNPageAccessdef;
  //CuArrayNameOnly m_uaIceFacetNPageAccessdef;
  int m_objType;
};


#endif  // DOMSERIAL3_HEADER
