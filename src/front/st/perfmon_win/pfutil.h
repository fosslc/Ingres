/*
** Copyright (c) 1998 Ingres Corporation
*/
/*


Module Name:


Abstract:

    This file supports routines used to parse and
    create Performance Monitor Data Structures.
    It actually supports Performance Object types with
    multiple instances

Author:


Revision History:


--*/
/*{
** Name: 
**
** Description:
**
** History:
** 	15-oct-1998 (wonst02)
** 	    Created.
** 
*/
#ifndef _PFUTIL_H_
# define _PFUTIL_H_

/* enable this define to log process heap data to the event log */
#ifdef PROBE_HEAP_USAGE
#undef PROBE_HEAP_USAGE
#endif
/*
**  Utility macro.  This is used to reserve a DWORD multiple of
**  bytes for Unicode strings embedded in the definitional data,
**  viz., object instance names.
*/
# define DWORD_MULTIPLE(x) (((x+sizeof(DWORD)-1)/sizeof(DWORD))*sizeof(DWORD))

/*    (assumes dword is 4 bytes long and pointer is a dword in size) */
# define ALIGN_ON_DWORD(x) ((VOID *)( ((DWORD) x & 0x00000003) ? ( ((DWORD) x & 0xFFFFFFFC) + 4 ) : ( (DWORD) x ) ))

extern WCHAR  GLOBAL_STRING[];      /* Global command (get all local ctrs) */
extern WCHAR  FOREIGN_STRING[];           /* get data from foreign computers */
extern WCHAR  COSTLY_STRING[];      
extern WCHAR  NULL_STRING[];

# define QUERY_GLOBAL    1
# define QUERY_ITEMS     2
# define QUERY_FOREIGN   3
# define QUERY_COSTLY    4

/*
** The definition of the routines of pfutil.c, It builds part of a 
** performance data instance (PERF_INSTANCE_DEFINITION) as described in 
** winperf.h
*/

HANDLE pfOpenEventLog ();
VOID pfCloseEventLog ();
DWORD pfGetQueryType (IN LPWSTR);
BOOL pfIsNumberInUnicodeList (DWORD, LPWSTR);

BOOL
pfBuildInstanceDefinition(
    PERF_INSTANCE_DEFINITION *pBuffer,
    PVOID *pBufferNext,
    DWORD ParentObjectTitleIndex,
    DWORD ParentObjectInstance,
    DWORD UniqueID,
    LPCWSTR Name
    );

typedef struct _LOCAL_HEAP_INFO_BLOCK {
    DWORD   AllocatedEntries;
    DWORD   AllocatedBytes;
    DWORD   FreeEntries;
    DWORD   FreeBytes;
} LOCAL_HEAP_INFO, *PLOCAL_HEAP_INFO;


/*
**  Memory Probe macro
*/
#ifdef PROBE_HEAP_USAGE

# define HEAP_PROBE()    { \
    DWORD   dwHeapStatus[5]; \
    NTSTATUS CallStatus; \
    dwHeapStatus[4] = __LINE__; \
    if (!(CallStatus = memprobe (dwHeapStatus, 16L, NULL))) { \
        REPORT_INFORMATION_DATA (TCP_HEAP_STATUS, LOG_DEBUG,    \
            &dwHeapStatus, sizeof(dwHeapStatus));  \
    } else {  \
        REPORT_ERROR_DATA (TCP_HEAP_STATUS_ERROR, LOG_DEBUG, \
            &CallStatus, sizeof (DWORD)); \
    } \
}

#else

# define HEAP_PROBE()    ;

#endif

#endif  /*_PFUTIL_H_ */
