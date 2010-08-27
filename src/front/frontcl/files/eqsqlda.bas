!
! Copyright (c) 2004, 2010 Ingres Corporation
!

!
!  Name: eqsqlda.bas - Defines the SQLDA
!
!  Description:
! 	The SQLDA is a structure for holding data descriptions, used by 
!	embedded program and INGRES run-time system during execution of
!  	Dynamic SQL statements.
!
!  Defines:
!	IISQLDA		- SQLDA type definition that programs use.
!	Constants and type codes required for using the SQLDA.
!
!  History:
!	29-dec-1987	- Written (neil)
!	27-apr-1990	- Updated IISQ_MAX_COLS to 300 (barbara)
!	16-Oct-1992	- Added new datatype codes and structure
!			  type definition for the DATAHANDLER. (lan)
!	17-sep-1993	- Added new byte datatypes. (sandyd)
!	19-apr-1999 (hanch04)
!	    Replace long with int
!	07-jan-2002	- Updated IISQ_MAX_COLS to 1024 (toumi01)
!	12-feb-2003 (abbjo03)
!	    Undo 19-apr-1999 change since int is not a BASIC datatype.
!       18-nov-2009 (joea)
!           Add IISQ_BOO_TYPE.
!       29-jul-2010 (hweho01) SIR 121123
!           Increased the size of sqlnamec from 34 to 258, to match 
!           with the IISQD_NAMELEN in IISQLDA (iisqlda.h).

!
! IISQ_MAX_COLS - Maximum number of columns returned from INGRES
!
	declare word constant IISQ_MAX_COLS = 1024

!
! IISQLDA - SQLDA with maximum number of entries for variables.
!
	record IISQLDA

	    string	sqldaid = 8
	    long	sqldabc
	    word	sqln
	    word	sqld

	    group sqlvar(IISQ_MAX_COLS)

		word	sqltype
		word 	sqllen
		long	sqldata		! Address of any type
		long	sqlind		! Address of 2-byte integer

		group sqlname
		    word	sqlnamel
		    string  	sqlnamec = 258
		end group sqlname
	    
	    end group sqlvar

	end record IISQLDA

!
! IISQLHDLR - Structure type with function pointer and function argument
!	      for the DATAHANDLER.
!
	record IISQLHDLR
	    long    sqlarg
	    long    sqlhdlr
	end record IISQLHDLR


!
! Allocation sizes - When allocating an SQLDA for N results use:
!		IISQDA_HEAD_SIZE + (N * IISQDA_VAR_SIZE)
!
	declare integer constant IISQDA_HEAD_SIZE = 16
	declare integer constant IISQDA_VAR_SIZE  = 48

!
! Type Codes
!
	declare integer constant IISQ_DTE_TYPE = 3	! Date - Out
	declare integer constant IISQ_MNY_TYPE = 5	! Money - Out
	declare integer constant IISQ_DEC_TYPE = 10	! Decimal - In/Out
	declare integer constant IISQ_CHA_TYPE = 20	! Char - In/Out
	declare integer constant IISQ_VCH_TYPE = 21	! Varchar - In/Out
	declare integer constant IISQ_INT_TYPE = 30	! Integer - In/Out
	declare integer constant IISQ_FLT_TYPE = 31	! Float - In/Out
	declare integer constant IISQ_BOO_TYPE = 38	! Boolean - In/Out
	declare integer constant IISQ_TBL_TYPE = 52	! Table field - Out
	declare integer constant IISQ_DTE_LEN  = 25	! Date length
	declare integer constant IISQ_LVCH_TYPE = 22	! Long varchar
	declare integer constant IISQ_LBIT_TYPE = 16	! Long bit
	declare integer constant IISQ_HDLR_TYPE = 46	! Datahandler
	declare integer constant IISQ_BYTE_TYPE = 23	! Byte - In/Out
	declare integer constant IISQ_VBYTE_TYPE = 24	! Byte Varying - In/Out
	declare integer constant IISQ_LBYTE_TYPE = 25	! Long Byte - Out
