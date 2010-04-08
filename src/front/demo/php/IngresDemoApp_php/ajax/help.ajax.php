<?php
/*
 * Copyright (c) 2007 Ingres Corporation
 */
	global $xajax;
	
	$xajax->registerFunction("showNextHelp");

	function showNextHelp($nextpage) {
		$res = new xajaxResponse();
		$iframe = "<iframe src=\"help/".$nextpage."\" name=\"helpIframe\">Your Browser doesn't support iframes</iframe>";
		$res->addAssign("contentView","innerHTML",$iframe);
		return $res;
	}

?>
