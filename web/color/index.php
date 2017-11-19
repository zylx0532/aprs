<?php

include "../db.php";

@$cmd=$_REQUEST["cmd"];
if($cmd=="add") {
	$rgb=$_REQUEST["rgb"];
	$q="insert into color values(?)";
	$stmt=$mysqli->prepare($q);
	$stmt->bind_param("s",$rgb);
	$stmt->execute();
	$stmt->close();
} else if($cmd=="del") {
	$rgb=$_REQUEST["rgb"];
	$q="delete from color where rgb=?";
	$stmt=$mysqli->prepare($q);
	$stmt->bind_param("s",$rgb);
	$stmt->execute();
	$stmt->close();
} 
$q="select rgb from color order by rgb";
$stmt=$mysqli->prepare($q);
$stmt->execute();
$stmt->bind_result($rgb);
$stmt->store_result();	
echo "<table>";
$c="";
while($stmt->fetch()) {
	echo "<tr>";
	echo "<td>$rgb</td><td width=40 bgcolor=\"#$rgb\">&nbsp;</td><td><a href=index.php?cmd=del&rgb=$rgb>del</a></tr>";
	$c=$c.",\"FF".substr($rgb,4,2).substr($rgb,2,2).substr($rgb,0,2)."\"";
}
?>
</table>
<form action=index.php method=get>
RGB:<input name=rgb>
<input name=cmd type=submit value=add>
</form>

<hr>
<?php
echo $c;
?>
