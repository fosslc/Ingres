/*
** Copyright (c) 2003, 2009 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.Data;
using System.Text;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: catalog.cs
	**
	** Description:
	**	Manages the bookkeeping for catalog cache info on tables and columns.
	**
	** History:
	**	30-Jan-03 (thoda04)
	**	    Created.
	**	13-Apr-04 (thoda04)
	**	    Removed IDbConnection field; use AdvanConnect instead.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 1-jun-09 (thoda04) B122127
	**	    If no primary key specified, default to physical underlying key.
	*/

	/*
	** Name: Catalog
	**
	** Description:
	**	Class providing access to Table and Column definitions in the catalog.
	**
	** Public Methods:
	**	FindMissingSchemaName	Search catalog for schema (table_owner).
	**	GetCatalogTable      	Find the schema.table in the in-core catalog.
	**
	** Internal Classes:
	**	Table 	Describes the catalog definition of a table and a list of columns.
	**	Column	Describes the catalog definition of a column.
	**
	*/
	
	/// <summary>
	/// Catalog (in-core cache) management of tables and columns.
	/// </summary>
	public sealed class Catalog
	{
		/*
		** Query types.
		*/
		private UserSearchOrder userSearchOrder;
		private AdvanConnect     AdvanConnect;
		private System.Collections.Hashtable tables;

		private ArrayList userTablesList;
		private ArrayList userViewsList;
		private ArrayList allTablesList;
		private ArrayList allViewsList;


		internal Catalog(AdvanConnect advanConnect)
		{
			// associate with the pooled connection
			this.AdvanConnect = advanConnect;

			tables = new System.Collections.Hashtable();
		}

		/*
		** GetDefaultSchema
		**
		** History:
		**	17-Aug-06 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Return the current_user as the default schema name.
		/// </summary>
		/// <returns></returns>
		public string GetDefaultSchema()
		{
			if (userSearchOrder == null)  // build current_user, dba, $ingres
				userSearchOrder = new UserSearchOrder(AdvanConnect);
			if (userSearchOrder == null || userSearchOrder.Count == 0)
				return null;

			return (string)(userSearchOrder[0]);
		}


		/*
		** FindMissingSchemaName
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Search catalog for schema name (table_owner) for a
		/// table name reference that does not contain the schema qualification.
		/// </summary>
		public string FindMissingSchemaName(string tableName)
		{
			if (userSearchOrder == null)  // build current_user, dba, $ingres
				userSearchOrder = new UserSearchOrder(AdvanConnect);
			if (userSearchOrder == null  ||  userSearchOrder.Count == 0)
				return null;

			if (tableName == null  ||  tableName.Length == 0)
				return null;

			if (userSearchOrder.Count == 0)  // return if no users
				return null;                 // in list to search on
				                             // (should never happen)

			// build the query to send to the database catalog
			IDbCommand cmd = AdvanConnect.Connection.CreateCommand();

			StringBuilder sb = new StringBuilder(
				"SELECT DISTINCT table_owner FROM iitables " +
					"WHERE table_name = ? AND (", 100);

			IDataParameter parm = cmd.CreateParameter();
			parm.Value  = tableName;
			parm.DbType = DbType.AnsiString;  // don't send Unicode
			cmd.Parameters.Add(parm);

			int i = 0;
			foreach(string user in userSearchOrder)
			{
				if (i != 0)
					sb.Append(" OR ");
				i++;

				sb.Append("table_owner = ?");
				parm = cmd.CreateParameter();
				parm.Value  = user;
				parm.DbType = DbType.AnsiString;  // don't send Unicode
				cmd.Parameters.Add(parm);
			}
			sb.Append(")");

			cmd.CommandText = sb.ToString();
				// SELECT table_owner FROM iitables WHERE table_name = <tablename>
				//   AND (table_owner = <user> OR table_owner = <dbaname> OR 
				//        table_owner = <$ingres>)

			// send the query to the database catalog to get possible owners
			IDataReader rdr = null;
			int    ownerIndex;
			int    ownerIndexBest = 9;
			string tableOwner;

			try
			{
				rdr = cmd.ExecuteReader();  // read the owners with same tablename
				while (rdr.Read())  // process list of owners
				{
					if (rdr.IsDBNull(0))
						continue;
					tableOwner = rdr.GetString(0).TrimEnd();
					ownerIndex = userSearchOrder.IndexOf(tableOwner);
					if (ownerIndex != -1  &&  ownerIndex < ownerIndexBest)
						ownerIndexBest = ownerIndex;  // a better user found
				}
				rdr.Close();

				if (ownerIndexBest != 9)
					return (string)userSearchOrder[ownerIndexBest];
			}
			catch (SqlEx)
			{
				throw;
			}
			finally
			{
				if (rdr != null)
					rdr.Close();
			}

			return null;  // no user found in userSearchOrder for named table
		}  // FindMissingSchemaName

		/*
		** GetAllCatalogTablesAndViews
		**
		** History:
		**	23-Jun-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Get a list of all table or view names, qualified by
		/// "User Tables", "User Views", "All Tables", "All Views".
		/// </summary>
		/// <param name="qualification"></param>
		/// <returns>list of tables or views requested</returns>
		public ArrayList GetAllCatalogTablesAndViews(string qualification)
		{
			if (qualification == null)  // if qualificatin is unknown then
				return new ArrayList(); //   return empty list as safety check

			ArrayList tablesList;  // pointer to tables list we are building
			ArrayList viewsList;   // pointer to views list we are building

			string schemaName = null;
			string readSchemaName;
			string readTableName;
			string readType;

			qualification = qualification.ToUpper(
				System.Globalization.CultureInfo.InvariantCulture);

			switch (qualification)
			{
				case "USER TABLES":
				{
					if (this.userTablesList != null)  // return if already built
						return this.userTablesList;

					tablesList = this.userTablesList = new ArrayList();
					viewsList  = this.userViewsList  = new ArrayList();

				if (userSearchOrder == null)  // build current_user, dba, $ingres
					userSearchOrder = new UserSearchOrder(AdvanConnect);
				if (userSearchOrder != null  ||  userSearchOrder.Count > 0)
					schemaName = (string)this.userSearchOrder[0];

				break;
				}

				case "USER VIEWS":
				{
					if (this.userViewsList != null)  // return if already built
						return this.userViewsList;

					tablesList = this.userTablesList = new ArrayList();
					viewsList  = this.userViewsList  = new ArrayList();

					if (userSearchOrder == null)  // build current_user, dba, $ingres
						userSearchOrder = new UserSearchOrder(AdvanConnect);
					if (userSearchOrder != null  ||  userSearchOrder.Count > 0)
						schemaName = (string)this.userSearchOrder[0];

					break;
				}

				case "ALL TABLES":
				{
					if (this.allTablesList != null)  // return if already built
						return this.allTablesList;

					tablesList = this.allTablesList = new ArrayList();
					viewsList  = this.allViewsList  = new ArrayList();

					break;
				}

				case "ALL VIEWS":
				{
					if (this.allViewsList != null)  // return if already built
						return this.allViewsList;

					tablesList = this.userTablesList = new ArrayList();
					viewsList  = this.userViewsList  = new ArrayList();

					break;
				}

				default:
					return new ArrayList(); // return empty list as safety check
			}  // end switch

			// We need to build the tables list and views list.
			// We distinguish between user and all for performance reasons
			// since all tables/views could be several thousand entries vs.
			// a couple dozen user tables/views.
			// We do batch table list and view list building together since
			// if the user is interested in a table list, we assume they are
			// interested in a view list on the next mouse click.  This
			// eliminates a second query.

			// get a list of catalog columns for the specified schema and table
			// (use tilde to avoid ambiguity with "." in delimited identifiers
			// when building the key).
			IDbCommand cmd = AdvanConnect.Connection.CreateCommand();
			IDataParameter parm;

			StringBuilder sb = new StringBuilder(
				"SELECT DISTINCT table_owner, table_name, table_type " +
					"FROM iitables " +
					"WHERE " +
					"system_use <> 'S' AND " +
					"table_type in ('T','V') AND " +
					"table_name NOT LIKE 'ii%' AND " +
					"table_name NOT LIKE 'II%' "
					, 200);

			if (schemaName != null)  // "User Tables" or "User Views"
			{
				sb.Append("AND table_owner = ? ");  // qualify by user

				parm = cmd.CreateParameter();

				parm.Value  = schemaName;
				parm.DbType = DbType.AnsiString;  // don't send Unicode
				cmd.Parameters.Add(parm);

			}  // end if schemaName != null

			sb.Append("ORDER BY 1, 2");

			cmd.CommandText = sb.ToString();

			// send the query to the database catalog to get columns
			IDataReader rdr = null;

			try
			{
				// read the table/view names from the catalog
				rdr = cmd.ExecuteReader();
				while (rdr.Read())  // process list of owners
				{
					if (rdr.IsDBNull(0)  ||
						rdr.IsDBNull(1)  ||
						rdr.IsDBNull(2))  // skip table/view if somehow null
						continue;

					readSchemaName = rdr.GetString(0).TrimEnd();
					readTableName  = rdr.GetString(1).TrimEnd();
					readType       = rdr.GetString(2).TrimEnd();

					Catalog.Table  catTable =
						new Catalog.Table(readSchemaName, readTableName);

					if (readType == "T")
						tablesList.Add(catTable);  // add table to list
					else
						viewsList.Add(catTable);  // add view to list

				}  // end while loop through tables/views in catalog
			}
			catch (SqlEx /*ex*/)
			{
				//Console.WriteLine(ex);
				throw;
			}
			finally
			{
				if (rdr != null)
					rdr.Close();
			}

			// The right lists are built now.
			// Call ourselves recursively to return the desired list.
			return GetAllCatalogTablesAndViews(qualification);
		}  // GetAllCatalogTablesAndViews


		
		/*
		** GetCatalogTable
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Find the schemaname.tablename in the in-core catalog.
		/// Add if not found and build its list of columns.
		/// </summary>
		public Catalog.Table GetCatalogTable(MetaData.Table table)
		{
			if (table            == null  || // safety check
				table.TableName  == null  || // if table name is missing or
				table.SchemaName == null)    // if schema is unknown then
				return null;    //   return "table doesn't exist'

			// get a list of catalog columns for the specified schema and table
			// (use tilde to avoid ambiguity with "." in delimited identifiers
			// when building the key).
			string key = table.SchemaName + "^" + table.TableName;

			Catalog.Table  catTable = tables[key] as Catalog.Table;
			Catalog.Column catColumn;

			if (catTable != null)
				return catTable;

			catTable = new Catalog.Table(table.SchemaName, table.TableName);

			IDbCommand cmd = AdvanConnect.Connection.CreateCommand();
			IDataParameter parm;

			cmd.CommandText =
				"SELECT DISTINCT column_name, key_sequence, column_sequence " +
				"FROM iicolumns " +
				"WHERE table_owner = ? AND table_name = ? " +
				"ORDER BY column_sequence";

			parm = cmd.CreateParameter();
			parm.Value  = table.SchemaName;
			parm.DbType = DbType.AnsiString;  // don't send Unicode
			cmd.Parameters.Add(parm);

			parm = cmd.CreateParameter();
			parm.Value  = table.TableName;
			parm.DbType = DbType.AnsiString;  // don't send Unicode
			cmd.Parameters.Add(parm);

			// send the query to the database catalog to get columns
			IDataReader rdr = null;

			try
			{
				// read the columns of table from the catalog
				rdr = cmd.ExecuteReader();
				while (rdr.Read())  // process list of owners
				{
					if (rdr.IsDBNull(0))  // skip column if somehow null
						continue;
					catColumn = new Catalog.Column();
					catColumn.SchemaName = table.SchemaName;
					catColumn.TableName  = table.TableName;
					catColumn.ColumnName = rdr.GetString(0).TrimEnd();
					// KeySequence may be used later for PrimaryKeySequence
					if (rdr.IsDBNull(1))
						catColumn.KeySequence = 0;
					else
					{
							Object obj = rdr.GetValue(1); // int32 or Oracle NUMERIC float
						catColumn.KeySequence = Convert.ToInt32(obj);
					}


					if (catTable.Columns == null)
						catTable.Columns = new ArrayList();
					catTable.Columns.Add(catColumn);  // add column to list
				}  // end while loop through columns in catalog
			}
			catch (SqlEx /*ex*/)
			{
				//Console.WriteLine(ex);
				throw;
			}
			finally
			{
				if (rdr != null)
					rdr.Close();
			}

			if (catTable.Columns == null)  // if no columns found
				return null;               //   return "table doesn't exist'

			GetCatalogTableIndexes(catTable);    // mark the unique columns
			GetCatalogTablePrimaryKey(catTable); // mark primary key columns
			tables[key] = catTable;

			return catTable;  // return a MetaData.Table with the Columns
		}  // GetCatalogTable

		/*
		** GetCatalogTableIndexes
		** 
		** Description:
		**	Find each column that is unique, is the only column in this index,
		**	and is not-null.
		**
		** History:
		**	07-Feb-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Fill in the unique index information for the table's columns.
		/// </summary>
		public Catalog.Table GetCatalogTableIndexes(Catalog.Table catTable)
		{
			ArrayList indexes = new ArrayList();  // list of unique indexes
			Catalog.Table  index;
			Catalog.Column indexColumn;

			if (catTable            == null  || // safety check
				catTable.TableName  == null  || // if table name is missing or
				catTable.SchemaName == null)    // if schema is unknown then
				return catTable;    //   just return return catTable as-is

			IDbCommand cmd = AdvanConnect.Connection.CreateCommand();
			IDataParameter parm;

			cmd.CommandText =
				"SELECT DISTINCT ic.column_name, ic.index_name " + 
				"FROM iicolumns c, iiindex_columns ic, iiindexes i " +
				"WHERE " +
				"i.base_owner = ? AND i.base_name = ? AND " +
				"c.table_name = i.base_name AND " +
				"c.table_owner = i.base_owner AND " +
				"ic.index_name = i.index_name AND " +
				"ic.index_owner = i.index_owner AND " +
				"ic.column_name = c.column_name AND " +
				"i.unique_rule  = 'U' AND " +
				"c.column_nulls  <> 'Y' ";
			//----------------------

			parm = cmd.CreateParameter();
			parm.Value  = catTable.SchemaName;
			parm.DbType = DbType.AnsiString;  // don't send Unicode
			cmd.Parameters.Add(parm);

			parm = cmd.CreateParameter();
			parm.Value  = catTable.TableName;
			parm.DbType = DbType.AnsiString;  // don't send Unicode
			cmd.Parameters.Add(parm);

			// send the query to the database catalog to get columns
			IDataReader rdr = null;

			try
			{
				// read the columns of table from the catalog
				rdr = cmd.ExecuteReader();
				while (rdr.Read())  // process list of columns
				{
					if (rdr.IsDBNull(0)  ||  // skip if somehow null
						rdr.IsDBNull(1))
						continue;
					String indexName    = rdr.GetString(1).TrimEnd();
					index = null;
					foreach(Catalog.Table indexSearch in indexes)
					{
						if (indexSearch.TableName.Equals(indexName))
						{
							index = indexSearch;
							break;
						}
					}  // end loop through existing indexes

					if (index == null)  // index is not in our local list yet
					{
						index = new Catalog.Table(indexName);  // add to list
						indexes.Add(index);
					}

					indexColumn = new Catalog.Column();
					//	indexColumn.SchemaName = index.SchemaName;  // not needed
					//	indexColumn.TableName  = index.TableName;   // not needed
					indexColumn.ColumnName = rdr.GetString(0).TrimEnd();


					if (index.Columns == null)  // if 1st time, build col list
						index.Columns = new ArrayList();
					index.Columns.Add(indexColumn);  // add column to list
				}  // end while loop reading through columns in catalog
			}
			catch (SqlEx) // ex)
			{
				// Console.WriteLine(ex);
				throw;
			}
			finally
			{
				if (rdr != null)
					rdr.Close();
			}


			// At this point: indexes->index(Table)->indexColumn(Column)
			foreach(Catalog.Table indexSearch in indexes)
			{
				if (indexSearch.Columns == null  ||    // skip indexes with
					indexSearch.Columns.Count != 1)    // multiple columns
					continue;

				indexColumn = (Catalog.Column)(indexSearch.Columns[0]);
				string indexColumnName = indexColumn.ColumnName;
				// match up the index column to the caller's table column
				foreach (Catalog.Column col in catTable.Columns)
				{
					if (indexColumnName.Equals(col.ColumnName))
					{
						col.IsUnique = true;  // mark Catalog.Column as unique
						break;
					}
				}  // end loop thru table's columns
			}  // end loop through indexes

			return catTable;  // return a MetaData.Table with the Columns
		}  // GetCatalogTableIndexes

		/*
		** GetCatalogTablePrimaryKey
		** 
		** Description:
		**	Find the fields that make up the table's primary key.
		**
		** History:
		**	10-Feb-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Find the fields that make up the table's primary key.
		/// </summary>
		public Catalog.Table GetCatalogTablePrimaryKey(Catalog.Table catTable)
		{
			string sql_lvl = AdvanConnect.conn.dbCaps.getDbCap( "COMMON/SQL_LEVEL" );
			int    cs_lvl  = 0;
			int    key_id;
			int    key_id_prior = -1;
			Object obj;

			if ( sql_lvl != null )  // get the Ingres or Gateway support level
				try { cs_lvl = Int32.Parse( sql_lvl ); }
				catch( Exception /*ignore*/ ) {};

			IDbCommand cmd = AdvanConnect.Connection.CreateCommand();
			IDataParameter parm;

			parm = cmd.CreateParameter();
			parm.Value  = catTable.SchemaName;
			parm.DbType = DbType.AnsiString;  // don't send Unicode
			cmd.Parameters.Add(parm);

			parm = cmd.CreateParameter();
			parm.Value  = catTable.TableName;
			parm.DbType = DbType.AnsiString;  // don't send Unicode
			cmd.Parameters.Add(parm);

			IDataReader rdr = null;
			bool PrimaryKeyFound   = false;

			// if Ingres server and iikeys is supported
			if (AdvanConnect.conn.is_ingres  &&  cs_lvl > 601)
			{
				cmd.CommandText =
					"SELECT DISTINCT k.column_name, k.key_position " +
					"FROM iikeys k, iiconstraints c " +
					"WHERE k.schema_name = ? AND k.table_name = ? AND " +
					"c.constraint_type = 'P'  AND " +
					"k.constraint_name = c.constraint_name ";

				// send the query to the database catalog to get keys
				rdr = null;

				try
				{
					// read the primary key columns of table from iikeys
					rdr = cmd.ExecuteReader();
					while (rdr.Read())  // process list of columns
					{
						if (rdr.IsDBNull(0))  // skip if somehow null
							continue;
						String columnName    = rdr.GetString(0).TrimEnd();
						// find the primary key column in the table's columns
						foreach(Catalog.Column col in catTable.Columns)
						{
							if (col.ColumnName.Equals(columnName))
							{
								if (rdr.IsDBNull(1))
									col.PrimaryKeySequence = 0;
								else
								{
									obj = rdr.GetValue(1); // int32 or Oracle NUMERIC float
									col.PrimaryKeySequence = Convert.ToInt32(obj);
									PrimaryKeyFound   = true;
								}
								break;
							}
						}  // end loop through columns
					}  // end while loop reading through columns in catalog
				}
				catch (SqlEx) // ex)
				{
					//Console.WriteLine(ex);
					throw;
				}
				finally
				{
					if (rdr != null)
						rdr.Close();
				}

				if (PrimaryKeyFound)
					return catTable;   // no need to look further
			}  // end if Ingres server


			// Primary key not found in iikeys; try iialt_columns.
			// Use the primary key or first unique key.

			cmd.CommandText =
				"SELECT DISTINCT k.column_name, k.key_sequence, k.key_id " +
				"FROM iialt_columns k " +
				"WHERE table_owner = ? AND table_name = ? AND " +
				"key_sequence <> 0 " +
				"ORDER BY 3";

			// send the query to the database catalog to get keys
			rdr = null;

			try
			{
				// read the first unique key columns of table from iialt_columns
				rdr = cmd.ExecuteReader();
				while (rdr.Read())  // process list of columns
				{
					if (rdr.IsDBNull(0))  // skip if somehow null
						continue;

					if (rdr.IsDBNull(2))
						key_id = 0;
					else
					{
						obj = rdr.GetValue(2); // int32 or Oracle NUMERIC float
						key_id = Convert.ToInt32(obj);
					}
					if (PrimaryKeyFound  && key_id != key_id_prior)
						break;  // break out if found new index key

					String columnName    = rdr.GetString(0).TrimEnd();
					// find the primary key column in the table's columns
					foreach(Catalog.Column col in catTable.Columns)
					{
						if (col.ColumnName.Equals(columnName))
						{
							if (rdr.IsDBNull(1))
								col.PrimaryKeySequence = 0;
							else
							{
								obj = rdr.GetValue(1); // int32 or Oracle NUMERIC float
								col.PrimaryKeySequence = Convert.ToInt32(obj);
								PrimaryKeyFound = true;
								key_id_prior = key_id;  // save key_id of first key
							}
							break;  // break out of table's columns search
						}
					}  // end loop through columns
				}  // end while loop reading through columns in catalog
			}
			catch (SqlEx) // ex)
			{
				//Console.WriteLine(ex);
				throw;
			}
			finally
			{
				if (rdr != null)
				{
					cmd.Cancel();  // cancel the remainder to avoid spurious msg
					rdr.Close();
				}
			}

			if (PrimaryKeyFound)
				return catTable;

			// Primary key not found in iikeys nor iialt_columns
			// Fall back to trying to use the physical underlying key
			foreach (Catalog.Column col in catTable.Columns)
			{
				col.PrimaryKeySequence = col.KeySequence;
			}

			return catTable;
		}  // GetCatalogTablePrimaryKey

		/// <summary>
		/// Refresh the tables and views list for any new items
		/// by clear out the cache of lists.
		/// </summary>
		public void Refresh()
		{
			userTablesList = null;
			userViewsList  = null;
			allTablesList  = null;
			allViewsList   = null;
		}  // Refresh


		/*
		** Table class
		**
		** Description:
		**	Describes the table definition from the SQL catalog.
		**
		**	Properties:
		**	SchemaName
		**	TableName
		**	Columns   	The ArrayList of columns for the table
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Table definition from the catalog.
		/// </summary>
		public sealed class Table : MetaData.DbTableDefinition
		{
			/// <summary>
			/// Constructor for a new empty table definition.
			/// </summary>
			public Table()
			{
			}

			/// <summary>
			/// Constructor for a new table definition for a given table name.
			/// </summary>
			/// <param name="tableName"></param>
			public Table(string  tableName)
			{
				SchemaName = null;
				TableName  = tableName;
				Columns    = null;
			}

			/// <summary>
			/// Constructor for a new table definition for a
			/// given schema name and table name.
			/// </summary>
			/// <param name="schemaName"></param>
			/// <param name="tableName"></param>
			public Table(string  schemaName, string  tableName)
			{
				SchemaName = schemaName;
				TableName  = tableName;
				Columns    = null;
			}

			/// <summary>
			/// Constructor for a new table definition for a
			/// given schema name, table name, and column list.
			/// </summary>
			/// <param name="schemaName"></param>
			/// <param name="tableName"></param>
			/// <param name="columns"></param>
			public Table(string  schemaName, string  tableName, ArrayList columns)
			{
				SchemaName = schemaName;
				TableName  = tableName;
				Columns    = columns;
			}

			private ArrayList _columns;
			/// <summary>
			/// Return the columns associated with the table.
			/// </summary>
			public  ArrayList  Columns
			{
				get { return _columns; }
				set { _columns = value;}
			}

			/// <summary>
			/// Clear the names from the table definition.
			/// </summary>
			public override void ClearNames()
			{
				TableName  = null;
				SchemaName = null;
				Columns    = null;
			}

			/// <summary>
			/// Return table name as qualified name.
			/// </summary>
			/// <returns></returns>
			public override string ToString()
			{
				return base.ToString();
			}
		}  // Table

		/*
		** Column class
		**
		** Description:
		**	Describes the column definitions from the SQL catalog.
		**
		**	Properties:
		**	SchemaName
		**		The schema name, stripped of quotes and case
		**	TableName
		**		The table name, stripped of quotes and case
		**	ColumnName
		**		The column name, stripped of quotes and case
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Column definition from the catalog.
		/// </summary>
		public sealed class Column : MetaData.DbColumnDefinition
		{
			/// <summary>
			/// Constructor for new column definition.
			/// </summary>
			public Column()
			{
			}

			/// <summary>
			/// Constructor for new column definition with
			/// given schema and table.
			/// </summary>
			/// <param name="schemaName"></param>
			/// <param name="tableName"></param>
			public Column(string schemaName, string tableName)
			{
				SchemaName = schemaName;
				TableName  = tableName;
			}

			private int _primaryKeySequence; // = 0;
			internal  int  PrimaryKeySequence
			{
				get { return _primaryKeySequence; }
				set { _primaryKeySequence = value;}
			}

			private  int _keySequence; // = 0;
			internal int  KeySequence
			{
				get { return _keySequence; }
				set { _keySequence = value;}
			}

			private bool _isUnique; // = false;
			internal  bool  IsUnique
			{
				get { return _isUnique; }
				set { _isUnique = value;}
			}

		}  // class Column

	}  // class Catalog


	class UserSearchOrder : ArrayList
	{
		private AdvanConnect     AdvanConnect;

		public UserSearchOrder(AdvanConnect advanConnect)
		{
			AdvanConnect = advanConnect;
			bool isGateway = false;
			string Username        = null;
			string DBAname         = null;
			string SystemOwnername = "$ingres";

			if (!advanConnect.conn.is_ingres)
			{
			string str;
				if ((str = AdvanConnect.conn.dbCaps.getDbCap("DBMS_TYPE")) != null)
				{
					str = str.ToUpper(
						System.Globalization.CultureInfo.InvariantCulture);
					// certain gateways don't have iidbconstants(system_owner)
					if (str.Equals("VSAM")  ||
						str.Equals("IMS"))
						isGateway = true;
				}
			}

			IDbCommand cmd = AdvanConnect.Connection.CreateCommand();

			if (isGateway)
				cmd.CommandText =
				"SELECT user_name, dba_name, dba_name FROM iidbconstants";
			else // Ingres and other full-service gateways
				cmd.CommandText =
				"SELECT user_name, dba_name, system_owner FROM iidbconstants";

			// send the query to the database catalog to get columns
			IDataReader rdr = null;

			try
			{
				// read the user search order
				rdr = cmd.ExecuteReader();
				while (rdr.Read())  // dummy loop for break convenience
				{
					if (!rdr.IsDBNull(0))
						Username        = rdr.GetString(0).TrimEnd();
					if (!rdr.IsDBNull(1))
						DBAname         = rdr.GetString(1).TrimEnd();
					if (!rdr.IsDBNull(2))
						SystemOwnername = rdr.GetString(2).TrimEnd();
					break;  // read just the first row
				}  // end dummy while loop
			}
			catch (SqlEx /*ex*/)
			{
				//Console.WriteLine(ex);
				throw;
			}
			finally
			{
				if (rdr != null)
				{
					cmd.Cancel();  // cancel the remainder to avoid spurious msg
					rdr.Close();   // close the data reader
				}
			}

			// eliminate duplicates
			if (Username != null)
			{
				if (DBAname != null)
				{
					if (DBAname.Equals(Username))
						DBAname = null;
				}
				if (SystemOwnername != null)
				{
					if (SystemOwnername.Equals(Username))
						SystemOwnername = null;
				}
			}
			if (DBAname != null)
			{
				if (SystemOwnername != null)
				{
					if (SystemOwnername.Equals(DBAname))
						SystemOwnername = null;
				}
			}

			if (Username        != null)
				base.Add(Username);
			if (DBAname         != null)
				base.Add(DBAname);
			if (SystemOwnername != null)
				base.Add(SystemOwnername);
		}
	}  // class UserSearchOrder

}  // namespace
