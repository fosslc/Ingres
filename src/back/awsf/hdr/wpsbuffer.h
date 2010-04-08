/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
** History:
**	07-Sep-1998 (fanra01)
**	    Corrected incomplete last line.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#ifndef WPS_BUFFER_INCLUDED
#define WPS_BUFFER_INCLUDED

#include <ddfcom.h>
#include <wpsfile.h>

#define WPS_BUFFER_EMPTY_BLOCK(X)	{ (X)->block[0] = EOS; (X)->position = 1;(X)->total = 0; }

typedef struct __WPS_BUFFER 
{ 
	u_i4				page_type;
	char				*header;
	u_i4				header_position;
	u_i4				header_size;

	u_i4				block_size;
	char				*block;
	u_i4				position;
	u_i4				size;
	i4			total;

	WCS_PFILE		file;
} WPS_BUFFER, *WPS_PBUFFER;

GSTATUS 
WPSBlockAppend (	
	WPS_PBUFFER buffer, 
	char* str, 
	u_i4 length);

GSTATUS 
WPSHeaderAppend (	
	WPS_PBUFFER buffer, 
	char* str, 
	u_i4 length);

GSTATUS 
WPSHeaderInitialize (
	WPS_PBUFFER buffer);

GSTATUS 
WPSBlockInitialize (	
	WPS_PBUFFER buffer);

GSTATUS 
WPSMoveBlock (	
	WPS_PBUFFER dest, 
	WPS_PBUFFER src);

GSTATUS 
WPSBufferGet (
	WPS_PBUFFER buffer,
	u_i4		*read,
	i4 *total,
	char*		*str,
	u_i4		keep);

GSTATUS 
WPSFreeBuffer (
	WPS_PBUFFER buffer);

#endif
