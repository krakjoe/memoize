--TEST--
Memoize closure arg
--EXTENSIONS--
apcu
--INI--
opcache.enable_cli=1
--FILE--
<?php
/**
* @memoize
**/
function mem_test(Closure $disallowed) {
	return 10;
}

$test = mem_test(function(){});

var_dump($test);
--EXPECT--
int(10)
