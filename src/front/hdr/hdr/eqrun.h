/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
+* Filename:	eqrun.h
** Purpose:	Header for EQUEL amd ESQL.
**
**	These are constants that are shared by both the Equel pre-processor
**	and the Equel Runtime System.  Therefore any change in these constants
**	are reflected in both systems.
**
** 	Also constants that are shared between LIBQ and the FRS are put here,
**	so that Libq can be compiled with only the top headers.
**
** History:
**		10-jan-1985	- Written (from eqforms.h and runtime.h) for
**				  EQUEL (ncg)
**		21-aug-1985	- Added definitions for ESQL (ncg)
**		08-dec-1989	- Added definitions for Phoenix/Alerters. (bjb)
**		25-jun-1990	- Added support for decimal. (teresal)
**		13-sep-1990     - Added IL_modNAMES for W4GL cursors (Joe)
**		01-mar-1991	- Added DISCONNECT ALL constant. (teresal)
**		22-feb-1991	- Added definitions for preprocessor target
**				  list processing. (kathryn)
**		15-mar-1991	- Add constants for saving last query text,
**				  i.e., SET_INGRES "savequery". (teresal)
**	 	13-mar-1991 (barbara)
**		    Added definitions for PRINTTRACE.
**		25-mar-1991 (barbara)
**		    Added SET_INGRES constant for COPYHANDLER.
**		15-apr-1991 (teresal)
**		    Add INQUIRE_INGRES contants for events.
**		19-jul-1991 (teresal)
**		    Change EVENT to DBEVENT.  Also, deleted some obsolete
**		    event stuff.
**		01-nov-1991 (kathryn)
**		    Define IITRC_HDLON & IITRC_HDLOFF for printgca, and 
**		    IIPTR_HDLON & IIPRT_HDLOFF for printqry.  
**		    For Trace output handlers.
**		22-sep-1992 (lan)
**		    Added INQUIRE_SQL constants for blobs.
**		01-nov-1992 (kathryn)
**		    Added gdSEGMENT, gdSEGMENTLENGTH, gdDATAEND, gdMAXLENGTH
**		    - GET/PUT DATA statement attribute constants.
**		06-nov-92 (davel)
**		    Added IIsqconSysGen constant, so that LIBQ can generate
**		    a unique session ID for a multi-session connection.
**		09-nov-92 (lan)
**		    Added T_HDLR type.
**	16-nov-92 (larrym)
**	    Added IISESSHIFT and IISESVECTORSIZE constant to support System 
**	    Generated session id's.  When Libq tries to find the next session,
**	    it scans through existing sessions and set's bits in a bit vector 
**	    to track ses id usage.  On the off chance someone has more sessions
**	    than we have bit's we rescan with an offset of IISESSHIFT (looping,
**	    up to a max of MAXI4).
**	21-dec-1992 (larrym)
**	    Added isCONTARG to support INQUIRE_SQL (CONNECTION_TARGET)
**	03-mar-1993 (larrym)
**	    Added siCONNAME to support connection name.
**	10-jun-1993 (kathryn)
**	    Add IImodCPAD and IImodNOCPAD for support of ANSI STD Retrieval
**	    assignment rules.  Ingres assigns null terminated strings into
**	    fixed length "C" host variables. ANSI requires that fixed length 
**	    "C" host variables be blank padded to the length of the host 
**	    variable - 1, and then EOS terminated.  
**	    IIsqMods(IImodCPAD) will give the ANSI behavior.
**	    IIsqMods(IImodNOCPAD) will give the default Ingres behavior.
**	07/18/93 (dkh) - Added definition for cmPURGE to support the
**			 purgetable statement.
**	07/19/93 (dkh) - Added definitions for IIFRresNEXT and IIFRresPREV
**			 to support resume nextfield/previousfield.
**	15-sep-93 (sandyd)
**	    Added T_VBYTE for support of VARBYTE datatype in ESQL/C.
**	01-oct-1993 (kathryn)
**	    Added define for II_DATLEN used by eqgentl for GET/PUT DATA stmts.
**	17-nov-93 (teresal)
**	    Add defines for checking for C string null terminator. Required for 
**	    FIPS. Bug 55915.
**	9-nov-93 (robf)
**          Add dbprompthandler support
**	10-apr-1997 (kosma01)
**	    Remove YOURS and THIERS integration markers.
**	18-Dec-97 (gordy)
**	    Cleaned up tracing options.  Added T_SQLCA for declaring
**	    host SQLCA variables.  Added IIsqsetNONE for releasing
**	    current session without selecting new session.
**	07-mar-2001 (abbjo03)
**	    Added T_WCHAR and T_NVCH for Unicode support.
**      24-jan-2005 (stial01)
**          Added II_BCOPY binary copy LONG NVARCHAR (DB_LNVCHR_TYPE) (b113792)
**	07-aug-2006 (gupsh01)
**	    Added constants for ANSI date/time types.
**	13-feb-2007 (dougi)
**	    Added flags to define fetch orientation options.
**      11-nov-2009 (joea)
**          Add T_BOO for Boolean support.
*/

/* Shared flags between Libq and Forms (not used by preprocessor) */
# define	II_TRIM		01
# define	II_LOWER	02
# define	II_CONV		(II_TRIM|II_LOWER)

/* LIBQ/FRS flags to be used with format string processor IIParProc() */
# define	II_PARRET	0x000	/* Param str describes sending vars */
# define	II_PARSET	0x001	/* Param str describes receiving vars */
# define	II_PARFRS	0x002	/* Param str from FRS statement */

/* LIBQ constants for setting/unsetting tracing */
# define	IITRC_OFF	0	/* Turn tracing to file off */
# define	IITRC_ON	1	/* Turn tracing to file on */
# define	IITRC_SWITCH	2	/* Update tracing state for session */
# define	IITRC_RESET	3	/* Close trace file temporarily */
# define	IITRC_HDLON	4	/* Turn tracing to handler on */
# define	IITRC_HDLOFF	5	/* Turn tracing to handler off */


/* 
** Data types - the three basic data types are taken from the main ingres
** constant headers.  The T_INT, T_FLOAT and T_CHAR and T_DBV constant values 
** must be the same as the corresponding Equel runtime values, defined 
*/

/*
** The values for names with DB_???_TYPE equivalents in a comment
*/
# define	T_NONE  	0	/* DB_NODT */
# define	T_STRUCT	1
# define	T_DATE		3	/* DB_DTE_TYPE - see below */
# define	T_PACKED	10	/* DB_DEC_TYPE */
# define	T_BYTE		28
# define	T_INT		30	/* DB_INT_TYPE */
# define	T_FLOAT		31	/* DB_FLT_TYPE */
# define	T_CHAR		32	/* DB_CHR_TYPE */
# define	T_DBV		44	/* DB_DBV_TYPE */
# define	T_NUL		50	/* DB_NUL_TYPE really DB_NULBASE_TYPE */
# define	T_PACKASCHAR	53	/* DB_DEC_CHR_TYPE */
# define	T_VCH		21	/* DB_VCH_TYPE */
# define	T_VBYTE		24	/* DB_VBYTE_TYPE */
# define	T_CHA		20	/* DB_CHA_TYPE */
# define	T_WCHAR		26	/* DB_NCHR_TYPE */
# define	T_NVCH		27	/* DB_NVCH_TYPE */
#define         T_BOO           38      /* DB_BOO_TYPE */
# define	T_SQLCA		96	/* IISQLCA */
# define	T_NOHINT	97	/* No valid hint for SQL */
# define	T_FORWARD	98	/* Base type not yet known */
# define	T_UNDEF  	99	/* DB_DT_ILLEGAL Undefined variable */
# define	T_HDLR		46	/* Datahandler type */
#define		T_ADTE		4	/* ANSI date type */
#define		T_TMWO		6	/* time without time zone */
#define		T_TMW		7	/* time with time zone */
#define		T_TME		8	/* Ingres time with local time zone */
#define		T_TSWO		9	/* timestamp without time zone */
#define		T_TSW		18	/* timestamp with time zone */
#define		T_TSTMP		19	/* Ingres timestamp with 
					** local time zone */
#define		T_INYM		33	/* interval (year-month) */
#define		T_INDS		34	/* interval (day-second) */

/*  Constants for Activate Command statements */

# define	c_CTRLY		0
# define	c_CTRLC		1
# define	c_CTRLQ		2
# define	c_CTRLESC	3
# define	c_CTRLT		4
# define	c_CTRLF		5
# define	c_CTRLG		6
# define	c_CTRLB		7
/* # define	c_CTRLV		8	reserved for EDIT option */
# define	c_CTRLZ		9
# define	c_MENUCHAR	10
# define	c_MAX		11

/* Scroll modes for scrolling table fields */
# define	sclUP		0
# define	sclDOWN		1
# define	sclTO		2
# define	sclNONE		3

/* Row modes passed by preprocessor to table fields */
# define	rowDOWN		(-6)	/* scroll current row down */
# define	rowUP		(-5)	/* scroll current row up */
# define	rowALL		(-4)	/* act on all displayed rows	*/
# define	rowNONE		(-3)	/* ignore row number on scroll	*/
# define	rowCURR 	(-2)	/* no row given -- use current	*/
# define	rowEND		(-1)	/* act on last row or record	*/

/* Table field Command modes - generated by preproccessor */
# define	cmSCROLL	1			/* ## SCROLL    */
# define	cmGETR		2			/* ## GETROW    */
# define	cmPUTR		3			/* ## PUTROW    */
# define	cmDELETER	4			/* ## DELETEROW */
# define	cmINSERTR	5			/* ## INSERTROW */
# define	cmCLEARR	6			/* ## CLEARROW	*/
# define	cmVALIDR	7			/* ## VALIDROW	*/
# define	cmPURGE		8			/* ## PURGETABLE */

/* Constants used in resume nextfield/previousfield */
# define	IIFRresNEXT	0	/* resume nextfield */
# define	IIFRresPREV	1	/* resume previousfield */

/* ESQL statement-types for response count (known also to LIBQ) */
# define	esqNUL		0
# define	esqDEL		1
# define	esqUPD		2
# define	esqINS		3

/* ESQL transaction definitions (known also to LIBQ) */
# define	esqXOFF		0
# define	esqXON		1
# define	esqXACT		(-1)

/* Constants known to both IIutsys.c and preprocessor */
# define	IIutPROG	0
# define	IIutARG		1
# define	IIutEXE		2

/* Constants to indicate what type of transaction statement is going on */
# define	IIxactBEGIN	0			/* Begin transaction */
# define	IIxactCOMMIT	1			/* End transaction */
# define	IIxactABORT	2			/* Abort/Rollback */
# define	IIxactSCOMMIT	3			/* Commit */

/* Constants to distinguish between starting/ending repeat/cursor statements */
# define	II_EXQREAD	0		/* Read returning DB_QRY_ID */
# define	II_EXQUERY	1		/* Define a repeat query */
# define	II_EXCURSOR	2		/* Define a cursor */

# define	II_EXCLRRDO	0		/* Reset "for readonly" */
# define	II_EXSETRDO	1		/* Set "for readonly" */
# define	II_EXVARRDO	2		/* Set/reset "for readonly" */
# define	II_EXGETRDO	3		/* Maybe send "for readonly" */

/* Constants to pass to IIsqMods */
# define	IImodSINGLE	1		/* Singleton SELECT */
# define	IImodWITH	2		/* CONNECT with WITH clause */
# define	IImodDYNSEL	3		/* EXEC IMMED SELECT */ 
# define	IImodNAMES	4		/* Get names in descriptors */
# define	IImodCPAD     	5		/* Ansi CHR blank-pad & EOS */	
# define	IImodNOCPAD 	6		/* Ingres EOS CHR_TYPE */
# define	IImodCEOS 	7		/* ANSI check EOS C string */
# define	IImodNOCEOS 	8		/* Ingres don't check EOS */

/* Constants to pass to IIsqParms */
# define	IIsqpINI	0		/* Initialize parms */
# define	IIsqpADD	1		/* Add parm */
# define	IIsqpEND	2		/* End message */

/* Constants to pass to IIsqDescribe */
# define	IIsqdNULUSE	0		/* No USING clause*/
# define	IIsqdNAMUSE	1		/* USING NAMES */
# define	IIsqdADFUSE	2		/* USING IIADFTYPES */

/* Constants to pass to IILQsidSessID */
# define	IIsqdisALL	-99		/* DISCONNECT ALL sessions */
# define	IIsqconSysGen	-98		/* CONNECT: generate sess id */
# define	IIsqsetNONE	-97		/* SET_SQL( SESSION = NONE ) */

/* Constants to support system generated session id's */
# define	IISESVECTORSIZE	 8		/* ~64 bits */
# define	IISESSHIFT	(IISESVECTORSIZE * sizeof(char)	* BITSPERBYTE)
						/* num of bits in ses vector */

/*
** Constants to indicate control chars passed to GCA by IIputctrl.
** (Used as index into run-time table - currently of length 1)
*/
# define	II_cNEWLINE	0		/* newline */

/*
** Constants to control actions of IILQpriProcInit.
** (Used as index into run-time table)
*/
# define	IIPROC_CREATE	0		/* create procedure */
# define	IIPROC_DROP	1		/* drop procedure */
# define	IIPROC_EXEC	2		/* execute procedure */


/* 
**  The following is a list of constants used by the preprocessor and at
**  runtime to represent the attributes of the following statements:
**	SET_SQL and INQUIRE_SQL
**	GET DATA and PUT DATA
**  The attributes of these statements are evaluated at pre-process time,
**  and the names of the attributes are converted from string to these
**  constants.
**
**  GET/PUT DATA attributes:
**
*/
# define   	gdSEGMENT	0	/* segment */
# define	gdSEGLENG	1	/* segmentlength */
# define	gdDATAEND	2	/* dataend */
# define	gdMAXLENG	3	/* maxlength */

/* Constants for flags variable passed to IILQlpd_LoPutData & IILQlgd_LoGetData
** for GET/PUT DATA statments. When adding new flags be sure to update any
** internal calls to these functions.  Currently, both COPY and the TM call
** these routines directly.
*/
# define	II_DATSEG	0x0001	/* segment attribute was specified */
# define	II_DATLEN	0x0002  /* seglen on PUT or maxlen on GET */
# define	II_BCOPY	0x0004  /* binary copy data */

/* [si,ss,and is] are all set/inqure sql attribute constants.
** These constants should never be changed. 
** For every element you add here here you must increment MAXTL_ELEMS 
** in eqtgt.h  MAXTL_ELEMS should be the highest number of attributes
** any one statement may have.
*/

# define	siDBMSER	0	/* dbmserror */
# define        siERHDLR        1       /* errorhandler */
# define	siERNO		2       /* errorno */
# define	siERTYPE	3       /* errortype */
# define        siDBEVHDLR      4       /* dbeventhandler */
# define        siMSGHDLR       5       /* messagehandler */
# define	siPFROWS        6	/* prefetchrows */
# define	siPRQUIT	7	/* programquit */
# define	siROWCNT	8	/* rowcount */
# define        siSAVEQRY       9 	/* savequery */
# define	siSAVEST       10	/* savestate */
# define	siSESSION      11	/* session */
# define	siCPYHDLR      12	/* copyhandler */
# define	siCONNAME      13	/* connection_target */
# define        siDBPRMPTHDLR  14	/* dbprompthandler */
# define	siSAVEPASSWD   15	/* savepassword */


/*
** Constants specific to set_sql attributes.  
*/

# define	ssERDISP	40	/* errordisp */
# define	ssERMODE	41	/* errormode */
# define	ssDBEVDISP	42	/* dbeventdisplay */
# define	ssGCAFILE	43	/* gcafile */
# define	ssPRTCSR	44	/* printcsr */
# define	ssPRTGCA	45	/* printgca */
# define	ssPRTTRC	46	/* printtrace */
# define	ssPRTQRY	47	/* printqry */
# define	ssQRYFILE	48	/* qryfile */
# define	ssTRCFILE       49      /* tracefile */

/*
** Constants specific to inquire_sql attributes. 
*/

# define	isDIFARCH       60      /* diffarch */
# define	isENDQRY	61	/* endquery */
# define	isERSEV         62      /* errorseverity */
# define	isERTEXT	63	/* errortext */
# define	isDBEVASY       64      /* dbeventasync */
# define	isHISTMP	65	/* highstamp */
# define	isIIRET         66      /* iiret */
# define	isLOSTMP	67	/* lowstamp */
# define	isMSGNO		68	/* messagenumber */
# define	isMSGTXT        69      /* messagetext */
# define	isOBJKEY	70	/* object_key */
# define	isQRYTXT	71	/* querytext */
# define	isTABKEY	72	/* table_key */
# define	isTRANS		73	/* transaction */
# define	isDBEVNAME	74	/* dbevent name */
# define	isDBEVOWNER	75	/* dbevent owner */
# define	isDBEVDB	76	/* dbevent database */
# define	isDBEVTIME	77	/* dbevent time */
# define	isDBEVTEXT	78	/* dbevent text */
# define	isCOLNAME	79	/* columname */
# define	isCOLTYPE	80	/* columntype */
# define	isCONTARG	81	/* connection_target */

/*
** Flags for passing FETCH orientation from parser to libq.
*/

# define fetFIRST      0x0001
# define fetLAST       0x0002
# define fetNEXT       0x0004
# define fetPRIOR      0x0008
# define fetABSOLUTE   0x0010
# define fetRELATIVE   0x0020

