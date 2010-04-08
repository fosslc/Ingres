/*
** Copyright (c) 2006, 2008 Ingres Corporation. All Rights Reserved.
*/

/*
	** Name: ddexconnectionui.cs
	**
	** Description:
	**	Implements the .NET Provider extensibility Visual Studio
	**	Add Connection interface.
	**
	** Classes:
	**	IngresDataConnectionUIControl  GUI interface for Ingres Add Connection dialog.
	**
	** History:
	**	07-Sep-06 (thoda04)
	**	    Created.
	**	27-Feb-08 (thoda04)
	**	    Added ToolboxItem(false) attribute to IngresDataConnectionUIControl
	**	    to prevent the component from being accidentally listed
	**	    in the Visual Studio/Toolbox/Choose Item list.
	*/

//#define USE_USERCONTROL_TO_USE_VS_DESIGNER

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.VisualStudio.Data;

namespace Ingres.Client.Design
{
	[ToolboxItem(false)]
	public partial class IngresDataConnectionUIControl :
#if USE_USERCONTROL_TO_USE_VS_DESIGNER
		UserControl    // useful for using the VS Designer
#else
		DataConnectionUIControl
#endif
	{
		private bool isLoadingPropeties = false;

		private ConnectionEditorForm.NamedArrayList[] dropDownValues;  // array of the arrays
		private ConnectionEditorForm.NamedArrayList serverDropDownValues;
		private ConnectionEditorForm.NamedArrayList portDropDownValues;
		private ConnectionEditorForm.NamedArrayList databaseDropDownValues;
		private ConnectionEditorForm.NamedArrayList userDropDownValues;
		private ConnectionEditorForm.NamedArrayList groupDropDownValues;
		private ConnectionEditorForm.NamedArrayList roleDropDownValues;
		private ConnectionEditorForm.NamedArrayList connectionStringLastValue;


		public IngresDataConnectionUIControl()
		{
			InitializeComponent();
		}

#if !USE_USERCONTROL_TO_USE_VS_DESIGNER
		public override void LoadProperties()
		{
			isLoadingPropeties = true;  // disable the OnTextChanged handler
			try
			{
				comboServer.Text   = ConnectionProperties["Server"]   as string;
				comboPort.Text     = ConnectionProperties["Port"]     as string;
				comboDatabase.Text = ConnectionProperties["Database"] as string;
				comboUser.Text     = ConnectionProperties["User ID"]  as string;
				textPassword.Text  = ConnectionProperties["Password"] as string;
			}
			finally
			{ isLoadingPropeties = false; }
		}
#endif

		private void OnLoad(object sender, EventArgs e)
		{
			string bitmapName   =
				"Ingres.Client.Design.ingresbox.bmp";

			System.Reflection.Assembly assembly = GetType().Module.Assembly;
			Bitmap bmp =
				new Bitmap(assembly.GetManifestResourceStream(bitmapName));
			this.pictureBox.Image = bmp;
			this.pictureBox.BorderStyle = BorderStyle.FixedSingle;
			this.pictureBox.Size = new Size(bmp.Width, bmp.Height);

			LoadDropDownValues();  // load the saved drop-down values
		}

		private void OnLeave(object sender, EventArgs e)
		{
			UpdateDropDownList(serverDropDownValues,   comboServer);
			UpdateDropDownList(portDropDownValues,     comboPort);
			UpdateDropDownList(databaseDropDownValues, comboDatabase);
			UpdateDropDownList(userDropDownValues,     comboUser);

			SaveDropDownValues();
		}  // SaveDropDownValues

		private void OnTextChanged(object sender, EventArgs e)
		{
			if (isLoadingPropeties)  // skip setting control value if
				return;              // LoadProperties is setting control values

			if (sender == comboServer)
				SetConnectionProperty("Server", comboServer.Text);
			else
			if (sender == comboPort)
				SetConnectionProperty("Port", comboPort.Text);
			else
			if (sender == comboDatabase)
				SetConnectionProperty("Database", comboDatabase.Text);
			else
			if (sender == comboUser)
				SetConnectionProperty("User ID", comboUser.Text);
			else
			if (sender == textPassword)
				SetConnectionProperty("Password", textPassword.Text);
		}

		private void SetConnectionProperty(string key, string value)
		{
#if !USE_USERCONTROL_TO_USE_VS_DESIGNER
			ConnectionProperties[key] = value.Trim();
#endif
		}

		private void LoadDropDownValues()
		{
			dropDownValues = new ConnectionEditorForm.NamedArrayList[]
			{
				connectionStringLastValue =
				                        new ConnectionEditorForm.NamedArrayList("ConnectionStringLast"),
				serverDropDownValues  = new ConnectionEditorForm.NamedArrayList("Server"),
				portDropDownValues    = new ConnectionEditorForm.NamedArrayList("Port"),
				databaseDropDownValues= new ConnectionEditorForm.NamedArrayList("Database"),
				userDropDownValues    = new ConnectionEditorForm.NamedArrayList("User"),
				groupDropDownValues   = new ConnectionEditorForm.NamedArrayList("Group"),
				roleDropDownValues    = new ConnectionEditorForm.NamedArrayList("Role")
			};
			ConnectionEditorForm.LoadValuesFromApplicationDataFolder(  // load from ConnectionEditor.xml
				dropDownValues, ConnectionEditorForm.ApplicationDataFileName);

		//	this.comboServer.Text = ConnectionEditorForm.GetValueOfKeyword(nv, DrvConst.DRV_PROP_HOST);
			this.comboServer.Items.AddRange(serverDropDownValues.ToArray());
			if (serverDropDownValues.IndexOf("(local)") == -1)
				this.comboServer.Items.Add("(local)");

		//	this.comboPort.Text = ConnectionEditorForm.GetValueOfKeyword(nv, DrvConst.DRV_PROP_PORT, "II7");
			this.comboPort.Items.AddRange(portDropDownValues.ToArray());

		//	this.comboDatabase.Text = ConnectionEditorForm.GetValueOfKeyword(nv, DrvConst.DRV_PROP_DB);
			this.comboDatabase.Items.AddRange(databaseDropDownValues.ToArray());

		//	this.comboUser.Text = ConnectionEditorForm.GetValueOfKeyword(nv, DrvConst.DRV_PROP_USR);
			this.comboUser.Items.AddRange(userDropDownValues.ToArray());

		}  // LoadDropDownValues

		private static ConnectionEditorForm.NamedArrayList UpdateDropDownList(
			ConnectionEditorForm.NamedArrayList array, Control ctrl)
		{
			return UpdateDropDownList(array, ctrl.Text);
		}

		private static ConnectionEditorForm.NamedArrayList UpdateDropDownList(
			ConnectionEditorForm.NamedArrayList array, string str)
		{
			return ConnectionEditorForm.UpdateDropDownList(array, str);
		}

		private void SaveDropDownValues()
		{
			if (dropDownValues == null)
				return;

			ConnectionEditorForm.SaveValuesToApplicationDataFolder(
				dropDownValues, ConnectionEditorForm.ApplicationDataFileName);
		}

	}  // class IngresDataConnectionUIControl
}