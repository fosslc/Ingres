/*
**  Copyright (c) 1989, 2008 Ingres Corporation
*/

#include	<compat.h>
#include	<ol.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abclass.h>

/**
** Name:	iamsystm.c -	Application System Functions Definitions File.
**
** Description:
**	Contains the application component definitions of the application
**	system frames, procedures, and globals.  Defines:
**
**	WARNING - ALL SYSTEM OBJECTS THAT ARE PREALLOCATED (AS IS THE
**	CASE FOR OBJECTS IN THIS FILE) MUST ONLY USE LOWERCASED NAMES
**	AND HAVE NO TRAILING BLANKS.  FAILURE TO DO SO WILL CAUSE
**	LOOKUP PROBLEMS AT RUNTIME.  THIS RESTRICTION IS NEEDED SINCE
**	SYSTEM OBJECTS CAN'T BE CHANGED BY THE HASHING ROUTINES DUE
**	TO THEIR BEING READONLY ON THE ALPHA SYSTEM.
**
**	IIamFrames	application system frames list.
**	IIamProcs	application system procedures list.
**	IIamGlobals	application system globals list.
**
** History:
**	Revision 6.4
**	03/13/91 (emerson)
**		Added procedure commandlineparameters (for Topaz release).
**
**	Revision 6.3/03/00  89/07  wong
**	Initial revision.
**
**	11/21/92 (dkh) - Added above warning due to alpha port.
**	17-jun-1993 (essi)
**		Added support for External File I/O
**	25-may-95 (emmag)
**		_beep conflicts with Microsoft's VC++ for NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-apr-2003 (somsa01)
**	    Updated definition of CommandLineParameters to properly match
**	    external definition (i.e., DB_MAXSTRING does not mean 2000
**	    anymore).
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

# ifdef NT_GENERIC
# define _beep ii_beep
# endif //NT_GENERIC

static const
	char	_Ingres[]	= ERx("$ingres"),
		_integer[]	= ERx("integer"),
		_none[]		= ERx("none"),
		_emerson[]	= ERx("emerson"),
		_Wong[]		= ERx("wong"),
		_Essi[]		= ERx("essi");

/*:
** Name:	IIamFrames -	Application System Frames List
**
** Description:
**	The list of system application component frames.
*/
static USER_FRAME
		_lookup = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLFRAME,	/* object Class */
			ERx("look_up"),	/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			NULL,		/* _next */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtlkup.qsc"), /* source-code file */
			NULL,		/* form reference */
			ERx("IIARlookup"), /* symbol name */
			FALSE,		/* is not static */
			OC_UNDEFINED,	/* IF object ID */
};

GLOBALDEF APPL_COMP	*IIamFrames = (APPL_COMP *)&_lookup;

/*:
** Name:	IIamProcs -	Application System Procedures List
**
** Description:
**	The list of system application component procedures.
*/


static _4GLPROC _close_file = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("closefile"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Essi,		/* last altered by */
			NULL, 		/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtxio.c"), /* source-code file */
			ERx("IIARcfCloseFile"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};
static _4GLPROC _position_file = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("positionfile"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Essi,		/* last altered by */
			(APPL_COMP *)&_close_file, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtxio.c"), /* source-code file */
			ERx("IIARpfPositionFile"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};
static _4GLPROC _inquire_file = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("inquirefile"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Essi,		/* last altered by */
			(APPL_COMP *)&_position_file, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtxio.c"), /* source-code file */
			ERx("IIARifInquireFile"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};
static _4GLPROC _flush_file = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("flushfile"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Essi,		/* last altered by */
			(APPL_COMP *)&_inquire_file, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtxio.c"), /* source-code file */
			ERx("IIARffFlushFile"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};
static _4GLPROC _rewind_file = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("rewindfile"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Essi,		/* last altered by */
			(APPL_COMP *)&_flush_file, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtxio.c"), /* source-code file */
			ERx("IIARfrRewindFile"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};
static _4GLPROC _read_file = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("readfile"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Essi,		/* last altered by */
			(APPL_COMP *)&_rewind_file, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtxio.c"), /* source-code file */
			ERx("IIARrfReadFile"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _write_file = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("writefile"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Essi,		/* last altered by */
			(APPL_COMP *)&_read_file, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtxio.c"), /* source-code file */
			ERx("IIARwfWriteFile"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _open_file = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("openfile"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Essi,		/* last altered by */
			(APPL_COMP *)&_write_file, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtxio.c"), /* source-code file */
			ERx("IIARofOpenFile"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _arrayClear = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("arrayclear"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_open_file,	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(STATUS), DB_INT_TYPE, 0 },
			ERx("abrtarry.c"), /* source-code file */
			ERx("IIARaxClear"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _arrayRemoveRow = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("arrayremoverow"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_arrayClear,	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(STATUS), DB_INT_TYPE, 0 },
			ERx("abrtarry.c"), /* source-code file */
			ERx("IIARaxRemoveRow"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _arraySetRowDeleted = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("arraysetrowdeleted"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_arrayRemoveRow,	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(STATUS), DB_INT_TYPE, 0 },
			ERx("abrtarry.c"), /* source-code file */
			ERx("IIARaxSetRowDeleted"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _arrayInsertRow = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("arrayinsertrow"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_arraySetRowDeleted, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(STATUS), DB_INT_TYPE, 0 },
			ERx("abrtarry.c"), /* source-code file */
			ERx("IIARaxInsertRow"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _arrayFirstRow = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("arrayfirstrow"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_arrayInsertRow, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtarry.c"), /* source-code file */
			ERx("IIARaxFirstRow"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _arrayLastRow = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("arraylastrow"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_arrayFirstRow, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtarry.c"), /* source-code file */
			ERx("IIARaxLastRow"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _arrayAllRows = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("arrayallrows"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_arrayLastRow, /* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtarry.c"), /* source-code file */
			ERx("IIARaxAllRows"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _CommandLineParameters = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("commandlineparameters"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_emerson,	/* last altered by */
			(APPL_COMP *)&_arrayAllRows,	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{
				ERx("varchar(2000)"),
				2000 + sizeof(DB_TEXT_STRING) - 1,
				DB_VCH_TYPE,
				0
			},
			ERx("abrtclp.c"), /* source-code file */
			ERx("IIARclpCmdLineParms"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static HLPROC _getFrame = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_HLPROC,	/* object Class */
			ERx("ii_frame_name"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_CommandLineParameters, /*next component*/
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ ERx("c32"), FE_MAXNAME, DB_CHR_TYPE, 0 },
			ERx("abrtgtfr.c"), /* source-code file */
			ERx("IIARgtFrmName"), /* symbol name */
			OLC		/* host-language type */
};

static _4GLPROC _help = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("help"),	/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_getFrame,	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _none, 0, DB_NODT, 0 },
			ERx("abrthelp.qc"), /* source-code file */
			ERx("IIARhelp"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _help_menu = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("help_menu"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_help, 	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _none, 0, DB_NODT, 0 },
			ERx("abrthelp.qc"), /* source-code file */
			ERx("IIARhlpMenu"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _help_field = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("help_field"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_help_menu, 	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrthelp.qc"), /* source-code file */
			ERx("IIARfieldHelp"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static HLPROC _beep = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_HLPROC,	/* object Class */
			ERx("beep"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_help_field, 	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _none, 0, DB_NODT, 0 },
			ERx("abrtbeep.c"), /* source-code file */
			ERx("IIARbeep"), /* symbol name */
			OLC		/* host-language type */
};

static _4GLPROC _sequence_value = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("sequence_value"), /* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_beep, 	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtseqv.sc"), /* source-code file */
			ERx("IIARsequence_value"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

static _4GLPROC _find_row = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_OSLPROC,	/* object Class */
			ERx("find_record"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)&_sequence_value, 	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* depended-on objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 },
			ERx("abrtfrow.c"), /* source-code file */
			ERx("IIARfnd_row"), /* symbol name */
			OC_UNDEFINED	/* IL frame ID */
};

GLOBALDEF APPL_COMP	*IIamProcs = (APPL_COMP *)&_find_row;


/*:
** Name:	IIamGlobals -	Application System Global Variables List.
**
** Description:
**	The list of system application component global variables.
*/

static GLOBVAR _iiretval = {
			OC_UNDEFINED,	/* object ID */
			{0x3},		/* dataWord */
			OC_GLOBAL,	/* object Class */
			ERx("iiretval"),/* name */
			0,		/* env */
			_Ingres,	/* owner */
			0,		/* is_current */
			NULL,		/* short remark */
			NULL,		/* create date */
			NULL,		/* alter date */
			NULL,		/* long remark */
			_Wong,		/* last altered by */
			(APPL_COMP *)NULL, 	/* next component */
			NULL,		/* application */
			0,		/* version */
			NULL,		/* local symbol (build only) */
			NULL,		/* Compilation date */
			0,		/* flags */
			NULL,		/* Depended-upon objects */
			/* Data type Descriptor */
			{ _integer, sizeof(i4), DB_INT_TYPE, 0 }
};

GLOBALDEF APPL_COMP	*IIamGlobals = (APPL_COMP *)&_iiretval;

/*
**	Routines to return values from IAOM across boundaries.
**
**	History:
**
**	02-dec-91 (leighb) DeskTop Porting Change: Created
**		Added routines to pass data across ABF/IAOM boundary.
**		(See iaom/abfiaom.c for more detailed comments on the
**		reason and method for this.)
*/

APPL_COMP *
IIAMgfrGetIIamFrames()
{
	return(IIamFrames);
}

APPL_COMP *
IIAMgprGetIIamProcs()
{
	return(IIamProcs);
}

APPL_COMP *
IIAMgglGetIIamGlobals()
{
	return(IIamGlobals);
}						 
