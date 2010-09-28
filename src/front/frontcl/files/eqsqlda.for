C
C Copyright (c) 2004, 2010 Ingres Corporation
C

C
C  Name: eqsqlda.for - Defines the SQLDA
C
C  Description:
C 	The SQLDA is a structure for holding data descriptions, used by 
C	embedded program and INGRES run-time system during execution of
C  	Dynamic SQL statements.
C
C  Defines:
C	IISQLDA		- SQLDA type definition that programs use.
C	Constants and type codes required for using the SQLDA.
C
C  History:
C	20-oct-1987	- Written (neil)
C       27-apr-1990	- Updated IISQ_MAX_COLS to 300 (barbara)
C       16-Oct-1992	- Added new datatype codes and structure
C			  type definition for the DATAHANDLER. (lan)
C       17-sep-1993     - Added new byte datatype codes. (sandyd)
C       22-apr-1994     - Added datatype code for W4GL objects. (timt)
C       07-jan-2002	- Updated IISQ_MAX_COLS to 1024 (toumi01)
C       18-nov-2009     - Add IISQ_BOO_TYPE (joea)
C       29-jul-2010     - Increased the size of sqlnamec from 34 to 
C			  258, to match with the IISQD_NAMELEN in  
C			  IISQLDA (iisqlda.h). (hweho01) SIR 121123 
C       17-aug-2010     - Add spatial IISQ types skipped 60 for SECL 
C                         consistency. (thich01)
c

C
C IISQLVAR - Single element of SQLDA variable as described in manual.
C
	structure /IISQLVAR/
		integer*2	sqltype
		integer*2 	sqllen
		integer*4	sqldata		! Address of any type
		integer*4	sqlind		! Address of 2-byte integer
		structure /IISQLNAME/ sqlname
		    integer*2 	  sqlnamel
		    character*258 sqlnamec
		end structure
	end structure

C
C IISQ_MAX_COLS - Maximum number of columns returned from INGRES
C
	parameter IISQ_MAX_COLS = 1024

C
C IISQLDA - SQLDA with maximum number of entries for variables.
C
	structure /IISQLDA/
	    character*8		sqldaid
	    integer*4		sqldabc
	    integer*2		sqln
	    integer*2		sqld
	    record /IISQLVAR/ 	sqlvar(IISQ_MAX_COLS)
	end structure

C
C IISQLHDLR - Structure type with function pointer and function argument
C	      for the DATAHANDLER.
C
	structure /IISQLHDLR/
	    integer*4		sqlarg
	    integer*4		sqlhdlr
	end structure


C
C Allocation sizes - When allocating an SQLDA for the size use:
C		IISQDA_HEAD_SIZE + (N * IISQDA_VAR_SIZE)
C
	parameter IISQDA_HEAD_SIZE = 16,
	1	  IISQDA_VAR_SIZE  = 48

C
C Type and Length Codes
C
	parameter IISQ_DTE_TYPE = 3,	! Date - Output
	1	  IISQ_MNY_TYPE = 5,	! Money - Output
	2	  IISQ_DEC_TYPE = 10,	! Decimal - Output
	3	  IISQ_CHA_TYPE	= 20,	! Char - Input, Output
	4	  IISQ_VCH_TYPE	= 21,	! Varchar - Input, Output
	5	  IISQ_INT_TYPE	= 30,	! Integer - Input, Output
	6	  IISQ_FLT_TYPE = 31,	! Float - Input, Output
	7	  IISQ_BOO_TYPE = 38,	! Boolean - Input, Output
	8	  IISQ_TBL_TYPE = 52,	! Table field - Output
	9	  IISQ_DTE_LEN  = 25	! Date length
	parameter IISQ_LVCH_TYPE = 22	! Long varchar
	parameter IISQ_LBIT_TYPE = 16	! Long bit
	parameter IISQ_HDLR_TYPE = 46	! Datahandler
	parameter IISQ_BYTE_TYPE = 23	! Byte - Input, Output
	parameter IISQ_VBYTE_TYPE = 24	! Byte Varying - Input, Output
	parameter IISQ_LBYTE_TYPE = 25	! Long Byte - Output
        parameter IISQ_OBJ_TYPE = 45    ! Object - Output
        parameter IISQ_GEOM_TYPE = 56;  ! Spatial
        parameter IISQ_POINT_TYPE = 57;
        parameter IISQ_MPOINT_TYPE = 58;
        parameter IISQ_LINE_TYPE = 59;
        parameter IISQ_MLINE_TYPE = 61;
        parameter IISQ_POLY_TYPE = 62;
        parameter IISQ_MPOLY_TYPE = 63;
        parameter IISQ_NBR_TYPE = 64;
        parameter IISQ_GEOMC_TYPE = 65;

