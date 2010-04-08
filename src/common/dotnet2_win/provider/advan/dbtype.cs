/*
** Copyright (c) 2002, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;

	/*
	** Name: dbtype.cs
	**
	** Description:
	**	Defines the data types for Ingres providers.
	**
	**
	** History:
	**	26-Aug-02 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	12-Oct-06 (thoda04)
	**	    Added Date, Time, Interval.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	**	15-Sep-08 (thoda04)
	**	    Added max decimal precision as returned by DBMS.
	**	02-Nov-08 (thoda04)
	**	    "INGRES DATE" and "ANSI DATE" should be "INGRESDATE" and "ANSIDATE".
	**	22-Jun-09 (thoda04) Bug 122228 fix
	**	    In GetDbType(), for ProviderType.Date and ProviderType.Time,
	**	        return DbType.Date and DbType.Time.
	**	    In GetProviderType(), for DbType.Date and DbType.Time,
	**	        return ProviderType.Date and ProviderType.Time.
	**	 7-Dec-09 (thoda04)
	**	    Added support for BOOLEAN data type.
	**	 3-Mar-10 (thoda04)  SIR 123368
	**	    Added support for IngresType.IngresDate parameter data type.
	*/
	
	

namespace Ingres.Utility
{
	/*
	** Description:
	**	Defines the provider independent data types and utility methods.
	**
	** History:
	**	14-Nov-02 (thoda04)
	**	    Created.
	**	12-Oct-06 (thoda04)
	**	    Added Date, Time, Interval.
	**	21-Mar-07 (thoda04)
	**	    Added IntervalDayToSecond, IntervalYearToMonth.
	*/


	internal enum ProviderType
	{
		                          // codes are same as ODBC
		Binary           =  (-2),
		Boolean          =  16,
		VarBinary        =  (-3),
		LongVarBinary    =  (-4),
		Char             =    1,
		VarChar          =   12,
		LongVarChar      =  (-1),
		TinyInt          =  (-6),
		BigInt           =  (-5),
		Decimal          =    3,
		Numeric          =    2,  // always treated as decimal
		SmallInt         =    5,
		Integer          =    4,
		Real             =    7,  // synonym of Single
		Single           =    7,  // synonym of Real
		Double           =    8,
		DateTime         =   93,  // synonym of Timestamp
		IngresDate       = 1093,
		NChar            = (-95),
		NVarChar         = (-96),
		LongNVarChar     = (-97),
		Other            = 1111,
		DBNull           =    0,
		Date             =   91,
		Time             =   92,
		Interval         =   10,  // Interval (generic)
		IntervalYearToMonth= 107, // Interval Year To Month
		IntervalDayToSecond= 110, // Interval Day To Second
	} // ProviderType

	internal class ProviderTypeMgr
	{
		internal static string GetDataTypeName(ProviderType i)
		{
			return GetDataTypeName(i, true);
		}

		internal static string GetDataTypeName(
			ProviderType i, bool isANSIDateTimeSupported)
		{
			switch(i)
			{
				case ProviderType.BigInt:
					return IdMap.get(8, DbmsConst.intMap);   // integer8
				case ProviderType.Integer:
					return IdMap.get(4, DbmsConst.intMap);   // integer
				case ProviderType.SmallInt:
					return IdMap.get(2, DbmsConst.intMap);   // smallint
				case ProviderType.TinyInt:
					return IdMap.get(1, DbmsConst.intMap);   // tinyint

				case ProviderType.Double:
					return IdMap.get(8, DbmsConst.floatMap); // float8
				case ProviderType.Real:
					return IdMap.get(4, DbmsConst.floatMap); // real

				case ProviderType.Binary:
					return IdMap.get(DbmsConst.DBMS_TYPE_BYTE,
						DbmsConst.typeMap);                  // byte
				case ProviderType.Char:
					return IdMap.get(DbmsConst.DBMS_TYPE_CHAR,
						DbmsConst.typeMap);                  // char
				case ProviderType.DateTime:
					if (isANSIDateTimeSupported)
						return IdMap.get(DbmsConst.DBMS_TYPE_TSWO,
							DbmsConst.typeMap);              // date (TS w/o TZ)
					else
						return IdMap.get(DbmsConst.DBMS_TYPE_IDATE,
							DbmsConst.typeMap);              // date
				case ProviderType.IngresDate:
					if (isANSIDateTimeSupported)
						return IdMap.get(DbmsConst.DBMS_TYPE_INGRESDATE,
							DbmsConst.typeMap);              // ingresdate
					else
						return IdMap.get(DbmsConst.DBMS_TYPE_IDATE,
							DbmsConst.typeMap);              // date
				case ProviderType.Decimal:
					return IdMap.get(DbmsConst.DBMS_TYPE_DECIMAL,
						DbmsConst.typeMap);                  // decimal
				case ProviderType.LongNVarChar:
					return IdMap.get(DbmsConst.DBMS_TYPE_LONG_NCHAR,
						DbmsConst.typeMap);                  // long nvarchar
				case ProviderType.LongVarBinary:
					return IdMap.get(DbmsConst.DBMS_TYPE_LONG_BYTE,
						DbmsConst.typeMap);                  // long byte
				case ProviderType.LongVarChar:
					return IdMap.get(DbmsConst.DBMS_TYPE_LONG_CHAR,
						DbmsConst.typeMap);                  // long varchar
				case ProviderType.NChar:
					return IdMap.get(DbmsConst.DBMS_TYPE_NCHAR,
						DbmsConst.typeMap);                  // nchar
				case ProviderType.NVarChar:
					return IdMap.get(DbmsConst.DBMS_TYPE_NVARCHAR,
						DbmsConst.typeMap);                  // nvarchar
				case ProviderType.VarBinary:
					return IdMap.get(DbmsConst.DBMS_TYPE_VARBYTE,
						DbmsConst.typeMap);                  // byte varying
				case ProviderType.VarChar:
					return IdMap.get(DbmsConst.DBMS_TYPE_VARCHAR,
						DbmsConst.typeMap);                  // varchar
				case ProviderType.Date:
					return IdMap.get(DbmsConst.DBMS_TYPE_ADATE,
						DbmsConst.typeMap);                  // ansi date
				case ProviderType.Time:
					return IdMap.get(DbmsConst.DBMS_TYPE_TMWO,
						DbmsConst.typeMap);                  // ansi time w/o TZ
				case ProviderType.Interval:
					return IdMap.get(DbmsConst.DBMS_TYPE_INTDS,
						DbmsConst.typeMap);                  // generic interval
				case ProviderType.IntervalDayToSecond:
					return IdMap.get(DbmsConst.DBMS_TYPE_INTDS,
						DbmsConst.typeMap);                  // interval d_s
				case ProviderType.IntervalYearToMonth:
					return IdMap.get(DbmsConst.DBMS_TYPE_INTYM,
						DbmsConst.typeMap);                  // interval y_m

				case ProviderType.Boolean:
					return IdMap.get(DbmsConst.DBMS_TYPE_BOOL,
						DbmsConst.typeMap);                  // boolean

				default:
					return "unknownDbType" + i.ToString();
			}
		}  // GetDataTypeName


		internal static Type GetNETType(ProviderType i)
		{
			switch(i)
			{
				case ProviderType.BigInt:
					return typeof(System.Int64);          // bigint
				case ProviderType.Integer:
					return typeof(System.Int32);          // integer
				case ProviderType.SmallInt:
					return typeof(System.Int16);          // smallint
				case ProviderType.TinyInt:
					return typeof(System.SByte);          // tinyint

				case ProviderType.Double:
					return typeof(System.Double);         // float8
				case ProviderType.Real:
					return typeof(System.Single);         // real

				case ProviderType.Binary:
					return  typeof(System.Byte[]);        // byte
				case ProviderType.Char:
					return  typeof(String);               // char
				case ProviderType.DateTime:
					return  typeof(System.DateTime);      // date
				case ProviderType.IngresDate:
					return typeof(System.DateTime);       // ingresdate
				case ProviderType.Decimal:
					return  typeof(System.Decimal);       // decimal
				case ProviderType.LongNVarChar:
					return  typeof(String);        // long nvarchar
				case ProviderType.LongVarBinary:
					return  typeof(System.Byte[]);        // long byte
				case ProviderType.LongVarChar:
					return  typeof(String);        // long varchar
				case ProviderType.NChar:
					return  typeof(String);        // nchar
				case ProviderType.NVarChar:
					return  typeof(String);        // nvarchar
				case ProviderType.VarBinary:
					return  typeof(System.Byte[]);        // byte varying
				case ProviderType.VarChar:
					return  typeof(String);        // varchar
				case ProviderType.Date:
					return typeof(System.DateTime); // ANSI date
				case ProviderType.Time:
					return typeof(System.String);   // ANSI time
				case ProviderType.Interval:
				case ProviderType.IntervalYearToMonth:
					return typeof(System.String);   // ANSI interval YM
				case ProviderType.IntervalDayToSecond:
					return typeof(System.TimeSpan); // ANSI interval DS
				case ProviderType.Boolean:
					return typeof(System.Boolean);  // boolean
				default:
					return typeof(System.Object);
			}
		}

		public static System.Data.DbType GetDbType(ProviderType i)
		{
			switch(i)
			{
				case ProviderType.BigInt:
					return  System.Data.DbType.Int64;         // bigint
				case ProviderType.Integer:
					return  System.Data.DbType.Int32;         // integer
				case ProviderType.SmallInt:
					return  System.Data.DbType.Int16;         // smallint
				case ProviderType.TinyInt:
					return  System.Data.DbType.SByte;         // tinyint

				case ProviderType.Double:
					return  System.Data.DbType.Double;        // float8
				case ProviderType.Real:
					return  System.Data.DbType.Single;        // real

				case ProviderType.Binary:
					return  System.Data.DbType.Binary;        // byte
				case ProviderType.Char:
					return  System.Data.DbType.String;        // char

				case ProviderType.Date:
					return  System.Data.DbType.Date;          // date
				case ProviderType.Time:
					return  System.Data.DbType.Time;          // time
				case ProviderType.DateTime:
					return  System.Data.DbType.DateTime;      // datetime
				case ProviderType.IngresDate:
					return  System.Data.DbType.DateTime;      // ingresdate

				case ProviderType.Interval:
				case ProviderType.IntervalYearToMonth:
					return System.Data.DbType.String;         // interval
				case ProviderType.IntervalDayToSecond:
					return System.Data.DbType.Object;         // interval DS
				case ProviderType.Decimal:
					return  System.Data.DbType.Decimal;       // decimal
				case ProviderType.LongNVarChar:
					return  System.Data.DbType.String;        // long nvarchar
				case ProviderType.LongVarBinary:
					return  System.Data.DbType.Binary;        // long byte
				case ProviderType.LongVarChar:
					return  System.Data.DbType.String;        // long varchar
				case ProviderType.NChar:
					return  System.Data.DbType.String;        // nchar
				case ProviderType.NVarChar:
					return  System.Data.DbType.String;        // nvarchar
				case ProviderType.VarBinary:
					return  System.Data.DbType.Binary;        // byte varying
				case ProviderType.VarChar:
					return  System.Data.DbType.String;        // varchar
				case ProviderType.Boolean:
					return System.Data.DbType.Boolean;        // boolean
				default:
					return System.Data.DbType.Object;
			}
		}

		public static ProviderType GetProviderType(System.Data.DbType i)
		{
			switch(i)
			{
				case System.Data.DbType.AnsiString:
					return ProviderType.VarChar;         // varchar
				case System.Data.DbType.AnsiStringFixedLength:
					return ProviderType.Char;            // char

				case System.Data.DbType.Binary:
					return ProviderType.VarBinary;       // byte varying

				case System.Data.DbType.Byte:
					return ProviderType.TinyInt;        // keep as integer

				case System.Data.DbType.SByte:
					return ProviderType.TinyInt;         // tinyint

				case System.Data.DbType.Currency:
				case System.Data.DbType.Decimal:
					return ProviderType.Decimal;         // decimal

				case System.Data.DbType.Date:
					return ProviderType.Date;            // date
				case System.Data.DbType.Time:
					return ProviderType.Time;            // time
				case System.Data.DbType.DateTime:
					return ProviderType.DateTime;        // datetime

				case System.Data.DbType.Double:
					return ProviderType.Double;          // float8
				case System.Data.DbType.Single:
					return ProviderType.Real;            // real

				case System.Data.DbType.Int16:
					return ProviderType.SmallInt;        // smallint
				case System.Data.DbType.Int32:
					return ProviderType.Integer;         // integer
				case System.Data.DbType.Int64:
					return ProviderType.BigInt;          // bigint

				case System.Data.DbType.String:
					return ProviderType.NVarChar;        // nvarchar
				case System.Data.DbType.StringFixedLength:
					return ProviderType.NChar;           // nchar

				case System.Data.DbType.UInt16:
					return ProviderType.Integer;         // unsigned smallint

				case System.Data.DbType.UInt32:          // unsigned 32 bit
				case System.Data.DbType.UInt64:          // unsigned 64 bit
				case System.Data.DbType.VarNumeric:
					return ProviderType.Decimal;

				case System.Data.DbType.Boolean:
					return ProviderType.Boolean;

				default:
					return ProviderType.Other;
			}
		}

		public static ProviderType GetProviderType(String typeName, int length)
		{
			if (typeName == null)
				return ProviderType.Other;

			typeName = typeName.TrimEnd().ToUpperInvariant();

			switch (typeName)
			{
				case "BYTE":
					return ProviderType.Binary;
				case "BYTE VARYING":
					return ProviderType.VarBinary;
				case "C":
					return ProviderType.Char;
				case "CHAR":
					return ProviderType.Char;
				case "CHARACTER":
					return ProviderType.Char;
				case "DATE":
					return ProviderType.DateTime;
				case "DECIMAL":
					return ProviderType.Decimal;
				case "FLOAT":
					if (length == 4)
						return ProviderType.Single;
					return ProviderType.Double;
				case "INTEGER":
					if (length == 1)
						return ProviderType.TinyInt;
					if (length == 2)
						return ProviderType.SmallInt;
					if (length == 8)
						return ProviderType.BigInt;
					return ProviderType.Integer;
				case "LONG BYTE":
					return ProviderType.LongVarBinary;
				case "LONG VARCHAR":
					return ProviderType.LongVarChar;
				case "MONEY":
					return ProviderType.Decimal;
				case "TEXT":
					return ProviderType.VarChar;
				case "VARCHAR":
					return ProviderType.VarChar;
				case "NCHAR":
					return ProviderType.NChar;
				case "NVARCHAR":
					return ProviderType.NVarChar;
				case "LONG NVARCHAR":
					return ProviderType.LongNVarChar;
				case "INGRESDATE":
				case "ANSIDATE":
				case "TIMESTAMP WITHOUT TIME ZONE":
				case "TIMESTAMP WITH TIME ZONE":
				case "TIMESTAMP WITH LOCAL TIME ZONE":
					return ProviderType.DateTime;
				case "TIME WITHOUT TIME ZONE":
				case "TIME WITH TIME ZONE":
				case "TIME WITH LOCAL TIME ZONE":
					return ProviderType.DateTime;
				case "INTERVAL YEAR TO MONTH":
					return ProviderType.IntervalYearToMonth;
				case "INTERVAL DAY TO SECOND":
					return ProviderType.IntervalDayToSecond;
				case "BOOLEAN":
					return ProviderType.Boolean;

				// Cases beyond this point should not happen
				// but handle anyway.

				case "TINYINT":
					return ProviderType.TinyInt;
				case "SMALLINT":
					return ProviderType.SmallInt;
				case "BIGINT":
					return ProviderType.BigInt;
				case "FLOAT8":
					return ProviderType.Double;
				case "DOUBLE PRECISION":
					return ProviderType.Double;
				case "FLOAT4":
					return ProviderType.Single;
				case "REAL":
					return ProviderType.Single;
			}

			return ProviderType.Binary;
		}  // GetProviderType(String typeName, int length)


		internal static bool isLong(ProviderType providerType)
		{
			switch(providerType)
			{
				case ProviderType.LongNVarChar:   // long nvarchar
				case ProviderType.LongVarBinary:  // long byte
				case ProviderType.LongVarChar:    // long varchar
					return true;
			}
			return false;
		}

		internal static bool hasPrecision(ProviderType providerType)
		{
			switch(providerType)
			{
				case ProviderType.BigInt:      // BigInt
				case ProviderType.Integer:     // integer
				case ProviderType.SmallInt:    // smallint
				case ProviderType.TinyInt:     // tinyint
				case ProviderType.Double:      // float8
				case ProviderType.Real:        // real
				case ProviderType.DateTime:    // date
				case ProviderType.IngresDate:  // ingresdate
				case ProviderType.Interval:    // interval
				case ProviderType.IntervalYearToMonth:    // interval YM
				case ProviderType.IntervalDayToSecond:    // interval DS
				case ProviderType.Decimal:     // decimal
					return true;
			}
			return false;
		}

		internal static bool hasScale(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.DateTime:    // date
				case ProviderType.IngresDate:  // ingresdate
				case ProviderType.Decimal:     // decimal
					return true;
			}
			return false;
		}

		internal static bool isCaseSensitive(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.LongVarChar:
				case ProviderType.VarChar:
				case ProviderType.Char:
					return true;
			}
			return false;
		}

		internal static bool isFixedLength(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.LongNVarChar:   // long nvarchar
				case ProviderType.LongVarBinary:  // long byte
				case ProviderType.LongVarChar:    // long varchar
				case ProviderType.NVarChar:       // nvarchar
				case ProviderType.VarBinary:      // byte
				case ProviderType.VarChar:        // varchar
					return false;
			}
			return true;
		}

		internal static bool isFixedPrecAndScale(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.BigInt:      // BigInt
				case ProviderType.Integer:     // integer
				case ProviderType.SmallInt:    // smallint
				case ProviderType.TinyInt:     // tinyint
				case ProviderType.Decimal:     // decimal
					return true;
			}
			return false;
		}

		internal static Boolean isSigned(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.BigInt:      // BigInt
				case ProviderType.Integer:     // integer
				case ProviderType.SmallInt:    // smallint
				case ProviderType.TinyInt:     // tinyint
				case ProviderType.Double:      // float8
				case ProviderType.Real:        // real
				case ProviderType.Decimal:     // decimal
				case ProviderType.Numeric:     // numeric
					return true;
			}
			return false;
		}

		internal static bool isSearchable(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.LongNVarChar:   // long nvarchar
				case ProviderType.LongVarBinary:  // long byte
				case ProviderType.LongVarChar:    // long varchar
					return false;
			}
			return true;
		}

		internal static bool isSearchableWithLike(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.NVarChar:
				case ProviderType.NChar:
				case ProviderType.VarChar:
				case ProviderType.Char:
					return true;
			}
			return false;
		}

		internal static Int64 ColumnSize(ProviderType providerType, int max_dec_prec)
		{
			switch (providerType)
			{
				case ProviderType.BigInt:
					return 20;          // bigint
				case ProviderType.Integer:
					return 10;          // integer
				case ProviderType.SmallInt:
					return 5;          // smallint
				case ProviderType.TinyInt:
					return 3;          // tinyint
				case ProviderType.Double:
					return 53;         // float8
				case ProviderType.Real:
					return 24;         // real
				case ProviderType.Binary:
					return 2000;        // byte
				case ProviderType.Char:
					return 2000;        // char
				case ProviderType.DateTime:
				case ProviderType.IngresDate:
					return 19;          // date
				case ProviderType.Date:
					return 10;          // ansidate
				case ProviderType.Time:
					return 18;          // time
				case ProviderType.Interval:
					return 15;        // interval (generic)
				case ProviderType.IntervalDayToSecond:
					return 19;        // interval DS
				case ProviderType.IntervalYearToMonth:
					return 15;        // interval YM
				case ProviderType.Decimal:
					return max_dec_prec;  // decimal
				case ProviderType.LongNVarChar:
					return Int32.MaxValue / sizeof(char);        // long nvarchar
				case ProviderType.LongVarBinary:
					return Int32.MaxValue;        // long byte
				case ProviderType.LongVarChar:
					return Int32.MaxValue;        // long varchar
				case ProviderType.NChar:
					return 2000 / sizeof(char);        // nchar
				case ProviderType.NVarChar:
					return Int32.MaxValue / sizeof(char);        // nvarchar
				case ProviderType.VarBinary:
					return 2000;        // byte varying
				case ProviderType.VarChar:
					return 2000;        // varchar
				case ProviderType.Boolean:
					return 1;           // boolean
			}
			return 0;
		}

		internal static Int16 MaximumScale(ProviderType providerType, int max_dec_prec)
		{
			switch (providerType)
			{
				case ProviderType.Decimal:
					return (Int16)max_dec_prec;       // decimal
			}
			return 0;
		}

		internal static Int16 MinimumScale(ProviderType providerType)
		{
			return 0;
		}

		internal static Object CreateFormat(
			ProviderType providerType, bool isANSIDateTimeSupported)
		{
			switch (providerType)
			{
				case ProviderType.BigInt:
					return "integer8";               // bigint
				case ProviderType.Integer:
					return "integer";                // integer
				case ProviderType.SmallInt:
					return "smallint";               // smallint
				case ProviderType.TinyInt:
					return "integer1";               // tinyint
				case ProviderType.Double:
					return "double precision";       // float8
				case ProviderType.Real:
					return "real";                   // real
				case ProviderType.Binary:
					return "byte({0})";              // byte
				case ProviderType.Char:
					return "char({0})";              // char
				case ProviderType.DateTime:
					return isANSIDateTimeSupported?
						"timestamp":                 // ansi timestamp
						"date";                      // old ingres date keyword
				case ProviderType.IngresDate:
					return isANSIDateTimeSupported ?
						"ingresdate" :               // ingresdate
						"date";                      // old ingres date keyword
				case ProviderType.Date:
					return isANSIDateTimeSupported ?
						"ansidate" :                 // ansi date
						"date";                      // old ingres date keyword
				case ProviderType.Time:
					return isANSIDateTimeSupported ?
						"time" :                     // ansi time
						"date";                      // old ingres date keyword
				case ProviderType.Interval:
				case ProviderType.IntervalDayToSecond:
					return "interval day to second"; // interval DS
				case ProviderType.IntervalYearToMonth:
					return "interval year to month"; // interval YM
				case ProviderType.Decimal:
					return "decimal({0},{1})";       // decimal
				case ProviderType.LongNVarChar:
					return "long nvarchar";          // long nvarchar
				case ProviderType.LongVarBinary:
					return "long byte";              // long byte
				case ProviderType.LongVarChar:
					return "long varchar";           // long varchar
				case ProviderType.NChar:
					return "nchar({0})";             // nchar
				case ProviderType.NVarChar:
					return "nvarchar({0})";          // nvarchar
				case ProviderType.VarBinary:
					return "byte varying";           // byte varying
				case ProviderType.VarChar:
					return "varchar({0})";           // varchar
				case ProviderType.Boolean:
					return "boolean";                // boolean
			}
			return DBNull.Value;
		}

		internal static Object CreateParameters(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.Binary:
					return "length";        // byte
				case ProviderType.Char:
					return "length";        // char
				case ProviderType.Decimal:
					return "precision,scale";       // decimal
				case ProviderType.NChar:
					return "length";        // nchar
				case ProviderType.NVarChar:
					return "max length";        // nvarchar
				//case ProviderType.VarBinary:
				//    return "length";        // byte varying
				case ProviderType.VarChar:
					return "max length";        // varchar
			}
			return DBNull.Value;
		}

		internal static Object LiteralPrefix(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.LongNVarChar:   // long nvarchar
				case ProviderType.NVarChar:       // nvarchar
				case ProviderType.NChar:          // nchar
				case ProviderType.LongVarChar:    // long varchar
				case ProviderType.VarChar:        // varchar
				case ProviderType.Char:           // char
				case ProviderType.Interval:       // interval (generic)
				case ProviderType.IntervalDayToSecond:// interval DS
				case ProviderType.IntervalYearToMonth:// interval YM
					return "'";

				case ProviderType.DateTime:       // ingresdate or ansi ts
				case ProviderType.IngresDate:
					return "{ts '";

				case ProviderType.Date:           // date
					return "{d '";

				case ProviderType.Time:           // time
					return "{t '";

				case ProviderType.LongVarBinary:  // long byte
				case ProviderType.VarBinary:      // byte varying
				case ProviderType.Binary:         // byte
					return "X'";
			}
			return DBNull.Value;
		}

		internal static Object LiteralSuffix(ProviderType providerType)
		{
			switch (providerType)
			{
				case ProviderType.LongNVarChar:   // long nvarchar
				case ProviderType.NVarChar:       // nvarchar
				case ProviderType.NChar:          // nchar
				case ProviderType.LongVarChar:    // long varchar
				case ProviderType.VarChar:        // varchar
				case ProviderType.Char:           // char
				case ProviderType.Interval:       // interval (generic)
				case ProviderType.IntervalDayToSecond:// interval DS
				case ProviderType.IntervalYearToMonth:// interval YM
				case ProviderType.LongVarBinary:  // long byte
				case ProviderType.VarBinary:      // byte
				case ProviderType.Binary:         // byte
					return "'";

				case ProviderType.DateTime:       // ingresdate or ansi ts
				case ProviderType.IngresDate:
				case ProviderType.Date:           // date
				case ProviderType.Time:           // time
					return "'}";
			}
			return DBNull.Value;
		}

	} // class
}  // namespace


