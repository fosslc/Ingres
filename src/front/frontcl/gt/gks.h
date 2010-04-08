/*
**	sccs id @(#)gks.h	3.14	12/19/85
**
**	gks.h
**	GKS (ANSI C) User Header File
**	(C) Copyright (c) 2004 Ingres Corporation
** History:
**	08-Mar-2007 (kiria01) b117892
**	    Renamed Gattr (unused?) enum value DB_TXT_TYPE to
**	    GKS_TXT_TYPE to avoid namespace clash with iicommon.h
*/

#include "vdi.h"
#ifdef vms
#include "vmsname.h"
#else
#ifdef FLEXNAMES
#include "gkslong.h"
#endif
#endif

/*--------------------------------------*/
/*		BASIC TYPES		*/
/*--------------------------------------*/



typedef char		Gchar;		/* Gks DB_CHR_TYPE type */

typedef long int 	Glong;		/* LONG integer */

typedef int 		Gint;		/* INTeger */

typedef double		Gfloat;		/* FLOATing point type */





#ifndef INTERNAL
typedef Gchar		Gws;  		/* Work station */
#endif

typedef Gfloat		Gwc;		/* World Coordinates */

typedef Gfloat		Gdc;		/* Device Coordinates */

typedef Gfloat		Gndc;		/* Normalized Device Cooridnates */

typedef Gint		Gerror;		/* ERROR code */

typedef Gint		Gpet;		/* Prompt and Echo Type */

typedef Gint		Gcolour;	/* COLOR index type */

typedef	Gfloat		Gchrsp;		/* CHaRacter SPacing type */

typedef	Gfloat		Gchrexp;	/* CHaRacter EXPansion factor type */

typedef	Gwc		Gchrht; 	/* CHaRacter HeighT type */

typedef Gfloat		Ginten;		/* color INTENisty */

typedef Gint		Gindex;		/* bundle INDEX */

typedef Gint		Gidevno;	/* Input DEVice Number */

typedef Gfloat		Gsegpri;	/* SEGment PRIority */

typedef Gfloat		Gscale;		/* SCALE factor */

typedef Gint		Gpickid;	/* PICK IDentifier */





/*----------------------------------------------*/
/*		ENUMERATED TYPES		*/
/*----------------------------------------------*/


typedef Gint		Gmktype;	/* MarK TYPE */
typedef Gint		Glntype;	/* LiNe TYPE */


/* hatch styles */

#define HF_TIGHT	1	/* tight (essentially solid) hatch style*/
#define HF_RSTRP	2	/* 45 deg positive slope stripe		*/
#define HF_LSTRP	3	/* 45 deg negative slope stripe		*/
#define HF_RDSTRP	4	/* 45 deg positive slope double stripe	*/
#define HF_LDSTRP	5	/* 45 deg positive slope double stripe	*/
#define HF_SQ		6	/* 45 deg square criss-cross		*/
#define HF_DBLSQ	7	/* 45 deg double square criss-cross	*/
#define HF_DIAG		8	/* 30 deg positive slope stripe		*/
#define HF_DBLDIAG	9	/* 30 deg positive slope double stripe	*/

/* following aren't supported */
#define HF_HDIAG	10	/* criss-cross; +60 deg and -60 deg	*/
#define HF_DBLHDIAG	11	/* double criss-cross as above		*/
#define HF_RSQ		12	/* 90 deg sq criss-cross, thickness 1	*/
#define HF_RSQ2		13	/* 90 deg sq criss-cross, thickness 2	*/
#define HF_RSQ3		14	/* 90 deg sq criss-cross, thickness 3	*/
#define HF_RSQW		15	/* 90 deg sq criss-cross, thickness 4	*/
#define HF_DBLRSQ	16	/* double 90 deg square criss-cross	*/
#define HF_TRPLRSQ	17	/* triple 90 deg square criss-cross	*/
#define HF_LOWDIAG	18	/* criss-cross; +15 deg and -15 deg	*/
#define HF_HIDIAG	19	/* criss-cross; +75 deg and -75 deg	*/
#define HF_VERT		20	/* tight (vertical) hatch style		*/

typedef Gint		Gflstyle;	/* FiLl STYLE */



/* font types */

#define ASCIIFT			1	/* ASCII FonT			*/
#define H_CARTO_SIMPLEX		2	/* Ascii UNiplex SiMplex RoMan Up case*/
#define H_UNI_SIMPLEX		3	/* Ascii UNiplex SiMplex RoMan	*/
#define H_DUP_COMPLEX		4	/* Ascii DUplex COmplex RoMan	*/
#define H_DUP_SIMPLEX		5	/* Ascii DUplex SiMplex RoMan	*/
#define H_TRI_COMPLEX		6	/* Ascii TRiplex COmplex RoMan	*/
#define F_MED_HELV		7	/* Ascii MEDium HELvetica	*/
typedef Gint		Gtxfont;	/* TeXt FONT */




/* dynamic modification type */

typedef enum {
	IRG,
	IMM
} Gmodtype;



/* FiLl area INTERior style */
typedef Gint Gflinter;


/* TOGGLE switch */

typedef enum {
	OFF,
	ON
} Gtoggle;



/* CLIPping indicator */

typedef enum {
		CLIP,
		NOCLIP
} Gclip;



/* New FRAME action at update */

typedef enum {
	 YES,
	 NO
} Gnframe;



/* GKS Operating State */

typedef enum {
	 GKSERR = -1,		/* GKS error (internal use)	*/
	 GKCL,			/* GKS closed			*/
	 GKOP,			/* GKS open			*/
	 WSOP,			/* workstation open		*/
	 WSAC,			/* workstation active		*/
	 SGOP 			/* segment open 		*/
} Gos;



/* CLeaR control FLAG */

typedef enum {
	 CONDITIONALLY,
	 ALWAYS
} Gclrflg;



/* COlor AVAILability */

typedef enum {
	 COLOUR,
	 MONOCHROME
} Gcoavail;



/* Aspect Source FlagS */

typedef enum {
	INDIVIDUAL,
	BUNDLED
} Gasf;



/* ATTRibuteS used */

typedef enum {
	POLYLINE,
	POLYMARKER,
	GKS_TXT_TYPE,
	FILLAREA
} Gattrs;



/* Bundle Index Info */

typedef enum {
	DEFINED,
	UNDEFINED
} Gbis;		/* Bundle Index Setting 	*/



/* COlor values VALID */

typedef enum {
	 VALID,
	 INVALID
} Gcovalid;



/* Coordinate SWitch */

typedef enum {
	WC,
	NDC
} Gcsw;



/* DiSPlay SURFace */

typedef enum {
	EMPTY,
	NOTEMPTY
} Gdspsurf;



/* DEVice coordinate UNITS */

typedef enum {
	DC_METRES,
	DC_OTHER
} Gdevunits;



/* Implicit Regeneration MODE */

typedef enum {
	D_ALLOWED,
	D_SUPPRESSED
} Girgmode;



/* Event CLASS */

typedef enum {
	E_NONE,
	E_LOCATOR,
	E_STROKE,
	E_VALUATOR,
	E_CHOICE,
	E_PICK,	
	E_STRING
} Geclass;



/* viewport input priority */

typedef enum {
	HIGHER,
	LOWER
} Gvpri;



/* WorkStation CATegory */

/*
typedef enum {
	OUTPUT,
	OUTIN,
	INPUT,
	MO,
	MI,
	WISS
} Gwscat;
*/

typedef Gint Gwscat;


/* WorkStation CLASSification */

typedef Gint Gwsclass;


/* WorkStation STATE */

typedef enum {
	ACTIVE,
	INACTIVE
} Gwsstate;



/* WorkStation TRANsformation Update State */

typedef enum {
	PENDING,
	NOTPENDING
} Gwstus;




/* Implicit ReGeneration MODE */

typedef enum {
	ASAP,
	BNIG,
	BNIL,
	ASTI
} Gdefmode;



/* Input MODE */

typedef enum {
	REQUEST,
	SAMPLE,
	EVENT
} Gimode;



/* INput CLASS */

typedef enum {
	IC_LOCATOR,
	IC_STROKE,
	IC_VALUATOR,
	IC_CHOICE,
	IC_PICK,
	IC_STRING
} Giclass;



/* Echo SWitch */

typedef enum {
	ECHO,
	NOECHO
} Gesw;



/* SEGment HIlighting */

typedef enum {
	NORMAL,
	HIGHLIGHTED
} Gseghi;



/* SEGment DETectability */

typedef enum {
	UNDETECTABLE,
	DETECTABLE
} Gsegdet;


/* REGENeration flag */

typedef enum {
	PERFORM,
	SUPPRESS
} Gregen;



/* Inquiry TYPE */

typedef enum {
	SET,
	REALIZED
} Gitype;

/* Pick STATus */

typedef enum {
	OKPICK,
	NOPICK,
	NONEPICK
} Gpstat;

/* Polyline / Fill area Control Flag */

typedef enum {
	PF_POLYLINE,
	PF_FILLAREA
} Gpfcf;




/* TeXt alignment VERtical component */

typedef Gint	Gtxver;



/* TeXt alignment HORizontal component */

typedef Gint	Gtxhor;


/* request STATUS */

typedef enum {
	OK,
	NONE
} Gstatus;



/* SIMULtaneous EVents */

typedef enum {
	MORE,
	NOMORE
} Gsimultev;



/* SEGment VISibility */

typedef enum {
	VISIBLE,
	INVISIBLE
} Gsegvis;



/* TeXt PRECision */

typedef Gint Gtxprec;


/* TeXt PATH */

typedef Gint Gtxpath;


/* Attribute Control Flag */

typedef enum {
	CURRENT,
	SPECIFIED
} Gacf;






/*--------------------------------------*/
/*		COORDINATES		*/
/*--------------------------------------*/



/* Normalized device coordinate POINT */

typedef struct {
	Gndc		x;		/* x coordinate			*/
	Gndc		y;		/* y coordinate			*/
} Gnpoint;



/* Device cooridinate POINT */

typedef struct {
	Gdc		x;		/* x coordinate			*/
	Gdc		y;		/* y coordinate			*/
} 
Gdpoint;



/* World Coordinate POINT */

typedef struct {
	Gwc		x;		/* x coordinate			*/
	Gwc		y;		/* y coordinate			*/
}
Gwpoint;



/* integer point */

typedef struct {
	Gint		x;		/* x coordinate			*/
	Gint		y;		/* y coordinate			*/
} 
Gipoint;





/*--------------------------------------*/
/*		RECTANGLES		*/
/*--------------------------------------*/



/* Normalized device coordinate RECTangle */

typedef struct {
	Gnpoint		ll;		/* lower left hand coordinate	*/
	Gnpoint		ur;		/* upper right hand coordinate	*/
} 
Gnrect;



/* Device cooridinate RECTangle */

typedef struct {
	Gdpoint		ll;		/* lower left hand corner	*/
	Gdpoint		ur;		/* upper right hand corner	*/
} 
Gdrect;



/* World coordinate RECTangle */

typedef struct {
	Gwpoint		ll;		/* lower left hand corner	*/
	Gwpoint		ur;		/* upper right hand corner	*/
}
Gwrect;



/* world coordinate text EXTENT rectangle */

typedef struct {
	Gwpoint		ll;		/* lower left hand corner	*/
	Gwpoint		lr;		/* lower right hand corner	*/
	Gwpoint		ur;		/* upper right hand corner	*/
	Gwpoint		ul;		/* upper left hand corner	*/
}
Gextent;





/*--------------------------------------*/
/*		SEGMENTS		*/
/*--------------------------------------*/

/* dynamic MODification of SEGment attributes */

typedef struct {
	Gmodtype	transform;	/* transformation		*/
	Gmodtype	appear;		/* appearing (--> visible)	*/
	Gmodtype	disappear;	/* disappearing (--> invisible)	*/
	Gmodtype	hilite;		/* hilight			*/
	Gmodtype	priority;	/* priority			*/
	Gmodtype	addition;	/* addition of primitives to seg*/
	Gmodtype	deletion;	/* deletion of segment		*/
} 
Gmodseg;




/* SEGment TRANsformation */

typedef struct {
	Gfloat		m11;		/* matrix element 1,1		*/
	Gfloat		m12;		/* matrix element 1,2		*/
	Gndc		m13;		/* matrix element 1,3		*/
	Gfloat		m21;		/* matrix element 2,1		*/
	Gfloat		m22;		/* matrix element 2,2		*/
	Gndc		m23;		/* matrix element 2,3		*/
} 
Gsegtran;




/* SEGment */

#define MNSEGID		0
#define MXSEGID		32000
#define SGWSDIM		11
#define	SGLISTFAC	10
#define	NOOPENSEG	-1

#define	SEGITEM		20
#define	SEGCOOR		100

/* ASF packing mask */
#define	ASFLINETYPE	000001
#define	ASFLINEWDTH	000002
#define	ASFLINECLR	000004
#define	ASFMARKTYPE	000010
#define	ASFMARKSIZE	000020
#define	ASFMARKCLR	000040
#define	ASFTEXTFP	000100
#define	ASFCHAREXP	000200
#define	ASFCHARSP	000400
#define	ASFTEXTCLR	001000
#define	ASFFILLINT	002000
#define	ASFFILLSTYL	004000
#define	ASFFILLCLR	010000

/* segment opcode header file */

#define		SENDITEM		 0
#define		SPOLYLINE		11
#define		SPOLYMARKER		12
#define		STEXT			13
#define		SFILLAREA		14
#define		SCELLARRAY		15
#define		SLINEINDEX		21
#define		SLINETYPE		22
#define		SLINEWIDTH		23
#define		SLINECOLOUR		24
#define		SMARKERINDEX		25
#define		SMARKERTYPE		26
#define		SMARKERSIZE		27
#define		SMARKERCOLOUR		28
#define		STEXTINDEX		29
#define		STEXTFONTPREC		30
#define		SCHAREXP		31
#define		SCHARSPACE		32
#define		STEXTCOLOUR		33
#define		SCHARVECT		34
#define		STEXTPATH		35
#define		STEXTALIGN		36
#define		SFILLINDEX		37
#define		SFILLINTSTYLE		38
#define		SFILLSTYLEINDEX		39
#define		SFILLCOLOUR		40
#define		SPATSIZE		41
#define		SPATREFPOINT		42
#define		SASF			43
#define		SCLIPRECT		61

typedef struct {
	Gchar op;	/* short pair structure */
	union {
		Glong	ptr;
		struct {
			short ival1,ival2;	
		} ipair;
		float	flt;
	} uval;
} Gsegitem;


typedef struct {
	float x,y;
} Gsgfpoint;

typedef union {
	Gchar	chr[8];
	Vlongxy pnt;
	Gsgfpoint	fpnt;
} Gsegcoor;

/* LONG INTEGER SEGment TRANsformation */

typedef struct {
	long		m11;		/* matrix element 1,1		*/
	long		m12;		/* matrix element 1,2		*/
	long		m13;		/* matrix element 1,3		*/
	long		m21;		/* matrix element 2,1		*/
	long		m22;		/* matrix element 2,2		*/
	long		m23;		/* matrix element 2,3		*/
	char		identity;	/* Identity flag		*/
} 
Gisegtran;


/* Segment */

typedef struct {
	Gsegtran	xfm;		/* current transformation */
	Gsegtran	reqxfm;		/* requested transformation */
	Gisegtran	invxfm;		/* save inverse transformation	*/
	Gisegtran	ixfm;		/* current integer transformation */
	Vlrect	 	box;		/* horiz extent box of segment */
	Gsegvis		vis;		/* appearing (--> visible) */
	Gseghi		hilite;		/* hilight	*/
	Gsegpri		pri;		/* priority	*/
	Gsegdet		detect;		/* detectability of seg	*/
	Glong		itemcnt;
	Glong		coorcnt;
	Glong		coorndx;
	Glong		itemndx;
	Gint		id;
	Gint		userid;
	int		assocws[SGWSDIM];
	Gsegitem	*list;
	Gsegcoor	*coor;
} Gseg;






/*------------------------------*/
/*		DB_TXT_TYPE		*/
/*------------------------------*/


/* TeXt ALIGNment */

typedef struct { 
	Gtxhor		hor;		/* horizontal component		*/
	Gtxver		ver;		/* vertical component		*/
}
Gtxalign;




/* TeXt Font & Precision pair */

typedef struct {
	Gtxfont		font;		/* text font			*/
	Gtxprec		prec;		/* text precision		*/
}
Gtxfp;














/*---------------------------------------*/
/*		 BUNDLES		 */
/*---------------------------------------*/




/* Bundle Index Info */

typedef struct {
	Gint		number;		/* number of bundle 		*/
	Gbis		*defined;	/* defined bundle 		*/
} 
Gbii;




/* FiLl area BUNDLe */

typedef struct {
	Gflinter	inter;		/* fill area interior style	*/
	Gflstyle	style;		/* fill area style index	*/
	Gcolour		colour;		/* fill area color index 	*/
} 
Gflbundl;




/* polyLiNe BUNDLe */

typedef struct {
	Glntype		type;		/* line type			*/
	Gscale		width;		/* line width scale factor	*/
	Gcolour		colour;		/* polyline color index		*/
} 
Glnbundl;




/* polyMarKer BUNDLe */

typedef struct {
	Gmktype		type;		/* marker type 			*/
	Gscale		size;		/* marker size scale factor	*/
	Gcolour		colour;		/* polymarker color index	*/
} 
Gmkbundl;




/* PaTtern BUNDLe */

typedef struct {
	Gipoint		size;		/* pattern array size		*/
	Gcolour		*array;		/* pattern array		*/
} 
Gptbundl;




/* TeXt BUNDLe */

typedef struct {
	Gtxfp		fp;		/* font and precision		*/
	Gchrexp		exp;		/* char expansion		*/
	Gchrsp		space;		/* char spacing			*/
	Gcolour		colour;		/* text color			*/
}
Gtxbundl;




/* COlor BUNDLe */

typedef struct {
	Ginten		red;		/* red intensity 		*/
	Ginten		green;		/* green intensity 		*/
	Ginten		blue;		/* blue intensity 		*/
} 
Gcobundl;







/*------------------------------*/
/*		INPUT		*/
/*------------------------------*/



/* NUMber of input DEVices */

typedef struct {
	Gint		locator;	/* locators			*/
	Gint		stroke;		/* strokes			*/
	Gint		valuator;	/* valuators			*/
	Gint		choice;		/* choices			*/
	Gint		pick;		/* picks			*/
	Gint		string;		/* strings			*/
} 
Gnumdev;


/*--------------------------------------*/
/*		LOCATOR			*/
/*--------------------------------------*/

/* LOCator data RECord */

typedef struct {
	Gpfcf		pfcf;		/* polyline fa control flag	*/
	Gacf		acf;		/* attribute control flag	*/
	Gasf		ln_type;	/* line type ASF		*/
	Gasf		ln_width;	/* line width ASF		*/
	Gasf		ln_colour;	/* line color ASF		*/
	Gasf		fl_inter;	/* fa interior style ASF	*/
	Gasf		fl_style;	/* fa style index ASF		*/
	Gasf		fl_colour;	/* fa color ASF			*/
	Gindex		line;		/* polyline index		*/
	Glnbundl	lnbundl;	/* polyline bundle		*/
	Gindex		fill;		/* fa index			*/
	Gflbundl	flbundl;	/* fa bundle			*/
} 
Glocrec;




/* LOCator data */

typedef struct {
	Gint		transform;	/* norm transform number	*/
	Gwpoint		position;	/* locator position		*/
} 
Gloc;




/* reQuest LOCator */

typedef struct {
	Gstatus		status;		/* request status		*/
	Gloc		loc;		/* locator data			*/
} 
Gqloc;




/* LOCator STate */

typedef struct {
	Gimode		mode;		/* mode				*/
	Gesw		esw;		/* echo switch			*/
	Gloc		loc;		/* locator data			*/
	Gpet		pet;		/* prompt and echo type		*/
	Gdrect		e_area;		/* echo area			*/
	Glocrec		record;		/* locator data record		*/
} 
Glocst;




/* DEFault LOCator data */

typedef struct {
	Gwpoint		position;	/* initial position 		*/
	Gint		n_pets;		/* number of prompt & echo types*/
	Gpet		*pets;		/* prompt & echo types		*/
	Gdrect		e_area;		/* echo area			*/
	Glocrec		record;		/* default locator data record	*/
} 
Gdefloc;




/*------------------------------*/
/*		PICK		*/
/*------------------------------*/



/* PICK data RECord */

typedef struct {
	Gwpoint		position;	/* Initial graphics cursor position */
} 
Gpickrec;




/* reQuest PICK */

typedef struct {
	Gpstat		status;		/* request status		*/
	Gseg		*seg;		/* segment			*/
	Gpickid		pickid;		/* pick identifier		*/
} 
Gqpick;




/* PICK data */

typedef struct {
	Gpstat		status;		/* pick status			*/
	Gseg		*seg;		/* pick segment			*/
	Gpickid		pickid;		/* pick identifier		*/
} 
Gpick;




/* PICK STate */

typedef struct {
	Gimode		mode;		/* mode				*/
	Gesw		esw;		/* echo switch			*/
	Gpick		pick;		/* pick data			*/
	Gpet		pet;		/* prompt & echo type		*/
	Gdrect		e_area;		/* echo area			*/
	Gpickrec	record;		/* pick data record		*/
} 
Gpickst;




/* DEFault PICK data */

typedef struct {
	Gint		n_pets;		/* number of prompt & echo types*/
	Gpet		*pets;		/* prompt & echo types		*/
	Gdrect		e_area;		/* echo area			*/
	Gpickrec	record;		/* default pick data record	*/
} 
Gdefpick;








/*--------------------------------------*/
/*		STRING			*/
/*--------------------------------------*/



/* reQuest STRING */

typedef struct {
	Gstatus		status;		/* request status		*/
	Gchar		*string;	/* string data			*/
} 
Gqstring;


/* STRING data RECord */

typedef struct {
	Gint		bufsiz;		/* buffer size			*/
	Gint		position;	/* position in buffer		*/
} 
Gstringrec;




/* STRING STate */

typedef struct {
	Gimode		mode;		/* mode				*/
	Gesw		esw;		/* echo switch			*/
	Gchar		*string;	/* string data			*/
	Gpet		pet;		/* prompt and echo type		*/
	Gdrect		e_area;		/* echo area			*/
	Gstringrec	record;		/* string data record		*/
} 
Gstringst;




/* DEFault STRING data */

typedef struct {
	Gint		bufsiz;		/* initial buffer size		*/
	Gint		n_pets;		/* number of prompt & echo types*/
	Gpet		*pets;		/* prompt & echo types		*/
	Gdrect		e_area;		/* echo area			*/
	Gstringrec	record;		/* default string data record	*/
} 
Gdefstring;







/*--------------------------------------*/
/*		STROKE			*/
/*--------------------------------------*/





/* STROKE data RECord */

typedef struct {
	Gint		bufsiz;		/* input buffer size 		*/
	Gint		editpos;	/* edinting position		*/
	Gwpoint		interval;	/* x,y interval			*/
	Gfloat		time;		/* time interval		*/
	Gacf		acf;		/* attribute control flag	*/
	Gasf		ln_type;	/* line type ASF		*/
	Gasf		ln_width;	/* line width ASF		*/
	Gasf		ln_colour;	/* line color ASF		*/
	Gasf		mk_type;	/* marker type ASF		*/
	Gasf		mk_size;	/* marker size ASF		*/
	Gasf		mk_colour;	/* marker color ASF		*/
	Gindex		line;		/* polyline index		*/
	Glnbundl	lnbundl;	/* polyline bundle		*/
	Gindex		mark;		/* polymarker index		*/
	Gmkbundl	mkbundl;	/* marker bundle		*/
}
Gstrokerec;




/* STROKE data */

typedef struct {
	Gint		transform;	/* norm transform number	*/
	Gint		n_points;	/* number of points in stroke	*/
	Gwpoint		*points;	/* points in stroke		*/
} 
Gstroke;




/* reQuest STROKE */

typedef struct {
	Gstatus		status;		/* request status		*/
	Gstroke		stroke;		/* stroke data			*/
} 
Gqstroke;




/* STROKE STate */

typedef struct {
	Gimode		mode;		/* mode				*/
	Gesw		esw;		/* echo switch			*/
	Gstroke		stroke;		/* stroke data			*/
	Gpet		pet;		/* prompt and echo type		*/
	Gdrect		e_area;		/* echo area			*/
	Gstrokerec	record;		/* stroke data record		*/
}
Gstrokest;




/* DEFault STROKE data */

typedef struct {
	Gint		bufsiz;		/* initial buffer size		*/
	Gint		n_pets;		/* number of prompt & echo types*/
	Gpet		*pets;		/* prompt & echo types		*/
	Gdrect		e_area;		/* echo area			*/
	Gstrokerec	record;		/* default stroke data record	*/
} 
Gdefstroke;




/*--------------------------------------*/
/*		VALUATOR		*/
/*--------------------------------------*/



/* VALuator data RECord */

typedef struct {
	Gfloat		high;		/* high range limit	*/
	Gfloat		low;		/* low range limit	*/
}
Gvalrec;




/* reQuest VALuator */

typedef struct {
	Gstatus		status;		/* status			*/
	Gfloat		val;		/* valuator data		*/
} 
Gqval;



/* VALuator STate */

typedef struct {
	Gimode		mode;		/* mode			*/
	Gesw		esw;		/* echo switch		*/
	Gfloat		val;		/* valuator data	*/
	Gpet		pet;		/* prompt and echo type	*/
	Gdrect		e_area;		/* echo area		*/
	Gvalrec		record;		/* valuator data record	*/
} Gvalst;



/* DEFault VALuator data */

typedef struct {
	Gfloat		value;		/* initial value 		*/
	Gint		n_pets;		/* number of prompt & echo types*/
	Gpet		*pets;		/* prompt & echo types		*/
	Gdrect		e_area;		/* echo area			*/
	Gvalrec		record;		/* default valuator data record	*/
} 
Gdefval;




/*--------------------------------------*/
/*		CHOICE			*/
/*--------------------------------------*/



/* CHOICE input data RECord */

typedef struct {
	Gint		number;		/* number of choices 		*/
	Gtoggle		*enable;	/* prompt enables		*/
	Gchar		**strings;	/* choice strings		*/
	Gseg		*seg;		/* choice prompt segment	*/
} 
Gchoicerec;




/* reQuest CHOICE */

typedef struct {
	Gstatus		status;		/* request status		*/
	Gint		choice;		/* choice data			*/
} 
Gqchoice;




/* CHOICE input STate */

typedef struct {
	Gimode		mode;		/* mode			*/
	Gesw		esw;		/* echo switch			*/
	Gint		choice;		/* choice data			*/
	Gpet		pet;		/* prompt and echo type		*/
	Gdrect		e_area;		/* echo area			*/
	Gchoicerec	record;		/* choice data record		*/
} 
Gchoicest;




/* DEFault CHOICE data */

typedef struct {
	Gint		choices;	/* maximum number of choices 	*/
	Gint		n_pets;		/* number of prompt & echo types*/
	Gpet		*pets;		/* prompt & echo types		*/
	Gdrect		e_area;		/* echo area			*/
	Gchoicerec	record;		/* default choice data record	*/
} 
Gdefchoice;







/*------------------------------*/
/*		MISC		*/
/*------------------------------*/




/* GKS Metafile ITem */

typedef struct {
	Gint		type;		/* item type			*/
	Gint		length;		/* list of attributes used	*/
} 
Ggksmit;




/* Aspect Source Flag structure */

typedef struct {			/* aspect source flags		*/
	Gasf		ln_type;	/* line type 			*/
	Gasf		ln_width;	/* line width 			*/
	Gasf		ln_colour;	/* line colour 			*/
	Gasf		mk_type;	/* marker type 			*/
	Gasf		mk_size;	/* marker size 			*/
	Gasf		mk_colour;	/* polymarker color 		*/
	Gasf		tx_fp;		/* text font & prec 		*/
	Gasf		tx_exp;		/* text char expansion factor	*/
	Gasf		tx_space;	/* text char spacing		*/
	Gasf		tx_colour;	/* text color			*/
	Gasf		fl_inter;	/* fill area interior style	*/
	Gasf		fl_style;	/* fill area style index	*/
	Gasf		fl_colour;	/* fill area color		*/
} 
Gasfs;






/* DEFERral state */

typedef struct {
	Gdefmode	defmode;	/* deferral mode		*/
	Girgmode	irgmode;	/* implicit regeneration mode	*/
} 
Gdefer;






/*--------------------------------------*/
/*		WS FACILITIES		*/
/*--------------------------------------*/


/* COlor FACilities */

typedef struct {
	Gint		colours;	/* number of colors		*/
	Gcoavail	coavail;	/* color availability		*/
	Gint		predefined;	/* number of predefined bundles	*/
} 
Gcofac;




/* GDP FACilities */

typedef struct {
	Gint		number;		/* number of GDP's		*/
	Gattrs		*atts;		/* list of attributes		*/
} 
Ggdpfac;




/* DiSPlay SIZE */

typedef struct {
	Gdevunits	units;		/* device coordinate units	*/
	Gdpoint		device;		/* device coordinate unit size	*/
	Gipoint		raster;		/* raster unit size		*/
} 
Gdspsize;




/* FiLl area FACilities */

typedef struct {
	Gint		n_inter;	/* number of interior styles	*/
	Gflinter	*interiors;	/* list of avail interior styles*/
	Gint		n_hatch;	/* number of hatch styles	*/
	Gflstyle	*hatchs;	/* list of avail hatch stylesu	*/
	Gint		predefined;	/* number of predefined bundles	*/
} 
Gflfac;




/* polyLiNe FACilities */

typedef struct {
	Gint		types;		/* number of line types		*/
	Gint		*list;		/* list of avalable line types	*/
	Gint		widths;		/* number of line widths	*/
	Gdc		nom;		/* nominal width 		*/
	Gdc		min;		/* minimum width 		*/
	Gdc		max;		/* maximum width 		*/
	Gint		predefined;	/* number of predefined bundles */
} 
Glnfac;




/* polyMarKer FACilities */

typedef struct {
	Gint		types;		/* number of marker types	*/
	Gmktype		*list;		/* list of avail marker types	*/
	Gint		sizes;		/* number of marker sizes	*/
	Gdc		nom;		/* nominal size			*/
	Gdc		min;		/* minimum size			*/
	Gdc		max;		/* maximum size			*/
	Gint		predefined;	/* number of predefined bundles	*/
} 
Gmkfac;




/* PaTtern FACilities */

typedef struct {
	Gint		types;		/* number of pattern types	*/
	Gint		predefined;	/* number of predefined bundles */
} 
Gptfac;




/* TeXt FACilities */

typedef struct {
	Gint		fps;		/* number of fonts and prec's	*/
	Gtxfp		*fp_list;	/* list of avail fonts & prec's	*/
	Gint		heights;	/* number of char heights	*/
	Gdc		nom_ht;		/* nominal height		*/
	Gdc		min_ht;		/* minimum height		*/
	Gdc		max_ht;		/* maximum height		*/
	Gint		expansions;	/* number of character exp fac	*/
	Gdc		nom_ex;		/* nominal expansion factor	*/
	Gdc		min_ex;		/* minimum expansion factor	*/
	Gdc		max_ex;		/* maximum expansion factor	*/
	Gint		predefined;	/* number of predefined bundles	*/
}
Gtxfac;




/* dynamic MODification of WorkStation attributes */

typedef struct {
	Gmodtype	line;		/* polyline			*/
	Gmodtype	mark;		/* polymarker			*/
	Gmodtype	text;		/* text				*/
	Gmodtype	fill;		/* fill area			*/
	Gmodtype	pat;		/* pattern			*/
	Gmodtype	colour;		/* color			*/
	Gmodtype	wstran;		/* workstation transform	*/
} 
Gmodws;




/* WorkStation Deferral and Update State */

typedef struct {
	Gdefmode	defmode;	/* deferral mode		*/
	Gdspsurf	dspsurf;	/* display surface		*/
	Girgmode	irgmode;	/* implicit regeneration mode	*/
	Gnframe		nframe;		/* new frame action at update	*/
}
Gwsdus;




/* WorkStation Connection Type */

typedef struct {
	FILE		*r;		/* read connection		*/
	FILE		*w;		/* write connection		*/
	Gchar		*type;		/* workstation type		*/
}
Gwsct;




/* WorkStation MAXimum numbers */

typedef struct {
	Gint		open;		/* number of open workstations	*/
	Gint		active;		/* number of active workstations*/
	Gint		assoc;		/* number of assoc workstations */
}
Gwsmax;




/* length of workstation tables */

typedef struct {
	Gint		line;		/* polyline tables		*/
	Gint		mark;		/* polymarker tables		*/
	Gint		text;		/* text tables			*/
	Gint		fill;		/* fill area tables		*/
	Gint		pat;		/* pattern tables		*/
	Gint		colour;		/* colour tables		*/
}
Gwstables;




/* WorkStation TRANsformation */

typedef struct {
	Gnrect		w;		/* window			*/
	Gdrect		v;		/* viewport			*/
}
Gwstran;




/* WorkStation Transformation Info */

typedef struct {
	Gwstus		wstus;		/* ws transform update state	*/
	Gwstran		request;	/* requested transformation	*/
	Gwstran		current;	/* current transformation	*/
}
Gwsti;

typedef struct {
	Gwrect		w;	/* Normalization Trans for window */
	Gnrect		v;	/* Normalization Trans for viewport */
}
Gnormtran;

/* ESCAPE FUNCTIONS */


/* ESCape STRING */

typedef struct {
	Gchar		*ws;		/* ws to send string to		*/
	Gchar		*s;		/* string to send		*/
} Gescstring;




/* ESCape PENSPEED */

typedef struct {
	Gchar		*ws;		/* ws to change penspeed on	*/
	Gfloat		speed;		/* speed [0,1] percent of max	*/
} Gescpenspeed;






/*----------------------------------------------*/
/*		GLOBAL ERROR STATE		*/
/*----------------------------------------------*/

#ifdef	GKSMAIN
Gerror		gkserror;
#endif


/*--------------------------------------*/
/*		ERROR CODES		*/
/*--------------------------------------*/

/* General */

#define NO_ERROR	0


/* States */

#define ENOTGKCL	1
#define ENOTGKOP	2
#define ENOTWSAC	3
#define ENOTSGOP	4
#define ENOTACOP	5
#define ENOTOPAC	6
#define ENOTWSOP	7
#define ENOTGWWS	8


/* Workstations */

#define EWSIDINV	20
#define ECNIDINV	21
#define EWSTPINV	22
#define ENOWSTYP	23
#define EWSISOPN	24
#define EWSNOTOP	25
#define EWSCNTOP	26
#define EWISSNOP	27
#define EWISSOPN	28
#define EWSISACT	29
#define EWSNTACT	30
#define EWSCATMO	31
#define EWSNOTMO	32
#define EWSCATMI	33
#define EWSNOTMI	34
#define EWSCATIN	35
#define EWSIWISS	36
#define EWSNOTOI	37
#define EWSNOTIO	38
#define EWSNOTOO	39
#define EWSNOPXL	40
#define EWSNOGDP	41


/* TRANSFORMATIONS */

#define EBADXFRM	50
#define EBADRCTD        51
#define EBDVIEWP        52
#define EBDWINDW 	53
#define EVIEWDSP	54


/* OUTPUT ATTRIBUTES */

#define EBADLINX	60
#define ENOLINEX	61 
#define ELINELEZ	62
#define ENOLINTP	63
#define EBADMRKX	64
#define ENOMARKX	65
#define EMARKLEZ	66
#define ENOMRKTP	67
#define EBADTXTX	68
#define ENOTEXTX	69
#define ETXTFLEZ	70
#define ENOTXTFP	71
#define ECEXFLEZ	72
#define ECHHTLEZ	73
#define ECHRUPVZ	74
#define EBADFILX	75
#define ENOFILTP	76
#define ENOFSTYL	77
#define ESTYLLEZ	78
#define EBADPATN	79
#define ENOHATCH	80
#define EPATSZLZ	81
#define ENOPATNX	82
#define ENOPSTYL	83
#define ECADIMEN	84
#define ECINDXLZ	85
#define EBADCOLX	86
#define ENOCOLRX	87
#define ECOLRNGE	88
#define EBADPICK	89


/* OUTPUT PRIMITIVES */

#define ENPOINTS	100
#define ECHRCODE	101
#define EBDGDPID	102
#define EGDPDATA	103
#define ECANTGDP	104


/* SEGMENTS */

#define EBADNAME	120
#define ENAMUSED	121
#define EWHATSEG	122
#define EWORKSEG	123
#define EWISSSEG	124
#define ESEGOPEN	125
#define ESEGPRIR	126


/* INPUT */

#define ENOINDEV	140
#define EREQUEST	141
#define ENSAMPLE	142
#define ENOEVSMP	143
#define ENOPETWS	144
#define EEBOUNDS	145
#define EBADDATA	146
#define EINQOVFL	147
#define ENOQOVFL	148
#define EASWSCLO	149
#define ENOCURIV	150
#define EINVTOUT	151
#define EBDINITV	152
#define ESTRSIZE	153


/* METAFILE */

#define ERESERVE	160
#define EBDLNGTH	161
#define EMNOITEM	162
#define EMITMINV	163
#define ENOTGDSI	164
#define EBADCNTS	165
#define EEBDMXDR	166
#define EINTERPT	167


/* ESCAPE */

#define ENOFUNCT	180
#define EESCDATA	181


/* IMPLEMENTATION DEPENDENT */

#define ENOFLEVL	300
#define EMEMSPAC	301
#define ESEGSPAC	302
#define EIO_READ	303
#define EIOWRITE	304
#define EIOSENDD	305
#define EIORECDA	306
#define EIOLIBMG	307
#define EIORDWDT	308
#define EINSTIEN	309 

#define ESERIALN	910 
#define EGRAPHCP	911 
#define EVDIOPEN	912 
#define EVDICLOS	913 

