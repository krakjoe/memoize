--TEST--
Memoize debug ini setting
--EXTENSIONS--
apcu
--INI--
memoize.debug=1
--FILE--
<?php
/**
* @memoize(1)
**/
function thing() {
	$array = [1,2,3];
	return $array;
}

$things = [];

$things[0] = thing();

sleep(2);

$things[1] = thing();

$info = php_memoize_info(true);

var_dump($info["num_hits"], $info["num_misses"]);
--EXPECT--
float(0)
float(2)

