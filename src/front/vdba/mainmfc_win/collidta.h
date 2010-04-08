/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.                   //
//                                                                                     //
//    Source   : collidta.h, Header File.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : Ingres II / Visual DBA.                                               //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//    Purpose  : Data defintion for the management of Visual Collision Resolution.     //
****************************************************************************************/
#if !defined (VDBA_REPLICATION_COLLISION_DATA_HEADER)
#define VDBA_REPLICATION_COLLISION_DATA_HEADER

class CaCollisionItem: public CObject
{
	DECLARE_SERIAL (CaCollisionItem)
public:
	enum {TRANS_NORMAL, TRANS_RESOLVED, PREVAIL_UNRESOLVED, PREVAIL_SOURCE, PREVAIL_TARGET};
	CaCollisionItem()
		: m_nImageType(-1), m_nID(-1), m_bResolved(FALSE), m_bExpanded(FALSE), m_bSelected (FALSE){}
	CaCollisionItem(long nID, int nImage)
		: m_nImageType(nImage), m_nID(nID), m_bResolved(FALSE), m_bExpanded(FALSE), m_bSelected (FALSE){}

	virtual ~CaCollisionItem(){}
	virtual BOOL IsTransaction(){ASSERT (FALSE); return TRUE;}
	virtual void Serialize (CArchive& ar);
	virtual void SetImageType (int nImage){m_nImageType = nImage;}

	int  GetImageType(){return m_nImageType;}
	long GetID() {return m_nID;}
	void SetID(long nID) {m_nID = nID;}
	
	virtual void SetResolved (BOOL bResolved = TRUE){m_bResolved = bResolved;}
	BOOL IsResolved(){return m_bResolved;}
	void SetExpanded (BOOL bExpanded = TRUE){m_bExpanded = bExpanded;}
	BOOL IsExpanded(){return m_bExpanded;}
	void SetSelected (BOOL bSelected = TRUE){m_bSelected = bSelected;}
	BOOL IsSelected(){return m_bSelected;}


protected:
	int  m_nImageType;
	long m_nID;
	BOOL m_bResolved;
	BOOL m_bExpanded;
	BOOL m_bSelected;

};

class CaCollisionColumn: public CObject
{
	DECLARE_SERIAL (CaCollisionColumn)
public:
	CaCollisionColumn()
		: m_nKey(0), m_strColumn(_T("")),  m_strSource(_T("")), m_strTarget(_T("")), m_bConflicted(FALSE){}
	CaCollisionColumn(LPCTSTR lpszColumn, LPCTSTR lpszSource, LPCTSTR lpszTarget, int nKey = 0)
		:m_strColumn(lpszColumn),  m_strSource(lpszSource), m_strTarget(lpszTarget), m_nKey(nKey), m_bConflicted(FALSE){}

	virtual ~CaCollisionColumn(){}
	virtual void Serialize (CArchive& ar);
	void SetConflicted (BOOL bConflicted = TRUE){m_bConflicted = bConflicted;}
	BOOL IsConflicted(){return m_bConflicted;}

	int     m_nKey;
	CString m_strColumn;
	CString m_strSource;
	CString m_strTarget;
protected:
	BOOL    m_bConflicted;

};


class CaCollisionSequence: public CaCollisionItem
{
	DECLARE_SERIAL (CaCollisionSequence)
public:
	int m_nTblNo;
	int m_nSequenceNo;
	int m_nSourceDB;

	CaCollisionSequence(): CaCollisionItem(-1, CaCollisionItem::PREVAIL_UNRESOLVED){};
	CaCollisionSequence(long nID): CaCollisionItem(nID, CaCollisionItem::PREVAIL_UNRESOLVED){};

	virtual ~CaCollisionSequence();
	virtual void Serialize (CArchive& ar);
	virtual BOOL IsTransaction(){return FALSE;}

	virtual void SetImageType (int nImage)
	{
		ASSERT (
			nImage == CaCollisionItem::PREVAIL_UNRESOLVED || 
			nImage == CaCollisionItem::PREVAIL_SOURCE || 
			nImage == CaCollisionItem::PREVAIL_TARGET);
		m_nImageType = nImage;
	}
	int  GetResolvedType() {return GetImageType();}
	void AddColumn(LPCTSTR lpszColumn, LPCTSTR lpszSource, LPCTSTR lpszTarget, int nKey = 0, BOOL bConflicted = FALSE);


	int     m_nCDDS;
	long    m_nTransaction;
	long    m_nPrevTransaction;
	
	CString m_strSourceTransTime;
	CString m_strTargetTransTime;
	CString m_strCollisionType;

	CString m_strSourceVNode;
	CString m_strTargetVNode;
	CString m_strLocalVNode;
	CString m_strSourceDatabase;
	CString m_strTargetDatabase;
	CString m_strLocalDatabase;
	
	CString m_strTable;
	CString m_strTableOwner;

	int     m_nSvrTargetType;

	CTypedPtrList<CObList, CaCollisionColumn*> m_listColumns; 

};



class CaCollisionTransaction: public CaCollisionItem
{
	DECLARE_SERIAL (CaCollisionTransaction)
public:
	CaCollisionTransaction(): CaCollisionItem(-1, CaCollisionItem::TRANS_NORMAL){}
	CaCollisionTransaction(long nID): CaCollisionItem(nID, CaCollisionItem::TRANS_NORMAL){}

	virtual ~CaCollisionTransaction();
	virtual void Serialize (CArchive& ar);
	virtual BOOL IsTransaction(){return TRUE;}
	virtual void SetResolved (BOOL bResolved = TRUE)
	{
		m_bResolved = bResolved;
		m_nImageType = m_bResolved? CaCollisionItem::TRANS_RESOLVED: CaCollisionItem::TRANS_NORMAL;
	}

	CTypedPtrList<CObList, CaCollisionSequence*>& GetListCollisionSequence() {return m_listCollisionSequence;}
	BOOL SequenceWithSameNumber(long CurSeqID);
	void AddCollisionSequence (CaCollisionSequence* pNewSequence);

protected:
	CTypedPtrList<CObList, CaCollisionSequence*> m_listCollisionSequence; 
};



class CaCollision: public CObject
{
	DECLARE_SERIAL (CaCollision)
public:
	CaCollision(){}
	virtual ~CaCollision();
	virtual void Serialize (CArchive& ar);

	CTypedPtrList<CObList, CaCollisionTransaction*>& GetListTransaction() {return m_listTransaction;}
	void Cleanup();

	void AddTransaction       (long nTransID);
	void AddCollisionSequence (long nTransID, CaCollisionSequence* pNewSequence);

	CaCollisionTransaction* Find (long nTransID);                   // Return NULL if not found
protected:
	//
	// List of Transactions:
	CTypedPtrList<CObList, CaCollisionTransaction*> m_listTransaction; 


};



#endif // VDBA_REPLICATION_COLLISION_DATA_HEADER
