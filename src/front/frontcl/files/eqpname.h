/*
** Copyright (c) 2006 Ingres Corporation
*/

#ifndef EQPNAME_H_INCLUDED
#define EQPNAME_H_INCLUDED

/*
** Defines INGRES Prototyped Runtime Calls used by ESQL/C and ESQL/C++
**
** 	** Calls are listed in alphabetical order **
*/

#define	IIbreak			IIpbreak
#define	IIcsClose		IIpcsClose
#define	IIcsDaGet		IIpcsDaGet
#define	IIcsDelete		IIpcsDelete
#define	IIcsERetrieve		IIpcsERetrieve
#define	IIcsERplace		IIpcsERplace
#define	IIcsGetio		IIpcsGetio
#define	IIcsOpen		IIpcsOpen
#define	IIcsParGet		IIpcsParGet
#define	IIcsQuery		IIpcsQuery
#define	IIcsRdO			IIpcsRdO
#define	IIcsReplace		IIpcsReplace
#define	IIcsRetrieve		IIpcsRetrieve
#define IIcsRetScroll		IIpcsRetScroll
#define	IIeqiqio		IIpeqiqio
#define	IIeqstio		IIpeqstio
#define	IIerrtest		IIperrtest
#define	IIexDefine		IIpexDefine
#define	IIexExec		IIpexExec
#define	IIexit			IIpexit
#define	IIflush			IIpflush
#define	IIgetdomio		IIpgetdomio
#define	IIingopen		IIpingopen
#define	IILQcnConName		IIpLQcnConName
#define IILQdbl			IIpLQdbl
#define	IILQesEvStat		IIpLQesEvStat
#define IILQint			IIpLQint
#define	IILQisInqSqlio		IIpLQisInqSqlio
#define	IILQldh_LoDataHandler	IIpLQldh_LoDataHandler
#define	IILQled_LoEndData	IIpLQled_LoEndData
#define	IILQlgd_LoGetData	IIpLQlgd_LoGetData
#define	IILQlpd_LoPutData	IIpLQlpd_LoPutData
#define	IILQpriProcInit		IIpLQpriProcInit
#define	IILQprsProcStatus	IIpLQprsProcStatus
#define	IILQprvProcValio	IIpLQprvProcValio
#define	IILQshSetHandler	IIpLQshSetHandler
#define	IILQsidSessID		IIpLQsidSessID
#define	IILQssSetSqlio		IIpLQssSetSqlio
#define	IInexec			IIpnexec
#define	IInextget		IIpnextget
#define	IIparret		IIpparret
#define	IIparset		IIpparset
#define	IIputctrl		IIpputctrl
#define	IIputdomio		IIpputdomio
#define	IIretinit		IIpretinit
#define	IIseterr		IIpseterr
#define	IIsexec			IIpsexec
#define	IIsqConnect		IIpsqConnect
#define	IIsqDaIn		IIpsqDaIn
#define IIsqDescInput		IIpsqDescInput
#define IIsqDescribe		IIpsqDescribe
#define	IIsqDisconnect		IIpsqDisconnect
#define	IIsqExImmed		IIpsqExImmed
#define	IIsqExStmt		IIpsqExStmt
#define	IIsqFlush		IIpsqFlush
#define	IIsqGdInit		IIpsqGdInit
#define	IIsqInit		IIpsqInit
#define	IIsqMods		IIpsqMods
#define	IIsqParms		IIpsqParms
#define	IIsqPrepare		IIpsqPrepare
#define	IIsqPrint		IIpsqPrint
#define	IIsqStop		IIpsqStop
#define	IIsqTPC			IIpsqTPC
#define	IIsqUser		IIpsqUser
#define	IIsqlcdInit		IIpsqlcdInit
#define	IIsyncup		IIpsyncup
#define	IItupget		IIptupget
#define	IIutsys			IIputsys
#define	IIvarstrio		IIpvarstrio
#define	IIwritio		IIpwritio
#define	IIxact			IIpxact

/* Forms Runtime System Calls */

#define IIactcomm		IIpactcomm
#define IIactscrl		IIpactscrl
#define IIaddform		IIpaddform
#define IIchkfrm		IIpchkfrm
#define IIclrflds		IIpclrflds
#define IIclrscr		IIpclrscr
#define IIdispfrm		IIpdispfrm
#define IIendforms		IIpendforms
#define IIendfrm		IIpendfrm
#define IIendmu			IIpendmu
#define IIendnest		IIpendnest
#define IIfldclear		IIpfldclear
#define IIfnames		IIpfnames
#define IIforminit		IIpforminit
#define IIforms			IIpforms
#define IIFRaeAlerterEvent	IIpFRaeAlerterEvent
#define IIFRafActFld		IIpFRafActFld
#define IIFRgotofld		IIpFRgotofld
#define IIFRgpcontrol		IIpFRgpcontrol
#define IIFRgpsetio		IIpFRgpsetio
#define IIFRInternal		IIpFRInternal
#define IIFRitIsTimeout		IIpFRitIsTimeout
#define IIFRreResEntry		IIpFRreResEntry
#define IIFRsaSetAttrio		IIpFRsaSetAttrio
#define IIfrshelp		IIpfrshelp
#define IIFRsqDescribe		IIpFRsqDescribe
#define IIFRsqExecute		IIpFRsqExecute
#define IIFRtoact		IIpFRtoact
#define IIfsetio		IIpfsetio
#define IIfsinqio		IIpfsinqio
#define IIfssetio		IIpfssetio
#define IIgetfldio		IIpgetfldio
#define IIgetoper		IIpgetoper
#define IIgtqryio		IIpgtqryio
#define IIhelpfile		IIphelpfile
#define IIinitmu		IIpinitmu
#define IIinqset		IIpinqset
#define IIiqfsio		IIpiqfsio
#define IIiqset			IIpiqset
#define IImessage		IIpmessage
#define IImuonly		IIpmuonly
#define IInestmu		IIpnestmu
#define IInfrskact		IIpnfrskact
#define IInmuact		IIpnmuact
#define IIprmptio		IIpprmptio
#define IIprnscr		IIpprnscr
#define IIputfldio		IIpputfldio
#define IIputoper		IIpputoper
#define IIredisp		IIpredisp
#define IIrescol		IIprescol
#define IIresfld		IIpresfld
#define IIresmu			IIpresmu
#define IIresnext		IIpresnext
#define IIretval		IIpretval
#define IIrf_param		IIprf_param
#define IIrunform		IIprunform
#define IIrunmu			IIprunmu
#define IIrunnest		IIprunnest
#define IIset_random		IIpset_random
#define IIsf_param		IIpsf_param
#define IIsleep			IIpsleep
#define IIstfsio		IIpstfsio
#define IItbact			IIptbact
#define IITBcaClmAct		IIpTBcaClmAct
#define IITBceColEnd		IIpTBceColEnd
#define IItbinit		IIptbinit
#define IItbsetio		IIptbsetio
#define IItbsmode		IIptbsmode
#define IItclrcol		IIptclrcol
#define IItclrrow		IIptclrrow
#define IItcogetio		IIptcogetio
#define IItcolval		IIptcolval
#define IItcoputio		IIptcoputio
#define IItdata			IIptdata
#define IItdatend		IIptdatend
#define IItdelrow		IIptdelrow
#define IItfill			IIptfill
#define IIthidecol		IIpthidecol
#define IItinsert		IIptinsert
#define IItrc_param		IIptrc_param
#define IItscroll		IIptscroll
#define IItsc_param		IIptsc_param
#define IItunend		IIptunend
#define IItunload		IIptunload
#define IItvalrow		IIptvalrow
#define IItvalval		IIptvalval
#define IIvalfld		IIpvalfld
 



/* 4GL Runtime System Calls */

#define IIG4acArrayClear        IIG4pacArrayClear
#define IIG4bpByrefParam        IIG4pbpByrefParam
#define IIG4ccCallComp          IIG4pccCallComp
#define IIG4chkobj              IIG4pchkobj
#define IIG4drDelRow            IIG4pdrDelRow
#define IIG4fdFillDscr          IIG4pfdFillDscr
#define IIG4gaGetAttr           IIG4pgaGetAttr
#define IIG4ggGetGlobal         IIG4pggGetGlobal
#define IIG4grGetRow            IIG4pgrGetRow
#define IIG4i4Inq4GL            IIG4pi4Inq4GL
#define IIG4icInitCall          IIG4picInitCall
#define IIG4irInsRow            IIG4pirInsRow
#define IIG4rrRemRow            IIG4prrRemRow
#define IIG4rvRetVal            IIG4prvRetVal
#define IIG4s4Set4GL            IIG4ps4Set4GL
#define IIG4saSetAttr           IIG4psaSetAttr
#define IIG4seSendEvent         IIG4pseSendEvent
#define IIG4sgSetGlobal         IIG4psgSetGlobal
#define IIG4srSetRow            IIG4psrSetRow
#define IIG4udUseDscr           IIG4pudUseDscr
#define IIG4vpValParam          IIG4pvpValParam


#endif /* EQPNAME_H_INCLUDED */
