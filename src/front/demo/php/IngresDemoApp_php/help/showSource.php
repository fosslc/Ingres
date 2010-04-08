<?php
	$allowedfiles = array(
			"connect" => "../connect.php",
			"profile" => "../profile.php",
			"routes" => "../routesCriteria.php",
			"airportAJAX" => "../ajax/airport.ajax.php",
			"connectAJAX" => "../ajax/connect.ajax.php",
			"profileAJAX" => "../ajax/profile.ajax.php",
			"routesAJAX" => "../ajax/routes.ajax.php",
			"ProfileModel" => "../models/ProfileModel.php",
			"RoutesModel" => "../models/RoutesModel.php",
			"DBConnection" => "../database/DBConnection.php",
			"index" => "../index.php"
			);
	$source = "";
	if(isset($_GET['file']) && array_key_exists($_GET['file'],$allowedfiles)) {
		$source	= htmlentities(file_get_contents($allowedfiles[$_GET['file']]));
		echo "<pre>".$source."</pre>";
	}else {
		echo "showing source for specified file not possible";
	}

?>
