dnl $Id$
dnl config.m4 for extension memoize


PHP_ARG_ENABLE(memoize, whether to enable memoize support,
[  --enable-memoize           Enable memoize support])

if test "$PHP_MEMOIZE" != "no"; then
  PHP_NEW_EXTENSION(memoize, memoize.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
  PHP_ADD_EXTENSION_DEP(memoize, APCu)
  PHP_ADD_EXTENSION_DEP(memoize, SPL)
fi
