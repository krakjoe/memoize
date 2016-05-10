--TEST--
Memoize no args
--EXTENSIONS--
apcu
--FILE--
<?php
/**
* @memoize
**/
function mem_test() {
	$arr = [];

	for ($i = 0; $i < 10; $i++) {
		$arr[] = $i;
	}

	return $arr;
}

$tests[] = mem_test();
$tests[] = mem_test();

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);
--EXPECT--
float(1)
float(1)
