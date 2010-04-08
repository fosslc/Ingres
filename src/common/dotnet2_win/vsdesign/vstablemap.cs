/*
** Copyright (c) 2004, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Data;
using System.Data.Common;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace Ingres.Client.Design
{
	/*
	** Name: vstablemap.cs
	**
	** Description:
	**	Implements the VS.NET interfaces for DataAdapter TableMapping Editor.
	**
	** Classes:
	**	TableMapForm
	**	TableMappingsUITypeEditor
	**
	** History:
	**	13-Apr-04 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	**	14-jul-04 (thoda04)
	**	    Replaced Ingres bitmaps.
	*/

	/// <summary>
	/// Class for Table Mappings form.
	/// </summary>
	internal class TableMapForm : System.Windows.Forms.Form
	{
		const string bitmapName = "Ingres.Client.Design.ingresbox.bmp";

		private DbDataAdapter dbAdapter;

		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.PictureBox pictureBox1;
		private System.Windows.Forms.Label labelSourceTable;
		private System.Windows.Forms.Label labelDatasetTable;
		private System.Windows.Forms.ComboBox comboBoxSourceTable;
		private System.Windows.Forms.TextBox textBoxDatasetTable;
		private System.Windows.Forms.Label labelColumnMappings;
		private System.Windows.Forms.DataGrid dataGrid1;
		private System.Windows.Forms.Button buttonReset;
		private System.Windows.Forms.Button buttonOK;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.DataGridTableStyle dataGridTableStyle1;
		private System.Windows.Forms.DataGridTextBoxColumn dataGridTextBoxColumn1;
		private System.Windows.Forms.DataGridTextBoxColumn dataGridTextBoxColumn2;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		internal TableMapForm()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		internal TableMapForm(DbDataAdapter dbAdapter)
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			this.dbAdapter = dbAdapter;
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

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.label1 = new System.Windows.Forms.Label();
			this.pictureBox1 = new System.Windows.Forms.PictureBox();
			this.labelSourceTable = new System.Windows.Forms.Label();
			this.labelDatasetTable = new System.Windows.Forms.Label();
			this.comboBoxSourceTable = new System.Windows.Forms.ComboBox();
			this.textBoxDatasetTable = new System.Windows.Forms.TextBox();
			this.labelColumnMappings = new System.Windows.Forms.Label();
			this.dataGrid1 = new System.Windows.Forms.DataGrid();
			this.dataGridTableStyle1 = new System.Windows.Forms.DataGridTableStyle();
			this.dataGridTextBoxColumn1 = new System.Windows.Forms.DataGridTextBoxColumn();
			this.dataGridTextBoxColumn2 = new System.Windows.Forms.DataGridTextBoxColumn();
			this.buttonReset = new System.Windows.Forms.Button();
			this.buttonOK = new System.Windows.Forms.Button();
			this.buttonCancel = new System.Windows.Forms.Button();
			((System.ComponentModel.ISupportInitialize)(this.dataGrid1)).BeginInit();
			this.SuspendLayout();
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(40, 24);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(392, 40);
			this.label1.TabIndex = 0;
			this.label1.Text = "For each column in the source database table, specify the corresponding dataset t" +
				"able column.";
			// 
			// pictureBox1
			// 
			this.pictureBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.pictureBox1.Location = new System.Drawing.Point(496, 24);
			this.pictureBox1.Name = "pictureBox1";
			this.pictureBox1.Size = new System.Drawing.Size(88, 88);
			this.pictureBox1.TabIndex = 1;
			this.pictureBox1.TabStop = false;
			// 
			// labelSourceTable
			// 
			this.labelSourceTable.Location = new System.Drawing.Point(48, 80);
			this.labelSourceTable.Name = "labelSourceTable";
			this.labelSourceTable.TabIndex = 2;
			this.labelSourceTable.Text = "Source table:";
			// 
			// labelDatasetTable
			// 
			this.labelDatasetTable.Location = new System.Drawing.Point(264, 80);
			this.labelDatasetTable.Name = "labelDatasetTable";
			this.labelDatasetTable.TabIndex = 3;
			this.labelDatasetTable.Text = "Dataset table:";
			// 
			// comboBoxSourceTable
			// 
			this.comboBoxSourceTable.Location = new System.Drawing.Point(48, 112);
			this.comboBoxSourceTable.Name = "comboBoxSourceTable";
			this.comboBoxSourceTable.Size = new System.Drawing.Size(184, 21);
			this.comboBoxSourceTable.TabIndex = 4;
			this.comboBoxSourceTable.Text = "Table";
			// 
			// textBoxDatasetTable
			// 
			this.textBoxDatasetTable.Location = new System.Drawing.Point(264, 112);
			this.textBoxDatasetTable.Name = "textBoxDatasetTable";
			this.textBoxDatasetTable.Size = new System.Drawing.Size(184, 20);
			this.textBoxDatasetTable.TabIndex = 5;
			this.textBoxDatasetTable.Text = "Table";
			// 
			// labelColumnMappings
			// 
			this.labelColumnMappings.Location = new System.Drawing.Point(48, 160);
			this.labelColumnMappings.Name = "labelColumnMappings";
			this.labelColumnMappings.TabIndex = 6;
			this.labelColumnMappings.Text = "Column mappings:";
			// 
			// dataGrid1
			// 
			this.dataGrid1.CaptionVisible = false;
			this.dataGrid1.DataMember = "";
			this.dataGrid1.HeaderForeColor = System.Drawing.SystemColors.ControlText;
			this.dataGrid1.Location = new System.Drawing.Point(48, 192);
			this.dataGrid1.Name = "dataGrid1";
			this.dataGrid1.PreferredColumnWidth = 180;
			this.dataGrid1.Size = new System.Drawing.Size(416, 168);
			this.dataGrid1.TabIndex = 7;
			this.dataGrid1.TableStyles.AddRange(new System.Windows.Forms.DataGridTableStyle[] {
																								  this.dataGridTableStyle1});
			// 
			// dataGridTableStyle1
			// 
			this.dataGridTableStyle1.DataGrid = this.dataGrid1;
			this.dataGridTableStyle1.GridColumnStyles.AddRange(new System.Windows.Forms.DataGridColumnStyle[] {
																												  this.dataGridTextBoxColumn1,
																												  this.dataGridTextBoxColumn2});
			this.dataGridTableStyle1.HeaderForeColor = System.Drawing.SystemColors.ControlText;
			this.dataGridTableStyle1.MappingName = "";
			this.dataGridTableStyle1.PreferredColumnWidth = 204;
			this.dataGridTableStyle1.RowHeaderWidth = 204;
			// 
			// dataGridTextBoxColumn1
			// 
			this.dataGridTextBoxColumn1.Format = "";
			this.dataGridTextBoxColumn1.FormatInfo = null;
			this.dataGridTextBoxColumn1.HeaderText = "Source Columns";
			this.dataGridTextBoxColumn1.MappingName = "";
			this.dataGridTextBoxColumn1.NullText = "";
			this.dataGridTextBoxColumn1.Width = 204;
			// 
			// dataGridTextBoxColumn2
			// 
			this.dataGridTextBoxColumn2.Format = "";
			this.dataGridTextBoxColumn2.FormatInfo = null;
			this.dataGridTextBoxColumn2.HeaderText = "Dataset Columns";
			this.dataGridTextBoxColumn2.MappingName = "";
			this.dataGridTextBoxColumn2.NullText = "";
			this.dataGridTextBoxColumn2.Width = 204;
			// 
			// buttonReset
			// 
			this.buttonReset.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonReset.Location = new System.Drawing.Point(56, 400);
			this.buttonReset.Name = "buttonReset";
			this.buttonReset.TabIndex = 8;
			this.buttonReset.Text = "Reset";
			this.buttonReset.Click += new System.EventHandler(this.buttonReset_Click);
			// 
			// buttonOK
			// 
			this.buttonOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOK.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonOK.Enabled = false;
			this.buttonOK.Location = new System.Drawing.Point(408, 400);
			this.buttonOK.Name = "buttonOK";
			this.buttonOK.TabIndex = 9;
			this.buttonOK.Text = "OK";
			this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.Location = new System.Drawing.Point(512, 400);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.TabIndex = 10;
			this.buttonCancel.Text = "Cancel";
			// 
			// TableMapForm
			// 
			this.AcceptButton = this.buttonOK;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.buttonCancel;
			this.ClientSize = new System.Drawing.Size(626, 442);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.buttonOK);
			this.Controls.Add(this.buttonReset);
			this.Controls.Add(this.dataGrid1);
			this.Controls.Add(this.labelColumnMappings);
			this.Controls.Add(this.textBoxDatasetTable);
			this.Controls.Add(this.comboBoxSourceTable);
			this.Controls.Add(this.labelDatasetTable);
			this.Controls.Add(this.labelSourceTable);
			this.Controls.Add(this.pictureBox1);
			this.Controls.Add(this.label1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
			this.Name = "TableMapForm";
			this.Text = "Ingres Table Mappings";
			this.Load += new System.EventHandler(this.TableMapForm_Load);
			((System.ComponentModel.ISupportInitialize)(this.dataGrid1)).EndInit();
			this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// Form initialiation and load.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void TableMapForm_Load(object sender, System.EventArgs e)
		{
			bool result = true;  // if all is well, enable the OK button

			//load the .gif image for the corner
			System.Reflection.Assembly assembly =
				GetType().Module.Assembly;
			Bitmap advanBitmap = new Bitmap(
				assembly.GetManifestResourceStream( bitmapName ));
			this.pictureBox1.Image = advanBitmap;
			this.pictureBox1.Size  =
				new Size(advanBitmap.Width, advanBitmap.Height);
			this.pictureBox1.BorderStyle = BorderStyle.FixedSingle;

			// build a DataTable that can map the 2-column DataSource to the grid
			System.Data.DataTable dt = new System.Data.DataTable("mapping");
			dt.Columns.Add( TableMappings.SOURCE_COLUMNS );
			dt.Columns.Add( TableMappings.DATASET_COLUMNS );

			// safety check that IngresDataAdapter exists
			if (this.dbAdapter == null)
			{
				System.Windows.Forms.MessageBox.Show(
					"Data Adapter is null; Table mappings is not available.",
					"Table Mappings");
				return;
			}

			DataTableMappingCollection mapList = this.dbAdapter.TableMappings;

			// safety check that TableMappings exists; should never fail
			if (mapList == null)
			{
				System.Windows.Forms.MessageBox.Show(
					"Data Adapter's Table mappings are null and are not available.",
					"Table Mappings");
				return;
			}

			// if empty TableMappings, build new set from SELECT's column
			// else use the existing list.
			if (mapList.Count > 0)
				result = BuildMapFromExistingMap(dt, mapList);

			dt.AcceptChanges();
			this.dataGrid1.DataMember = dt.TableName;
			this.dataGrid1.DataSource = dt;

			this.buttonOK.Enabled = result;  // all went well
		}


		/// <summary>
		/// Build a DataTable for the form's datagrid from the
		/// existing TableMappings.
		/// </summary>
		/// <param name="dt">2-column DataTable to hold the names.</param>
		/// <param name="mapList">TableMapping collection usually
		/// from the DataAdapter.</param>
		/// <returns></returns>
		private bool BuildMapFromExistingMap(
			System.Data.DataTable dt, DataTableMappingCollection mapList)
		{
			DataTableMapping map = mapList[0];
			string sourceTable = map.SourceTable;
			string datasetTable= map.DataSetTable;
			comboBoxSourceTable.Text = sourceTable;
			textBoxDatasetTable.Text = datasetTable;

			// copy the column mappings to the 2-column DataTable
			// that will be the DataSource for the datagrid
			foreach (DataColumnMapping colMap in map.ColumnMappings)
			{
				System.Data.DataRow datarow = dt.NewRow();
				datarow[ TableMappings.SOURCE_COLUMNS  ] = colMap.SourceColumn;
				datarow[ TableMappings.DATASET_COLUMNS ] = colMap.DataSetColumn;
				dt.Rows.Add( datarow );
			}

			return true;
		}


		/// <summary>
		/// OK Button event handler.
		/// Copy the column mappings back the TableMapping collection.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void buttonOK_Click(object sender, System.EventArgs e)
		{
			System.Data.DataTable dt =
				(System.Data.DataTable)this.dataGrid1.DataSource;
			if (dt == null)
				return;

			dt.AcceptChanges();

			string sourceTable = comboBoxSourceTable.Text.Trim();
			string datasetTable= textBoxDatasetTable.Text.Trim();
			if (sourceTable.Length == 0)
			{
				System.Windows.Forms.MessageBox.Show(
					"Source Table name is invalid.",
					"Table Mappings");
				return;
			}
			if (datasetTable.Length == 0)
			{
				System.Windows.Forms.MessageBox.Show(
					"DataSet Table name is invalid.",
					"Table Mappings");
				return;
			}

			DataTableMappingCollection mapList = this.dbAdapter.TableMappings;
			mapList.Clear();
			DataTableMapping map = mapList.Add(sourceTable, datasetTable);

			for (int i=0; i<dt.Rows.Count; i++)
			{
				System.Data.DataRow datarow = dt.Rows[ i ];
				string sourceCol =
					((string)datarow[ TableMappings.SOURCE_COLUMNS   ]).Trim();
				string datasetCol=
					((string)datarow[ TableMappings.DATASET_COLUMNS  ]).Trim();
				if (sourceCol.Length == 0  ||  // drop empty mappings 
					datasetCol.Length== 0)
					continue;
				map.ColumnMappings.Add(sourceCol, datasetCol);
			}
		}


		/// <summary>
		/// Reset button event handler.
		/// Discard the existing mappings in the datagrid and
		/// build a new set from the database.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		private void buttonReset_Click(object sender, System.EventArgs e)
		{
			if (this.dbAdapter == null)
			{
				System.Windows.Forms.MessageBox.Show(
					"Data Adapter is null; Table mappings is not available.",
					"Table Mappings");
				return;
			}

			DataTableMappingCollection mapList = this.dbAdapter.TableMappings;

			if (mapList == null)
			{
				System.Windows.Forms.MessageBox.Show(
					"Data Adapter's Table mappings are null and are not available.",
					"Table Mappings");
				return;
			}

			System.Data.DataTable dt;
			dt = TableMappings.BuildMapFromDatabase(
				DataAdapterUtil.GetDataAdapterClone(this.dbAdapter));
			textBoxDatasetTable.Text =
				TableMappings.BuildDataSetNameFromCommand(this.dbAdapter);

			dt.AcceptChanges();
			this.dataGrid1.DataSource = dt;
		}  // buttonReset_Click

	}  // class TableMapForm



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

	internal class TableMappingsUITypeEditor :
		System.Drawing.Design.UITypeEditor
	{
		public TableMappingsUITypeEditor() : base()
		{
		}

		public object EditIngresValue(
			ITypeDescriptorContext context,
			IServiceProvider       provider,
			object                 value)
		{
			return EditValue(context, provider, value);
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
			if (context          != null  &&
				context.Instance != null  &&
				provider         != null)
			{
				IWindowsFormsEditorService edSvc =
					(IWindowsFormsEditorService)provider.GetService(
					typeof(IWindowsFormsEditorService));
				if (edSvc != null)
				{
					System.ComponentModel.Design.IDesignerHost host =
						provider.GetService(
							typeof(System.ComponentModel.Design.IDesignerHost))
						as System.ComponentModel.Design.IDesignerHost;
					if (host == null)
					{
						System.Windows.Forms.MessageBox.Show(
							"The IDesignerHost service interface " +
							"could not be obtained to process the request.",
							"Table Mappings");
						return value;
					}

					System.ComponentModel.Design.IComponentChangeService chgSvc = 
						provider.GetService(
						typeof(System.ComponentModel.Design.IComponentChangeService))
						as System.ComponentModel.Design.IComponentChangeService;
					if (chgSvc == null)
					{
						System.Windows.Forms.MessageBox.Show(
							"The IComponentChangeService service interface " +
							"could not be obtained to process the request.",
							"Table Mappings");
						return value;
					}

					if (!context.OnComponentChanging())
					{
						System.Windows.Forms.MessageBox.Show(
							"The component is not permitted to be changed.",
							"Table Mappings");
						return value;
					}

					System.ComponentModel.Design.DesignerTransaction
						designerTxn = host.CreateTransaction("TableMapping");

					DbDataAdapter adapter = (DbDataAdapter)context.Instance;
					Form form = new TableMapForm( adapter );

					chgSvc.OnComponentChanging(adapter, null);
					using (form)
					{
						using (designerTxn)
						{
							try
							{
								DialogResult result = edSvc.ShowDialog( form );
								if (result == DialogResult.OK)
									designerTxn.Commit();
								else
									designerTxn.Cancel();
							}
							finally
							{
								chgSvc.OnComponentChanged(adapter, null, null, null);
//								context.OnComponentChanged();
							}
						}
					}
				}  // end if (edSvc != null)
			} // end if context is OK
			return value;
		}

		public override System.Drawing.Design.UITypeEditorEditStyle
			GetEditStyle(ITypeDescriptorContext context)
		{
			if (context          != null  &&
				context.Instance != null)
				return System.Drawing.Design.UITypeEditorEditStyle.Modal;
			return base.GetEditStyle(context);
		}
	}  // class TableMappingsUITypeEditor




	/// <summary>
	/// Generate an adapter's TableMappings
	/// </summary>
	internal class TableMappings
	{
		internal const string SOURCE_COLUMNS  = "Source Columns";
		internal const string DATASET_COLUMNS = "Dataset Columns";

		static internal bool Generate(DbDataAdapter origDbDataAdapter)
		{
			if (origDbDataAdapter == null)
				return false;

			DbDataAdapter  dbAdapter   =
				DataAdapterUtil.GetDataAdapterClone(origDbDataAdapter);
			IDbDataAdapter dataAdapter = (IDbDataAdapter)dbAdapter;
			IDbCommand     dbCommand   = dataAdapter.SelectCommand;

			// safety check for a missing or incomplete Command
			if (dbCommand == null  ||
				dbCommand.CommandText == null  ||
				dbCommand.CommandText.Trim().Length == 0)
				return false;

			// safety check for a missing Connection
			if (dbCommand.Connection == null)
				return false;

			string cmdText = dbCommand.CommandText;

			// make a schema table that the DbAdapter can fill in
			DataTable dt;

			// build the DataTable with the column mapping
			dt = BuildMapFromDatabase(dbAdapter);

			// move the mapping in the data table to adapter.TableMappings
			Generate(
				origDbDataAdapter, dt, 
				"Table", BuildDataSetNameFromCommand(cmdText));

			return true;  // return with all went well
		}  // Generate


		static internal bool Generate(
			DbDataAdapter dbDataAdapter,
			DataTable     dt,
			string sourceTableName,
			string datasetTableName)
		{
			if (sourceTableName.Length == 0)  // Source Table name is invalid
				return false;
			if (datasetTableName.Length == 0)  // DataSet Table name is invalid
				return false;

			DataTableMappingCollection mapList = dbDataAdapter.TableMappings;
			mapList.Clear();
			DataTableMapping map = mapList.Add(sourceTableName, datasetTableName);

			for (int i=0; i<dt.Rows.Count; i++)
			{
				System.Data.DataRow datarow = dt.Rows[ i ];
				string sourceCol = ((string)datarow[ SOURCE_COLUMNS   ]).Trim();
				string datasetCol= ((string)datarow[ DATASET_COLUMNS  ]).Trim();
				if (sourceCol.Length == 0  ||  // drop empty mappings 
					datasetCol.Length== 0)
					continue;
				map.ColumnMappings.Add(sourceCol, datasetCol);
			}

			return true;
		}  // Generate


		/// <summary>
		/// Build a new set of TableMappings from SELECT's column.
		/// Make a copy of the DataAdapter, Command, and Connection
		/// objects to avoid accidentally overriding fields in them.
		/// </summary>
		/// <param name="dbDataAdapter">A writeable DataAdapter.</param>
		/// <returns>2-column DataTable to hold the names.</returns>
		static internal DataTable BuildMapFromDatabase(DbDataAdapter dbAdapter)
		{
			DataTable dt       = new DataTable("mapping");
			dt.Columns.Add( SOURCE_COLUMNS );
			dt.Columns.Add( DATASET_COLUMNS );

			IDbDataAdapter dataAdapter= (IDbDataAdapter)dbAdapter;
			IDbCommand     dbCommand  = dataAdapter.SelectCommand;

			// safety check for a missing or incomplete Command
			if (dbCommand == null  ||
				dbCommand.CommandText == null  ||
				dbCommand.CommandText.Trim().Length == 0)
			{
				System.Windows.Forms.MessageBox.Show(
					"Data Adapter's SELECT command is null or an empty string; " +
					"table mappings are not available.",
					"Table Mappings");
				return dt;
			}

			// safety check for a missing Connection
			if (dbCommand.Connection == null)
			{
				System.Windows.Forms.MessageBox.Show(
					"Data Adapter's SELECT command's Connection is null; " +
					"table mappings are not available.",
					"Table Mappings");
				return dt;
			}

			// make a schema table that the DbAdapter can fill in
			DataTable dtSchema = new DataTable();

			Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
			Cursor.Current = Cursors.WaitCursor;  // hourglass cursor
			try
			{
				dbAdapter.FillSchema(dtSchema, SchemaType.Source);

				// copy the schema columns to the 2-column DataTable
				// that will be the DataSource for the form's datagrid.
				for (int i=0; i<dtSchema.Columns.Count; i++)
				{
					DataColumn dataColumn = dtSchema.Columns[i];

					System.Data.DataRow datarow = dt.NewRow();
					datarow[ TableMappings.SOURCE_COLUMNS  ] =
						dataColumn.ColumnName;
					datarow[ TableMappings.DATASET_COLUMNS ] =
						dataColumn.ColumnName;
					dt.Rows.Add( datarow );
				}
			}
			finally { Cursor.Current = cursor; } // resored Arrow cursor

			return dt;  // return with all went well
		}  // BuildMapFromDatabase


		static internal string BuildDataSetNameFromCommand(
			DbDataAdapter dbAdapter)
		{
			if (dbAdapter == null)
				return "Table";

			IDbDataAdapter dataAdapter = (IDbDataAdapter)dbAdapter;
			IDbCommand     dbCommand   = dataAdapter.SelectCommand;
			if (dataAdapter                           == null  || 
				dataAdapter.SelectCommand             == null  ||
				dataAdapter.SelectCommand.CommandText == null)
				return "Table";

			return BuildDataSetNameFromCommand(
				dataAdapter.SelectCommand.CommandText);
		}  // BuildDataSetNameFromCommand(DbDataAdapter dbAdapter)


		static internal string BuildDataSetNameFromCommand(
			string cmdText)
		{
			if (cmdText == null  ||
				cmdText.Length == 0)
				return "Table";

			try
			{
				// Scan the SELECT statment,
				// set the CmdTokenList token list for the SQL statement, and
				// build the column and table names lists.
				Ingres.ProviderInternals.MetaData md =
					new Ingres.ProviderInternals.MetaData(cmdText);
				if (md        != null  &&
					md.Tables != null  &&
					md.Tables.Count > 0)
				{
					Ingres.ProviderInternals.MetaData.Table t =
						(Ingres.ProviderInternals.MetaData.Table)md.Tables[0];
					return t.TableName;  // found it!  return 1st table name
				}
			}
			catch(FormatException)  // catch invalid syntax; i.e. no SELECT
			{}
			return "Table";  // no SELECT stmt or tables; default to "Table"
		}



	}  // class TableMappings


}
