/*
**  Copyright (c) 2006 Ingres Corporation
*/

/*
**  Name: SetupDriver.c: Implementation of the driver program for installers.
**						 The goal of this application is to make functionality of 
**						 setup.exe for documentation and .NET Data Provider 
**						 installers backwards compatable to that of setup.exe
**					     for Ingres cdimage. This module will accept the same
**						 flags as setup.exe for Ingres including silent install
**					     flags.  It will also user response file in silent
**						 install.
**
**  History:
**		16-Nov-2006 (drivi01)
**			Created.
**		19-Jul-2007 (drivi01)
**			.NET Data Provider package will no longer contain .NET 
**			Framework 2.0.  Add routines for directing users to the 
**			Microsoft website to download Framework.
**		31-Jul-2008 (drivi01)
**			.NET Data Provider 2.0 replaced with version 2.1.
**		01-Aug-2008 (drivi01)
**			Clean up warnings.
**
*/

# include <windows.h>
# include <string.h>
# include <stdlib.h>
# include <stdio.h>
#include <clusapi.h>
#include <resapi.h>

HWND		hDlgGlobal;	/* Handle to Main Dialog */




int _access(char *,int);
VOID BoxErr4 (CHAR *, CHAR *);
int count_tokens(char *);

/*
**  Name: WinMain()
**
**  Description:
**	Entry point of the Application.
**  
**	Parameters: hInstance - Handle to the current instance of the application. 
**				hPrevInstance - Handle to the previous instance of the application. 
**								This parameter is always NULL. If you need to detect 
**								whether another instance already exists, create a 
**								uniquely named mutex using the CreateMutex function. 
**								CreateMutex will succeed even if the mutex already 
**								exists, but the function will return ERROR_ALREADY_EXISTS. 
**								This indicates that another instance of your application 
**								exists, because it created the mutex first. 
**				lpCmdLine - Pointer to a null-terminated string specifying the command 
**							line for the application, excluding the program name. To 
**							retrieve the entire command line, use the GetCommandLine function. 
**				nCmdShow - Specifies how the window is to be shown. This parameter can 
**						   be one of the following values. 
**
**  Return Code: 0 - Successful execution
**				 1 - Failure
*/
INT WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					LPSTR lpszCmdLine, INT nCmdShow)
{
int rc = 0;
char *args = lpszCmdLine;  /*Pointer to command line arguments*/
char logBuf[1024]; /* buffers used to storing log messages and command line */
PROCESS_INFORMATION pi; 
STARTUPINFO si;
DWORD	dwError;
int bSilent=0;
char cmd[MAX_PATH+1], pathBuf[MAX_PATH+1], token[MAX_PATH+1], rspFile[MAX_PATH+1], buf[MAX_PATH+1];
size_t pathLen;
char *ptoken = (char *)&token;
char location[MAX_PATH+1];
WIN32_FIND_DATA msifile;
char msifileloc[MAX_PATH+1];
int silent=0;
char msiFile[MAX_PATH+1];
char SubKey[MAX_PATH], SubKey2[MAX_PATH];
HKEY hkSubKey, hkSubKey2;

	GetModuleFileName(NULL,pathBuf,sizeof(pathBuf));
	pathLen=strlen(pathBuf);
	while(pathBuf[pathLen]!='\\')
	{
		pathBuf[pathLen]='\0';
		pathLen--;
	}
	pathBuf[pathLen]='\0';


	ptoken = strtok(lpszCmdLine, " ");
	if (ptoken && strcmp(ptoken, "/r") == 0) //Routines for Silent install from response file
	{
		silent = 1;
		ptoken = strtok(NULL, " ");
		//if (_access(ptoken, 00))
		if (GetFileAttributes(ptoken) == -1)
		{
			sprintf(logBuf, "Couldn't find response file %s.", ptoken);
			if (!silent)
				BoxErr4("Setup Wizard Installation Error:\r\n\r\n", logBuf);
			return 1;
		}
		sprintf(msifileloc, "%s\\*.msi", pathBuf);
		//Looks for msi file in the source directory of this program
		if (FindFirstFile(msifileloc, &msifile) != INVALID_HANDLE_VALUE)
		{
			sprintf(msiFile, "%s\\%s", pathBuf, msifile.cFileName);
			if (strstr(ptoken, "\\")==NULL && strstr(ptoken, "/")==NULL)
			{
				GetCurrentDirectory(sizeof(pathBuf), pathBuf);
				sprintf(rspFile, "%s\\%s", pathBuf, ptoken);
			}
			else 
				sprintf(rspFile, "%s", ptoken);
			if (strstr(msifile.cFileName, "Provider") != 0)
				GetPrivateProfileString("Locations", "II_LOCATION_DOTNET", "", location, sizeof(location), rspFile);
			else
			{
				GetPrivateProfileString("Locations", "II_LOCATION_DOCUMENTATION", "", location, sizeof(location), rspFile);
			}
			dwError = GetLastError();
			sprintf(cmd, "MsiExec.exe /i \"%s\" /qn INSTALLDIR=\"%s\" REINSTALLMODE=vamus", msiFile, location);
		}
		else
		{
			sprintf(logBuf, "Couldn't locate msi file in %s directory.", msifileloc);
			if (!silent)
				BoxErr4("Setup Wizard Installation Error:\r\n\r\n", logBuf);
			return 1;
		}
	}
	else if (ptoken && strcmp(ptoken, "/l") == 0) //routines to print out msi logging information
	{
		char temp[MAX_PATH+1];
		char logfile[MAX_PATH+1];
		if (GetTempPath(sizeof(temp), temp))
		{
			sprintf(logfile, "%s\\ingresmsi.log", temp);
			sprintf(msifileloc, "%s\\*.msi", pathBuf);
			if (FindFirstFile(msifileloc, &msifile) != INVALID_HANDLE_VALUE)
			{
				sprintf(msiFile, "%s\\%s", pathBuf, msifile.cFileName);
				sprintf(cmd, "MsiExec.exe /i \"%s\" /l*v \"%s\"", msiFile, logfile);
			}
			else
			{
				sprintf(logBuf, "Couldn't locate msi file in %s directory.", msifileloc);
				if (!silent)
					BoxErr4("Setup Wizard Installation Error:\r\n\r\n", logBuf);
				return 1;
			}
		}
		else
		{
			sprintf(logBuf, "Couldn't locate temporary directory to write logging information. System Error (%d)", GetLastError());
			if (!silent)
				BoxErr4("Setup Wizard Installation Error:\r\n\r\n", logBuf);
			return 1;
		}
	}
	else if (ptoken && strcmp(ptoken, "/?") == 0) //Usage details
	{
		BoxErr4("Setup Wizard Usage:\r\n\r\nsetup.exe /r <response file> -- To run in unattended mode using a response file.\r\nsetup.exe /l -- To generate log file in %TEMP%\\ingresmsi.log.", "");
		return 0;
	}
	else //Routines for interactive install
	{
		sprintf(msifileloc, "%s\\*.msi", pathBuf);
		//searches for msi file in the root directory of this program
		if (FindFirstFile(msifileloc, &msifile) != INVALID_HANDLE_VALUE)
		{
			sprintf(msiFile, "%s\\%s", pathBuf, msifile.cFileName);
			if (strstr(msifile.cFileName, "Provider")!= NULL)
			{
				//These routines will prompt user to install .NET Framework 2.0 if this driver is used to
				//run Ingres .NET Data Provider 2.1 install.
				sprintf(SubKey, "SOFTWARE\\Microsoft\\.NETFramework\\policy\\v2.0");
				sprintf(SubKey2, "SOFTWARE\\Wow6432Node\\Microsoft\\.NETFramework\\policy\\v2.0");
				if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey, 0, KEY_QUERY_VALUE, &hkSubKey) && RegOpenKeyEx(HKEY_LOCAL_MACHINE, SubKey2, 0, KEY_QUERY_VALUE, &hkSubKey2))
				{
					sprintf(buf, "The Ingres .NET Data Provider component requires Microsoft .NET Framework 2.0 which could not be found on your machine.\r\nTo obtain the Microsoft .NET Framework 2.0 Package from the Microsoft website click OK, else click Cancel to exit.\r\n");
					if (MessageBox(NULL, buf, "Framework Requirement", MB_OKCANCEL|MB_ICONQUESTION)==IDOK)
					{
						SHELLEXECUTEINFO shex;

						memset(&shex, 0, sizeof(shex));
	
						shex.cbSize	= sizeof(SHELLEXECUTEINFO);
						shex.hwnd	= NULL;
						shex.lpVerb	= "open";
						shex.lpFile	= "http:\\\\www.microsoft.com\\downloads";
						shex.nShow	= SW_NORMAL;
	
						ShellExecuteEx(&shex);
						return 1;
						
						/*sprintf(cmd, "\"%s\\dotnetfx20\\install.exe\" /qb", pathBuf);
						memset(&si, 0, sizeof(si));
						memset(&pi, 0, sizeof(pi));
						if(!CreateProcess(NULL,cmd,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi))
    					{
							dwError=GetLastError();
							sprintf(logBuf, "Error returned by Operating System: (%d).", dwError);
							if (!bSilent)
								BoxErr4("Error while trying to start .NET Framework installer.\r\n", logBuf);
							return 1;
    					}
						else
						{
							WaitForSingleObject(pi.hProcess, INFINITE);
							if (GetExitCodeProcess(pi.hProcess, &dwError)) 
							return_code=ERROR_SUCCESS;
							else 
							return_code=GetLastError();
							if (dwError)
							{
								CloseHandle(pi.hProcess);
								CloseHandle(pi.hThread);
								sprintf(logBuf, "Failed to install Microsoft .NET Framework!\r\nError returned by Operating System: (%d).", dwError);
								if (!bSilent)
								BoxErr4("Error installing .NET Framework 2.0 file.\r\n", logBuf);
								return 1;
							}
						}*/
					}

					else
					{
						*logBuf = 0;
						BoxErr4("You must install Microsoft .NET Framework 2.0 before installing Ingres .NET Data Provider.\r\n", logBuf);
						return 1;
					}
				}
				if (hkSubKey)
					RegCloseKey(hkSubKey);
				if (hkSubKey2)
					RegCloseKey(hkSubKey2);

			}
			sprintf(cmd, "MsiExec.exe /i \"%s\" ", msiFile);
		}
		else
		{
			sprintf(logBuf, "Couldn't locate msi file in %s directory.", msifileloc);
			if (!silent)
				BoxErr4("Setup Wizard Installation Error:\r\n\r\n", logBuf);
			return 1;
		}
	}
		

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	if(!CreateProcess(NULL,cmd,NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,NULL,NULL,&si,&pi))
    {
		dwError=GetLastError();
		sprintf(logBuf, "Error returned by Operating System: (%d).", dwError);
		if (!bSilent)
			BoxErr4("Error initiating demo setup.\r\n", logBuf);
		return 1;
    }


    return 0;
}


/*
**  Name: count_tokens
**  
**  Description: Counts tokens in a string.  Tokens are considered
**				 any string separated by tab, new line, carriage return
**               or space.
**
**	Parameters:  str - String to be parsed for tokens.
**
**  Return Code: Number of tokens found in a string parameter. 
*/
int
count_tokens(char *str)
{
	char *token;
	int	counter=0;
	char	sep[] = " \t\r\n";
	token = strtok(str, sep);
	while (token != NULL)
	{
		counter++;
		token = strtok(NULL, sep);
	}

	return counter;
}


/*
**  Name: BoxErr4
**
**  Description:
**	Quick little function for printing messages to the screen, with
**	the stop sign. Takes two strings as it's input.
**
**	Parameter: MessageString - String 1.
**             MessageString2 - String2.
**
*/
VOID
BoxErr4 (CHAR *MessageString, CHAR *MessageString2)
{
    CHAR	MessageOut[1024];

    sprintf(MessageOut,"%s%s",MessageString, MessageString2);
    MessageBox( hDlgGlobal, MessageOut, "Setup Application", MB_APPLMODAL | MB_ICONSTOP | MB_OK);
}