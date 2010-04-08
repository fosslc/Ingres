!
! Copyright (c) 1984, Ingres Corporation
!
! Defines:
!	iicl__inredirect
!	iicl__outredirect
!	iicl__sysin
!	iicl__sysout
!	iicl__syserr
!	iicl__commandline
!	iicl_feerror
!	iicl_fertnerror
!
!	History:
!		22-jun-1987 (Joe)
!			Built error message files for errors during
!			FE startup.
!			
!		12/90 (Mike S)
!			Added messages for frontstart's top-level exception
!			handler to use on fatal errors.
!
.facility	iicl,2000/prefix=iicl__
.ident		'Version 6.0'
.severity	error
.base		10	! 
inredirect	<can't open file for input redirection.>
outredirect	<can't open file for output redirection.>
sysin		<can't open SYS$INPUT.>
sysout		<can't open SYS$OUTPUT.>
syserr		<can't open SYS$ERROR.>
commandline	<can't open mailbox to get extra command line arguments.>
feerror		<fatal INGRES front-end error '!XL'>
fertnerror	<fatal INGRES front-end error '!XL' in '!AS'>
