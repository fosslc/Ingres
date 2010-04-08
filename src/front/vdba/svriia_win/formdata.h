/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : formdata.h: interface for the CaFormatData class
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : FILE FORMAT IDENTIFICATION DATA 
**
** History:
**
** 08-Nov-2000 (uk$so01)
**    Created
** 17-Oct-2001 (uk$so01)
**    SIR #103852, miss some behaviour.
** 17-Jan-2002 (uk$so01)
**    (bug 106844). Add BOOL m_bStopScanning to indicate if we really stop
**     scanning the file due to the limitation.
** 21-Jan-2002 (uk$so01)
**    (bug 106844). Additional fix: Replace ifstream and some of CFile.by FILE*.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 03-Apr-2002 (uk$so01)
**    BUG #107488, Make svriia.dll enable to import empty .DBF file
**    It just create the table.
** 04-Apr-2002 (uk$so01)
**    BUG #107505, import column that has length 0, record length != sum (column's length)
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
** 10-Mar-2010 (drivi01) SIR 123397
**    If a user tries to import an empty file into IIA, the utility 
**    needs to provide a better error message.  Before this change
**    the error said something about missing carriage return/line feed.
**    Add function File_Empty to check for the empty file.
**/

#if !defined (FORMDATA_HEADER)
#define FORMDATA_HEADER
#include "dmlbase.h"
#include "dmlcolum.h"
#include "dmlcsv.h"
#include "dmldbf.h"
#include "lschrows.h"
#include "ieatree.h"


//
// Type of the Imported File
typedef enum
{
	IMPORT_UNKNOWN = 0,
	IMPORT_CSV,
	IMPORT_DBF,
	IMPORT_XML,
	IMPORT_TXT
} ImportedFileType;


//
// CLASS CaIngresHeaderRowItemData: 
// ************************************************************************************************
class CaIngresHeaderRowItemData: public CaHeaderRowItemData
{
public:
	CaIngresHeaderRowItemData():CaHeaderRowItemData()
	{
		m_bExistingTable = TRUE;
		m_bAllowChange   = TRUE;
		m_bGateway = FALSE;
	}
	virtual ~CaIngresHeaderRowItemData(){Cleanup();}

	virtual void Display (CuListCtrlDoubleUpper* pListCtrl, int i);
	virtual void EditValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CRect rcCell);
	virtual BOOL UpdateValue (CuListCtrlDoubleUpper* pListCtrl, int iItem, int iSubItem, CString& strNewValue);
	
	void Cleanup()
	{
		m_arrayColumn.RemoveAll();
		m_arrayColumnOrder.RemoveAll();
		m_bExistingTable = FALSE;
		m_bAllowChange = FALSE;
		m_bGateway = FALSE;
	}

	CTypedPtrArray < CPtrArray, CaColumn* > m_arrayColumn;
	CTypedPtrArray < CPtrArray, CaColumn* > m_arrayColumnOrder;
	BOOL m_bExistingTable;
	BOOL m_bAllowChange;
	BOOL m_bGateway;
};



//
// CLASS CaRecord:
// ************************************************************************************************
class CaRecord
{
public:
	CaRecord(){}
	~CaRecord(){m_arrayFields.RemoveAll();}
	CStringArray& GetArrayFields(){return m_arrayFields;}
protected:
	CStringArray m_arrayFields;
};



//
// Step 1 (Page 1) data:
// ************************************************************************************************
#define MAX_ENUMPAGESIZE 6
class CaDataPage1 
{
public:
	enum {ANSI = 0, OEM};
	CaDataPage1();
	virtual ~CaDataPage1(){Cleanup();}
	void Cleanup()
	{
		while (!m_listColumn.IsEmpty())
			delete m_listColumn.RemoveHead();
		m_arrayLine.RemoveAll();
		m_arrayTreeItemPath.RemoveAll();
		CleanupFailureInfo();
		for (int i=0; i<MAX_ENUMPAGESIZE; i++) 
			m_bArrayPageSize[i] = FALSE;
		m_bStopScanning = FALSE;
	}
	CStringArray& GetTreeItemPath(){return m_arrayTreeItemPath;}
	CaIeaTree& GetTreeData(){return m_ieaTree;}

	BOOL    GetGatewayDatabase(){return m_bDatabaseGateway;}
	CString GetServerClass(){return m_strServerClass;}
	CString GetNode(){return m_strNode;}
	CString GetDatabase(){return m_strDatabase;}
	CString GetTable(){return m_strTable;}
	CString GetTableOwner(){return m_strTableOwner;}
	CString GetFileImportParameter(){return m_strFileImportParameter;}
	CString GetFile2BeImported(){return m_strFile2BeImported;}
	int GetCodePage(){return m_nCodePage;}
	int GetFileMatchTable(){return m_nFileMatchTable;}
	BOOL IsExistingTable(){return m_bExistingTable;}
	ImportedFileType GetImportedFileType(){return m_typeImportedFile ;}
	CTypedPtrList< CObList, CaDBObject* >& GetTableColumns(){return m_listColumn;}
	CString GetCustomSeparator () {return m_strOthersSeparators;}
	CString GetNewTable(){return m_strNewTable;}
	CStringArray& GetArrayTextLine()
	{
		ASSERT(GetImportedFileType() == IMPORT_TXT || GetImportedFileType() == IMPORT_CSV);
		return m_arrayLine;
	}
	CTypedPtrList< CObList, CaFailureInfo* >& GetListFailureInfo(){return m_listFailureInfo;}
	
	int  GetMaxRecordLength(){return m_nMaxRecordLen;}
	int  GetMinRecordLength(){return m_nMinRecordLen;}
	int  GetFileSize(){return m_nFileSize;}
	int  GetKBToScan(){return m_nKB2Scan;}
	int  GetLineCountToSkip(){return m_nLineCountToSkip;}
	BOOL*GetArrayPageSize(){return m_bArrayPageSize;}
	UINT GetLastDetection(){return m_nLastDetection;}
	BOOL GetStopScanning(){return m_bStopScanning;}
	BOOL GetFirstLineAsHeader(){return m_bFirstLineHeader;}

	void SetGatewayDatabase(BOOL bSet){m_bDatabaseGateway = bSet;}
	void SetServerClass(LPCTSTR lpszServerClass){m_strServerClass = lpszServerClass;}
	void SetNode(LPCTSTR lpszNode){m_strNode = lpszNode;}
	void SetDatabase(LPCTSTR lpszDatabase){m_strDatabase = lpszDatabase;}
	void SetTable(LPCTSTR lpszTable, LPCTSTR lpszOwner){m_strTable = lpszTable; m_strTableOwner = lpszOwner;}
	void SetFileImportParameter(LPCTSTR lpszFile){m_strFileImportParameter = lpszFile;}
	void SetFile2BeImported(LPCTSTR lpszFile){m_strFile2BeImported = lpszFile;}
	void SetCodePage(int nVal){m_nCodePage = nVal;}
	void SetFileMatchTable(int nVal){m_nFileMatchTable = nVal;}
	void SetExistingTable(BOOL bSet){m_bExistingTable = bSet;}
	void SetImportedFileType(ImportedFileType nType){m_typeImportedFile = nType;}
	void SetCustomSeparator (LPCTSTR lpString) {m_strOthersSeparators = lpString;}
	void SetNewTable(LPCTSTR lpString){m_strNewTable = lpString;}
	void SetMaxRecordLength(int nMax){m_nMaxRecordLen = nMax;}
	void SetMinRecordLength(int nMin){m_nMinRecordLen = nMin;}
	void SetFileSize(int nSize){m_nFileSize = nSize;}
	void SetKBToScan(int nBK){m_nKB2Scan = nBK;}
	void SetLineCountToSkip(int nCount){m_nLineCountToSkip = nCount;}
	void SetNewDetection(){m_nLastDetection++;}
	void SetStopScanning(BOOL bSet){m_bStopScanning = bSet;}
	void SetFirstLineAsHeader(BOOL bSet){m_bFirstLineHeader = bSet;}

	void CleanupFailureInfo();
protected:
	CaIeaTree m_ieaTree;

	BOOL    m_bDatabaseGateway;          // The gateway database. (in case that we don't connect from server class.)
	CString m_strServerClass;            // Gateway (if not empty)
	CString m_strNode;                   // Virtual Node Name (Input or user's selection)
	CString m_strDatabase;               // Database (Input or user's selection)
	CString m_strTable;                  // Table (Input or user's selection)
	CString m_strTableOwner;             // Table Owner (Input or user's selection)

	CString m_strFileImportParameter;    // File containing the parameters (user's selection)
	CString m_strFile2BeImported;        // File to be imported (Input or user's selection)
	CString m_strNewTable;               // Table name to be created on the fly.
	BOOL m_bExistingTable;               // TRUE if user selected the existing table:
	int m_nCodePage;                     // 0=ANSI, 1=OEM
	//
	// Number and order of columns of the file expected to match
	// exactly those of the Ingres Table ?
	int m_nFileMatchTable;               // 1=YES,  0=NO 

	ImportedFileType m_typeImportedFile; // File type (extension)
	CString m_strOthersSeparators;       // User provides the custom separators (each sep is separated by a space)
	//
	// Available only if m_bExistingTable = TRUE. Otherwise at step 3, this list will
	// contain the specified columns of Table to be created on the FLY:
	CTypedPtrList< CObList, CaDBObject* > m_listColumn;

	//
	// For the text file mode, this array contains the lines from the file:
	CStringArray m_arrayLine;
	int m_nMaxRecordLen;
	int m_nMinRecordLen;

	int  m_nFileSize;        // File size (in KB).
	int  m_nKB2Scan;         // Number KB to scan. 0: scan the whole file.
	int  m_nLineCountToSkip; // Number of lines to skip.
	BOOL m_bStopScanning;    // TRUE = has been stopped scanning due to limitation.
	BOOL m_bFirstLineHeader; // Only for CSV file and m_nLineCountToSkip = 1
private:
	UINT m_nLastDetection;   // The ID of detection:
	CStringArray m_arrayTreeItemPath;
	CTypedPtrList< CObList, CaFailureInfo* > m_listFailureInfo;
	BOOL m_bArrayPageSize[MAX_ENUMPAGESIZE];
};


//
// Step 2 (Page 2) data:
// ************************************************************************************************
class CaDataPage2
{
public:
	CaDataPage2():m_pSeparatorSet(NULL), m_pSeparator(NULL), m_nSeparatorType(FMT_UNKNOWN)
	{
		m_nColumnCount = 0;
		m_nDBFThericRecordSize = 0;
		m_bFixedWidthUser = FALSE;
	}
	virtual ~CaDataPage2(){Cleanup();}
	void Cleanup()
	{
		while (!m_listChoice.IsEmpty())
			delete m_listChoice.RemoveHead();
		int i, nSize = m_arrayRecord.GetSize();
		for (i=0; i<nSize; i++)
		{
			CaRecord* pRec = (CaRecord*)m_arrayRecord.GetAt(i);
			delete pRec;
		}
		m_arrayRecord.RemoveAll();
		m_arrayColumn.RemoveAll();
		m_arrayColumnType.RemoveAll();
		m_arrayDividerPos.RemoveAll();
		CleanFieldSizeInfo();
	}
	CTypedPtrList < CObList, CaSeparatorSet* >& GetListChoice(){return m_listChoice;}

	void SetColumnCount(int nCount){m_nColumnCount = nCount;}
	void SetCurrentSeparatorType(ColumnSeparatorType nType){m_nSeparatorType = nType;}
	void SetCurrentFormat (CaSeparatorSet* pSet, CaSeparator* pSeparator)
	{
		m_pSeparatorSet = pSet;
		m_pSeparator = pSeparator;
	}
	void SetDBFTheoricRecordSize(int nSize){m_nDBFThericRecordSize = nSize;}
	void SetFixedWidthUser(BOOL bUser){m_bFixedWidthUser = bUser;}

	int GetColumnCount(){return m_nColumnCount;}
	ColumnSeparatorType GetCurrentSeparatorType(){return m_nSeparatorType;}
	void GetCurrentFormat (CaSeparatorSet*& pSet, CaSeparator*& pSeparator)
	{
		pSet = m_pSeparatorSet;
		pSeparator = m_pSeparator;
	}
	BOOL GetFixedWidthUser(){return m_bFixedWidthUser;}
	CPtrArray& GetArrayRecord(){return m_arrayRecord;}
	//
	// This array is available for only the file in which we can 
	// query the column names. Ex the .DBF file.
	CStringArray& GetArrayColumns(){return m_arrayColumn;}
	CStringArray& GetArrayColumnType(){return m_arrayColumnType;}

	//
	// Available for Fixed Width: (array of column positions)
	CArray < int, int >& GetArrayDividerPos(){return m_arrayDividerPos;}
	int GetDBFTheoricRecordSize(){return m_nDBFThericRecordSize;}
	int GetFieldSizeInfoCount(){return m_arrayFieldSizeInfo.GetSize();}
	//
	// The members: CleanFieldSizeInfo(), SetFieldSizeInfo(), GetFieldSizeMax(), GetFieldSizeEffective()
	// are for manipulating column length: (MAX and Effective=no trailing space)
	void CleanFieldSizeInfo();
	void SetFieldSizeInfo (int nNewSize);
	int& GetFieldSizeMax(int nIndex);
	int& GetFieldSizeEffectiveMax(int nIndex);
	BOOL& IsTrailingSpace(int nIndex);
protected:
	//
	// The object CaSeparatorSet* in the list is for constructing one Tab:
	CTypedPtrList < CObList, CaSeparatorSet* > m_listChoice;

	//
	// Fixed width? Separator Separated ?
	ColumnSeparatorType m_nSeparatorType;

	//
	// Current Format (Available for the CSV format only):
	CaSeparatorSet* m_pSeparatorSet;
	CaSeparator* m_pSeparator;
	//
	// Available for Fixed Width: (array of column positions)
	CArray < int, int > m_arrayDividerPos;

	//
	// Number of Columns detected from the imported file:
	int m_nColumnCount;

	//
	// The array of void pointer (void*) to the class CaRecord.
	// In order to optimize the use of memory, this array is shared to be used
	// in step3 and step4 the list of the preview rows:
	CPtrArray m_arrayRecord;

	//
	// This array is available for only the file in which we can 
	// query the column names. Ex the .DBF file.
	CStringArray m_arrayColumn;
	CStringArray m_arrayColumnType;

	class CaColumnSize
	{
	public:
		CaColumnSize():m_nMaxEffectiveLength(0), m_bTrailingSpace(FALSE), m_nMaxLength(0){}
		~CaColumnSize(){}

		int  m_nMaxEffectiveLength; // Max effective length (i.e without trailing space)
		int  m_nMaxLength;          // Total length
		BOOL m_bTrailingSpace;      // TRUE if exists a column value with trailing space
	};
	CPtrArray m_arrayFieldSizeInfo;

	//
	// Theoric record size (read from the file):
	int m_nDBFThericRecordSize;

	//
	// For the fixed width page:
	// TRUE (the one of the user defined). FALSE (the one detected by the program)
	BOOL m_bFixedWidthUser;
};



//
// Step 3 (Page 3) data:
// ************************************************************************************************
class CaDataPage3
{
public:
	CaDataPage3()
	{
		m_strIIDateFormat = _T("");
		m_strIIDecimalFormat = _T("");
		m_strCreateTableStatement = _T("");
		m_bCreateTableOnly = FALSE;
	}
	~CaDataPage3(){Cleanup();}
	void Cleanup(){m_CommonItemData.Cleanup();}
	
	void SetIIDateFormat(LPCTSTR lpszSet){m_strIIDateFormat = lpszSet;}
	void SetIIDecimalFormat(LPCTSTR lpszSet){m_strIIDecimalFormat = lpszSet;}
	void SetStatement(CString& strStatement){m_strCreateTableStatement = strStatement;}

	CString& GetIIDateFormat(){return m_strIIDateFormat;}
	CString& GetIIDecimalFormat(){return m_strIIDecimalFormat;}
	CString& GetStatement(){return m_strCreateTableStatement;}
	CaIngresHeaderRowItemData& GetUpperCtrlCommonData(){return m_CommonItemData;}

	void SetCreateTableOnly(BOOL bSet = TRUE){m_bCreateTableOnly = bSet;}
	BOOL GetCreateTableOnly(){return m_bCreateTableOnly;}

protected:
	CaIngresHeaderRowItemData m_CommonItemData;
	CString m_strCreateTableStatement;
	CString m_strIIDateFormat;
	CString m_strIIDecimalFormat;
private:
	//
	// For DBF, when there are no rows to be imported, m_bCreateTableOnly can be TRUE
	// to indicate that we cretae the table anyway
	BOOL m_bCreateTableOnly;

};



//
// Step 4 (Page 4) data:
// ************************************************************************************************
class CaDataPage4
{
public:
	CaDataPage4(){Init();}
	~CaDataPage4(){}
	void Init()
	{
		m_bAppend = TRUE;
		m_nCommit = 0;
		m_bTableIsEmpty = TRUE;
	}
	void Cleanup(){Init();}
	void SetAppend(BOOL bSet){m_bAppend = bSet;}
	void SetCommitStep(int nStep){m_nCommit = nStep;}
	void SetTableState(BOOL bEmpty){m_bTableIsEmpty = bEmpty;}
	BOOL GetAppend(){return m_bAppend;}
	int  GetCommitStep(){return m_nCommit;}
	BOOL GetTableState(){return m_bTableIsEmpty;}


protected:
	BOOL m_bAppend;       // TRUE -> Append to the existing records. FALSE: delete old records first
	int  m_nCommit;       // > 0: commit every 'm_nCommit' rows
	BOOL m_bTableIsEmpty; // TRUE if the table is currently empty
};



//
// CLASS CaDataForLoadSave (data for load / save)
// ************************************************************************************************
class CaDataForLoadSave: public CObject
{
	DECLARE_SERIAL (CaDataForLoadSave)
public:
	CaDataForLoadSave();
	virtual ~CaDataForLoadSave();
	virtual void Serialize (CArchive& ar);
	void Cleanup();

	//
	// LoadData(): Load data from the file and update the object pIIAData.
	void LoadData (CaIIAInfo* pIIAData, CString& strFile);
	//
	// SaveData(): Get data from object and save it into the file.
	void SaveData (CaIIAInfo* pIIAData, CString& strFile);

	void SetUsedByPage(int nPage){m_nUsedByPage = nPage;}
	int  GetUsedByPage(){return m_nUsedByPage;}

	CString GetHostName() {return m_strHostName;}
	CString GetIISystem() {return m_strIISystem;}

	//
	// GET: (Page1):
	CStringArray& GetTreeItemPath(){return m_arrayTreeItemPath;}
	CString GetGateway(){return m_strServerClass;}
	CString GetNode(){return m_strNode;}
	CString GetDatabase(){return m_strDatabase;}
	CString GetTable(){return m_strTable;}
	CString GetTableOwner(){return m_strTableOwner;}
	CString GetFile2BeImported(){return m_strFile2BeImported;}
	int GetCodePage(){return m_nCodePage;}
	int GetFileMatchTable(){return m_nFileMatchTable;}
	BOOL IsExistingTable(){return m_bExistingTable;}
	ImportedFileType GetImportedFileType(){return m_typeImportedFile ;}
	CString GetCustomSeparator () {return m_strOthersSeparators;}
	CString GetNewTable(){return m_strNewTable;}
	int  GetKBToScan(){return m_nKB2Scan;}
	int  GetLineCountToSkip(){return m_nLineCountToSkip;}
	BOOL GetFirstLineAsHeader(){return m_bFirstLineHeader;}

	//
	// GET: (Page2):
	ColumnSeparatorType GetSeparatorType() {return m_nSeparatorType;}
	int GetColumnCount(){return m_nColumnCount;}
	//
	// For m_nSeparatorType = FMT_FIELDSEPARATOR:
	CString GetSeparator() {return m_strSeparator;}
	CString GetTextQualifier() {return m_strTQ;}
	BOOL GetConsecutiveAsOne() {return m_bCS;}
	//
	// For m_nSeparatorType = FMT_FIXEDWIDTH:
	BOOL GetFixedWidthUser(){return m_bUser;}
	CArray < int, int >& GetArrayDividerPos(){return m_arrayDividerPos;}

	//
	// GET: (Page3):
	CTypedPtrArray < CObArray, CaColumn* >& GetColumnOrder(){return m_arrayColumnOrder;}
	CString& GetIIDateFormat(){return m_strIIDateFormat;}
	CString& GetIIDecimalFormat(){return m_strIIDecimalFormat;}

	//
	// GET: (Page4):
	BOOL GetAppend(){return m_bAppend;}
	int  GetCommitStep(){return m_nCommit;}
	BOOL GetTableState(){return m_bTableIsEmpty;}

	//
	// SET (Page1):
	void SetServerClass(LPCTSTR lpszServerClass){m_strServerClass = lpszServerClass;}
	void SetNode(LPCTSTR lpszNode){m_strNode = lpszNode;}
	void SetDatabase(LPCTSTR lpszDatabase){m_strDatabase = lpszDatabase;}
	void SetTable(LPCTSTR lpszTable, LPCTSTR lpszOwner){m_strTable = lpszTable; m_strTableOwner = lpszOwner;}
	void SetFile2BeImported(LPCTSTR lpszFile){m_strFile2BeImported = lpszFile;}
	void SetCodePage(int nVal){m_nCodePage = nVal;}
	void SetFileMatchTable(int nVal){m_nFileMatchTable = nVal;}
	void SetExistingTable(BOOL bSet){m_bExistingTable = bSet;}
	void SetImportedFileType(ImportedFileType nType){m_typeImportedFile = nType;}
	void SetCustomSeparator (LPCTSTR lpString) {m_strOthersSeparators = lpString;}
	void SetNewTable(LPCTSTR lpString){m_strNewTable = lpString;}
	void SetKBToScan(int nBK){m_nKB2Scan = nBK;}
	void SetLineCountToSkip(int nCount){m_nLineCountToSkip = nCount;}
	BOOL SetFirstLineAsHeader(BOOL bSet){m_bFirstLineHeader = bSet;}

	//
	// SET: (Page2):
	void SetSeparatorType(ColumnSeparatorType nType) {m_nSeparatorType = nType;}
	void SetColumnCount(int nVal){m_nColumnCount = nVal;}
	//
	// For m_nSeparatorType = FMT_FIELDSEPARATOR:
	void SetSeparator(LPCTSTR lpszSeparator) {m_strSeparator = lpszSeparator;}
	void SetTextQualifier(TCHAR tchTQ) {m_strTQ = tchTQ;}
	void SetConsecutiveAsOne(BOOL bSet) {m_bCS=bSet;}
	//
	// For m_nSeparatorType = FMT_FIXEDWIDTH:
	void SetFixedWithUser(BOOL bSet){m_bUser = bSet;}

	//
	// SET: (Page3):
	void SetIIDateFormat(LPCTSTR lpszSet){m_strIIDateFormat = lpszSet;}
	void SetIIDecimalFormat(LPCTSTR lpszSet){m_strIIDecimalFormat = lpszSet;}

	//
	// SET: (Page4):
	void SetAppend(BOOL bSet){m_bAppend = bSet;}
	void SetCommitStep(int nStep){m_nCommit = nStep;}
	void SetTableState(BOOL bEmpty){m_bTableIsEmpty = bEmpty;}

protected:
	CString m_strHostName;
	CString m_strIISystem;
	//
	// When the object is load from the file, m_nUsedByPage has a value 1.
	// When m_nUsedByPage has a value n (n >= 1) that means the pages 1 to n have already
	// used the data:
	int m_nUsedByPage;

	//
	// Page 1:
	// *******
	CString m_strServerClass;            // Gateway (if not empty)
	CString m_strNode;                   // Virtual Node Name (Input or user's selection)
	CString m_strDatabase;               // Database (Input or user's selection)
	CString m_strTable;                  // Table (Input or user's selection)
	CString m_strTableOwner;             // Table Owner (Input or user's selection)
	CString m_strFile2BeImported;        // File to be imported (Input or user's selection)
	CString m_strNewTable;               // Table name to be created on the fly.
	BOOL m_bExistingTable;               // TRUE if user selected the existing table:
	int  m_nCodePage;                    // 0=ANSI, 1=OEM
	//
	// Number and order of columns of the file expected to match
	// exactly those of the Ingres Table ?
	int m_nFileMatchTable;               // 1=YES,  0=NO 
	ImportedFileType m_typeImportedFile; // File type (extension)
	CString m_strOthersSeparators;       // User provides the custom separators (each sep is separated by a space)
	int  m_nKB2Scan;                     // Number KB to scan. 0: scan the whole file.
	int  m_nLineCountToSkip;             // Number of lines to skip.
	BOOL m_bFirstLineHeader;             // Consider the first line as the header.
	CStringArray m_arrayTreeItemPath;

	//
	// Page 2:
	// *******
	ColumnSeparatorType m_nSeparatorType; // Fixed width? or Separator Separated ?
	//
	// For m_nSeparatorType = FMT_FIELDSEPARATOR:
	CString m_strSeparator;               // Separator
	CString m_strTQ;                      // Text Qualifier
	BOOL    m_bCS;                        // Consecutive Separators considered as one
	int     m_nColumnCount;               // Number of columns
	//
	// For m_nSeparatorType = FMT_FIXEDWIDTH:
	BOOL m_bUser;                         // TRUE if user has specified one. FALSE it is an auto-detected one.
	CArray < int, int > m_arrayDividerPos;// Position amoung the character string (it is not a pixel)

	//
	// Page 3:
	// *******
	CTypedPtrArray < CObArray, CaColumn* > m_arrayColumnOrder;
	CString m_strIIDateFormat;
	CString m_strIIDecimalFormat;

	//
	// Page 4:
	// *******
	BOOL m_bAppend;       // TRUE -> Append to the existing records. FALSE: delete old records first
	int  m_nCommit;       // > 0: commit every 'm_nCommit' rows
	BOOL m_bTableIsEmpty; // TRUE if the table is currently empty
};



//
// Global Import Assistant Data
// ************************************************************************************************
class CaIIAInfo
{
public:
	CaIIAInfo();
	~CaIIAInfo();
	CaDataForLoadSave* GetLoadSaveData(){return m_pLoadSaveData;}
	void CleanLoadSaveData();
	void LoadData(CString& strFile);
	void SaveData(CString& strFile);
	void SetInterrupted(BOOL bSet){m_bInterrupted = bSet;}
	BOOL GetInterrupted(){return m_bInterrupted;}


	CaDataPage1 m_dataPage1;
	CaDataPage2 m_dataPage2;
	CaDataPage3 m_dataPage3;
	CaDataPage4 m_dataPage4;
protected:
	BOOL m_bInterrupted;
	CaDataForLoadSave* m_pLoadSaveData;
};


//
// Global in theApp (CSvriiaApp)
// ************************************************************************************************
class CaUserData4GetRow
{
public:
	enum {INTERRUPT_CANCEL, INTERRUPT_COMMIT, INTERRUPT_NOCOMMIT, INTERRUPT_KILL};
	CaUserData4GetRow();
	~CaUserData4GetRow();
	void Cleanup();
	void InitResourceString();

	//
	// GET:
	int& GetCurrentProgressPos(){return m_nCurrentPosInfo;}
	int& GetCurrentRecord(){return m_nCurrentRecord;}
	int  GetTotalSize(){return m_nTotalSize;}
	BOOL GetUseFileDirectly(){return m_bUseFileDirectly;}
	FILE* GetDataFile(){return m_pFile;}
	CaIIAInfo* GetIIAData() {return m_pIIAData;}
	HWND GetAnimateDlg (){return m_hwndAnimateDlg;}
	int& GetCopyRowcount(){return m_nCopyRowcount;}
	int& GetCommittedCount(){return m_nProgressCommit;}
	int  GetCommitStep(){return m_nCommitStep;}
	BOOL GetCopyCompleteState(){return m_bCopiedComplete;}
	int& GetCommitCounter(){return m_nCommitCounter;}

	int GetInterruptConfirm(){return m_nConfirm;}
	int GetError(){return m_nError;}

	CString GetMsgImportInfo1(){return m_strMsgImportInfo1;}
	CString GetMsgImportInfo2(){return m_strMsgImportInfo2;}
	BOOL    IsBufferTooSmall(){return m_bBufferTooSmall;}
	//
	// SET:
	void SetCurrentProgressPos(int nVal){m_nCurrentPosInfo = nVal;}
	void SetCurrentRecord(int nVal){m_nCurrentRecord = nVal;}
	void SetTotalSize(int nVal){m_nTotalSize = nVal;}
	void SetUseFileDirectly(BOOL bSet){m_bUseFileDirectly = bSet;}
	void SetDataFile(FILE* pDataFile){m_pFile = pDataFile;}
	void SetIIAData(CaIIAInfo* pIIAData) {m_pIIAData = pIIAData;}
	void SetAnimateDlg (HWND hWndTaskDlg){m_hwndAnimateDlg = hWndTaskDlg;}
	
	void SetInterruptConfirm(int nConfirm){m_nConfirm = nConfirm;}
	void SetBufferTooSmall(BOOL bErrorBufferTooSmall){m_bBufferTooSmall = bErrorBufferTooSmall;}
	void SetCommitStep(int nStep){m_nCommitStep = nStep;}
	void SetCopyCompleteState(BOOL bSet){m_bCopiedComplete = bSet;}
	void SetError(int nError){m_nError = nError;}


private:
	BOOL m_bCopiedComplete; // TRUE when the end of file has been reached.
	int  m_nCommitCounter;  // Re-initialize to 0 each time a COMMIT is performed. +1 each time a row is copied.
	int  m_nCommitStep;     // Number of rows to commit (commit every m_nCommitStep copied rows)
	int  m_nProgressCommit; // Number of rows copied and committed partially.
	int  m_nCopyRowcount;   // The total number of rows that has been copied into table.
	int  m_nCurrentRecord;  // The record (0-based index) has been fetched OR the amount of
	                        // bytes has been fetched.
	int  m_nTotalSize;      // The total records OR total number of bytes to import.
	int  m_nCurrentPosInfo; // For the progress control (current display position)
	BOOL m_bUseFileDirectly;// TRUE if the file can be used directly (i.e no need to format the record)
	HWND m_hwndAnimateDlg;  // Handle of the animate dialog.
	
	FILE*      m_pFile;     // File to read (only if the total file has not been pre-scanned)
	CaIIAInfo* m_pIIAData;  // The import assistant data (page1, ..., page3)

	int m_nConfirm;
	int m_nError;           // We cannot throw exception from the COPY callback, so it must set this error.
	CString m_strMsgImportInfo1;
	CString m_strMsgImportInfo2;
	BOOL    m_bBufferTooSmall;
};



#define MAX_RECORDLEN     32000  // Max record length in the file File 
#define MAX_FIXEDWIDTHLEN  3500  // Max record length in the file File that can be handled by fixed width.
//
// Check to see if the file has CR in the first nLimitLen.
BOOL  File_HasCRLF(LPCTSTR lpszFile, int nLimitLen = MAX_RECORDLEN);
BOOL File_Empty(LPCTSTR);
DWORD File_GetSize(LPCTSTR lpszFile);

void AutoDetectFixedWidth(CaIIAInfo* pData, TCHAR*& pArrayPos, int& nArraySize);
void AutoDetectFormatTxt (CaIIAInfo* pData);
void AutoDetectFormatCsv (CaIIAInfo* pData);
void AutoDetectFormatXml (CaIIAInfo* pData);


#define DBFxERROR_UNSUPORT            1
#define DBFxERROR_HASMEMO_FILE        2
#define DBFxERROR_CANNOT_READ         3
#define DBFxERROR_WRONGCOLUMN_COUNT   4 // The number of columns in dbf file is not equal to the number
                                        // of columns in the table.
#define DBFxERROR_UNSUPORT_DATATYPE   5
#define DBFxERROR_RECORDLEN           6 // SUM of column's length is not equal to record length in the header of .dbf file

#define CSVxERROR_CANNOT_OPEN       100 // The file cannot be opened
#define CSVxERROR_LINE_TOOLONG      101 // Line too long in the file..
#define CSVxERROR_TABLE_WRONGDESC   102 // The current description of table is not match the one reading from file.

//
// Return the message depending on nError
// that can take on of the above DBFxERROR_XX, CSVxERR_XX
CString MSG_DetectFileError(int nError);


#endif // FORMDATA_HEADER
