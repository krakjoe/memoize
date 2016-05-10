--TEST--
Memoize disallow generator
--EXTENSIONS--
apcu
--FILE--
<?php
/**
* @memoize
*/
function thing() {
	yield 1;
}

thing();
--EXPECTF--
Fatal error: Uncaught RuntimeException: cannot memoize generator in %s:9
Stack trace:
#0 {main}
  thrown in %s on line 9
