--TEST--
Memoize disallow constructor
--EXTENSIONS--
apcu
--FILE--
<?php
class Foo {
	/**
	* @memoize
	**/
	public function __construct() {
		
	}
}

new Foo();
--EXPECTF--
Fatal error: Uncaught RuntimeException: cannot memoize constructor in %s:11
Stack trace:
#0 {main}
  thrown in %s on line 11
