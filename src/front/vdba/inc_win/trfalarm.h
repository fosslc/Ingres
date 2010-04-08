/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlalarm.h: interface for the CtrfAlarm, CtrfFolderAlarm class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Alarm object and Folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFALARM_HEADER)
#define TRFALARM_HEADER
#include "trctldta.h"
#include "dmlalarm.h"
#include "comsessi.h"


class CtrfFolderAlarm;
//
// Object: Alarm
// ************************************************************************************************
class CtrfAlarm: public CtrfItemData
{
	DECLARE_SERIAL(CtrfAlarm)
public:
	CtrfAlarm(): CtrfItemData(O_ALARM){}
	CtrfAlarm(CaAlarm* pObj): CtrfItemData(O_ALARM)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfAlarm(){}
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetRuleImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaAlarm& GetObject(){return m_object;}
	void GetObject(CaAlarm*& pObject){pObject = new CaAlarm(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaAlarm m_object;
};


//
// Folder of Alarm:
// ************************************************************************************************
class CtrfFolderAlarm: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderAlarm)
	CtrfFolderAlarm();
public:
	CtrfFolderAlarm(int nObjectParentType);
	virtual ~CtrfFolderAlarm();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual UINT GetFolderFlag();
	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_ALARM);}


	CtrfItemData* SearchObject(CaAlarm* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

protected:
	CtrfItemData m_EmptyNode;
	int  m_nObjectParentType;

};


#endif // TRFALARM_HEADER