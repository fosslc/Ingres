/*
** Copyright (c) 2004 Ingres Corporation. All Rights Reserved.
*/
using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using Ingres.ProviderInternals;

namespace Ingres.Client.Design
{
	/// <summary>
	/// Summary description for vsaddtable.
	/// </summary>
	internal class AddTableForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Button buttonClose;
		private System.Windows.Forms.Button buttonAdd;
		private System.Windows.Forms.Button buttonRefresh;
		private System.Windows.Forms.TabPage tabPage;
		private System.Windows.Forms.TabControl tabControl;
		private System.Windows.Forms.ListBox listBox;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		static string[] pageTitles = new String[]
				{
					"User Tables",
					"User Views",
					"All Tables",
					"All Views"
				};

		/// <summary>
		/// Catalog property for tables and view information
		/// </summary>
		Catalog _catalog;
		Catalog Catalog
		{
			get {return _catalog;}
		}

		/// <summary>
		/// QueryDesignerForm that is requesting Add Table... and
		/// who should be called back to update the quest test.
		/// </summary>
		Design.QueryDesignerForm _queryDesignForm;
		Design.QueryDesignerForm QueryDesignerForm
		{
			get {return _queryDesignForm;}
		}

		/// <summary>
		/// Constructor.
		/// </summary>
		/// <param name="catalog"></param>
		/// <param name="queryDesignForm"></param>
		internal AddTableForm(
			Catalog catalog, Design.QueryDesignerForm queryDesignForm)
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			_catalog = catalog;
			_queryDesignForm = queryDesignForm;
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
			this.buttonClose = new System.Windows.Forms.Button();
			this.buttonAdd = new System.Windows.Forms.Button();
			this.buttonRefresh = new System.Windows.Forms.Button();
			this.tabControl = new System.Windows.Forms.TabControl();
			this.tabPage = new System.Windows.Forms.TabPage();
			this.listBox = new System.Windows.Forms.ListBox();
			this.tabControl.SuspendLayout();
			this.tabPage.SuspendLayout();
			this.SuspendLayout();
			// 
			// buttonClose
			// 
			this.buttonClose.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right);
			this.buttonClose.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonClose.Location = new System.Drawing.Point(384, 232);
			this.buttonClose.Name = "buttonClose";
			this.buttonClose.TabIndex = 0;
			this.buttonClose.Text = "Close";
			// 
			// buttonAdd
			// 
			this.buttonAdd.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right);
			this.buttonAdd.Enabled = false;
			this.buttonAdd.Location = new System.Drawing.Point(280, 232);
			this.buttonAdd.Name = "buttonAdd";
			this.buttonAdd.TabIndex = 1;
			this.buttonAdd.Text = "Add";
			this.buttonAdd.Click += new System.EventHandler(this.buttonAdd_Click);
			// 
			// buttonRefresh
			// 
			this.buttonRefresh.Anchor = (System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left);
			this.buttonRefresh.Location = new System.Drawing.Point(16, 232);
			this.buttonRefresh.Name = "buttonRefresh";
			this.buttonRefresh.TabIndex = 2;
			this.buttonRefresh.Text = "Refresh";
			this.buttonRefresh.Click += new System.EventHandler(this.buttonRefresh_Click);
			// 
			// tabControl
			// 
			this.tabControl.Controls.AddRange(new System.Windows.Forms.Control[] {
																					 this.tabPage});
			this.tabControl.Location = new System.Drawing.Point(24, 8);
			this.tabControl.Name = "tabControl";
			this.tabControl.SelectedIndex = 0;
			this.tabControl.Size = new System.Drawing.Size(432, 208);
			this.tabControl.TabIndex = 3;
			this.tabControl.Click += new System.EventHandler(this.tabControl_Click);
			// 
			// tabPage
			// 
			this.tabPage.Controls.AddRange(new System.Windows.Forms.Control[] {
																				  this.listBox});
			this.tabPage.Location = new System.Drawing.Point(4, 22);
			this.tabPage.Name = "tabPage";
			this.tabPage.Size = new System.Drawing.Size(424, 182);
			this.tabPage.TabIndex = 0;
			this.tabPage.Text = "UserTables";
			// 
			// listBox
			// 
			this.listBox.Dock = System.Windows.Forms.DockStyle.Fill;
			this.listBox.Name = "listBox";
			this.listBox.Size = new System.Drawing.Size(424, 173);
			this.listBox.TabIndex = 0;
			this.listBox.SelectedIndexChanged += new System.EventHandler(this.listBox_SelectedIndexChanged);
			// 
			// AddTableForm
			// 
			this.AcceptButton = this.buttonClose;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(482, 273);
			this.ControlBox = false;
			this.Controls.AddRange(new System.Windows.Forms.Control[] {
																		  this.tabControl,
																		  this.buttonRefresh,
																		  this.buttonAdd,
																		  this.buttonClose});
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
			this.Name = "AddTableForm";
			this.Text = "Add Table";
			this.Load += new System.EventHandler(this.vsaddtable_Load);
			this.tabControl.ResumeLayout(false);
			this.tabPage.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		private string _selectedItem = null;
		private string SelectedItem
		{
			get { return _selectedItem; }
			set
			{
				_selectedItem = value;
				if (_selectedItem == null)
					this.buttonAdd.Enabled = false;
				else
					this.buttonAdd.Enabled = true;
			}
		}

		private void vsaddtable_Load(object sender, System.EventArgs e)
		{
			this.SuspendLayout();
			this.tabControl.SuspendLayout();

			this.tabControl.TabPages.Clear();

			int tabIndex = 0;

			foreach (string title in pageTitles)
			{
				ListBox listBox = new ListBox();
				listBox.Dock = System.Windows.Forms.DockStyle.Fill;
				listBox.Name = "listBox";
				listBox.Size = new System.Drawing.Size(424, 173);
				listBox.TabIndex = 0;
				listBox.SelectedIndexChanged +=
					new System.EventHandler(this.listBox_SelectedIndexChanged);

				TabPage tabPage = new TabPage();
				tabPage.Controls.Add(listBox);
				tabPage.Location = new System.Drawing.Point(4, 22);
				//tabPage.Name = "tabPage";
				tabPage.Size = new System.Drawing.Size(424, 182);
				tabPage.TabIndex = tabIndex++;
				tabPage.Text = title;

				this.tabControl.Controls.Add(tabPage);
			}
		
			this.tabControl.ResumeLayout();
			this.ResumeLayout();

			// load initial tab page
			tabControl_Click(this.tabControl, System.EventArgs.Empty);
		}

		private void tabControl_Click(object sender, System.EventArgs e)
		{
			TabControl tabControl = (TabControl)sender;
			TabPage    tabPage    = tabControl.SelectedTab as TabPage;
			if (tabPage == null)  // no page selected for some reason
				return;

			ListBox listBox = (ListBox)tabPage.Controls[0];
			listBox.SelectedIndex = -1;  // no selection just yet.
			this.SelectedItem = listBox.SelectedItem as string;

			if (listBox.DataSource != null)
				return;

			Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
			Cursor.Current = Cursors.WaitCursor;  // hourglass cursor

			try
			{
				listBox.DataSource  = new ArrayList(new string[] {"Loading..."});
				listBox.SelectedIndex = -1;  // no selection just yet.

				// get the list of tables/views for
				// "User Tables" or "User Tables" or "All Tables" or etc.
				ArrayList list =
					Catalog.GetAllCatalogTablesAndViews(pageTitles[tabPage.TabIndex]);

				ArrayList tables;

				if (tabPage.TabIndex < 2  &&  // if user tables or user views
					list.Count > 0)
				{
					tables = new ArrayList();
					foreach (Catalog.Table catTable in list)
						tables.Add(MetaData.WrapInQuotesIfEmbeddedSpaces(
							catTable.TableName)); // just show table/view name
				}
				else
					tables = list;  // show "schema name"."table name"
			
				listBox.DataSource = tables;
				listBox.SelectedIndex = -1;  // no selection just yet.
			}
			finally
			{
				Cursor.Current = cursor;  // restore Arrow cursor
			}
		}

		private void buttonRefresh_Click(object sender, System.EventArgs e)
		{
			// clear out the DataSource for all of the ListBoxes
			// and force a reload from the catalog.
			foreach (object obj in this.tabControl.Controls)
				if (obj.GetType() == typeof(TabPage))
				{
					TabPage tabPage = (TabPage)obj;
					ListBox listBox = (ListBox)tabPage.Controls[0];
					if (listBox.DataSource != null)
					{
						listBox.DataSource  = null;
						listBox.Items.Clear();
						listBox.Update();  // force a repaint to clear region
					}
				}

			Catalog.Refresh();  // clear out cache

			// reload current tab page
			tabControl_Click(this.tabControl, System.EventArgs.Empty);
		}

		private void listBox_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			ListBox listBox = (ListBox)sender;
			this.SelectedItem =
				listBox.SelectedItem!=null?listBox.SelectedItem.ToString():null;
		}

		private void buttonAdd_Click(object sender, System.EventArgs e)
		{
			Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
			Cursor.Current = Cursors.WaitCursor;  // hourglass cursor

			try
			{
				if (this.SelectedItem != null)
					QueryDesignerForm.AddTableToQueryText(this.SelectedItem);
			}
			finally
			{
			}
		}

	}
}
