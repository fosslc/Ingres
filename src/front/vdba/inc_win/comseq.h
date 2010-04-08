/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comseq.h: interface for the CmtSequence, CmtFolderSequence class
**    Project  : Meta data library 
**    Author   : Schalk Philippe (schph01)
**    Purpose  : Sequence object and folder
**
** History:
**
** 22-April-2003 (schph01)
**    SIR 107523 Created
**/


#if !defined (COMSEQUENCE_HEADER)
#define COMSEQUENCE_HEADER
#include "combase.h"
#include "dmlseq.h"
#include "comaccnt.h"

class CmtFolderSequence;
class CmtDatabase;

//
// Object: Sequence 
// *****************
class CmtSequence: public CmtItemData, public CmtProtect
{
public:
	CmtSequence(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtSequence(CaSequence* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtSequence(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderSequence* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderSequence* GetBackParent(){return m_pBackParent;}
	CaSequence& GetSequence(){return m_object;}
	void GetSequence(CaSequence*& pObject){pObject = new CaSequence(m_object);}

protected:
	CaSequence m_object;
	CmtFolderSequence* m_pBackParent;

};

//
// Object: Folder Sequence 
// *************************
class CmtFolderSequence: public CmtItemData, public CmtProtect
{
public:
	CmtFolderSequence():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderSequence();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtDatabase* pBackParent){m_pBackParent = pBackParent;}
	CmtDatabase* GetBackParent(){return m_pBackParent;}
	CmtSequence* SearchObject (CaLLQueryInfo* pInfo, CaSequence* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtSequence* > m_listObject;
	CmtDatabase* m_pBackParent;
};

#endif // COMSEQUENCE_HEADER