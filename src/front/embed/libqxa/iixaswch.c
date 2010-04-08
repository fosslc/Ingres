/*
**	Copyright (c) 2004 Ingres Corporation
*/

#include	<compat.h>
#include        <xa.h>
#include 	<iixa.h>
#include        <iixaapi.h>

/**
+*  Name: iixaswch.c - Runtime declaration of the Ingres xa_switch structure.
**
**  Description:
**	The global declaration of the Ingres xa_switch structure is in this
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
**     (or typedef) of IIXA_SWITCH contained in "iixa.h" file.
**
**	LIBQXA never directly references the IIXA_SWITCH. The IIXA_SWITCH instance
**      is referenced by the TP system. The embedded program relies on a typedef
**      contained in the file iixa.h and passes a pointer to the structure into the
**      TP system. 
-*
**  History:
**	13-aug-1992	- written (mani)
**/


    IIXA_SWITCH	iixa_switch = { {'I', 'N', 'G', 'R', 'E', 'S', '\0'},
                                TMNOFLAGS | TMNOMIGRATE,
                                0,
                                IIxa_open,
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
