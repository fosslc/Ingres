/*
** Copyright (c) 1989, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<oosymbol.h>
#include	<abclass.h>
#include	<oocollec.h>
#include	"abproc.h"
#include	"eram.h"

/**
** Name:	abclass.c -	ABF Application Class Object Initializations.
**
** Description:
**	Generated automatically by classout.  Defines:
**
**	IIamObjects[]	array of references to objects.
**	IIamClass[1]	application class.
**	IIamUserFrmCl[1] user frame class.
**	IIamRWFrmCl[1]	report frame class.
**	IIamQBFFrmCl[1]	QBF frame class.
**	IIamGrFrmCl[1]	graph frame class.
**	IIam4GLProcCl[1] 4GL procedure class.
**	IIamDBProcCl[1]	database procedure class.
**	IIamProcCl[1]	3GL procedure class.
**	IIamProcCl[1]	3GL procedure class.
**	IIamGlobCl[1]   global variable class.
**	IIamRecCl[1]   class definition class.
**
** History:
**	Revision 6.3  90/04  wong
**	Replaced some class names with message IDs in the class 'env' member
**	for internationalization.
**
**	Revision 6.2  89/02  wong
**	Created by hand.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

static const char	_ii_abfobjects[]	= ERx("ii_abfobjects");
static const char	_ii_abfclasses[]	= ERx("ii_abfclasses");
static const char	_object_id[]		= ERx("object_id");

/*
**	Appl -- Class object initialization
*/
/* Application Object Methods Array */
static OO_METHOD	mO2001[] = {
	{44,{0x3},OC_METHOD,_authorized,0,0,0,0,0,0,0,1,
		ERx("iiamAuthorized"),(OOID (*)()) iiamAuthorized,0,1,0,0,0},
	{46,{0x3},OC_METHOD,_withName,0,0,0,0,0,0,0,1,
		ERx("iiamDbApplName"),iiamDbApplName,1,1,0,1,1},
	{45,{0x3},OC_METHOD,_destroy,0,0,0,0,0,0,0,1,
		ERx("iiamDestroy"),(OOID (*)()) iiamDestroy,0,1,0,0,0},
	{47,{0x3},OC_METHOD,_confirmName,0,0,0,0,0,0,0,1,
		ERx("iiamCkApplName"),iiamCkApplName,0,1,0,0,0},
};

/* Object methods collection */
static OOID	xO_46[] = {
	(OOID)44,
	(OOID)45,
	(OOID)46,
	(OOID)47
};

static OO_COLLECTION	O_46= {
	-46, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	4,
	0,
	0,
	xO_46,
};

/* Application class structure */
GLOBALDEF OO_CLASS	IIamClass[1] = {
	OC_APPL, {0x3},
	OC_CLASS,
	NULL,
	F_AM0010_Application,_,1,
	ERx("ABF Application Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(APPL),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-46,
	(OOID)0,
};

/*
**	ILCode -- Class object initialization
*/
/* ILCode class structure */
static OO_CLASS	O2010[1] = {
	OC_ILCODE, {0x3},
	OC_CLASS,
	ERx("ILCode"),
	0,_,1,
	ERx("4GL Intermediate Language Code Class"),
	_,_,_,
	ERx("ii_encodings"),
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(OO_OBJECT),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)0,
	(OOID)0,
};

/* Application Procedure Object Methods Array */
static OO_METHOD	mO2020[] = {
	{48,{0x3},OC_METHOD,_withName,0,0,0,0,0,0,0,1,
		ERx("iiamDbGlobalName"),iiamDbGlobalName,1,1,0,1,1},
	{49,{0x3},OC_METHOD,_confirmName,0,0,0,0,0,0,0,1,
		ERx("iiamCkProcName"),iiamCkProcName,0,1,0,0,0},
};

/* Application Procedure Object methods collection */
static OOID	xO_47[] = {
	(OOID)44,
	(OOID)45,
	(OOID)48,
	(OOID)49,
};

static OO_COLLECTION	O_47= {
	-47, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	4,
	0,
	0,
	xO_47,
};

/*
**	HLProc -- Class object initialization
*/
/* HLProc class structure */
GLOBALDEF OO_CLASS	IIamProcCl[1] = {
	OC_HLPROC, {0x3},
	OC_CLASS,
	NULL,
	F_AM0012_HLProc,_,1,
	ERx("Application 3GL Procedure Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(HLPROC),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-47,
	(OOID)0,
};

/*
**	OSLProc -- Class object initialization
*/
/* OSLProc class structure */
GLOBALDEF OO_CLASS	IIam4GLProcCl[1] = {
	OC_OSLPROC, {0x3},
	OC_CLASS,
	NULL,
	F_AM0013_4GLProc,_,1,
	ERx("Application 4GL Procedure Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(_4GLPROC),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-47,
	(OOID)0,
};

/*
**	DBProc -- Class object initialization
*/
/* DBProc class structure */
GLOBALDEF OO_CLASS	IIamDBProcCl[1] = {
	OC_DBPROC, {0x3},
	OC_CLASS,
	NULL,
	F_AM0014_DBProc,_,1,
	ERx("Application SQL (DataBase) Procedure Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(DBPROC),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-47,
	(OOID)0,
};

/*
**	UnProc -- Class object initialization
*/
/* UnProc class structure */
static OO_CLASS	O2190[1] = {
	OC_UNPROC, {0x3},
	OC_CLASS,
	ERx("UnProc"),
	0,_,1,
	ERx("Undefined Procedure Class"),
	_,_,_,
	_,
	_,
	(OOID)1,
	1,
	0,
	0,
	sizeof(OO_OBJECT),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-47,
	(OOID)0,
};

/* Application Frame Object Methods Array */
static OO_METHOD	mO2200[] = {
	{50,{0x3},OC_METHOD,_withName,0,0,0,0,0,0,0,1,
		ERx("iiamDbGlobalName"),iiamDbGlobalName,1,1,0,1,1},
	{51,{0x3},OC_METHOD,_confirmName,0,0,0,0,0,0,0,1,
		ERx("iiamCkFrmName"),iiamCkFrmName,0,1,0,0,0},
};

/* Application Frame Object methods collection */
static OOID	xO_48[] = {
	(OOID)44,
	(OOID)45,
	(OOID)50,
	(OOID)51,
};

static OO_COLLECTION	O_48= {
	-48, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	4,
	0,
	0,
	xO_48,
};

/*
**	OSLFrame -- Class object initialization
*/
/* OSLFrame class structure */
GLOBALDEF OO_CLASS	IIamUserFrmCl[1] = {
	OC_OSLFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM0015_4GLFrame,_,1,
	ERx("User (4GL) Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(USER_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

GLOBALDEF OO_CLASS	IIabMuFrmCl[1] = {
	OC_MUFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM0019_MenuFrame,_,1,
	ERx("Menu Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(USER_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

GLOBALDEF OO_CLASS	IIabAppFrmCl[1] = {
	OC_APPFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM001A_AppendFrame,_,1,
	ERx("Visual Query Append Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(USER_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

GLOBALDEF OO_CLASS	IIabUpdFrmCl[1] = {
	OC_UPDFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM001B_UpdateFrame,_,1,
	ERx("Visual Query Update Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(USER_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};


GLOBALDEF OO_CLASS	IIabBrwFrmCl[1] = {
	OC_BRWFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM001C_BrowseFrame,_,1,
	ERx("Visual Query Browse Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(USER_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

/*
**	OLDOSLFrame -- Class object initialization
*/
/* OLDOSLFrame class structure */
GLOBALDEF OO_CLASS	IIamOSLFrmCl[1] = {
	OC_OLDOSLFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM0015_4GLFrame,_,1,
	ERx("OLDOSL Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(USER_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

/*
**	RWFrame -- Class object initialization
*/
/* RWFrame class structure */
GLOBALDEF OO_CLASS	IIamRWFrmCl[1] = {
	OC_RWFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM0016_ReportFrame,_,1,
	ERx("Report Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(REPORT_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

/*
**	QBFFrame -- Class object initialization
*/
/* QBFFrame class structure */
GLOBALDEF OO_CLASS	IIamQBFFrmCl[1] = {
	OC_QBFFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM0017_QBFFrame,_,1,
	ERx("QBF Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(QBF_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

/*
**	GRFrame -- Class object initialization
*/
/* GRFrame class structure */
GLOBALDEF OO_CLASS	IIamGrFrmCl[1] = {
	OC_GRFRAME, {0x3},
	OC_CLASS,
	NULL,
	F_AM0018_GraphFrame,_,1,
	ERx("VIGRAPH Graph Frame Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(GRAPH_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

/*
**	GBFFrame -- Class object initialization
*/
/* GBFFrame class structure */
static OO_CLASS	O2249[1] = {
	OC_GBFFRAME, {0x3},
	OC_CLASS,
	ERx("GBF Frame"),
	0,_,1,
	ERx(" GBF Frame Class (probably won't be used)"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(GRAPH_FRAME),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-48,
	(OOID)0,
};

/*
**	UnFrame -- Class object initialization
*/
/* UnFrame class structure */
static OO_CLASS	O2250[1] = {
	OC_UNFRAME, {0x3},
	OC_CLASS,
	ERx("UnFrame"),
	0,_,1,
	ERx("Undefined Frame Class"),
	_,_,_,
	_,
	_,
	(OOID)1,
	1,
	0,
	0,
	sizeof(OO_OBJECT),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)0,
	(OOID)0,
};

/*
**	AFORMREF -- Class object initialization
*/
/* AFORMREF class structure */
static OO_CLASS	O3001[1] = {
	OC_AFORMREF, {0x3},
	OC_CLASS,
	ERx("AFORMREF"),
	0,_,1,
	ERx("ABF Form Reference Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(FORM_REF),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-46,
	(OOID)0,
};

/*
**	4GLGlobs -- Class object initialization
*/
/* Application Globals and Constants Object Methods Array */
static OO_METHOD	mO2110[] = {
	{52,{0x3},OC_METHOD,_withName,0,0,0,0,0,0,0,1,
		ERx("iiamDbGlobalName"),iiamDbGlobalName,1,1,0,1,1},
	{54,{0x3},OC_METHOD,_confirmName,0,0,0,0,0,0,0,1,
		ERx("iiamCkGlobalName"),iiamCkGlobalName,0,1,0,0,0},
};

/* Application Globals Object methods collection */
static OOID	xO_49[] = {
	(OOID)44,
	(OOID)45,
	(OOID)52,
	(OOID)54,
};

static OO_COLLECTION	O_49= {
	-49, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	4,
	0,
	0,
	xO_49,
};

/* 4GLGlobs class structure */
GLOBALDEF OO_CLASS	IIamGlobCl[1] = {
	OC_GLOBAL, {0x3},
	OC_CLASS,
	NULL,
	F_AM001D_GlobalVar,_,1,
	ERx("Application Global Variable Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(GLOBVAR),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-49,
	(OOID)0,
};

/* 4GLConstants class structure.  This utilizes the same methods as globals. */
GLOBALDEF OO_CLASS  IIamConstCl[1] = 
{
	OC_CONST, {0x3},
	OC_CLASS,
	NULL,
	F_AM001E_Constant,_,1,
	ERx("Application Constant Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(CONSTANT),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-49,   /* "49" refers to the O_49, globals, collection. */
	(OOID)0,
};

/*
**	4GLRecs -- Class object initialization
*/
/* Application Class Definition Object Methods Array */
static OO_METHOD	mO2130[] = {
	{53,{0x3},OC_METHOD,_withName,0,0,0,0,0,0,0,1,
		ERx("iiamDbRecordName"),iiamDbRecordName,1,1,0,1,1},
};

/* Application Class-Definition Object methods collection */
static OOID	xO_50[] = {
	(OOID)44,
	(OOID)45,
	(OOID)53,
	(OOID)54,
};

static OO_COLLECTION	O_50= {
	-50, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	4,
	0,
	0,
	xO_50,
};

/* 4GL class definitions class structure */
GLOBALDEF OO_CLASS	IIamRecCl[1] = {
	OC_RECORD, {0x3},
	2,
	NULL,
	F_AM001F_ClassDef,_,1,
	ERx("Application Class Definition Class"),
	_,_,_,
	_ii_abfobjects,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(RECDEF),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-50,
	(OOID)0,
};

/*
**	4GLRAtts -- Class object initialization
*/
/* Application Class Attribute Object Methods Array */
static OO_METHOD	mO2133[] = {
	{55,{0x3},OC_METHOD,_withName,0,0,0,0,0,0,0,1,
		ERx("iiamDbRattName"),iiamDbRattName,1,1,0,1,1},
	{56,{0x3},OC_METHOD,_confirmName,0,0,0,0,0,0,0,1,
		ERx("iiamCkRattName"),iiamCkRattName,0,1,0,0,0},
};

/* Application Class-Attribute Object methods collection */
static OOID	xO_51[] = {
	(OOID)44,
	(OOID)55,
	(OOID)56,
};

static OO_COLLECTION	O_51= {
	-51, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	3,
	0,
	0,
	xO_51,
};

/* 4GL class attributes class structure */
GLOBALDEF OO_CLASS	IIamRAttCl[1] = {
	OC_RECMEM, {0x3},
	2,
	NULL,
	F_AM0020_ClassAttr,_,1,
	ERx("Application Class Attribute Class"),
	_,_,_,
	_ii_abfclasses,
	_object_id,
	(OOID)1,
	1,
	0,
	0,
	sizeof(RECMEM),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-51,
	(OOID)0,
};

/*
**	start-up object list
*/

GLOBALDEF OO_OBJECT	*IIamObjects[] = {
	(OO_OBJECT *)IIamClass,
	(OO_OBJECT *)&mO2001[0],
	(OO_OBJECT *)&mO2001[1],
	(OO_OBJECT *)&mO2001[2],
	(OO_OBJECT *)&mO2001[3],
	(OO_OBJECT *)&O_46,
	(OO_OBJECT *)O2010,
	(OO_OBJECT *)IIamProcCl,
	(OO_OBJECT *)IIam4GLProcCl,
	(OO_OBJECT *)IIamDBProcCl,
	(OO_OBJECT *)&mO2020[0],
	(OO_OBJECT *)&mO2020[1],
	(OO_OBJECT *)&O_47,
	(OO_OBJECT *)O2190,
	(OO_OBJECT *)IIamUserFrmCl,
	(OO_OBJECT *)IIamOSLFrmCl,
	(OO_OBJECT *)IIamRWFrmCl,
	(OO_OBJECT *)IIamQBFFrmCl,
	(OO_OBJECT *)IIamGrFrmCl,
	(OO_OBJECT *)&mO2200[0],
	(OO_OBJECT *)&mO2200[1],
	(OO_OBJECT *)&O_48,
	(OO_OBJECT *)IIabMuFrmCl,
	(OO_OBJECT *)IIabAppFrmCl,
	(OO_OBJECT *)IIabUpdFrmCl,
	(OO_OBJECT *)IIabBrwFrmCl,
	(OO_OBJECT *)&mO2110[0],
	(OO_OBJECT *)&mO2110[1],
	(OO_OBJECT *)&O_49,
	(OO_OBJECT *)IIamGlobCl,
	(OO_OBJECT *)&mO2130[0],
	(OO_OBJECT *)&O_50,
	(OO_OBJECT *)IIamRecCl,
	(OO_OBJECT *)&mO2133[0],
	(OO_OBJECT *)&mO2133[1],
	(OO_OBJECT *)&O_51,
	(OO_OBJECT *)IIamRAttCl,
	(OO_OBJECT *)O2249,
	(OO_OBJECT *)O2250,
	(OO_OBJECT *)O3001,
	(OO_OBJECT *)IIamConstCl,
	0,
};

/*
**	Routines to return values from IAOM across boundaries.
**
**	History:
**
**	02-dec-91 (leighb) DeskTop Porting Change: Created
**		Added routine to pass data across ABF/IAOM boundary.
**		(See iaom/abfiaom.c for more detailed comments on the
**		reason and method for this.)
*/

OO_OBJECT **
IIAMgobGetIIamObjects()
{
	return(IIamObjects);
}						 
