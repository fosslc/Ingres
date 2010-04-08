// Copyright (c) 2006 Ingres Corporation

// Name: Form1.cs
//
// Description:
//      File extends the IngresFrequentFlyer class to provide the graphical
//      user interface for the Ingres Frequent Flyer application.
//
// Public:
//
// Private:
//
// History:
//      02-Oct-2006 (ray.fan@ingres.com)
//          Created.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace IngresDemoApp
{
    public partial class IngresFrequentFlyer : Form
    {
        public String appConfigFile;
        public String currentDirectory;

        public System.Resources.ResourceManager rm;
        public Panel[] formsPanels;
        public Panel[] infoPanels;
        public GroupBox[] profileGroupBoxes;
        public GroupBox[] instanceGroupBoxes;

        OpenFileDialog openFileDialog = new OpenFileDialog();
        SaveFileDialog saveFileDialog = new SaveFileDialog();

        private DataRow LastDataRow = null;

        public IngresFrequentFlyer()
        {
            InitializeComponent();         

            //  Get a reference to the resource manager.
            rm = new System.Resources.ResourceManager("IngresDemoApp.Properties.Resources",
                System.Reflection.Assembly.GetExecutingAssembly());

            // Get application configuration filename
            appConfigFile =
                System.IO.Path.GetFullPath(rm.GetString("AppConfig"));

            // Get current directory
            currentDirectory = Directory.GetCurrentDirectory();

            // Create an array of the forms panels.
            formsPanels  = new Panel[] { routesFormPanel, profilesFormPanel,
                connectFormPanel };

            // Create an array of information panels.
            infoPanels = new Panel[] { resultPanel, helpPanel, profilePanel };

            // Create an array of profile group boxes panels.
            profileGroupBoxes = new GroupBox[] {
                profilePersonalGroupBox,
                profilePreferenceGroupBox,
                profileAirportGroupBox };

            // Create an array of instance group boxes panels.
            instanceGroupBoxes = new GroupBox[] {
                defineSourceGroupBox };
        }

        // Name: ReadConfiguration
        //
        // Description:
        //      Reads the configuration from the specified configuration file.
        //
        // Inputs:
        //      configFile      Path and filename of configuration file.
        //
        // Outputs:
        //      ingresConnection1   Updated
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public void ReadConfiguration(String configfile)
        {
            appConfiguration = new DataSet("AppConfig");
            String appconfigxsd = rm.GetString("AppConfigSchema");
            try
            {
                xsdfile = System.IO.Path.GetFullPath(appconfigxsd);
                appConfiguration.ReadXmlSchema(appconfigxsd);
                appConfiguration.ReadXml(configfile, XmlReadMode.Auto);
                appConfiguration.Locale = System.Globalization.CultureInfo.CurrentUICulture;
                foreach (System.Data.DataRow row in appConfiguration.Tables["AppConfig"].Rows)
                {
                    String name = row["ParamName"].ToString();
                    String value = row["ParamValue"].ToString();
                    switch (name)
                    {
                        case "Host":
                            currentSource.host = value;
                            break;
                        case "Instance":
                            // Get instance id and add port number.
                            currentSource.port = value + "7";
                            break;
                        case "Database":
                            currentSource.db = value;
                            break;
                        case "UserID":
                            currentSource.user = value;
                            break;
                        case "Password":
                            currentSource.pwd = value;
                            break;
                    }
                }
                SetConnection(ingresConnection1, currentSource.GetConnectionString());
                if (appConfiguration.Tables["AppProfile"].Rows.Count > 0)
                {
                    foreach (System.Data.DataRow row in appConfiguration.Tables["AppProfile"].Rows)
                    {
                        String name = row["ParamName"].ToString();
                        String value = row["ParamValue"].ToString();
                        switch (name)
                        {
                            case "Email":
                                profileEmail = value;
                                break;
                        }
                    }
                }
                else
                {
                    profileEmail = null;
                }
            }
            catch (Exception ex)
            {
                DialogResult result = MessageBox.Show(ex.ToString(),
                    rm.GetString("Error"),
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
                throw;
            }
        }

        // Name: WriteConfiguration
        //
        // Description:
        //      Writes the current configuration into a configuration file.
        //
        // Inputs:
        //      configFile      Path and filename of configuration file.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public void WriteConfiguration(String configFile)
        {
            if (appConfiguration != null)
            {
                foreach (System.Data.DataRow row in appConfiguration.Tables["AppConfig"].Rows)
                {
                    String name = row["ParamName"].ToString();
                    String value = row["ParamValue"].ToString();
                    switch (name)
                    {
                        case "Host":
                            row["ParamValue"] = currentSource.host;
                            break;
                        case "Instance":
                            // Set instance to remove port number.
                            row["ParamValue"] = currentSource.port.Remove(2);
                            break;
                        case "Database":
                            row["ParamValue"] = currentSource.db;
                            break;
                        case "UserID":
                            row["ParamValue"] = currentSource.user;
                            break;
                        case "Password":
                            row["ParamValue"] = currentSource.pwd;
                            break;
                    }
                }
                if (appConfiguration.Tables["AppProfile"].Rows.Count > 0)
                {
                    foreach (System.Data.DataRow row in appConfiguration.Tables["AppProfile"].Rows)
                    {
                        String name = row["ParamName"].ToString();
                        String value = row["ParamValue"].ToString();
                        switch (name)
                        {
                            case "Email":
                                row["ParamValue"] = profileEmail;
                                break;
                        }
                    }
                }
                else
                {
                    String[] s = { "Email", profileEmail };
                    appConfiguration.Tables["AppProfile"].LoadDataRow(s, true);
                }
                appConfiguration.WriteXml(configFile);
                String path = System.IO.Path.GetDirectoryName(configFile);
                String tpath = Path.Combine(path, rm.GetString("AppConfigSchema"));
                if ((xsdfile != null) && (File.Exists(tpath) == false))
                {
                    File.Copy(xsdfile, tpath);
                }
            }
            else
            {
                DialogResult result = MessageBox.Show("Application parameters have not been configured",
                    rm.GetString("Error"),
                    MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        // Name: SetStaticControls
        //
        // Description:
        //      The airport country combo boxes contains data that doesn't
        //      change often.  Retrieve it once and make copies of it.
        //
        // Inputs:
        //      None.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void SetStaticControls()
        {
            BindingSource departingCountry = new BindingSource();
            BindingSource arrivingCountry = new BindingSource();
            BindingSource profileCountry = new BindingSource();

            this.Cursor = Cursors.WaitCursor;
            departingCountry.DataSource = countryDataSet1;
            departingCountry.DataMember = "country";
            departCountryComboBox.DataSource = departingCountry;
            LoadCountry(countryDataSet1);
            departCountryComboBox.SelectedIndex = -1;

            arriving.countryDataSet = (CountryDataSet)countryDataSet1.Copy();
            arrivingCountry.DataSource = arriving.countryDataSet;
            arrivingCountry.DataMember = "country";
            arriveCountryComboBox.DataSource = arrivingCountry;

            profile.countryDataSet = (CountryDataSet)countryDataSet1.Copy();
            profileCountry.DataSource = profile.countryDataSet;
            profileCountry.DataMember = "country";
            profileCountryComboBox.DataSource = profileCountry;
            this.Cursor = Cursors.Arrow;
        }

        // Name: SetStatus
        //
        // Description:
        //      Updates the status string displayed in the application status
        //      bar.
        //
        // Inputs:
        //      Current     Current active connection parameters.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void SetStatus(TargetDetails Current)
        {
            hostNameText.Text = Current.host;
            instanceText.Text = String.Format(rm.GetString("InstanceBase"),
                Current.port.Remove(2));
            databaseText.Text = Current.db;
            userIdText.Text = Current.user;
        }

        // Name: SetAirportControls
        //
        // Description:
        //      Populate the airport combo boxes and select the specified
        //      values.  Airport combo boxes consist of country, region and
        //      IATA code.
        //
        // Inputs:
        //      Selected    List of combo boxes to update.
        //      IATAcode    Retrieve information for this airport code.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void SetAirportControls(ComboBox[] Selected, String IATAcode)
        {
            AirportDataSet airportDataSet = new AirportDataSet();

            // Ensure we have the required number of combo boxes.
            if (Selected.Length == 3)
            {
                GetAirportInfo(airportDataSet.Tables["airportInfo"],
                    IATAcode);
                if (airportDataSet.Tables["airportInfo"].Rows.Count > 0)
                {
                    String ap_place =
                        (string)airportDataSet.Tables["airportInfo"].Rows[0]["ap_place"];
                    String ct_name =
                        (string)airportDataSet.Tables["airportInfo"].Rows[0]["ct_name"];
                    Selected[0].SelectedIndex = Selected[0].FindString(ct_name);
                    Selected[1].SelectedIndex = Selected[1].FindString(ap_place);
                    Selected[2].SelectedIndex = Selected[2].FindString(IATAcode);
                }
            }
        }

        // Name: ShowPanel
        //
        // Description:
        //      Makes the specified panel visible and makes all other panels
        //      in the list invisible.
        //
        // Inputs:
        //      PanelList       Array of Panel.
        //      Visible         Panel to make visible.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void ShowPanel(Panel[] PanelList, Panel Visible)
        {
            foreach (Panel show in PanelList)
            {
                show.Visible = (show == Visible) ? true : false;
            }
        }

        // Name: ShowGroupBoxes
        //
        // Description:
        //      Makes the specified buttons active and deactivates all other
        //      buttons in the list.
        //
        // Inputs:
        //      ButtonList      Array of buttons.
        //      ActiveList      Buttons to activate.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void ShowGroupBoxes(GroupBox[] GroupList, GroupBox[] ActiveList)
        {
            foreach (GroupBox groupBox in GroupList)
            {
                // Disable each button.
                groupBox.Enabled = false;
                foreach (GroupBox active in ActiveList)
                {
                    if (groupBox == active)
                    {
                        groupBox.Enabled = true;
                    }
                }
            }
        }

        // Name: ShowButtons
        //
        // Description:
        //      Makes the specified buttons active and deactivates all other
        //      buttons in the list.
        //
        // Inputs:
        //      ButtonList      Array of buttons.
        //      ActiveList      Buttons to activate.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void ShowButtons(Button[] ButtonList, Button[] ActiveList)
        {
            foreach (Button button in ButtonList)
            {
                // Disable each button.
                button.Enabled = false;
                foreach (Button active in ActiveList)
                {
                    if (button == active)
                    {
                        button.Enabled = true;
                    }
                }
            }
        }

        // Name: ClearComboBoxes
        //
        // Description:
        //      Given a list of combo boxes clear the list of items from each
        //      if it has been populated using a data source.
        //
        // Inputs:
        //      ComboBoxList    A list of combo boxes to clear.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public void ClearComboBoxes(ComboBox[] ComboBoxList)
        {
            foreach (ComboBox comboBox in ComboBoxList)
            {
                if (comboBox.Items.Count > 0)
                {
                    BindingSource bindingSource =
                        (BindingSource)comboBox.DataSource;
                    Type dataType = bindingSource.DataSource.GetType().BaseType;
                    if (dataType.Name.Equals("DataSet"))
                    {
                        DataSet dataSet = (DataSet)bindingSource.DataSource;
                        dataSet.Clear();
                    }
                    else
                    {
                        DataTable dataTable = (DataTable)bindingSource.DataSource;
                        dataTable.Clear();
                    }
                }
                else
                {
                    comboBox.Text = "";
                }
            }
        }

        // Name: LoadHelpTopic
        //
        // Description:
        //      Retrieves the path string from the topic parameter and loads
        //      the file into a browser control.  Adds the browser control to
        //      the helpPanel.
        //
        // Inputs:
        //      Topic       Help topic name
        //
        // Outputs:
        //      wb          Browser control updated.
        //      helpPanel   Help panel updated.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void LoadHelpTopic(String Topic)
        {
            System.Windows.Forms.WebBrowser wb = new WebBrowser();
            wb.Dock = DockStyle.Fill;
            String file = rm.GetString(Topic);
            if (file == null)
            {
                file = rm.GetString("HelpContents");
            }
            System.IO.FileInfo f = new FileInfo(file);

            wb.Navigate(f.FullName);
            helpPanel.Controls.Clear();
            helpPanel.Controls.Add(wb);
        }

        // 
        // Event Handlers
        //

        // Name: IngresFrequentFlyer_Load
        //
        // Description:
        //      Application form load initialization function.
        //
        // Inputs:
        //      sender      Application object.
        //      e           Event arguments.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void IngresFrequentFlyer_Load(object sender, EventArgs e)
        {
            String topic = "ConnectHelp";
            // Ensure that the lower horizontal panels fill the available
            // space.
            helpPanel.Dock = DockStyle.Fill;
            resultPanel.Dock = DockStyle.Fill;
            profilePanel.Dock = DockStyle.Fill;

            try
            {
                topic = "RouteHelp";
                ReadConfiguration(appConfigFile);
                savedSource = currentSource;
            }
            catch
            {
                // If an exception is caught here then the configuration
                // file is missing.
                // Display the open file dialog to select a new configuration
                // file.
                EventArgs eArg = new EventArgs();
                openToolStripMenuItem_Click(openToolStripMenuItem, eArg);
            }

            try
            {
                // Perform Ingres initialization.
                InitializeIngres();

                SetStatus(currentSource);
                SetStaticControls();
                RouteFormInitialize();
            }
            catch
            {
                // Data adapter initialization error.
                InstanceFormInitialize();
                definedSourceRadioButton.Checked = true;
                topic = "ConnectHelp";
            }

            finally
            {
                LoadHelpTopic(topic);
                ShowPanel(infoPanels, helpPanel);
            }
        }

        // Name: menuStrip1_Click
        //      
        // Description:
        //      Handles application menu selection.
        //
        // Inputs:
        //      sender  Form object.
        //      e       Event arguments.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void menuStrip1_Click(object sender, EventArgs e)
        {
            if ((sender == routeSearchToolStripMenuItem) ||
                (sender == routesToolButton))
            {
                RouteFormInitialize();
            }
            if ((sender == myProfileToolStripMenuItem) ||
                (sender == profilesToolButton))
            {
                ProfileFormInitialize();
            }
            if ((sender == connectToolStripMenuItem) ||
                (sender == connectToolButton))
            {
                InstanceFormInitialize();
            }
            if ((sender == exitToolStripMenuItem) ||
                (sender == exitToolButton))
            {
                    Application.Exit();
            }
        }

        // Name: openToolStripMenuItem_Click
        //
        // Description:
        //      Display an open file dialog for browsing and choosing
        //      another configuration file.
        //
        // Inputs:
        //      sender  Menu.
        //      e       Event arguments.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {
            openFileDialog.Title = rm.GetString("OpenConfig");
            openFileDialog.Filter = "XML|*.xml|All|*.*";
            openFileDialog.FilterIndex = 1;
            openFileDialog.FileName = appConfigFile;
            if (openFileDialog.ShowDialog() != DialogResult.Cancel)
            {
                appConfigFile = 
                    System.IO.Path.GetFullPath(openFileDialog.FileName);
                ReadConfiguration(appConfigFile);
                savedSource = currentSource;
                SetStatus(currentSource);
            }
            Directory.SetCurrentDirectory(currentDirectory);
        }

        // Name: saveToolStripMenuItem_Click
        //
        // Description:
        //      Event handler for the MenuItem click event.
        //
        // Inputs:
        //      sender  MenuItem
        //      e       Event arguments.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void saveToolStripMenuItem_Click(object sender, EventArgs e)
        {
            WriteConfiguration(appConfigFile);
        }

        // Name: saveAsToolStripMenuItem_Click
        //
        // Description:
        //      Event handler for the MenuItem click event.
        //
        // Inputs:
        //      sender  MenuItem
        //      e       Event arguments.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void saveAsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            openFileDialog.Title = rm.GetString("SaveAsConfig");
            openFileDialog.Filter = "XML|*.xml|All|*.*";
            openFileDialog.FilterIndex = 1;
            openFileDialog.FileName = appConfigFile;
            if (openFileDialog.ShowDialog() != DialogResult.Cancel)
            {
                appConfigFile =
                    System.IO.Path.GetFullPath(openFileDialog.FileName);
                WriteConfiguration(appConfigFile);
            }
            Directory.SetCurrentDirectory(currentDirectory);
        }

        // Name: aboutToolStripMenuItem_Click
        //
        // Description:
        //      Event handler for the MenuItem click event.
        //
        // Inputs:
        //      sender  MenuItem
        //      e       Event arguments.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            AboutBox aboutBox = new AboutBox();
            aboutBox.ShowDialog();
        }

        // Name: countryComboBox_HelpRequested
        //
        // Description:
        //      Event handler for the help requested event.
        //
        // Inputs:
        //      sender  Requesting control.
        //      e       Event arguments.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void control_HelpRequested(object sender, HelpEventArgs hlpevent)
        {
            // This event is raised when the F1 key is pressed or the
            // Help cursor is clicked on any of the address fields.
            // The Help keyword is used to retrieve the topic name that
            // specifies the path to the HTML help file.
            Control requestingControl = (Control)sender;
            String topic = helpProvider1.GetHelpKeyword(requestingControl);
            if (topic != null)
            {
                LoadHelpTopic(topic);
                ShowPanel(infoPanels, helpPanel);
                hlpevent.Handled = true;
            }
        }

        // Name: contentsToolStripMenuItem_Click
        //
        // Description:
        //      Event handler for the help content menu event.
        //
        // Inputs:
        //      sender  Menu.
        //      e       Event arguments.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void contentsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            String fullpath = System.IO.Path.GetFullPath(
                rm.GetString("HelpContents"));
            Help.ShowHelpIndex(departCountryComboBox,fullpath);
        }
    }
}