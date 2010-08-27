/*
** Copyright (c) 2003, 2010 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.Text;
using Ingres.Utility;

namespace Ingres.ProviderInternals
{
	/*
	** Name: metadata.cs
	**
	** Description:
	**	Manages the bookkeeping for tables and columns of a SELECT statement.
	**
	** History:
	**	27-Jan-03 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	13-Oct-05 (thoda04) B115418
	**	    CommandBehavior.SchemaOnly should not call database and
	**	    needs to return an empty RSLT but with non-empty RSMD metadata.
	**	30-sep-08 (thoda04)
	**	    ExpandSelectStarColumnReference's "SELECT *"  must insert columns
	**	    from all tables before removing * reference.  Bug 120972.
	**	20-oct-08 (thoda04)
	**	    Accept "FIRST nn" and "TOP nn" in a SELECT statement.  Bug 121066.
	**	12-oct-09 (thoda04)
	**	    GetSchemaTable() does not return BaseTableName
	**	    if table name reference is in the JOIN clause.  Bug 122652.
	**	19-Jan-10 (thoda04)  B124115
	**	    Added slash-star comment support to ScanSqlStatement().
	*/

	/*
	** Name: MetaData
	**
	** Description:
	**	Class providing Table and Column name management of a SELECT statement.
	**
	**  Public Methods:
	**
	**	FindCatalogColumnForColumn
	**		Find the catalog column that belongs to this column.
	**	WrapInQuotesIfEmbeddedSpaces
	**		Wrap the identifier in quotes if it contains special characters.
	**
	**  Private Methods:
	**
	**	BuildCatalogTableInfoForSELECTStatement
	**		Resolve missing schema names in the FROM table list.
	**	BuildKeyInfoForSELECTStatement
	**		Build the KeyInfo metadata for CommandBehavior.KeyInfo.
	**	BuildTableColumnInfoForSELECTStatement
	**		Scan the SELECT statment and build the column and table names lists.
	**	ExpandSelectStarColumnReference
	**		Expand the "*" column reference in a SELECT *
	**		to all of the columns defined in the table or tables.
	**	FindTableReferenceForColumn
	**		Find the FROM Table reference that belongs to this column.
	**	IsIdentifier
	**		True if regular or delimited identifer.
	**	ScanPastParenthesisGroup
	**		Scan past the opening parenthesis/bracket and
	**		find token after closing paren/bracket.
	**	ScanSqlStatement
	**		Scan the SQL statment and build tokens.
	**	StripQuotesAndCorrectCase
	**		Strip the quotes from the delimited identifier
	**		and normalize the case to the database's working case.
	**
	*/

	/// <summary>
	/// Metadata management of tables and columns.
	/// </summary>
	public sealed class MetaData
	{
		private Catalog catalog;
		private string          DelimitedIdentifierCase = "LOWER";
		private string          RegularIdentifierCase   = "LOWER";
		private ArrayList       upperList;


		/// <summary>
		/// Constructor for the MetaData definition.
		/// </summary>
		/// <param name="cmdText"></param>
		public MetaData( string cmdText ) :
				this(null, cmdText, false)
		{
		}

		internal MetaData(
			AdvanConnect advanConnect, string cmdText, bool expandSelectStar)
		{
			_advanConnect = advanConnect;

			if (AdvanConnect != null  &&  // if connection is Closed, just return
				!AdvanConnect.isClosed())
			{
				string str;

				if ((str = AdvanConnect.conn.dbCaps.getDbCap(
						"DB_DELIMITED_CASE")) != null)
					DelimitedIdentifierCase = str;  // "LOWER", "UPPER", or "MIXED"

				if ((str = AdvanConnect.conn.dbCaps.getDbCap(
						"DB_NAME_CASE")) != null)
					RegularIdentifierCase = str;
			}
			else
				AdvanConnect = null;

			try
			{
				// Scan the SELECT statment,
				// set the CmdTokenList token list for the SQL statement, and
				// build the column and table names lists.
				BuildTableColumnInfoForSELECTStatement(cmdText);
			}
			catch(FormatException)  // catch invalid syntax
			{}

			// At this point, the table and column lists just 
			// have the names only.
			// If no tables could be scanned 
			// from the SELECT statement (or no SELECT at all) then give up.
			if (Tables  == null  ||  Tables.Count  == 0)
				return;

			if (AdvanConnect == null)  // if no Connection to get catalog
				return;              // then just return

			// get the catalog to search for
			// table, column, and key information later.
			if (AdvanConnect.Catalog == null)
				AdvanConnect.Catalog =
					new Catalog(AdvanConnect);
			catalog = AdvanConnect.Catalog;

			// Resolve missing schema names in the FROM table list.
			BuildCatalogTableInfoForSELECTStatement();

			// Build the KeyInfo metadata
			BuildKeyInfoForSELECTStatement(expandSelectStar);

		}  // MetaData constructor


		private  AdvanConnect _advanConnect;
		internal AdvanConnect  AdvanConnect
		{
			get { return _advanConnect; }
			set { _advanConnect = value;}
		}

		private ArrayList _cmdTokenList = new ArrayList(0);
		/// <summary>
		/// List of string tokens for the command.
		/// </summary>
		public  ArrayList  CmdTokenList
		{
			get { return _cmdTokenList; }
			set { _cmdTokenList = value; }
		}

		private ArrayList _columns = new ArrayList(0);
		/// <summary>
		/// List of MetaData.Column for the command.
		/// </summary>
		public  ArrayList  Columns
		{
			get { return _columns; }
		}

		private ArrayList _tables = new ArrayList(0);
		/// <summary>
		/// List of MetaData.Table for the command.
		/// </summary>
		public  ArrayList  Tables
		{
			get { return _tables; }
		}

		private ArrayList _columnsOrderBy = new ArrayList(0);
		/// <summary>
		/// List of MetaData.Column for ORDER BY.
		/// </summary>
		public  ArrayList  ColumnsOrderBy
		{
			get { return _columnsOrderBy; }
		}

		private  int _tokenFROM = 0;
		/// <summary>
		/// Starting index of 'FROM' keyword token in CmdTokenList
		/// that marks the end of the select column token list
		/// </summary>
		public   int  TokenFROM
		{
			get { return _tokenFROM; }
			set { _tokenFROM = value;}
		}

		private  int _tokenORDERBY = 0;
		/// <summary>
		/// Starting index of 'ORDER' keyword token in CmdTokenList
		/// that marks the start of the ORDER BY column token list
		/// </summary>
		public   int  TokenORDERBY
		{
			get { return _tokenORDERBY; }
			set { _tokenORDERBY = value;}
		}

		/*
		** BuildTableColumnInfoForSELECTStatement method
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		**	20-oct-08 (thoda04)
		**	    Accept "FIRST nn" and "TOP nn" in a SELECT statement.
		**	    Bug 121066.
		*/

		/// <summary>
		/// Scan the SELECT statment and build the column and table names lists.
		/// </summary>
		private void BuildTableColumnInfoForSELECTStatement(string cmdText)
		{
			string token, token2;
			Column col;
			int i;
			int j;

			ArrayList Columns = new ArrayList();
			ArrayList Tables  = new ArrayList();

			CmdTokenList = ScanSqlStatement(cmdText);
			if (CmdTokenList == null)    // return if not a SELECT stmt
				return;
			if (CmdTokenList.Count < 2)  // return if stmt can't at least
				return;                  // be "select from "
			if (!ToInvariantUpper(CmdTokenList[0].ToString()).Equals("SELECT"))
				return;  // return empty-handed if not a SELECT stmt

			CmdTokenList.Add(";");  // add a few tokens at end to avoid 
			CmdTokenList.Add(";");  // worrying about overflow on lookahead
			CmdTokenList.Add(";");
			CmdTokenList.Add(";");
			CmdTokenList.Add(";");
			CmdTokenList.Add(";");
			CmdTokenList.Add(";");
			CmdTokenList.Add(";");
			CmdTokenList.Add(";");
			CmdTokenList.Add(";");

			// build a copy of the token list in uppercase for easy search
			upperList = new ArrayList(CmdTokenList.Count+15);

			foreach (string s in CmdTokenList)
			{
				string firstchar = s.Substring(0,1);
				if (firstchar.Equals("\"")  ||  firstchar.Equals("\'"))
					upperList.Add(s);  // leave quoted strings untouched
				else
					upperList.Add(ToInvariantUpper(s));
			}

			i = 1;  // index into the SELECT stmt token list

			token = (string)upperList[i];
			if (token.Equals("FIRST") ||
				token.Equals("TOP"))
			{
				token2 = (string)upperList[i+1]; // peek at next token
				int int32Val;  // bit bucket for the TryParse()
				if (token2 == "?" ||  //  if 2nd token is "?" or an integer
					Int32.TryParse(token2, out int32Val) == true)
				{
					i += 2;    // skip the "FIRST nn" or "TOP nn" keywords
				}
			}

			token = (string)upperList[i];
			if (token.Equals("ALL")  ||
				token.Equals("DISTINCT"))
				i++;    // skip the SELECT ALL/DISTINCT keywords

			// catch case where there is no column list; we still want tables
			token = (string)upperList[i];
			if (token.Equals("FROM"))
			{
				TokenFROM = i;  // index of FROM keyword at end of column list

				token = (string)upperList[++i];
				goto ScanTableList;    // go process the FROM table-list
			}


			// scan the column list
			while (i < CmdTokenList.Count)  // loop through select columns
			{
				// start parsing the select-list
				col = BuildColumnInfoForColumnReference(i);
				if (col == null)  // if error, return and leave Columns, Tables null
					return;

				Columns.Add(col);  // at last, add the Column definition

				i += col.TokenCount;
				token = (string)upperList[i];

				if (token.Equals(","))
				{
					token = (string)upperList[++i];
					continue;    // go process the next column in select list
				}

				if (token.Equals("FROM"))
				{
					TokenFROM = i;  // index of FROM keyword at end of column list

					token = (string)upperList[++i];
					break;    // go process the FROM table-list
				}
				else
					return;   // else give up because of unexpected syntax

			}  // end while loop through columns list


			ScanTableList:
			// start scanning the table list after the FROM keyword
			while (i < CmdTokenList.Count)  // loop through tables list
			{
				token = (string)upperList[i];
				if (IsKeywordMarkingEndOfTableList(token)) // if end of tables
				    break;                                 //   break out

				if (token.Equals("("))  // skip ahead if FROM ( <table_list> )
				{
					token = (string)upperList[++i];
					if (token.Equals("SELECT")) // skip over "(subquery)"
						i = ScanPastParenthesisGroup(i-1, CmdTokenList);
					continue;
				}

				if (token.Equals(")"))  // skip closing parenthesis
				{
					i++;
					continue;
				}

				// start parsing the <table reference list>
				Table table = new Table();  // new table

				if (i >= CmdTokenList.Count)
					return;   // return if premature end of stmt
				token = (string)CmdTokenList[i];
				table.TableName = token;
				table.Token     = i;
				token = (string)upperList[++i];
				if (token.Equals("."))         // myschema.mytable ?
				{
					if (++i >= CmdTokenList.Count)
						return;   // return if premature end of stmt
					token = (string)CmdTokenList[i];
					table.SchemaName = table.TableName;
					table.TableName  = token;
					token = (string)upperList[++i];
				}

				if(
					!token.Equals(",")    &&  // isolate correlation name
					!token.Equals(";")    &&
					!token.Equals("AS")   &&
					!token.Equals("ON")   &&
					!token.Equals("USING")&&
					!IsKeywordAssociatedWithJOIN(token)
					)
				{
					token = (string)CmdTokenList[i];
					table.AliasName = token;
					token = (string)upperList[++i];
				}
				else
				if (token.Equals("AS"))         // mytable AS myalias?
				{
					if (++i >= CmdTokenList.Count)
						return;   // return if premature end of stmt
					token = (string)CmdTokenList[i];
					table.AliasName = token;
					token = (string)upperList[++i];
				}

				if (!IsIdentifier(table.TableName, "\""))
					return;     // if not a valid table name, give up

				// save the name specifications as entered by the user
				// for later if we need the references for CommandBuilder,
				// and set the true names stripped of quotes and
				// adjusted for case into the names.
				table.SchemaNameSpecification = table.SchemaName;
				table.TableNameSpecification  = table.TableName;
				table.AliasNameSpecification  = table.AliasName;

				table.SchemaName = StripQuotesAndCorrectCase(table.SchemaName);
				table.TableName  = StripQuotesAndCorrectCase(table.TableName);
				table.AliasName  = StripQuotesAndCorrectCase(table.AliasName);

				table.TokenCount = i - table.Token;

				Tables.Add(table);  // at last, add the Table definition

				i = ScanPastJoinONandUSINGclauses(i, CmdTokenList);
				token = (string)upperList[i];

				if (token.Equals(","))
				{
					i++;
					continue;    // go process the next table in table list
				}

				j = TestForKeywordJOIN(i, CmdTokenList);  // JOIN?
				if (j != 0)
				{
					i = j + 1;   // i = index of token after the JOIN keyword
					continue;    // go process the next table in join table list
				}

				break;  // no more tables
			}  // end while loop through tables list


			this._columns = Columns;
			this._tables  = Tables;

			if (i >= CmdTokenList.Count)  // return if no more syntax tokens
				return;

			i = ScanToORDERBY(i, CmdTokenList);  // scan ahead to the ORDER BY
			token = (string)upperList[i];

			if (!token.Equals("ORDER"))   // if not "ORDER", return
				return;
			token = (string)upperList[++i];
			if (!token.Equals("BY"))   // if not "ORDER BY", return
				return;

			TokenORDERBY = i-1;  // starting token number of "ORDER BY"
			if ((++i) >= CmdTokenList.Count) // return if nothing after ORDER BY
				return;
			token = (string)CmdTokenList[i];

			while (i < CmdTokenList.Count)  // loop through ORDER BY columns
			{
				// start parsing the orderby-list
				col = BuildColumnInfoForColumnReference(i, true);
				if (col == null)  // if no more, return
					return;

				col.IsOrderByDefinition = true;  // this is ORDER BY def column
				ColumnsOrderBy.Add(col);  // at last, add the Column definition

				i += col.TokenCount;
				token = (string)upperList[i];

				if (token.Equals(";"))
					break;       // all done with ORDER BY list

				if (token.Equals(","))
				{
					token = (string)upperList[++i];
					continue;    // go process the next column in ORDER BY list
				}
			}  // end while loop through ORDER BY column list

			int sortOrder = 0;
			foreach (object oColumnOrderBy in ColumnsOrderBy)
			{
				sortOrder++;
				Column columnOrderBy = (Column)oColumnOrderBy;
				if (columnOrderBy == null  ||
					columnOrderBy.ColumnName == null)
					continue;

				// set the SortDirection for ASC or DESC
				int sortDirection;
				if (columnOrderBy.AliasName != null  &&
					ToInvariantUpper(columnOrderBy.AliasName) == "DESC")
					sortDirection = -1;
				else
					sortDirection =  1;

				// find the column in the SELECT list that ORDER BY is referring
				int selectColumnNumber = 0;
				if (IsNumber(columnOrderBy.ColumnName))
					selectColumnNumber =
						Convert.ToInt32(columnOrderBy.ColumnName);
				else
				{
					Column column;

					foreach (object oColumn in Columns)
					{
						selectColumnNumber++;
						column = (Column)oColumn;

						if (column.AliasName != null  &&
							column.AliasName == columnOrderBy.ColumnName)
							break;

						if (column.ColumnName == null)
							continue;
						if (column.ColumnName != columnOrderBy.ColumnName)
							continue;
						if (column.TableName        != null  &&
							columnOrderBy.TableName != null  &&
							column.TableName != columnOrderBy.TableName)
							continue;
						if (column.SchemaName        != null  &&
							columnOrderBy.SchemaName != null  &&
							column.SchemaName != columnOrderBy.SchemaName)
							continue;
						break;
					}
				}
				if (selectColumnNumber < 1  ||
					selectColumnNumber > Columns.Count)
					continue;  // skip if beyond select list count
				selectColumnNumber--;  // index into list

				col = (Column)Columns[selectColumnNumber];
				if (col.SortOrder > 0)  // if in ORDER BY list twice, skip it
					continue;
				col.SortOrder     = sortOrder;
				col.SortDirection = sortDirection;
			}  // end loop through ColumnsOrderBy ORDER BY columns

			return;
		}

		/*
		** BuildColumnInfoForColumnReference
		**
		** History:
		**	12-Jan-04 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Scan the statment and build a column definition item.
		/// </summary>
		private Column BuildColumnInfoForColumnReference(int i)
		{
			return BuildColumnInfoForColumnReference(i, false);
		}

		/// <summary>
		/// Scan the statment and build a column definition item.
		/// </summary>
		private Column BuildColumnInfoForColumnReference(int i, bool isOrderBy)
		{
			Column col = new Column();  // new select column
			string token;
			string token1;
			int    expressionTokenStart;
			int    expressionTokenCount;

			col.Token  = i;             // starting token index

			token = (string)upperList[i];
			if ((i + 1) < CmdTokenList.Count)
			{
				token1 = (string)upperList[i + 1];
				if (token1.Equals("="))  // if "resultname = " expression
				{
					col.AliasName = token;  // save resultname
					i += 2;                 // skip over "resultname = "
				}
			}

			token = (string)upperList[i];

			expressionTokenStart = i;   // in case of expression, remember

			if (token.Equals("SELECT"))  // give up if nested SELECT
				return  null;                  // not wrapped in parentheses

			if (token.Equals("(") ||  // skip past "(expression)"
				token.Equals("{"))    // skip past "{...}"
			{
				i = ScanPastParenthesisGroup(i, CmdTokenList) - 1;
				token = null;  // use null as ColumnName for expr
			}  // end if

			if ((i + 1) >= CmdTokenList.Count)
				return null;   // return if premature end of syntax
			col.ColumnName = (token==null)?null:(string)CmdTokenList[i];
			token = (string)upperList[++i];

			if (token.Equals("."))         // mytable.mycol ?
			{
				if (++i >= CmdTokenList.Count)
					return null;   // return if premature end of stmt
				token = (string)CmdTokenList[i];
				if (col.ColumnName != null)
				{
					col.TableName = col.ColumnName;
					col.ColumnName = token;
				}
				token = (string)upperList[++i];
				if (token.Equals("."))         // myschema.mytable.mycol ?
				{
					if (++i >= CmdTokenList.Count)
						return null;   // return if premature end of stmt
					token = (string)CmdTokenList[i];
					if (col.ColumnName != null)
					{
						col.SchemaName = col.TableName;
						col.TableName  = col.ColumnName;
						col.ColumnName = token;
					}
					token = (string)upperList[++i];
				}  // end if (myschema.mytable.mycol format)
			}  // end if (mytable.mycol format)

			// check if optional 'AS'
			if ((!token.Equals("AS"))   &&
				 !token.Equals("FROM")  &&
				 IsIdentifier(token, "\""))
			{
				token = "AS";  // insert a virtual "AS" keyword
				expressionTokenCount  =  // remember length of expression
					i - expressionTokenStart;
				i--;           // back up one token
				goto ScanColumnAlias;   // go process the alias
			}

			while(
				!token.Equals("AS") &&  // look for end of term
				!token.Equals(",")  &&
				!token.Equals(";")  &&
				!token.Equals("FROM"))
			{
				// we don't have a term, we have an expression
				if (i >= CmdTokenList.Count)  // safety check
					return null;   // return if premature end of stmt
				col.SchemaName = null;  // clear all if expression
				col.TableName  = null;
				col.ColumnName = null;
				// leave col.AliasName alone
				if (token.Equals("(") ||  // skip past "(expression)"
					token.Equals("{"))    // skip past "{...}"
				{
					i = ScanPastParenthesisGroup(i, CmdTokenList);
					token = (string)upperList[i];
				}
				else
					token = (string)upperList[++i];
			}  // end while looping to find end of column reference

			expressionTokenCount  =  // remember length of expression
				i - expressionTokenStart;

			ScanColumnAlias:
			if (token.Equals("AS"))         // mycol AS myalias?
			{
				if (++i >= CmdTokenList.Count)
					return null;   // return if premature end of stmt
				token = (string)CmdTokenList[i];
				col.AliasName = token;
				token = (string)upperList[++i];
			}

			// if no column name or (bad column name and is not SELECT * FROM...)
			// then clear our the column definition for an expression.
			if (col.ColumnName == null  ||
				( (!isOrderBy)  &&  !IsIdentifier(col.ColumnName, "\"")  &&
				  !col.ColumnName.Equals("*")))
			{
				col.SchemaName = null;  // clear all if expression
				col.TableName  = null;
				col.ColumnName = null;
			}

			// save the name specifications as entered by the user
			// for later if we need the references for CommandBuilder,
			// and set the true names stripped of quotes and
			// adjusted for case into the names.
			col.SchemaNameSpecification = col.SchemaName;
			col.TableNameSpecification  = col.TableName;
			col.ColumnNameSpecification = col.ColumnName;
			col.AliasNameSpecification  = col.AliasName;

			col.SchemaName = StripQuotesAndCorrectCase(col.SchemaName);
			col.TableName  = StripQuotesAndCorrectCase(col.TableName);
			col.ColumnName = StripQuotesAndCorrectCase(col.ColumnName);
			col.AliasName  = StripQuotesAndCorrectCase(col.AliasName);

			// if expression and not db column reference, save it in
			// col.ColumnNameSpecification and leave
			// col.ColumnName == null.
			if (col.ColumnNameSpecification == null)
				col.ColumnNameSpecification = TokensToString(
					CmdTokenList, expressionTokenStart, expressionTokenCount);

			// count of tokens that make up the column definition 
			// including alias
			col.TokenCount = i - col.Token;

			return col;
		}


		/*
		** BuildCatalogTableInfoForSELECTStatement method
		**
		** Description:
		**	Resolve missing schema names in the FROM table list.
		**
		** Notes:
		**	Called by MetaData constructor.
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Resolve missing schema names in the FROM table list.
		/// </summary>
		private void BuildCatalogTableInfoForSELECTStatement()
		{
			// Fill in any missing schema names for table references
			foreach (Table table in this.Tables)
			{
				if (table.SchemaName == null)
					table.SchemaName = 
						catalog.FindMissingSchemaName(table.TableName);
				if (table.SchemaName == null)  // if not found then
					continue;                  // move on to the next

				// build in-core catalog entry for this table reference
				table.CatTable = catalog.GetCatalogTable(table);
			}
		}


		/*
		** BuildKeyInfoForSELECTStatement method
		**
		** Description:
		**	Build the catalog columns information for the table.
		**	Resolve the catalog column information for the column
		**	(including its primary key, index, and base information).
		**
		** Notes:
		**	Called by MetaData constructor.
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Build the KeyInfo metadata for CommandBehavior.KeyInfo.
		/// </summary>
		private void BuildKeyInfoForSELECTStatement(bool expandSelectStar)
		{
			// Fill in any missing schema names for table references
			foreach (Table table in this.Tables)
			{
				if (table.SchemaName == null)
					table.SchemaName = 
						catalog.FindMissingSchemaName(table.TableName);
				if (table.SchemaName == null)  // if not found then
					continue;                  // move on to the next

				// build in-core catalog entry for this table reference
				table.CatTable = catalog.GetCatalogTable(table);
			}

			// Table references in the FROM clause are well-defined now
			// and have the catalog information.
			// Now, resolve the column reference to get its table reference.
				int i = -1;   // first column will be index 0
				foreach (Column col in Columns)
				{
					i++;
					col.Table     = FindTableReferenceForColumn(Tables, col);

					if (expandSelectStar        &&
						col.ColumnName != null  &&   // if SELECT * FROM ...
						col.ColumnName.Equals("*"))
					{
						ArrayList tablelist;
						if (col.Table == null)
							tablelist = _tables;  // expand * from all tables
						else
						{                         // expand * from named table
							tablelist = new ArrayList(1);
							tablelist.Add(col.Table);
						}
						ExpandSelectStarColumnReference(Columns, i, tablelist);
						break; // "*" must be the only column reference
					}
					col.CatColumn = FindCatalogColumnForColumn(col);
				}  // end loop through Columns

		}  // BuildKeyInfoForSELECTStatement


		/*
		** ExpandSelectStarColumnReference method
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		**	30-sep-08 (thoda04)
		**	    "SELECT *"  must insert columns from all tables
		**	    before removing * reference.  Bug 120972.
		*/

		/// <summary>
		/// Expand the "*" column reference in a SELECT *
		/// to all of the columns defined in the table or tables.
		/// </summary>
		private void ExpandSelectStarColumnReference(
			ArrayList columns,  // array of columns that contains the "*" column
			int index,          // index into columns for the "*" column
			ArrayList tablelist)// list of tables to expand from
		{
			int starindex = index;    // save the index of the "*" column

			foreach(Table table in tablelist)
			{
				Catalog.Table catTable = table.CatTable;
				if (catTable         == null ||
				    catTable.Columns == null ||
				    catTable.Columns.Count == 0)
					continue;
				foreach (Catalog.Column catCol in catTable.Columns)
				{
					Column col = new MetaData.Column();
					col.SchemaName = catCol.SchemaName;
					col.TableName  = catCol.TableName;
					col.ColumnName = "*";  // indicate to GetSchemaTable() that
					                       // this column is from "SELECT * ..."
					if (table.AliasNameSpecification != null)
						col.TableNameSpecification = table.AliasNameSpecification;
					else
					{
						col.TableNameSpecification = table.TableNameSpecification;
						if (table.SchemaNameSpecification != null)
							col.SchemaNameSpecification = 
								table.SchemaNameSpecification;
					}
					col.Table = table;      // Table reference in FROM list

					columns.Insert(++index, col);  // insert new column into list
				}  // end loop thru catalog columns for the table
			} // end loop thru tables

			if (index != starindex)  // if new columns replaced "*" column
				columns.RemoveAt(starindex);  // remove the "*" column
			else  // for some strange reason (e.g. table not in database),
			      // couldn't replace columns
			{
				Column col = (Column)columns[starindex];
				col.Clear();   // clear the strange "*" column
			}
		}  // ExpandSelectStarColumnReference


		/*
		** FindCatalogColumnForColumn
		**
		** Description:
		**	For a given metadata column, find its
		**	counterpart in the catalog.  Search the
		**	metadata table and its catalog info for
		**	catalog column.
		**
		** Returns:
		**	if found: Catalog.Column
		**	else:     null
		**
		** History:
		**	03-Feb-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Find the catalog column that belongs to this column.
		/// </summary>
		public Catalog.Column FindCatalogColumnForColumn(Column col)
		{
			if (col.CatColumn != null)  // return if Catalog Column
				return col.CatColumn;   // is already known

			// return if nothing to search on
			if (col.Table                  == null  ||
				col.Table.CatTable         == null  ||
				col.Table.CatTable.Columns == null)
				return null;

			foreach (Catalog.Column catCol in col.Table.CatTable.Columns)
			{
				if (col.ColumnName.Equals(catCol.ColumnName))
					return catCol;
			}
			return null;
		}


		/*
		** FindTableReferenceForColumn
		**
		** Description:
		**	If column has a table-name reference then
		**	use that to match against the Table's
		**	alias name or table name.
		**	If column has no table-name reference then
		**	match against the Table's catalog columns.
		**
		** History:
		**	03-Feb-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Find the FROM Table reference that belongs to this column.
		/// </summary>
		private Table FindTableReferenceForColumn(ArrayList tables, Column col)
		{
			// return if nothing to search on
			if (tables == null  ||  col == null  ||  col.ColumnName == null)
				return null;

			if (col.Table != null)   // return the Table reference definition
				return col.Table;    // if already known

			foreach (Table table in tables)
			{
				// if col was qualified ny tablename or aliasname then try it
				if (col.TableName != null)
				{
					if (table.AliasName != null)
						if (col.TableName.Equals(table.AliasName))
							return table;
						else
							continue;  // if table has alias, only test alias

					if (col.TableName.Equals(table.TableName))
						return table;
				}
				else // no qualification, just a column name
				{
					// move on if no catalog information for table
					if (table.CatTable         == null  ||
						table.CatTable.Columns == null)
						continue;

					if (col.ColumnName == "*")  // if SELECT * FROM table
					{
						if (tables.Count == 1)  // if only one table in list
							return table;       // and no ambiguity, return it.
						else
							return null;
					}

					// try to match against the table's columns
					foreach (Catalog.Column catCol in table.CatTable.Columns)
					{
						if (col.ColumnName.Equals(catCol.ColumnName))
							return table;
					}
				}
			}  // end foreach loop through the tables

			// table reference not found.  Could be a reserved word
			// like "user", or something like "SELECT * FROM ...".
			return null;
		}


		/*
		** IsIdentifier method
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// True if regular or delimited identifer.
		/// </summary>
		internal static bool IsIdentifier(string token, string delimiter)
		{
			if (token == null  || token.Length == 0)
				return false;

			string quotechar = delimiter.Equals("'") ? "\"" : "\'";
			string tokenchar = token.Substring(0,1);  // first char of token

			if (delimiter.Equals(tokenchar) || // true if delimited identifier
				"_".Equals(tokenchar))         // true if start with underscore
				return true;

			if (!Char.IsLetter(tokenchar,0))   // false if no start with letter
				return false;

			if (token.Length == 1)   // true if single letter
				return true;

			tokenchar = token.Substring(1,1);  // second char of token
			if (quotechar.Equals(tokenchar))
				return false;      // false if X'01EF' or N'unicode'

			return true;           // true for regular identifier
		} // IsIdentifier

		/*
		** IsIdentifier method
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Test if a string is a number.  True if a non-negative integer.
		/// </summary>
		internal static bool IsNumber(string token)
		{
			if (token == null  ||  token.Length == 0)
				return false;

			foreach (char c in token)
			{
				if (!Char.IsDigit(c))
					return false;
			}
			return true;
		}

		/// <summary>
		/// Remove Token(s) that make up the Column at the zero-based index.
		/// </summary>
		/// <param name="columnIndex">Zero-based index into Columns.</param>
		/// <returns>Token index (zero-based) of removal point.</returns>
		public int RemoveToken(int columnIndex)
		{
			if (columnIndex < 0  ||  columnIndex >= Columns.Count)
				return 0;

			MetaData.Column column = (MetaData.Column)Columns[columnIndex];

			return RemoveToken(column, columnIndex);
		}

		/// <summary>
		/// Remove Token(s) that make up the Column at the zero-based index.
		/// </summary>
		/// <param name="column">MetaData.Column describing the column.</param>
		/// <param name="columnIndex">Zero-based index into Columns.</param>
		/// <returns>Token index (zero-based) of removal point.</returns>
		public int  RemoveToken(MetaData.Column column, int columnIndex)
		{
			if (column.SortOrder > 0)  // if column is in the ORDER BY, remove
			{
				int i = column.SortOrder-1;  // index into ColumnsOrderBy
				MetaData.Column col = (MetaData.Column)ColumnsOrderBy[ i ];
				RemoveToken(ColumnsOrderBy, col, i);
			}

			return RemoveToken(Columns, column, columnIndex);
		}


		/// <summary>
		/// Remove Token(s) that make up the Column at the zero-based index
		/// from the column list specified.
		/// </summary>
		/// <param name="columnList">The ArrayList of MetaData.Column's.</param>
		/// <param name="column">MetaData.Column describing the column.</param>
		/// <param name="columnIndex">Zero-based index into list.</param>
		/// <returns>Token index (zero-based) of removal point.</returns>
		public int  RemoveToken(
			ArrayList columnList, MetaData.Column column, int columnIndex)
		{
			if (columnIndex < 0                  ||
				columnIndex >= columnList.Count  ||
				column == null)  // just return if token not found
				return 0;

			// number of tokens that make up the column reference
			int token      = column.Token;
			int tokenCount = column.TokenCount;

			// if we are not the last column
			// then get rid of the following comma
			if (columnIndex + 1 < columnList.Count)
				tokenCount++;

			// if we are the last column then get rid of the leading comma
			if (columnList.Count > 1  &&  columnIndex + 1 == columnList.Count)
			{
				token--;
				tokenCount++;
			}

			// remove the tokens that make up the column reference
			CmdTokenList.RemoveRange(token, tokenCount);

			return token;
		}  // RemoveToken


		/// <summary>
		/// After processing JOIN table_reference, scan past
		/// ON search_condition and USING (join_column_list) clauses.
		/// </summary>
		/// <param name="i"></param>
		/// <param name="CmdTokenList"></param>
		/// <returns></returns>
		private int ScanPastJoinONandUSINGclauses(int i, ArrayList CmdTokenList)
		{
			string token,token2;
			int    j;

			while (i < CmdTokenList.Count)
			{
				token = ((string)CmdTokenList[i]).ToUpperInvariant();

				if (token.Equals(")"))  // skip closing paren of JOIN groups
				{
					i++;
					continue;
				}

				if (token.Equals("("))  // skip over (SELECT...) sub-queries
				{
					token2 = ((string)CmdTokenList[i+1]).ToUpperInvariant();
					if (token2.Equals("SELECT")) // skip over "(subquery)"
					{
						i = ScanPastParenthesisGroup(i, CmdTokenList);
						continue;
					}
				}

				if (token.Equals("USING") &&   // if USING (col1, col2...)
					((String)CmdTokenList[i+1]).Equals("("))
				{
					i = ScanPastParenthesisGroup(i + 1, CmdTokenList);
					continue;
				}

				if (token.Equals(","))
					return i;          // return index -> ","

				if (IsKeywordMarkingEndOfTableList(token))
					return i;          // return if WHERE, ORDER BY, etc.

				j = TestForKeywordJOIN(i, CmdTokenList);  // JOIN?
				if (j != 0)
					return j;          // return index -> "JOIN"

				i++;                   // check the next token for a keyword
			}  // end while (i < tokenlist.Count)

			return i;  // return current index
		}  // ScanPastJoinONandUSINGclauses

		/*
		** ScanPastParenthesisGroup method
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Scan past the opening parenthesis/bracket and 
		/// return index to token after closing paren/bracket.
		/// </summary>
		internal static int ScanPastParenthesisGroup(int i, ArrayList tokenlist)
		{
			string token;
			string openChar = (string)tokenlist[i];  // "(" or "{"
			string closeChar = openChar.Equals("(") ? ")" : "}";  // closing char

			int parenCount = 1;
			while (++i < tokenlist.Count  &&  parenCount > 0)
			{
				token = (string)tokenlist[i];
				if     (token.Equals(openChar))  parenCount++;
				else
					if (token.Equals(closeChar))  parenCount--;
			}
			//if (i >= tokenlist.Count)
				//throw new FormatException(); // unmatched parens

			return i;  // return index to token after closing paren
		}

		private static int ScanToORDERBY(int i, ArrayList tokenlist)
		{
			string token, token2;

			while (i < tokenlist.Count)
			{
				token = ((string)tokenlist[i]).ToUpperInvariant();

				if (token.Equals("ORDER"))
				{
					token2 = ((string)tokenlist[i+1]).ToUpperInvariant();
					if (token2.Equals("BY"))
						return i;
				}

				// skip over (SELECT ...) subquery to avoid any 
				// possible ORDER BY embedded in the subquery
				if (token.Equals("("))
				{
					token2 = ((string)tokenlist[i+1]).ToUpperInvariant();
					if (token2.Equals("SELECT"))
					{
						i = ScanPastParenthesisGroup(i, tokenlist);
						continue;
					}
				}

				if (token.Equals(";"))  // return if end of list
				{
					return i;
				}
				i++;  // try next token looking for "ORDER BY"
			}  // end while loop through tokens

			return i-1;  // return index to last token in the token list
		}


		/// <summary>
		/// Determine if current token is a keyword that
		/// marks the end of FROM table_reference_list.
		/// </summary>
		/// <param name="token">Identifier that might be a keyword.</param>
		/// <returns>True if one of the keywords, else false.</returns>
		private static Boolean IsKeywordMarkingEndOfTableList(string token)
		{
			if((token.Equals("WHERE")  ||
				token.Equals("GROUP")  ||
				token.Equals("HAVING") ||
				token.Equals("UNION")  ||
				token.Equals("ORDER")  ||
				token.Equals("OFFSET") ||
				token.Equals("FETCH")  ||
				token.Equals("WITH")   ||
				token.Equals(";")))
				return true;

			return false;  // return not a keyword marking end of <table list>
		}


		/// <summary>
		/// Returns TRUE if token is part of the JOIN keyword family.
		/// </summary>
		/// <param name="token"></param>
		/// <returns>True if one of the keywords, else false.</returns>
		private static Boolean IsKeywordAssociatedWithJOIN(string token)
		{
			if (token == null)
				return false;

			token = token.ToUpperInvariant();

			if (token.Equals("JOIN") ||
				token.Equals("CROSS") ||
				token.Equals("NATURAL") ||
				token.Equals("UNION") ||
				token.Equals("INNER") ||
				token.Equals("LEFT") ||
				token.Equals("RIGHT") ||
				token.Equals("FULL") ||
				token.Equals("OUTER") ||
				token.Equals("UNION"))
				return true;

			return false;  // return not a keyword associated with JOIN keyword
		}

		/// <summary>
		/// Test if current token is part of JOIN syntax.
		/// If yes, scan ahead and return the index to the JOIN.
		/// </summary>
		/// <param name="i"></param>
		/// <param name="tokenlist"></param>
		/// <returns>Return index of JOIN keyword; 0 if none.</returns>
		private static int TestForKeywordJOIN(int i, ArrayList tokenlist)
		{
			string token;

			while (i < tokenlist.Count)
			{
				token = ((string)tokenlist[i]).ToUpperInvariant();

				if (token.Equals("JOIN"))
					return i;

				if (IsKeywordAssociatedWithJOIN(token))
				{
					return TestForKeywordJOIN(i + 1, tokenlist);
				}
				
				break;
			} 

			return 0;  // return 0 to indicate no JOIN table
		}


		/*
		** ScanSqlStatement method
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		**	19-Jan-10 (thoda04) B124115
		**	    Added slash-star comment support.
		*/

		/// <summary>
		/// Scan the SQL statment and build tokens.
		/// </summary>
		public static ArrayList ScanSqlStatement(string cmdText)
		{
			ArrayList list = new ArrayList();

			bool insideQuoteString = false;
			bool insideIdentifier  = false;
			bool insideNumeric     = false;
			bool insideNumericExponent= false;
			bool insideSlashStarComment= false;
			bool isPrevCharSlash= false;
			StringBuilder sbToken = new System.Text.StringBuilder(100);
			char priorChar = Char.MinValue;
			char quoteChar = Char.MinValue;
			char c;

			foreach(char cCurrent in cmdText)
			{
				c = cCurrent;   // make a working and writeable copy of char

				if (insideQuoteString)
				{
					// We are inside a quoted string.  If prior char 
					// was a quote char then that prior char was either
					// the terminating quote or the first quote for a
					// quote-quote sequence.  Look at the current char
					// to resolve the context.
					if (priorChar == quoteChar)  // if prior char was a quote char
					{                            // and current char is a quote char
						if (c == quoteChar) // then doubled quotes, append the quote
						{
							sbToken.Append(c);   // append doubled quote to token
							priorChar = Char.MinValue;  // clear prior quote context
							continue;  // continue the string and get next character
						}
						else // we had a closing quote in the prior char
						{
							insideQuoteString = false;  // we out of quote string
							// fall through to normal token processing for current char
						}
					}
					else  // ordinary char inside string or possible ending quote
					{
						priorChar = c;      // remember this char as prior char
						sbToken.Append(c);   // append to token
						continue;           // get next char
					}
				}

				if (insideIdentifier)
				{
					if (Char.IsLetterOrDigit(c) ||  c == '_' ||
						c == '#'  ||  c == '@'  ||  c == '$')
					{
						sbToken.Append(c);   // append to token
						continue;                // get next char
					}
					else
					{
						insideIdentifier  = false;

						// check for x'01EF' hexadecimal literal
						if (c == '\''  &&  sbToken.Length == 1  &&
							(sbToken[0] == 'x'  ||  sbToken[0] == 'X'))
						{
							sbToken.Append(c);         // append quote to token
							insideQuoteString = true;  // starting the string
							quoteChar = c;             // remember starting quote
							priorChar = Char.MinValue; // clear any doubled quote context
							continue;                  // build up the string
						}
						}
				}

				if (insideNumeric)
				{
					if (Char.IsDigit(c) ||  c == '.' ||  c == 'E' ||  c == 'e')
					{
						sbToken.Append(c);   // append to token
						if (c == 'E' ||  c == 'e')
						{
							insideNumeric         = false;
							insideNumericExponent = true;
						}

						continue;                // get next char
					}
					else
						insideNumeric  = false;
				}

				if (insideNumericExponent)
				{
					if (Char.IsDigit(c) ||  c == '+' ||  c == '-')
					{
						sbToken.Append(c);   // append to token
						continue;                // get next char
					}
					else
						insideNumericExponent  = false;
				}

				if (isPrevCharSlash)   // was prev char a "/" as part of "/*"?
				{
					isPrevCharSlash = false;
					if (c == '*')      // is char sequence "/*"
					{
						insideSlashStarComment = true;  // /* comment */
						sbToken.Length = 0;             // discard '/' token
						priorChar = Char.MinValue;      // clear any context
						continue;
					}
				}

				if (insideSlashStarComment)
				{
					if (priorChar == '*' && c == '/')  // end of */ comment?
					{
						insideSlashStarComment = false;
						c = Char.MinValue;             // clear context
					}
					priorChar = c;
					continue;
				}


				if (sbToken.Length != 0)   // flush token to token list
				{
					list.Add(sbToken.ToString());
					sbToken.Length = 0;
				}

				if (Char.IsWhiteSpace(c))  // ignore whitespace
					continue;


				sbToken.Append(c);   // start new token with this first char

				if (Char.IsLetter(c)  ||  c == '_')
				{
					insideIdentifier = true;   // starting the identifier
					priorChar = Char.MinValue;
					continue;                  // build up the identifier
				}

				if (Char.IsDigit(c))
				{
					insideNumeric = true;      // starting the numeric
					priorChar = Char.MinValue;
					continue;                  // build up the numeric
				}

				if (c == '\"'  ||  c == '\'')
				{
					insideQuoteString = true;  // starting the string
					quoteChar = c;             // remember starting quote
					priorChar = Char.MinValue; // clear any doubled quote context
					continue;                  // build up the string
				}

				if (c == '/')                  // start of /* comment?
				{
					isPrevCharSlash = true;
					priorChar = Char.MinValue; // clear any context
					continue;                  // go check for '/*'
				}

			}  // end foreach

			if (sbToken.Length != 0)   // flush last token to token list
			{
				list.Add(sbToken.ToString());
			}

			return list;  // return list of token strings
		}  // ScanSqlStatement


		/*
		** StripQuotesAndCorrectCase
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Strip the quotes from the delimited identifier
		/// and normalize the case to the database's working case.
		/// </summary>
		string StripQuotesAndCorrectCase(string name)
		{
			if (name == null  ||  name.Length == 0)  // check for null name
				return name;

			char quotechar = name[0];
			string IdentifierCase;

			// if first char is a quote then process as a delimited identifier
			if (quotechar == '\"')
			{
				name = StripQuotesFromIdentifier(name, "\"");

				IdentifierCase = DelimitedIdentifierCase;
			}
			else   // process as a regular identifier
			{
				IdentifierCase = RegularIdentifierCase;
			}

			switch (IdentifierCase)
			{
				case "UPPER":
					return name.ToUpper(
						System.Globalization.CultureInfo.InvariantCulture);

				case "LOWER":
					return name.ToLower(
						System.Globalization.CultureInfo.InvariantCulture);

				default: // "MIXED"
					return name;
			}
		}  // StripQuotesAndCorrectCase

		/// <summary>
		/// Strip the quotes from the delimited identifier
		/// </summary>
		public static string StripQuotesFromIdentifier(string name, string quote)
		{
			if (name == null || name.Length == 0)  // check for null name
				return name;

			char quotechar = quote[0];
			char firstchar = name[0];

			// if first char is a quote then process as a delimited identifier
			if (firstchar == quotechar)
			{
				if (name.Length <= 2)  // if name == "" then return blank name
					return (" ");
				// strip leading and trailing quotes
				char[] charname = name.ToCharArray(1, name.Length - 2);
				StringBuilder sb = new StringBuilder(charname.Length);
				char priorChar = Char.MinValue;

				foreach (char c in charname)  // scan for quote-quote
				{
					if (priorChar == quotechar && c == quotechar)
					{
						priorChar = Char.MinValue;  // drop quote context
						continue;                   // don't include 2nd quote
					}
					priorChar = c;
					sb.Append(c);
				}
				name = sb.ToString().TrimEnd();  // strip trailing blanks
				if (name.Length == 0)
					return " ";
			}
			return name;
		}


		/*
		** TokensToString
		**
		** History:
		**	12-Jun-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Format the token strings into a single string.
		/// </summary>
		public static string TokensToString(
			ArrayList cmdTokenList, int index, int count)
		{
			if (count <= 0) return String.Empty;

			StringBuilder sb = new StringBuilder(100);
			bool appendLeadingSpace = false;

			for (int i = 0; i < count;  i++, index++)
			{
				string s = (string)cmdTokenList[index];

				if (s == ".")
				{
					sb.Append(s);
					appendLeadingSpace = false; //don't space around period
					continue;  // go process next token
				}  // end if s == "."

				if (s == ")")
				{
					appendLeadingSpace = false; //don't space before rparen
				}  // end if s == rparen

				if (appendLeadingSpace)
					sb.Append(' ');

				sb.Append(s);   // add the token string to big string

				if (s == "(")
				{
					appendLeadingSpace = false; //don't space after lparen
				}  // end if s == lparen
				else
					appendLeadingSpace = true;

			}  // end for loop thru tokens

			return sb.ToString();
		}


		/*
		** WrapInQuotesIfEmbeddedSpaces
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Wrap the identifier in quotes if it contains special characters.
		/// </summary>
		public static string WrapInQuotesIfEmbeddedSpaces(string identifier)
		{
			return WrapInQuotesIfEmbeddedSpaces(identifier, "\"");
		}

		/// <summary>
		/// Wrap the identifier in quotes if it contains special characters.
		/// </summary>
		public static string WrapInQuotesIfEmbeddedSpaces(
			string identifier, string quote)
		{
			bool sw = true;
			char quoteChar = quote[0];
			// do a quick check now since most identifiers are simple
			foreach (char c in identifier)
			{
				if (Char.IsLetterOrDigit(c)  ||
					c == '_'  ||  c == '@'  ||  c == '#'  ||  c == '$')
					continue;
				sw = false;
			}
			if (sw)
				return identifier;  // no change is needed

			// need quotes around embedded special chars
			StringBuilder sb = new StringBuilder(quoteChar.ToString(), 35);
			foreach (char c in identifier)
			{
				if (c == quoteChar)
					sb.Append(quoteChar);  // double up on the quote
				sb.Append(c);
			}
			sb.Append(quoteChar);  // closing quote

			return sb.ToString();
		}

		private static string ToInvariantUpper(string str)
		{
			return str.ToUpper(System.Globalization.CultureInfo.InvariantCulture);
		}


		/*
		** Table class
		**
		** Description:
		**	Describes the table references in an SQL statement.
		**	E.g SELECT ... FROM myschema.mytable  mytablealias WHERE ...
		**
		**	Properties:
		**	SchemaNameSpecification
		**		The schema name (quotes and case) as entered by the user
		**	TableNameSpecification
		**		The table name (quotes and case) as entered by the user
		**	AliasNameSpecification
		**		The alias name (quotes and case) as entered by the user
		**	SchemaName
		**		The schema name, stripped of quotes and case
		**	TableName
		**		The table name, stripped of quotes and case
		**	AliasName
		**		The alias name, stripped of quotes and case
		**	CatTable
		**		The catalog table definition that belongs to this
		**		SQL statement table-reference
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Table reference of the SQL statement.
		/// </summary>
		public sealed class Table : MetaData.DbTableDefinition
		{
			/// <summary>
			/// Constructor for the table definition.
			/// </summary>
			public Table()
			{
			}

			private  int _token = 0;
			/// <summary>
			/// Starting index of token in token list that starts the column ref
			/// </summary>
			public   int  Token
			{
				get { return _token; }
				set { _token = value;}
			}

			private  int _tokenCount = 0;
			/// <summary>
			/// Count of tokens in token list that make up the column reference.
			/// Does not include terminating comma or keyword 'FROM'.
			/// </summary>
			public   int  TokenCount
			{
				get { return _tokenCount; }
				set { _tokenCount = value;}
			}

			private  string _schemaNameSpecification;
			/// <summary>
			/// Schema name specification for this table.
			/// </summary>
			public   string  SchemaNameSpecification
			{
				get { return _schemaNameSpecification; }
				set { _schemaNameSpecification = value;}
			}

			private  string _tableNameSpecification;
			/// <summary>
			/// Table name specification.
			/// </summary>
			public   string  TableNameSpecification
			{
				get { return _tableNameSpecification; }
				set { _tableNameSpecification = value;}
			}

			private  string _aliasNameSpecification;
			/// <summary>
			/// Alias name specification for this table.
			/// </summary>
			public   string  AliasNameSpecification
			{
				get { return _aliasNameSpecification; }
				set { _aliasNameSpecification = value;}
			}

			private  Catalog.Table _catTable;
			/// <summary>
			/// Catalog table that is referred to by this table definition.
			/// </summary>
			public   Catalog.Table  CatTable
			{
				get { return _catTable; }
				set { _catTable = value;}
			}

			/// <summary>
			/// Clear the name specifications from the table definition.
			/// </summary>
			public  override void ClearNames()
			{
				base.ClearNames();
				TableNameSpecification  = null;
				SchemaNameSpecification = null;
				AliasNameSpecification  = null;
			}

			/// <summary>
			/// String representation of qualified table name.
			/// </summary>
			/// <returns></returns>
			public override string ToString()
			{
				StringBuilder sb = new StringBuilder();

				// schema name
				if (SchemaNameSpecification != null  &&
					SchemaNameSpecification.Length > 0)
				{
					sb.Append(SchemaNameSpecification);
					sb.Append(".");
				}
				else  // no schema qualification in the SQL statement
				{
					if (CatTable != null  &&
						CatTable.SchemaName != null  &&
						CatTable.SchemaName.Length > 0)
					{
						sb.Append(MetaData.WrapInQuotesIfEmbeddedSpaces(
							CatTable.SchemaName));
						sb.Append(".");
					}
				}

				// schema.tablename
				sb.Append(TableNameSpecification);

				// schema.tablename as aliasname
				if (AliasNameSpecification != null  &&
					AliasNameSpecification.Length != 0)
				{
					sb.Append(" as ");
					sb.Append(AliasNameSpecification);
				}

				return sb.ToString();
			}

		}  // class Table

		/*
		** Column class
		**
		** Description:
		**	Describes the column references in an SQL statement or SQL catalog.
		**	E.g SELECT myschema.mytable.mycol AS mycolalias,... FROM
		**
		**	Properties:
		**	SchemaNameSpecification
		**		The schema name (quotes and case) as entered by the user
		**	TableNameSpecification
		**		The table name (quotes and case) as entered by the user
		**	ColumnNameSpecification
		**		The column name (quotes and case) as entered by the user
		**	AliasNameSpecification
		**		The alias name (quotes and case) as entered by the user
		**	SchemaName
		**		The schema name, stripped of quotes and case
		**	TableName
		**		The table name, stripped of quotes and case
		**	AliasName
		**		The alias name, stripped of quotes and case
		**	ColumnName
		**		The column name, stripped of quotes and case
		**	CatTable
		**		The catalog table definition that belongs to this metadata table
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Column reference of the SQL statement.
		/// </summary>
		public sealed class Column : MetaData.DbColumnDefinition
		{
			/// <summary>
			/// Constructor for column definition.
			/// </summary>
			public Column()
			{
			}

			private  int _token = 0;
			/// <summary>
			/// Starting index of token in token list that starts the column ref
			/// </summary>
			public   int  Token
			{
				get { return _token; }
				set { _token = value;}
			}

			private  int _tokenCount = 0;
			/// <summary>
			/// Count of tokens in token list that make up the column reference.
			/// Does not include terminating comma or keyword 'FROM'.
			/// </summary>
			public   int  TokenCount
			{
				get { return _tokenCount; }
				set { _tokenCount = value;}
			}

			private  string _schemaNameSpecification;
			/// <summary>
			/// Schema name as typed in by user (including quotes if any).
			/// Null if no schema name qualification.
			/// </summary>
			public   string  SchemaNameSpecification
			{
				get { return _schemaNameSpecification; }
				set { _schemaNameSpecification = value;}
			}

			private  string _tableNameSpecification;
			/// <summary>
			/// Table name as typed in by user (including quotes if any).
			/// Null if no table name qualification.
			/// </summary>
			public   string  TableNameSpecification
			{
				get { return _tableNameSpecification; }
				set { _tableNameSpecification = value;}
			}

			private  string _columnNameSpecification;
			/// <summary>
			/// Column name as typed in by user.
			/// Should never by null.
			/// </summary>
			public   string  ColumnNameSpecification
			{
				get { return _columnNameSpecification; }
				set { _columnNameSpecification = value;}
			}

			private  string _aliasNameSpecification;
			/// <summary>
			/// Alias name as typed in by user (including quotes if any).
			/// Null if no alias name qualification.
			/// </summary>
			public   string  AliasNameSpecification
			{
				get { return _aliasNameSpecification; }
				set { _aliasNameSpecification = value;}
			}

			private  MetaData.Table _table;
			/// <summary>
			/// The Table definition in the FROM clause that corresponds
			/// to this select column reference.
			/// </summary>
			public   MetaData.Table  Table
			{
				get { return _table; }
				set { _table = value;}
			}

			private  Catalog.Column _catColumn;
			/// <summary>
			/// The Catalog.Column definition in the Catalog.Table
			/// that corresponds to this select column reference.
			/// </summary>
			public   Catalog.Column  CatColumn
			{
				get { return _catColumn; }
				set { _catColumn = value;}
			}

			private  int _sortOrder;
			/// <summary>
			/// The Sort order of this column.  0 if none, else > 0.
			/// </summary>
			public   int  SortOrder
			{
				get { return _sortOrder; }
				set { _sortOrder = value;}
			}

			private  int _sortDirection;
			/// <summary>
			/// The Sort direction of this column.  0 if none, 1 if ASC, -1 if DESC.
			/// </summary>
			public   int  SortDirection
			{
				get { return _sortDirection; }
				set { _sortDirection = value;}
			}

			/// <summary>
			/// True if this column is a definition column for OrderByColumns.
			/// </summary>
			private  bool _isOrderByDefinition = false;
			internal bool  IsOrderByDefinition
			{
				get { return _isOrderByDefinition; }
				set { _isOrderByDefinition = value;}
			}


			/// <summary>
			/// Return string in form of schema.table.columnname AS alias.
			/// </summary>
			public override string ToString()
			{
				StringBuilder sb = new StringBuilder();

				if (SchemaNameSpecification != null  &&
					SchemaNameSpecification.Length > 0)
				{
					sb.Append(SchemaNameSpecification);
					sb.Append(".");
				}
				else
				if (SchemaName != null  &&
					SchemaName.Length > 0)
				{
					sb.Append(SchemaName);
					sb.Append(".");
				}

				if (TableNameSpecification != null  &&
					TableNameSpecification.Length > 0)
				{
					sb.Append(TableNameSpecification);
					sb.Append(".");
				}
				else
				if (TableName != null  &&
					TableName.Length > 0)
				{
					sb.Append(TableName);
					sb.Append(".");
				}

				if (ColumnNameSpecification != null  &&
					ColumnNameSpecification.Length > 0)
				{
					sb.Append(ColumnNameSpecification);
				}
				else
				if (ColumnName != null)
					sb.Append(ColumnName);
				else
					sb.Append("<null>");

				if (AliasNameSpecification != null  &&
					AliasNameSpecification.Length > 0)
				{
					sb.Append(IsOrderByDefinition?" ":" AS ");
					sb.Append(AliasNameSpecification);
				}
				else
				if (AliasName != null  &&
					AliasName.Length > 0)
				{
					sb.Append(IsOrderByDefinition?" ":" AS ");
					sb.Append(AliasName);
				}

				return sb.ToString();
			}

		}  // class Column


		/*
		** DbTableDefinition class
		**
		** Description:
		**	Describes the table references in an SQL statement or SQL catalog.
		**	E.g SELECT ... FROM myschema.mytable  mytablealias WHERE ...
		**
		**	Properties:
		**	SchemaName
		**		The schema name, stripped of quotes and case
		**	TableName
		**		The table name, stripped of quotes and case
		**	AliasName
		**		The alias name, stripped of quotes and case
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Table reference of the SQL statement.
		/// </summary>
		public class DbTableDefinition
		{
			/// <summary>
			/// Constructor for table definition.
			/// </summary>
			public DbTableDefinition()
			{
			}

			private string _tableName;
			/// <summary>
			/// Name of the table.
			/// </summary>
			public  string  TableName
			{
				get { return _tableName; }
				set { _tableName = value;}
			}

			private string _schemaName;
			/// <summary>
			/// Schema name qualifier of the table.
			/// </summary>
			public  string  SchemaName
			{
				get { return _schemaName; }
				set { _schemaName = value;}
			}

			private string _aliasName;
			/// <summary>
			/// Alias name of the table.
			/// </summary>
			public  string  AliasName
			{
				get { return _aliasName; }
				set { _aliasName = value;}
			}

			/// <summary>
			/// Clear names from table definition.
			/// </summary>
			public virtual void ClearNames()
			{
				TableName  = null;
				SchemaName = null;
				AliasName  = null;
			}

			/// <summary>
			/// Return string as qualified table name.
			/// </summary>
			/// <returns></returns>
			public override string ToString()
			{
				StringBuilder sb = new StringBuilder();

				if (SchemaName != null  &&  SchemaName.Length > 0)
				{
					sb.Append(MetaData.WrapInQuotesIfEmbeddedSpaces(SchemaName));
					sb.Append(".");
				}
				if (TableName != null  &&  TableName.Length > 0)
					sb.Append(MetaData.WrapInQuotesIfEmbeddedSpaces(TableName));
				else
					sb.Append("<null>");

				if (AliasName != null  &&  AliasName.Length > 0)
				{
					sb.Append(" as ");
					sb.Append(MetaData.WrapInQuotesIfEmbeddedSpaces(AliasName));
				}

				return sb.ToString();
			}

		}  // class DbTableDefinition

		/*
		** DbColumnDefinition class
		**
		** Description:
		**	Describes the column references in an SQL statement or SQL catalog.
		**	E.g SELECT myschema.mytable.mycol AS mycolalias,... FROM
		**
		**	Properties:
		**	SchemaName
		**		The schema name, stripped of quotes and case
		**	TableName
		**		The table name, stripped of quotes and case
		**	ColumnName
		**		The column name, stripped of quotes and case
		**	AliasName
		**		The alias name, stripped of quotes and case
		**
		** History:
		**	28-Jan-03 (thoda04)
		**	    Created.
		*/

		/// <summary>
		/// Column reference of the SQL statement.
		/// </summary>
		public class DbColumnDefinition
		{
			/// <summary>
			/// Constructor.
			/// </summary>
			public DbColumnDefinition()
			{
			}

			private string _columnName;
			/// <summary>
			/// Column name.
			/// </summary>
			public  string  ColumnName
			{
				get { return _columnName; }
				set { _columnName = value;}
			}

			private string _tableName;
			/// <summary>
			/// Table name qualifier for column.
			/// </summary>
			public  string  TableName
			{
				get { return _tableName; }
				set { _tableName = value;}
			}

			private string _schemaName;
			/// <summary>
			/// Schema name qualifier for column name.
			/// </summary>
			public  string  SchemaName
			{
				get { return _schemaName; }
				set { _schemaName = value;}
			}

			private string _aliasName;
			/// <summary>
			/// Column alias name.
			/// </summary>
			public  string  AliasName
			{
				get { return _aliasName; }
				set { _aliasName = value;}
			}

			/// <summary>
			/// Clear the entries within the definition.
			/// </summary>
			public void Clear()
			{
				ColumnName = null;
				TableName  = null;
				SchemaName = null;
				AliasName  = null;
			}

			/// <summary>
			/// Return string as qualified column name.
			/// </summary>
			/// <returns></returns>
			public override string ToString()
			{
				StringBuilder sb = new StringBuilder();

				if (SchemaName != null)
				{
					sb.Append(SchemaName);
					sb.Append(".");
				}
				if (TableName != null)
				{
					sb.Append(TableName);
					sb.Append(".");
				}
				if (ColumnName != null)
					sb.Append(ColumnName);
				else
					sb.Append("<null>");

				if (AliasName != null)
				{
					sb.Append(" AS ");
					sb.Append(AliasName);
				}

				return sb.ToString();
			}

		}  // class DbColumnDefinition

	}  // class MetaData
}
