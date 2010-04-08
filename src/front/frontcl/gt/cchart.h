/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	ccht.h
**
**	header file for c-chart routines
**	(c) 1983, 1984 Visual Engineering, Inc.
**	sccs id @(#)cchart.h	3.9	12/3/85
*/

#ifdef FLEXNAMES
#include "cchlong.h"
#endif

#ifdef vms
#define SAVER	".bin]savegraf"
#else
#define SAVER	"bin/savegraf"
#endif


#define ALL	        -1
#define MXDTSETS	50
#define MAXCLRS	         8

#define CNONE           -1  /* FILL AREA KND OF NONE */
#define NONEFA          -1  /* FILL AREA KND OF NONE */
#define NONELN          -1  /* LINE KND OF NONE */

#define SAMEI		-32000
#define SAMER		1e-30

#define UNSETIT		0
#define SETIT		1

#define NOTSET		1e-30  /* USED IN LBLSPOS WHEN ELEMENT NOT USED */

/* PREVIEW MODES */
#define QUICKPRE         0  /* FA PREVIEW MODE */
#define MEDIUMPRE        1  /* MIDDLE PREVIEW MODE */
#define VCTRBWPRE        2  /* VECTOR BW MODE */
#define VCTRPRE          3  /* VECTOR FINAL COPY MODE */
#define FINALPRE         4  /* FINAL COPY MODE */


/* GRAPH TYPES */
#define BAR              0
#define PIE              1
#define CURVE		 2  /* Same as the CV_STRAIGHT enum */
#define CV_STRAIGHT      2  /* straight lnes between points */
#define CV_STEP          3  /* steps between points */
#define CV_QSPLINE       4  /* fit cubic splne to points */
#define CV_REGRESS       5  /* do a lnear regression fit on points */
#define CV_HILO          6  /* do a hi lo needle chart fit on points */
#define CV_HILOAVG       7  /* do a hi lo needle chart fit on points */
#define CV_CROSS         8  /* do cross member at each point */
#define CV_SCATTER       9  /* do markers only */
#define CV_BSPLINE      10  /* fit b-splne to points */

#define GLOBALGRAPH	99

/* HEADING PATH */
#define NORMALPATH       TP_RIGHT
#define DOWNPATH         TP_DOWN

/* ORIENTATION */
#define HORIZ		0
#define VERT		1

/* FILL CURVE */
#define F_BELOW         0
#define F_ABOVE         1
#define F_BETWEEN	2

/* BULLET KINDS */
#define DOT		64
#define SQUARE		35
#define DIAMOND		94
#define USERDEF		126

/* POSITION / LOCATION */
#define GLEFT            0
#define GRIGHT           1
#define GUP              2
#define GDOWN            3


/* INDIVIDUAL ANNOTATION PLACEMENT */
#define EXTERNAL         1
#define BOTTOM           2
#define MIDDLE           3
#define TOPOFBAR         4
#define BELOW            5


/* INDIVIDUAL ANNOTATION JUSTIFICATION */
#define CENTERED         0
#define INWARD           1
#define OUTWARD          2
#define LEFT             3
#define RIGHT            4


/* TICK TYPE */
#define TK_MAJOR         0
#define TK_MINOR         1

/* TICK KIND */
#define OUTTIK           0
#define INTIK            1
#define CROSSTIK         2

/* NUMBER FORMAT */
#define NORMALFMT        0
#define COMMAFMT         1
#define SPACEFMT         2
#define EUROPEFMT        3

/* CROSSED AXIS TYPES */
#define CROSSX           0
#define CROSSY           1

/* AXIS KND */
#define LINEARAX         0
#define LOGAX            1
#define LABELAX          2
#define MONTHAX          3

/* DB_DTE_TYPE NAME TYPE */
#define NUMBER		0
#define DAYNAME 	1
#define MONTHNAME	2
#define QUARNAME	3
#define QUARROMAN	4
#define QUARNUMB	5
#define QUARMONTH	6


/* DB_DTE_TYPE FORMAT */
#define UANAME		0
#define MANAME		1
#define LANAME		2
#define UTNAME		3
#define MTNAME		4
#define LTNAME		5
#define USNAME		6
#define MSNAME		7
#define LSNAME		8

/* AXIS LABEL SUPPRESSION */
#define NOSUPR           0  /* don't suppress any labels */
#define ALLSUPR          1  /* suppress all labels */
#define ENDSUPR          2  /* suppress first and last label on axis */
#define MIDSUPR          3  /* only print first and last label on axis */
#define FIRSTSUPR        4  /* suppress first label */
#define LASTSUPR         5  /* suppress last label */


/* Pie styles */
#define STDPIE           0
#define RADPIE           1
#define AREAPIE          2

/* JUSTIFICATION */
#define CENTERJUST       0
#define LEFTJUST         1
#define RIGHTJUST        2
#define TYPEJUST         3  /* left and right justified by char spacing */
#define TOPJUST          4
#define BOTJUST          5

#define	NOOBJ	      -99
#define	LC_MESSAGE	0
#define	LC_LEGEND	1
#define	LC_TITLE	2
#define	LC_PIE		3
#define	LC_AXLABEL	4
#define	LC_AXHEADER	5
#define	LC_AXAREA	6
#define	LC_BARLBL	7
#define	LC_CURVE	8
#define	LC_BAR		9
#define	LC_SUBPLOT	10
#define	LC_BORDER	11

/* Boolean */
#define	TRUE		1
#define	FALSE		0

/* data type for use with cchart extent locator functions */
typedef struct {
	Gchar opcode;   /* see LC_* above */
	Gint detail1;	/* detail such as set#, axis#, message# */
	Gint detail2;	/* detail such as ptnm#, axlabel# */
	Gextent extent;	/* extent of cchart object at location */
} Glocobject;



/* data type for Gfloat prec, GKS uses double prec */

typedef Gint Cerror;
typedef Gint Gbool;
typedef Gfloat CGfloat;

/* MARGIN */

typedef struct {
	Gfloat right,
	left,
	top,
	bottom;
} Cmarg;


/* Axis Boundary */
typedef struct {
	Gfloat strt,
		end;
} Cbound;

/* Transform structure */
typedef struct {
	Gint nxfnm1,
	nxfnm2;
} Ctran;


/* Text bundle structure */

typedef struct {
	Gint clr;
	Gint  fnt;
	Gfloat  ht;
} Ctxbd;

/* Line bundle structure */

typedef struct {
	Gflbundl flbundl;
	Glnbundl lnbundl;
} FALN;

typedef struct { /* PROCHART (L)OOK (U)P (T)ABLE ENTRY */
	Gfloat red;
	Gfloat green;
	Gfloat blue;
	Gchar name[12];
} LUT;


#define NO_ERR		100
#define CERRRNG		101
#define CERRTRUE	102
#define CERRMXSLN	103
#define CERRINTER	104
#define CERRCLR		105
#define CERRSTYL	106
#define CERRLNTP	107
#define CERRWDTH	108
#define CERRFNT		109
#define CERRHT		110
#define NO_MEM		111

#define CERRAXRNG	200
#define CERRTKTP	201
#define CERRLBRNG	202

#define CERRDTSET	300
#define CERRDTTP	301
#define CERRXAX		302
#define CERRYAX		303
#define CERRDTIND	304
#define CERRMKTP	305
#define CERRMKSZ	306

#define CERRMSG		400
#define CERRGRAPH	500

