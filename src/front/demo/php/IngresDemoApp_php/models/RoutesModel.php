<?php
/*
 * Copyright (c) 2007 Ingres Corporation
 */

/*
 * Name:	RoutesModel.php
 *
 * Description: retrieves and sets data in database. the retrieved data is prepared for use in ajax functions
 *		uses DYNAMIC SELECT (in doSearch) and ROW PRODUCING PROCEDURES (getAirports)
 *
 * History:	14-Feb-2007 (michael.lueck@ingres.com)
 *		Created
 *
 */
	require_once("database/DBConnection.php");
	
	class RoutesModel{
	
	   //Membervariable to contain an instance of DBConnection	
	   private $DBConn;
	   		
	/*
	 * Name:	__construct
	 *
	 * Description: Constructor. creates new Instance of DBConnection and saves it in $DBConn
	 *
	 * Inputs:	None.
	 *
	 * Outputs: 	None.
	 *
	 * Returns:	instance of RoutesModel
	 *
	 * Exceptions: 	None.
	 */
	   function __construct()
	   {
		$this->DBConn = new DBConnection();
	   }
	
	/*
	 * Name:	getCountrys
	 *
	 * Description:	selects name and countrycode from table country and converts the values to UTF-8
	 *
	 * Inputs:	None.
	 *
	 * Outputs:	None.
	 *	
	 * Returns:	two dimensional array with country name and country code for each country
	 *
	 * Exceptions:	None.
	 */
	   function getCountrys()
	   {
		$this->DBConn->query("select ct_name, ct_code from country ORDER BY ct_name");
		$resultArray = $this->DBConn->getResultArray();
		$countryArray=array();
		foreach($resultArray as $key => $value)
		{
			$countryArray[$key]['ct_name'] = iconv("UTF-16LE","UTF-8",$value['ct_name']);
			$countryArray[$key]['ct_code'] = iconv("UTF-16LE","UTF-8",$value['ct_code']);
		}	
		return $countryArray;
		
	   }

	/*
	 * Name:	getRegionsByCountry
	 *
	 * Description: selects regions (ap_place) and country code for a specific country. Converts the results to UTF-8
	 *
	 * Inputs:	$ct_code	- 	ap_ccode for country for which the regions should be read from database
	 *
	 * Outputs:	None.
	 *
	 * Returns: 	two dimensional array	- each row contains array of region (ap_place) and countrycode (ap_ccode)
	 *
	 * Exceptions: 	None
	 */
	   function getRegionByCountry($ct_code) {

		$this->DBConn->query("SELECT DISTINCT ap_place, ap_ccode FROM airport WHERE ap_ccode='".$ct_code."' ORDER BY ap_place");
		$resultArray = $this->DBConn->getResultArray();
		$regionsArray = array();
		foreach($resultArray as $key => $regionRow)
		{
			$regionsArray[$key]['ap_place'] = iconv("UTF-16LE","UTF-8",$regionRow['ap_place']);
			$regionsArray[$key]['ap_ccode'] = iconv("UTF-16LE","UTF-8",$regionRow['ap_ccode']);
		}
		return $regionsArray;
	   }
		
	/*
	 * Name:	getAirports
	 *
	 * Description: calls stored procedure get_my_airports. this selects all airports in a specified 
	 *		country and an specified region. Converts the result values to UTF-8
	 *
	 * Inputs:	$ap_ccode	-	country code of the country where the procedure should search for airports 
	 *		$ap_place	-	region where the procedure should search for airports
	 *
	 * Outputs:	None.
	 *
	 * Returns:	two dimensional array	- every row consists of an array with the elements
	 *					  ap_iatacode, ap_place and ap_name. values are all in UTF-8
	 *
	 * Exceptions:	None
	 */
	   function getAirports($ap_ccode, $ap_place)
	   {
		
		$sqltypes = "nN"; /*nchar, nvarchar*/
		$params = array("ccode" => iconv("UTF-8","UTF-16LE",$ap_ccode), "area" => iconv("UTF-8","UTF-16LE",$ap_place));
		$this->DBConn->query("call get_my_airports(ccode = ?, area =?)",$params,$sqltypes);
		
		$resultArray = $this->DBConn->getResultArray();

		if(count($resultArray) == 0){
			 $ret=array('ap_iatacode' => 'not existing', 'ap_place' => $ap_place, 'ap_name' => $ap_place);
			 return $ret;
		}
		//TODO: count on result
		$airportArray = array();
		foreach($resultArray as $key => $airportRow)
		{
			$airportArray[$key]['ap_iatacode'] = iconv("UTF-16LE","UTF-8",$airportRow[0]);
			$airportArray[$key]['ap_place']    = iconv("UTF-16LE","UTF-8",$airportRow[1]);
			$airportArray[$key]['ap_name']	   = iconv("UTF-16LE","UTF-8",$airportRow[2]);
		}
		return $airportArray;
	   }
	
	/*
	 * Name:	doSearch
	 *
	 * Description:	selects routes and respective information the user searched for. Converts char-results to UTF-8
	 *		uses DYNAMIC SELECT (with placeholders)
	 *
	 * Inputs:	$depart_from	-	iatacode of the departure airport
	 *		$arrive_to	-	iatacode of the arrive airport
	 *		$flightDays	-	String of length 7 containing one digit for each weekday or % 
	 *		$returnTrip	-	bool, sign whether search should seek for a return trip too
	 *
	 * Outputs:	None	
	 *
	 * Returns:	two dimensional array	-	each row is an array of the elements rt_airline, al_iatacode,
	 *						rt_flight_num, rt_depart_from, rt_arrive_to, rt_depart_at,
	 *						rt_arrive_at,rt_arrive_offset,rt_flight_day where the string values
	 *						are converted to UTF-8
	 *
	 * Exceptions:	None
	 */
	   function doSearch($depart_from,$arrive_to,$flightDays,$returnTrip)
	   {
		$sql = ""; 
		$params = "";
		$sqltypes = "";

		if($returnTrip)
		{
			$sql = "SELECT 
					rt_airline, 
					al_iatacode, 
					rt_flight_num, 
					rt_depart_from, 
					rt_arrive_to, 
					rt_depart_at, 
					rt_arrive_at, 
					rt_arrive_offset, 
					rt_flight_day 
				FROM 
					route r, 
					airline a 
				WHERE 
					((rt_depart_from = ? and rt_arrive_to = ?) 
						OR (rt_depart_from = ? and rt_arrive_to = ?)) 
					and rt_flight_day LIKE ? 
					and rt_airline = al_icaocode 
				ORDER BY rt_airline, rt_flight_num";
			//fill an array with the paramters for the placeholders ("?" in select statement above)
			$params = array("rt_depart_from" => iconv("UTF-8","UTF-16LE",$depart_from),
					"rt_arrive_to"  => iconv("UTF-8","UTF-16LE",$arrive_to),
					"return_depart_from" => iconv("UTF-8","UTF-16LE",$arrive_to),
					"return_arrive_to"   => iconv("UTF-8","UTF-16LE",$depart_from),
					"flight_day" => iconv("UTF-8","UTF-16LE",$flightDays));
			//make string containing type information for the parameters above
			//They all are of type NCHAR 
			$sqltypes = "nnnnn";
		} else {
			$sql=	"SELECT 
					rt_airline, 
					al_iatacode, 
					rt_flight_num, 
					rt_depart_from, 
					rt_arrive_to, 
					rt_depart_at,
					rt_arrive_at, 
					rt_arrive_offset, 
					rt_flight_day 
				FROM 
					route r, 
					airline a 
				WHERE 
					rt_depart_from = ? 
					and rt_arrive_to = ? 
					and rt_flight_day LIKE ? 
					and rt_airline = al_icaocode 
				ORDER BY rt_airline, rt_flight_num";
			//make parameter array for placeholders (the order is important not the indizes of the array)
			$params = array("depart_from" => iconv("UTF-8","UTF-16LE",$depart_from),
					"arrive_to"   => iconv("UTF-8","UTF-16LE",$arrive_to),
					"flight_day"   => iconv("UTF-8","UTF-16LE",$flightDays));
			//all parameters are of type NCHAR
			$sqltypes = "nnn";
		}
	
		//call query with parameter array and sqltype mapping	
		$this->DBConn->query($sql,$params,$sqltypes);

		$resultArray = $this->DBConn->getResultArray(/*only associative array*/true);
	
		$routeResults = array();
		
		foreach($resultArray as $key => $value)
		{
			foreach($value as $fieldname => $fieldValue)
			{
				if($fieldname == "rt_airline"   || $fieldname == "al_iatacode" ||
				   $fieldname == "al_name"      || $fieldname == "rt_depart_from" ||
				   $fieldname == "rt_arrive_to" || $fieldname == "rt_flight_day")	
					$routeResults[$key][$fieldname] = iconv("UTF-16LE","UTF-8",$resultArray[$key][$fieldname]);
				else
					$routeResults[$key][$fieldname] = $resultArray[$key][$fieldname];
				
			}
		}
		
		return $routeResults;
	   }

	/*
	 * Name:	__destruct
	 *
	 * Description:	destructor
	 *	
	 * Inputs:	None.
	 *
	 * Outputs:	None.
	 *
	 * Returns:	None.
	 *
	 * Exceptions:	None.
	 */
	   function __destruct()
	   {

	   }

	}
?>
