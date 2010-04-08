/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.util;

/*
** Name: DbmsConst.java
**
** Description:
**	Defines constants for EDBC DBMS Servers.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	18-Apr-00 (gordy)
**	    Extracted DBMS constants from EdbcMsgConst.java.
**	10-May-01 (gordy)
**	    Added UCS2 datatypes.
**	20-Feb-01 (gordy)
**	    Added DBMS protocol levels.
*/


/*
** Name: DbmsConst
**
** Description:
**	Interface defining the EDBC DBMS constants.
**
**  Constants:
**
**	DBMS_COL_MAX		Max length of CHAR/VARCHAR column.
**	DBMS_NAME_LEN		Max length of an identifier.
**	DBMS_TBLS_INSEL		Max tables in select.
**	DBMS_TYPE_DATE		Date datatype.
**	DBMS_TYPE_MONEY		Money datatype.
**	DBMS_TYPE_DECIMAL	Decimal datatype.
**	DBMS_TYPE_CHAR		Char datatype.
**	DBMS_TYPE_VARCHAR	Varchar datatype.
**	DBMS_TYPE_LONG_CHAR	Long varchar datatype.
**	DBMS_TYPE_BYTE		Byte datatype.
**	DBMS_TYPE_VARBYTE	Byte varying datatype.
**	DBMS_TYPE_LONG_BYTE	Long byte datatype.
**	DBMS_TYPE_NCHAR		NChar datatype.
**	DBMS_TYPE_NVARCHAR	NVarchar datatype.
**	DBMS_TYPE_LONG_NCHAR	Long nvarchar datatype.
**	DBMS_TYPE_INT		Integer datatype.
**	DBMS_TYPE_FLOAT		Float datatype.
**	DBMS_TYPE_C		'C' datatype.
**	DBMS_TYPE_TEXT		Text datatype.
**	DBMS_TYPE_LONG_TEXT	Long text datatype.
**
**  Public Data:
**
**	typeMap			Mapping table for most DBMS datatypes.
**	intMap			Mapping table for DBMS integer types.
**	floatMap		Mapping table for DBMS float types.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	11-Nov-99 (rajus01)
**	    Implement DatabaseMetaData: Add DBMS_NAME_LEN, DBMS_TBLS_INSEL. 
**	10-May-01 (gordy)
**	    Added UCS2 datatypes: DBMS_TYPE_NCHAR, DBMS_TYPE_NVARCHAR,
**	    DBMS_TYPE_LONG_NCHAR.
**	20-Feb-01 (gordy)
**	    Added DBMS protocol levels: DBMS_PROTO_[0,1,2].
**	29-Jan-04 (fei wei) Bug # 111723; Startrak # EDJDBC94
**	    Added  dataTypeMap.
*/

public interface
DbmsConst
{
    short	DBMS_COL_MAX	    = 2000;	// Max col length.
    short	DBMS_NAME_LEN	    = 32;	// Max Identifier length,
    short	DBMS_TBLS_INSEL	    = 30;	// Max tables in select.

    byte	DBMS_PROTO_0	    = 0;	// Base level.
    byte	DBMS_PROTO_1	    = 1;	// Added decimal, byte, varbyte
    byte	DBMS_PROTO_2	    = 2;	// Added nchar, nvarchar,

    short	DBMS_TYPE_DATE	    = 3;
    short	DBMS_TYPE_MONEY	    = 5;
    short	DBMS_TYPE_DECIMAL   = 10;
    short	DBMS_TYPE_CHAR	    = 20;
    short	DBMS_TYPE_VARCHAR   = 21;
    short	DBMS_TYPE_LONG_CHAR = 22;
    short	DBMS_TYPE_BYTE	    = 23;
    short	DBMS_TYPE_VARBYTE   = 24;
    short	DBMS_TYPE_LONG_BYTE = 25;
    short	DBMS_TYPE_NCHAR	    = 26;
    short	DBMS_TYPE_NVARCHAR  = 27;
    short	DBMS_TYPE_LONG_NCHAR= 28;
    short	DBMS_TYPE_INT	    = 30;
    short	DBMS_TYPE_FLOAT	    = 31;
    short	DBMS_TYPE_C	    = 32;
    short	DBMS_TYPE_TEXT	    = 37;
    short	DBMS_TYPE_LONG_TEXT = 41;

    IdMap	typeMap[] =
    {
	new IdMap( DBMS_TYPE_DATE,	"date" ),
	new IdMap( DBMS_TYPE_MONEY,	"money" ),
	new IdMap( DBMS_TYPE_DECIMAL,	"decimal" ),
	new IdMap( DBMS_TYPE_CHAR,	"char" ),
	new IdMap( DBMS_TYPE_VARCHAR,	"varchar" ),
	new IdMap( DBMS_TYPE_LONG_CHAR,	"long varchar" ),
	new IdMap( DBMS_TYPE_BYTE,	"byte" ),
	new IdMap( DBMS_TYPE_VARBYTE,	"byte varying" ),
	new IdMap( DBMS_TYPE_LONG_BYTE,	"long byte" ),
	new IdMap( DBMS_TYPE_NCHAR,	"nchar" ),
	new IdMap( DBMS_TYPE_NVARCHAR,	"nvarchar" ),
	new IdMap( DBMS_TYPE_LONG_NCHAR,"long nvarchar" ),
	new IdMap( DBMS_TYPE_C,		"c" ),
	new IdMap( DBMS_TYPE_TEXT,	"text" ),
	new IdMap( DBMS_TYPE_LONG_TEXT,	"long text" ),
    };

    IdMap	intMap[] =
    {
	new IdMap( 1,	"integer1" ),
	new IdMap( 2,	"smallint" ),
	new IdMap( 4,	"integer" ),
    };

    IdMap	floatMap[] =
    {
	new IdMap( 4,	"float4" ),
	new IdMap( 8,	"float" ),
    };

    IdMap   dataTypeMap[] =
    {
         new IdMap( 30,    "integer"),
         new IdMap( 30,    "smallint"),
         new IdMap( 30,    "int"),
         new IdMap( 31,    "float"),
         new IdMap( 31,    "real"),
         new IdMap( 31,    "double precision"),
         new IdMap( 31,    "double p"),
         new IdMap( 20,    "char"),
         new IdMap( 20,    "character"),
         new IdMap( 21,    "varchar"),
         new IdMap( 22,    "long char"),
         new IdMap( 23,    "byte"),
         new IdMap( 25,    "long byte"),
         new IdMap( 32,    "c"),
         new IdMap( 37,    "text"),
         new IdMap( 3,     "date"),
         new IdMap( 5,     "money"),
    };

} // interface DbmsConst
