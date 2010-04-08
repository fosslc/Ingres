/*
**	MWhost.h
**	"@(#)mwhost.h	1.4"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	MWhost.h - Test host Pascal interface.
**
** Usage:
**	INGRES or Macintosh specific declarations.
**
** Description:
**	Contains the Pascal interface declarations for the MWhost
**	module.  These functions are not used by the SUN or VMS
**	host, so they are nulled out if not MWhost.
**	This file also defines data structures used on the Mac
**	and passed to the host.
**	MAC_HOST is a pre-processor variable that defines whether
**	we are MWhost or otherwise.
**
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**/

# ifndef MAC_HOST

/*
**  Null out functions that are not needed on the
**  UNIX or VMS host.
*/
# define MWS_D_Select(a)
# define MWS_I_Select(a)
# define MWS_L_Select(a)
# define MWS_M_Select(a)
# define MWS_P_Init(a)
# define MWS_P_Disconnect()
# define MWS_P_Quit(a)
# define MWS_P_Prime(a)
# define MWS_P_Dispatch(a)

/*
**  The following data type is defined on the Mac,
**  and is used by MWS to pass info to the host.
*/
typedef struct _Rect {
	i2 top;
	i2 left;
	i2 bottom;
	i2 right;
} Rect;

# else	/* MAC_HOST */

/*
**	Pascal interface declarations for the MWhost module.
/*

#define ProgInit		PROGINIT
#define Msg_Get			MSG_GET
#define Msg_NextEvent		MSG_NEXTEVENT
#define Msg_Response		MSG_RESPONSE
#define MWS_Event		MWS_EVENT
#define MWS_P_Init		MWS_P_INIT
#define MWS_P_Disconnect	MWS_P_DISCONNECT
#define MWS_P_Quit		MWS_P_QUIT
#define MWS_P_Prime		MWS_P_PRIME
#define MWS_P_Dispatch		MWS_P_DISPATCH
#define MWS_I_Select		MWS_I_SELECT
#define MWS_L_Select		MWS_L_SELECT
#define MWS_M_Select		MWS_M_SELECT
#define MWS_D_Select		MWS_D_SELECT

/*	general pascal routines	*/
pascal void ProgInit();

/*	receiving mws events & message processing	*/
pascal TPMsg Msg_Get();
pascal bool Msg_NextEvent(TPMsg theMsg,i4 timeout);
pascal bool Msg_Response(TPMsg theMsg,i2 class,i4 id,i4 timeout);

/*	handling events from MacWorkStation	*/
pascal void MWS_Event(TPMsg theMsg);

/*	MacWorkStation process control	*/
pascal void MWS_P_Init(TPMsg theMsg);
pascal void MWS_P_Disconnect();
pascal void MWS_P_Quit(TPMsg theMsg);
pascal void MWS_P_Prime(TPMsg theMsg);
pascal void MWS_P_Dispatch(TPMsg theMsg);

/*	MacWorkStation icons	*/
pascal void MWS_I_Select(TPMsg theMsg);

/*	MacWorkStation lists	*/
pascal void MWS_L_Select(TPMsg theMsg);

/*	MacWorkStation menus	*/
pascal void MWS_M_Select(TPMsg theMsg);

/*	MacWorkStation dialogs	*/
pascal void MWS_D_Select(TPMsg theMsg);

# endif	/* MAC_HOST */
