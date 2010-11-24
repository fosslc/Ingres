{
| Copyright (c) 2004, 2010 Ingres Corporation
}

{
|  Name: eqsqlda.pas - Defines the SQLDA
|
|  Description:
| 	The SQLDA is a record for holding data descriptions, used by 
|	embedded program and INGRES run-time system during execution of
|  	Dynamic SQL statements.
|
|  Defines:
|	IISQLDA		- SQLDA type definition that programs use.
|	Constants and type codes required for using the SQLDA.
|  History:
|	20-oct-1987	- Written (neil)
|       27-apr-1990	- Updated IISQ_MAX_COLS to 300 (barbara)
|       16-Oct-1992	- Added new datatype codes and structure
|			  type definition for the DATAHANDLER. (lan) 
|       17-sep-1993	- Added new byte datatype codes. (sandyd)
|       07-jan-2002	- Updated IISQ_MAX_COLS to 1024 (toumi01)
|       18-nov-2009     - Add IISQ_BOO_TYPE (joea)
|       29-jul-2010     - Increase the size of sqlname from 34 to  
|			  258, match with IISQD_NAMELEN in IISQLDA 
|			  ( iisqlda.h). (hweho01) SIR 121123 
|       17-aug-2010     - Add IISQ spatial types skipped 60 for SECL 
|                         consistency(thich01)
}

{
| IISQ_MAX_COLS - Maximum number of columns returned from INGRES
}
    const
	IISQ_MAX_COLS = 1024;

{
| IIsqlvar - Single element of SQLDA variable as described in manual.
}
    type
	II_int2 = [word] -32768 .. 32767;

    type
	IIsqlvar = record
	    sqltype:	II_int2;
	    sqllen:	II_int2;
	    sqldata:	Integer;		{ Address of any type }
	    sqlind:	Integer;		{ Address of 2-byte integer }
	    sqlname:	Varying[258] of Char;
	end;

{
| IIsqlda - SQLDA with maximum number of entries for variables.
}
    type
	IIsqlda = record
	    sqldaid:	packed array[1..8] of Char;
	    sqldabc:	Integer;
	    sqln:	II_int2;
	    sqld:	II_int2;
	    sqlvar:	array[1..IISQ_MAX_COLS] of IIsqlvar;
	end;

{
| IIsqlhdlr - Structure type with function pointer and function argument
|	      for the DATAHANDLER.
}
    type
	IIsqlhdlr = record
	    sqlarg:	[volatile] Integer;
	    sqlhdlr:	[volatile] Integer;
	end;
{
| Allocation sizes - When allocating an SQLDA using raw allocation, use:
|		IISQDA_HEAD_SIZE + (N * IISQDA_VAR_SIZE)
}
    const
	IISQDA_HEAD_SIZE = 16;
	IISQDA_VAR_SIZE  = 48;

{
| Data Type Codes
}
    const
	IISQ_DTE_TYPE = 3;	{ Date - Output }
	IISQ_MNY_TYPE = 5;	{ Money - Output }
	IISQ_DEC_TYPE = 10;	{ Decimal - Output }
	IISQ_CHA_TYPE = 20;	{ Char - Input, Output }
	IISQ_VCH_TYPE = 21;	{ Varchar - Input, Output }
	IISQ_INT_TYPE = 30;	{ Integer - Input, Output }
	IISQ_FLT_TYPE = 31;	{ Float - Input, Output }
        IISQ_BOO_TYPE = 38;     { Boolean - Input, Output }
	IISQ_TBL_TYPE = 52;	{ Table field - Output }
	IISQ_DTE_LEN  = 25;	{ Date length }
	IISQ_LVCH_TYPE = 22;	{ Long varchar }
	IISQ_LBIT_TYPE = 16;	{ Long bit }
	IISQ_HDLR_TYPE = 46;	{ Datahandler }
	IISQ_BYTE_TYPE = 23;	{ Byte - Input, Output }
	IISQ_VBYTE_TYPE = 24;	{ Byte Varying - Input, Output }
	IISQ_LBYTE_TYPE = 25;	{ Long Byte - Output }
	IISQ_GEOM_TYPE = 56;    { Spatial types }
	IISQ_POINT_TYPE = 57;
	IISQ_MPOINT_TYPE = 58;
	IISQ_LINE_TYPE = 59;
	IISQ_MLINE_TYPE = 61;
	IISQ_POLY_TYPE = 62;
	IISQ_MPOLY_TYPE = 63;
	IISQ_NBR_TYPE = 64;
	IISQ_GEOMC_TYPE = 65;
