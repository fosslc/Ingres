C
C	Copyright (c) 2004 Ingres Corporation
C	eqdef.for
C		-- External INGRES integer functions for VMS FORTRAN
C
	external	IInexec, IInextget, IItupget, IIerrtest, IIcsRetrieve
	external	IITBcaClmAct, IIactcomm, IIFRafActFld, IInmuact
	external	IIactscrl, IIchkfrm, IIdispfrm, IIendmu
	external	IInestmu, IIrunnest
	external	IIfnames, IIfsetio, IIinitmu, IIinqset, IIretval
	external	IIFRtoact, IIFRitIsTimeout, IIFRaeAlerterEvent
	external	IIrunform, IIvalfld, IInfrskact, IIiqset
	external	IItbact, IItbinit, IItbsetio, IItdata, IItdelrow
	external	IItinsert, IItscroll, IItunload, IItvalval
	external	IIsd, IIsd_no
	external	IIxintrans, IIxouttrans
	external	IILQprsProcStatus
	external	IIcsRetScroll

	integer*4	IInexec, IInextget, IItupget, IIerrtest, IIcsRetrieve
	integer*4	IITBcaClmAct, IIactcomm, IIFRafActFld, IInmuact
	integer*4	IIactscrl, IIchkfrm, IIdispfrm, IIendmu
	integer*4	IInestmu, IIrunnest
	integer*4	IIfnames, IIfsetio, IIinitmu, IIinqset, IIretval
	integer*4	IIFRtoact, IIFRitIsTimeout, IIFRaeAlerterEvent
	integer*4	IIrunform, IIvalfld, IInfrskact, IIiqset
	integer*4	IItbact, IItbinit, IItbsetio, IItdata, IItdelrow
	integer*4	IItinsert, IItscroll, IItunload, IItvalval
	integer*4	IIsd, IIsd_no
	integer*4	IIxintrans, IIxouttrans
	integer*4	IILQprsProcStatus
	integer*4	IIcsRetScroll

C	-- Utility calls for user
	external	IIdesc, IImdesc, IItuples
	integer*4	IIdesc, IImdesc, IItuples

C	-- Old style calls (pre 6.4)
	external	IIactclm, IIactfld
	integer*4	IIactclm, IIactfld
