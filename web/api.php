<?php

// /api/getlastcall/long/lat/delta 获取以lng/lat为中心点，delta周边的站点
// /api/getlastpos/CALL 获取某站点最后位置
// /api/gettrak/CALL 获取某站点轨迹

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

date_default_timezone_set( 'Asia/Shanghai');
$startdate=date_create();
date_sub($startdate,date_interval_create_from_date_string("1 days"));
$startdatestr=date_format($startdate,"Y-m-d 00:00:00");

$url_array = explode('/', $_SERVER['REQUEST_URI']);
array_shift($url_array);
array_shift($url_array);

if ($url_array[0] == "getlastcall") {
	$x = $url_array[1];
	$y = $url_array[2];
	$delta = $url_array[3];

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
		$icon = "http://202.141.176.2/img/".bin2hex($dts).".png";
		if( ($lon > $x - $delta) && ($lon < $x + $delta) && ($lat > $y -$delta) && ($lat < $y+$delta)) {
			if($count!=0)
				echo ",\n";
			else
				echo "\n";
			echo "{\"id\": $count, "; 
        		echo "\"iconPath\": \"$icon\", ";
        		echo "\"callname\":\"$call\", ";
        		echo "\"latitude\": $lat, ";
        		echo "\"longitude\": $lon, ";
			echo "\"timestamp\": $dtmstr }";
			$count ++;
		}
	} 
	$stmt->close();
	echo "\n] }";
	exit(0);
}

if ($url_array[0] == "getlastpos") {
	$call = $url_array[1];

	$q="select lat,lon,tm,concat(`table`,symbol),datatype from lastpacket where tm>? and `call`=?";
	$stmt=$mysqli->prepare($q);
	$stmt->bind_param("ss",$startdatestr, $call);
	$stmt->execute();
	$stmt->bind_result($glat,$glon,$dtmstr,$dts,$ddt);
	$stmt->store_result();
	echo "{ ";
	$count = 0;

	while($stmt->fetch()) {
		$lat = strtolat($glat);
        	$lon = strtolon($glon);
		$icon = "http://202.141.176.2/img/".bin2hex($dts).".png";
		echo "\"id\": $count, "; 
        	echo "\"iconPath\": \"$icon\", ";
        	echo "\"callname\":\"$call\", ";
        	echo "\"latitude\": $lat, ";
        	echo "\"longitude\": $lon, ";
		echo "\"timestamp\": $dtmstr }";
		$count ++;
		$stmt->close();
		exit(0);
	} 
	echo "}";
	exit(0);
}

if ($url_array[0] == "gettrack") {
	$call = $url_array[1];

	$q="select lat,lon,tm,concat(`table`,symbol),datatype from posaprspacket where tm>? and `call`=? and lat<>'' and not lat like '0000.00%' order by tm";
	$stmt=$mysqli->prepare($q);
	$stmt->bind_param("ss",$startdatestr,$call);
	$stmt->execute();
	$stmt->bind_result($glat,$glon,$dtmstr,$dts,$ddt);
	$stmt->store_result();
	echo "{ \"callname\": \"$call\", tracks: [";
	$count = 0;

	while($stmt->fetch()) {
		$lat = strtolat($glat);
        	$lon = strtolon($glon);
		$icon = "http://202.141.176.2/img/".bin2hex($dts).".png";
		if($count!=0)
			echo ",\n";
		else
			echo "\n";
		echo "{\"id\": $count, "; 
       		echo "\"iconPath\": \"$icon\", ";
       		echo "\"latitude\": $lat, ";
       		echo "\"longitude\": $lon, ";
		echo "\"timestamp\": $dtmstr }";
		$count ++;
	} 
	$stmt->close();
	echo "\n] }";
	exit(0);
}
exit(0);
?>
