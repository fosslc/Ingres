/*
**  fsiconsts.h
**
**  Constants for frs set and inquire under 4.0.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 08/12/85 (dkh)
**	Added definitions for preprocessor interface - 08/29/85 (ncg)
**	12/22/86 - (dkh) Added support for new activations.
**	86/11/19  barbara - Initial60 edit of files
**	85/12/05  dave - Changed FsiATTR_MAX to 38. (dkh)
**	85/12/04  neil - Check-8 on FsiVALID
**	85/12/04  neil - Added FsiACTIVATE as intermediate value.
**	85/12/04  dave - Added new constants FsiCOLOR, FsiNXACT and
**			 FsiPRACT. (dkh)
**	85/10/05  dave - Added new constant FsiCMDMAX. (dkh)
**	85/10/05  dave - Added new constants FsiPRSCR and FsiSKPFLD. (dkh)
**	85/09/05  neil - Added maximum for menuitems.
**	85/09/03  neil - Added defintion of max objects = 40.
**	85/08/30  dave - Swapped values between FsiESC_CTRL and
**			 FsiDEL_CTRL. (dkh)
**	85/08/30  dave - Added warning message. (dkh)
**	85/08/30  dave - Reassigned constants to reduce code size. (dkh)
**	85/08/30  neil - Chenged SI_ to Fsi and added a few EQUEL defines.
**	85/08/22  dave - Initial revision
**
**	07/28/87 (dkh) - Added FsiDATYPE.
**	09/12/87 (dkh) - Added FsiERRTXT.
** 	02/27/88 (dkh) - Added FsiNXITEM and fixed up changes made by IBM.
**	04/12/88 bobm  - Added FSP_*, FSBD_*, FSSTY_*, some Fsi's
**	05/05/88 marge - Added FSV_FLOATING
**	07/13/88 marge - Added FSBD_RDFORM
**	07/22/88 marge - Changed FSBD_NONE, FSBD_SINGLE to match user constant
**			 value.
**	20-dec-88 (bruceb)
**		Added FsiRDONLY and FsiINVIS.
**	24-apr-89 (bruceb)
**		Added FsiSHLKEY.
**	07/03/89 (dkh) - Added support for 80/132 and arrow key mappings;
**			 FSP_SCRWID, FSSW_CUR, FSSW_DEF, FSSW_NARROW, FSSW_WIDE,
**			 FsiUPARROW, FsiDNARROW, FsiLTARROW, FsiRTARROW.
**	07/04/89 (dkh) - Rdefined values for FSSW_CUR, FSSW_DEF, FSSW_NARROW,
**			 and FSSW_WIDE.
**	08/04/89 (dkh) - Fixed transposition of FsiLTARROW and FsiRTARROW.
**	08/10/89 (dkh) - Added definition of FsiLFRSKEY (supports inquiring on
**			 last frskey that caused an activation.
**	08/11/89 (dkh) - Swapped values for FSSW_CUR and FSSW_DEF.
**	08/28/89 (dkh) - Increased FsiATTR_MAX to 57 to account for FsiLFRSKEY.
**	27-sep-89 (bruceb)
**		Added FsiEDITKEY and increased FsiATTR_MAX to 58.
**	10/06/89 (dkh) - Added FsiDRVED and FsiDRVSTR.
**	12/06/89 (dkh) - Added FsiDEC_PREC, FsiDEC_SCL and FsiDEC_SZE.
**	12/26/89 (dkh) - Changed FsiDEC_SZE to FsiDEC for uniqueness.
**	07-dec-89 (bruceb)
**		Added FsiGMSGS for Getmessage display/supression.
**		Added FsiKEYFIND for single character find function.
**	01/11/90 (esd) - Modifications for FT3270 (bracketed by #ifdef)
**			 Only change is to number of PF keys.
**	08/19/90 (esd) - Modifications for hp9_mpe (bracketed by #ifdef)
**			 Only change is to number of PF keys.
**	03/21/91 (dkh) - Added support for (alerter) events in FRS.
**	08/28/92 (dkh) - Added new definitions for 6.5.
**	09/01/92 (dkh) - Added more stuff for 6.5.
**	14-jan-1993 (lan)
**		Added FsiDEC_FMT for decimal_format.
**	01/20/93 (dkh) - Added FsiEMASK for inputmasking toggle.
**	01/21/93 (dkh) - Make FsiEMASK the same value as FsiDEC_FMT.  The
**			 latter is unnecessary since FsiDEC should be used.
**	10-mar-1993 (sylviap)
**		Took out FsiDEC_FMT since we use FsiDEC.
*/

/*
**********************************************************************
**
**  WARNING!!!!!!!!!!!!!!
**
**  Do not change the values of the constants below unless the
**  corresponding changes are also made in file MAPPING.H in
**  the frontcl facility header directory. (dkh)
**
**********************************************************************
*/


/*
**  Defines for object-type identification.
*/

# define	FsiFRS			0	/* frs object */
# define	FsiFORM			1	/* form object */
# define	FsiFIELD		2	/* field object */
# define	FsiTABLE		3	/* table object */
# define	FsiCOL			4	/* tf column object */
# define	FsiROW			5	/* tf row object */
# define	FsiRWCL			6	/* tf row/col (cell) object */
# define	FsiMUOBJ		7	/* menu object */


/*
** Defines for preprocessor mappings from frs-constants that are only temps
** These must be negative.
*/

# define	FsiBAD			(-1)
# define	FsiVALDATE		(-2)
# define	FsiACTIVATE		(-3)

/*
** Maximum number of different objects (PF, MENU, FRSKEY)
*/
# ifdef  FT3270
# ifdef  hp9_mpe
# define	Fsi_KEY_PF_MAX		9
# else   /* ~hp9_mpe */
# define	Fsi_KEY_PF_MAX		25
# endif  /* hp9_mpe */
# else   /* ~FT3270 */
# define	Fsi_KEY_PF_MAX		40
# endif  /* FT3270 */

# define	Fsi_MU_MAX		25

/*
**  Defines for frs-constant identification.
*/

# define	FsiATTR_MAX		70

# define	FsiTERMINAL		0
# define	FsiERRORNO		1
# define	FsiNXFLDVAL		2
# define	FsiPRFLDVAL		3
# define	FsiMENUVAL		4
# define	FsiKEYSVAL		5
# define	FsiANYMUVAL		6
# define	FsiMENUMAP		7
# define	FsiMAP			8
# define	FsiLABEL		9
# define	FsiMAPFILE		10
# define	FsiPCSTYLE		11
# define	FsiSTATLN		12
# define	FsiFRSKVAL		13
# define	FsiNORMAL		14
# define	FsiRVVID		15
# define	FsiBLINK		16
# define	FsiUNLN			17
# define	FsiCHGINT		18
# define	FsiNAME			19
# define	FsiCHG			20
# define	FsiFMODE		21
# define	FsiFLD			22
# define	FsiNUMBER		23
# define	FsiLENGTH		24
# define	FsiTYPE			25
# define	FsiFORMAT		26
# define	FsiVALID		27
# define	FsiFLDTBL		28
# define	FsiROWNO		29
# define	FsiMAXROW		30
# define	FsiLASTROW		31
# define	FsiDATAROWS		32
# define	FsiMAXCOL		33
# define	FsiTBLCOL		34
# define	FsiCOLOR		35
# define	FsiNXACT		36
# define	FsiPRACT		37
# define	FsiLCMD			38
# define	FsiMUACT		39
# define	FsiMIACT		40
# define	FsiKYACT		41
# define	FsiDATYPE		42
# define	FsiERRTXT		43
# define	FsiITHEIGHT		44
# define	FsiITWIDTH		45
# define	FsiITSROW		46
# define	FsiITSCOL		47
# define	FsiCROW			48
# define	FsiCCOL			49
# define	FsiTIMEOUT		50
# define	FsiENTACT		51
# define	FsiDELROW		52
# define	FsiRDONLY		53
# define	FsiINVIS		54
# define	FsiSHLKEY		55
# define	FsiLFRSKEY		56
# define	FsiEDITKEY		57
# define	FsiDRVED		58
# define	FsiDRVSTR		59
# define	FsiDEC_PREC		60	/* decimal_precision */
# define	FsiDEC_SCL		61	/* decimal_scale */
# define	FsiDEC			62
# define	FsiGMSGS		63
# define	FsiKEYFIND		64
# define	FsiEXISTS		65	/* the exists option */
# define	FsiMURENAME		66	/* the menu rename option */
# define	FsiMUACTIVE		67	/* the menu active option */
# define	FsiODMSG		68	/* out of data msg handling */
# define	FsiEMASK		69	/* inputmasking toggle */


/*
**  Please note that frskeys are numbered from 4000 to 4999.
*/

# define	FsiFRS_OFST		4000


/*
**  Please noe that menuitem positions are numbered from
**  3000 to 3999.
*/

# define	FsiMU_OFST		3000


/*
**  Forms system command definitions.
*/

# define	FsiNEXTFLD		2000
# define	FsiPREVFLD		2001
# define	FsiNXWORD		2002
# define	FsiPRWORD		2003
# define	FsiMODE			2004
# define	FsiREDRAW		2005
# define	FsiDELCHAR		2006
# define	FsiRUBOUT		2007
# define	FsiEDITOR		2008
# define	FsiLEFTCH		2009
# define	FsiRGHTCH		2010
# define	FsiDNLINE		2011
# define	FsiUPLINE		2012
# define	FsiNEWROW		2013
# define	FsiCLEAR		2014
# define	FsiCLREST		2015
# define	FsiMENU			2016
# define	FsiSCRLUP		2017
# define	FsiSCRLDN		2018
# define	FsiSCRLLT		2019
# define	FsiSCRLRT		2020
# define	FsiDUP			2021
# define	FsiPRSCR		2022
# define	FsiSKPFLD		2023

# define	FsiTPSCRLL		2024	/* FT3270 use only */
# define	FsiBTSCRLL		2025	/* FT3270 use only */
# define	FsiCOMMAND		2026	/* FT3270 use only */
# define	FsiDIAG			2027	/* FT3270 use only */

# define	FsiSHELL		2028
# define	FsiNXITEM		2029

# define	FsiCMDMAX		2029


/*
**  Defines for control key representation.
*/

# define	FsiA_CTRL		5000
# define	FsiB_CTRL		5001
# define	FsiC_CTRL		5002
# define	FsiD_CTRL		5003
# define	FsiE_CTRL		5004
# define	FsiF_CTRL		5005
# define	FsiG_CTRL		5006
# define	FsiH_CTRL		5007
# define	FsiI_CTRL		5008
# define	FsiJ_CTRL		5009
# define	FsiK_CTRL		5010
# define	FsiL_CTRL		5011
# define	FsiM_CTRL		5012
# define	FsiN_CTRL		5013
# define	FsiO_CTRL		5014
# define	FsiP_CTRL		5015
# define	FsiQ_CTRL		5016
# define	FsiR_CTRL		5017
# define	FsiS_CTRL		5018
# define	FsiT_CTRL		5019
# define	FsiU_CTRL		5020
# define	FsiV_CTRL		5021
# define	FsiW_CTRL		5022
# define	FsiX_CTRL		5023
# define	FsiY_CTRL		5024
# define	FsiZ_CTRL		5025
# define	FsiESC_CTRL		5026
# define	FsiDEL_CTRL		5027


/*
**  Defines for function key representation start
**  at 6000.
*/

# define	FsiPF_OFST		6000

/*
**  Magic values for arrow key mappings.
*/

# define	FsiUPARROW		7000	/* up arrow const for set/inq */
# define	FsiDNARROW		7001	/* down arrow for set/inq */
# define	FsiRTARROW		7002	/* right arrow for set/inq */
# define	FsiLTARROW		7003	/* left arrow for set/inq */


/*
** Global parameter defn's
*/

/*
** state args for IIFRgpcontrol
*/
# define FSPS_OPEN	1
# define FSPS_CLOSE	2
# define FSPS_RESET	3

/*
** pid's - These are used as array indices at runtime, so don't make
** them too large.  They should also be non-negative.
*/
# define FSP_SROW	1	/* starting row */
# define FSP_SCOL	2	/* starting col */
# define FSP_ROWS	3	/* number of rows */
# define FSP_COLS	4	/* number of cols */
# define FSP_BORDER	5	/* type of border */
# define FSP_STYLE	6	/* style of message or form */
# define FSP_SCRWID	7	/* screen width specified */

/*
** border types
*/
# define FSBD_DEF	200	/* default border */
# define FSBD_NONE	1	/* WARNING: USER-VISIBLE CONSTANTS */
# define FSBD_SINGLE	2	/* WARNING: USER-VISIBLE CONSTANTS */
# define FSBD_RDFORM	203	/* Read form for border */

/*
** styles
*/
# define FSSTY_DEF	300	/* default style */
# define FSSTY_POP	301	/* popup style */
# define FSSTY_NOPOP	302	/* nopop style */

/*
** values
*/
# define FSV_FLOATING	-1	/* use "floating" position */

/*
**  Screen width options.
*/
# define	FSSW_DEF	0	/* use width defined in vifred */
# define	FSSW_CUR	1	/* use current screen width */
# define	FSSW_NARROW	2	/* use narrow screen width */
# define	FSSW_WIDE	3	/* use wide screen width */

/*
**  Magic values for inquring on the various components of
**  an (alerter) event.  This must be in sync with the preprocessors.
**  These will eventually get moved to fsicnts.h.
*/
# define	AE_NAME		0	/* name component of event */
# define	AE_OWNER	1	/* owner component of event */
# define	AE_DB		3	/* database component of event */
# define	AE_TIME		4	/* time component of event */
# define	AE_TEXT		5	/* text component of event */
