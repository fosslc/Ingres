C
C Copyright (c) 2008 Ingres Corporation
C

C
C  Name: eqsqlda.for - Defines the SQLDA
C
C  Description:
C         The SQLDA is a structure for holding data descriptions, used by 
C        embedded program and INGRES run-time system during execution of
C          Dynamic SQL statements.
C
C  Defines:
C        IISQLDA - SQLDA type definition that programs use.
C        Constants and type codes required for using the SQLDA.
C
C  History:
C        20-Jun-2008 (hweho01) -Added for 64-bit Fortran support on
C                               Unix hybrid platforms. The file is made  
C                               from eqsqlda_f.pp rev. 14.   
C        18-nov-2009 (joea)    - Add IISQ_BOO_TYPE.
C

C
C IISQLVAR - Single element of SQLDA variable as described in manual.
C
        structure /IISQLNAME/ 
                integer*2           sqlnamel
                 character*34  sqlnamec
        end structure
        structure /IISQLVAR/
                integer*2        sqltype
                integer*2        sqllen
                integer*8        sqldata        
                integer*8        sqlind        
                record /IISQLNAME/       sqlname
        end structure

C
C IISQ_MAX_COLS - Maximum number of columns returned from INGRES
C
        parameter (IISQ_MAX_COLS = 1024)

C
C IISQLDA - SQLDA with maximum number of entries for variables.
C
        structure /IISQLDA/
            character*8                sqldaid
            integer*4                sqldabc
            integer*2                sqln
            integer*2                sqld
            record /IISQLVAR/         sqlvar(IISQ_MAX_COLS)
        end structure

C
C IISQLHDLR - Structure type with function pointer and function argument
C	      for the DATAHANDLER.
C
        structure /IISQLHDLR/
            integer*4                sqlarg
            integer*4                sqlhdlr
        end structure


C
C Allocation sizes - When allocating an SQLDA for the size use:
C                IISQDA_HEAD_SIZE + (N * IISQDA_VAR_SIZE)
C
        parameter (IISQDA_HEAD_SIZE = 16,
     1          IISQDA_VAR_SIZE  = 48)

C
C Type and Length Codes
C
        parameter (IISQ_DTE_TYPE = 3,
     1          IISQ_MNY_TYPE = 5,
     2          IISQ_DEC_TYPE = 10,
     3          IISQ_CHA_TYPE = 20,
     4          IISQ_VCH_TYPE = 21,
     5          IISQ_INT_TYPE = 30,
     6          IISQ_FLT_TYPE = 31,
     7          IISQ_BOO_TYPE = 38,
     8          IISQ_TBL_TYPE = 52,
     9          IISQ_DTE_LEN  = 25)
	parameter (IISQ_LVCH_TYPE = 22)
	parameter (IISQ_LBIT_TYPE = 16)
	parameter (IISQ_HDLR_TYPE = 46)
	parameter (IISQ_BYTE_TYPE = 23)
	parameter (IISQ_VBYTE_TYPE = 24)
	parameter (IISQ_LBYTE_TYPE = 25)
	parameter (IISQ_OBJ_TYPE = 45)
