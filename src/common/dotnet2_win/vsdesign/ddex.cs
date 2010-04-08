/*
** Copyright (c) 2006 Ingres Corporation. All Rights Reserved.
*/

/*
	** Name: ddex.cs
	**
	** Description:
	**	Implements the .NET Provider extensibility to Visual Studio.
	**
	** Classes:
	**	IngresDataObjectTypes     	Convenient class to hold constant names like "Table".
	**	IngresDataObjectEnumerator	Data object enumerator.
	**	IngresDataObjectIdentifierResolver
	**	IngresDataSourceInformation
	**	IngresConnectionProperties
	**
	** History:
	**	10-Jul-06 (thoda04)
	**	    Created.

	*/

using System;
using Microsoft.VisualStudio.Data;
using Microsoft.VisualStudio.Data.AdoDotNet;
using Microsoft.VisualStudio.OLE.Interop;

namespace Ingres.Client.Design
{

	/*
	DDEX extends Visual Studio design-time by allowing VS data designers
	and the Server Explorer to interact with the Ingres Data Provider
	and its object hierarchies.  DDEX allows the Ingres Data Provider
	to integrate into the VS Data Connection dialog box, display Ingres
	data source objects in the VS Server Explorer, and achieve basic 
	integration with project designers.

	The Ingres Data Provider is registered in the Visual Studio registry root
	with the Ingres Data Provider DDEX provider used as a sub-key under
	the "DataProviders".  This key becomes the Ingres DDEX provider 
	registry root:
		DataProviders\{abcdefg}
		@=".NET Framework Data Provider for Ingres"
		"Technology"="{77AB9A9D-78B9-4ba7-91AC-873F5338F1D2}"
		"InvariantName"="Ingres.Client"

	A "SupportedObjects" key is added the under the Ingres DDEX provider
	registry root.  Under the SupportedObjects key is added
	the sub-keys that describe the objects supported by the Ingres
	DDEX Provider:
		DataConnectionProperties
		DataConnectionSupport
		DataCommand
		DataTransaction
		DataSourceInformation

	Additional entries are added under the DataSourceInformation key
	to describe static information specific to the Ingres provider.
	Each value name is the name of a data source information property.

	Ingres DDEX connection support specifies connection information 
	(connection string information).

	Ingres DDEX data object support is through an XML file that defines
	the data objects exposed by the Ingres .NET Data Provider and the
	hierarchy of those data object.  The XML file maps these Ingres 
	object types to one of the generic "concepts" (e.g. tables, views,
	procedures) recognized by the DDEX metadata engine.

	The Ingres DDEX provider uses a data view support XML to define the 
	the physical data object hierarachy displayed under a specified 
	connection node in the Visual Studio Server Explorer.
	*/


	//internal class IngresDataConnectionUIControl :
	//    DataConnectionUIControl
	//{
	//    public IngresDataConnectionUIControl()
	//    {
	//        InitializeComponent();
	//    }

	//    private void InitializeComponent()
	//    {
	//        System.ComponentModel.ComponentResourceManager resources =
	//            new System.ComponentModel.ComponentResourceManager(
	//                typeof(IngresDataConnectionUIControl));
	//    }
	//}  // class IngresDataConnectionUIControl


	/// <summary>
	/// Contains the categories of metadata available
	/// from the Ingres data provider.
	/// </summary>
	internal static class IngresDataObjectTypes
	{
		public const string Root                           = "";
		public const string Column                         = "Column";
		public const string ViewColumn                     = "ViewColumn";
		public const string Database                       = "Database";
		public const string ForeignKey                     = "ForeignKey";
		public const string Index                          = "Index";
		public const string ProcedureParameter             = "ProcedureParameter";
		public const string Procedure                      = "Procedure";
		public const string Table                          = "Table";
		public const string View                           = "View";
	}  // class IngresDataObjectTypes


	/// <summary>
	/// This converter class parses and formats identifiers to and from
	/// a qualified string format.
	/// </summary>
	internal class IngresDataObjectIdentifierConverter :
		DataObjectIdentifierConverter, IObjectWithSite
	{
		public IngresDataObjectIdentifierConverter()
		{
	//        System.Windows.Forms.MessageBox.Show(
	//"Constructor Called", "IngresDataObjectIdentifierConverter");
		}

		protected override string FormatPart(string typeName, object identifierPart, bool withQuotes)
		{
			if (identifierPart == null  ||  // convert null to ""
				identifierPart == DBNull.Value)
				return String.Empty;

			string quote = _dataConnection.SourceInformation[
				DataSourceInformation.IdentifierOpenQuote] as string;
			if (quote == null) quote = "\"";

			string identifierString = identifierPart.ToString();

			if (withQuotes)
				identifierString =
					Ingres.ProviderInternals.MetaData.WrapInQuotesIfEmbeddedSpaces(
					identifierString, quote);

			return identifierString;
		}

		protected override object UnformatPart(string typeName, string identifierPart)
		{
			string quote = _dataConnection.SourceInformation[
				DataSourceInformation.IdentifierOpenQuote] as string;
			if (quote == null)  quote = "\"";

			string identifierString =
				Ingres.ProviderInternals.MetaData.StripQuotesFromIdentifier(
					identifierPart, quote);

			return identifierString;
		}

		protected override string[] SplitIntoParts(string typeName, string identifier)
		{
			if (typeName == null)  // safety check
				throw new ArgumentNullException("SplitIntoParts(typeName=null)");

			System.Collections.ArrayList tokens =
				Ingres.ProviderInternals.MetaData.ScanSqlStatement(identifier);

			int lengthFullIdentifer =
				IngresDataObjectIdentifierResolver.GetIdentifierMaxPartCount(
					typeName);
			if (lengthFullIdentifer == -1)
				throw new NotSupportedException(
					"SplitIntoParts(typeName=" + typeName + ")");

			// allocate the string array to hold the identifier parts
			string[] arrayIdentifier = new string[lengthFullIdentifer];

			if (identifier == null)
				return arrayIdentifier;

			if (tokens.Count%2 == 0)  // there had better be an odd count (1, 3, 5)
				throw new FormatException(
					"SplitIntoParts(identifier=" + identifier +
					") Incorrect token count.");

			int iToken = tokens.Count;
			int iPart = lengthFullIdentifer;
			string token = null;
			while (--iToken >= 0)  // move the tokens into the array
			{
				token = tokens[iToken].ToString();
				if (token.Equals("."))
					throw new FormatException(
						"SplitIntoParts(identifier=" + identifier + 
						") Unexpected leading separator.");

				--iPart;  // index into the array
				if (iPart < 0)
					throw new FormatException(
						"SplitIntoParts(typeName='" + typeName +
						"',identifier='" + identifier +
						"') More qualifiers encountered than expected for typeName.");

				arrayIdentifier[iPart] = token;

				if (iToken <= 0) // all done
					break;

				token = tokens[--iToken].ToString();  // next must be "."
				if (!(token.Equals(".")))
					throw new FormatException(
						"SplitIntoParts(typeName='" + typeName +
						"',identifier='" + identifier +
						") A period separator was expected.");

			} // end while

			return arrayIdentifier;
		}



		private DataConnection _dataConnection;
		void IObjectWithSite.GetSite(ref Guid riid, out IntPtr ppvSite)
		{
			ppvSite = IntPtr.Zero;
			throw new NotImplementedException(
				"IngresDataObjectIdentifierResolver.GetSite was called unexpectedly.");
		}

		// Called by DDEX engine to set the site with a DataConnection
		void IObjectWithSite.SetSite(object pUnkSite)
		{
			_dataConnection = (DataConnection)pUnkSite;
		}

	}  // IngresDataObjectIdentifierConverter



	/// <summary>
	/// The class implements a data object enumerator for Ingres.  It 
	/// enumerates root information, indexes and index columns,
	/// primary keys and unique keys, and foreign keys.  These enumerations
	/// are displayed in Server Explorer under the connection node.
	/// </summary>
	internal class IngresDataObjectEnumerator : DataObjectEnumerator
	{
		private AdoDotNetObjectEnumerator adoDataObjectEnumerator;

		public IngresDataObjectEnumerator()
		{
			adoDataObjectEnumerator = new AdoDotNetObjectEnumerator();
		}

		/// <summary>
		/// Enumerate the items for the Ingres data objects of the 
		/// specified type (e.g. Index), with the specified restrictions
		/// and sort string.
		/// </summary>
		/// <param name="typeName">Type name of the object to enumerate.</param>
		/// <param name="items">The items (strings) to enumerate.</param>
		/// <param name="restrictions">Filtering restrictions.</param>
		/// <param name="sort">ORDER BY sort specification.</param>
		/// <param name="parameters"></param>
		/// <returns></returns>
		public override DataReader EnumerateObjects(
			string   typeName,
			object[] items,
			object[] restrictions,
			string   sort,
			object[] parameters)
		{
			//System.Windows.Forms.MessageBox.Show(
			//    "EnumerateObjects Called", "IngresDataObjectEnumerator");

			if (typeName == null)   // safety check; should not happen
				throw new ArgumentNullException("typeName");

			// retrieve the Ingres data provider connection that
			// supports the the current connection.
			IngresConnection conn =
				Connection.GetLockedProviderObject() as IngresConnection;
			if (conn == null)
				throw new ArgumentException(
					"EnumerateObjects does not have the correct " +
					"IngresConnection type for the underlying connection.");
			try
			{
				if (Connection.State != DataConnectionState.Open)
					Connection.Open();

				switch (typeName)
				{
					case IngresDataObjectTypes.Root:
						System.Data.DataTable dt = conn.GetSchema("Root");
						return new AdoDotNetDataTableReader(dt);

				}  // end switch

				return adoDataObjectEnumerator.EnumerateObjects(
					typeName,
					items,
					restrictions,
					sort,
					parameters);
			}
			finally
			{
				// Unlock the DDEX Provider object
				Connection.UnlockProviderObject();
			}
		}  // EnumerateObjects

	}  // class IngresDataObjectEnumerator



	internal class IngresDataObjectIdentifierResolver :
		DataObjectIdentifierResolver, IObjectWithSite
	{
		public IngresDataObjectIdentifierResolver()
		{
	//        System.Windows.Forms.MessageBox.Show(
	//"Constructor Called", "IngresDataObjectIdentifierResolver");
		}

		private DataConnection _dataConnection;
		void IObjectWithSite.GetSite(ref Guid riid, out IntPtr ppvSite)
		{
			ppvSite = IntPtr.Zero;
			throw new NotImplementedException(
				"IngresDataObjectIdentifierResolver.GetSite was called unexpectedly.");
		}

		// Called by DDEX engine to set the site with a DataConnection
		void IObjectWithSite.SetSite(object pUnkSite)
		{
			_dataConnection = (DataConnection)pUnkSite;
		}

		protected override object[] QuickExpandIdentifier(
			string typeName, object[] partialIdentifier)
		{
	//        System.Windows.Forms.MessageBox.Show(
	//"QuickExpandIdentifier Called. TypeName="+typeName, "IngresDataObjectIdentifierResolver");

			object[] fullIdentifier = null;
			int      fullLength = 0;
			int      partialLength = 
				(partialIdentifier==null)?0:partialIdentifier.Length;

			if (typeName == null)  // safety check
				throw new ArgumentException("typeName==null");

			// Get the part count for an identifier (e.g. Table=3)
			fullLength = GetIdentifierMaxPartCount(typeName);

			// length has length of full name qualifiers
			fullIdentifier = new object[fullLength];

			// copy the partial qualifiers into the rightmost of
			// the fully qualified array
			if (partialIdentifier != null)
			{
				if (partialLength > fullLength)
				{
					// should not happen: the partial is longer
					// than the fully qualified name!
					throw new InvalidOperationException(
						"QuickExpandIdentifier(typeName=='" +
						typeName + "') has incorrect partialIdentifier.Length=" +
						partialLength.ToString());
				}

				// copy the partial qualifiers
				// into the full qualifiers, right justified
				partialIdentifier.CopyTo(
					fullIdentifier, fullLength - partialLength);
			}  // end if (partialIdentifier != null)

			if (fullLength > 1)  // if not root, if any kind of name
			{
				// if qualifier for schema name is not filled in, then use default
				if (!(fullIdentifier[1] is string))
				{
					fullIdentifier[1] =
						_dataConnection.SourceInformation[
							DataSourceInformation.DefaultSchema] as string;

					//string debugString = fullIdentifier[0] as string;
					//if (debugString==null)
					//    debugString = "<null>";
					//System.Windows.Forms.MessageBox.Show(
					//    "Default schema = "+debugString, "QuickExpandIdentifier");
				}  // end if (!(fullIdentifier[1] is string))
			}  // end if (fullLength > 1)

			return fullIdentifier;  // return fully qualified name
		}

		protected override object[] QuickContractIdentifier(string typeName, object[] fullIdentifier)
		{
			if (typeName == null)  // safety check
				throw new ArgumentException(
					"QuickContractIdentifier(typeName==null)");

			// if root, nothing to contract
			if (typeName == IngresDataObjectTypes.Root)
				return base.QuickContractIdentifier(typeName, fullIdentifier);

			int partialLength = GetIdentifierMaxPartCount(typeName);
			if (partialLength==-1)
				throw new NotSupportedException(
					"QuickContractIdentifier(typeName==\'"+typeName+"\')");

			object[] partialIdentifier =
				new object[partialLength];  // 3,4,or 5 parts

			if (fullIdentifier == null)  // safety check
				return partialIdentifier;
			int fullLength = fullIdentifier.Length;

			// copy the entries of the full to the partial copy
			fullIdentifier.CopyTo(partialIdentifier, partialLength - fullLength);

			// leave the catalog name as null

			// leave the schema name as null if matches default schema
			if (partialLength > 1 &&
				partialIdentifier[1] != null)
			{
				string partialSchema = partialIdentifier[1] as string;
				string defaultSchema =
					_dataConnection.SourceInformation[
						DataSourceInformation.DefaultSchema] as string;
				if (partialSchema == null ||
					partialSchema.Equals(defaultSchema, StringComparison.InvariantCulture))
				{
					partialIdentifier[1] = null;
				}
			}

			return partialIdentifier;
		}

		/// <summary>
		/// Get the part count for an identifier (e.g. Table=3,
		/// catalogname, schemaname, tablename).
		/// </summary>
		/// <param name="typeName">e.g. "Table", "View", etc.</param>
		/// <returns></returns>
		static internal int GetIdentifierMaxPartCount(string typeName)
		{
			switch (typeName)
			{
				case IngresDataObjectTypes.Root:
					return 0;

				case IngresDataObjectTypes.Table:
				case IngresDataObjectTypes.View:
				case IngresDataObjectTypes.Procedure:
					return 3;

				case IngresDataObjectTypes.Column:
				case IngresDataObjectTypes.ViewColumn:
				case IngresDataObjectTypes.ForeignKey:
				case IngresDataObjectTypes.Index:
				case IngresDataObjectTypes.ProcedureParameter:
					return 4;

				default:
					throw new NotSupportedException(
						"Unknown QuickExpandIdentifier() typeName = '" +
						typeName + "'");
			} // end switch
		}  // GetIdentifierMaxPartCount

	}  // class IngresDataObjectIdentifierResolver

	internal class IngresConnectionProperties : AdoDotNetConnectionProperties
	{
		public IngresConnectionProperties() :
			base("Ingres.Client")
		{ }

		public IngresConnectionProperties(string connectionString) :
			base("Ingres.Client", connectionString)
		{ }

		public override string[] GetBasicProperties()
		{
			return new string[] { "Database" };
		}

		public override bool IsComplete
		{
			get
			{
				string database = this["Database"] as string;
				if (database == null || database.Length == 0)
					return false;

				return true;
			}
		}

		public override bool EquivalentTo(DataConnectionProperties connectionProperties)
		{
			if (connectionProperties == null ||   // safety check
				!(connectionProperties is IngresConnectionProperties))
				return false;

			//if (CompareProperties(connectionProperties, "Server") &&
			//    CompareProperties(connectionProperties, "Database") &&
			//    CompareProperties(connectionProperties, "User ID"))
			//    return true;

			if (CompareProperties(connectionProperties, "Database"))
				return true;

			return false;
		}

		private bool CompareProperties(
			DataConnectionProperties connProperties, string propName)
		{
			string prop1;
			string prop2;

			if (propName == "Server")
			{
				prop1 = GetMachineNameIfLocal(this);
				prop2 = GetMachineNameIfLocal(connProperties);
			}
			else
			{
				prop1 = this[propName] as string;
				prop2 = connProperties[propName] as string;
			}

			return (String.Compare(prop1, prop2,
				StringComparison.InvariantCultureIgnoreCase)) == 0;
		}

		private string GetMachineNameIfLocal(DataConnectionProperties connProperties)
		{
			string server = connProperties["Server"] as string;

			if (server == null)
				return null;

			server = server.ToUpperInvariant();

			if (server.Equals("(LOCAL)", StringComparison.InvariantCulture)  ||
				server.StartsWith("(LOCAL)\\"))
				// return machinenmae + name beyond the "(LOCAL)"
				return Environment.MachineName.ToUpperInvariant() +
					server.Substring("(LOCAL)".Length);

			return server;
		}
	}  // class IngresConnectionProperties


	/// <summary>
	/// The class contains a set of constant strings that indicate the names 
	/// of well-known data source information properties.  The generic properties
	/// can be used by Visual Studio and the Server Explorer.  The properties
	/// can be defined in the registry under:
	/// HKLM\SOFTWARE\Microsoft\VisualStudio\8.0\DataProviders\<guid>\</guid>\ 
	///    SupportedObjects\DataSourceInformation
	/// </summary>
		internal class IngresDataSourceInformation : DataSourceInformation
	{
		public IngresDataSourceInformation()
		{
			//System.Windows.Forms.MessageBox.Show(
			//    "IngresDataSourceInformation constructor called",
			//    "IngresDataSourceInformation");

			AddProperty(DefaultSchema);  // add default owner name
		}

		protected override object RetrieveValue(string propertyName)
		{
			// fill in DefaultSchema
			while (propertyName.Equals(DefaultSchema,
				StringComparison.InvariantCultureIgnoreCase))
			{
				if (Connection.State != DataConnectionState.Open)
					Connection.Open();  // not open for some reason, do it now

				IngresConnection conn =
					ConnectionSupport.ProviderObject as IngresConnection;
				if (conn == null)  // should not happen
					break;

				Ingres.ProviderInternals.Catalog catalog =
					conn.OpenCatalog();
				if (catalog == null)  // should not happen
					break;

				string defaultSchema = catalog.GetDefaultSchema();

				if (defaultSchema == null) defaultSchema = "<null>";
				//System.Windows.Forms.MessageBox.Show(
				//    "DefaultSchema=" + defaultSchema, "IngresDataSourceInformation");

				return defaultSchema;

			}  // end if (propertyName == "DefaultSchema")

			return base.RetrieveValue(propertyName);
		}
	}  // class IngresDataSourceInformation : DataSourceInformation

}  // namespace
