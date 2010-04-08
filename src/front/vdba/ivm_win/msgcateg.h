/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : msgcateg.h, Header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : Visual Manager                                                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Define Message Categories and Notification Levels                     //
****************************************************************************************/
/* History:
* --------
* uk$so01: (21-May-1999 created)
*
*
*/


#if !defined(AFX_MSGCATEG_H__D636D272_14C9_11D3_A2EF_00609725DDAF__INCLUDED_)
#define AFX_MSGCATEG_H__D636D272_14C9_11D3_A2EF_00609725DDAF__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

typedef enum
{
	NODE_NEW = 0,
	NODE_EXIST,
	NODE_DELETE
} NODEINFOTYPE;


typedef enum 
{
	IMSG_NOTIFY = 0,
	IMSG_ALERT,
	IMSG_DISCARD
} Imsgclass;

class CaMessage : public CObject
{
	DECLARE_SERIAL (CaMessage)
public:
	CaMessage();
	CaMessage(LPCTSTR lpszTitle, long lCode, long lCat, Imsgclass msgClass);

	virtual ~CaMessage();
	virtual void Serialize (CArchive& ar);

	//
	// Title (msg text):
	CString GetTitle(){return m_strTitle;}
	void SetTitle(LPCTSTR lpszTitle){m_strTitle = lpszTitle;}
	//
	// Msg code:
	long GetCode(){return m_lCode;}
	void  SetCode(long lCode){m_lCode = lCode;}
	//
	// Msg Category:
	void SetCategory(long lCat){m_lCategory = lCat;}
	long GetCategory(){return m_lCategory;}

	//
	// Msg classification:
	Imsgclass GetClass(){return m_msgClass;}
	void  SetClass(Imsgclass msgClass){m_msgClass = msgClass;}


	CaMessage* Clone();

protected:
	CString   m_strTitle;
	long      m_lCode;
	long      m_lCategory;
	Imsgclass m_msgClass;
};


class CaMessageEntry: public CaMessage
{
	DECLARE_SERIAL (CaMessageEntry)
public:
	CaMessageEntry();
	CaMessageEntry(LPCTSTR lpszTitle, long lCode, long lCat, Imsgclass msgClass);
	virtual ~CaMessageEntry();
	virtual void Serialize (CArchive& ar);

	CaMessage* Add (CaMessage* pNewMessage);
	void Remove (long nID);
	void Remove (CaMessage* pMessage);
	CaMessage* Search (long nID);

	CaMessageEntry* Clone();
	CTypedPtrList<CObList, CaMessage*>& GetListMessage(){return m_listMessage;}

	CString GetIngresCategoryTitle(){return m_strTitleIngresCategory;}
	void SetIngresCategoryTitle(LPCTSTR lpszText){m_strTitleIngresCategory = lpszText;}
protected:
	CString   m_strTitleIngresCategory;
	CTypedPtrList<CObList, CaMessage*> m_listMessage;
};

class CaMessageManager : public CObject
{
	DECLARE_SERIAL (CaMessageManager)
public:
	CaMessageManager();
	virtual ~CaMessageManager();
	virtual void Serialize (CArchive& ar);

	//
	// Not null if the message has been classified:
	CaMessage* Search (long nCategory, long nID);

	//
	// Update this from m:
	void Update (CaMessageManager& m);
	CaMessage* Add (long nCategory, CaMessage* pNewMessage);
	void Remove (long nCategory, long nID);
	void Remove (CaMessage* pMessage);
	//
	// Entry (or the Other Message):
	CaMessageEntry* FindEntry (long nCategory);
	CaMessageManager* Clone();
	CTypedPtrArray <CObArray, CaMessageEntry*>& GetListEntry(){return m_arrayMsgEntry;}

	void Output();
protected:

	CTypedPtrArray <CObArray, CaMessageEntry*> m_arrayMsgEntry;

};



//
// Serializeable user defined categories:
class CaCategoryDataUser: public CObject
{
	DECLARE_SERIAL (CaCategoryDataUser)
public:
	CaCategoryDataUser();
	CaCategoryDataUser(long nCat, long nCode, BOOL bFolder, LPCTSTR lpszPath);

	virtual ~CaCategoryDataUser();
	virtual void Serialize (CArchive& ar);
	
	BOOL IsFolder(){return m_bFolder;}
	void SetFolder(BOOL bFolder){m_bFolder = bFolder;}
	CString GetPath(){return m_strPath;}
	void SetPath(LPCTSTR lpszPath){m_strPath = lpszPath;}

	void SetCategory (long nCategory){m_nCategory = nCategory;}
	long GetCategory (){return m_nCategory;}
	void SetCode (long nCode){m_nMsgCode = nCode;}
	long GetCode (){return m_nMsgCode;}

protected:
	CString m_strPath; // Ex: F0/F1 (The root has a folder F0, and F0 has Sub Folder F1)
	                   //     m001  (The root has an Item m001)
	BOOL m_bFolder;    // Terminal or Folder.
	long m_nCategory;  // if (m_bFolder == FALSE) category of the message of terminal item.
	long m_nMsgCode;   // (m_bFolder == FALSE) the code of the message of terminal item.
};


class CaCategoryDataUserManager: public CObject
{
	DECLARE_SERIAL (CaCategoryDataUserManager)
public:
	CaCategoryDataUserManager();
	virtual ~CaCategoryDataUserManager();
	virtual void Serialize (CArchive& ar);

//	BOOL LoadCategory();
//	BOOL SaveCategory();

	CTypedPtrList<CObList, CaCategoryDataUser*>& GetListUserFolder(){return m_listUserFolder;}

	void Output();
protected:
	CTypedPtrList<CObList, CaCategoryDataUser*> m_listUserFolder;
	CString m_userSpecializedFile;
	BOOL m_bSimulate;

};


#define M_CLASSIFIED 0x0001
#define M_ALERT      0x0002
#define M_NOTIFY     0x0004
#define M_DISCARD    0x0008


class CaMessageItemData: public CObject
{
public:
	CaMessageItemData(LPCTSTR lpszText, long lCat, long lCode, UINT nState = M_NOTIFY)
	{
		m_lCategory = lCat;
		m_lCode = lCode;
		m_nState = nState;
		m_strText = lpszText;
		m_nCount = 1;
	}
	virtual ~CaMessageItemData(){}
	void SetCount(int nCount){m_nCount = nCount;}
	int  GetCount(){return m_nCount;}


	long m_lCategory;
	long m_lCode;
	UINT m_nState;
	CString m_strText;
protected:
	int m_nCount;
};


#endif // !defined(AFX_MSGCATEG_H__D636D272_14C9_11D3_A2EF_00609725DDAF__INCLUDED_)
