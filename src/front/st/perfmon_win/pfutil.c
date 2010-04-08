/*
** Copyright (c) 1998 Ingres Corporation
*/
        
# include <windows.h>
# include <string.h>
# include <winperf.h>
# include "pfdata.h"
# include "pfctrmsg.h"	/* error messages */
# include "pfmsg.h"	/* error message macros and definitions */
# include "pfutil.h"

/*{
** Name: 
**	pfutil.c - Utilities for the performance DLL
**
** Description:
**	This file implements the utility routines used to construct the
**	common parts of a PERF_INSTANCE_DEFINITION (see winperf.h) and
**	perform event logging functions.
**
** History:
** 	15-oct-1998 (wonst02)
** 	    Created.
** 
*/

/*
** Global data definitions.
*/

ULONG ulInfoBufferSize = 0;

HANDLE hEventLog = NULL;      /* event log handle for reporting events */
                              /* initialized in Open... routines */
DWORD  dwLogUsers = 0;        /* count of functions using event log */

DWORD MESSAGE_LEVEL = 0;

WCHAR GLOBAL_STRING[] = L"Global";
WCHAR FOREIGN_STRING[] = L"Foreign";
WCHAR COSTLY_STRING[] = L"Costly";

WCHAR NULL_STRING[] = L"\0";    /* pointer to null string  */

/* test for delimiter, end of line and non-digit characters
** used by pfIsNumberInUnicodeList routine
*/
# define DIGIT       1
# define DELIMITER   2
# define INVALID     3

# define EvalThisChar(c,d) ( \
     (c == d) ? DELIMITER : \
     (c == 0) ? DELIMITER : \
     (c < (WCHAR)'0') ? INVALID : \
     (c > (WCHAR)'9') ? INVALID : \
     DIGIT)


HANDLE
pfOpenEventLog (
)
/*++

Routine Description:

    Reads the level of event logging from the registry and opens the
        channel to the event logger for subsequent event log entries.

Arguments:

      None

Return Value:

    Handle to the event log for reporting events.
    NULL if open not successful.

--*/
{
    HKEY hAppKey;
    TCHAR LogLevelKeyName[] = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
    TCHAR LogLevelValueName[] = "EventLogLevel";

    LONG lStatus;

    DWORD dwLogLevel;
    DWORD dwValueType;
    DWORD dwValueSize;
   
    /* if global value of the logging level not initialized or is disabled, 
    **  check the registry to see if it should be updated. */

    if (!MESSAGE_LEVEL) {

       lStatus = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                               LogLevelKeyName,
                               0,                         
                               KEY_READ,
                               &hAppKey);

       dwValueSize = sizeof (dwLogLevel);

       if (lStatus == ERROR_SUCCESS) {
            lStatus = RegQueryValueEx (hAppKey,
                               LogLevelValueName,
                               (LPDWORD)NULL,           
                               &dwValueType,
                               (LPBYTE)&dwLogLevel,
                               &dwValueSize);

            if (lStatus == ERROR_SUCCESS) {
               MESSAGE_LEVEL = dwLogLevel;
            } else {
               MESSAGE_LEVEL = MESSAGE_LEVEL_DEFAULT;
            }
            RegCloseKey (hAppKey);
       } else {
         MESSAGE_LEVEL = MESSAGE_LEVEL_DEFAULT;
       }
    }
       
    if (hEventLog == NULL){
         hEventLog = RegisterEventSource (
            (LPTSTR)NULL,            /* Use Local Machine */
            APP_NAME);               /* event log app name to find in registry */

         if (hEventLog != NULL) {
            REPORT_INFORMATION (PFUTIL_LOG_OPEN, LOG_DEBUG);
         }
    }
    
    if (hEventLog != NULL) {
         dwLogUsers++;           /* increment count of perfctr log users */
    }
    return (hEventLog);
}


VOID
pfCloseEventLog (
)
/*++

Routine Description:

      Closes the handle to the event logger if this is the last caller
      
Arguments:

      None

Return Value:

      None

--*/
{
    if (hEventLog != NULL) {
        dwLogUsers--;         /* decrement usage */
        if (dwLogUsers <= 0) {    /* and if we're the last, then close up log */
            REPORT_INFORMATION (PFUTIL_CLOSING_LOG, LOG_DEBUG);
            DeregisterEventSource (hEventLog);
        }
    }
}

DWORD
pfGetQueryType (
    IN LPWSTR lpValue
)
/*++

GetQueryType

    returns the type of query described in the lpValue string so that
    the appropriate processing method may be used

Arguments

    IN lpValue
        string passed to PerfRegQuery Value for processing

Return Value

    QUERY_GLOBAL
        if lpValue == 0 (null pointer)
           lpValue == pointer to Null string
           lpValue == pointer to "Global" string

    QUERY_FOREIGN
        if lpValue == pointer to "Foreign" string

    QUERY_COSTLY
        if lpValue == pointer to "Costly" string

    otherwise:

    QUERY_ITEMS

--*/
{
    WCHAR   *pwcArgChar, *pwcTypeChar;
    BOOL    bFound;

    if (lpValue == 0) {
        return QUERY_GLOBAL;
    } else if (*lpValue == 0) {
        return QUERY_GLOBAL;
    }

    /* check for "Global" request */

    pwcArgChar = lpValue;
    pwcTypeChar = GLOBAL_STRING;
    bFound = TRUE;  /* assume found until contradicted */

    /* check to the length of the shortest string */
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; /* no match */
            break;          /* bail out now */
        }
    }

    if (bFound) return QUERY_GLOBAL;

    /* check for "Foreign" request */
    
    pwcArgChar = lpValue;
    pwcTypeChar = FOREIGN_STRING;
    bFound = TRUE;  /* assume found until contradicted */

    /* check to the length of the shortest string */
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; /* no match */
            break;          /* bail out now */
        }
    }

    if (bFound) return QUERY_FOREIGN;

    /* check for "Costly" request */
    
    pwcArgChar = lpValue;
    pwcTypeChar = COSTLY_STRING;
    bFound = TRUE;  /* assume found until contradicted */

    /* check to the length of the shortest string */
    
    while ((*pwcArgChar != 0) && (*pwcTypeChar != 0)) {
        if (*pwcArgChar++ != *pwcTypeChar++) {
            bFound = FALSE; /* no match */
            break;          /* bail out now */
        }
    }

    if (bFound) return QUERY_COSTLY;

    /* if not Global and not Foreign and not Costly,  */
    /* then it must be an item list */
    
    return QUERY_ITEMS;

}

BOOL
pfIsNumberInUnicodeList (
    IN DWORD   dwNumber,
    IN LPWSTR  lpwszUnicodeList
)
/*++

IsNumberInUnicodeList

Arguments:
        
    IN dwNumber
        DWORD number to find in list

    IN lpwszUnicodeList
        Null terminated, Space delimited list of decimal numbers

Return Value:

    TRUE:
            dwNumber was found in the list of unicode number strings

    FALSE:
            dwNumber was not found in the list.

--*/
{
    DWORD   dwThisNumber;
    WCHAR   *pwcThisChar;
    BOOL    bValidNumber;
    BOOL    bNewItem;
    WCHAR   wcDelimiter;    /* could be an argument to be more flexible */

    if (lpwszUnicodeList == 0) return FALSE;    /* null pointer, # not found */

    pwcThisChar = lpwszUnicodeList;
    dwThisNumber = 0;
    wcDelimiter = (WCHAR)' ';
    bValidNumber = FALSE;
    bNewItem = TRUE;
    
    while (TRUE) {
        switch (EvalThisChar (*pwcThisChar, wcDelimiter)) {
            case DIGIT:
                /* if this is the first digit after a delimiter, then  */
                /* set flags to start computing the new number */
                if (bNewItem) {
                    bNewItem = FALSE;
                    bValidNumber = TRUE;
                }
                if (bValidNumber) {
                    dwThisNumber *= 10;
                    dwThisNumber += (*pwcThisChar - (WCHAR)'0');
                }
                break;
            
            case DELIMITER:
                /* a delimiter is either the delimiter character or the 
                ** end of the string ('\0') if when the delimiter has been
                ** reached a valid number was found, then compare it to the
                ** number from the argument list. if this is the end of the
                ** string and no match was found, then return.
                */
                if (bValidNumber) {
                    if (dwThisNumber == dwNumber) return TRUE;
                    bValidNumber = FALSE;
                }
                if (*pwcThisChar == 0) {
                    return FALSE;
                } else {
                    bNewItem = TRUE;
                    dwThisNumber = 0;
                }
                break;

            case INVALID:
                /* if an invalid character was encountered, ignore all
                ** characters up to the next delimiter and then start fresh.
                ** the invalid number is not compared. */
                bValidNumber = FALSE;
                break;

            default:
                break;

        }
        pwcThisChar++;
    }
    return FALSE;
}   /* pfIsNumberInUnicodeList */


/****************************************************************************
**
** Name: pfBuildInstanceDefinition  -   Build an instance of an object
**
** Description:
**
** Inputs:

            pBuffer         -   pointer to buffer where instance is to
                                be constructed

            pBufferNext     -   pointer to a pointer which will contain
                                next available location, DWORD aligned

            ParentObjectTitleIndex
                            -   Title Index of parent object type; 0 if
                                no parent object

            ParentObjectInstance
                            -   Index into instances of parent object
                                type, starting at 0, for this instances
                                parent object instance

            UniqueID        -   a unique identifier which should be used
                                instead of the Name for identifying
                                this instance

            Name            -   Name of this instance
**
** Outputs:
**	
** Returns:
** 	
** Exceptions:
** 
** Side Effects:
**
** History:
**	10-Oct-1998 (wonst02)
**	    Original.
**
****************************************************************************/
BOOL
pfBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION *pBuffer,
    PVOID *pBufferNext,
    DWORD ParentObjectTitleIndex,
    DWORD ParentObjectInstance,
    DWORD UniqueID,
    LPCWSTR Name
    )
{
    DWORD NameLength;
    LPWSTR pName;
    /*
    **  Include trailing null in name size
    */

    NameLength = (lstrlenW(Name) + 1) * sizeof(WCHAR);

    pBuffer->ByteLength = sizeof(PERF_INSTANCE_DEFINITION) +
                          DWORD_MULTIPLE(NameLength);

    pBuffer->ParentObjectTitleIndex = ParentObjectTitleIndex;
    pBuffer->ParentObjectInstance = ParentObjectInstance;
    pBuffer->UniqueID = UniqueID;
    pBuffer->NameOffset = sizeof(PERF_INSTANCE_DEFINITION);
    pBuffer->NameLength = NameLength;

    /* copy name to name buffer */
    pName = (LPWSTR)&pBuffer[1];
    RtlMoveMemory(pName,Name,NameLength);

    /* update "next byte" pointer */
    *pBufferNext = (PVOID) ((PCHAR) pBuffer + pBuffer->ByteLength);

    return 0;
}

/*{
** Name: pf_load_dll	Load the application DLL and get its function addresses
**
** Description:
**      Load the application DLL and find its functions: AS_Initiate,
** 	AS_ExecuteMethod, AS_Terminate, etc.
**
** Inputs:
**      ursm				The URS Manager Control Block
**
** Outputs:
**
**	Returns:
** 	    E_DB_OK			DLL loaded OK
** 	    E_DB_ERROR			Error loading DLL
** 					(ursb_error field has more info)
**
**	Exceptions:
**
** Side Effects:
**	    none
**
** History:
** 	28-Jan-1999 (wonst02)
** 	    Original (cloned from ursappl.c)
*/
//BOOL
//pf_load_dll(PFUTIL_MGR_CB	*ursm,
//             FAS_APPL	*appl,
//		URSB		*ursb)
//{
//	CL_ERR_DESC	err ZERO_FILL;
//	int		method;
//	char		appl_name[sizeof appl->fas_appl_name + 1];
//	char		*name = appl->fas_appl_name.db_name;
//	char		*nameend;
//	STATUS		s = OK;
//	DB_STATUS	status = E_DB_OK;

//	/* load the application DLL */
//	nameend = STindex(name, ERx(" "), sizeof appl->fas_appl_name.db_name);
//	if (nameend == NULL)
//		nameend = name + sizeof appl->fas_appl_name.db_name;
//	STlcopy(name, appl_name, nameend - name);
//    	if ( (s = DLprepare_loc((char *)NULL, appl_name,
//			    Pf_AS_names, &appl->fas_appl_loc, 0,
//			    &appl->fas_appl_handle, &err)) != OK )
//    	{
//	    pf_error(E_UR0372_DLL_LOAD, PFUTIL_INTERR, 2,
//	    	      sizeof(s), &s,
//		      STlength(appl->fas_appl_loc.string), 
//		      appl->fas_appl_loc.string);
//	    SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_ERROR)
//	    return E_DB_ERROR;
//	}

//	if (appl->fas_appl_type != FAS_APPL_OPENROAD)
//	{
//	    return E_DB_OK;
//	}

//	/*
//	** Get the addresses of OpenRoad application entry points
//	*/
//	for (method = 0; Pf_AS_names[method] != NULL; method++)
//	{
//    	    if (s = DLbind(appl->fas_appl_handle, Pf_AS_names[method],
//			&appl->fas_appl_addr[method], &err) != OK)
//    	    {
//		/* function isn't there -- cleanup dll */
//	    	pf_error(E_UR0373_DLL_BIND, PFUTIL_INTERR, 3,
//	    	    	  sizeof(s), &s,
//			  STlength(Pf_AS_names[method]), Pf_AS_names[method],
//			  STlength(appl->fas_appl_loc.string), 
//			  appl->fas_appl_loc.string);
//		(void)pf_free_dll(ursm, appl, ursb);
//	    	SET_URSB_ERROR(ursb, E_UR0100_APPL_STARTUP, E_DB_ERROR)
//		return E_DB_ERROR;
//    	    }
//	}
//	return status;
//}

