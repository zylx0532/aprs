<?php

include "db.php";

$ak = "7RuEGPr12yqyg11XVR9Uz7NI";

date_default_timezone_set( 'Asia/Shanghai');

if (!isset($_SESSION["jiupian"]))
	$_SESSION["jiupian"]=1;

if (!isset($_SESSION["span"]))
	$_SESSION["span"]=1;
$span = $_SESSION["span"];
if ( ($span<=0) || ($span >10) ) $span=1;
$span--;
$startdate=date_create();
date_sub($startdate,date_interval_create_from_date_string("$span days"));
$startdatestr=date_format($startdate,"Y-m-d 00:00:00");

if (isset($_REQUEST["call"]))
	$call=$_REQUEST["call"];
else {
	echo "请提供call";
}

$title = $call."APRS轨迹";
?>
<!DOCTYPE html>
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<meta name="viewport" content="initial-scale=1.0, user-scalable=no" />
	<style type="text/css">
		body, html{width: 100%;height: 100%;margin:0;font-family:"微软雅黑";}
		#allmap {height:100%; width: 100%;}
	</style>
	<title><?php echo $title; ?></title>
	<script type="text/javascript" src="http://api.map.baidu.com/api?v=2.0&ak=<?php echo $ak;?>"></script>
</head>
<body>
<div id="allmap">
</div>
</body>
</html>
<script type="text/javascript">
var marker;
var movepath = new Array();
var polyline;
var jiupian=1;
var debug=true;

function addpathpoint(lon, lat, msg) {
	var icon = new BMap.Icon("/p.png", new BMap.Size(5, 5));	
        var p = new BMap.Point(lon, lat);
	movepath.push(p);
	var m = new BMap.Marker(new BMap.Point(lon,lat), {icon: icon});
	var infowindow = new BMap.InfoWindow(msg, {width:300});
	(function(){
        	m.addEventListener('click', function(){
            	this.openInfoWindow(infowindow);
        	});
	})();
	map.addOverlay(m);
}

function setstation(label, lon, lat, iconurl, msg)
{	
	var icon = new BMap.Icon(iconurl, new BMap.Size(24, 24), {anchor: new BMap.Size(12, 12)});	

	var marker = new BMap.Marker(new BMap.Point(lon,lat), {icon: icon});
	var lb = new BMap.Label(label, {offset: new BMap.Size(20,-10)});
	lb.setStyle({border:0, background: "#eeeeee"});
	marker.setLabel(lb);
	var infowindow = new BMap.InfoWindow(msg, {width:300});
	(function(){
        	marker.addEventListener('click', function(){
            	this.openInfoWindow(infowindow);
        	});
	})();
	map.addOverlay(marker);
	map.centerAndZoom(new BMap.Point(lon, lat), 6);
	polyline = new BMap.Polyline(movepath,{strokeColor:'#1400ff', strokeWeight:4, strokeOpacity:0.9});
	map.addOverlay(polyline);
}

// 百度地图API功能
var map = new BMap.Map("allmap");
map.enableScrollWheelZoom();

var top_left_control = new BMap.ScaleControl({anchor: BMAP_ANCHOR_TOP_LEFT});// 左上角，添加比例尺
var top_left_navigation = new BMap.NavigationControl();  //左上角，添加默认缩放平移控件
	
//添加控件和比例尺
map.addControl(top_left_control);        
map.addControl(top_left_navigation);     
map.addControl(new BMap.MapTypeControl());
map.centerAndZoom(new BMap.Point(108.940178,34.5), 6);

<?php

$jiupian = 0; 	// 0 不处理
		// 1 存储的是地球坐标，转换成baidu显示, 默认情况
		// 2 存储的是火星坐标，转换成baidu显示

$jiupian = 1;

if( isset( $_SESSION["jiupian"]))
	$jiupian=$_SESSION["jiupian"];

if($jiupian>0) {
	require "wgtochina_baidu.php";
	$mp=new Converter();
}

function urlmessage($call, $icon, $dtmstr, $msg, $path, $ddt) {
	global $mysqli;
	global $startdatestr;
	$m = "<font face=微软雅黑 size=2><img src=".$icon."> ".$call;
	$m = $m."<hr color=green>".$dtmstr."<br>";

	$msg = rtrim($msg);
	if(strlen($msg)>=4) 
		if(substr($msg, strlen($msg)-4,4)=="/UDP")   // strip /UDP
			$msg = substr($msg,0, strlen($msg)-4);
	$msg = rtrim($msg);

	if (  (strlen($msg)>=32) &&
		(substr($msg,3,1)=='/') &&
		(substr($msg,7,1)=='g') &&
		(substr($msg,11,1)=='t') &&
		( (substr($msg,15,1)=='r') // 090/001g003t064r000p000h24b10212 or 000/000g000t064r000P000p000h39b10165
                ||(substr($msg,16,1)=='r')))  // 090/001g003t0064r000p000h24b10212 or 000/000g000t0064r000P000p000h39b10165
	{
		$c = substr($msg,0,3)*1; //wind dir
		$s = number_format(substr($msg,4,3)*0.447,1); //wind speed
		$g = number_format(substr($msg,8,3)*0.447,1); //5min wind speed
		if(substr($msg,15,1)=='r') {
			$t = number_format((substr($msg,12,3)-32)/1.8,1); //temp
			$r = number_format(substr($msg,16,3)*25.4/100,1); //rainfall in mm 1 hour
		} else {
			$t = number_format((substr($msg,12,4)-32)/1.8,1); //temp
			$r = number_format(substr($msg,17,3)*25.4/100,1); //rainfall in mm 1 hour
		}
		$msg = strstr($msg,"p");
		$p = number_format(substr($msg,1,3)*25.4/100,1); //rainfall in mm 24 hour
		$msg = strstr($msg,"h");
		$h = substr($msg,1,2);	//hum
		$b = substr($msg,4,5)/10; //press
		$msg = substr($msg,9);
		$m = $m."<b>温度".$t."°C 湿度".$h."% 气压".$b."mbar<br>";
		$m = $m."风".$c."°".$s."m/s(大风".$g."m/s)<br>";
	 	$m = $m."雨".$r."mm/1h ".$p."mm/24h</b><br>";
		if(substr($msg,0,1)==',') {  // ,053,069,071,035,047,058,28.928 PM1.0 PM2.5 PM10 
			$msg = substr($msg,1);
			$uspm1 = 0 + $msg;
			$msg = strstr($msg,",");
			$msg = substr($msg,1);
			$uspm25 = 0 + $msg;
			$msg = strstr($msg,",");
			$msg = substr($msg,1);
			$uspm10 = 0 + $msg;
			$msg = strstr($msg,",");
			$msg = substr($msg,1);
			$cnpm1 = 0 + $msg;
			$msg = strstr($msg,",");
			$msg = substr($msg,1);
			$cnpm25 = 0 + $msg;
			$msg = strstr($msg,",");
			$msg = substr($msg,1);
			$cnpm10 = 0 + $msg;
			$msg = strstr($msg,",");
			$msg = substr($msg,1);
			$jiaquan = 0 + $msg;
			$m = $m."<b>美标PM1.0/2.5/10</b>: ".$uspm1."/".$uspm25."/".$uspm10." ug/m3<br>";	
			$m = $m."<b>国标PM1.0/2.5/10</b>: ".$cnpm1."/".$cnpm25."/".$cnpm10." ug/m3<br>";	
			$m = $m."<b>甲醛</b>: ".$jiaquan." ug/m3<br>";
			while(1) {
				$c=substr($msg,0,1);
				if( is_numeric($c) || ($c=='.')) {
					$msg = substr($msg,1);
					continue;
				}
				break;
			}
		}
	}
	if (  (strlen($msg)>=27) &&
		(substr($msg,3,1)=='/') &&
		(substr($msg,7,1)=='g') &&
		(substr($msg,11,1)=='t') &&
		(substr($msg,15,1)=='P') )  // 090/000g002t046P099h51b10265V130OTW1
	{
		$c = substr($msg,0,3)*1; //wind dir
		$s = number_format(substr($msg,4,3)*0.447,1); //wind speed
		$g = number_format(substr($msg,8,3)*0.447,1); //5min wind speed
		$t = number_format((substr($msg,12,3)-32)/1.8,1); //temp
		$r = number_format(substr($msg,16,3)*25.4/100,1); //rainfall in mm 1 hour
		$msg = strstr($msg,"h");
		$h = substr($msg,1,2);	//hum
		$b = substr($msg,4,5)/10; //press
		$msg = substr($msg,9);
		$m = $m."<b>温度".$t."°C 湿度".$h."% 气压".$b."mbar<br>";
		$m = $m."风".$c."°".$s."m/s(大风".$g."m/s)<br>";
	 	$m = $m."雨".$r."mm/自午夜起</b><br>";
	}
	if( (strlen($msg)>=7) &&
		(substr($msg,3,1)=='/'))  // 178/061/A=000033
	{
		$dir=substr($msg,0,3);
		$speed=number_format(substr($msg,4,3)*1.852,1);
		$m = $m."<b>".$speed." km/h ".$dir."°";
		$msg = substr($msg,7);
		if( substr($msg,0,3)=='/A=') {      // 178/061/A=000033
			if(strstr($msg, "51Track X2A")) {
				$alt=number_format(substr($msg,3,6),1);
			} else {
				$alt=number_format(substr($msg,3,6)*0.3048,1);
			}
			$m=$m." 海拔".$alt."m";
			$msg = substr($msg,9);
		}
		$m = $m."</b><br>";
	} else if( (strlen($msg)>=9) &&
		(substr($msg,0,3)=='/A=') )      // /A=000033
	{
		$alt=number_format(substr($msg,3,6)*0.3048,1);
		$m = $m."<b> 海拔".$alt."m</b><br>";
		$msg = substr($msg,9);
	} else if( ($ddt=='`')  &&
		 (strlen($msg)>=9) )   // `  0+jlT)v/]"4(}=
	{
		$speed = (ord(substr($msg,3,1))-28)*10;
		$t=ord(substr($msg,4,1))-28;
		$speed = $speed + $t/10;
		if($speed>=800) $speed-=800;
		$speed = number_format($speed*1.852,1);
		$dir = ($t%10)*100 + ord(substr($msg,5,1))-28;
		if($dir>=400) $dir -= 400;
		$msg = substr($msg,8);
		$alt=0;
		
		if((substr($msg,0,1)==']') || (substr($msg,0,1)=='`') )
			$msg=substr($msg,1);
		if( (strlen($msg)>=4) && (substr($msg,3,1)=='}') ) {
			$alt = (ord( substr($msg,0,1)) -33)*91*91+
				(ord( substr($msg,1,1)) -33)*91 +
				(ord( substr($msg,2,1)) -33) -10000;
			$alt = number_format($alt,1);
			$msg = substr($msg,4);
		}
		$m = $m."<b>".$speed." km/h ".$dir."° 海拔".$alt."m</b><br>";
		if(strlen($msg)>=2) 
			if((substr($msg,strlen($msg)-2,2)=='_(') 
			|| (substr($msg,strlen($msg)-2,2)=='_)')
			|| (substr($msg,strlen($msg)-2,2)=='_"')
			 )
				$msg=substr($msg,0,strlen($msg)-2);
	}  
	if( (strlen($msg)>=7) &&
                (substr($msg,0,3)=='PHG') )  // PHG
        {
		$pwr = ord(substr($msg,3,1))-ord('0');
		$pwr = $pwr*$pwr;
		$h = ord(substr($msg,4,1))-ord('0');
		$h = pow(2,$h)*10*0.3048;
		$h = round($h);
		$g = substr($msg,5,1);
                $m = $m."<b>功率".$pwr."瓦 天线高度".$h."m 增益".$g."dB</b><br>";
                $msg = substr($msg,7);
	}
	if( (strlen($msg)>=9) &&
		strstr($msg,"MHz/A=") )       // 430.100MHz/A=000392
	{	
		$hz = substr($msg,1,strpos($msg,"MHz/A="));
		$msg = strstr($msg,"MHz/A=");
		$msg = substr($msg,4);
		$alt=number_format(substr($msg,3,6)*0.3048,1);
		$m = $m." 海拔".$alt."m</b><br>";
		$msg = $hz."MHz ".substr($msg,9);
	}
		
	if(strlen($msg)>0)
	$m = $m."</font><font color=green face=微软雅黑 size=2>".addcslashes(htmlspecialchars($msg,ENT_QUOTES),"\\\r\n'\"")."</font><br>";

 	$q = "select raw from aprspacket where tm>=? and `call` = ? and lat ='' order by tm desc limit 1";
	$stmt = $mysqli->prepare($q);
	if(!$stmt) {
		echo "Prepare failed: (" . $mysqli->errno . ") " . $mysqli->error;
		exit;
	}
        $stmt->bind_param("ss",$startdatestr,$call);
        $stmt->execute();
	$rawmsg = "";
       	$stmt->bind_result($rawmsg);
	$stmt->fetch();
	if($rawmsg!="") {
		$rawmsg = strstr($rawmsg,":>");
		if($rawmsg) {
			$rawmsg = substr($rawmsg,2);
			$t = strpos($rawmsg, "/UDP");
			if($t !== false )
				$rawmsg = substr($rawmsg, 0, $t);
			$m = $m."<font color=red face=微软雅黑 size=2>".addcslashes(htmlspecialchars($rawmsg,ENT_QUOTES),"\\\r\n'\"")."</font><br>";
		}
	}	
	$stmt->close();
	if($path!="") 
		$m = $m."<font color=black face=微软雅黑 size=2>[".htmlspecialchars($path,ENT_QUOTES)."]</font>";
	return $m;	
}

include "strtolonlat.php";

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
map.panTo(movepath[movepath.length-1]);
map.setZoom(12);
</script>
