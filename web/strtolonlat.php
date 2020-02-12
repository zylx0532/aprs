<?php

function strtolat($glat) {
	$lat = 0;
	$lastc = substr($glat, -1);
	$lat = substr($glat,0,2) + substr($glat,2,-1)/60;
	if($lastc=='S')
		$lat = -$lat;
	return $lat;
}
function strtolon($glon) {
	$lon = 0;
	$lastc = substr($glon, -1);
	$lon = substr($glon,0,3) + substr($glon,3,-1)/60;
	if($lastc=='W')
		$lon = -$lon;
	return $lon;
}

?>
