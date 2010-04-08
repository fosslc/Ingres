<?php
/*
 * Copright (c) 2007 Ingres Corporation
 */

/* Name: airport.ajax.php
 *
 * Description: contains functions that create html strings for the select fields of
 *		country, region and airport code. It get's the required data from the models
 *		ProfileModel.php and RoutesModel.php.
 *		After the function has generated the html string it assigns this string to
 *		the innerHTML Attribute of div tags. These are set in the html templates
 *		where the xajax function is called. For example fillRegion is called by the 
 *		select fields dep_country_select and arv_country_select in routesCriteria.php
 *		or from home_country_select in profile.php. Look for the call xajax_fillRegion there
 *
 * History:	14-Feb-2007 (michael.lueck@ingres.com)
 *		Created
 */
	/*
	file must be included by index.php
	*/
	require_once("models/RoutesModel.php");
	require_once("models/ProfileModel.php");
	global $xajax;

	function makeSelect($name="",$id="",$onChange="",
			    $options = array(array("innerValue" => "", "attributeValue" => "")),$selected=array(),
			    $tabindex="" )
	{	
		$select = "<select";
		if($tabindex != "")
			$select .=" tabindex=\"".$tabindex."\"";
		if($name != "")
			$select .= " name=\"".$name."\"";
		if($id != "")
			$select .= " id=\"".$id."\"";
		if($onChange != "")
			$select .= " onChange=\"".$onChange."\"";
		
		$select .= ">\n";
		
		foreach($options as $option)
		{
			$select .= "<option";
			if( $option['attributeValue']!= "" )
				$select .= " value=\"".$option['attributeValue']."\"";
			if(in_array($option['innerValue'],$selected))
				$select .= " selected=\"selected\"";
			$select .= ">".$option['innerValue']."</option>\n";
		}
		
		$select .= "</select>\n";
		return $select;
	}
	function getRegionSelect($targetId, $ccode, $selected="")
	{
		$rtModel= new RoutesModel();
		if($targetId == "dep_region") { $ApCodeDivId="dep_apcode"; $tabindex=2; }
		if($targetId == "arv_region") { $ApCodeDivId="arv_apcode"; $tabindex=5; }
		if($targetId == "home_region"){ $ApCodeDivId="home_apcode"; $tabindex=6; }
		
		$regions = $rtModel->getRegionByCountry($ccode);
	
		$options = array();
		$options[] = array("innerValue" =>"Select Value ...", "attributeValue" => "invalid");
		foreach( $regions as $region )
		{	
			$options[] = array("innerValue" => $region['ap_place'], "attributeValue" => $region['ap_place']);
		}
		
		$onChange ="xajax_showAirports('".$ApCodeDivId."','".$ccode."',this.options[this.selectedIndex].value)";
		$select = makeSelect($targetId."_select",$targetId."_select",$onChange,$options,array($selected),$tabindex);

		return $select;

	}
	function getCountrySelect($targetId, $selected="")
	{
		if($targetId== "dep_country") { $regionDivId = "dep_region"; $tabindex=1; }
		if($targetId== "arv_country") { $regionDivId = "arv_region"; $tabindex=4; }
		$rtModel = new RoutesModel();
		$countrys = $rtModel->getCountrys();
		$options = array();
		$options[] = array("innerValue"=>"Select Value ...", "attributeValue"=>"invalid");
		foreach($countrys as $country)
		{
			$options[] = array("innerValue" => $country['ct_name'], "attributeValue" => $country['ct_code']);
		}
		
		$onChange = "xajax_fillRegion('".$regionDivId."', this.options[this.selectedIndex].value)";
		$select = makeSelect($targetId."_select",$targetId."_select",$onChange,$options,array($selected),$tabindex);
		return $select;
	}
	
	function getApCodeSelect($targetId,$ccode, $region, $selected="")
	{
		if($targetId == "dep_apcode") $tabindex=3;
		if($targetId == "arv_apcode") $tabindex=6;
		if($targetId == "home_apcode") $tabindex=7;
		$rtModel = new RoutesModel();
		$airports = $rtModel->getAirports($ccode,$region);
		$options=array();
		foreach($airports as $airport)
		{
			$options[] = array("innerValue" => $airport['ap_iatacode'], "attributeValue" => $airport['ap_iatacode']);
		}
		$select = makeSelect($targetId."_select",$targetId."_select","",$options,array($selected),$tabindex);
		return $select;

	}
	
	$xajax->registerFunction("loadInitialData");
	$xajax->registerFunction("fillRegion");
	$xajax->registerFunction("showAirports");

	function loadInitialData()
	{
		$res = new xajaxResponse();
		$regionSelect="<select disabled=\"disabled\"><option>Select Value ...</option></select>\n";
		$apcodeSelect="<select disabled=\"disabled\"><option>Select Value ...</option></select>\n";
		//if no default Profile is set in the SESSION we only need to load the Countrys
		if(!isset($_SESSION['defaultProfile']))
		{
			$depSelect = getCountrySelect("dep_country","Select Value ...");

			$res->addAssign("dep_country","innerHTML",$depSelect);
			$res->addAssign("dep_region","innerHTML",$regionSelect);
			$res->addAssign("dep_apcode","innerHTML",$apcodeSelect);
		} else {
			// Load the airport information from the profile and set all
			// relevant selects of the departure airport to that values
			$pm = new ProfileModel();
			try {
				$airportInfos = $pm->getAirportInfoByEmail($_SESSION['defaultProfile']);
			} catch (Exception $e) {
				if($e->getCode() == 6)
				{
					$res2 = new xajaxResponse();
					$res2->addAssign("contentView","innerHTML",$e->getMessage());
					return $res2;
				}
			}
			unset($pm);
			$depCountrySelect = getCountrySelect("dep_country",$airportInfos[0]['country']);
			$depRegionSelect  = getRegionSelect("dep_region",$airportInfos[0]['ccode'],$airportInfos[0]['region']);
			$depApCodeSelect  = getApCodeSelect(	"dep_apcode",
								$airportInfos[0]['ccode'],
								$airportInfos[0]['region'],
							 	$airportInfos[0]['iatacode']);
			$res->addAssign("dep_country","innerHTML",$depCountrySelect);
			$res->addAssign("dep_region","innerHTML",$depRegionSelect);
			$res->addAssign("dep_apcode","innerHTML",$depApCodeSelect);
		}
		
		$arvSelect = getCountrySelect("arv_country");	
		$res->addAssign("arv_country","innerHTML",$arvSelect);
		$res->addAssign("arv_region","innerHTML",$regionSelect);
		$res->addAssign("arv_apcode","innerHTML",$apcodeSelect);
		//$res->addAssign("contentView","innerHTML","");
		return $res;
	}
	
	function fillRegion($targetId, $ccode)
	{
		$select = getRegionSelect($targetId, $ccode);
		$res = new xajaxResponse();
		$res->addAssign($targetId,"innerHTML",$select);
		$help = "<iframe src=\"help/region_load.htm\"></iframe>";
		$res->addAssign("contentView","innerHTML",$help);
		return $res;
	}

	function showAirports($targetId,$ap_ccode, $ap_place)
	{
		/*
		 * After a Airport is chosen in the "region" select, the IATA-Code and the
		 * Airport-Name must be displayed in the part of the screen below the
		 * routes criteria input fields. Also the select "Airport Code" must be filled
		 * with the IATA-Code of the Airport chosen through "region"
		 */
		
		$res = new xajaxResponse();
		$select = getApCodeSelect($targetId,$ap_ccode,$ap_place);
		$res->addAssign($targetId,"innerHTML",$select);
		//show table with results in contentView div
		$rtModel = new RoutesModel();
		$airports = $rtModel->getAirports($ap_ccode,$ap_place);
		$contentView  ="<table width=\"100%\" border=\"1\" cellpadding=\"0\" cellspacing=\"0\"><tbody>\n";
		$contentView .="<tr><td><strong>IATA Code</strong></td><td><strong>Airport Name</strong></td></tr>\n";
		foreach($airports as $airportRow)
		{
			$contentView .= "<tr><td width=\"50%\">".$airportRow['ap_iatacode']."</td>
					     <td width=\"50%\">".$airportRow['ap_name']."</td></tr>\n";
		}
		$contentView .="\n</tbody></table>\n";
		$res->addAssign("contentView","innerHTML",$contentView);
		return $res;
	}
	
?>
