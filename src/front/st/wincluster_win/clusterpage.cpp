/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: ClusterPage.cpp - Windows Cluster Service property page 
** 
** Description:
** 	The cluster property page displays and edits the cluster group name and 
** 	resource name to be used by Ingres.
** 
** History:
**	29-apr-2004 (wonst02)
**	    Created.
** 	29-jul-2004 (wonst02)
** 		Display the cluster group name and resource name for the Ingres service.
**	30-nov-2004 (wonst02) Bug 113345
**	    Remove HELP from the property page.
**	02-dec-2004 (wonst02) Bug 113522
**	    Add Ingres service password and confirm password.
**	07-May-2009 (drivi01)
**	    Add a define for DNS_MAX_LABEL_LENGTH because in
**	    clusapi.h MAX_CLUSTERNAME_LENGTH is defined to be
**	    DNS_MAX_LABEL_LENGTH which isn't defined.
**	    This change is done in effort to port to VC 2008.
*/

// clusterpage.cpp : implementation file
//

#include "stdafx.h"
#include <clusapi.h>
#include <resapi.h>
#include "wincluster.h"
#include ".\clusterpage.h"

#define DNS_MAX_LABEL_LENGTH	63

// WcClusterPage dialog

IMPLEMENT_DYNAMIC(WcClusterPage, CPropertyPage)
WcClusterPage::WcClusterPage()
	: CPropertyPage(WcClusterPage::IDD)
	, m_ClusterName(_T(""))
	, m_GroupName(_T(""))
	, m_ResourceName(_T(""))
	, m_ServicePassword(_T(""))
	, m_ConfirmPassword(_T(""))
{
    m_psp.dwFlags &= ~(PSP_HASHELP);
}

WcClusterPage::~WcClusterPage()
{
}

void WcClusterPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CLUSTERNAME_EDIT, m_ClusterNameCtrl);
	DDX_Text(pDX, IDC_CLUSTERNAME_EDIT, m_ClusterName);
	DDX_Control(pDX, IDC_GROUPNAME_COMBO, m_GroupNameCtrl);
	DDX_CBString(pDX, IDC_GROUPNAME_COMBO, m_GroupName);
	DDX_Control(pDX, IDC_RESOURCENAME_COMBO, m_ResourceNameCtrl);
	DDX_CBString(pDX, IDC_RESOURCENAME_COMBO, m_ResourceName);
	DDX_Control(pDX, IDC_SERVICEPASSWORD_EDIT, m_ServicePasswordCtrl);
	DDX_CBString(pDX, IDC_SERVICEPASSWORD_EDIT, m_ServicePassword);
	DDX_Control(pDX, IDC_CONFIRMPASSWORD_EDIT, m_ConfirmPasswordCtrl);
	DDX_CBString(pDX, IDC_CONFIRMPASSWORD_EDIT, m_ConfirmPassword);
	DDX_Control(pDX, IDC_IMAGE, m_image);
}

BEGIN_MESSAGE_MAP(WcClusterPage, CPropertyPage)
END_MESSAGE_MAP()


// WcClusterPage message handlers

BOOL WcClusterPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	/*
    ** Load the dialog's strings.
    */
	DWORD dwRet = ERROR_SUCCESS;
	WCHAR szClusterName[MAX_CLUSTERNAME_LENGTH] = L"";
	DWORD cchClusterName = MAX_CLUSTERNAME_LENGTH;
	HCLUSENUM hEnum = NULL;

	// Use the OpenCluster function to open a connection to the local cluster (NULL->local).
	thePreSetup.m_hCluster = OpenCluster(NULL);

	// Use GetClusterInformation function to retrieve the local cluster's name and version.
	if (thePreSetup.m_hCluster &&
		((dwRet=GetClusterInformation( thePreSetup.m_hCluster, szClusterName,
			&cchClusterName, NULL )) == ERROR_SUCCESS))
	{
		m_ClusterName = szClusterName;	// Save the cluster name
	    SetDlgItemText(IDC_CLUSTERNAME_EDIT, m_ClusterName);

		// Open an enumerator handle for cluster groups and resources
		hEnum = ClusterOpenEnum( thePreSetup.m_hCluster, 
				CLUSTER_ENUM_GROUP|CLUSTER_ENUM_RESOURCE );

		if (hEnum)
		{
			WCHAR szName[MAXIMUM_NAMELENGTH] = L"";
			DWORD cchName = MAXIMUM_NAMELENGTH;
			// Enumerate all the cluster groups and cluster resources.
			DWORD dwType;
			DWORD dwIndex = 0;
			CString installCode;
			installCode.Format("Ingres_Database_%s", thePreSetup.m_InstallCode.GetBuffer());

			while ((dwRet=ClusterEnum( hEnum, dwIndex++, &dwType,
					szName, &cchName )) == ERROR_SUCCESS)
			{
				if (dwType == CLUSTER_ENUM_GROUP)
				{
					thePreSetup.AddClusterGroup(szName);
					m_GroupNameCtrl.AddString( CW2CT(szName) );
				}
				else if (dwType == CLUSTER_ENUM_RESOURCE)
				{
					HRESOURCE hRes = OpenClusterResource(thePreSetup.m_hCluster, szName);
					
					LPWSTR szType = GetResourceType(hRes);
					LPWSTR szPropValue = NULL;
					LPWSTR szResourceGroup = NULL;
					if (wcscmp( szType, L"Generic Service") == 0)
					{
						dwRet = GetPrivPropValue( hRes, L"ServiceName", &szPropValue );
						if (dwRet == ERROR_SUCCESS)
						{	// Is this the Ingres service resource?
							if (wcscmp( szPropValue, CT2CW(installCode.GetBuffer())) == 0)
							{
								m_ResourceName = szName;
								SetDlgItemText(IDC_RESOURCENAME_COMBO, m_ResourceName);
								dwRet = GetResourceState(hRes, &szResourceGroup);
								if (dwRet == ERROR_SUCCESS)
								{
									m_GroupName = szResourceGroup;
									SetDlgItemText(IDC_GROUPNAME_COMBO, m_GroupName);
								}
							}
						}
					}

					if (dwRet == ERROR_SUCCESS)
					{
						thePreSetup.AddClusterResource(szName, szType);
						m_ResourceNameCtrl.AddString( CW2CT(szName) );
					}
					CloseClusterResource(hRes);
					LocalFree( szType );
					LocalFree( szPropValue );
					LocalFree( szResourceGroup );
				}
				cchName = MAXIMUM_NAMELENGTH;
			};
			if (! (dwRet == ERROR_SUCCESS || dwRet == ERROR_NO_MORE_ITEMS) )
			{
				property->SysError(IDS_CLUSTERENUM_ERR, NULL, GetLastError());
			}
			ClusterCloseEnum( hEnum );
			hEnum = NULL;
		}
		else
		{
			property->SysError(IDS_CLUSTERENUMOPEN_ERR, NULL, GetLastError());
		}
	}
	else
	{
		property->SysError(IDS_CLUSTEROPEN_ERR, NULL, GetLastError());
	}

	if (property->m_GlobalError)
		property->EndDialog(property->m_GlobalError);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL WcClusterPage::OnSetActive()
{
	property->SetWizardButtons(PSWIZB_NEXT);
	return CPropertyPage::OnSetActive();
}


LRESULT WcClusterPage::OnWizardNext()
{
    GetDlgItemText(IDC_CLUSTERNAME_EDIT, m_ClusterName);
	thePreSetup.m_ClusterName = m_ClusterName;

    GetDlgItemText(IDC_GROUPNAME_COMBO, m_GroupName);
    m_GroupName.TrimLeft();
    m_GroupName.TrimRight();
	
    if (m_GroupName == "")
    {
		m_GroupNameCtrl.SetFocus();
		MyError(IDS_NEED_GROUPNAME);
		return -1;
    }
	thePreSetup.m_GroupName = m_GroupName;

    GetDlgItemText(IDC_RESOURCENAME_COMBO, m_ResourceName);
    m_ResourceName.TrimLeft();
    m_ResourceName.TrimRight();
	
	if (m_ResourceName == "")
	{
		m_ResourceNameCtrl.SetFocus();
		MyError(IDS_NEED_RESOURCENAME);
		return -1;
	}
	thePreSetup.m_ResourceName = m_ResourceName;

    GetDlgItemText(IDC_SERVICEPASSWORD_EDIT, m_ServicePassword);
    GetDlgItemText(IDC_CONFIRMPASSWORD_EDIT, m_ConfirmPassword);
	if (m_ServicePassword.Compare(m_ConfirmPassword))
	{
		m_ServicePasswordCtrl.SetFocus();
		MyError(IDS_CONFIRMPASSWORD);
		return -1;
	}
	thePreSetup.m_ServicePassword = m_ServicePassword;

	return CPropertyPage::OnWizardNext();
}


// Get the resource type for the named cluster resource.
LPWSTR WcClusterPage::GetResourceType(HRESOURCE hRes)
{
    DWORD  dwResult      = ERROR_SUCCESS;
    DWORD  cbBufSize     = MAXIMUM_NAMELENGTH;
    DWORD  cbDataSize    = 0;
    LPWSTR lpszType      = NULL;
    DWORD  dwControlCode = CLUSCTL_RESOURCE_GET_RESOURCE_TYPE;

    lpszType = (LPWSTR) LocalAlloc( LPTR, cbBufSize );

    if( lpszType == NULL )
    {
        dwResult = GetLastError();
        goto endf;
    }

	//  Call the control code function to get the resource type:
	do 
	{
		dwResult = ClusterResourceControl(
					hRes,            // Resource to query.
					NULL,            // Node to perform operation.
					dwControlCode,   // Control code.
					NULL,            // Input buffer (not used).
					0,               // Input buffer size (not used).
					(LPVOID)lpszType,// Output buffer.
					cbBufSize,       // Byte size of the buffer.
					&cbDataSize      // Byte size of the resulting data.
				);
		if (dwResult == ERROR_MORE_DATA)
		{
		//	Reallocate if necessary. If the buffer was too small, 
		//	cbDataSize now indicates the necessary byte size.
			LocalFree( lpszType );
			cbBufSize = cbDataSize;
			lpszType = (LPWSTR) LocalAlloc( LPTR, cbBufSize );
			if( lpszType == NULL )
			{
				dwResult = GetLastError();
				goto endf;
			}
		}
    } while ( dwResult == ERROR_MORE_DATA );

//
//  Handle errors.
//
    if( dwResult != ERROR_SUCCESS )
    {
        LocalFree( lpszType );
        lpszType = NULL;
    }

endf:

    SetLastError( dwResult );

	return lpszType;
}

// ====================================================================
// 
// Description:
// 
//     Retrieves the read-write private properties of a resource
//     using CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES.
//     
// >!! Call LocalFree on the returned pointer to free the memory. !!<
// 
// 
// Arguments:
// 
//    IN HRESOURCE hRes           Handle to the resource to query.
//                                
//    IN HNODE     hNode          Handle to the node to perform the
//                                operation or NULL to default to
//                                the owner node.
// 
//    OUT LPDWORD  lpcbDataSize   Returns the byte size of the
//                                resulting data.
// 
// Return Value:
// 
//    If successful, returns a property list containing the
//    read-write private properties of the resource identified
//    by hRes.
// 
//    If unsuccessful, returns a NULL pointer; 
//    call GetLastError() for details.
// 
// 
// Local Variables:
// 
//    dwResult        Captures return values.
//    cbBufSize       Allocated byte size of the output buffer.
//    lpOutBuffer     Output buffer for the control code operation.
//    dwControlCode   Specifies the operation to perform.
// 
// ====================================================================
LPVOID WcClusterPage::GetPrivProperties(
    IN HRESOURCE hRes,
    IN HNODE     hNode,
    OUT LPDWORD  lpcbDataSize)
{
    DWORD  dwResult      = ERROR_SUCCESS;
    DWORD  cbBufSize     = MAXIMUM_NAMELENGTH * sizeof( WCHAR );
    LPVOID lpOutBuffer   = NULL;
    DWORD  dwControlCode = CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES;

    lpOutBuffer = LocalAlloc( LPTR, cbBufSize );

    if( lpOutBuffer == NULL )
    {
        dwResult = GetLastError();
        goto endf;
    }

//
//  1.  Call the control code function:  
//
    dwResult = ClusterResourceControl(
                   hRes,            // Resource to query.
                   hNode,           // Node to perform operation.
                   dwControlCode,   // Control code.
                   NULL,            // Input buffer (not used).
                   0,               // Input buffer size (not used).
                   lpOutBuffer,     // Output buffer.
                   cbBufSize,       // Byte size of the buffer.
                   lpcbDataSize     // Byte size of the resulting data.
               );

//
//  2.  Reallocate if necessary. If the buffer was too small, 
//      lpcbDataSize now indicates the necessary byte size.
//
    if ( dwResult == ERROR_MORE_DATA )
    {
        LocalFree( lpOutBuffer );

        cbBufSize = *lpcbDataSize;

        lpOutBuffer = LocalAlloc( LPTR, cbBufSize );

        if( lpOutBuffer == NULL )
        {
            dwResult = GetLastError();
            goto endf;
        }

        dwResult =
            ClusterResourceControl(
                   hRes,            // Resource to query.
                   hNode,           // Node to perform operation.
                   dwControlCode,   // Control code.
                   NULL,            // Input buffer (not used).
                   0,               // Input buffer size (not used).
                   lpOutBuffer,     // Output buffer.
                   cbBufSize,       // Byte size of the buffer.
                   lpcbDataSize     // Byte size of the resulting data.
            );
    }

//
//  3.  Handle errors.
//
    if( dwResult != ERROR_SUCCESS )
    {
        LocalFree( lpOutBuffer );
        lpOutBuffer = NULL;
        *lpcbDataSize = 0;
    }


endf:

    SetLastError( dwResult );

    return lpOutBuffer;  // Call LocalFree to release the memory.

//
//  The calling function will need to parse the property list to
//  extract the property names and values.
//
//  See "Parsing Property Lists" for procedures and an example.
//

}

//--------------------------------------------------------------------
//
//  GetPrivPropValue
//
//  Retrieves a resource's read-write private property value.
//
//  Arguments:
//      IN HRESOURCE hRes           Handle to the resource.
//      IN LPWSTR lpszPropName      Name of property to find.
//      OUT LPWSTR* lpszPropValue   Returns the value of property.
//
// Return Value:
//      Win32 code
//
//--------------------------------------------------------------------
DWORD WcClusterPage::GetPrivPropValue( 
    IN HRESOURCE hRes,
    IN LPCWSTR lpszPropName,
    OUT LPWSTR* lpszPropValue)
{
    DWORD dwResult = ERROR_SUCCESS;
    DWORD cbPropListSize = 0;

//  Retrieve a property list

    LPVOID lpPropList = GetPrivProperties(
                            hRes,
                            NULL,           // use local cluster node
                            &cbPropListSize );

    if( lpPropList != NULL )
    {

    	//  Parse the property list for the property specified
    	//  by lpszPropName.

		// The ResUtilFindSzProperty utility function locates a string property 
		// in a property list.
        dwResult = ResUtilFindSzProperty(lpPropList, cbPropListSize, 
			lpszPropName, lpszPropValue );
		switch (dwResult)
		{
			case ERROR_SUCCESS: // The specified property was found in the property list.
				break;
			case ERROR_INVALID_DATA: // The property list is incorrectly formatted. 
				break;
			case ERROR_NOT_ENOUGH_MEMORY: // The function could not allocate a buffer in which to return the property value. 
				break;
			case ERROR_FILE_NOT_FOUND: // The specified property could not be located in the property list. 
				break;
			default:
				break;
		}

        LocalFree( lpPropList );

    }
    else
    {
        dwResult = GetLastError();
    }

    return dwResult;

}

DWORD WcClusterPage::GetResourceState(HRESOURCE hRes, 
					   LPWSTR *lpszGroupName)
{
    DWORD     cchNameSize = MAXIMUM_NAMELENGTH;
 
    *lpszGroupName = (LPWSTR) LocalAlloc( LPTR, MAXIMUM_NAMELENGTH );
    if( *lpszGroupName == NULL )
    {
        return GetLastError();
    }

	//
    // Use the valid resource handle to call GetClusterResourceState.
    // This yields the name of the group to which it belongs, which
    // can be used to obtain a group handle.
    //
    DWORD dwState = GetClusterResourceState(
                hRes,                       // resource handle
                NULL,                       // buffer for node name
                0,                          // length of node name buffer
                *lpszGroupName,	            // buffer for group name
                &cchNameSize );	    		// length of group name buffer
	switch (dwState)
	{
		case ClusterResourceInitializing:	// The resource is performing initialization. 
		case ClusterResourceOnline:			// The resource is operational and functioning normally. 
		case ClusterResourceOffline:		// The resource is not operational. 
		case ClusterResourceFailed:			// The resource has failed. 
		case ClusterResourcePending:		// The resource is in the process of coming online or going offline. 
		case ClusterResourceOnlinePending:	// The resource is in the process of coming online. 
		case ClusterResourceOfflinePending:	// The resource is in the process of going offline. 
			dwState = ERROR_SUCCESS;
			break;
		case ClusterResourceStateUnknown:	// The operation was not successful. 
			// For more information about the error, call the Win32 function GetLastError. 
			LocalFree(*lpszGroupName);
			*lpszGroupName = NULL;
			return GetLastError();
			break;
		default:
			break;
	}

	return dwState;
}
