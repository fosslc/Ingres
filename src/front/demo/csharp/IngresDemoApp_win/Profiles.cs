// Copyright (c) 2006 Ingres Corporation

// Name: Profile.cs
//
// Description:
//      File extends the IngresFrequentFlyer class to provide the interface
//      for the profile functions.
//
// Public:
//
// Private:
//
// History:
//      02-Oct-2006 (ray.fan@ingres.com)
//          Created.
//      11-Oct-2006 (ray.fan@ingres.com)
//          Add item count checks to combo boxes before trying to update
//          the selected index.
//          Check that the up_image is not null before trying to assign into
//          a PictureBox control.

using System;
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace IngresDemoApp
{
    public partial class IngresFrequentFlyer
    {
        Button[] profileButtons;

        // Name: ProfileFormInitialize
        //
        // Description:
        //      Initializes variables at runtime and sets default properties
        //      of the forms panel and the profile tab control.
        //      Then calls the SetProfileControls to set values into the
        //      profile form controls.
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
        public void ProfileFormInitialize()
        {
            profileButtons = new Button[] {
                profileNewButton,
                profileModifyButton,
                profileAddButton,
                profileRemoveButton,
                profileChangeButton };
            profilesFormPanel.Dock = System.Windows.Forms.DockStyle.Fill;
            profileTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
            SetProfileControls();
        }

        // Name: SetProfileControls
        //
        // Description:
        //      Initializes the controls on all the panels used for
        //      the personal profile function.
        //      All controls except the New and Modify buttons are disabled
        //      by default.
        //      If no default profile has been configured the help panel is
        //      displayed instead of the profile information.
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
        public void SetProfileControls()
        {
            ShowGroupBoxes(profileGroupBoxes, new GroupBox[] { });
            profileDefaultCheckBox.Enabled = false;
            SetProfileInfo(profileEmail);
            ShowButtons(profileButtons,
                new Button[] { profileNewButton, profileModifyButton });
            ShowPanel(formsPanels, profilesFormPanel);
            if ((profileEmail != null) && (profileEmail.Length > 0))
            {
                ShowPanel(infoPanels, profilePanel);
            }
            else
            {
                LoadHelpTopic("ProfileHelp");
                ShowPanel(infoPanels, helpPanel);
            }
        }

        // Name: SetProfileInfo
        //
        // Description:
        //      Sets the profile controls specified by the email address.
        //      The email address is defined as a key and should only return
        //      one row.
        //
        //      The prefered departure airport is used to set the default
        //      departing airport controls and the profile airport controls.
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
        private void SetProfileInfo(String email)
        {
            if ((email != null) && (email.Length > 0))
            {
                ProfileDataSet profileDataSet = new ProfileDataSet();

                // Set the check box if the specified email is the same as
                // the default email address.
                profileDefaultCheckBox.Checked =
                    (email.CompareTo(profileEmail) == 0) ? true : false;
                // Retrieve the data for the specified profile.
                LoadProfile(profileDataSet.Tables["user_profile"], email);

                if (profileDataSet.Tables["user_profile"].Rows.Count > 0)
                {
                    DataRow resultRow;
                    String lastName;
                    String firstName;
                    String IATAcode;
                    Image photo;                    

                    MemoryStream memoryStream;

                    // Take the first row to be the result row.
                    // There should only be one row.
                    resultRow = profileDataSet.Tables["user_profile"].Rows[0];
                    lastName = (string)
                        resultRow[resultRow.Table.Columns["up_last"].Ordinal];
                    firstName = (string)
                        resultRow[resultRow.Table.Columns["up_first"].Ordinal];
                    IATAcode = (string)
                        resultRow[resultRow.Table.Columns["up_airport"].Ordinal];

                    // Convert the BLOB into a memory stream object.
                    // If the column contains a null value don't try to load
                    // it into the PictureBox.
                    if (resultRow[resultRow.Table.Columns["up_image"].Ordinal]
                        != System.DBNull.Value)
                    {
                        memoryStream =
                            new MemoryStream(
                                (Byte[])resultRow.ItemArray[resultRow.Table.Columns["up_image"].Ordinal]
                                );

                        // Read the BLOB stream into an image object.
                        photo = Image.FromStream(memoryStream);

                        // Set the image into an image box.
                        profilePictureBox.Image = photo;
                    }
                    else
                    {
                        profilePictureBox.Image = profilePictureBox.ErrorImage;
                    }

                    // Set the combo box selected item or the text.
                    if (profileEmailComboBox.Items.Count > 0)
                    {
                        profileEmailComboBox.SelectedIndex =
                            profileEmailComboBox.FindString(email);
                    }
                    else
                    {
                        profileEmailComboBox.Text = email;
                    }

                    profileLastNameTextBox.Text = lastName;
                    profileFirstNameTextBox.Text = firstName;
                    welcomeLabel.Text = String.Format(
                        rm.GetString("ProfileWelcome"), firstName);
                    emailAddressLabel.Text = email;
                    airportPreferenceLabel.Text = IATAcode;
                    SetAirportControls(
                        new ComboBox[] { profileCountryComboBox, profileRegionComboBox, profileAirportComboBox },
                        IATAcode);

                    SetAirportControls(
                        new ComboBox[] { departCountryComboBox, departRegionComboBox, departAirportComboBox },
                        IATAcode);
                }
            }
        }

        // Name: ProfileCheckItems
        //
        // Description:
        //      Checks each form entry field for a value.
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
        //      result  DialogResult.OK
        //              DialogResult.Retry
        //              DialogResult.Cancel
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private DialogResult ProfileCheckItems()
        {
            DialogResult result = DialogResult.OK;

            if ((result == DialogResult.OK) && (profileEmailComboBox.Text.Length == 0))
            {
                profileEmailComboBox.Focus();
                result = MessageBox.Show(rm.GetString("EmptyEmail"),
                        rm.GetString("ProfileCaption"),
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
            }
            if ((result == DialogResult.OK) && (profileLastNameTextBox.Text.Length == 0))
            {
                profileLastNameTextBox.Focus();
                result = MessageBox.Show(rm.GetString("EmptyLastName"),
                        rm.GetString("ProfileCaption"),
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
            }
            if ((result == DialogResult.OK) && (profileFirstNameTextBox.Text.Length == 0))
            {
                profileFirstNameTextBox.Focus();
                result = MessageBox.Show(rm.GetString("EmptyFirstName"),
                        rm.GetString("ProfileCaption"),
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
            }
            if ((result == DialogResult.OK) && (profileAirportComboBox.SelectedIndex < 0))
            {
                profileAirportComboBox.Focus();
                result = MessageBox.Show(rm.GetString("EmptyProfileAirport"),
                        rm.GetString("ProfileCaption"),
                        MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
            }
            return (result);
        }

        //
        // Event handlers
        //

        // Name: profileCountryComboBox_SelectedIndexChanged
        //
        // Description:
        //      Passes the event to countryComboBox_SelectedIndexChanged.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profileCountryComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            countryComboBox_SelectedIndexChanged(sender, e);
        }

        // Name: profileRegionComboBox_SelectedIndexChanged
        //
        // Description:
        //      Passes the event to regionComboBox_SelectedIndexChanged.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profileRegionComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            regionComboBox_SelectedIndexChanged(sender, e);
        }

        // Name: profileNewButton_Click
        //
        // Description:
        //      Handles the New button click event.
        //      Activates all fields and clears all form entries.
        //      Loads the profile help topic.
        //      Enables the help panel.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profileNewButton_Click(object sender, EventArgs e)
        {
            // New button click event.
            // Clear all the profile controls.

            // Check each combo box in the list of combo boxes.
            // Determine the type of the data source and clear the result
            // set.
            ComboBox[] comboBoxes = new ComboBox[] {
                    profileEmailComboBox,
                    profileRegionComboBox, 
                    profileAirportComboBox };

            ClearComboBoxes(comboBoxes);

            profileEmailComboBox.DropDownStyle = ComboBoxStyle.Simple;
            profileLastNameTextBox.Clear();
            profileFirstNameTextBox.Clear();
            profilePhotoTextBox.Clear();
            if (profileCountryComboBox.Items.Count > 0)
            {
                profileCountryComboBox.SelectedIndex = 0;
            }
            ShowGroupBoxes(profileGroupBoxes, new GroupBox[] {
                profilePersonalGroupBox,
                profilePreferenceGroupBox,
                profileAirportGroupBox});
            profileDefaultCheckBox.Enabled = true;
            profileDefaultCheckBox.Checked = false;
            ShowButtons(profileButtons, new Button[] { profileNewButton,
                profileModifyButton, profileAddButton });

            // Load and show the help panel.
            LoadHelpTopic("ProfileHelp");
            ShowPanel(infoPanels, helpPanel);
        }

        // Name: profileModifyButton_Click
        //
        // Description:
        //      Handles the Modify button click event.
        //      Activates all fields and populates the Email combo box list.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profileModifyButton_Click(object sender, EventArgs e)
        {
            // Modfy button click event.
            // Populate the email combo box list and convert from a text box.

            ProfileDataSet profileDataSet = new ProfileDataSet();
            BindingSource profileBindingSource = new BindingSource();
            String email;

            email = profileEmailComboBox.Text;
            profileBindingSource.DataSource = profileDataSet;
            profileBindingSource.DataMember = "profileList";
            profileEmailComboBox.DataSource = profileBindingSource;

            profileEmailComboBox.DropDownStyle = ComboBoxStyle.DropDownList;
            profilePhotoTextBox.Clear();
            GetProfileList(profileDataSet.Tables["profileList"]);
            // Check that we have entries in the data set that populates
            // combo box.
            if (profileDataSet.Tables["profileList"].Rows.Count > 0)
            {
                // Temporaily remove the event handler for the
                // SelectedIndexChanged event.  Stops it from triggering while
                // combo box is updated.
                profileEmailComboBox.SelectedIndexChanged -=
                    profileEmailComboBox_SelectedIndexChanged;
                profileEmailComboBox.SelectedIndex =
                    profileEmailComboBox.FindString(email);
                // Reinstate the event handler for the
                // SelectedIndexChanged event.
                profileEmailComboBox.SelectedIndexChanged +=
                    profileEmailComboBox_SelectedIndexChanged;
                ShowGroupBoxes(profileGroupBoxes, new GroupBox[] {
                profilePersonalGroupBox,
                profilePreferenceGroupBox,
                profileAirportGroupBox});
                profileDefaultCheckBox.Enabled = true;
                ShowButtons(profileButtons, new Button[] { profileNewButton,
                profileModifyButton, profileRemoveButton,
                profileChangeButton });
            }
            else
            {
                // Alert for no results.
                DialogResult result = MessageBox.Show(rm.GetString("EmptyProfileList"),
                    rm.GetString("EmptyResultsCaption"),
                    MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
        }

        // Name: profileEmailComboBox_SelectedIndexChanged
        //
        // Description:
        //      Handles a changed email selection.
        //      Retrieves information for the specified email selection.
        //      Updates all forms fields.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profileEmailComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            // Email combo box selected index changed event.
            // Set the profile form controls according to the specified email
            // address.
            ComboBox comboBox = (ComboBox)sender;
            ProfileDataSet profileDataSet = new ProfileDataSet();

            if (comboBox.SelectedValue != null)
            {
                String email = comboBox.SelectedValue.ToString();

                // Set the default check box if the default profile email
                // address is the same as the selected email address.
                profileDefaultCheckBox.Checked =
                    (email.CompareTo(profileEmail) == 0) ? true : false;

                LoadProfile(profileDataSet.Tables["user_profile"], email);
                if (profileDataSet.Tables["user_profile"].Rows.Count > 0)
                {
                    DataRow resultRow;
                    String lastName;
                    String firstName;
                    String IATAcode;
                    Image photo;

                    MemoryStream memoryStream;

                    // Take the first row to be the result row.
                    // There should only be one row.
                    resultRow = profileDataSet.Tables["user_profile"].Rows[0];
                    lastName = (string)
                        resultRow[resultRow.Table.Columns["up_last"].Ordinal];
                    firstName = (string)
                        resultRow[resultRow.Table.Columns["up_first"].Ordinal];
                    IATAcode = (string)
                        resultRow[resultRow.Table.Columns["up_airport"].Ordinal];
                    // If the column contains a null value don't try to load
                    // it into the PictureBox.
                    if (resultRow[resultRow.Table.Columns["up_image"].Ordinal]
                        != System.DBNull.Value)
                    {
                        memoryStream =
                            new MemoryStream(
                                (byte[])resultRow[resultRow.Table.Columns["up_image"].Ordinal]
                                );
                        photo = Image.FromStream(memoryStream);
                        profilePictureBox.Image = photo;
                    }
                    else
                    {
                        profilePictureBox.Image = profilePictureBox.ErrorImage;
                    }
                    // Check the items in the combo box list and set its value
                    // if there are entries in it.
                    if (profileEmailComboBox.Items.Count > 0)
                    {
                        profileEmailComboBox.SelectedIndex =
                            profileEmailComboBox.FindString(email);
                    }
                    else
                    {
                        profileEmailComboBox.Text = email;
                    }

                    profileLastNameTextBox.Text = lastName;
                    profileFirstNameTextBox.Text = firstName;
                    welcomeLabel.Text = String.Format(
                        rm.GetString("ProfileWelcome"), firstName);
                    emailAddressLabel.Text = email;
                    airportPreferenceLabel.Text = IATAcode;
                    SetAirportControls(
                        new ComboBox[] { profileCountryComboBox, profileRegionComboBox, profileAirportComboBox },
                        IATAcode);
                    ShowPanel(infoPanels, profilePanel);
                }
            }
        }

        // Name: profilePhotoPathButton_Click
        //
        // Description:
        //      Handles the Browse button click event.
        //      Displays a file open dialog and updates the path text box.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profilePhotoPathButton_Click(object sender, EventArgs e)
        {
            // Photo button path click event.
            // Display the open file dialog.
            openFileDialog.Title = rm.GetString("openphoto");
            openFileDialog.Filter = "JPEG|*.jpg|GIF|*.gif|PNG|*.png|BMP|*.bmp|All|*.*";
            openFileDialog.FilterIndex = 1;
            openFileDialog.FileName = "";
            if (openFileDialog.ShowDialog() != DialogResult.Cancel)
            {
                profilePhotoTextBox.Text = openFileDialog.FileName;
            }
            Directory.SetCurrentDirectory(currentDirectory);
        }

        // Name: profileAddButton_Click
        //
        // Description:
        //      Handles the Add button click event.
        //      Extracts the values from the forms fields and creates a
        //      data set into which a new row will be inserted.
        //      If a new row is inserted the profile panel fields are updated.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profileAddButton_Click(object sender, EventArgs e)
        {
            // Add button click event.
            // Take the values from the profile forms and insert them into
            // the user_profile table.

            DialogResult result;
            if ((result = ProfileCheckItems()) == DialogResult.OK)
            {
                ProfileDataSet profileDataSet = new ProfileDataSet();

                String email;
                String lastName;
                String firstName;
                String airport;
                String photoPath;

                email = (profileEmailComboBox.Items.Count > 0) ?
                    profileEmailComboBox.SelectedValue.ToString() :
                    profileEmailComboBox.Text;
                airport = (profileAirportComboBox.Items.Count > 0) ?
                    profileAirportComboBox.SelectedValue.ToString() :
                    profileAirportComboBox.Text;

                lastName = profileLastNameTextBox.Text;
                firstName = profileFirstNameTextBox.Text;
                photoPath = profilePhotoTextBox.Text;

                LoadProfile(profileDataSet.Tables["user_profile"], email);

                if (ProfileAdd(profileDataSet,
                    lastName,
                    firstName,
                    email,
                    airport,
                    photoPath) > 0)
                {
                    ShowButtons(profileButtons,
                        new Button[] {
                            profileModifyButton, 
                            profileNewButton,
                            profileRemoveButton,
                            profileChangeButton });
                    if (profileDefaultCheckBox.Checked == true)
                    {
                        profileEmail = email;
                        WriteConfiguration(appConfigFile);
                    }
                    SetProfileInfo(email);
                    ShowPanel(infoPanels, profilePanel);
                }
            }
            else if (result == DialogResult.Cancel)
            {
                Exception ex = new Exception("Add aborted");
                throw ex;
            }
        }

        // Name: profileRemoveButton_Click
        //
        // Description:
        //      Handles the Remove button click event.
        //      Extracts the selected email value and deletes the row identified
        //      by it.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profileRemoveButton_Click(object sender, EventArgs e)
        {
            // Remove button click event.
            // Take the email address and delete the row identified.

            String email;
            ProfileDataSet profileDataSet = new ProfileDataSet();

            email = (profileEmailComboBox.Items.Count > 0) ?
                profileEmailComboBox.SelectedValue.ToString() :
                profileEmailComboBox.Text;

            if (email != null)
            {
                if (ProfileRemove(profileDataSet, email) > 0)
                {
                    if (profileEmail == email)
                    {
                        profileEmail = null;
                        profileDefaultCheckBox.Checked = false;
                    }
                    // Check each combo box in the list of combo boxes.
                    // Determine the type of the data source and clear the result
                    // set.
                    ComboBox[] comboBoxes = new ComboBox[] {
                    profileEmailComboBox,
                    profileRegionComboBox, 
                    profileAirportComboBox };

                    ClearComboBoxes(comboBoxes);

                    profileLastNameTextBox.Clear();
                    profileFirstNameTextBox.Clear();
                    if (profileCountryComboBox.Items.Count > 0)
                    {
                        profileCountryComboBox.SelectedIndex = 0;
                    }
                    SetProfileControls();
                }
            }
        }

        // Name: profileChangeButton_Click
        //
        // Description:
        //      Handles the Change button click event.
        //      Updates the row identified by the email address with the
        //      values from the form.
        //      All values, except the photo, are updated even if unchanged.
        //
        // Inputs:
        //      sender      Control object
        //      e           Event arguments
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
        private void profileChangeButton_Click(object sender, EventArgs e)
        {
            // Change button click event.
            // Update the values in the table for the specified email
            // address.
            DialogResult result;

            if ((result = ProfileCheckItems()) == DialogResult.OK)
            {
                ProfileDataSet profileDataSet = new ProfileDataSet();

                String email;
                String lastName;
                String firstName;
                String airport;
                String photoPath;

                email = (profileEmailComboBox.Items.Count > 0) ?
                    profileEmailComboBox.SelectedValue.ToString() :
                    profileEmailComboBox.Text;
                airport = (profileAirportComboBox.Items.Count > 0) ?
                    profileAirportComboBox.SelectedValue.ToString() :
                    profileAirportComboBox.Text;

                lastName = profileLastNameTextBox.Text;
                firstName = profileFirstNameTextBox.Text;
                photoPath = profilePhotoTextBox.Text;

                LoadProfile(profileDataSet.Tables["user_profile"], email);

                if (ProfileChange(profileDataSet,
                    lastName,
                    firstName,
                    email,
                    airport,
                    photoPath) > 0)
                {
                    ShowButtons(profileButtons,
                        new Button[] {
                            profileModifyButton, 
                            profileNewButton,
                            profileRemoveButton,
                            profileChangeButton });
                    if (profileDefaultCheckBox.Checked == true)
                    {
                        profileEmail = email;
                        WriteConfiguration(appConfigFile);
                    }
                    SetProfileInfo(email);
                    ShowPanel(infoPanels, profilePanel);
                }
            }
            else if (result == DialogResult.Cancel)
            {
                Exception ex = new Exception("Change aborted");
                throw ex;
            }
        }
    }
}
