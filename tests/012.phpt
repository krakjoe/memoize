--TEST--
Memoize scope
--EXTENSIONS--
apcu
--FILE--
<?php
class Foo {

	public function __construct($state) {
		$this->state = $state;
	}

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

$bar = new Bar(1);

$tests[] = $bar->test(); # miss
$tests[] = $bar->test(); # hit

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);

$qux = new Bar(2);

$tests[] = $qux->test(); # miss
$tests[] = $qux->test(); # hit

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);
--EXPECT--
float(1)
float(1)
float(2)
float(2)
