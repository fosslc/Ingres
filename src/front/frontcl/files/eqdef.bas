	!
	! Filename:	EQDEF.BAS - BASIC definition file for EQUEL 
	!		runtime II routines.
	!
	! History:
	!	10-mar-86 - Written (bjb)
	!	23-may-88 - Added 6.1 IIFRgpcontrol, IIFRgpsetio.  (marge)
	!	15-may-89 - Added 6.4 IIFRreResEntry, IIFRafActFld, and
	!		    IITBcaClmAct. (teresa)
	!	10-aug-89 - Added 6.4 IITBceColEnd. (teresa)
	!	20-dec-89 - Added 6.4 IILQesEvStat and IILQegEvGetio. (barbara)
	!	22-apr-91 - Add 6.4 IIFRaeAlerterEvent and remove 
	!		    IILQegEvGetio. (teresal)
	!	14-oct-92 - Added IILQled_LoEndData. (lan)
	!	08/19/93 (dkh) - Added entry IIFRgotofld to support resume
	!			 nextfield/previousfield.
        !	1-nov-93 (donc) - Removed extraneous comma after IIFRgotofld
	!			  Bug 56276
	!	19-apr-1999 (hanch04)
	!	    Replace long with int
	!	12-feb-2003 (abbjo03)
	!	    Undo 19-apr-1999 change since int is not a BASIC datatype.
	!       09-apr-2007 (drivi01)
	!  	    BUG 117501
	!	    Adding references to function IIcsRetScroll in order to fix
	!	    build on windows.
	!
	! Copyright (c) 2004 Ingres Corporation
	!

	!
	! Variable IIf8 for standardization.
	! IIseterr for error handling.
	! IIutilities for string descriptor information.
	!
	declare real double IIf8
	external long function IIseterr
	external long function IIsd

	!
	! EQUEL (Libq) subroutines and functions
	!
	external sub IIflush, IIxnotrimio, IIgetdomio, IIxgetdomio, IIretinit
	external sub IIputdomio, IIxputdomio, IIsexec 
	external sub IIsyncup, IIxwritio, IIxact, IIxvarstrio
	external sub IIbreak, IIeqiqio, IIxeqiqio, IIeqstio, IIxeqstio
	external sub IIexit, IIingopen, IIutsys, IIexExec, IIexDefine
	external sub IILQled_LoEndData
	external long function IInexec, IInextget, IItupget, IItuples
	external long function IIerrtest

	!
	! Event support
	!
	external sub IILQesEvStat

	!
	! Stored-procedure support
	!
	external sub IILQpriProcInit, IILQprvProcValio, IIxLQprvProcValio
	external sub IIputctrl
	external long function IILQprsProcStatus

	!
	! EQUEL (Cursor) subroutines and functions
	!
	external sub IIcsOpen, IIcsQuery, IIcsGetio, IIxcsGetio, IIcsClose
	external sub IIcsDelete, IIcsReplace, IIcsERetrieve, IIcsERplace
	external sub IIcsRdO, IIcsRetScroll
	external long function IIcsRetrieve

	!
	! Forms (Runtime) subroutines and functions
	!
	external sub IIaddform, IIforms, IIgetoper, IIputoper, IIrunmu
	external sub IIsleep, IIclrflds, IIclrscr, IIendforms, IIendfrm
	external sub IImuonly, IIredisp, IIresnext, IIresmu, IIxprmptio
	external sub IIfldclear, IIforminit, IImessage, IIresfld, IIFRgotofld
	external sub IIprnscr, IIgetfldio, IIxgetfldio, IIfsinqio, IIxfsinqio
	external sub IIputfldio, IIxputfldio, IIfssetio, IIxfssetio 
	external sub IIhelpfile, IIrescol, IIfrshelp, IIiqfsio, IIxiqfsio 
	external sub IIstfsio, IIxstfsio, IIendnest
	external sub IIFRsaSetAttrio, IIxFRsaSetAttrio
	external long function IITBcaClmAct, IIactcomm, IIFRafActFld, IInmuact
	external long function IIactscrl, IInfrskact, IIFRtoact
	external long function IIchkfrm, IIFRitIsTimeout, IIFRaeAlerterEvent
	external long function IIendmu, IIfnames, IIinitmu, IIretval 
	external long function IIrunform, IIdispfrm, IIfsetio, IIvalfld 
	external long function IIiqset, IInestmu, IIrunnest
	external sub IIFRgpcontrol, IIFRgpsetio, IIxFRgpsetio
	external sub IIFRsqDescribe, IIFRsqExecute, IIFRreResEntry
	! Old style (pre 6.4)
	external long function IIactclm, IIactfld

	!
	! Tablefield (Tbacc) procedures and functions
	!
	external sub IItbsmode, IItclrcol, IItcolval, IItcogetio
	external sub IIxtcogetio, IItcoputio, IIxtcoputio, IItclrrow, IItdatend
	external sub IItfill, IItunend, IItvalrow, IIthidecol, IITBceColEnd
	external long function IItbact, IItbinit, IItbsetio, IItdata 
	external long function IItinsert, IItunload, IItscroll 
	external long function IItdelrow, IItvalval
