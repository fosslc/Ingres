/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: icegca.h
**
** Description:
**      Types, definitions and prototype for ICE client communications.
**
** History:
**      18-May-2000 (fanra01)
**          Bug 101345
**          Add session parameters to track the processing of the response
**          from the ice server.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef ICEGCA_INCLUDED
#define ICEGCA_INCLUDED

#include <ddfcom.h>
#include <gca.h>

/*
** File: icegca.h
**
** History:
**	09-aug-1999 (mcgem01)
**	    Change nat and longnat to i4.
*/

#define		GCA_BUFFER_SIZE	2008

#define  ICE_QUERY_HEADER    0x0002
#define  ICE_QUERY_FINAL     0x0001


typedef struct __ICEGCASession 
{
    i4                      aid;
    char                    buf[GCA_BUFFER_SIZE + 50];
    char*                   rcv;
    GCA_PARMLIST            gca_parms;
    i4                      mlen;       /* length of message */
    i4                      tdata;      /* offset of next tuple data */
    i4                      blobsegs;   /* blob segment counter */
    i4                      flen;       /* fragment length */
    i4                      segflgs;    /* segment processing state */
    i4                      seglen;
    i4                      gca_hdr_len;
    bool                    complete;
    struct __ICEGCASession  *next;
} ICEGCASession, *PICEGCASession;

#define WPS_HTML_BLOCK      1
#define WPS_URL_BLOCK       4
#define WPS_DOWNLOAD_BLOCK  8

GSTATUS
ICEGCAInitiate( i4 timeout, i4* gca_hdr_len );

GSTATUS
ICEGCAConnect(
		PICEGCASession session,
		char*					 node);

GSTATUS
ICEGCASendQuery(
		PICEGCASession session, 
		char*					 query,
		u_i4					 size,
		bool					 end);

GSTATUS
ICEGCASendData(
    PICEGCASession session,
    char* data,
    i4 size,
    i4* flags,
    i4* sent );

GSTATUS
ICEGCAWait(
		PICEGCASession session, 
		u_i4					 *messageType);

GSTATUS
ICEGCAHeader(
		PICEGCASession session, 
		char*					 *result, 
		u_i4					 *returned);

GSTATUS
ICEGCAPage(
    PICEGCASession  session,
    char*           *result,
    u_i4            *returned,
    i4* more);

GSTATUS
ICEGCADisconnect(
		PICEGCASession session);


#endif
