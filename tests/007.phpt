--TEST--
Memoize method single arg
--EXTENSIONS--
apcu
--FILE--
<?php
class Foo {
	/**
	* @memoize
	*/
	public function test(int $int) {
		$arr = [];

		for ($i = 0; $i < 10; $i++) {
			$arr[] = $int;
		}

		return $arr;
	}
}

$foo = new Foo;

$tests[] = $foo->test(1); # miss
$tests[] = $foo->test(2); # miss

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);

$tests[] = $foo->test(1); # hit
$tests[] = $foo->test(2); # hit

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);
--EXPECT--
float(2)
float(0)
float(2)
float(2)
