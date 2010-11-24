--
-- File Name: 	EQSQLDA.ADA
-- Purpose:	ADA package specification file for the SQLDA.   The SQLDA is
--		a record for holding data descriptions, used by Embedded SQL
--		program and INGRES run-time system during execution of Dynamic
--		SQL statements.
--
-- Packages:	ESQLDA 	    - Interface to INGRES/SQLDA.
-- Usage:
--		EXEC SQL INCLUDE SQLDA;
--    generates:
--		with ESQLDA; use ESQLDA;
--
-- History:	04-Jan-1988	- Written. (neil)
--              27-Apr-1990	- Updated IISQ_MAX_COLS to 300. (barbara)
--              16-Oct-1992	- Added new datatype codes and structure
--				  type definition for the DATAHANDLER. (lan)
--		17-sep-1993	- Added new byte datatypes. (sandyd)
--              22-apr-1994     - Added datatype code for W4GL objects. (timt)
--              20-Mar-2002	- Updated IISQ_MAX_COLS to 1024. (toumi01)
--              18-nov-2009     - Add IISQ_BOO_TYPE. (joea)
--              29-Jul-2010     - Increase the size of sqlnamec from 34 to 
--				  258, to match with the IISQD_NAMELEN in  
--				  IISQLDA. (hweho01) S121123
--              17-aug-2010     - Add IISQ spatial types skipped 60 for SECL
--                                consistency. (thic01)
--
-- Copyright (c) 2004, 2010 Ingres Corporation
--

with SYSTEM;

package	ESQLDA is

    use SYSTEM;

    --
    -- IISQ_MAX_COLS - Maximum number of columns returned from INGRES
    --
    IISQ_MAX_COLS: constant := 1024;

    --
    -- Data Type Codes
    --
    IISQ_DTE_TYPE: constant := 3;	-- Date - Output
    IISQ_MNY_TYPE: constant := 5;	-- Money - Output
    IISQ_DEC_TYPE: constant := 10;	-- Decimal - Output
    IISQ_CHA_TYPE: constant := 20;	-- Char - Input, Output
    IISQ_VCH_TYPE: constant := 21;	-- Varchar - Input, Output
    IISQ_INT_TYPE: constant := 30;	-- Integer - Input, Output
    IISQ_FLT_TYPE: constant := 31;	-- Float - Input, Output
    IISQ_BOO_TYPE: constant := 38;	-- Boolean - Input, Output
    IISQ_TBL_TYPE: constant := 52;	-- Table field - Output
    IISQ_DTE_LEN:  constant := 25;	-- Date length
    IISQ_LVCH_TYPE:  constant := 22;	-- Long varchar
    IISQ_LBIT_TYPE:  constant := 16;	-- Long bit
    IISQ_HDLR_TYPE:  constant := 46;	-- Datahandler
    IISQ_BYTE_TYPE:  constant := 23;    -- Byte - Input, Output
    IISQ_VBYTE_TYPE: constant := 24;    -- Byte Varying - Input, Output
    IISQ_LBYTE_TYPE: constant := 25;    -- Long Byte - Output
    IISQ_OBJ_TYPE:  constant := 45;     -- 4GL object - Output
    IISQ_GEOM_TYPE: constant := 56;     -- Spatial types
    IISQ_POINT_TYPE: constant := 57;
    IISQ_MPOINT_TYPE: constant := 58;
    IISQ_LINE_TYPE: constant := 59;
    IISQ_MLINE_TYPE: constant := 61;
    IISQ_POLY_TYPE: constant := 62;
    IISQ_MPOLY_TYPE: constant := 63;
    IISQ_NBR_TYPE: constant := 64;
    IISQ_GEOMC_TYPE: constant := 65;


    --
    -- Address constant so that the inclusion of package SYSTEM is
    -- not required.
    --
    IISQ_ADR_ZERO: constant ADDRESS := ADDRESS_ZERO;

    --
    -- IISQL_NAME - Varying length string
    --
    type IISQL_NAME is
	record
	    sqlnamel: Short_Integer;
	    sqlnamec: String(1..258);
	end record;

    --
    -- IISQL_VAR - Single element of SQLDA variable as described in manual
    --
    type IISQL_VAR is
	record
	    sqltype:	Short_Integer;
	    sqllen:	Short_Integer;
	    sqldata:	Address;		-- Address of any type
	    sqlind:	Address;		-- Address of 2-byte integer
	    sqlname:	IISQL_NAME;
	end record;

    --
    -- IISQL_VARS - Array of IISQL_VAR elements
    --
    type IISQL_VARS is array(Short_Integer range <>) of IISQL_VAR;

    --
    -- IISQLDA - SQLDA with varying number of result variables. Default is
    --		 maximum number (IISQ_MAX_COLS).
    --
    -- 		 Note that the discriminant, sqln, is a 2-byte object that
    --		 is allocated in the record layout at the start of the record.
    --		 This layout is not equivalent to the documented layout, as
    --		 defined by SQL and other embedded languages.  In order to 
    --		 resolve this, an ADA representation clause is issued
    --		 which forces the discriminant, sqln, between the components
    --		 sqldabc and sqld, which is the correct internal record
    --		 layout.
    --
    -- 		 When declaring a static object of your own (ie, not of type
    --		 IISQLDA but with an SQLDA-like layout) you must either have
    --		 a 2-byte discriminant (with a similar representation clause)
    --		 or (if there is no discriminant) declare the sqln record
    --		 component in its correct place.
    --
    type IISQLDA (sqln: Short_Integer := IISQ_MAX_COLS) is
	record
	    sqldaid:	String(1..8);
	    sqldabc:	Integer;
	    sqld:	Short_Integer;
	    -- sqln:	Short_Integer;
	    sqlvar:	IISQL_VARS(1..sqln);
	end record;

    --
    -- IISQHDLR - Structure type with function pointer and function argument
    --		  for the DATAHANDLER.
    --
    type IISQHDLR is
	record
	    sqlarg:	Address;
	    sqlhdlr:	Address;
	end record;

    --
    -- As described above, adjust layout of IISQLDA so that its layout is
    -- identical to the generic record layout, and that of other languages.
    --
    for IISQLDA use
	record
	    sqldaid at 0  range 0..63;	-- Bytes 0..7   = String(1..8);
	    sqldabc at 8  range 0..31;	-- Bytes 8..11  = Integer;
	    sqln    at 12 range 0..15;  -- Bytes 12..13 = Short_Integer;
	    sqld    at 14 range 0..15;  -- Bytes 14..15 = Short_Integer;
	    -- Varying length array, sqlvar, is on end of record
	end record;

end ESQLDA;
