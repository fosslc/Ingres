{* 
** Filename: 	EQDEF.PAS
** Purpose:	PASCAL definiton file for EQUEL run-time II routine interface.
**
** History:	31-dec-1985	- Written.
**		15-may-1989	- Added 6.4 IIFRreResEntry, IITBcaClmAct,
**				  IIFRafActFld calls.
**		10-aug-1989	- Added 6.4 IITBceColEnd.
**		20-dec-1989	- Added 6.4 IILQesEvStat and IILQegEvGetio
**		08-aug-1990	- New call for setting attributes. (bjb)
**		15-feb-1991	- Add float variable (teresal)
**		01-mar-1991	- Add: IILQssSetSqlio - IILQisInqSqlio and
**				       IILQshSetHandler.   (kathryn)
**		21-apr-1991	- Add activate event all - remove
**				  IILQegEvGetio. (teresal)
**		14-oct-1992	- Added IILQled_LoEndData. (lan)
**		15-jan-1993	- Added IILQlpd_LoPutData and IILQlgd_LoGetData
**				  (kathryn)
**		18-jan-1993	- Added IILQldh_LoDataHandler. (lan)
**		08-mar-1993	- Changed variable name from 'type' to
**				  'typ' in IILQldh_LoDataHandler and
**				  IIxLQldh_LoDataHandler since 'type' is
**				  a pascal reserved word. (lan)
**		26-jul-1993	- Added entries (IIG4...) for EXEC 4GL (lan).
**	08/19/93 (dkh) - Added entry IIFRgotofld to support RESUME
**			 NEXTFIELD/PREVIOUSFIELD.
**	    	09-apr-2007 (drivi01)
**			BUG 117501
**			Adding references to function IIcsRetScroll in order to fix
**			build on windows.
**	    	07-jan-2009 (upake01)
**			BUG 121487
**			Change IIcsRetScroll from "procedure" to "function" returning
**			an "integer".
**
** Copyright (c) 2004 Ingres Corporation
*}

{*
** Utility Routines
*}

{ Indicator variable type }
type
	indicator = [word] -32768..32767;

{ Float variable used by preprocessor for temporary storage }
var
	IIf8:	double;

{ String descriptor converters }
function	IIsd( %immed str: [unsafe] integer ): integer; extern;
function	IIsd_no( %immed str: [unsafe] integer ): integer; extern;
function	IIdesc( %immed str: [unsafe] integer ): integer; extern;

{ Error trap routine }
procedure	IIseterr( %immed efunc: [unsafe] integer ); extern;

{ Backward compatible hardcoded cII/II calls }
procedure	cIIseterr( %immed efunc: [unsafe] integer ); extern;
function	cIItuples: integer; extern;
function	IItuples: integer; extern;

{*
** QUEL (Libq) database procedures and functions 
*}

{ DB connect/disconnect }
procedure	IIexit; extern;
procedure	IIingopen( %immed lang: integer;
			   %immed a1: [unsafe] integer := %immed 0;
			   %immed a2: [unsafe] integer := %immed 0;
			   %immed a3: [unsafe] integer := %immed 0;
			   %immed a4: [unsafe] integer := %immed 0;
			   %immed a5: [unsafe] integer := %immed 0;
			   %immed a6: [unsafe] integer := %immed 0;
			   %immed a7: [unsafe] integer := %immed 0;
			   %immed a8: [unsafe] integer := %immed 0;
			   %immed a9: [unsafe] integer := %immed 0;
			   %immed a10: [unsafe] integer := %immed 0;
			   %immed a11: [unsafe] integer := %immed 0;
			   %immed a12: [unsafe] integer := %immed 0;
			   %immed a13: [unsafe] integer := %immed 0 ); extern;

{ DB query }
procedure	IIsyncup( %immed fname: [unsafe] integer;
			  %immed line: integer ); extern;
procedure	IIwritio( %immed trim: integer;
			  %immed ind: [unsafe] integer;
			  %immed isvar: integer;
			  %immed typ: integer;
			  %immed len: integer;
			  %immed qry: [unsafe] integer ); extern;
procedure	IIxwritio( %immed trim: integer;
			   %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %immed strdesc: [unsafe] integer ); extern;

{ Retrieve data }
procedure	IIflush( %immed fname: [unsafe] integer;
			 %immed line: integer ); extern;
function	IInextget: integer; extern;
procedure	IIretinit( %immed fname: [unsafe] integer;
			   %immed line: integer ); extern;

{ Repeat Utils }
function	IInexec: integer; extern;
procedure	IIsexec; extern;
procedure	IIexExec( %immed exec: integer;
			  %immed qrynam: [unsafe] integer;
			  %immed qryid1: integer;
			  %immed qryid2: integer ); extern;
procedure	IIexDefine( %immed exec: integer;
			    %immed qrynam: [unsafe] integer;
			    %immed qryid1: integer;
			    %immed qryid2: integer ); extern;

{ Misc }
procedure	IIbreak; extern;
procedure	IILQled_LoEndData; extern;
procedure	IIxact( %immed xact: integer ); extern;

{ Stored procedure support }
procedure	IILQpriProcInit( %immed typ: integer;
				 %immed name: [unsafe] integer ); extern;
procedure	IILQprvProcValio( %immed name: [unsafe] integer;
			          %immed pbyref: integer;
				  %immed ind: [unsafe] integer;
			          %immed isvar: integer;
			          %immed typ: integer;
			          %immed len: integer;
			          %immed data: [unsafe] integer ); extern;
procedure	IIxLQprvProcValio( %immed name: [unsafe] integer;
			           %immed pbyref: integer;
				   %immed ind: [unsafe] integer;
			           %immed isvar: integer;
			           %immed typ: integer;
			           %immed len: integer;
			           %immed strdesc: [unsafe] integer ); extern;
function	IILQprsProcStatus: integer; extern;
procedure	IIputctrl( %immed code: integer ); extern;

{ DB I/O }
procedure	IIputdomio( %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %immed data: [unsafe] integer ); extern;
procedure	IIxputdomio( %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer ); extern;
procedure	IIgetdomio( %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %ref addr: [unsafe] integer ); extern;
procedure	IIxgetdomio( %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer ); extern;
procedure	IIvarstrio( %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %immed data: [unsafe] integer ); extern;
procedure	IIxvarstrio( %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer ); extern;
procedure	IIxnotrimio( %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer ); extern;
procedure	IILQlpd_LoPutData( %immed isvar: integer;
				   %immed typ: integer;
				   %immed len: integer;
				   %immed addr: [unsafe] integer;
				   %immed seglen: integer;
				   %immed datend: integer ); extern;
procedure	IIxLQlpd_LoPutData(%immed isvar: integer;
				   %immed typ: integer;
				   %immed len: integer;
				   %immed strdesc: [unsafe] integer;
				   %immed seglen: integer;
				   %immed datend: integer ); extern;
procedure	IILQlgd_LoGetData( %immed isvar: integer;
				   %immed typ: integer;
				   %immed len: integer;
				   %ref addr: [unsafe] integer;
				   %immed max: integer;
				   %ref seglen: integer;
				   %ref datend: integer ); extern;
procedure	IIxLQlgd_LoGetData(%immed isvar: integer;
				   %immed typ: integer;
				   %immed len: integer;
				   %immed strdesc: [unsafe] integer;
				   %immed max: integer;
				   %ref seglen: integer;
				   %ref datend: integer ); extern;
procedure	IILQldh_LoDataHandler( %immed typ: integer;
				   %immed indvar: [unsafe] integer;
				   %immed datahdlr: [unsafe] integer;
				   %ref hdlr_arg: [unsafe] integer ); extern;
procedure	IIxLQldh_LoDataHandler( %immed typ: integer;
				   %immed indvar: [unsafe] integer;
				   %immed datahdlr: [unsafe] integer;
				   %ref hdlr_arg: [unsafe] integer ); extern;

{ Cursors Control }

procedure	IIcsOpen( %immed csrnam: [unsafe] integer;
			  %immed csrid1: integer;
			  %immed csrid2: integer ); extern;
procedure	IIcsQuery( %immed csrnam: [unsafe] integer;
			   %immed csrid1: integer;
			   %immed csrid2: integer ); extern;
procedure	IIcsRdO( %immed rdo: integer;
			 %immed str: [unsafe] integer ); extern;
function	IIcsRetrieve( %immed csrnam: [unsafe] integer;
			      %immed csrid1: integer;
			      %immed csrid2: integer ): integer; extern;
procedure	IIcsERetrieve; extern;
function	IIcsRetScroll( %immed csrnam: [unsafe] integer;
			       %immed csrid1: integer;
			       %immed csrid2: integer;
			       %immed fetcho: integer;
			       %immed fetchn: integer ): integer; extern;
procedure	IIcsClose( %immed csrnam: [unsafe] integer;
			   %immed csrid1: integer;
			   %immed csrid2: integer ); extern;
procedure	IIcsDelete( %immed tabnam: [unsafe] integer;
			    %immed csrnam: [unsafe] integer;
			    %immed csrid1: integer;
			    %immed csrid2: integer ); extern;
function	IIcsReplace( %immed csrnam: [unsafe] integer;
			     %immed csrid1: integer;
			     %immed csrid2: integer ): integer; extern;
function	IIcsERplace( %immed csrnam: [unsafe] integer;
			     %immed csrid1: integer;
			     %immed csrid2: integer ): integer; extern;

{ Cursors I/O }

procedure	IIcsGetio( %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %ref addr: [unsafe] integer ); extern;
procedure	IIxcsGetio( %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %immed strdesc: [unsafe] integer ); extern;

{ Events }

procedure	IILQesEvStat( %immed flag: integer;
			      %immed waitsecs: integer ); extern;

{ 
  PARAM calls not implemented - Procedure template is:

  function	IIxintrans: integer; extern;
  function	IIxouttrans: integer; extern;
  procedure	IIparret( %immed tlist: integer;
			  %immed array: [unsafe] integer;
			  %immed transfunc: [unsafe] integer ); extern;
  procedure	IIparset( %immed tlist: integer;
			  %immed array: [unsafe] integer;
			  %immed transfunc: [unsafe] integer ); extern;
  function	IItupget: integer; extern;
}

{* 
** EQUEL (Libq) non-database procedures and functions 
*}

{ Calling subsystems }
procedure	IIutsys( %immed flag: integer;
			 %immed name: [unsafe] integer;
			 %immed strval: [unsafe] integer ); extern;

{ Status info I/O }

procedure	IIeqstio( %immed obj: [unsafe] integer;
			  %immed ind: [unsafe] integer;
			  %immed isvar: integer;
			  %immed typ: integer;
			  %immed len: integer;
			  %immed data: [unsafe] integer ); extern;
procedure	IIxeqstio( %immed obj: [unsafe] integer;
			   %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %immed strdesc: [unsafe] integer ); extern;
procedure	IIeqiqio( %immed ind: [unsafe] integer;
			  %immed isvar: integer;
			  %immed typ: integer;
			  %immed len: integer;
			  %ref addr: [unsafe] integer;
			  %immed obj: [unsafe] integer ); extern;
procedure	IIxeqiqio( %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %immed strdesc: [unsafe] integer;
			   %immed obj: [unsafe] integer ); extern;
procedure	IILQssSetSqlio( %immed obj: [unsafe] integer;
			        %immed ind: [unsafe] integer;
			        %immed isvar: integer;
			        %immed typ: integer;
			        %immed len: integer;
			        %immed data: [unsafe] integer ); extern;
procedure	IIxLQssSetSqlio( %immed obj: [unsafe] integer;
			         %immed ind: [unsafe] integer;
			         %immed isvar: integer;
			         %immed typ: integer;
			         %immed len: integer;
			         %immed strdesc: [unsafe] integer ); extern;
procedure	IILQisInqSqlio( %immed ind: [unsafe] integer;
			        %immed isvar: integer;
			        %immed typ: integer;
			        %immed len: integer;
			        %ref addr: [unsafe] integer;
			        %immed obj: [unsafe] integer ); extern;
procedure	IIxLQisInqSqlio( %immed ind: [unsafe] integer;
			         %immed isvar: integer;
			         %immed typ: integer;
			         %immed len: integer;
			         %immed strdesc: [unsafe] integer;
			   	 %immed obj: [unsafe] integer ); extern;
{ exec 4gl }
procedure	IIG4acArrayClear( %immed ary: integer ); extern;
procedure	IIG4rrRemRow( %immed ary: integer;
				%immed indx: integer ); extern;
procedure	IIG4irInsRow( %immed ary: integer;
				%immed indx: integer;
				%immed row: integer;
				%immed state: integer;
				%immed which: integer ); extern;
procedure	IIG4drDelRow( %immed ary: integer;
				%immed indx: integer ); extern;
procedure	IIG4srSetRow( %immed ary: integer;
				%immed indx: integer;
				%immed row: integer ); extern;
procedure	IIG4grGetRow( %immed rowind: [unsafe] integer;
			        %immed isvar: integer;
			        %immed rowtyp: integer;
			        %immed rowlen: integer;
			        %ref rowptr: [unsafe] integer;
				%immed ary: integer;
			        %immed indx: integer ); extern;
procedure	IIxG4grGetRow( %immed rowind: [unsafe] integer;
				%immed isvar: integer;
				%immed rowtyp: integer;
				%immed rowlen: integer;
				%immed strdesc: [unsafe] integer;
				%immed ary: integer;
			        %immed indx: integer ); extern;
procedure	IIG4ggGetGlobal( %immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer;
				%immed name: [unsafe] integer;
				%immed gtype: integer ); extern;
procedure	IIxG4ggGetGlobal( %immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer;
				%immed name: [unsafe] integer;
				%immed gtype: integer ); extern;
procedure	IIG4sgSetGlobal( %immed name: [unsafe] integer;
				%immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer ); extern;
procedure	IIxG4sgSetGlobal( %immed name: [unsafe] integer;
				%immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer ); extern;
procedure	IIG4gaGetAttr( %immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer;
				%immed name: [unsafe] integer ); extern;
procedure	IIxG4gaGetAttr( %immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer;
				%immed name: [unsafe] integer ); extern;
procedure	IIG4saSetAttr( %immed name: [unsafe] integer;
				%immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer ); extern;
procedure	IIxG4saSetAttr( %immed name: [unsafe] integer;
				%immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer ); extern;
procedure	IIG4chkobj( %immed obj: integer;
				%immed acs: integer;
				%immed indx: integer;
				%immed caller: integer ); extern;
procedure	IIG4udUseDscr( %immed lang: integer;
				%immed direction: integer;
				%immed sqlda: [unsafe] integer ); extern;
procedure	IIG4fdFillDscr( %immed obj: integer;
				%immed lang: integer;
				%immed sqlda: [unsafe] integer ); extern;
procedure	IIG4icInitCall( %immed name: [unsafe] integer;
				%immed typ: integer ); extern;
procedure	IIG4vpValParam( %immed name: [unsafe] integer;
				%immed ind: [unsafe] integer;
				%immed isval: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer ); extern;
procedure	IIxG4vpValParam( %immed name: [unsafe] integer;
				%immed ind: [unsafe] integer;
				%immed isval: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer ); extern;
procedure	IIG4bpByrefParam( %immed ind: [unsafe] integer;
				%immed isval: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer;
				%immed name: [unsafe] integer ); extern;
procedure	IIxG4bpByrefParam( %immed ind: [unsafe] integer;
				%immed isval: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer;
				%immed name: [unsafe] integer ); extern;
procedure	IIG4rvRetVal( %immed ind: [unsafe] integer;
				%immed isval: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer ); extern;
procedure	IIxG4rvRetVal( %immed ind: [unsafe] integer;
				%immed isval: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer ); extern;
procedure	IIG4ccCallComp; extern;
procedure	IIG4i4Inq4GL( %immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer;
				%immed obj: integer;
				%immed code: integer ); extern;
procedure	IIxG4i4Inq4GL( %immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer;
				%immed obj: integer;
				%immed code: integer ); extern;
procedure	IIG4s4Set4GL( %immed obj: integer;
				%immed code: integer;
				%immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%ref data: [unsafe] integer ); extern;
procedure	IIxG4s4Set4GL( %immed obj: integer;
				%immed code: integer;
				%immed ind: [unsafe] integer;
				%immed isvar: integer;
				%immed typ: integer;
				%immed len: integer;
				%immed strdesc: [unsafe] integer ); extern;
procedure	IIG4seSendEvent( %immed frame: integer ); extern;
{ Misc }
function	IIerrtest: integer; extern;
procedure       IILQshSetHandler( %immed hdlr: integer;
			          %immed funcptr: integer ); extern;


{* 
** Forms (Runtime) procedures and functions 
*}

{ Forms setup }
procedure	IIaddform( %immed formid: [unsafe] integer ); extern;
procedure	IIendforms; extern;
procedure	IIforminit( %immed fld: [unsafe] integer ); extern;
procedure	IIforms( %immed lang: integer ); extern;

{ Menus }
function	IIendmu: integer; extern;
function	IIinitmu: integer; extern;
procedure	IImuonly; extern;
procedure	IIrunmu( %immed onoff: integer ); extern;

{ Misc Utils (Clear, Sleep, Message, Prompt, Help) }
procedure	IIclrflds; extern;
procedure	IIclrscr; extern;
procedure	IIfldclear( %immed fld: [unsafe] integer ); extern;
procedure	IIfrshelp( %immed flag: integer;
			   %immed subj: [unsafe] integer;
			   %immed filenm: [unsafe] integer ); extern;
procedure	IIhelpfile( %immed subj: [unsafe] integer;
			    %immed filenm: [unsafe] integer ); extern;
procedure	IImessage( %immed fld: [unsafe] integer ); extern;
procedure	IIprnscr( %immed fld: [unsafe] integer ); extern;
procedure	IIsleep( %immed secs: integer ); extern;
procedure	IIxprmptio( %immed echo: integer;
			    %immed prmpt: [unsafe] integer;
			    %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %immed strdesc: [unsafe] integer ); extern;

{ Validation }
function	IIchkfrm: integer; extern;
function	IIFRitIsTimeout: integer; extern;
function	IIvalfld( %immed fld: [unsafe] integer ): integer; extern;

{ Display Loop }
function	IIdispfrm( %immed form: [unsafe] integer;
			   %immed mode: [unsafe] integer ): integer; extern;
procedure	IIendfrm; extern;
function	IIfnames: integer; extern;
procedure	IIredisp; extern;
procedure	IIrescol( %immed tab: [unsafe] integer;
			  %immed col: [unsafe] integer ); extern;
procedure	IIresfld( %immed fld: [unsafe] integer ); extern;
procedure	IIFRgotofld( %immed dir: integer ); extern;
procedure	IIresmu; extern;
procedure	IIresnext; extern;
function	IIretval: integer; extern;
procedure	IIFRreResEntry; extern;
function	IIrunform: integer; extern;

function	IInestmu: integer; extern;
function	IIrunnest: integer; extern;
procedure	IIendnest; extern;

{ Activate blocks }
{ Old style (pre 6.4) }
function	IIactclm( %immed tab: [unsafe] integer;
			  %immed col: [unsafe] integer;
			  %immed intrp: integer ): integer; extern;
{ New style (6.4 version) }
function	IITBcaClmAct( %immed tab: [unsafe] integer;
			      %immed col: [unsafe] integer;
			      %immed entact: integer;
			      %immed intrp: integer ): integer; extern;
function	IIactcomm( %immed command: integer;
			   %immed intrp: integer ): integer; extern;
{ Old style (pre 6.4) }
function	IIactfld( %immed fld: [unsafe] integer;
			  %immed intrp: integer ): integer; extern;
{ New style (6.4 version) }
function	IIFRafActFld( %immed fld: [unsafe] integer;
			      %immed entact: integer;
			      %immed intrp: integer ): integer; extern;
function	IInmuact( %immed menu: [unsafe] integer;
			   %immed expl: [unsafe] integer;
			   %immed valid: integer;
			   %immed activ: integer;
			   %immed intrp: integer ): integer; extern;
function	IIactscrl( %immed tab: [unsafe] integer;
			   %immed updown: integer;
			   %immed intrp: integer ): integer; extern;
function	IInfrskact( %immed keynum: integer;
			    %immed expl: [unsafe] integer;
			    %immed valid: integer;
			    %immed activ: integer;
			    %immed intrp: integer ): integer; extern;
function	IIFRtoact( %immed tval: integer;
			   %immed intrp: integer ): integer; extern;
function	IIFRaeAlerterEvent( %immed intrp: integer ): integer; extern;

{ Field I/O }
function	IIfsetio( %immed form: [unsafe] integer ): integer; extern;
procedure	IIgetoper( %immed putget: integer ); extern;
procedure	IIputoper( %immed putget: integer ); extern;

procedure	IIputfldio( %immed fld: [unsafe] integer;
			    %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %immed data: [unsafe] integer ); extern;
procedure	IIxputfldio( %immed fld: [unsafe] integer;
			     %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer ); extern;
procedure	IIgetfldio( %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %ref addr: [unsafe] integer;
			    %immed fld: [unsafe] integer ); extern;
procedure	IIxgetfldio( %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer;
			     %immed fld: [unsafe] integer ); extern;

procedure	IIgtqryio( %immed ind: [unsafe] integer;
			   %immed isvar: integer;
		           %immed typ: integer;
			   %immed len: integer;
			   %ref addr: [unsafe] integer;
			   %immed fld: [unsafe] integer ); extern;

{
  PARAM IIrf_param and IIsf_param not implemented.
  See QUEL (Libq) for declaration.
}

{ Inq/Set FRS }
{ Old style }
function	IIinqset( %immed objnm: [unsafe] integer;
			  %immed nm1: [unsafe] integer := %immed 0;
			  %immed nm2: [unsafe] integer := %immed 0;
			  %immed nm3: [unsafe] integer := %immed 0;
			  %immed nm4: [unsafe] integer := %immed 0
			  ): integer; extern;
procedure	IIfssetio( %immed obj: [unsafe] integer;
			   %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %immed data: [unsafe] integer ); extern;
procedure	IIxfssetio( %immed obj: [unsafe] integer;
			    %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %immed strdesc: [unsafe] integer ); extern;
procedure	IIfsinqio( %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %ref addr: [unsafe] integer;
			   %immed obj: [unsafe] integer ); extern;
procedure	IIxfsinqio( %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %immed strdesc: [unsafe] integer;
			    %immed obj: [unsafe] integer ); extern;
{ New style }
function	IIiqset( %immed objtype: integer;
			 %immed row: integer;
			 %immed nm1: [unsafe] integer := %immed 0;
			 %immed nm2: [unsafe] integer := %immed 0;
			 %immed nm3: [unsafe] integer := %immed 0
			  ): integer; extern;
procedure	IIstfsio( %immed attr: integer;
			  %immed name: [unsafe] integer;
			  %immed row: integer;
			  %immed ind: [unsafe] integer;
			  %immed isvar: integer;
			  %immed typ: integer;
			  %immed len: integer;
			  %immed data: [unsafe] integer ); extern;
procedure	IIxstfsio( %immed attr: integer;
			   %immed name: [unsafe] integer;
			   %immed row: integer;
			   %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %immed strdesc: [unsafe] integer ); extern;
procedure	IIiqfsio( %immed ind: [unsafe] integer;
			  %immed isvar: integer;
			  %immed typ: integer;
			  %immed len: integer;
			  %ref addr: [unsafe] integer;
			  %immed attr: integer;
			  %immed name: [unsafe] integer;
			  %immed row: integer ); extern;
procedure	IIxiqfsio( %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %immed strdesc: [unsafe] integer;
			   %immed attr: integer;
			   %immed name: [unsafe] integer;
			   %immed row: integer ); extern;

{ Insertrow/Loadtale with clause setting of attributes}
procedure	IIFRsaSetAttrio( %immed attr: integer;
			  %immed name: [unsafe] integer;
			  %immed ind: [unsafe] integer;
			  %immed isvar: integer;
			  %immed typ: integer;
			  %immed len: integer;
			  %immed data: [unsafe] integer ); extern;
procedure	IIxFRsaSetAttrio( %immed attr: integer;
			   %immed name: [unsafe] integer;
			   %immed ind: [unsafe] integer;
			   %immed isvar: integer;
			   %immed typ: integer;
			   %immed len: integer;
			   %immed strdesc: [unsafe] integer ); extern;

{* 
** Table field (Tbacc) procedures and functions 
*}

{ Inittable }
function	IItbinit( %immed form: [unsafe] integer;
			  %immed tab: [unsafe] integer;
			  %immed mode: [unsafe] integer ): integer; extern;
procedure	IItfill; extern;
procedure	IIthidecol( %immed col: [unsafe] integer;
			    %immed typ: [unsafe] integer ); extern;

{ Load/Unload }
function	IItbact( %immed form: [unsafe] integer;
			 %immed tab: [unsafe] integer;
			 %immed load: integer ): integer; extern;
procedure	IItunend; extern;
function	IItunload: integer; extern;

{ Statment Setup (Regular, Insert, Delete, Scroll) }
function	IItbsetio( %immed act: integer;
			   %immed form: [unsafe] integer;
			   %immed tab: [unsafe] integer;
			   %immed row: integer ): integer; extern;
procedure	IItbsmode( %immed scrlmd: [unsafe] integer ); extern;
function	IItdelrow( %immed tofill: integer ): integer; extern;
function	IItinsert: integer; extern;
function	IItscroll( %immed tofill: integer;
			   %immed recnum: integer ): integer; extern;

{ Misc Utils (Clear, Validation, TableData) }
procedure	IItclrcol( %immed col: [unsafe] integer ); extern;
procedure	IItclrrow; extern;
procedure	IItcolval( %immed col: [unsafe] integer ); extern;
function	IItdata: integer; extern;
procedure	IItdatend; extern;
procedure	IItvalrow; extern;
function	IItvalval( %immed check: integer ): integer; extern;
procedure	IITBceColEnd; extern;

{ Column I/O }
procedure	IItcoputio( %immed col: [unsafe] integer;
			    %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %immed data: [unsafe] integer ); extern;
procedure	IIxtcoputio( %immed col: [unsafe] integer;
			     %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer ); extern;
procedure	IItcogetio( %immed ind: [unsafe] integer;
			    %immed isvar: integer;
			    %immed typ: integer;
			    %immed len: integer;
			    %ref addr: [unsafe] integer;
			    %immed col: [unsafe] integer ); extern;
procedure	IIxtcogetio( %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer;
			     %immed col: [unsafe] integer ); extern;

{
  PARAM IItrc_param and IItsc_param not implemented.
  See QUEL (Libq) for declaration.
}

{ VENUS set popup routines }
procedure	IIFRgpcontrol( %immed state: integer;
			     %immed alt: integer ); extern;
procedure	IIFRgpsetio( %immed pid: integer;
			     %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed data: [unsafe] integer ); extern;
procedure	IIxFRgpsetio( %immed pid: integer;
			     %immed ind: [unsafe] integer;
			     %immed isvar: integer;
			     %immed typ: integer;
			     %immed len: integer;
			     %immed strdesc: [unsafe] integer ); extern;

{ Dynamic FRS routines }

procedure 	IIFRsqDescribe( %immed lang: Integer;
				%immed ftag: Integer;
				%immed fname: [unsafe] Integer;
				%immed tname: [unsafe] Integer;
				%immed mname: [unsafe] Integer;
				%immed sqd: [unsafe] Integer ); extern;

procedure 	IIFRsqExecute(  %immed lang: Integer;
				%immed ftag: Integer;
				%immed itag: Integer;
				%immed sqd: [unsafe] Integer ); extern;
{*
** End EQDEF.PAS
*}
