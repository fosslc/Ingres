// Copyright (c) 2006 Ingres Corporation

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

// Name: Routes.cs
//
// Description:
//      File extends the IngresFrequentFlyer class to provide the elements
//      for obtaining the route search criteria, retrieving data and updating
//      the route table.
//
// Public:
//
// Private:
//
// Methods:
//      setRouteControls
//
// History:
//      02-Oct-2006 (ray.fan@ingres.com)
//          Created.
//      11-Oct-2006 (ray.fan@ingres.com)
//          Add item count checks to combo boxes before trying to update
//          the selected index.
//

namespace IngresDemoApp
{
    public partial class IngresFrequentFlyer
    {
        // Combo box saves the current airport code combo for update from
        // the airport names results.
        ComboBox iataCodeComboBox;
        SearchParams lastQuery;

        // Name: RouteFormInitialize
        //
        // Description:
        //      Initializes the user interface control properties.
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
        public void RouteFormInitialize()
        {
            iataCodeComboBox = new ComboBox();
            flightDays = new char[7];
            routesFormPanel.Dock = DockStyle.Fill;
            routeTabControl.Dock = DockStyle.Fill;
            SetRouteControls();
        }

        // Name: SetRouteControls
        //
        // Description:
        //      Initializes the UI controls that make up the route form.
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
        private void SetRouteControls()
        {
            // Check that the combo box has items
            if (departCountryComboBox.Items.Count > 0)
            {
                departCountryComboBox.SelectedIndex = 0;
            }
            // Check that the combo box has items
            if (arriveCountryComboBox.Items.Count > 0)
            {
                arriveCountryComboBox.SelectedIndex = 0;
            }
            allFlightDayCheckBox.Checked = true;

            SetProfileInfo(profileEmail);
            ShowPanel(formsPanels, routesFormPanel);
            LoadHelpTopic("RouteHelp");
            ShowPanel(infoPanels, helpPanel);
        }

        // Name: ShowAirportNames
        //
        // Description:
        //      Populates a data grid view with results from a row producing
        //      procedure.
        //      Manually defines the columns for the data grid.
        //      Loads the result set.
        //      Enables the cell click event for the data grid.
        //
        // Inputs:
        //      direction           DEPART or ARRIVE
        //      area                Region or place selected.
        //      airportNameTable    Table to hold results.
        //
        // Outputs:
        //      airportNameTable    Updated with results from the query.
        //      CellClick           Event handler added.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void ShowAirportNames(int direction, String area,
            DataTable airportNameTable)
        {
            DataGridViewTextBoxColumn c1 = new DataGridViewTextBoxColumn();
            DataGridViewTextBoxColumn c3 = new DataGridViewTextBoxColumn();
            BindingSource airportNameBindingSource = new BindingSource();

            // Initialize dataGridView help for airport name.
            helpProvider1.SetHelpKeyword((Control)dataGridView1,
                "RouteAirportNameHelp");
            helpProvider1.SetHelpNavigator((Control)dataGridView1,
                HelpNavigator.Topic);

            dataGridView1.DataSource = airportNameBindingSource;
            airportNameBindingSource.DataSource = airportNameTable;
            dataGridView1.AutoGenerateColumns = false;
            dataGridView1.Dock = DockStyle.Fill;

            c1.DataPropertyName = rm.GetString("AirportNameCol1");
            c1.HeaderText = rm.GetString("IATAColumn");
            c1.Name = "IATAColumn";
            c1.ReadOnly = true;

            c3.DataPropertyName = rm.GetString("AirportNameCol2");
            c3.HeaderText = rm.GetString("AirportColumn");
            c3.Name = "AirportColumn";
            c3.ReadOnly = true;

            dataGridView1.Columns.Clear();
            dataGridView1.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            c1, c3});
            LoadAirportName(direction, area, airportNameTable);
            ShowPanel(infoPanels, resultPanel);
            dataGridView1.CellClick += dataGridView1_CellClick;
        }

        // Name: GetFlightDays
        //
        // Description:
        //      Extract the list of days from the flightListBox control
        //
        // Inputs:
        //      None.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private String GetFlightDays(ListBox listBox)
        {
            // Get the character wildcard meta character
            char[] day = rm.GetString("AnyDay").ToCharArray();

            if (allFlightDayCheckBox.Checked == true)
            {
                // Any day is selected set all days to be the
                // wild card.
                for (int i = 0; (i < flightDays.Length); i+=1)
                {
                    flightDays.SetValue(day[0], i);
                }
            }
            else
            {
                int count = 0;
                int val = 0;

                // Scan through the list of days in the listbox
                for (int i = 0; i < flightDayListBox.Items.Count; i += 1)
                {
                    Char[] work;

                    val = i + 1;

                    // Convert the value to a character
                    work = val.ToString().ToCharArray();

                    // If the day of the week is selected set the value into
                    // the flightDays array.
                    if ((flightDayListBox.SelectedIndices.Count > 0) &&
                        (i == flightDayListBox.SelectedIndices[count]))
                    {
                        flightDays.SetValue(work[0], i);

                        if (count < (flightDayListBox.SelectedIndices.Count - 1))
                        {
                            // Bump the indices index
                            count += 1;
                        }
                    }
                    else
                    {
                        // Day is not selected put a wild card character in.
                        flightDays.SetValue(day[0], i);
                    }
                }
            }
            return (new String(flightDays));
        }

        // Name: SetRouteResultDataGridView
        //
        // Description:
        //      Initialize the data grid view for the returned data set.
        //
        // Inputs:
        //      dataGridView    Data grid for initialization.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void SetRouteResultDataGridView(DataGridView dataGridView)
        {
            DataGridViewTextBoxColumn[] columns = new DataGridViewTextBoxColumn[9];
            // Table column names
            String[] attributes = new string[] {
                "rt_airline",
                "al_iatacode",
                "rt_flight_num",
                "rt_depart_from",
                "rt_arrive_to",
                "rt_depart_at",
                "rt_arrive_at",
                "rt_arrive_offset",
                "rt_flight_day"};

            // Data grid view column names
            String[] columnNames = new string[] {
                rm.GetString("AirlineColumn"),
                rm.GetString("IATAColumn"),
                rm.GetString("FlightNumberColumn"),
                rm.GetString("DepartAirportColumn"),
                rm.GetString("ArriveAirportColumn"),
                rm.GetString("DepartTimeColumn"),
                rm.GetString("ArriveTimeColumn"),
                rm.GetString("DayOffsetColumn"),
                rm.GetString("FlightDaysColumn")
                 };

            // Initialize dataGridView help for airport name.
            helpProvider1.SetHelpKeyword((Control)dataGridView1,
                "RouteResultHelp");
            helpProvider1.SetHelpNavigator((Control)dataGridView1,
                HelpNavigator.Topic);

            // Remove the cell enter event handler that has been set when
            // loading the airport names into the grid.
            dataGridView.CellClick -= dataGridView1_CellClick;

            // Set manual definition of columns which comes next.
            dataGridView.AutoGenerateColumns = false;

            // Set properties for editing data.
            dataGridView.EditMode = DataGridViewEditMode.EditOnKeystrokeOrF2;
            dataGridView.ShowEditingIcon = true;
            dataGridView.AllowUserToAddRows = true;
            dataGridView.AllowUserToDeleteRows = true;
            dataGridView.MultiSelect = false;
            dataGridView.ReadOnly = false;

            for (int i = 0; (i < columns.Length); i+=1)
            {
                DataGridViewTextBoxColumn column = new DataGridViewTextBoxColumn();

                column.DataPropertyName = attributes[i];
                column.HeaderText = columnNames[i];
                column.Name = attributes[i];
                if (attributes[i].Equals("al_iatacode"))
                {
                    column.ReadOnly = true;
                }
                else
                {
                    column.ReadOnly = false;
                }
                // If the column is one of the date/time columns
                if ((attributes[i].Equals("rt_depart_at")) ||
                    (attributes[i].Equals("rt_arrive_at")))
                {
                    // Set the display format for time only.
                    column.DefaultCellStyle.Format = "t";
                }
                columns[i] = column;
            }
            dataGridView.Columns.Clear();
            dataGridView.Columns.AddRange(columns);
        }

        // Name: RouteCheckItems
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
        private DialogResult RouteCheckItems()
        {
            DialogResult result = DialogResult.OK;

            if ((result == DialogResult.OK) && (departAirportComboBox.SelectedIndex <= 0))
            {
                if (departCountryComboBox.SelectedIndex <= 0)
                {
                    departCountryComboBox.Focus();
                    result = MessageBox.Show(rm.GetString("EmptyRouteDepartCountry"),
                            rm.GetString("RouteCaption"),
                            MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
                }
                else if (departRegionComboBox.SelectedIndex <= 0)
                {
                    departRegionComboBox.Focus();
                    result = MessageBox.Show(rm.GetString("EmptyRouteDepartRegion"),
                            rm.GetString("RouteCaption"),
                            MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
                }
                else
                {
                    departAirportComboBox.Focus();
                    result = MessageBox.Show(rm.GetString("EmptyRouteDepartAirport"),
                            rm.GetString("RouteCaption"),
                            MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
                }
            }
            if ((result == DialogResult.OK) && (arriveAirportComboBox.SelectedIndex <= 0))
            {
                if (arriveCountryComboBox.SelectedIndex <= 0)
                {
                    arriveCountryComboBox.Focus();
                    result = MessageBox.Show(rm.GetString("EmptyRouteArriveCountry"),
                            rm.GetString("RouteCaption"),
                            MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
                }
                else if (arriveRegionComboBox.SelectedIndex <= 0)
                {
                    arriveRegionComboBox.Focus();
                    result = MessageBox.Show(rm.GetString("EmptyRouteArriveRegion"),
                            rm.GetString("RouteCaption"),
                            MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
                }
                else
                {
                    arriveAirportComboBox.Focus();
                    result = MessageBox.Show(rm.GetString("EmptyRouteArriveAirport"),
                            rm.GetString("RouteCaption"),
                            MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
                }
            }
            if ((result == DialogResult.OK) && (allFlightDayCheckBox.Checked == false))
            {
                if (flightDayListBox.SelectedIndices.Count == 0)
                {
                    flightDayListBox.Focus();
                    result = MessageBox.Show(rm.GetString("EmptyRouteFlightDay"),
                            rm.GetString("RouteCaption"),
                            MessageBoxButtons.RetryCancel, MessageBoxIcon.Warning);
                }
            }
            return (result);
        }

        //
        // Event handlers.
        //

        // Name: countryComboBox_SelectedIndexChanged
        //
        // Description:
        //      This event causes the regionComboBox to be populated with
        //      results restricted by country.
        //      Cursor is updated during the loading of data.
        //
        // Inputs:
        //      sender          Form control object
        //      e               Event arguments
        //
        // Outputs:
        //      departRegionComboBox
        //      arriveRegionComboBox
        //      profileRegionComboBox
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void countryComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox comboBox = (ComboBox)sender;
            ComboBox regionComboBox = new ComboBox();
            ComboBox airportComboBox = new ComboBox();

            int direction = DEPART;
            RegionDataSet regionDataSet = new RegionDataSet();

            if (comboBox.SelectedValue != null)
            {
                if (sender == departCountryComboBox)
                {
                    departing.country = comboBox.SelectedValue.ToString();
                    departing.regionDataSet = regionDataSet;
                    departPlaceBindingSource.DataSource = regionDataSet;
                    departRegionComboBox.DataSource = departPlaceBindingSource;
                    regionComboBox = departRegionComboBox;
                    airportComboBox = departAirportComboBox;
                }
                else if (sender == arriveCountryComboBox)
                {
                    direction = ARRIVE;
                    arriving.country = comboBox.SelectedValue.ToString();
                    arriving.regionDataSet = regionDataSet;
                    arrivePlaceBindingSource.DataSource = regionDataSet;
                    arriveRegionComboBox.DataSource = arrivePlaceBindingSource;
                    regionComboBox = arriveRegionComboBox;
                    airportComboBox = arriveAirportComboBox;
                }
                else if (sender == profileCountryComboBox)
                {
                    profile.country = comboBox.SelectedValue.ToString();
                    profile.regionDataSet = regionDataSet;
                    profilePlaceBindingSource.DataSource = regionDataSet;
                    profileRegionComboBox.DataSource = profilePlaceBindingSource;
                    regionComboBox = profileRegionComboBox;
                    airportComboBox = profileAirportComboBox;
                }
                if ((comboBox.SelectedValue.ToString().Length > 0) &&
                    (comboBox.SelectedIndex > 0))
                {
                    this.Cursor = Cursors.WaitCursor;
                    LoadRegion(direction, comboBox.SelectedValue.ToString(), regionDataSet);
                    if (regionDataSet.Tables["airport"].Rows.Count > 0)
                    {
                        regionComboBox.Enabled = true;
                    }
                    else
                    {
                        DialogResult result = MessageBox.Show(rm.GetString("EmptyAirport"),
                            rm.GetString("EmptyResultsCaption"),
                            MessageBoxButtons.OK, MessageBoxIcon.Information);
                    }
                    this.Cursor = Cursors.Arrow;
                }
                else
                {
                    ComboBox[] comboBoxes = new ComboBox[] {
                            regionComboBox,
                            airportComboBox };
                    ClearComboBoxes(comboBoxes);
                    foreach (ComboBox combo in comboBoxes)
                    {
                        combo.Enabled = false;
                    }
                }
            }
        }

        // Name: regionComboBox_SelectedIndexChanged
        //
        // Description:
        //      This event causes the airportComboBox to be populated with
        //      results restricted by region.
        //      Cursor is updated during the loading of data.
        //
        // Inputs:
        //      sender          Form control object
        //      e               Event arguments
        //
        // Outputs:
        //      departAirportComboBox
        //      arriveAirportComboBox
        //      profileAirportComboBox
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void regionComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            ComboBox comboBox = (ComboBox)sender;
            AirportDataSet airportDataSet = new AirportDataSet();

            int direction = DEPART;
            if ((comboBox.SelectedValue != null) && (comboBox.SelectedIndex >= 0))
            {
                if (sender == departRegionComboBox)
                {
                    departing.icaocode = comboBox.SelectedValue.ToString();
                    departIatacodeBindingSource.DataSource = airportDataSet.Tables["airport"];
                    departAirportComboBox.DataSource = departIatacodeBindingSource;
                    iataCodeComboBox = departAirportComboBox;
                }
                if (sender == arriveRegionComboBox)
                {
                    direction = ARRIVE;
                    arriving.icaocode = comboBox.SelectedValue.ToString();
                    arriveIatacodeBindingSource.DataSource = airportDataSet.Tables["airport"];
                    arriveAirportComboBox.DataSource = arriveIatacodeBindingSource;
                    iataCodeComboBox = arriveAirportComboBox;
                }
                if (sender == profileRegionComboBox)
                {
                    profile.icaocode = comboBox.SelectedValue.ToString();
                    profileIatacodeBindingSource.DataSource = airportDataSet.Tables["airport"];
                    profileAirportComboBox.DataSource = profileIatacodeBindingSource;
                    iataCodeComboBox = profileAirportComboBox;
                }
                this.Cursor = Cursors.WaitCursor;
                LoadAirport(direction, comboBox.SelectedValue.ToString(),
                    airportDataSet.Tables["airport"]);
                if (airportDataSet.Tables["airport"].Rows.Count > 0)
                {
                    ShowAirportNames(direction, comboBox.SelectedValue.ToString(),
                        airportDataSet.Tables["airportName"]);
                }
                else
                {
                    DialogResult result = MessageBox.Show(rm.GetString("EmptyAirport"),
                        rm.GetString("EmptyResultsCaption"),
                        MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                iataCodeComboBox.Enabled = true;
                this.Cursor = Cursors.Arrow;
            }
        }

        // Name: allFlightDayCheckBox_CheckedChanged
        //
        // Description:
        //      This event controls the flightDayListBox.
        //
        // Inputs:
        //      sender          Form control object
        //      e               Event arguments
        //
        // Outputs:
        //      departAirportComboBox
        //      arriveAirportComboBox
        //      profileAirportComboBox
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void allFlightDayCheckBox_CheckedChanged(object sender, EventArgs e)
        {
            CheckBox checkBox = (CheckBox)sender;

            flightDayListBox.Enabled =
                (checkBox.CheckState == CheckState.Checked) ? false : true;
        }

        // Name: routeGoButton_Click
        //
        // Description:
        //
        // Inputs:
        //      sender          Form control object
        //      e               Event arguments
        //
        // Outputs:
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void routeGoButton_Click(object sender, EventArgs e)
        {
            DialogResult result;
            BindingSource routeBindingSource = new BindingSource();

            if ((result = RouteCheckItems()) == DialogResult.OK)
            {
                String flightMask;
                String departingFromAirport;
                String arrivingToAirport;

                // Extract the values from the form controls.
                departingFromAirport = departAirportComboBox.SelectedValue.ToString();
                arrivingToAirport = arriveAirportComboBox.SelectedValue.ToString();
                CheckState roundTrip = routeOptionCheckBox.CheckState;
                flightMask = GetFlightDays(flightDayListBox);

                // Setup of the data grid control with columns.
                SetRouteResultDataGridView(dataGridView1);

                // Bind the data grid view to the data table returned by the
                // query.
                dataGridView1.DataSource = routeBindingSource;
                routeBindingSource.DataSource = routeDataSet1;
                routeBindingSource.DataMember = "route";
                routeBindingSource.PositionChanged += routeBindingSource_PositionChanged;
                // Execute the query an populate the data set.
                RouteSearch(routeDataSet1, departingFromAirport,
                    arrivingToAirport, flightMask, roundTrip);
                if (routeDataSet1.Tables["route"].Rows.Count == 0)
                {
                    result = MessageBox.Show(rm.GetString("EmptyRouteSearch"),
                        rm.GetString("EmptyResultsCaption"),
                        MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                lastQuery = new SearchParams(departingFromAirport, arrivingToAirport, flightMask,
                    roundTrip);
                dataGridView1.CellMouseMove += dataGridView1_CellMouseMove;
                dataGridView1.UserDeletingRow += dataGridView1_UserDeletingRow;
            }
        }

        // Name: routeReload
        //
        // Description:
        //      Reload the data set bound to the data grid view.
        //
        // Inputs:
        //      sender          Form control object
        //      e               Event arguments
        //
        // Outputs:
        //      .DataSource     Updated with new result set.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void routeReload(object sender, EventArgs e)
        {
            Timer t = (Timer)sender;

            //  Disable the timer
            t.Enabled = false;

            // Reload the data set.
            BindingSource bs = (BindingSource)dataGridView1.DataSource;
            DataSet ds = (DataSet)bs.DataSource;

            // Rerun the last SELECT query.
            RouteSearch(ds, lastQuery.departingAirport,
                lastQuery.arrivingAirport, lastQuery.onDays,
                lastQuery.roundTrip);
        }

        // Name: routeNewButton_Click
        //
        // Description:
        //
        // Inputs:
        //      sender          Form control object
        //      e               Event arguments
        //
        // Outputs:
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void routeNewButton_Click(object sender, EventArgs e)
        {
            SetRouteControls();
        }

        // Name: dataGridView1_CellClick
        //
        // Description:
        //      Event handler for the DataGridView cell click event.
        //      This event is enabled to allow a selection in the
        //      airport list to updated the airport combo box with
        //      the equivalent value.
        //
        // Inputs:
        //      sender  DataGridView
        //      e       Event arguments.
        //
        // Outputs:
        //      iataCodeComboBox    Selected item updated.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void dataGridView1_CellClick(object sender, DataGridViewCellEventArgs e)
        {
            DataGridView dataGridView = (DataGridView)sender;
            if (dataGridView.Rows.Count > 0)
            {
                if ((iataCodeComboBox.Items.Count > 0) &&
                    (dataGridView.Rows.Count <= iataCodeComboBox.Items.Count) &&
                    (e.RowIndex < iataCodeComboBox.Items.Count))
                {
                    iataCodeComboBox.SelectedIndex = e.RowIndex + 1;
                }
            }
        }

        // Name: dataGridView1_CellMouseMove
        //
        // Description:
        //      Event handler for the DataGridView cell mouse move event.
        //      This event is enabled for the route result set to provide
        //      airline name tooltips when hovering over the airline fields
        //      in the result set.
        //
        // Inputs:
        //      sender  DatGridView
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
        void dataGridView1_CellMouseMove(object sender, DataGridViewCellMouseEventArgs e)
        {
            DataGridView dataGridView = (DataGridView)sender;
            if ((dataGridView != null) && (e.RowIndex >= 0) &&
                (e.RowIndex < (dataGridView.Rows.Count - 1)))
            {
                if (dataGridView.IsCurrentCellInEditMode == false)
                {
                    if (e.RowIndex < routeDataSet1.Tables["route"].Rows.Count)
                    {
                        String airline;

                        if (routeDataSet1.Tables["route"].Rows[e.RowIndex].ItemArray[9] != null)
                        {
                            airline =
                                routeDataSet1.Tables["route"].Rows[e.RowIndex].ItemArray[9].ToString();
                            dataGridView.Rows[e.RowIndex].Cells[0].ToolTipText = airline;
                            dataGridView.Rows[e.RowIndex].Cells[1].ToolTipText = airline;
                        }
                    }
                }
            }
        }

        // Name: dataGridView1_DataSourceChanged
        //
        // Description:
        //      Event handler for the DataGridView data source changed event.
        //      The DataGridView object is shared between the airport names
        //      list and the route result set.
        //      This event is enabled so that cell and mouse events are
        //      diabled when the data source changes.
        //      The event will be re-enabled where the data source is changed.
        //
        // Inputs:
        //      sender  DataGridView
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
        private void dataGridView1_DataSourceChanged(object sender, EventArgs e)
        {
            DataGridView dataGridView = (DataGridView)sender;

            // Disable handling of cell events when the datasource changes.
            dataGridView.CellClick -= dataGridView1_CellClick;
            dataGridView.CellMouseMove -= dataGridView1_CellMouseMove;
            dataGridView1.UserDeletingRow -= dataGridView1_UserDeletingRow;
        }

        // Name: dataGridView1_UserDeletingRow
        //
        // Description:
        //      Event handler for the DataGridView user deleteing row event.
        //      When the row is selected pressing the Delete key will attempt
        //      to delete the row from the DataGridView.
        //      The row is deleted from the data table.  If the delete is not
        //      successful the cancel state is set for the event.
        //
        // Inputs:
        //      sender  DataGridView
        //      e       Event arguments.
        //
        // Outputs:
        //      Cancel  If the delete is not successful the cancel state
        //              is set to true.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void dataGridView1_UserDeletingRow(object sender, DataGridViewRowCancelEventArgs e)
        {
            System.Diagnostics.Debug.WriteLine(dataGridView1.DataSource.ToString() +
                " User deleting row " + e.Row.ToString(), "Event");
            if (RouteDelete(e.Row) == 0)
            {
                e.Cancel = true;
            }
            else
            {
                // Disable hover over tool tips
                dataGridView1.CellMouseMove -= dataGridView1_CellMouseMove;
                LastDataRow = null;
            }
        }

        // Name: routeBindingSource_PositionChanged
        //
        // Description:
        //      Event handler for the BindingSource position changed event.
        //      The event will trigger for each position change within the 
        //      DataGridView.  Maintain the previous position and use the
        //      reposition event to trigger the action.
        //      Test the RowState and perform the appropriate query.
        //
        // Inputs:
        //      sender  BindingSource
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
        private void routeBindingSource_PositionChanged(object sender, EventArgs e)
        {
            BindingSource source = (BindingSource)sender;
            DataRow ThisDataRow;

            if (source.Current != null)
            {
                // Get the current row.  The row where the position has
                // changed to.
                ThisDataRow = ((DataRowView)source.Current).Row;

                // Check that the position has changed to a different row.
                // Don't want duplicate changes with position changes within 
                // the same row.
                if (ThisDataRow != LastDataRow)
                {
                    if (LastDataRow != null)
                    {
                        // Check the RowState for the row from where the
                        // position has changed.

                        // A new row is added.
                        if (LastDataRow.RowState == DataRowState.Added)
                        {
                            // Clone the table structure.
                            DataTable dataTable = routeDataSet1.Tables["updateRoute"].Clone();

                            // Create a new row.
                            DataRow dataRow = dataTable.NewRow();
                            for (int i = 0, j = 0;
                                (i < LastDataRow.ItemArray.Length); i += 1)
                            {
                                // Populate the update row with values from
                                // matching columns.
                                if ((j < dataRow.ItemArray.Length) &&
                                    (dataRow.Table.Columns[j].ColumnName ==
                                    LastDataRow.Table.Columns[i].ColumnName))
                                {
                                    // Save the query parameters to re-issue
                                    // the query.
                                    if (dataRow.Table.Columns[j].ColumnName.Equals("rt_depart_from"))
                                    {
                                        lastQuery.departingAirport =
                                            LastDataRow.ItemArray[i].ToString();
                                    }
                                    if (dataRow.Table.Columns[j].ColumnName.Equals("rt_arrive_to"))
                                    {
                                        lastQuery.arrivingAirport =
                                            LastDataRow.ItemArray[i].ToString();
                                    }
                                    if (dataRow.Table.Columns[j].ColumnName.Equals("rt_flight_day"))
                                    {
                                        lastQuery.onDays =
                                            LastDataRow.ItemArray[i].ToString();
                                    }
                                    // Copy the value in the equivalent column.
                                    dataRow[j] = LastDataRow.ItemArray[i].ToString();
                                    j += 1;
                                }
                            }
                            // Add the row to the table.  Could add more than
                            // one here and add them all at once or used the
                            // rows rather than the table.
                            dataTable.Rows.Add(dataRow);
                            RouteAdd(dataTable);
                        }
                        else if (LastDataRow.RowState == DataRowState.Modified)
                        {
                            // A row has been modified.
                            // Update the underlying data set.
                            RouteChange(routeDataSet1);
                        }
                    }
                    LastDataRow = ThisDataRow;
                }
            }
        }
    }

    // Name: SearchParams
    //
    // Description:
    //      Structure to hold search parameters
    //
    // Public members:
    //
    // Private members:
    //      departingAirport
    //      arrivingAirport
    //      onDays
    //      roundTrip
    //
    //  History:
    //      02-Oct-2006 (ray.fan@ingres.com)
    //          Created.
    public struct SearchParams
    {
        public String departingAirport;
        public String arrivingAirport;
        public String onDays;
        public CheckState roundTrip;

        public SearchParams(String DepartingAirport, String ArrivingAirport,
            String OnDays, CheckState RoundTrip)
        {
            departingAirport = DepartingAirport;
            arrivingAirport = ArrivingAirport;
            onDays = OnDays;
            roundTrip = RoundTrip;
        }
    }
}
