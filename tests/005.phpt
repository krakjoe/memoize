--TEST--
Memoize closure return
--EXTENSIONS--
apcu
--FILE--
<?php
/**
* @memoize
**/
function mem_test(int $int) {
	return function(){};
}

$tests[] = mem_test(10);
$tests[] = mem_test(10);

var_dump($tests);

$info = php_memoize_info(true);

var_dump($info["num_hits"], $info["num_misses"]);
--EXPECT--
array(2) {
  [0]=>
  object(Closure)#1 (0) {
  }
  [1]=>
  object(Closure)#2 (0) {
  }
}
float(0)
float(1)
