/*
**	fedsz.h
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	fedsz.h - Front End Data Structure size header.
**
** Description:
**	This file contains definitions of front end data struture
**	size header information.  Used by feds and vifred.
**	The size of i4  is assumed to be 4 bytes on a pyramid.
**
** History:
**	?/?/? (?) - Written
**	05/07/87 (dkh) - Fixed to work with new 6.0 data structures.
**	06/01/87 (dkh) - Sync up with frame changes.
**	02/18/88 (boba) - Rename FSfrtrimno to FSfrtrmno.
**	13-jan-93 (sweeney) fixed up "trigraph" in comment above.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# ifdef	PYRAMID
# define	INCL_FEDSZ

/* used in feds!dsatt.c */
# define	ATatt_name	0
# define	ATatt_format	8
# define	ATatt_tformat	12

/* used in feds!dscs.c */
# define	CScs_flist	0
# define	CScs_tlist	4
# define	CScs_name	8

/* used in feds!dsfield.c */
# define	FLDTAG	0
# define	FLDVAR	4

/* used in feds!dsfldcol.c */
# define	CFHTITLE	44
# define	CFTDEFAULT	112
# define	CFTVALSTR	144
# define	CFTVALMSG	148
# define	CFTVALCHK	152
# define	CFTFMTSTR	156
# define	CFTFMT		160

/* used in feds!dsfldhdr.c */
# define	FHSfhtitle	44

/* used in feds!dsfldtype.c */
# define	FTdefault	0
# define	FTvalstr	32
# define	FTvalmsg	36
# define	FTvalchk	40
# define	FTfmtstr	44
# define	FTfmt		48

/* used in feds!dsfldval.c */
# define	FVfvbufr	8

/* used in feds!dsfmt.c */
# define	FMTfmt_template	8

/* used in feds!dsframe.c */
# define	FSfrfld		44
# define	FSfrfldno	48
# define	FSfrnsfld	52
# define	FSfrnsno	56
# define	FSfrtrim	60
# define	FSfrtrmno	64
# define	FSfrcurnm	124

/* used in feds!dslist.c */
# define	LTlt_next	0
# define	LTlt_utag	4
# define	LTlt_var	8

/* used in feds!dsmqqdef.c */
# define	MQmqtabs	16
# define	MQmqatts	56
# define	MQmqjoins	1136
# define	MQmqdata	1396

/* used in feds!dsrbfshds.c */
# define	SEn_n_attribs	0
# define	SOther	36
# define	SCs_top	40
# define	SCopt_top	44
# define	SPtr_att_top	48
# define	SEn_relation	52
# define	STokchar	56
# define	SEn_database	60
# define	SEn_report	64

/* used in feds!dsregfld.c */
# define	RFhtitle	44
# define	RFtdefault	112
# define	RFtvalstr	144
# define	RFtvalmsg	148
# define	RFtvalchk	152
# define	RFtfmtstr	156
# define	RFtfmt		160

/*
# define	RFvbufr		112
*/

/* used in feds!dstblfld.c */
# define	TFSfhtitle	44
# define	TFStfcols	144
# define	TFStfflds	164
# define	TFStfwins	168

/*
# define	TFStftoprow	156
# define	TFStfbotrow	160
*/

/* used in feds!dstrim.c */
# define	TStrmstr	24

/* used in feds!dsvallist.c */
# define	VLlisttag	0
# define	VLlistnext	4
# define	VLlistdata	8

/* used in feds!dsvalroot.c */
# define	VRlistroot	0

/* used in feds!dsvtree.c */
# define	Vdatatag	0
# define	Vptrtag	4
# define	Vv_left	16
# define	Vv_right	20
# define	Vv_data	24

/* used in feds!dsxframe.c */
# define	FRfrcurnm	104

/* used in vifred!dspos.c */
# define	PSps_name	16
# define	PSps_feat	24
# define	PSps_column	28

/* used in vifred!dsvfnode.c */
# define	VNnd_pos	4
# define	VNnd_adj	8

/* used in vifred!dswrtfrm.c */
# define	WFlinesize	0
# define	WFline	4
# define	WFframe	8
# endif	/* PYRAMID */
