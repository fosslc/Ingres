
/*
**Copyright (c) 1989, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<fdesc.h>
#include	<abfrts.h>

/**
** Name:	ilsystab.roc -	Application Run-Time System Routines Table.
**
** Description:
**	Contains the interpreter application system function routines table.
**	These tables map system function names to the ABF run-time routines
**	that implement their functionality.
**
**	Note:  The internal system function component objects must be defined
**	in "iaom!iamsystm.c" while the ABF run-time routines are defined in the
**	"abfrt" directory.
**
**	Defines:
**
**	iiITsyProcs[]		application system procedures function table.
**	iiITsyFrames[]		application system frames function table.
**
** History:
**	Revision 6.4
**	03/13/91 (emerson)
**		Added commandlineparameters to iiITsyProcs.
**
**	Revision 6.3/03/00  89/07  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Fix routine types to match reality.
**	    Doesn't really matter but keeps aux-info output clean.
**/

/*:
** Name:	iiITsyProcs[] -	ABF System Procedures Function Tables.
**
** Description:
**	Application system procedures function table used by the interpreter.
**	This links and maps the system procedures into the interpreter.
**
**	Note:  System procedures must also be defined in "iaom!iamsystm.c".
**
** History:
**	07/89 (jhw) -- Written.
**	03/13/91 (emerson)
**		Added commandlineparameters (for Topaz release).
**	17-jun-93 (essi)
**		Added support for External File I/O.
*/

PTR	IIARsequence_value();
PTR	IIARfnd_row();
void	IIARbeep();
PTR	IIARhelp();
PTR	IIARhlpMenu();
PTR	IIARfieldHelp();
char *	IIARgtFrmName();

PTR	IIARaxAllRows();
PTR	IIARaxLastRow();
PTR	IIARaxFirstRow();
PTR	IIARaxInsertRow();
PTR	IIARaxSetRowDeleted();
PTR	IIARaxRemoveRow();
PTR	IIARaxClear();

PTR	IIARclpCmdLineParms();

PTR	IIARofOpenFile();
PTR	IIARwfWriteFile();
PTR	IIARrfReadFile();
PTR	IIARcfCloseFile();
PTR	IIARfrRewindFile();
PTR	IIARpfPositionFile();
PTR	IIARffFlushFile();
PTR	IIARifInquireFile();

GLOBALDEF ABRTSFUNCS	iiITsyProcs[] =
{
	{ERx("sequence_value"), IIARsequence_value},
	{ERx("find_record"), IIARfnd_row},
	{ERx("beep"), (PTR (*)()) IIARbeep},
	{ERx("help"), IIARhelp},
	{ERx("help_menu"), IIARhlpMenu},
	{ERx("help_field"), IIARfieldHelp},
	{ERx("ii_frame_name"), (PTR (*)()) IIARgtFrmName},
	{ERx("arrayallrows"), IIARaxAllRows},
	{ERx("arraylastrow"), IIARaxLastRow},
	{ERx("arrayfirstrow"), IIARaxFirstRow},
	{ERx("arrayinsertrow"), IIARaxInsertRow},
	{ERx("arraysetrowdeleted"),IIARaxSetRowDeleted},
	{ERx("arrayremoverow"), IIARaxRemoveRow},
	{ERx("arrayclear"), IIARaxClear},
	{ERx("commandlineparameters"), IIARclpCmdLineParms},
	{ERx("openfile"), IIARofOpenFile},
	{ERx("writefile"), IIARwfWriteFile},
	{ERx("readfile"), IIARrfReadFile},
	{ERx("closefile"), IIARcfCloseFile},
	{ERx("rewindfile"), IIARfrRewindFile},
	{ERx("positionfile"), IIARpfPositionFile},
	{ERx("flushfile"), IIARffFlushFile},
	{ERx("inquirefile"), IIARifInquireFile},
	NULL
};

/*:
** Name:	iiITsyFrames[] -	ABF System Frames Function Table.
**
** Description:
**	Application system frames function table used by the interpreter.
**	This links and maps the system frames into the interpreter.
**
**	Note:  System frames must also be defined in "iaom!iamsystm.c".
**
** History:
**	07/89 (jhw) -- Written.
*/

PTR	IIARlookup();

GLOBALDEF ABRTSFUNCS	iiITsyFrames[] =
{
	{ERx("look_up"), IIARlookup},
	NULL
};
