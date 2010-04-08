/*
**	Copyright (c) 2004 Ingres Corporation
*/
/**
** Name:	graf.h -	Graphics System Frame Structure Definition.
**
** Description:
**	This file defines the Graphics System frame structure and related
**	elements.  (Should change flag constants to bit fields!)
**
** History:	
 * Revision 4003.2  86/07/02  17:59:14  mgw
 * Re-integrated with VMS
 * 
**		
**		8/2/89 (Mike S)
**		Increase maxima (MAX_BAR, MAX_STRINGS, MAX_XSTRINGS)
**		Fix for bug 7225.
**		This is a version of a fix made by bruceb in 5.0 which was left **		out of 6.1.  We can do a better job now because VEC has 
**		increased MXNLBLS (in ccht.h) to 60.
**
**		Revision 41.00	 89/02/19 Mike S.
**		Conditionalize for DG
**
**		Revision 40.107  86/03/21  18:27:50  bobm
**		up dataset limit to 16
**		
**		Revision 40.106  86/03/18  10:31:39  wong
**		Changed 'ax_g_dash' to support all line types.
**		
**		Revision 40.105  86/03/12  14:06:08  bobm
**		hatch pattern definition, pie chart individual slice attributes
**		
**		Revision 40.104  86/03/11  16:15:07  wong
**		Added axis line visibility bit field to "AXIS" structure.
**		
**		Revision 40.103  86/03/11  13:40:48  bobm
**		add "obj" item to LEGEND structure.
**		
**		Revision 40.102  86/03/07  16:33:12  wong
**		Added label placement field to "LABEL" structure.
**		
**		Revision 40.101  86/02/27  11:01:43  wong
**		Add mark style and line style definitions; changed chart flags
**		to bit fields and added 'box_axes'; removed 'graf', 'gpavail', 'locate'
**		from "GR_FRAME" structure.
**		
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include	<ooclass.h>

/*
**	graphical object class codes.
*/
#define GROB_BAR	OC_GRBAR
#define GROB_LINE	OC_GRLINE
#define GROB_SCAT	OC_GRSCAT
#define GROB_PIE	OC_GRPIE
#define GROB_TRIM	OC_GRTEXT
#define GROB_LEG	OC_GRLEGEND
#define GROB_GROUP	1	/* not currently used */
#define GROB_NULL	0

/*
**	subcomponent codes
*/
#define SCM_NULL 0
#define SCM_LEGTITLE 1
#define SCM_LEGTEXT 2
#define SCM_AXHDR 3
#define SCM_AXIS 4
#define SCM_DLABEL 5
#define SCM_DATA 6
#define SCM_SUBPLOT 7

/*
**	global flags for all graphics objects (GR_OBJ structure)
*/
#define GRF_BORDER 1	/* draw a border around object */
#define GRF_CLEAR 2	/* clear extent before drawing */

/*
**	Dataset maximums for chart types, ie. max number of series
*/
#define MAX_B_DS 16	/* maximum datasets for a bar chart */
#define MAX_L_DS 16	/* maximum datasets for a line chart */
#define MAX_S_DS 16	/* maximum datasets for a scatter chart */
#define MAX_P_DS 1	/* maximum datasets for a pie chart */
#define MAX_DS 16	/* max of above 4 */

/*
**	These are applied when plotting the chart.  Attached vectors
**	may actually contain more data points to accomodate chart type
**	switching.
*/
#define MAX_SLICE 24	/* maximum pie slices */
#define MAX_BAR 31	/* maximum bars in one dataset */

/*
**	Maximums used in building data vectors.
**
**	MAX_STRINGS is >= MAX_SLICE and MAX_BAR to accomodate data labels
**
**	MAX_XSTRINGS is the maximum number of labels which will be placed
**	on an axis.
**	
**	These should both be <= MXNLBLS (in ccht.h)
**
*/
#define MAX_STRINGS 50	/* maximum number of entries in a string vector */
#define MAX_XSTRINGS 50	/* maximum number of strings in an x axis array */
#define MAX_POINTS 15000	/* maximum points in any dataset */

/*
**	axis flags (AXIS structure)
*/
#define AX_L_CENTER 1	/* center labels between tick marks */
#define AX_GRID 2	/* draw axis grid lines */
#define AX_HD_SUP 4	/* suppress axis header */
#define AX_HD_DOWN 8	/* right or left header drawn downward */
#define AX_HD_PERP 16	/* perpendicular to axis */
#define AX_LOG 32	/* log axis for numeric data */
#define AX_REVERSE 64	/* reverse the axis */
#define AX_CROSS 128	/* cross-hair axis at 0,0 */
#define AX_DATE 256	/* axis represents date data */
#define AX_STRING 512	/* axis represents date data */
#define AX_HDB_SUP 1024 /* suppress heading background */
#define AX_LIM 2048	/* axis has specified range */

/*
** Name:	LPLACE -	Label Placement Type Definitions.
*/

#ifndef EXTERNAL
# define EXTERNAL         1
# define BOTTOM           2
# define MIDDLE           3
# define TOPOFBAR         4
# define BELOW            5
#endif

/*
** Name:	MARKS -		Mark Type Definitions.
*/
#ifndef DOT
# define DOT 1		/* MK_DOT */
# define CROSS 2	/* MK_CROSS (a plus) */
# define STAR 3		/* MK_STAR */
# define CIRCLE 4	/* MK_CIRCLE */
# define X 5		/* MX_X */
# define SQUARE 6	/* MK_SQUARE */
# define TRIANGLE 7	/* MK_TRI */
#endif

/*
** Name:	DASH -	Line Dash Type Definitions.
*/
#ifndef SOLID
# define SOLID 0	/* L_SOLID - 1 ( ______ ) */
# define DASHED 1	/* L_DASHED - 1 ( _ _ _ _ ) */
# define DOTTED 2	/* L_DOTTED - 1 ( . . . . ) */
# define DDASHED 3	/* L_DDASHED - 1 ( ._._._. ) */
#endif

/*
** Name:	CONNECT -	Connection Types for Line Charts Definitions.
*/
#define XC_STRAIGHT 0	/* eqv. CV_STRAIGHT - straight lines between points */
#define XC_STEP 1	/* eqv. CV_STEP - steps between points */
#define XC_BSPLINE 2	/* eqv. CV_BSPLINE - fit b-spline to points */
#define XC_QSPLINE 3	/* eqv. CV_QSPLINE - fit cubic spline to points */
#define XC_REGRESS 4	/* eqv. CV_REGRESS - do a linear regession fit on points */
#define XC_CROSS 5	/* eqv. CV_CROSS - do cross member at each point */
#define XC_NONE 6	/* eqv. CV_SCATTER - do markers only */

/*}
** Name:	GFONT -		Graphics System Font Type.
*/
typedef i2	GFONT;

# ifndef DGC_AOS
# define ASCII_FONT	1	/* ASCII (maps to hardware) */
# define CARTO_SIMPLEX	2	/* uniplex simplex cartographic roman ascii */
# define UNI_SIMPLEX	3	/* uniplex simplex principal roman ascii */
# define DUP_COMPLEX	4	/* duplex complex index roman ascii */
# define DUP_SIMPLEX	5	/* duplex simplex principal roman ascii */
# define TRI_COMPLEX	6	/* triplex complex principal roman ascii */
# define MED_HELV	7	/* filled helvetica */
# else
# define CARTO_SIMPLEX	6	/* uniplex simplex cartographic roman ascii */
# define UNI_SIMPLEX	6	/* uniplex simplex principal roman ascii */
# define TRI_COMPLEX	2	/* triplex complex principal roman ascii */
# endif

/*}
** Name:	COLOR -		Graphics System Color Type.
*/
typedef i2	COLOR;

# define BLACK		0	/* Background color */
# define WHITE		1	/* Foreground color */

/*}
** Name:	EXTENT -	Graphic Object Extents Structure.
**
** Description:
**	This structure describes the extents of a graphic object within
**	the coordinates recognized by the C-Chart/GKS package.  Coordinates
**	are within the unit quadrant, 0.0 to 1.0.
**
**		    ^
**		1.0 |
**		    |
**		    |
**		0.0 +-------->
**                 0.0    1.0
*/
typedef struct
{
	float xlow,ylow,xhi,yhi;	/* objects "window" */
} EXTENT;

/* structure to contain coordinate points (same system as EXTENT) */
typedef struct
{
	float x,y;
} XYPOINT;

/*}
** Name:	MARGIN -	Graphic Object Margins Structure.
**
** Description:
**	This structure describes the margins for a graphic object as a
**	fraction of its extent dimensions.
*/
typedef struct
{
	float left,right,top,bottom;	/* fraction of window to use */
} MARGIN;

# define MIN_MARGIN 0.001	/* minimum margin (allowing for round-off) */
# define DEF_MARGIN 0.04	/* default margin */
# define DEF_AX_MARGIN 0.15	/* default axis margin */

/*
**	structure containing COLOR and hatch pattern for those items to which
**	both apply
*/
typedef struct
{
	COLOR color;
	i2 hatch;
} FILLREP;

/*}
** Name:	LEGEND -	Graphic Legend Object Structure.
*/
typedef struct
{
	char	*title;
	struct gr_obj_s *assoc;		/* associated GR_OBJ (chart) */
	struct gr_obj_s *obj;		/* legend's own GR_OBJ */
	i2	items;			/* count for plot-time use */
	u_i2	lg_flags;
	GFONT	ttl_font, txt_font;
	COLOR	ttl_color, txt_color;
} LEGEND;

/*
**	GR_OBJ structure defining graphical components.  "type" item
**	determines what object class "attr" points at:
**
**		GROB_BAR:  BARCHART
**		GROB_LINE: LINECHART
**		GROB_SCAT: SCATCHART
**		GROB_LEG:  LEGEND
**		GROB_TEXT: field or trim (GR_TEXT)
**		GROB_PIE:  PIECHART
**		GROB_NULL: nothing - these shouldn't actually appear in the
**			   graph - editing uses nodes temporarily.
*/
typedef struct gr_obj_s
{
	i4	id;	/* Object (database) id */
	EXTENT ext;
	MARGIN margin;
	i4 *attr;		/* attributes structure specific to object */
	struct gr_obj_s *next;
	struct gr_obj_s *prev;
	LEGEND *legend;		/* associated LEGEND, or NULL */
	COLOR bck;		/* background color */
	u_i2 goflgs;	/* global object flags */
	i2 layer;	/* layer of object - dynamic use during edit */
	i2 scratch;	/* edit temporary */
	i2 order;	/* used for "undo" */
	OOID type;	/* GR_xxxx code */
} GR_OBJ;

/* structure for constructing temporary lists of objects */
typedef struct gr_o_l_s
{
	struct gr_o_l_s *next;
	GR_OBJ *elt;
} GR_SLIST;

/*
**	graphics frame structure used by interactive graphics routines
**
**	floating point coordinates are 0.0 to 1.0 in both x and y directions.
**	this coordinate system is used in all graphics object calculations,
**	and is mapped to the lrow,hrow,lcol,hcol window on the screen.
**	the transforms (xb,xm,yb,ym) are applied only to coordinates
**	communicated to drawing, and are mainly a mechanism for "shrinking"
**	drawings off of the menu line without changing coordinates while
**	editting.  For non-interactive plotting, these transforms will be
**	identities.
**
**	head and tail point to the two ends of the doubly linked (in drawing
**	order) list of objects making up the graph.  List is linked through
**	the "next" and "previous" items of GR_OBJ.  "head"->prev and
**	"tail"->next are NULL.  The "cur" item points into the middle of
**	the list to the item the user is pointing at during edit.
*/
typedef struct
{
	GR_OBJ *head;	/* head of component list */
	GR_OBJ *tail;	/* tail of component list */
	GR_OBJ *cur;	/* current component */
	EXTENT *lim;	/* current screen cursor limits */
	GR_SLIST *set;	/* selected group of components */
	EXTENT *subex;	/* extent of subcomponent */
	float cx,cy;	/* current coordinates */
	float xm,xb;	/* x-coordinate drawing transform */
	float ym,yb;	/* y-coordinate drawing transform */
	float csx,csy;	/* cursor size in graphics coordinates */
	float lx,ly;	/* line width */
	float aspect;	/* aspect ratio */
	OOID comp;	/* class of current component */
	i2 subcomp;	/* type of current sub-component */
	i2 subidx;	/* data index associated with subcomponent */
	i2 labidx;	/* label index for label subcomponents */
	i2 clayer;	/* current number of layers on screen */
	i2 lnum;	/* total layers */
	i2 preview;	/* preview level for drawing */
	i2 crow,ccol;	/* current row and column */
	i2 lrow,hrow;	/* rows of graphics window */
	i2 lcol,hcol;	/* columns of graphics window */
	u_i2 gomask;	/* mask for all goflags */
} GR_FRAME;

/*}
** Name:	AXIS -		Graphic Chart Axes Axis Structure.
**
** Description:
**	Describes one axis for a graphics chart.  There are arrays of four
**	of these for each chart that can have axes.  Indices defined below
**	show which entry goes with which axis.
*/
typedef struct
{
	float ticks;	/* ticks per axis step */
	float grids;	/* grids per axis step */
	float r_low;	/* low axis range */
	float r_hi;	/* high axis range */
	float r_inc;	/* axis increment */
	float margin;	/* margin - overides MARGIN array if visible */
	struct {	/* Header */
		char	*hd_text; 	/* text */
		GFONT	hd_font;	/* font */
		COLOR	hd_b_clr;	/* background color */
		COLOR	hd_t_clr;	/* header text */
		u_i4	hd_just : 2;	/* heading justification */
# define			AXHJ_LEFT 1
# define			AXHJ_CENTER 2
# define			AXHJ_RIGHT 3
		u_i4	hd_suppress : 1;/* suppress axis header */
		u_i4	hd_down : 1;	/* right/left header drawn downward */
		u_i4	hd_perp : 1;	/* perpendicular to axis */
		u_i4	hd_b_suppr : 1;	/* suppress background */
	} ax_header;
	struct {	/* Labels */
		char	**lb;		/* axis label array */
		u_i2	lb_len;		/* number of labels */
		GFONT	lb_font;	/* font */
		COLOR	lb_clr;		/* color */
		i2	lb_num_fmt;	/* numeric format */
		i2	lb_date_fmt;	/* date format */
		i2	lb_prec;	/* decimal precision for numeric labels */
		u_i4	lb_supr : 3;	/* label suppression */
# define			ALL_LABS 0	/* NOSUPR -- all labels */
# define			NO_LABS 1	/* ALLSUPR -- no labels */
# define			MID_LABS 2	/* ENDSUPR -- no 1st or last */
# define			END_LABS 3	/* MIDSUPR -- only 1st and last */
# define			NOT_1LAB 4	/* FIRSTSUPR -- no 1st label */
# define			NOT_XLAB 5	/* LASTSUPR -- no last label */
		u_i4	lb_center : 1;	/* center labels between ticks */
	} ax_labels;
	u_i2	tick_loc;	/* tick location */
# define		AXT_OUT 0	/* OUTTIK -- outside axis */
# define		AXT_IN 1	/* INTIK -- inside axis */
# define		AXT_CROSS 2	/* CROSSTIK -- cross axis (on both sides) */
# define		AXT_NONE 4
	u_i2	ds_idx;	/* dataset axis is associated with */
	COLOR	g_clr;	/* grid and actual lines */
	u_i2	a_flags;
	u_i4	ax_exists : 1;	/* axis should be drawn */
	u_i4	ax_reverse : 1;	/* reverse the axis */
	u_i4	ax_cross : 1;	/* cross axis at 0 */
	u_i4	ax_log : 1;	/* logrithmic axis for numeric data */
	u_i4	ax_lim : 1;	/* axis has specified range */
	u_i4	ax_ln_invis : 1;/* axis line visibility */
	u_i4	ax_grid : 1;	/* draw axis grid lines */
	u_i4	ax_g_dash : 2;	/* axis grid lines are dashed */
	u_i4	ax_date : 1;	/* axis represents date data */
	u_i4	ax_string : 1;	/* axis represents string data */
} AXIS;

/*}
** Name:	AXES -		Graphic Chart Axes Structure.
**
** Description:
**	Describes the axes for a graphics chart.  This contains the four axes
**	that a graphics chart can have, with an array of four AXIS structures
**	describing each axes.
*/
typedef struct
{
	AXIS	ax[4];		/* Axes descriptors */
/*
**	axis numbers 0 - 3, may be used as indices into array of
**	AXIS structures.
*/
# define LEFT_AXIS 0
# define RIGHT_AXIS 1
# define TOP_AXIS 2
# define BTM_AXIS 3
	/* Individual axes descriptors */
# define	ax_left		ax[LEFT_AXIS]
# define	ax_right	ax[RIGHT_AXIS]
# define	ax_top		ax[TOP_AXIS]
# define	ax_bottom	ax[BTM_AXIS]
# define	ax_x_left	ax[LEFT_AXIS].ax_exists
# define	ax_x_right	ax[RIGHT_AXIS].ax_exists
# define	ax_x_top	ax[TOP_AXIS].ax_exists
# define	ax_x_bottom	ax[BTM_AXIS].ax_exists
} AXES;

/*}
** Name:	DEPDAT -	DataSet Dependent Information Structure.
**		some parts of this structure don't apply for certain types
**		of charts.
*/
typedef struct
{
	char *field;	/* "y" axis internal name */
	char *xname;	/* internal name for "x" axis vector */
	char *desc;	/* description for legend */
	FILLREP fill;	/* representation for filled areas under lines */
	FILLREP data;	/* representation of bars, lines and marks */
	u_i4	ds_cnct : 4;	/* connection type for lines */
	u_i4	ds_mark : 3;	/* mark for scatter plots / line graphs */
	u_i4	ds_dash : 2;	/* dash type for lines */
} DEPDAT;

/*}
** Name:	LABEL -	DataSet Label Information Structure.
*/
typedef struct
{
	char	*l_fmt;		/* format string */
	char	*l_field;	/* internal name for vector of label strings */
	COLOR	l_clr;
	GFONT	l_font;
	u_i1	l_num;		/* number of labels, used dynamically */
	u_i4	l_place : 3;	/* label placement (0 ==> no label) */
} LABEL;

/*
**	we keep fixed arrays of axes and datasets so that attributes can
**	be stored for all possible series and axes, not just the active ones.
*/
typedef struct
{
	float	baseline;	/* where to draw zero line on chart */
	char	*indep;		/* independent axis - spreadsheet model */
	AXIS	ax[4];
	LABEL	lab[MAX_B_DS];
	DEPDAT	ds[MAX_B_DS];
	i2	ds_num;		/* drawable items in ds/lab arrays */
	i2	ds_set;		/* no. of set (non-default) ds items */
	i2	cluster;	/* cluster index - controls bar stacking */
	u_i4	bar_horizontal : 1;	/*  horizontal bar chart */
	u_i4	box_axes : 1;		/* box chart axes */
} BARCHART;

/*
**	LINECHART and SCATCHART very similar to BARCHART
*/
typedef struct
{
	float	baseline;
	char	*indep;
	AXIS	ax[4];
	DEPDAT	ds[MAX_L_DS];
	i2	ds_num;		/* drawable items in ds/lab arrays */
	i2	ds_set;		/* no. of set (non-default) ds items */
	u_i4	line_fill : 1;	/* fill under lines */
	u_i4	box_axes : 1;	/* box chart axes */
} LINECHART, SCATCHART;

typedef struct
{
	LABEL lab;
	DEPDAT ds;		/* data */
	float rot;		/* rotation of pie */
	i2 exstart, exend;	/* slices to explode */
	float exfact;		/* explosion factor */
	u_i2 p_flags;
	FILLREP slice[MAX_SLICE];
} PIECHART;

/* object for graphics text - either fields or trim */
typedef struct
{
	char *text;		/* text string */
	char *column;		/* field column, NULL if unmapped */
	char *name;		/* field name, NULL if trim */
	GFONT font;
	COLOR ch_color;
} GR_TEXT;
