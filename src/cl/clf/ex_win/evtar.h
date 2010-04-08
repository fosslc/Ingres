/*
** Copyright (C) 2010 Ingres Corporation
*/

/**
** Name:        evtar.h        - EVtar interface
**
** Description:
**      Prototype for the EVtar function and flag values.
**
** History:
**      27-Jan-2010 (horda03) Bug 121811
**          Created.
**/

/* Provide BASIC TAR functionality on Windows. */

#define EVTAR_VERBOSE      0x00000001 /* Provide diagnostic messages */
#define EVTAR_VERBOSE_HDR  0x00000002 /* Additional diagnostics for TAR Header use */
#define EVTAR_VERBOSE_DIAG 0x00000004 /* Even more diagnostics */
#define EVTAR_UNTAR        0x00000008 /* UNTAR a TART file */
#define EVTAR_REMOVE_LEAD  0x00000010 /* Don't keep %II_EXCEPTION%\ingres\except\EVSETxxx\ */

STATUS EVtar(i4 tar_flags, char *tar_dir, char *tar_file);
