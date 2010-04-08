/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: PreSetup.h: interface for the WcPreSetup class.
**
** History:
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
** 	29-jul-2004 (wonst02)
** 		Remove parameter from WcPreSetup::AddClusterResource().
** 	03-aug-2004 (wonst02)
** 		Add comments.
**	02-dec-2004 (wonst02) Bug 113522
**	    Add Ingres service password.
*/

#if !defined(AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_)
#define AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <clusapi.h>
#include "afxcoll.h"

class WcPreSetup  
{
public:
// Construction
	WcPreSetup();
	virtual ~WcPreSetup();

// Methods
	BOOL SetupHighAvailability();
	void AddClusterGroup(LPCWSTR name);
	// Add a resource to the Cluster resource list
	void AddClusterResource(LPCWSTR name, LPCWSTR type);
	void AddClusterResource(LPCTSTR name, LPCTSTR type);
	// Remove all resources from the resource list
	void EmptyResourceList(void);
	// Add a resource to the Ingres resource dependency list
	void AddDependency(LPCWSTR name, LPCWSTR type);
	void AddDependency(LPCTSTR name, LPCTSTR type);
	// Empty the dependency list
	void EmptyDependencies(void);

private:
	void FindOldInstallations();
	void AddInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded);

// Member Attributes
public:
	CString m_MSILog;
	CObArray m_Installations;	// Current Ingres installations
	CString m_Ver25;
	CString m_II_System;
	CString m_ResponseFile;
	CString m_CreateResponseFile;
	CString m_EmbeddedRelease;
	CString m_InstallCode;		// Two-letter Ingres Install code
	CString m_ClusterName;		// Windows cluster name
	HCLUSTER m_hCluster;		// Handle for cluster API
	CObArray m_ClusterGroups;	// List of cluster groups
	CString m_GroupName;		// Name of cluster group containing the Ingres service resource
	CObArray m_ClusterResources;// List of cluster resources
	CString m_ResourceName;		// Name of Ingres service resource
	CObArray m_Dependencies;	// Ingres service resource dependencies
	CString m_ServicePassword;	// Password for the Ingres service
};

class WcInstallation : public CObject  
{
public:
// Construction
	WcInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded);
	WcInstallation();
	virtual ~WcInstallation();

// Member Attributes
	BOOL m_ver25;
	CString m_embedded;
	CString m_path;
	CString m_id;
};

class WcClusterGroup : public CObject
{
// Construction
public:
	WcClusterGroup::WcClusterGroup(LPCWSTR name);
	virtual ~WcClusterGroup();

// Member Attributes
	CString m_name;
};

class WcClusterResource : public CObject
{
// Construction
public:
	WcClusterResource::WcClusterResource(LPCWSTR name, LPCWSTR type);
	WcClusterResource::WcClusterResource(LPCTSTR name, LPCTSTR type);
	virtual ~WcClusterResource();

// Member Attributes
	CString m_name;
	CString m_type;
};

#endif // !defined(AFX_PREINSTALLATION_H__07D79F53_A1D1_40C9_90F0_F313E872141D__INCLUDED_)
