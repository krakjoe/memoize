memoize
======
*caching the result of your expensive function calls since 2016*

[![Build Status](https://travis-ci.org/krakjoe/memoize.svg?branch=master)](https://travis-ci.org/krakjoe/memoize)

> In computing, memoization is an optimization technique used primarily to speed up computer programs by storing 
> the results of expensive function calls and returning the cached result when the same inputs occur again.

The ```memoize``` extention leverages [APCu](https://github.com/krakjoe/apcu) and Zend API to implement memoization for functions and methods in PHP.

how?
====

We don't yet have attributes in PHP 7, we hopefully will get them one day soon ... for now functions that should be memoized should be annotated with ```@memoize```:

```php
/**
* @memoize
**/
function thing() {
	/* ... */
}
```

*Note that the annotation is case sensitive, and must not contain spaces; Case insensitivity would be a wasteful use of resources.*

 * When ```thing``` is invoked for the first time, the result is cached by ```memoize```.
 * Subsequent invocation of ```thing``` will result in the cached value being returned by ```memoize```.

configure
========

__```memoize``` requires APCu for it's C API, and requires that it is loaded, but APCu does not have to be enabled; ```memoize``` uses a separately configured cache.__

The following INI settings configure the cache ```memoize```:

 * ```memoize.enabled``` default(1)
 * ```memoize.segs``` default(1)
 * ```memoize.size``` default(32M)
 * ```memoize.entries``` default(4093)
 * ```memoize.ttl``` default(0)
 * ```memoize.smart``` default(1)

ttl
===

```memoize.ttl``` sets the global TTL for the cache, and the GC list (equivalent to ```apc.gc_ttl``` *and* ```apc.ttl```), it is also possible to set a ttl (in seconds) on an individual entry:

```php
/**
* @memoize(3600)
**/
function thing() {
	/* ... */
}
```

*Note that the annotation is case sensitive, and must not contain spaces; Case insensitivity would be a wasteful use of resources.*
