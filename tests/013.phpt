--TEST--
Memoize disallow closure
--EXTENSIONS--
apcu
--FILE--
<?php
/**
* @memoize
*/
$foo = function() {};

$foo();
--EXPECTF--
Fatal error: Uncaught RuntimeException: cannot memoize closure in %s:7
Stack trace:
#0 {main}
  thrown in %s on line 7
