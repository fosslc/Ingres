/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : loadsave.h, Header File 
** Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Load / Store Schema in the compound file.
**
** Root (IStorage for Installation)
**   |_ SCHEMAINFO   (IStream)
**   |_ USER         (IStream)
**   |_ GROUP        (IStream)
**   |_ ROLE         (IStream)
**   |_ PROFILE      (IStream)
**   |_ LOCATION     (IStream)
**   |_ GRANTEE      (IStream)
**   |_ ALARM        (IStream)
**   |_ DBNAMExENTRY (IStream) List of databases
**   |_ <DB NAME>*   (IStorage) DB%08d where d is the position of database in DBNAMExENTRY
**       |_ DATABASE     (IStream). Detail of database
**       |_ SYNONYM      (IStream)  list of synonyms of in this database
**       |_ GRANTEE      (IStream)  list of grantees of database
**       |_ ALARM        (IStream)  list of alarm of database
**       |_ <TABLE>      (IStorage)
**       |     |_ TABLExENTRY  (IStream) List of tables
**       |     |_ <Table Name>*(IStorage) TB%08d where d is the position of table in TABLExENTRY
**       |        |_ TABLE     (IStream) detail of table
**       |        |_ INDEX     (IStream) list of indices in this table
**       |        |_ RULE      (IStream) list of rules in this table
**       |        |_ INTEGRITY (IStream) list of integrities in this table
**       |        |_ GRANTEE   (IStream) list of grantees of this table
**       |        |_ ALARM     (IStream) list of alarms in this table
**       |_ <VIEW>       (IStorage)
**       |     |_ VIEWxENTRY   (IStream) List of views
**       |     |_ <View Name>* (IStorage) VI%08d where d is the position of view in VIEWxENTRY
**       |        |_ VIEW      (IStream) detail of view
**       |        |_ GRANTEE   (IStream) list of grantees of this table
**       |_ <PROCEUDURE> (IStorage)
**       |     |_ PROCxENTRY   (IStream) List of Procedures
**       |     |_ <Procedure Name>* (IStorage) PR%08d where d is the position of procedure in PROCxENTRY
**       |        |_ PROCEDURE (IStream) detail of procedure
**       |        |_ GRANTEE   (IStream) list of grantees of this table
**       |_ <SEQUENCE> (IStorage)
**       |     |_ SEQUxENTRY   (IStream) List of Sequences
**       |     |_ <Sequence Name>* (IStorage) SE%08d where d is the position of Sequence in SEQUxENTRY
**       |        |_ SEQUENCE (IStream) detail of sequence
**       |        |_ GRANTEE   (IStream) list of grantees of this sequence
**       |_ <DBEVENT>    (IStorage)
**       |     |_ DBEVENTxENTRY(IStream) List of dbevents
**             |_ <Dbevent Name>* (IStorage) EV%08d where d is the position of dbevent in DBEVENTxENTRY
**                |_ DBEVENT   (IStream) detail of dbevent
**                |_ GRANTEE   (IStream) list of grantees of this table
**
** Root (IStorage for database schema)
**   |_ SCHEMAINFO (IStream)
**   |_ <DB NAME>  (IStorage)
**       |_ DATABASE     (IStream). Detail of database
**       |_ SYNONYM      (IStream)  list of synonyms of in this database
**       |_ GRANTEE      (IStream)  list of grantees of database
**       |_ ALARM        (IStream)  list of alarm of database
**       |_ <TABLE>      (IStorage)
**       |     |_ <Table Name>*
**       |        |_ TABLE     (IStream) detail of table
**       |        |_ INDEX     (IStream) list of indices in this table
**       |        |_ RULE      (IStream) list of rules in this table
**       |        |_ INTEGRITY (IStream) list of integrities in this table
**       |        |_ GRANTEE   (IStream) list of grantees of this table
**       |        |_ ALARM     (IStream) list of alarms in this table
**       |_ <VIEW>       (IStorage)
**       |     |_ <View Name>*
**       |        |_ VIEW      (IStream) detail of view
**       |        |_ GRANTEE   (IStream) list of grantees of this table
**       |_ <PROCEUDURE> (IStorage)
**       |     |_ <Procedure Name>*
**       |        |_ PROCEDURE (IStream) detail of procedure
**       |        |_ GRANTEE   (IStream) list of grantees of this table
**       |_ <DBEVENT>    (IStorage)
**             |_ <Dbevent Name>*
**                |_ DBEVENT   (IStream) detail of dbevent
**                |_ GRANTEE   (IStream) list of grantees of this table
**
** History:
**
** 17-Sep-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    Created
** 07-May-2003 (schph01)
**    SIR #107523, Add Sequence Objects
*/

#if !defined (_LOADSAVE_SCHEMA_HEADER)
#define _LOADSAVE_SCHEMA_HEADER
#include "snaparam.h"


//
// Object: CaVsdaLoadSchema 
// ************************************************************************************************
class CaDBObject;
class CaCompoundFile;
class CaDatabaseDetail;
class CaVsdaLoadSchema: public CaVsdStoreSchema
{
public:
	CaVsdaLoadSchema();
	~CaVsdaLoadSchema();
	void Close();

	void SetFile(LPCTSTR lpszFile1){m_strFile = lpszFile1;}
	CString GetFile(){return m_strFile;}
	BOOL LoadSchema ();

	void GetListUser(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListGroup(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListRole(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListProfile(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListLocation(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListDatabase(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);

	void GetListTable(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListView(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListProcedure(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListSequence(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListDBEvent(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListSynonym(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListIndex(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListRule(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListIntegrity(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListGrantee(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);
	void GetListAlarm(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);

	void GetDetailUser(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailGroup(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailRole(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailProfile(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailLocation(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailDatabase(CaDBObject*& pDetail);
	void GetDetailTable(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailView(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailProcedure(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailSequence(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailDBEvent(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailSynonym(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailIndex(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailRule(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailIntegrity(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailGrantee(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);
	void GetDetailAlarm(CaLLQueryInfo* pInfo, CaDBObject*& pDetail);

protected:
	IStorage* FindStorage (LPCTSTR lpszName);
	IStorage* FindIStorageDatabase(LPCTSTR lpszDatabase);
	IStorage* FindIRootTVDPObject(IStorage* pRootObject, int nObjectType, LPCTSTR lpObject, LPCTSTR lpOwner);

private:
	CaCompoundFile* m_pCompoundFile;

};


//
// If lpszUser = NULL then all users are considered.
BOOL VSD_StoreInstallation (CaVsdStoreSchema& storeSchema);
BOOL VSD_StoreSchema (CaVsdStoreSchema& storeSchema);
BOOL VSD_llQueryObject (CaLLQueryInfo* pInfo, CaVsdaLoadSchema* pLoadedSchema, CTypedPtrList<CObList, CaDBObject*>& listObject);
BOOL VSD_llQueryDetailObject(CaLLQueryInfo* pInfo, CaVsdaLoadSchema* pLoadedSchema, CaDBObject*& pDetail);

#endif // _LOADSAVE_SCHEMA_HEADER