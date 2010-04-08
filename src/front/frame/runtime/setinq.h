/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	setinq.h  -  Header for EQUEL/FORMS
**
**	These are constants that are shared by both the Equel pre-processor
**	and the Forms Runtime System.  Therefore any change in these constants
**	are reflected in both systems.
**
**  History:
**	07-20-83  -  Commented (jen)
**	07-20-83  -  Added Neil's constants for FRS data (jen)
**	23-mar-84 -  Added frsFLDTBL and frsFLDMODE option (ncg)
**	8-july-85 -  Added ifdef ATTCURSES the defines for
**			AT&T's function keys--probably goes away
**			again in Ingres 4.0.
**	12/23/86 (dkh) - Added support for new activations.
**	07/28/87 (dkh) - Added definition of frsDATYPE.
**	09/12/87 (dkh) - Added frsERRTXT.
**	11-nov-88 (bruceb)
**		Added frsTMOUT.
**	20-dec-88 (bruceb)
**		Added frsRDONLY and frsINVIS.
**	09-mar-89 (bruceb)
**		Added frsENTACT.
**	24-apr-89 (bruceb)
**		Added frsSHELL.
**	08/28/89 (dkh) - Added support for inquiring on frskey that
**			 caused an activation.
**	27-sep-89 (bruceb)
**		Added frsEDITOR.
**	10/06/89 (dkh) - Added support for inquiring on derivation string
**			 and whether a field/column is derived.
**	12/07/89 (dkh) - Added support for inquiring on size information
**			 of a decimal field type.
**	07-feb-90 (bruceb)
**		Added support for Getmessage display/supression.
**		Added support for single character find feature.
**	08/28/92 (dkh) - Added support for inquiring on existence of frs object.
**	09/01/92 (dkh) - Added support for dynamic menus, etc.
**	01/20/93 (dkh) - Added support for inputmasking toggle.
*/

/*
**  Constants for activate command statements.
*/
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
# ifdef	ATTCURSES
# define	c_F1		11	/* for AT&T key_f1, RTI frskey1 */
# define	c_F2		12
# define	c_F3		13
# define	c_F4		14
# define	c_F5		15
# define	c_F6		16
# define	c_F7		17
# define	c_F8		18	/* for AT&T key_f8, RTI frskey8 */
# define	c_MAX		19
# else /* notATTCURSES */
# define	c_MAX		11
# endif

/*
**  Constants for indicating mode in display statement.
*/
# define	fdrtNOP		0
# define	fdrtUPD		1
# define	fdrtINS		2
# define	fdrtRD		3
# define	fdrtNAMES	4
# define	fdrtQRY		5


/*
**  Constants for FRS data values.
*/

# define	frsTERM		1		/* terminal type */
# define	frsERRNO	2		/* error number */

					/* regular field FRS flags 	*/
# define	frsFLDNM	1			/* _field 	*/
# define	frsFLDNO	2			/* _fieldno 	*/
# define	frsFLDLEN	3			/* _length	*/
# define	frsFLDTYPE	4			/* _type	*/
# define	frsFLDFMT	5			/* _format	*/
# define	frsFLDVAL	6			/* _valid	*/
# define	frsFLDTBL	7			/*   is table	*/
# define	frsFLDMODE	8			/*   mode	*/
# define	frsFLD		9

					/* frame FRS flag		*/
# define	frsFRMCHG	11			/* _change	*/

					/* table field FRS flags	*/
# define	frsTABLE	21			/* _table	*/
# define	frsROWNO	22			/* _rowno	*/
# define	frsROWMAX	23			/* _maxrow	*/
# define	frsLASTROW	24			/* _lastrow	*/
# define	frsCOLMAX	25			/* _maxcol	*/
# define	frsCOLUMN	26			/* _column	*/
# define	frsDATAROWS	27			/* datarows	*/

					/* runtime detail flags		*/
# define	frsRUNERR	31			/* _error	*/


# define	frsVALNXT	40		/* validate to next field */
# define	frsVALPRV	41		/* validate to prev field */
# define	frsVMUKY	42		/* validate on menu key */
# define	frsVKEYS	43		/* validate on any frs key */
# define	frsVMENU	44		/* validate on a menu item */
# define	frsMENUMAP	45		/* menumap command */
# define	frsMAP		46		/* mapping commands */
# define	frsLABEL	47		/* label command */
# define	frsMAPFILE	48		/* mapping file */
# define	frsPCSTYLE	49		/* pc style menus */
# define	frsSTATLN	50		/* status line */
# define	frsVFRSKY	51		/* validate on a frs key */
# define	frsNORMAL	52		/* normal display */
# define	frsRVVID	53		/* reverse video display */
# define	frsBLINK	54		/* blinking display */
# define	frsUNLN		55		/* underline display */
# define	frsINTENS	56		/* change display intensity */
# define	frsCOLOR	57		/* color value */
# define	frsACTNXT	58		/* activate to next field */
# define	frsACTPRV	59		/* activate to prev field */
# define	frsLCMD		60		/* last frs command */
# define	frsAMUKY	61		/* activate on menu key */
# define	frsAKEYS	62		/* activate on any frs key */
# define	frsAMUI		63		/* activate on any menu item */
# define	frsDATYPE	64		/* 6.0 datatype for field */
# define	frsERRTXT	65		/* 6.0 frs error text */
# define	frsSROW		66		/* starting row */
# define	frsSCOL		67		/* starting column */
# define	frsCROW		68		/* current row */
# define	frsCCOL		69		/* current column */
# define	frsHEIGHT	70		/* height of objects */
# define	frsWIDTH	71		/* width of objects */
# define	frsTMOUT	72		/* num seconds until timeout */
# define	frsRDONLY	73		/* readonly field attribute */
# define	frsINVIS	74		/* invisible field attribute */
# define	frsENTACT	75		/* activate on field entry */
# define	frsSHELL	76		/* turn on/off the shell key */
# define	frsLFRSK	77		/* frskey that caused act */
# define	frsEDITOR	78		/* turn on/off the editor key */
# define	frsDRVED	79		/* is field/column derived */
# define	frsDRVSTR	80		/* derivation string */
# define	frsDPREC	81		/* precision of decimal */
# define	frsDSCL		82		/* scale of decimal */
# define	frsDSZE		83		/* prec & scale of decimal */
# define	frsGMSGS	84		/* 'get' error message on/off */
# define	frsKEYFIND	85		/* single char find on/off */
# define	frsEXISTS	86		/* does a frs object exists */
# define	frsMURNME	87		/* rename a menu item */
# define	frsMUONOF	88		/* toggle a menu item on/off */
# define	frsODMSG	89		/* out of data msg handling*/
# define	frsEMASK	90		/* inputmasking toggle */
