/*
** Copyright (c) 2003, 2006 Ingres Corporation. All Rights Reserved.
*/

using System;
using System.Collections;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Drawing;
using System.Resources;
using System.Runtime.Serialization;
using System.Windows.Forms;

namespace Ingres.Client.Design
{
	/*
	** Name: vsadapter.cs
	**
	** Description:
	**	Implements the VS.NET interfaces for DataAdapter wizard.
	**
	** Classes:
	**	IngresDataAdapterToolboxItem
	**	DataAdapterWizard
	**	DataAdapterWizardWelcomeForm
	**	DataAdapterWizardGenerateForm
	**	DataAdapterWizardBaseForm
	**	DesignerNameManager
	**
	** History:
	**	01-Apr-03 (thoda04)
	**	    Created.
	**	21-jun-04 (thoda04)
	**	    Cleaned up code for Open Source.
	*/



	internal class DataAdapterWizardWelcomeForm : DataAdapterWizardBaseForm
	{
		public DataAdapterWizardWelcomeForm()
		{
			this.Name = "DataAdapterWizardWelcomeForm";
			btnBackButton.Enabled = false;

			// load the picture bitmap
			PictureBox picture = new PictureBox();
			System.Reflection.Assembly assembly = GetType().Module.Assembly;
			Bitmap bitmap =
				new Bitmap(assembly.GetManifestResourceStream(bitmapName));
			picture.Size = new Size(bitmap.Width, bitmap.Height);
			picture.Image = bitmap;
			picture.TabStop = false;
			picture.BorderStyle = BorderStyle.FixedSingle;
			picture.Parent  = this;

			Label titleLabel = new Label();
			titleLabel.Anchor   =
				AnchorStyles.Top     |
				AnchorStyles.Left    |
				AnchorStyles.Right;
			titleLabel.Font = new Font(this.Font.FontFamily, 14F, FontStyle.Bold);
			titleLabel.BackColor = SystemColors.Window;
			titleLabel.ForeColor = SystemColors.WindowText;
			titleLabel.Location  = new Point(picture.Size.Width, 0);
			titleLabel.Name     = "titleLabel";
			titleLabel.Size      = new Size(MainSize.Width - titleLabel.Location.X,
			                                72);
			titleLabel.Parent    = this;
			titleLabel.Text      = "Welcome to the " + prodName +
				" Data Adapter Configuration Wizard";

			Label msgLabel = new Label();
			msgLabel.Anchor   =
				AnchorStyles.Top     |
				AnchorStyles.Bottom  |
				AnchorStyles.Left    |
				AnchorStyles.Right;
			msgLabel.BackColor = SystemColors.Window;
			msgLabel.ForeColor = SystemColors.WindowText;
			msgLabel.Location  = new Point(picture.Size.Width, titleLabel.Size.Height);
			msgLabel.Name      = "msgLabel";
			msgLabel.Size      = new Size(MainSize.Width - titleLabel.Location.X,
				MainSize.Height - titleLabel.Size.Height);
			msgLabel.Parent    = this;
			msgLabel.Text      =
				"This wizard helps you in specifying the Connection and " +
				"set of SQL Command objects that are to be used by the DataAdapter " +
				"in retrieving and updating records in the database.\n\n" +
				"Click Next to continue";

		}  //  DataAdapterWizardWelcomeForm constructor
	}  //  class DataAdapterWizardWelcomeForm


	internal class DataAdapterWizardGenerateForm : DataAdapterWizardBaseForm
	{
		TextBox txtbox;

		public DataAdapterWizardGenerateForm(IngresCommand command)
		{
			this.Command = command;
			this.CommandText =
				command.CommandText!=null?command.CommandText:String.Empty;

			this.Name = "DataAdapterWizardGenerateForm";

			Panel panel = new Panel();
			panel.Parent = this;
			panel.Location = new Point(0,0);
			panel.Size     = new Size(MainSize.Width, 68);
			panel.BackColor= SystemColors.Window;
			panel.BorderStyle = BorderStyle.FixedSingle;
			panel.Anchor   =
				AnchorStyles.Top     |
				AnchorStyles.Left    |
				AnchorStyles.Right;

			// load the picture bitmap
			PictureBox picture = new PictureBox();
			System.Reflection.Assembly assembly = GetType().Module.Assembly;
			Image bitmap =
				Image.FromStream(assembly.GetManifestResourceStream(facemapName));
			picture.Size = new Size(48, 60);
			picture.Image = bitmap;
			picture.SizeMode = PictureBoxSizeMode.StretchImage;
			picture.TabStop = false;
			picture.Location = new Point(panel.Width-picture.Size.Width-15, 3);
			picture.Anchor   =
				AnchorStyles.Top     |
				AnchorStyles.Right;
			picture.BorderStyle = BorderStyle.FixedSingle;
			picture.Parent  = panel;

			Label titleLabel = new Label();
			titleLabel.Text      = "Generate the SQL statements";
			titleLabel.Parent    = panel;
			titleLabel.Anchor    =
				AnchorStyles.Top     |
				AnchorStyles.Left    |
				AnchorStyles.Right;
			titleLabel.Font = new Font(
				this.Font.FontFamily, this.Font.Height, FontStyle.Bold);
			titleLabel.BackColor = SystemColors.Window;
			titleLabel.ForeColor = SystemColors.WindowText;
			titleLabel.Location  = new Point(FormFontHeight, FormFontHeight);
			titleLabel.Name     = "titleLabel";
			titleLabel.Size      = new Size(MainSize.Width, 3*this.Font.Height/2);

			Label msgLabel = new Label();
			msgLabel.Text      =
				"The Select statement will be used to retrieve data from " +
				"the database.";
			msgLabel.Parent   = panel;
			msgLabel.Anchor   =
				AnchorStyles.Top     |
				AnchorStyles.Left    |
				AnchorStyles.Right;
			//msgLabel.Font = new Font(this.Font.FontFamily, 14F, FontStyle.Bold);
			msgLabel.BackColor = SystemColors.Window;
			msgLabel.ForeColor = SystemColors.WindowText;
			msgLabel.Location  = new Point(
				2*FormFontHeight, titleLabel.Location.Y + titleLabel.Size.Height);
			msgLabel.Name      = "msgLabel";
			msgLabel.Size      = new Size(
				MainSize.Width - 6*FormFontHeight, 3*FormFontHeight);

			Label msgLabel1 = new Label();
			msgLabel1.Text      =
				"Enter your SQL command or click on Query Builder to design your query.";
			msgLabel1.Parent    = this;
			msgLabel1.Anchor   =
				AnchorStyles.Top     |
				AnchorStyles.Left    |
				AnchorStyles.Right;
			//msgLabel1.Font = new Font(this.Font.FontFamily, 14F, FontStyle.Bold);
			//msgLabel1.BackColor = SystemColors.Control;
			//msgLabel1.ForeColor = SystemColors.ControlText;
			msgLabel1.Location  = Below(panel, FormFontHeight, FormFontHeight);
			msgLabel1.Name      = "msgLabelSelect";
			msgLabel1.Size      = new Size(
				MainSize.Width - msgLabel1.Location.X, 2*FormFontHeight);

			Label msgLabel2 = new Label();
			msgLabel2.Text      =
				"SELECT Command Text:";
			msgLabel2.Parent    = this;
			msgLabel2.Anchor   =
				AnchorStyles.Top     |
				AnchorStyles.Left    |
				AnchorStyles.Right;
			//msgLabel2.Font = new Font(this.Font.FontFamily, 14F, FontStyle.Bold);
			//msgLabel2.BackColor = SystemColors.Control;
			//msgLabel2.ForeColor = SystemColors.ControlText;
			msgLabel2.Location  = Below(msgLabel1, FormFontHeight);
			msgLabel2.Name      = "msgLabelSelectCommand";
			msgLabel2.Size      = new Size(
				MainSize.Width - msgLabel2.Location.X, FormFontHeight);

			txtbox = new TextBox();
			txtbox.Parent = this;
			txtbox.Anchor   =
				AnchorStyles.Top     |
				AnchorStyles.Bottom  |
				AnchorStyles.Left    |
				AnchorStyles.Right;
			txtbox.Multiline = true;
			txtbox.ScrollBars = ScrollBars.Both;
			txtbox.AcceptsReturn = true;
			txtbox.AcceptsTab    = true;
			txtbox.Location  = Below(msgLabel2);
			txtbox.Size      = new Size(
				MainSize.Width - 2*txtbox.Location.X,
				MainSize.Height- txtbox.Location.Y - 2*ButtonSize.Height);
			txtbox.TextChanged +=
				new System.EventHandler(this.txtbox_TextChanged);
			txtbox.Leave +=
				new System.EventHandler(this.txtbox_OnLeave);

			// QueryBuilder button
			Button btnBuilderButton   = new Button();
			btnBuilderButton.Parent   = this;
			btnBuilderButton.Text     = "&Query Builder...";
			btnBuilderButton.Size     = new Size(4*ButtonSize.Width/3, ButtonSize.Height);
			btnBuilderButton.Location =
				new Point(ClientSize.Width - btnBuilderButton.Size.Width-10,
							Below(txtbox, FormFontHeight).Y);
			btnBuilderButton.Anchor   =
				AnchorStyles.Bottom |
				AnchorStyles.Right;
			btnBuilderButton.Click +=
				new System.EventHandler(this.btnQueryBuilder_Click);

			// if Back or Finish buttons clicked then set form's CommandText
			btnBackButton.Click +=
				new System.EventHandler(this.btnFinishOrBackButton_Click);
			btnFinishButton.Click +=
				new System.EventHandler(this.btnFinishOrBackButton_Click);

			this.Load += new System.EventHandler(
				this.DataAdapterWizardGenerateForm_Load);

		}  //  DataAdapterWizardGenerateForm constructor

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
		** Command property
		*/
		IngresCommand      _command;
		[Description("The Command to be used by this query designer.")]
		public IngresCommand Command
		{
			get {	return _command;  }
			set {	_command = (IngresCommand)value;  }
		}

		// Called (eventually) by form.ShowDialog().
		// The constructors for the form and its controls was called already.
		private void DataAdapterWizardGenerateForm_Load(
			object sender, System.EventArgs e)
		{
			txtbox.Text = this.CommandText;  // init the dialog text box
			btnNextButton.Enabled = false;   // no more Next button
		}

		private void txtbox_TextChanged(object sender, System.EventArgs e)
		{
			if (txtbox.Text.Length        == 0  ||
				txtbox.Text.Trim().Length == 0)
				btnFinishButton.Enabled = false;
			else
				btnFinishButton.Enabled = true;
		}

		// Finish Button clicked.
		private void txtbox_OnLeave(object sender, System.EventArgs e)
		{
			this.CommandText = txtbox.Text;
		}

		// Finish Button clicked.
		private void btnFinishOrBackButton_Click(
			object sender, System.EventArgs e)
		{
			this.CommandText = txtbox.Text.Trim();
		}

		// Query Builder Button clicked.
		private void btnQueryBuilder_Click(object sender, System.EventArgs e)
		{
			Cursor cursor = Cursor.Current;   // save cursor, probably Arrow
			Cursor.Current = Cursors.WaitCursor;  // hourglass cursor
			QueryDesignerForm form =
				new QueryDesignerForm(Command.Connection, this.CommandText);
			Cursor.Current = cursor;  // resored Arrow cursor

			DialogResult rc = form.ShowDialog();
			if (rc != DialogResult.OK)
				return;

			txtbox.Text = form.CommandText;  // return the CommandText
			this.CommandText = form.CommandText;
		}  // btnQueryBuilder_Click

	}  // class DataAdapterWizardGenerateForm

	
}  // namespace
