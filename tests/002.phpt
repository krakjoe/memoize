--TEST--
Memoize single arg
--EXTENSIONS--
apcu
--FILE--
<?php
/**
* @memoize
**/
function mem_test(int $int) {
	$arr = [];

	for ($i = 0; $i < 10; $i++) {
		$arr[] = $int;
	}

	return $arr;
}

$tests[] = mem_test(1); # miss
$tests[] = mem_test(2); # miss

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);

$tests[] = mem_test(1); # hit
$tests[] = mem_test(2); # hit

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);
--EXPECT--
float(2)
float(0)
float(2)
float(2)
