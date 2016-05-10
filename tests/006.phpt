--TEST--
Memoize method no args
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

$foo = new Foo;

$tests[] = $foo->test();
$tests[] = $foo->test();

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);
--EXPECT--
float(1)
float(1)
