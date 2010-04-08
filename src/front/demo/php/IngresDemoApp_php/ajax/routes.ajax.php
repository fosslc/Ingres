<?php
/*
 * Copyright (c) 2007 Ingres Corporation
 */

/*
 * Name:	routes.ajax.php	
 *
 * Description: evaluates form data of routesCriteria.php and get's routes data from RoutesModel
 *		to create html code. This code is assigned to the respective DOM Node.
 *
 * History:	14-Feb-2007 (michael.lueck@ingres.com)
 *		Created
 *
 */
	/*
	file must be included by index.php
	*/
	require_once("airport.ajax.php");
	require_once("models/RoutesModel.php");
	global $xajax;
	
	$xajax->registerFunction("doSearch");
	
	
	function doSearch($formValues)
	{
		$res = new xajaxResponse();
		$inputsMissing = false;
		//do check wheter all needed parameters have been set before klicking GO
		if($formValues['dep_country_select'] == "invalid") $inputsMissing = true;
		elseif($formValues['arv_country_select'] == "invalid") $inputsMissting	= true;
		elseif($formValues['arv_region_select'] == "invalid") $inputsMissing = true;	
		elseif($formValues['dep_region_select'] == "invalid") $inputsMissing = true;
		elseif($formValues['arv_apcode_select'] == "invalid") $inputsMissing = true;
		elseif($formValues['dep_apcode_select'] == "invalid") $inputsMissing = true;
	
		if($inputsMissing)
		{
			$res->addAssign("contentView","innerHTML","At least one of the specified arguments is invalid");
			return $res;
		}

		$flightDays = "%%%%%%%";
		if(!isset($formValues['flyOnAnyDay']))
		{
			/*checkbox fly on any Day is not checked*/
				foreach($formValues['weekDay'] as $day)	
				{
					$flightDays[$day - 1] = $day;
				}	
		}			

		$returnRoute=false;
		if(isset($formValues['return_route']))
			$returnRoute = true;

		$rtModel = new RoutesModel();
		$searchResults = $rtModel->doSearch($formValues['dep_apcode_select'],
						    $formValues['arv_apcode_select'],
						    $flightDays,
						    $returnRoute);
		//Got the results. Now make a table and show results within that table in contentView div
		$resultTable = "<table border=\"1\" cellpadding=\"0\" cellspacing=\"0\">\n";
		$resultTable .= "<thead<tr><th>Airline</th><th>IATA Code</th><th>Flight Number</th><th>Depart From</th>\n";
		$resultTable .= "<th>Arrive To</th><th>Departure Time</th><th>Arrival Time</th><th>Days later</th><th>On Days</th>\n";
		$resultTable .= "</tr></thead><tbody>";
		foreach($searchResults as $key => $result)
		{
			$resultTable .= "<tr>";	
			foreach($result as $resultKey => $resultValue)
				$resultTable .= "<td style=\"text-align:center;\">".$resultValue."</td>";
			$resultTable .= "</tr>\n";
		}
		$resultTable .= "</tbody></table>\n";

		$res->addAssign("contentView","innerHTML",$resultTable);
		return $res;	
	}
?>
