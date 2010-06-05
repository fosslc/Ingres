/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/
#ifndef QR_INCLUDE
#define QR_INCLUDE

# ifndef	SI_INCLUDE
# include	<si.h>
# endif	/* SI_INCLUDE */

# ifndef	ADI_MXDTS
# include	<adf.h>
# endif	/* ADI_MXDTS */

#include	<fstm.h>

# include	<iirowdsc.h>

/**
** Name:	qr.h - header file for Query Runner module
**
** Description:
**	#defines and globals for Query Runner
**
** History:
**	86/09/08  12:58:52  peterk
**		initial version
**	86/10/14  13:05:23  peterk
**		use symbolic constant for MAXDOM;
**		add QRSD statement descriptor struct;
**		reorder QRB struct members;
**		eliminate fixed stmt buffer;
**		added/modified several QRB members:
** 			QRSD	*stmtdesc, *nextdesc;
** 			DB_DATA_VALUE dv[];
** 			ADF_CB	*adfscb;
** 			output buffer and pointers;
**		eliminate struct out_arg (using ADF_CB instead.)
**	86/11/03  11:19:05  peterk
**		added DV_LEN typedef, for default & worstcase lengths for
**		returned data-values; added array of MAXDOM DV_LEN's in QRB
**		struct to store lengths for each possible returned column
**		of a result.
**	86/11/06  10:06:36  peterk
**		mods for 6.0 LIBQ;
**	 	eliminate borrowed RET_DESC, TUP_DESC typedefs;
** 		eliminate extern reference to IItp_desc;
** 		eliminate QRB structure members .tuple (result tuple buffer),
** 		.td (TUP_DESC), .tv (array of ptrs to result attribute data),
** 		.dv (array of DB_DATA_VALUE structs for result attributes);
** 		added .da (ULC_SQLDA *) pointing to 6.0 tuple descriptor,
** 		and .dattnames (DB_ATT_NAME *) pointing to column names.
**	86/12/03  13:06:28  peterk
**		checking into 6.0 development environment.
**	87/01/27  14:53:43  peterk
**		change include SI.h to si.h
**	87/03/18  16:50:37  peterk
**		added sfirst member to QRB structure;
**		ifdef'ed around nexted includes.
**	87/04/02  14:21:52  peterk
**		in QRB struct, replace da and dattnames members by new ROW_DESC
**		structure (in iirowdesc.h).
**	87/04/08  01:42:25  joe
**		Added compat, dbms and fe.h
**	18-aug-87 (daver) added QRVIEW, QRINTEGRITY and QRPERMIT definitions.
**	12-aug-88 (bruceb)
**		Added putfunc function pointer.
**	27-sep-88 (bruceb)
**		Removed QRVIEW, QRINTEGRITY and QRPERMIT definitions.
**	12-oct-88 (kathryn) bugs 2763, 3269
**		Added qrptr to QRB structure. - Points to next char of
**		output to be displayed.
**	28-sep-89 (teresal)
**		Added define and a global variable for Runtime INGRES.
**	23-oct-89 (sylviap)
**		Copied qr.h from tm/hdr to $HDR2 for the garnet integrations.
**		Left the RCS history in tm/hdr because r63/piccolo still
**		expect it to be in tm/hdr.
**	28-nov-89 (teresal)
**		Removed global variable Runtime_INGRES because it was causing
**		build problems for Report Writer.  Note this global is now
**		defined and referenced in the individual QR files.
**	01-aug-91 (kathryn)
**		Added boolean "striptok" to QRB.  This boolean indicates that 
**		newlines (<cr>) must be stripped from current token before 
**		sending to DBMS.  Set when current token is a string literal
**		containing a nl.
**	5-oct-92 (rogerl)
**		Add qrb->tm as 'fetchret()' needs to know whether to output
**		blob values (TM) or not (FSTM, other).  Delete growbuf.
**	23-apr-96 (chech02)
**		added function prototypes for windows 3.1 port.
**	18-jul-96 (chech02) bug # 77880
**		changed to __huge * in QRB to access query buffer that
**		larger than 64K for windows 3.1 port.
**	13-mar-2001 (somsa01)
**	    Change members of DV_LEN to be i4's.
**	02-Mar-2003 (hanje04)
**	    Prototype qrrun for all platforms.
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**	    Drop the old WIN16 section, misleading and out of date.  Better
**	    to just reconstruct as needed.
**	11-nov-2009 (wanfr01) b122875
**	    Added paramter to qrinit
**	    Added Outisterm to QRB
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
** QRB_BUF_SIZ needs enough room for LIFOBUF structure + max tuple
** length + room for alignment of each domain.
*/
# define	QRB_BUF_SIZ	2536

/*
**	DV_LEN - struct to hold lengths associated with formatting
**	of DB_DATA_VALUEs.  The values are obtained by calling the
**	ADF routine adm_tmlen().
*/
typedef struct {
	i4	deflen;
	i4	worstlen;
} DV_LEN;
    

/*}
** Name:	QRSD - QR Stmt Descriptor
**
** Description:
**	Describes a statement that has been recognized, including
**	the initial keyword and a statement type constant.
**	If the initial keyword token requires a 2nd following keyword
**	token to qualify as a legitimate keyword (e.g. BEGIN
**	TRANSACTION or DEFINE PERMIT), then the list of possible
**	2nd keywords is given as .qrsd_follow.
**
** History:
**	9/29/86 (peterk) - created.
**	7/11/07 (kibro01) b119432
**		Added SETstmt to handle set-type statements
*/
typedef struct
{
	char	*qrsd_key;	/* initial keyword	*/
	i4	qrsd_type;	/* statement type	*/

# define	NOstmt		0	/* not a statement */
# define	PLAINstmt	1	/* ordinary stmt (none of the below) */
# define	RETSELstmt	2	/* RETRIEVE (QUEL) or SELECT (SQL) */
# define	PRINTstmt	3	/* PRINT statement (QUEL) */
# define	HELPstmt	4	/* HELP statement */
# define	RUNTIMEstmt	5	/* RUNTIME INGRES statement */
# define	SETstmt		6	/* SET statement */

	char	**qrsd_follow;	/* list of 2nd keywords or NULL */
} QRSD;


/*}
** Name:	QRB - Query Runner control Block
**
** Description:
**	The general control block structure used by QR routines.
**
** History:
**	8/86 (peterk) - created.
**	20-aug-01 (inkdo01)
**	    Added casecnt to track case funcs (and corresponding "end"s)
**	    inside create procedure.
**      27-oct-2004 (huazh01)
**          Added 'help_stmt'. True if user is running a detailed
**          help statement. 
**          b64679, INGCBT545.
*/

# define	QRTOKENSIZ	100

typedef struct {
	i4	lang;			/* QUEL/SQL indicator	*/

# define	qrQUEL	0
# define	qrSQL	1

	char	*inbuf;			/* literal input string	*/
	char	(*infunc)();		/* input function	*/
	i4	(*contfunc)();		/* continue function	*/
	i4	(*errfunc)();		/* error function	*/
	FILE	*script;		/* scripting file	*/
	bool	echo;			/* echo statement flag	*/
	bool	nosemi;			/* no-semicolon flag	*/

	i4	(*saveerr)();		/* saved error function	*/
	i4	(*savedisp)();		/* saved IIdisperr fcn	*/

	char	token[QRTOKENSIZ];	/* token buffer		*/
	char	*t;			/* next byte in token	*/
        bool    striptok;               /* strip nl from token for DBMS */
	char	*stmt;		 	/* statement echo buffer	*/
	i4	stmtbufsiz;		/* stmt buffer size	*/
	char	*s;			/* next byte in stmt	*/
	i4	sfirst;			/* index of first real token	*/
	i4	sno;			/* statement number	*/
	QRSD	*stmtdesc;		/* cur. stmt descriptor	*/
	QRSD	*nextdesc;		/* next stmt descriptor	*/
	i4	lines;			/* line no. in go-block */
	i4	stmtoff;		/* line offset of stmt	*/
	i4	step;			/* step in doing stmt	*/
	char	peek;			/* peek character	*/

	bool	nonnull;		/* nonnull stmt flag	*/
	bool	eoi;			/* end-of-input flag	*/
	bool	comment;		/* in-a-comment flag	*/
	bool	nobesend;		/* don't send to BE flag */
	bool	norowmsg;		/* don't print the (N rows) message */
        bool    help_stmt;              /* TRUE if this is a detailed 
                                        ** help statement */
        bool    Outisterm;              /* TRUE if output is to terminal */

	ROW_DESC *rd;			/* data row descriptor	*/
	DV_LEN	dvlen[DB_GW2_MAX_COLS];	/* lens for dv's in rd	*/
	ADF_CB	*adfscb;		/* ADF control block	*/

	i4	error;			/* BE error number	*/
	i4	severity;		/* error severity level	*/
	char	*errmsg;		/* ptr to error message */

	VOID	(*flushfunc)();		/* flush (error output) function */
	PTR	flushparam;		/* flush function parameters */

	char 	*qrptr;			/* output pointer 	*/
	char	*outbuf;		/* output buffer	*/
	i4	outlen;			/* output buffer length	*/
	char	*outlin;		/* next output string	*/
	char	*outp;			/* end-of-output ptr	*/
	bool	endtrans;		/* need end transaction for help all */

	VOID	(*putfunc)();		/* put out (normal output) function */

	bool	tm;			/* true if program is TM */
	i4	casecnt;		/* count of outstanding "case" funcs */
	i4	col_maxname;		/* for formatting < DB_MAXNAME */
} QRB;


STATUS qrinit(QRB *qrb, bool semi, char	*inbuf, char	(*infunc)(),
            STATUS	(*contfunc)(), i4	(*errfunc)(), FILE	*script,
            bool	echo, DB_DATA_VALUE 	*outbuf, ADF_CB	*adfscb, 
            VOID	(*flfunc)(),    PTR	flparam, VOID	(*putfunc)(), 
            bool	tm, bool Outisterm);

FUNC_EXTERN bool IIUFdsp_Display(BCB *, QRB *, char *, char *, bool, char *, char *, bool);

# endif	/* QR_INCLUDE */
