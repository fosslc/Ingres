/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : exportdt.h: header file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Manage data for exporting 
**
** History:
**
** 18-Oct-2001 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**    Created
** 26-Feb-2003 (uk$so01)
**    SIR  #106952, Date conversion and numeric display format.
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
** 23-Mar-2005 (komve01)
**    Issue# 13919436, Bug#113774. 
**    Changed the return type of UpdateValueFixedWidth to return bool status.
**/

#if !defined (FORMAT_EXPORT_DATA_HEADER)
#define FORMAT_EXPORT_DATA_HEADER
#include "dmlcolum.h" // CaColumn.
#include "lsselres.h" // CaRowTransfer.
#include "lschrows.h" // CaHeaderRowItemData.
#include "xmlgener.h" 

//
// Type of the Exported File
typedef enum
{
	EXPORT_CSV = 0,
	EXPORT_DBF,
	EXPORT_XML,
	EXPORT_COPY_STATEMENT,
	EXPORT_FIXED_WIDTH,
	EXPORT_UNKNOWN
} ExportedFileType;


//
// CLASS: CaQueryResult (Result of the select statement)
// ************************************************************************************************
class CaQueryResult: public CObject
{
	DECLARE_SERIAL (CaQueryResult)
public:
	CaQueryResult();
	CaQueryResult(const CaQueryResult& c){Copy (c);}
	CaQueryResult operator = (const CaQueryResult& c) {Copy (c); return *this;}
	virtual ~CaQueryResult();
	virtual void Serialize (CArchive& ar);
	void Cleanup();
	CTypedPtrList< CObList, CaColumn* >& GetListColumn() {return m_listColumn;}
	CTypedPtrList< CObList, CaColumnExport* >& GetListColumnLayout(){return m_listLayout;}
	CPtrArray& GetListRecord(){return m_listRow;}

protected:
	void Copy (const CaQueryResult& c);
	CTypedPtrList< CObList, CaColumn* > m_listColumn;       // List of columns (initial value)
	CTypedPtrList< CObList, CaColumnExport* > m_listLayout; // List of columns (user can modify the characteristics)
	CPtrArray m_listRow;    // List of rows

};


//
// Step 1 (Page 1) data:
// ************************************************************************************************
class CaIeaDataPage1: public CObject
{
	DECLARE_SERIAL (CaIeaDataPage1)
public:
	CaIeaDataPage1();
	CaIeaDataPage1 (const CaIeaDataPage1& c){Copy (c);}
	CaIeaDataPage1 operator = (const CaIeaDataPage1& c){Copy (c); return * this;}

	virtual ~CaIeaDataPage1(){Cleanup();}
	virtual void Serialize (CArchive& ar);
	//
	// Manually manage the object version:
	// When you detect that you must manage the newer version (class members have been changed),
	// the derived class should override GetObjectVersion() and return
	// the value N same as IMPLEMENT_SERIAL (CaXyz, XXX, N).
	virtual UINT GetObjectVersion(){return 1;}
	void Cleanup()
	{
		m_QueryResult.Cleanup();
	}
	CStringArray& GetTreeItemPath(){return m_arrayTreeItemPath;}

	BOOL    GetGatewayDatabase() const {return m_bDatabaseGateway;}
	CString GetServerClass() const {return m_strServerClass;}
	CString GetNode() const {return m_strNode;}
	CString GetDatabase() const {return m_strDatabase;}
	CString GetFile2BeExported() const {return m_strFile2BeExported;}
	CString GetStatement() const {return m_strStatement;}
	ExportedFileType GetExportedFileType() const {return n_ExportFormat ;}
	CaQueryResult& GetQueryResult() {return m_QueryResult;}
	CaFloatFormat& GetFloatFormat(){return m_floatFormat;}


	void SetGatewayDatabase(BOOL bSet){m_bDatabaseGateway = bSet;}
	void SetServerClass(LPCTSTR lpszServerClass){m_strServerClass = lpszServerClass;}
	void SetNode(LPCTSTR lpszNode){m_strNode = lpszNode;}
	void SetDatabase(LPCTSTR lpszDatabase){m_strDatabase = lpszDatabase;}
	void SetFile2BeExported(LPCTSTR lpszFile){m_strFile2BeExported = lpszFile;}
	void SetStatement(LPCTSTR lpszStatement){m_strStatement = lpszStatement;}
	void SetExportedFileType(ExportedFileType nType){n_ExportFormat = nType;}

protected:
	void Copy (const CaIeaDataPage1& c);

	BOOL    m_bDatabaseGateway;          // The gateway database. (in case that we don't connect from server class.)
	CString m_strServerClass;            // Gateway (if not empty)
	CString m_strNode;                   // Virtual Node Name (Input or user's selection)
	CString m_strDatabase;               // Database (Input or user's selection)
	CString m_strFile2BeExported;        // File to be exported (Input or user's selection)
	CString m_strStatement;              // Select statement (Input or user's selection)
	ExportedFileType n_ExportFormat;     // EXPORT_FORMAT_CSV, EXPORT_FORMAT_DBF, ...

	CaFloatFormat m_floatFormat;

private:
	CStringArray m_arrayTreeItemPath;
	CaQueryResult m_QueryResult;
};


//
// Step 2 (Page 2) data:
// ************************************************************************************************
class CaIeaDataPage2: public CObject
{
	DECLARE_SERIAL (CaIeaDataPage2)
public:
	CaIeaDataPage2();
	CaIeaDataPage2 (const CaIeaDataPage2& c){Copy (c);}
	CaIeaDataPage2 operator = (const CaIeaDataPage2& c){Copy (c); return *this;}
	virtual ~CaIeaDataPage2(){Cleanup();}
	virtual void Serialize (CArchive& ar);
	//
	// Manually manage the object version:
	// When you detect that you must manage the newer version (class members have been changed),
	// the derived class should override GetObjectVersion() and return
	// the value N same as IMPLEMENT_SERIAL (CaXyz, XXX, N).
	virtual UINT GetObjectVersion(){return 1;}
	void Cleanup()
	{
	}

	void SetDelimiter(TCHAR tchDelimiter) {m_tchDelimiter = tchDelimiter;}
	void SetQuote(TCHAR tchQuote) {m_tchQuote = tchQuote;}
	void SetQuoteIfDelimiter(BOOL bSet) {m_bQuoteIfDelimiter = bSet;}
	void SetExportHeader(BOOL bSet) {m_bExportHeader = bSet;}
	void SetReadLock(BOOL bSet) {m_bReadLock = bSet;}

	TCHAR GetDelimiter() const {return m_tchDelimiter;}
	TCHAR GetQuote() const {return m_tchQuote;}
	BOOL  IsQuoteIfDelimiter() const {return m_bQuoteIfDelimiter;}
	BOOL  IsExportHeader() const {return m_bExportHeader;}
	BOOL  IsReadLock() const {return m_bReadLock;}
protected:
	void Copy (const CaIeaDataPage2& c);

	TCHAR m_tchDelimiter;      // Delimiter to be used
	TCHAR m_tchQuote;          // Quote character to be used (' or ")
	BOOL  m_bQuoteIfDelimiter; // TRUE: quote the field value only if it contains delimiter

	BOOL  m_bExportHeader;     // Export header (first line in the file)
	BOOL  m_bReadLock;         // TRUE: Lock=on while executing the query for exporting.

};




//
// CLASS CaIeaDataForLoadSave (data for load / save)
// ************************************************************************************************
class CaIEAInfo;
class CaIeaDataForLoadSave: public CObject
{
	DECLARE_SERIAL (CaIeaDataForLoadSave)
public:
	CaIeaDataForLoadSave();
	virtual ~CaIeaDataForLoadSave();
	virtual void Serialize (CArchive& ar);
	void Cleanup();

	//
	// LoadData(): Load data from the file and update the object pIEAData.
	void LoadData (CaIEAInfo* pIEAData, CString& strFile);
	//
	// SaveData(): Get data from object and save it into the file.
	void SaveData (CaIEAInfo* pIEAData, CString& strFile);

	void SetUsedByPage(int nPage){m_nUsedByPage = nPage;}
	int  GetUsedByPage(){return m_nUsedByPage;}

	//
	// GET: (Page1):
	CStringArray& GetTreeItemPath(){return m_dataPage1.GetTreeItemPath();}
	CString GetServerClass(){return m_dataPage1.GetServerClass();}
	CString GetNode(){return m_dataPage1.GetNode();}
	CString GetDatabase(){return m_dataPage1.GetDatabase();}
	CString GetFile2BeExported(){return m_dataPage1.GetFile2BeExported();}
	int     GetExportedFileType(){return m_dataPage1.GetExportedFileType();}
	CTypedPtrList< CObList, CaColumn* >& GetListColumn(){return m_listColumn;}

	//
	// GET: (Page2):
	TCHAR GetDelimiter(){return m_dataPage2.GetDelimiter();}
	TCHAR GetQuote(){return m_dataPage2.GetQuote();}
	BOOL  IsQuoteIfDelimiter(){return m_dataPage2.IsQuoteIfDelimiter();}
	BOOL  IsExportHeader(){return m_dataPage2.IsExportHeader();}
	BOOL  IsReadLock(){return m_dataPage2.IsReadLock();}

	//
	// SET (Page1):
	void SetServerClass(LPCTSTR lpszServerClass){m_dataPage1.SetServerClass(lpszServerClass);}
	void SetNode(LPCTSTR lpszNode){m_dataPage1.SetNode(lpszNode);}
	void SetDatabase(LPCTSTR lpszDatabase){m_dataPage1.SetDatabase(lpszDatabase);}
	void SetFile2BeExported(LPCTSTR lpszFile){m_dataPage1.SetFile2BeExported(lpszFile);}
	void SetExportedFileType(ExportedFileType nType){m_dataPage1.SetExportedFileType(nType);}

	//
	// SET: (Page2):
	void SetDelimiter(TCHAR tchDelimiter){ m_dataPage2.SetDelimiter(tchDelimiter);}
	void SetQuote(TCHAR tchQuote){ m_dataPage2.SetQuote(tchQuote);}
	void SetQuoteIfDelimiter(BOOL bSet){ m_dataPage2.SetQuoteIfDelimiter(bSet);}
	void SetExportHeader(BOOL bSet){ m_dataPage2.SetExportHeader(bSet);}
	void SetReadLock(BOOL bSet){ m_dataPage2.SetReadLock(bSet);}

	//
	// When the object is load from the file, m_nUsedByPage has a value 1.
	// When m_nUsedByPage has a value n (n >= 1) that means the pages 1 to n have already
	// used the data:
	int m_nUsedByPage;
	CTypedPtrList< CObList, CaColumn* > m_listColumn;  // List of columns (initial value)

	CaIeaDataPage1 m_dataPage1;
	CaIeaDataPage2 m_dataPage2;
};



//
// Global Export Assistant Data
// ************************************************************************************************
class CaIEAInfo
{
public:
	CaIEAInfo();
	~CaIEAInfo();
	CaIeaDataForLoadSave* GetLoadSaveData(){return m_pLoadSaveData;}
	void CleanLoadSaveData();
	void LoadData(CString& strFile);
	void SaveData(CString& strFile);
	void SetInterrupted(BOOL bSet){m_bInterrupted = bSet;}
	BOOL GetInterrupted(){return m_bInterrupted;}
	void UpdateFromLoadData(); // Fill the current structure by the loaded data.
	void SetBatchMode (BOOL bSilent, BOOL bOverWrite, LPCTSTR lpszLogFile);
	void GetBatchMode (BOOL& bSilent, BOOL& bOverWrite, CString& strLogFile);

	CaIeaDataPage1 m_dataPage1;
	CaIeaDataPage2 m_dataPage2;
protected:
	BOOL m_bInterrupted;
	CaIeaDataForLoadSave* m_pLoadSaveData;
	BOOL m_bSilent;
	BOOL m_bOverWrite;
	CString m_strLogFile;
};


//
// CLASS CaIngresExportHeaderRowItemData: 
// ************************************************************************************************
class CaIngresExportHeaderRowItemData: public CaHeaderRowItemData
{
public:
	CaIngresExportHeaderRowItemData():CaHeaderRowItemData()
	{
		m_bAllowChange   = TRUE;
		m_bGateway = FALSE;
	}
	virtual ~CaIngresExportHeaderRowItemData(){Cleanup();}

	virtual void Display (CuListCtrlDoubleUpper* pListCtrl, int i);
	virtual void EditValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell);
	virtual BOOL UpdateValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue);
	
	void Cleanup()
	{
		m_arrayColumn.RemoveAll();
		m_bAllowChange = FALSE;
		m_bGateway = FALSE;
	}
	void SetExportType(ExportedFileType expft){m_exportType = expft;}

	CTypedPtrArray < CPtrArray, CaColumnExport* > m_arrayColumn;
	BOOL m_bAllowChange;
	BOOL m_bGateway;
	ExportedFileType m_exportType;
protected:
	void LayoutHeaderCsv(CuListCtrlDoubleUpper* pListCtrl, int i);
	void LayoutHeaderXml(CuListCtrlDoubleUpper* pListCtrl, int i);
	void LayoutHeaderDbf(CuListCtrlDoubleUpper* pListCtrl, int i);
	void LayoutHeaderIngresCopy(CuListCtrlDoubleUpper* pListCtrl, int i);
	void LayoutHeaderFixedWidth(CuListCtrlDoubleUpper* pListCtrl, int i);

	void EditValueCsv(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell);
	void EditValueXml(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell);
	void EditValueDbf(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell);
	void EditValueIngresCopy(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell);
	void EditValueFixedWidth(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell);

	void UpdateValueCsv(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue);
	void UpdateValueXml(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue);
	void UpdateValueDbf(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue);
	void UpdateValueIngresCopy(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue);
	BOOL UpdateValueFixedWidth(CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue);
};


#endif // FORMAT_EXPORT_DATA_HEADER
