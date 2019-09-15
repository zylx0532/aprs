<?php

include "db.php";

function strtolat($glat) {
	$lat = 0;
	$lat = substr($glat,0,2) + substr($glat,2,5)/60;
	if(substr($glat,7,1)=='S')
		$lat = -$lat;
	return $lat;
}
function strtolon($glon) {
	$lon = 0;
	$lon = substr($glon,0,3) + substr($glon,3,5)/60;
	if(substr($glon,8,1)=='W')
		$lon = -$lon;
	return $lon;
}

$url_array = explode('/', $_SERVER['REQUEST_URI']);
array_shift($url_array);
array_shift($url_array);

if ($url_array[0] == "getlastcall") {
	$x = $url_array[1];
	$y = $url_array[2];
	$delta = $url_array[3];


	date_default_timezone_set( 'Asia/Shanghai');
	$startdate=date_create();
	date_sub($startdate,date_interval_create_from_date_string("1 days"));
	$startdatestr=date_format($startdate,"Y-m-d 00:00:00");

	$q="select `call`,lat,lon,tm,concat(`table`,symbol),datatype from lastpacket where tm>? and lat<>'' and not lat like '0000.00%' order by `call`";
	$stmt=$mysqli->prepare($q);
	$stmt->bind_param("s",$startdatestr);
	$stmt->execute();
	$stmt->bind_result($call,$glat,$glon,$dtmstr,$dts,$ddt);
	$stmt->store_result();
	echo "{ stations: [";
	$count = 0;

	while($stmt->fetch()) {
		$lat = strtolat($glat);
        	$lon = strtolon($glon);
		$icon = "img/".bin2hex($dts).".png";
		if( ($lon > $x - $delta) && ($lon < $x + $delta) && ($lat > $y -$delta) && ($lat < $y+$delta)) {
			if($count!=0)
				echo ",\n";
			else
				echo "\n";
			echo "{\"id\": $count, "; 
        		echo "\"iconPath\": \"$icon\", ";
        		echo "\"callname\":\"$call\", ";
        		echo "\"latitude\": $lat, ";
        		echo "\"longitude\": $lon }";
			$count ++;
		}
	} 
	$stmt->close();
	echo "\n] }";
	exit(0);
}

exit(0);
$span=$_SESSION["span"];  	
$q="select lat,lon,tm,concat(`table`,symbol),msg,datatype from posaprspacket where tm>? and `call`=? and lat<>'' and not lat like '0000.00%' order by tm limit 50000";
$stmt=$mysqli->prepare($q);
$stmt->bind_param("ss",$startdatestr,$call);
$stmt->execute();
$stmt->bind_result($glat,$glon,$dtmstr,$dts,$dmsg,$ddt);
$stmt->store_result();
while($stmt->fetch()) {
	$lat = strtolat($glat);
        $lon = strtolon($glon);
	if($jiupian==1) {
		$p=$mp->WGStoBaiDuPoint($lon,$lat);
		$lon = $p->getX();
		$lat = $p->getY();
	} else if($jiupian==2) {
		$p=$mp->ChinatoBaiDuPoint($lon,$lat);
		$lon = $p->getX();
		$lat = $p->getY();
	} 
	$icon = "img/".bin2hex($dts).".png";
	$dmsg = urlmessage($call, $icon, $dtmstr, $dmsg, "", $ddt);
	echo "addpathpoint(".$lon.",".$lat.",\"".$dmsg."\");\n";
}
	echo "setstation(\"".$call."\",".$lon.",".$lat.",\"".$icon."\",\n\"".$dmsg."\");\n";
$stmt->close();

?>
