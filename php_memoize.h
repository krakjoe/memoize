/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2016 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: krakjoe                                                      |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_MEMOIZE_H
#define PHP_MEMOIZE_H

extern zend_module_entry memoize_module_entry;
#define phpext_memoize_ptr &memoize_module_entry

#define PHP_MEMOIZE_VERSION "0.0.1"
#define PHP_MEMOIZE_EXTNAME "memoize"

#ifdef PHP_WIN32
#	define PHP_MEMOIZE_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_MEMOIZE_API __attribute__ ((visibility("default")))
#else
#	define PHP_MEMOIZE_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

ZEND_BEGIN_MODULE_GLOBALS(memoize)
	volatile zend_bool initialized;
	struct {
		zend_long segs;
		zend_long size;
		zend_long entries;
		zend_long ttl;
		zend_bool smart;
		zend_bool enabled;
		zend_bool debug;
	} ini;
ZEND_END_MODULE_GLOBALS(memoize)

#ifdef ZTS
#	define MG(v) TSRMG(memoize_globals_id, zend_memoize_globals *, v)
#else
#	define MG(v) memoize_globals.v
#endif

#if defined(ZTS) && defined(COMPILE_DL_MEMOIZE)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif	/* PHP_MEMOIZE_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
