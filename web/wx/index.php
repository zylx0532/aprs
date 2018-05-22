
<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
	<meta name="viewport" content="initial-scale=1.0" />
</head>
<body>
<?php

echo "<h3>微信位置GPS经纬度提取</h3>\n";

$url =@$_REQUEST["url"];
if($url!="")  {
	echo "您的输入：".$url."<p>";
	$u=explode("&",$url);
	$x=0;
	$y=0;
	for($i=0;$i<count($u);$i++) {
		$r=explode('=',$u[$i]);
		if($r[0]=="pointx")
			$x=$r[1];
		else if($r[0]=="pointy")
			$y=$r[1];
	}
	echo "wx y = ".$y."<br>";
	echo "wx x = ".$x."<p>";

include "GpsPositionTransform.class.php";

	if(GpsPositionTransform::outOfChina($y,$x)) {
		echo "<font color=red>非中国点，不需要转换<p>";
	} else {
		$r=GpsPositionTransform::gcj_To_Gps84($y,$x);
		$x=$r["lon"];
		$y=$r["lat"];
		echo "GPS y = ".$y." º<br>";
		echo "GPS x = ".$x." º<p>";

		echo "GPS y = ".intval($y)."º".sprintf("%02.3f",(($y-intval($y))*60))."'<br>";
		echo "GPS x = ".intval($x)."º".sprintf("%02.3f",(($x-intval($x))*60))."'<p>";

		echo "GPS y = ".intval($y)."º";
		$y=($y-intval($y))*60;
		echo sprintf("%02.0f",intval($y))."'";
		$y=($y-intval($y))*60;
		echo sprintf("%02.0f",intval($y))."\"<br>";

		echo "GPS x = ".intval($x)."º";
		$x=($x-intval($x))*60;
		echo sprintf("%02.0f",intval($x))."'";
		$x=($x-intval($x))*60;
		echo sprintf("%02.0f",intval($x))."\"<p>";
	}
}

echo "<form name=aprs action=index.php method=post>";
echo "微信链接：<input name=url size=200><p>\n";
echo "<input type=submit value=\"提取信息\"></input><p>\n";
echo "</from><p>";
?>

