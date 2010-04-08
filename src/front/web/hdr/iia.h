/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: iia.h
**
** Description:
**      ICE Interface Adapter header. The IIA localises and encapsulates
**      the differences between the various supported Web server intefaces.
**
** History:
**      20-nov-1996 (wilan06)
**          Created
**      24-jan-1996 (harpa06)
**          Moved union in tagICECLIENT structure to the end since it
**          varies in size. 
**
**          Added to ICEADI and ICENSAPI structures.
**	06-feb-1996 (harpa06)
**          Added ICELogEntry() to tagICECLIENT to allow log entries added to
**          a Web server's error log file. 
**      28-feb-1996 (harpa06)
**          Removed the #define ICE_ISAPI and added it to it's proper source
**          file. This header file is generic to all OS platforms as well as
**          Web server interfaces.
**      07-mar-1997 (harpa06)
**          Removed ICELogEntry(). ICE logging is in ICEWriteLog().
*/

# ifndef IIA_H_INCLUDED
# define IIA_H_INCLUDED

/*
** ICEADI
** Contains the server specific information necessary to communicate
** with a Spyglass Web server.
*/
typedef struct tagICEADI
{
# ifdef ICE_ADI
    char                requri [SI_MAX_TXT_REC];
                                /* Request data via GET method */
    ADITransID          tid;    /* Client transaction ID */
    ADIRequestPointer   pReq;   /* Pointer to client request */
# else
    int                 dummy;
# endif
} ICEADI, *PICEADI;

/*
** ICEFILE
** Contains the server specific information necessary to communicate
** with a CGI Web server, or a file.
*/
typedef struct tagICEFILE
{
    FILE*       fpRead;
    FILE*       fpWrite;
} ICEFILE, *PICEFILE;

/*
** ICEISAPI
** Contains the server specific information necessary to communicate
** with an ISAPI Web server.
*/
typedef struct tagICEISAPI
{
# ifdef ICE_ISAPI
    EXTENSION_CONTROL_BLOCK*    pecb;
# else
    int                         dummy;
# endif
    
} ICEISAPI, *PICEISAPI;

/*
** ICENSAPI
** Contains the server specific information necessary to communicate
** with an NSAPI Web server.
*/
typedef struct tagICENSAPI
{
# ifdef ICE_NSAPI
    char    reqmethod [5];  /* Request method */
    char    reqdata [SI_MAX_TXT_REC]; 
                            /* Request data via the GET method */
    Request *request;       /* Pointer to client request */
    pblock  *pblock;        /* Block containing server variables */
    Session *session;       /* Client session */
# else
    int     dummy;
# endif
} ICENSAPI, *PICENSAPI;

/*
** ICEWSTYPE
** This enum defines identifiers for each supported Web server interface
*/
typedef enum wstype
{
    WS_CGI, WS_ISAPI, WS_NSAPI, WS_ADI, WS_FILE, WS_INVALID
} ICEWSTYPE;

/*
** ICEPFN
** Generic function pointer
*/
typedef ICERES (*ICEPFN)();

/*
** ICECLIENT
**
*/
typedef struct tagICECLIENT
{
    ICEWSTYPE           wsType;
    ICEPFN              ICEInitPage;
    ICEPFN              ICEReadClient;
    ICEPFN              ICEWriteClient;
    ICEPFN              ICESendURL;
    ICEPFN              ICEGetVariable;
    PICECONFIGDATA      picd;
    PICEPAGE            pip;
    PICEREQUEST         pir;
    /* Union is placed here due to the varying size of each structure defined
    ** within
    */
    union {
        ICEFILE         ifl;
        ICEISAPI        iis;
        ICENSAPI        ins;
        ICEADI          iad;
    } ua;
} ICECLIENT, *PICECLIENT;

# endif
