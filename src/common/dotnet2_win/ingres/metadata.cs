/*
** Copyright (c) 2002, 2010 Ingres Corporation. All Rights Reserved.
*/


/*
** Name: metadata.cs
**
** Description:
**	Implements the .NET	Provider IngresMetaDataCollectionNames and
**	IngresConnection.GetSchema().
**
** Classes:
**	IngresMetaData
**	IngresMetaDataCollectionNames
**
** History:
**	 6-Sep-05 (thoda04)
**		Created.
**	30-Jan-07 (thoda04)
**		GetSchemaDataTypes was returning an empty DataTable with no datatypes.
**		Bug 117575.
**	21-Mar-07 (thoda04)
**	    Added IntervalDayToSecond, IntervalYearToMonth.
**	15-Sep-08 (thoda04)
**	    Added max decimal precision as returned by DBMS.
**	02-Nov-08 (thoda04)
**	    GetSchema("Columns") does map integer and float to its subtypes.
**	    Bug 121167.
**	 7-Dec-09 (thoda04)
**	    Added support for BOOLEAN data type.
**	 3-Mar-10 (thoda04)  SIR 123368
**	    Added support for IngresType.IngresDate parameter data type.
**	26-May-10 (thoda04)  Bug 123815
**	    GetSchema("MetaDataCollection") should return
**	    ProcedureParameters, not ProcedureColumns.
**	26-May-10 (thoda04)  Bug 123816
**	    GetSchema("DataSourceInformation") returned
**	    incorrect or incomplete regular expression
**	    match patterns and DataSourceProductVersion.
*/

using System;
using System.Data;
using System.Data.Common;
using System.Collections;
using System.Collections.Generic;

using Ingres.Utility;


namespace Ingres.Client
{
	/// <summary>
	/// Contains the categories of metadata available
	/// from the Ingres data provider.
	/// </summary>
	public abstract class IngresMetaDataCollectionNames
	{
		/// <summary>
		/// Category name of the list of columns in a database table.
		/// </summary>
		public static readonly string Columns          = "Columns";
		/// <summary>
		/// Category name of the list of foreign keys in a database table.
		/// </summary>
		public static readonly string ForeignKeys      = "ForeignKeys";
		/// <summary>
		/// Category name of the list of indexes in a database table.
		/// </summary>
		public static readonly string Indexes          = "Indexes";
		/// <summary>
		/// Category name of the list of columns in a database procedure.
		/// </summary>
		public static readonly string ProcedureParameters="ProcedureParameters";
		/// <summary>
		/// Category name of the list of procedures in a database.
		/// </summary>
		public static readonly string Procedures       = "Procedures";
		/// <summary>
		/// Category name of the list of tables in a database.
		/// </summary>
		public static readonly string Tables           = "Tables";
		/// <summary>
		/// Category name of the list of views in a database.
		/// </summary>
		public static readonly string Views            = "Views";
	}  // class IngresMetaDataCollectionNames


	internal static class IngresMetaData
	{
		private const string TABLE_CATALOG= "TABLE_CATALOG";
		private const string TABLE_SCHEMA = "TABLE_SCHEMA";
		private const string TABLE_NAME   = "TABLE_NAME";
		private const string COLUMN_NAME  = "COLUMN_NAME";

		private static RestrictionCollection
			metaDataCollectionsRestrictionCollection =
				new RestrictionCollection("MetaDataCollections");

		private static RestrictionCollection
			dataSourceInformationRestrictionCollection =
				new RestrictionCollection("DataSourceInformation");

		private static RestrictionCollection
			dataTypesRestrictionCollection =
				new RestrictionCollection("DataTypes");

		private static RestrictionCollection
			restrictionsRestrictionCollection =
				new RestrictionCollection("Restrictions");

		private static RestrictionCollection
			reservedWordsRestrictionCollection =
				new RestrictionCollection("ReservedWords");

		private static RestrictionCollection
			columnsRestrictionCollection =
				new RestrictionCollection("Columns", 4, new Restriction[]
				{
					new Restriction(TABLE_CATALOG, null, 1),
					new Restriction(TABLE_SCHEMA,  null, 2, "table_owner"),
					new Restriction(TABLE_NAME,    null, 3),
					new Restriction(COLUMN_NAME,   null, 4),
				});

		private static RestrictionCollection
			tablesRestrictionCollection =
				new RestrictionCollection("Tables", 3, new Restriction[]
				{
					new Restriction(TABLE_CATALOG, null, 1),
					new Restriction(TABLE_SCHEMA,  null, 2, "table_owner"),
					new Restriction(TABLE_NAME,    null, 3),
				});

		private static RestrictionCollection
			viewsRestrictionCollection =
				new RestrictionCollection("Views", 3, new Restriction[]
				{
					new Restriction(TABLE_CATALOG, null, 1),
					new Restriction(TABLE_SCHEMA,  null, 2, "table_owner"),
					new Restriction(TABLE_NAME,    null, 3),
				});

		private static RestrictionCollection
			indexesRestrictionCollection =
				new RestrictionCollection("Indexes", 3, new Restriction[]
				{
					new Restriction(TABLE_CATALOG, null, 1),
					new Restriction(TABLE_SCHEMA,  null, 2, "i.base_owner"),
					new Restriction(TABLE_NAME,    null, 3, "i.base_name"),
					new Restriction("INDEX_NAME",  null, 4, "i.index_name"),
				});

		private static RestrictionCollection
			proceduresRestrictionCollection =
				new RestrictionCollection("Procedures", 3, new Restriction[]
				{
					new Restriction("PROCEDURE_CATALOG", null, 1),
					new Restriction("PROCEDURE_SCHEMA",  null, 2, "procedure_owner"),
					new Restriction("PROCEDURE_NAME",    null, 3),
				});

		private static RestrictionCollection
			procedureParametersRestrictionCollection =
				new RestrictionCollection("ProcedureParameters", 4, new Restriction[]
				{
					new Restriction("PROCEDURE_CATALOG", null, 1),
					new Restriction("PROCEDURE_SCHEMA",  null, 2, "procedure_owner"),
					new Restriction("PROCEDURE_NAME",    null, 3),
					new Restriction(COLUMN_NAME,         null, 4, "param_name"),
				});



		private static RestrictionCollection[]
			restrictionCollections = new RestrictionCollection[]
			{
				metaDataCollectionsRestrictionCollection,
				dataSourceInformationRestrictionCollection,
				dataTypesRestrictionCollection,
				restrictionsRestrictionCollection,
				reservedWordsRestrictionCollection,
				tablesRestrictionCollection,
				viewsRestrictionCollection,
				indexesRestrictionCollection,
				proceduresRestrictionCollection,
				procedureParametersRestrictionCollection,
			}; // restrictionCollections


		
		static internal DataTable GetSchema(
			DbConnection conn,
			string       collectionName,
			string[]     restrictionValues)
		{
			string[] restrictValues = new string[4];

			if (restrictionValues != null)
				Array.Copy(restrictionValues, restrictValues,
					Math.Min(4, restrictionValues.Length));

			if (collectionName == null || collectionName == String.Empty)
				collectionName = "MetaDataCollections";

			string collectionNameLower = collectionName.ToLowerInvariant();

			switch (collectionNameLower)
			{
				case "metadatacollections":
					return GetSchemaMetaDataCollection(conn, restrictValues);

				case "datasourceinformation":
					return GetSchemaDataSourceInformation(conn, restrictValues);

				case "datatypes":
					return GetSchemaDataTypes(conn, restrictValues);

				case "restrictions":
					return GetSchemaRestrictions(conn, restrictValues);

				case "reservedwords":
					return GetSchemaReservedWords(conn, restrictValues);

				case "tables":
					return GetSchemaTables(conn, restrictValues);

				case "columns":
					return GetSchemaColumns(conn, restrictValues);

				case "views":
					return GetSchemaViews(conn, restrictValues);

				case "foreignkeys":
					return GetSchemaForeignKeys(conn, restrictValues);

				case "indexes":
					return GetSchemaIndexes(conn, restrictValues);

				case "procedures":
					return GetSchemaProcedures(conn, restrictValues);

				case "procedureparameters":
					return GetSchemaProcedureParameters(conn, restrictValues);

				case "root":
					return GetSchemaRootInformation(conn, restrictValues);

				default:
					if (collectionName == null)
						collectionName = "<null>";
					throw new ArgumentException(
						"The requested collection (" +
						collectionName +
						") is not defined.");
			}  // end switch

		}  // GetSchema


		private static DataTable GetSchemaMetaDataCollection(
			DbConnection conn,
			string[]     restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("CollectionName",          typeof(String)),
				new DataColumn("NumberOfRestrictions",    typeof(Int32)),
				new DataColumn("NumberOfIdentifierParts", typeof(Int32)),
			};

			DataTable dt = CreateSchemaDataTable(
				"MetaDataCollection", columnNames);

			foreach (RestrictionCollection collection in restrictionCollections)
			{
				object[] row = new Object[3];
				row[0] = collection.Name;
				row[1] = collection.Restrictions.Length;
				row[2] = collection.NumberIdentifierParts;
				dt.Rows.Add(row);
			}

			return dt;
		}  // GetSchemaMetaDataCollection


		private static DataTable GetSchemaDataSourceInformation(
			DbConnection conn,
			string[]     restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("CompositeIdentifierSeparatorPattern",typeof(String)),
				new DataColumn("DataSourceProductName",    typeof(String)),
				new DataColumn("DataSourceProductVersion", typeof(String)),
				new DataColumn("DataSourceProductVersionNormalized",typeof(String)),
				new DataColumn("GroupByBehavior",          typeof(GroupByBehavior)),
				new DataColumn("IdentifierPattern",        typeof(String)),
				new DataColumn("IdentifierCase",           typeof(IdentifierCase)),
				new DataColumn("OrderByColumnsInSelect",   typeof(Boolean)),
				new DataColumn("ParameterMarkerFormat",    typeof(String)),
				new DataColumn("ParameterMarkerPattern",   typeof(String)),
				new DataColumn("ParameterNameMaxLength",   typeof(Int32)),
				new DataColumn("ParameterNamePattern",     typeof(String)),
				new DataColumn("QuotedIdentifierPattern",  typeof(String)),
				new DataColumn("QuotedIdentifierCase",     typeof(IdentifierCase)),
				new DataColumn("StatementSeparatorPattern",typeof(String)),
				new DataColumn("StringLiteralPattern",     typeof(String)),
				new DataColumn("SupportedJoinOperators",typeof(SupportedJoinOperators)),
			};

			// conn.ServerVersion is similar to
			// "09.03.0001 II 9.3.1 (int.w32/101)"
			// find the end of "09.03.0001" substring
			int    i = conn.ServerVersion.IndexOf(' ');

			// set the product version, e.g. "09.03.0001"
			string productVersion = (i==-1)?
				"03.00.0000":
				conn.ServerVersion.Substring(0,i);

			Object[] values = new Object[]
			{
				new Object[] {   // just one row in the DataTable
					@"\.",                    // IdentifierSeparatorPattern
					"Ingres",                 // ProductName
					productVersion,           // ProductVersion
					productVersion,           // ProductVersionNormalized
					GroupByBehavior.MustContainAll,// GroupByBehavior
					@"(^[\p{Ll}\p{Lu}\p{Lo}_]"+ // IdentifierPattern
					  @"[\p{Ll}\p{Lu}\p{Lo}\p{Nd}_@#$]*$)|"+
					@"(^\""([^\""\0]|\""\"")+\""$)",
					IdentifierCase.Insensitive,//IdentifierCase
					false,                    // OrderByColumnsInSelect
					"?",                      // ParameterMarkerFormat
					@"\?",                    // ParameterMarkerPattern
					0,                        // ParameterNameMaxLength
					DBNull.Value,             // ParameterNamePattern
					"\"(([^\"]|\"\")*)\"",    // QuotedIdentifierPattern
					IdentifierCase.Insensitive,//QuotedIdentifierCase
					"",                       // StatementSeparatorPattern
					"'(([^']|'')*)'",         // StringLiteralPattern
					SupportedJoinOperators.FullOuter| // SupportedJoinOperators
					SupportedJoinOperators.LeftOuter|
					SupportedJoinOperators.RightOuter,
					}
			};

			DataTable dt = CreateSchemaDataTable(
				"DataSourceInformation", columnNames);

			foreach (object[] row in values)
				dt.Rows.Add(row);

			return dt;
		}  // GetSchemaDataSourceInformation


		private static DataTable GetSchemaDataTypes(
			DbConnection conn,
			string[]     restrictionValues)
		{
			bool isUnicodeSupported = true;
			bool isANSIDateTimeSupported = false;
			bool isBooleanSupported = false;
			int  maxDecimalPrecision    = DbmsConst.DBMS_PREC_MAX;

			DataColumn[] columnNames = new DataColumn[]
			{
			    new DataColumn("TypeName",              typeof(String)),
			    new DataColumn("ProviderDbType",        typeof(Int32)),
			    new DataColumn("ColumnSize",            typeof(Int64)),
			    new DataColumn("CreateFormat",          typeof(String)),
			    new DataColumn("CreateParameters",      typeof(String)),
			    new DataColumn("DataType",              typeof(String)),
			    new DataColumn("IsAutoIncrementable",   typeof(Boolean)),
			    new DataColumn("IsBestMatch",           typeof(Boolean)),
			    new DataColumn("IsCaseSensitive",       typeof(Boolean)),
			    new DataColumn("IsFixedLength",         typeof(Boolean)),
			    new DataColumn("IsFixedPrecisionScale", typeof(Boolean)),
			    new DataColumn("IsLong",                typeof(Boolean)),
			    new DataColumn("IsNullable",            typeof(Boolean)),
			    new DataColumn("IsSearchable",          typeof(Boolean)),
			    new DataColumn("IsSearchableWithLike",  typeof(Boolean)),
			    new DataColumn("IsUnsigned",            typeof(Boolean)),
			    new DataColumn("MaximumScale",          typeof(Int16)),
			    new DataColumn("MinimumScale",          typeof(Int16)),
			    new DataColumn("IsConcurrencyType",     typeof(Boolean)),
			    new DataColumn("IsLiteralSupported",    typeof(Boolean)),
			    new DataColumn("LiteralPrefix",         typeof(String)),
			    new DataColumn("LiteralSuffix",         typeof(String)),
			};

			ProviderType[] providerTypeList =
				new ProviderType[]
			{
				ProviderType.LongNVarChar,
				ProviderType.NVarChar,
				ProviderType.NChar,
				ProviderType.TinyInt,
				ProviderType.LongVarBinary,
				ProviderType.VarBinary,
				ProviderType.Binary,
				ProviderType.LongVarChar,
				ProviderType.Char,
				ProviderType.Decimal,
				ProviderType.Integer,
				ProviderType.BigInt,
				ProviderType.SmallInt,
				ProviderType.Double,
				ProviderType.Real,
				ProviderType.DateTime,
				ProviderType.IngresDate,
				ProviderType.Date,
				ProviderType.Time,
				ProviderType.IntervalYearToMonth,
				ProviderType.IntervalDayToSecond,
				ProviderType.VarChar,
				ProviderType.Boolean,
			};

			IngresConnection ingresConn = conn as IngresConnection;
			if (ingresConn != null)
			{
				Ingres.ProviderInternals.AdvanConnect advanConn =
					ingresConn.advanConnect;
				if (advanConn != null  &&  advanConn.conn != null)
				{
					Ingres.ProviderInternals.DrvConn drvConn = advanConn.conn;

					isUnicodeSupported  = drvConn.ucs2_supported;
					maxDecimalPrecision = drvConn.max_dec_prec;

					if (drvConn.db_protocol_level >= DbmsConst.DBMS_API_PROTO_4)
						isANSIDateTimeSupported = true;
					if (drvConn.db_protocol_level >= DbmsConst.DBMS_API_PROTO_6)
						isBooleanSupported      = true;
				}
			}

			DataTable dt = CreateSchemaDataTable(
				"DataTypes", columnNames);

			foreach (ProviderType providerType
				in providerTypeList)
			{
				if (!isUnicodeSupported)  // if no NCHAR, NVARCHAR, ..., skip it
				{
					if (providerType == ProviderType.NChar ||
						providerType == ProviderType.NVarChar ||
						providerType == ProviderType.LongNVarChar)
						continue;
				}  // end if (!isUnicodeSupported)

				if (!isANSIDateTimeSupported)
				{
					if (providerType == ProviderType.Date ||
						providerType == ProviderType.Time ||
						providerType == ProviderType.IntervalYearToMonth ||
						providerType == ProviderType.IntervalDayToSecond  ||
						providerType == ProviderType.IngresDate)
						   //use old date, not ingresdate if no ANSI types
						continue;
				}

				if (!isBooleanSupported)
				{
					if (providerType == ProviderType.Boolean)
						continue;
				}

				String typeName =
					ProviderTypeMgr.GetDataTypeName(providerType, isANSIDateTimeSupported);
				Int32  providerDbType =(Int32)
					ProviderTypeMgr.GetDbType(providerType);
				Int64 columnSize =
					ProviderTypeMgr.ColumnSize(providerType, maxDecimalPrecision);
				Object createFormat =
					ProviderTypeMgr.CreateFormat(providerType, isANSIDateTimeSupported);
				Object createParam =
					ProviderTypeMgr.CreateParameters(providerType);
				Object isBestMatch =
					DBNull.Value;
				Type netType =
					ProviderTypeMgr.GetNETType(providerType);
				Boolean isCaseSensitive =
					ProviderTypeMgr.isCaseSensitive(providerType);
				Boolean isFixedLength =
					ProviderTypeMgr.isFixedLength(providerType);
				Boolean isFixedPrecAndScale =
					ProviderTypeMgr.isFixedPrecAndScale(providerType);
				Boolean isLong =
					ProviderTypeMgr.isLong(providerType);
				Boolean isSearchable =
					ProviderTypeMgr.isSearchable(providerType);
				Boolean isSearchableWithLike =
					ProviderTypeMgr.isSearchableWithLike(providerType);
				Object  isUnSigned =
					!ProviderTypeMgr.isSigned(providerType);
				Object maximumScale =
					ProviderTypeMgr.MaximumScale(providerType, maxDecimalPrecision);
				Object minimumScale =
					ProviderTypeMgr.MinimumScale(providerType);
				Object literalPrefix =
					ProviderTypeMgr.LiteralPrefix(providerType);
				Object literalSuffix =
					ProviderTypeMgr.LiteralSuffix(providerType);
				Object  isLiteralSupported =
					(literalPrefix != null &&
					 literalPrefix != DBNull.Value);

				if (providerType == ProviderType.Boolean)
				{
					isBestMatch        = true;
					isUnSigned         = DBNull.Value;
					isLiteralSupported = true;
					maximumScale       = DBNull.Value;
					minimumScale       = DBNull.Value;
				}

				DataRow row = dt.NewRow();
				row["TypeName"             ] = typeName;
				row["ProviderDbType"       ] = providerDbType;
				row["ColumnSize"           ] = columnSize;
				row["CreateFormat"         ] = createFormat;
				row["CreateParameters"     ] = createParam;
				row["DataType"             ] = netType.ToString();
				row["IsAutoIncrementable"  ] = false;
				row["IsBestMatch"          ] = isBestMatch;
				row["IsCaseSensitive"      ] = isCaseSensitive;
				row["IsFixedLength"        ] = isFixedLength;
				row["IsFixedPrecisionScale"] = isFixedPrecAndScale;
				row["IsLong"               ] = isLong;
				row["IsNullable"           ] = true;
				row["IsSearchable"         ] = isSearchable;
				row["IsSearchableWithLike" ] = isSearchableWithLike;
				row["IsUnsigned"           ] = isUnSigned;
				row["MaximumScale"         ] = maximumScale;
				row["MinimumScale"         ] = minimumScale;
				row["IsConcurrencyType"    ] = DBNull.Value;
				row["IsLiteralSupported"   ] = isLiteralSupported;
				row["LiteralPrefix"        ] = literalPrefix;
				row["LiteralSuffix"        ] = literalSuffix;
				dt.Rows.Add(row);
			}

			return dt;
		}  // GetSchemaDataTypes

		private static DataTable GetSchemaRootInformation(
			DbConnection conn,
			string[] restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("Server",       typeof(String)),
				new DataColumn("Database",     typeof(String)),
				new DataColumn("DefaultSchema",typeof(String)),
			};

			if (conn.State != ConnectionState.Open)
				conn.Open();

			IngresConnection ingresConn = conn as IngresConnection;
			if (ingresConn == null)
				throw new ArgumentException(
					"GetSchemaRootInformation does not have the correct " +
					"IngresConnection type for the underlying connection.");

			Ingres.ProviderInternals.Catalog catalog =
				ingresConn.OpenCatalog();
			if (catalog == null)  // should not happen
				throw new ArgumentException(
					"GetSchemaRootInformation does not have the correct " +
					"IngresConnection catalog.");

			DataTable dt = CreateSchemaDataTable(
				"Root", columnNames);

			string datasource = conn.DataSource;
			if (datasource == null || datasource.Length == 0)
				datasource = "(local)";

			string defaultSchema = catalog.GetDefaultSchema();
			if (defaultSchema == null  ||  defaultSchema.Length == 0)
				defaultSchema = "<no default schema>";

			object[] row = new Object[columnNames.Length];

			row[0] = TrimDBName(datasource);
			row[1] = TrimDBName(conn.Database);
			row[2] = TrimDBName(defaultSchema);
			dt.Rows.Add(row);

			return dt;
		}


		private static DataTable GetSchemaRestrictions(
			DbConnection conn,
			string[]     restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("CollectionName",      typeof(String)),
				new DataColumn("RestrictionName",     typeof(String)),
				new DataColumn("RestrictionDefault",  typeof(String)),
				new DataColumn("RestrictionNumber",   typeof(Int32)),
			};

			DataTable dt = CreateSchemaDataTable(
				"Restrictions", columnNames);

			foreach (RestrictionCollection collection in restrictionCollections)
			{
				if (collection.Restrictions.Length == 0)
					continue;

				foreach (Restriction restriction in collection.Restrictions)
				{
					object[] row = new Object[4];
					row[0] = collection.Name;
					row[1] = restriction.Name;
					row[2] = restriction.Default;
					row[3] = restriction.Number;
					dt.Rows.Add(row);
				}
			}


			return dt;
		}  // GetSchemaRestrictions


		private static DataTable GetSchemaReservedWords(
			DbConnection conn,
			string[]     restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("ReservedWord",     typeof(String)),
			};

			String[] reservedWords =
			{
				//                  Unreserved ANSI/ISO SQL keywords
				//                  are not included in the list at
				//                  this time since Ingres syntax
				//                  parsers are not confused at this
				//                  time.
				"ABORT",
				//"ABSOLUTE",    // Unreserved ANSI/ISO SQL keyword
				//"ACTION",      // Unreserved ANSI/ISO SQL keyword
				"ACTIVATE", "ABORT", "ACTIVATE", "ADD",
				"ADDFORM", "AFTER", "ALL",
				//"ALLOCATE",    // Unreserved ANSI/ISO SQL keyword
				"ALTER", "AND", "ANY",
				"APPEND",
				//"ARE",         // Unreserved ANSI/ISO SQL keyword
				"ARRAY", "AS", "ASC",
				//"ASENSITIVE",  // Unreserved ANSI/ISO SQL keyword
				//"ASSERTION",   // Unreserved ANSI/ISO SQL keyword
				"ASYMMETRIC",
				"AT",
				//"ATOMIC",      // Unreserved ANSI/ISO SQL keyword
				"AUTHORIZATION", "AVG", "AVGU",
				"BEFORE", "BEGIN", "BETWEEN",
				//"BIT",         // Unreserved ANSI/ISO SQL keyword
				//"BIT_LENGTH",  // Unreserved ANSI/ISO SQL keyword
				//"BOTH",        // Unreserved ANSI/ISO SQL keyword
				"BREAKDISPLAY",
				"BY", "BYREF",
				"CACHE", "CALL",
				//"CALLED",      // Unreserved ANSI/ISO SQL keyword
				"CALLFRAME", "CALLPROC",
				//"CARDINALITY", // Unreserved ANSI/ISO SQL keyword
				"CASCADE",
				//"CASCADED",    // Unreserved ANSI/ISO SQL keyword
				"CASE", "CAST",
				//"CATALOG",     // Unreserved ANSI/ISO SQL keyword
				//"CHAR",        // Unreserved ANSI/ISO SQL keyword
				//"CHAR_LENGTH", // Unreserved ANSI/ISO SQL keyword
				//"CHARACTER",   // Unreserved ANSI/ISO SQL keyword
				//"CHARACTER_LENGTH",  // Unreserved ANSI/ISO SQL keyword
				"CHECK",
				"CLEAR", "CLEARROW", "CLOSE", "COALESCE",
				"COLLATE",
				//"COLLATION",   // Unreserved ANSI/ISO SQL keyword
				//"COLLECT",     // Unreserved ANSI/ISO SQL keyword
				"COLUMN", "COMMAND", "COMMENT",
				"COMMIT", "COMMITED",
				//"CONDITION",   // Unreserved ANSI/ISO SQL keyword
				"CONNECT",
				//"CONNECTION",  // Unreserved ANSI/ISO SQL keyword
				"CONSTRAINT",
				//"CONSTRAINTS", // Unreserved ANSI/ISO SQL keyword
				"CONTINUE",
				//"CONVERT",     // Unreserved ANSI/ISO SQL keyword
				"COPY", "COPY_FROM", "COPY_INTO",
				//"CORR",        // Unreserved ANSI/ISO SQL keyword
				//"CORRESPONDING",//Unreserved ANSI/ISO SQL keyword
				"COUNT", "COUNTU", "CREATE",
				//"CROSS",       // Unreserved ANSI/ISO SQL keyword
				//"CUBE",        // Unreserved ANSI/ISO SQL keyword
				"CURRENT",
				//"CURRENT_DATE",// Unreserved ANSI/ISO SQL keyword
				"CURRENT_DEFAULT_TRANSFORM_GROUP", // Unreserved ANSI/ISO SQL
				//"CURRENT_PATH",// Unreserved ANSI/ISO SQL keyword
				//"CURRENT_ROLE",// Unreserved ANSI/ISO SQL keyword
				//"CURRENT_TIME",// Unreserved ANSI/ISO SQL keyword
				//"CURRENT_TIMESTAMP", // Unreserved ANSI/ISO SQL keyword
				"CURRENT_USER", "CURRVAL",
				"CURSOR", "CYCLE",
				"DATAHANDLER",
				//"DATE",        // Unreserved ANSI/ISO SQL keyword
				//"DAY",         // Unreserved ANSI/ISO SQL keyword
				//"DEALLOCATE",  // Unreserved ANSI/ISO SQL keyword
				//"DEC",         // Unreserved ANSI/ISO SQL keyword
				//"DECIMAL",     // Unreserved ANSI/ISO SQL keyword
				"DECLARE", "DEFAULT",
				//"DEFERRABLE",  // Unreserved ANSI/ISO SQL keyword
				//"DEFERRED",    // Unreserved ANSI/ISO SQL keyword
				"DEFINE",
				"DELETE", "DELETEROW",
				//"DEREF",       // Unreserved ANSI/ISO SQL keyword
				"DESC", "DESCRIBE",
				"DESCRIPTOR", "DESTROY",
				//"DETERMINISTIC",//Unreserved ANSI/ISO SQL keyword
				//"DIAGNOSTICS", // Unreserved ANSI/ISO SQL keyword
				"DIRECT", "DISABLE",
				"DISCONNECT", "DISPLAY", "DISTINCT",
				"DISTRIBUTE", "DO",
				//"DOMAIN",      // Unreserved ANSI/ISO SQL keyword
				//"DOUBLE",      // Unreserved ANSI/ISO SQL keyword
				"DOWN", "DROP",
				//"DYNAMIC",     // Unreserved ANSI/ISO SQL keyword
				//"EACH",        // Unreserved ANSI/ISO SQL keyword
				//"ELEMENT",     // Unreserved ANSI/ISO SQL keyword
				"ELSE", "ELSEIF", "ENABLE", "END", "END-EXEC",
				"ENDDATA", "ENDDISPLAY", "ENDFOR", "ENDFORMS",
				"ENDIF", "ENDLOOP", "ENDREPEAT", "ENDRETRIEVE",
				"ENDSELECT", "ENDWHILE", "ESCAPE",
				//"EVERY",       // Unreserved ANSI/ISO SQL keyword
				"EXCEPT",
				//"EXCEPTION",   // Unreserved ANSI/ISO SQL keyword
				"EXCLUDE", "EXCLUDING",
				//"EXEC",        // Unreserved ANSI/ISO SQL keyword
				"EXECUTE",
				"EXISTS", "EXIT",
				//"EXTERNAL",    // Unreserved ANSI/ISO SQL keyword
				//"EXTRACT",     // Unreserved ANSI/ISO SQL keyword
				//"FALSE",       // Unreserved ANSI/ISO SQL keyword
				"FETCH", "FIELD",
				//"FILTER",      // Unreserved ANSI/ISO SQL keyword
				"FINALIZE", "FIRST",
				//"FLOAT",       // Unreserved ANSI/ISO SQL keyword
				"FOR",
				"FOREIGN", "FORMDATA", "FORMINIT", "FORMS",
				//"FOUND",       // Unreserved ANSI/ISO SQL keyword
				//"FREE",        // Unreserved ANSI/ISO SQL keyword
				"FROM", "FULL",
				//"FUNCTION",    // Unreserved ANSI/ISO SQL keyword
				//"FUSION",      // Unreserved ANSI/ISO SQL keyword
				"GET", "GETFORM", "GETOPER", "GETROW", "GLOBAL",
				//"GO",          // Unreserved ANSI/ISO SQL keyword
				"GOTO", "GRANT", "GRANTED", "GROUP",
				//"GROUPING",    // Unreserved ANSI/ISO SQL keyword
				"HAVING", "HELP", "HELP_FORMS",
				"HELP_FRS", "HELPFILE",
				//"HOLD",        // Unreserved ANSI/ISO SQL keyword
				//"HOUR",        // Unreserved ANSI/ISO SQL keyword
				"IDENTIFIED",
				//"IDENTITY",    // Unreserved ANSI/ISO SQL keyword
				"IF", "IIMESSAGE",
				"IIPRINTF", "IIPROMPT", "IISTATEMENT",
				"IMMEDIATE", "IMPORT", "IN", "INCLUDE",
				"INCREMENT", "INDEX", "INDICATOR",
				"INGRES", "INITIAL_USER", "INITIALIZE",
				//"INITIALLY",   // Unreserved ANSI/ISO SQL keyword
				"INITTABLE", "INNER",
				//"INOUT",       // Unreserved ANSI/ISO SQL keyword
				//"INPUT",       // Unreserved ANSI/ISO SQL keyword
				"INQUIRE_EQUEL",
				"INQUIRE_FORMS", "INQUIRE_FRS", "INQUIRE_INGRES",
				"INQUIRE_SQL",
				//"INSENSITIVE", // Unreserved ANSI/ISO SQL keyword
				"INSERT", "INSERTROW",
				//"INT",         // Unreserved ANSI/ISO SQL keyword
				//"INTEGER",     // Unreserved ANSI/ISO SQL keyword
				"INTEGRITY", "INTERSECT",
				//"INTERSECTION",// Unreserved ANSI/ISO SQL keyword
				//"INTERSECTS",  // Unreserved ANSI/ISO SQL keyword
				//"INTERVAL",    // Unreserved ANSI/ISO SQL keyword
				"INTO", "IS", "ISOLATION",
				"JOIN",
				"KEY",
				//"LANGUAGE",    // Unreserved ANSI/ISO SQL keyword
				//"LARGE",       // Unreserved ANSI/ISO SQL keyword
				//"LAST",        // Unreserved ANSI/ISO SQL keyword
				//"LATERAL",     // Unreserved ANSI/ISO SQL keyword
				//"LEADING",     // Unreserved ANSI/ISO SQL keyword
				"LEAVE", "LEFT", "LEVEL", "LIKE",
				//"LN",          // Unreserved ANSI/ISO SQL keyword
				"LOADTABLE", "LOCAL",
				"MAX", "MAXVALUE", "MENUITEM", "MESSAGE", "MIN", "MINVALUE",
				"MODE", "MODIFY", "MODULE", "MOVE",
				"NATURAL", "NEXT", "NEXTVAL", "NOCACHE", "NOCYCLE", "NOECHO",
				"NOMAXVALUE", "NOMINVALUE", "NOORDER", "NOT", "NOTRIM",
				"NULL", "NULLIF",
				"OF", "ON", "ONLY", "OPEN", "OPTION", "OR", "ORDER",
				"OUT", "OUTER",
				"PARAM", "PARTITION", "PERMIT", "PREPARE", "PRESERVE",
				"PRIMARY", "PRINT", "PRINTSCREEN", "PRIVILEGES",
				"PROCEDURE", "PROMPT", "PROMPT", "PUBLIC", "PURGETABLE",
				"PUTFORM", "PUTOPER", "PUTROW",
				"QUALIFICATION",
				"RAISE", "RANGE", "RAWPCT", "READ", "REDISPLAY",
				"REFERENCES", "REFERENCING", "REGISTER", "RELOCATE",
				"REMOVE", "REMOVE", "RENAME",
				"REPEAT", "REPEATABLE", "REPEATED",
				"REPLACE", "REPLICATE", "RESTART", "RESTRICT", "RESULT",
				"RESUME", "RETRIEVE", "RETURN", "REVOKE", "RIGHT",
				"ROLE", "ROLLBACK", "ROW", "ROWS", "RUN",
				"SAVE", "SAVEPOINT", "SCHEMA", "SCREEN",
				"SCROLL", "SCROLLDOWN", "SCROLLUP", "SECTION",
				"SELECT", "SERIALIZABLE", "SESSION", "SESSION_USER",
				"SET", "SET_4GL", "SET_EQUEL", "SET_FORMS",
				"SET_FRS", "SET_INGRES", "SET_SQL",
				"SLEEP", "SOME", "SORT", "SQL", "START", "STOP",
				"SUBMENU", "SUBSTRING", "SUM", "SUMU", "SYMMETRIC",
				"SYSTEM", "SYSTEM_MAINTAINED", "SYSTEM_USER",
				"TABLE", "TABLEDATA", "TEMPORARY", "THEN", "TO", "TYPE",
				"UNCOMMITTED", "UNION", "UNIQUE", "UNLOADTABLE",
				"UNTIL", "UP", "UPDATE", "USER", "USING",
				"VALIDATE", "VALIDROW", "VALUES", "VIEW",
				"WHEN", "WHENEVER", "WHERE", "WHILE",
				"WITH", "WORK", "WRITE",
			};

			DataTable dt = CreateSchemaDataTable(
				"ReservedWords", columnNames);

			//foreach (object[] row in values)
			//    dt.Rows.Add(row);
			foreach (String reservedWord in reservedWords)
			{
				DataRow row = dt.NewRow();
				row[0] = reservedWord;
				dt.Rows.Add(row);
			}

			return dt;
		}  // GetSchemaReservedWords


		private static DataTable GetSchemaTables(
			DbConnection conn,
			string[] restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("TABLE_CATALOG", typeof(String)),
				new DataColumn("TABLE_SCHEMA",  typeof(String)),
				new DataColumn("TABLE_NAME",    typeof(String)),
				new DataColumn("TABLE_TYPE",    typeof(String)),
			};

			DataTable dt = CreateSchemaDataTable(
				"Tables", columnNames);

			// if user specifies catalog restriction, Ingres does not
			// support catalog.  So return empty datatable.
			if (restrictionValues[0] != null)
				return dt;

			DbCommand cmd = conn.CreateCommand();

			string sql =
				"SELECT " +
					"null as TABLE_CATALOG, " +
					"table_owner as TABLE_SCHEMA, " +
					"table_name as TABLE_NAME, " +
					"'TABLE' as TABLE_TYPE " +
				"FROM  iitables " +
				" WHERE system_use <> 'S' AND (TABLE_TYPE  IN ('T', 'P')) AND " +
						"TABLE_NAME NOT LIKE 'ii%' AND TABLE_NAME NOT LIKE 'II%' AND " +
						"TABLE_NAME NOT LIKE 'sys%' AND TABLE_NAME NOT LIKE 'SYS%' AND "
				//          TABLE_SCHEMA = <szOwner> AND         -- can't = 'SYSTEM'
				//          TABLE_NAME = <szTable>
				;

			sql =
				AddRestrictionsToSqlQueryText(
					sql, tablesRestrictionCollection, restrictionValues);

			cmd.CommandText = RemoveLastAND(sql) + " ORDER BY 2, 3";

			IDataReader rdr = cmd.ExecuteReader();

			object[] row = new Object[rdr.FieldCount];

			while (rdr.Read())
			{
				rdr.GetValues(row);
				row[1] = TrimDBName(row[1]);
				row[2] = TrimDBName(row[2]);
				dt.Rows.Add(row);
			} // end while through reader
			rdr.Close();

			return dt;
		}  // GetSchemaTables


		private static DataTable GetSchemaColumns(
			DbConnection conn,
			string[] restrictionValues)
		{

			int maxDecimalPrecision = DbmsConst.DBMS_PREC_MAX;

			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("TABLE_CATALOG",            typeof(String)),
				new DataColumn("TABLE_SCHEMA",             typeof(String)),
				new DataColumn("TABLE_NAME",               typeof(String)),
				new DataColumn("COLUMN_NAME",              typeof(String)),
				new DataColumn("ORDINAL_POSITION",         typeof(Int16)),
				new DataColumn("COLUMN_DEFAULT",           typeof(String)),
				new DataColumn("IS_NULLABLE",              typeof(String)),
				new DataColumn("DATA_TYPE",                typeof(String)),
				new DataColumn("CHARACTER_MAXIMUM_LENGTH", typeof(Int32)),
				new DataColumn("CHARACTER_OCTET_LENGTH",   typeof(Int32)),
				new DataColumn("NUMERIC_PRECISION",        typeof(Byte)),
				new DataColumn("NUMERIC_PRECISION_RADIX",  typeof(Int16)),
				new DataColumn("NUMERIC_SCALE",            typeof(Int32)),
				new DataColumn("DATETIME_PRECISION",       typeof(Int16)),
			};

			DataTable dt = CreateSchemaDataTable(
				"Columns", columnNames);

			// if user specifies catalog restriction, Ingres does not
			// support catalog.  So return empty datatable.
			if (restrictionValues[0] != null)
				return dt;

			// Get the connection specific information
			IngresConnection ingresConn = conn as IngresConnection;
			if (ingresConn != null)
			{
				Ingres.ProviderInternals.AdvanConnect advanConn =
					ingresConn.advanConnect;
				if (advanConn != null && advanConn.conn != null)
				{
					Ingres.ProviderInternals.DrvConn drvConn = advanConn.conn;

					maxDecimalPrecision = drvConn.max_dec_prec;
				}
			}


			DbCommand cmd = conn.CreateCommand();

			string sql =
				"SELECT  null, "                     /* TABLE_CATALOG   */ +
						"table_owner, "              /* TABLE_SCHEMA */    +
						"table_name, "               /* TABLE_NAME  */     +
						"column_name, "              /* COLUMN_NAME */     +
						"column_sequence, "          /* ORDINAL_POSITION  */ +
						"column_default_val, "       /* COLUMN_DEFAULT    */ +
						"column_nulls, "             /* IS_NULLABLE  */ +
						"column_datatype, "          /* DATA_TYPE (human readable) */ +
						"column_length, "            /* CHARACTER_MAXIMUM_LENGTH */ +
						"null, "                     /* CHARACTER_OCTET_LENGTH */ +
						"null, "                     /* NUMERIC_PRECISION   */ +
						"null, "                     /* NUMERIC_PRECISION_RADIX */ +
						"column_scale, "             /* NUMERIC_SCALE */ +
						"null "                      /* DATETIME_PRECISION   */ +
					"FROM iicolumns " +
					"WHERE "
				/*         table_owner = <szOwner> AND */
				/*         table_name  = <szTable> AND */
				/*         column_name = <szColumn>    */
				;

			sql =
				AddRestrictionsToSqlQueryText(
					sql, columnsRestrictionCollection, restrictionValues);

			cmd.CommandText = RemoveLastAND(sql) + " ORDER BY 2, 3, 5";

			IDataReader rdr = cmd.ExecuteReader();

			object[] row = new Object[rdr.FieldCount];
			string typeName;
			int    length;
			int    octet_length;
			string s;
			ProviderType providerType;

			while (rdr.Read())
			{
				rdr.GetValues(row);
				row[1] = TrimDBName(row[1]);      // TABLE_SCHEMA
				row[2] = TrimDBName(row[2]);      // TABLE_NAME
				row[3] = TrimDBName(row[3]);      // COLUMN_NAME
				row[4] = Convert.ToInt16(row[4]); // ORDINAL_POSITION
				row[5] = TrimDBName(row[5]);      // COLUMN_DEFAULT

				s      = TrimDBName(row[6]) as String;
				if (s != null  &&  s.ToUpperInvariant()[0] == 'Y')
					s = "YES";
				else
					s = "NO";
				row[6] = s;                       // IS_NULLABLE

				row[7] = TrimDBName(row[7]);      // DATA_TYPE
				typeName = row[7] as String;
				if (typeName != null)
				{
					// use lower case to match list in GetSchema("DataTypes")
					typeName = typeName.ToLowerInvariant();
					row[7] = typeName;
				}

				length = Math.Max(Convert.ToInt32(row[8]), 0);

				providerType =
					ProviderTypeMgr.GetProviderType(
						typeName, length);

				if (typeName == "integer" || // use specific subtypename
				    typeName == "float")
				{
					typeName = ProviderTypeMgr.GetDataTypeName(
						providerType).ToLowerInvariant();  // tinyint, smallint,...
					row[7] = typeName;            // DATA_TYPE
				}

				if (providerType == ProviderType.LongVarBinary ||
					providerType == ProviderType.LongVarChar)
					length = Int32.MaxValue;
				else
				if (providerType == ProviderType.LongNVarChar)
					length = Int32.MaxValue / sizeof(Char);

				if (providerType == ProviderType.NChar ||
					providerType == ProviderType.NVarChar  ||
					providerType == ProviderType.LongNVarChar)
					octet_length = sizeof(Char) * length;
				else
					octet_length = length;

				if (ProviderTypeMgr.hasPrecision(providerType))
				{
					row[8] = DBNull.Value;              // CHARACTER_MAXIMUM_LENGTH
					row[9] = DBNull.Value;              // CHARACTER_OCTET_LENGTH
					row[10] = Convert.ToByte(           // NUMERIC_PRECISION
						ProviderTypeMgr.ColumnSize(providerType, maxDecimalPrecision));
					row[11] = (Int16)10; ;              // NUMERIC_PRECISION_RADIX
					row[12] = Convert.ToInt32(row[12]); // NUMERIC_SCALE
					if (providerType == ProviderType.Double  ||
						providerType == ProviderType.Single)
							row[12] = DBNull.Value;

					if (providerType == ProviderType.DateTime  ||
						providerType == ProviderType.IngresDate)
					{
						row[11] = DBNull.Value;         // NUMERIC_PRECISION_RADIX
						row[13] = (Int16)3;             // DATETIME_PRECISION
					}
					else
						row[13] = DBNull.Value;
				}
				else
				{
					row[8] = length;            // CHARACTER_MAXIMUM_LENGTH
					row[9] = octet_length;      // CHARACTER_OCTET_LENGTH
					row[10]= DBNull.Value;      // NUMERIC_PRECISION
					row[11]= DBNull.Value;      // NUMERIC_PRECISION_RADIX
					row[12]= DBNull.Value;      // NUMERIC_SCALE
					row[13]= DBNull.Value;      // DATETIME_PRECISION
				}

				if (providerType == ProviderType.Boolean)
				{
					row[8] = DBNull.Value;      // CHARACTER_MAXIMUM_LENGTH
					row[9] = DBNull.Value;      // CHARACTER_OCTET_LENGTH
				}

				dt.Rows.Add(row);
			} // end while through reader
			rdr.Close();

			return dt;
		}  // GetSchemaColumns


		private static DataTable GetSchemaViews(
			DbConnection conn,
			string[] restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("TABLE_CATALOG", typeof(String)),
				new DataColumn("TABLE_SCHEMA",  typeof(String)),
				new DataColumn("TABLE_NAME",    typeof(String)),
				new DataColumn("TABLE_TYPE",    typeof(String)),
			};

			DataTable dt = CreateSchemaDataTable(
				"Views", columnNames);

			// if user specifies catalog restriction, Ingres does not
			// support catalog.  So return empty datatable.
			if (restrictionValues[0] != null)
				return dt;

			DbCommand cmd = conn.CreateCommand();

			string sql =
				"SELECT " +
					"null as TABLE_CATALOG, " +
					"table_owner as TABLE_SCHEMA, " +
					"table_name as TABLE_NAME, " +
					"'VIEW' as TABLE_TYPE " +
				"FROM  iitables " +
				" WHERE system_use <> 'S' AND (TABLE_TYPE  IN ('V')) AND " +
						"TABLE_NAME NOT LIKE 'ii%' AND TABLE_NAME NOT LIKE 'II%' AND " +
						"TABLE_NAME NOT LIKE 'sys%' AND TABLE_NAME NOT LIKE 'SYS%' AND "
				//          TABLE_SCHEMA = <szOwner> AND         -- can't = 'SYSTEM'
				//          TABLE_NAME = <szTable>
				;

			sql =
				AddRestrictionsToSqlQueryText(
					sql, tablesRestrictionCollection, restrictionValues);

			cmd.CommandText = RemoveLastAND(sql) + " ORDER BY 2, 3";

			IDataReader rdr = cmd.ExecuteReader();

			object[] row = new Object[rdr.FieldCount];

			while (rdr.Read())
			{
				rdr.GetValues(row);
				row[1] = TrimDBName(row[1]);
				row[2] = TrimDBName(row[2]);
				dt.Rows.Add(row);
			} // end while through reader
			rdr.Close();

			return dt;
		}  // GetSchemaViews


		private static DataTable GetSchemaForeignKeys(
			DbConnection conn,
			string[] restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("CollectionName",          typeof(String)),
				new DataColumn("NumberOfRestrictions",    typeof(Int32)),
				new DataColumn("NumberOfIdentifierParts", typeof(Int32)),
			};

			DataTable dt = CreateSchemaDataTable(
				"ForeignKeys", columnNames);

			//foreach (object[] row in values)
			//    dt.Rows.Add(row);

			return dt;
		}  // GetSchemaForeignKeys


		private static DataTable GetSchemaIndexes(
			DbConnection conn,
			string[] restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn(TABLE_CATALOG,      typeof(String)),
				new DataColumn(TABLE_SCHEMA,       typeof(String)),
				new DataColumn(TABLE_NAME,         typeof(String)),
				new DataColumn("NON_UNIQUE",       typeof(Int16)),
				new DataColumn("INDEX_QUALIFIER",  typeof(String)),
				new DataColumn("INDEX_NAME",       typeof(String)),
				new DataColumn("TYPE",             typeof(Int16)),
				new DataColumn("ORDINAL_POSITION", typeof(Int16)),
				new DataColumn(COLUMN_NAME,        typeof(String)),
				new DataColumn("ASC_OR_DSC",       typeof(String)),
			};

			DataTable dt = CreateSchemaDataTable(
				"Indexes", columnNames);

			// if user specifies catalog restriction, Ingres does not
			// support catalog.  So return empty datatable.
			if (restrictionValues[0] != null)
				return dt;

			DbCommand cmd = conn.CreateCommand();

			string sql =
			"SELECT null as TABLE_CATALOG, "                     +
				"i.base_owner as TABLE_SCHEMA, "                +
				"i.base_name as TABLE_NAME, "                   +
				"i.unique_rule, "        /* NON_UNIQUE      remap char8 to smallint*/ +
				"i.index_owner, "        /* INDEX_QUALIFIER */  +
				"i.index_name as INDEX_NAME, " /* INDEX_NAME */ +
				 "3, "                   /* TYPE  (SQL_INDEX_OTHER) */ +
				"ic.key_sequence, "      /* SEQ_IN_INDEX    */  +
				"ic.column_name, "       /* COLUMN_NAME */      +
				"ic.sort_direction "     /* COLLATION       remap char8 to char1*/ +
			"FROM iiindexes i, iiindex_columns ic " +
			"WHERE i.index_name = ic.index_name AND " +
				"i.index_owner = ic.index_owner AND ";
//				 TABLE_SCHEMA = <szOwner> AND
//				 TABLE_NAME   = <szTable> AND


			sql =
				AddRestrictionsToSqlQueryText(
					sql, indexesRestrictionCollection, restrictionValues);

			cmd.CommandText = RemoveLastAND(sql) + " ORDER BY 5, 6, 8" ;

			IDataReader rdr = cmd.ExecuteReader();

			object[] row = new Object[rdr.FieldCount];

			while (rdr.Read())
			{
				string s;
				object o;
				rdr.GetValues(row);
				row[1] = TrimDBName(row[1]);  // TABLE_SCHEMA
				row[2] = TrimDBName(row[2]);  // TABLE_NAME
				o      = TrimDBName(row[3]);  // NON-UNIQUE?
				if (o == null || o == DBNull.Value)
					row[3] = DBNull.Value;
				else
				{
					s = ((string)o).ToUpperInvariant();
					row[3] =(s == "U")?(Int16)(1):(Int16)(0);
				}
				row[4] = TrimDBName(row[4]);      // INDEX_QUALIFIER
				row[5] = TrimDBName(row[5]);      // INDEX_NAME
				row[6] = (Int16)(3);              // TYPE = SQL_INDEX_OTHER
				row[7] = Convert.ToInt16(row[7]); // ORDINAL_POSITION
				row[8] = TrimDBName(row[8]);      // COLUMN_NAME
				row[9] = TrimDBName(row[9]);      // ASC_OR_DSC?

				dt.Rows.Add(row);
			} // end while through reader
			rdr.Close();

			return dt;
		}  // GetSchemaIndexes


		private static DataTable GetSchemaProcedures(
			DbConnection conn,
			string[] restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("PROCEDURE_CATALOG", typeof(String)),
				new DataColumn("PROCEDURE_SCHEMA",  typeof(String)),
				new DataColumn("PROCEDURE_NAME",    typeof(String)),
			};

			DataTable dt = CreateSchemaDataTable(
				"Procedures", columnNames);

			// if user specifies catalog restriction, Ingres does not
			// support catalog.  So return empty datatable.
			if (restrictionValues[0] != null)
				return dt;

			DbCommand cmd = conn.CreateCommand();

			string sql =
				"SELECT " +
					"null, " +
					"procedure_owner, " +
					"procedure_name " +
				"FROM  iiprocedures " +
				" WHERE "
				//          PROCEDURE_OWNER = <szOwner> AND
				//          PROCEDURE_NAME  = <szTable>
				;

			sql =
				AddRestrictionsToSqlQueryText(
					sql, proceduresRestrictionCollection, restrictionValues);

			cmd.CommandText = RemoveLastAND(sql) + " ORDER BY 2, 3";

			IDataReader rdr = cmd.ExecuteReader();

			object[] row = new Object[rdr.FieldCount];

			while (rdr.Read())
			{
				rdr.GetValues(row);
				row[1] = TrimDBName(row[1]);
				row[2] = TrimDBName(row[2]);
				dt.Rows.Add(row);
			} // end while through reader
			rdr.Close();

			return dt;
		}  // GetSchemaProcedures


		private static DataTable GetSchemaProcedureColumns(
			DbConnection conn,
			string[] restrictionValues)
		{
			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("CollectionName",          typeof(String)),
				new DataColumn("NumberOfRestrictions",    typeof(Int32)),
				new DataColumn("NumberOfIdentifierParts", typeof(Int32)),
			};

			DataTable dt = CreateSchemaDataTable(
				"ProcedureColumns", columnNames);

			//foreach (object[] row in values)
			//    dt.Rows.Add(row);

			return dt;
		}  // GetSchemaProcedureColumns


		private static DataTable GetSchemaProcedureParameters(
			DbConnection conn,
			string[] restrictionValues)
		{

			int maxDecimalPrecision = DbmsConst.DBMS_PREC_MAX;

			DataColumn[] columnNames = new DataColumn[]
			{
				new DataColumn("PROCEDURE_CATALOG",        typeof(String)),
				new DataColumn("PROCEDURE_SCHEMA",         typeof(String)),
				new DataColumn("PROCEDURE_NAME",           typeof(String)),
				new DataColumn("COLUMN_NAME",              typeof(String)),
				new DataColumn("ORDINAL_POSITION",         typeof(Int16)),
				new DataColumn("COLUMN_DEFAULT",           typeof(String)),
				new DataColumn("IS_NULLABLE",              typeof(String)),
				new DataColumn("DATA_TYPE",                typeof(String)),
				new DataColumn("CHARACTER_MAXIMUM_LENGTH", typeof(Int32)),
				new DataColumn("CHARACTER_OCTET_LENGTH",   typeof(Int32)),
				new DataColumn("NUMERIC_PRECISION",        typeof(Byte)),
				new DataColumn("NUMERIC_PRECISION_RADIX",  typeof(Int16)),
				new DataColumn("NUMERIC_SCALE",            typeof(Int32)),
				new DataColumn("DATETIME_PRECISION",       typeof(Int16)),
				new DataColumn("INGRESTYPE",               typeof(IngresType)),
			};

			DataTable dt = CreateSchemaDataTable(
				"ProcedureParameters", columnNames);

			// if user specifies catalog restriction, Ingres does not
			// support catalog.  So return empty datatable.
			if (restrictionValues[0] != null)
				return dt;

			// Get the connection specific information
			IngresConnection ingresConn = conn as IngresConnection;
			if (ingresConn != null)
			{
				Ingres.ProviderInternals.AdvanConnect advanConn =
					ingresConn.advanConnect;
				if (advanConn != null && advanConn.conn != null)
				{
					Ingres.ProviderInternals.DrvConn drvConn = advanConn.conn;

					maxDecimalPrecision = drvConn.max_dec_prec;
				}
			}


			DbCommand cmd = conn.CreateCommand();

			string sql =
				"SELECT  null, "                     /* PROCEDURE_CATALOG */ +
						"procedure_owner, "          /* PROCEDURE_SCHEMA  */ +
						"procedure_name, "           /* PROCEDURE_NAME    */ +
						"param_name, "               /* COLUMN_NAME */     +
						"param_sequence, "           /* ORDINAL_POSITION  */ +
						"null, "                     /* COLUMN_DEFAULT    */ +
						"param_nulls, "              /* IS_NULLABLE  */ +
						"param_datatype, "           /* DATA_TYPE (human readable) */ +
						"param_length, "             /* CHARACTER_MAXIMUM_LENGTH */ +
						"null, "                     /* CHARACTER_OCTET_LENGTH */ +
						"null, "                     /* NUMERIC_PRECISION   */ +
						"null, "                     /* NUMERIC_PRECISION_RADIX */ +
						"param_scale, "              /* NUMERIC_SCALE */ +
						"null,"                      /* DATETIME_PRECISION   */ +
						"null "                      /* INGRESTYPE   */ +
					"FROM iiproc_params " +
					"WHERE "
				/*         procedure_owner = <szOwner> AND */
				/*         procedure_name  = <szProcedure> AND */
				/*         column_name = <szColumn>    */
				;

			sql =
				AddRestrictionsToSqlQueryText(
					sql, procedureParametersRestrictionCollection, restrictionValues);

			cmd.CommandText = RemoveLastAND(sql) + " ORDER BY 2, 3, 5";

			IDataReader rdr = cmd.ExecuteReader();

			object[] row = new Object[rdr.FieldCount];
			string typeName;
			int length;
			int octet_length;
			string s;
			ProviderType providerType;

			while (rdr.Read())
			{
				rdr.GetValues(row);
				row[1] = TrimDBName(row[1]);      // PROCEDURE_SCHEMA
				row[2] = TrimDBName(row[2]);      // PROCEDURE_NAME
				row[3] = TrimDBName(row[3]);      // COLUMN_NAME
				row[4] = Convert.ToInt16(row[4]); // ORDINAL_POSITION

				s = TrimDBName(row[6]) as String; // IS_NULLABLE
				if (s != null && s.ToUpperInvariant()[0] == 'Y')
					s = "YES";
				else
					s = "NO";
				row[6] = s;                       // IS_NULLABLE

				row[7] = TrimDBName(row[7]);      // DATA_TYPE
				typeName = row[7] as String;

				length = Math.Max(Convert.ToInt32(row[8]), 0);

				providerType =
					ProviderTypeMgr.GetProviderType(
						typeName, length);

				if (typeName == "INTEGER" || // use specific subtypename
				    typeName == "FLOAT")
				{
					typeName = ProviderTypeMgr.GetDataTypeName(
						providerType).ToUpperInvariant();  // tinyint, smallint,...
					row[7] = typeName;            // DATA_TYPE
				}

				if (providerType == ProviderType.LongVarBinary ||
					providerType == ProviderType.LongVarChar)
					length = Int32.MaxValue;
				else
					if (providerType == ProviderType.LongNVarChar)
						length = Int32.MaxValue / sizeof(Char);

				if (providerType == ProviderType.NChar ||
					providerType == ProviderType.NVarChar ||
					providerType == ProviderType.LongNVarChar)
					octet_length = sizeof(Char) * length;
				else
					octet_length = length;

				if (ProviderTypeMgr.hasPrecision(providerType))
				{
					row[8] = DBNull.Value;              // CHARACTER_MAXIMUM_LENGTH
					row[9] = DBNull.Value;              // CHARACTER_OCTET_LENGTH
					row[10] = Convert.ToByte(           // NUMERIC_PRECISION
						ProviderTypeMgr.ColumnSize(providerType, maxDecimalPrecision));
					row[11] = (Int16)10; ;              // NUMERIC_PRECISION_RADIX
					row[12] = Convert.ToInt32(row[12]); // NUMERIC_SCALE
					if (providerType == ProviderType.Double ||
						providerType == ProviderType.Single)
						row[12] = DBNull.Value;

					if (providerType == ProviderType.DateTime  ||
						providerType == ProviderType.IngresDate)
					{
						row[11] = DBNull.Value;         // NUMERIC_PRECISION_RADIX
						row[13] = (Int16)3;             // DATETIME_PRECISION
					}
					else
						row[13] = DBNull.Value;
				}
				else
				{
					row[8] = length;             // CHARACTER_MAXIMUM_LENGTH
					row[9] = octet_length;       // CHARACTER_OCTET_LENGTH
					row[10] = DBNull.Value;      // NUMERIC_PRECISION
					row[11] = DBNull.Value;      // NUMERIC_PRECISION_RADIX
					row[12] = DBNull.Value;      // NUMERIC_SCALE
					row[13] = DBNull.Value;      // DATETIME_PRECISION
				}

				row[14] = (IngresType)providerType;

				dt.Rows.Add(row);
			} // end while through reader
			rdr.Close();

			return dt;
		}  // GetSchemaProcedureParameters



		/// <summary>
		/// Create a DataTable with a specified tablename and columnnames.
		/// </summary>
		/// <param name="tableName"></param>
		/// <param name="columnNames"></param>
		/// <returns></returns>
		private static DataTable CreateSchemaDataTable(
			string tableName, DataColumn[] columnNames)
		{
			DataTable dt = new DataTable(tableName);
			dt.Locale = System.Globalization.CultureInfo.InvariantCulture;

			dt.Columns.AddRange(columnNames);

			return dt;
		}

		/// <summary>
		/// Remove the extraneous "AND" at the end of the SQL query.
		/// </summary>
		/// <param name="commandText"></param>
		/// <returns></returns>
		private static string RemoveLastAND(string commandText)
		{
			if (commandText == null  ||
				commandText.Length < 4)
				return commandText;

			if (commandText.EndsWith(" AND "))
				return commandText.Substring(0, commandText.Length-4);

			if (commandText.EndsWith(" AND"))
				return commandText.Substring(0, commandText.Length-3);

			if (commandText.Length < 6)
				return commandText;

			if (commandText.EndsWith(" WHERE "))
				return commandText.Substring(0, commandText.Length - 6);

			if (commandText.EndsWith(" WHERE"))
				return commandText.Substring(0, commandText.Length - 5);

			return commandText;
		}

		private static object TrimDBName(object obj)
		{
			if (obj == null ||
				obj == DBNull.Value)
				return DBNull.Value;

			if (obj.GetType() != typeof(String))
				return obj;

			return ((String)obj).TrimEnd();
		}

		private static string AddRestrictionsToSqlQueryText(
			string sql, RestrictionCollection restrictions, string[] restrictionValues)
		{

			for(int i = 0; i < restrictions.Restrictions.Length; i++)
			{
				Restriction restriction = restrictions.Restrictions[i];
				string restrictionValue = restrictionValues[i];
				if (restrictionValue == null)
					continue;
				string restrictionName =
					restriction.IngresCatalogColumnName != null ?
					restriction.IngresCatalogColumnName :
					restriction.Name;
				string valueQuoted = QuoteLiteral(restrictionValue);
				sql = sql + restrictionName + " = " + valueQuoted + " AND ";
			}  // end for loop through restrictions

			return sql;
		}

		private static string QuoteLiteral(string literal)
		{
			System.Text.StringBuilder sb =
				new System.Text.StringBuilder(literal.Length + 2);

			sb.Append("'");

			foreach(char c in literal)
			{
				if (c == '\'')  // if quote character, double it up
					sb.Append(c);
				sb.Append(c);
			}

			sb.Append("'");
			return sb.ToString();
		}

		private class RestrictionCollection
		{
			public string        Name;
			public int           NumberIdentifierParts;
			public Restriction[] Restrictions;

			public RestrictionCollection(
				String name)
			{
				this.Name = name;
				this.NumberIdentifierParts = 0;
				this.Restrictions = new Restriction[0];
			}

			public RestrictionCollection(
				String name, Restriction[] restrictions)
			{
				this.Name         = name;
				this.NumberIdentifierParts = restrictions.Length;
				this.Restrictions = restrictions;
			}

			public RestrictionCollection(
				String name, int numberIdentifierParts, Restriction[] restrictions)
			{
				this.Name         = name;
				this.NumberIdentifierParts = numberIdentifierParts;
				this.Restrictions = restrictions;
			}
		};

		private class Restriction
		{
			/// <summary>
			/// Column name for the restriction as known to the user.
			/// </summary>
			public string Name;
			/// <summary>
			/// Default restriction value if any.
			/// </summary>
			public string Default;
			/// <summary>
			/// Restriction order number.
			/// </summary>
			public int    Number;
			/// <summary>
			/// The internal Ingres catalog column name. e.g. "table_owner".
			/// </summary>
			public string IngresCatalogColumnName;

			public Restriction(
				String restrictionName,
				String restrictionDefault,
				int    restrictionNumber)
			{
				this.Name    = restrictionName;
				this.Default = restrictionDefault;
				this.Number  = restrictionNumber;
				this.IngresCatalogColumnName = null;
			}

			public Restriction(
				String restrictionName,
				String restrictionDefault,
				int    restrictionNumber,
				String ingresCatalogColumnName)
			{
				this.Name = restrictionName;
				this.Default = restrictionDefault;
				this.Number = restrictionNumber;
				this.IngresCatalogColumnName = ingresCatalogColumnName;
			}
		};

	} // class IngresMetaDataGetSchema

}  // namespace
