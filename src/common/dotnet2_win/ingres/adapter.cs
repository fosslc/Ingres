/*
** Copyright (c) 2002, 2006 Ingres Corporation. All Rights Reserved.
*/

	/*
	** Name: adapter.cs
	**
	** Description:
	**	DataAdapter class and RowUpdate/RowUpdating events.
	**
	** History:
	**	 5-Aug-02 (thoda04)
	**	    Created for .NET Provider.
	**	27-Feb-04 (thoda04)
	**	    Added additional XML Intellisense comments.
	**	 9-Mar-04 (thoda04)
	**	    Added support for ICloneable interface.
	**	27-Apr-04 (thoda04)
	**	    Added support for TableMappings editor and Gen DataSet designer.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/

using System;
using System.ComponentModel;
using System.Data;
using System.Data.Common;
using System.Reflection;

namespace Ingres.Client
{
	/// <summary>
	/// Represents a set of data commands and a database connection
	/// that are used to fill the DataSet and update the database.
	/// </summary>
	//	Allow this type to be visible to COM.
	[System.Runtime.InteropServices.ComVisible(true)]
//	 DefaultEvent will cause VS.NET editor to position in the
//	 ingresDataAdapter1_RowUpdated method source code.
	[DefaultEvent("RowUpdated")]

//	 ToolBoxItemAttribute specifies what should be used to create the
//	 IngresDataAdapter when dragged from the toolbox onto a design surface.
	[Designer(typeof(Ingres.Client.Design.GenDataSetDesigner))]
	[ToolboxItem(typeof(Ingres.Client.Design.IngresDataAdapterToolboxItem))]
	public sealed class IngresDataAdapter : DbDataAdapter, IDbDataAdapter, ICloneable
{
	/*
	** The objEventRowUpdated and objEventRowUpdating singleton objects
	** are used as keys into the Component's event list.
	** The keys are used to add/remove the events and 
	** for firing off the events.
	**/
	static private readonly object objEventRowUpdated  = new object(); 
	static private readonly object objEventRowUpdating = new object(); 


	/*
	** DataAdapter constructor.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// Create a new DataAdapter object.
	/// </summary>
	public IngresDataAdapter()
	{
	}  // end constructor DataAdapter


	/// <summary>
	/// Create a new DataAdapter object with specified SELECT Command object.
	/// </summary>
	/// <param name="selectCommand">
	/// Command object with SELECT SQL CommandText.</param>
	public IngresDataAdapter(IngresCommand selectCommand)
	{
		this.SelectCommand = selectCommand;
	}  // end constructor DataAdapter


	/// <summary>
	/// Create a new DataAdapter object with specified SELECT text string
	/// and specific Connection object.
	/// </summary>
	/// <param name="selectCommandText">SELECT text string.</param>
	/// <param name="selectConnection">Connection object.</param>
	public IngresDataAdapter(
		string selectCommandText, IngresConnection selectConnection)
	{
		this.SelectCommand =
			new IngresCommand(selectCommandText, selectConnection);
	}  // end constructor DataAdapter


		/// <summary>
		/// Create a new DataAdapter object with specified SELECT text string
		/// and specific connection string text.
		/// </summary>
		/// <param name="selectCommandText">SELECT text string</param>
		/// <param name="selectConnectionString">Connection string text.</param>
	public IngresDataAdapter(
		string selectCommandText, string selectConnectionString)
	{
		this.SelectCommand =
			new IngresCommand(
				selectCommandText,
				new IngresConnection(selectConnectionString));
	}  // end constructor DataAdapter



		/// <summary>
		/// Create a new DataAdapter object from an old DataAdapter.
		/// </summary>
		/// <param name="oldAdapter">Old DataAdapter to clone from.</param>
	private IngresDataAdapter(IngresDataAdapter oldAdapter) :
		base (oldAdapter)
	{
	}  // end constructor DataAdapter



	/*
	** SELECT, INSERT, UPDATE, and DELETE Command property objects
	** that are associated with this DataAdapter.
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

	private IngresCommand _selectCommand;
	/// <summary>
	/// Command to be used to SELECT records from the Ingres database.
	/// </summary>
	[Description("Command to be used to SELECT records from the Ingres database.")]
	[DefaultValue(null)]
	[CategoryAttribute("Fill")]
	public new IngresCommand SelectCommand 
	{
		get { return _selectCommand; }
		set { _selectCommand = value; }
	}

	IDbCommand IDbDataAdapter.SelectCommand 
	{
		get { return _selectCommand; }
		set { _selectCommand = (IngresCommand)value; }
	}

	private IngresCommand _insertCommand;
	/// <summary>
	/// Command to be used to INSERT records from the Ingres database.
	/// </summary>
	[Description("Command to be used to INSERT records into the Ingres database.")]
	[DefaultValue(null)]
	[CategoryAttribute("Update")]
	public new IngresCommand InsertCommand 
	{
		get { return _insertCommand; }
		set { _insertCommand = value; }
	}

	IDbCommand IDbDataAdapter.InsertCommand 
	{
		get { return _insertCommand; }
		set { _insertCommand = (IngresCommand)value; }
	}

	private IngresCommand _updateCommand;
	/// <summary>
	/// Command to be used to UPDATE records from the Ingres database.
	/// </summary>
	[Description("Command to be used to UPDATE records in the Ingres database.")]
	[DefaultValue(null)]
	[CategoryAttribute("Update")]
	public new IngresCommand UpdateCommand 
	{
		get { return _updateCommand; }
		set { _updateCommand = value; }
	}

	IDbCommand IDbDataAdapter.UpdateCommand 
	{
		get { return _updateCommand; }
		set { _updateCommand = (IngresCommand)value; }
	}

	private IngresCommand _deleteCommand;
	/// <summary>
	/// Command to be used to DELETE records from the Ingres database.
	/// </summary>
	[Description("Command to be used to DELETE records from the Ingres database.")]
	[DefaultValue(null)]
	[CategoryAttribute("Update")]
	public new IngresCommand DeleteCommand 
	{
		get { return _deleteCommand; }
		set { _deleteCommand = value; }
	}

	IDbCommand IDbDataAdapter.DeleteCommand 
	{
		get { return _deleteCommand; }
		set { _deleteCommand = (IngresCommand)value; }
	}


	/// <summary>
	/// Get a collection that provides the table/column mapping
	/// between a source table and a DataTable.
	/// </summary>
	[Editor(
		typeof(Ingres.Client.Design.TableMappingsUITypeEditor),
		typeof(System.Drawing.Design.UITypeEditor))]
	[CategoryAttribute("Mapping")]
	[DesignerSerializationVisibilityAttribute(
		DesignerSerializationVisibility.Content)]  // serialize to code
	new public DataTableMappingCollection TableMappings
	{
		get { return base.TableMappings; }
	}

	/*
	** Implementation of DbDataAdapter inherited abstract methods
	**
	** History:
	**	27-Aug-02 (thoda04)
	**	    Created.
	*/

		/// <summary>
		/// Create a new instance of RowUpdatedEventArgs.
		/// </summary>
		/// <param name="dataRow"></param>
		/// <param name="command"></param>
		/// <param name="statementType"></param>
		/// <param name="tableMapping"></param>
		/// <returns></returns>
	override protected RowUpdatedEventArgs CreateRowUpdatedEvent(
		DataRow dataRow,
		IDbCommand command,
		StatementType statementType,
		DataTableMapping tableMapping)
	{
		return new IngresRowUpdatedEventArgs(
			dataRow, command, statementType, tableMapping);
	}

		/// <summary>
		/// Create a new instance of RowUpdatingEventArgs.
		/// </summary>
		/// <param name="dataRow"></param>
		/// <param name="command"></param>
		/// <param name="statementType"></param>
		/// <param name="tableMapping"></param>
		/// <returns></returns>
	override protected RowUpdatingEventArgs CreateRowUpdatingEvent(
		DataRow dataRow,
		IDbCommand command,
		StatementType statementType,
		DataTableMapping tableMapping)
	{
		return new IngresRowUpdatingEventArgs(
			dataRow, command, statementType, tableMapping);
	}

		/// <summary>
		/// Fire a RowUpdating event and invoke the handlers.
		/// </summary>
		/// <param name="value"></param>
	override protected void OnRowUpdating(RowUpdatingEventArgs value)
	{
		IngresRowUpdatingEventHandler handler =
			(IngresRowUpdatingEventHandler) Events[objEventRowUpdating];
		if ((handler != null) && (value is IngresRowUpdatingEventArgs)) 
		{
			// call the handler for the event
			handler(this, (IngresRowUpdatingEventArgs) value);
		}
	}

		/// <summary>
		/// Fire a RowUpdated event and invoke the handlers.
		/// </summary>
		/// <param name="value"></param>
	override protected void OnRowUpdated(RowUpdatedEventArgs value)
	{
		/*
		 * The objEventRowUpdated singleton object
		 * is used as a key into the Component's event list.
		 */
		IngresRowUpdatedEventHandler handler =
			(IngresRowUpdatedEventHandler) Events[objEventRowUpdated];
		if ((handler != null) && (value is IngresRowUpdatedEventArgs)) 
		{
			// call the handler for the event
			handler(this, (IngresRowUpdatedEventArgs) value);
		}
	}

	/// <summary>
	/// Event handler that processes the RowUpdating
	/// event of the DataAdapter.
	/// </summary>
	public event IngresRowUpdatingEventHandler RowUpdating
	{
		/*
		 * The objEventRowUpdating singleton object
		 * is used as a key into the Component's event list.
		 */
		add    { Events.AddHandler   (objEventRowUpdating, value); }
		remove { Events.RemoveHandler(objEventRowUpdating, value); }
	}

	/// <summary>
	/// Event handler that processes the RowUpdated
	/// event of the DataAdapter.
	/// </summary>
	public event IngresRowUpdatedEventHandler RowUpdated
	{
		/*
		 * The objEventRowUpdated singleton object
		 * is used as a key into the Component's event list.
		 */
		add    { Events.AddHandler   (objEventRowUpdated, value); }
		remove { Events.RemoveHandler(objEventRowUpdated, value); }
	}


	#region ICloneable Members
	/// <summary>
	/// Returns a clone of the Command object
	/// </summary>
	/// <returns></returns>
	public object Clone()
	{
		return new IngresDataAdapter(this);
	}

	#endregion
}  // class IngresDataAdapter

	/// <summary>
	/// Defines the delegate method that will handle
	/// the RowUpdating event of the IngresDataAdapter.
	/// </summary>
	//	Allow this type to be visible to COM.
[System.Runtime.InteropServices.ComVisible(true)]
public delegate void IngresRowUpdatingEventHandler(
	object sender, IngresRowUpdatingEventArgs e);

	/// <summary>
	/// Defines the delegate method that will handle
	/// the RowUpdated event of the IngresDataAdapter.
	/// </summary>
	//	Allow this type to be visible to COM.
[System.Runtime.InteropServices.ComVisible(true)]
public delegate void IngresRowUpdatedEventHandler(
	object sender, IngresRowUpdatedEventArgs e);

	/// <summary>
	/// Provides the data for the RowUpdatingEventArgs for the data provider.
	/// </summary>
//	Allow this type to be visible to COM.
[System.Runtime.InteropServices.ComVisible(true)]
public class IngresRowUpdatingEventArgs : RowUpdatingEventArgs
	{
	/// <summary>
	/// Provides the data for the RowUpdatingEventArgs
	/// for the Ingres data provider.
	/// </summary>
	/// <param name="row"></param>
	/// <param name="command"></param>
	/// <param name="statementType"></param>
	/// <param name="tableMapping"></param>
		public IngresRowUpdatingEventArgs(
		DataRow row,
		IDbCommand command, StatementType statementType,
		DataTableMapping tableMapping) 
		: base(row, command, statementType, tableMapping) 
	{
	}

	/// <summary>
	/// Get/set the Command object when the data adapter's Update is called.
	/// </summary>
	// Hide the inherited implementation of the command property.
	new public IngresCommand Command
	{
		get  { return (IngresCommand)base.Command; }
		set  { base.Command = value; }
	}
}

	/// <summary>
	/// Provides the data for the RowUpdatedEventArgs
	/// for the Ingres data provider.
	/// </summary>
//	Allow this type to be visible to COM.
[System.Runtime.InteropServices.ComVisible(true)]
public class IngresRowUpdatedEventArgs : RowUpdatedEventArgs
{
	/// <summary>
	/// Provides the data for the RowUpdatedEventArgs
	/// for the Ingres data provider.
	/// </summary>
	/// <param name="row"></param>
	/// <param name="command"></param>
	/// <param name="statementType"></param>
	/// <param name="tableMapping"></param>
	public IngresRowUpdatedEventArgs(
		DataRow row,
		IDbCommand command,
		StatementType statementType,
		DataTableMapping tableMapping)
		: base(row, command, statementType, tableMapping) 
	{
	}

	/// <summary>
	/// Get/set the Command object when the data adapter's Update is called.
	/// </summary>
	// Hide the inherited implementation of the command property.
	new public IngresCommand Command
	{
		get  { return (IngresCommand)base.Command; }
	}
}  // IngresRowUpdatedEventArgs

} // namespace


namespace Ingres.Client.Design
{


	/// <summary>
	/// Designer for IngresDataAdapter's Generate Dataset.
	/// </summary>
	internal sealed class GenDataSetDesigner :
		System.ComponentModel.Design.ComponentDesigner
	{
		public override
			System.ComponentModel.Design.DesignerVerbCollection Verbs
		{
			get
			{
				return new System.ComponentModel.Design.DesignerVerbCollection(
					new System.ComponentModel.Design.DesignerVerb[] {
						new System.ComponentModel.Design.DesignerVerb(
							"Generate DataSet...",
							new EventHandler(this.OnGenerate)) } );
			}
		}

		// Event handling method for the "Generate DataSet..." verb
		private void OnGenerate(object sender, EventArgs e)
		{
			Assembly assemDesign =
				IngresDataAdapterToolboxItem.LoadDesignAssembly();
			if (assemDesign == null)
				return;

			// call the OnGenerate method item in the VSDesign assembly.
			Object[] parms = new Object[3];
			parms[0] = this.Component;
			parms[1] = sender;
			parms[2] = e;

			Type t = assemDesign.GetType(
				"Ingres.Client.Design.GenDataSetDesigner");
			if (t == null)
				return;
			object o = Activator.CreateInstance(t);
			MethodInfo mi = t.GetMethod("OnGenerateIngresDataSet");
			if (mi == null)
				return;
			mi.Invoke(o, parms);
		}  // OnGenerate

	}  // class GenDataSetDesigner



	/// <summary>
	/// ToolboxItem class for DataAdapter object to launch the
	/// DataAdapter Wizard and include the object into the component tray.
	/// </summary>
	[System.Runtime.InteropServices.ComVisible(false)]
	[Serializable]
	internal sealed class IngresDataAdapterToolboxItem :
		System.Drawing.Design.ToolboxItem
	{
		/// <summary>
		/// Constructor for IngresDataAdapterToolboxItem.
		/// </summary>
		public IngresDataAdapterToolboxItem() : base()
		{
		}

		/// <summary>
		/// Constructor for IngresDataAdapterToolboxItem with a given type.
		/// </summary>
		public IngresDataAdapterToolboxItem(Type type) : base(type)
		{
		}

		private IngresDataAdapterToolboxItem(
			System.Runtime.Serialization.SerializationInfo info,
			System.Runtime.Serialization.StreamingContext context)
		{
			Deserialize(info, context);
		}

		/*
		** Name: CreateComponentsCore
		**
		** Description:
		**	Create the Connection object
		**	in the components tray of the designer.
		**
		** Called by:
		**	CreateComponents()
		**
		** Input:
		**	host	IDesignerHost
		**
		** Returns:
		**	IComponent[] array of components to be displayed as selected
		**
		** History:
		**	17-Mar-03 (thoda04)
		**	    Created.
		*/
		
		protected override IComponent[]
			CreateComponentsCore(System.ComponentModel.Design.IDesignerHost host)
		{
			Assembly assemDesign =
				IngresDataAdapterToolboxItem.LoadDesignAssembly();
			if (assemDesign == null)
				return null;

			// call the counterpart method in the VSDesign assembly.
			Object[] parms = new Object[1];
			parms[0] = host;

			Type t = assemDesign.GetType(
				"Ingres.Client.Design.IngresDataAdapterToolboxItem");
			if (t == null)
				return null;
			object o = Activator.CreateInstance(t);
			MethodInfo mi = t.GetMethod("CreateIngresComponents");
			if (mi == null)
				return null;
			return mi.Invoke(o, parms) as IComponent[];
		}  // CreateComponentsCore

		internal static string GetDesignAssembly()
		{
			Assembly assemExecuting = Assembly.GetExecutingAssembly();
			string fullnameExecuting = assemExecuting.FullName;
			int index = fullnameExecuting.IndexOf(',');
			if (index == -1)  // should never happen
				return null;
			// designAssembly should be something like:
			// Ingres.VSDesign,Version=1.1.0.0,Culture=neutral,PublicKeyToken=..."
			string designAssembly =
				"Ingres.VSDesign" + fullnameExecuting.Substring( index );
			return designAssembly;
		}  // GetDesignAssembly

		internal static Assembly LoadDesignAssembly()
		{
			string designAssembly =
				IngresDataAdapterToolboxItem.GetDesignAssembly();
			Assembly assemDesign = Assembly.Load(designAssembly);
			return assemDesign;
		}

	}  // class IngresDataAdapterToolboxItem


	/*
	** Name: TableMappingsUITypeEditor class
	**
	** Description:
	**	UserInterface editor for DataAdapter TableMappings designer.
	**
	** History:
	**	13-Apr-04 (thoda04)
	**	    Created.
	*/

	internal sealed class TableMappingsUITypeEditor :
		System.Drawing.Design.UITypeEditor
	{
		public TableMappingsUITypeEditor() : base()
		{
		}

		public override System.Drawing.Design.UITypeEditorEditStyle
			GetEditStyle(ITypeDescriptorContext context)
		{
			if (context          != null  &&
				context.Instance != null)
				return System.Drawing.Design.UITypeEditorEditStyle.Modal;
			return base.GetEditStyle(context);
		}

		
		/// <summary>
		/// Call the dialog to edit the TableMappings
		/// </summary>
		/// <param name="context"></param>
		/// <param name="provider">Service provider.</param>
		/// <param name="value">DataTableMappingCollection object.</param>
		/// <returns></returns>
		public override object EditValue(
			ITypeDescriptorContext context,
			IServiceProvider       provider,
			object                 value)  // DataTableMappingCollection object
		{
			// load the VSDesign assembly
			Assembly assemDesign =
				IngresDataAdapterToolboxItem.LoadDesignAssembly();
			if (assemDesign == null)
				return null;

			// call the counterpart method in the VSDesign assembly.
			Object[] parms;
			parms    = new Object[3];
			parms[0] = context;
			parms[1] = provider;
			parms[2] = value;

			Type t = assemDesign.GetType(
				"Ingres.Client.Design.TableMappingsUITypeEditor");
			if (t == null)
				return null;
			object o = Activator.CreateInstance(t);
			MethodInfo mi = t.GetMethod("EditIngresValue");
			if (mi == null)
				return null;
			return mi.Invoke(o, parms);
		}

	}  // class TableMappingsUITypeEditor


}  // namespace Design
