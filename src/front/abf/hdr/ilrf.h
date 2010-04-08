/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    ilrf.h -	Interpreted Frame Object Run-time Facility
**				Interface Definitions File
** Description:
**	Contains definitions that define the interface to ILRF.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Remove da_get, dbd_get, and push callback routine pointers;
**		ILRF is no longer responsible for manipulating stack frames.
**		Remove RETADDR structure;
**		ILRF is no longer responsible for maintaining return addresses.
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**
**	Revision 5.1  86/08  bobm
**	Initial revision.
*/

/*}
** Name:    IRBLK -	ILRF Block Description Structure.
*/
typedef struct
{
	OLIMHDR	image;	/* image file structure */

	i4 *curframe;	/* pointer to current ILRF internal frame structure */
	i2 magic;	/* identify legally opened IRBLK */
	i2 irbid;	/* IRBLK identifier - "client" */
	i2 irtype;	/* type of interaction for client */
} IRBLK;

/*
** encodes for types of memory free requests
*/
#define IORM_SINGLE 1	/* any single unused frame */
#define IORM_ALL 2	/* all unused frames */
#define IORM_FRAME 3	/* a specific frame */

/*
** encodes for mode of frame retrieval in ILRF session
*/
#define IORI_DB 1	/* from database */
#define IORI_IMAGE 2	/* from image file */
#define IORI_CORE 3	/* from core structure */

