;/*{
;** Name:
;**	pfctrmsg.h (derived from pfctrmsg.mc by the message compiler)
;**
;** Description:
;**	Event message definitions used by routines in iipfctrs.dll
;**	
;** History:
;** 	15-oct-1998 (wonst02)
;** 	    Created.
;** 	10-jan-1999 (wonst02)
;** 	    Added more messages.
;** 	16-aug-1999 (somsa01)
;**	    Added messages PFCTRS_UNABLE_READ_LAST_COUNTER and
;**	    PFAPI_EXEC_PROCEDURE_ERROR.
;*/
;/* */
;#ifndef _PFCTRMSG_H_
;# define _PFCTRMSG_H_
;/* */
MessageIdTypedef=DWORD
;/*
;**     pfutil messages    (51712 = x'CA00')
;*/
MessageId=51712
Severity=Informational
Facility=Application
SymbolicName=PFUTIL_LOG_OPEN
Language=English
An extensible counter has opened the Event Log for iipfctrs.dll
.
;/* */
MessageId=+1
Severity=Informational
Facility=Application
SymbolicName=PFUTIL_CLOSING_LOG
Language=English
An extensible counter has closed the Event Log for iipfctrs.dll
.
;/*
;**     pfctrs messages    (51728 = x'CA10')
;*/
MessageId=51728
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_OPEN_DRIVER_KEY
Language=English
Unable to open "Performance" key of oiPfCtrs driver in registry (%1). Status code is returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_READ_FIRST_COUNTER
Language=English
Unable to read the "First Counter" value under the oiPfCtrs\Performance Key. Status codes returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_READ_LAST_COUNTER
Language=English
Unable to read the "Last Counter" value under the oiPfCtrs\Performance Key. Status codes returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_READ_FIRST_HELP
Language=English
Unable to read the "First Help" value under the oiPfCtrs\Performance Key. Status codes returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_TO_GET_MEMORY
Language=English
Unable to allocate memory from MEreqmem(). Status codes returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_TO_INIT_API
Language=English
Unable to initialize the API to the Ingres database server. Status codes returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_TO_TERM_API
Language=English
Error during terminate of the API to the Ingres database server. Status codes returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_ERROR_LOADING_DLL
Language=English
Error loading module. Status code of DLprepare() returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_TO_LOAD_DLL
Language=English
Unable to load module %1. Status code of DLprepare() returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_ERROR_BINDING_DLL
Language=English
Error finding function in DLL. Status code of DLbind() returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_TO_BIND_DLL
Language=English
Error finding function %1 in DLL. Status code of DLbind() returned in data.
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFCTRS_UNABLE_TO_UNLOAD_DLL
Language=English
Error unloading module. Status code of DLunload() returned in data. %1
.
;/*
;**     pfapi messages    (51744 = x'CA20')
;** 
;** 		In the following messages, the %1 insertion string 
;** 		will contain the Ingres SQL error text.
;*/
MessageId=51744
Severity=Error
Facility=Application
SymbolicName=PFAPI_UNABLE_TO_CONNECT
Language=English
Unable to connect to the Ingres database server. Status codes returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFAPI_UNABLE_TO_DISCONNECT
Language=English
Error during disconnect from the Ingres database server. Status codes returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFAPI_UNABLE_TO_CLOSE_STMT
Language=English
Unable to close the statement handle. Status codes returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFAPI_UNABLE_TO_COMMIT
Language=English
Unable to do a database server commit. Status codes returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFAPI_UNABLE_TO_ROLLBACK
Language=English
Unable to do a database server rollback. Status codes returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFAPI_SELECT_ERROR
Language=English
Error doing an Ingres Select statement. Status codes returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFAPI_REPEATED_SELECT_ERROR
Language=English
Error doing an Ingres Repeated Select statement. Status codes returned in data. %1
.
;/* */
MessageId=+1
Severity=Error
Facility=Application
SymbolicName=PFAPI_EXEC_PROCEDURE_ERROR
Language=English
Error executing an Ingres Procedure. Status codes returned in data. %1
.
;/* */
;#endif /* _PFCTRMSG_H_ */
