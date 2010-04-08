/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Data;
using System.Data.Common;
using System.Diagnostics;
using System.Drawing;
using System.Resources;
using System.Runtime.Serialization;
using System.Windows.Forms;

namespace Ingres.Client.Design
{
	/*
	** Name: vsadapterbase.cs
	**
	** Description:
	**	Implements the base form for DataAdapter wizard.
	**
	** Classes:
	**	DataAdapterWizardBaseForm
	**	DesignerNameManager
	**
	** History:
	**	01-Apr-03 (thoda04)
	**	    Created.
	**	26-Apr-04 (thoda04)
	**	    Added DataAdapterUtil class and GetDataAdapterClone method.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-jul-04 (thoda04)
	**	    Replaced Ingres bitmaps.
	*/

	public class DataAdapterWizardBaseForm : System.Windows.Forms.Form
	{
		protected const string bitmapName   =
			"Ingres.Client.Design.install_block.bmp";
		protected const string facemapName  =
			"Ingres.Client.Design.ingresbox.bmp";
		protected const string prodName     = "Ingres";

		protected internal int     FormFontHeight;
		protected internal Button  btnCancelButton;
		protected internal Button  btnBackButton;
		protected internal Button  btnNextButton;
		protected internal Button  btnFinishButton;

		internal DataAdapterWizardBaseForm()
		{
			MainSize = new Size(528, 307);  // dimensions of main area
			const int SeparatorLineHeight = 4;
			const int dxButtons = 10;    // distance between buttons

			this.SuspendLayout();  // suspend the Layout events of the form
			                       // until we build it up

			Text = "Data Adapter Configuration Wizard";
			FormFontHeight = Font.Height;
			ButtonSize = new Size(6*FormFontHeight, 2*FormFontHeight);  // w, h

			this.ClientSize = new Size(MainSize.Width,
				MainSize.Height + 2*ButtonSize.Height + SeparatorLineHeight);
			this.AutoScaleBaseSize = new Size(5,13);
			this.MinimumSize       = new Size(400, 400);


			// Set dialog box style
			FormBorderStyle = FormBorderStyle.Sizable;
			//ControlBox      = false;
			//MaximizeBox     = false;
			//MinimizeBox     = false;
			//ShowInTaskbar   = false;

			// Center the dialog
			StartPosition = FormStartPosition.CenterScreen;

			// add the Cancel, Back, Next, and Finish buttons
			int y = ClientSize.Height - 3*ButtonSize.Height/2;

			// Next button (do this button first to make it the default button)
			btnNextButton          = new Button();
			btnNextButton.Parent   = this;
			btnNextButton.Text     = "&Next >";
			btnNextButton.Location =
				new Point(ClientSize.Width - 2*(ButtonSize.Width+dxButtons)-5, y);
			btnNextButton.Size     = new Size(ButtonSize.Width, ButtonSize.Height);
			btnNextButton.Anchor   = AnchorStyles.Bottom | AnchorStyles.Right;
			btnNextButton.DialogResult = DialogResult.OK;
			AcceptButton = btnNextButton;

			// Cancel button
			btnCancelButton          = new Button();
			btnCancelButton.Parent   = this;
			btnCancelButton.Text     = "Cancel";
			btnCancelButton.Location =
				new Point(ClientSize.Width - 4*(ButtonSize.Width+dxButtons)-5, y);
			btnCancelButton.Size     = new Size(ButtonSize.Width, ButtonSize.Height);
			btnCancelButton.Anchor   = AnchorStyles.Bottom | AnchorStyles.Right;
			btnCancelButton.DialogResult = DialogResult.Cancel;
			CancelButton = btnCancelButton;

			// Back button
			btnBackButton          = new Button();
			btnBackButton.Parent   = this;
			btnBackButton.Text     = "< &Back";
			btnBackButton.Location =
				new Point(ClientSize.Width - 3*(ButtonSize.Width+dxButtons)-5, y);
			btnBackButton.Size     = new Size(ButtonSize.Width, ButtonSize.Height);
			btnBackButton.Anchor   = AnchorStyles.Bottom | AnchorStyles.Right;
			btnBackButton.DialogResult = DialogResult.Retry;

			// Finish button
			btnFinishButton          = new Button();
			btnFinishButton.Parent   = this;
			btnFinishButton.Text     = "&Finish";
			btnFinishButton.Location =
				new Point(ClientSize.Width -   (ButtonSize.Width+dxButtons)-5, y);
			btnFinishButton.Size     = new Size(ButtonSize.Width, ButtonSize.Height);
			btnFinishButton.Anchor   = AnchorStyles.Bottom | AnchorStyles.Right;
			btnFinishButton.DialogResult = DialogResult.Yes;
			btnFinishButton.Enabled  = false;

			Panel separatorLine  = new Panel();
			separatorLine.Height = SeparatorLineHeight;
			separatorLine.Dock   = DockStyle.Bottom;
			separatorLine.Parent =this;
			separatorLine.Paint += new PaintEventHandler(OnSeparatorLinePaint);

			StatusBar sizingGrip;
			sizingGrip = new StatusBar();     // SizingGrip in bottom right corner
			sizingGrip.Height = 2*ButtonSize.Height;
			sizingGrip.Parent   = this;

			this.ResumeLayout(false);  // resume the Layout events of the form
		}  // DataAdapterWizardBaseForm() constructor

		private Size _mainSize;
		protected Size MainSize
		{
			get { return _mainSize; }
			set { _mainSize = value; }
		}

		private Size _buttonSize;
		protected Size ButtonSize
		{
			get { return _buttonSize; }
			set { _buttonSize = value; }
		}

		/*
		** OnButtonsLinePaint paint event
		**
		** Description:
		**	Draw separation line between mainarea and bottom buttons
		**
		** History:
		**	27-Aug-02 (thoda04)
		**	    Created.
		*/
		static void OnSeparatorLinePaint(object obj, PaintEventArgs eventArgs)
		{
			Control  control = (Control) obj;
			Graphics grfx = eventArgs.Graphics;

			int      y = 2;
			grfx.DrawLine(SystemPens.ControlDark,
				0,                       y,
				control.Width, y);
			grfx.DrawLine(SystemPens.ControlLightLight,
				0,                       y+1,
				control.Width, y+1);
		}  // OnButtonsLinePaint

		static protected Point Below(Control ctl)
		{
			return new Point(
				ctl.Location.X,
				ctl.Location.Y + ctl.Size.Height);
		}

		static protected Point Below(Control ctl, int y)
		{
			return new Point(
				ctl.Location.X,
				ctl.Location.Y + ctl.Size.Height + y);
		}

		static protected Point Below(Control ctl, int x, int y)
		{
			return new Point(
				ctl.Location.X + x,
				ctl.Location.Y + ctl.Size.Height + y);
		}

	}  // class DataAdapterWizardBaseForm


	class DesignerNameManager
	{
		/*
		** Name: CreateUniqueName
		**
		** Description:
		**	Create a unique command name for a new component
		**	that about to go into a container.
		**
		** Input:
		**	container	IContainer that contains the other components
		**	prefix   	prefix for the name, usually "Ingres"
		**	baseName 	base name like "SelectCommand" or "InsertCommand"
		**
		** Returns:
		**	string   	new unique name like "ingresSelectCommand1"
		**
		** History:
		**	17-Mar-03 (thoda04)
		**	    Created.
		*/
		
		/// <summary>
		/// Create a unique command name for a new component
		/// that about to go into a container.
		/// </summary>
		/// <param name ="container">IContainer that contains the other components
		/// </param>
		/// <param name ="prefix">prefix for the name, usually "Ingres"
		/// </param>
		/// <param name ="baseName">base name like "SelectCommand" or "InsertCommand"
		/// </param>
		internal static string CreateUniqueName (
			IContainer container,
			string prefix,
			string baseName)
		{
			string name;
			int i = 0;
			ComponentCollection components = container.Components;

			prefix = prefix.ToLower(  // convert Ingres to ingres
				System.Globalization.CultureInfo.InvariantCulture);
			baseName = prefix + baseName;  // "ingresSelectCommand"

			while(true)
			{
				i++;
				name = baseName + i.ToString();
				if (components[name] == null)
					return name;
			}
		}
	}  // class DesignerNameManager

	internal struct VSNETConst
	{
		internal const string fullTitle  = "Ingres .NET Provider";
		internal const string shortTitle = "Ingres";
		internal const string strApplicationDataFolder =
			"\\IngresCorporation\\" + 
			VSNETConst.shortTitle +  // "Ingres"
			"\\.NETDataProvider";
	}

	/// <summary>
	/// ToolboxItem for DataAdapter object to launch the DataAdapter
	/// Wizard and include the object into the component tray.
	/// </summary>
	[System.Runtime.InteropServices.ComVisible(true)]
	[Serializable]
	public sealed class IngresDataAdapterToolboxItem :
		System.Drawing.Design.ToolboxItem
	{
		//static string CLASSNAME = "IngresDataAdapterToolboxItem";

		public IngresDataAdapterToolboxItem() : base()
		{
		}

		public IngresDataAdapterToolboxItem(Type type) : base(type)
		{
		}

		private IngresDataAdapterToolboxItem(Bitmap icon)
		{
			DisplayName = "IngresDataAdapter";
			Bitmap = icon;
		}


		private IngresDataAdapterToolboxItem(
			SerializationInfo info, StreamingContext context)
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
		
		/// <summary>
		/// Public face for CreateComponentsCore that can be dynamically
		/// linked to for the toolbox item reference in the provider.
		/// </summary>
		/// <param name="host"></param>
		/// <returns></returns>
		public IComponent[]  CreateIngresComponents(IDesignerHost host)
		{
			return CreateComponentsCore(host);
		}

		protected override IComponent[]
			CreateComponentsCore(IDesignerHost host)
		{
			const string prefix = VSNETConst.shortTitle;  // "Ingres"
			IContainer designerHostContainer = host.Container;
			ArrayList newComponents          = new ArrayList();
			ArrayList newCommandComponents   = new ArrayList();
			string    name;
			IngresConnection connection;
			IngresCommand    providerCommand;

			if (designerHostContainer == null)  // safety check
				return null;

			IngresDataAdapter dataAdapter = new IngresDataAdapter();

			// Once a reference to an assembly has been added to this service,
			// this service can load types from names that
			// do not specify an assembly.
			ITypeResolutionService resService =
				(ITypeResolutionService)host.GetService(typeof(ITypeResolutionService));

			System.Reflection.Assembly assembly = dataAdapter.GetType().Module.Assembly;

			if (resService != null)
			{
				resService.ReferenceAssembly(assembly.GetName());
				// set the assembly name to load types from
			}

			name = DesignerNameManager.CreateUniqueName(
				designerHostContainer, prefix, "DataAdapter");
			try
			{
				designerHostContainer.Add(dataAdapter, name);
				newComponents.Add(dataAdapter);
			}
			catch (ArgumentException ex)
			{
				string exMsg = ex.ToString() +
					"\nRemove IngresDataAdapter component '" + name + "'" +
					" that is defined outside of the designer. ";
				MessageBox.Show(exMsg, "Add " + name + " failed");
				return null;
			}

			string [] commandName = new String []
				{
					"SelectCommand",
					"InsertCommand",
					"UpdateCommand",
					"DeleteCommand",
			};

			providerCommand = new IngresCommand();     // SELECT
			newCommandComponents.Add(providerCommand);
			providerCommand.DesignTimeVisible = false;
			dataAdapter.SelectCommand = providerCommand;

			providerCommand = new IngresCommand();     // INSERT
			newCommandComponents.Add(providerCommand);
			providerCommand.DesignTimeVisible = false;
			dataAdapter.InsertCommand = providerCommand;

			providerCommand = new IngresCommand();     // UPDATE
			newCommandComponents.Add(providerCommand);
			providerCommand.DesignTimeVisible = false;
			dataAdapter.UpdateCommand = providerCommand;

			providerCommand = new IngresCommand();     // DELETE
			newCommandComponents.Add(providerCommand);
			providerCommand.DesignTimeVisible = false;
			dataAdapter.DeleteCommand = providerCommand;

			// invoke the wizard to create the connection and query
			// pass VS.NET IDesignerHost host so we can add Connection
			DataAdapterWizard wizard = new DataAdapterWizard(dataAdapter, host);

			// The DataAdapterWizard called the ConnectionEditor and
			// gave the Wizard and Editor a chance to create the Connection
			// object and SELECT statement, or to cancel out.
			// If not cancelled, add the Connection object to 
			// the VS.NET component tray if not already done.
			if (dataAdapter.SelectCommand != null  &&
				dataAdapter.SelectCommand.Connection != null)
			{
				connection = dataAdapter.SelectCommand.Connection;
				name = DesignerNameManager.CreateUniqueName(
					designerHostContainer, prefix, "Connection");
				//add the connection to the VS.NET component tray
				designerHostContainer.Add(connection, name);
			}

			// add the four Command objects (each with a unique name)
			// to the designer's components tray
			for (int i=0; i<4; i++)
			{
				name = DesignerNameManager.CreateUniqueName(
					designerHostContainer, prefix, commandName[i]);
				designerHostContainer.Add(
					(IComponent) newCommandComponents[i], name);
			}  // end for loop thru command components

			string s = "Creating component list for "+ this.GetType() +"?\n";

			return (IComponent[])(newComponents.ToArray(typeof(IComponent)));
			//return base.CreateComponentsCore(host);

		}  // CreateComponentsCore
	}  // class IngresDataAdapterToolboxItem


	internal class DataAdapterWizard
	{
		//public DataAdapterWizard() : this(new IngresDataAdapter(), null)
		//{
		//}

		public DataAdapterWizard(
			IngresDataAdapter adapter,
			System.ComponentModel.Design.IDesignerHost host)
		{
			Form dlgWelcome = new DataAdapterWizardWelcomeForm();
			DataAdapterWizardGenerateForm dlgGenerate= null;
			DialogResult result = DialogResult.OK;
			//goto debugLabel;
			ShowWelcome:
				result = dlgWelcome.ShowDialog();
			if (result == DialogResult.Cancel) // if Cancel button, return
				return;
			//debugLabel:
			ConnectionEditor connectionEditor =
				new ConnectionEditor(adapter, host);

			if (dlgGenerate == null)
				dlgGenerate =
					new DataAdapterWizardGenerateForm(adapter.SelectCommand);

			//ShowGenerate:
			result = dlgGenerate.ShowDialog();
			if (result == DialogResult.Cancel) // if Cancel button, return
				return;
			// update the SelectCommand with the entry from the form's text
			if (result == DialogResult.Retry)  // if Back button, go back one
				goto ShowWelcome;

			adapter.SelectCommand.CommandText = dlgGenerate.CommandText;

			try
			{
				TableMappings.Generate(adapter);
			}
			catch (Exception ex)
			{
				MessageBox.Show(
					"Could not generate DataAdapter's TableMappings.\n" +
					ex.Message,
					"Data Adapter Wizard");
			}
		}
	}  // class DataAdapterWizard



	/// <summary>
	/// Class to hold miscellaneous utility static methods.
	/// </summary>
	internal class DataAdapterUtil
	{
		static internal DbDataAdapter GetDataAdapterClone (
			System.Data.Common.DbDataAdapter origDbDataAdapter)
		{
			if (origDbDataAdapter == null)
				return null;

			// Make a copy of the DataAdapter, Command, and Connection
			// objects to avoid accidentally overriding fields in them.
			DbDataAdapter  dbAdapter  =
				(DbDataAdapter)((ICloneable)origDbDataAdapter).Clone();
			IDbDataAdapter dataAdapter=
				(IDbDataAdapter)dbAdapter;
			IDbCommand     dbCommand  =
				dataAdapter.SelectCommand;

			// return if missing Command
			if (dbCommand == null)
				return dbAdapter;

			// make copy of Command
			dbCommand =
				(IDbCommand)((ICloneable)dbCommand).Clone();

			// return if missing Connection
			if (dbCommand.Connection == null)
				return dbAdapter;

			// make copy of Connection and plug in Command copy into adapter
			dbCommand.Connection =
				(IDbConnection)((ICloneable)dbCommand.Connection).Clone();
			dataAdapter.SelectCommand = dbCommand;

			// simplify the user's DataAdapter actions for our internal use
			dataAdapter.MissingSchemaAction = MissingSchemaAction.Add;
			if (dataAdapter.MissingMappingAction ==MissingMappingAction.Error)
				dataAdapter.MissingMappingAction = MissingMappingAction.Ignore; 

			return dbAdapter;
		}  // GetDataAdapterClone

	}  // DataAdapterUtil

}  // namespace
