/*
**	vdi.h
**
**	public include file for Visual:VDI
**	(C) Copyright (c) 2004 Ingres Corporation
**	@(#)vdi.h	1.16	12/23/85
**
**  History:
**	10/28/92 (dkh) - Changed ALPHA to WS_ALPHA to avoid problems when
**			 compiling on the DEC/ALPHA machine.
**	18-jun-1997 (kosma01)
**	   iicommon.h already has a DB_CHR_TYPE, and its value is not 1.
**	   renamed DB_CHR_TYPE to DB_CHR
*/

#ifndef	_VDI_H
#define _VDI_H

#define MAXNDC		32768L		/* 2**15 for scaling NDC */
#define MAXNDC2		65536L		/* 2**15 for scaling NDC */
#define SQRTMXNDC	46340L		/* Sqrt of 2**31 for overflow chk */
#define SHIFTNDC        15		/* 15 bit shift for scaling NDC */
#define MAXDC           32768L		/* 2**15 for scaling DC */
#define SHIFTDC		15		/* 15 bit shift for scaling DC */

/* MARKERS */
#define MK_DOT		1
#define MK_CROSS	2
#define MK_STAR		3
#define MK_CIRCLE	4
#define MK_X		5
#define MK_SQUARE	6
#define MK_TRI		7

/* WS CLASS */
#define	RASTER		0
#define	VECTOR		1
#define	OTHER		2
#define	PLOTTER		3
#define	DOTMATRIX	4
#define	LASER		5
#define	WS_ALPHA	6

/* DB_TXT_TYPE PATH */
#define	TP_RIGHT	0
#define	TP_LEFT		1
#define	TP_UP		2
#define	TP_DOWN		3

/* DB_TXT_TYPE HORIZONTAL ALIGNMENT */
#define	TH_NORMAL	0
#define	TH_LEFT		1
#define	TH_CENTRE	2
#define	TH_RIGHT	3

/* DB_TXT_TYPE VERTICAL ALIGNMENT */
#define	TV_NORMAL	0
#define	TV_TOP		1
#define	TV_HALF		2
#define	TV_BASE		3
#define	TV_BOTTOM	4
#define	TV_CAP		5

/* WRITE MODES */
#define	MODE_SET	0
#define	MODE_XOR	1
#define	MODE_OR		2
#define	MODE_AND	3

/* DB_TXT_TYPE PRECISION */
#define STROKE		0
#define DB_CHR		1
#define STRING		2

/* fonts */
#define ASCIIFT			1	/* ASCII FonT			*/
#define H_CARTO_SIMPLEX		2	/* Ascii UNiplex SiMplex RoMan Up case*/
#define H_UNI_SIMPLEX		3	/* Ascii UNiplex SiMplex RoMan	*/
#define H_DUP_COMPLEX		4	/* Ascii DUplex COmplex RoMan	*/
#define H_DUP_SIMPLEX		5	/* Ascii DUplex SiMplex RoMan	*/
#define H_TRI_COMPLEX		6	/* Ascii TRiplex COmplex RoMan	*/
#define F_MED_HELV		7	/* Ascii MEDium HELvetica	*/

/* line styles */
#define L_SOLID		1
#define	L_DASHED	2
#define	L_DOTTED	3
#define	L_DDASHED	4

/* fill area types */
#define	HOLLOW		0
#define	SOLID		1
#define	PATTERN		2
#define	HATCH		3

/* number of hatch styles */
#define	MXNSWHCH	40
#define NHATCHS		50

/* INPUT */
#define L_ESCAPE	1
#define L_BREAK		2
#define L_OK		3
#define MXWS		9	/* max number of ws open at once */
#define DEVNMLEN	35	/* ws name length */
#define MXWSNAMES	7	/* number of ws name synonymns */

/* recalc switch */
#define RECALC		1
#define NORECALC	2

/* wsxf type */
#define POINT		1
#define VIEWPORT	2

/* ws catagory */
#define OUTPUT		0
#define OUTIN		1
#define INPUT		2
#define MO		3
#define MI		4
#define WISS		5

/* output primitive type */
#define	VPOLYLINE	1
#define	VPOLYMARKER	2
#define	VFILLAREA	3



/* LONG XY PAIR */
typedef struct {
	long	x,y;
} Vlongxy;

/* SHORT XY PAIR */
typedef struct {
	short	x,y;
} Vshortxy;

/* LONG RECTANGLE */
typedef struct {
	Vlongxy ll,ur;
} Vlrect;

/* DB_TXT_TYPE EXTENT */
typedef struct {
	Vlongxy		ll,ur,ul,lr,con;
} Vextent;




/* WORKSTATION MAP */
typedef struct {
	short	num;			/* number of names for this ws */
	short	cat;			/* ws category */
	short	hashid[MXWSNAMES];	/* ws name hashed */
	char	*name[MXWSNAMES];	/* ws name */
	char	*stdname;		/* the standard name */
} Vwsmap;



/* DB_TXT_TYPE ALIGNMENT */
typedef struct {
	short		h,v;
} Vtxalign;



/* font prec pair */
typedef struct {
	short font;
	short prec;
} Vtxfp;


/* WS TRANSFORM */
typedef struct {
	Vlongxy scl;	/* (x,y) scaling factor * MAXNDC */
	Vlongxy ll;	/* lower left pt of viewport * MAXNDC */
} Vwsxf;


typedef struct {
	long a;
	long b;
	long c;
} Vline;


/* CUR OUTPUT STATE */
typedef struct {
	short		color;	

	short		lntype;
	float		lnwidth;
	short		lnnom;

	short		mktype;
	short		mknom;

	short		fatype;
	short		fastyle;
} Vcurst;


/* VDI OUTPUT STATE PER WS */
typedef struct {
	short		lncolor;	
	short		lntype;
	float		lnwidth;

	short		mkcolor;
	short		mktype;
	float		mksize;
	short		mknom;

	short		facolor;
	short		fatype;
	short		fastyle;

	short		txcolor;
	Vtxfp		txfp;
	short		propflag;

	Vlrect		clip;		/* output clips against this */
	Vlrect		reqclip;	/* ndc vprt to clip against */
	short		clear;

} Voutst;

typedef struct {
	short		id;
	char		*name;		/* standard name */
	Voutst		outst;		/* output state */
	Vcurst		cur;		/* current driver output state */
	Vlrect		wndw;		/* ws window */
	Vlrect		dcvprt;		/* ws viewport (device coor) */
	Vlrect		ndcvprt;	/* ws viewport (norm device coor) */
	Vwsxf		wsxf;		/* ws transform */
	Vwsxf		mwsxf;		/* ws transform in meters */
	Vlrect		display;	/* display coors */
	Vlongxy		size;		/* size in meters of device */
	Vlrect		hwvprt;		/* hardware viewport coor system */
	Vwsxf		vwsxf;		/* ws transform for hw viewport */
	short		cat;		/* ws catagory (out,in,etc)*/
	short		class;		/* ws class */
	FILE		*fo;		/* output file */
	FILE		*fi;		/* input file */
	short		ncolors;	/* number of colors 	*/
	short		nlines;		/* number line styles	*/
	short		nhatchs;	/* number hatch styles	*/
	short		nmarkers;	/* number of markers	*/
	char		qback;		/* can draw in bckgrnd?	*/
	char		mode;		/* drawing mode		*/
	char		buf[BUFSIZ];	/* output buffer */
} Vws;



/* GENERAL OUTPUT STATE */
typedef struct {
	short		txpath;
	long		chrht;
	long		chrexp;
	float		chrsp;
	Vtxalign	txalign;
	Vlongxy		upvec;
	Vlongxy		sclvec;
} Vgenout;


/* VDI STATE */
typedef struct {
	Vgenout		genout;		/* general output state */
	short		open;		/* vdi open flag */
	short		navwsknd;	/* how many avail ws types */
	Vwsmap		*wsmap;		/* list of ws types */
	FILE		*f_font;	/* font file pointer */
	FILE		*f_err;		/* error file */
} Vopst;


/*
**	FUNCTIONS
*/
char *Vgtwsname();


/*
**	MACRO BINDINGS 
*/
#define Vw(id)		Vwss[id]

/*
**	CONTROL
*/
#define V_OPEN(file)		(VDI_ok = Vdiopen(file))
#define V_CLOSE()		(VDI_ok = Vdiclose())
#define V_OPWS(id,type,fname)	(VDI_ok = Vopenvws(id,type,fname))
#define	V_CLWS(id) {			\
				Vuselocalvars(Vw(id));		\
				VDI_ok = Vclosevws(Vw(id));	\
}
#define	V_CLEAR(id,flag)	Vdiclear(Vw(id), flag)
#define	V_UPDATE(id)	{		\
				Vuselocalvars(Vw(id));		\
				Vfnupdate(Vw(id));		\
				Vwsflush(Vw(id));		\
}
#define V_FLUSH(id)		Vwsflush(Vw(id))

/*
**	ATTRIBUTES
*/
#define V_WRITEMODE(id,mode)	Vwritemode(Vw(id),mode)
#define V_FONTPRC(id, f, p) {		\
				Vw(id)->outst.txfp.font = (f);	\
				Vw(id)->outst.txfp.prec = (p);	\
}
#define V_CHARVEC(pt)		Vstate.genout.upvec = *(pt)
#define V_SCLVEC(pt)		Vstate.genout.sclvec = *(pt)
#define V_TXALIGN(hor,vert) {		\
				Vstate.genout.txalign.h = (hor);	\
				Vstate.genout.txalign.v = (vert);	\
}
#define V_CHEXP(id, exp) 	Vstate.genout.chrexp = (long) (MAXNDC * (exp))
#define V_CHSPACE(id, sp) 	Vstate.genout.chrsp = (sp)
#define V_LUT(id,i,r,g,b)	{ Vfnlut(Vw(id),i,r,g,b); Vwsflush(Vw(id)); }
#define V_TXPATH(path)		Vstate.genout.txpath = path
#define V_TXCOLOR(id,color)	Vw(id)->outst.txcolor = color
#define V_FACOLOR(id,color)	Vw(id)->outst.facolor = color
#define V_FATYPE(id,type)	Vw(id)->outst.fatype = type
#define V_FASTYLE(id,index)	Vw(id)->outst.fastyle = index
#define V_LNCOLOR(id,color)	Vw(id)->outst.lncolor = color
#define V_LNTYPE(id,type)	Vw(id)->outst.lntype = type
#define V_LNWIDTH(id,width)	Vw(id)->outst.lnwidth = width
#define V_MKCOLOR(id,color)	Vw(id)->outst.mkcolor = color
#define V_MKTYPE(id,type)	Vw(id)->outst.mktype = type
#define V_MKSIZE(id,size)	Vw(id)->outst.mksize = (float) (size)

/*
**	TRANSFORMATIONS
*/
#define V_DCMETERS(id,dc,mdc)	Vdctomdc(Vw(id), dc, mdc)
#define V_METERSDC(id,mdc,dc)	Vmdctodc(Vw(id), mdc, dc)
#define V_APPWSXF(id,ndc,dc)	Vappwsxf(&(ndc), &(Vw(id)->wsxf), &(dc))
#define V_INVWSXF(id,dc,ndc) 	Vinvwsxf(&(dc), &(Vw(id)->wsxf), &(ndc))
#define V_CLIP(id,cliprect)	{			\
				Vuselocalvars(Vw(id));			\
				Vw(id)->outst.reqclip = *(cliprect);	\
				VDI_ok = Vnewclip(Vw(id));		\
}
#define V_WNDW(id,w)	 { 						\
				Vuselocalvars(Vw(id));			\
				Vw(id)->wndw = *(w);			\
				Vcmpwsxf(POINT, Vw(id), w,		\
					&Vw(id)->dcvprt, &Vw(id)->wsxf);\
				VDI_ok = Vnewclip(Vw(id));		\
}
#define V_VPRT(id,v)	{ 						\
				Vuselocalvars(Vw(id));			\
				Vw(id)->dcvprt = *(v);			\
				Vcmpwsxf(POINT,Vw(id), &Vw(id)->wndw, 	\
					v, &Vw(id)->wsxf);		\
				VDI_ok = Vnewclip(Vw(id));		\
}

/*
**	OUTPUT
*/
#define V_FA(id,npts,pts)	{ 				\
				Vuselocalvars(Vw(id));		\
				Vdofaws(Vw(id), npts, pts);	\
}
#define V_PLYLN(id,npts,pts)	{ 				\
				Vuselocalvars(Vw(id));		\
				Vdolnws(Vw(id),npts,pts,VPOLYLINE,RECALC,1);\
}
#define	V_PLYMK(id,npts,pts)	{ 				\
				Vuselocalvars(Vw(id));		\
				Vdomkws(Vw(id), npts, pts, RECALC);	\
}
#define	V_TEXT(id,pt,string)	{ Vextent VVext;		\
				Vuselocalvars(Vw(id));		\
				VDI_ok=Vdotx(Vw(id),0,pt,string,&VVext);\
}

/*
**	INPUT MACROS
*/
/* LOCATOR */
#define V_INLOC(id,pnt)		Vsetuploc(Vw(id), pnt, 1, 1, 1)
#define V_REQLOC(id,pnt) 	Vgetloc(Vw(id), pnt, 1)

/* STROKE */
#define V_INSTK(id,line,clr,stl,size) \
				Vinstroke(Vw(id), line, clr, stl, size)
#define V_DRSTK(id,npts,pts)	Vshowstroke(Vw(id), npts, pts)
#define V_REQSTK(id,n,pts,ed)	Vdostroke(Vw(id), &(n), &(pts), ed)
#define V_RMSTK(id,npts,pts)	Vrmstroke(Vw(id), npts, pts)

/* VALUATOR */
#define V_INVAL(id,echo)	Vinitval(Vw(id), echo)
#define V_DRVAL(id,echo,loval,hival,valinc) 	\
				Vdrawval(Vw(id),echo,loval,hival,valinc,0)
#define V_REQVAL(id,echo,loval,hival,value) 	\
				Vreqval(Vw(id), echo, loval, hival, &value)
#define V_RMVAL(id,echo,loval,hival,valinc) 	{	\
				Vdrawval(Vw(id),echo,loval,hival,valinc,1);\
				Vfintext(Vw(id));}

/* STRING */
#define V_INSTR(id)		Vinstg(Vw(id))
#define V_DRSTR(id,pt,s,ed)	Vdrawstg(Vw(id), &(pt), s, &ed, 0)
#define V_REQSTR(id,pt,s,ed)	Vreqstg(Vw(id), &(pt), &ed, s)
#define V_RMSTR(id,pt,s,ed)	{ Vdrawstg(Vw(id), &(pt), s, &ed, 1); \
				  Vfintext(Vw(id));}

/* CHOICE */
#define V_INCHC(id,pt,mtp,n,cs)	Vinitchc(Vw(id),&(pt),mtp,n,cs,0)
#define V_REQCHC(id,n)		Vreqchc(Vw(id),n)
#define V_RMCHC(id,pt,mtp,n,cs)	{ Vinitchc(Vw(id),&(pt),mtp,n,cs,1);\
				  Vfintext(Vw(id));}


/*
**	QUERIES
*/
#define	V_QTXEXT(id,pt,string,extent)	{		\
				Vuselocalvars(Vw(id));	\
				VDI_ok = Vdotx(Vw(id),1,pt,string,extent);\
}
#define	V_QCONCT(id,o,i,devname) {	\
				o = Vw(id)->fo;	\
				i = Vw(id)->fi;	\
				devname = Vw(id)->name;	\
}
#define V_QWSLIST(wsnames)	Vgetwslist(&wsnames)
#define V_QWSNAME(name,stdname) {	\
				Vstrcpy(stdname,Vgtwsname(name));	\
				VDI_ok = (stdname[0] != '\0');		\
}
#define V_QWSCAT(name,cat)	(VDI_ok = ((cat=Vgtwscat(name)) != -1))
#define V_QNCOLORS(id,nclr)	(nclr = Vw(id)->ncolors)
#define V_QNMARKS(id,nmk)	(nmk = Vw(id)->nmarkers)
#define V_QNHATCHS(id,nhch)	(nhch = Vw(id)->nhatchs)
#define V_QNLINES(id,nln)	(nln = Vw(id)->nlines)
#define V_QBACKGRND(id,qbk)	(qbk = Vw(id)->qback)


/*
**	ESCAPES
*/
#define V_GRAFMODE(id)		Vfngrafmode(Vw(id))
#define V_ALPHAMODE(id)		Vfnalphamode(Vw(id))
#define V_PENSPEED(id,speed)	(Vplpenspeed(Vw(id),speed))
#define V_STRING(id,str) 	fprintf(Vw(id)->fo, "%s", str)
#define V_BADALPHA(id,flag)	(flag = Vfnbadalpha(Vw(id)))


/*
**	GENISYS FUNCS
*/

#define V_QSCANLINE(id)		Vqscanline(Vw(id))
#define V_QNBANKS(id)		Vqnbanks(Vw(id))
#define V_QNRINTEN(id)		Vqnrinten(Vw(id))
#define V_QNGINTEN(id)		Vqnginten(Vw(id))
#define V_QNBINTEN(id)		Vqnbinten(Vw(id))
#define V_QPSEUDO(id)		Vqpseudocolor(Vw(id))
#define V_QMAXXY(id,x,y)	Vqresdc(Vw(id),x,y)
#define V_QSCANBYTE(id)		Vqscanbyte(Vw(id))

#define V_SCANLINE(id,x,y,n,a)	Vscanline(Vw(id),x,y,n,(char *)a)
#define V_SETBANK(id,bank)	Vsetbank(Vw(id),bank)
#define V_FULLCOLOR(id,r,g,b)	Vfullcolor(Vw(id),r,g,b)


/*
**	VDI structures
*/

/* HATCH STYLE DEFINITION */
typedef struct {
	short	c1;		/* y component of slope */
	short	c2;		/* x component of slope */
	short	lw;		/* line thickness in ndc */
	short	g1;		/* distance between lines */
	short	ct1;		/* number of lines */
	short	g2;		/* 2nd spacing if one exists */
	short	sqrflag;	/* flag if hatch in both directions */
} hatchdef;




#ifdef VDIMAIN
Vws		*Vwss[MXWS];	/* pointers to the workstation states */
Vopst		Vstate;		/* vdi state */
int		VDI_ok;		/* error status flag */
#endif

#endif	/* ifdef _VDI_H */
