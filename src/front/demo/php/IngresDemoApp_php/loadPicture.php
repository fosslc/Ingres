<?php
	require_once("models/ProfileModel.php");
	@session_start();
	if(isset($_SESSION['email'])) {	
		$pm = new ProfileModel();
		$picture = $pm->getPicture($_SESSION['email']);	
		header("Cache-Control: no-cache, must-revalidate");
		header("Content-Type: image/jpeg");
		echo $picture;
	}
?>
