{
| File Name: 	EQSQLCA.PAS
| Purpose:	PASCAL package specification file for Embedded SQL record
|		definition and run-time INGRES/SQL  subprogram interface.
|
| Usage:
|		EXEC SQL INCLUDE SQLCA;
|    generates:
|		%include 'sys_ingres:[ingres.files]esqlca.pas/nolist'
|
| History:	20-Jun-1986	- Written. (mrw)
|		12-May-1987	- Modified for 6.0. (ncg)
|		17-dec-1992 (larrym)
|		    Added IIsqlcdInit and IIsqGdInit
|		6-Jun-2006 (kschendel)
|		    Add IIsqDescInput
|
| Example Interface Format:
|		procedure IIname ( %immed arg1 : [unsafe] Integer;
|				    %ref arg2: String );
|
| Copyright (c) 2004 Ingres Corporation
} 

{Define the program SQL error structure.}

type
    IISQLCA = record
	sqlcaid: packed array[1..8] of Char;
	sqlcabc: Integer;
	sqlcode: Integer;
	sqlerrm: varying[70] of Char;
	sqlerrp: packed array[1..8] of Char;
	sqlerrd: array[1..6] of Integer;
	sqlwarn: record
	    sqlwarn0: Char;
	    sqlwarn1: Char;
	    sqlwarn2: Char;
	    sqlwarn3: Char;
	    sqlwarn4: Char;
	    sqlwarn5: Char;
	    sqlwarn6: Char;
	    sqlwarn7: Char;
	end;
	sqlext:  packed array[1..8] of Char;
    end;

var
    sqlca: [common] IISQLCA;	{sqlca shared with others at run-time}


{LIBQ - General SQLCA routines}

procedure IIsqUser( %immed usrname: [unsafe] Integer ); extern;

procedure IILQsidSessID( %immed session_id: Integer ); extern;

procedure IILQcnConName( %immed conname: [unsafe] Integer ); extern;

procedure IIsqConnect( %immed lang: Integer;
		       %immed a1: [unsafe] Integer := %immed 0;
		       %immed a2: [unsafe] Integer := %immed 0;
		       %immed a3: [unsafe] Integer := %immed 0;
		       %immed a4: [unsafe] Integer := %immed 0;
		       %immed a5: [unsafe] Integer := %immed 0;
		       %immed a6: [unsafe] Integer := %immed 0;
		       %immed a7: [unsafe] Integer := %immed 0;
		       %immed a8: [unsafe] Integer := %immed 0;
		       %immed a9: [unsafe] Integer := %immed 0;
		       %immed a10: [unsafe] Integer := %immed 0;
		       %immed a11: [unsafe] Integer := %immed 0;
		       %immed a12: [unsafe] Integer := %immed 0;
		       %immed a13: [unsafe] Integer := %immed 0;
		       %immed a14: [unsafe] Integer := %immed 0 ); extern;

procedure IIsqDisconnect; extern;

procedure IIsqFlush( %immed fname: [unsafe] Integer;
		     %immed lnum: Integer); extern;

procedure IIsqMods( %immed oper: Integer ); extern;

procedure IIsqParms( %immed poper: Integer;
		     %immed pname: [unsafe] Integer;
		     %immed ptype: Integer;
		     %immed pstr: [unsafe] Integer;
		     %immed pint: Integer ); extern;

procedure IIsqTPC( %immed highdxid: Integer;
		   %immed lowdxid: Integer ); extern;

{LIBQ - SQLCA support routines}

procedure IIsqInit( %immed sqc: [unsafe] Integer ); extern;

procedure IIsqlcdInit(	%immed sqc: [unsafe] Integer;
			%immed sqlcd: [unsafe] Integer ); extern;

procedure IIsqGdInit(	%immed ptype : Integer;
			%immed sqlst: [unsafe] Integer ); extern;

procedure IIsqStop( %immed sqc: [unsafe] Integer ); extern;

procedure IIsqPrint( %immed sqc: [unsafe] Integer ); extern;


{LIBQ - Dynamic SQL and SQLDA support routines}

procedure IIsqExImmed( %immed qry: [unsafe] Integer ); extern;

procedure IIsqExStmt( %immed stmt: [unsafe] Integer;
		      %immed usevars: Integer ); extern;

procedure IIsqDaIn( %immed lang: Integer;
		    %immed sqd: [unsafe] Integer ); extern;

procedure IIsqPrepare( %immed lang: Integer;
		       %immed stmt: [unsafe] Integer;
		       %immed sqd: [unsafe] Integer;
		       %immed usenams: Integer;
		       %immed qry: [unsafe] Integer ); extern;

procedure IIsqDescInput( %immed lang: Integer;
		        %immed stmt: [unsafe] Integer;
		        %immed sqd: [unsafe] Integer ); extern;

procedure IIsqDescribe( %immed lang: Integer;
		        %immed stmt: [unsafe] Integer;
		        %immed sqd: [unsafe] Integer;
		        %immed usenams: Integer ); extern;

procedure IIcsDaGet( %immed lang: Integer;
		     %immed sqd: [unsafe] Integer ); extern;
