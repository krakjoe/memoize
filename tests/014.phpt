--TEST--
Memoize disallow negative ttl
--EXTENSIONS--
apcu
--FILE--
<?php
/**
* @memoize(-1)
*/
function thing() {
	return true;
}

thing();
--EXPECTF--
Fatal error: Uncaught RuntimeException: cannot memoize with negative ttl in %s:9
Stack trace:
#0 {main}
  thrown in %s on line 9

