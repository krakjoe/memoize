--TEST--
Memoize method many args
--EXTENSIONS--
apcu
--FILE--
<?php
class Foo {
	/**
	* @memoize
	**/
	public function test(int $int, string $thing) {
		$arr = [];

		for ($i = 0; $i < 10; $i++) {
			$arr[] = strlen($thing) * $int;
		}

		return $arr;
	}
}

$foo = new Foo;

$tests[] = $foo->test(1, "hello"); # miss
$tests[] = $foo->test(2, "hello"); # miss

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);

$tests[] = $foo->test(1, "world"); # miss
$tests[] = $foo->test(2, "world"); # miss

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);

$tests[] = $foo->test(1, "hello"); # hit
$tests[] = $foo->test(2, "hello"); # hit
$tests[] = $foo->test(1, "world"); # hit
$tests[] = $foo->test(2, "world"); # hit

$info = php_memoize_info(true);

var_dump($info["num_misses"], $info["num_hits"]);
--EXPECT--
float(2)
float(0)
float(4)
float(0)
float(4)
float(4)
