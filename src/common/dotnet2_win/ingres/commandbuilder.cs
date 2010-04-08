/*
** Copyright (c) 2005 Ingres Corporation. All Rights Reserved.
*/



/*
	** Name: commandbuilder.cs
	**
	** Description:
	**	Implements the .NET Provider IngresCommandBuilder to 
	**      assist applications in dynamically building their queries.
	**
	** Classes:
	**	IngresCommandBuilder	CommandBuilder class.
	**
	** History:
	**	01-Sep-05 (thoda04)
	**	    Created.
	**	10-Sep-05 (thoda04)
	**	    Added more debugging information to err msg if invalid quote character.
	*/

using System;
using System.Collections;
using System.Data;
using System.Data.Common;

namespace Ingres.Client
{
	/*
	** Name: IngresCommandBuilder
	**
	** Description:
	**	Helper class to automatically generate single-table
	**	commands for DataSet to database reconciliation.
	**
	*/
	/// <summary>
	/// Automatically generate single-table commands for
	/// DataSet to database reconciliation.
	/// </summary>
	public sealed class IngresCommandBuilder : DbCommandBuilder
	{
		/// <summary>
		/// Ingres implementatin of DbCommandBuilder to assist in
		/// automatically generating single-table commands for
		/// DataSet to database reconciliation.
		/// </summary>
		public IngresCommandBuilder()
			: base()
		{
			this.QuotePrefix = "\"";
			this.QuoteSuffix = "\"";
		}

		/// <summary>
		/// Ingres implementatin of DbCommandBuilder to assist in
		/// automatically generating single-table commands for
		/// DataSet to database reconciliation.
		/// </summary>
		/// <param name="adapter"></param>
		public IngresCommandBuilder(IngresDataAdapter adapter)
			: this()
		{
			this.DataAdapter = adapter;
		}

		/// <summary>
		/// Get or set the IngresDataAdapter associated with the IngresCommandBuilder.
		/// </summary>
		public new IngresDataAdapter DataAdapter
		{
			get { return base.DataAdapter as IngresDataAdapter;  }
			set { base.DataAdapter = value; }
		}

		/// <summary>
		/// Allows the Ingres Data Provider implementation of the
		/// DbCommandBuilder class to handle additional parameter properties.
		/// </summary>
		/// <param name="parm"></param>
		/// <param name="row"></param>
		/// <param name="statementType"></param>
		/// <param name="whereClause"></param>
		protected override void ApplyParameterInfo(
			DbParameter   parm,
			DataRow       row,
			StatementType statementType,
			bool          whereClause)
		{
			IngresParameter ingresParm = (IngresParameter)parm;

			ingresParm.IngresType = (IngresType)row["ProviderType"];

			Object objPrecision = row["NumericPrecision"];
			if (objPrecision != DBNull.Value)
				ingresParm.Precision = Convert.ToByte(objPrecision);

			Object objScale     = row["NumericScale"];
			if (objScale != DBNull.Value)
				ingresParm.Scale = Convert.ToByte(objScale);

		}  // ApplyParameterInfo

		/// <summary>
		/// Populate the IngresCommand.Parameters collection with data type
		/// metadata about the parameters of the specified database procedure.
		/// </summary>
		/// <param name="command"></param>
		public static void DeriveParameters(IngresCommand command)
		{
			if (command == null)
				throw new ArgumentNullException(
					"DeriveParameters parameter 'command' must not be null.");

			IngresConnection conn = command.Connection;
			if (conn == null || conn.State != ConnectionState.Open)
				throw new InvalidOperationException(
					"The IngresCommand.Connection must be specified and open.");

			if (command.CommandText == null  ||
				command.CommandText.Length == 0)
				throw new InvalidOperationException(
					"The IngresCommand.CommandText must be specify a procedure name.");

			if (command.CommandType != CommandType.StoredProcedure)
				throw new InvalidOperationException(
					"Only CommandType.StoredProcedure is supported.");

			ArrayList tokens = Ingres.ProviderInternals.MetaData.ScanSqlStatement(
				command.CommandText);

			String[] restriction = new String[3];
			if (tokens.Count == 1)             // procname
			{
				restriction[2] = UnquoteIdent((String)tokens[0], conn, "\"", "\"");
			}
			else
			if (tokens.Count == 3  &&          // schemaname.procname
				(String)tokens[1] == ".")
			{
				restriction[1] = UnquoteIdent((String)tokens[0], conn, "\"", "\"");
				restriction[2] = UnquoteIdent((String)tokens[2], conn, "\"", "\"");
			}
			else
			if (tokens.Count == 5  &&          // catalogname.schemaname.procname
				(String)tokens[1] == "."  &&
				(String)tokens[3] == ".")
			{
				restriction[0] = UnquoteIdent((String)tokens[0], conn, "\"", "\"");
				restriction[1] = UnquoteIdent((String)tokens[2], conn, "\"", "\"");
				restriction[2] = UnquoteIdent((String)tokens[4], conn, "\"", "\"");
			}
			else
				throw new InvalidOperationException(
					"Invalid procedure name.");


			DataTable datatable = conn.GetSchema("ProcedureParameters", restriction);

			command.Parameters.Clear();

			foreach (DataRow row in datatable.Rows)
			{
				string     name       =     (String)row["COLUMN_NAME"];
				IngresType ingresType = (IngresType)row["INGRESTYPE"];
				IngresParameter parm = new IngresParameter(name, ingresType);
				command.Parameters.Add(parm);
			}  // end foreach (DataRow row in datatable)
		}

		/// <summary>
		/// Return the IngresCommand for the DELETE statement.
		/// </summary>
		/// <returns></returns>
		public new IngresCommand GetDeleteCommand()
		{
			return (IngresCommand)base.GetDeleteCommand();
		}

		/// <summary>
		/// Return the IngresCommand for the DELETE statement.
		/// </summary>
		/// <returns></returns>
		public new IngresCommand GetDeleteCommand(bool useColumnsForParameterNames)
		{
			return (IngresCommand)
				base.GetDeleteCommand(useColumnsForParameterNames);
		}

		/// <summary>
		/// Return the IngresCommand for the INSERT statement.
		/// </summary>
		/// <returns></returns>
		public new IngresCommand GetInsertCommand()
		{
			return (IngresCommand)base.GetInsertCommand();
		}

		/// <summary>
		/// Return the IngresCommand for the INSERT statement.
		/// </summary>
		/// <returns></returns>
		public new IngresCommand GetInsertCommand(bool useColumnsForParameterNames)
		{
			return (IngresCommand)
				base.GetInsertCommand(useColumnsForParameterNames);
		}

		/// <summary>
		/// Return the IngresCommand for the UPDATE statement.
		/// </summary>
		/// <returns></returns>
		public new IngresCommand GetUpdateCommand()
		{
			return (IngresCommand)base.GetUpdateCommand();
		}

		/// <summary>
		/// Return the IngresCommand for the UPDATE statement.
		/// </summary>
		/// <returns></returns>
		public new IngresCommand GetUpdateCommand(bool useColumnsForParameterNames)
		{
			return (IngresCommand)
				base.GetUpdateCommand(useColumnsForParameterNames);
		}

		/// <summary>
		/// Return the parameter name for a specified index
		/// in the format of @p#.
		/// </summary>
		/// <param name="parameterOrdinal"></param>
		/// <returns></returns>
		protected override string GetParameterName(int parameterOrdinal)
		{
			return "p" + parameterOrdinal.ToString();
		}  // GetParameterName(int)

		/// <summary> parameter name 
		/// Return the parameter name for a specified partial name
		/// </summary>
		/// <param name="parameterName"></param>
		/// <returns></returns>
		protected override string GetParameterName(string parameterName)
		{
			return parameterName;
		}  // GetParameterName(string)

		/// <summary>
		/// Get the string that specified a parameter marker within 
		/// SQL query text.
		/// </summary>
		/// <param name="parameterOrdinal"></param>
		/// <returns></returns>
		protected override string GetParameterPlaceholder(int parameterOrdinal)
		{
			return "?";
		}  // GetParameterPlaceholder

		/// <summary>
		/// Opening quote string to be used for identifiers.
		/// </summary>
		public override string QuotePrefix
		{
			get  { return base.QuotePrefix; }
			set
			{
				if (value == null  ||
					(value != "\""  &&  value != "\'"))
					throw new ArgumentException("Invalid quote value. In IngresCommandBuilder.QuotePrefix. Value ="+
						(value==null?"<null>":value.ToString()));

				base.QuotePrefix = value;
			}
		}

		/// <summary>
		/// Closing quote string to be used for identifiers.
		/// </summary>
		public override string QuoteSuffix
		{
			get { return base.QuoteSuffix; }
			set
			{
				if (value == null ||
					(value != "\"" && value != "\'"))
//					throw new ArgumentException("Invalid quote value.");
					throw new ArgumentException("Invalid quote value. In IngresCommandBuilder.QuoteSuffix. Value =" +
					(value == null ? "<null>" : value.ToString()));

				base.QuoteSuffix = value;
			}
		}

		/// <summary>
		/// Wrap the identifier in quotes.
		/// </summary>
		/// <param name="unquotedIdentifier"></param>
		/// <returns></returns>
		public override string QuoteIdentifier(
			string unquotedIdentifier)
		{
			return this.QuoteIdentifier(unquotedIdentifier, null);
		}

		/// <summary>
		/// Wrap the identifier in leading and trailing quote literals.
		/// </summary>
		/// <param name="unquotedIdentifier"></param>
		/// <param name="connection"></param>
		/// <returns></returns>
		public string QuoteIdentifier(
			string unquotedIdentifier,  IngresConnection connection)
		{
			if (unquotedIdentifier == null)
				throw new ArgumentNullException(
					"Parameter 'unquotedIdentifier' must not be null.");

			string quotePrefix = this.QuotePrefix;
			string quoteSuffix = this.QuoteSuffix;

			// If the quotes are not set then InvalidOperationException.
			// Should not happen since they are set by the constructor.
			if (quotePrefix == null     ||
				quoteSuffix == null     ||
				quotePrefix.Length == 0 ||
				quoteSuffix.Length == 0)
					throw new InvalidOperationException("Quotes are not set.");

			System.Text.StringBuilder sb =
				new System.Text.StringBuilder(unquotedIdentifier.Length + 2);
			sb.Append(quotePrefix);    // starting quote
			for (int i = 0; i < unquotedIdentifier.Length; i++)
			{
				if ((i + quotePrefix.Length) > unquotedIdentifier.Length)
					continue;  // avoid falling off the edge of the string

				string ss = unquotedIdentifier.Substring(i, quotePrefix.Length);
				if (ss == quotePrefix)  // if embedded quote, double up
				{
					sb.Append(quotePrefix);
					sb.Append(quotePrefix);
					i += quotePrefix.Length - 1;
					continue;
				}

				// not an embedded quote
				sb.Append(unquotedIdentifier[i]);
			}  // end for loop through unquotedIdentifier
			sb.Append(quoteSuffix);  // closing quote

			return sb.ToString();
		}

		/// <summary>
		/// Remove the leading and trailing quote literals
		/// from the specified identifier.
		/// </summary>
		/// <param name="quotedIdentifier"></param>
		/// <returns></returns>
		public override string UnquoteIdentifier(
			string quotedIdentifier)
		{
			return this.UnquoteIdentifier(quotedIdentifier, null);
		}

		/// <summary>
		/// Remove the leading and trailing quote literals
		/// from the specified identifier.
		/// </summary>
		/// <param name="quotedIdentifier"></param>
		/// <param name="connection"></param>
		/// <returns></returns>
		public string UnquoteIdentifier(
			string quotedIdentifier, IngresConnection connection)
		{
			return UnquoteIdent(
				quotedIdentifier, connection, this.QuotePrefix, this.QuoteSuffix);
		}

		/// <summary>
		/// Remove the leading and trailing quote literals
		/// from the specified identifier.
		/// </summary>
		/// <param name="quotedIdentifier"></param>
		/// <param name="connection"></param>
		/// <param name="quotePrefix"></param>
		/// <param name="quoteSuffix"></param>
		/// <returns></returns>
		internal static string UnquoteIdent(
			string           quotedIdentifier,
			IngresConnection connection,
			string           quotePrefix,
			string           quoteSuffix)
		{
			string ss;  // working substring through the identifier

			if (quotedIdentifier == null)
				throw new ArgumentNullException(
					"Parameter 'quotedIdentifier' must not be null.");

			// if the quotes are not set then return original
			if (quotePrefix == null     ||
				quoteSuffix == null     ||
				quotePrefix.Length == 0 ||
				quoteSuffix.Length == 0)
					return quotedIdentifier;

			int quotesLength = quotePrefix.Length + quoteSuffix.Length;

			// if identifier could not possibly hold the quotes, return original.
			if (quotesLength > quotedIdentifier.Length)
				return quotedIdentifier;

			// check leading quote is there, else return.
			ss = quotedIdentifier.Substring(0, quotePrefix.Length);
			if (ss != quotePrefix)
				return quotedIdentifier;  // no leading quote

			// check trailing quote is there, else return.
			ss = quotedIdentifier.Substring(
				quotedIdentifier.Length-quoteSuffix.Length, quoteSuffix.Length);
			if (ss != quoteSuffix)
				return quotedIdentifier;  // no trailing quote

			int    identLength = quotedIdentifier.Length - quotesLength;
			string ident = quotedIdentifier.Substring(
				quotePrefix.Length, identLength);

			System.Text.StringBuilder sb =
				new System.Text.StringBuilder(quotedIdentifier.Length);
			bool quoteAppended = false;
			for (int i = 0; i < identLength; i++)
			{
				// if not  falling off the edge of the string
				if ((i + quotePrefix.Length) <= ident.Length)
				{
					ss = ident.Substring(i, quotePrefix.Length);
					if (ss == quotePrefix)
					{
						if (quoteAppended) // doubled up quote already in?
						{
							quoteAppended = false;
							i += quotePrefix.Length - 1;
							continue;                // drop the second quote
						}
						else // start of possible doubled up quote 
						{
							sb.Append(quotePrefix);  // add the first quote
							quoteAppended = true;
							i += quotePrefix.Length - 1;
							continue;
						}
					}  // end if (ss == quotePrefix)
				}  // end possible embedded quote string

				quoteAppended = false;  // clear slate on doubled up quotes
				// not an embedded quote
				sb.Append(ident[i]);
			}  // end for loop through quotedIdentifier

			return sb.ToString();
		}

		/// <summary>
		/// Set the DataAdapter into this CommandBuilder.
		/// </summary>
		/// <param name="adapter"></param>
		protected override void SetRowUpdatingHandler(DbDataAdapter adapter)
		{
			IngresDataAdapter oldAdapter = this.DataAdapter as IngresDataAdapter;
			IngresDataAdapter newAdapter =      adapter     as IngresDataAdapter;

			// remove old event handler (this.DataAdapter is in use)
			if (oldAdapter != null)
			{
				oldAdapter.RowUpdating -=
					new IngresRowUpdatingEventHandler(this.RowUpdatingHandler);
				return;
			}

			// add new event handler (this.DataAdapter is not in use)
			if (newAdapter != null)
				newAdapter.RowUpdating +=
					new IngresRowUpdatingEventHandler(this.RowUpdatingHandler);
		}  // SetRowUpdatingHandler

		private void RowUpdatingHandler(object sender, IngresRowUpdatingEventArgs e)
		{
			base.RowUpdatingHandler(e);
		}

	}  // class IngresCommandBuilder

}
