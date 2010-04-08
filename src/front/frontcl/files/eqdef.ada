-- {
-- File Name: 	EQDEF.ADA
-- Purpose:	ADA package specification file for EQUEL run-time II routine
--		interface.
--
-- Packages:	EQUEL 	    - Interface to non-forms routines.
-- 		EQUEL_FORMS - Interface to forms system.
-- Usage:
--		## with EQUEL;  	Brings in only EQUEL
--		## with EQUEL_FORMS;	Brings in both EQUEL and EQUEL_FORMS.
--
-- History:	19-Feb-1986	- Written.
--		23-Jul-1987	- Modified for 6.0.
--		10-Aug-1989	- Modified for 6.4.
--		28-Dec-1989	- Updated for 6.4.		
--		27-Jul-1990	- Updated for decimal.
--		09-Aug-1990	- New call for setting attributes (barbara)
--		01-mar-1991	- Updated for new inquire/set_sql including
--				  handlers.  (kathryn)
--		21-apr-1991	- Add activate event. (teresal)
--		14-oct-1992	- Added IILQled_LoEndData. (lan)
--		20-nov-1992     - Added IILQldh_LoDataHandler, 
--			          IILQlgd_LoGetData[A,S] and 
--				  IILQlpd_LoPutData[A,I,F,S] (kathryn).
--	        26-jul-1993	- Added entries (IIG4...) for EXEC 4GL (lan).
--		08/19/93 (dkh)  - Added entry IIFRgotofld to support RESUME
--			 	  nextfield/previousfield.
--	        10-sep-1993	- Fixed compile problem (lan).
--		06/02/03 (toumi01) - Fix dbproc function definition for Ingres 2.6
--				     row producing procedures feature.
--	    	09-apr-2007 (drivi01)
--			BUG 117501
--			Adding references to function IIcsRetScroll in order to fix
--			build on windows.
--	    	07-jan-2009 (upake01)
--			BUG 121487
--			Change IIcsRetScroll from "procedure" to "function" returning
--                      an "integer".  Add a line for call mechanism for the function.
--                      Change the first parameter from "Address" to "String".
--
-- Example Interface Format:
--		procedure IIname ( arg1 : Integer; arg2: String );
--		 pragma   interface( C, IIname );
--		 pragma   import_procedure( IIname, 
--				mechanism 	=> (value, reference) );
--  - only if overloaded:
--		 		external	=> "C name",
--
-- Notes:
--    PARAM calls are not yet implemented.
--
-- Copyright (c) 2004 Ingres Corporation
-- } 

--
-- Non-forms EQUEL package
--

with SYSTEM;

package	EQUEL is

use SYSTEM;

--
-- Utility Routines
--

-- Null terminate most strings passed to II calls.

II0: character renames ascii.nul;
IInull: address renames address_zero;

-- String descriptor converters

function  IIsd( str: String ) return Integer;
 pragma   interface( C, IIsd );
 pragma   import_function( IIsd,
	    mechanism => (descriptor(s)) );
function  IIsd_no( str: String ) return Integer;
 pragma   interface( C, IIsd_no );
 pragma   import_function( IIsd_no,
	    mechanism => (descriptor(s)) );

-- Error trap routine

procedure IIseterr( errfunc: Address );
 pragma   interface( C, IIseterr );
 pragma   import_procedure( IIseterr,
	    mechanism => (value) );

--
-- QUEL (LIBQ) database procedures and functions 
--

-- DB connect/disconnect 
procedure IIexit;
 pragma   interface( C, IIexit );
 pragma   import_procedure( IIexit );

procedure IIingopen( lang: Integer;
		     a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
		     a11, a12, a13: String := String'null_parameter );
 pragma   interface( C, IIingopen );
 pragma   import_procedure( IIingopen,
	    mechanism => (value,
			  reference, reference, reference, reference,
			  reference, reference, reference, reference,
			  reference, reference, reference, reference,
			  reference) );

-- DB query 
procedure IIsyncup( fname: String := String'null_parameter;
		     line: Integer := 0 );
 pragma   interface( C, IIsyncup );
 pragma   import_procedure( IIsyncup,
	    mechanism => (reference, value) );

procedure IIwritioS( trim: Integer; ind: Address; isvar, typ, len: Integer;
		str: String );
 pragma   interface( C, IIwritioS );
 pragma	  import_procedure( IIwritioS,
	    external => "IIxwritio",
	    mechanism => (value, value, value, value, value, descriptor(s)) );

-- RETRIEVE data 
procedure IIflush( fname: String := String'null_parameter;
		   line: Integer := 0 );
 pragma   interface( C, IIflush );
 pragma	  import_procedure( IIflush,
	    mechanism => (reference, value) );

function  IInextget return Integer;
 pragma   interface( C, IInextget );
 pragma   import_function( IInextget );

procedure IIretinit( fname: String := String'null_parameter;
		     line: Integer := 0 );
 pragma   interface( C, IIretinit );
 pragma	  import_procedure( IIretinit,
	    mechanism => (reference, value) );

-- REPEAT Utilities 
function  IInexec return Integer;
 pragma   interface( C, IInexec );
 pragma   import_function( IInexec );

procedure IIsexec;
 pragma   interface( C, IIsexec );
 pragma   import_procedure( IIsexec );

-- Miscellaneous 
procedure IIbreak;
 pragma   interface( C, IIbreak );
 pragma   import_procedure( IIbreak );

procedure IILQled_LoEndData;
 pragma   interface( C, IILQled_LoEndData );
 pragma   import_procedure( IILQled_LoEndData );

-- DB stored procedures 
procedure IILQpriProcInit( ptype: Integer; pname: String );
 pragma   interface( C, IILQpriProcInit );
 pragma   import_procedure( IILQpriProcInit,
	    mechanism => (value, reference) );

procedure IILQprvProcValioI( name: String; pbyref: Integer; ind: Address; 
		isvar, typ, len: Integer; ival: Integer );
 pragma   interface( C, IILQprvProcValioI );
 pragma   import_procedure( IILQprvProcValioI,
	    external  => "IILQprvProcValio",
	    mechanism => (reference,value,value,value,value,value,value) );
procedure IILQprvProcValioF( name: String; pbyref: Integer; ind: Address; 
		isvar, typ, len: Integer; fval: Long_Float );
 pragma   interface( C, IILQprvProcValioF );
 pragma   import_procedure( IILQprvProcValioF,
	    external  => "IILQprvProcValio",
	    mechanism => (reference,value,value,value,value,value,reference));
procedure IILQprvProcValioA( name: String; pbyref: Integer; ind: Address; 
		isvar, typ, len: Integer; addr: Address );
 pragma   interface( C, IILQprvProcValioA );
 pragma   import_procedure( IILQprvProcValioA,
	    external  => "IILQprvProcValio",
	    mechanism => (reference,value,value,value,value,value,value) );
procedure IILQprvProcValioD( name: String; pbyref: Integer; ind: Address; 
		isvar, typ, len: Integer; decstr: String );
 pragma   interface( C, IILQprvProcValioD );
 pragma   import_procedure( IILQprvProcValioD,
	    external  => "IILQprvProcValio",
	    mechanism => (reference,value,value,value,value,value,reference) );
procedure IILQprvProcValioS( name: String; pbyref: Integer; ind: Address; 
		isvar, typ, len: Integer; str: String );
 pragma   interface( C, IILQprvProcValioS );
 pragma   import_procedure( IILQprvProcValioS,
	external  => "IIxLQprvProcValio",
	mechanism => (reference,value,value,value,value,value,descriptor(s)));

function  IILQprsProcStatus( count: Integer ) return Integer;
 pragma   interface( C, IILQprsProcStatus );
 pragma   import_function( IILQprsProcStatus,
	mechanism => (value) );

procedure IIputctrl( code: Integer );
 pragma   interface( C, IIputctrl );
 pragma   import_procedure( IIputctrl,
	    mechanism => (value) );

-- DB Input/Output
-- IIputdomioI also used for IIsd (when blank-trimmed)
procedure IIputdomioI( ind: Address; isvar, typ, len: Integer; ival: Integer );
 pragma   interface( C, IIputdomioI );
 pragma   import_procedure( IIputdomioI,
	    external  => "IIputdomio",
	    mechanism => (value, value, value, value, value) );
procedure IIputdomioF( ind: Address;  isvar, typ, len: Integer;
		fval: Long_Float );
 pragma   interface( C, IIputdomioF );
 pragma   import_procedure( IIputdomioF,
	    external  => "IIputdomio",
	    mechanism => (value, value, value, value, reference) );
procedure IIputdomioA( ind: Address; isvar, typ, len: Integer; addr: Address );
 pragma   interface( C, IIputdomioA );
 pragma   import_procedure( IIputdomioA,
	    external  => "IIputdomio",
	    mechanism => (value, value, value, value, value) );
procedure IIputdomioS( ind: Address; isvar, typ, len: Integer; str: String );
 pragma   interface( C, IIputdomioS );
 pragma   import_procedure( IIputdomioS,
	    external  => "IIxputdomio",
	    mechanism => (value, value, value, value, descriptor(s)) );

procedure IInotrimioS( ind: Address; isvar, typ, len: Integer; str: String );
 pragma   interface( C, IInotrimioS );
 pragma   import_procedure( IInotrimioS,
	    external  => "IIxnotrimio",
	    mechanism => (value, value, value, value, descriptor(s)) );

procedure IIgetdomioA( ind: Address; isvar, typ, len: Integer; addr: Address );
 pragma   interface( C, IIgetdomioA );
 pragma   import_procedure( IIgetdomioA,
	    external  => "IIgetdomio",
	    mechanism => (value, value, value, value, value) );
procedure IIgetdomioS( ind: Address; isvar, typ, len: Integer; buffer: String );
 pragma   interface( C, IIgetdomioS );
 pragma   import_procedure( IIgetdomioS,
	    external  => "IIxgetdomio",
	    mechanism => (value, value, value, value, descriptor(s)) );
procedure IILQlpd_LoPutDataI(isvar, typ, len, segment,seglen,dataend: Integer);
 pragma   interface( C, IILQlpd_LoPutDataI );
 pragma   import_procedure( IILQlpd_LoPutDataI,
            external  => "IILQlpd_LoPutData",
            mechanism => (value, value, value, value, value, value) );
procedure IILQlpd_LoPutDataF(isvar, typ, len: Integer; segment: Long_Float;
		seglen, dataend: Integer);
 pragma   interface( C, IILQlpd_LoPutDataF );
 pragma   import_procedure( IILQlpd_LoPutDataF,
            external  => "IILQlpd_LoPutData",
            mechanism => (value, value, value, reference, value, value) );
procedure IILQlpd_LoPutDataA(isvar, typ, len: Integer; addr: Address;
		seglen, dataend: Integer);
 pragma   interface (C, IILQlpd_LoPutDataA);
 pragma   import_procedure(IILQlpd_LoPutDataA,
	    external  => "IILQlpd_LoPutData",
	    mechanism => (value, value, value, value, value, value) );
procedure IILQlpd_LoPutDataS(isvar, typ, len: Integer; str: String;
		seglen, dataend: Integer);
 pragma   interface( C, IILQlpd_LoPutDataS);
 pragma   import_procedure( IILQlpd_LoPutDataS,
            external  => "IIxLQlpd_LoPutData",
            mechanism => (value, value, value, descriptor(s), value, value) );
procedure IILQlgd_LoGetDataA(isvar, typ, len: Integer; addr: Address; 
		maxlen: Integer; seglen, dataend: Address);
 pragma   interface (C, IILQlgd_LoGetDataA);
 pragma   import_procedure(IILQlgd_LoGetDataA,
	    external  => "IILQlgd_LoGetData",
	    mechanism => (value, value, value, value, value, value, value) );
procedure IILQlgd_LoGetDataS(isvar, typ, len: Integer; buffer: String;
                maxlen: Integer; seglen, dataend: Address);
 pragma   interface (C, IILQlgd_LoGetDataS);
 pragma   import_procedure(IILQlgd_LoGetDataS,
        external  => "IIxLQlgd_LoGetData",
        mechanism => (value, value, value, descriptor(s),value, value, value) );


-- 
-- EQUEL (Libq) non-database procedures and functions 
--

-- CALL to subsystems 
procedure IIutsys( flag: Integer;  name, strval: String );
 pragma   interface( C, IIutsys );
 pragma   import_procedure( IIutsys,
	    mechanism => (value, reference, reference) );

-- Status info Input/Output

procedure IIeqstioI( obj: String; ind: Address; isvar, typ, len: Integer;
		ival: Integer );
 pragma   interface( C, IIeqstioI );
 pragma   import_procedure( IIeqstioI,
	    external  => "IIeqstio",
	    mechanism => (reference, value, value, value, value, value) );
procedure IIeqstioA( obj: String; ind: Address; isvar, typ, len: Integer;
		addr: Address );
 pragma   interface( C, IIeqstioA );
 pragma   import_procedure( IIeqstioA,
	    external  => "IIeqstio",
	    mechanism => (reference, value, value, value, value, value) );
procedure IIeqstioS( obj: String; ind: Address; isvar, typ, len: Integer;
		str: String );
 pragma   interface( C, IIeqstioS );
 pragma   import_procedure( IIeqstioS,
	    external  => "IIxeqstio",
	    mechanism => (reference, value, value, value, value, descriptor(s))
	  );

procedure IIeqiqioA( ind: Address; isvar, typ, len: Integer; addr: Address;
		obj: String );
 pragma   interface( C, IIeqiqioA );
 pragma   import_procedure( IIeqiqioA,
	    external  => "IIeqiqio",
	    mechanism => (value, value, value, value, value, reference) );

procedure IIeqiqioS( ind: Address; isvar, typ, len: Integer; buffer,
		obj: String );
 pragma   interface( C, IIeqiqioS );
 pragma   import_procedure( IIeqiqioS,
	    external  => "IIxeqiqio",
	    mechanism => (value, value, value, value, descriptor(s), reference)
	  );

procedure IILQssSetSqlioI( obj: Integer; ind: Address; isvar, typ, len: Integer;
		ival: Integer );
 pragma   interface( C, IILQssSetSqlioI );
 pragma   import_procedure( IILQssSetSqlioI,
	    external  => "IILQssSetSqlio",
	    mechanism => (value, value, value, value, value, value) );
procedure IILQssSetSqlioA( obj: Integer; ind: Address; isvar, typ, len: Integer;
		addr: Address );
 pragma   interface( C, IILQssSetSqlioA );
 pragma   import_procedure( IILQssSetSqlioA,
	    external  => "IILQssSetSqlio",
	    mechanism => (value, value, value, value, value, value) );
procedure IILQssSetSqlioS( obj: Integer; ind: Address; isvar, typ, len: Integer;
		str: String );
 pragma   interface( C, IILQssSetSqlioS );
 pragma   import_procedure( IILQssSetSqlioS,
	    external  => "IIxLQssSetSqlio",
	    mechanism => (value, value, value, value, value, descriptor(s))
	  );

procedure IILQisInqSqlioA( ind: Address; isvar, typ, len: Integer; 
                addr: Address; obj: Integer );
 pragma   interface( C, IILQisInqSqlioA );
 pragma   import_procedure( IILQisInqSqlioA,
	    external  => "IILQisInqSqlio",
	    mechanism => (value, value, value, value, value, value) );

procedure IILQisInqSqlioS( ind: Address; isvar, typ, len: Integer; 
		buffer: String; obj: Integer );
 pragma   interface( C, IILQisInqSqlioS );
 pragma   import_procedure( IILQisInqSqlioS,
	    external  => "IIxLQisInqSqlio",
	    mechanism => (value, value, value, value, descriptor(s), value)
	  );

-- Handlers
procedure IILQshSetHandler(hdlr : Integer; funcptr : Address);
	  pragma   interface( C, IILQshSetHandler);
	  pragma   import_procedure( IILQshSetHandler,
	  mechanism => (value, value) );

procedure IILQldh_LoDataHandler(typ: Integer; indvar, hdlr, hdlarg: Address);
 pragma interface (C, IILQldh_LoDataHandler);
 pragma   import_procedure(IILQldh_LoDataHandler,
	  mechanism => (value, value, value, value) );



-- Miscellaneous 
function  IIerrtest return Integer;
 pragma   interface( C, IIerrtest );
 pragma   import_function( IIerrtest );

--
-- 6.0 routines
--

-- Miscellaneous utilities
procedure IIxact( oper: Integer );
 pragma   interface( C, IIxact );
 pragma   import_procedure( IIxact,
	    mechanism => (value) );

procedure IIvarstrioI(  ind: Address; isvar, typ, len: Integer;
			ival: Integer );
 pragma   interface( C, IIvarstrioI );
 pragma   import_procedure( IIvarstrioI,
	    external  => "IIvarstrio",
	    mechanism => (value, value, value, value, value) );
procedure IIvarstrioF( ind: Address; isvar, typ, len: Integer;
			fval: Long_Float );
 pragma   interface( C, IIvarstrioF );
 pragma   import_procedure( IIvarstrioF,
	    external  => "IIvarstrio",
	    mechanism => (value, value, value, value, reference) );
procedure IIvarstrioA(  ind: Address; isvar, typ, len: Integer;
			addr: Address );
 pragma   interface( C, IIvarstrioA );
 pragma   import_procedure( IIvarstrioA,
	    external  => "IIvarstrio",
	    mechanism => (value, value, value, value, value) );
procedure IIvarstrioS(  ind: Address; isvar, typ, len: Integer;
			str: String );
 pragma   interface( C, IIvarstrioS );
 pragma   import_procedure( IIvarstrioS,
	    external  => "IIxvarstrio",
	    mechanism => (value, value, value, value, descriptor(s))
	  );

-- Repeat queries
procedure IIexExec( oper: Integer; name: String; id1, id2: Integer );
 pragma   interface( C, IIexExec );
 pragma   import_procedure( IIexExec,
	    mechanism => (value, reference, value, value ) );

procedure IIexDefine( oper: Integer; name: String; id1, id2: Integer );
 pragma   interface( C, IIexDefine );
 pragma   import_procedure( IIexDefine,
	    mechanism => (value, reference, value, value ) );

-- Cursors
procedure IIcsOpen( name: String; id1, id2: Integer );
 pragma   interface( C, IIcsOpen );
 pragma   import_procedure( IIcsOpen,
	    mechanism => (reference, value, value ) );

procedure IIcsQuery( name: String; id1, id2: Integer );
 pragma   interface( C, IIcsQuery );
 pragma   import_procedure( IIcsQuery,
	    mechanism => (reference, value, value ) );

procedure IIcsRdO( oper: Integer; rdo: String );
 pragma   interface( C, IIcsRdO );
 pragma   import_procedure( IIcsRdO,
	    mechanism => (value, reference) );

function  IIcsRetrieve( name: String; id1, id2: Integer ) return Integer;
 pragma   interface( C, IIcsRetrieve );
 pragma   import_function( IIcsRetrieve,
	    mechanism => (reference, value, value) );

procedure IIcsGetioA( ind: Address; isvar, typ, len: Integer; addr: Address );
 pragma   interface( C, IIcsGetioA );
 pragma   import_procedure( IIcsGetioA,
	    external => "IIcsGetio",
	    mechanism => (value, value, value, value, value) );

procedure IIcsGetioS( ind: Address; isvar, typ, len: Integer; buffer: String );
 pragma   interface( C, IIcsGetioS );
 pragma   import_procedure( IIcsGetioS,
	    external => "IIxcsGetio",
	    mechanism => (value, value, value, value, descriptor(s)) );

procedure IIcsERetrieve;
 pragma   interface( C, IIcsERetrieve );
 pragma   import_procedure( IIcsERetrieve );

function  IIcsRetScroll( csname: String; id1, id2, fetcho, fetchn: Integer ) return Integer;
 pragma   interface(C, IIcsRetScroll );
 pragma   import_function( IIcsRetScroll,
	    mechanism => (reference, value, value, value, value) );

procedure IIcsClose( csname: String; id1, id2: Integer );
 pragma   interface( C, IIcsClose );
 pragma   import_procedure( IIcsClose,
	    mechanism => (reference, value, value) );

procedure IIcsDelete( tblname, csname: String; id1, id2: Integer );
 pragma   interface( C, IIcsDelete );
 pragma   import_procedure( IIcsDelete,
	    mechanism => (reference, reference, value, value) );

procedure IIcsReplace( csname: String; id1, id2: Integer );
 pragma   interface( C, IIcsReplace );
 pragma   import_procedure( IIcsReplace,
	    mechanism => (reference, value, value) );

procedure IIcsERplace( csname: String; id1, id2: Integer );
 pragma   interface( C, IIcsERplace );
 pragma   import_procedure( IIcsERplace,
	    mechanism => (reference, value, value) );

-- Events
procedure IILQesEvStat( flag: Integer; waitsecs: Integer );
 pragma   interface( C, IILQesEvStat );
 pragma   import_procedure( IILQesEvStat,
	    mechanism => (value, value) );

-- exec 4gl
procedure IIG4acArrayClear(ary: Integer);
 pragma interface (C, IIG4acArrayClear);
 pragma import_procedure (IIG4acArrayClear, 
	    mechanism => (value) );

procedure IIG4rrRemRow(ary, indx: Integer);
 pragma interface (C, IIG4rrRemRow);
 pragma import_procedure (IIG4rrRemRow, 
	    mechanism => (value, value) );

procedure IIG4irInsRow(ary, indx, row, state, which: Integer);
 pragma interface (C, IIG4irInsRow);
 pragma import_procedure (IIG4irInsRow, 
	    mechanism => (value, value, value, value, value) );

procedure IIG4drDelRow(ary, indx: Integer);
 pragma interface (C, IIG4drDelRow);
 pragma import_procedure (IIG4drDelRow, 
	    mechanism => (value, value) );

procedure IIG4srSetRow(ary, indx, row: Integer);
 pragma interface (C, IIG4srSetRow);
 pragma import_procedure (IIG4srSetRow, 
	    mechanism => (value, value, value) );

procedure IIG4grGetRowA(rowind: Address;
			isvar, rowtype, rowlen: Integer;
			rowptr: Address; ary, indx: Integer);
 pragma interface (C, IIG4grGetRowA);
 pragma import_procedure (IIG4grGetRowA, 
	    external  => "IIG4grGetRow",
	    mechanism => (value, value, value, value, value, value, value) );

procedure IIG4grGetRowS(rowind: Address;
			isvar, rowtype, rowlen: Integer;
			rowptr: String; ary, indx: Integer);
 pragma interface (C, IIG4grGetRowS);
 pragma import_procedure (IIG4grGetRowS, 
	    external  => "IIG4grGetRow",
	    mechanism => (value, value, value, value, descriptor(s),
			value, value) );

procedure IIG4ggGetGlobalA(ind: Address; isvar, typ, len: Integer;
			data, name: Address; gtype: Integer);
 pragma interface (C, IIG4ggGetGlobalA);
 pragma import_procedure (IIG4ggGetGlobalA, 
	    external  => "IIG4ggGetGlobal",
	    mechanism => (value, value, value, value, value, value, value) );

procedure IIG4ggGetGlobalS(ind: Address; isvar, typ, len: Integer;
			data, name: String; gtype: Integer);
 pragma interface (C, IIG4ggGetGlobalS);
 pragma import_procedure (IIG4ggGetGlobalS, 
	    external  => "IIG4ggGetGlobal",
	    mechanism => (value, value, value, value, descriptor(s),
			descriptor(s), value) );

procedure IIG4sgSetGlobalA(name, ind: Address; isvar, typ, len: Integer;
			data: Address);
 pragma interface (C, IIG4sgSetGlobalA);
 pragma import_procedure (IIG4sgSetGlobalA, 
	    external  => "IIG4sgSetGlobal",
	    mechanism => (value, value, value, value, value, value) );

procedure IIG4sgSetGlobalI(name, ind: Address; isvar, typ, len: Integer;
			data: Integer);
 pragma interface (C, IIG4sgSetGlobalI);
 pragma import_procedure (IIG4sgSetGlobalI, 
	    external  => "IIG4sgSetGlobal",
	    mechanism => (value, value, value, value, value, value) );

procedure IIG4sgSetGlobalS(name, ind: Address; isvar, typ, len: Integer;
			data: String);
 pragma interface (C, IIG4sgSetGlobalS);
 pragma import_procedure (IIG4sgSetGlobalS, 
	    external  => "IIG4sgSetGlobal",
	    mechanism => (value, value, value, value, value, descriptor(s)) );

procedure IIG4gaGetAttrA(ind: Address; isvar, typ, len: Integer;
			data, name: Address);
 pragma interface (C, IIG4gaGetAttrA);
 pragma import_procedure (IIG4gaGetAttrA, 
	    external  => "IIG4gaGetAttr",
	    mechanism => (value, value, value, value, value, value) );

procedure IIG4gaGetAttrS(ind: Address; isvar, typ, len: Integer;
			data, name: String);
 pragma interface (C, IIG4gaGetAttrS);
 pragma import_procedure (IIG4gaGetAttrS, 
	    external  => "IIG4gaGetAttr",
	    mechanism => (value, value, value, value,
			descriptor(s), descriptor(s)) );

procedure IIG4saSetAttrA(name, ind: Address; isvar, typ, len: Integer;
			data: Address);
 pragma interface (C, IIG4saSetAttrA);
 pragma import_procedure (IIG4saSetAttrA, 
	    external  => "IIG4saSetAttr",
	    mechanism => (value, value, value, value, value, value) );

procedure IIG4saSetAttrI(name, ind: Address; isvar, typ, len: Integer;
			data: Integer);
 pragma interface (C, IIG4saSetAttrI);
 pragma import_procedure (IIG4saSetAttrI, 
	    external  => "IIG4saSetAttr",
	    mechanism => (value, value, value, value, value, value) );

procedure IIG4saSetAttrS(name, ind: Address; isvar, typ, len: Integer;
			data: String);
 pragma interface (C, IIG4saSetAttrS);
 pragma import_procedure (IIG4saSetAttrS, 
	    external  => "IIG4saSetAttr",
	    mechanism => (value, value, value, value, value, descriptor(s)) );

procedure IIG4chkobj(obj, acs, indx, caller: Integer);
 pragma interface (C, IIG4chkobj);
 pragma import_procedure (IIG4chkobj, 
	    mechanism => (value, value, value, value) );

procedure IIG4udUseDscrA(lang, direction: Integer; sqlda: Address);
 pragma interface (C, IIG4udUseDscrA);
 pragma import_procedure (IIG4udUseDscrA, 
	    external  => "IIG4udUseDscr",
	    mechanism => (value, value, value) );

procedure IIG4udUseDscrI(lang, direction: Integer; sqlda: Integer);
 pragma interface (C, IIG4udUseDscrI);
 pragma import_procedure (IIG4udUseDscrI, 
	    external  => "IIG4udUseDscr",
	    mechanism => (value, value, value) );

procedure IIG4udUseDscrS(lang, direction: Integer; sqlda: String);
 pragma interface (C, IIG4udUseDscrS);
 pragma import_procedure (IIG4udUseDscrS, 
	    external  => "IIG4udUseDscr",
	    mechanism => (value, value, descriptor(s)) );

procedure IIG4fdFillDscrA(obj, lang: Integer; sqlda: Address);
 pragma interface (C, IIG4fdFillDscrA);
 pragma import_procedure (IIG4fdFillDscrA, 
	    external  => "IIG4fdFillDscr",
	    mechanism => (value, value, value) );

procedure IIG4fdFillDscrS(obj, lang: Integer; sqlda: String);
 pragma interface (C, IIG4fdFillDscrS);
 pragma import_procedure (IIG4fdFillDscrS, 
	    external  => "IIG4fdFillDscr",
	    mechanism => (value, value, descriptor(s)) );

procedure IIG4icInitCall(name: Address; typ: Integer);
 pragma interface (C, IIG4icInitCall);
 pragma import_procedure (IIG4icInitCall, 
	    mechanism => (value, value) );

procedure IIG4vpValParamA(name, ind: Address; isval, typ, len: Integer;
			data: Address);
 pragma interface (C, IIG4vpValParamA);
 pragma import_procedure (IIG4vpValParamA, 
	    external  => "IIG4vpValParam",
	    mechanism => (value, value, value, value, value, value) );

procedure IIG4vpValParamS(name, ind: Address; isval, typ, len: Integer;
			data: String);
 pragma interface (C, IIG4vpValParamS);
 pragma import_procedure (IIG4vpValParamS, 
	    external  => "IIG4vpValParam",
	    mechanism => (value, value, value, value, value, descriptor(s)) );

procedure IIG4bpByrefParamA(ind: Address; isval, typ, len: Integer;
			data, name: Address);
 pragma interface (C, IIG4bpByrefParamA);
 pragma import_procedure (IIG4bpByrefParamA, 
	    external  => "IIG4bpByrefParam",
	    mechanism => (value, value, value, value, value, value) );

procedure IIG4bpByrefParamS(ind: Address; isval, typ, len: Integer;
			data, name: String);
 pragma interface (C, IIG4bpByrefParamS);
 pragma import_procedure (IIG4bpByrefParamS, 
	    external  => "IIG4bpByrefParam",
	    mechanism => (value, value, value, value,
			descriptor(s), descriptor(s)) );

procedure IIG4rvRetValA(ind: Address; isval, typ, len: Integer;
			data: Address);
 pragma interface (C, IIG4rvRetValA);
 pragma import_procedure (IIG4rvRetValA, 
	    external  => "IIG4rvRetVal",
	    mechanism => (value, value, value, value, value) );

procedure IIG4rvRetValS(ind: Address; isval, typ, len: Integer;
			data: String);
 pragma interface (C, IIG4rvRetValS);
 pragma import_procedure (IIG4rvRetValS, 
	    external  => "IIG4rvRetVal",
	    mechanism => (value, value, value, value, descriptor(s)) );

procedure IIG4ccCallComp;
 pragma interface (C, IIG4ccCallComp);
 pragma import_procedure (IIG4ccCallComp); 

procedure IIG4i4Inq4GLA(ind: Address; isvar, typ, len: Integer;
			data: Address; obj, code: Integer);
 pragma interface (C, IIG4i4Inq4GLA);
 pragma import_procedure (IIG4i4Inq4GLA, 
	    external  => "IIG4i4Inq4GL",
	    mechanism => (value, value, value, value, value, value, value) );

procedure IIG4i4Inq4GLS(ind: Address; isvar, typ, len: Integer;
			data: String; obj, code: Integer);
 pragma interface (C, IIG4i4Inq4GLS);
 pragma import_procedure (IIG4i4Inq4GLS, 
	    external  => "IIG4i4Inq4GL",
	    mechanism => (value, value, value, value, descriptor(s),
			value, value) );

procedure IIG4s4Set4GLA(obj, code: Integer; ind: Address;
			isvar, typ, len: Integer; data: Address);
 pragma interface (C, IIG4s4Set4GLA);
 pragma import_procedure (IIG4s4Set4GLA, 
	    external  => "IIG4s4Set4GL",
	    mechanism => (value, value, value, value, value, value, value) );

procedure IIG4s4Set4GLI(obj, code: Integer; ind: Address;
			isvar, typ, len: Integer; data: Integer);
 pragma interface (C, IIG4s4Set4GLI);
 pragma import_procedure (IIG4s4Set4GLI, 
	    external  => "IIG4s4Set4GL",
	    mechanism => (value, value, value, value, value, value, value) );

procedure IIG4s4Set4GLS(obj, code: Integer; ind: Address;
			isvar, typ, len: Integer; data: String);
 pragma interface (C, IIG4s4Set4GLS);
 pragma import_procedure (IIG4s4Set4GLS, 
	    external  => "IIG4s4Set4GL",
	    mechanism => (value, value, value, value, value, value,
			descriptor(s)) );

procedure IIG4seSendEvent(frame: Integer);
 pragma interface (C, IIG4seSendEvent);
 pragma import_procedure (IIG4seSendEvent, 
	    mechanism => (value) );

end EQUEL;


--
-- EQUEL Forms System package
--

with SYSTEM;

package	EQUEL_FORMS is

use SYSTEM;

-- 
-- Forms (RUNTIME) procedures and functions 
--

-- Forms setup 
procedure IIaddform( formid: Integer );
 pragma   interface( C, IIaddform );
 pragma   import_procedure( IIaddform,
	    mechanism => (value) );

procedure IIendforms;
 pragma   interface( C, IIendforms );
 pragma   import_procedure( IIendforms );

procedure IIforminit( formnm: String );
 pragma   interface( C, IIforminit );
 pragma   import_procedure( IIforminit,
	    mechanism => (reference) );

procedure IIforms( lang: Integer );
 pragma   interface( C, IIforms );
 pragma   import_procedure( IIforms,
	    mechanism => (value) );

-- Menus 
function  IIendmu return Integer;
 pragma   interface( C, IIendmu );
 pragma   import_function( IIendmu );

function  IIinitmu return Integer;
 pragma   interface( C, IIinitmu );
 pragma   import_function( IIinitmu );

procedure IImuonly;
 pragma   interface( C, IImuonly );
 pragma   import_procedure( IImuonly );

procedure IIrunmu( onoff: Integer );
 pragma   interface( C, IIrunmu );
 pragma   import_procedure( IIrunmu,
	    mechanism => (value) );

-- 6.0 Menu routines

function  IInestmu return Integer;
 pragma   interface( C, IInestmu );
 pragma   import_function( IInestmu );

function IIrunnest return Integer;
 pragma   interface( C, IIrunnest );
 pragma   import_function( IIrunnest );

procedure IIendnest;
 pragma   interface( C, IIendnest );
 pragma   import_procedure( IIendnest );

-- Miscellaneous Utilities (CLEAR, SLEEP, MESSAGE, PROMPT, HELP) 
procedure IIclrflds;
 pragma   interface( C, IIclrflds );
 pragma   import_procedure( IIclrflds );

procedure IIclrscr;
 pragma   interface( C, IIclrscr );
 pragma   import_procedure( IIclrscr );

procedure IIfldclear( fld: String );
 pragma   interface( C, IIfldclear );
 pragma   import_procedure( IIfldclear,
	    mechanism => (reference) );

procedure IIfrshelp( flag: Integer; subj, filenm: String );
 pragma   interface( C, IIfrshelp );
 pragma   import_procedure( IIfrshelp,
	    mechanism => (value, reference, reference) );

procedure IIhelpfile( subj, filenm: String );
 pragma   interface( C, IIhelpfile );
 pragma   import_procedure( IIhelpfile,
	    mechanism => (reference, reference) );

procedure IImessage( msg: String );
 pragma   interface( C, IImessage );
 pragma   import_procedure( IImessage,
	    mechanism => (reference) );

procedure IIprnscr( filenm: String );
 pragma   interface( C, IIprnscr );
 pragma   import_procedure( IIprnscr,
	    mechanism => (reference) );

procedure IIsleep( secs: Integer );
 pragma   interface( C, IIsleep );
 pragma   import_procedure( IIsleep,
	    mechanism => (value) );

procedure IIprmptioS( echo: Integer; prompt: String; ind: Address;
			isvar, typ, len: Integer; retbuf: String );
 pragma   interface( C, IIprmptioS );
 pragma   import_procedure( IIprmptioS,
	    external  => IIxprmptio,
	    mechanism => (value, reference, value, value, value, value,
				descriptor(s)) );

-- Validation 
function  IIchkfrm return Integer;
 pragma   interface( C, IIchkfrm );
 pragma   import_function( IIchkfrm );

function  IIvalfld( fld: String ) return Integer;
 pragma   interface( C, IIvalfld );
 pragma   import_function( IIvalfld,
	    mechanism => (reference) );

-- DISPLAY Loop 
function  IIdispfrm( form, mode: String ) return Integer;
 pragma   interface( C, IIdispfrm );
 pragma   import_function( IIdispfrm,
	    mechanism => (reference, reference) );

procedure IIendfrm;
 pragma   interface( C, IIendfrm );
 pragma   import_procedure( IIendfrm );

function  IIfnames return Integer;
 pragma   interface( C, IIfnames );
 pragma   import_function( IIfnames );

function  IIFRitIsTimeout return Integer;
 pragma   interface( C, IIFRitIsTimeout );
 pragma   import_function( IIFRitIsTimeout );

procedure IIredisp;
 pragma   interface( C, IIredisp );
 pragma   import_procedure( IIredisp );

procedure IIrescol( tab, col: String );
 pragma   interface( C, IIrescol );
 pragma   import_procedure( IIrescol,
	    mechanism => (reference, reference) );

procedure IIresfld( fld: String );
 pragma   interface( C, IIresfld );
 pragma   import_procedure( IIresfld,
	    mechanism => (reference) );

procedure IIFRgotofld( dir: Integer );
 pragma   interface( C, IIFRgotofld );
 pragma   import_procedure( IIFRgotofld,
	    mechanism => (value) );

procedure IIresmu;
 pragma   interface( C, IIresmu );
 pragma   import_procedure( IIresmu );

procedure IIresnext;
 pragma   interface( C, IIresnext );
 pragma   import_procedure( IIresnext );

procedure IIFRreResEntry;
 pragma   interface( C, IIFRreResEntry );
 pragma   import_procedure( IIFRreResEntry );

function  IIretval return Integer;
 pragma   interface( C, IIretval );
 pragma   import_function( IIretval );

function  IIrunform return Integer;
 pragma   interface( C, IIrunform );
 pragma   import_function( IIrunform );

-- ACTIVATE blocks 

-- Old style (pre 6.4)
function  IIactclm( tab, col: String; intrp: Integer ) return Integer;
 pragma   interface( C, IIactclm );
 pragma   import_function( IIactclm,
	    mechanism => (reference, reference, value) );

-- New style (6.4 version)
function  IITBcaClmAct( tab, col: String; 
				entact, intrp: Integer ) return Integer;
 pragma   interface( C, IITBcaClmAct );
 pragma   import_function( IITBcaClmAct,
	    mechanism => (reference, reference, value, value) );

function  IIactcomm( command, intrp: Integer ) return Integer;
 pragma   interface( C, IIactcomm );
 pragma   import_function( IIactcomm,
	    mechanism => (value, value) );

-- Old style (pre 6.4)
function  IIactfld( fld: String; intrp: Integer ) return Integer;
 pragma   interface( C, IIactfld );
 pragma   import_function( IIactfld,
	    mechanism => (reference, value) );
	    
-- New style (6.4 version)
function  IIFRafActFld( fld: String; entact, intrp: Integer ) return Integer;
 pragma   interface( C, IIFRafActFld );
 pragma   import_function( IIFRafActFld,
	    mechanism => (reference, value, value) );

function  IInmuact( menu, expl: String; valid, activ, intrp: Integer )
			return Integer;
 pragma   interface( C, IInmuact );
 pragma   import_function( IInmuact,
	    mechanism => (reference, reference, value, value, value) );

function  IIactscrl( tab: String; updown, intrp: Integer ) return Integer;
 pragma   interface( C, IIactscrl );
 pragma   import_function( IIactscrl,
	    mechanism => (reference, value, value) );

function  IInfrskact( keynum: Integer;
				expl: String; valid, activ, intrp: Integer )
				return Integer;
 pragma   interface( C, IInfrskact );
 pragma   import_function( IInfrskact,
	    mechanism => (value, reference, value, value, value) );

function  IIFRtoact( actval: Integer; intrp: Integer ) return Integer;
 pragma  interface( C, IIFRtoact );
 pragma  import_function( IIFRtoact,
		mechanism => (value, value) );

function  IIFRaeAlerterEvent( intrp: Integer ) return Integer;
 pragma  interface( C, IIFRaeAlerterEvent );
 pragma  import_function( IIFRaeAlerterEvent,
		mechanism => (value) );

-- Field Input/Output
function  IIfsetio( form: String ) return Integer;
 pragma   interface( C, IIfsetio );
 pragma   import_function( IIfsetio,
	    mechanism => (reference) );

procedure IIgetoper( putget: Integer );
 pragma   interface( C, IIgetoper );
 pragma   import_procedure( IIgetoper,
	    mechanism => (value) );

procedure IIputoper( putget: Integer );
 pragma   interface( C, IIputoper );
 pragma   import_procedure( IIputoper,
	    mechanism => (value) );

procedure IIputfldioI( fld: String; ind: Address; isvar, typ, len: Integer;
			ival: Integer );
 pragma   interface( C, IIputfldioI );
 pragma   import_procedure( IIputfldioI,
	    external  => "IIputfldio",
	    mechanism => (reference, value, value, value, value, value) );
procedure IIputfldioF( fld: String; ind: Address; isvar, typ, len: Integer;
		       fval: Long_Float );
 pragma   interface( C, IIputfldioF );
 pragma   import_procedure( IIputfldioF,
	    external  => "IIputfldio",
	    mechanism => (reference, value, value, value, value, reference) );
procedure IIputfldioA( fld: String; ind: Address; isvar, typ, len: Integer;
			addr: Address );
 pragma   interface( C, IIputfldioA );
 pragma   import_procedure( IIputfldioA,
	    external  => "IIputfldio",
	    mechanism => (reference, value, value, value, value, value) );
procedure IIputfldioD( fld: String; ind: Address; isvar, typ, len: Integer;
			decstr: String );
 pragma   interface( C, IIputfldioD );
 pragma   import_procedure( IIputfldioD,
	    external  => "IIputfldio",
	    mechanism => (reference, value, value, value, value, reference) );
procedure IIputfldioS( fld: String; ind: Address; isvar, typ, len: Integer;
			str: String );
 pragma   interface( C, IIputfldioS );
 pragma   import_procedure( IIputfldioS,
	    external  => "IIxputfldio",
	    mechanism => (reference, value, value, value, value, descriptor(s))
	  );

procedure IIgetfldioA( ind: Address; isvar, typ, len: Integer; addr: Address;
		fld: String );
 pragma   interface( C, IIgetfldioA );
 pragma   import_procedure( IIgetfldioA,
	    external  => "IIgetfldio",
	    mechanism => (value, value, value, value, value, reference) );
procedure IIgetfldioS( ind: Address; isvar, typ, len: Integer; buffer: String;
		fld: String );
 pragma   interface( C, IIgetfldioS );
 pragma   import_procedure( IIgetfldioS,
	    external  => "IIxgetfldio",
	    mechanism => (value, value, value, value, descriptor(s), reference)
	  );


-- INQUIRE_FRS and SET_FRS

-- Old style (pre 4.0) is not supported. Error from preprocessor.

-- New style (4.0 version)
function  IIiqset( objtype: Integer; row: Integer;
		   nm1, nm2, nm3: String := String'null_parameter ) 
		   return Integer;
 pragma   interface( C, IIiqset );
 pragma   import_function( IIiqset,
	    mechanism => (value, value, reference, reference, reference) );

procedure IIstfsioI(attr: Integer; obj: String; row: Integer; ind: Address;
			isvar, typ, len: Integer; ival: Integer);
 pragma   interface( C, IIstfsioI );
 pragma   import_procedure( IIstfsioI,
	    external  => "IIstfsio",
	    mechanism => (value, reference, value, value, value, value, value,
			  value) );
procedure IIstfsioF(attr: Integer; obj: String; row: Integer; ind: Address;
			isvar, typ, len: Integer; fval: Long_Float);
 pragma   interface( C, IIstfsioF );
 pragma   import_procedure( IIstfsioF,
	    external  => "IIstfsio",
	    mechanism => (value, reference, value, value, value, value, value,
			  reference) );
procedure IIstfsioA(attr: Integer; obj: String; row: Integer; ind: Address;
			isvar, typ, len: Integer; addr: Address);
 pragma   interface( C, IIstfsioA );
 pragma   import_procedure( IIstfsioA,
	    external  => "IIstfsio",
	    mechanism => (value, reference, value, value, value, value, value,
			  value) );
procedure IIstfsioS(attr: Integer; obj: String; row: Integer; ind: Address;
			isvar, typ, len: Integer; str: String);
 pragma   interface( C, IIstfsioS );
 pragma   import_procedure( IIstfsioS,
	    external  => "IIxstfsio",
	    mechanism => (value, reference, value, value, value, value, value,
			  descriptor(s)) );

procedure IIiqfsioA( ind: Address; isvar, typ, len: Integer; addr: Address;
			attr: Integer; obj: String; row: Integer );
 pragma   interface( C, IIiqfsioA );
 pragma   import_procedure( IIiqfsioA,
	    external  => "IIiqfsio",
	    mechanism => (value, value, value, value, value,
			  value, reference, value) );
procedure IIiqfsioS( ind: Address; isvar, typ, len: Integer; buffer: String;
			attr: Integer; obj: String; row: Integer );
 pragma   interface( C, IIiqfsioS );
 pragma   import_procedure( IIiqfsioS,
	    external  => "IIxiqfsio",
	    mechanism => (value, value, value, value, descriptor(s),
			  value, reference, value) );

-- 6.4 setting per value display attributes

procedure IIFRsaSetAttrioI(attr: Integer; obj: String; ind: Address;
			isvar, typ, len: Integer; ival: Integer);
 pragma   interface( C, IIFRsaSetAttrioI );
 pragma   import_procedure( IIFRsaSetAttrioI,
	    external  => "IIFRsaSetAttrio",
	    mechanism => (value, reference, value, value, value, value,
			  value) );
procedure IIFRsaSetAttrioF(attr: Integer; obj: String; ind: Address;
			isvar, typ, len: Integer; fval: Long_Float);
 pragma   interface( C, IIFRsaSetAttrioF );
 pragma   import_procedure( IIFRsaSetAttrioF,
	    external  => "IIFRsaSetAttrio",
	    mechanism => (value, reference, value, value, value, value,
			  reference) );
procedure IIFRsaSetAttrioA(attr: Integer; obj: String; ind: Address;
			isvar, typ, len: Integer; addr: Address);
 pragma   interface( C, IIFRsaSetAttrioA );
 pragma   import_procedure( IIFRsaSetAttrioA,
	    external  => "IIFRsaSetAttrio",
	    mechanism => (value, reference, value, value, value, value,
			  value) );
procedure IIFRsaSetAttrioS(attr: Integer; obj: String; ind: Address;
			isvar, typ, len: Integer; str: String);
 pragma   interface( C, IIFRsaSetAttrioS );
 pragma   import_procedure( IIFRsaSetAttrioS,
	    external  => "IIxFRsaSetAttrio",
	    mechanism => (value, reference, value, value, value, value,
			  descriptor(s)) );


-- 
-- Table field (TBACC) procedures and functions 
--

-- INITTABLE 
function  IItbinit( form, tab, mode: String ) return Integer;
 pragma   interface( C, IItbinit );
 pragma   import_function( IItbinit,
	    mechanism => (reference, reference, reference) );

procedure IItfill;
 pragma   interface( C, IItfill );
 pragma   import_procedure( IItfill );

procedure IIthidecol( col, typ: String );
 pragma   interface( C, IIthidecol );
 pragma   import_procedure( IIthidecol,
	    mechanism => (reference, reference) );

-- LOADTABLE and UNLOADTABLE
function  IItbact( form, tab: String; toload: Integer ) return Integer;
 pragma   interface( C, IItbact );
 pragma   import_function( IItbact,
	    mechanism => (reference, reference, value) );

procedure IItunend;
 pragma   interface( C, IItunend );
 pragma   import_procedure( IItunend );

function  IItunload return Integer;
 pragma   interface( C, IItunload );
 pragma   import_function( IItunload );

-- Table Field Statment Setup (Standard, Insert, Delete, Scroll) 
function  IItbsetio( comm: Integer; form, tab: String; row: Integer )
		     return Integer;
 pragma   interface( C, IItbsetio );
 pragma   import_function( IItbsetio,
	    mechanism => (value, reference, reference, value) );

procedure IItbsmode( scrlmd: String );
 pragma   interface( C, IItbsmode );
 pragma   import_procedure( IItbsmode,
	    mechanism => (reference) );

function  IItdelrow( tofill: Integer ) return Integer;
 pragma   interface( C, IItdelrow );
 pragma   import_function( IItdelrow,
	    mechanism => (value) );

function  IItinsert return Integer;
 pragma   interface( C, IItinsert );
 pragma   import_function( IItinsert );

function  IItscroll( tofill, recnum: Integer ) return Integer;
 pragma   interface( C, IItscroll );
 pragma   import_function( IItscroll,
	    mechanism => (value, value) );

-- Miscellaneous Utilities (CLEARROW, VALIDROW, TABLEDATA) 
procedure IItclrcol( col: String );
 pragma   interface( C, IItclrcol );
 pragma   import_procedure( IItclrcol,
	    mechanism => (reference) );

procedure IItclrrow;
 pragma   interface( C, IItclrrow );
 pragma   import_procedure( IItclrrow );

procedure IItcolval( col: String );
 pragma   interface( C, IItcolval );
 pragma   import_procedure( IItcolval,
	    mechanism => (reference) );

function  IItdata return Integer;
 pragma   interface( C, IItdata );
 pragma   import_function( IItdata );

procedure IItdatend;
 pragma   interface( C, IItdatend );
 pragma   import_procedure( IItdatend );

procedure IItvalrow;
 pragma   interface( C, IItvalrow );
 pragma   import_procedure( IItvalrow );

function  IItvalval( check: Integer ) return Integer;
 pragma   interface( C, IItvalval );
 pragma   import_function( IItvalval,
	    mechanism => (value) );

procedure IITBceColEnd;
 pragma   interface( C, IITBceColEnd );
 pragma   import_procedure( IITBceColEnd );

-- COLUMN Input/Output

procedure IItcoputioI( col: String; ind: Address; isvar, typ, len: Integer;
			ival: Integer );
 pragma   interface( C, IItcoputioI );
 pragma   import_procedure( IItcoputioI,
	    external  => "IItcoputio",
	    mechanism => (reference, value, value, value, value, value) );
procedure IItcoputioF( col: String; ind: Address; isvar, typ, len: Integer;
			fval: Long_Float );
 pragma   interface( C, IItcoputioF );
 pragma   import_procedure( IItcoputioF,
	    external  => "IItcoputio",
	    mechanism => (reference, value, value, value, value, reference) );
procedure IItcoputioA( col: String; ind: Address; isvar, typ, len: Integer;
			addr: Address );
 pragma   interface( C, IItcoputioA );
 pragma   import_procedure( IItcoputioA,
	    external  => "IItcoputio",
	    mechanism => (reference, value, value, value, value, value) );
procedure IItcoputioD( col: String; ind: Address; isvar, typ, len: Integer;
			decstr: String );
 pragma   interface( C, IItcoputioD );
 pragma   import_procedure( IItcoputioD,
	    external  => "IItcoputio",
	    mechanism => (reference, value, value, value, value, reference) );
procedure IItcoputioS( col: String; ind: Address; isvar, typ, len: Integer;
			str: String );
 pragma   interface( C, IItcoputioS );
 pragma   import_procedure( IItcoputioS,
	    external  => "IIxtcoputio",
	    mechanism => (reference, value, value, value, value, descriptor(s))
	  );

procedure IItcogetioA( ind: Address; isvar, typ, len: Integer; addr: Address;
			col: String );
 pragma   interface( C, IItcogetioA );
 pragma   import_procedure( IItcogetioA,
	    external  => "IItcogetio",
	    mechanism => (value, value, value, value, value, reference) );
procedure IItcogetioS( ind: Address; isvar, typ, len: Integer; buffer: String;
			col: String );
 pragma   interface( C, IItcogetioS );
 pragma   import_procedure( IItcogetioS,
	    external  => "IIxtcogetio",
	    mechanism => (value, value, value, value, descriptor(s), reference)
	  );

-- 6.1  Set VENUS POPUP Info
-- IIFRgpsetio

procedure IIFRgpsetioI( pid: Integer; nullind: Address; 
		    isvar, xtype, len: Integer; val: Integer );
 pragma   interface( C, IIFRgpsetioI );
 pragma   import_procedure( IIFRgpsetioI,
	    external  => "IIFRgpsetio",
	    mechanism => (value, value, value, value, value, value) );
procedure IIFRgpsetioF( pid: Integer; nullind: Address;  
		isvar, xtype, len: Integer; fval: Long_Float );
 pragma   interface( C, IIFRgpsetioF );
 pragma   import_procedure( IIFRgpsetioF,
	    external  => "IIFRgpsetio",
	    mechanism => (value, value, value, value, value, reference) );
procedure IIFRgpsetioA( pid: Integer; nullind: Address; 
		isvar, xtype, len: Integer; addr: Address );
 pragma   interface( C, IIFRgpsetioA );
 pragma   import_procedure( IIFRgpsetioA,
	    external  => "IIFRgpsetio",
	    mechanism => (value, value, value, value, value, value) );
procedure IIFRgpsetioS( pid: Integer; nullind: Address; 
		isvar, xtype, len: Integer; str: String );
 pragma   interface( C, IIFRgpsetioS );
 pragma   import_procedure( IIFRgpsetioS,
	    external  => "IIxFRgpsetio",
	    mechanism => (value, value, value, value, value, descriptor(s)) );

-- Open/Close call to IIFRgp Unit 
procedure IIFRgpcontrol( state: Integer;  alt: Integer );
 pragma   interface( C, IIFRgpcontrol );
 pragma   import_procedure( IIFRgpcontrol,
	    mechanism => (value, value) );

-- Dynamic FRS calls
procedure IIFRsqDescribe( lang, ftag: Integer; form, tab, mode: String;
			  sq: Address );
 pragma	  interface( C, IIFRsqDescribe );
 pragma   import_procedure( IIFRsqDescribe,
	   mechanism => (value, value, reference, reference, reference, value));

procedure IIFRsqExecute( lang, ftag, itag: Integer; sq: Address );
 pragma	  interface( C, IIFRsqExecute );
 pragma   import_procedure( IIFRsqExecute,
	   mechanism => (value, value, value, value));

end EQUEL_FORMS;
