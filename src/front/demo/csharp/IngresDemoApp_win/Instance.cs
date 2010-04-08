// Copyright (c) 2006 Ingres Corporation

// Name: Instance.cs
//
// Description:
//      File extends the IngresFrequentFlyer class to provide the interface
//      for the instance functions.
//
// Public:
//
// Private:
//
// History:
//      02-Oct-2006 (ray.fan@ingres.com)
//          Created.
//      02-Feb-2006 (ray.fan@ingres.com)
//          Bug 117620
//          Centralize version specification.


using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace IngresDemoApp
{
    public partial class IngresFrequentFlyer
    {
        public Button[] instanceButtons;

        // Name: InstanceFormInitialize
        //
        // Description:
        //      Initializes variables at runtime and sets default properties
        //      of the instance panel and the instance tab control.
        //      Then calls the SetInstanceControls to set values into the
        //      instance form controls.
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
        public void InstanceFormInitialize()
        {
            instanceButtons = new Button[] { changeButton, resetButton };
            connectFormPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            connectTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            SetInstanceControls();
        }

        // Name: SetInstanceControls
        //
        // Description:
        //      Initializes the controls on all the panels used for
        //      the instance function.
        //      All controls default source and defined source buttons are
        //      disabled by default.
        //      Radio button choice is honoured on subsequent calls.
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
        private void SetInstanceControls()
        {
            hostComboBox.DropDownStyle = ComboBoxStyle.Simple;
            databaseComboBox.DropDownStyle = ComboBoxStyle.Simple;
            if (defaultSourceRadioButton.Checked == true)
            {
                ShowGroupBoxes(instanceGroupBoxes, new GroupBox[] { });
                ShowButtons(instanceButtons, new Button[] { });
            }
            else if (definedSourceRadioButton.Checked == true)
            {
                ShowGroupBoxes(instanceGroupBoxes,
                    new GroupBox[] { defineSourceGroupBox });
                ShowButtons(instanceButtons,
                    new Button[] { changeButton, resetButton });
            }
            hostComboBox.Text = currentSource.host;
            instanceComboBox.Text = currentSource.port.Remove(2);
            useridTextBox.Text = currentSource.user;
            passwordTextBox.Text = currentSource.pwd;
            databaseComboBox.Text = currentSource.db;
            connectDefaultCheckBox.Checked = false;
            ShowPanel(formsPanels, connectFormPanel);
            LoadHelpTopic("ConnectHelp");
            ShowPanel(infoPanels, helpPanel);
        }

        // Name: InstanceCheckItems
        //
        // Description:
        //      Tests each form entry field for a value.
        //      If a field contains no value, focus is set to the
        //      offending control and a message specifying an error
        //      is displayed.
        //
        // Inputs:
        //      None.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      DialogResult.OK
        //      DialogResult.Retry
        //      DialogResult.Cancel
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private DialogResult InstanceCheckItems()
        {
            DialogResult result = DialogResult.OK;
            if ((result == DialogResult.OK) && (hostComboBox.Text.Length == 0))
            {
                hostComboBox.Focus();
                result = MessageBox.Show(rm.GetString("EmptyHost"),
                        rm.GetString("DatasourceCaption"),
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
            }
            if ((result == DialogResult.OK) &&
                (instanceComboBox.Text.Length == 0))
            {
                instanceComboBox.Focus();
                result = MessageBox.Show(rm.GetString("EmptyInstance"),
                        rm.GetString("DatasourceCaption"),
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
            }
            if ((result == DialogResult.OK) &&
                (databaseComboBox.Text.Length == 0))
            {
                databaseComboBox.Focus();
                result = MessageBox.Show(rm.GetString("EmptyDatabase"),
                        rm.GetString("DatasourceCaption"),
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
            }
            return (result);
        }

        //
        // Event handlers
        //

        // Name: dataSourceRadioButton_CheckedChanged
        //
        // Description:
        //      Detect and react to the change in state of the data source
        //      radio buttons.
        //
        // Inputs:
        //      sender      Radio button.
        //      e           event arguments.
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
        private void dataSourceRadioButton_CheckedChanged(object sender, EventArgs e)
        {
            if (sender == definedSourceRadioButton)
            {
                ShowGroupBoxes(instanceGroupBoxes,
                    new GroupBox[] { defineSourceGroupBox });
                ShowButtons(instanceButtons,
                    new Button[] { changeButton, resetButton });
            }
            if (sender == defaultSourceRadioButton)
            {
                ShowGroupBoxes(instanceGroupBoxes,
                    new GroupBox[] { });
                ShowButtons(instanceButtons,
                    new Button[] {});
            }
        }

        // Name: changeButton_Click
        //
        // Description:
        //      Check that each field in the form has a value.
        //      Extract the values and save the current connection
        //      information.
        //      Set the connection string in the ingresConnection1 control.
        //      If the default check box is checked write the configuration
        //      into the configuration file.
        //
        // Inputs:
        //      sender      changeButton.
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
        //      02-Feb-2006 (ray.fan@ingres.com)
        //          Removed specifying of version in the call to CheckVersion.
        private void changeButton_Click(object sender, EventArgs e)
        {
            TargetDetails newSource = new TargetDetails();

            if (InstanceCheckItems() == DialogResult.OK)
            {
                if (hostComboBox.SelectedValue != null)
                {
                    newSource.host = hostComboBox.SelectedValue.ToString();
                }
                else
                {
                    newSource.host = hostComboBox.Text.ToString();
                }
                if (instanceComboBox.SelectedValue != null)
                {
                    newSource.port = String.Format(rm.GetString("PortAddress"),
                        instanceComboBox.SelectedValue.ToString());
                }
                else
                {
                    newSource.port = String.Format(rm.GetString("PortAddress"),
                        instanceComboBox.Text.ToString());
                }
                if (useridTextBox != null)
                {
                    newSource.user = useridTextBox.Text;
                }
                if (passwordTextBox != null)
                {
                    newSource.pwd = passwordTextBox.Text;
                }
                if (databaseComboBox.SelectedValue != null)
                {
                    newSource.db = databaseComboBox.SelectedValue.ToString();
                }
                else
                {
                    newSource.db = databaseComboBox.Text.ToString();
                }
                savedSource = currentSource;
                currentSource = newSource;
                SetConnection(ingresConnection1,
                    currentSource.GetConnectionString());
                if (connectDefaultCheckBox.Checked == true)
                {
                    WriteConfiguration(appConfigFile);
                    defaultSourceRadioButton.Checked = true;
                }
                try
                {
                    if (CheckVersion() == true)
                    {
                        SetInstanceControls();
                        SetStatus(currentSource);
                        SetStaticControls();
                    }
                    else
                    {
                        DialogResult result = MessageBox.Show(rm.GetString("ErrorVersion"),
                            rm.GetString("InstanceCaption"),
                            MessageBoxButtons.RetryCancel, MessageBoxIcon.Error);
                        if (result != DialogResult.Retry)
                        {
                            currentSource = savedSource;
                        }
                    }
                }
                catch
                {
                    // Data adapter initialization error.
                    InstanceFormInitialize();
                    LoadHelpTopic("ConnectHelp");
                    ShowPanel(infoPanels, helpPanel);
                }
            }
        }

        // Name: resetButton_Click
        //
        // Description:
        //      Reset the values in the instance form to the last saved
        //      version and re-initialize all form controls depending
        //      on the instance connection.
        //
        // Inputs:
        //      sender  resetButton.
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
        private void resetButton_Click(object sender, EventArgs e)
        {
            currentSource = savedSource;
            SetInstanceControls();
            SetStatus(currentSource);
            SetStaticControls();
        }
    }
}
