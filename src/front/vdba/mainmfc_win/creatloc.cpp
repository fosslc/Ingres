/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**    Project : Ingres II/ VDBA
**
**    Source   : creatloc.cpp
**
**    Author   : Schalk Philippe
**
**    Purpose  : Create and Alter a location.
**
**   09-May-2001 (schph01)
**     SIR 102509 new dialog to manage the location.
**     If INGRES version is II 2.6 "raw data location" is available.
**     This code replace the existing code in dbadlg1\creatloc.c
**   06-Jun-2001(schph01)
**     (additional change for SIR 102509) update of previous change
**     because of backend changes
********************************************************************/
#include "stdafx.h"
#include "mainmfc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" 
{
#include "dba.h"
#include "main.h"
#include "dbaset.h"
#include "dbadlg1.h"
#include "domdata.h"
#include "dbaginfo.h"
#include "msghandl.h"
#include "lexical.h"
#include "getdata.h"
#include "dlgres.h"
extern LPTSTR INGRESII_llDBMSInfo(LPTSTR lpszConstName, LPTSTR lpVer);
}

#include "creatloc.h"

/////////////////////////////////////////////////////////////////////////////
// interface to dom2.c
extern "C" BOOL MfcDlgCreateLocation( LPLOCATIONPARAMS lpLocParam )
{
	int iret;
	CxDlgCreateLocation dlg;
	dlg.m_lpLocation = lpLocParam;

	// alter location not available for Raw Data Location
	if (!dlg.m_lpLocation->bCreate && dlg.m_lpLocation->iRawPct != 0) 
	{
		AfxMessageBox(IDS_E_ALTERLOCRAW);//"A Raw Data Location cannot be altered."
		return FALSE;
	}

	if (dlg.m_lpLocation->bCreate)
		dlg.IsRawLocAvailable();// location raw type not available on platforms NT and VMS
	else
		dlg.m_bLocRawAvailable = FALSE; // alter mode location raw type not available
	iret = dlg.DoModal();
	if (iret == IDOK)
		return TRUE;
	else
		return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
// CxDlgCreateLocation dialog


CxDlgCreateLocation::CxDlgCreateLocation(CWnd* pParent /*=NULL*/)
	: CDialog(CxDlgCreateLocation::IDD, pParent)
{
	//{{AFX_DATA_INIT(CxDlgCreateLocation)
	m_bCheckPoint = FALSE;
	m_bDatabase = FALSE;
	m_bDump = FALSE;
	m_bJournal = FALSE;
	m_bWork = FALSE;
	m_edArea = _T("");
	m_edLocationName = _T("");
	m_bRawArea = FALSE;
	m_editPctRaw = 0;
	//}}AFX_DATA_INIT
}


void CxDlgCreateLocation::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CxDlgCreateLocation)
	DDX_Control(pDX, IDC_EDIT_PCT_RAW, m_ctrlEdPctRaw);
	DDX_Control(pDX, IDC_STATIC_PCTRAW, m_ctrlPctRaw);
	DDX_Control(pDX, IDC_SPIN1, m_ctrlSpinPctRaw);
	DDX_Control(pDX, IDC_CHECK_WORK, m_ctrlCheckWork);
	DDX_Control(pDX, IDC_CHECK_JOURNAL, m_ctrlCheckJournal);
	DDX_Control(pDX, IDC_CHECK_DUMP, m_ctrlCheckDump);
	DDX_Control(pDX, IDC_CHECK_DATABASE, m_ctrlCheckDatabase);
	DDX_Control(pDX, IDC_CHECK_CHECKPOINT, m_ctrlCheckCheckPoint);
	DDX_Control(pDX, IDC_EDIT_LOC_NAME, m_ctrlEditLocationName);
	DDX_Control(pDX, IDC_EDIT_AREA, m_ctrlEditArea);
	DDX_Control(pDX, IDC_CHECK_RAW_AREA, m_ctrlRawArea);
	DDX_Check(pDX, IDC_CHECK_CHECKPOINT, m_bCheckPoint);
	DDX_Check(pDX, IDC_CHECK_DATABASE, m_bDatabase);
	DDX_Check(pDX, IDC_CHECK_DUMP, m_bDump);
	DDX_Check(pDX, IDC_CHECK_JOURNAL, m_bJournal);
	DDX_Check(pDX, IDC_CHECK_WORK, m_bWork);
	DDX_Text(pDX, IDC_EDIT_AREA, m_edArea);
	DDX_Text(pDX, IDC_EDIT_LOC_NAME, m_edLocationName);
	DDX_Check(pDX, IDC_CHECK_RAW_AREA, m_bRawArea);
	DDX_Text(pDX, IDC_EDIT_PCT_RAW, m_editPctRaw);
	DDV_MinMaxInt(pDX, m_editPctRaw, 0, 100);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CxDlgCreateLocation, CDialog)
	//{{AFX_MSG_MAP(CxDlgCreateLocation)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_CHECK_RAW_AREA, OnCheckRawArea)
	ON_EN_CHANGE(IDC_EDIT_AREA, OnChangeEditArea)
	ON_EN_CHANGE(IDC_EDIT_LOC_NAME, OnChangeEditLocName)
	ON_WM_CANCELMODE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CxDlgCreateLocation message handlers


void CxDlgCreateLocation::OnDestroy() 
{
	CDialog::OnDestroy();
	PopHelpId ();
}

BOOL CxDlgCreateLocation::OnInitDialog() 
{
	CString csTitle;
	CDialog::OnInitDialog();

	if (m_lpLocation->bCreate)
	{
		csTitle.Format(IDS_T_CREATE_LOCATION,GetVirtNodeName (GetCurMdiNodeHandle ()));
		PushHelpId(IDD_LOCATION);
	}
	else
	{
		csTitle.Format (IDS_T_ALTER_LOCATION,GetVirtNodeName (GetCurMdiNodeHandle ()));
		m_ctrlEditArea.EnableWindow(FALSE);
		m_ctrlEditLocationName.EnableWindow(FALSE);
		m_ctrlRawArea.EnableWindow(FALSE);
		PushHelpId(IDD_ALTERLOCATION);
	}

	SetWindowText (csTitle);

	if (GetOIVers() >= OIVERS_26)
	{
		if (!m_bLocRawAvailable)
			m_ctrlRawArea.EnableWindow(FALSE);
	}
	else
		m_ctrlRawArea.ShowWindow(SW_HIDE);

	m_ctrlEditArea.LimitText(MAXAREANAME-1);
	m_ctrlEditLocationName.LimitText(MAXOBJECTNAME -1);
	UpdateControlsFromStructure ();
	EnableDisableOKButton ();
	OnCheckRawArea();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CxDlgCreateLocation::UpdateControlsFromStructure()
{

	m_bCheckPoint    = m_lpLocation->bLocationUsage[LOCATIONCHECKPOINT];
	m_bDatabase      = m_lpLocation->bLocationUsage[LOCATIONDATABASE];
	m_bJournal       = m_lpLocation->bLocationUsage[LOCATIONJOURNAL];
	m_bWork          = m_lpLocation->bLocationUsage[LOCATIONWORK];
	m_bDump          = m_lpLocation->bLocationUsage[LOCATIONDUMP];

	m_edArea         = m_lpLocation->AreaName;
	m_edLocationName = m_lpLocation->objectname;

	if (m_lpLocation->iRawPct == 0) // 0 it is not a raw location
		m_bRawArea       = FALSE;
	else
		m_bRawArea       = TRUE ;

	UpdateData(FALSE);

}

void CxDlgCreateLocation::OnCheckRawArea()
{
	if (m_ctrlRawArea.GetCheck() == BST_CHECKED)
	{
		if ( m_ctrlCheckCheckPoint.GetCheck() == BST_CHECKED ||
			 m_ctrlCheckDump.GetCheck ()      == BST_CHECKED ||
			 m_ctrlCheckJournal.GetCheck ()   == BST_CHECKED ||
			 m_ctrlCheckWork.GetCheck ()      == BST_CHECKED )
				AfxMessageBox(IDS_E_UNCHECHECK);//"All usage types other than 'database' will be un-checked.\n\n"
												//"('database' is the only possible usage for Raw Data Locations)");

		m_ctrlSpinPctRaw.ShowWindow(SW_SHOW);
		m_ctrlPctRaw.ShowWindow(SW_SHOW);
		m_ctrlEdPctRaw.ShowWindow(SW_SHOW);

		//only database location is available for location type "Raw"
		m_ctrlCheckDatabase.EnableWindow(FALSE);
		m_ctrlCheckCheckPoint.EnableWindow (FALSE);
		m_ctrlCheckDump.EnableWindow (FALSE);
		m_ctrlCheckJournal.EnableWindow (FALSE);
		m_ctrlCheckWork.EnableWindow (FALSE);

		m_ctrlCheckDatabase.SetCheck(TRUE);
		m_ctrlCheckCheckPoint.SetCheck (FALSE);
		m_ctrlCheckDump.SetCheck (FALSE);
		m_ctrlCheckJournal.SetCheck (FALSE);
		m_ctrlCheckWork.SetCheck (FALSE);
	}
	else
	{
		m_ctrlSpinPctRaw.ShowWindow(SW_HIDE);
		m_ctrlPctRaw.ShowWindow(SW_HIDE);
		m_ctrlEdPctRaw.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_CHECK_DATABASE)->EnableWindow (TRUE);
		GetDlgItem(IDC_CHECK_CHECKPOINT) ->EnableWindow (TRUE);
		GetDlgItem(IDC_CHECK_DUMP) ->EnableWindow (TRUE);
		GetDlgItem(IDC_CHECK_JOURNAL)->EnableWindow (TRUE);
		GetDlgItem(IDC_CHECK_WORK)->EnableWindow (TRUE);
	}
}

void CxDlgCreateLocation::EnableDisableOKButton()
{
	if ( m_ctrlEditArea.LineLength() == 0 ||
		 m_ctrlEditLocationName.LineLength() == 0 )
		GetDlgItem (IDOK)->EnableWindow (FALSE);
	else
		GetDlgItem (IDOK)->EnableWindow (TRUE);
}

void CxDlgCreateLocation::OnChangeEditArea()
{
	EnableDisableOKButton();
}

void CxDlgCreateLocation::OnChangeEditLocName()
{
	EnableDisableOKButton();
}

int CxDlgCreateLocation::AlterLocation()
{
	LOCATIONPARAMS loc2;
	LOCATIONPARAMS loc3;
	BOOL Success, bOverwrite;
	int  ires,irestmp,hdl   = GetCurMdiNodeHandle ();
	TCHAR *vnodeName        = GetVirtNodeName (hdl);
	BOOL bChangedbyOtherMessage = FALSE;
	memset (&loc2,0,sizeof(LOCATIONPARAMS));
	memset (&loc3,0,sizeof(LOCATIONPARAMS));

	x_strcpy ((LPTSTR)loc2.objectname, (LPTSTR)m_lpLocation->objectname);
	x_strcpy ((LPTSTR)loc3.objectname, (LPTSTR)m_lpLocation->objectname);
	Success = TRUE;
	ires    = GetDetailInfo ((LPUCHAR)vnodeName, OT_LOCATION, &loc2, TRUE, &hdl);
	if (ires != RES_SUCCESS)
	{
		ReleaseSession(hdl,RELEASE_ROLLBACK);
		switch (ires)
		{
			case RES_ENDOFDATA:
			ires = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_CREATE);
			SetFocus();
			if (ires == IDYES)
			{
				Success = FillStructureFromControls (&loc3);
				if (Success)
				{
					ires = DBAAddObject ((LPUCHAR)vnodeName, OT_LOCATION, (void *) &loc3 );
					if (ires != RES_SUCCESS)
					{
						ErrorMessage ((UINT) IDS_E_CREATE_LOCATION_FAILED, ires);
						Success=FALSE;
					}
				}
			}
			break;
			default:
			Success=FALSE;
			ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			break;
		}
		FreeAttachedPointers ((void*)&loc2, OT_LOCATION);
		FreeAttachedPointers ((void*)&loc3, OT_LOCATION);
		return Success;
	}

	if (!AreObjectsIdentical (m_lpLocation, &loc2, OT_LOCATION))
	{
		ReleaseSession(hdl,RELEASE_ROLLBACK);
		ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
		bChangedbyOtherMessage = TRUE;
		irestmp=ires;
		if (ires == IDYES)
		{
			ires = GetDetailInfo ((LPUCHAR)vnodeName, OT_LOCATION, m_lpLocation, FALSE, &hdl);
			if (ires != RES_SUCCESS)
			{
				Success =FALSE;
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			}
			else
			{
				UpdateControlsFromStructure ();
				FreeAttachedPointers ((void*)&loc2, OT_LOCATION);
				return FALSE;
			}
		}
		else
		{
			// Double Confirmation ?
			ires   = ConfirmedMessage ((UINT)IDS_C_ALTER_CONFIRM_OVERWRITE);
			if (ires != IDYES)
				Success=FALSE;
			else
			{
				char buf [2*MAXOBJECTNAME+5], buf2 [MAXOBJECTNAME];
				if (GetDBNameFromObjType (OT_USER,  buf2, NULL))
				{
					wsprintf (buf,"%s::%s", vnodeName, buf2);
					ires = Getsession ((LPUCHAR)buf, SESSION_TYPE_CACHEREADLOCK, &hdl);
					if (ires != RES_SUCCESS)
						Success = FALSE;
				}
				else
					Success = FALSE;
			}
		}
		if (!Success)
		{
			FreeAttachedPointers ((void*)&loc2, OT_LOCATION);
			return Success;
		}
	}

	Success = FillStructureFromControls ( &loc3);
	if (Success)
	{
		ires = DBAAlterObject ((void*)m_lpLocation, (void*)&loc3, OT_LOCATION, hdl);
		if (ires != RES_SUCCESS)
		{
			Success=FALSE;
			ErrorMessage ((UINT) IDS_E_ALTER_LOCATION_FAILED, ires);
			ires = GetDetailInfo ((LPUCHAR)vnodeName, OT_LOCATION, &loc2, FALSE, &hdl);
			if (ires != RES_SUCCESS)
				ErrorMessage ((UINT)IDS_E_ALTER_ACCESS_PROBLEM, RES_ERR);
			else
			{
				if (!AreObjectsIdentical (m_lpLocation, &loc2, OT_LOCATION))
				{
					if (!bChangedbyOtherMessage)
					{
						ires  = ConfirmedMessage ((UINT)IDS_C_ALTER_RESTART_LASTCHANGES);
					}
					else
						ires=irestmp;

					if (ires == IDYES)
					{
						ires = GetDetailInfo ((LPUCHAR)vnodeName, OT_LOCATION, m_lpLocation, FALSE, &hdl);
						if (ires == RES_SUCCESS)
							UpdateControlsFromStructure ();
					}
					else
						bOverwrite = TRUE;
					
				}
			}
		}
	}

	FreeAttachedPointers ((void*)&loc2, OT_LOCATION);
	FreeAttachedPointers ((void*)&loc3, OT_LOCATION);
	return Success;
}


int CxDlgCreateLocation::CreateLocation()
{
	int     ires;
	LPUCHAR vnodeName = (LPUCHAR)GetVirtNodeName (GetCurMdiNodeHandle ());

	ires = DBAAddObject (vnodeName, OT_LOCATION, (void *) m_lpLocation );

	if (ires != RES_SUCCESS)
	{
		ErrorMessage ((UINT) IDS_E_CREATE_LOCATION_FAILED, ires);
		return FALSE;
	}

	return TRUE;

}

void CxDlgCreateLocation::OnOK() 
{
	BOOL bSuccess;

	if (m_lpLocation->bCreate)
	{
		bSuccess = FillStructureFromControls ( NULL );
		if (!bSuccess)
			return;
		bSuccess = CreateLocation ();
	}
	else
		bSuccess = AlterLocation ();

	if (!bSuccess)
		return;
	else
		CDialog::OnOK();
}


BOOL CxDlgCreateLocation::FillStructureFromControls(LPLOCATIONPARAMS lpLocTempo)
{
	LPLOCATIONPARAMS lpCurrentLoc;
	
	if ( !UpdateData())
		return FALSE;

	if (!lpLocTempo)
		lpCurrentLoc = m_lpLocation;
	else
		lpCurrentLoc =lpLocTempo;


	lpCurrentLoc->bLocationUsage[LOCATIONCHECKPOINT] = m_bCheckPoint;
	lpCurrentLoc->bLocationUsage[LOCATIONDATABASE]   = m_bDatabase;
	lpCurrentLoc->bLocationUsage[LOCATIONJOURNAL]    = m_bJournal;
	lpCurrentLoc->bLocationUsage[LOCATIONWORK]       = m_bWork;
	lpCurrentLoc->bLocationUsage[LOCATIONDUMP]       = m_bDump;
	x_strcpy ( (LPTSTR)lpCurrentLoc->AreaName, (LPTSTR)(LPCTSTR)m_edArea);
	x_strcpy ( (LPTSTR)lpCurrentLoc->objectname, (LPTSTR)(LPCTSTR)m_edLocationName);

	if (!IsObjectNameValid ((LPTSTR)lpCurrentLoc->objectname, OT_UNKNOWN))
	{
		AfxMessageBox( IDS_E_NOT_A_VALIDE_OBJECT);
		m_ctrlEditLocationName.SetSel(0,-1);
		m_ctrlEditLocationName.SetFocus();
		return FALSE;
	}

	if (m_ctrlRawArea.GetCheck() == BST_CHECKED)
		lpCurrentLoc->iRawPct = m_editPctRaw;
	else
		lpCurrentLoc->iRawPct = 0;

	return TRUE;
}

void CxDlgCreateLocation::IsRawLocAvailable()
{
	int ires,isess;
	TCHAR tcVersion[40];
	TCHAR buf[MAXOBJECTNAME*2];
	CString csVer;
	m_bLocRawAvailable = FALSE;

	if (GetOIVers() < OIVERS_26)
		return;
	wsprintf(buf,"%s::iidbdb",GetVirtNodeName (GetCurMdiNodeHandle ()));
	ires = Getsession((LPUCHAR)buf,SESSION_TYPE_CACHENOREADLOCK, &isess);
	if (ires != RES_SUCCESS)
		return;

	csVer = INGRESII_llDBMSInfo(_T("_version"),tcVersion);
	ReleaseSession(isess, RELEASE_COMMIT);
	if (csVer.IsEmpty())
		return;
	csVer.MakeLower();
	// A Raw location type is not available on platform NT and VMS
	if (csVer.Find(_T("w32")) == -1 && csVer.Find(_T("vms")) == -1)
		m_bLocRawAvailable = TRUE;
}
