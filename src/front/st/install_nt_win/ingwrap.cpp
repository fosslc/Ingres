/*
**  Copyright (c) 2000, 2001 Ingres Corporation
*/

#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

BOOL IsWindows95();
BOOL Execute(LPCSTR, char *, char *);
void SetOneEnvVar(LPCSTR, LPCSTR);
void SetEnvVars(LPCSTR);
int  cmd_show;

/*
**  Name: ingwrap.cpp
**
**  Description:
**	input example:	"%II_SYSTEM%\\ingres\\bin\\winstart.exe /stop"
**	
**  History:
**	15-nov-2000 (penga03)
**	    Created.
**	05-apr-2001 (penga03)
**	    Ingwrap exits if no arguments passed.
**	25-apr-2001 (mcgem01)
**	    Only use write for the errlog.log file on '95 use notepad on NT
**      27-jan-2004 (rodjo04) INGCBT508 (bug 111694)
**          When looking for II_SYSTEM from the command line, if we should 
**          find the 'ingres' token, then we should exit the loop.     
**	02-Feb-2007 (drivi01)
**	    Modify Execute function to use ShellExecute instead of CreateProcedure
**	    which is recommended for Vista.  Reworked routines to take out absolete
**	    code and added new code to allow for wrapping of cmd.exe.
**	14-Mar-2007 (drivi01)
**	    Don't put quotes around parameters that are flags, s.a. -loop or /K.
**	    Don't process spaces as parameters.
**	  
*/

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	char *token;
	char *tokens[256];
	char string[1024];
	char ii_system[1024];
 	int count=0, flag=0;
	int i,j;
	char seps[] = "\\";
	int argc=0;
	char *argv[256];
	char *arg;
	char cmd[1024];
	char tbuf[4096];
	char file[MAX_PATH];
	char *params=NULL;
	BOOL win95=IsWindows95();
	int len, argv_len = 0;

	if (!strcmp(lpCmdLine, ""))
		return 1;

	nCmdShow=SW_HIDE;
	memset((char*)&cmd,0,sizeof(cmd));
	strncpy(cmd, lpCmdLine, strlen(lpCmdLine));
	arg=strtok(cmd,"\"");
	while(arg!=NULL)
	{
		argc++;
 		argv[argc]=arg;
		arg=strtok(NULL,"\"");
	}

	/* Grab %II_SYSTEM% from argv[1] if there is one */
	strcpy(ii_system, "\0");
	strcpy(string,argv[1]);
	token = strtok( string, seps );
	while( token != NULL )
    	{
		tokens[count]=token;
		token = strtok( NULL, seps );
		count++;
	}
	for(i=count-1; i>=0 && count > 1 && i<sizeof(tokens); i--)
	{
		if(strcmp(tokens[i], "ingres")==0)
		{
			flag=1; /* there is %II_SYSTEM% in argv[1] */
			len = sizeof(ii_system);
			for(j=0; j<=i-2; ++j)
			{
				if (len > (sizeof(tokens[j])+ sizeof(seps) +1))
				{
				strcat(ii_system,tokens[j]);
				strcat(ii_system,seps);
				len = len - sizeof(tokens[j]) - sizeof(seps);
				}
			}
			if (len > sizeof(tokens[i-1]))
				strcat(ii_system, tokens[i-1]);
			break;
		}
	}

	/* Set environment variable II_SYSTEM, PATH, LIB, INCLUDE */
	if(flag)
		SetEnvVars((LPCSTR)ii_system);
	if(strstr(argv[1], "inguninst"))
		SetEnvVars(argv[2]);

	/* Create a process to run argv[1] */	
	strcpy(tbuf, "");

	/*separate file from directory of the binary being executed*/
	i=0;
	len=sizeof(tbuf);
	while(i < (count-1))
	{
		if (len > sizeof(tokens[i]+sizeof(seps) +1))
		{
		strcat(tbuf, tokens[i]);
		strcat(tbuf, seps);
		i++;
		len = len - sizeof(tokens[i]) - sizeof(seps);
		}
	}
	if (count>=1)
		strcpy(file, tokens[count-1]);

	/*handle any parameters being passed to binary executed*/
	len=0;
	if (argc > 1)
	{
		char *ptr;
		int space = 0;
		flag = 0;
		for (i=2; i<=argc; i++)
			len+=strlen(argv[i])+3;  //2 for quote on each side of string and a space
		params = (char *)calloc(++len, sizeof(char));
		for (i=2; i<=argc; i++)
		{
			argv_len = strlen(argv[i]);
			ptr = argv[i];
			while (ptr != '\0')
			{
				if (*ptr == '/' || *ptr == '-')
					flag = 1;
				else if(*ptr == ' ' && strlen(ptr) == 1)
					space = 1;
				else if (isalnum(*ptr))
					break;
				ptr++;
			
			}
			if (len > argv_len+3 && !space)
			{
			if (!flag)
			strcat(params, "\"");
			strcat(params, argv[i]);
			if (!flag)
			strcat(params, "\" ");
			else
			strcat(params, " ");
			len = len - argv_len-3;
			flag = 0;
			}
			space = 0;
		}
	}

	if(!Execute(tbuf, file, params))
	{
		if (params)
		{
			free(params);
			params=NULL;
		}
		return 1;
	}

	if (params)
	{
		free(params);
		params=NULL;
	}

	return 0;
}

/*
**  Name: IsWindows95()
**
**  Description:
**
**  History:
**	15-nov-2000 (penga03)
**		Created.	
*/

BOOL IsWindows95()
{
	OSVERSIONINFO osver; 
	memset((char *) &osver,0,sizeof(osver)); 
	osver.dwOSVersionInfoSize=sizeof(osver); 
	GetVersionEx(&osver);
	
	if	((osver.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS) && 
		(osver.dwMajorVersion>3))
	{
		return TRUE;
	}
	return FALSE;
}

/*
**  Name: Execute(LPCSTR)
**
**  Description:
**
**  History:
**	15-nov-2000 (penga03)
**	    Created.
**	27-nov-2001 (penga03)
**	    Removed the creation flag DETACHED_PROCESS, since it is only 
**	    for console processes.
**	    The parent process has to wait until the child process has 
**	    finished its initialization.
*/

BOOL Execute(LPCSTR lpCmdLine, char *file, char *parms)
{
	DWORD dwRet;
	SHELLEXECUTEINFO shex;

    memset( &shex, 0, sizeof( shex) );

    shex.cbSize        = sizeof( SHELLEXECUTEINFO ); 
    shex.fMask        = SEE_MASK_NOCLOSEPROCESS; 
    shex.hwnd        = NULL;
    shex.lpVerb        = NULL; 
    shex.lpFile        = file; 
    shex.lpParameters    = parms; 
    shex.lpDirectory    = lpCmdLine; 
    shex.nShow        = SW_NORMAL; 
 

	if ( !ShellExecuteEx( &shex ) )
		return FALSE;

	if (shex.hProcess != NULL)
	{
	    dwRet = WaitForInputIdle(shex.hProcess, 1000);
	    while (dwRet==WAIT_TIMEOUT)
			dwRet=WaitForInputIdle(shex.hProcess, 1000);

	    CloseHandle(shex.hProcess);
	}
	
	return TRUE;
}

/*
**  Name: SetOneEnvVar(LPCSTR, LPCSTR)
**
**  Description:
**
**  History:
**	15-nov-2000 (penga03)
**		Created.
*/

void SetOneEnvVar(LPCSTR lpEnv,LPCSTR addValue)
{
	char temp='\0';
	int nret;
	char *oldEnv=NULL;
	char *buf=NULL;

	nret=GetEnvironmentVariable(lpEnv,&temp,1);
	oldEnv=(char *)malloc(nret+1);
	if (oldEnv)
		GetEnvironmentVariable(lpEnv, oldEnv, nret+1);
	buf=(char *)malloc(nret+strlen(addValue)+1);
	if (buf != NULL && oldEnv != NULL)
	{
		strcpy(buf,addValue);
		if(nret>0)
		{
		strcat(buf,";");
		strcat(buf, oldEnv);
		}
		SetEnvironmentVariable(lpEnv,buf);
	}
	if (oldEnv)
		free(oldEnv);
	if (buf)
		free(buf);
}

/*
**  Name: SetEnvVars(LPCSTR)
**
**  Description:
**
**  History:
**	15-nov-2000 (penga03)
**	    Created.
*/

void SetEnvVars(LPCSTR lpValue)
{	
	char tbuf[MAX_PATH];
		
	SetEnvironmentVariable("II_SYSTEM",NULL);
	SetEnvironmentVariable("II_SYSTEM",lpValue);
	
	sprintf(tbuf,"%s\\ingres\\bin;%s\\ingres\\utility;%s\\ingres\\vdba",lpValue,lpValue,lpValue);
	SetOneEnvVar("PATH", tbuf);
	
	sprintf(tbuf, "%s\\ingres\\lib",lpValue);
	SetOneEnvVar("LIB",tbuf);
	
	sprintf(tbuf,"%s\\ingres\\files",lpValue);
	SetOneEnvVar("INCLUDE",tbuf);	
}
