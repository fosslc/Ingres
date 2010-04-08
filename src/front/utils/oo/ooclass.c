/*
** Copyright (c) 1987, 2008 Ingres Corporation
**	All rights reserved.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<oosymbol.h>
#include	<ooclass.h>
#include	<oocollec.h>
#include	<lo.h>
#include	"ooproc.h"
#include	"eroo.h"

/**
** Name:	ooclass.c -	Generic Class Object Initializations.
**
** Description:
**	Generated automatically by classout.  Defines:
**
**	IIooObjects[]	array of references to objects.
**
** History:
**	Revision 6.1  87/01  peterk
**	Generated.
**	88/12  Removed non-universal Class methods and Graph classes.  Graph
**	classes are now in "graphics!graf!grclass.qsc".
**	89/01  Removed Application classes to "abf!iaom!abclass.qsc".
**	89/03  Moved OO_aCOLLECTION definition to "front!hdr!oocollec.h".
**	Revision 6.2  89/07  wong
**	Portability changes for Collection objects:  Changed collection to be
**	an array pointer rather than an array (defining array elements after the
**	structure is not portable.)  Also, added Collection allocation method.
**	Also, replaced some class constants with their defines (those that were
**	defined.)
**	89/11/10  terryr  Modified OO_REFERENCE member offsets ('moffset')
**	definitions to calculate each offset rather than having it hardwired.
**
**	Revision 6.3  90/01  mgw
**	Sequent and VMS compiler bugs preclude pointer arithmetic in
**	structure initializations so just use the real offsets
**	in the Sequent and VMS cases.
**	90/04  wong  Replaced some class names with message IDs in class 'env'
**		member, which will be set when initialized for international
**		support.
**
**	8-feb-93 (blaise)
**		Changed _flush, etc. to ii_flush because of conflict with
**		Microsoft C library.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	29-may-1997 (canor01)
**	    Cleaned up compiler warnings from CL prototypes.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/
# if defined(NT_GENERIC)
# include "oosymbol.roc"
# endif             /* NT_GENERIC  */

/*
**	Object -- Class object initialization
*/
/* Object methods array */
static OO_METHOD	mO1[] = {
    {117,{0x3},OC_METHOD,_alloc,0,0,0,0,0,0,0,1,
		ERx("iiooAlloc"),iiooAlloc,1,1,0,1,1},
    {30,{0x3},OC_METHOD,_mlook,0,0,0,0,0,0,0,1,
		ERx("iiooClook"),iiooClook,1,1,0,1,1},
    {22,{0x3},OC_METHOD,_new,0,0,0,0,0,0,0,1,
		ERx("iiooCnew"),iiooCnew,-1,1,0,1,1},
    {101,{0x3},OC_METHOD,_new0,0,0,0,0,0,0,0,1,
		ERx("iiooC0new"),iiooC0new,1,1,0,1,1},
    {116,{0x3},OC_METHOD,_newDb,0,0,0,0,0,0,0,1,
		ERx("iiooCDbnew"),iiooCDbnew,-1,1,0,1,1},
    {11,{0x3},OC_METHOD,_perform,0,0,0,0,0,0,0,1,
		ERx("iiooCperform"),iiooCperform,-1,1,0,1,1},
    {154,{0x3},OC_METHOD,_withName,0,0,0,0,0,0,0,1,
		ERx("iiooCwithName"),iiooCwithName,1,1,0,1,1},
    {33,{0x3},OC_METHOD,_authorized,0,0,0,0,0,0,0,1,
		ERx("iiobAuthorized"),(OOID (*)())iiobAuthorized,0,1,0,0,0},
    {109,{0x3},OC_METHOD,_confirmName,0,0,0,0,0,0,0,1,
		ERx("iiobCheckName"),iiobCheckName,0,1,0,0,0},
    {69,{0x3},OC_METHOD,_class,0,0,0,0,0,0,0,1,
		ERx("OBclass"),OBclass,0,1,0,0,0},
    {16,{0x3},OC_METHOD,_copy,0,0,0,0,0,0,0,1,
		ERx("OBcopy"),OBcopy,0,1,0,0,0},
    {17,{0x3},OC_METHOD,_copyId,0,0,0,0,0,0,0,1,
		ERx("OBcopyId"),(OOID (*)()) OBcopyId,0,1,0,0,0},
    {15,{0x3},OC_METHOD,_destroy,0,0,0,0,0,0,0,1,
		ERx("OBdestroy"),(OOID (*)()) OBdestroy,0,1,0,0,0},
    {26,{0x3},OC_METHOD,_edit,0,0,0,0,0,0,0,1,
		ERx("OBedit"),OBedit,0,1,0,0,0},
    {192,{0x3},OC_METHOD,_encode,0,0,0,0,0,0,0,1,
		ERx("OBencode"),OBencode,0,1,0,0,0},
    {74,{0x3},OC_METHOD,_init,0,0,0,0,0,0,0,1,
		ERx("OBinit"),OBinit,8,1,0,0,0},
    {114,{0x3},OC_METHOD,_init0,0,0,0,0,0,0,0,1,
		ERx("OBinit"),OBinit,8,1,0,0,0},
    {115,{0x3},OC_METHOD,_initDb,0,0,0,0,0,0,0,1,
		ERx("OBinit"),OBinit,8,1,0,0,0},
    {37,{0x3},OC_METHOD,_isEmpty,0,0,0,0,0,0,0,1,
		ERx("OBisEmpty"),(OOID (*)()) OBisEmpty,0,1,0,0,0},
    {36,{0x3},OC_METHOD,_isNil,0,0,0,0,0,0,0,1,
		ERx("OBisNil"),(OOID (*)()) OBisNil,0,1,0,0,0},
    {20,{0x3},OC_METHOD,_markChange,0,0,0,0,0,0,0,1,
		ERx("OBmarkChange"),OBmarkChange,0,1,0,0,0},
    {67,{0x3},OC_METHOD,_name,0,0,0,0,0,0,0,1,
		ERx("OBname"),(OOID (*)())OBname,0,1,0,0,0},
    {29,{0x3},OC_METHOD,_noMethod,0,0,0,0,0,0,0,1,
		ERx("OBnoMethod"),OBnoMethod,1,1,0,0,0},
    {156,{0x3},OC_METHOD,_print,0,0,0,0,0,0,0,1,
		ERx("OBprint"),OBprint,0,1,0,0,0},
    {24,{0x3},OC_METHOD,_rename,0,0,0,0,0,0,0,1,
		ERx("OBrename"),(OOID (*)()) OBrename,1,1,0,0,0},
    {21,{0x3},OC_METHOD,_setPermit,0,0,0,0,0,0,0,1,
		ERx("OBsetPermit"),OBsetPermit,3,1,0,0,0},
    {68,{0x3},OC_METHOD,_setRefNull,0,0,0,0,0,0,0,1,
		ERx("OBsetRefNull"),OBsetRefNull,1,1,0,0,0},
    {70,{0x3},OC_METHOD,_subResp,0,0,0,0,0,0,0,1,
		ERx("OBsubResp"),OBsubResp,1,1,0,0,0},
    {195,{0x3},OC_METHOD,_time,0,0,0,0,0,0,0,1,
		ERx("OBtime"),OBtime,-1,1,0,0,0},
    {86,{0x3},OC_METHOD,_fetch0,0,0,0,0,0,0,0,1,
		ERx("fetch0"),fetch0,0,1,0,0,0},
    {76,{0x3},OC_METHOD,_fetchAll,0,0,0,0,0,0,0,1,
		ERx("fetchAll"),fetchAll,0,1,0,0,0},
    {27,{0x3},OC_METHOD,_fetch,0,0,0,0,0,0,0,1,
		ERx("fetchAt"),fetchAt,0,1,0,0,0},
    {100,{0x3},OC_METHOD,_fetchDb,0,0,0,0,0,0,0,1,
		ERx("fetchDb"),fetchDb,0,1,0,0,0},
    {84,{0x3},OC_METHOD,ii_flush0,0,0,0,0,0,0,0,1,
		ERx("flush0"),flush0,0,1,0,0,0},
    {85,{0x3},OC_METHOD,ii_flushAll,0,0,0,0,0,0,0,1,
		ERx("IIflushAll"),IIflushAll,0,1,0,0,0},
    {28,{0x3},OC_METHOD,ii_flush,0,0,0,0,0,0,0,1,
		ERx("flushAt"),flushAt,0,1,0,0,0},
};

/* Object methods collection */
static OOID	xO_2[] = {
	(OOID)117,
	(OOID)30,
	(OOID)22,
	(OOID)101,
	(OOID)116,
	(OOID)11,
	(OOID)154,
	(OOID)33,
	(OOID)109,
	(OOID)69,
	(OOID)16,
	(OOID)17,
	(OOID)15,
	(OOID)26,
	(OOID)192,
	(OOID)74,
	(OOID)114,
	(OOID)115,
	(OOID)37,
	(OOID)36,
	(OOID)20,
	(OOID)67,
	(OOID)29,
	(OOID)156,
	(OOID)24,
	(OOID)21,
	(OOID)68,
	(OOID)70,
	(OOID)195,
	(OOID)86,
	(OOID)76,
	(OOID)27,
	(OOID)100,
	(OOID)84,
	(OOID)85,
	(OOID)28,
};

static OO_COLLECTION	O_2= {
	-2, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_2)/sizeof(xO_2[0]),
	0,
	0,
	xO_2,
};

/* Object detail refs collection (entries in a masterRefs array) */
static OOID	xO_3[] = {
	(OOID)7,
};

static OO_COLLECTION	O_3= {
	-3, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_3)/sizeof(xO_3[0]),
	0,
	0,
	xO_3,
};

/* Object attributes array */
static OO_ATTRIB	aO_4[] = {
	{ 2, ERx("class"), 30, 2},
	{ 3, ERx("name"), 32, 32},
	{ 4, ERx("env"), 30, 2},
	{ 5, ERx("owner"), 32, 24},
	{ 6, ERx("is_current"), 30, 1},
	{ 7, ERx("short_remark"), 32, 60},
	{ 8, ERx("create_date"), 3, 25},
	{ 9, ERx("alter_date"), 3, 25},
	{ 10, ERx("long_remark"), 43, 600},
};

/* Object attributes collection */
static OO_ATTRIB	*xO_4[] = {
	&aO_4[0],
	&aO_4[1],
	&aO_4[2],
	&aO_4[3],
	&aO_4[4],
	&aO_4[5],
	&aO_4[6],
	&aO_4[7],
	&aO_4[8],
};

static OO_aCOLLECTION	O_4= {
	-4, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	9,
	0,
	0,
	xO_4,
};

/* Object class structure */
GLOBALDEF OO_CLASS	O1[1] = {
	OC_OBJECT, {0x3},
	OC_CLASS,
	ERx("Object"),
	0,_,1,
	ERx("ObjectClass"),
	_,_,_,
	ERx("ii_objects"),
	ERx("object_id"),
	(OOID)0,
	0,
	0,
	0,
	sizeof(OO_OBJECT),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)-3,
	(OOID)-2,
	(OOID)-4,
};

/*
**	Class -- Class object initialization
*/
/* Class master references array */

/*
**	A dummy structure used to accurately calculate member offsets
**	('moffset') in the reference structure for the class class.
*/
static const OO_CLASS	class_ref;

static OO_REFERENCE	rO2[] = {
{
	6, {0x3},
	OC_REFERENCE,
	ERx("subClasses"),
	0,_,1,_,_,_,_,
	2,
	-1,
	CL_OFFSETOF(OO_CLASS, subClasses),
	ERx("super"),
	2,
	0,
	52,
},
{
	7, {0x3},
	OC_REFERENCE,
	ERx("instances"),
	0,_,1,_,_,_,_,
	2,
	-1,
	CL_OFFSETOF(OO_CLASS, instances),
	ERx("class"),
	1,
	0,
	8,
},
{
	8, {0x3},
	OC_REFERENCE,
	ERx("masterRefs"),
	0,_,1,_,_,_,_,
	2,
	-1,
	CL_OFFSETOF(OO_CLASS, masterRefs),
	ERx("master"),
	3,
	0,
	44,
},
{
	9, {0x3},
	OC_REFERENCE,
	ERx("detailRefs"),
	0,_,1,_,_,_,_,
	2,
	-1,
	CL_OFFSETOF(OO_CLASS, detailRefs),
	ERx("detail"),
	3,
	0,
	60,
},
{
	10, {0x3},
	OC_REFERENCE,
	ERx("methods"),
	0,_,1,_,_,_,_,
	2,
	1,
	CL_OFFSETOF(OO_CLASS, methods),
	ERx("mclass"),
	4,
	0,
	44,
},
{
	95, {0x3},
	OC_REFERENCE,
	ERx("attributes"),
	0,_,1,_,_,_,_,
	2,
	1,
	CL_OFFSETOF(OO_CLASS, attributes),
	_,
	1,
	0,
	0,
},
};

/* Class master reference collection */
static OOID	xO_5[] = {
	(OOID)6,
	(OOID)7,
	(OOID)8,
	(OOID)9,
	(OOID)10,
	(OOID)95,
};

static OO_COLLECTION	O_5= {
	-5, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_5)/sizeof(xO_5[0]),
	0,
	0,
	xO_5,
};

/* Class detail refs collection (entries in a masterRefs array) */
static OOID	xO_6[] = {
	(OOID)6,
};

static OO_COLLECTION	O_6= {
	-6, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_6)/sizeof(xO_6[0]),
	0,
	0,
	xO_6,
};

/* Class attributes array */
static OO_ATTRIB	aO_7[] = {
	{ 2, ERx("table"), 32, 24},
	{ 3, ERx("surrogate"), 32, 24},
	{ 4, ERx("super"), 30, 2},
	{ 5, ERx("level"), 30, 1},
	{ 6, ERx("keepstatus"), 30, 1},
	{ 7, ERx("hasarray"), 30, 1},
	{ 8, ERx("memsize"), 30, 2},
};

/* Class attributes collection */
static OO_ATTRIB	*xO_7[] = {
	&aO_7[0],
	&aO_7[1],
	&aO_7[2],
	&aO_7[3],
	&aO_7[4],
	&aO_7[5],
	&aO_7[6],
};

static OO_aCOLLECTION	O_7= {
	-7, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	7,
	0,
	0,
	xO_7,
};

/* Class class structure */
GLOBALDEF OO_CLASS	O2[1] = {
	OC_CLASS, {0x3},
	OC_CLASS,
	ERx("Class"),
	0,_,1,
	ERx("ClassClass"),
	_,_,_,
	ERx("iiclass"),
	ERx("id"),
	(OOID)1,
	1,
	1,
	0,
	sizeof(OO_CLASS),
	(OOID)-1,
	(OOID)-1,
	(OOID)-5,
	(OOID)-6,
	(OOID)0,
	(OOID)-7,
};

/*
**	Reference -- Class object initialization
*/
/* Reference methods array */
static OO_METHOD	mO3[] = {
    {113,{0x3},OC_METHOD,_init,0,0,0,0,0,0,0,3,
		ERx("REinit"),(OOID (*)()) REinit,7,1,0,0,0},
    {119,{0x3},OC_METHOD,_initDb,0,0,0,0,0,0,0,3,
		ERx("REinitDb"),(OOID (*)()) REinitDb,7,1,0,0,0},
};

/* Reference methods collection */
static OOID	xO_10[] = {
	(OOID)113,
	(OOID)119,
};

static OO_COLLECTION	O_10= {
	-10, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_10)/sizeof(xO_10[0]),
	0,
	0,
	xO_10,
};

/* Reference detail refs collection (entries in a masterRefs array) */
static OOID	xO_11[] = {
	(OOID)8,
	(OOID)9,
};

static OO_COLLECTION	O_11= {
	-11, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_11)/sizeof(xO_11[0]),
	0,
	0,
	xO_11,
};

/* Reference attributes array */
static OO_ATTRIB	aO_12[] = {
	{ 2, ERx("master"), 30, 2},
	{ 3, ERx("mdelete"), 30, 1},
	{ 4, ERx("moffset"), 30, 2},
	{ 5, ERx("connector"), 37, 14},
	{ 6, ERx("detail"), 30, 2},
	{ 7, ERx("ddelete"), 30, 1},
	{ 8, ERx("doffset"), 30, 2},
};

/* Reference attributes collection */
static OO_ATTRIB	*xO_12[] = {
	&aO_12[0],
	&aO_12[1],
	&aO_12[2],
	&aO_12[3],
	&aO_12[4],
	&aO_12[5],
	&aO_12[6],
};

static OO_aCOLLECTION	O_12= {
	-12, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	7,
	0,
	0,
	xO_12,
};

/* Reference class structure */
GLOBALDEF OO_CLASS	O3[1] = {
	OC_REFERENCE, {0x3},
	OC_CLASS,
	ERx("Reference"),
	0,_,1,
	ERx("ReferenceClass"),
	_,_,_,
	ERx("iireference"),
	ERx("id"),
	(OOID)1,
	1,
	1,
	0,
	sizeof(OO_REFERENCE),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)-11,
	(OOID)-10,
	(OOID)-12,
};

/*
**	Method -- Class object initialization
*/
/* Method methods array */
static OO_METHOD	mO4[] = {
    {112,{0x3},OC_METHOD,_init,0,0,0,0,0,0,0,4,
		ERx("Minit"),Minit,8,1,0,0,0},
    {118,{0x3},OC_METHOD,_initDb,0,0,0,0,0,0,0,4,
		ERx("MinitDb"),MinitDb,8,1,0,0,0},
};

/* Method methods collection */
static OOID	xO_13[] = {
	(OOID)112,
	(OOID)118,
};

static OO_COLLECTION	O_13= {
	-13, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_13)/sizeof(xO_13[0]),
	0,
	0,
	xO_13,
};

/* Method detail refs collection (entries in a masterRefs array) */
static OOID	xO_14[] = {
	(OOID)10,
};

static OO_COLLECTION	O_14= {
	-14, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_14)/sizeof(xO_14[0]),
	0,
	0,
	xO_14,
};

/* Method attributes array */
static OO_ATTRIB	aO_15[] = {
	{ 2, ERx("mclass"), 30, 2},
	{ 3, ERx("procname"), 37, 22},
	{ 4, ERx("entrypt"), 5, 8},
	{ 5, ERx("argcount"), 30, 1},
	{ 6, ERx("defaultperm"), 30, 1},
	{ 7, ERx("keepstatus"), 30, 1},
	{ 8, ERx("fetchlevel"), 30, 1},
	{ 9, ERx("classmeth"), 30, 1},
};

/* Method attributes collection */
static OO_ATTRIB	*xO_15[] = {
	&aO_15[0],
	&aO_15[1],
	&aO_15[2],
	&aO_15[3],
	&aO_15[4],
	&aO_15[5],
	&aO_15[6],
	&aO_15[7],
};

static OO_aCOLLECTION	O_15= {
	-15, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	8,
	0,
	0,
	xO_15,
};

/* Method class structure */
GLOBALDEF OO_CLASS	O4[1] = {
	OC_METHOD, {0x3},
	OC_CLASS,
	ERx("Method"),
	0,_,1,
	ERx("MethodClass"),
	_,_,_,
	ERx("iimethod"),
	ERx("id"),
	(OOID)1,
	1,
	1,
	0,
	sizeof(OO_METHOD),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)-14,
	(OOID)-13,
	(OOID)-15,
};

/*
**	Collection -- Class object initialization
*/
/* Collection methods array */
static OO_METHOD	mO5[] = {
    {139,{0x3},OC_METHOD,_at,0,0,0,0,0,0,0,5,
		ERx("iioCOat"),iioCOat,0,1,0,1,0},
    {38,{0x3},OC_METHOD,_atEnd,0,0,0,0,0,0,0,5,
		ERx("iioCOxatEnd"),(OOID (*)()) iioCOxatEnd,0,1,0,1,0},
    {140,{0x3},OC_METHOD,_atPut,0,0,0,0,0,0,0,5,
		ERx("iioCOpatPut"),iioCOpatPut,0,1,0,1,0},
    {40,{0x3},OC_METHOD,_currentName,0,0,0,0,0,0,0,5,
		ERx("iioCOcurrentName"),(OOID (*)()) iioCOcurrentName,0,1,0,1,0},
    {186,{0x3},OC_METHOD,_do,0,0,0,0,0,0,0,5,
		ERx("iioCOdo"),iioCOdo,-1,1,0,1,0},
    {41,{0x3},OC_METHOD,_first,0,0,0,0,0,0,0,5,
		ERx("iioCOfirst"),iioCOfirst,0,1,0,1,0},
    {152,{0x3},OC_METHOD,_init,0,0,0,0,0,0,0,5,
		ERx("iioCOinit"),iioCOinit,1,1,0,0,0},
    {42,{0x3},OC_METHOD,_isEmpty,0,0,0,0,0,0,0,5,
		ERx("iioCOisEmpty"),(OOID (*)()) iioCOisEmpty,0,1,0,1,0},
    {43,{0x3},OC_METHOD,_next,0,0,0,0,0,0,0,5,
		ERx("iioCOnext"),iioCOnext,0,1,0,1,0},
    {187,{0x3},OC_METHOD,_print,0,0,0,0,0,0,0,5,
		ERx("iioCOprint"),(OOID (*)()) iioCOprint,0,1,0,1,0},
    {111,{0x3},OC_METHOD,_alloc,0,0,0,0,0,0,0,1,
		ERx("iioCOalloc"),iioCOalloc,1,1,0,1,1},
};

/* Collection methods collection */
static OOID	xO_8[] = {
	(OOID)139,
	(OOID)38,
	(OOID)140,
	(OOID)40,
	(OOID)186,
	(OOID)41,
	(OOID)152,
	(OOID)42,
	(OOID)43,
	(OOID)187,
	(OOID)111,
};

static OO_COLLECTION	O_8= {
	-8, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_8)/sizeof(xO_8[0]),
	0,
	0,
	xO_8,
};

/* Collection attributes array */
static OO_ATTRIB	aO_9[] = {
	{ 2, ERx("size"), 30, 2},
	{ 3, ERx("current"), 30, 2},
	{ 4, ERx("atend"), 30, 1},
};

/* Collection attributes collection */
static OO_ATTRIB	*xO_9[] = {
	&aO_9[0],
	&aO_9[1],
	&aO_9[2],
};

static OO_aCOLLECTION	O_9= {
	-9, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	3,
	0,
	0,
	xO_9,
};

/* Collection class structure */
GLOBALDEF OO_CLASS	O5[1] = {
	OC_COLLECTION, {0x3},
	OC_CLASS,
	ERx("Collection"),
	0,_,1,
	ERx("CollectionClass"),
	_,_,_,
	ERx("iicollection"),
	ERx("id"),
	(OOID)1,
	1,
	0,
	1,
	sizeof(OO_COLLECTION),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-8,
	(OOID)-9,
};

/*
**	Encoding -- Class object initialization
*/
/* Encoding methods array */
static OO_METHOD	mO12[] = {
    {13,{0x3},OC_METHOD,_init,0,0,0,0,0,0,0,12,
		ERx("ENinit"),ENinit,2,1,0,0,0},
    {14,{0x3},OC_METHOD,_initDb,0,0,0,0,0,0,0,12,
		ERx("ENinit"),ENinit,2,1,0,0,0},
    {18,{0x3},OC_METHOD,_fetch,0,0,0,0,0,0,0,12,
		ERx("ENfetch"),ENfetch,1,1,0,0,0},
    {19,{0x3},OC_METHOD,ii_flushAll,0,0,0,0,0,0,0,12,
		ERx("ENflush"),ENflush,1,1,0,0,0},
    {23,{0x3},OC_METHOD,_decode,0,0,0,0,0,0,0,12,
		ERx("ENdecode"),ENdecode,0,1,0,1,0},
    {32,{0x3},OC_METHOD,_destroy,0,0,0,0,0,0,0,12,
		ERx("ENdestroy"),ENdestroy,0,1,0,1,0},
    {34,{0x3},OC_METHOD,_readfile,0,0,0,0,0,0,0,12,
		ERx("ENreadfile"),ENreadfile,1,1,0,1,1},
    {35,{0x3},OC_METHOD,_writefile,0,0,0,0,0,0,0,12,
		ERx("ENwritefile"),ENwritefile,1,1,0,1,0},
};

/* Encoding methods collection */
static OOID	xO_16[] = {
	(OOID)13,
	(OOID)14,
	(OOID)18,
	(OOID)19,
	(OOID)23,
	(OOID)32,
	(OOID)34,
	(OOID)35,
};

static OO_COLLECTION	O_16= {
	-16, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	sizeof(xO_16)/sizeof(xO_16[0]),
	0,
	0,
	xO_16,
};

/* Encoding attributes array */
static OO_ATTRIB	aO_17[] = {
	{ 2, ERx("encode_object"), 30, 4},
	{ 3, ERx("encode_sequence"), 30, 2},
	{ 4, ERx("encode_estring"), 37, 1992},
};

/* Encoding attributes collection */
static OO_ATTRIB	*xO_17[] = {
	&aO_17[0],
	&aO_17[1],
	&aO_17[2],
};

static OO_aCOLLECTION	O_17= {
	-17, {0x3},
	OC_COLLECTION,
	_,0,_,1,_,_,_,_,
	3,
	0,
	0,
	xO_17,
};

/* Encoding class structure */
GLOBALDEF OO_CLASS	O12[1] = {
	OC_ENCODING, {0x3},
	OC_CLASS,
	ERx("Encoding"),
	0,_,1,
	ERx("EncodedObjectClass"),
	_,_,_,
	ERx("ii_encodings"),
	ERx("object_id"),
	(OOID)1,
	1,
	0,
	0,
	sizeof(OO_ENCODING),
	(OOID)-1,
	(OOID)-1,
	(OOID)0,
	(OOID)0,
	(OOID)-16,
	(OOID)-17,
};

/*
**	UnknownRef -- Class object initialization
*/
/* UnknownRef class structure */
GLOBALDEF OO_CLASS	O75[1] = {
	OC_UNKNOWNREF, {0x3},
	OC_CLASS,
	ERx("UnknownRef"),
	0,_,1,
	ERx("Special class for UNKNOWN Reference"),
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
**	JoinDef -- Class object initialization
*/
/* JoinDef class structure */
GLOBALDEF OO_CLASS	O1002[1] = {
	OC_JOINDEF, {0x3},
	OC_CLASS,
	NULL,
	F_OO0010_JoinDef,_,1,
	ERx("JoinDef Class"),
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
**	Report -- Class object initialization
*/
/* Report class structure */
GLOBALDEF OO_CLASS	O1501[1] = {
	OC_REPORT, {0x3},
	OC_CLASS,
	NULL,
	F_OO0011_Report,_,1,
	ERx("generic Report Class"),
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
**	RWRep -- Class object initialization
*/
/* RWRep class structure */
GLOBALDEF OO_CLASS	O1502[1] = {
	OC_RWREP, {0x3},
	OC_CLASS,
	NULL,
	F_OO0012_RWRep,_,1,
	ERx("Report-Writer Report Class"),
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
**	RBFRep -- Class object initialization
*/
/* RBFRep class structure */
GLOBALDEF OO_CLASS	O1511[1] = {
	OC_RBFREP, {0x3},
	OC_CLASS,
	NULL,
	F_OO0013_RBFRep,_,1,
	ERx("RBF Report Class"),
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
**	Form -- Class object initialization
*/
/* Form class structure */
GLOBALDEF OO_CLASS	O1601[1] = {
	OC_FORM, {0x3},
	OC_CLASS,
	NULL,
	F_OO0014_Form,_,1,
	ERx("Form Class"),
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
**	UnProc -- Class object initialization
*/
/* UnProc class structure */
GLOBALDEF OO_CLASS	O2190[1] = {
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
	(OOID)0,
	(OOID)0,
};

/*
**	QBFName -- Class object initialization
*/
/* QBFName class structure */
GLOBALDEF OO_CLASS	O2201[1] = {
	OC_QBFNAME, {0x3},
	OC_CLASS,
	NULL,
	F_OO0015_QBFName,_,1,
	ERx("QBF Name Class"),
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
**	UnFrame -- Class object initialization
*/
/* UnFrame class structure */
GLOBALDEF OO_CLASS	O2250[1] = {
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
GLOBALDEF OO_CLASS	O3001[1] = {
	OC_AFORMREF, {0x3},
	OC_CLASS,
	ERx("AFORMREF"),
	0,_,1,
	ERx("ABF Form Reference Class"),
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
**	start-up object list
*/

# if defined(NT_GENERIC)
FACILITYREF OO_OBJECT	iiOOnil[];
# else              /* NT_GENERIC  */
GLOBALREF OO_OBJECT	iiOOnil[];
# endif             /* NT_GENERIC  */
GLOBALDEF OO_OBJECT	*IIooObjects[] = {
	iiOOnil,
	(OO_OBJECT *)&mO1[0],
	(OO_OBJECT *)&mO1[1],
	(OO_OBJECT *)&mO1[2],
	(OO_OBJECT *)&mO1[3],
	(OO_OBJECT *)&mO1[4],
	(OO_OBJECT *)&mO1[5],
	(OO_OBJECT *)&mO1[6],
	(OO_OBJECT *)&mO1[7],
	(OO_OBJECT *)&mO1[8],
	(OO_OBJECT *)&mO1[9],
	(OO_OBJECT *)&mO1[10],
	(OO_OBJECT *)&mO1[11],
	(OO_OBJECT *)&mO1[12],
	(OO_OBJECT *)&mO1[13],
	(OO_OBJECT *)&mO1[14],
	(OO_OBJECT *)&mO1[15],
	(OO_OBJECT *)&mO1[16],
	(OO_OBJECT *)&mO1[17],
	(OO_OBJECT *)&mO1[18],
	(OO_OBJECT *)&mO1[19],
	(OO_OBJECT *)&mO1[20],
	(OO_OBJECT *)&mO1[21],
	(OO_OBJECT *)&mO1[22],
	(OO_OBJECT *)&mO1[23],
	(OO_OBJECT *)&mO1[24],
	(OO_OBJECT *)&mO1[25],
	(OO_OBJECT *)&mO1[26],
	(OO_OBJECT *)&mO1[27],
	(OO_OBJECT *)&mO1[28],
	(OO_OBJECT *)&mO1[29],
	(OO_OBJECT *)&mO1[30],
	(OO_OBJECT *)&mO1[31],
	(OO_OBJECT *)&mO1[32],
	(OO_OBJECT *)&mO1[33],
	(OO_OBJECT *)&mO1[34],
	(OO_OBJECT *)&mO1[35],
	(OO_OBJECT *)&O_2,
	(OO_OBJECT *)&O_3,
	(OO_OBJECT *)&O_4,
	(OO_OBJECT *)O1,
	(OO_OBJECT *)&rO2[0],
	(OO_OBJECT *)&rO2[1],
	(OO_OBJECT *)&rO2[2],
	(OO_OBJECT *)&rO2[3],
	(OO_OBJECT *)&rO2[4],
	(OO_OBJECT *)&rO2[5],
	(OO_OBJECT *)&O_5,
	(OO_OBJECT *)&O_6,
	(OO_OBJECT *)&O_7,
	(OO_OBJECT *)O2,
	(OO_OBJECT *)&mO3[0],
	(OO_OBJECT *)&mO3[1],
	(OO_OBJECT *)&O_10,
	(OO_OBJECT *)&O_11,
	(OO_OBJECT *)&O_12,
	(OO_OBJECT *)O3,
	(OO_OBJECT *)&mO4[0],
	(OO_OBJECT *)&mO4[1],
	(OO_OBJECT *)&O_13,
	(OO_OBJECT *)&O_14,
	(OO_OBJECT *)&O_15,
	(OO_OBJECT *)O4,
	(OO_OBJECT *)&mO5[0],
	(OO_OBJECT *)&mO5[1],
	(OO_OBJECT *)&mO5[2],
	(OO_OBJECT *)&mO5[3],
	(OO_OBJECT *)&mO5[4],
	(OO_OBJECT *)&mO5[5],
	(OO_OBJECT *)&mO5[6],
	(OO_OBJECT *)&mO5[7],
	(OO_OBJECT *)&mO5[8],
	(OO_OBJECT *)&mO5[9],
	(OO_OBJECT *)&mO5[10],
	(OO_OBJECT *)&O_8,
	(OO_OBJECT *)&O_9,
	(OO_OBJECT *)O5,
	(OO_OBJECT *)&mO12[0],
	(OO_OBJECT *)&mO12[1],
	(OO_OBJECT *)&mO12[2],
	(OO_OBJECT *)&mO12[3],
	(OO_OBJECT *)&mO12[4],
	(OO_OBJECT *)&mO12[5],
	(OO_OBJECT *)&mO12[6],
	(OO_OBJECT *)&mO12[7],
	(OO_OBJECT *)&O_16,
	(OO_OBJECT *)&O_17,
	(OO_OBJECT *)O12,
	(OO_OBJECT *)O75,
	(OO_OBJECT *)O1002,
	(OO_OBJECT *)O1501,
	(OO_OBJECT *)O1502,
	(OO_OBJECT *)O1511,
	(OO_OBJECT *)O1601,
	(OO_OBJECT *)O2190,
	(OO_OBJECT *)O2201,
	(OO_OBJECT *)O2250,
	(OO_OBJECT *)O3001,
	0,
};
GLOBALDEF OOID	TempId = -49;
GLOBALDEF OOID	KernelTempId = -10;
