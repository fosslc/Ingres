/*
**  Copyright (c) 2006 Ingres Corporation.  All Rights Reserved.
*/

/*
**  Name: PreInstallation.h: interface for the CPreInstallation class.
**
**  History:
**
**	05-Jun-2001 (penga03)
**	    Created.
**	07-Jun-2001 (penga03)
**	    Remove the obsolete member variable m_ModulePath.
**	23-July-2001 (penga03)
**	    Added m_Ver25.
**	05-nov-2001 (penga03)
**	    Added a new class CInstallation.
**	    Added a new member variable m_Installations in class CPreInstallation.
**	31-dec-2001 (penga03)
**	    Removed m_SkipLicenseCheck.
**	30-jul-2002 (penga03)
**	    Added m_embedded and Modified AddInstallation() and CInstallation().
**	30-aug-2002 (penga03)
**	    Added m_MSILog to hold the path of the log file.
**	16-jul-2004 (penga03)
**	    Added m_InstallMDB.
**	10-sep-2004 (penga03)
**	    Removed MDB.
**	13-dec-2004 (penga03)
**	    Added m_UpgradeType, m_RestartIngres.
**	01-Mar-2006 (drivi01)
**	    Reused and adopted for MSI Patch Installer on Windows.
**		Added m_SilentInstall.
*/

#if !defined(AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_)
#define AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPreInstallation  
{
public:
	CString m_MSILog;
	CObArray m_Installations;
	CString m_Ver25;
	CString m_ResponseFile;
	CString m_CreateResponseFile;
	CString m_EmbeddedRelease;
	CString m_InstallCode;
	BOOL m_SilentInstall;
	CPreInstallation();
	virtual ~CPreInstallation();
	BOOL LaunchPatchInstaller();
private:
	void FindOldInstallations();
	void AddInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded);
public:
	int m_UpgradeType;
	int m_RestartIngres;
};

class CInstallation : public CObject  
{
public:
	CInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded);
	CString m_embedded;
	BOOL m_ver25;
	CString m_path;
	CString m_id;
	CInstallation();
	virtual ~CInstallation();

};

#endif // !defined(AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_)
