--TEST--
Memoize many args
--EXTENSIONS--
apcu
--FILE--
<?php
/**
* @memoize
**/
function mem_test(int $int, string $thing) {
	$arr = [];

	for ($i = 0; $i < 10; $i++) {
		$arr[] = strlen($thing) * $int;
	}

	return $arr;
}

$tests[] = mem_test(1, "hello"); # miss
$tests[] = mem_test(2, "hello"); # miss

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);

$tests[] = mem_test(1, "world"); # miss
$tests[] = mem_test(2, "world"); # miss

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);

$tests[] = mem_test(1, "hello"); # hit
$tests[] = mem_test(2, "hello"); # hit
$tests[] = mem_test(1, "world"); # hit
$tests[] = mem_test(2, "world"); # hit

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);
--EXPECT--
float(2)
float(0)
float(4)
float(0)
float(4)
float(4)
