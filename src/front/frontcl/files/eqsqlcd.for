C
C  Copyright (c) 2004 Ingres Corporation
C

C ************************ esqlca definition ***************************

C
C  SQLCA - Structure to hold the error and status information returned 
C	   by INGRES runtime routines
C
	character*8    sqlcai
	integer*4      sqlcab
	integer*4      iisqcd
	integer*2      sqltxl
	character*70   sqltxt
	character*8    sqlerp
	integer*4      sqlerr(6)
	character*1    sqlwrn(0:7)
	character*8    sqlext

	common /sqlca/ sqlcai, sqlcab, iisqcd, sqltxl, sqltxt,
	1              sqlerp, sqlerr, sqlwrn, sqlext

C
C  Variable to map the "sqlca" common area to an "sqlca" structure.
C
	integer*4	sqlcax
	equivalence	(sqlcax, sqlcai)
