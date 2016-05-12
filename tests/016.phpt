--TEST--
Memoize return constants
--EXTENSIONS--
apcu
--FILE--
<?php
/**
* @memoize
**/
function thing() {
	return [1,2,3];
}

$things = [];

$things[0] = thing();
$things[1] = thing();

$info = php_memoize_info(true);

var_dump($info["num_hits"], $info["num_misses"]);
--EXPECT--
float(1)
float(1)

