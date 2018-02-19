<?php

date_default_timezone_set("Asia/Shanghai");
include "web/db.php";

$q="delete from aprspacket where tm<=date_sub(now(), interval 2 day)";
$result = $mysqli->query($q);

$q="delete from posaprspacket where tm<=date_sub(now(), interval 2 day) limit 2000";
$result = $mysqli->query($q);
?>
