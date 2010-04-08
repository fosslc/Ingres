/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : qresurow.h, Header File
**
**  Project  : CA-OpenIngres/Visual DBA.
**  Author   : UK Sotheavut.(uk$so01)
**  Purpose  : The rows data. (Tuple)
**
** History:
** xx-xx-1997 (uk$so01)
**    Created
** 15-feb-2000 (somsa01)
**    Removed extra comma from QueryPageDataType declaration
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
*/

#if !defined (QRESUROW_HEADER)
#define QRESUROW_HEADER


class CdQueryExecutionPlanDoc;
class CaQueryResultRowsHeader: public CObject
{
	DECLARE_SERIAL (CaQueryResultRowsHeader)
public:
	CaQueryResultRowsHeader():m_strHeader(""), m_nWidth(0), m_nAlign(0){};
	CaQueryResultRowsHeader(LPCTSTR lpszHeader, int nWidth, int nAlign): 
		m_strHeader(lpszHeader), m_nWidth(nWidth), m_nAlign(nAlign){}
	virtual ~CaQueryResultRowsHeader(){}
	virtual void Serialize (CArchive& ar);

	//
	// Data members
	int     m_nAlign;      // Column alignment
	int     m_nWidth;      // Width of the column;
	CString m_strHeader;   // Column header
};


class CaQueryResultRows: public CObject
{
	DECLARE_SERIAL (CaQueryResultRows)
public:

	CaQueryResultRows(){};
	virtual ~CaQueryResultRows();
	virtual void Serialize (CArchive& ar);

	//
	// Data members

	int m_nHeaderCount;
	CTypedPtrList<CObList, CaQueryResultRowsHeader*> m_listHeaders;  // List of header;
	CTypedPtrList<CObList, CStringList*>             m_listAdjacent; // Adjacent list
	CArray <int, int>                                m_aItemData;
};

typedef enum 
{
	QUERYPAGETYPE_UNKNOWN = 0,
	QUERYPAGETYPE_NONE,
	QUERYPAGETYPE_SELECT,
	QUERYPAGETYPE_NONSELECT,
	QUERYPAGETYPE_QEP,
	QUERYPAGETYPE_RAW,
	QUERYPAGETYPE_XML,
} QueryPageDataType;

class CaQueryPageData: public CObject
{
	DECLARE_SERIAL (CaQueryPageData)
public:
	CaQueryPageData();
	virtual ~CaQueryPageData(){}
	virtual void Serialize (CArchive& ar);

	//
	// Derived class must implement these following virtual members:
	virtual CWnd* LoadPage(CWnd* pParent){ASSERT(FALSE); return NULL;};
	virtual QueryPageDataType GetQueryPageType() { ASSERT(FALSE); return QUERYPAGETYPE_UNKNOWN; } 

public:
	//
	// Data members.
	UINT    m_nID;               // Dialog ID of the page
	int     m_nTabImage;         // Image ID of the Tab Control.

	BOOL    m_bShowStatement;    // TRUE: Page is considered to be old
	LONG    m_nStart;            // First position of the statement taken from the global statement
	LONG    m_nEnd;              // Last  position of the statement taken from the global statement
	CString m_strStatement;      // The statement (Sub string of the global statement)
	                             // Note: The global statement is the statement appears in the Edit Box.
	CString m_strDatabase;       // Database;
};


class CaQuerySelectPageData: public CaQueryPageData
{
	DECLARE_SERIAL (CaQuerySelectPageData)
public:
	CaQuerySelectPageData():m_nSelectedItem (-1), CaQueryPageData()
	{
		m_nTopIndex = 0;
		m_nMaxTuple = 0;
		m_strFetchInfo = _T("n/a");
	}
	virtual ~CaQuerySelectPageData(){}
	virtual void Serialize (CArchive& ar);

	virtual CWnd* LoadPage(CWnd* pParent);
	virtual QueryPageDataType GetQueryPageType() { return QUERYPAGETYPE_SELECT; }

public:
	//
	// Data members.
	CString m_strFetchInfo;        // Information about how many rows fetched
	int m_nSelectedItem;           // Selected Item
	int m_nTopIndex;               // Top Index. Save top index. Load -> Ensure its visibility.
	int m_nMaxTuple;               // Store it in order to use the same value at loading time. !!

	CaQueryResultRows m_listRows;
};


class CaQueryNonSelectPageData: public CaQueryPageData
{
	DECLARE_SERIAL (CaQueryNonSelectPageData)
public:
	CaQueryNonSelectPageData():CaQueryPageData(){}
	virtual ~CaQueryNonSelectPageData(){}
	virtual void Serialize (CArchive& ar);

	virtual CWnd* LoadPage(CWnd* pParent);
	virtual QueryPageDataType GetQueryPageType() { return QUERYPAGETYPE_NONSELECT; }

public:
	//
	// Data members.
};


class CaQueryQepPageData: public CaQueryPageData
{
	DECLARE_SERIAL (CaQueryQepPageData)
public:
	CaQueryQepPageData():CaQueryPageData(), m_pQepDoc(NULL){}
	virtual ~CaQueryQepPageData(){}
	virtual void Serialize (CArchive& ar);

	virtual CWnd* LoadPage(CWnd* pParent);
	virtual QueryPageDataType GetQueryPageType() { return QUERYPAGETYPE_QEP; }

public:
	//
	// Data members.
	CdQueryExecutionPlanDoc* m_pQepDoc;
};


class CaQueryRawPageData: public CaQueryPageData
{
	DECLARE_SERIAL (CaQueryRawPageData)
public:
	CaQueryRawPageData():CaQueryPageData(), m_strTrace(_T(""))
	{
		m_nLine = 0;
		m_nLength = 0;
		m_bLimitReached = FALSE;
	}
	virtual ~CaQueryRawPageData(){}
	virtual void Serialize (CArchive& ar);

	virtual CWnd* LoadPage(CWnd* pParent);
	virtual QueryPageDataType GetQueryPageType() { return QUERYPAGETYPE_RAW; }

public:
	//
	// Data members.
	CString m_strTrace;
	int  m_nLine;
	int  m_nLength;
	BOOL m_bLimitReached;

};

class CaExecParamGenerateXMLfromSelect;
class CaQueryXMLPageData: public CaQueryPageData
{
	DECLARE_SERIAL (CaQueryXMLPageData)
public:
	CaQueryXMLPageData();
	virtual ~CaQueryXMLPageData();
	virtual void Serialize (CArchive& ar);

	virtual CWnd* LoadPage(CWnd* pParent);
	virtual QueryPageDataType GetQueryPageType() { return QUERYPAGETYPE_RAW; }

public:
	//
	// Data members.
	CaExecParamGenerateXMLfromSelect* m_pParam;
};

#endif
