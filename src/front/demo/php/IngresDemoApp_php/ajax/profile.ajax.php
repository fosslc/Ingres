<?php
/*
 * Copyright (c) 2007 Ingres Corporation
 */

/*
 * Name:	profile.ajax.php	
 *
 * Description: encapsulates the ajax functions for the profile View. The functions get their required data from
 *		the model classes. They create HTML code with this data and assign it to the respective DOM Node
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
	require_once("models/ProfileModel.php");
	global $xajax;
	
	$xajax->registerFunction("loadValuesForNewProfile");
	$xajax->registerFunction("loadEmail");
	$xajax->registerFunction("loadCredentials");
	$xajax->registerFunction("deleteTempFiles");
	$xajax->registerFunction("deleteProfile");
	
	function loadValuesForNewProfile()
	{
		$rt = new RoutesModel();
		$countrys = $rt->getCountrys();
		//generate new select with onChange eventhandler to xajax_fillRegion
		$select = "<select tabindex=\"5\" name=\"home_country_select\" "
				."onChange=\"xajax_fillRegion('home_region',this.options[this.selectedIndex].value)\">\n";
	
		$select .= "<option value=\"invalid\">Select Value ...</option>\n";
		foreach($countrys as $country)
		{
			$select .= "<option value=\"".$country['ct_code']."\">".$country['ct_name']."</option>\n";	
		}
		$select .="</select>\n";
		$res = new xajaxResponse();
		$res->addAssign("home_country", "innerHTML", $select);
		$res->addAssign("mailaddress","innerHTML","<input tabindex=\"1\" type=\"text\" id=\"mailaddressText\" name=\"mailaddressText\"".
				" size=\"40\" maxlength=\"40\">");
		$res->addAssign("contentView","innerHTML","");
		return $res;
	}
	
	function loadEmail()
	{
		$pm = new ProfileModel();
		$mailaddresses = $pm->getMailAddresses();
		$select = "<select tabindex=\"1\" name=\"mailaddressText\" id=\"mailaddressText\""
				." onChange=\"xajax_loadCredentials(this.options[this.selectedIndex].value)\">";
		$select .= "<option value\"invalid\">Select Value ... </option>\n";
		foreach($mailaddresses as $mailadress)
		{
			$select .= "<option value=\"".$mailadress['up_email']."\">".$mailadress['up_email']."</option>\n";
		}
		$select .= "</select>\n";
		$res = new xajaxResponse();
		$res->addAssign("mailaddress","innerHTML",$select);
		return $res;
	}
	
	function loadCredentials($mailadress)
	{
		$res = new xajaxResponse();
		$pm = new ProfileModel();
		try
		{
			$profile = $pm->getProfileByEmail($mailadress);
		} 
		catch( Exception $e)
		{	
			die($e->getMessage());
			if($e->getCode() == 1)
			{	
				//no profile found for given mailadress => reload Mailaddress Drop Down List
				$res->addScriptCall("setModifyMask");		
				$res->addAssign("contentView","innerHTML","Chosen profile dont't exist. Probably deleted by another user");
				return $res;
			} elseif( $e->getCode() == 6) {
				$res->addAssign("contentView","innerHTML",$e->getMessage());
			} else {
				throw $e;
			}
			
		}
		unset($pm);
		$res->addAssign("firstname","value",html_entity_decode($profile->first,ENT_QUOTES,"UTF-8"));
		$res->addAssign("lastname","value",html_entity_decode($profile->last,ENT_QUOTES,"UTF-8"));
		
		$rm = new RoutesModel();
		//Make the country Select and select the entry from the profile by default
		$countrys = $rm->getCountrys();
		$select = "<select tabindex=\"5\" id=\"home_country_select\" name=\"home_country_select\""
				."onChange=\"xajax_fillRegion('home_region',this.options[this.selectedIndex].value)\">\n";
		foreach($countrys as $country)
		{
			if($country['ct_name'] == $profile->country)
				$select .= "<option value=\"".$country['ct_code']."\" selected>".$country['ct_name']."</option>\n";
			else
				$select .= "<option value=\"".$country['ct_code']."\">".$country['ct_name']."</option>\n";
		}
		$res->addAssign("home_country","innerHTML",$select);

		//Make the region select and select the entry from the profile by default
		$regions = $rm->getRegionByCountry($profile->ccode);
		$regionSelect = "<select tabindex=\"6\" id=\"home_region_select\" name=\"home_region_select\" "
					."onChange=\"xajax_showAirports('home_apcode',".$profile->ccode.",".$profile->ap_place.")\">\n";
		foreach($regions as $region)
		{
			if($region['ap_place'] == $profile->ap_place)
			 	$regionSelect .= "<option selected>".$region['ap_place']."</option>\n";
			else
				$regionSelect .= "<option>".$region['ap_place']."</option>\n";
		}
		$regionSelect .= "</select>\n";
		$res->addAssign("home_region","innerHTML",$regionSelect);
		
		//Make the Airport-Code Select and select the entry from the profile by default
		$airports = $rm->getAirports($profile->ccode, $profile->ap_place);
		$apSelect = "<select tabindex=\"7\" id=\"home_apcode_select\" name=\"home_apcode_select\">\n";
		foreach($airports as $airport)
		{
			$apSelect .= "<option>".$airport['ap_iatacode']."</option>\n";	
		}
		$apSelect .= "</select>\n";
		
		$res->addAssign("home_apcode","innerHTML",$apSelect);

		//Show the picture in the contentView diff
		$content = "<br/>\n<table style=\"text-align:left;\" border=\"0\"";
		$content .= " cellspacing=\"0\" cellpadding=\"0\" width=\"100%\">\n<tbody>\n";
		$content .= "<tr><td class=\"topField\" colspan=\"3\"><h2>Welcome ".
				$profile->first."</h2><br/></td></tr>\n<tr valign=\"bottom\">\n<td class=\"leftField\" rowspan=\"2\"";
		if($profile->image == null)
		{
			$size = getimagesize("pictures/NoPicture.bmp");
			$content .="width=\"".$size[0]."\"><img src=\"pictures/NoPicture.bmp\">";
		} else {	
			/*$path="pictures/".session_id();
			if(!file_exists($path)) 
				mkdir($path);
		
			$file= $path."/".$profile->email;
			$handle = fopen($file,"w");
			fwrite($handle,$profile->image);
			fclose($handle);
			$size = getimagesize($file);
			$content .= "width=\"".$size[0]."\"> ";
		//	$content .= "<img src=\"".$file."\">";*/
			@session_start();
			$_SESSION['email'] = $profile->email;
			srand(time());
			$content .= "<img src=loadPicture.php?".rand().">";
		}
		$content .="</td><td style=\"padding-left:20px\">Email Adress</td><td class=\"rightField\"><strong>".
				$profile->email."</strong></td></tr>\n";
		$content .="<tr valign=\"top\"><td style=\"padding-left:20px;\">Airport Preference</td><td class=\"rightField\"><strong>".
				$profile->airport."</strong></td></tr>";
		$content .="<tr><td class=\"bottomField\" colspan=\"3\">&nbsp;</td></tr>";
		$content .="</tbody></table>\n";
		$res->addAssign("contentView","innerHTML",$content);
		return $res;
	}
	
	//this function is the event handler for onunload Event at the body tag in index.php 
	function deleteTempFiles()
	{
		/*@session_start();
		$path = "pictures/".session_id();
		if(file_exists($path))
		{
			if(!is_dir($path))
				unlink($path);
			else
			{
				$dir= opendir($path);
				while($file=readdir($dir))
				{	
					if(is_dir($path."/".$file)) continue;
					unlink($path."/".$file);
				}
				closedir($dir);
				rmdir($path);
			}
		}*/
		$res = new xajaxResponse();
		return $res;
	}
	
	function deleteProfile($aFormValues)
	{
		$pm = new ProfileModel();
		$res = new xajaxResponse();
		if(isset($aFormValues['mailaddressText']))
		{	
			$mailaddress = $aFormValues['mailaddressText'];
			try
			{
				$pm->delete($aFormValues['mailaddressText']);
			} catch (Exception $e) {
				if($e->getCode() == 6){
					$res->addAssign("contentView","innerHTML","Error:".$e->getMessage());
					return $res;
				} else {
					throw $e;
				}
			}
		} else {
			return $res;
		}
		if(isset($_COOKIE['defaultProfile']) && $_COOKIE['defaultProfile'] == $aFormValues['mailaddressText'])
			setcookie("defaultProfile","",time() - 3600,dirname($_SERVER['PHP_SELF']),$_SERVER['SERVER_NAME']);
		if(isset($_SESSION['defaultProfile']) && $_SESSION['defaultProfile'] == $aFormValues['mailaddressText'])
			unset($_SESSION['defaultProfile']);
		$res->addScriptCall("setModifyMask");
		$res->addAssign("home_country_select","innerHTML","");
		$res->addAssign("home_apcode_select","innerHTML","");
		$res->addAssign("home_region_select","innerHTML","");
		$res->addAssign("contentView","innerHTML","Profile ".$mailaddress." deleted");
		return $res;
	}
?>
