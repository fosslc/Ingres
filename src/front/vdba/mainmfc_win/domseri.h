/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : DomSeri.h, Header File 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : Emmanuel Blattes 
**
**    Purpose  : Serialization of DOM right pane property pages (modeless dialog)
**
** History:
**
** xx-Dec-1997 (Emmanuel Blattes)
**    Created.
** 24-Oct-2000 (SCHPH01)
**    Sir 102821 comment on table and columns.
** 09-May-2001 (schph01)
**    SIR 102509 add new variable member in CuDomPropDataLocation class to
**    manage the raw data location.
** 06-Jun-2001(schph01)
**    (additional change for SIR 102509) update of previous change
**    because of backend changes
** 15-Jun-2001(schph01)
**    SIR 104991 add new variables (m_bGrantUsr4Rmcmd ,m_bPartialGrantDefined )
**    in CuDomPropDataUser class to manage grant for rmcmd.
** 21-Jun-2001 (schph01)
**    (SIR 103694) STEP 2 support of UNICODE datatypes.
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 02-Apr-2003 (schph01)
**    SIR #107523 Declare new class for sequences.
** 10-Jun-2004 (schph01)
**    SIR #112460, Add variable m_csCatalogsPageSize in CuDomPropDataDb class
**    to manage "Catalogs Page Size" information
**  06-Sep-2005) (drivi01)
**    Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**    two available unicode normalization options, added group box with
**    two checkboxes corresponding to each normalization, including all
**    expected functionlity.
*/

#ifndef DOMSERIAL_HEADER
#define DOMSERIAL_HEADER

#include "ipmprdta.h"
#include "multcol.h"    // Utility classes for multicolumn dom detail windows
#include "alarms.h"     // classes for alarms matrix management
#include "grantees.h"   // classes for grantees matrix management
#include "columns.h"    // classes for table columns management
#include "replic.h"     // Classes for replicator multicolumn management (cdds propagation paths and database information, columns for a registered table, etc)
#include "granted.h"    // classes for granted (xref) matrix management
#include "tbstatis.h"   // Statistic data
#include "domrfsh.h"    // dom refresh params management for detail panes
#include "dmlbase.h"    // CaDBObject

//
// LOCATION
//
class CuDomPropDataLocation: public CuIpmPropertyData
{
  // Only CuDlgDomPropLocation will have access to private/protected member variables
  friend class CuDlgDomPropLocation;

    DECLARE_SERIAL (CuDomPropDataLocation)
public:
    CuDomPropDataLocation();
    virtual ~CuDomPropDataLocation(){};

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_Area;
  BOOL    m_bDatabase;
  BOOL    m_bWork;
  BOOL    m_bJournal;
  BOOL    m_bCheckPoint;
  BOOL    m_bDump;
  int     m_iRawPct;
};

//
// USER
//
class CuDomPropDataUser: public CuIpmPropertyData
{
  // Only CuDlgDomPropUser will have access to private/protected member variables
  friend class CuDlgDomPropUser;

    DECLARE_SERIAL (CuDomPropDataUser)
public:
    CuDomPropDataUser();
    virtual ~CuDomPropDataUser(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUser(const CuDomPropDataUser & refPropDataUser);
    CuDomPropDataUser operator = (const CuDomPropDataUser & refPropDataUser);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csDefGroup      ;
  CString m_csDefProfile    ;
  CString m_csExpireDate    ;
  CString m_csLimSecureLabel;
  BOOL    m_aPrivilege[USERPRIVILEGES];
  BOOL    m_aDefaultPrivilege[USERPRIVILEGES];
  BOOL    m_aSecAudit[USERSECAUDITS];

  CStringArray m_acsGroupsContainingUser;
  CStringArray m_acsSchemaDatabases;
  BOOL    m_bGrantUsr4Rmcmd;
  BOOL    m_bPartialGrantDefined;
};


//
// ROLE
//
class CuDomPropDataRole: public CuIpmPropertyData
{
  // Only CuDlgDomPropRole will have access to private/protected member variables
  friend class CuDlgDomPropRole;

    DECLARE_SERIAL (CuDomPropDataRole)
public:
    CuDomPropDataRole();
    virtual ~CuDomPropDataRole(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataRole(const CuDomPropDataRole & refPropDataRole);
    CuDomPropDataRole operator = (const CuDomPropDataRole & refPropDataRole);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csLimSecureLabel;
  BOOL    m_aPrivilege[USERPRIVILEGES];
  BOOL    m_aSecAudit[USERSECAUDITS];

  CStringArray m_acsGranteesOnRole;
};


//
// PROFILE
//
class CuDomPropDataProfile: public CuIpmPropertyData
{
  // Only CuDlgDomPropProfile will have access to private/protected member variables
  friend class CuDlgDomPropProfile;

    DECLARE_SERIAL (CuDomPropDataProfile)
public:
    CuDomPropDataProfile();
    virtual ~CuDomPropDataProfile(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataProfile(const CuDomPropDataProfile & refPropDataProfile);
    CuDomPropDataProfile operator = (const CuDomPropDataProfile & refPropDataProfile);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csDefGroup      ;
  CString m_csExpireDate    ;
  CString m_csLimSecureLabel;
  BOOL    m_aPrivilege[USERPRIVILEGES];
  BOOL    m_aDefaultPrivilege[USERPRIVILEGES];
  BOOL    m_aSecAudit[USERSECAUDITS];
};


//
// INDEX
//
class CuDomPropDataIndex: public CuIpmPropertyData
{
  // Only CuDlgDomPropIndex will have access to private/protected member variables
  friend class CuDlgDomPropIndex;

    DECLARE_SERIAL (CuDomPropDataIndex)
public:
    CuDomPropDataIndex();
    virtual ~CuDomPropDataIndex(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIndex(const CuDomPropDataIndex & refPropDataIndex);
    CuDomPropDataIndex operator = (const CuDomPropDataIndex & refPropDataIndex);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CStringArray m_acsIdxColumns;
    CWordArray   m_awIdxColumns;
    CStringArray m_acsLocations;

    CString m_csSchema;
    CString m_csParent;

    CString m_csStorage;
    BOOL    m_bStruct1;
    CString m_csCaptStruct1;
    CString m_csStruct1;
    BOOL    m_bStruct2;
    CString m_csCaptStruct2;
    CString m_csStruct2;
    BOOL    m_bStruct3;
    CString m_csCaptStruct3;
    CString m_csStruct3;
    BOOL    m_bStruct4;
    CString m_csCaptStruct4;
    CString m_csStruct4;

    CString m_csUnique;
    CString m_csPgSize;
    CString m_csAlloc;
    CString m_csExtend;
    CString m_csAllocatedPages;
    CString m_csFillFact;
    BOOL    m_bPersists;
    BOOL    m_bCompression;   // compression exists
    BOOL    m_bCompress;
    CString m_csCompress;

    BOOL    m_bEnterprise;
};


//
// INTEGRITY
//
class CuDomPropDataIntegrity: public CuIpmPropertyData
{
  // Only CuDlgDomPropIntegrity will have access to private/protected member variables
  friend class CuDlgDomPropIntegrity;

    DECLARE_SERIAL (CuDomPropDataIntegrity)
public:
    CuDomPropDataIntegrity();
    virtual ~CuDomPropDataIntegrity(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIntegrity(const CuDomPropDataIntegrity & refPropDataIntegrity);
    CuDomPropDataIntegrity operator = (const CuDomPropDataIntegrity & refPropDataIntegrity);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    long    m_lNumber;
    CString m_csText;
};

//
// RULE
//
class CuDomPropDataRule: public CuIpmPropertyData
{
  // Only CuDlgDomPropRule will have access to private/protected member variables
  friend class CuDlgDomPropRule;

    DECLARE_SERIAL (CuDomPropDataRule)
public:
    CuDomPropDataRule();
    virtual ~CuDomPropDataRule(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataRule(const CuDomPropDataRule & refPropDataRule);
    CuDomPropDataRule operator = (const CuDomPropDataRule & refPropDataRule);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CString m_csProc;
    CString m_csText;
};


//
// PROCEDURE
//
class CuDomPropDataProcedure: public CuIpmPropertyData
{
  // Only CuDlgDomPropProcedure and CuDlgDomPropProcLink will have access to private/protected member variables
  friend class CuDlgDomPropProcedure;
  friend class CuDlgDomPropProcLink;

    DECLARE_SERIAL (CuDomPropDataProcedure)
public:
    CuDomPropDataProcedure();
    virtual ~CuDomPropDataProcedure(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataProcedure(const CuDomPropDataProcedure & refPropDataProcedure);
    CuDomPropDataProcedure operator = (const CuDomPropDataProcedure & refPropDataProcedure);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csText;
  CStringArray m_acsRulesExecutingProcedure;
};

//
// SEQUENCE
//
class CuDomPropDataSequence: public CuIpmPropertyData
{
  // Only CuDlgDomSeqProcedure and CuDlgDomPropProcLink will have access to private/protected member variables
  friend class CuDlgDomPropDbSeq;
  friend class CuDlgDomPropSeq;

    DECLARE_SERIAL (CuDomPropDataSequence)
public:
    CuDomPropDataSequence();
    virtual ~CuDomPropDataSequence(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataSequence(const CuDomPropDataSequence & refPropDataSequence);
    CuDomPropDataSequence operator = (const CuDomPropDataSequence & refPropDataSequence);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csNameSeq;
  CString m_csOwnerSeq;
  BOOL    m_bDecimalType;
  CString m_csPrecSeq;
  CString m_csStartWith;
  CString m_csIncrementBy;
  CString m_csMaxValue;
  CString m_csMinValue;
  BOOL    m_bCache;
  CString m_csCacheSize;
  BOOL    m_bCycle;
};

//
// GROUP
//
class CuDomPropDataGroup: public CuIpmPropertyData
{
  // Only CuDlgDomPropGroup will have access to private/protected member variables
  friend class CuDlgDomPropGroup;

    DECLARE_SERIAL (CuDomPropDataGroup)
public:
    CuDomPropDataGroup();
    virtual ~CuDomPropDataGroup(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataGroup(const CuDomPropDataGroup & refPropDataGroup);
    CuDomPropDataGroup operator = (const CuDomPropDataGroup & refPropDataGroup);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CStringArray m_acsUsersInGroup;
  int m_objType;
};


//
// DB ALARMS
//
class CuDomPropDataDbAlarms: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbAlarms will have access to private/protected member variables
  friend class CuDlgDomPropDbAlarms;

    DECLARE_SERIAL (CuDomPropDataDbAlarms)
public:
    CuDomPropDataDbAlarms();
    virtual ~CuDomPropDataDbAlarms(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbAlarms(const CuDomPropDataDbAlarms & refPropDataDbAlarms);
    CuDomPropDataDbAlarms operator = (const CuDomPropDataDbAlarms & refPropDataDbAlarms);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayDbAlarms m_uaDbAlarms;
};

//
// TABLE ALARMS
//
class CuDomPropDataTableAlarms: public CuIpmPropertyData
{
  // Only CuDlgDomPropTableAlarms will have access to private/protected member variables
  friend class CuDlgDomPropTableAlarms;

    DECLARE_SERIAL (CuDomPropDataTableAlarms)
public:
    CuDomPropDataTableAlarms();
    virtual ~CuDomPropDataTableAlarms(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTableAlarms(const CuDomPropDataTableAlarms & refPropDataTableAlarms);
    CuDomPropDataTableAlarms operator = (const CuDomPropDataTableAlarms & refPropDataTableAlarms);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayTableAlarms m_uaTableAlarms;
};

//
// TABLE GRANTEES
//
class CuDomPropDataTableGrantees: public CuIpmPropertyData
{
  // Only CuDlgDomPropTableGrantees will have access to private/protected member variables
  friend class CuDlgDomPropTableGrantees;

    DECLARE_SERIAL (CuDomPropDataTableGrantees)
public:
    CuDomPropDataTableGrantees();
    virtual ~CuDomPropDataTableGrantees(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTableGrantees(const CuDomPropDataTableGrantees & refPropDataTableGrantees);
    CuDomPropDataTableGrantees operator = (const CuDomPropDataTableGrantees & refPropDataTableGrantees);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayTableGrantees m_uaTableGrantees;
};


//
// DB GRANTEES
//
class CuDomPropDataDbGrantees: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbGrantees will have access to private/protected member variables
  friend class CuDlgDomPropDbGrantees;

    DECLARE_SERIAL (CuDomPropDataDbGrantees)
public:
    CuDomPropDataDbGrantees();
    virtual ~CuDomPropDataDbGrantees(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbGrantees(const CuDomPropDataDbGrantees & refPropDataDbGrantees);
    CuDomPropDataDbGrantees operator = (const CuDomPropDataDbGrantees & refPropDataDbGrantees);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayDbGrantees m_uaDbGrantees;
};

//
// DB
//
class CuDomPropDataDb: public CuIpmPropertyData
{
  // Only CuDlgDomPropDb will have access to private/protected member variables
  friend class CuDlgDomPropDb;

    DECLARE_SERIAL (CuDomPropDataDb)
public:
    CuDomPropDataDb(LPCTSTR MarkerString = "CuDomPropDataDb");
    virtual ~CuDomPropDataDb(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDb(const CuDomPropDataDb & refPropDataDb);
    CuDomPropDataDb operator = (const CuDomPropDataDb & refPropDataDb);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

    // changed from private to protected for derived DDb class
protected:
    CString m_csOwner;
    CString m_csLocDb;
    CString m_csLocChk;
    CString m_csLocJnl;
    CString m_csLocDmp;
    CString m_csLocSrt;
    BOOL    m_bPrivate;
    CString m_csFeCatalogs;
    CuArrayAltLocs m_uaAltLocs;
    BOOL    m_bReadOnly;
    int    m_bUnicodeDB;
    CString m_csCatalogsPageSize;
};

//
// TABLE
//
class CuDomPropDataTable: public CuIpmPropertyData
{
  // Only CuDlgDomPropTable will have access to private/protected member variables
  friend class CuDlgDomPropTable;

    DECLARE_SERIAL (CuDomPropDataTable)
public:
    CuDomPropDataTable();
    virtual ~CuDomPropDataTable(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTable(const CuDomPropDataTable & refPropDataTable);
    CuDomPropDataTable operator = (const CuDomPropDataTable & refPropDataTable);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CString m_szSchema;
    TCHAR   m_cJournaling;
    BOOL    m_bDupRows;
    BOOL    m_bReadOnly;
    int     m_nStructure;
    int     m_nEstimNbRows;
    int     m_nFillFact;
    int     m_nMinPages;
    int     m_nMaxPages;
    int     m_nLeafFill;
    int     m_nNonLeafFill;
    long    m_lPgAlloc;
    long    m_lExtend;
    long    m_lAllocatedPages;
    long    m_lPgSize;
    BOOL    m_bExpire;
    CString m_csExpire;
    int     m_nUnique;
    CString m_csTableComment;

    CuArrayTblConstraints  m_uaConstraints;
};


//
// VIEW GRANTEES
//
class CuDomPropDataViewGrantees: public CuIpmPropertyData
{
  // Only CuDlgDomPropViewGrantees will have access to private/protected member variables
  friend class CuDlgDomPropViewGrantees;

    DECLARE_SERIAL (CuDomPropDataViewGrantees)
public:
    CuDomPropDataViewGrantees();
    virtual ~CuDomPropDataViewGrantees(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataViewGrantees(const CuDomPropDataViewGrantees & refPropDataViewGrantees);
    CuDomPropDataViewGrantees operator = (const CuDomPropDataViewGrantees & refPropDataViewGrantees);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayViewGrantees m_uaViewGrantees;
};

//
// VIEW
//
class CuDomPropDataView: public CuIpmPropertyData
{
  // Only CuDlgDomPropView, CuDlgDomPropViewLink and CuDlgDomPropViewNative will have access to private/protected member variables
  friend class CuDlgDomPropView;
  friend class CuDlgDomPropViewLink;
  friend class CuDlgDomPropViewNative;

    DECLARE_SERIAL (CuDomPropDataView)
public:
    CuDomPropDataView();
    virtual ~CuDomPropDataView(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataView(const CuDomPropDataView & refPropDataView);
    CuDomPropDataView operator = (const CuDomPropDataView & refPropDataView);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString       m_csText;
  CString       m_csComment;
  CStringArray  m_acsViewComponents;
  CStringArray  m_acsSchema;
  CWordArray    m_awCompType;     // OT_TABLE or OT_VIEW
};

//
// CDDS
//
class CuDomPropDataCdds: public CuIpmPropertyData
{
  // Only CuDlgDomPropCdds will have access to private/protected member variables
  friend class CuDlgDomPropCdds;

    DECLARE_SERIAL (CuDomPropDataCdds)
public:
    CuDomPropDataCdds();
    virtual ~CuDomPropDataCdds(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataCdds(const CuDomPropDataCdds & refPropDataCdds);
    CuDomPropDataCdds operator = (const CuDomPropDataCdds & refPropDataCdds);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    int     m_no;
    CString m_csName;
    int     m_collisionMode;
    int     m_errorMode;

    CuArrayCddsPaths   m_uaPropagationPaths;
    CuArrayCddsDbInfos m_uaDbInfos;
};


//
// CDDS TABLES
//
class CuDomPropDataCddsTables: public CuIpmPropertyData
{
  // Only CuDlgDomPropCddsTables will have access to private/protected member variables
  friend class CuDlgDomPropCddsTables;

    DECLARE_SERIAL (CuDomPropDataCddsTables)
public:
    CuDomPropDataCddsTables();
    virtual ~CuDomPropDataCddsTables(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataCddsTables(const CuDomPropDataCddsTables & refPropDataCddsTables);
    CuDomPropDataCddsTables operator = (const CuDomPropDataCddsTables & refPropDataCddsTables);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CuArrayCddsTables     m_uaCddsTables;
    //CuArrayCddsTableCols  m_uaCddsTableCols;
    int m_nCurSel;  // current selection in list of tables
};

//
// DBEVENT GRANTEES
//
class CuDomPropDataDbEventGrantees: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbEventGrantees will have access to private/protected member variables
  friend class CuDlgDomPropDbEventGrantees;

    DECLARE_SERIAL (CuDomPropDataDbEventGrantees)
public:
    CuDomPropDataDbEventGrantees();
    virtual ~CuDomPropDataDbEventGrantees(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbEventGrantees(const CuDomPropDataDbEventGrantees & refPropDataDbEventGrantees);
    CuDomPropDataDbEventGrantees operator = (const CuDomPropDataDbEventGrantees & refPropDataDbEventGrantees);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayDbEventGrantees m_uaDbEventGrantees;
};


//
// PROCEDURE GRANTEES
//
class CuDomPropDataProcGrantees: public CuIpmPropertyData
{
  // Only CuDlgDomPropProcGrantees will have access to private/protected member variables
  friend class CuDlgDomPropProcGrantees;

    DECLARE_SERIAL (CuDomPropDataProcGrantees)
public:
    CuDomPropDataProcGrantees();
    virtual ~CuDomPropDataProcGrantees(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataProcGrantees(const CuDomPropDataProcGrantees & refPropDataProcGrantees);
    CuDomPropDataProcGrantees operator = (const CuDomPropDataProcGrantees & refPropDataProcGrantees);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayProcGrantees m_uaProcGrantees;
};

//
// SEQUENCE GRANTEES
//
class CuDomPropDataSeqGrantees: public CuIpmPropertyData
{
    // Only CuDlgDomPropSeqGrantees will have access to private/protected member variables
    friend class CuDlgDomPropSeqGrantees;

    DECLARE_SERIAL (CuDomPropDataSeqGrantees)
public:
    CuDomPropDataSeqGrantees();
    virtual ~CuDomPropDataSeqGrantees(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataSeqGrantees(const CuDomPropDataSeqGrantees & refPropDataSeqGrantees);
    CuDomPropDataSeqGrantees operator = (const CuDomPropDataSeqGrantees & refPropDataSeqGrantees);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArraySeqGrantees m_uaSeqGrantees;
};

//
// TABLE COLUMNS
//
class CuDomPropDataTableColumns: public CuIpmPropertyData
{
  // Only CuDlgDomPropTableColumns will have access to private/protected member variables
  friend class CuDlgDomPropTableColumns;

    DECLARE_SERIAL (CuDomPropDataTableColumns)
public:
    CuDomPropDataTableColumns();
    virtual ~CuDomPropDataTableColumns(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTableColumns(const CuDomPropDataTableColumns & refPropDataTableColumns);
    CuDomPropDataTableColumns operator = (const CuDomPropDataTableColumns & refPropDataTableColumns);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CuArrayTblColumns      m_uaColumns;
};


//
// TABLE/VIEW ROWS
//
class CuDomPropDataTableRows: public CuIpmPropertyData
{
	DECLARE_SERIAL (CuDomPropDataTableRows)
public:
	CuDomPropDataTableRows();
	virtual ~CuDomPropDataTableRows(){};

	virtual void NotifyLoad (CWnd* pDlg);
	virtual void Serialize (CArchive& ar);
	COleStreamFile& GetStreamFile(){return m_streamFile;}
protected:
	COleStreamFile m_streamFile;
};


//
// TABLE/VIEW STATISTIC
//
class CuDomPropDataTableStatistic: public CuIpmPropertyData
{
    DECLARE_SERIAL (CuDomPropDataTableStatistic)
public:
    CuDomPropDataTableStatistic();
    virtual ~CuDomPropDataTableStatistic(){};
    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

    CaTableStatistic m_statisticData;
    int   m_nSelectColumn;
    CSize m_scrollPos;

};

//
// STAR (Distributed) DATABASE - DDB
//
class CuDomPropDataDDb: public CuDomPropDataDb
{
  // Only CuDlgDomPropDDb will have access to private/protected member variables
  friend class CuDlgDomPropDDb;

    DECLARE_SERIAL (CuDomPropDataDDb)
public:
    CuDomPropDataDDb();
    virtual ~CuDomPropDataDDb(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDDb(const CuDomPropDataDDb & refPropDataDDb);
    CuDomPropDataDDb operator = (const CuDomPropDataDDb & refPropDataDDb);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
    CString m_csCoordinator;
};

//
// STAR TABLE REGISTERED AS LINK
//
class CuDomPropDataStarTableLink: public CuIpmPropertyData
{
  // Only CuDlgDomPropStarTableLink will have access to private/protected member variables
  friend class CuDlgDomPropTableLink;

    DECLARE_SERIAL (CuDomPropDataStarTableLink)
public:
    CuDomPropDataStarTableLink();
    virtual ~CuDomPropDataStarTableLink(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataStarTableLink(const CuDomPropDataStarTableLink & refPropDataStarTableLink);
    CuDomPropDataStarTableLink operator = (const CuDomPropDataStarTableLink & refPropDataStarTableLink);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CString m_csLdbObjName;
    CString m_csLdbObjOwner;
    CString m_csLdbDatabase;
    CString m_csLdbNode;
    CString m_csLdbDbmsType;

    CuArrayStarTblColumns   m_uaColumns;
};

//
// STAR INDEX REGISTERED AS LINK: SOURCE
//
class CuDomPropDataIndexLinkSource: public CuIpmPropertyData
{
  // Only CuDlgDomPropIndexLinkSource will have access to private/protected member variables
  friend class CuDlgDomPropIndexLinkSource;

    DECLARE_SERIAL (CuDomPropDataIndexLinkSource)
public:
    CuDomPropDataIndexLinkSource();
    virtual ~CuDomPropDataIndexLinkSource(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataIndexLinkSource(const CuDomPropDataIndexLinkSource & refPropDataIndexLinkSource);
    CuDomPropDataIndexLinkSource operator = (const CuDomPropDataIndexLinkSource & refPropDataIndexLinkSource);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CString m_csLdbObjName;
    CString m_csLdbObjOwner;
    CString m_csLdbDatabase;
    CString m_csLdbNode;
    CString m_csLdbDbmsType;
};


//
// STAR VIEW REGISTERED AS LINK: SOURCE
//
class CuDomPropDataViewLinkSource: public CuIpmPropertyData
{
  // Only CuDlgDomPropViewLinkSource will have access to private/protected member variables
  friend class CuDlgDomPropViewLinkSource;

    DECLARE_SERIAL (CuDomPropDataViewLinkSource)
public:
    CuDomPropDataViewLinkSource();
    virtual ~CuDomPropDataViewLinkSource(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataViewLinkSource(const CuDomPropDataViewLinkSource & refPropDataViewLinkSource);
    CuDomPropDataViewLinkSource operator = (const CuDomPropDataViewLinkSource & refPropDataViewLinkSource);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CString m_csLdbObjName;
    CString m_csLdbObjOwner;
    CString m_csLdbDatabase;
    CString m_csLdbNode;
    CString m_csLdbDbmsType;
};


//
// STAR VIEW NATIVE: SOURCE
//
class CuDomPropDataViewNativeSource: public CuIpmPropertyData
{
  // Only CuDlgDomPropViewNativeSource will have access to private/protected member variables
  friend class CuDlgDomPropViewNativeSource;

    DECLARE_SERIAL (CuDomPropDataViewNativeSource)
public:
    CuDomPropDataViewNativeSource();
    virtual ~CuDomPropDataViewNativeSource(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataViewNativeSource(const CuDomPropDataViewNativeSource & refPropDataViewNativeSource);
    CuDomPropDataViewNativeSource operator = (const CuDomPropDataViewNativeSource & refPropDataViewNativeSource);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CString m_csLdbObjName;
    CString m_csLdbObjOwner;
    CString m_csLdbDatabase;
    CString m_csLdbNode;
    CString m_csLdbDbmsType;
};


//
// STAR PROCEDURE REGISTERED AS LINK: SOURCE
//
class CuDomPropDataProcLinkSource: public CuIpmPropertyData
{
  // Only CuDlgDomPropProcLinkSource will have access to private/protected member variables
  friend class CuDlgDomPropProcLinkSource;

    DECLARE_SERIAL (CuDomPropDataProcLinkSource)
public:
    CuDomPropDataProcLinkSource();
    virtual ~CuDomPropDataProcLinkSource(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataProcLinkSource(const CuDomPropDataProcLinkSource & refPropDataProcLinkSource);
    CuDomPropDataProcLinkSource operator = (const CuDomPropDataProcLinkSource & refPropDataProcLinkSource);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
    CString m_csLdbObjName;
    CString m_csLdbObjOwner;
    CString m_csLdbDatabase;
    CString m_csLdbNode;
    CString m_csLdbDbmsType;
};

//
// SHOW NO ROWS
//
class CuDomPropDataTableRowsNA: public CuIpmPropertyData
{
  // Only CuDlgDomPropTableRowsNA will have access to private/protected member variables
  friend class CuDlgDomPropTableRowsNA;

    DECLARE_SERIAL (CuDomPropDataTableRowsNA)
public:
    CuDomPropDataTableRowsNA();
    virtual ~CuDomPropDataTableRowsNA(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTableRowsNA(const CuDomPropDataTableRowsNA & refPropDataTableRowsNA);
    CuDomPropDataTableRowsNA operator = (const CuDomPropDataTableRowsNA & refPropDataTableRowsNA);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
    CString m_csDummy;
};


//
// DOM PROP: AUDIT ALL USERS
// ***************************************************************
class CuDomPropDataPageInstallationLevelAuditAllUsers: public CuIpmPropertyData
{
    DECLARE_SERIAL (CuDomPropDataPageInstallationLevelAuditAllUsers)
public:
    CuDomPropDataPageInstallationLevelAuditAllUsers ();
    virtual ~CuDomPropDataPageInstallationLevelAuditAllUsers();

    virtual void NotifyLoad (CWnd* pDlg);
    virtual void Serialize  (CArchive& ar);

    CTypedPtrList<CObList, CuNameOnly*> m_listTuple;

};


//
// DOM PROP: ENABLED LEVEL
// ***************************************************************
class CaAuditEnabledLevel: public CObject
{
    DECLARE_SERIAL (CaAuditEnabledLevel)
public:
    CaAuditEnabledLevel(): m_strAuditFlag(_T("")), m_bEnabled (FALSE){}
    CaAuditEnabledLevel(LPCTSTR lpszAuditFlag, BOOL bEnable): m_strAuditFlag(lpszAuditFlag), m_bEnabled (bEnable){}
    virtual ~CaAuditEnabledLevel(){};
    virtual void Serialize  (CArchive& ar);

    CString m_strAuditFlag;
    BOOL m_bEnabled;
};

class CuDomPropDataPageInstallationLevelEnabledLevel: public CuIpmPropertyData
{
    DECLARE_SERIAL (CuDomPropDataPageInstallationLevelEnabledLevel)
public:
    CuDomPropDataPageInstallationLevelEnabledLevel ();
    virtual ~CuDomPropDataPageInstallationLevelEnabledLevel();

    virtual void NotifyLoad (CWnd* pDlg);
    virtual void Serialize  (CArchive& ar);

    CTypedPtrList<CObList, CaAuditEnabledLevel*> m_listTuple;

};


//
// DOM PROP: AUDIT LOG
// ***************************************************************
class CuDomPropDataPageInstallationLevelAuditLog: public CuIpmPropertyData
{
    DECLARE_SERIAL (CuDomPropDataPageInstallationLevelAuditLog)
public:
    CuDomPropDataPageInstallationLevelAuditLog ();
    virtual ~CuDomPropDataPageInstallationLevelAuditLog();

    virtual void NotifyLoad (CWnd* pDlg);
    virtual void Serialize  (CArchive& ar);

    int m_nSelEventType;
    int m_nSelDatabase;
    int m_nSelObjectType;
    int m_nSelObjectName;
    int m_nSelUser;
    int m_nSelRealUser;

    CString m_strBegin;
    CString m_strEnd;

    CStringList m_listEventType;
    CStringList m_listDatabase;
    CStringList m_listObjectType;
    CStringList m_listUser;
    CStringList m_listRealUser;
    CStringList m_listBegin;
    CStringList m_listEnd;

    CTypedPtrList<CObList, CaDBObject*> m_listObjectName;
    CuDomPropDataTableRows* m_pRowData;
};


#endif  // DOMSERIAL_HEADER
