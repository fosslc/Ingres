/*
** Copyright (c) 2003, 2008 Ingres Corporation
*/
#if !defined VMSTYPES_H
#define VMSTYPES_H
/**
** Name: vmstypes.h - Ingres definition of VMS types not covered elsewhere
**
** Description:
**      This file contains definitions of VMS datatypes which are described
**      in the OpenVMS System Services Reference Manual, but which are not
**	defined in HP-provided headers.
**
** History:
**	29-aug-2003 (abbjo03)
**	    Created.
**	24-jul-2006 (abbjo03)
**	    Added II_VMS_LOCK_ID.
**	09-oct-2008 (stegr01/joea)
**	    Remove II_VMS_ITEM_LIST_3 as we'll use ILE3 instead.
*/

typedef unsigned short II_VMS_CHANNEL;

typedef unsigned int II_VMS_EF_NUMBER;

typedef unsigned short II_VMS_FILE_PROTECTION;

typedef unsigned int II_VMS_LOCK_ID;

typedef unsigned int II_VMS_MASK_LONGWORD;

typedef unsigned int II_VMS_RIGHTS_ID;

#endif
