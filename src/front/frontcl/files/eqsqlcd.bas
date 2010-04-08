	!
	!  Copyright (c) 2004 Ingres Corporation
	!

	! **************** esqlca definition ***************************

	!
	!  SQLCA - Structure to hold the error and status information returned 
	!	   by INGRES runtime routines
	!
	    common (sqlca)	string	sqlcaid = 8,			&
				long	sqlcabc,			&
				long	iisqlcd,			&
				word	sqlerrml,			&
				string	sqlerrmc = 70,			&
				string	sqlerrp = 8,			&
				long	sqlerrd(5),			&
				string	sqlwarn0 = 1,			&
				string	sqlwarn1 = 1,			&
				string	sqlwarn2 = 1,			&
				string	sqlwarn3 = 1,			&
				string	sqlwarn4 = 1,			&
				string	sqlwarn5 = 1,			&
				string	sqlwarn6 = 1,			&
				string	sqlwarn7 = 1,			&
				string	sqlext = 8

	!
	!  Variable to map the "sqlca" common area to an "sqlca" struct.
	!
		declare long sqlcax
		sqlcax = loc(sqlcaid)

	!
	!  ESQL (Libq) subroutines
	!
	   external sub IIcsDaGet, IIsqConnect, IIsqDisconnect, IIsqFlush
	   external sub IIsqInit, IIsqStop, IIsqUser, IIsqPrint, IIsqExImmed
	   external sub IIsqExStmt, IIsqDaIn, IIsqPrepare, IIsqDescribe
	   external sub IIsqMods, IIsqParms, IIsqTPC, IILQsidSessID
	   external sub IIsqlcdInit, IIsqGdInit, IILQcnConName
