/*
** Copyright (c) 2005, 2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Name: factory.cs
**
** Description:
**	Implements the .NET Provider factory to create other provider objects.
**
** Classes:
**	IngresFactory	Subclass of DbProviderFactory class.
**
** History:
**	04-Apr-05 (thoda04)
**	    Created.
*/

using System;
using System.Data;
using System.Data.Common;

//using Ingres.Utility;
//using Ingres.ProviderInternals;

namespace Ingres.Client
{
	/*
	 * Inside %WINDIR%\Microsoft.NET\Framework\v2.0.????\CONFIG\machine.config
	 * 
		<system.data>
			<DbProviderFactories>
				<add name="Ingres Data Provider"
				invariant="Ingres.Client"
				support = "BF"
				description = ".Net Data Provider for Ingres"
				type = "Ingres.Client.IngresFactory, Ingres.Client,
					Version = 2.0.0.0, Culture = neutral,
					PublicKeyToken = 7ab2d069d405ce41"
				/>
			</DbProviderFactories>
	 */
	/// <summary>
	/// Implements the Ingres .NET Provider factory to create other provider objects.
	/// </summary>
	public sealed class IngresFactory : DbProviderFactory
	{
		static IngresFactory()
		{
			Instance = new IngresFactory();
		} // static IngresFactory

		private IngresFactory() : base()
		{
		} // constructor IngresFactory


		/// <summary>
		/// Returns the current instance of the IngresFactory.
		/// </summary>
		public static readonly IngresFactory Instance;



		/// <summary>
		/// Create an IngresCommand object to represent an executable SQL command
		/// or database procedure to be sent to the database.
		/// </summary>
		/// <returns></returns>
		public override DbCommand CreateCommand()
		{
			return new IngresCommand();
		} // CreateCommand


		/// <summary>
		/// Create an IngresCommandBuilder object that automatically generates
		/// single-table commands used to reconcile changes from a DataSet
		/// to the Ingres database.
		/// </summary>
		/// <returns></returns>
		public override DbCommandBuilder CreateCommandBuilder()
		{
			return new IngresCommandBuilder();
		} // CreateCommandBuilder


		/// <summary>
		/// Create an IngresConnection object to represent a connection to the database.
		/// </summary>
		/// <returns></returns>
		public override DbConnection CreateConnection()
		{
			return new IngresConnection();
		} // CreateConnection


		/// <summary>
		/// Create an IngresConnectionStringBuilder to create the keyword/value
		/// pairs of the connection string used by the IngresConnection object.
		/// </summary>
		/// <returns></returns>
		public override DbConnectionStringBuilder CreateConnectionStringBuilder()
		{
			return new IngresConnectionStringBuilder();
		} // CreateConnectionStringBuilder


		/// <summary>
		/// Create an IngresDataAdapter object to hold
		/// a set of data commands and a database connection.
		/// The adapter is used to fill a DataSet and update the database.
		/// </summary>
		/// <returns></returns>
		public override DbDataAdapter CreateDataAdapter()
		{
			return new IngresDataAdapter();
		} // CreateDataAdapter


		/// <summary>
		/// Create an IngresParameter to represents a parameter,
		/// and optionally, its mapping to DataSet columns.
		/// </summary>
		/// <returns></returns>
		public override DbParameter CreateParameter()
		{
			return new IngresParameter();
		} // CreateParameter


		/// <summary>
		/// Create an IngresPermission to help validate that a user has a
		/// security level adequate to access an Ingres data source.
		/// </summary>
		/// <param name="state">
		/// PermissionState that is to have all or no access to the database resource.
		/// This permission has no affect on standard Ingres security checking.</param>
		/// <returns></returns>
		public override System.Security.CodeAccessPermission
			CreatePermission(
				System.Security.Permissions.PermissionState state)
		{
			return new IngresPermission(state);
		} // CreatePermission

	}  // class IngresFactory

} // namespace Ingres.Client
