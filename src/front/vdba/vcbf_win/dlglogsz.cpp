/*
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Source   : DlgLogSz.cpp, Implementation File
**
**
**    Project  : OpenIngres Configuration Manager
**    Author   : Blattes Emmanuel
**
**  History:
**	18-Jul-98 (mcgem01)
**	    The maximum allowable transaction log file size is 2048M
**	20-Nov-98 (cucjo01)
**	    Add multiple striped log file and dual log file support
**	18-Dec-98 (cucjo01)
**	    Added field notifying user that the "read-only" transaction log
**	    size is the same size as the other transaction log
**	15-Mar-99 (cucjo01)
**	    Typecasted CString.Format() calls to work with MAINWIN
**	24-Nov-99 (cucjo01)
**	    Updated log directory creation class to work with MAINWIN and UNIX.
**	24-jan-2000 (hanch04)
**	    Updated for UNIX build using mainwin, added FILENAME_SEPARATOR
**	06-Jun-2000: (uk$so01) 
**	    (BUG #99242)
**	    Cleanup code for DBCS compliant
**	01-nov-2001 (somsa01)
**	    Cleaned up 64-bit compiler warnings / errors.
**	23-May-2003 (schph01)
**	    bug #110304 Retrieve the same error message than CBF for the size of
**	    transaction log.
**	    Test the "Size Per File" value instead the "Total Size (in MB)" for the transaction log file.
**	03-jun-2003 (somsa01)
**	    Modified previous change to include erst.h, not fecat.h.
**	20-jun-2003 (schph01)
**	    (bug 110430) Create the directory for primary or dual log only if this
**	    one doesn't exist
**	17-Dec-2003 (schph01)
**	    SIR #111462, Put string into resource files
**  14-Oct-2004 (schph01)
**     BUG #113281 Remove the limitation (2048M) for transaction log file
**     size.
*/

#include "stdafx.h"
#include "vcbf.h"
#include "DlgLogSz.h"
#include "cudlglogfileadd.h"

extern "C"{
#include <compat.h>
#include <erst.h>
#include <er.h>
}

#ifndef MAINWIN
#define FILENAME_SEPARATOR _T('\\')
#define FILENAME_SEPARATOR_STR _T("\\")
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LP_LOG_TYPE_TEXT m_bPrimary ? _T("Primary") : _T("Dual")
#define LP_ROOT_PATHS(x)     m_bPrimary ? (LPTSTR)m_lpS->szRootPaths1[x] : (LPTSTR)m_lpS->szRootPaths2[x]
#define LP_LOG_FILE_NAME(x)  m_bPrimary ? (LPTSTR)m_lpS->szLogFileNames1[x] : (LPTSTR)m_lpS->szLogFileNames2[x]
#define LP_CONFIG_DAT_LOG_FILE_NAME m_bPrimary ? (LPTSTR)m_lpS->szConfigDatLogFileName1: (LPTSTR)m_lpS->szConfigDatLogFileName2
#define LP_CONFIG_DAT_LOG_FILE_SIZE (m_bPrimary ? m_lpS->iConfigDatLogFileSize1 : m_lpS->iConfigDatLogFileSize2)
#define LOG_ROOT_DIR m_bPrimary ? _T("%s_%i") : _T("%s_Dual_%i")
#define LP_OTHER_LOG_TYPE_TEXT m_bPrimary ? _T("Dual") : _T("Primary")
#define LP_OTHER_NUMBER_OF_PARTITIONS (m_bPrimary ? m_lpS->iNumberOfLogFiles2 : m_lpS->iNumberOfLogFiles1)
#define LP_OTHER_LOG_ENABLED m_bPrimary ? (LPTSTR)m_lpS->szenabled2 : (LPTSTR)m_lpS->szenabled1
#define LP_OTHER_CONFIG_DAT_LOG_FILE_SIZE (m_bPrimary ? m_lpS->iConfigDatLogFileSize2 : m_lpS->iConfigDatLogFileSize1)
                                                                      
/////////////////////////////////////////////////////////////////////////////
// CuDlgLogSize dialog


CuDlgLogSize::CuDlgLogSize(CWnd* pParent /*=NULL*/)
	: CDialog(CuDlgLogSize::IDD, pParent)
{
	//{{AFX_DATA_INIT(CuDlgLogSize)
	m_size = 0;
	m_edSizePerFile = 0.0f;
	//}}AFX_DATA_INIT
}


void CuDlgLogSize::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CuDlgLogSize)
	DDX_Control(pDX, IDC_ROOT_LOCS, m_lbRootLocs);
	DDX_Text(pDX, IDC_SIZE_TOTAL, m_size);
	DDX_Text(pDX, IDC_SIZE_PER_FILE, m_edSizePerFile);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CuDlgLogSize, CDialog)
	//{{AFX_MSG_MAP(CuDlgLogSize)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	ON_EN_CHANGE(IDC_SIZE_TOTAL, OnChangeSizeTotal)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CuDlgLogSize message handlers

void CuDlgLogSize::OnAdd() 
{ CString strMessage;
  CString szII_SYSTEM;
     
   int iCount=m_lbRootLocs.GetCount();
   
   if (iCount > m_iMaxLogFiles - 1)
   { MessageBeep(MB_ICONEXCLAMATION);
     strMessage.Format(IDS_MAX_LOG_FILES,m_iMaxLogFiles);
     AfxMessageBox(strMessage);
     return;
   }

   CCuDlgLogFileAdd dlg;
   
   TCHAR* penv = _tgetenv(_T("II_SYSTEM"));
   if (!penv)
      szII_SYSTEM = _T("C:\\INGRESII");
   else
      szII_SYSTEM = penv;

   dlg.m_LogFileName.Format(LOG_ROOT_DIR, (LPCTSTR)szII_SYSTEM, iCount+1);
   INT_PTR ret = dlg.DoModal();
   if (ret != IDOK)
     return;
   m_lbRootLocs.InsertString(-1, dlg.m_LogFileName);
   OnChangeSizeTotal();

}

void CuDlgLogSize::OnRemove() 
{
   int iSel = m_lbRootLocs.GetCurSel();
   m_lbRootLocs.DeleteString(iSel);
   if (iSel == 0)
     iSel = m_lbRootLocs.SetCurSel(0);
   else
     iSel = m_lbRootLocs.SetCurSel(iSel-1);

   #if 0
   int iCount = m_lbRootLocs.GetCount();
   if (iCount <= 0)
   { CEdit* edit1 = (CEdit*)  GetDlgItem (IDC_SIZE_TOTAL);
     if (edit1)
        edit1->SetWindowText(_T("0"));
   }
   #endif

   OnChangeSizeTotal();
}

BOOL CuDlgLogSize::OnInitDialog() 
{ int loop;
  CString strText;

	CDialog::OnInitDialog();
	
    CStatic* static1 = (CStatic*) GetDlgItem(IDC_SIZE_LOG_TEXT);
    if (static1)
       static1->ShowWindow(SW_HIDE);
        
    for (loop = 0; loop < LG_MAX_FILE; loop++)
       if (_tcscmp((LPCTSTR)LP_ROOT_PATHS(loop), _T("")))
          m_lbRootLocs.AddString(LP_ROOT_PATHS(loop));

    CEdit* edit1 = (CEdit*)  GetDlgItem (IDC_SIZE_LOG);
    if (edit1)
    { CString s;
      if (!_tcscmp((LPCTSTR)LP_OTHER_LOG_ENABLED, _T("yes")))
      { s.Format(_T("%d"), LP_OTHER_CONFIG_DAT_LOG_FILE_SIZE);
        edit1->SetWindowText(s);
        edit1->SetReadOnly(TRUE);
         
        CStatic* static1 = (CStatic*) GetDlgItem(IDC_SIZE_LOG_TEXT);
        if (static1)
        { s.Format(_T("(Same as %s)"), LP_OTHER_LOG_TYPE_TEXT);
          static1->SetWindowText(s);
          static1->ShowWindow(SW_SHOW);
        }
      }
      else
      { int iCount = m_lbRootLocs.GetCount();
        if (iCount > 0)
        { s.Format(_T("%d"), LP_CONFIG_DAT_LOG_FILE_SIZE);
          edit1->SetWindowText(s);
        }
      }

      edit1->SetLimitText(6);
    }

    OnChangeSizeTotal();   // set max/min of field

    strText.Format(_T("Create %s Transaction Log"), LP_LOG_TYPE_TEXT);
    SetWindowText(strText);
        
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CuDlgLogSize::OnOK() 
{ int i, rc, iCount, iSize, iTotSize;
  CString strText, strBuffer, strMessage;

  iCount = m_lbRootLocs.GetCount();

  if ((!_tcscmp(LP_OTHER_LOG_ENABLED, _T("yes"))) && (iCount != LP_OTHER_NUMBER_OF_PARTITIONS))
  { MessageBeep(MB_ICONEXCLAMATION);
    if (LP_OTHER_NUMBER_OF_PARTITIONS == 1)
       strMessage.Format(IDS_TRANSACTION_LOG_ONE, LP_LOG_TYPE_TEXT, LP_OTHER_LOG_TYPE_TEXT);
    else
       strMessage.Format(IDS_ERR_TRANSACTION_PART, LP_OTHER_NUMBER_OF_PARTITIONS, LP_LOG_TYPE_TEXT, LP_OTHER_LOG_TYPE_TEXT, LP_OTHER_NUMBER_OF_PARTITIONS);

    AfxMessageBox(strMessage);
    return;
  }

  if (iCount == 0)
  { MessageBeep(MB_ICONEXCLAMATION);
    AfxMessageBox(IDS_AT_LEAST_ONE_LOG);
    return;
  }
  
  CEdit* edit1 = (CEdit*)  GetDlgItem (IDC_SIZE_LOG);
  if (edit1)
  { CString s;
    edit1->GetWindowText(s);

    iTotSize = _ttoi(s); 
  }

  CEdit* edit = (CEdit*)  GetDlgItem (IDC_SIZE_PER_FILE);
  if (edit)
  { CString s;
    edit->GetWindowText(s);
    iSize = _ttoi(s); 

    if (iSize < 16)
    {
      MessageBeep(MB_ICONEXCLAMATION);
      strMessage.Format(ERget( S_ST039E_log_too_small ), iCount, iCount*16);
      AfxMessageBox(strMessage);
      return;
    }

  }
  //
  // DBCS compliant:
  if (iCount > 1)
  { /// Check Total Space if all values are on the same drive
    CString strComp;
    bool bSame=TRUE;

    m_lbRootLocs.GetText(0, strText);
    strComp.Format(_T("%c:\\"), strText.GetAt(0));
    for (i=0; i < iCount; i++)
    { strText=_T("");
      m_lbRootLocs.GetText(i, strText);
    
      strBuffer.Format(_T("%c:\\"), strText.GetAt(0));
      if (strBuffer.Compare(strComp) != 0)
        bSame=FALSE;
    }

    if (bSame)
    { DWORD SectorsPerCluster; 
	  DWORD BytesPerSector; 
	  DWORD NumberOfFreeClusters; 
	  DWORD TotalNumberOfClusters; 
	
      int rc=GetDiskFreeSpace(strBuffer, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters );
	
      if (rc)
      { double dwClusterSize=SectorsPerCluster*BytesPerSector;
        int iFreeSpace = int ((dwClusterSize*NumberOfFreeClusters)/1048576);
       
        if (iFreeSpace < iTotSize)
        { CString strMessage; 
          MessageBeep(MB_ICONEXCLAMATION);
          strMessage.Format(IDS_ERR_NOT_SPACE, (LPCTSTR)strComp, iFreeSpace, iTotSize);
          rc = AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);
          return;
        }
      } // if (rc)...

    } // if (bSame)
  }



  // Check Avail Space - Temporary until back end is fixed
  for (i=0; i < iCount; i++)
  { strText=_T("");
    m_lbRootLocs.GetText(i, strText);
    
    strBuffer.Format(_T("%c:\\"), strText.GetAt(0));
    
    DWORD SectorsPerCluster; 
	DWORD BytesPerSector; 
	DWORD NumberOfFreeClusters; 
	DWORD TotalNumberOfClusters; 
	
	int rc=GetDiskFreeSpace(strBuffer, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters );
	
    if (rc)
    {  double dwClusterSize=SectorsPerCluster*BytesPerSector;
       int iFreeSpace = int ((dwClusterSize*NumberOfFreeClusters)/1048576);
       
       if (iFreeSpace < iSize)
       { CString strMessage; 
         MessageBeep(MB_ICONEXCLAMATION);
         strMessage.Format(IDS_ERR_NOT_SPACE, (LPCTSTR)strBuffer, iFreeSpace, iSize);
         rc = AfxMessageBox(strMessage, MB_OK | MB_ICONEXCLAMATION);
         return;
       } // if (rc)...
    } // if (bSame)

  }

  for (i=0; i < LG_MAX_FILE; i++)
  {  _tcscpy (LP_ROOT_PATHS(i), _T(""));
  }

  // Create directory structure and fill in dir/file arrays
  for (i=0; i < iCount; i++)
  { strText=_T("");
    m_lbRootLocs.GetText(i, strText);
    
    strBuffer.Format(_T("%s%cingres%clog"), (LPCTSTR)strText,FILENAME_SEPARATOR,FILENAME_SEPARATOR);

    if (!IsExistingDirectory(strBuffer))
      rc = CreateDirectoryRecursively(strBuffer);

    if (!rc)
    { rc = GetLastError();
      /// if (rc == ERROR_ALREADY_EXIST)  ERROR_INVALID_NAME
      //    strMessage.Format("Cannot create directory: %s", (LPCTSTR)strBuffer);
      //  else
      { MessageBeep(MB_ICONEXCLAMATION);
        strMessage.Format(IDS_ERR_CREATE_DIR, (LPCTSTR)strBuffer, rc);
        AfxMessageBox(strMessage);
        m_lbRootLocs.SetCurSel(i);
        return;
      }
    }

    _tcscpy (LP_ROOT_PATHS(i), strText);
    _stprintf((LPTSTR)LP_LOG_FILE_NAME(i), _T("%s.l%02d"), LP_CONFIG_DAT_LOG_FILE_NAME, i+1);

  }

  CDialog::OnOK();
}

int CuDlgLogSize::IsExistingDirectory(CString strDir)
{

  DWORD dwAttr = GetFileAttributes(strDir);

  if (dwAttr == 0xffffffff)
     return(0);

  if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
    return(1);
  else
    return(0);

}

int CuDlgLogSize::CreateDirectoryRecursively(CString strDirectory)
{ TCHAR szBuffer[MAX_PATH]=_T("");
  TCHAR szNewDirectory[MAX_PATH]=_T("");
  TCHAR *p;
  int rc;
  TCHAR szSlash[5];
  _stprintf(szSlash, _T("%c"), FILENAME_SEPARATOR);

  if (strDirectory.IsEmpty()) 
     return(0);

  _tcscpy(szNewDirectory, strDirectory);
  
  p = _tcstok(szNewDirectory, szSlash);
       
  if (p)
  { 
#ifndef MAINWIN
    _tcscpy(szBuffer, p);
#else
    _stprintf(szBuffer, _T("/%s"), p);
#endif
  }  
  
  while (p)
  { p = _tcstok(NULL, szSlash);
         
    if (p)
    { _tcscat(szBuffer, szSlash);
      _tcscat(szBuffer, p);
      rc = CreateDirectory(szBuffer, NULL);
    }   
  }  // while...

  DWORD dwAttr = GetFileAttributes(strDirectory);

  if (dwAttr == 0xffffffff)
     return(0);
  
  if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
    return(1);
  else
    return(0);
}

void CuDlgLogSize::OnChangeSizeTotal() 
{ CString s;
  int iCount, iSize;
  
  CEdit* edtSizeTotal = (CEdit*)  GetDlgItem (IDC_SIZE_TOTAL);
  CEdit* edtSizePerFile  = (CEdit*)  GetDlgItem (IDC_SIZE_PER_FILE);
  
  if ((!edtSizeTotal) || (!edtSizePerFile) )
    return;

  iCount = m_lbRootLocs.GetCount();
  if (iCount <= 0)
  { edtSizePerFile->SetWindowText(_T("0"));
    return;
  }

  edtSizeTotal->GetWindowText(s);
  iSize = _ttoi(s); 
  
  if (iSize % iCount == 0)
     s.Format(_T("%d"), iSize/iCount);
  else
     s.Format(_T("%.2f"), (float)iSize/iCount);
   
  edtSizePerFile->SetWindowText(s);
	
}
