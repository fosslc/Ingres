/*
** Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include        <xa.h>
#include 	<iitxxa.h>
#include        <iixaapi.h>
#include        <iitxapi.h>
#include	<iitxinit.h>

/*
**  Name: iitxswch.c - Runtime declaration of the Ingres tux_switch structure.
**
**  Description:
**	The global declaration of the Ingres tux_switch structure is in this
**      file.
**
**      Current definition:
**	The static structure(s) in this file are defined in the LIBQXA library, 
**      with which all embedded programs that are coded to operate in an XA
**      environment must link.  The xa_switch structure is global so that
**      (on most machines) the xa_switch pointer can be passed from the program
**      to the TP run-time system across different files and even different
**      programming languages.
**
**	When an embedded program declares the use of the Ingres xa_switch, the
**	preprocessor generates an external (or common) declaration of
**	the global xa_switch structure.  Embedded programs use the definition
**      (or typedef) of IITUX_SWITCH contained in "iitx.h" file.
**
**	LIBQXA never directly references the IITUX_SWITCH. The IITUX_SWITCH 
**	instance is referenced by the TP system. The embedded program relies 
**	on a typedef contained in the file iixa.h and passes a pointer to the 
**	structure into the TP system. 
**
**  History:
**	17-nov-1993 (larrym)
**	    gleened from iixaswch.c
*/
    IITUX_SWITCH  iitux_switch = { {'I', 'N', 'G', 'R', 'E', 'S', '\0'},
                                TMNOFLAGS | TMNOMIGRATE,
                                0,
                                IItux_open,
                                IIxa_close,
                                IIxa_start,
                                IIxa_end,
                                IIxa_rollback,
                                IIxa_prepare,
                                IIxa_commit,
                                IIxa_recover,
                                IIxa_forget,
                                IIxa_complete
			      };

