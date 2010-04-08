/*
** Copyright (c) 2004, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Data;
using System.Drawing;
using System.Drawing.Design;
using System.Collections;
using System.ComponentModel;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.Design;
using Ingres.ProviderInternals;

namespace Ingres.Client.Design
{
	/// <summary>
	/// Summary description for vsquerydesigner.
	/// </summary>
	public class QueryDesignerForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Button btnOK;
		private System.Windows.Forms.Panel panelMain;
		private System.Windows.Forms.Panel panelTables;
		private System.Windows.Forms.Splitter splitter1;
		private System.Windows.Forms.DataGrid gridColumns;
		private System.Windows.Forms.Splitter splitter2;
		private System.Windows.Forms.RichTextBox textQuery;
		//private System.Data.DataTable dataTableColumns;
		//private System.Windows.Forms.DataGridTableStyle dataGridTableStyle1;
		private System.Data.DataSet dataSetColumns;
		private System.Data.DataTable dataTableColumns;
		private System.Data.DataColumn dataColumn1;
		private System.Data.DataColumn dataColumn2;
		private System.Data.DataColumn dataColumn3;
		private System.Data.DataColumn dataColumn4;
		private System.Data.DataColumn dataColumn5;
		private System.Data.DataColumn dataColumn6;
		private System.Data.DataColumn dataColumn7;
		private System.Windows.Forms.DataGridTableStyle dataGridTableStyle1;
		private System.Windows.Forms.DataGridTextBoxColumn dataGridTextBoxColumn1;
		private System.Windows.Forms.DataGridTextBoxColumn dataGridTextBoxColumn2;
		private System.Windows.Forms.DataGridTextBoxColumn dataGridTextBoxColumn3;
		private System.Windows.Forms.DataGridTextBoxColumn dataGridTextBoxColumn5;
		private System.Windows.Forms.DataGridTextBoxColumn dataGridTextBoxColumn6;
		private float proportionPanelTables = 0.333f;
		private float proportionGridColumns = 0.333f;
		private System.Windows.Forms.TabControl tabTableColumns;
		private System.Windows.Forms.TabPage tabPage;
		private System.Windows.Forms.Label labelTableColumns;
		private System.Windows.Forms.CheckedListBox checkedListTableColumns;
		private MetaData md = new MetaData(String.Empty);
		private System.Windows.Forms.Panel panelKey;  // SELECT's metadata
		private string textQueryEnterText = "";
		private System.Windows.Forms.Label labelDebug;
		private System.Data.DataColumn dataColumn8;
		private bool  rebuildDataGrid = false;
		private const string constAddTable = "Add Table...";

		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public QueryDesignerForm(IngresCommand providerCmd) :
			this(providerCmd.Connection, providerCmd.CommandText)
		{
		}

		public QueryDesignerForm(
			IngresConnection connection, string commandText)
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			// remember the command text and connection to work against
			if (commandText != null)
			{
				string savedConnectionString;  // saved connection string

				if (commandText != null)
					this.CommandText = commandText.Trim();

				this.Connection = connection;
				if (this.Connection != null)
				{
					Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
					Cursor.Current = Cursors.WaitCursor;  // hourglass cursor

					// save the connection because 'Persist Security Info'
					// option will destroy the password during open
					savedConnectionString = this.Connection.ConnectionString;

					try // to open the connection
					{
						this.Connection.Open();  // try opening the connection
						_catalog= this.Connection.OpenCatalog();
					}
					catch (Exception ex)
					{
						MessageBox.Show(
							ex.ToString(),
							VSNETConst.shortTitle +
							" Query Designer    Server Is Not Available");
						this.Connection = null;
					}
					finally
					{
						Cursor.Current = cursor;  // restore Arrow cursor

						//restore the ConnectionString if changed by Open()
						if (this.Connection != null  &&
							this.Connection.ConnectionString!= savedConnectionString)
							this.Connection.ConnectionString = savedConnectionString;
					}
				}
			}
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		/*
		** CommandText property
		*/
		private string _commandText = String.Empty;
		[Description("SQL statement string built or initially set.")]
		public string CommandText
		{
			get { return _commandText;  }
			set  { _commandText = value;  }
		}


		/*
		** Connection property
		*/
		IngresConnection      _connection;
		[Description("The Connection to be used by this query designer.")]
		public IngresConnection Connection
		{
			get {	return _connection;  }
			set {	_connection = (IngresConnection)value;  }
		}

		/*
		** Catalog property
		*/
		Catalog      _catalog;
		[Description("The Catalog information associated with this connection.")]
		internal Catalog Catalog
		{
			get {	return _catalog;  }
			set {	_catalog = (Catalog)value;  }
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.btnCancel = new System.Windows.Forms.Button();
			this.btnOK = new System.Windows.Forms.Button();
			this.panelMain = new System.Windows.Forms.Panel();
			this.textQuery = new System.Windows.Forms.RichTextBox();
			this.splitter2 = new System.Windows.Forms.Splitter();
			this.gridColumns = new System.Windows.Forms.DataGrid();
			this.dataSetColumns = new System.Data.DataSet();
			this.dataTableColumns = new System.Data.DataTable();
			this.dataColumn1 = new System.Data.DataColumn();
			this.dataColumn2 = new System.Data.DataColumn();
			this.dataColumn3 = new System.Data.DataColumn();
			this.dataColumn4 = new System.Data.DataColumn();
			this.dataColumn5 = new System.Data.DataColumn();
			this.dataColumn6 = new System.Data.DataColumn();
			this.dataColumn7 = new System.Data.DataColumn();
			this.dataColumn8 = new System.Data.DataColumn();
			this.dataGridTableStyle1 = new System.Windows.Forms.DataGridTableStyle();
			this.dataGridTextBoxColumn1 = new System.Windows.Forms.DataGridTextBoxColumn();
			this.dataGridTextBoxColumn2 = new System.Windows.Forms.DataGridTextBoxColumn();
			this.dataGridTextBoxColumn3 = new System.Windows.Forms.DataGridTextBoxColumn();
			this.dataGridTextBoxColumn5 = new System.Windows.Forms.DataGridTextBoxColumn();
			this.dataGridTextBoxColumn6 = new System.Windows.Forms.DataGridTextBoxColumn();
			this.splitter1 = new System.Windows.Forms.Splitter();
			this.panelTables = new System.Windows.Forms.Panel();
			this.tabTableColumns = new System.Windows.Forms.TabControl();
			this.tabPage = new System.Windows.Forms.TabPage();
			this.checkedListTableColumns = new System.Windows.Forms.CheckedListBox();
			this.panelKey = new System.Windows.Forms.Panel();
			this.labelTableColumns = new System.Windows.Forms.Label();
			this.labelDebug = new System.Windows.Forms.Label();
			this.panelMain.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.gridColumns)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.dataSetColumns)).BeginInit();
			((System.ComponentModel.ISupportInitialize)(this.dataTableColumns)).BeginInit();
			this.panelTables.SuspendLayout();
			this.tabTableColumns.SuspendLayout();
			this.tabPage.SuspendLayout();
			this.SuspendLayout();
			// 
			// btnCancel
			// 
			this.btnCancel.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right);
			this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnCancel.Location = new System.Drawing.Point(480, 400);
			this.btnCancel.Name = "btnCancel";
			this.btnCancel.TabIndex = 1;
			this.btnCancel.Text = "Cancel";
			// 
			// btnOK
			// 
			this.btnOK.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right);
			this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.btnOK.Location = new System.Drawing.Point(384, 400);
			this.btnOK.Name = "btnOK";
			this.btnOK.TabIndex = 0;
			this.btnOK.Text = "OK";
			this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
			// 
			// panelMain
			// 
			this.panelMain.Anchor = (((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right);
			this.panelMain.Controls.AddRange(new System.Windows.Forms.Control[] {
																					this.textQuery,
																					this.splitter2,
																					this.gridColumns,
																					this.splitter1,
																					this.panelTables});
			this.panelMain.Location = new System.Drawing.Point(24, 16);
			this.panelMain.Name = "panelMain";
			this.panelMain.Size = new System.Drawing.Size(536, 368);
			this.panelMain.TabIndex = 2;
			this.panelMain.Resize += new System.EventHandler(this.panelMain_Resize);
			// 
			// textQuery
			// 
			this.textQuery.AcceptsTab = true;
			this.textQuery.DetectUrls = false;
			this.textQuery.Dock = System.Windows.Forms.DockStyle.Fill;
			this.textQuery.Font = new System.Drawing.Font("Microsoft Sans Serif", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((System.Byte)(0)));
			this.textQuery.Location = new System.Drawing.Point(0, 228);
			this.textQuery.Name = "textQuery";
			this.textQuery.Size = new System.Drawing.Size(536, 140);
			this.textQuery.TabIndex = 4;
			this.textQuery.Text = "";
			this.textQuery.Leave += new System.EventHandler(this.textQuery_Leave);
			this.textQuery.Enter += new System.EventHandler(this.textQuery_Enter);
			// 
			// splitter2
			// 
			this.splitter2.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.splitter2.Dock = System.Windows.Forms.DockStyle.Top;
			this.splitter2.Location = new System.Drawing.Point(0, 224);
			this.splitter2.Name = "splitter2";
			this.splitter2.Size = new System.Drawing.Size(536, 4);
			this.splitter2.TabIndex = 3;
			this.splitter2.TabStop = false;
			this.splitter2.SplitterMoving += new System.Windows.Forms.SplitterEventHandler(this.splitter2_SplitterMoving);
			// 
			// gridColumns
			// 
			this.gridColumns.AllowSorting = false;
			this.gridColumns.BackgroundColor = System.Drawing.SystemColors.Control;
			this.gridColumns.CaptionVisible = false;
			this.gridColumns.DataMember = "Columns";
			this.gridColumns.DataSource = this.dataSetColumns;
			this.gridColumns.Dock = System.Windows.Forms.DockStyle.Top;
			this.gridColumns.HeaderForeColor = System.Drawing.SystemColors.ControlText;
			this.gridColumns.Location = new System.Drawing.Point(0, 156);
			this.gridColumns.Name = "gridColumns";
			this.gridColumns.Size = new System.Drawing.Size(536, 68);
			this.gridColumns.TabIndex = 2;
			this.gridColumns.TableStyles.AddRange(new System.Windows.Forms.DataGridTableStyle[] {
																									this.dataGridTableStyle1});
			this.gridColumns.CurrentCellChanged += new System.EventHandler(this.gridColumns_CurrentCellChanged);
			this.gridColumns.Leave += new System.EventHandler(this.gridColumns_Leave);
			// 
			// dataSetColumns
			// 
			this.dataSetColumns.DataSetName = "NewDataSet";
			this.dataSetColumns.Locale = new System.Globalization.CultureInfo("");
			this.dataSetColumns.Tables.AddRange(new System.Data.DataTable[] {
																				this.dataTableColumns});
			// 
			// dataTableColumns
			// 
			this.dataTableColumns.Columns.AddRange(new System.Data.DataColumn[] {
																					this.dataColumn1,
																					this.dataColumn2,
																					this.dataColumn3,
																					this.dataColumn4,
																					this.dataColumn5,
																					this.dataColumn6,
																					this.dataColumn7,
																					this.dataColumn8});
			this.dataTableColumns.Constraints.AddRange(new System.Data.Constraint[] {
																						new System.Data.UniqueConstraint("Constraint1", new string[] {
																																						 "DataTable Primary Key"}, true)});
			this.dataTableColumns.Locale = new System.Globalization.CultureInfo("");
			this.dataTableColumns.PrimaryKey = new System.Data.DataColumn[] {
																				this.dataColumn8};
			this.dataTableColumns.TableName = "Columns";
			this.dataTableColumns.ColumnChanged += new System.Data.DataColumnChangeEventHandler(this.dataTableColumns_ColumnChanged);
			this.dataTableColumns.ColumnChanging += new System.Data.DataColumnChangeEventHandler(this.dataTableColumns_ColumnChanging);
			this.dataTableColumns.RowDeleting += new System.Data.DataRowChangeEventHandler(this.dataTableColumns_RowDeleting);
			this.dataTableColumns.RowDeleted += new System.Data.DataRowChangeEventHandler(this.dataTableColumns_RowDeleted);
			// 
			// dataColumn1
			// 
			this.dataColumn1.ColumnName = "Column Name";
			// 
			// dataColumn2
			// 
			this.dataColumn2.ColumnName = "Alias";
			// 
			// dataColumn3
			// 
			this.dataColumn3.ColumnName = "Table";
			// 
			// dataColumn4
			// 
			this.dataColumn4.ColumnName = "Output";
			this.dataColumn4.DataType = typeof(bool);
			this.dataColumn4.DefaultValue = true;
			// 
			// dataColumn5
			// 
			this.dataColumn5.ColumnName = "Sort Type";
			// 
			// dataColumn6
			// 
			this.dataColumn6.ColumnName = "Sort Order";
			this.dataColumn6.DataType = typeof(short);
			// 
			// dataColumn7
			// 
			this.dataColumn7.ColumnName = "Criteria";
			// 
			// dataColumn8
			// 
			this.dataColumn8.AllowDBNull = false;
			this.dataColumn8.AutoIncrement = true;
			this.dataColumn8.ColumnName = "DataTable Primary Key";
			this.dataColumn8.DataType = typeof(int);
			this.dataColumn8.ReadOnly = true;
			// 
			// dataGridTableStyle1
			// 
			this.dataGridTableStyle1.AllowSorting = false;
			this.dataGridTableStyle1.DataGrid = this.gridColumns;
			this.dataGridTableStyle1.GridColumnStyles.AddRange(new System.Windows.Forms.DataGridColumnStyle[] {
																												  this.dataGridTextBoxColumn1,
																												  this.dataGridTextBoxColumn2,
																												  this.dataGridTextBoxColumn3,
																												  this.dataGridTextBoxColumn5,
																												  this.dataGridTextBoxColumn6});
			this.dataGridTableStyle1.HeaderForeColor = System.Drawing.SystemColors.ControlText;
			this.dataGridTableStyle1.MappingName = "Columns";
			// 
			// dataGridTextBoxColumn1
			// 
			this.dataGridTextBoxColumn1.Format = "";
			this.dataGridTextBoxColumn1.FormatInfo = null;
			this.dataGridTextBoxColumn1.HeaderText = "Column";
			this.dataGridTextBoxColumn1.MappingName = "Column Name";
			this.dataGridTextBoxColumn1.NullText = "";
			this.dataGridTextBoxColumn1.Width = 150;
			// 
			// dataGridTextBoxColumn2
			// 
			this.dataGridTextBoxColumn2.Format = "";
			this.dataGridTextBoxColumn2.FormatInfo = null;
			this.dataGridTextBoxColumn2.HeaderText = "Alias";
			this.dataGridTextBoxColumn2.MappingName = "Alias";
			this.dataGridTextBoxColumn2.NullText = "";
			this.dataGridTextBoxColumn2.Width = 75;
			// 
			// dataGridTextBoxColumn3
			// 
			this.dataGridTextBoxColumn3.Format = "";
			this.dataGridTextBoxColumn3.FormatInfo = null;
			this.dataGridTextBoxColumn3.HeaderText = "Table";
			this.dataGridTextBoxColumn3.MappingName = "Table";
			this.dataGridTextBoxColumn3.NullText = "";
			this.dataGridTextBoxColumn3.Width = 150;
			// 
			// dataGridTextBoxColumn5
			// 
			this.dataGridTextBoxColumn5.Format = "";
			this.dataGridTextBoxColumn5.FormatInfo = null;
			this.dataGridTextBoxColumn5.HeaderText = "Sort Type";
			this.dataGridTextBoxColumn5.MappingName = "Sort Type";
			this.dataGridTextBoxColumn5.NullText = "";
			this.dataGridTextBoxColumn5.ReadOnly = true;
			this.dataGridTextBoxColumn5.Width = 60;
			// 
			// dataGridTextBoxColumn6
			// 
			this.dataGridTextBoxColumn6.Format = "";
			this.dataGridTextBoxColumn6.FormatInfo = null;
			this.dataGridTextBoxColumn6.HeaderText = "Sort Order";
			this.dataGridTextBoxColumn6.MappingName = "Sort Order";
			this.dataGridTextBoxColumn6.NullText = "";
			this.dataGridTextBoxColumn6.ReadOnly = true;
			this.dataGridTextBoxColumn6.Width = 60;
			// 
			// splitter1
			// 
			this.splitter1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.splitter1.Dock = System.Windows.Forms.DockStyle.Top;
			this.splitter1.Location = new System.Drawing.Point(0, 152);
			this.splitter1.Name = "splitter1";
			this.splitter1.Size = new System.Drawing.Size(536, 4);
			this.splitter1.TabIndex = 1;
			this.splitter1.TabStop = false;
			this.splitter1.SplitterMoving += new System.Windows.Forms.SplitterEventHandler(this.splitter1_SplitterMoving);
			// 
			// panelTables
			// 
			this.panelTables.BackColor = System.Drawing.SystemColors.Window;
			this.panelTables.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
			this.panelTables.Controls.AddRange(new System.Windows.Forms.Control[] {
																					  this.tabTableColumns});
			this.panelTables.Dock = System.Windows.Forms.DockStyle.Top;
			this.panelTables.Name = "panelTables";
			this.panelTables.Size = new System.Drawing.Size(536, 152);
			this.panelTables.TabIndex = 0;
			// 
			// tabTableColumns
			// 
			this.tabTableColumns.Controls.AddRange(new System.Windows.Forms.Control[] {
																						  this.tabPage});
			this.tabTableColumns.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tabTableColumns.Name = "tabTableColumns";
			this.tabTableColumns.SelectedIndex = 0;
			this.tabTableColumns.Size = new System.Drawing.Size(532, 148);
			this.tabTableColumns.SizeMode = System.Windows.Forms.TabSizeMode.FillToRight;
			this.tabTableColumns.TabIndex = 0;
			this.tabTableColumns.Resize += new System.EventHandler(this.tabTableColumns_Resize);
			// 
			// tabPage
			// 
			this.tabPage.BackColor = System.Drawing.SystemColors.Window;
			this.tabPage.Controls.AddRange(new System.Windows.Forms.Control[] {
																				  this.checkedListTableColumns,
																				  this.panelKey,
																				  this.labelTableColumns});
			this.tabPage.Location = new System.Drawing.Point(4, 22);
			this.tabPage.Name = "tabPage";
			this.tabPage.Size = new System.Drawing.Size(524, 122);
			this.tabPage.TabIndex = 0;
			// 
			// checkedListTableColumns
			// 
			this.checkedListTableColumns.BorderStyle = System.Windows.Forms.BorderStyle.None;
			this.checkedListTableColumns.CheckOnClick = true;
			this.checkedListTableColumns.Location = new System.Drawing.Point(24, 23);
			this.checkedListTableColumns.Name = "checkedListTableColumns";
			this.checkedListTableColumns.Size = new System.Drawing.Size(552, 75);
			this.checkedListTableColumns.TabIndex = 2;
			// 
			// panelKey
			// 
			this.panelKey.BackColor = System.Drawing.SystemColors.Window;
			this.panelKey.Location = new System.Drawing.Point(0, 23);
			this.panelKey.Name = "panelKey";
			this.panelKey.Size = new System.Drawing.Size(4, 80);
			this.panelKey.TabIndex = 1;
			// 
			// labelTableColumns
			// 
			this.labelTableColumns.Dock = System.Windows.Forms.DockStyle.Top;
			this.labelTableColumns.Name = "labelTableColumns";
			this.labelTableColumns.Size = new System.Drawing.Size(524, 23);
			this.labelTableColumns.TabIndex = 0;
			this.labelTableColumns.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
			// 
			// labelDebug
			// 
			this.labelDebug.Location = new System.Drawing.Point(32, 400);
			this.labelDebug.Name = "labelDebug";
			this.labelDebug.Size = new System.Drawing.Size(312, 40);
			this.labelDebug.TabIndex = 3;
			// 
			// QueryDesignerForm
			// 
			this.AcceptButton = this.btnOK;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.btnCancel;
			this.ClientSize = new System.Drawing.Size(584, 445);
			this.ControlBox = false;
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.labelDebug,
																		  this.panelMain,
																		  this.btnOK,
																		  this.btnCancel});
			this.Name = "QueryDesignerForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Query Designer";
			this.Closing += new System.ComponentModel.CancelEventHandler(this.QueryDesignerForm_Closing);
			this.Load += new System.EventHandler(this.QueryDesignerForm_Load);
			this.Activated += new System.EventHandler(this.QueryDesignerForm_Activated);
			this.panelMain.ResumeLayout(false);
			((System.ComponentModel.ISupportInitialize)(this.gridColumns)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.dataSetColumns)).EndInit();
			((System.ComponentModel.ISupportInitialize)(this.dataTableColumns)).EndInit();
			this.panelTables.ResumeLayout(false);
			this.tabTableColumns.ResumeLayout(false);
			this.tabPage.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		// OK Button clicked
		private void btnOK_Click(object sender, System.EventArgs e)
		{
			if (this.textQuery.Text.Length > 0)  // if query is present
			{
				// Reformat, highlight, and make the SQL statement look pretty
				// one last time.
				UpdateQueryTextBox(md.CmdTokenList);

				System.Text.StringBuilder sb =new System.Text.StringBuilder(
					this.textQuery.Text.Length);

				// concatenate all the lines into one long command string
				foreach(string lineItem in this.textQuery.Lines)
				{
					string line = lineItem.Trim(); // trim leading and trailing
					if (line.Length == 0)          // spaces; skip if all space
						continue;

					if (sb.Length > 0)             // separate lines by a space
						sb.Append(' ');

					sb.Append(line);               // add the tokens
				}  // end for loop
				this.CommandText = sb.ToString();
			}
			else
				this.CommandText = String.Empty;
		}  // btnOK_Click

		// Called (eventually) by form.ShowDialog().
		// The constructors for the form and its controls was called already.
		private void QueryDesignerForm_Load(object sender, System.EventArgs e)
		{
			string commandText;
			if (this.CommandText != null)
				commandText = this.CommandText.Trim();
			else
				commandText = String.Empty;

			if (commandText.Length == 0)
				commandText = "SELECT FROM";  // plant the seed

			this.Text = VSNETConst.shortTitle +" Query Designer";
			// force the panel sizes now
			panelMain_Resize(panelMain, System.EventArgs.Empty);

			if (Catalog != null)
			{
				ContextMenu cm = new ContextMenu();  // build the context menu
				EventHandler eh = new EventHandler(this.contextMenuOnClick);
				cm.MenuItems.Add(constAddTable, eh); // "Add Table..."
				ContextMenu = cm;
			}

			// format the incoming CommandText into the textQuery box
			Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
			Cursor.Current = Cursors.WaitCursor;  // hourglass cursor
			md = this.Connection.GetMetaData(commandText);
			Cursor.Current = cursor;  // restore Arrow cursor

			UpdateQueryTextBox(md.CmdTokenList);

			// update table property pages display
			UpdateTableColumnCheckBoxList(md);

			// update grid display
			UpdateGridColumns(md);
		}

		private void splitter1_SplitterMoving(object sender, System.Windows.Forms.SplitterEventArgs e)
		{
			System.Windows.Forms.Splitter splitter =
				(System.Windows.Forms.Splitter) sender;
			proportionPanelTables = (float) e.SplitY / splitter.Parent.Height;

			panelMain_Resize(this.panelMain, System.EventArgs.Empty);

			this.panelMain.Invalidate(true);  // force a repaint of all controls
		}

		private void splitter2_SplitterMoving(object sender, System.Windows.Forms.SplitterEventArgs e)
		{
			System.Windows.Forms.Splitter splitter =
				(System.Windows.Forms.Splitter) sender;
			proportionGridColumns = (float) e.SplitY / splitter.Parent.Height
				- proportionPanelTables;

			panelMain_Resize(this.panelMain, System.EventArgs.Empty);

			this.gridColumns.Invalidate(true);  // force a repaint
			this.textQuery.Invalidate(true);    // force a repaint
		}

		private void panelMain_Resize(object sender, System.EventArgs e)
		{
			System.Windows.Forms.Panel panelMain =
				(System.Windows.Forms.Panel) sender;
			this.panelTables.Height =
				(int) (proportionPanelTables * panelMain.Height);
			this.gridColumns.Height =
				(int) (proportionGridColumns * panelMain.Height);
		}

		private void UpdateQueryTextBox(ArrayList cmdTokenList)
		{
			//this.panelTables.Enabled = false;
			//this.gridColumns.Enabled = false;

			string s;
			string sUpper;
			string sPrior = String.Empty;
			System.Text.StringBuilder sb =
				new System.Text.StringBuilder(100);

			foreach(string sItem in cmdTokenList)
			{
				s = sItem;             // work copy
				if (s.Length == 1)
				{
					if (s.Equals(",")  ||
						s.Equals("."))
					{
						sb.Append(s);  // append with no prior space
						sPrior = s;
						continue;
					}
				}  // end if Length == 1

				sUpper = s.ToUpper(       // s in UPPERCASE
					System.Globalization.CultureInfo.InvariantCulture);
				// run the list of keywords we'd like to highlight and new line
				if (sUpper.Equals("SELECT")  ||
					sUpper.Equals("INSERT")  ||
					sUpper.Equals("UPDATE")  ||
					sUpper.Equals("DELETE")  ||

					sUpper.Equals("ALTER")   ||
					sUpper.Equals("CREATE")  ||
					sUpper.Equals("DROP")    ||
					sUpper.Equals("GRANT")   ||
					sUpper.Equals("MODIFY")  ||
					sUpper.Equals("REMOVE")  ||
					sUpper.Equals("REVOKE")  ||
					sUpper.Equals("SET")     ||

					sUpper.Equals("FROM")    ||
					sUpper.Equals("ORDER")   ||
					sUpper.Equals("GROUP")   ||
					sUpper.Equals("HAVING")  ||
					sUpper.Equals("UNION")   ||
					sUpper.Equals("WHERE")   ||
					sUpper.Equals("VALUES")  ||
					sUpper.Equals("FOR"))
				{
					s = sUpper;  // raise the keyword to upper case
					if (sb.Length > 0)  // flush out the string to a Line
					{
						sb.Append(" \n");
					}
				}  // end if keyword hightlight list

				// run the list of keywords we'd like to highlight but no new line
				if (sUpper.Equals("ALL")     ||
					sUpper.Equals("ASC")     ||
					sUpper.Equals("DESC")    ||
					sUpper.Equals("DISTINCT")||
					sUpper.Equals("DEFERRED")||
					sUpper.Equals("DIRECT"))
				{
					s = sUpper;  // raise the keyword to upper case
				}  // end if keyword hightlight list

				if (sUpper.Equals("BY")  &&  // hightlight ORDER BY and GROUP BY
				(sPrior.Equals("ORDER")  ||  sPrior.Equals("GROUP")))
					s = sUpper;  // raise the keyword to upper case

				if (sUpper.Equals("OF")  &&  // hightlight UPDATE OF
					sPrior.Equals("UPDATE"))
					s = sUpper;  // raise the keyword to upper case


				if (sb.Length > 0  &&     // if not first token in line and
					!sPrior.Equals("."))  // unless a period separator
					sb.Append(' ');       // insert a space

				sb.Append(s);

				sPrior = s;               // remember the prior token
			}  // foreach string in cmdTokenList

			this.textQuery.Text = sb.ToString();

			//MetaData md = new MetaData(this.Connection, cmdText);
			//ArrayList columns = md.Columns;
			//ArrayList tables  = md.Tables;
		}  // UpdateQueryTextBox

		private void UpdateTableColumnCheckBoxList(MetaData md)
		{
			bool visible = false;  // assume that there are no pages

			this.SuspendLayout();
			this.panelTables.SuspendLayout();
			this.tabTableColumns.SuspendLayout();

			this.tabTableColumns.TabPages.Clear();  // clear all of the pages
			this.tabTableColumns.SelectedIndex = 0;

			if (md.Tables != null  &&  md.Tables.Count > 0)
			{
				foreach (MetaData.Table table in md.Tables)
				{
					Catalog.Table catTable = table.CatTable;
					if (catTable == null)  // skip ones with no cat info
						continue;

					CheckedListBox checkedListTableColumns =
					                            new CheckedListBox();
					Panel   panelKey          = new Panel();
					Label   labelTableColumns = new Label();
					TabPage tabPage           = new TabPage();
					PrimaryKeyIconList primaryKeyIconList =
						new PrimaryKeyIconList(checkedListTableColumns.ItemHeight);

					tabPage.SuspendLayout();

					// Use the form's checkedListTableColumns as a template
					// when coding this checkedListTableColumns entries
					// in order to take advantage of designer settings.
					checkedListTableColumns.BorderStyle = 
						this.checkedListTableColumns.BorderStyle;
					checkedListTableColumns.CheckOnClick = 
						this.checkedListTableColumns.CheckOnClick;
					checkedListTableColumns.Dock =
						this.checkedListTableColumns.Dock;
					checkedListTableColumns.Location = new Point(
						this.panelKey.Width,
						this.labelTableColumns.Height);
					checkedListTableColumns.SelectionMode = 
						this.checkedListTableColumns.SelectionMode;
					checkedListTableColumns.Size = 
						new Size(this.checkedListTableColumns.Size.Width,
						         this.checkedListTableColumns.Size.Height);
					checkedListTableColumns.TabIndex =
						this.checkedListTableColumns.TabIndex;
					checkedListTableColumns.Name = table.TableName;
					checkedListTableColumns.SelectedIndexChanged +=
						new System.EventHandler(
							this.checkedListTableColumns_SelectedIndexChanged);
					checkedListTableColumns.Invalidated +=
						new InvalidateEventHandler(
							this.checkedListTableColumns_Invalidated);
					checkedListTableColumns.Tag = table;

					panelKey.Tag  = primaryKeyIconList;

					// This code has been disabled because there is no way
					// to get .NET to fire an event that the CheckedListBox
					// has been scrolled and its TopIndex has been changed.
					// We need this information to repaint the "key" icon
					// in the right location in the PanelKey.  This code
					// has been left in for the day when capability has
					// been added. (thoda04)
					//panelKey.Paint +=
					//	new System.Windows.Forms.PaintEventHandler(this.panelKey_Paint);

					foreach (Catalog.Column catColumn in catTable.Columns)
					{
						if (catColumn.ColumnName == null)  // safety check
							continue;

						bool isChecked = false;

						// find if this catalog column is in the SELECT collist
						foreach(MetaData.Column column in md.Columns)
						{
							// if right table and right catalog column
							if (ReferenceEquals(column.Table,     table)  &&
								ReferenceEquals(column.CatColumn, catColumn))
							{
								isChecked = true;  // matched select col w/ cat col
								break;
							}
						}  // end foreach column
						checkedListTableColumns.Items.Add(
							catColumn.ColumnName, isChecked);

						// save indication whether column is a primary key
						//primaryKeyIconList.Add(catColumn.KeySequence > 0);
					}  // end foreach catColumn

					panelKey.BackColor = this.panelKey.BackColor;
					panelKey.Dock      = this.panelKey.Dock;
					panelKey.Location  = this.panelKey.Location;
					panelKey.Size =
						new Size(this.panelKey.Size.Width,
						         this.panelKey.Size.Height);
					panelKey.TabIndex  = this.panelKey.TabIndex;

					labelTableColumns.Dock = this.labelTableColumns.Dock;
					labelTableColumns.Size =
						new Size(this.labelTableColumns.Size.Width,
						         this.labelTableColumns.Size.Height);
					labelTableColumns.TabIndex  = this.labelTableColumns.TabIndex;
					labelTableColumns.TextAlign = this.labelTableColumns.TextAlign;

					// fully qualified tablename above the list
					labelTableColumns.Text = table.ToString();

					tabPage.Controls.AddRange(
						new System.Windows.Forms.Control[] {
							checkedListTableColumns,
							panelKey,
							labelTableColumns});
					tabPage.Location = this.tabPage.Location;
					tabPage.Dock = this.tabPage.Dock;
					//tabPage.Name = "tabPage";
					tabPage.Size = new Size(
						this.tabPage.Size.Width,
						this.tabPage.Size.Height);
					tabPage.TabIndex = 0;
					tabPage.Text = table.TableName;

					this.tabTableColumns.TabPages.Add(tabPage);

					this.tabPage.ResumeLayout(false);

					visible = true;  // there is at least one page
				}  // end for each
			} // end if (md.Tables.Count > 0)

			this.tabTableColumns.Enabled = visible;

			// correctly size the controls in the property tabs based
			// upon the tab control size.
			tabTableColumns_Resize(this.tabTableColumns, System.EventArgs.Empty);

			this.tabTableColumns.ResumeLayout(false);
			this.panelTables.ResumeLayout(false);
			this.ResumeLayout(false);
		}

		private void UpdateGridColumns(MetaData md)
		{
			DataGridCell dataGridCurrentCell = this.gridColumns.CurrentCell;

			// don't fire Changing events while we load the dataset/datatable
			DisableDataTableChangeEvents(this.dataTableColumns);
			DisableGridColumnsCurrentCellChangeEvents(this.gridColumns);

			this.dataTableColumns.Clear();

			if (md.Columns.Count == 0)  // if no columns, add a blank row
				this.dataTableColumns.Rows.Add(this.dataTableColumns.NewRow());

			foreach (MetaData.Column column in md.Columns)
			{
				MetaData.Table table = column.Table;
				Catalog.Table catTable;
				if (column.Table != null)
					catTable = column.Table.CatTable;
				else
					catTable = null;

				System.Data.DataRow dataRow = this.dataTableColumns.NewRow();

				if (column.ColumnName != null)  // column reference
					dataRow["Column Name"] = column.ColumnName;
				else                            // expression definition
					dataRow["Column Name"] = column.ColumnNameSpecification;
				if (column.SortDirection != 0)
					if (column.SortDirection == 1)
						dataRow["Sort Type"] = "ASC";
					else
					if (column.SortDirection ==-1)
						dataRow["Sort Type"] = "DESC";
				if (column.SortOrder != 0)
					dataRow["Sort Order"] = column.SortOrder.ToString();

				if (column.AliasName != null)
					dataRow["Alias"] = column.AliasName;
				if (table != null)
					if (table.AliasName != null  &&  table.AliasName.Length > 0)
						dataRow["Table"] = table.AliasName;
					else
						dataRow["Table"] = table.TableName;

				this.dataTableColumns.Rows.Add(dataRow);
			}  // end loop through SELECT column references

			this.dataTableColumns.AcceptChanges();  // start with clean slate

			// accept Changing events from now on
			EnableGridColumnsCurrentCellChangeEvents(this.gridColumns);
			EnableDataTableChangeEvents(this.dataTableColumns);

		}  // UpdateGridColumns


		/// <summary>
		/// Don't fire Changing events while we load the dataset/datatable.
		/// </summary>
		/// <param name="dataTable"></param>
		private void DisableGridColumnsCurrentCellChangeEvents(DataGrid grid)
		{
			grid.CurrentCellChanged -=
				new System.EventHandler(
					this.gridColumns_CurrentCellChanged);
		}


		/// <summary>
		/// Don't fire Changing events while we load the dataset/datatable.
		/// </summary>
		/// <param name="dataTable"></param>
		private void EnableGridColumnsCurrentCellChangeEvents(DataGrid grid)
		{
			grid.CurrentCellChanged +=
				new System.EventHandler(
					this.gridColumns_CurrentCellChanged);
		}


		/// <summary>
		/// Don't fire Changing events while we load the dataset/datatable.
		/// </summary>
		/// <param name="dataTable"></param>
		private void DisableDataTableChangeEvents(DataTable dataTable)
		{
			dataTable.ColumnChanging -=
				new System.Data.DataColumnChangeEventHandler(
				this.dataTableColumns_ColumnChanging);

			dataTable.ColumnChanged -=
				new System.Data.DataColumnChangeEventHandler(
				this.dataTableColumns_ColumnChanged);
		}


		// accept Changing events from now on
		private void EnableDataTableChangeEvents(DataTable dataTable)
		{
			dataTable.ColumnChanging +=
				new System.Data.DataColumnChangeEventHandler(
				this.dataTableColumns_ColumnChanging);

			dataTable.ColumnChanged +=
				new System.Data.DataColumnChangeEventHandler(
				this.dataTableColumns_ColumnChanged);
		}


		private void QueryDesignerForm_Closing(
			object sender, System.ComponentModel.CancelEventArgs e)
		{
			try
			{
				if (this.Connection != null)
					Connection.Close();  // close the connection
			}
			catch {}  // ignore any closing errors
		}

		// Enter event fired when textQuery TextBox is gaining focus
		private void textQuery_Enter(object sender, System.EventArgs e)
		{
			textQueryEnterText = textQuery.Text;
		}

		// Leave event fired when textQuery TextBox is losing focus
		private void textQuery_Leave(object sender, System.EventArgs e)
		{
			// if no change in the QueryText panel, just return to save time
			if (textQueryEnterText.Equals(textQuery.Text))
				return;

			Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
			Cursor.Current = Cursors.WaitCursor;  // hourglass cursor
			md = this.Connection.GetMetaData(this.textQuery.Text);
			Cursor.Current = cursor;  // resored Arrow cursor

			// Reformat, highlight, and make the SQL statement look pretty
			UpdateQueryTextBox(md.CmdTokenList);

			// update table property pages display
			UpdateTableColumnCheckBoxList(md);

			// update grid display
			UpdateGridColumns(md);
		}

		// If table list panel resizes, resize the checked box list and Key panel.
		// Need to do this manually due to NET 1.0 bug where DockStyle.Fill
		// do not work on a TabPage control.
		private void tabTableColumns_Resize(object sender, System.EventArgs e)
		{
			TabControl tabControl = (TabControl)sender;
			//MessageBox.Show("Size=" + tabControl.ClientSize.ToString());
			foreach (TabPage tabPage in tabControl.TabPages)
			{
				if (tabPage.Controls.Count < 2)  // safety check; should not occur
					continue;

				CheckedListBox checkedListTableColumns =
					(CheckedListBox)tabPage.Controls[0];
				Panel   panelKey =
					(Panel)         tabPage.Controls[1];

				// get the parent control's Width and Height
				int width  = tabPage.Width  - this.panelKey.Width;
				int height = tabPage.Height - this.labelTableColumns.Height;

				// round down height into integral units
				int itemHeight = checkedListTableColumns.ItemHeight;
				if (itemHeight < 1)  // safety check
					itemHeight = 15;
				height = height/itemHeight*itemHeight;

				// checked box list control
				checkedListTableColumns.Size = new Size(width, height);

				// primary key icon control
				panelKey.Size = new Size(panelKey.Width, height);

				//tabPage.Invalidate(true);  // force a repaint
			}
		}

		private void checkedListTableColumns_Invalidated(
			object sender, InvalidateEventArgs e)
		{
			CheckedListBox checkedListTableColumns =
				(CheckedListBox)sender;
			TabPage tabPage = checkedListTableColumns.Parent as TabPage;
			if (tabPage == null)
				return;

			if (tabPage.Controls.Count < 2)  // safety check; should not occur
				return;

			Panel   panelKey = tabPage.Controls[1] as Panel;
			if (panelKey == null)
				return;
			panelKey.Invalidate();  // force a repaint of the key images
		}

		private void checkedListTableColumns_SelectedIndexChanged(
			object sender, System.EventArgs e)
		{
			CheckedListBox checkedListTableColumns =
				(CheckedListBox)sender;
			int selectedIndex   = checkedListTableColumns.SelectedIndex;
			if (selectedIndex == -1)  // just return if our recursive call
				return;

			// save the column name
			string columnName = (string)checkedListTableColumns.SelectedItem;

			checkedListTableColumns.SelectedIndex = -1;  // clear the highlight
			// this triggers a recursive call to us

			object obj = checkedListTableColumns.DataSource;
			MetaData.Table table = (MetaData.Table) checkedListTableColumns.Tag;
			bool isChecked = checkedListTableColumns.GetItemChecked(selectedIndex);

			if (isChecked)
			{
				if (this.md.TokenFROM > 0)  // if there is a FROM keyword (should be)
				{
					System.Text.StringBuilder sb =
						new System.Text.StringBuilder(100);

					if (this.md.Columns.Count > 0)  // do we need a comma separator
						sb.Append(", ");

					if (table.AliasNameSpecification != null  &&
						table.AliasNameSpecification.Length > 0)
					{
						sb.Append(table.AliasNameSpecification);
						sb.Append(".");
					}
					else 
						if (table.TableNameSpecification != null  &&
						table.TableNameSpecification.Length > 0   &&
						this.md.Tables.Count > 1) // qualify if more than one table
					{
						sb.Append(table.TableNameSpecification);
						sb.Append(".");
					}
					sb.Append(
						MetaData.WrapInQuotesIfEmbeddedSpaces(columnName));

					this.md.CmdTokenList.Insert(md.TokenFROM, sb.ToString());
				}
			}
			else // field name is being unchecked; search and remove it from query
			{
				// compare on the table names rather than comparing on object
				// instances since the Table in checkedListTableColumns.Tag
				// might be older that the Table in the Metadata.Columns.
				string fullTableName = table.ToString(); //tablename and alias

				int columnIndex = -1;
				foreach (MetaData.Column column in this.md.Columns)
				{
					columnIndex++;  // track position in list

					Catalog.Column catColumn = column.CatColumn;
					if (catColumn == null)  // skip non-column refs
						continue;           // like expressions

					// skip if different columns names
					if (columnName != catColumn.ColumnName)
						continue;

					// skip if same column name but different table alias
					if (fullTableName != column.Table.ToString())
						continue;

					// found the column that matches the unchecked item!

					// remove the tokens that make up the column (and comma)
					this.md.RemoveToken(column, columnIndex);

					// we're done
					break;
				}  // end loop through md.Columns
			}  // end else removing item from query

			// update the query text pane
			UpdateQueryTextBox(this.md.CmdTokenList);

			// rebuild the metadata
			this.md = this.Connection.GetMetaData(this.textQuery.Text);

			// update grid display
			UpdateGridColumns(this.md);
		}

		private void panelKey_Paint(
			object sender, System.Windows.Forms.PaintEventArgs e)
		{
			Panel panelKey = (Panel)sender;
			Graphics grfx = e.Graphics;

			PrimaryKeyIconList primaryKeyIconList =
				panelKey.Tag as PrimaryKeyIconList;
			int itemHeight = primaryKeyIconList.ItemHeight;

			TabPage tabPage = panelKey.Parent as TabPage;
			if (tabPage == null  ||  itemHeight <= 0)
				return;

			if (tabPage.Controls.Count < 2)  // safety check; should not occur
				return;

			CheckedListBox checkedListTableColumns =
				(CheckedListBox)tabPage.Controls[0];
			int topIndex = checkedListTableColumns.TopIndex;
			int numberItems = Math.Min(
				primaryKeyIconList.Count - topIndex,
				panelKey.Size.Height / itemHeight);

			int x = 2;
			int y = 2;
			for (;topIndex < numberItems; topIndex++, y += itemHeight)
			{
				if ((bool)primaryKeyIconList[topIndex])
					DrawKey(grfx, Color.Black, x, y);
			}  // end loop

			grfx.Dispose();
		}  // checkedListTableColumns_SelectedIndexChanged

		private class PrimaryKeyIconList : ArrayList
		{
			internal PrimaryKeyIconList(int itemHeight)
			{
				ItemHeight = itemHeight;
			}

			/*
			** itemHeight property
			*/
			private int _itemHeight;
			[Description("Height of each item that key icon must fit in.")]
			public int ItemHeight
			{
				get { return _itemHeight;  }
				set  { _itemHeight = value;  }
			}

		}  // end class PrimaryKeyIconList

		static void DrawKey(Graphics grfx, Color color, int x, int y)
		{
			const int thickness = 2;
			Pen pen  = new Pen(color, thickness);

			grfx.SmoothingMode   =
				System.Drawing.Drawing2D.SmoothingMode.HighQuality;
			grfx.PixelOffsetMode =
				System.Drawing.Drawing2D.PixelOffsetMode.HighQuality;

			grfx.DrawEllipse(pen, x+0, y+0, 5, 8);

			grfx.SmoothingMode   =
				System.Drawing.Drawing2D.SmoothingMode.None;
			grfx.PixelOffsetMode =
				System.Drawing.Drawing2D.PixelOffsetMode.None;

			//pen = new Pen(color, 2);
			y += 4;
			grfx.DrawLine(pen, x+6, y, x+20, y);
			y += thickness;
			grfx.DrawLine(pen, x+13,y, x+17, y);
			y += thickness;
			grfx.DrawLine(pen, x+10,y, x+14, y);
			grfx.DrawLine(pen, x+16,y, x+20, y);
		}

		private void gridColumns_CurrentCellChanged(object sender, System.EventArgs e)
		{
			//MessageBox.Show("CurrentCell={" +
			//	this.gridColumns.CurrentCell.ColumnNumber + "," +
			//	this.gridColumns.CurrentCell.RowNumber + "}",
			//		"CurrentCellChanged");

			DataGrid  grid      = (DataGrid)sender;
			DataSet   dataSet   = (DataSet)grid.DataSource;
			DataTable dataTable = dataSet.Tables[0];
			DataGridCell currentCell = grid.CurrentCell;

			// if we are adding a row to the grid, add a row to datatable
			if (grid.CurrentCell.RowNumber + 1 > dataTable.Rows.Count)
			{
				// don't fire Changing events while we add a blank row
				DisableDataTableChangeEvents(dataTable);
				DisableGridColumnsCurrentCellChangeEvents(grid);

				// add blank row to datatable
				dataTable.Rows.Add(dataTable.NewRow());

				// the add messes up the grid.CurrentCell, so restore it
				grid.CurrentCell = currentCell;

				// start acceptin Changing events from now on
				EnableGridColumnsCurrentCellChangeEvents(grid);
				EnableDataTableChangeEvents(dataTable);
			}

			this.dataTableColumns.AcceptChanges();

			if (rebuildDataGrid)
			{
				rebuildDataGrid = false;
				UpdateGridColumns(this.md);
			}

			// restore the cell position after all the changes
			grid.CurrentCell = new DataGridCell(
				Math.Min(currentCell.RowNumber,
					Math.Max(dataTable.Rows.Count-1, 0)),
				currentCell.ColumnNumber);

			//if (dataTableColumns.Rows != null)
			//	this.labelDebug.Text = "dataTableColumns.Rows.Count=" +
			//		dataTableColumns.Rows.Count.ToString();
		}

		private void dataTableColumns_ColumnChanging(
			object sender, System.Data.DataColumnChangeEventArgs e)
		{
			/*
			MessageBox.Show(
				"ColumnName    = " + e.Column.ColumnName +";\n" +
				"ProposedValue = " + (e.ProposedValue!=null?e.ProposedValue:"<null>") + "\n" +
				"RowState      = " + e.Row.RowState,
				"ColumnChanging");
			e.Row.AcceptChanges();
			*/
			DataRow        row    = e.Row;
			DataRowState rowState = e.Row.RowState;

			Object obj = e.ProposedValue;
			if (obj.GetType() == typeof(String))
				e.ProposedValue = ((String)obj).Trim();  // clean string
		}

		private void dataTableColumns_ColumnChanged(
			object sender, System.Data.DataColumnChangeEventArgs e)
		{
			/*
			MessageBox.Show(
				"ColumnName    = " + e.Column.ColumnName +";\n" +
				"ProposedValue = " + (e.ProposedValue!=null?e.ProposedValue:"<null>") + "\n" +
				"RowState      = " + e.Row.RowState,
				"ColumnChanged");
			*/
			DataRow        row    = e.Row;
			DataRowState rowState = e.Row.RowState;

			MetaData.Column oldCol = null;
			string oldColumnName   = "";
			string oldTableName    = "";
			string oldTableNameSpec= "";

			Object obj;  // work object

			int index = DataRowIndex(row);  // index into table

			string columnName;
			obj = row["Column Name"];
			if (!Convert.IsDBNull(obj))
				columnName = ((string)obj).Trim();
			else
				columnName = String.Empty;

			if (columnName.Length != 0)
			{
				string aliasSpec;
				obj = row["Alias"];
				if (!Convert.IsDBNull(obj))
					aliasSpec = (string)obj;
				else
					aliasSpec = String.Empty;

				string tableSpec;
				obj = row["Table"];
				if (!Convert.IsDBNull(obj))
					tableSpec = (string)obj;
				else
					tableSpec = String.Empty;

				MetaData.Column newCol = new MetaData.Column();
				newCol.ColumnName = columnName;
				//newCol.TableName  = tableSpec;
				newCol.AliasName  = aliasSpec;
				newCol.AliasNameSpecification =
					MetaData.WrapInQuotesIfEmbeddedSpaces(aliasSpec);

				if (index != -1  &&
					index < this.md.Columns.Count)
				{
					oldCol = (MetaData.Column)this.md.Columns[index];
					if (oldCol != null)
					{
						if (oldCol.ColumnName != null)
							oldColumnName = oldCol.ColumnName;
						if (oldCol.TableName != null)
							oldTableName = oldCol.TableName;
						if (oldCol.TableNameSpecification != null)
							oldTableNameSpec =
								oldCol.TableNameSpecification;
					}
				}

				int token;
				StringBuilder sb = new StringBuilder(100);

				if (oldCol != null)
				{
					token = oldCol.Token;       // replacing old entry
					token = this.md.RemoveToken(index);
				}
				else 
					token = this.md.TokenFROM;  // adding new entry

				// if inserting last col, need a preceding comma
				if (index > 0  &&  index + 1 >= this.md.Columns.Count)
					sb.Append(", ");  // append leading comma

				sb.Append(newCol.ToString());

				// if we are not the last column then append a
				// trailing comma.  e.g. "mycol,"
				if (index + 1 < this.md.Columns.Count)
					sb.Append(",");  // append trailing comma

				this.md.CmdTokenList.Insert(token, sb.ToString());
			}
			else  // columnName == String.Empty (treat as delete)
			{
				this.md.RemoveToken(index);  // remove the column's token
			}
			// update the query text pane
			UpdateQueryTextBox(this.md.CmdTokenList);

			// rebuild the metadata
			this.md = this.Connection.GetMetaData(this.textQuery.Text);

			// update table property pages display
			UpdateTableColumnCheckBoxList(md);

			this.rebuildDataGrid = true;  // rebuild the data grid later
		}

		/// <summary>
		/// Add table reference to end of FROM list.
		/// </summary>
		/// <param name="catTable"></param>
		internal void AddTableToQueryText(string tableName)
		{
			if (this.textQuery.Text.Length == 0)
				this.textQuery.Text = "SELECT ";

			if (md.TokenFROM == 0)
			{
				this.textQuery.Text += " FROM ";
				// rebuild the metadata for the new tokens
				md = this.Connection.GetMetaData(this.textQuery.Text);
				if (md.TokenFROM == 0)  // safety check; should not happen
					return;
			}

			int insertpoint;
			if (md.Tables.Count > 0)
			{
				MetaData.Table lastTable =
					(MetaData.Table)(md.Tables[md.Tables.Count-1]);
				insertpoint = lastTable.Token + lastTable.TokenCount;
				md.CmdTokenList.Insert(insertpoint, ",");
				insertpoint++;
			}
			else
				insertpoint = md.TokenFROM + 1;  // insert after the FROM

			md.CmdTokenList.Insert(insertpoint, tableName);

			UpdateQueryTextBox(md.CmdTokenList);  // rebuild Text and MetaData
			this.md = this.Connection.GetMetaData(this.textQuery.Text);

			// update table property pages display
			UpdateTableColumnCheckBoxList(md);

			// update grid display
			UpdateGridColumns(md);
		}

		/// <summary>
		/// Find the index of datarow that in the datatable.
		/// </summary>
		/// <param name="row"></param>
		/// <returns>Index of row in table.  -1 if not found.</returns>
		private int DataRowIndex(DataRow row)
		{
			DataTable table = row.Table;

			int i = -1;
			foreach (DataRow tableRow in table.Rows)
			{
				// don't count deleted rows in datatable
				if (tableRow.RowState == DataRowState.Deleted)
					continue;
				i++;
				if (tableRow == row)
					return i;  // return index of row in table
			}

			return -1;
		}

		private void dataTableColumns_RowDeleted(
			object sender, System.Data.DataRowChangeEventArgs e)
		{
			e.Row.Table.AcceptChanges();  // remove the row really from the table
		}

		private void dataTableColumns_RowDeleting(
			object sender, System.Data.DataRowChangeEventArgs e)
		{
			DataRowAction action  = e.Action;
			DataRow        row    = e.Row;
			DataRowState rowState = e.Row.RowState;
			//MetaData.Column oldCol = null;

			int rowIndex = DataRowIndex(row);  // index into table

			this.md.RemoveToken(rowIndex);  // remove the column's token

			// update the query text pane
			UpdateQueryTextBox(this.md.CmdTokenList);

			// rebuild the metadata
			this.md = this.Connection.GetMetaData(this.textQuery.Text);

			// update table property pages display
			UpdateTableColumnCheckBoxList(md);
		}

		/// <summary>
		/// GridColumns OnLeave.  Flush out any grid column changes.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void gridColumns_Leave(object sender, System.EventArgs e)
		{
			gridColumns_CurrentCellChanged(sender, e);
		}

		/// <summary>
		/// Process right-click to Add Tables, etc.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void contextMenuOnClick(object sender, System.EventArgs e)
		{
			MenuItem menuItem = (MenuItem)sender;

			if (menuItem.Text == constAddTable)  // "Add Table..."
			{
				Form form = new AddTableForm(Catalog, this);
				form.ShowDialog();
			}
		}

		private void QueryDesignerForm_Activated(
			object sender, System.EventArgs e)
		{
			// On the first time that the form is activated,
			// remove the Activated event handler (meaning this
			// code is called only once at form startup) and check if there
			// are no tables in the query.  If no table then invoke
			// the Add Tables dialog.
			this.Activated -=
				new System.EventHandler(this.QueryDesignerForm_Activated);

			// if no tables and a FROM clause then start prompting for tables
			if (md.Tables.Count == 0  &&  md.TokenFROM > 0)
				contextMenuOnClick(  // invoke the AddTable dialog
					new MenuItem(constAddTable), System.EventArgs.Empty);
		}

	}  // class QueryDesignerForm




	/*
	** Name: QueryDesignerUITypeEditor class
	**
	** Description:
	**	Launch an editor to show the QueryDesignerForm.
	**
	** Called by:
	**	CommandText UITypeEditor
	**
	** History:
	**	12-May-03 (thoda04)
	**	    Created.
	*/

	public class QueryDesignerUITypeEditor : UITypeEditor
	{
		public QueryDesignerUITypeEditor() : base()
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
			//string s = "ITypeDescriptorContext.Instance=";
			//s += (context.Instance==null)?"<null>":context.Instance.ToString();
			//MessageBox.Show(s, "QueryDesignerUITypeEditor");

			if (context          != null  &&
				context.Instance != null  &&
				provider         != null)
			{
				IWindowsFormsEditorService edSvc =
					(IWindowsFormsEditorService)provider.GetService(
					typeof(IWindowsFormsEditorService));
				if (edSvc != null)
				{
					//string s = typeof(IngresCommand).GUID.ToString();
					//string t = context.Instance.GetType().GUID.ToString();
					//string x = (s==t)?"==\n":"!=\n";
					//MessageBox.Show(s+x+t+";", "QueryDesignerUITypeEditor");

					IngresCommand providerCommand =
						(IngresCommand) context.Instance;

					//MessageBox.Show("assignment OK", "QueryDesignerUITypeEditor");

					IngresConnection connection = providerCommand.Connection;

					// check that we have a connection
					if (connection == null)
					{
						MessageBox.Show(
							"Connection property is missing in the Command.  " +
							"Set the connection for the query designer.",
							VSNETConst.shortTitle + " Query Designer",
							MessageBoxButtons.OK, MessageBoxIcon.Stop);
						return value;
					}

					// check that we have a connection string
					if (connection.ConnectionString == null  ||
						connection.ConnectionString.Trim().Length == 0)
					{
						MessageBox.Show(
							"ConnectionString property is missing " +
							"in the Command's Connection property.  " +
							"Set the connection string for the query designer.",
							VSNETConst.shortTitle + " Query Designer",
							MessageBoxButtons.OK, MessageBoxIcon.Stop);
						return value;
					}

					try
					{
						// call the dialog for building/modifying the command
						// text and also passing the connection to use
						QueryDesignerForm form =
							new QueryDesignerForm(providerCommand);
						DialogResult result = edSvc.ShowDialog(form);
						if (result == DialogResult.OK)
							value = form.CommandText;  // return updated cmd text
					}
					catch (Exception ex)
					{MessageBox.Show(ex.ToString(), "QueryDesignerUITypeEditor");}
				}  // end if (edSvc != null)
			} // end if context is OK
			return value;
		}

		public override UITypeEditorEditStyle
			GetEditStyle(ITypeDescriptorContext context)
		{

			//string s = "ITypeDescriptorContext.Instance=";
			//s += (context.Instance==null)?"<null>":context.Instance.ToString();
			//MessageBox.Show(s, "GetEditStyle");
			if (context          != null  &&
				context.Instance != null)
				return UITypeEditorEditStyle.Modal;
			return base.GetEditStyle(context);
		}
	}  // class QueryDesignerUITypeEditor



}  // namespace
