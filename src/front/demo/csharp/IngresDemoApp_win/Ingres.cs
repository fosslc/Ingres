// Copyright (c) 2006, 2009 Ingres Corporation

// Name: Ingres.cs
//
// Description:
//      File extends the IngresFrequentFlyer class to provide the interface
//      to the Ingres Data Provider.
//
// Public:
//
// Private:
//
// History:
//      02-Oct-2006 (ray.fan@ingres.com)
//          Created.
//      11-Oct-2006 (ray.fan@ingres.com)
//          Add procedure string comment to match help and correct typos.
//      13-Oct-2006 (ray.fan@ingres.com)
//          Add a start row to results sets bound to combo boxes.
//      02-Feb-2006 (ray.fan@ingres.com)
//          Bug 117620
//          Update the DisplayError function to display a user message for
//          connection errors (SQLstate 08001).
//          Centralize version specification by declaring a parameterless
//          method that uses defined constants in preparation for the
//          introduction schema changes.
//      28-Oct-2009 (david.thole@ingres.com)
//          Bug 122812
//          Ensure that connection is closed should an exception occur.
using System;
using System.Collections.Generic;
using System.Data;
using System.Drawing;
using System.IO;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Ingres.Client;

namespace IngresDemoApp
{
    public partial class IngresFrequentFlyer
    {
        // Constants
        const int SCHEMA_VER_MAJOR = 1;
        const int SCHEMA_VER_MINOR = 0;
        const int SCHEMA_VER_RELEASE = 0;
        const int DEPART = 1;
        const int ARRIVE = 2;

        // Current departing and arriving airport locations
        Airport departing;
        Airport arriving;
        Airport profile;

        // Current and saved data source details
        TargetDetails currentSource;
        TargetDetails savedSource;

        // Flight days selection
        private char[] flightDays;

        // Manually defined Ingres data adapters and commands.
        // Other adapters used by the application are declared in the
        // Form1.cs [Desgin] view.
        IngresCommand ingresSelectCommand4;
        IngresDataAdapter ingresDataAdapter4;
        IngresCommand ingresSelectCommand7;
        IngresDataAdapter ingresDataAdapter7;
        IngresCommand ingresSelectCommand8;
        IngresDataAdapter ingresDataAdapter8;

        DataSet appConfiguration;
        DataSet versionDataSet;

        private String xsdfile;
        private String profileEmail;

        private void InitializeIngres()
        {
            versionDataSet = new DataSet();

            // The following query text is used to show the syntax for
            // executing a database procedure.
            ingresSelectCommand4 = new IngresCommand(
                "{ call get_my_airports( ccode = ?, area = ? ) }",
                ingresConnection1);
            ingresSelectCommand4.Connection = ingresConnection1;
            ingresDataAdapter4 = new IngresDataAdapter(ingresSelectCommand4);

            ingresSelectCommand7 = new IngresCommand(
                "SELECT up_email FROM user_profile ORDER BY up_email");
            ingresSelectCommand7.Connection = ingresConnection1;
            ingresDataAdapter7 = new IngresDataAdapter(ingresSelectCommand7);

            ingresSelectCommand8 = new IngresCommand(
                "SELECT airport.ap_iatacode, airport.ap_place, " +
                    "country.ct_name " +
                "FROM airport, country " +
                "WHERE airport.ap_ccode = country.ct_code " +
                    "AND airport.ap_iatacode = ?");
            ingresSelectCommand8.Connection = ingresConnection1;
            ingresDataAdapter8 = new IngresDataAdapter(ingresSelectCommand8);

            if (CheckVersion() == false)
            {
                // Incorrect version exception.
                // Get the string from the string table
                Exception badVersion = new Exception(rm.GetString("ErrorVersion"));
                throw badVersion;
            }
        }

        // Name: LoadCountry - populate a dataset with retrieved data
        //
        // Description:
        //      Perform a SELECT query to populate a data set with results
        //      from the database.  Add an empty row into the head of the
        //      data set.
        //
        // Inputs:
        //      countryDataSet
        //
        // Outputs:
        //      countryDataSet
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        //      13-Oct-2006 (ray.fan@ingres.com)
        //          Add a start row to the data set.
        private void LoadCountry(System.Data.DataSet countryDataSet)
        {
            try
            {
                DataRow dr;

                countryDataSet.Clear();
                dr = countryDataSet.Tables["country"].NewRow();
                dr["ct_code"] = rm.GetString("StartCountryCode");
                dr["ct_name"] = rm.GetString("StartCountryName");
                countryDataSet.Tables["country"].Rows.InsertAt(dr, 0);
                ingresDataAdapter1.Fill(countryDataSet);
            }
            catch (Ingres.Client.IngresException ex)
            {
                DialogResult result = DisplayError(ex);
                if (result == DialogResult.OK)
                {
                    throw;
                }
            }
        }

        // Name: LoadRegion - populate a dataset with retrieved data
        //
        // Description:
        //      Perform a SELECT query to populate a data set with results
        //      from the database.
        //
        // Inputs:
        //      direction       Saves the country name according to direction.
        //      regionDataSet
        //
        // Outputs:
        //      regionDataSet
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        //      13-Oct-2006 (ray.fan@ingres.com)
        //          Add start row to the result set.
        private void LoadRegion(int direction, String nation,
            System.Data.DataSet placeDataSet)
        {
            DataRow dr;

            IngresParameter country = new
                Ingres.Client.IngresParameter("ap_ccode", IngresType.NChar);
            country.Value = nation;
            placeDataSet.Clear();
            dr = placeDataSet.Tables["airport"].NewRow();
            dr["ap_place"] = rm.GetString("StartRegionPlace");
            placeDataSet.Tables["airport"].Rows.InsertAt(dr, 0);

            try
            {
                ingresDataAdapter2.SelectCommand.Parameters.Clear();
                ingresDataAdapter2.SelectCommand.Connection.Open();
                ingresDataAdapter2.SelectCommand.Prepare();
                switch (direction)
                {
                    case DEPART:
                        departing.country = country.Value.ToString();
                        break;
                    case ARRIVE:
                        arriving.country = country.Value.ToString();
                        break;
                }
                ingresDataAdapter2.SelectCommand.Parameters.Add(country);
                ingresDataAdapter2.Fill(placeDataSet);
            }
            finally
            {
                if (ingresDataAdapter2 != null &&
                    ingresDataAdapter2.SelectCommand != null &&
                    ingresDataAdapter2.SelectCommand.Connection != null)
                    ingresDataAdapter2.SelectCommand.Connection.Close();
            }
        }

        // Name: LoadAirport - populate a dataset with retrieved data
        //
        // Description:
        //      Perform a call to a row producing procedure to populate a data
        //      set with results from the database.
        //
        // Inputs:
        //      direction       Sets the parameters according to direction.
        //      area
        //      airportDataSet
        //
        // Outputs:
        //      airpotDataSet
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        //      13-Oct-2006 (ray.fan@ingres.com)
        //          Add start row to the result set.
        public void LoadAirport(int direction, String area, System.Data.DataTable airportTable)
        {
            DataRow dataRow;

            IngresParameter country = new Ingres.Client.IngresParameter(
                "ap_ccode", IngresType.NChar);
            IngresParameter region = new Ingres.Client.IngresParameter(
                "ap_place", IngresType.NVarChar);
            region.Value = area;
            airportTable.Clear();
            dataRow = airportTable.NewRow();
            dataRow["ap_iatacode"] = rm.GetString("StartAirportCode"); ;
            airportTable.Rows.InsertAt(dataRow, 0);

            try
            {
                ingresDataAdapter3.SelectCommand.Connection.Open();
                ingresDataAdapter3.SelectCommand.Prepare();
                ingresDataAdapter3.SelectCommand.Parameters.Clear();
                switch (direction)
                {
                    case DEPART:
                        country.Value = departing.country;
                        departing.region = region.Value.ToString();
                        break;
                    case ARRIVE:
                        country.Value = arriving.country;
                        arriving.region = region.Value.ToString();
                        break;
                }
                ingresDataAdapter3.SelectCommand.Parameters.Add(country);
                ingresDataAdapter3.SelectCommand.Parameters.Add(region);
                ingresDataAdapter3.Fill(airportTable);
            }
            finally
            {
                if (ingresDataAdapter3 != null &&
                    ingresDataAdapter3.SelectCommand != null &&
                    ingresDataAdapter3.SelectCommand.Connection != null)
                    ingresDataAdapter3.SelectCommand.Connection.Close();
            }
        }

        // Name: LoadAirportName - Populate s data set with airport names
        //
        // Description:
        //      Execute a query to return the airport codes with their names.
        //
        // Inputs:
        //      direction           Specifies which store country code to use.
        //      area                Specifies the place value to use in the
        //                          WHERE clause.
        //      airportNameTable    Area to store the result set.
        //
        // Outputs:
        //      airportNameTable    Updated with result set.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public void LoadAirportName(int direction, String area,
            System.Data.DataTable airportNameTable)
        {
            IngresParameter country =
                new Ingres.Client.IngresParameter("ap_ccode",
                    IngresType.NChar);
            IngresParameter region =
                new Ingres.Client.IngresParameter("ap_place",
                    IngresType.NVarChar);
            region.Value = area;
            airportNameTable.Clear();
            try
            {
                ingresDataAdapter4.SelectCommand.Connection.Open();
                ingresDataAdapter4.SelectCommand.Prepare();
                ingresDataAdapter4.SelectCommand.Parameters.Clear();
                switch (direction)
                {
                    case DEPART:
                        country.Value = departing.country;
                        break;
                    case ARRIVE:
                        country.Value = arriving.country;
                        break;
                }
                ingresDataAdapter4.SelectCommand.Parameters.Add(country);
                ingresDataAdapter4.SelectCommand.Parameters.Add(region);
                ingresDataAdapter4.Fill(airportNameTable);
            }
            finally
            {
                if (ingresDataAdapter4 != null &&
                    ingresDataAdapter4.SelectCommand != null &&
                    ingresDataAdapter4.SelectCommand.Connection != null)
                    ingresDataAdapter4.SelectCommand.Connection.Close();
            }
        }

        // Name: GetAirportInfo - Get the values for a specified airport code
        //
        // Description:
        //      Executes a query to return the airport values identified by
        //      the IATA code.
        //
        // Inputs:
        //      airportInfo     Area to store result set.
        //      IATAcode        Airport identified by IATA code.
        //
        // Outputs:
        //      airportInfo     Updated result set.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public void GetAirportInfo(DataTable airportInfo, String IATAcode)
        {
            airportInfo.Clear();
            try
            {
                ingresDataAdapter8.SelectCommand.Parameters.Clear();
                ingresDataAdapter8.SelectCommand.Connection.Open();
                ingresDataAdapter8.SelectCommand.Prepare();
                ingresDataAdapter8.SelectCommand.Parameters.Add("ap_iatacode",
                    IATAcode);
                ingresDataAdapter8.Fill(airportInfo);
            }
            finally
            {
                if (ingresDataAdapter8 != null &&
                    ingresDataAdapter8.SelectCommand != null &&
                    ingresDataAdapter8.SelectCommand.Connection != null)
                    ingresDataAdapter8.SelectCommand.Connection.Close();
            }
        }

        //
        // Routes
        // ------
        //

        // Name: RouteSearch
        //
        // Description:
        //      Executes a SELECT query to retrieve routes.
        //      Query command text is varied according to the round trip
        //      parameter as is the WHERE clause.
        //
        // Inputs:
        //      routeDataSet        Area to store query results.
        //      departingAirport    Departing airport code.
        //      arrivingAirport     Arriving airport code.
        //      flightDays          Prefered days mask.
        //      roundTrip           Flag to include return flights
        //
        // Outputs:
        //      routeDataSet        Updated with result set.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public void RouteSearch(DataSet routeDataSet, String departingAirport,
            String arrivingAirport, String flightDays, CheckState roundtrip)
        {
            IngresParameter depart =
                new Ingres.Client.IngresParameter("rt_depart_from",
                    IngresType.NChar);
            IngresParameter arrive =
                new Ingres.Client.IngresParameter("rt_arrive_to",
                    IngresType.NChar);
            IngresParameter returnTripDepartingAirport =
                new Ingres.Client.IngresParameter("rt_depart_from",
                    IngresType.NChar);
            IngresParameter returnTripArrivingAirport =
                new Ingres.Client.IngresParameter("rt_arrive_to",
                    IngresType.NChar);
            IngresParameter onDays =
                new Ingres.Client.IngresParameter("rt_flight_day",
                    IngresType.NChar);

            depart.Value = departingAirport;
            arrive.Value = arrivingAirport;
            returnTripDepartingAirport.Value = arrivingAirport;
            returnTripArrivingAirport.Value = departingAirport;
            onDays.Value = flightDays;
            routeDataSet.Clear();

            // Clear the query parameter list, parameters need to be added in
            // the order they appear in the query.
            ingresDataAdapter5.SelectCommand.Parameters.Clear();

            // Add route query parameters
            ingresDataAdapter5.SelectCommand.Parameters.Add(depart);
            ingresDataAdapter5.SelectCommand.Parameters.Add(arrive);

            if (roundtrip == CheckState.Checked)
            {
                // Set the query command text with additional WHERE clause
                ingresDataAdapter5.SelectCommand.CommandText =
                    String.Format(rm.GetString("RouteQuery"),
                    rm.GetString("RouteQueryRoundTripWhereClause"));
                // Add return trip query paramters
                ingresDataAdapter5.SelectCommand.Parameters.Add(returnTripDepartingAirport);
                ingresDataAdapter5.SelectCommand.Parameters.Add(returnTripArrivingAirport);
            }
            else
            {
                // Set the query command text
                ingresDataAdapter5.SelectCommand.CommandText =
                    String.Format(rm.GetString("RouteQuery"), "");
            }
            ingresDataAdapter5.SelectCommand.Parameters.Add(onDays);

            try
            {
                ingresDataAdapter5.SelectCommand.Connection.Open();
                ingresDataAdapter5.SelectCommand.Prepare();
                ingresDataAdapter5.Fill(routeDataSet);
            }
            finally
            {
                if (ingresDataAdapter5 != null &&
                    ingresDataAdapter5.SelectCommand != null &&
                    ingresDataAdapter5.SelectCommand.Connection != null)
                    ingresDataAdapter5.SelectCommand.Connection.Close();
            }
        }

        // Name: RouteAdd
        //
        // Description:
        //      Executes an INSERT query to add a new route.
        //
        // Inputs:
        //      dataTable           Table containing new values.
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
        public void RouteAdd(DataTable dataTable)
        {
            try
            {
                ingresDataAdapter5.InsertCommand.Parameters.Clear();
                ingresDataAdapter5.InsertCommand.Parameters.Add("@rt_airline",
                    Ingres.Client.IngresType.NChar, "rt_airline");
                ingresDataAdapter5.InsertCommand.Parameters.Add("@rt_flight_num",
                    Ingres.Client.IngresType.Int, "rt_flight_num");
                ingresDataAdapter5.InsertCommand.Parameters.Add("@rt_depart_from",
                    Ingres.Client.IngresType.NChar, "rt_depart_from");
                ingresDataAdapter5.InsertCommand.Parameters.Add("@rt_arrive_to",
                    Ingres.Client.IngresType.NChar, "rt_arrive_to");
                ingresDataAdapter5.InsertCommand.Parameters.Add("@rt_depart_at",
                    Ingres.Client.IngresType.DateTime, "rt_depart_at");
                ingresDataAdapter5.InsertCommand.Parameters.Add("@rt_arrive_at",
                    Ingres.Client.IngresType.DateTime, "rt_arrive_at");
                ingresDataAdapter5.InsertCommand.Parameters.Add("@rt_arrive_offset",
                    Ingres.Client.IngresType.TinyInt, "rt_arrive_offset");
                ingresDataAdapter5.InsertCommand.Parameters.Add("@rt_flight_day",
                    Ingres.Client.IngresType.NChar, "rt_flight_day");
                ingresDataAdapter5.Update(dataTable);
            }
            catch (IngresException ex)
            {
                DialogResult result = DisplayError(ex);
                throw;
            }
        }

        // Name: RouteChange
        //
        // Description:
        //      Executes an UPDATE query to change a route.
        //
        // Inputs:
        //      dataSet     Table containing changed values.
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
        public void RouteChange(DataSet dataSet)
        {
            try
            {
                ingresDataAdapter5.UpdateCommand.Parameters.Clear();
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@rt_airline",
                    Ingres.Client.IngresType.NChar, "rt_airline");
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@rt_flight_num",
                    Ingres.Client.IngresType.Int, "rt_flight_num");
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@rt_depart_from",
                    Ingres.Client.IngresType.NChar, "rt_depart_from");
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@rt_arrive_to",
                    Ingres.Client.IngresType.NChar, "rt_arrive_to");
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@rt_depart_at",
                    Ingres.Client.IngresType.DateTime, "rt_depart_at");
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@rt_arrive_at",
                    Ingres.Client.IngresType.DateTime, "rt_arrive_at");
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@rt_arrive_offset",
                    Ingres.Client.IngresType.TinyInt, "rt_arrive_offset");
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@rt_flight_day",
                    Ingres.Client.IngresType.NChar, "rt_flight_day");
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@airline", IngresType.NChar, "rt_airline").SourceVersion = DataRowVersion.Original;
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@flight", IngresType.Int, "rt_flight_num").SourceVersion = DataRowVersion.Original;
                ingresDataAdapter5.UpdateCommand.Parameters.Add("@days", IngresType.NChar, "rt_flight_day").SourceVersion = DataRowVersion.Original;
                ingresDataAdapter5.Update(dataSet);
            }
            catch (IngresException ex)
            {
                DialogResult result = DisplayError(ex);
                throw;
            }
        }

        // Name: RouteDelete
        //
        // Description:
        //      Executes an DELET query to remove a route.
        //
        // Inputs:
        //      dataTable           Table containing new values.
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
        public int RouteDelete(DataGridViewRow row)
        {
            int deleted = 0;
            IngresParameter airline = new IngresParameter("rt_airline", IngresType.NChar);
            IngresParameter flight = new IngresParameter("rt_flight_num", IngresType.Int);
            IngresParameter flight_day = new IngresParameter("rt_flight_day", IngresType.NChar);
            IngresCommand delCommand = ingresDataAdapter5.DeleteCommand;
            try
            {
                IngresTransaction delTxn;

                airline.Value = row.Cells[0].Value;
                flight.Value = row.Cells[2].Value;
                flight_day.Value = row.Cells[8].Value;
                delCommand.Connection.Open();
                delTxn = delCommand.Connection.BeginTransaction();
                delCommand.Transaction = delTxn;
                delCommand.Prepare();
                delCommand.Parameters.Clear();
                delCommand.Parameters.Add(airline);
                delCommand.Parameters.Add(flight);
                delCommand.Parameters.Add(flight_day);
                deleted = delCommand.ExecuteNonQuery();
                DialogResult result = MessageBox.Show(
                    String.Format(
                        rm.GetString("RouteDeleteConfirm"),
                        airline.Value.ToString(),
                        flight.Value.ToString(),
                        row.Cells[3].Value,
                        row.Cells[4].Value,
                        row.Cells[8].Value),
                    rm.GetString("RouteDeleteCaption"),
                    MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                if (result == DialogResult.No)
                {
                    deleted = 0;
                    delTxn.Rollback();
                }
                else if (result == DialogResult.Yes)
                {
                    delTxn.Commit();
                }
            }
            catch (IngresException ex)
            {
                DialogResult result = DisplayError(ex);
                throw;
            }
            finally
            {
                if (delCommand != null   &&
                    delCommand.Connection != null)
                    delCommand.Connection.Close();
            }
            return (deleted);
        }

        //
        // Profiles
        // --------
        //

        // Name: LoadProfile - Returns the values for a specfied profile
        //
        // Description:
        //      Executes the query to return the values identified by the
        //      profile email address.
        //
        // Inputs:
        //      profileDataTable    Area to store query results.
        //      email               Profile identifying email address.
        //
        // Outputs:
        //      profileDataTable    Updated result table.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public void LoadProfile(DataTable profileDataTable, String email)
        {
            IngresParameter up_email =
                new Ingres.Client.IngresParameter("up_email", IngresType.NChar);

            try
            {
                up_email.Value = email;
                profileDataTable.Clear();
                ingresDataAdapter6.SelectCommand.Connection.Open();
                ingresDataAdapter6.SelectCommand.Prepare();
                ingresDataAdapter6.SelectCommand.Parameters.Clear();
                ingresDataAdapter6.SelectCommand.Parameters.Add(up_email);
                ingresDataAdapter6.Fill(profileDataTable);
            }
            catch (IngresException ex)
            {
                DialogResult result = DisplayError(ex);
            }
            finally
            {
                if (ingresDataAdapter6 != null &&
                    ingresDataAdapter6.SelectCommand != null &&
                    ingresDataAdapter6.SelectCommand.Connection != null)
                    ingresDataAdapter6.SelectCommand.Connection.Close();
            }
        }

        // Name: GetProfileList - Get the list of profile email addresses
        //
        // Description:
        //      Executes a query to return the list of identifying email
        //      addresses.
        //
        // Inputs:
        //      profileDataTable    Area to store result set.
        //
        // Outputs:
        //      profileDataTable    Update result set.
        //  
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        //      13-Oct-2006 (ray.fan@ingres.com)
        //          Add start row to the result set.
        public void GetProfileList(DataTable profileDataTable)
        {
            try
            {
                DataRow dr;

                profileDataTable.Clear();
                dr = profileDataTable.NewRow();
                dr["up_email"] = rm.GetString("StartProfileEmail");
                profileDataTable.Rows.InsertAt(dr, 0);
                ingresDataAdapter7.Fill(profileDataTable);
            }
            catch (IngresException ex)
            {
                DialogResult result = DisplayError(ex);
            }
        }

        // Name: ProfileAdd
        //
        // Description:
        //      Insert a new profile
        //
        // Inputs:
        //      porfileDataSet  Data set containing rows to update.
        //      lastName        Last name value.
        //      firstName       First name value.
        //      email           Identifying email address.
        //      airport         Airport preference value.
        //      photoFile       Path to graphics image.
        //
        // Outputs:
        //      profileDataSet  Updated with new row.
        //
        // Returns:
        //      resAffected     Number of rows affected.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private int ProfileAdd(ProfileDataSet profileDataSet, String lastName,
            String firstName, String email, String airport, String photoFile)
        {
            int resAffected = 0;
            try
            {
                // Make a new row can use clone or just invoke NewRow method
                // for the table.
                DataTable dataTable = profileDataSet.Tables["user_profile"].Clone();
                DataRow dataRow = dataTable.NewRow();

                // Populate the columns with new values.
                dataRow["up_last"] = lastName;
                dataRow["up_first"] = firstName;
                dataRow["up_email"] = email;
                dataRow["up_airport"] = airport;

                // If a path to an image is specfied convert it into a Byte
                // array value and assign it tp the up_image column.
                // If there is no path a default null value will be used.
                if (photoFile.Length > 0)
                {
                    MemoryStream stream = new MemoryStream();
                    Image photo = System.Drawing.Image.FromFile(photoFile);
                    photo.Save(stream, photo.RawFormat);
                    dataRow["up_image"] = stream.ToArray();
                    stream.Close();
                }
                // Add the row into the table.
                dataTable.Rows.Add(dataRow);

                // Clear the command parameter list.
                ingresDataAdapter6.InsertCommand.Parameters.Clear();

                // Associate the data row columns with the data set columns.
                ingresDataAdapter6.InsertCommand.Parameters.Add("@up_last",
                    Ingres.Client.IngresType.NVarChar, "up_last");
                ingresDataAdapter6.InsertCommand.Parameters.Add("@up_first",
                    Ingres.Client.IngresType.NVarChar, "up_first");
                ingresDataAdapter6.InsertCommand.Parameters.Add("@up_email",
                    Ingres.Client.IngresType.NVarChar, "up_email");
                ingresDataAdapter6.InsertCommand.Parameters.Add("@up_airport",
                    Ingres.Client.IngresType.NChar, "up_airport");
                ingresDataAdapter6.InsertCommand.Parameters.Add("@up_image",
                    Ingres.Client.IngresType.LongVarBinary, "up_image");
                
                // Perform the insert.
                resAffected = ingresDataAdapter6.Update(dataTable);
            }
            catch (IngresException ex)
            {
                DialogResult result = DisplayError(ex);
            }
            return (resAffected);
        }

        // Name: ProfileChange - Update the specified profile
        //
        // Description:
        //      Updated the profile identified by the email.
        //
        // Inputs:
        //      dataSet     Data set containing rows to update.
        //      lastName    Last name value.
        //      firstName   First name value.
        //      email       Identifying email address.
        //      airport     Airport preference value.
        //      photoFile   Path to new or updated graphics image.
        //
        // Outputs:
        //      profileDataSet  Updated result set.
        //
        // Returns:
        //      rowsDeleted     Number of rows affected by the query.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        //      11-Oct-2006 (ray.fan@ingres.com)
        //          Corrected typos.
        private int ProfileChange(ProfileDataSet dataSet, String lastName,
            String firstName, String email, String airport, String photoFile)
        {
            int resAffected = 0;
            try
            {
                // Clear the command parameter list
                ingresDataAdapter6.UpdateCommand.Parameters.Clear();

                // Associate the data row columns with the data set columns.
                ingresDataAdapter6.UpdateCommand.Parameters.Add("@up_last",
                    Ingres.Client.IngresType.NVarChar, "up_last");
                ingresDataAdapter6.UpdateCommand.Parameters.Add("@up_first",
                    Ingres.Client.IngresType.NVarChar, "up_first");
                ingresDataAdapter6.UpdateCommand.Parameters.Add("@up_email",
                    Ingres.Client.IngresType.NVarChar, "up_email");
                ingresDataAdapter6.UpdateCommand.Parameters.Add("@up_airport",
                    Ingres.Client.IngresType.NChar, "up_airport");

                // If a path to an image is specified convert it into a Byte
                // array value and assign it to the up_image column.
                // If there is no path do not specify up_image as a parameter.
                if (photoFile.Length > 0)
                {
                    MemoryStream stream = new MemoryStream();
                    Image photo = System.Drawing.Image.FromFile(photoFile);
                    photo.Save(stream, photo.RawFormat);
                    ingresDataAdapter6.UpdateCommand.Parameters.Add("@up_image",
                        Ingres.Client.IngresType.LongVarBinary, "up_image");
                    dataSet.Tables["user_profile"].Rows[0]["up_image"] = stream.ToArray();
                    stream.Close();
                    ingresDataAdapter6.UpdateCommand.CommandText =
                        String.Format(rm.GetString("ProfileUpdateQuery"),
                            rm.GetString("ImageAttribute"));
                }
                else
                {
                    ingresDataAdapter6.UpdateCommand.CommandText =
                        String.Format(rm.GetString("ProfileUpdateQuery"), "");
                }
                // Specify the previous value of email for the row as the
                // identity of the row for update in the WHERE clause.
                ingresDataAdapter6.UpdateCommand.Parameters.Add("@up_email",
                    IngresType.NChar, "up_email").SourceVersion = 
                    DataRowVersion.Original;

                // Assign the values into the data set table.
                dataSet.Tables["user_profile"].Rows[0]["up_last"] = lastName;
                dataSet.Tables["user_profile"].Rows[0]["up_first"] = firstName;
                dataSet.Tables["user_profile"].Rows[0]["up_email"] = email;
                dataSet.Tables["user_profile"].Rows[0]["up_airport"] = airport;

                // Perform the update.
                resAffected = ingresDataAdapter6.Update(dataSet);
            }
            catch (IngresException ex)
            {
                DialogResult result = DisplayError(ex);
            }
            return (resAffected);
        }

        // Name: ProfileRemove - Delete the specified profile
        //
        // Description:
        //      Deletes the profile identified by the email.
        //
        // Inputs:
        //      profileDataSet  Data set.
        //      profileAddr     Identifying profile address.
        //
        // Outputs:
        //      profileDataSet  Updated result set.
        //
        // Returns:
        //      rowsDeleted     Number of rows affected by the query.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
	//	11-Aug-2009 (Viktoriya.Driker@ingres.com)
	//	    Added Transaction before Prepare is called on the 
	//	    delete command.
        public int ProfileRemove(DataSet profileDataSet, string profileAddr)
        {
            int rowsDeleted = 0;
            IngresParameter email = new IngresParameter("up_email", IngresType.NChar);
            try
            {
                IngresTransaction delete;
                email.Value = profileAddr;
                ingresDataAdapter6.DeleteCommand.Connection.Open();
                delete = ingresDataAdapter6.DeleteCommand.Connection.BeginTransaction();
                ingresDataAdapter6.DeleteCommand.Transaction = delete;
                ingresDataAdapter6.DeleteCommand.Prepare();
                ingresDataAdapter6.DeleteCommand.Parameters.Clear();
                ingresDataAdapter6.DeleteCommand.Parameters.Add(email);
                rowsDeleted = ingresDataAdapter6.DeleteCommand.ExecuteNonQuery();
                DialogResult result = MessageBox.Show("Profile: " + profileAddr,
                    "Remove profile", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);
                if (result == DialogResult.No)
                {
                    rowsDeleted = 0;
                    delete.Rollback();
                }
                else if (result == DialogResult.Yes)
                {
                    delete.Commit();
                }
            }
            catch (IngresException ex)
            {
                DialogResult result = DisplayError(ex);
                if (result != DialogResult.OK)
                {
                    throw;
                }
            }
            finally
            {
                if (ingresDataAdapter6 != null &&
                    ingresDataAdapter6.DeleteCommand != null &&
                    ingresDataAdapter6.DeleteCommand.Connection != null)
                    ingresDataAdapter6.DeleteCommand.Connection.Close();
            }
            return (rowsDeleted);
        }

        //
        // Utility methods
        // ---------------
        //

        // Name: CheckVersion - Test the stored schema version
        //
        // Description:
        //      Compares the specified version number with the stored version
        //      of the database schema.
        //      If the version matches or exceeds the required version the
        //      function returns true.
        //
        // Inputs:
        //      None.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      true        Version matches or exceeds the required version.
        //      false       Incorrect version level.
        //
        // History:
        //      28-Nov-2006 (ray.fan@ingres.com)
        //          Created.
        public bool CheckVersion()
        {
            return(CheckVersion(SCHEMA_VER_MAJOR, SCHEMA_VER_MINOR,
                SCHEMA_VER_RELEASE));
        }

        // Name: CheckVersion - Test the stored schema version
        //
        // Description:
        //      Compares the specified version number with the stored version
        //      If the verson matches or exceeds the required version the
        //      function returns true.
        //
        // Inputs:
        //      Major       Major version number.
        //      Minor       Minor version number.
        //      Release     Release version number.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      true        Version matches or exceeds the required version.
        //      false       Incorrect version level.
        //
        // History
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public bool CheckVersion(int Major, int Minor, int Release)
        {
            bool retcode = false;
            IngresCommand ingresVersionSelect = new IngresCommand();
            IngresDataAdapter ingresVersionAdapter;

            // Create query parameters and store the method parameter values
            // into each one.
            IngresParameter majorVersion = new IngresParameter("ver_major", IngresType.Int);
            IngresParameter minorVersion = new IngresParameter("ver_minor", IngresType.Int); ;
            IngresParameter releaseVersion = new IngresParameter("ver_release", IngresType.Int);

            majorVersion.Value = Major;
            minorVersion.Value = Minor;
            releaseVersion.Value = Release;

            ingresVersionSelect = new IngresCommand(
                "SELECT FIRST 1 ver_major, ver_minor, ver_release, ver_date, ver_install" +
                " FROM version" +
                " WHERE ver_major >= ? AND ver_minor >= ? AND ver_release >=?" +
                " ORDER BY ver_id DESC",
                ingresConnection1);
            ingresVersionSelect.Connection = ingresConnection1;
            ingresVersionAdapter = new IngresDataAdapter(ingresVersionSelect);
            try
            {
                ingresVersionSelect.Connection.Open();
                ingresVersionSelect.Prepare();
                ingresVersionSelect.Parameters.Clear();
                ingresVersionSelect.Parameters.Add(majorVersion);
                ingresVersionSelect.Parameters.Add(minorVersion);
                ingresVersionSelect.Parameters.Add(releaseVersion);
                versionDataSet.Clear();
                ingresVersionAdapter.Fill(versionDataSet);
                if (versionDataSet.Tables[0].Rows.Count > 0)
                {
                    retcode = true;
                }
            }
            catch (Ingres.Client.IngresException ex)
            {
                if (DisplayError(ex) == DialogResult.OK)
                {
                    throw;
                }
                else
                {
                    Application.Exit();
                }
            }
            finally
            {
                if (ingresVersionSelect != null &&
                    ingresVersionSelect.Connection != null)
                    ingresVersionSelect.Connection.Close();
            }
            return (retcode);
        }

        // Name: DisplayError
        //
        // Description:
        //      Function to show the properties of IngresException and
        //      IngresError classes.
        //      Extracts exception and error text and forms a dialog string.
        //      Provides an example of handling SQLstate errors.
        //
        // Input:
        //      ex      IngresException class.
        //
        // Output:
        //      None.
        //
        // Returns:
        //      Depends on the buttons enabled for the dialog.
        //      DialogResult.OK
        //      DialogResult.Cancel
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        //      02-Feb-2006 (ray.fan@ingres.com)
        //          Add user message string for connection failure.
        public DialogResult DisplayError(IngresException ex)
        {
            String userText = "";
            String ingError = "";
            MessageBoxButtons button = MessageBoxButtons.OK;
            MessageBoxIcon icon = MessageBoxIcon.Information;
            Int32 unwindLevel = 5;  // Set the level to scan through errors
                                    // Adjust this value to include the
                                    // collection of errors in the display.
            Int32 nested = 0;

            ingError += ex.GetType().ToString() + "\n\nException Properties\n";
            // IngresException Class properties:
            //
            // InnerException   The nested Exception instance that caused the
            //                  current exception. Inherited from Exception.
            // Message          A concatenation of all the messages in the
            //                  Errors collection.
            // Source           Name of the data provider that generated the
            //                  error. Always "Ingres."
            // StackTrace       A string representation of the frames on the
            //                  call stack at the time the current exception
            //                  was thrown.
            // TargetSite       The method that threw the current exception.
            //                  Inherited from Exception.
            //
            // For property descriptions of the IngresException class see
            // the IngresException Class Properties section in the Ingres
            // Connectivity Guide
            if (ex.HelpLink != null)
            {
                ingError += "HelpLink: " + ex.HelpLink.ToString() + "\n";
            }
            if (ex.InnerException != null)
            {
                ingError += "Inner Exception: " + ex.InnerException.ToString() + "\n";
            }
            if (ex.Message != null)
            {
                ingError += "Message: " + ex.Message.ToString() + "\n";
            }
            if (ex.Source != null)
            {
                ingError += "Source: " + ex.Source.ToString() + "\n";
            }
            if (ex.TargetSite != null)
            {
                ingError += "Target Site: " + ex.TargetSite.ToString() + "\n";
            }
            ingError += "\nErrors\n";
            foreach (IngresError e in ex.Errors)
            {
                // Don't processes any errors beyond the unwindLevel
                if (nested == unwindLevel)
                    continue;
                // IngresError Class properties:
                //
                // Message     A description of the error.
                // Number      The database-specific error integer information
                //             returned by the Ingres database.
                // Source      Name of the data provider that generated the
                //             error. Always "Ingres."
                // SQLState    The standard five-character SQLSTATE code.
                //
                // For property descriptions of the IngresError class see
                // the IngresError Class Properties section in the Ingres
                // Connectivity Guide
                switch (e.SQLState)
                {
                    case "00000":
                    case "01000":
                        break;
                    case "02000":
                        icon = MessageBoxIcon.Warning;
                        break;
                    case "08001":
                        if (nested == 0)
                        {
                            userText += rm.GetString("SQLSTATE" + e.SQLState);
                        }
                        button = MessageBoxButtons.OKCancel;
                        icon = MessageBoxIcon.Error;
                        break;
                    default:
                        button = MessageBoxButtons.OKCancel;
                        icon = MessageBoxIcon.Error;
                        break;
                }
                ingError += String.Format("SQLState: " + e.SQLState.ToString() + " " + e.Message + "\n");
                if (e.NativeError != 0)
                {
                    ingError += String.Format("Ingres Error code: 0x{0:x08}\n", e.Number);
                }
                nested += 1;
            }
            if (userText.Length > 0)
            {
                ingError += "\n";
                ingError += userText;
            }
            DialogResult result = MessageBox.Show(ingError,
                rm.GetString("Display"),
                button, icon);
            return (result);
        }

        // Name: SetConnection - Change the connection string
        //
        // Description:
        //      Change the connection string in the Ingres connection obbject.
        //      
        // Input:
        //      ingresConnection    Connection object to update.
        //      connectString       Updated connection string.
        //
        // Output:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void SetConnection(IngresConnection ingresConnection, String connectString)
        {
            ingresConnection.ConnectionString = connectString;
        }

        //
        // Event handlers
        // --------------
        //

        // Name: ingresDataAdapter5_RowUpdated
        //
        // Description:
        //      Set a timer to reload the data set.
        //      The data used to populate the DataGridView is composed from an
        //      inner join query.  Updating through the DataGridView requires
        //      that on commit the content of DataGridView needs to be
        //      refreshed to populate all fields in the new row.
        //      This event handler starts a timer that invokes a refresh by
        //      issuing a query with the insert data as the set of query
        //      parameters.
        //
        // Input:
        //      sender  Ingres data adapter.
        //      e       event argument.
        // Output:
        //      None.
        //
        // Returns:
        //      None.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        private void ingresDataAdapter5_RowUpdated(object sender, Ingres.Client.IngresRowUpdatedEventArgs e)
        {
            if (e.StatementType == StatementType.Insert)
            {
                // Inserted row needs to be updated, a datagridview with
                // only one row will treat updates as new inserts since the
                // RowState doesn't change.
                // Also the populating query needs to be rerun to get
                // joined information.
                System.Windows.Forms.Timer t = new System.Windows.Forms.Timer();
                t.Tick +=new EventHandler(routeReload);
                t.Interval = 50;
                t.Enabled = true;
            }
        }
    }

    //
    // Structure definitions
    // ---------------------
    //

    // Name: TargetDetails
    //
    // Description:
    //  Structure to hold target data source details.
    //  Methods are provided for returning an Ingres connection string
    //  which can be used with the IngresConnection class.
    //
    //  Public members:
    //      host                    Target host name.
    //      port                    Ingres port identifier.
    //                              E.g., II7.
    //      db                      Database name.
    //      user                    User ID.
    //      pwd                     User password.
    //
    //  Public methods:
    //      TargetDetails          Constructor
    //      GetConnectionString     Returns the connection string for use with the
    //                              IngresConnection class.
    //      GetConnectForDisplay    Returns the connection string with
    //                              dummy password field.
    //
    //  History:
    //      02-Oct-2006 (ray.fan@ingres.com)
    //          Created.
    public struct TargetDetails
    {
        public String host;
        public String port;
        public String db;
        public String user;
        public String pwd;

        private String connect;
        private String display;
        private String targetFormat;
        private String credentialFormat;

        private System.Resources.ResourceManager resource;

        // Constructors

        // Name: TargetDetails
        //
        // Description:
        //  Constructor for the TargetDetails structure.
        //
        // Inputs:
        //      Host                    Target host name.
        //      Port                    Ingres port identifier.
        //                              E.g., II7.
        //      Database                Database name.
        //      UserID                  User ID.
        //      Password                User password.
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
        public TargetDetails(String Host, String Port,
            String Database, String UserID, String Password )
        {
            host = Host;
            port = Port;
            db = Database;
            user = UserID;
            pwd = Password;

            resource =
            new System.Resources.ResourceManager("IngresDemoApp.Properties.Resources",
                System.Reflection.Assembly.GetExecutingAssembly());

            targetFormat = resource.GetString("ConnectString");
            credentialFormat = resource.GetString("ConnectStringCredentials");

            connect = String.Format(targetFormat, host, port, db);
            if ((user != null) && (user.Length > 0))
            {
                connect += String.Format(credentialFormat, user, pwd);
            }

            display = String.Format(targetFormat, host, port, db);
            if ((user != null) && (user.Length > 0))
            {
                display += String.Format(credentialFormat, user, "**********");
            }
        }

        // Methods

        // Name: GetConnectionString
        //
        // Description:
        //      Method for retrieving the connection string.
        //
        // Inputs:
        //      None.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      Connection string which can be used with the IngresConnection
        //      class.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public String GetConnectionString()
        {
            resource =
            new System.Resources.ResourceManager("IngresDemoApp.Properties.Resources",
                System.Reflection.Assembly.GetExecutingAssembly());

            targetFormat = resource.GetString("ConnectString");
            credentialFormat = resource.GetString("ConnectStringCredentials");

            connect = String.Format(targetFormat, host, port, db);
            if ((user != null) && (user.Length > 0))
            {
                connect += String.Format(credentialFormat, user, pwd);
            }
            return (connect);
        }

        // Name: GetConnectionForDisplay
        //
        // Description:
        //      Method for retrieving the connection string without password.
        //
        // Inputs:
        //      None.
        //
        // Outputs:
        //      None.
        //
        // Returns:
        //      Connection string without password information.
        //
        // History:
        //      02-Oct-2006 (ray.fan@ingres.com)
        //          Created.
        public String GetConnectForDisplay()
        {
            resource =
            new System.Resources.ResourceManager("IngresDemoApp.Properties.Resources",
                System.Reflection.Assembly.GetExecutingAssembly());

            targetFormat = resource.GetString("ConnectString");
            credentialFormat = resource.GetString("ConnectStringCredentials");
            display = String.Format(targetFormat, host, port, db);
            if ((user != null) && (user.Length > 0))
            {
                display += String.Format(credentialFormat, user, "**********");
            }
            return (display);
        }
    }

    // Name: Airport
    //
    // Description:
    //      Structure to hold airport information
    //
    // Public members:
    //
    //      country     Country name string
    //      region      Region name string
    //      icaocode    ICAO airport code string
    //
    //  History:
    //      02-Oct-2006 (ray.fan@ingres.com)
    //          Created.
    public struct Airport
    {
        public String country;
        public String region;
        public String icaocode;

        public CountryDataSet countryDataSet;
        public RegionDataSet regionDataSet;
        public AirportDataSet icaocodeDataSet;

        //Constructors
        public Airport(String Country, String Region, String ICAOcode)
        {
            country = Country;
            region = Region;
            icaocode = ICAOcode;
            countryDataSet = null;
            regionDataSet = null;
            icaocodeDataSet = null;
        }
        public Airport(String Country, String Region)
        {
            country = Country;
            region = Region;
            icaocode = null;
            countryDataSet = null;
            regionDataSet = null;
            icaocodeDataSet = null;
        }
        public Airport(String Country)
        {
            country = Country;
            region = null;
            icaocode = null;
            countryDataSet = null;
            regionDataSet = null;
            icaocodeDataSet = null;
        }
    }
}
