namespace Ingres.Client.Design
{
	partial class IngresDataConnectionUIControl
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.groupServer = new System.Windows.Forms.GroupBox();
			this.comboDatabase = new System.Windows.Forms.ComboBox();
			this.labelDatabase = new System.Windows.Forms.Label();
			this.comboPort = new System.Windows.Forms.ComboBox();
			this.labelPort = new System.Windows.Forms.Label();
			this.comboServer = new System.Windows.Forms.ComboBox();
			this.labelServer = new System.Windows.Forms.Label();
			this.groupLogin = new System.Windows.Forms.GroupBox();
			this.textPassword = new System.Windows.Forms.TextBox();
			this.labelPassword = new System.Windows.Forms.Label();
			this.comboUser = new System.Windows.Forms.ComboBox();
			this.labelUser = new System.Windows.Forms.Label();
			this.pictureBox = new System.Windows.Forms.PictureBox();
			this.groupServer.SuspendLayout();
			this.groupLogin.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.pictureBox)).BeginInit();
			this.SuspendLayout();
			// 
			// groupServer
			// 
			this.groupServer.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupServer.Controls.Add(this.comboDatabase);
			this.groupServer.Controls.Add(this.labelDatabase);
			this.groupServer.Controls.Add(this.comboPort);
			this.groupServer.Controls.Add(this.labelPort);
			this.groupServer.Controls.Add(this.comboServer);
			this.groupServer.Controls.Add(this.labelServer);
			this.groupServer.Location = new System.Drawing.Point(13, 13);
			this.groupServer.Name = "groupServer";
			this.groupServer.Size = new System.Drawing.Size(401, 100);
			this.groupServer.TabIndex = 0;
			this.groupServer.TabStop = false;
			this.groupServer.Text = "Server";
			// 
			// comboDatabase
			// 
			this.comboDatabase.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.comboDatabase.FormattingEnabled = true;
			this.comboDatabase.Location = new System.Drawing.Point(116, 52);
			this.comboDatabase.Name = "comboDatabase";
			this.comboDatabase.Size = new System.Drawing.Size(145, 21);
			this.comboDatabase.TabIndex = 5;
			this.comboDatabase.TextChanged += new System.EventHandler(this.OnTextChanged);
			// 
			// labelDatabase
			// 
			this.labelDatabase.AutoSize = true;
			this.labelDatabase.Location = new System.Drawing.Point(56, 55);
			this.labelDatabase.Name = "labelDatabase";
			this.labelDatabase.Size = new System.Drawing.Size(56, 13);
			this.labelDatabase.TabIndex = 4;
			this.labelDatabase.Text = "Database:";
			// 
			// comboPort
			// 
			this.comboPort.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.comboPort.FormattingEnabled = true;
			this.comboPort.Location = new System.Drawing.Point(300, 16);
			this.comboPort.Name = "comboPort";
			this.comboPort.Size = new System.Drawing.Size(78, 21);
			this.comboPort.TabIndex = 3;
			this.comboPort.TextChanged += new System.EventHandler(this.OnTextChanged);
			// 
			// labelPort
			// 
			this.labelPort.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.labelPort.AutoSize = true;
			this.labelPort.Location = new System.Drawing.Point(267, 20);
			this.labelPort.Name = "labelPort";
			this.labelPort.Size = new System.Drawing.Size(29, 13);
			this.labelPort.TabIndex = 2;
			this.labelPort.Text = "Port:";
			this.labelPort.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// comboServer
			// 
			this.comboServer.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.comboServer.FormattingEnabled = true;
			this.comboServer.Location = new System.Drawing.Point(116, 17);
			this.comboServer.Name = "comboServer";
			this.comboServer.Size = new System.Drawing.Size(145, 21);
			this.comboServer.TabIndex = 1;
			this.comboServer.TextChanged += new System.EventHandler(this.OnTextChanged);
			// 
			// labelServer
			// 
			this.labelServer.AutoSize = true;
			this.labelServer.Location = new System.Drawing.Point(7, 20);
			this.labelServer.Name = "labelServer";
			this.labelServer.Size = new System.Drawing.Size(105, 13);
			this.labelServer.TabIndex = 0;
			this.labelServer.Text = "Data Access Server:";
			this.labelServer.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// groupLogin
			// 
			this.groupLogin.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.groupLogin.Controls.Add(this.textPassword);
			this.groupLogin.Controls.Add(this.labelPassword);
			this.groupLogin.Controls.Add(this.comboUser);
			this.groupLogin.Controls.Add(this.labelUser);
			this.groupLogin.Location = new System.Drawing.Point(13, 154);
			this.groupLogin.Name = "groupLogin";
			this.groupLogin.Size = new System.Drawing.Size(245, 99);
			this.groupLogin.TabIndex = 1;
			this.groupLogin.TabStop = false;
			this.groupLogin.Text = "Login";
			// 
			// textPassword
			// 
			this.textPassword.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.textPassword.Location = new System.Drawing.Point(70, 63);
			this.textPassword.Name = "textPassword";
			this.textPassword.Size = new System.Drawing.Size(142, 20);
			this.textPassword.TabIndex = 3;
			this.textPassword.UseSystemPasswordChar = true;
			this.textPassword.TextChanged += new System.EventHandler(this.OnTextChanged);
			// 
			// labelPassword
			// 
			this.labelPassword.AutoSize = true;
			this.labelPassword.Location = new System.Drawing.Point(8, 66);
			this.labelPassword.Name = "labelPassword";
			this.labelPassword.Size = new System.Drawing.Size(56, 13);
			this.labelPassword.TabIndex = 2;
			this.labelPassword.Text = "Password:";
			this.labelPassword.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// comboUser
			// 
			this.comboUser.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
						| System.Windows.Forms.AnchorStyles.Right)));
			this.comboUser.FormattingEnabled = true;
			this.comboUser.Location = new System.Drawing.Point(70, 17);
			this.comboUser.Name = "comboUser";
			this.comboUser.Size = new System.Drawing.Size(142, 21);
			this.comboUser.TabIndex = 1;
			this.comboUser.TextChanged += new System.EventHandler(this.OnTextChanged);
			// 
			// labelUser
			// 
			this.labelUser.AutoSize = true;
			this.labelUser.Location = new System.Drawing.Point(18, 20);
			this.labelUser.Name = "labelUser";
			this.labelUser.Size = new System.Drawing.Size(46, 13);
			this.labelUser.TabIndex = 0;
			this.labelUser.Text = "User ID:";
			this.labelUser.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
			// 
			// pictureBox
			// 
			this.pictureBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.pictureBox.Location = new System.Drawing.Point(315, 134);
			this.pictureBox.Name = "pictureBox";
			this.pictureBox.Size = new System.Drawing.Size(98, 119);
			this.pictureBox.TabIndex = 2;
			this.pictureBox.TabStop = false;
			// 
			// IngresDataConnectionUIControl
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Controls.Add(this.pictureBox);
			this.Controls.Add(this.groupLogin);
			this.Controls.Add(this.groupServer);
			this.Name = "IngresDataConnectionUIControl";
			this.Size = new System.Drawing.Size(432, 300);
			this.Load += new System.EventHandler(this.OnLoad);
			this.Leave += new System.EventHandler(this.OnLeave);
			this.groupServer.ResumeLayout(false);
			this.groupServer.PerformLayout();
			this.groupLogin.ResumeLayout(false);
			this.groupLogin.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.pictureBox)).EndInit();
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.GroupBox groupServer;
		private System.Windows.Forms.GroupBox groupLogin;
		private System.Windows.Forms.Label labelServer;
		private System.Windows.Forms.ComboBox comboPort;
		private System.Windows.Forms.Label labelPort;
		private System.Windows.Forms.ComboBox comboServer;
		private System.Windows.Forms.Label labelDatabase;
		private System.Windows.Forms.ComboBox comboDatabase;
		private System.Windows.Forms.Label labelUser;
		private System.Windows.Forms.ComboBox comboUser;
		private System.Windows.Forms.Label labelPassword;
		private System.Windows.Forms.TextBox textPassword;
		private System.Windows.Forms.PictureBox pictureBox;
	}
}