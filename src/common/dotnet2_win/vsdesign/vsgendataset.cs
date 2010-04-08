/*
** Copyright (c) 2004, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Data;
using System.Data.Common;
using System.Drawing;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel.Design;
using System.ComponentModel;
using System.Windows.Forms;

namespace Ingres.Client.Design
{
	/*
	** Name: vsgendataset.cs
	**
	** Description:
	**	Designer and form code to process
	**	data adapter's "Generate DataSet..." verb.
	**
	** History:
	**	22-March-04 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/


	/// <summary>
	/// Summary description for vsgendataset.
	/// </summary>
	internal class GenDataSetForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.RadioButton radioButtonExisting;
		private System.Windows.Forms.ComboBox comboBoxExisting;
		private System.Windows.Forms.RadioButton radioButtonNew;
		private System.Windows.Forms.TextBox textBoxNew;
		private System.Windows.Forms.CheckedListBox checkedListBoxTables;
		private System.Windows.Forms.Button buttonOK;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.GroupBox groupBoxChooseDataset;
		private IServiceProvider serviceProvider;
		private DbDataAdapter    dbDataAdapter;
		private EnvDTE._DTE      dte;
		private EnvDTE.Project   currentProject;
		private string           currentProjectFullPath;
		private string           currentProjectNamespace;
		private ArrayList        existingDataSetProjectItems;
		private StringCollection existingDataSetNames;
		private StringCollection tableNames;

		private const string DATASETNAME_PREFIX = "IngresDataSet";
		private System.Windows.Forms.CheckBox checkBoxAddToDesigner;

		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		internal GenDataSetForm()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();
		}

		internal GenDataSetForm(
			IServiceProvider serviceProvider,
			DbDataAdapter dbDataAdapter,
			EnvDTE._DTE   dte)
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			this.serviceProvider = serviceProvider;
			this.dbDataAdapter   = dbDataAdapter;
			this.dte             = dte;
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
			this.radioButtonExisting = new System.Windows.Forms.RadioButton();
			this.comboBoxExisting = new System.Windows.Forms.ComboBox();
			this.radioButtonNew = new System.Windows.Forms.RadioButton();
			this.textBoxNew = new System.Windows.Forms.TextBox();
			this.label3 = new System.Windows.Forms.Label();
			this.checkedListBoxTables = new System.Windows.Forms.CheckedListBox();
			this.buttonOK = new System.Windows.Forms.Button();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.groupBoxChooseDataset = new System.Windows.Forms.GroupBox();
			this.checkBoxAddToDesigner = new System.Windows.Forms.CheckBox();
			this.groupBoxChooseDataset.SuspendLayout();
			this.SuspendLayout();
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(48, 24);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(432, 23);
			this.label1.TabIndex = 0;
			this.label1.Text = "Generate a Typed DataSet that includes the specified tables.";
			// 
			// radioButtonExisting
			// 
			this.radioButtonExisting.Enabled = false;
			this.radioButtonExisting.Location = new System.Drawing.Point(16, 32);
			this.radioButtonExisting.Name = "radioButtonExisting";
			this.radioButtonExisting.Size = new System.Drawing.Size(80, 24);
			this.radioButtonExisting.TabIndex = 2;
			this.radioButtonExisting.Text = "Existing:";
			this.radioButtonExisting.CheckedChanged += new System.EventHandler(this.radioButtonExisting_CheckedChanged);
			// 
			// comboBoxExisting
			// 
			this.comboBoxExisting.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.comboBoxExisting.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxExisting.Enabled = false;
			this.comboBoxExisting.Location = new System.Drawing.Point(136, 96);
			this.comboBoxExisting.Name = "comboBoxExisting";
			this.comboBoxExisting.Size = new System.Drawing.Size(306, 21);
			this.comboBoxExisting.TabIndex = 3;
			// 
			// radioButtonNew
			// 
			this.radioButtonNew.Enabled = false;
			this.radioButtonNew.Location = new System.Drawing.Point(16, 72);
			this.radioButtonNew.Name = "radioButtonNew";
			this.radioButtonNew.Size = new System.Drawing.Size(80, 24);
			this.radioButtonNew.TabIndex = 4;
			this.radioButtonNew.Text = "New:";
			this.radioButtonNew.CheckedChanged += new System.EventHandler(this.radioButtonNew_CheckedChanged);
			// 
			// textBoxNew
			// 
			this.textBoxNew.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.textBoxNew.Enabled = false;
			this.textBoxNew.Location = new System.Drawing.Point(136, 136);
			this.textBoxNew.Name = "textBoxNew";
			this.textBoxNew.Size = new System.Drawing.Size(306, 20);
			this.textBoxNew.TabIndex = 5;
			this.textBoxNew.Text = "";
			// 
			// label3
			// 
			this.label3.Location = new System.Drawing.Point(48, 216);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(416, 23);
			this.label3.TabIndex = 6;
			this.label3.Text = "Choose which Tables(s) to add to the DataSet:";
			// 
			// checkedListBoxTables
			// 
			this.checkedListBoxTables.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.checkedListBoxTables.Location = new System.Drawing.Point(48, 248);
			this.checkedListBoxTables.Name = "checkedListBoxTables";
			this.checkedListBoxTables.Size = new System.Drawing.Size(426, 64);
			this.checkedListBoxTables.TabIndex = 7;
			// 
			// buttonOK
			// 
			this.buttonOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonOK.Enabled = false;
			this.buttonOK.Location = new System.Drawing.Point(296, 339);
			this.buttonOK.Name = "buttonOK";
			this.buttonOK.TabIndex = 8;
			this.buttonOK.Text = "OK";
			this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
			// 
			// buttonCancel
			// 
			this.buttonCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.Location = new System.Drawing.Point(400, 339);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.TabIndex = 9;
			this.buttonCancel.Text = "Cancel";
			// 
			// groupBoxChooseDataset
			// 
			this.groupBoxChooseDataset.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.groupBoxChooseDataset.Controls.Add(this.radioButtonNew);
			this.groupBoxChooseDataset.Controls.Add(this.radioButtonExisting);
			this.groupBoxChooseDataset.Location = new System.Drawing.Point(48, 64);
			this.groupBoxChooseDataset.Name = "groupBoxChooseDataset";
			this.groupBoxChooseDataset.Size = new System.Drawing.Size(426, 120);
			this.groupBoxChooseDataset.TabIndex = 10;
			this.groupBoxChooseDataset.TabStop = false;
			this.groupBoxChooseDataset.Text = "Choose a dataset";
			// 
			// checkBoxAddToDesigner
			// 
			this.checkBoxAddToDesigner.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.checkBoxAddToDesigner.Checked = true;
			this.checkBoxAddToDesigner.CheckState = System.Windows.Forms.CheckState.Checked;
			this.checkBoxAddToDesigner.Location = new System.Drawing.Point(48, 336);
			this.checkBoxAddToDesigner.Name = "checkBoxAddToDesigner";
			this.checkBoxAddToDesigner.Size = new System.Drawing.Size(208, 24);
			this.checkBoxAddToDesigner.TabIndex = 11;
			this.checkBoxAddToDesigner.Text = "Add this dataset to the designer";
			// 
			// GenDataSetForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.buttonCancel;
			this.ClientSize = new System.Drawing.Size(522, 378);
			this.Controls.Add(this.checkBoxAddToDesigner);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.buttonOK);
			this.Controls.Add(this.checkedListBoxTables);
			this.Controls.Add(this.label3);
			this.Controls.Add(this.textBoxNew);
			this.Controls.Add(this.comboBoxExisting);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.groupBoxChooseDataset);
			this.Name = "GenDataSetForm";
			this.Text = "Generate DataSet";
			this.Load += new System.EventHandler(this.GenDataSetForm_Load);
			this.groupBoxChooseDataset.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		private void radioButtonExisting_CheckedChanged(object sender, System.EventArgs e)
		{
			RadioButton button = (RadioButton)sender;
			this.comboBoxExisting.Enabled = button.Checked;
		}

		private void radioButtonNew_CheckedChanged(object sender, System.EventArgs e)
		{
			RadioButton button = (RadioButton)sender;
			this.textBoxNew.Enabled = button.Checked;
		}

		private void GenDataSetForm_Load(object sender, System.EventArgs e)
		{
			if (dte == null)
			{
				MessageBox.Show(
					"Visual Studio .NET DTE could not be found.");
				return;
			}

			if (dte.ActiveDocument == null  ||
				dte.ActiveDocument.ProjectItem == null  ||
				dte.ActiveDocument.ProjectItem.ContainingProject == null)
			{
				MessageBox.Show(
					"Visual Studio .NET current project could not be found.");
				return;
			}

			currentProject = dte.ActiveDocument.ProjectItem.ContainingProject;
			currentProjectFullPath =
				GetProperty(currentProject.Properties, "FullPath");
			if (currentProjectFullPath.Length == 0  ||
				!System.IO.Directory.Exists(currentProjectFullPath))
			{
				MessageBox.Show(
					"Visual Studio .NET current project directory " +
					"\"" + currentProjectFullPath + "\"" +
					"could not be found.");
				return;
			}
			currentProjectNamespace=
				GetProperty(currentProject.Properties, "RootNamespace");
			if (currentProjectNamespace.Length == 0)
				currentProjectNamespace=
					GetProperty(currentProject.Properties, "DefaultNamespace");

			existingDataSetProjectItems =
				GetAllDataSetsInProjectItems(
					new ArrayList(), currentProject.ProjectItems);

			existingDataSetNames =
				GetProjectItemsNames(existingDataSetProjectItems);

			this.comboBoxExisting.DataSource = existingDataSetNames;

			this.textBoxNew.Text = CreateUniqueDataSetName();


			tableNames = new StringCollection();

			string tableName = "Table";
			IDbDataAdapter idbDataAdapter = this.dbDataAdapter as IDbDataAdapter;
			if (idbDataAdapter == null  ||
				idbDataAdapter.SelectCommand == null  ||
				idbDataAdapter.SelectCommand.CommandText == null ||
				idbDataAdapter.SelectCommand.CommandText.Length == 0)
			{
				MessageBox.Show("Adapter's SelectCommand is not configured",
					"Generate " + DATASETNAME_PREFIX);
				return;
			}

			string cmdText = idbDataAdapter.SelectCommand.CommandText;
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
					tableName = t.TableName;
				}
			}
			catch(FormatException)  // catch invalid syntax
			{
				MessageBox.Show("Adapter's SelectCommand is not valid syntax.",
					"Generate " + DATASETNAME_PREFIX);
				return;
			}
			Component component = (Component)this.dbDataAdapter;
			tableName = tableName + " (" + component.Site.Name + ")";
			this.checkedListBoxTables.Items.Add(tableName, CheckState.Checked);

			// we're good to go; activate controls
			if (existingDataSetProjectItems.Count > 0)
				this.radioButtonExisting.Enabled = true;
			this.radioButtonNew.Enabled      = true;
			this.radioButtonNew.Checked      = true;
			this.buttonOK.Enabled            = true;
			this.buttonOK.Focus();  // set focus to OK button
		}  // GenDataSetForm_Load


		/// <summary>
		/// Sweep the env and return a list of DataSets' ProjectItem.
		/// </summary>
		/// <returns>An ArrayList of ProjectItem that reference a DataSet.</returns>
		private ArrayList
		GetAllDataSetProjectItems()
		{
			ArrayList list = new ArrayList();

			Array projects = dte.ActiveSolutionProjects as Array;
			if (projects == null  ||  projects.Length == 0)  // shouldn't happen
				return list;  // return empty list

			foreach(Object obj in projects)
			{
				EnvDTE.Project project = obj as EnvDTE.Project;
				if (project == null)
					continue;
				GetAllDataSetsInProjectItems(list, project.ProjectItems);
			}

			return list;
		}  // GetAllDataSets


		/// <summary>
		/// Sweep the ProjectItems collections and add to the list of
		/// DataSets' ProjectItem.  
		/// Can be called recursively for nested projects.
		/// </summary>
		/// <param name="list">ArrayList to add ProjectItem of DataSet to.</param>
		/// <param name="projectItems">
		/// ProjectItems collections to iterate through.</param>
		/// <returns>The same ArrayList as was passed in but perhaps
		/// with additional memebers.</returns>
		private ArrayList
		GetAllDataSetsInProjectItems(ArrayList list, EnvDTE.ProjectItems projectItems)
		{
			if (projectItems == null)
				MessageBox.Show("Project.projectItems is null");

			foreach(Object obj in projectItems)
			{
				EnvDTE.ProjectItem projItem = obj as EnvDTE.ProjectItem;

				if (projItem != null       &&
					projItem.Name != null  &&
					projItem.Name.Length > 0)
				{
					// only add xsd files that use MSDataSetGenerator
					string extension =   // get the file extension
						GetProperty(projItem.Properties, "Extension");
					if (ToLower(extension) != ".xsd")
						continue;
					string customTool =
						GetProperty(projItem.Properties, "CustomTool");
					if (ToLower(customTool) != "msdatasetgenerator")
						continue;
					list.Add(projItem);
				}
			}

			return list;
		}  // GetAllDataSetsInProject


		private StringCollection GetProjectItemsNames(ArrayList list)
		{
			StringCollection stringCollection = new StringCollection();

			foreach(object obj in list)
			{
				EnvDTE.ProjectItem projectItem = obj as EnvDTE.ProjectItem;
				if (projectItem == null)  continue;  // safety check

				System.Text.StringBuilder sb = new System.Text.StringBuilder();

				GetParentProjectName(sb, projectItem);
				if (projectItem.Name == null)  // should never happen
					continue;

				sb.Append(projectItem.Name);

				if (sb.Length > 4)  // strip the .xsd suffix off the string
				{
					string s = sb.ToString();
					if (s.Substring(s.Length-4) == ".xsd")
						sb.Length -= 4;
				}
				stringCollection.Add(sb.ToString());
			}

			return stringCollection;
		}

		private void GetParentProjectName(
			System.Text.StringBuilder sb, EnvDTE.ProjectItem projectItem)
		{
			if (projectItem == null)  // safety check
				return;

			EnvDTE.Project parentProject = projectItem.ContainingProject
				as EnvDTE.Project;
			if (parentProject == null)
				MessageBox.Show("parentProject is null", projectItem.Name);
			if (parentProject.Name == null)
				MessageBox.Show("parentProject.Name is null", projectItem.Name);

			if (parentProject == null  ||  parentProject.Name == null)
				return;

			EnvDTE.ProjectItem parentProjectItem =
				parentProject.ParentProjectItem as EnvDTE.ProjectItem;
			if (parentProjectItem != null)  // get nested parent
				GetParentProjectName(sb, parentProjectItem);

			sb.Append(parentProject.Name);
			sb.Append('.');
		}


		/// <summary>
		/// Create a unique DataSet Name for the New text box by
		/// scanning the current project's directory for any conflicts.
		/// Starting at 1, next the next available numeric.
		/// </summary>
		/// <returns></returns>
		private string CreateUniqueDataSetName()
		{
			string[] files =  // file list is fully qualified
				System.IO.Directory.GetFiles(currentProjectFullPath);

			int fileNumberHWM = 0;
			string prefix = ToLower(DATASETNAME_PREFIX);

			foreach(string filenm in files)  // find highest IngresDataSetnn
			{
				string filename = ToLower(filenm);

				int index = filename.LastIndexOf('\\');  // strip path qualifier
				if (index ==filename.Length)  // should never happen but...
					continue;
				filename = filename.Substring(index+1);

				if (filename.Length <= prefix.Length) // skip the too short ones
					continue;

				if (filename.Substring(0, prefix.Length) != prefix)
					continue;

				int fileNumber = 0;
				for (int i = prefix.Length; i < filename.Length; i++)
				{
					char c = filename[i];
					if (c == '.')       // all done if end of digits
						break;
					if (!Char.IsDigit(c))  // bad number format; don't count it
					{
						fileNumber = 0;
						break;
					}
					fileNumber = fileNumber*10 + Convert.ToInt32(c.ToString());
				}  // end loop through filename

				if (fileNumberHWM < fileNumber)  // bump up high water mark
					fileNumberHWM = fileNumber;  // of file name number
			}  // end loop through files

			fileNumberHWM++;  // next higher number for the prefix
			return (DATASETNAME_PREFIX + fileNumberHWM.ToString());
		}

		private string GetProperty(EnvDTE.Properties properties, string name)
		{
			foreach(EnvDTE.Property property in properties)
			{
				if (property.Name == name  &&  property.Value != null)
					return (property.Value.ToString());
			}
			return String.Empty;
		}

		private string ToLower(string strValue)
		{
			if (strValue == null)
				return null;
			return strValue.ToLower(
				System.Globalization.CultureInfo.InvariantCulture);
		}

		private DbDataAdapter GetDataAdapterClone( DbDataAdapter oldAdapter )
		{
			DbDataAdapter adapter =
				DataAdapterUtil.GetDataAdapterClone(oldAdapter);

			IDbCommand selectCommand = ((IDbDataAdapter)adapter).SelectCommand;
			if (selectCommand                    == null  ||
				selectCommand.CommandText        == null  ||
				selectCommand.CommandText.Length == 0)
			{
				MessageBox.Show("Adapter's SelectCommand is not configured",
					"Generate " + DATASETNAME_PREFIX);
				return null;
			}

			IDbConnection connection = selectCommand.Connection;
			if (connection                         == null  ||
				connection.ConnectionString        == null  ||
				connection.ConnectionString.Length == 0)
			{
				MessageBox.Show("Adapter's SelectCommand.Connection is not configured",
					"Generate " + DATASETNAME_PREFIX);
				return null;
			}

			return adapter;
		}  // GetDataAdapterClone


		private void buttonOK_Click(object sender, System.EventArgs e)
		{
			string tableName = "Table";

			if (this.checkedListBoxTables.CheckedItems.Count == 0)
			{
				MessageBox.Show("No tables were selected to be added to the dataset.",
					"Generate " + DATASETNAME_PREFIX);
				return;
			}
			string entry = this.checkedListBoxTables.CheckedItems[0].ToString();
			int i = entry.LastIndexOf(" (");
			if (i == -1)  // should never happen
				return;
			tableName = entry.Substring(0, i);

			DbDataAdapter adapter = GetDataAdapterClone( this.dbDataAdapter );
			if (adapter == null)  // return if badly configured adapter
				return;

			string dsName = "Table";
			if (this.radioButtonNew.Checked)
				dsName = this.textBoxNew.Text.Trim();
			else
			{
				string s = this.comboBoxExisting.Text;
				i = s.LastIndexOf('.');
				if (i == -1)
					dsName = s.Trim();
				else
					if (i < (s.Length-1))
						dsName = s.Substring(i+1).Trim();
					else
						dsName = String.Empty;

			}

			if (dsName.Length == 0)
			{
				MessageBox.Show("Dataset name is invalid.",
					"Generate " + DATASETNAME_PREFIX);
				return;
			}

			// make a DataSet that the DbAdapter can fill in
			DataSet ds = new DataSet(dsName);

			Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
			Cursor.Current = Cursors.WaitCursor;  // hourglass cursor
			adapter.MissingSchemaAction = MissingSchemaAction.Add;
			adapter.MissingMappingAction=
				(this.dbDataAdapter.MissingMappingAction==
					MissingMappingAction.Error)?
				MissingMappingAction.Ignore : 
				this.dbDataAdapter.MissingMappingAction;
			try { adapter.FillSchema(ds, SchemaType.Mapped); }
			catch(Exception ex)
			{
				Cursor.Current = cursor;
				MessageBox.Show(ex.Message,
					"Unable to generate " + DATASETNAME_PREFIX);
				return;
			}
			finally { Cursor.Current = cursor; } // restored Arrow cursor

			ds.Namespace = "http://www.tempuri.org/" + dsName+".xsd";

			// strip the useless MaxLength info that adds clutter to the XML
			foreach (DataTable dataTable in ds.Tables)
				for (int j=0; j<dataTable.Columns.Count; j++)
				{
					DataColumn dataColumn = dataTable.Columns[j];
					dataColumn.MaxLength = -1;
				}

			string fileName =
				System.IO.Path.Combine(currentProjectFullPath, dsName+".xsd");

			ds.WriteXmlSchema(fileName);  // write the ingresDataSetn.xsd file

			EnvDTE.ProjectItem projItem = null;
			if (this.radioButtonExisting.Checked)  // if  existing project item
			{
				i = this.comboBoxExisting.SelectedIndex;
				projItem =
					(EnvDTE.ProjectItem)(this.existingDataSetProjectItems[i]);
			}
			else                                       // else new project item
			{
				projItem = currentProject.ProjectItems.AddFromFile(fileName);
				EnvDTE.Property pp;
				pp = projItem.Properties.Item("CustomTool");
				pp.Value = "MSDataSetGenerator";
			}

			VSLangProj.VSProjectItem vsprojItem =
				projItem.Object as VSLangProj.VSProjectItem;
			if (vsprojItem != null)
				vsprojItem.RunCustomTool();    // update the .cs or .vb file

			if (!this.checkBoxAddToDesigner.Checked)  // return if all done
			{
				this.DialogResult = DialogResult.OK;
				return;
			}

			// add the new dataset to the component tray
			string componentBaseName;
			if (currentProjectNamespace.Length == 0)
				componentBaseName = dsName;
			else
				componentBaseName = currentProjectNamespace + "." + dsName;

			System.ComponentModel.Design.IDesignerHost host =
				serviceProvider.GetService(
				typeof(System.ComponentModel.Design.IDesignerHost))
				as System.ComponentModel.Design.IDesignerHost;
			if (host == null)
			{
				MessageBox.Show(
					"The IDesignerHost service interface " +
					"could not be obtained to process the request.",
					"Generate " + DATASETNAME_PREFIX);
				return;
			}

			Type componentType;
			try
			{
				componentType = host.GetType(componentBaseName);
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message,
					"Unable to get Type of " + componentBaseName);
				return;
			}

			IComponent newComponent;
			try
			{
				newComponent = host.CreateComponent(componentType);
				host.Container.Add(newComponent);
			}
			catch (Exception ex)
			{
				MessageBox.Show(ex.Message,
					"Unable to create component for " + componentBaseName);
				return;
			}

			// move the select pointer to the new component in the tray
			ISelectionService selService = newComponent.Site.GetService(
				typeof(ISelectionService)) as ISelectionService;
			if (selService != null)
			{
				ArrayList selList= new ArrayList(1);
				selList.Add(newComponent);
				selService.SetSelectedComponents(  // move the selection to new
					selList, SelectionTypes.Replace);
			}

			this.DialogResult = DialogResult.OK;
		}

	}




	public class GenDataSetDesigner : ComponentDesigner
	{
		public override DesignerVerbCollection Verbs
		{
			get
			{
				return new DesignerVerbCollection(
					new DesignerVerb[] {
						new DesignerVerb(
							"Generate DataSet...",
							new EventHandler(this.OnGenerate)) } );
			}
		}

		// Event handling method for the "Generate DataSet..." verb
		public void OnGenerate(object sender, EventArgs e)
		{
			EnvDTE._DTE dte = (EnvDTE._DTE)GetService(typeof(EnvDTE._DTE));

			Form dlg = new GenDataSetForm(
				this.Component.Site,                   // IServiceProvider
				(DbDataAdapter)this.Component, dte);
			DialogResult result = dlg.ShowDialog();
		}  // OnGenerate

		public void OnGenerateIngresDataSet(
			Component component, object sender, EventArgs e)
		{
			IServiceProvider sp = component.Site;
			EnvDTE._DTE dte = (EnvDTE._DTE)sp.GetService(typeof(EnvDTE._DTE));

			Form dlg = new GenDataSetForm(
				component.Site,                   // IServiceProvider
				(DbDataAdapter)component, dte);
			DialogResult result = dlg.ShowDialog();
		}  // OnGenerate
	}  // class GenDataSetDesigner

}
