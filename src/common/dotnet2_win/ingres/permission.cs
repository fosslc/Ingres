/*
** Copyright (c) 2005, 2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Name: permission.cs
**
** Description:
**	Implements the .NET Provider IngresPermission and IngresPermissionAttribute.
**
** Classes:
**	IngresPermission
**	IngresPermissionAttribute
**
** History:
**	14-Jul-05 (thoda04)
**	    Created.
*/

using System;
using System.Data.Common;
using System.Security.Permissions;

namespace Ingres.Client
{
	/// <summary>
	/// Represents a security level to allow data provider to check that
	/// user has sufficient authority to access an Ingres data source.
	/// </summary>
	public sealed class IngresPermission : DBDataPermission
	{
		/// <summary>
		/// Creates a new instance of IngresPermission.
		/// </summary>
		public IngresPermission() :
			base(PermissionState.None)
		{
		}

		/// <summary>
		/// Create a new new instance of IngresPermission
		/// with specified PermissionState.
		/// </summary>
		/// <param name="state">
		/// PermissionState for access to all or no resources.</param>
		public IngresPermission(
				PermissionState state) :
			base(state)
		{
		}

		/// <summary>
		/// Creates a new instance of IngresPermission.
		/// </summary>
		/// <param name="state">
		/// PermissionState for access to all or no resources.</param>
		/// <param name="allowBlankPassword"></param>
		public IngresPermission(
				PermissionState state,
				bool allowBlankPassword) :
			base(state)
		{
			this.AllowBlankPassword = allowBlankPassword;
		}

		private IngresPermission(DBDataPermission permission)
			:
			base(permission)
		{
		}

		internal IngresPermission(IngresPermissionAttribute attribute)
			:
			base(attribute)
		{
		}

		/// <summary>
		/// Returns the IngresPermission as an IPermission.
		/// </summary>
		/// <returns>System.Security.IPermission</returns>
		public override System.Security.IPermission Copy()
		{
			return new IngresPermission((DBDataPermission)this);
		}

	}  // class IngresPermission



	/// <summary>
	/// Associates a security action with a custom security attribute.
	/// </summary>
	public class IngresPermissionAttribute : DBDataPermissionAttribute
	{
		/// <summary>
		/// Creates a new instance of IngresPermission
		/// with specified SecurityAction.
		/// </summary>
		/// <param name="action"></param>
		public IngresPermissionAttribute(SecurityAction action) : base(action)
		{
		}

		/// <summary>
		/// Returns an IngresPermission that is configured
		/// according to the IngresPermissionAttribute properties.
		/// </summary>
		/// <returns>AN IngresPermission object.</returns>
		public override System.Security.IPermission CreatePermission()
		{
			return new IngresPermission(this);
		}

	}  // class IngresPermissionAttribute
}
