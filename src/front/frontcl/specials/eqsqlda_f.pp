C
C Copyright (c) 2005 Ingres Corporation
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
C        04-sep-1990 (kathryn) - Written.
C        15-aug-1991 (hhsiung) - rework IISQLVAR structure to the format
C                                that accept by not only Sun and VAX but
C                                also Bull.
C	 16-Oct-1992 (lan)     - Added new datatype codes and structure
C				 type definition for the DATAHANDLER. (lan)
C	 17-sep-1993 (sandyd)  - Added new byte datatype codes.
C        22-apr-1994 (timt)    - Added datatype code for W4GL objects.
C        10-feb-2004 (toumi01) - IISQ_MAX_COLS increased to 1024.
C	 01-Nov-2005 (hanje04) - BUGS 113212, 114839 & 115285.
C				 Use INTEGER*8 for sqldata & sqlind on 64bit
C				 Linux.
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
                integer*2         sqllen
#if defined(i64_hpu) || defined (axp_osf) || (defined(LNX) && defined(LP64))
                integer*8        sqldata        
                integer*8        sqlind        
#else
                integer*4        sqldata        
                integer*4        sqlind        
#endif
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
