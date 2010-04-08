/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : DomSeri2.h, Header File 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : Emmanuel Blattes 
**
**    Purpose  : Serialization of DOM right pane property pages (modeless dialog)
**
** History:
**
** xx-Dec-1997 (Emmanuel Blattes)
**    Created.
** 27-Mar-2001 (noifr01)
**    (sir 104270) removal of code for managing Ingres/Desktop
** 10-May-2001 (noifr01)
**      (bug 104694) added the missing member variables in the CuDomPropDataPages class
** 21-Mar-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
** 02-Apr-2003 (schph01)
**    SIR #107523 Declare new class for sequences.
*/

#ifndef DOMSERIAL2_HEADER
#define DOMSERIAL2_HEADER

#include "ipmprdta.h"
#include "multcol.h"    // Utility classes for multicolumn dom detail windows
#include "alarms.h"     // classes for alarms matrix management
#include "grantees.h"   // classes for grantees matrix management
#include "columns.h"    // classes for table columns management
#include "replic.h"     // Classes for replicator multicolumn management (cdds propagation paths and database information, columns for a registered table, etc)
#include "granted.h"    // classes for granted (xref) matrix management
#include "tbstatis.h"   // Statistic data
#include "domrfsh.h"    // dom refresh params management for detail panes

//
// REPLICATOR CDDS
//
class CuDomPropDataReplicCdds: public CuIpmPropertyData
{
  // Only CuDlgDomPropReplicCdds will have access to private/protected member variables
  friend class CuDlgDomPropReplicCdds;

    DECLARE_SERIAL (CuDomPropDataReplicCdds)
public:
    CuDomPropDataReplicCdds();
    virtual ~CuDomPropDataReplicCdds(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataReplicCdds(const CuDomPropDataReplicCdds & refPropDataReplicCdds);
    CuDomPropDataReplicCdds operator = (const CuDomPropDataReplicCdds & refPropDataReplicCdds);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CStringArray m_acsReplicCdds;
};

//
// REPLICATOR CONNECTIONS
//
class CuDomPropDataReplicConn: public CuIpmPropertyData
{
  // Only CuDlgDomPropReplicConn will have access to private/protected member variables
  friend class CuDlgDomPropReplicConn;

    DECLARE_SERIAL (CuDomPropDataReplicConn)
public:
    CuDomPropDataReplicConn();
    virtual ~CuDomPropDataReplicConn(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataReplicConn(const CuDomPropDataReplicConn & refPropDataReplicConn);
    CuDomPropDataReplicConn operator = (const CuDomPropDataReplicConn & refPropDataReplicConn);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithNumber  m_uaReplicConn;
};

//
// REPLICATOR MAIL USERS
//
class CuDomPropDataReplicMailU: public CuIpmPropertyData
{
  // Only CuDlgDomPropReplicMailU will have access to private/protected member variables
  friend class CuDlgDomPropReplicMailU;

    DECLARE_SERIAL (CuDomPropDataReplicMailU)
public:
    CuDomPropDataReplicMailU();
    virtual ~CuDomPropDataReplicMailU(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataReplicMailU(const CuDomPropDataReplicMailU & refPropDataReplicMailU);
    CuDomPropDataReplicMailU operator = (const CuDomPropDataReplicMailU & refPropDataReplicMailU);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CStringArray m_acsReplicMailU;
};

//
// REPLICATOR REGISTERED TABLES
//
class CuDomPropDataReplicRegTbl: public CuIpmPropertyData
{
  // Only CuDlgDomPropReplicRegTbl will have access to private/protected member variables
  friend class CuDlgDomPropReplicRegTbl;

    DECLARE_SERIAL (CuDomPropDataReplicRegTbl)
public:
    CuDomPropDataReplicRegTbl();
    virtual ~CuDomPropDataReplicRegTbl(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataReplicRegTbl(const CuDomPropDataReplicRegTbl & refPropDataReplicRegTbl);
    CuDomPropDataReplicRegTbl operator = (const CuDomPropDataReplicRegTbl & refPropDataReplicRegTbl);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaReplicRegTbl;
};


//
// DB TABLES
//
class CuDomPropDataDbTbl: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbTbl and CuDlgDomPropDDbTbl will have access to private/protected member variables
  friend class CuDlgDomPropDbTbl;
  friend class CuDlgDomPropDDbTbl;

    DECLARE_SERIAL (CuDomPropDataDbTbl)
public:
    CuDomPropDataDbTbl();
    virtual ~CuDomPropDataDbTbl(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbTbl(const CuDomPropDataDbTbl & refPropDataDbTbl);
    CuDomPropDataDbTbl operator = (const CuDomPropDataDbTbl & refPropDataDbTbl);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaDbTbl;
  int m_objType;
};

//
// DB VIEWS
//
class CuDomPropDataDbView: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbView and CuDlgDomPropDDbView will have access to private/protected member variables
  friend class CuDlgDomPropDbView;
  friend class CuDlgDomPropDDbView;

    DECLARE_SERIAL (CuDomPropDataDbView)
public:
    CuDomPropDataDbView();
    virtual ~CuDomPropDataDbView(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbView(const CuDomPropDataDbView & refPropDataDbView);
    CuDomPropDataDbView operator = (const CuDomPropDataDbView & refPropDataDbView);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaDbView;
  int m_objType;
};


//
// DB PROCEDURES
//
class CuDomPropDataDbProc: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbProc and CuDlgDomPropDDbProc will have access to private/protected member variables
  friend class CuDlgDomPropDbProc;
  friend class CuDlgDomPropDDbProc;

    DECLARE_SERIAL (CuDomPropDataDbProc)
public:
    CuDomPropDataDbProc();
    virtual ~CuDomPropDataDbProc(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbProc(const CuDomPropDataDbProc & refPropDataDbProc);
    CuDomPropDataDbProc operator = (const CuDomPropDataDbProc & refPropDataDbProc);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaDbProc;
  int m_objType;
};

//
// DB SEQUENCES
//
class CuDomPropDataDbSeq: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbProc and CuDlgDomPropDDbProc will have access to private/protected member variables
  friend class CuDlgDomPropDbSeq;
  friend class CuDlgDomPropDDbSeq;

    DECLARE_SERIAL (CuDomPropDataDbSeq)
public:
    CuDomPropDataDbSeq();
    virtual ~CuDomPropDataDbSeq(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbSeq(const CuDomPropDataDbSeq & refPropDataDbSeq);
    CuDomPropDataDbSeq operator = (const CuDomPropDataDbSeq & refPropDataDbSeq);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaDbSeq;
  int m_objType;
};

//
// DB SCHEMAS
//
class CuDomPropDataDbSchema: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbSchema will have access to private/protected member variables
  friend class CuDlgDomPropDbSchema;

    DECLARE_SERIAL (CuDomPropDataDbSchema)
public:
    CuDomPropDataDbSchema();
    virtual ~CuDomPropDataDbSchema(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbSchema(const CuDomPropDataDbSchema & refPropDataDbSchema);
    CuDomPropDataDbSchema operator = (const CuDomPropDataDbSchema & refPropDataDbSchema);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly m_uaDbSchema;
  int m_objType;
};


//
// DB SYNONYMS
//
class CuDomPropDataDbSyn: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbSyn will have access to private/protected member variables
  friend class CuDlgDomPropDbSyn;

    DECLARE_SERIAL (CuDomPropDataDbSyn)
public:
    CuDomPropDataDbSyn();
    virtual ~CuDomPropDataDbSyn(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbSyn(const CuDomPropDataDbSyn & refPropDataDbSyn);
    CuDomPropDataDbSyn operator = (const CuDomPropDataDbSyn & refPropDataDbSyn);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaDbSyn;
  int m_objType;
};


//
// DB DBEVENTS
//
class CuDomPropDataDbEvt: public CuIpmPropertyData
{
  // Only CuDlgDomPropDbEvt will have access to private/protected member variables
  friend class CuDlgDomPropDbEvt;

    DECLARE_SERIAL (CuDomPropDataDbEvt)
public:
    CuDomPropDataDbEvt();
    virtual ~CuDomPropDataDbEvt(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataDbEvt(const CuDomPropDataDbEvt & refPropDataDbEvt);
    CuDomPropDataDbEvt operator = (const CuDomPropDataDbEvt & refPropDataDbEvt);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaDbEvt;
  int m_objType;
};

//
// TABLE INTEGRITIES
//
class CuDomPropDataTblInteg: public CuIpmPropertyData
{
  // Only CuDlgDomPropTblInteg will have access to private/protected member variables
  friend class CuDlgDomPropTblInteg;

    DECLARE_SERIAL (CuDomPropDataTblInteg)
public:
    CuDomPropDataTblInteg();
    virtual ~CuDomPropDataTblInteg(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTblInteg(const CuDomPropDataTblInteg & refPropDataTblInteg);
    CuDomPropDataTblInteg operator = (const CuDomPropDataTblInteg & refPropDataTblInteg);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaTblInteg;
  int m_objType;
};

//
// TABLE RULES
//
class CuDomPropDataTblRule: public CuIpmPropertyData
{
  // Only CuDlgDomPropTblRule will have access to private/protected member variables
  friend class CuDlgDomPropTblRule;

    DECLARE_SERIAL (CuDomPropDataTblRule)
public:
    CuDomPropDataTblRule();
    virtual ~CuDomPropDataTblRule(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTblRule(const CuDomPropDataTblRule & refPropDataTblRule);
    CuDomPropDataTblRule operator = (const CuDomPropDataTblRule & refPropDataTblRule);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaTblRule;
  int m_objType;
};

//
// TABLE INDEXES
//
class CuDomPropDataTblIdx: public CuIpmPropertyData
{
  // Only CuDlgDomPropTblIdx and CuDlgDomPropTableLinkIdx will have access to private/protected member variables
  friend class CuDlgDomPropTblIdx;
  friend class CuDlgDomPropTableLinkIdx;

    DECLARE_SERIAL (CuDomPropDataTblIdx)
public:
    CuDomPropDataTblIdx();
    virtual ~CuDomPropDataTblIdx(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTblIdx(const CuDomPropDataTblIdx & refPropDataTblIdx);
    CuDomPropDataTblIdx operator = (const CuDomPropDataTblIdx & refPropDataTblIdx);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaTblIdx;
  int m_objType;
};

//
// TABLE LOCATIONS
//
class CuDomPropDataTblLoc: public CuIpmPropertyData
{
  // Only CuDlgDomPropTblLoc will have access to private/protected member variables
  friend class CuDlgDomPropTblLoc;

    DECLARE_SERIAL (CuDomPropDataTblLoc)
public:
    CuDomPropDataTblLoc();
    virtual ~CuDomPropDataTblLoc(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataTblLoc(const CuDomPropDataTblLoc & refPropDataTblLoc);
    CuDomPropDataTblLoc operator = (const CuDomPropDataTblLoc & refPropDataTblLoc);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaTblLoc;
  int m_objType;
};

//
// USER RELATED TABLE SECURITY ALARMS (XALARMS)
//
class CuDomPropDataUserXAlarm: public CuIpmPropertyData
{
  // Only CuDlgDomPropUserXAlarms will have access to private/protected member variables
  friend class CuDlgDomPropUserXAlarms;

    DECLARE_SERIAL (CuDomPropDataUserXAlarm)
public:
    CuDomPropDataUserXAlarm();
    virtual ~CuDomPropDataUserXAlarm(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUserXAlarm(const CuDomPropDataUserXAlarm & refPropDataUserXAlarm);
    CuDomPropDataUserXAlarm operator = (const CuDomPropDataUserXAlarm & refPropDataUserXAlarm);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayUserXAlarms m_uaUserXAlarm;
};

//
// USER RELATED GRANTED DATABASES
//
class CuDomPropDataUserXGrantedDb: public CuIpmPropertyData
{
  // Only CuDlgDomPropUserXGrantedDb will have access to private/protected member variables
  friend class CuDlgDomPropUserXGrantedDb;

    DECLARE_SERIAL (CuDomPropDataUserXGrantedDb)
public:
    CuDomPropDataUserXGrantedDb();
    virtual ~CuDomPropDataUserXGrantedDb(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUserXGrantedDb(const CuDomPropDataUserXGrantedDb & refPropDataUserXGrantedDb);
    CuDomPropDataUserXGrantedDb operator = (const CuDomPropDataUserXGrantedDb & refPropDataUserXGrantedDb);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedDbs m_uaUserXGrantedDb;
};


//
// USER RELATED GRANTED TABLES
//
class CuDomPropDataUserXGrantedTbl: public CuIpmPropertyData
{
  // Only CuDlgDomPropUserXGrantedTbl will have access to private/protected member variables
  friend class CuDlgDomPropUserXGrantedTbl;

    DECLARE_SERIAL (CuDomPropDataUserXGrantedTbl)
public:
    CuDomPropDataUserXGrantedTbl();
    virtual ~CuDomPropDataUserXGrantedTbl(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUserXGrantedTbl(const CuDomPropDataUserXGrantedTbl & refPropDataUserXGrantedTbl);
    CuDomPropDataUserXGrantedTbl operator = (const CuDomPropDataUserXGrantedTbl & refPropDataUserXGrantedTbl);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedTbls m_uaUserXGrantedTbl;
};

//
// USER RELATED GRANTED VIEWS
//
class CuDomPropDataUserXGrantedView: public CuIpmPropertyData
{
  // Only CuDlgDomPropUserXGrantedView will have access to private/protected member variables
  friend class CuDlgDomPropUserXGrantedView;

    DECLARE_SERIAL (CuDomPropDataUserXGrantedView)
public:
    CuDomPropDataUserXGrantedView();
    virtual ~CuDomPropDataUserXGrantedView(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUserXGrantedView(const CuDomPropDataUserXGrantedView & refPropDataUserXGrantedView);
    CuDomPropDataUserXGrantedView operator = (const CuDomPropDataUserXGrantedView & refPropDataUserXGrantedView);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedViews m_uaUserXGrantedView;
};

//
// USER RELATED GRANTED DBEVENTS
//
class CuDomPropDataUserXGrantedEvent: public CuIpmPropertyData
{
  // Only CuDlgDomPropUserXGrantedEvent will have access to private/protected member variables
  friend class CuDlgDomPropUserXGrantedEvent;

    DECLARE_SERIAL (CuDomPropDataUserXGrantedEvent)
public:
    CuDomPropDataUserXGrantedEvent();
    virtual ~CuDomPropDataUserXGrantedEvent(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUserXGrantedEvent(const CuDomPropDataUserXGrantedEvent & refPropDataUserXGrantedEvent);
    CuDomPropDataUserXGrantedEvent operator = (const CuDomPropDataUserXGrantedEvent & refPropDataUserXGrantedEvent);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedEvents m_uaUserXGrantedEvent;
};

//
// USER RELATED GRANTED PROCEDURES
//
class CuDomPropDataUserXGrantedProc: public CuIpmPropertyData
{
  // Only CuDlgDomPropUserXGrantedProc will have access to private/protected member variables
  friend class CuDlgDomPropUserXGrantedProc;

    DECLARE_SERIAL (CuDomPropDataUserXGrantedProc)
public:
    CuDomPropDataUserXGrantedProc();
    virtual ~CuDomPropDataUserXGrantedProc(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUserXGrantedProc(const CuDomPropDataUserXGrantedProc & refPropDataUserXGrantedProc);
    CuDomPropDataUserXGrantedProc operator = (const CuDomPropDataUserXGrantedProc & refPropDataUserXGrantedProc);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedProcs m_uaUserXGrantedProc;
};

//
// USER RELATED GRANTED SEQUENCES
//
class CuDomPropDataUserXGrantedSeq: public CuIpmPropertyData
{
  // Only CuDlgDomPropUserXGrantedProc will have access to private/protected member variables
  friend class CuDlgDomPropUserXGrantedSeq;

    DECLARE_SERIAL (CuDomPropDataUserXGrantedSeq)
public:
    CuDomPropDataUserXGrantedSeq();
    virtual ~CuDomPropDataUserXGrantedSeq(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUserXGrantedSeq(const CuDomPropDataUserXGrantedSeq & refPropDataUserXGrantedSeq);
    CuDomPropDataUserXGrantedSeq operator = (const CuDomPropDataUserXGrantedSeq & refPropDataUserXGrantedSeq);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedSeqs m_uaUserXGrantedSeq;
};

//
// USER RELATED GRANTED ROLES
//
class CuDomPropDataUserXGrantedRole: public CuIpmPropertyData
{
  // Only CuDlgDomPropUserXGrantedRole will have access to private/protected member variables
  friend class CuDlgDomPropUserXGrantedRole;

    DECLARE_SERIAL (CuDomPropDataUserXGrantedRole)
public:
    CuDomPropDataUserXGrantedRole();
    virtual ~CuDomPropDataUserXGrantedRole(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataUserXGrantedRole(const CuDomPropDataUserXGrantedRole & refPropDataUserXGrantedRole);
    CuDomPropDataUserXGrantedRole operator = (const CuDomPropDataUserXGrantedRole & refPropDataUserXGrantedRole);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedRoles m_uaUserXGrantedRole;
};

//
// GROUP RELATED GRANTED DATABASES
//
class CuDomPropDataGroupXGrantedDb: public CuIpmPropertyData
{
  // Only CuDlgDomPropGroupXGrantedDb will have access to private/protected member variables
  friend class CuDlgDomPropGroupXGrantedDb;

    DECLARE_SERIAL (CuDomPropDataGroupXGrantedDb)
public:
    CuDomPropDataGroupXGrantedDb();
    virtual ~CuDomPropDataGroupXGrantedDb(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataGroupXGrantedDb(const CuDomPropDataGroupXGrantedDb & refPropDataGroupXGrantedDb);
    CuDomPropDataGroupXGrantedDb operator = (const CuDomPropDataGroupXGrantedDb & refPropDataGroupXGrantedDb);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedDbs m_uaGroupXGrantedDb;
};

//
// GROUP RELATED GRANTED TABLES
//
class CuDomPropDataGroupXGrantedTbl: public CuIpmPropertyData
{
  // Only CuDlgDomPropGroupXGrantedTbl will have access to private/protected member variables
  friend class CuDlgDomPropGroupXGrantedTbl;

    DECLARE_SERIAL (CuDomPropDataGroupXGrantedTbl)
public:
    CuDomPropDataGroupXGrantedTbl();
    virtual ~CuDomPropDataGroupXGrantedTbl(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataGroupXGrantedTbl(const CuDomPropDataGroupXGrantedTbl & refPropDataGroupXGrantedTbl);
    CuDomPropDataGroupXGrantedTbl operator = (const CuDomPropDataGroupXGrantedTbl & refPropDataGroupXGrantedTbl);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedTbls m_uaGroupXGrantedTbl;
};

//
// GROUP RELATED GRANTED VIEWS
//
class CuDomPropDataGroupXGrantedView: public CuIpmPropertyData
{
  // Only CuDlgDomPropGroupXGrantedView will have access to private/protected member variables
  friend class CuDlgDomPropGroupXGrantedView;

    DECLARE_SERIAL (CuDomPropDataGroupXGrantedView)
public:
    CuDomPropDataGroupXGrantedView();
    virtual ~CuDomPropDataGroupXGrantedView(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataGroupXGrantedView(const CuDomPropDataGroupXGrantedView & refPropDataGroupXGrantedView);
    CuDomPropDataGroupXGrantedView operator = (const CuDomPropDataGroupXGrantedView & refPropDataGroupXGrantedView);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedViews m_uaGroupXGrantedView;
};

//
// GROUP RELATED GRANTED DBEVENTS
//
class CuDomPropDataGroupXGrantedEvent: public CuIpmPropertyData
{
  // Only CuDlgDomPropGroupXGrantedEvent will have access to private/protected member variables
  friend class CuDlgDomPropGroupXGrantedEvent;

    DECLARE_SERIAL (CuDomPropDataGroupXGrantedEvent)
public:
    CuDomPropDataGroupXGrantedEvent();
    virtual ~CuDomPropDataGroupXGrantedEvent(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataGroupXGrantedEvent(const CuDomPropDataGroupXGrantedEvent & refPropDataGroupXGrantedEvent);
    CuDomPropDataGroupXGrantedEvent operator = (const CuDomPropDataGroupXGrantedEvent & refPropDataGroupXGrantedEvent);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedEvents m_uaGroupXGrantedEvent;
};

//
// GROUP RELATED GRANTED PROCEDURES
//
class CuDomPropDataGroupXGrantedProc: public CuIpmPropertyData
{
  // Only CuDlgDomPropGroupXGrantedProc will have access to private/protected member variables
  friend class CuDlgDomPropGroupXGrantedProc;

    DECLARE_SERIAL (CuDomPropDataGroupXGrantedProc)
public:
    CuDomPropDataGroupXGrantedProc();
    virtual ~CuDomPropDataGroupXGrantedProc(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataGroupXGrantedProc(const CuDomPropDataGroupXGrantedProc & refPropDataGroupXGrantedProc);
    CuDomPropDataGroupXGrantedProc operator = (const CuDomPropDataGroupXGrantedProc & refPropDataGroupXGrantedProc);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedProcs m_uaGroupXGrantedProc;
};

//
// GROUP RELATED GRANTED SEQUENCES
//
class CuDomPropDataGroupXGrantedSeq: public CuIpmPropertyData
{
  // Only CuDlgDomPropGroupXGrantedSeq will have access to private/protected member variables
  friend class CuDlgDomPropGroupXGrantedSeq;

    DECLARE_SERIAL (CuDomPropDataGroupXGrantedSeq)
public:
    CuDomPropDataGroupXGrantedSeq();
    virtual ~CuDomPropDataGroupXGrantedSeq(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataGroupXGrantedSeq(const CuDomPropDataGroupXGrantedSeq & refPropDataGroupXGrantedSeq);
    CuDomPropDataGroupXGrantedSeq operator = (const CuDomPropDataGroupXGrantedSeq & refPropDataGroupXGrantedSeq);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedSeqs m_uaGroupXGrantedSeq;
};

//
// ROLE RELATED GRANTED DATABASES
//
class CuDomPropDataRoleXGrantedDb: public CuIpmPropertyData
{
  // Only CuDlgDomPropRoleXGrantedDb will have access to private/protected member variables
  friend class CuDlgDomPropRoleXGrantedDb;

    DECLARE_SERIAL (CuDomPropDataRoleXGrantedDb)
public:
    CuDomPropDataRoleXGrantedDb();
    virtual ~CuDomPropDataRoleXGrantedDb(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataRoleXGrantedDb(const CuDomPropDataRoleXGrantedDb & refPropDataRoleXGrantedDb);
    CuDomPropDataRoleXGrantedDb operator = (const CuDomPropDataRoleXGrantedDb & refPropDataRoleXGrantedDb);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedDbs m_uaRoleXGrantedDb;
};

//
// ROLE RELATED GRANTED TABLES
//
class CuDomPropDataRoleXGrantedTbl: public CuIpmPropertyData
{
  // Only CuDlgDomPropRoleXGrantedTbl will have access to private/protected member variables
  friend class CuDlgDomPropRoleXGrantedTbl;

    DECLARE_SERIAL (CuDomPropDataRoleXGrantedTbl)
public:
    CuDomPropDataRoleXGrantedTbl();
    virtual ~CuDomPropDataRoleXGrantedTbl(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataRoleXGrantedTbl(const CuDomPropDataRoleXGrantedTbl & refPropDataRoleXGrantedTbl);
    CuDomPropDataRoleXGrantedTbl operator = (const CuDomPropDataRoleXGrantedTbl & refPropDataRoleXGrantedTbl);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedTbls m_uaRoleXGrantedTbl;
};

//
// ROLE RELATED GRANTED VIEWS
//
class CuDomPropDataRoleXGrantedView: public CuIpmPropertyData
{
  // Only CuDlgDomPropRoleXGrantedView will have access to private/protected member variables
  friend class CuDlgDomPropRoleXGrantedView;

    DECLARE_SERIAL (CuDomPropDataRoleXGrantedView)
public:
    CuDomPropDataRoleXGrantedView();
    virtual ~CuDomPropDataRoleXGrantedView(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataRoleXGrantedView(const CuDomPropDataRoleXGrantedView & refPropDataRoleXGrantedView);
    CuDomPropDataRoleXGrantedView operator = (const CuDomPropDataRoleXGrantedView & refPropDataRoleXGrantedView);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedViews m_uaRoleXGrantedView;
};

//
// ROLE RELATED GRANTED DBEVENTS
//
class CuDomPropDataRoleXGrantedEvent: public CuIpmPropertyData
{
  // Only CuDlgDomPropRoleXGrantedEvent will have access to private/protected member variables
  friend class CuDlgDomPropRoleXGrantedEvent;

    DECLARE_SERIAL (CuDomPropDataRoleXGrantedEvent)
public:
    CuDomPropDataRoleXGrantedEvent();
    virtual ~CuDomPropDataRoleXGrantedEvent(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataRoleXGrantedEvent(const CuDomPropDataRoleXGrantedEvent & refPropDataRoleXGrantedEvent);
    CuDomPropDataRoleXGrantedEvent operator = (const CuDomPropDataRoleXGrantedEvent & refPropDataRoleXGrantedEvent);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedEvents m_uaRoleXGrantedEvent;
};

//
// ROLE RELATED GRANTED PROCEDURES
//
class CuDomPropDataRoleXGrantedProc: public CuIpmPropertyData
{
  // Only CuDlgDomPropRoleXGrantedProc will have access to private/protected member variables
  friend class CuDlgDomPropRoleXGrantedProc;

    DECLARE_SERIAL (CuDomPropDataRoleXGrantedProc)
public:
    CuDomPropDataRoleXGrantedProc();
    virtual ~CuDomPropDataRoleXGrantedProc(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataRoleXGrantedProc(const CuDomPropDataRoleXGrantedProc & refPropDataRoleXGrantedProc);
    CuDomPropDataRoleXGrantedProc operator = (const CuDomPropDataRoleXGrantedProc & refPropDataRoleXGrantedProc);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedProcs m_uaRoleXGrantedProc;
};

//
// ROLE RELATED GRANTED SEQUENCES
//
class CuDomPropDataRoleXGrantedSeq: public CuIpmPropertyData
{
  // Only CuDlgDomPropRoleXGrantedSeq will have access to private/protected member variables
  friend class CuDlgDomPropRoleXGrantedSeq;

    DECLARE_SERIAL (CuDomPropDataRoleXGrantedSeq)
public:
    CuDomPropDataRoleXGrantedSeq();
    virtual ~CuDomPropDataRoleXGrantedSeq(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataRoleXGrantedSeq(const CuDomPropDataRoleXGrantedSeq & refPropDataRoleXGrantedSeq);
    CuDomPropDataRoleXGrantedSeq operator = (const CuDomPropDataRoleXGrantedSeq & refPropDataRoleXGrantedSeq);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayGrantedSeqs m_uaRoleXGrantedSeq;
};


//
// CONNECTION
//
class CuDomPropDataConnection: public CuIpmPropertyData
{
  // Only CuDlgDomPropConnection will have access to private/protected member variables
  friend class CuDlgDomPropConnection;

  DECLARE_SERIAL (CuDomPropDataConnection)

public:
  CuDomPropDataConnection();
  virtual ~CuDomPropDataConnection(){};

  // copy constructor and '=' operator overloading
  CuDomPropDataConnection(const CuDomPropDataConnection & refPropDataConnection);
  CuDomPropDataConnection operator = (const CuDomPropDataConnection & refPropDataConnection);

  virtual void NotifyLoad (CWnd* pDlg);  
  virtual void Serialize (CArchive& ar);

public:
  CuDomRefreshParams m_refreshParams;

private:
  CString m_csDescription;
  int     m_nNumber;
  CString m_csVnode;
  CString m_csDb;
  CString m_csType;
  BOOL    m_bLocal;
};

//
// SCHEMA TABLES
//
class CuDomPropDataSchemaTbl: public CuIpmPropertyData
{
  // Only CuDlgDomPropSchemaTbl will have access to private/protected member variables
  friend class CuDlgDomPropSchemaTbl;

    DECLARE_SERIAL (CuDomPropDataSchemaTbl)
public:
    CuDomPropDataSchemaTbl();
    virtual ~CuDomPropDataSchemaTbl(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataSchemaTbl(const CuDomPropDataSchemaTbl & refPropDataSchemaTbl);
    CuDomPropDataSchemaTbl operator = (const CuDomPropDataSchemaTbl & refPropDataSchemaTbl);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaSchemaTbl;
};

//
// SCHEMA VIEWS
//
class CuDomPropDataSchemaView: public CuIpmPropertyData
{
  // Only CuDlgDomPropSchemaView will have access to private/protected member variables
  friend class CuDlgDomPropSchemaView;

    DECLARE_SERIAL (CuDomPropDataSchemaView)
public:
    CuDomPropDataSchemaView();
    virtual ~CuDomPropDataSchemaView(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataSchemaView(const CuDomPropDataSchemaView & refPropDataSchemaView);
    CuDomPropDataSchemaView operator = (const CuDomPropDataSchemaView & refPropDataSchemaView);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaSchemaView;
};


//
// SCHEMA PROCEDURES
//
class CuDomPropDataSchemaProc: public CuIpmPropertyData
{
  // Only CuDlgDomPropSchemaProc will have access to private/protected member variables
  friend class CuDlgDomPropSchemaProc;

    DECLARE_SERIAL (CuDomPropDataSchemaProc)
public:
    CuDomPropDataSchemaProc();
    virtual ~CuDomPropDataSchemaProc(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataSchemaProc(const CuDomPropDataSchemaProc & refPropDataSchemaProc);
    CuDomPropDataSchemaProc operator = (const CuDomPropDataSchemaProc & refPropDataSchemaProc);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaSchemaProc;
};

//
// TABLE AND INDEX PAGES
//
class CuDomPropDataPages: public CuIpmPropertyData
{
  // Only CuDlgDomPropTblPages and CuDlgDomPropIndexPages will have access to private/protected member variables
  friend class CuDlgDomPropTblPages;
  friend class CuDlgDomPropIndexPages;

    DECLARE_SERIAL (CuDomPropDataPages)
public:
    CuDomPropDataPages();
    virtual ~CuDomPropDataPages();

    // copy constructor and '=' operator overloading
    CuDomPropDataPages(const CuDomPropDataPages & refPropDataPages);
    CuDomPropDataPages operator = (const CuDomPropDataPages & refPropDataPages);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

    void SetMustDeleteArray() { m_bMustDeleteArray = TRUE; }

public:
  CuDomRefreshParams m_refreshParams;

private:
  // pages counts
  long  m_lTotal;
  long  m_lInUse;
  long  m_lOverflow;
  long  m_lFreed;
  long  m_lNeverUsed;

  // for cellular control
  long  m_lItemsPerByte;
  long  m_nbElements;
  CByteArray* m_pByteArray;

  // choice of pages per cell
  long  m_lCurPgPerCell;

  // Is structure of type "Hash" ?
  BOOL m_bHashed;

  // If created with copy constructor or after a load,
  // it is our responsibility to delete pByteArray  in order to avoid free chunks
  BOOL  m_bMustDeleteArray;

  long m_lbuckets_no_ovf;
  long m_lemptybuckets_no_ovf;
  long m_lbuck_with_ovf;
  long m_lemptybuckets_with_ovf;
  long m_loverflow_not_empty;
  long m_loverflow_empty;
  long m_llongest_ovf_chain;
  long m_lbuck_longestovfchain;
  double m_favg_ovf_chain;

};

//
// STATIC DATABASES
//
class CuDomPropDataStaticDb: public CuIpmPropertyData
{
  // Only CuDlgDomPropStaticDb will have access to private/protected member variables
  friend class CuDlgDomPropStaticDb;

    DECLARE_SERIAL (CuDomPropDataStaticDb)
public:
    CuDomPropDataStaticDb();
    virtual ~CuDomPropDataStaticDb(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataStaticDb(const CuDomPropDataStaticDb & refPropDataStaticDb);
    CuDomPropDataStaticDb operator = (const CuDomPropDataStaticDb & refPropDataStaticDb);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameWithOwner m_uaStaticDb;
  CWordArray           m_awDbType;    // Normal, DDB, CDB
};

//
// STATIC PROFILES
//
class CuDomPropDataStaticProfile: public CuIpmPropertyData
{
  // Only CuDlgDomPropStaticProfile will have access to private/protected member variables
  friend class CuDlgDomPropStaticProfile;

    DECLARE_SERIAL (CuDomPropDataStaticProfile)
public:
    CuDomPropDataStaticProfile();
    virtual ~CuDomPropDataStaticProfile(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataStaticProfile(const CuDomPropDataStaticProfile & refPropDataStaticProfile);
    CuDomPropDataStaticProfile operator = (const CuDomPropDataStaticProfile & refPropDataStaticProfile);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly   m_uaStaticProfile;
};

//
// STATIC USERE
//
class CuDomPropDataStaticUser: public CuIpmPropertyData
{
  // Only CuDlgDomPropStaticUser will have access to private/protected member variables
  friend class CuDlgDomPropStaticUser;

    DECLARE_SERIAL (CuDomPropDataStaticUser)
public:
    CuDomPropDataStaticUser();
    virtual ~CuDomPropDataStaticUser(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataStaticUser(const CuDomPropDataStaticUser & refPropDataStaticUser);
    CuDomPropDataStaticUser operator = (const CuDomPropDataStaticUser & refPropDataStaticUser);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly   m_uaStaticUser;
};

//
// STATIC GROUPS
//
class CuDomPropDataStaticGroup: public CuIpmPropertyData
{
  // Only CuDlgDomPropStaticGroup will have access to private/protected member variables
  friend class CuDlgDomPropStaticGroup;

    DECLARE_SERIAL (CuDomPropDataStaticGroup)
public:
    CuDomPropDataStaticGroup();
    virtual ~CuDomPropDataStaticGroup(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataStaticGroup(const CuDomPropDataStaticGroup & refPropDataStaticGroup);
    CuDomPropDataStaticGroup operator = (const CuDomPropDataStaticGroup & refPropDataStaticGroup);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly   m_uaStaticGroup;
};

//
// STATIC ROLES
//
class CuDomPropDataStaticRole: public CuIpmPropertyData
{
  // Only CuDlgDomPropStaticRole will have access to private/protected member variables
  friend class CuDlgDomPropStaticRole;

    DECLARE_SERIAL (CuDomPropDataStaticRole)
public:
    CuDomPropDataStaticRole();
    virtual ~CuDomPropDataStaticRole(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataStaticRole(const CuDomPropDataStaticRole & refPropDataStaticRole);
    CuDomPropDataStaticRole operator = (const CuDomPropDataStaticRole & refPropDataStaticRole);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly   m_uaStaticRole;
};

//
// STATIC LOCATIONS
//
class CuDomPropDataStaticLocation: public CuIpmPropertyData
{
  // Only CuDlgDomPropStaticLocation will have access to private/protected member variables
  friend class CuDlgDomPropStaticLocation;

    DECLARE_SERIAL (CuDomPropDataStaticLocation)
public:
    CuDomPropDataStaticLocation();
    virtual ~CuDomPropDataStaticLocation(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataStaticLocation(const CuDomPropDataStaticLocation & refPropDataStaticLocation);
    CuDomPropDataStaticLocation operator = (const CuDomPropDataStaticLocation & refPropDataStaticLocation);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly   m_uaStaticLocation;
};

//
// STATIC DATABASES ON GATEWAY ( IDMS, DATACOMM, ETC... )
//
class CuDomPropDataStaticGwDb: public CuIpmPropertyData
{
  // Only CuDlgDomPropStaticGwDb will have access to private/protected member variables
  friend class CuDlgDomPropStaticGwDb;

    DECLARE_SERIAL (CuDomPropDataStaticGwDb)
public:
    CuDomPropDataStaticGwDb();
    virtual ~CuDomPropDataStaticGwDb(){};

    // copy constructor and '=' operator overloading
    CuDomPropDataStaticGwDb(const CuDomPropDataStaticGwDb & refPropDataStaticGwDb);
    CuDomPropDataStaticGwDb operator = (const CuDomPropDataStaticGwDb & refPropDataStaticGwDb);

    virtual void NotifyLoad (CWnd* pDlg);  
    virtual void Serialize (CArchive& ar);

private:
  CuArrayNameOnly   m_uaStaticGwDb;
};


#endif  // DOMSERIAL2_HEADER
