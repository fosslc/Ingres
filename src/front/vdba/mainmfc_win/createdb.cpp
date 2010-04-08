/****************************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project  : Visual DBA
**
**  createdb.cpp : implementation file
**
**  History after 04-May-1999:
**	19-Apr-2000 (schph01)
**	SIR #101243 for create database, checked by default the checkbox "Generate Front End catalogs"
**	and all products in the list 
**  21-Jun-2001 (schph01)
**  (SIR 103694) STEP 2 support of UNICODE datatypes
**  14-May-2002 (noifr01 for uk$so01 and noifr01)
**  (bug 107773) don't refuse any more international characters (such as
**  accented characters) for the database name, and now leave the DBMS 
**  return its own error if invalid characters are found
**  13-Jun-2002 (hanje04)
**  (bug 108023)
**  In OccupyLocationsControl, when csWork and Default are passed into various
**  Cstrings using Format, they are corrupted resulting in m_lstCheckWorkLoc
**  containing corrutped data. To stop this cast csWork and Default with 
**  (LPCTSTR) when passing into Cstring.Format.
**  24-Jun-2002 (hanje04)
**  Cast all CStrings being passed to other functions or methods using %s
**  with (LPCTSTR) and made calls more readable on UNIX.
**  15-May-2003 (schph01)
**  bug 110247 Update the unicode control in Alter mode
**  02-feb-2004 (somsa01)
**  Updated to "typecast" UCHAR into CString before concatenation.
**  26-May-2004 (schph01)
**  SIR #112460 add "Catalogs Page Size" control in createdb dialog box.
**  23-July-2004 (schph01)
**  SIR #112756 allow in locations "extend" tab to uncheck a location.
**  30-May-2005 (komve01)
**  Bug #114123 Used radio button enabling command for public/private
**  database option in side OnReadonly() function. Removed cbutton
**  enabling. (Submitted on behalf of shaha03).
**  06-Sep-2005) (drivi01)
**  Bug #115173 Updated createdb dialogs and alterdb dialogs to contain
**  two available unicode normalization options, added group box with
**  two checkboxes corresponding to each normalization, including all
**  expected functionlity.
**  09-Mar-2010 (drivi01) 
**  SIR #123397 Update the dialog to be more user friendly.  Minimize the amount
**  of controls exposed to the user when dialog is initially displayed.
**  Add "Show/Hide Advanced" button to see more/less options in the 
**  dialog. Take out DBD from the list of fe-clients.  Put all the options
**  into a tab control.

*/
//
// UNMASK THE FOLLOWING DEFINE IN ORDER TO HAVE WINDOWS4GL PRESENCE/ABSENCE MANAGEMENT
//#define TESTW4GL
#define NO_UNEXTENDDB

#include "stdafx.h"
#include "mainmfc.h"
#include "createdb.h"
#include "dgerrh.h"     // MessageWithHistoryButton
#include "replutil.h"
extern "C" {
extern BOOL MfcGetRmcmdObjects_launcher(char *VnodeName, char *objName);
#include "main.h"
#include "domdata.h"
#include "getdata.h"
#include "dlgres.h"
#include "lexical.h"
#include "msghandl.h"
#include "dbaginfo.h"
#include "monitor.h"
#include "cv.h"
#ifdef TESTW4GL
#include "esltools.h"
#endif
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const int DU_DBS_LOC = 8;
const int DU_WORK_LOC = 16;
const int DU_AWORK_LOC = 32;


/////////////////////////////////////////////////////////////////////////////
// interface to dom.c
extern "C" BOOL MfcDlgCreateDatabase( LPCREATEDB lpcreatedb )
{
	CxDlgCreateAlterDatabase dlg;
	dlg.m_StructDatabaseInfo = lpcreatedb;
	int iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}



/////////////////////////////////////////////////////////////////////////////
// CxDlgCreateAlterDatabase dialog


CxDlgCreateAlterDatabase::CxDlgCreateAlterDatabase(CWnd* pParent /*=NULL*/)
	: CDialogEx(CxDlgCreateAlterDatabase::IDD, pParent)
{
	m_pDlgcreateDBLocation = NULL;
	m_pDlgcreateDBAlternateLoc = NULL;
	//{{AFX_DATA_INIT(CxDlgCreateAlterDatabase)
	//}}AFX_DATA_INIT
}


void CxDlgCreateAlterDatabase::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgCreateAlterDatabase)
	DDX_Control(pDX, IDC_CHECK_UNICODE, m_ctrlUnicodeEnable);
	DDX_Control(pDX, IDC_CHECK_UNICODE2, m_ctrlUnicodeEnable2);
	DDX_Control(pDX, IDC_CHECK_NONUNICODE, m_ctrlEnableUnicode);
	DDX_Control(pDX, IDC_TAB_LOCATION, m_cTabLoc);
	DDX_Control(pDX, IDC_MORE_OPTIONS, m_ctrlMoreOptions);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DBNAME, m_ctrlDatabaseName);
}


BEGIN_MESSAGE_MAP(CxDlgCreateAlterDatabase, CDialogEx)
	//{{AFX_MSG_MAP(CxDlgCreateAlterDatabase)
	ON_WM_DESTROY()
	ON_EN_CHANGE(IDC_DBNAME, OnChangeDbname)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_LOCATION, OnSelchangeTabLocation)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CHECK_NONUNICODE, OnBnClickedEnableUnicode)
	ON_BN_CLICKED(IDC_MORE_OPTIONS, OnBnClickedShowMoreOptions)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgCreateAlterDatabase message handlers

BOOL CxDlgCreateAlterDatabase::OnInitDialog() 
{
	CDialogEx::OnInitDialog();

	CString csTitle;

	CString TabTitleOne,TabTitleTwo, TabTitleThree;
	TC_ITEM tcitem;
	memset (&tcitem, 0, sizeof (tcitem));
	tcitem.mask = TCIF_TEXT;
	tcitem.cchTextMax = 32;

	if (m_StructDatabaseInfo->bAlter)
	{
		if (!TabTitleOne.LoadString(IDS_TAB_TITLE_ONE_V2) || //"Primary"
			!TabTitleTwo.LoadString(IDS_TAB_TITLE_TWO) ||
			!TabTitleThree.LoadString(IDS_TAB_TITLE_THREE))	       //"Extend"
			return TRUE;
	}
	else
	{
			if (!TabTitleOne.LoadString(IDS_TAB_TITLE_ONE) || //"Primary"
				!TabTitleTwo.LoadString(IDS_TAB_TITLE_TWO))  
			return TRUE;
	}
	
	tcitem.pszText = (LPSTR)(LPCTSTR)TabTitleOne;
	m_cTabLoc.InsertItem(0, &tcitem);

	tcitem.pszText = (LPSTR)(LPCTSTR)TabTitleTwo;
	m_cTabLoc.InsertItem(1, &tcitem);
	
	if (m_StructDatabaseInfo->bAlter)
	{
		tcitem.pszText = (LPSTR)(LPCTSTR)TabTitleThree;
		m_cTabLoc.InsertItem(2, &tcitem);
	}

	m_pDlgcreateDBLocation = new CuDlgcreateDBLocation(&m_cTabLoc);
	m_pDlgcreateDBLocation->Create (IDD_CREATEDB_LOCATION, &m_cTabLoc);
	m_pDlgcreateDBAdvanced = new CuDlgcreateDBAdvanced(&m_cTabLoc);
	m_pDlgcreateDBAdvanced->Create(IDD_CREATEDB_ADVANCED, &m_cTabLoc);
	m_pDlgcreateDBAlternateLoc = new CuDlgcreateDBAlternateLoc (&m_cTabLoc);
	if (m_StructDatabaseInfo->bAlter)
		m_pDlgcreateDBAlternateLoc->Create  (IDD_CREATEDB_LOC_ALTERNATE, &m_cTabLoc);
	m_pDlgcreateDBAlternateLoc->m_bAlterDatabase = m_StructDatabaseInfo->bAlter;
	m_cTabLoc.SetCurSel (0);

	CRect r;
	m_cTabLoc.GetClientRect (r);
	m_cTabLoc.AdjustRect (FALSE, r);
	m_pDlgcreateDBAlternateLoc->ShowWindow  (SW_HIDE);
	m_pDlgcreateDBAdvanced->ShowWindow(SW_HIDE);
	m_pDlgcreateDBLocation->MoveWindow (r);
	m_pDlgcreateDBLocation->ShowWindow (SW_SHOW);

	//Fill the windows title bar
	if (m_StructDatabaseInfo->bAlter)
	{
		ASSERT (m_StructDatabaseInfo->szDBName[0] != 0);
		// Change the title to 'Alter Database'
		if (csTitle.LoadString(IDS_T_ALTER_DATABASE)) 
		{
			csTitle = csTitle + " "+m_StructDatabaseInfo->szDBName+" on "+GetVirtNodeName ( GetCurMdiNodeHandle ());
			SetWindowText(csTitle);
		}
	}
	else
	{
		ASSERT (m_StructDatabaseInfo->szDBName[0] == 0);
		// Change the title to 'Create Database'
		GetWindowText(csTitle);
		csTitle = csTitle+ " " + GetVirtNodeName ( GetCurMdiNodeHandle ());
		SetWindowText(csTitle);
	}

	// fill locations
	if (!OccupyLocationsControl())
	{
		ASSERT(NULL);
		EndDialog (-1);
		return TRUE;
	}

	// check PUBLIC or PRIVATE
	CheckRadioButton( IDC_PUBLIC, IDC_PRIVATE, m_StructDatabaseInfo->bPrivate ? IDC_PRIVATE : IDC_PUBLIC);

	//Check default Unicode support
	if (CMischarset_utf8())
	{
		m_ctrlUnicodeEnable.EnableWindow(TRUE);
		m_ctrlUnicodeEnable2.EnableWindow(TRUE);
		m_ctrlEnableUnicode.SetCheck(1);
		m_ctrlUnicodeEnable2.SetCheck(1);
	}

	// initialise DBname ,Language ,Coordinator
	InitialiseEditControls();

	SelectDefaultLocation();

	// hide Star and FrontEnd related controls
	if (m_StructDatabaseInfo->bAlter)
	{
		m_ctrlUnicodeEnable.EnableWindow(FALSE);
		m_ctrlUnicodeEnable2.EnableWindow(FALSE);
		m_ctrlEnableUnicode.EnableWindow(FALSE);
		// Update Unicode checkbox
		if (m_StructDatabaseInfo->bUnicodeDatabaseNFD )
			m_ctrlUnicodeEnable.SetCheck(1);
		else
			m_ctrlUnicodeEnable.SetCheck(0);
		if (m_StructDatabaseInfo->bUnicodeDatabaseNFC )
			m_ctrlUnicodeEnable2.SetCheck(1);
		else
			m_ctrlUnicodeEnable2.SetCheck(0);
		if (m_StructDatabaseInfo->bUnicodeDatabaseNFD || m_StructDatabaseInfo->bUnicodeDatabaseNFC)
			m_ctrlEnableUnicode.SetCheck(1);
		else
			m_ctrlEnableUnicode.SetCheck(0);

		m_ctrlDatabaseName.EnableWindow(FALSE);

		//change database location not available with relocatedb
		m_pDlgcreateDBLocation->m_ctrlLocDatabase.EnableWindow(FALSE);
		m_pDlgcreateDBLocation->m_ctrlLocWork.EnableWindow(FALSE);
	}

	if (m_StructDatabaseInfo->bAlter)
	{
		m_ctrlMoreOptions.ShowWindow(SW_HIDE);
		bExpanded = true;
	}
	else
	{
	/** Adding some code to simplify the dialog.  Adding Advanced button.
	*/
	CRect rect;
	this->GetWindowRect(&rect);
	this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (rect.bottom-rect.top)/3);
	bExpanded = false;
	}

	PushHelpId (m_StructDatabaseInfo->bAlter? HELPID_IDD_DATABASE_ALTER: IDD_CREATEDB);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CxDlgCreateAlterDatabase::OnDestroy() 
{
	CDialogEx::OnDestroy();
	PopHelpId();
}

void CxDlgCreateAlterDatabase::OnOK() 
{
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle ()); 

	int ires;
	if (!m_StructDatabaseInfo->bAlter)
	{
		// Create database
		FillStructureWithCtrlInfo();
		ires = DBAAddObjectLL (vnodeName, OT_DATABASE, (void *) m_StructDatabaseInfo );
		if (ires != RES_SUCCESS)
		{
			// Create database failed
			CString strMsg;
			strMsg.LoadString(IDS_E_CREATE_DATABASE_FAILED);
			AfxMessageBox (strMsg);
		}
		else
			EndDialog( TRUE);
	}
	else
	{
		if (!VerifyOneWorkLocExist())
		{
			CString strMsg;
			//strMsg.LoadString(IDS_E_ALTERDB_AT_LEAST_ONE_WORK);
			//strMsg = _T("Must have at least one \"Work\" location in alternate location.");
			AfxMessageBox (VDBA_MfcResourceString(IDS_E_ALTERDB_AT_LEAST_ONE_WORK));
			return;
		}
		ChangeExtendedLocationStatus();
		ExecuteRemoteCommand();
		// change status access ( Public Private )
		BOOL oldPrivateStatus = m_StructDatabaseInfo->bPrivate;
		BOOL newPublicStatus = m_pDlgcreateDBAdvanced->m_ctrlPublic.GetCheck()?TRUE:FALSE;
		if	( newPublicStatus == oldPrivateStatus )
		{
			ChangeGlobalAccessStatus(newPublicStatus);
		}
		int nIndex = m_pDlgcreateDBAdvanced->m_CtrlCatalogsPageSize.GetCurSel();
		if ( nIndex != LB_ERR )
		{
			long lCurPgSize,lOldPgSize;
			lCurPgSize = m_pDlgcreateDBAdvanced->m_CtrlCatalogsPageSize.GetItemData(nIndex);
			CVal((LPSTR)m_StructDatabaseInfo->szCatalogsPageSize,&lOldPgSize);
			if (lCurPgSize != lOldPgSize)
				ChangeCatalogPageSize(lCurPgSize);
		}

	}
	CDialogEx::OnOK();
}


BOOL CxDlgCreateAlterDatabase::IsW4GLInLocalInstall()
{
#ifdef TESTW4GL
	BOOL b4GL = TRUE; 	  // default as if there
	char *pBufFile = NULL;
	int  fileLen = 0;
	char szProdsFile[_MAX_PATH];
	char *penv;

	// build full name of file
	penv = getenv(VDBA_GetInstallPathVar());
	if (!penv || !penv[0])
	{
		AfxMessageBox (VDBA_ErrMsgIISystemNotSet());
		return b4GL;
	}
	fstrncpy((UCHAR *)szProdsFile, (UCHAR *)penv, _MAX_PATH);
#ifdef MAINWIN
	x_strcat(szProdsFile, "/ingres/files/dictfiles/prods.lst");
#else
	x_strcat(szProdsFile, "\\ingres\\files\\dictfiles\\prods.lst");
#endif

  // Quote in case path contains spaces
    x_strcpy(szProdsFile, quoted(szProdsFile));

	fileLen = Vdba_GetLenOfFileName((char*)szProdsFile);
	if (fileLen > 0)	{
	int iretlen;
		pBufFile = (char *)ESL_AllocMem(fileLen + 1);
		if (ESL_FillBufferFromFile((UCHAR *)szProdsFile, (UCHAR *)pBufFile, fileLen,
							   &iretlen, FALSE) == RES_SUCCESS) {
	if (x_strstr(pBufFile, "WINDOWS_4GL") != 0)
		b4GL = TRUE;
	else
		b4GL = FALSE;
	}
	ESL_FreeMem(pBufFile);
	}
	return b4GL;

#else	// ifdef TESTW4GL
	// No test: Act as if there so we have old behaviour
	return TRUE;
#endif	// ifdef TESTW4GL

}

void CxDlgCreateAlterDatabase::InitialiseEditControls()
{
	m_ctrlDatabaseName.SetLimitText( MAXOBJECTNAME-1);

	m_ctrlDatabaseName.SetWindowText((LPCTSTR)m_StructDatabaseInfo->szDBName);
}


void CxDlgCreateAlterDatabase::OnChangeDbname() 
{
	BOOL bEnable = TRUE;

	// Disable OK button if no database name
	if (m_ctrlDatabaseName.GetWindowTextLength() == 0)
		bEnable = FALSE;
	CButton * cbTemp =(CButton *) GetDlgItem(IDOK);
	cbTemp->EnableWindow(bEnable);

	// update coordinator db name
	m_pDlgcreateDBAdvanced->UpdateCoordName();

}

BOOL CxDlgCreateAlterDatabase::OccupyLocationsControl()
{
	BOOL bSystem;
	int hNode,err,TypeLocation,i,iIndex;
	char szObject[MAXOBJECTNAME];
	char szFilter[MAXOBJECTNAME];
	CString csTemp, CompleteDefault,Default,csWork,csAuxiliary;
	LPDB_EXTENSIONS lpObj;

	Default.LoadString (IDS_DEFAULT);
	csWork.LoadString (IDS_LOC_TYPE_WORK);
	csAuxiliary.LoadString (IDS__LOC_TYPE_AUXILIARY);

	m_pDlgcreateDBLocation->m_ctrlLocDatabase.ResetContent();
	m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.ResetContent();
	m_pDlgcreateDBLocation->m_ctrlLocDump.ResetContent();
	m_pDlgcreateDBLocation->m_ctrlLocJournal.ResetContent();
	m_pDlgcreateDBLocation->m_ctrlLocWork.ResetContent();
	m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.ResetContent();
	m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.ResetContent();

	bSystem = TRUE;
	hNode = GetCurMdiNodeHandle();

	err = DOMGetFirstObject(hNode,
							OT_LOCATION,
							0,
							NULL,
							bSystem,
							NULL,
							(LPUCHAR)szObject,
							NULL,
							NULL);
	while (err == RES_SUCCESS){
			BOOL bOK;
			for (i = 0, TypeLocation = 0;i <= LOCATIONDUMP;i++,TypeLocation=i)
			{
				if (DOMLocationUsageAccepted(hNode,(LPUCHAR)szObject,TypeLocation,&bOK) == RES_SUCCESS && bOK)
				{
					switch (TypeLocation)
					{
						case LOCATIONDATABASE:
							CompleteDefault.Empty();
							if (m_StructDatabaseInfo->bAlter)
							{
								if (lstrcmp(szObject,(char *)m_StructDatabaseInfo->LocDatabase) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocDatabase.InsertString(0,CompleteDefault);
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocDatabase.AddString(szObject);
							}
							else
							{
								if (lstrcmp(szObject,_T("ii_database")) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocDatabase.InsertString(0,CompleteDefault);
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocDatabase.AddString(szObject);
							}
							// fill Check Listbox for alternate location.
							lpObj = FindExtendLoc(szObject,LOCATIONDATABASE);
							if (lpObj != NULL)
							{
								if (lpObj->status == LOCEXT_DATABASE)
								{
									if (!CompleteDefault.IsEmpty())
									{
										iIndex = m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.AddString(CompleteDefault);
										m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.SetCheck(CompleteDefault);
									}
									else
									{
										iIndex = m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.AddString(lpObj->lname);
										m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.SetCheck(lpObj->lname);
										m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.EnableItem(iIndex,FALSE);
									}
#ifndef NO_UNEXTENDDB
									// Unextenddb available on for extended locations
									if ( GetOIVers() >= OIVERS_30 && CompleteDefault.IsEmpty())
										m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.EnableItem(iIndex,TRUE);
									else
#endif
										m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.EnableItem(iIndex,FALSE);
								}
							}
							else
								m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.AddString(szObject);

							break;
						case LOCATIONWORK:
							{
							BOOL bDefaultLoc=FALSE;
							if (m_StructDatabaseInfo->bAlter)
							{
								if (lstrcmp(szObject,(char *)m_StructDatabaseInfo->LocWork) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocWork.InsertString(0,CompleteDefault);
									bDefaultLoc=TRUE;
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocWork.AddString(szObject);
							}
							else
							{
								if (lstrcmp(szObject,_T("ii_work")) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocWork.InsertString(0,CompleteDefault);
									bDefaultLoc=TRUE;
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocWork.AddString(szObject);
							}
							// fill Check Listbox for alternate location.
							lpObj = FindExtendLoc(szObject,LOCATIONWORK);
							if (lpObj != NULL)
							{
								if (lpObj->status == LOCEXT_WORK)
								{
									if (bDefaultLoc)
										csTemp.Format("%s %s (%s)",(LPCTSTR)csWork,(LPCTSTR)Default,lpObj->lname);
									else
										csTemp.Format("%s %s",(LPCTSTR)csWork,lpObj->lname);
									iIndex = m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.AddString(csTemp);
									m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.SetCheck(iIndex,TRUE);
#ifndef NO_UNEXTENDDB
									if ( GetOIVers() >= OIVERS_30 && !bDefaultLoc)
										m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.EnableItem(iIndex,TRUE);
									else
#endif
										m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.EnableItem(iIndex,FALSE);
								}
								else if (lpObj->status == LOCEXT_AUXILIARY)
								{
									if (bDefaultLoc)
										csTemp.Format("%s %s (%s)",csAuxiliary,Default,lpObj->lname);
									else
										csTemp.Format("%s %s",csAuxiliary,lpObj->lname);
									iIndex = m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.AddString(csTemp);
									m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.SetCheck(iIndex,TRUE);
#ifndef NO_UNEXTENDDB
									if ( GetOIVers() >= OIVERS_30 && !bDefaultLoc)
										m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.EnableItem(iIndex,TRUE);
									else
#endif
										m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.EnableItem(iIndex,FALSE);
								}
							}
							else
								m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.AddString(szObject);
							}
							break;
						case LOCATIONJOURNAL:
							if (m_StructDatabaseInfo->bAlter)
							{
								if (lstrcmp(szObject,(char *)m_StructDatabaseInfo->LocJournal) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocJournal.InsertString(0,CompleteDefault);
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocJournal.AddString(szObject);
							}
							else
							{
								if (lstrcmp(szObject,_T("ii_journal")) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocJournal.InsertString(0,CompleteDefault);
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocJournal.AddString(szObject);
							}
							break;
						case LOCATIONCHECKPOINT:
							if (m_StructDatabaseInfo->bAlter)
							{
								if (lstrcmp(szObject,(char *)m_StructDatabaseInfo->LocCheckpoint) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.InsertString(0,CompleteDefault);
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.AddString(szObject);
							}
							else
							{
								if (lstrcmp(szObject,_T("ii_checkpoint")) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.InsertString(0,CompleteDefault);
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.AddString(szObject);
							}
							break;
						case LOCATIONDUMP:
							if (m_StructDatabaseInfo->bAlter)
							{
								if (lstrcmp(szObject,(char *)m_StructDatabaseInfo->LocDump) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocDump.InsertString(0,CompleteDefault);
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocDump.AddString(szObject);
							}
							else
							{
								if (lstrcmp(szObject,_T("ii_dump")) == 0)
								{
									CompleteDefault.Format("(%s)",szObject);
									m_pDlgcreateDBLocation->m_ctrlLocDump.InsertString(0,CompleteDefault);
								}
								else
									m_pDlgcreateDBLocation->m_ctrlLocDump.AddString(szObject);
							}
							break;
						default:
						break;
					}
					//if ( iCbErr == CB_ERR || iCbErr == CB_ERRSPACE)
				}
			}
		err = DOMGetNextObject ((LPUCHAR)szObject,(LPUCHAR)szFilter, NULL);
	}
	return TRUE;
}

void CxDlgCreateAlterDatabase::FillStructureWithCtrlInfo()
{
	// Database Name
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle ());
	CString csDbName;
	m_ctrlDatabaseName.GetWindowText( csDbName );
	lstrcpy ((char *)m_StructDatabaseInfo->szDBName, csDbName);

	// store in the structure the current user name   
	ZEROINIT (m_StructDatabaseInfo->szUserName);
	DBAGetUserName(vnodeName,m_StructDatabaseInfo->szUserName);

	ZEROINIT (m_StructDatabaseInfo->Language);
	if ( m_pDlgcreateDBAdvanced->m_ctrlLanguage.GetWindowTextLength() != 0)
	{
		CString csTempo;
		m_pDlgcreateDBAdvanced->m_ctrlLanguage.GetWindowText( csTempo );
		lstrcpy ((char *)m_StructDatabaseInfo->Language, csTempo);
	}

	// Star
	if (m_pDlgcreateDBAdvanced->m_ctrlDistributed.GetCheck() == 1)
	{
		CString csTempo;
		m_StructDatabaseInfo->bDistributed = TRUE;
		m_pDlgcreateDBAdvanced->m_ctrlCoordName.GetWindowText(csTempo);
		lstrcpy ((char *)m_StructDatabaseInfo->szCoordName, csTempo);
	}
	else
	{
		m_StructDatabaseInfo->bDistributed = FALSE;
		m_StructDatabaseInfo->szCoordName[0] = '\0';
	}

	// Front end products
	m_StructDatabaseInfo->bAllFrontEnd = FALSE;
	if (m_pDlgcreateDBAdvanced->m_ctrlGenerateFrontEnd.GetCheck() == 1)
		m_StructDatabaseInfo->bGenerateFrontEnd = TRUE;
	else
		m_StructDatabaseInfo->bGenerateFrontEnd = FALSE;
	int nCount = m_pDlgcreateDBAdvanced->m_CheckFrontEndList.GetCount();
	ZEROINIT (m_StructDatabaseInfo->Products);
	if ( nCount != 0 )
	{
		m_StructDatabaseInfo->bAllFrontEnd = TRUE;  // by default
		for (int i = 0; i < nCount; i++)
		{
			if (m_pDlgcreateDBAdvanced->m_CheckFrontEndList.GetCheck (i))
			{
				char szName[MAXOBJECTNAME];
				DWORD dwtype = m_pDlgcreateDBAdvanced->m_CheckFrontEndList.GetItemData(i);
				ProdNameFromProdType(dwtype, szName);
				x_strcat((char *)m_StructDatabaseInfo->Products,szName);
				x_strcat((char *)m_StructDatabaseInfo->Products," ");
			}
			else
				m_StructDatabaseInfo->bAllFrontEnd = FALSE;  // at least one missing
		}
		// reset AllFE if Window4GL Not Available
		if (!IsW4GLInLocalInstall())
			m_StructDatabaseInfo->bAllFrontEnd = FALSE;  // will force enumeration of fe components
	}
	// if public is not checked then private is check.
	if(!m_pDlgcreateDBAdvanced->m_ctrlPublic.GetCheck())
		m_StructDatabaseInfo->bPrivate = TRUE;
	
	//retrieve location
	int nIndex;
	nIndex = m_pDlgcreateDBLocation->m_ctrlLocDatabase.GetCurSel();
	if (nIndex > 0) 
		m_pDlgcreateDBLocation->m_ctrlLocDatabase.GetLBText(nIndex,(char *)m_StructDatabaseInfo->LocDatabase);
	nIndex = m_pDlgcreateDBLocation->m_ctrlLocWork.GetCurSel();
	if (nIndex > 0) 
		m_pDlgcreateDBLocation->m_ctrlLocWork.GetLBText(nIndex,(char *)m_StructDatabaseInfo->LocWork);
	nIndex = m_pDlgcreateDBLocation->m_ctrlLocJournal.GetCurSel();
	if (nIndex > 0) 
		m_pDlgcreateDBLocation->m_ctrlLocJournal.GetLBText(nIndex,(char *)m_StructDatabaseInfo->LocJournal);
	nIndex = m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.GetCurSel();
	if (nIndex > 0) 
		m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.GetLBText(nIndex,(char *)m_StructDatabaseInfo->LocCheckpoint);
	nIndex = m_pDlgcreateDBLocation->m_ctrlLocDump.GetCurSel();
	if (nIndex > 0) 
		m_pDlgcreateDBLocation->m_ctrlLocDump.GetLBText(nIndex,(char *)m_StructDatabaseInfo->LocDump);

	// ReadOnly
	if ( m_pDlgcreateDBAdvanced->m_cbReadOnly.GetCheck() == BST_CHECKED )
		m_StructDatabaseInfo->bReadOnly = TRUE;
	else
		m_StructDatabaseInfo->bReadOnly = FALSE;

	// Unicode enable
	if (m_ctrlUnicodeEnable.GetCheck() == BST_CHECKED  )
		m_StructDatabaseInfo->bUnicodeDatabaseNFD = 1;
	else
		m_StructDatabaseInfo->bUnicodeDatabaseNFD = 0;

	if (m_ctrlUnicodeEnable2.GetCheck() == BST_CHECKED )
		m_StructDatabaseInfo->bUnicodeDatabaseNFC = 1;
	else
		m_StructDatabaseInfo->bUnicodeDatabaseNFC = 0;
	// Catalogs Page Size
	nIndex = m_pDlgcreateDBAdvanced->m_CtrlCatalogsPageSize.GetCurSel();
	if ( nIndex != LB_ERR && nIndex > 0)
	{
		long lCurPgSize;
		lCurPgSize = m_pDlgcreateDBAdvanced->m_CtrlCatalogsPageSize.GetItemData(nIndex);
		CVla(lCurPgSize,(char*)m_StructDatabaseInfo->szCatalogsPageSize);
	}
	else
		m_StructDatabaseInfo->szCatalogsPageSize[0]=_T('\0');
}

void CxDlgCreateAlterDatabase::SelectDefaultLocation()
{
	int nIndex;
	if (m_StructDatabaseInfo->bAlter)
	{
		if (m_StructDatabaseInfo->LocDatabase[0] != '\0')
		{
			nIndex = m_pDlgcreateDBLocation->m_ctrlLocDatabase.FindString(-1,(LPCTSTR)m_StructDatabaseInfo->LocDatabase);
			if (nIndex != CB_ERR)
				m_pDlgcreateDBLocation->m_ctrlLocDatabase.SetCurSel(nIndex);
			else
				m_pDlgcreateDBLocation->m_ctrlLocDatabase.SetCurSel(0);
		}
		else
			m_pDlgcreateDBLocation->m_ctrlLocDatabase.SetCurSel(0);
		if (m_StructDatabaseInfo->LocWork[0] != '\0')
		{
			nIndex = m_pDlgcreateDBLocation->m_ctrlLocWork.FindString(-1,(LPCTSTR)m_StructDatabaseInfo->LocWork);
			if (nIndex != CB_ERR)
				m_pDlgcreateDBLocation->m_ctrlLocWork.SetCurSel(nIndex);
			else
				m_pDlgcreateDBLocation->m_ctrlLocWork.SetCurSel(0);
		}
		else
			m_pDlgcreateDBLocation->m_ctrlLocWork.SetCurSel(0);
		if (m_StructDatabaseInfo->LocJournal[0] != '\0')
		{
			nIndex = m_pDlgcreateDBLocation->m_ctrlLocJournal.FindString(-1,(LPCTSTR)m_StructDatabaseInfo->LocJournal);
			if (nIndex != CB_ERR)
				m_pDlgcreateDBLocation->m_ctrlLocJournal.SetCurSel(nIndex);
			else
				m_pDlgcreateDBLocation->m_ctrlLocJournal.SetCurSel(0);
		}
		else
			m_pDlgcreateDBLocation->m_ctrlLocJournal.SetCurSel(0);
		if (m_StructDatabaseInfo->LocCheckpoint[0] != '\0')
		{
			nIndex = m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.FindString(-1,(LPCTSTR)m_StructDatabaseInfo->LocCheckpoint);
			if (nIndex != CB_ERR)
				m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.SetCurSel(nIndex);
			else
				m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.SetCurSel(0);
		}
		else
			m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.SetCurSel(0);
		if (m_StructDatabaseInfo->LocDump[0] != '\0')
		{
			nIndex = m_pDlgcreateDBLocation->m_ctrlLocDump.FindString(-1,(LPCTSTR)m_StructDatabaseInfo->LocDump);
			if (nIndex != CB_ERR)
				m_pDlgcreateDBLocation->m_ctrlLocDump.SetCurSel(nIndex);
			else
				m_pDlgcreateDBLocation->m_ctrlLocDump.SetCurSel(0);
		}
		else
			m_pDlgcreateDBLocation->m_ctrlLocDump.SetCurSel(0);
	}
	else
	{
		m_pDlgcreateDBLocation->m_ctrlLocDatabase.SetCurSel(0);
		m_pDlgcreateDBLocation->m_ctrlLocWork.SetCurSel(0);
		m_pDlgcreateDBLocation->m_ctrlLocJournal.SetCurSel(0);
		m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.SetCurSel(0);
		m_pDlgcreateDBLocation->m_ctrlLocDump.SetCurSel(0);
	}
}

void CxDlgCreateAlterDatabase::ExecuteRemoteCommand()
{
	int hnode;
	CString csCommandLine,csTempo,csNodeName,csTitle;
	int nIndex;
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle ());
	csNodeName = vnodeName;
	hnode = OpenNodeStruct  ((LPUCHAR)(LPCTSTR)csNodeName);
	if (hnode<0)
	{
		CString strMsg = VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);//_T("Maximum number of connections has been reached"
		strMsg += VDBA_MfcResourceString (IDS_E_ALTER_DB);			//	" - Cannot alter database.");
		AfxMessageBox (strMsg);
		return;
	}
	// Temporary for activate a session
	UCHAR buf[MAXOBJECTNAME];
	DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);
	
	nIndex = m_pDlgcreateDBLocation->m_ctrlLocWork.GetCurSel();
	if (nIndex != CB_ERR)
	{
		m_pDlgcreateDBLocation->m_ctrlLocWork.GetLBText(nIndex,csTempo);
		if (csTempo.Find((char *)m_StructDatabaseInfo->LocWork) == -1)// the location have changed
		{
			if (csTempo.Find(_T("ii_work"))!=-1)
				csTempo = _T("ii_work");
			csTitle.Format("Relocate WORK Location");
			csCommandLine.Format("relocatedb %s -new_work_location=%s",m_StructDatabaseInfo->szDBName,csTempo);
			execrmcmd1(	(char *)(LPCTSTR)csNodeName,
						(char *)(LPCTSTR)csCommandLine,
						(char *)(LPCTSTR)csTitle,
						TRUE);
		}
	}

	nIndex = m_pDlgcreateDBLocation->m_ctrlLocJournal.GetCurSel();
	if (nIndex != CB_ERR)
	{
		m_pDlgcreateDBLocation->m_ctrlLocJournal.GetLBText(nIndex,csTempo);
		if (csTempo.Find((char *)m_StructDatabaseInfo->LocJournal) == -1)// the location have changed
		{
			if (csTempo.Find(_T("ii_journal"))!=-1)
				csTempo = _T("ii_journal");
			csTitle.Format("Relocate Journal Location");
			csCommandLine.Format("relocatedb %s -new_jnl_location=%s",m_StructDatabaseInfo->szDBName,csTempo);
			execrmcmd1(	(char *)(LPCTSTR)csNodeName,
						(char *)(LPCTSTR)csCommandLine,
						(char *)(LPCTSTR)csTitle,
						TRUE);
		}
	}

	nIndex = m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.GetCurSel();
	if (nIndex != CB_ERR)
	{
		m_pDlgcreateDBLocation->m_ctrlLocCheckPoint.GetLBText(nIndex,csTempo);
		if (csTempo.Find((char *)m_StructDatabaseInfo->LocCheckpoint) == -1)// the location have changed
		{
			if (csTempo.Find(_T("ii_checkpoint"))!=-1)
				csTempo = _T("ii_checkpoint");
			csTitle.Format("Relocate Checkpoint Location");
			csCommandLine.Format("relocatedb %s -new_ckp_location=%s",m_StructDatabaseInfo->szDBName,(LPCTSTR)csTempo);
			execrmcmd1(	(char *)(LPCTSTR)csNodeName,
						(char *)(LPCTSTR)csCommandLine,
						(char *)(LPCTSTR)csTitle,
						TRUE);
		}
	}

	nIndex = m_pDlgcreateDBLocation->m_ctrlLocDump.GetCurSel();
	if (nIndex != CB_ERR)
	{
		m_pDlgcreateDBLocation->m_ctrlLocDump.GetLBText(nIndex,csTempo);
		if (csTempo.Find((char *)m_StructDatabaseInfo->LocDump) == -1)// the location have changed
		{
			if (csTempo.Find(_T("ii_dump"))!=-1)
				csTempo = _T("ii_dump");
			csTitle.Format("Relocate Dump Location");
			csCommandLine.Format("relocatedb %s -new_dump_location=%s",m_StructDatabaseInfo->szDBName,(LPCTSTR)csTempo);
			execrmcmd1(	(char *)(LPCTSTR)csNodeName,
						(char *)(LPCTSTR)csCommandLine,
						(char *)(LPCTSTR)csTitle,
						TRUE);
		}
	}
	csTitle.Empty();
	csCommandLine.Empty();
	if (m_pDlgcreateDBAdvanced->m_ctrlGenerateFrontEnd.GetCheck() == 1)
	{
		int nCount,i;
		BOOL bFirst = TRUE;
		nCount = m_pDlgcreateDBAdvanced->m_CheckFrontEndList.GetCount();
		for (i=0; i<nCount; i++)
		{
			if (m_pDlgcreateDBAdvanced->m_CheckFrontEndList.IsItemEnabled(i) && 
				m_pDlgcreateDBAdvanced->m_CheckFrontEndList.GetCheck(i) == 1)
			{
				char szName[MAXOBJECTNAME];
				DWORD dwtype = m_pDlgcreateDBAdvanced->m_CheckFrontEndList.GetItemData(i);
				if (bFirst)
				{
				csCommandLine = _T("upgradefe ");
				csCommandLine += CString(m_StructDatabaseInfo->szDBName);
				csCommandLine += _T(" ");
				bFirst = FALSE;
				}
				else
					csCommandLine += _T(" ");

				switch (dwtype)
				{
					case PROD_INGRES :
						ProdNameFromProdType(dwtype, szName);
						csCommandLine += szName;
						break;
					case PROD_VISION :
						ProdNameFromProdType(dwtype, szName);
						csCommandLine += szName;
						break;
					case PROD_W4GL :
						ProdNameFromProdType(dwtype, szName);
						csCommandLine += szName;
						break;
					/*case PROD_INGRES_DBD :
						ProdNameFromProdType(dwtype, szName);
						csCommandLine += szName;
						break;
						*/
				}
			}
		}
		if (!csCommandLine.IsEmpty())
		{
			csCommandLine += _T(" -u");
			csCommandLine += CString(m_StructDatabaseInfo->szUserName);
			csTitle.Format("Generate front end catalog on %s",m_StructDatabaseInfo->szDBName);
			execrmcmd1(	(char *)(LPCTSTR)csNodeName,
						(char *)(LPCTSTR)csCommandLine,
						(char *)(LPCTSTR)csTitle,
						FALSE);
		}
	}
	CloseNodeStruct(hnode,FALSE);
}

void CxDlgCreateAlterDatabase::ChangeGlobalAccessStatus(BOOL bPublic)
{
	int hnode;
	CString csNodeName;
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle ());
	csNodeName = vnodeName;
	hnode = OpenNodeStruct  ((LPUCHAR)(LPCTSTR)csNodeName);
	if (hnode<0) {
		CString strMsg =  VDBA_MfcResourceString (IDS_MAX_NB_CONNECT); //_T("Maximum number of connections has been reached"
		strMsg += VDBA_MfcResourceString (IDS_E_ALTER_DB);			   //" - Cannot alter database.");
		AfxMessageBox (strMsg);
		return;
	}
	// Temporary for activate a session
	UCHAR buf[MAXOBJECTNAME];
	int iret;
	DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);

	iret = GenerateSql4changeAccessDB(bPublic,m_StructDatabaseInfo->szDBName,vnodeName);
	if (iret != RES_SUCCESS)
		MessageWithHistoryButton(m_hWnd,VDBA_MfcResourceString (IDS_E_CHANGE_ACCESS));//"Change global access failed."
	CloseNodeStruct(hnode,FALSE);

}

void CxDlgCreateAlterDatabase::OnSelchangeTabLocation(NMHDR* pNMHDR, LRESULT* pResult) 
{
	int nSel = m_cTabLoc.GetCurSel();
	CRect r;

	m_cTabLoc.GetClientRect (r);
	m_cTabLoc.AdjustRect (FALSE, r);

	switch (nSel)
	{
	case 0:
		m_pDlgcreateDBAlternateLoc->ShowWindow  (SW_HIDE);
		m_pDlgcreateDBAdvanced->ShowWindow(SW_HIDE);
		m_pDlgcreateDBLocation->MoveWindow (r);
		m_pDlgcreateDBLocation->ShowWindow (SW_SHOW);
		break;
	case 1:
		m_pDlgcreateDBAlternateLoc->ShowWindow  (SW_HIDE);
		m_pDlgcreateDBLocation->ShowWindow  (SW_HIDE);
		m_pDlgcreateDBAdvanced->MoveWindow(r);
		m_pDlgcreateDBAdvanced->ShowWindow (SW_SHOW);
		break;
	case 2:
		m_pDlgcreateDBLocation->ShowWindow  (SW_HIDE);
		m_pDlgcreateDBAdvanced->ShowWindow (SW_HIDE);
		m_pDlgcreateDBAlternateLoc->MoveWindow (r);
		m_pDlgcreateDBAlternateLoc->ShowWindow (SW_SHOW);
	default:
		break;
	}

	*pResult = 0;
}

void CxDlgCreateAlterDatabase::PostNcDestroy() 
{
	CDialogEx::PostNcDestroy();
}

LPDB_EXTENSIONS CxDlgCreateAlterDatabase::FindExtendLoc(char *szLocName,int iTypeLoc)
{
	LPOBJECTLIST lplist = m_StructDatabaseInfo->lpLocExt;
	if (*szLocName == '\0')
		return NULL;

	while (lplist)
	{
		LPDB_EXTENSIONS lpObjTemp = (LPDB_EXTENSIONS)lplist->lpObject;
		if (lstrcmpi(szLocName,lpObjTemp->lname) == 0)
		{
			if (iTypeLoc==LOCATIONDATABASE)
			{
				if (lpObjTemp->status == LOCEXT_DATABASE)
					return lpObjTemp;
			}
			if (iTypeLoc==LOCATIONWORK)
			{
				if (lpObjTemp->status == LOCEXT_WORK ||
					lpObjTemp->status == LOCEXT_AUXILIARY )
					return lpObjTemp;
			}
		}
		lplist = (LPOBJECTLIST) lplist->lpNext;
	}
	return NULL;
}

int CxDlgCreateAlterDatabase::VerifyOneWorkLocExist()
{
	int nbItem,i,nbWorkItem;
	CString csCurentString = _T("");
	CString csWork = _T("");
	csWork.LoadString (IDS_LOC_TYPE_WORK);
	if (csWork.IsEmpty())
		return 0;
	nbItem = m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.GetCount();
	for (i=0,nbWorkItem=0;i<nbItem;i++)
	{
		m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.GetText(i,csCurentString);
		if ( csCurentString.Find(csWork) == 0)
			nbWorkItem++;
	}
	return nbWorkItem;
}

void CxDlgCreateAlterDatabase::ChangeExtendedLocationStatus()
{
	int nbItem,i;
	CString csCurentString,csWork,csAuxiliary;
	CString csStatement,csTempLocName;
	CStringList cslAllProcedure;
	CWaitCursor doWaitCursor;
	LPDB_EXTENSIONS lpObj;

	csWork.LoadString (IDS_LOC_TYPE_WORK);
	csAuxiliary.LoadString (IDS__LOC_TYPE_AUXILIARY);

	if (csWork.IsEmpty() || csAuxiliary.IsEmpty())
		return;
	nbItem = m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.GetCount();

	// find all work or auxillary unchecked
	for (i=0;i<nbItem;i++)
	{
		m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.GetText(i,csCurentString);
		if ( csCurentString.Find(csWork) != 0 &&
			 csCurentString.Find(csAuxiliary) != 0 )
		{
				csTempLocName.Empty();
				lpObj = FindExtendLoc((LPTSTR)(LPCTSTR)csCurentString,LOCATIONWORK);
				if (lpObj)
				{
					csStatement.Empty();
					int istatus = DU_WORK_LOC;
					if ( lpObj->status == LOCEXT_AUXILIARY )
						istatus = DU_AWORK_LOC;
					csStatement.Format("execute procedure iiqef_del_location ( database_name = '%s'," "location_name = '%s',access  = %d," "need_dbdir_flg = 1 )",
					m_StructDatabaseInfo->szDBName, 
					(LPCTSTR)csCurentString,
					istatus);
					cslAllProcedure.AddTail(csStatement);
				}
		}
	}

	// find all work location in checkListWork for change of auxiliary to work
	for (i=0;i<nbItem;i++)
	{
		m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.GetText(i,csCurentString);
		if ( csCurentString.Find(csWork) == 0)
		{
			csTempLocName.Empty();
			m_pDlgcreateDBAlternateLoc->GetLocName(csCurentString, csTempLocName);
			lpObj = FindExtendLoc((LPTSTR)(LPCTSTR)csTempLocName,LOCATIONWORK);
			if ( m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.GetCheck(i) == BST_CHECKED )
			{
				if (lpObj == NULL)
				{
					csStatement.Empty();
					m_pDlgcreateDBAlternateLoc->GetLocName(csCurentString, csTempLocName);
					csStatement.Format("execute procedure iiqef_add_location ( database_name = '%s', " "location_name = '%s',access  = %d," "need_dbdir_flg = 1 )",
					m_StructDatabaseInfo->szDBName,
					(LPCTSTR)csTempLocName,DU_WORK_LOC);
					cslAllProcedure.AddTail(csStatement);
				}
				else
				{
					csStatement.Empty();
					if (lpObj->status == LOCEXT_AUXILIARY)
					{
						csStatement.Format("execute procedure iiqef_alter_extension( database_name = '%s'," "location_name = '%s', drop_loc_type = %d," "add_loc_type  = %d )",	
						m_StructDatabaseInfo->szDBName,
						(LPCTSTR)csTempLocName, 
						LOCEXT_AUXILIARY-1, 
						LOCEXT_WORK-1);
						cslAllProcedure.AddTail(csStatement);
					}
				}
			}
		}
	} // END FOR

	// Find All work location who change of work to auxiliary
	for (i=0;i<nbItem;i++)
	{
		m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.GetText(i,csCurentString);
		if ( csCurentString.Find(csAuxiliary) == 0)
		{
			csTempLocName.Empty();
			m_pDlgcreateDBAlternateLoc->GetLocName(csCurentString, csTempLocName);
			lpObj = FindExtendLoc((LPTSTR)(LPCTSTR)csTempLocName,LOCATIONWORK);
			if (m_pDlgcreateDBAlternateLoc->m_lstCheckWorkLoc.GetCheck(i) == BST_CHECKED )
			{
				if (lpObj == NULL)
				{
					csStatement.Empty();
					csStatement.Format("execute procedure iiqef_add_location ( database_name = '%s'," "location_name = '%s',access  = %d," "need_dbdir_flg = 1 )",
					m_StructDatabaseInfo->szDBName, 
					(LPCTSTR)csTempLocName,
					DU_AWORK_LOC);
					cslAllProcedure.AddTail(csStatement);
				}
				else
				{
					csStatement.Empty();
					if (lpObj->status == LOCEXT_WORK)
					{
						csStatement.Format(" execute procedure iiqef_alter_extension ( database_name = '%s'," "location_name = '%s', drop_loc_type = %d," "add_loc_type  = %d )",	
						m_StructDatabaseInfo->szDBName, 
						(LPCTSTR)csTempLocName, 
						LOCEXT_WORK-1, 
						LOCEXT_AUXILIARY-1);
						cslAllProcedure.AddTail(csStatement);
					}
				}
			}
		}
	} // END FOR

	nbItem = m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.GetCount();
	for (i=0;i<nbItem;i++)
	{
		LPDB_EXTENSIONS lpObj;
		m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.GetText(i,csCurentString);
		lpObj = FindExtendLoc((LPTSTR)(LPCTSTR)csCurentString,LOCATIONDATABASE);
		if (lpObj != NULL )
		{
			if ( m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.GetCheck(i) == BST_UNCHECKED )
			{
				csStatement.Empty();
				csStatement.Format("execute procedure iiqef_del_location ( database_name = '%s'," "location_name = '%s',access  = %d," "need_dbdir_flg = 1 )",
				m_StructDatabaseInfo->szDBName, 
				(LPCTSTR)csCurentString,
				DU_DBS_LOC);
				cslAllProcedure.AddTail(csStatement);
			}
		}
		else
		{
			if ( m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.IsItemEnabled(i) &&
				 m_pDlgcreateDBAlternateLoc->m_lstCheckDatabaseLoc.GetCheck(i) == BST_CHECKED)
			{
				csStatement.Empty();
				csStatement.Format("execute procedure iiqef_add_location ( database_name = '%s'," "location_name = '%s',access  = %d," "need_dbdir_flg = 1 )",
				m_StructDatabaseInfo->szDBName, 
				(LPCTSTR)csCurentString,
				DU_DBS_LOC);
				cslAllProcedure.AddTail(csStatement);
			}
		}
	}

	POSITION pos;
	int hnode,nSession,iret;
	CString csConnection,csProcTempo;
	pos = cslAllProcedure.GetHeadPosition();
	if (pos != NULL)
	{
		if (IsDatabaseLocked())
			return;

		LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle ());
		hnode = OpenNodeStruct (vnodeName);
		if (hnode<0)
		{
			CString strMsg =  VDBA_MfcResourceString (IDS_MAX_NB_CONNECT); //_T("Maximum number of connections has been reached"
			strMsg += VDBA_MfcResourceString (IDS_E_ALTER_DB);			   //" - Cannot alter database.");
			AfxMessageBox (strMsg);
			return;
		}
		// Temporary for activate a session
		UCHAR buf[MAXOBJECTNAME];
		DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);

		csConnection.Format("%s::iidbdb",vnodeName);
		iret = Getsession ((unsigned char*)(LPCTSTR)csConnection, SESSION_TYPE_INDEPENDENT, &nSession);
		if (iret!=RES_SUCCESS)
		{
			MessageWithHistoryButton(m_hWnd,VDBA_MfcResourceString (IDS_E_GET_SESSION));//Cannot get a Session
			CloseNodeStruct(hnode,FALSE);
			return;
		}
		while ( pos != NULL )
		{
			csProcTempo = cslAllProcedure.GetNext(pos);

			iret = ExecuteProcedureLocExtend((LPTSTR)(LPCTSTR)csProcTempo);
			if (iret!=RES_SUCCESS)
			{
				MessageWithHistoryButton(m_hWnd,VDBA_MfcResourceString (IDS_E_EXEC_PROC));//"Execute Procedure failed."
				ReleaseSession(nSession, RELEASE_ROLLBACK);
				break;
			}
		}
		if (iret == RES_SUCCESS)
			ReleaseSession(nSession, RELEASE_COMMIT);
		CloseNodeStruct(hnode,FALSE);
	}
}

//		Verify if the database is locked by a other session and close all
//		internal vdba sessions.
//	return :
//		2 Closing sessions failed.
//		1 Database locked.
//		0 Database not Locked.
int CxDlgCreateAlterDatabase::IsDatabaseLocked()
{
	int iret,hNode;
	BOOL bDBLocked = FALSE;
	UCHAR VirtNode[MAXOBJECTNAME];
	VirtNode[0]=0;
	if (DBACloseNodeSessions(VirtNode,TRUE,TRUE)!=RES_SUCCESS)
	{
		//
		// Failure in closing sessions for this node. Probably active SQL Test window on this node
		//
		ErrorMessage ((UINT)IDS_E_CLOSING_SESSION2, RES_ERR);
		return 2;
	}

	RESOURCEDATAMIN ResDta;
	hNode = GetCurMdiNodeHandle ();
	iret = GetFirstMonInfo(hNode,0,NULL,OT_MON_RES_DB,&ResDta,NULL);
	while (iret == RES_SUCCESS)
	{
		TCHAR tchszDatabase [100];
		GetMonInfoName (hNode, OT_DATABASE, (void*)&ResDta,
							tchszDatabase, sizeof (tchszDatabase));
		if (lstrcmp (tchszDatabase,(char *)m_StructDatabaseInfo->szDBName) == 0)
		{
			CString strMsg;
			//_T("The database %s is locked."
			//"Change extended location not avalaible.")
			strMsg.Format(IDS_E_DB_LOCKED,m_StructDatabaseInfo->szDBName);
			AfxMessageBox (strMsg);
			bDBLocked = TRUE;
			break;
		}
		iret = GetNextMonInfo((void *)&ResDta);
	}
	if (DBACloseNodeSessions(VirtNode,TRUE,TRUE)!=RES_SUCCESS)
	{
		//
		// Failure in closing sessions for this node. Probably active SQL Test window on this node
		//
		ErrorMessage ((UINT)IDS_E_CLOSING_SESSION2, RES_ERR);
		return 2;
	}
	if (bDBLocked)
		return 1;
	return 0;
}


void CxDlgCreateAlterDatabase::ChangeCatalogPageSize(long lNewPageSize )
{
	int hnode,ires;
	UCHAR   UserName[MAXOBJECTNAME];
	UCHAR   RmcmdOwnerName[MAXOBJECTNAME];
	CString csCommandLine,csTempo,csNodeName,csTitle;
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle ());
	csNodeName = vnodeName;
	hnode = OpenNodeStruct  ((LPUCHAR)(LPCTSTR)csNodeName);
	if (hnode<0)
	{
		CString strMsg = VDBA_MfcResourceString (IDS_MAX_NB_CONNECT);//_T("Maximum number of connections has been reached"
		strMsg += VDBA_MfcResourceString (IDS_E_ALTER_DB);			//	" - Cannot alter database.");
		AfxMessageBox (strMsg);
		return;
	}
	// Temporary for activate a session
	UCHAR buf[MAXOBJECTNAME];
	DOMGetFirstObject (hnode, OT_DATABASE, 0, NULL, FALSE, NULL, buf, NULL, NULL);

	ires = MfcGetRmcmdObjects_launcher ((char*) vnodeName ,(char *) RmcmdOwnerName);
	if (!ires) {
		CString strMsg = VDBA_MfcResourceString (IDS_E_RMCMD_USER_EXECUTING);
		strMsg += VDBA_MfcResourceString (IDS_E_ALTER_DB); //" - Cannot alter database.");
		AfxMessageBox (strMsg);
		CloseNodeStruct(hnode,FALSE);
		return;
	}
	// check the user is the Owner of the rmcmd object
	DBAGetUserName(vnodeName, UserName);
	if (x_stricmp((char*)UserName, (char*)RmcmdOwnerName)) {
		CString strFormatMsg;
		strFormatMsg.Format(IDS_E_OWNER_CHANGING_CATALOG,RmcmdOwnerName);
		AfxMessageBox (strFormatMsg);
		CloseNodeStruct(hnode,FALSE);
		return;
	}

	csTitle.Format("Catalog Page Size");
	csCommandLine.Format("sysmod %s -page_size=%ld",m_StructDatabaseInfo->szDBName,lNewPageSize);
	execrmcmd1(	(char *)(LPCTSTR)csNodeName,
				(char *)(LPCTSTR)csCommandLine,
				(char *)(LPCTSTR)csTitle,
				TRUE);
	CloseNodeStruct(hnode,FALSE);
}



void CxDlgCreateAlterDatabase::OnBnClickedEnableUnicode()
{
	if (m_ctrlEnableUnicode.GetCheck()==BST_CHECKED)
	{
		m_ctrlUnicodeEnable.EnableWindow(TRUE);
		m_ctrlUnicodeEnable2.EnableWindow(TRUE);
		m_ctrlUnicodeEnable2.SetCheck(1);
	}
	else
	{
		m_ctrlUnicodeEnable.EnableWindow(FALSE);
		m_ctrlUnicodeEnable2.EnableWindow(FALSE);
		m_ctrlUnicodeEnable2.SetCheck(0);
		m_ctrlUnicodeEnable.SetCheck(0);
	}
}

void CxDlgCreateAlterDatabase::OnBnClickedShowMoreOptions()
{
	if (!bExpanded)
	{
	CRect rect;
	this->GetWindowRect(&rect);
	this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (rect.bottom-rect.top)*3);

	m_ctrlMoreOptions.SetWindowTextA("<< Hide &Advanced");
	bExpanded = true;
	}
	else
	{
	CRect rect;
	this->GetWindowRect(&rect);
	this->MoveWindow(rect.left, rect.top, rect.right-rect.left, (rect.bottom-rect.top)/3);

	m_ctrlMoreOptions.SetWindowTextA("Show &Advanced >>");
	bExpanded = false;
	}

}

