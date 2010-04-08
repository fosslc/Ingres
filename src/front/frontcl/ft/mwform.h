/*
**	MWform.h
**	"@(#)mwform.h	1.9"
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	MWform.h - Common MWS and host header file.
**
** Usage:
**	INGRES FE system and MWhost on Macintosh.
**
** Description:
**	Contains constants and structures used by MWS and the
**	host to communicate.
**
**	Associated with every forms-mode item is a set of display
**	attributes. These attributes define the appearance of the item
**	within the form. Most attributes are relevant to the
**	appearance of an item regardless of the itemUs type, while
**	others are only significant for specific types of items;
**	table-specific attributes are a good example of this.
**
**	The routine MXaddFormItem() initializes the itemEntry data
**	structure for a new item using the default attribute values.
**	The routine parseAttributes() modifies it using the
**	information contained in the sub-components of the attributes
**	parameter of the MXaddFormItem or MXsetFormItem message.
**
**	Items are displayed in a form by calling the routine
**	MXdispItem(). The single parameter passed to this routine is
**	a handle to the itemEntry data structure for that item. This
**	structure contains all the information that the Forms Director
**	maintains about the item.
**
**	Each display attribute is described briefly below. Some of
**	these map cleanly to the fields of a Macintosh text edit
**	record field. Others, such as Marquee, require specific
**	tailoring by the Forms Director.
**
**	
** History:
**	05/5/89 (RK)  - Initial definition.
**	05/19/89 (RGS) - Initial implementation.
**	04/21/97 (cohmi01)
**	    Add/update  defines for new version numbers. (bug 81574).
**	08/11/97 (kitch01)
**		Bug 84391. Since new functionality was added to fix bug 83587, CACI
**		require a new version number to enable Upfront to distinguish what
**		functionality is present. Starting now MWHOST_VER will be a number
**		and 1000 respresents the fact that this version has table scrolling
**		enhancements and the displayonly enhancement. This number will be 
**		incremented by 1 each time new functionality is added to MWS.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/** MWS common definitions **/

/* form bit modes */
# define MWvCHARS	0
# define MWvBITS	1

# define MAXFORM	0x7FFF	/* Maximum form number */
# define MAXITEM	0x7FFF	/* Maximum form item number */

# define FLDINX		1001	/* Base form sequenced field item number */
# define NSNOINX	2001	/* Base form unsequenced field item number */
# define TRIMINX	3001	/* Base form trim field item number */

/* Types for different key maps used by the system */
# define mwKMAP		'k'	/* map for function and control keys (keymap) */
# define mwFRSMAP	's'	/* map for FRS keys (frsoper) */
# define mwKEYOP	'o'	/* map for ASCII keys (froperation) */


/* Compatibility values passed by host to MWS */
# define MWOK_COMPAT	1
# define MWASK_COMPAT	2
# define MWNOT_COMPAT	3

/* Host version info */
# define MWHOST_VER	1000
/* MWS version checks */
# define MWMWS_FATAL	6*100000 + 3*10000 + 2*100 + 2
# define MWMWS_IFFY	6*100000 + 3*10000 + 2*100 + 2

/* Values for flags in the SetCurMode command */
# define MWKEYINT	0x01	/* key intercept is active */
# define MWKEYFIND	0x02	/* key find is active */
# define MWKEYFINDDO	0x04	/* if key find active, return from FT */

/* Values for flags in X451 event */
# define MWFLD_CHNGD	0x01	/* field value has changed */
# define MWMWS_DOES	0x02	/* key will be handled by MWS */



/*	Font
**	
**	The name of the font needs to be converted to a font resource id. The
**	txFont field of the text edit record for the item should be set to this
**	resource id.
*/	
# define systemFont	 0	/* Some known Macintosh font families */
# define applFont	 1
# define monaco		 4
# define courier	22
# define bulletFont	193
# define ingresFont	194

# define MWvFONT	ingresFont	/* default font should be fixed width */
# define MWvFONTnoECHO	bulletFont	/* this font should be all bullets */

/*	PtSz
**	
**	The txSize field of the text edit record for the item should be set to
**	this integer value. It defines the point size used to display the
**	textual portion of the item.
*/	
# define MWvPTSZ	12


/*
**	redefined frame.h flags
*/

/* fdSCRLFD fdREADONLY are moved from xxx2flags to xxxflags */
# define	mwSCRLFD	fdSTOUT		/* Scrolling field */
# define	mwREADONLY	fdKPREV
# define	mw2FLAGS	(mwSCRLFD | mwREADONLY)

/* unused or redefined by MWS, cleared before transmit by host */
# define	mwVALID		(~(fdCHK | fdI_CHG | fdX_CHG | fdVALCHKED))

/* redefined by MWS for internal use */
# define	mwWRITE		fdX_CHG		/* MWS can write to field */
# define	mwCHANGED	fdI_CHG		/* MWS dirty field */

/* Logging flags */
/* error messages */
# define mwERR_GENERIC		((i4)0x01)	/* generic error */
# define mwERR_SYSTEM		((i4)0x02)	/* system error */
# define mwERR_ALIAS		((i4)0x04)	/* frame alias error */
# define mwERR_LOG_FILE		((i4)0x08)	/* could not open log file */
# define mwERR_OUT_SYNC		((i4)0x10)	/* MWS and host out of sync */
# define mwERR_BMSG_CLASS	((i4)0x20)	/* bad message class */
# define mwERR_BAD_MSG		((i4)0x40)	/* bad message */
/* message traffic log */
# define mwMSG_READ		((i4)0x00100000)
						/* read on the port */
# define mwMSG_ALL_RCVD		((i4)0x00200000)
						/* log all msgs received */
# define mwMSG_ALL_SENT		((i4)0x00400000)
						/* log all msgs sent */
# define mwMSG_OBJ_CRE		((i4)0x00800000)
						/* object creations */
# define mwMSG_OBJ_DES		((i4)0x01000000)
						/* object destructions */
# define mwMSG_OBJ_GEN		((i4)0x02000000)
						/* other obj manipulations */
# define mwMSG_FLO_CNT		((i4)0x04000000)
						/* significant flow cntrl info */
# define mwMSG_STATE		((i4)0x08000000)
						/* significant state changes */
# define mwMSG_INS_RCV		((i4)0x10000000)
						/* insert msg into MWS msg str */
# define mwMSG_INS_SND		((i4)0x20000000)
						/* insert msg into host msg str */
/* miscellaneous logging */
# define mwLOG_GENERIC		((i4)0x01)	/* generic log msg */
# define mwLOG_EXTRA_MSG	((i4)0x02)	/* unexpected message */
# define mwLOG_DIR_MSG		((i4)0x04)	/* message from a director */
# define mwLOG_MAC_INFO		((i4)0x08)	/* info about the mac */
# define mwLOG_MWS_INFO		((i4)0x10)	/* info about MWS */
# define mwLOG_PUT_VAL		((i4)0x20)	/* value of fld changed by user */

/* log control variables */
# define mwVAR_ERR	 1
# define mwVAR_LOG	 2


struct MWSinfo {
	i4	version;		/* MWS version from P006 -> P261 msgs */
/* MWS Client P261 version numbers and their  additional capabilities 	      */
#define MWSVER_31	31	/* functionality as of UPFRONT 1.2 	      */
#define MWSVER_32	32	/* added X200 msg for scroll button position  */
	i4	v[5];			/* MWS version ids */
	char	text[256];		/* MWS version string */
};
typedef struct MWSinfo MWSinfo;

struct MWsysInfo {
	i4	environsVersion;/* System environment type (1) */
	i4	machineType;	/* Macintosh hardware type */
	i4	systemVersion;	/* System version number */
	i4	processor;	/* CPU type */
	i4	hasFPU;		/* Floating point processor */
	i4	hasColorQD;	/* Color QuickDraw is present */
	i4	keyBoardType;	/* Style of keyboard */
	i4	atDrvrVersNum;	/* AppleTalk driver version */
	i4	sysVRefNum;	/* Directory reference number of system file */
	i4	mBarHeight;	/* Height in pixels of menu bar (20) */
	Rect	screenSize;	/* Screen coordinates (left,top,right,bottom) */
};
typedef struct MWsysInfo MWsysInfo;

