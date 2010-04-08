/*
**  Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: PreSetup.cpp: Pre-setup information for the High Availability Option setup wizard
** 
** Description:
** 	Contains all the pre-setup cluster information for an Ingres installation.
**
** History:
**	29-apr-2004 (wonst02)
**	    Created; cloned from similar enterprise_preinst directory.
**	22-jun-2004 (wonst02)
**		Remove all "Advantage" qualifiers and use "Ingres" alone.
** 	29-jul-2004 (wonst02)
** 		Remove parameter from WcPreSetup::AddClusterResource().
** 	03-aug-2004 (wonst02)
** 		Add header comments.
**	02-dec-2004 (wonst02) Bug 113522
**	    Add Ingres service password.
*/

#include "stdafx.h"
#include "wincluster.h"
#include <msiquery.h>
#include <Winsvc.h>
#include <WinVer.h>
#include ".\presetup.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

char nfname[MAX_PATH+1], efname[MAX_PATH+1], ndcab[MAX_PATH+1], edcab[MAX_PATH+1];
CStringList CodeList;

BOOL setupii_edit(char *iicode, char *path);
BOOL setupii_vexe(MSIHANDLE hDatabase, char *szQuery, char *szValue=0);
DWORD ThreadCopyCab(LPVOID lpParameter);
BOOL check_windowsinstaller(void);
BOOL UpdateComponentId(MSIHANDLE hDatabase, int id);
INT CompareVersion(char *file1, char *file2);

/*
** Construction/Destruction
*/

WcPreSetup::WcPreSetup()
: m_ClusterName(_T(""))
, m_GroupName(_T(""))
, m_ResourceName(_T(""))
, m_ServicePassword(_T(""))
, m_hCluster(NULL)
{
    m_CreateResponseFile="0";
    m_EmbeddedRelease="0";
    m_InstallCode="II";
    m_ResponseFile="";
    m_Ver25="0";
    m_MSILog="";

    FindOldInstallations();
}

WcPreSetup::~WcPreSetup()
{
    for(int i=0; i<m_Installations.GetSize(); i++)
    {
		WcInstallation *obj=(WcInstallation *) m_Installations.GetAt(i);

		if(obj)
			delete obj;
    }
    m_Installations.RemoveAll();

    for(int i=0; i<m_ClusterGroups.GetSize(); i++)
    {
		WcClusterGroup *group=(WcClusterGroup *) m_ClusterGroups.GetAt(i);

		if(group)
			delete group;
    }
	m_ClusterGroups.RemoveAll();

	EmptyResourceList();

	EmptyDependencies();

	if (thePreSetup.m_hCluster)
	{
		CloseCluster(thePreSetup.m_hCluster);
		thePreSetup.m_hCluster = NULL;
	}
}

/*
**  Find the existing installations on this computer
*/
void WcPreSetup::FindOldInstallations()
{
    char CurKey[1024], Key[1024];
    HKEY hCurKey=0, hKey=0;
    char ii_code[3], ii_system[MAX_PATH], file2check[MAX_PATH];
    CStringList CodeList;
    CString Host, ConfigKey, ConfigKeyValue, embedded;
    char strBuffer[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwSize=sizeof(strBuffer);
	
    GetComputerName(strBuffer, &dwSize);
    Host=CString(strBuffer);
    Host.MakeLower();

    strcpy(CurKey, "SOFTWARE\\ComputerAssociates\\Ingres");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,CurKey,0,KEY_ENUMERATE_SUB_KEYS,&hCurKey)==ERROR_SUCCESS)
    {
	LONG retCode;
	int i=0;
	
	for (i, retCode = ERROR_SUCCESS; retCode == ERROR_SUCCESS; i++) 
	{
	    char SubKey[16];
	    DWORD dwTemp=sizeof(SubKey);
	    
	    retCode=RegEnumKeyEx(hCurKey,i,SubKey,&dwTemp,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {			
		HKEY hSubKey=0;

		if(RegOpenKeyEx(hCurKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey)== ERROR_SUCCESS)
		{
		    DWORD dwType, dwSize=sizeof(ii_system);
		    
		    if(RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize) == ERROR_SUCCESS)
		    {
				m_II_System = ii_system;
				strncpy(ii_code, SubKey, 2);
				ii_code[2]='\0';

				ConfigKey.Format("ii.%s.setup.embed_installation", Host);
				if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
					&& !ConfigKeyValue.CompareNoCase("ON"))
					embedded="1";
				else
					embedded="0";
				
				sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
				if (GetFileAttributes(file2check)!=-1)
				{
					CodeList.AddTail(ii_code);
					sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
					if (GetFileAttributes(file2check)!=-1)
						AddInstallation(ii_code, ii_system, 1, embedded);
					else
						AddInstallation(ii_code, ii_system, 0, embedded);
				}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hCurKey);
    }

    strcpy(Key, "SOFTWARE\\ComputerAssociates\\IngresII");
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,Key,0,KEY_ENUMERATE_SUB_KEYS,&hKey)==ERROR_SUCCESS)
    {
	LONG retCode;
	int i=0;

	for (i, retCode = ERROR_SUCCESS; retCode == ERROR_SUCCESS; i++) 
	{
	    char SubKey[16];
	    DWORD dwTemp=sizeof(SubKey);
	    
	    retCode=RegEnumKeyEx(hKey,i,SubKey,&dwTemp,NULL,NULL,NULL,NULL);
	    if(!retCode)
	    {
		strncpy(ii_code, SubKey, 2);
		ii_code[2]='\0';
		if(!CodeList.Find(ii_code))
		{
		    HKEY hSubKey=0;

		    if(RegOpenKeyEx(hKey,SubKey,0,KEY_QUERY_VALUE,&hSubKey)== ERROR_SUCCESS)
		    {
			DWORD dwType, dwSize=sizeof(ii_system);
		    
			if(RegQueryValueEx(hSubKey,"II_SYSTEM",NULL,&dwType,(BYTE *)ii_system,&dwSize) == ERROR_SUCCESS)
			{				
			    ConfigKey.Format("ii.%s.setup.embed_installation", Host);
			    if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
			        && !ConfigKeyValue.CompareNoCase("ON"))
				embedded="1";
			    else
				embedded="0";

			    sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
			    if (GetFileAttributes(file2check)!=-1)
			    {
				CodeList.AddTail(ii_code);
				sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
				if(GetFileAttributes(file2check)!=-1)
				    AddInstallation(ii_code, ii_system, 1, embedded);
				else 
				    AddInstallation(ii_code, ii_system, 0, embedded);
				
				sprintf(CurKey, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation", ii_code);
				if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, CurKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hCurKey, NULL) == ERROR_SUCCESS)
				{
				    RegSetValueEx(hCurKey, "II_SYSTEM", 0, REG_SZ, (BYTE *)ii_system, (DWORD)strlen(ii_system)+1);
				    RegCloseKey(hCurKey);
				}
			    }
			}
		    }
		    RegCloseKey(hSubKey);
		}
	    }
	} /* end of for loop */		
	RegCloseKey(hKey);
    }
    
	strcpy(Key, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment");
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, Key, 0,KEY_QUERY_VALUE, &hKey)==ERROR_SUCCESS)
    {
	DWORD dwType, dwSize=sizeof(ii_system);
	
	if (RegQueryValueEx(hKey, "II_SYSTEM", 0, &dwType, (BYTE *)ii_system, &dwSize)==ERROR_SUCCESS)
	{	
	    CString temp;
	    
	    if (Local_NMgtIngAt("II_INSTALLATION", ii_system, temp) && !CodeList.Find(temp))
	    {
		strcpy(ii_code, temp.GetBuffer(2));

		ConfigKey.Format("ii.%s.setup.embed_installation", Host);
		if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
		    && !ConfigKeyValue.CompareNoCase("ON"))
		    embedded="1";
		else
		    embedded="0";

		sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
		if (GetFileAttributes(file2check)!=-1)
		{
		    CodeList.AddTail(ii_code);
		    sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
		    if(GetFileAttributes(file2check)!=-1)
			AddInstallation(ii_code, ii_system, 1, embedded);
		    else
			AddInstallation(ii_code, ii_system, 0, embedded);
		
		    sprintf(CurKey, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation", ii_code);
		    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, CurKey, 0, NULL, REG_OPTION_NON_VOLATILE, 
				   KEY_ALL_ACCESS, NULL, &hCurKey, NULL) == ERROR_SUCCESS)
		    {
			RegSetValueEx(hCurKey, "II_SYSTEM", 0, REG_SZ, (BYTE *)ii_system, (DWORD)strlen(ii_system)+1);
			RegCloseKey(hCurKey);
		    }
		}
	    }
	}
	RegCloseKey(hKey);
    }
    
    if(RegOpenKeyEx(HKEY_CURRENT_USER, "Environment", 0, KEY_QUERY_VALUE, &hKey)==ERROR_SUCCESS)
    {
	DWORD dwType, dwSize=sizeof(ii_system);
	
	if(RegQueryValueEx(hKey, "II_SYSTEM", 0, &dwType, (BYTE *)ii_system, &dwSize) == ERROR_SUCCESS)
	{	
	    CString temp;
	    
	    if (Local_NMgtIngAt("II_INSTALLATION", ii_system, temp) && !CodeList.Find(temp))
	    {
		strcpy(ii_code, temp.GetBuffer(2));

		ConfigKey.Format("ii.%s.setup.embed_installation", Host);
		if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
		    && !ConfigKeyValue.CompareNoCase("ON"))
		    embedded="1";
		else
		    embedded="0";

		sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
		if (GetFileAttributes(file2check)!=-1)
		{
		    CodeList.AddTail(ii_code);
		    sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
		    if(GetFileAttributes(file2check)!=-1)
			AddInstallation(ii_code, ii_system, 1, embedded);
		    else
			AddInstallation(ii_code, ii_system, 0, embedded);
		
		    sprintf(CurKey, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation", ii_code);
		    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, CurKey, 0, NULL, REG_OPTION_NON_VOLATILE, 
				   KEY_ALL_ACCESS, NULL, &hCurKey, NULL) == ERROR_SUCCESS)
		    {
			RegSetValueEx(hCurKey, "II_SYSTEM", 0, REG_SZ, (BYTE *)ii_system, (DWORD)strlen(ii_system)+1);
			RegCloseKey(hCurKey);
		    }
		}
	    }
	}
	RegCloseKey(hKey);
    }
    
    if(GetEnvironmentVariable("II_SYSTEM", ii_system, sizeof(ii_system)))
    {
	CString temp;
	
	if (Local_NMgtIngAt("II_INSTALLATION", ii_system, temp) && !CodeList.Find(temp))
	{
	    strcpy(ii_code, temp.GetBuffer(2));

	    ConfigKey.Format("ii.%s.setup.embed_installation", Host);
	    if (Local_PMget(ConfigKey, ii_system, ConfigKeyValue) 
	        && !ConfigKeyValue.CompareNoCase("ON"))
		embedded="1";
	    else
		embedded="0";

	    sprintf(file2check, "%s\\ingres\\files\\config.dat", ii_system);
	    if (GetFileAttributes(file2check)!=-1)
	    {
		CodeList.AddTail(ii_code);
		sprintf(file2check, "%s\\CAREGLOG.LOG", ii_system);
		if (GetFileAttributes(file2check)!=-1)
		    AddInstallation(ii_code, ii_system, 1, embedded);
		else
		    AddInstallation(ii_code, ii_system, 0, embedded);
		
		sprintf(CurKey, "SOFTWARE\\ComputerAssociates\\Ingres\\%s_Installation", ii_code);
		if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, CurKey, 0, NULL, REG_OPTION_NON_VOLATILE, 
				   KEY_ALL_ACCESS, NULL, &hCurKey, NULL) == ERROR_SUCCESS)
		{
		    RegSetValueEx(hCurKey, "II_SYSTEM", 0, REG_SZ, (BYTE *)ii_system, (DWORD)strlen(ii_system)+1);
		    RegCloseKey(hCurKey);
		}
	    }
	}
    }
}


void WcPreSetup::AddInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded)
{
    WcInstallation *inst=new WcInstallation(id, path, ver25, embedded);
    if (inst)
		m_Installations.Add(inst);
}

void WcPreSetup::AddClusterGroup(LPCWSTR name)
{
    WcClusterGroup *group=new WcClusterGroup(name);
    if (group)
		m_ClusterGroups.Add(group);
}

// Add a resource to the Cluster resource list
void WcPreSetup::AddClusterResource(LPCWSTR name, LPCWSTR type)
{
    WcClusterResource *resource=new WcClusterResource(name, type);
    if (resource)
		m_ClusterResources.Add(resource);
}

// Add a resource to the Cluster resource list
void WcPreSetup::AddClusterResource(LPCTSTR name, LPCTSTR type)
{
    WcClusterResource *resource=new WcClusterResource(name, type);
    if (resource)
		m_ClusterResources.Add(resource);
}

// Remove all resources from the list
void WcPreSetup::EmptyResourceList()
{
    for(int i=0; i<m_ClusterResources.GetSize(); i++)
    {
		WcClusterResource *resource=(WcClusterResource *) m_ClusterResources.GetAt(i);

		if(resource)
			delete resource;
    }
	m_ClusterResources.RemoveAll();
}

// Add a resource to the Ingres resource dependency list
void WcPreSetup::AddDependency(LPCWSTR name, LPCWSTR type)
{
    WcClusterResource *resource=new WcClusterResource(name, type);
    if (resource)
		m_Dependencies.Add(resource);
}

// Add a resource to the Ingres resource dependency list
void WcPreSetup::AddDependency(LPCTSTR name, LPCTSTR type)
{
    WcClusterResource *resource=new WcClusterResource(name, type);
    if (resource)
		m_Dependencies.Add(resource);
}

// Empty the dependency list
void WcPreSetup::EmptyDependencies()
{
    for(int i=0; i<m_Dependencies.GetSize(); i++)
    {
		WcClusterResource *resource=(WcClusterResource *) m_Dependencies.GetAt(i);

		if(resource)
			delete resource;
    }
	m_Dependencies.RemoveAll();
}

/*
**  Name: CompareVersion
**
**  Return Codes:
**	 0 = Files are the same version
**	 1 = File1 is a newer version than file2
**	 2 = File2 is a newer version than file1
**	-1 = Error getting information on file1
**	-2 = Error getting information on file2
**	-3 = Memory allocation error
*/
INT
CompareVersion(char *file1, char *file2 )
{
    DWORD  dwHandle = 0L;	/* Ignored in call to GetFileVersionInfo */
    DWORD  cbBuf1   = 0L, cbBuf2 = 0L;
    LPVOID lpvData1 = NULL, lpvData2 = NULL;
    LPVOID lpValue1 = NULL, lpValue2 = NULL;
    UINT   wBytes1 = 0L, wBytes2 = 0L;
    WORD   wlang1 = 0, wcset1 = 0, wlang2 = 0, wcset2 = 0;
    char   SubBlk1[81], SubBlk2[81];
    INT    rcComp = 0;

    /* Retrieve Size of Version Resource */
    if ((cbBuf1 = GetFileVersionInfoSize(file1, &dwHandle)) == 0)
    {
	rcComp = -1;
	goto QuickExit;
    }
	
    if ((cbBuf2 = GetFileVersionInfoSize(file2, &dwHandle)) == 0)
    {
	rcComp = -2;
 	goto QuickExit;
    }

    lpvData1 = (LPVOID)malloc(cbBuf1);
    lpvData2 = (LPVOID)malloc(cbBuf2);

    if (!lpvData1 && lpvData2)
    {
	rcComp = -3;
	goto QuickExit;
    }

    /* Retrieve Version Resource */
    if (GetFileVersionInfo(file1, dwHandle, cbBuf1, lpvData1) == FALSE)
    {
	rcComp = -1;
 	goto QuickExit;
    }
	
    if (GetFileVersionInfo(file2, dwHandle, cbBuf2, lpvData2) == FALSE)
    {
	rcComp = -2 ;
 	goto QuickExit;
    }

    /* Retrieve the Language and Character Set Codes */
    VerQueryValue(lpvData1, TEXT("\\VarFileInfo\\Translation"), &lpValue1, &wBytes1);
    wlang1 = *((WORD *)lpValue1);
    wcset1 = *(((WORD *)lpValue1) + 1);
	                   
    VerQueryValue(lpvData2, TEXT("\\VarFileInfo\\Translation"), &lpValue2, &wBytes2);
    wlang2 = *((WORD *)lpValue2);
    wcset2 = *(((WORD *)lpValue2) + 1);

    /* Retrieve FileVersion Information */
    sprintf(SubBlk1, "\\StringFileInfo\\%.4x%.4x\\FileVersion", wlang1, wcset1);
    VerQueryValue(lpvData1, TEXT(SubBlk1), &lpValue1, &wBytes1);
    sprintf(SubBlk2, "\\StringFileInfo\\%.4x%.4x\\FileVersion", wlang2, wcset2);
    VerQueryValue(lpvData2, TEXT( SubBlk2), &lpValue2, &wBytes2);

    {
	int majver1=0, minver1=0, relno1=0, majver2=0, minver2=0, relno2=0;

	sscanf((char *)lpValue1, "%d.%d.%d", &majver1, &minver1, &relno1);
	sscanf((char *)lpValue2, "%d.%d.%d", &majver2, &minver2, &relno2);

	/* Check Major Version Number */
	if (majver1 == majver2)
	{
	    /* Check Minor Version Number */
	    if (minver1 == minver2)
	    {
		/* Check Release Number */
		if (relno1 == relno2)
		    rcComp = 0;
		else
		{
		    if (relno1 > relno2)
			rcComp = 1;
		    else
			rcComp = 2;
		}
	    }
	    else
	    {
		if (minver1 > minver2)
		    rcComp = 1;
		else
		    rcComp = 2;
	    }
	}
	else
	{
	    if (majver1 > majver2)
		rcComp = 1;
  	    else
		rcComp = 2;
	}
    }

QuickExit:
    if (lpvData1)
	free(lpvData1);
    if (lpvData2 )
	free(lpvData2);
    return (rcComp);
}

///////////////////////////////////////////////////////////////////////////////
// WcInstallation Class
///////////////////////////////////////////////////////////////////////////////

/*
** Construction/Destruction
*/

WcInstallation::WcInstallation()
{

}

WcInstallation::~WcInstallation()
{

}

WcInstallation::WcInstallation(LPCSTR id, LPCSTR path, BOOL ver25, LPCSTR embedded)
{
    m_id=id;
    m_path=path;
    m_ver25=ver25;
	m_embedded=embedded;
}

///////////////////////////////////////////////////////////////////////////////
// WcClusterGroup Class
///////////////////////////////////////////////////////////////////////////////

WcClusterGroup::WcClusterGroup(LPCWSTR name)
{
	m_name = name;
}

WcClusterGroup::~WcClusterGroup()
{

}

///////////////////////////////////////////////////////////////////////////////
// WcClusterResource Class
///////////////////////////////////////////////////////////////////////////////

WcClusterResource::WcClusterResource(LPCWSTR name, LPCWSTR type)
{
	m_name = name;
	m_type = type;
}

WcClusterResource::WcClusterResource(LPCTSTR name, LPCTSTR type)
{
	m_name = name;
	m_type = type;
}

WcClusterResource::~WcClusterResource()
{

}
