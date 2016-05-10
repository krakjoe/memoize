--TEST--
Memoize inheritance
--EXTENSIONS--
apcu
--FILE--
<?php
class Foo {
	/**
	* @memoize
	**/
	public function test() {
		$arr = [];

		for ($i = 0; $i < 10; $i++) {
			$arr[] = $i;
		}

		return $arr;
	}
}

class Bar extends Foo {}

$bar = new Bar;

$tests[] = $bar->test();
$tests[] = $bar->test();

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);
--EXPECT--
float(1)
float(1)
