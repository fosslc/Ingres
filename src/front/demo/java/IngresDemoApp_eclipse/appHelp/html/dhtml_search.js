/* Generated with AuthorIT version 4.3.556  1/9/2007 10:04:19 AM */
Page=new Array();
Page[0]=new Array("Welcome to the Frequent Flyer demonstration application, which is designed to introduce you to developing applications with Ingres, Java and Eclipse DTP 1.0. ","The application consists of the following:","A database supporting NFC Unicode data called &quot;demodb&quot; created when Ingres or the demonstration is installed. ","Sample data related to air travel—for example, airlines, airports, and routes. ","Sample code illustrating common functionality that an application developer may want to write. ","We hope that you find this application useful. If you have questions or comments, please raise an issue with  Ingres Technical Support.","Note: Knowledge of Java and Eclipse DTP is necessary to understand and browse the example code. ","Welcome",
"welcome.html");
Page[1]=new Array("The application consists of three functional areas:","Routes","Lets you find a route between airports, demonstrating the following: ","SELECT queries that require parameters","Execution of a row producing procedure","INSERT, UPDATE, and DELETE queries","Profile","Lets you select or create details for a Frequent Flyer, demonstrating the following:","Use of Unicode character types","Handling of binary large objects (BLOBs)","Connect","Lets you set up connection details to an Ingres instance","Ingres Features Demonstrated",
"ingres_features.html");
Page[2]=new Array("The application is derived from several Java packages containing the application source files. These Java packages include:","com.ingres.demoapp","Contains the application controlling classes.","com.ingres.demoapp.actions","Contains the application configuration classes.","com.ingres.demoapp.model","Contains classes that store information for the Database Connection, Airports, Countries, Routes and User Profiles.","com.ingres.demoapp.persistence","Contains classes that handle the Ingres database for Database Configuration, Airports, Countries, Routes and User Profiles.","com.ingres.demoapp.ui","Contains definitions of constants for use in the help system.","com.ingres.demoapp.ui.editors","Contains the user interface classes.","com.ingres.demoapp.ui.perspectives","Contains classes that define the initial layouts of the user interface windows.","com.ingres.demoapp.ui.views","Contains classes that define the views which display results information.","com.ingres.demoapp.ui.widget","Contains widgets for selecting information from the database.","&nbsp;&nbsp;&nbsp;","The database access source files include:","AirportDAO.java","Manages airport information.","CountryDAO.java","Manages country information.","DatabaseConfigurationDAO.java","Manages database connection information.","DataSourceFactory.java","Provides the Factory class for the Ingres data source.","RouteDAO.java","Manages route information.","UserProfileDAO.java","Manages user profile information.","Source Code Packages",
"source_code_packages.html");
Page[3]=new Array("The demodb schema consists of three tables. ","The tables Airport and Airline form master tables, and the Route table is a detail table.","Database Schema",
"database_schema.html");
Page[4]=new Array("The Routes form requires four data items for both the departure and arrival airport to retrieve the flights result set. These four data items include:","Country","Region","Airport","Day","The following three parameters are used to filter data from the database:","Departing airport code","Arriving airport code","Operating days","The Return Journey parameter extends queries passed to Ingres with the same criteria as for the outward bound queries, but with the departing and arriving airport codes swapped.  ","The list of airports is reduced by restricting the available airports based on the region. The list of regions is reduced by restricting the possible regions based on the country.","Airport code selection is composed of three form controls:","Form Control","Departing","Arriving","CountryComboBox","departingCountryComboBox","arrivingCountryComboBox","RegionComboBox","departingRegionComboBox","arrivingRegionComboBox","AirportComboBox","departingAirportComboBox","arrivingAirportComboBox","The Routes function uses the SWT GridLayout for displaying tabular results. ","In the following screen shot, the Country and Region selections have been used to return a list of airport codes and their equivalent airport names in a SWT GridLayout.","Routes Form",
"routes_form.html");
Page[5]=new Array("The values in the Departing and Arriving Country combo boxes are returned by a query from the database using the AirportDAO class in the com.ingres.demoapp.persistence package.","Note: The following procedures assume that a combo box has been placed on the Visual Editor design form.","To load the Country combo box control:","Create the Country Data Access Object Class","Construct the Country Query","Generate the Country List","How You Load the Country Combo Box Control",
"load_country_combo_box_control.html");
Page[6]=new Array("The first step to loading the Country combo box control is to create the Country Data Access Object class. The CountryDAO class extends the DataSourceFactory class and adds the getCountries() method.","To create the Country Data Access Object class","Create the Country Data Access Object Class",
"create_country_dao_class.html");
Page[7]=new Array("The second step to loading the Country combo box control is to create the SQL statement for the Country query. The getCountries() method uses a static SQL statement to retrieve all of the countries from the database, ordered alphabetically by name. If you have installed the Ingres DTP plug-in for Eclipse, you can use an SQL file to experiment with the query as demonstrated below. (See the Eclipse SDK online help for instructions on installing plug-ins.)","Use the Data Source Explorer to browse to the country table and expand the Columns branch.","&nbsp;&nbsp;","The names of the columns you need to retrieve for the getCountries() method are displayed.","Check the SELECT statement by executing it in the scratch pad. You should see results similar to the following:","&nbsp;&nbsp;","Select the SQL statement and copy it into your source. You should see results similar to the following:","Construct the Country Query",
"construct_country_query.html");
Page[8]=new Array("The third step to loading the Country combo box control is to generate the Country list. The Country list is used by the initializeCountryCombo() method in the AirportWidget class.","Generate the Country List",
"generate_country_list.html");
Page[9]=new Array("The values in the Departing and Arriving Region combo boxes are returned by a query from the database using the getRegionsByCountry() method of the AirportDAO class in the com.ingres.demoapp.persistence package.","Note: The following procedures assume that a combo box has been placed on the Visual Editor design form. ","To load the Region combo box control:","Create the Airport Data Access Object Class","Construct the Region Query","Generate the Region List","How You Load the Region Combo Box Control",
"load_region_combo_box_control.html");
Page[10]=new Array("The first step to loading the Region combo box control is to create the Airport Data Access Object class.The AirportDAO class extends the DataSourceFactory class and adds the method getRegions().","The getRegions() method returns a list of regions of the country whose country code is passed in as a parameter.","Create the Airport Data Access Object Class",
"create_airport_dao_class.html");
Page[11]=new Array("The second step to loading the Region combo box control is to construct the SQL statement for the Region query. The getRegions() method demonstrates using a dynamic SQL statement with parameters to pass in a restriction on the retrieved data set (only the regions in the specified country should be returned.)","If you have installed the Ingres DTP plug-in for Eclipse, you can use an SQL file to experiment with the query as demonstrated below. (See the Eclipse SDK online help for instructions on installing plug-ins.)","Use the Data Source Explorer to browse to the airport table and expand the Columns branch.","&nbsp;&nbsp;An inspection of the columns in the airport table should lead you to construct the following query:","The restriction in the WHERE clause limits the regions to those in the US. The DISTINCT keyword ensures that no duplicate regions appear, and the ORDER BY clause sorts the regions so they appear in the combo box in sorted order.","Check the SELECT statement by executing it in the scratch pad. You should see results similar to the following:","You should now parameterize the Region query to account for the country code.","Construct the Region Query",
"construct_region_query.html");
Page[12]=new Array("The Region query you created must take the country code as a parameter. You do this by replacing the parameter in the query with a &quot;?&quot; symbol. The query cannot be executed directly; it must be prepared and then the appropriate value for the Country restriction added at run time. The methods to accomplish this are prepareStatement() and setString(). The prepareStatement() method allows you to create an object with a parameterized query that can be used many times. The setString() method allows you to set the parameter values of the statement for each execution. ","In this example, the following calls are used:","&nbsp;&nbsp;","The results should look similar to the following:","Parameterize the Region Query",
"parameterize_region_query.html");
Page[13]=new Array("The third step to loading the Region combo box control is to generate the Region list. The combo box method that generates the region entries is the event handler that is invoked when the user selects a country in the country combo box.","Generate the Region List",
"generate_region_list.html");
Page[14]=new Array("The Ingres Frequent Flyer demonstration application uses models to maintain local copies of data retrieved from the database. The UserProfile class includes storage for the following information:","First Name","Last Name","Email Address","Airport Code (for the home airport)","Photograph ","The model is updated by the UI and the updates are then reflected in the database. When reading information from the database, the application first updates the model and then presents the information to the user.","User Profile Model",
"user_profile_model.html");
Page[15]=new Array("A user may display or modify personal profile information using the Profile form. The Profile function shows how information stored in the database can be represented in user interface controls.","The user_profile table stores profile information and contains a column of type LongVarBinary to hold binary large objects (BLOBs). The application stores the user's photograph as a BLOB. ","User names are stored in a Unicode character column. To display the full range of Unicode characters, you must install the appropriate fonts on your machine. ","Profile information is displayed in the result panel ProfileResult View. ","Profile information is gathered from the user with the UserProfileModifyEditor (Profile) form.","The Profile form contains two groups of buttons that perform the following functions:","Group 1: Use these buttons to create or modify a user profile.","New","Clears all entries in the form and enables the Add button.","Modify","Changes the Email Address control from a text field into a combo box list field. The list in the combo box reflects all entries stored in the user_profile table.","Group 2: Use these buttons to update the database with the form details.","Add","Executes an INSERT using values from the form as parameters to the query.","Remove","Executes a DELETE using the value from the Email Address combo box to identify the row in the database to be deleted.","Change","Executes an UPDATE using the values from the form as parameters to the query.","Profile Form",
"profile_form.html");
Page[16]=new Array("The Email Address combo box can assume two styles depending on the mode of the Profile form: ","When adding a new profile, the user enters the email address as text in the Email Address control. ","When modifying an existing profile, the user selects an email address from the Email Address combo box list control where the list of addresses is retrieved from the user_profile table.","To load the Email Address combo box control:","Create the User Profile Data Access Object Class","Construct the Email Query","Generate the Email List","Add a Listener to the Email Address Combo Box","How You Load the Email Address Combo Box Control",
"load_email_address_combo_box.html");
Page[17]=new Array("The first step to loading the Email Address combo box control is to create the User Profile Data Access Object class. The UserProfileDAO class extends the DataSourceFactory class and adds, among others, the getEmailAddresses() method, which is used to populate the email combo box when the user selects the Modify button.","To create the User Profile Data Access Object class:","Create the User Profile Data Access Object Class",
"create_user_profile_dao_class.html");
Page[18]=new Array("The second step to loading the Email Address combo box control is to create the SQL statement for the Email query. The getEmailAddresses() method uses a static SQL statement to retrieve all of the email addresses from the database, ordered alphabetically by address. If you have installed the Ingres DTP plug-in for Eclipse, you can use an SQL file to experiment with the query as demonstrated below. (See the Eclipse SDK online help for instructions on installing plug-ins.)","Use the Data Source Explorer to browse to the user_profile table and expand the Columns branch. ","The names of the columns you need to retrieve for the method are displayed.","Check the SELECT statement by executing it in the Eclipse scratch pad. You should see results similar to the following:","Select the SQL statement and copy it into your source. You should see results similar to the following:","Construct the Email Query",
"construct_email_query.html");
Page[19]=new Array("The third step to loading the Email Address combo box control is to generate the Email list.The Email Address combo box control is loaded in the UserProfileModifyEditor initialize() method. This method uses getEmailAddresses() in the UserProfileDAO class to return a list of email addresses from the database:","Generate the Email List",
"generate_email_list.html");
Page[20]=new Array("The fourth step to loading the Email Address combo box is to add a listener for the SelectionChangedEvent to the Email Address Combo box. The listener uses the getUserProfile() method of the UserProfileDAO class to retrieve the user profile details from the database. This method is shown below:","The SQL statement used in this method is:","SELECT up_last, up_first, up_email, up_airport, up_image","FROM user_profile","WHERE up_email = ?","This is a prepared statement with a placeholder of &quot;?&quot; which is replaced at run time by the value of the email address selected by the user from the combo box.","Add a Listener to the Email Address Combo Box",
"add_listener_email_combo_box.html");
Page[21]=new Array("Clicking the Add button in the application triggers the execution of SelectionListener of UserProfileNewEditor addButton. ","If the new profile contains a photograph (stored as a BLOB type for this demonstration application), the image is read as an input stream into a byte array. This is done so the image can be stored by the setImage() method of the UserProfile class prior to being saved to the database.  This process is achieved by the following code extract:","Inserting the  BLOB type uses the SQL INSERT statement and the setBytes() method of the PreparedStatement class prior to sending the image to the database for storage as shown in the following code extract:","The User Profile model is updated using the text from the edit form in the UserProfileModifyEditor as follows:","The userProfile object is then passed to the data access update() method:","How You Add a New Profile",
"add_new_profile.html");
Page[22]=new Array("Clicking the Change button in the application triggers the execution of the SelectionListener of the ChangeButton. This in turn calls the doSave() method of the UserProfileModifyEditor class.","The demonstration Update function performs the following tasks:","Retrieves the values for a particular email identifier and updates the User Profile Model and edit form ","Reads the (new) values from the edit form and updates the values in the User Profile Model ","Uses the values in the User Profile Model to update the user_profile table","The doSave() method reads the image file and converts it to an image using the following code extract:","Images are updated using the setBytes() method and profile information is updated using the setString() method. Both methods use an SQL UPDATE statement as shown in the Eclipse editor:"," The update() method of the UserProfileDAO class performs the database update using the email address to key into the user_profile table:","How You Change a Profile",
"change_profile.html");
Page[23]=new Array("Clicking the Remove button in the application triggers the execution of the SelectionListener of the removeButton in the UserProfileModify class. The delete() method of the UserProfileDAO class is called to remove the profile information from the database.","The SQL statement used is:","The delete operation is keyed on the user email address, which is the primary key for the user_profile table. The following code performs the delete function:","How You Remove a Profile",
"remove_profile.html");
Page[24]=new Array("The Ingres Instance Data Source form is used to change the settings for a connection string used by the JDBC driver to connect to an Ingres Instance.","The details entered on the Ingres Instance Data Source form are stored in the store() method of the DatabaseConfigurationDAO class. These details are used whenever the application connects to a database using the getConnection() method of the DataSource class.","The parameters in the database configuration are as follows:","Host name","Specifies the name or address of the target host. Ingres uses the server name &quot;(local)&quot; for servers on the local host.","Instance name","Specifies the unique identifier for the Ingres instance.","The default instance name is Ingres II. To connect to a remote instance, ask your system administrator for the instance name.","Java and .NET based applications connect to Ingres through the Ingres Data Access Server (DAS). The DAS can be configured to listen on a specific Port ID. The default Port ID is the digit 7 appended to the instance ID. Therefore, the default DAS Port ID for the default instance Ingres II is II7.","Database ","Target database name. The name of the demonstration database is demodb.","User ID ","The user ID of the user connecting to the Ingres instance.","Login credentials are required only when connecting to a remote Ingres database or connecting with different credentials.","Password ","The password associated with the specified user ID for connecting to the Ingres instance.","Login credentials are required only when connecting to a remote Ingres database or connecting with different credentials. ","The Database Configuration Editor is used to design the Ingres Instance Data Source form. The example screen below shows the configuration of the Host name parameter.","Ingres Instance Data Source Form",
"ingres_instance_data_source_form.html");
Page[25]=new Array("Clicking the Change button in the application invokes the SelectionListener. The text from the Ingres Instance Data Source form is collected in the databaseConfiguration object which makes the text available to the rest of the application. If the &quot;Make defined source default&quot; check box is enabled, the configuration information is permanently stored.","How You Change the Ingres Instance",
"change_ingres_instance.html");
Page[26]=new Array("The getDataSource method of the DataSourceFactory class sets up the connection parameters for the database and returns a dataSource object.","The getDataSource() Method",
"getdatasource_method.html");
var PageCount=27;

function search(SearchWord){
var Result="";
var SearchResultsTable="";
var NrRes=0;
var tmppara="";
Result='<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">\n';
Result+="<html>\n";
Result+="<head>\n";
Result+="<title>Search Results</title>\n";
Result+="<meta http-equiv='Content-Type' content='text/html; charset=ISO-8859-1'>\n";
Result+='<script language="javascript" type="text/javascript" src="dhtml_search.js"></script>\n';
Result+='<link rel="stylesheet" type="text/css" href="docsstylesheet.css">\n';
Result+="<style type='text/css'>\n";
Result+=".searchDetails {font-family:verdana; font-size:8pt; font-weight:bold}\n";
Result+=".searchResults {font-family:verdana; font-size:8pt}\n";
Result+="</style>\n";
Result+="</head>\n";
Result+="<body onload='javascript:document.SearchForm.SearchText.focus()'>\n";
Result+='<table class="searchDetails" border="0" cellspacing="0" cellpadding="2" width="100%">\n';
Result+='<tr><td>Search For:</td></tr>';
Result+='<tr><td>';
Result+='<form name="SearchForm" action="javascript:search(document.SearchForm.SearchText.value)">';
Result+='<input type="text" name="SearchText" size="25" value="' + SearchWord + '" />';
Result+='&nbsp;<input type="submit" value="&nbsp;Go&nbsp;"/></form>';
Result+='</td></tr></table>\n';
if(SearchWord.length>=1)
{
   SearchWord=SearchWord.toLowerCase();
   while(SearchWord.indexOf("<")>-1 || SearchWord.indexOf(">")>-1 || SearchWord.indexOf('"')>-1){
       SearchWord=SearchWord.replace("<","&lt;").replace(">","&gt;").replace('"',"&quot;");
   }
   this.status="Searching, please wait...";BeginTime=new Date();
   SearchResultsTable+="<table border='0' cellpadding='5' class='searchResults' width='100%'>";
   for(j=0;j<PageCount;j++){
       k=Page[j].length-1;
       LineNr=0;
       for(i=0;i<k;i++){
           WordPos=Page[j][i].toLowerCase().indexOf(SearchWord);
           if(WordPos>-1){
               FoundWord=Page[j][i].substr(WordPos,SearchWord.length);
               NrRes++;
               SearchResultsTable+="<tr><td>";
               /* add title */
               tmppara = Page[j][k-1];
               newpara = '';
               WordPos = tmppara.toLowerCase().indexOf(SearchWord);
               while (WordPos >= 0) {
                 if (WordPos > 0) { 
                    newpara += tmppara.substr(0,WordPos);
                    tmppara = tmppara.substr(WordPos);
                 }
                 newpara += tmppara.substr(0,FoundWord.length).bold();
                 tmppara = tmppara.substr(FoundWord.length);
                 WordPos = tmppara.toLowerCase().indexOf(SearchWord);
               }   
               newpara += tmppara; 
               SearchResultsTable+="<a target='BODY' href='"+Page[j][k]+"'>"+newpara+"</a><br/>\n";              
               if(i<k-1){
                   /* shorten topic text */
                   if (Page[j][i].length>350){
                      before = '...';
                      tmppara = Page[j][i].substr(WordPos-100,200+FoundWord.length); 
                      after = '...';
                   }
                   else{
                      before = '';
                      tmppara = Page[j][i];
                      after = '';
                   }
                   /* add topic text */
                   newpara = '';
                   WordPos = tmppara.toLowerCase().indexOf(SearchWord);
                   while (WordPos >= 0) {
                     if (WordPos > 0) { 
	                newpara += tmppara.substr(0,WordPos);
                        tmppara = tmppara.substr(WordPos);
                     }
                     newpara += tmppara.substr(0,FoundWord.length).bold();
                     tmppara = tmppara.substr(FoundWord.length);
                     WordPos = tmppara.toLowerCase().indexOf(SearchWord);
                   }   
                   newpara += tmppara;               
                   SearchResultsTable+=before+newpara+after+"\n";
               }
               SearchResultsTable+="</td></tr>";
               break;
           }
       }
   }
   SearchResultsTable+="</table>";  
   Result+="<p class='searchDetails'>&nbsp;" + NrRes + " result(s) found.</p>"+SearchResultsTable;
   if (NrRes > 5) {
    Result+="<p class='searchDetails'>&nbsp;" + NrRes + " result(s) found.</p>";
   }
   SearchResultsTable="";
   Result+="<p class='searchDetails'>&nbsp;<a href='dhtml_search.htm' target='TOC'>New Search</a></p>"; 
}
Result+="</body></html>";
this.status="";
this.document.open();
this.document.write(Result);
this.document.close();
}

