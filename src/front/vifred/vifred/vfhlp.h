/*
**	vfhlp.h
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	vfhlp.h - Vifred/Rbf help files declarations.
**
** Description:
**	This file contains the name of help files for Vifred/Rbf.
**
** History:
**	10/02/87 (dkh) - Initial version.
**	07/27/88 (dkh) - Shortened names to conform to coding standards.
**	09/21/88 (tom) - Changed name of VFH_UTILSMU so it's the same
**			 as the file (bug: 3368)
**
**	05/17/89 (dkh) - Fixed bug 3757.
**	20-jun-89 (bruceb)
**		Added VFH_VALID, VFH_DERIVE and VFH_DERERR.
**	07-sep-89 (bruceb)
**		Added VFH_EDTRIM and VFH_TRMATTR.
**	02-nov-89 (bruceb)
**		Added VFH_CRDUP, VFH_CRJDEF, VFH_LSDUP and VFH_LSJDEF.
**	01-dec-89 (bruceb)
**		Added VFH_DSTFRM, VFH_DSTQBF, VFH_SVEND, RFH_SVEND,
**		VFH_SVQUIT and RFH_SVQUIT.
**	09/22/92 (dkh) - Added VFH_GRPMV - help file for group move.
**			 Added VFH_RUCTRL - help file for ruler control.
**	09/23/92 (dkh) - Added VFH_SEMOV - help file for moving straight edges.
**			 Added VFH_QTBLNM - help file for prompting for
**			   a table name linked to a qbfname.
**			 Added VFH_QJDFNM - help file for prompting for
**			   a joindef name linked to a qbfname.
**	26-jan-94 (connie) Bug #57718
**		Added VFH_QJDFLS - help file for listchoice in entering a 
**		joindef name linked to a qbfname.
**/

# define	VFH_ATTRIBUTE		ERx("vfattr.hlp")
# define	VFH_VALID		ERx("vfvalid.hlp")
# define	VFH_DERIVE		ERx("vfderive.hlp")
# define	RFH_LAYOUT		ERx("rflayout.hlp")
# define	VFH_CURSOR		ERx("vfcursor.hlp")
# define	RFH_DETAIL		ERx("rbdetail.hlp")
# define	VFH_EDFLD		ERx("vfedfld.hlp")
# define	VFH_NEWFLD		ERx("vffield.hlp")
# define	VFH_QNSMU		ERx("vfqbfsmu.hlp")
# define	VFH_QNCAT		ERx("vfqbfcat.hlp")
# define	RFH_TMOVE		ERx("rbftmove.hlp")
# define	VFH_SELECT		ERx("vfselect.hlp")
# define	RFH_MOVE		ERx("rbfmove.hlp")
# define	VFH_SLFLD		ERx("vfslfld.hlp")
# define	RFH_SLDET		ERx("rbfsldet.hlp")
# define	RFH_SLCOL		ERx("rbfslcol.hlp")
# define	RFH_CMOVE		ERx("rbfcmove.hlp")
# define	VFH_SLCOM		ERx("vfslcom.hlp")
# define	VFH_EXPSHR		ERx("vfexpshr.hlp")
# define	VFH_RESEQ		ERx("vfreseq.hlp")
# define	VFH_TFMOVE		ERx("vftfmvsm.hlp")
# define	VFH_TBLFLD		ERx("vftblfld.hlp")
# define	VFH_VALERR		ERx("vfvalerr.hlp")
# define	VFH_DERERR		ERx("vfdererr.hlp")
# define	VFH_TABCR		ERx("vfcrtbsb.hlp")
# define	VFH_CRDUP		ERx("vfcrdup.hlp")
# define	VFH_CRJDEF		ERx("vfcrjdef.hlp")
# define	VFH_LSDUP		ERx("vflsdup.hlp")
# define	VFH_LSJDEF		ERx("vflsjdef.hlp")
# define	VFH_CREATE		ERx("vfcresb.hlp")
# define	VFH_UTILSMU		ERx("vfutsmu.hlp")
# define	VFH_CATALOG		ERx("vfcat.hlp")
# define	VFH_ECREATS		ERx("vfecreat.hlp")
# define	RFH_ECREATS		ERx("rfecreat.hlp")
# define	VFH_EDBOX		ERx("vfedbox.hlp")

# define	VFH_BOXATTR		ERx("vfboxatr.hlp")
# define	VFH_BOXMOV		ERx("vfboxmov.hlp")
# define	VFH_FRMATR		ERx("vffrmatr.hlp")
# define	VFH_FRMMOV		ERx("vffrmmov.hlp")
# define	VFH_VISADJ		ERx("vfvisadj.hlp")

# define	VFH_TRMATTR		ERx("vftrmatr.hlp")
# define	VFH_EDTRIM		ERx("vfedtrim.hlp")

# define	VFH_DSTFRM		ERx("vfdstfrm.hlp")
# define	VFH_DSTQBF		ERx("vfdstqbf.hlp")
# define	VFH_SVEND		ERx("vfsavend.hlp")
# define	RFH_SVEND		ERx("rfsavend.hlp")
# define	VFH_SVQUIT		ERx("vfsvquit.hlp")
# define	RFH_SVQUIT		ERx("rfsvquit.hlp")

# define	VFH_GRPMV		ERx("vfgrpmv.hlp")

# define	VFH_RUCTRL		ERx("vfructrl.hlp")

# define	VFH_SEMOV		ERx("vfsemov.hlp")

# define	VFH_QTBLNM		ERx("vfqtblnm.hlp")

# define	VFH_QJDFNM		ERx("vfqjdfnm.hlp")

# define	VFH_QJDFLS		ERx("vfqjdfls.hlp")
