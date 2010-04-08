/*
** Copyright (c) 2003, 2007 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Design;
using System.IO;
using System.Resources;
using System.Runtime.Serialization;
using System.Security;
using System.Windows.Forms;
using System.Windows.Forms.Design;
using System.Xml;

using Ingres.ProviderInternals;

//using Microsoft.Win32;

namespace Ingres.Client.Design
{
	/*
	** Name: vsconnection.cs
	**
	** Description:
	**	Implements the VS.NET interfaces for ConnectionString Editor.
	**
	** Classes:
	**	IngresConnectionToolboxItem
	**	ConnectionEditor
	**
	** History:
	**	15-Apr-03 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	 5-nov-04 (thoda04) Bug 113422
	**	    Don't lose options that are not in the Connection dialogs.
	**	12-mar-07 (thoda04) SIR 117018
	**	    Add dbms_user and dbms_password support.
	*/

	/*
	** Name: ConnectionEditorForm class
	**
	** Description:
	**	Form used for designer of the Connection object
	**	by the designer.
	**
	** Called by:
	**	DataAdapter Wizard
	**	ConnectionString UITypeEditor
	**
	** History:
	**	17-Mar-03 (thoda04)
	**	    Created.
	**	14-jul-04 (thoda04)
	**	    Replaced Ingres bitmaps.
	*/

	/// <summary>
	/// Form for connection string editor.
	/// </summary>
	internal class ConnectionEditorForm : System.Windows.Forms.Form
	{
		ConnectStringConfig nv;
		protected const string bitmapName   =
			"Ingres.Client.Design.install_block.bmp";
		internal const string ApplicationDataFileName =
			"ConnectionEditor";

		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.Label serverLabel;
		private System.Windows.Forms.Label portLabel;
		private System.Windows.Forms.Label databaseLabel;
		private System.Windows.Forms.ComboBox databaseBox;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.TabControl tabs;
		private System.Windows.Forms.Label userLabel;
		private System.Windows.Forms.ComboBox userBox;
		private System.Windows.Forms.TextBox passwordBox;
		private System.Windows.Forms.Label roleIdLabel;
		private System.Windows.Forms.ComboBox roleBox;
		private System.Windows.Forms.Label rolePwdLabel;
		private System.Windows.Forms.TextBox rolePwdBox;
		private System.Windows.Forms.Label groupLabel;
		private System.Windows.Forms.ComboBox groupBox;
		private System.Windows.Forms.GroupBox serverGroup;
		private System.Windows.Forms.GroupBox userGroup;
		private System.Windows.Forms.Button buttonOK;
		private System.Windows.Forms.Button buttonTest;
		private System.Windows.Forms.CheckBox checkConnectionPooling;
		private System.Windows.Forms.GroupBox groupCredentials;
		private System.Windows.Forms.GroupBox groupConnection;
		private System.Windows.Forms.Label timeoutLabel;
		private System.Windows.Forms.NumericUpDown timeoutUpDown;
		private System.Windows.Forms.Label timeoutSecondsLabel;
		private System.Windows.Forms.CheckBox checkPersistSecurityInfo;
		private System.Windows.Forms.PictureBox pictureBox1;
		private System.Windows.Forms.ComboBox portBox;
		private System.Windows.Forms.ComboBox serverBox;
		private System.Windows.Forms.Label passwordLabel;

		private NamedArrayList[] dropDownValues;  // array of the arrays
		private NamedArrayList   serverDropDownValues;
		private NamedArrayList   portDropDownValues;
		private NamedArrayList   databaseDropDownValues;
		private NamedArrayList   userDropDownValues;
		private NamedArrayList   groupDropDownValues;
		private NamedArrayList   roleDropDownValues;
		private NamedArrayList   dbmsUserDropDownValues;
		private NamedArrayList   connectionStringLastValue;

		private string connectionStringLast = null;
		private Label dbmsUserLabel;
		private ComboBox dbmsUserBox;
		private Label dbmsPwdLabel;
		private TextBox dbmsPwdBox;


		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public ConnectionEditorForm(ConnectStringConfig nvParm)
		{
			nv = nvParm;
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if (components != null) 
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.buttonOK = new System.Windows.Forms.Button();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.buttonTest = new System.Windows.Forms.Button();
			this.serverLabel = new System.Windows.Forms.Label();
			this.serverBox = new System.Windows.Forms.ComboBox();
			this.portLabel = new System.Windows.Forms.Label();
			this.portBox = new System.Windows.Forms.ComboBox();
			this.databaseLabel = new System.Windows.Forms.Label();
			this.databaseBox = new System.Windows.Forms.ComboBox();
			this.tabs = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.pictureBox1 = new System.Windows.Forms.PictureBox();
			this.userGroup = new System.Windows.Forms.GroupBox();
			this.userLabel = new System.Windows.Forms.Label();
			this.userBox = new System.Windows.Forms.ComboBox();
			this.passwordLabel = new System.Windows.Forms.Label();
			this.passwordBox = new System.Windows.Forms.TextBox();
			this.serverGroup = new System.Windows.Forms.GroupBox();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.groupConnection = new System.Windows.Forms.GroupBox();
			this.checkPersistSecurityInfo = new System.Windows.Forms.CheckBox();
			this.timeoutSecondsLabel = new System.Windows.Forms.Label();
			this.timeoutUpDown = new System.Windows.Forms.NumericUpDown();
			this.timeoutLabel = new System.Windows.Forms.Label();
			this.checkConnectionPooling = new System.Windows.Forms.CheckBox();
			this.groupCredentials = new System.Windows.Forms.GroupBox();
			this.dbmsUserLabel = new System.Windows.Forms.Label();
			this.dbmsUserBox = new System.Windows.Forms.ComboBox();
			this.dbmsPwdLabel = new System.Windows.Forms.Label();
			this.dbmsPwdBox = new System.Windows.Forms.TextBox();
			this.roleIdLabel = new System.Windows.Forms.Label();
			this.roleBox = new System.Windows.Forms.ComboBox();
			this.rolePwdLabel = new System.Windows.Forms.Label();
			this.rolePwdBox = new System.Windows.Forms.TextBox();
			this.groupLabel = new System.Windows.Forms.Label();
			this.groupBox = new System.Windows.Forms.ComboBox();
			this.tabs.SuspendLayout();
			this.tabPage1.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
			this.userGroup.SuspendLayout();
			this.serverGroup.SuspendLayout();
			this.tabPage2.SuspendLayout();
			this.groupConnection.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.timeoutUpDown)).BeginInit();
			this.groupCredentials.SuspendLayout();
			this.SuspendLayout();
			// 
			// buttonOK
			// 
			this.buttonOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOK.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonOK.Location = new System.Drawing.Point(432, 376);
			this.buttonOK.Name = "buttonOK";
			this.buttonOK.Size = new System.Drawing.Size(75, 23);
			this.buttonOK.TabIndex = 1;
			this.buttonOK.Text = "OK";
			this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.Location = new System.Drawing.Point(528, 376);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.Size = new System.Drawing.Size(75, 23);
			this.buttonCancel.TabIndex = 2;
			this.buttonCancel.Text = "Cancel";
			// 
			// buttonTest
			// 
			this.buttonTest.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonTest.Location = new System.Drawing.Point(24, 266);
			this.buttonTest.Name = "buttonTest";
			this.buttonTest.Size = new System.Drawing.Size(100, 21);
			this.buttonTest.TabIndex = 16;
			this.buttonTest.Text = "Test Connection";
			this.buttonTest.Click += new System.EventHandler(this.buttonTestConnection_Click);
			// 
			// serverLabel
			// 
			this.serverLabel.Location = new System.Drawing.Point(2, 24);
			this.serverLabel.Name = "serverLabel";
			this.serverLabel.Size = new System.Drawing.Size(120, 23);
			this.serverLabel.TabIndex = 3;
			this.serverLabel.Text = "Data Access Server:";
			this.serverLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// serverBox
			// 
			this.serverBox.Location = new System.Drawing.Point(124, 24);
			this.serverBox.Name = "serverBox";
			this.serverBox.Size = new System.Drawing.Size(168, 21);
			this.serverBox.TabIndex = 4;
			// 
			// portLabel
			// 
			this.portLabel.Location = new System.Drawing.Point(301, 24);
			this.portLabel.Name = "portLabel";
			this.portLabel.Size = new System.Drawing.Size(32, 23);
			this.portLabel.TabIndex = 5;
			this.portLabel.Text = "Port:";
			this.portLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// portBox
			// 
			this.portBox.Location = new System.Drawing.Point(334, 24);
			this.portBox.Name = "portBox";
			this.portBox.Size = new System.Drawing.Size(58, 21);
			this.portBox.TabIndex = 6;
			// 
			// databaseLabel
			// 
			this.databaseLabel.Location = new System.Drawing.Point(42, 56);
			this.databaseLabel.Name = "databaseLabel";
			this.databaseLabel.Size = new System.Drawing.Size(80, 23);
			this.databaseLabel.TabIndex = 7;
			this.databaseLabel.Text = "Database:";
			this.databaseLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// databaseBox
			// 
			this.databaseBox.Location = new System.Drawing.Point(124, 56);
			this.databaseBox.Name = "databaseBox";
			this.databaseBox.Size = new System.Drawing.Size(168, 21);
			this.databaseBox.TabIndex = 8;
			// 
			// tabs
			// 
			this.tabs.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
						| System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.tabs.Controls.Add(this.tabPage1);
			this.tabs.Controls.Add(this.tabPage2);
			this.tabs.Location = new System.Drawing.Point(16, 8);
			this.tabs.Name = "tabs";
			this.tabs.SelectedIndex = 0;
			this.tabs.Size = new System.Drawing.Size(600, 352);
			this.tabs.TabIndex = 9;
			this.tabs.Tag = "tabs";
			this.tabs.SelectedIndexChanged += new System.EventHandler(this.Advanced_SelectedIndexChanged);
			// 
			// tabPage1
			// 
			this.tabPage1.Controls.Add(this.buttonTest);
			this.tabPage1.Controls.Add(this.pictureBox1);
			this.tabPage1.Controls.Add(this.userGroup);
			this.tabPage1.Controls.Add(this.serverGroup);
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Size = new System.Drawing.Size(592, 326);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "Connection";
			// 
			// pictureBox1
			// 
			this.pictureBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.pictureBox1.Location = new System.Drawing.Point(440, 2);
			this.pictureBox1.Name = "pictureBox1";
			this.pictureBox1.Size = new System.Drawing.Size(96, 48);
			this.pictureBox1.TabIndex = 16;
			this.pictureBox1.TabStop = false;
			// 
			// userGroup
			// 
			this.userGroup.Controls.Add(this.userLabel);
			this.userGroup.Controls.Add(this.userBox);
			this.userGroup.Controls.Add(this.passwordLabel);
			this.userGroup.Controls.Add(this.passwordBox);
			this.userGroup.Location = new System.Drawing.Point(24, 136);
			this.userGroup.Name = "userGroup";
			this.userGroup.Size = new System.Drawing.Size(272, 100);
			this.userGroup.TabIndex = 14;
			this.userGroup.TabStop = false;
			this.userGroup.Text = "Login";
			// 
			// userLabel
			// 
			this.userLabel.Location = new System.Drawing.Point(16, 24);
			this.userLabel.Name = "userLabel";
			this.userLabel.Size = new System.Drawing.Size(64, 23);
			this.userLabel.TabIndex = 9;
			this.userLabel.Text = "User ID:";
			this.userLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// userBox
			// 
			this.userBox.Location = new System.Drawing.Point(84, 24);
			this.userBox.Name = "userBox";
			this.userBox.Size = new System.Drawing.Size(152, 21);
			this.userBox.TabIndex = 10;
			// 
			// passwordLabel
			// 
			this.passwordLabel.Location = new System.Drawing.Point(16, 64);
			this.passwordLabel.Name = "passwordLabel";
			this.passwordLabel.Size = new System.Drawing.Size(64, 23);
			this.passwordLabel.TabIndex = 11;
			this.passwordLabel.Text = "Password:";
			this.passwordLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// passwordBox
			// 
			this.passwordBox.Location = new System.Drawing.Point(84, 64);
			this.passwordBox.Name = "passwordBox";
			this.passwordBox.Size = new System.Drawing.Size(152, 20);
			this.passwordBox.TabIndex = 12;
			this.passwordBox.UseSystemPasswordChar = true;
			// 
			// serverGroup
			// 
			this.serverGroup.Controls.Add(this.serverLabel);
			this.serverGroup.Controls.Add(this.serverBox);
			this.serverGroup.Controls.Add(this.portLabel);
			this.serverGroup.Controls.Add(this.portBox);
			this.serverGroup.Controls.Add(this.databaseLabel);
			this.serverGroup.Controls.Add(this.databaseBox);
			this.serverGroup.Location = new System.Drawing.Point(24, 16);
			this.serverGroup.Name = "serverGroup";
			this.serverGroup.Size = new System.Drawing.Size(400, 100);
			this.serverGroup.TabIndex = 13;
			this.serverGroup.TabStop = false;
			this.serverGroup.Text = "Server";
			// 
			// tabPage2
			// 
			this.tabPage2.Controls.Add(this.groupConnection);
			this.tabPage2.Controls.Add(this.groupCredentials);
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(592, 326);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Advanced";
			// 
			// groupConnection
			// 
			this.groupConnection.Controls.Add(this.checkPersistSecurityInfo);
			this.groupConnection.Controls.Add(this.timeoutSecondsLabel);
			this.groupConnection.Controls.Add(this.timeoutUpDown);
			this.groupConnection.Controls.Add(this.timeoutLabel);
			this.groupConnection.Controls.Add(this.checkConnectionPooling);
			this.groupConnection.Location = new System.Drawing.Point(24, 16);
			this.groupConnection.Name = "groupConnection";
			this.groupConnection.Size = new System.Drawing.Size(272, 136);
			this.groupConnection.TabIndex = 18;
			this.groupConnection.TabStop = false;
			this.groupConnection.Text = "Connection";
			// 
			// checkPersistSecurityInfo
			// 
			this.checkPersistSecurityInfo.Location = new System.Drawing.Point(32, 88);
			this.checkPersistSecurityInfo.Name = "checkPersistSecurityInfo";
			this.checkPersistSecurityInfo.Size = new System.Drawing.Size(224, 32);
			this.checkPersistSecurityInfo.TabIndex = 2;
			this.checkPersistSecurityInfo.Text = "Return password text in Connection.ConnectionString property";
			// 
			// timeoutSecondsLabel
			// 
			this.timeoutSecondsLabel.Location = new System.Drawing.Point(152, 24);
			this.timeoutSecondsLabel.Name = "timeoutSecondsLabel";
			this.timeoutSecondsLabel.Size = new System.Drawing.Size(48, 23);
			this.timeoutSecondsLabel.TabIndex = 19;
			this.timeoutSecondsLabel.Text = "seconds";
			this.timeoutSecondsLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
			// 
			// timeoutUpDown
			// 
			this.timeoutUpDown.Location = new System.Drawing.Point(88, 24);
			this.timeoutUpDown.Name = "timeoutUpDown";
			this.timeoutUpDown.Size = new System.Drawing.Size(56, 20);
			this.timeoutUpDown.TabIndex = 0;
			// 
			// timeoutLabel
			// 
			this.timeoutLabel.Location = new System.Drawing.Point(32, 24);
			this.timeoutLabel.Name = "timeoutLabel";
			this.timeoutLabel.Size = new System.Drawing.Size(48, 23);
			this.timeoutLabel.TabIndex = 17;
			this.timeoutLabel.Text = "Timeout:";
			this.timeoutLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// checkConnectionPooling
			// 
			this.checkConnectionPooling.Location = new System.Drawing.Point(32, 56);
			this.checkConnectionPooling.Name = "checkConnectionPooling";
			this.checkConnectionPooling.Size = new System.Drawing.Size(168, 24);
			this.checkConnectionPooling.TabIndex = 1;
			this.checkConnectionPooling.Text = "Enable connection pooling";
			// 
			// groupCredentials
			// 
			this.groupCredentials.Controls.Add(this.dbmsUserLabel);
			this.groupCredentials.Controls.Add(this.dbmsUserBox);
			this.groupCredentials.Controls.Add(this.dbmsPwdLabel);
			this.groupCredentials.Controls.Add(this.dbmsPwdBox);
			this.groupCredentials.Controls.Add(this.roleIdLabel);
			this.groupCredentials.Controls.Add(this.roleBox);
			this.groupCredentials.Controls.Add(this.rolePwdLabel);
			this.groupCredentials.Controls.Add(this.rolePwdBox);
			this.groupCredentials.Controls.Add(this.groupLabel);
			this.groupCredentials.Controls.Add(this.groupBox);
			this.groupCredentials.Location = new System.Drawing.Point(24, 184);
			this.groupCredentials.Name = "groupCredentials";
			this.groupCredentials.Size = new System.Drawing.Size(432, 119);
			this.groupCredentials.TabIndex = 17;
			this.groupCredentials.TabStop = false;
			this.groupCredentials.Text = "Advanced Login";
			// 
			// dbmsUserLabel
			// 
			this.dbmsUserLabel.Location = new System.Drawing.Point(6, 53);
			this.dbmsUserLabel.Name = "dbmsUserLabel";
			this.dbmsUserLabel.Size = new System.Drawing.Size(66, 23);
			this.dbmsUserLabel.TabIndex = 3;
			this.dbmsUserLabel.Text = "DBMS User:";
			this.dbmsUserLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// dbmsUserBox
			// 
			this.dbmsUserBox.Location = new System.Drawing.Point(80, 53);
			this.dbmsUserBox.Name = "dbmsUserBox";
			this.dbmsUserBox.Size = new System.Drawing.Size(121, 21);
			this.dbmsUserBox.TabIndex = 5;
			// 
			// dbmsPwdLabel
			// 
			this.dbmsPwdLabel.Location = new System.Drawing.Point(200, 53);
			this.dbmsPwdLabel.Name = "dbmsPwdLabel";
			this.dbmsPwdLabel.Size = new System.Drawing.Size(100, 23);
			this.dbmsPwdLabel.TabIndex = 2;
			this.dbmsPwdLabel.Text = "DBMS Password:";
			this.dbmsPwdLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// dbmsPwdBox
			// 
			this.dbmsPwdBox.Location = new System.Drawing.Point(304, 53);
			this.dbmsPwdBox.Name = "dbmsPwdBox";
			this.dbmsPwdBox.Size = new System.Drawing.Size(100, 20);
			this.dbmsPwdBox.TabIndex = 6;
			this.dbmsPwdBox.UseSystemPasswordChar = true;
			// 
			// roleIdLabel
			// 
			this.roleIdLabel.Location = new System.Drawing.Point(16, 24);
			this.roleIdLabel.Name = "roleIdLabel";
			this.roleIdLabel.Size = new System.Drawing.Size(56, 23);
			this.roleIdLabel.TabIndex = 0;
			this.roleIdLabel.Text = "Role ID:";
			this.roleIdLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// roleBox
			// 
			this.roleBox.Location = new System.Drawing.Point(80, 24);
			this.roleBox.Name = "roleBox";
			this.roleBox.Size = new System.Drawing.Size(121, 21);
			this.roleBox.TabIndex = 3;
			// 
			// rolePwdLabel
			// 
			this.rolePwdLabel.Location = new System.Drawing.Point(200, 24);
			this.rolePwdLabel.Name = "rolePwdLabel";
			this.rolePwdLabel.Size = new System.Drawing.Size(100, 23);
			this.rolePwdLabel.TabIndex = 0;
			this.rolePwdLabel.Text = "Role Password:";
			this.rolePwdLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// rolePwdBox
			// 
			this.rolePwdBox.Location = new System.Drawing.Point(304, 24);
			this.rolePwdBox.Name = "rolePwdBox";
			this.rolePwdBox.Size = new System.Drawing.Size(100, 20);
			this.rolePwdBox.TabIndex = 4;
			this.rolePwdBox.UseSystemPasswordChar = true;
			this.rolePwdBox.TextChanged += new System.EventHandler(this.rolePwdBox_TextChanged);
			// 
			// groupLabel
			// 
			this.groupLabel.Location = new System.Drawing.Point(16, 82);
			this.groupLabel.Name = "groupLabel";
			this.groupLabel.Size = new System.Drawing.Size(56, 23);
			this.groupLabel.TabIndex = 0;
			this.groupLabel.Text = "Group ID:";
			this.groupLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// groupBox
			// 
			this.groupBox.Location = new System.Drawing.Point(80, 82);
			this.groupBox.Name = "groupBox";
			this.groupBox.Size = new System.Drawing.Size(121, 21);
			this.groupBox.TabIndex = 7;
			// 
			// ConnectionEditorForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(632, 413);
			this.Controls.Add(this.tabs);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.buttonOK);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.Name = "ConnectionEditorForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Connection String Editor";
			this.Load += new System.EventHandler(this.ConnectionEditorForm_Load);
			this.tabs.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
			this.userGroup.ResumeLayout(false);
			this.userGroup.PerformLayout();
			this.serverGroup.ResumeLayout(false);
			this.tabPage2.ResumeLayout(false);
			this.groupConnection.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.timeoutUpDown)).EndInit();
			this.groupCredentials.ResumeLayout(false);
			this.groupCredentials.PerformLayout();
			this.ResumeLayout(false);

		}
		#endregion

		private void buttonOK_Click(object sender, System.EventArgs e)
		{
			// Lay down the new values into the NamedValueCollection.
			// Do not clear the nv since there may be some options
			// that are not in the editor dialog.  We do not want to
			// nv.Clear() since these options would be lost.
			SetConnectionStringValuesFromControls(this.nv);

			UpdateDropDownList(serverDropDownValues,  this.serverBox);
			UpdateDropDownList(portDropDownValues,    this.portBox);
			UpdateDropDownList(databaseDropDownValues,this.databaseBox);
			UpdateDropDownList(userDropDownValues,    this.userBox);
			UpdateDropDownList(roleDropDownValues,    this.roleBox);
			UpdateDropDownList(dbmsUserDropDownValues,this.dbmsUserBox);
			UpdateDropDownList(groupDropDownValues,   this.groupBox);
			//string connstring = IngresConnection.ToConnectionString(nv);

			// make a sanitized copy of the connection string keyword/values
			// that does not include the "Password" values
			ConnectStringConfig cleannv = this.nv;  // debugging

			cleannv = new ConnectStringConfig(this.nv);
			cleannv.Remove(DrvConst.DRV_PROP_PWD);
			cleannv.Remove(DrvConst.DRV_PROP_ROLEPWD);
			cleannv.Remove(DrvConst.DRV_PROP_DBPWD);

			string connectionStringLast =  // sanitized connection string
				ConnectStringConfig.ToConnectionString(cleannv);
			connectionStringLastValue.Clear();
			connectionStringLastValue.Add(connectionStringLast);

			// save dropdown values to ConnectionEditor.xml
			SaveValuesToApplicationDataFolder(
				dropDownValues, ApplicationDataFileName);
			//exit and leave the values in this.nv (form caller's nv)
		}

		private void buttonTestConnection_Click(object sender, System.EventArgs e)
		{
			// make a working copy of the connection string keyword/values
			ConnectStringConfig nv = new ConnectStringConfig(this.nv);

			// lay down the new values into the NamedValueCollection
			SetConnectionStringValuesFromControls(nv);

			try
			{
				TestConnection(ConnectStringConfig.ToConnectionString(nv));
			}
			catch(Exception ex)
			{
				MessageBox.Show(ex.ToString(), "Connection Test Failed");
				return;
			}

			MessageBox.Show("Connection successful!", "Connection Test");
		}

		private void TestConnection(string connectionString)
		{
			IngresConnection connection =
				new IngresConnection(connectionString);

			Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
			Cursor.Current = Cursors.WaitCursor;  // hourglass cursor
			try // to open the connection
			{
				connection.Open();  // try opening the connection
			}
			finally
			{
				Cursor.Current = cursor;  // restore Arrow cursor
			}
			connection.Close();
		}

		private void Advanced_SelectedIndexChanged(object sender, System.EventArgs e)
		{
		
		}

		private void ConnectionEditorForm_Load(object sender, System.EventArgs e)
		{
			System.Reflection.Assembly assembly = GetType().Module.Assembly;
			Bitmap bmp =
				new Bitmap(assembly.GetManifestResourceStream(bitmapName));
			this.pictureBox1.Image = bmp;
			this.pictureBox1.BorderStyle = BorderStyle.FixedSingle;
			this.pictureBox1.Size = new Size(bmp.Width, bmp.Height);

			// default to caller's ConnectionString name/value pairs
			NameValueCollection nv = this.nv;

			dropDownValues = new NamedArrayList[]
					{
						connectionStringLastValue =
						new NamedArrayList("ConnectionStringLast"),
						serverDropDownValues  = new NamedArrayList("Server"),
						portDropDownValues    = new NamedArrayList("Port"),
						databaseDropDownValues= new NamedArrayList("Database"),
						userDropDownValues    = new NamedArrayList("User"),
						groupDropDownValues   = new NamedArrayList("Group"),
						roleDropDownValues    = new NamedArrayList("Role"),
						dbmsUserDropDownValues= new NamedArrayList("DbmsUser")
					};
			LoadValuesFromApplicationDataFolder(  // load from ConnectionEditor.xml
				dropDownValues, ApplicationDataFileName);

			if (connectionStringLastValue.Count > 0)
				connectionStringLast = (string)connectionStringLastValue[0];

			if (nv.Count == 0  &&  // if caller has no connection string and
				connectionStringLast != null  &&
				connectionStringLast.Length > 0) // we had a last string
			{
				// use the last connection string the editor built
				try
				{
					nv = ConnectStringConfig.ParseConnectionString(
						connectionStringLast);
				}
				catch {}
			}
		
			this.serverBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_HOST);
			this.serverBox.Items.AddRange(serverDropDownValues.ToArray());
			if (serverDropDownValues.IndexOf("(local)") == -1)
				this.serverBox.Items.Add("(local)");

			this.portBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_PORT, "II7");
			this.portBox.Items.AddRange(portDropDownValues.ToArray());

			this.databaseBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_DB);
			this.databaseBox.Items.AddRange(databaseDropDownValues.ToArray());

			this.userBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_USR);
			this.userBox.Items.AddRange(userDropDownValues.ToArray());

			this.passwordBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_PWD);

			this.checkPersistSecurityInfo.Checked =
				GetValueOfKeyword(nv, DrvConst.DRV_PROP_PERSISTSEC, false);

			this.timeoutUpDown.Minimum = Decimal.MinValue;
			this.timeoutUpDown.Maximum = Decimal.MaxValue;
			this.timeoutUpDown.Value   = 
				GetValueOfKeyword(nv, DrvConst.DRV_PROP_TIMEOUT, 15);

			this.checkConnectionPooling.Checked =
				GetValueOfKeyword(nv, DrvConst.DRV_PROP_CLIENTPOOL, true);

			this.roleBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_ROLE);
			this.roleBox.Items.AddRange(roleDropDownValues.ToArray());

			this.rolePwdBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_ROLEPWD);

			this.dbmsUserBox.Text= GetValueOfKeyword(nv, DrvConst.DRV_PROP_DBUSR);
			this.dbmsUserBox.Items.AddRange(dbmsUserDropDownValues.ToArray());

			this.dbmsPwdBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_DBPWD);

			this.groupBox.Text = GetValueOfKeyword(nv, DrvConst.DRV_PROP_GRP);
			this.groupBox.Items.AddRange(groupDropDownValues.ToArray());
		}

		private void rolePwdBox_TextChanged(object sender, System.EventArgs e)
		{
			TextBox textbox = (TextBox) sender;
			textbox.Text = textbox.Text;
		}

		internal static string GetValueOfKeyword(NameValueCollection nv, string strKey)
		{
			string[] strArray = nv.GetValues(strKey);
			if (strArray == null)
				return null;

			string strValue = strArray[0];
			return strValue;  // return null or value associated with keyword
		}  // GetValueOfKeyword

		internal static string GetValueOfKeyword(
			NameValueCollection nv, string strKey, string defaultValue)
		{
			string strValue = GetValueOfKeyword(nv, strKey);
			if (strValue != null)
				return strValue;  // value associated with keyword
			return defaultValue;
		}  // GetValueOfKeyword

		internal static decimal GetValueOfKeyword(
			NameValueCollection nv, string strKey, decimal defaultValue)
		{
			string strValue = GetValueOfKeyword(nv, strKey);
			if (strValue != null)    // return value associated with keyword
				return Convert.ToDecimal(strValue);
			return defaultValue;  // no entry, therefore return default
		}  // GetValueOfKeyword

		internal static Boolean GetValueOfKeyword(
			NameValueCollection nv, string strKey, Boolean defaultValue)
		{
			string strValue = GetValueOfKeyword(nv, strKey);
			if (strValue != null)    // return value associated with keyword
			{
				strValue = strValue.ToLower(
					System.Globalization.CultureInfo.InvariantCulture);
				if (strValue.Equals("true")  ||  strValue.Equals("yes"))
					return true;
				return false;
			}
			return defaultValue;  // no entry, therefore return default
		}  // GetValueOfKeyword


		// lay down the new values into the NamedValueCollection
		private void SetConnectionStringValuesFromControls(
			NameValueCollection nv)
		{
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_HOST,      this.serverBox);
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_PORT,      this.portBox, "II7");
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_DB,        this.databaseBox);
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_USR,       this.userBox);
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_PWD,       this.passwordBox);
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_PERSISTSEC,this.checkPersistSecurityInfo,
				"false");
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_TIMEOUT,this.timeoutUpDown.Value.ToString(),
				"15");
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_CLIENTPOOL,this.checkConnectionPooling,
				"true");
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_ROLE,      this.roleBox);
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_ROLEPWD,   this.rolePwdBox);
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_DBUSR,     this.dbmsUserBox);
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_DBPWD,     this.dbmsPwdBox);
			SetStringValueOfKeyword(
				nv, DrvConst.DRV_PROP_GRP,       this.groupBox);
		}

		private static void SetStringValueOfKeyword(
			NameValueCollection nv,
			string strKey,
			Control ctrl)
		{
			SetStringValueOfKeyword(nv, strKey, ctrl, null);
		}

		private static void SetStringValueOfKeyword(
			NameValueCollection nv,
			string strKey,
			CheckBox ctrl,
			string defaultValue )
		{
			String  strValue = ctrl.Checked ? "true" : "false";
			SetStringValueOfKeyword(nv, strKey, strValue, defaultValue );
		}

		private static void SetStringValueOfKeyword(
			NameValueCollection nv,
			string strKey,
			Control ctrl,
			string defaultValue )
		{
			SetStringValueOfKeyword(nv, strKey, ctrl.Text, defaultValue );
		}

		private static void SetStringValueOfKeyword(
			NameValueCollection nv,
			string strKey,
			string text,
			string defaultValue )
		{
			string strValue;
			string strKeyOriginal = DrvConst.GetKey(strKey);

			if (text == null)
				text = String.Empty;

			string[] strArray = nv.GetValues(strKey);

			if (strArray == null)  // if no old values
			{
				if (text.Length == 0)  // if no new values, return
					return;
				if (defaultValue != null  &&  defaultValue.Equals(text))
					return;  // don't add to connection string if default
				nv.Set(strKey, text);      // set brand new values
				nv.Add(strKey, strKeyOriginal);
				return;
			}

			strValue       = strArray[0];  // old value
			strKeyOriginal = strArray[1];  // key name as specified by user

			if (strValue.Equals(text))  // return if no change in value
				return;

			nv.Set(strKey, text);       // replace old with new values
			nv.Add(strKey, strKeyOriginal);
		}


		private static XmlTextReader OpenApplicationDataFileReader(string filename)
		{
			if (filename == null  ||  filename.Length == 0)  // safety check
				return null;

			// build "C:\Documents and Settings\myuserid\Local Settings\" +
			//       "Application Data\IngresCorporation\"+
			//       "Ingres\.NETDataProvider"
			string applicationDataFileName =
				Environment.GetFolderPath(
				Environment.SpecialFolder.LocalApplicationData);
			if (applicationDataFileName == null  ||
				applicationDataFileName.Length == 0)  // safety check
				return null;

			applicationDataFileName += VSNETConst.strApplicationDataFolder;
			applicationDataFileName += "\\" + filename + ".xml";

			FileInfo fileInfo = new FileInfo(applicationDataFileName);
			if (fileInfo.Exists == false)
				return null;

			//MessageBox.Show("Application Data Filename = " + applicationDataFileName +
			//	";  Length = " + applicationDataFileName.Length.ToString());
			//return null;

			XmlTextReader xmlreader =
				new XmlTextReader(applicationDataFileName);

			return xmlreader;
		}

		private static XmlTextReader CloseApplicationDataFileReader(
			XmlTextReader xmlreader)
		{
			if (xmlreader == null)
				return null;

			xmlreader.Close();
			return null;
		}

		private static XmlTextWriter OpenApplicationDataFileWriter(
			string title, string filename)
		{
			if (filename == null  ||  filename.Length == 0)  // safety check
				return null;

			// build "C:\Documents and Settings\myuserid\Local Settings\" +
			//       "Application Data\IngresCorporation\"+
			//       "Ingres\.NETDataProvider"
			string applicationDataFileName =
				Environment.GetFolderPath(
				Environment.SpecialFolder.LocalApplicationData);
			if (applicationDataFileName        == null  ||
				applicationDataFileName.Length == 0)  // safety check
				return null;

			applicationDataFileName += VSNETConst.strApplicationDataFolder;
			DirectoryInfo dirInfo = new DirectoryInfo(applicationDataFileName);
			if (dirInfo.Exists == false)
				dirInfo.Create();         // create the missing directory

			applicationDataFileName += "\\" + filename + ".xml";

			//MessageBox.Show("Application Data Filename = " + applicationDataFileName +
			//	";  Length = " + applicationDataFileName.Length.ToString());
			//return null;

			XmlTextWriter xmlwriter =
				new XmlTextWriter(
				applicationDataFileName, System.Text.Encoding.Default);

			xmlwriter.Formatting = Formatting.Indented;
			xmlwriter.WriteStartDocument();

			xmlwriter.WriteComment(
				"Ingres .NET Data Provider data file - Do not modify!");
			xmlwriter.WriteStartElement(title);

			return xmlwriter;
		}

		private static XmlTextWriter CloseApplicationDataFileWriter(
			XmlTextWriter xmlwriter)
		{
			if (xmlwriter == null)
				return null;

			xmlwriter.WriteEndElement();
			xmlwriter.WriteEndDocument();
			xmlwriter.Flush();
			xmlwriter.Close();
			return null;
		}

		internal static void LoadValuesFromApplicationDataFolder(
			NamedArrayList[] arrayOfArrayList, string filename)
		{
			NamedArrayList valuelist = null;
			if (arrayOfArrayList == null  ||  arrayOfArrayList.Length == 0)
				return;

			try
			{
				XmlTextReader xmlreader =
					OpenApplicationDataFileReader(filename);
				if (xmlreader == null)
					return;

				while(xmlreader.Read())
				{
					switch (xmlreader.NodeType)
					{
						case XmlNodeType.Element:
							if (xmlreader.HasAttributes)
							{
								valuelist = null;
								string listname = xmlreader[0];
								foreach(NamedArrayList array
											in arrayOfArrayList)
								{
									if (listname.Equals(array.Name))
									{
										valuelist = array;
										break;
									}
								}  // end foreach
							}  // end if
							break;

						case XmlNodeType.Text:
							if (valuelist == null)
								break;
							string str = xmlreader.Value;
							valuelist.Add(str);
							break;

					}  // end switch
				}  // end while

				CloseApplicationDataFileReader(xmlreader);
			}  // end try

			catch(Exception)  // don't let SecurityException stop us
			{                 // no dropdown list is not a critical error
			}
		}  // LoadValuesFromApplicationDataFolder


		internal static void SaveValuesToApplicationDataFolder(
			NamedArrayList[] arrayOfArrayList, string filename)
		{
			if (arrayOfArrayList == null  ||  arrayOfArrayList.Length == 0)
				return;

			try
			{
				XmlTextWriter xmlwriter =
					OpenApplicationDataFileWriter("configuration", filename);
				if (xmlwriter == null)
					return;

				foreach(NamedArrayList array in arrayOfArrayList)
				{
					if (array == null  ||  array.Count == 0)
						continue;

					xmlwriter.WriteStartElement("list");
					xmlwriter.WriteAttributeString("name", array.Name);
					//xmlwriter.WriteEndAttribute();

					int i = 0;
					foreach (string s in array)
					{
						if (++i > 10)  // limit to last 10 items
							break;
						xmlwriter.WriteElementString("value", s);
					}  // end foreach through strings
					xmlwriter.WriteEndElement();
				}  // end foreach through arrays

				CloseApplicationDataFileWriter(xmlwriter);
			}  // end try

			catch(Exception)  // don't let SecurityException stop us
			{}                // no dropdown list is not a critical error
		}  // SaveValuesToApplicationDataFolder


		internal static NamedArrayList UpdateDropDownList(
			NamedArrayList array, Control ctrl)
		{
			return UpdateDropDownList(array, ctrl.Text);
		}

		internal static NamedArrayList UpdateDropDownList(
			NamedArrayList array, string str)
		{
			if (str == null)  // if no string, do nothing
				return array;
			str = str.Trim();  // remove whitespace from beginning and end
			if (str.Length == 0)  // if no string, do nothing
				return array;

			int index = array.IndexOf(str); // find if already in list
			if (index != -1)                // if already in list,
				array.RemoveAt(index);      // remove it, and
			array.Insert(0, str);           // insert it at head of list
			return array;                   // return the updated list
		}

		internal class NamedArrayList : ArrayList
		{
			internal NamedArrayList(string name)
			{
				Name = name;
			}

			private  string _name;
			internal string  Name
			{
				get { return _name;  }
				set { _name = value; }
			}
		}

	}  // class ConnectionEditorForm


	
	/*
	** Name: ConnectionEditor class
	**
	** Description:
	**	Create the Connection object
	**	in the components tray of the designer.
	**
	** Called by:
	**	DataAdapter Wizard
	**	ConnectionString UITypeEditor
	**
	** History:
	**	17-Mar-03 (thoda04)
	**	    Created.
	*/

	internal class ConnectionEditor
	{
		private IngresConnection connection;
		private ConnectStringConfig nv;

		internal ConnectionEditor() : this(new IngresConnection())
		{
		}


		internal ConnectionEditor(IngresConnection connectionParm)
		{
			connection = connectionParm;

			// format the connection string in key/value pairs
			nv = ConnectStringConfig.ParseConnectionString(
				connection.ConnectionString);
			// the editor will update the key/value pairs

			Form dlgConnection = new ConnectionEditorForm(nv);
			DialogResult result = dlgConnection.ShowDialog();

			if (result == DialogResult.Cancel) // if Cancel button, return
			{
				//MessageBox.Show("Connection DialogResult.Cancel", "ConnectionEditor");
				return;
			}
			// else (result == DialogResult.OK)     // if Next button, go on

			// rebuild the updated key/value pairs back into a connection string
			connection.ConnectionString =
				ConnectStringConfig.ToConnectionString(nv);
		}

		internal ConnectionEditor(
			IngresDataAdapter adapter,
			System.ComponentModel.Design.IDesignerHost host)
		{
			if (adapter != null  &&
				adapter.SelectCommand != null  &&
				adapter.SelectCommand.Connection != null)
				connection = adapter.SelectCommand.Connection;
			else
				connection = new IngresConnection();

			// format the connection string in key/value pairs
			nv = ConnectStringConfig.ParseConnectionString(
				connection.ConnectionString);
			// the editor will update the key/value pairs

			Form dlgConnection = new ConnectionEditorForm(nv);
			DialogResult result = dlgConnection.ShowDialog();

			if (result == DialogResult.Cancel) // if Cancel button, return
			{
				//MessageBox.Show("Connection DialogResult.Cancel", "ConnectionEditor");
				return;
			}
			// else (result == DialogResult.OK)     // if Next button, go on

			// rebuild the updated key/value pairs back into a connection string
			connection.ConnectionString = ConnectStringConfig.ToConnectionString(nv);

			// connect the Connection object to the four Adapter commands
			if (adapter != null)
			{
				if (adapter.SelectCommand != null)
					adapter.SelectCommand.Connection = connection;
				if (adapter.InsertCommand != null)
					adapter.InsertCommand.Connection = connection;
				if (adapter.UpdateCommand != null)
					adapter.UpdateCommand.Connection = connection;
				if (adapter.DeleteCommand != null)
					adapter.DeleteCommand.Connection = connection;
			}
		}  //  ConnectionEditor

	}  // class ConnectionEditor



	/*
	** Name: ConnectionToolboxItem class
	**
	** Description:
	**	ToolboxItem object that holds a place
	**	in the Visual Studio .NET toolbox property page.
	**
	** Called by:
	**	DataAdapter Wizard
	**	ConnectionString UITypeEditor
	**
	** History:
	**	17-Mar-03 (thoda04)
	**	    Created.
	*/

	/// <summary>
	/// ToolboxItem for Connection object to launch the Connection
	/// Editor and include the object into the component tray.
	/// </summary>
	[System.Runtime.InteropServices.ComVisible(true)]
	[Serializable]
	public sealed class IngresConnectionToolboxItem :
		System.Drawing.Design.ToolboxItem
	{
		//static string CLASSNAME = "IngresConnectionToolboxItem";

		public IngresConnectionToolboxItem() : base()
		{
		}

		public IngresConnectionToolboxItem(Type type) : base(type)
		{
		}

		private IngresConnectionToolboxItem(Bitmap icon)
		{
			DisplayName = "IngresConnection";
			Bitmap = icon;
		}


		private IngresConnectionToolboxItem(
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
			CreateComponentsCore(System.ComponentModel.Design.IDesignerHost host)
		{
			//MessageBox.Show("ConnectionToolboxItem.CreateComponentsCore called!");

			const string prefix = VSNETConst.shortTitle;  // "Ingres"
			IContainer designerHostContainer = host.Container;
			//ComponentCollection components   = null;
			ArrayList newComponents          = new ArrayList();
			ArrayList newCommandComponents   = new ArrayList();
			string    name;

			if (designerHostContainer == null)  // safety check
				return null;

			IngresConnection connection = new IngresConnection();

			// Once a reference to an assembly has been added to this service,
			// this service can load types from names that
			// do not specify an assembly.
			ITypeResolutionService resService =
				(ITypeResolutionService)host.GetService(typeof(ITypeResolutionService));

			System.Reflection.Assembly assembly = connection.GetType().Module.Assembly;

			if (resService != null)
			{
				resService.ReferenceAssembly(assembly.GetName());
				// set the assembly name to load types from
			}

			name = DesignerNameManager.CreateUniqueName(
				designerHostContainer, prefix, "Connection");
			try
			{
				designerHostContainer.Add(connection, name);
				newComponents.Add(connection);
			}
			catch (ArgumentException ex)
			{
				string exMsg = ex.ToString() +
					"\nRemove IngresConnection component '" + name + "'" +
					" that is defined outside of the designer. ";
				MessageBox.Show(exMsg, "Add " + name + " failed");
				return null;
			}

			// invoke the wizard to create the connection and query
			ConnectionEditor wizard = new ConnectionEditor(connection);


			return (IComponent[])(newComponents.ToArray(typeof(IComponent)));
			//return base.CreateComponentsCore(host);

		}  // CreateComponentsCore
	}  // class IngresConnectionToolboxItem


	/*
	** Name: ConnectionStringUITypeEditor class
	**
	** Description:
	**	UserInterface editor used by the VS.NET designer.
	**
	** History:
	**	17-Mar-03 (thoda04)
	**	    Created.
	*/

	public class ConnectionStringUITypeEditor : UITypeEditor
	{
		public ConnectionStringUITypeEditor() : base()
		{
		}

		public object EditIngresValue(
			ITypeDescriptorContext context,
			IServiceProvider       provider,
			object                 value)
		{
			return EditValue(context, provider, value);
		}

		public override object EditValue(
			ITypeDescriptorContext context,
			IServiceProvider       provider,
			object                 value)
		{
			if (context          != null  &&
				context.Instance != null  &&
				provider         != null)
			{
				IWindowsFormsEditorService edSvc =
					(IWindowsFormsEditorService)provider.GetService(
					typeof(IWindowsFormsEditorService));
				if (edSvc != null)
				{
					// set connection string into an IngresConnection
					IngresConnection connection =
						new IngresConnection((string)value);
					// call the dialog for building/modifying the connection str
					ConnectionEditor editor =
						new ConnectionEditor(connection);
					value = connection.ConnectionString;  // return updated string
				}  // end if (edSvc != null)
			} // end if context is OK
			return value;
		}

		public override UITypeEditorEditStyle
			GetEditStyle(ITypeDescriptorContext context)
		{

			if (context          != null  &&
				context.Instance != null)
				return UITypeEditorEditStyle.Modal;
			return base.GetEditStyle(context);
		}
	}  // class ConnectionStringUITypeEditor


}  // namespace
