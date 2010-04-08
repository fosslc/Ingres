-- {
-- File Name: 	EQSQLCA.ADA
-- Purpose:	ADA package specification file for Embedded SQL record
--		definition and run-time INGRES/SQL  subprogram interface.
--		This version is for single-threaded programs.
--
-- Packages:	ESQL 	    - Interface to INGRES/SQL.
-- Usage:
--		EXEC SQL INCLUDE SQLCA;
--    generates:
--		with EQUEL; use EQUEL;
--		with ESQL; use ESQL;
--		with EQUEL_FORMS; use EQUEL_FORMS;
--
-- History:	01-Apr-1986	- Written. (ncg)
--		28-Jul-1987	- Modified for 6.0. (mrw)
--		17-dec-1992 (larrym)
--		    Added IIsqlcdInit and IIsqGdInit
--		31-jan-1994 (teresal)
--		    Fixed the IIsqGdInit interface to correctly pass
--		    a string descriptor address for SQLSTATE. 
--		    Bug fix 59015.
--		6-Jun-2006 (kschendel)
--		    Added IIsqDescInput
--		24-Mar-2007 (toumi01)
--		    Added II_sqlca for multi-threaded Ada.
--		04-May-2007 (toumi01)
--		    Move global copy of sqlca to package ESQLGBL so that
--		    the non-TLS sqlca can be excluded from -multi programs.
--		10-Oct-2007 (toumi01)
--		    eqsqlca.ada copied to eqsqlcam.ada and ESQL package
--		    renamed ESQLM for multi-threaded programs. So this file
--		    reverts to the version predating -multi Ada support.
--
-- Example Interface Format:
--		procedure IIname ( arg1 : Integer; arg2: String );
--		 pragma   interface( C, IIname );
--		 pragma   import_procedure( IIname, 
--				mechanism 	=> (value, reference) );
--  - only if overloaded:
--		 		external	=> "C name",
--
-- Copyright (c) 2004, 2007 Ingres Corporation
-- } 

with SYSTEM;

package	ESQL is

use SYSTEM;

-- Define the program SQL error structure.

type IISQL_ERRM is		-- Varying length string.
    record
	sqlerrml: Short_Integer;
	sqlerrmc: String(1..70);
    end record;

type IISQL_ERRD is array(1..6) of Integer;
	
type IISQL_WARN is 		-- Warning structure.
    record
      sqlwarn0: Character;
      sqlwarn1: Character;
      sqlwarn2: Character;
      sqlwarn3: Character;
      sqlwarn4: Character;
      sqlwarn5: Character;
      sqlwarn6: Character;
      sqlwarn7: Character;
    end record;

type IISQLCA is 
    record
	sqlcaid: String(1..8);
	sqlcabc: Integer;
	sqlcode: Integer;
	sqlerrm: IISQL_ERRM;
	sqlerrp: String(1..8);
	sqlerrd: IISQL_ERRD;
	sqlwarn: IISQL_WARN;
	sqlext:  String(1..8);
    end record;

sqlca: IISQLCA;
 pragma psect_object(sqlca);		-- Global and external


--
-- LIBQ - SQLCA support routines
--

procedure IIsqUser( usrname: String );
 pragma   interface( C, IIsqUser );
 pragma   import_procedure( IIsqUser,
	    mechanism => (reference) );

procedure IILQsidSessID( session_id: Integer );
 pragma   interface( C, IILQsidSessID );
 pragma   import_procedure( IILQsidSessID,
	    mechanism => (value) );

procedure IILQcnConName( conname: String );
 pragma interface (C, IILQcnConName);
 pragma   import_procedure( IILQcnConName, 
	    mechanism => (reference) );

procedure IIsqConnect( lang: Integer;
		     a1, a2, a3, a4, a5, a6, a7, a8, a9, a10,
		     a11, a12: String := String'null_parameter );
 pragma   interface( C, IIsqConnect );
 pragma   import_procedure( IIsqConnect,
	    mechanism => (value,
			  reference, reference, reference, reference,
			  reference, reference, reference, reference,
			  reference, reference, reference, reference) );

procedure IIsqDisconnect;
 pragma   interface( C, IIsqDisconnect );
 pragma   import_procedure( IIsqDisconnect );

procedure IIsqStop( sq: Address );
 pragma   interface( C, IIsqStop );
 pragma   import_procedure( IIsqStop,
	    mechanism => (value) );

procedure IIsqInit( sq: Address );
 pragma   interface( C, IIsqInit );
 pragma   import_procedure( IIsqInit,
	    mechanism => (value) );

procedure IIsqlcdInit( sq: Address; sqlcd: Address );
 pragma   interface( C, IIsqlcdInit );
 pragma   import_procedure( IIsqlcdInit,
	    mechanism => (value, value) );

procedure IIsqGdInitS( typ: Integer; sq: String );
 pragma   interface( C, IIsqGdInitS );
 pragma   import_procedure( IIsqGdInitS,
	    external => "IIxsqGdInit",
	    mechanism => (value, descriptor(s)) );

procedure IIsqFlush( name: String := String'Null_Parameter;
		     line: Integer := 0 );
 pragma   interface( C, IIsqFlush );
 pragma   import_procedure( IIsqFlush,
	    mechanism => (reference, value) );

procedure IIsqPrint( sq: Address );
 pragma   interface( C, IIsqPrint );
 pragma   import_procedure( IIsqPrint,
	    mechanism => (value) );

procedure IIsqMods( oper: Integer );
 pragma   interface( C, IIsqMods );
 pragma   import_procedure( IIsqMods,
	    mechanism => (value) );

procedure IIsqParms( poper: Integer; pname: String;
		     ptype: Integer; pstr: String; pint: Integer);
 pragma   interface( C, IIsqParms );
 pragma   import_procedure( IIsqParms,
	    mechanism => (value, reference, value, reference, value) );

procedure IIsqTPC( highdxid: Integer; lowdxid: Integer );
 pragma	  interface( C, IIsqTPC );
 pragma	  import_procedure( IIsqTPC,
	    mechanism => (value, value) );

--
-- 6.0 Calls (Dynamic SQL)
--

procedure IIsqExImmed( qry: String );
 pragma	  interface( C, IIsqExImmed );
 pragma   import_procedure( IIsqExImmed,
	    mechanism => (reference) );

procedure IIsqExStmt( name: String; using: Integer );
 pragma	  interface( C, IIsqExStmt );
 pragma   import_procedure( IIsqExStmt,
	    mechanism => (reference, value) );

procedure IIsqDaIn( lang: Integer; sq: Address );
 pragma	  interface( C, IIsqDaIn );
 pragma   import_procedure( IIsqDaIn,
	    mechanism => (value, value) );

procedure IIsqPrepare( lang: Integer; stmt_name: String; sq: Address;
		using: Integer; qry: String );
 pragma	  interface( C, IIsqPrepare );
 pragma   import_procedure( IIsqPrepare,
	    parameter_types => (Integer, String, Address, Integer, String),
	    mechanism => (value, reference, value, value, reference) );

procedure IIsqDescInput( lang: Integer; stmt_name: String; sq: Address );
 pragma	  interface( C, IIsqDescInput );
 pragma   import_procedure( IIsqDescInput,
	    mechanism => (value, reference, value) );

procedure IIsqDescribe( lang: Integer; stmt_name: String; sq: Address;
		using: Integer );
 pragma	  interface( C, IIsqDescribe );
 pragma   import_procedure( IIsqDescribe,
	    mechanism => (value, reference, value, value) );

procedure IIcsDaGet( lang: Integer; sq: Address );
 pragma	  interface( C, IIcsDaGet );
 pragma   import_procedure( IIcsDaGet,
	    mechanism => (value, value) );

end ESQL;
