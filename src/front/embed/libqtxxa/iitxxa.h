/*
** Copyright (c) 2004 Ingres Corporation
*/
/*--------------------------- iitx.h -----------------------------------*/

/*
** Name: iitx.h
**
** Description:
**	Main header file for Tuxedo bridge. If TUXEDO is defined at compile 
**	time defines a set of PTRS and MACROS to act as switches from current 
**	XA code into tuxedo specific rountines this should in theory be the 
**	only place where there is an #ifdef TUXEDO
**
**      Header should be included AFTER iicxfe.h as it refs 
**	IIcx_fe_thread_cb_main.
**
** History:
**	01-Oct-93	(mhughes)
**	First Implementation
**	17-nov-93 (larrym)
**	    moved IIXA_CHECK_IF_TUXEDO_MACRO to iixagbl.h.  Also removed
**	    IItux_main prototype (since it's always called through a func
**	    pointer.
*/

#ifndef IITX_H
#define IITX_H


/*
** Turn on TUXEDO functionality
*/

typedef struct  xa_switch_t  IITUX_SWITCH;

extern  IITUX_SWITCH  iitux_switch;


/*---------------  end of iixa.h  ----------------------------------------*/

#endif /* ifndef IITX_H */
/*--------------------------- iitx.h -----------------------------------*/
