/*
**	frserrno.h
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<erfi.h>

/**
** Name:	frserrno.h - Forms system error message numbers.
**
** Description:
**	This file contains the definitions of numbered error messages
**	that the forms system will use.  Placing numbers here
**	centralizes the numbers for easy lookup and allocation.
**	This file is composed of error numbers that were defined
**	in frame.h and runtime.h.
**
**	Errors are allocated as follows:
**		8000-8299    ???
**		8700-8719    Errors in range query checker.
**
** History:
**	02/18/87 (dkh) - Initial version.
**	03/12/87 (dkh) - Added definition for CV60BLEN.
**	5-may-1987 (Joe)
**		Added range query errors.
**	07/25/87 (dkh) - Fixed jup bug 515.  Added RTRNLNNL, RTSNLNNL,
**			 TBRNLNNL, TBSNLNNL.
**	09/03/87 (dkh) - Added definition of RTMUCNT.
**	09/05/87 (dkh) - Added numbers from "valid" directory.
**	09/05/87 (dkh) - Deleted unused error ids.
**	09/16/87 (dkh) - Integrated table field change bit.
**	29-nov-88 (bruceb)
**		Added SIBADTOV (bad timeout value) and
**		RTDBTACT (multiple activate timeouts in display loop.)
**	21-dec-88 (bruceb)
**		Added SITBLRO to indicate that set/inquire disponly attribute
**		isn't allowed on a table field.
**	31-jan-89 (bruceb)
**		Added SIOLDFRM to indicate that set/inquire disponly and
**		invisible are only allowed on non-old forms.
**	03-feb-89 (bruceb)
**		Added SIBADFMT to indicate specified format is bad for
**		the field's datatype, and SIFMTSZ to indicate specified
**		format is a bad size for the field.
**	06-feb-89 (bruceb)
**		Added SITBLFMT to indicate that set/inquire format not
**		allowed for table fields.
**	07-feb-89 (bruceb)
**		Added SIFDRVFMT to indicate that set format not
**		allowed for reverse fields, and SIFMTREV to indicate that
**		setting a field to a reverse format is not allowed.
**	13-feb-89 (bruceb)
**		Added SIINVALID to indicate that reformatting has failed
**		due to invalid field contents.
**	22-feb-89 (bruceb)
**		Added SISCRLJST to indicate that format specified for a
**		scrolling field is 'justified', which is illegal.
**/


/* ERROR MESSAGES */

# define	CFNOFRM		E_FI1F41_8001
# define	CFFRMS		E_FI1F42_8002
# define	CFFRAL		E_FI1F43_8003
# define	CFFLSQ		E_FI1F45_8005
# define	CFFLNS		E_FI1F46_8006
# define	CFFLAL		E_FI1F47_8007
# define	CFFLFMT		E_FI1F48_8008
# define	CFFLVAL		E_FI1F49_8009
# define	CFTBOC		E_FI1F4B_8011	/* Columns out of order */
# define	CFTBAL		E_FI1F4C_8012	/* Can't allocate Table */
# define	CFCLAL		E_FI1F4D_8013	/* Can't allocate column */
# define	CFFLTF		E_FI1F4E_8014	/* Too few fields */
# define	CFNSTF		E_FI1F4F_8015	/* Too few NS fields */
# define	CFCOTF		E_FI1F50_8016	/* Too few columns */
# define	CFCOOS		E_FI1F51_8017	/* Column out of range */
# define	RTTRMIN		E_FI1FA4_8100
# define	RTNOFRM		E_FI1FA5_8101
# define	RTFRACT		E_FI1FA6_8102
# define	RTFRIN		E_FI1FA7_8103
# define	RTDSMD		E_FI1FA8_8104
# define	RTFBFL		E_FI1FA9_8105
# define	RTRFFL		E_FI1FAA_8106
# define	RTSFEQ		E_FI1FAB_8107
# define	RTSFPC		E_FI1FAC_8108
# define	RTSFTF		E_FI1FAF_8111
# define	RTSFFP		E_FI1FB0_8112
# define	RTCLFL		E_FI1FB1_8113
# define	RTFREX		E_FI1FB2_8114
# define	RTMUCNT		E_FI1FB5_8117

# define	TBLNOFRM	E_FI1FB8_8120	/* no frame for table field */
# define	XCPTNOSET	E_FI1FBA_8122	/* couldn't set exception
						** handler
						*/


# define	FLTCONV		E_FI1FBF_8127	/* floating point conversion
						** error
						*/

# define	FRMNOINIT	E_FI1FC0_8128	/* specified frame isn't
						** initialized
						*/

# define	FDBADMODE	E_FI1FC1_8129	/* specified mode for frame
						** isn't legit
						*/

# define	RTGFERR		E_FI1FC2_8130	/* conv. error getting fld */
# define	RTPFERR		E_FI1FC3_8131	/* conv. error putting fld */
# define	RTGOERR		E_FI1FC4_8132	/* conv. err getting oper */
# define	RTPMERR		E_FI1FC5_8133	/* conv. err in prompt */
# define	RTSIERR		E_FI1FC6_8134	/* conv. err in set/inq */

# define	GFFLNF		E_FI2008_8200
# define	GFFLRG		E_FI200C_8204	/* Passed table field */
# define	PFFLNF		E_FI2012_8210

/*
**  Error numbers for the "valid" directory.
*/
# define	VALERR		E_FI206C_8300
# define	VALCHAR		E_FI206D_8301
# define	VALTYPE		E_FI206E_8302
# define	VALCONV		E_FI2070_8304
# define	VALNAME		E_FI2071_8305
# define	VALOPER		E_FI2072_8306
# define	VALSTRNG	E_FI2073_8307
# define	VALUCHR		E_FI2075_8309
# define	VALNOFLD	E_FI2077_8311
# define	VALNODB		E_FI2078_8312


/*
** Error flags from 8250 thru 8299 are reserved for Equel/Forms table 
** fields.
*/


/*
**  Forms conversion to 6.0 format error.
*/
# define	FDNOCVT60	E_FI2102_8450	/* error converting to 6.0 */


/*
** Set and inquire runtime errors.
*/
# define	SIOBJNF		E_FI1FD6_8150	/* Obj %0 not found */
# define	SIARGS		E_FI1FD7_8151	/* Insufficient args for
						** obj %0
						*/

# define	SIFRM		E_FI1FD8_8152	/* Frame %0 not found */
# define	SITBL		E_FI1FD9_8153	/* Table %0 not found */
# define	SIFLD		E_FI1FDA_8154	/* Field %0 not found */
# define	SICOL		E_FI1FDB_8155	/* Column %0 not found */
# define	SIATTR		E_FI1FDC_8156	/* Attribute %0 not found */
# define	SIATNUM		E_FI1FDD_8157	/* Bad datatype for numeric
						** attr %0
						*/

# define	SIATCHAR	E_FI1FDE_8158	/* Bad datatype for char
						** attr %0
						*/

# define	SIROW		E_FI1FDF_8159	/* More rows than in
						** table field
						*/

# define	SIRC		E_FI1FE0_8160	/* Column %0 not found
						** for row col
						*/

# define	SIDANOFF	E_FI1FE1_8161	/* Normal = off is not valid */
# define	SIBADCMD	E_FI1FE2_8162	/* bad command for map
						** and label
						*/

# define	SICTPFKY	E_FI1FE3_8163	/* unknow control/pf key */
# define	SIFLDDA		E_FI1FE4_8164	/* can't set D.A. on a TF */
# define	SIBCOLOR	E_FI1FE5_8165	/* bad color specified */
# define	SIBADCPF	E_FI1FE6_8166	/* bad control/pf key
						** specified
						*/
# define	SIBADTOV	E_FI1FE7_8167	/* bad timeout value */

# define	SITBLRO		E_FI1FE8_8168	/* no readonly on tblflds */
# define	SIOLDFRM	E_FI1FE9_8169	/* new attrib--old form */

# define	SIBADFMT	E_FI1FEA_8170	/* fmt wrong for datatype */
# define	SIFMTSZ		E_FI1FEB_8171	/* fmt size is wrong for fld */
# define	SITBLFMT	E_FI1FEC_8172	/* no formats for tblflds */
# define	SIFDRVFMT	E_FI1FED_8173	/* no set fmt for rev flds */
# define	SIFMTREV	E_FI1FEE_8174	/* no set fmt to rev fmt */
# define	SIINVALID	E_FI1FEF_8175	/* no set fmt for bad data */
# define	SISCRLJST	E_FI1FF0_8176	/* can't set justified fmt
						** for scrolling field
						*/


/*
** Table field runtime error numbers.
*/
# define	TBNODATSET	E_FI203A_8250	/* no data set linked
						** to table
						*/

# define	TBBAD		E_FI203B_8251	/* bad table name given
						** for frame
						*/

# define	TBNONE		E_FI203C_8252	/* no table name given */
# define	TBNOTACT	E_FI203D_8253	/* table not currently active */
# define	TBROWNUM	E_FI203E_8254	/* row number out of range */
# define	TBNOCOL		E_FI203F_8255	/* no column name given
						** in tb stmt
						*/

# define	TBBADCOL	E_FI2041_8257	/* column name not found
						** in table
						*/

# define	TBBADSCR	E_FI2042_8258	/* bad scroll mode given */
# define	TBEMPTSCR	E_FI2043_8259	/* empty table cannot scroll */
# define	TBEMPTDR	E_FI2044_8260	/* tried deleterow on
						** empty display
						*/

# define	TBEMPTGR	E_FI2045_8261	/* tried getrow on empty
						** table
						*/

# define	TBHCEXIST	E_FI2046_8262	/* cant hide col that
						** already exists
						*/

# define	TBHCTYPE	E_FI2047_8263	/* hiding col with bad
						** Ingres type
						*/

# define	TBUNNEST	E_FI2048_8264  	/* nesting unloadtab
						** statements
						*/

# define	TBDRINLIST	E_FI2049_8265	/* IN list with delrow
						** on data set
						*/

# define	TBSCINLIST	E_FI204A_8266	/* IN list with scroll
						** on data set
						*/

# define	TBINITMD	E_FI204B_8267	/* inittable with bad mode */
# define	TBDSINFO	E_FI204C_8268	/* _state, _record without
						** dataset
						*/

# define	TBRECNUM	E_FI204D_8269	/* bad record number in
						** scroll to
						*/

# define	TBQRYMD		E_FI204E_8270	/* query mode required
						** for _operator
						*/

# define	TBQRYHIDE	E_FI2050_8272	/* cannot query on hidden
						** column
						*/

# define	TBDTSET		E_FI2053_8275	/* set col - datatype
						** conversion
						*/

# define	TBDTRET		E_FI2054_8276	/* get col - datatype
						** conversion
						*/

# define	TBDTPOP		E_FI2055_8277	/* set coloper - datatype
						** conversion
						*/

# define	TBRNLNNL	E_FI2057_8279	/* ret col - null to non-null */
# define	TBSNLNNL	E_FI2058_8280	/* set col - null to non-null */


/*
** New runtime error numbers.
*/
# define	RTRESFLD	E_FI2017_8215	/* cannot resume on
						** specified field
						*/

# define	RTRESCOL	E_FI2018_8216	/* cannot resume on
						** specified column
						*/

# define	RTFRSKUSED	E_FI2019_8217	/* frs key already
						** activated on
						*/

# define	RTBADCHMU	E_FI201B_8219	/* control chars in menuitem */
# define	RTNOMURES	E_FI201C_8220	/* no menu to resume to */
# define	RTNOCB		E_FI201D_8221	/* couldn't allocate
						** control block
						*/

# define	RTRNLNNL	E_FI201E_8222	/* get from null to non-null */
# define	RTSNLNNL	E_FI201F_8223	/* set from null to non-null */
# define	RTCBNOROW	E_FI2020_8224	/* Not allowed to specify
						** a row number for set/inquire
						** on change bit when in
						** an unloadtable loop.
						*/

# define	RTCB0OR1	E_FI2021_8225	/* Value must be 0 or 1. */

# define	RTDBTACT	E_FI2022_8226	/* More than one activate
						** timeout block in a single
						** display loop.
						*/

/*
** 8700-8719
** Errors in range query checker.
*/
# define	FDRQNULL	E_FI21FC_8700	/* Null function pointer
						** to FErngchk
						*/

# define	FDRQLTXT	E_FI21FD_8701	/* Source not long text */
