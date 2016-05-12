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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_var.h"
#include "ext/apcu/apc_api.h"
#include "ext/spl/spl_exceptions.h"
#include "zend_smart_str.h"
#include "zend_extensions.h"
#include "zend_interfaces.h"

#include "php_memoize.h"

typedef int (*zend_vm_func_f)(zend_execute_data *);

typedef struct _php_memoize_info_t {
	zend_uchar flags;
	zend_ulong ttl;
} php_memoize_info_t;

#define PHP_MEMOIZE_USED     0x00000001
#define PHP_MEMOIZE_DISABLED 0x00000010

#define PHP_MEMOIZE_SCOPE_FAILURE ((zend_string*) -1)

int php_memoize_reserved;

#define PHP_MEMOIZE_INFO(f) (php_memoize_info_t*) ((f)->op_array.reserved[php_memoize_reserved])
#define PHP_MEMOIZE_INFO_SET(f, i) do { \
	php_memoize_info_t **_i = (php_memoize_info_t**) &(f)->op_array.reserved[php_memoize_reserved]; \
	*_i = (i); \
} while(0)

zend_vm_func_f zend_return_function;
zend_vm_func_f zend_do_ucall_function;
zend_vm_func_f zend_do_fcall_function;
zend_vm_func_f zend_do_fcall_by_name_function;

apc_sma_api_decl(php_memoize_sma);

apc_cache_t* php_memoize_cache = NULL;

apc_sma_api_impl(php_memoize_sma, &php_memoize_cache, apc_cache_default_expunge);

ZEND_DECLARE_MODULE_GLOBALS(memoize);

/* {{{ */
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("memoize.enabled", "1", PHP_INI_SYSTEM, OnUpdateBool, ini.enabled, zend_memoize_globals, memoize_globals)
    STD_PHP_INI_ENTRY("memoize.segs", "1", PHP_INI_SYSTEM, OnUpdateLong, ini.segs, zend_memoize_globals, memoize_globals)
    STD_PHP_INI_ENTRY("memoize.size", "32M", PHP_INI_SYSTEM, OnUpdateLong, ini.size, zend_memoize_globals, memoize_globals)
    STD_PHP_INI_ENTRY("memoize.entries", "4093", PHP_INI_SYSTEM, OnUpdateLong, ini.entries, zend_memoize_globals, memoize_globals)
    STD_PHP_INI_ENTRY("memoize.ttl", "0", PHP_INI_SYSTEM, OnUpdateLong, ini.ttl, zend_memoize_globals, memoize_globals)
    STD_PHP_INI_ENTRY("memoize.smart", "1", PHP_INI_SYSTEM, OnUpdateBool, ini.smart, zend_memoize_globals, memoize_globals)
PHP_INI_END()
/* }}} */

/* {{{ php_memoize_init_globals
 */
static void php_memoize_init_globals(zend_memoize_globals *mg)
{
	memset(mg, 0, sizeof(zend_memoize_globals));
}
/* }}} */

/* {{{ */
static inline zend_string* php_memoize_args(uint32_t argc, const zval *argv) {
	php_serialize_data_t data;
	zval serial;
	uint32_t it;
	smart_str smart = {0};

	array_init(&serial);

	for (it = 0; it < argc; it++) {
		if (add_index_zval(&serial, it, (zval*) argv + it) == SUCCESS) {
			Z_TRY_ADDREF_P((zval*) argv + it);
		}
	}

	PHP_VAR_SERIALIZE_INIT(data);
	php_var_serialize(&smart, &serial, &data);
	PHP_VAR_SERIALIZE_DESTROY(data);

	if (EG(exception)) {
		zval_ptr_dtor(&serial);
		smart_str_free(&smart);
		zend_clear_exception();
		return NULL;
	}

	zval_ptr_dtor(&serial);
	return smart.s;
} /* }}} */

/* {{{ */
static inline zend_string* php_memoize_scope(const zval *This, const zend_function *function) {
	if (!function->common.scope) {
		return NULL;
	}

	if (Z_TYPE_P(This) != IS_OBJECT) {
		return zend_string_copy(function->common.scope->name);
	} else {
		smart_str smart = {0};
		php_serialize_data_t data;

		if (Z_OBJCE_P(This)->serialize == zend_class_serialize_deny) {
			return PHP_MEMOIZE_SCOPE_FAILURE;
		}

		PHP_VAR_SERIALIZE_INIT(data);
		php_var_serialize(&smart, (zval*) This, &data);
		PHP_VAR_SERIALIZE_DESTROY(data);

		if (EG(exception)) {
			smart_str_free(&smart);
			zend_clear_exception();

			return PHP_MEMOIZE_SCOPE_FAILURE;
		}

		return smart.s;
	}

	return NULL;
} /* }}} */

/* {{{ */
static inline zend_string* php_memoize_key(const zval *This, const zend_function *function, uint32_t argc, const zval *argv) {
	zend_string *key;
	zend_string *scope = php_memoize_scope(This, function);
	zend_string *name = function->common.function_name;
	zend_string *args = php_memoize_args(argc, argv);

	if (!args) {
		return NULL;
	}

	if (scope == PHP_MEMOIZE_SCOPE_FAILURE) {
		if (args) {
			zend_string_release(args);
		}
		return NULL;
	}

	key = zend_string_alloc(scope ? 
		ZSTR_LEN(scope) + ZSTR_LEN(name) + ZSTR_LEN(args) + sizeof("::") :
		ZSTR_LEN(name) + ZSTR_LEN(args), 0);

	if (scope) {
		memcpy(&ZSTR_VAL(key)[0], ZSTR_VAL(scope), ZSTR_LEN(scope));
		memcpy(&ZSTR_VAL(key)[ZSTR_LEN(scope)], ZSTR_VAL(name), ZSTR_LEN(name));
		memcpy(&ZSTR_VAL(key)[ZSTR_LEN(scope) + ZSTR_LEN(name)], ZSTR_VAL(args), ZSTR_LEN(args));
	} else {
		memcpy(&ZSTR_VAL(key)[0], ZSTR_VAL(name), ZSTR_LEN(name));
		memcpy(&ZSTR_VAL(key)[ZSTR_LEN(name)], ZSTR_VAL(args), ZSTR_LEN(args));
	}
	ZSTR_VAL(key)[ZSTR_LEN(key)] = 0;

	if (scope) {
		zend_string_release(scope);
	}
	zend_string_release(args);
	
	return key;
} /* }}} */

/* {{{ */
static inline zend_bool php_memoize_is_memoizing(const zend_function *function, zend_ulong *ttl) {
	const zend_function *check = function;

	do {
		if (check->type == ZEND_USER_FUNCTION) {
			php_memoize_info_t *info = PHP_MEMOIZE_INFO(check);

			if (!info) {
				info = (php_memoize_info_t*) 
					ecalloc(1, sizeof(php_memoize_info_t));

				PHP_MEMOIZE_INFO_SET(check, info);

				if (check->op_array.doc_comment && ZSTR_LEN(check->op_array.doc_comment) >= (sizeof("@memoize")-1)) {
					const char *mem = 
						strstr(
							ZSTR_VAL(check->op_array.doc_comment), "@memoize");

					if (mem != NULL) {
						if (check->common.fn_flags & ZEND_ACC_GENERATOR) {
							zend_throw_exception_ex(spl_ce_RuntimeException, 0,
								"cannot memoize generator");
							return 0;
						}

						if (check->common.fn_flags & ZEND_ACC_CTOR) {
							zend_throw_exception_ex(spl_ce_RuntimeException, 0,
								"cannot memoize constructor");
							return 0;
						}

						if (check->common.fn_flags & ZEND_ACC_CLOSURE) {
							zend_throw_exception_ex(spl_ce_RuntimeException, 0,
								"cannot memoize closure");
							return 0;
						}

						sscanf(mem,
							"@memoize(%lu)", &info->ttl);
						info->flags |= PHP_MEMOIZE_USED;
					}
				}
			}

			if (info->flags & PHP_MEMOIZE_DISABLED) {
				return 0;
			}

			if (!(info->flags & PHP_MEMOIZE_USED)) {
				if ((check->common.fn_flags & ZEND_ACC_CLOSURE) || !check->common.prototype) {
					return 0;
				}
			} else {
				if (ttl && info->ttl) {
					*ttl = info->ttl;
				}
				return 1;
			}
		}
	} while (!(check->common.fn_flags & ZEND_ACC_CLOSURE) && (check = check->common.prototype));

	return 0;
} /* }}} */

/* {{{ */
static inline zend_bool php_memoize_is_memoized(const zend_execute_data *frame) {
	const zend_execute_data *call = frame->call;
	const zend_function *fbc = call->func;

	if (call && php_memoize_is_memoizing(fbc, NULL)) {
		const zend_op *opline = frame->opline;
		zend_string *key = php_memoize_key(
			&call->This,
			fbc,
			ZEND_CALL_NUM_ARGS(call), ZEND_CALL_ARG(call, 1));
		zval *return_value;

		if (!key) {
			return 0;
		}

		return_value = ZEND_CALL_VAR(frame, opline->result.var);

		if (apc_cache_fetch(php_memoize_cache, key, sapi_get_request_time(), &return_value)) {
			zend_string_release(key);
			return 1;
		}

		zend_string_release(key);
	}

	return 0;
} /* }}} */

/* {{{ */
static inline int php_memoize_leave_helper(zend_execute_data *frame) {
	zend_execute_data *call = frame->call;
	uint32_t info = ZEND_CALL_INFO(call);

	if (info & ZEND_CALL_RELEASE_THIS) {
		OBJ_RELEASE(Z_OBJ(call->This));
	} else if (info & ZEND_CALL_CLOSURE) {
		 OBJ_RELEASE((zend_object*)call->func->op_array.prototype);
	}

	frame->call = call->prev_execute_data;

	zend_vm_stack_free_args(call);
	zend_vm_stack_free_call_frame(call);

	frame->opline = frame->opline + 1;

	return ZEND_USER_OPCODE_LEAVE;
} /* }}} */

/* {{{ */
static int php_memoize_ucall(zend_execute_data *frame) {
	if (MG(ini.enabled) && php_memoize_is_memoized(frame)) {
		return php_memoize_leave_helper(frame);
	}

	if (zend_do_ucall_function) {
		return zend_do_ucall_function(frame);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

/* {{{ */
static int php_memoize_fcall(zend_execute_data *frame) {
	if (MG(ini.enabled) && php_memoize_is_memoized(frame)) {
		return php_memoize_leave_helper(frame);
	}

	if (zend_do_fcall_function) {
		return zend_do_fcall_function(frame);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

/* {{{ */
static int php_memoize_fcall_by_name(zend_execute_data *frame) {
	if (MG(ini.enabled) && php_memoize_is_memoized(frame)) {
		return php_memoize_leave_helper(frame);
	}

	if (zend_do_fcall_by_name_function) {
		return zend_do_fcall_by_name_function(frame);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

/* {{{ */
static int php_memoize_return(zend_execute_data *frame) {
	zend_ulong ttl = 0;
	const zend_function *fbc = frame->func;

	if (MG(ini.enabled) && php_memoize_is_memoizing(fbc, &ttl)) {
		zend_string *key = php_memoize_key(
			&frame->This,
			fbc, 
			ZEND_CALL_NUM_ARGS(frame), ZEND_CALL_ARG(frame, 1));

		if (key) {
			apc_cache_store(php_memoize_cache, key, 
				ZEND_CALL_VAR(frame, frame->opline->op1.var), (zend_long) ttl, 1);

			if (EG(exception)) {
				php_memoize_info_t *info = PHP_MEMOIZE_INFO(fbc);

				info->flags |= PHP_MEMOIZE_DISABLED;

				zend_clear_exception();
			}

			zend_string_release(key);
		}
	}
	
	if (zend_return_function) {
		return zend_return_function(frame);
	}

	return ZEND_USER_OPCODE_DISPATCH;
} /* }}} */

/* {{{ */
static inline void php_memoize_info_dtor(zend_op_array *op_array) {
	php_memoize_info_t *info = PHP_MEMOIZE_INFO((zend_function*) op_array);
	if (info) {
		efree(info);
	}
} /* }}} */

/* {{{ */
/* not exported on purpose */ zend_extension zend_extension_entry = {
	PHP_MEMOIZE_EXTNAME,
	PHP_MEMOIZE_VERSION,
	"Joe Watkins <krakjoe@php.net>",
	"https://github.com/krakjoe/memoize",
	"Copyright (c) 2016",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, /* op_array_handler_func_t */
	NULL, /* statement_handler_func_t */
	NULL, /* fcall_begin_handler_func_t */
	NULL, /* fcall_end_handler_func_t */
	NULL, /* op_array_ctor_func_t */
	php_memoize_info_dtor,
	STANDARD_ZEND_EXTENSION_PROPERTIES
}; /* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(memoize)
{
	ZEND_INIT_MODULE_GLOBALS(memoize, php_memoize_init_globals, NULL);

	REGISTER_INI_ENTRIES();

	if (!zend_get_extension("memoize")) {
		zend_register_extension(&zend_extension_entry, NULL);
	}

	if (MG(ini.enabled) && !MG(initialized)) {
		MG(initialized) = 1;

		php_memoize_sma.init(MG(ini.segs), MG(ini.size), NULL);

		php_memoize_cache = apc_cache_create(
			&php_memoize_sma,
		    NULL,
			MG(ini.entries), MG(ini.ttl), MG(ini.ttl), MG(ini.smart), 1
		);

		php_memoize_reserved = zend_get_resource_handle(&zend_extension_entry);
	}

	zend_return_function = zend_get_user_opcode_handler(ZEND_RETURN);
	zend_set_user_opcode_handler(ZEND_RETURN, php_memoize_return);
	zend_do_ucall_function = zend_get_user_opcode_handler(ZEND_DO_UCALL);
	zend_set_user_opcode_handler(ZEND_DO_UCALL, php_memoize_ucall);
	zend_do_fcall_function = zend_get_user_opcode_handler(ZEND_DO_FCALL);
	zend_set_user_opcode_handler(ZEND_DO_FCALL, php_memoize_fcall);
	zend_do_fcall_by_name_function = zend_get_user_opcode_handler(ZEND_DO_FCALL_BY_NAME);
	zend_set_user_opcode_handler(ZEND_DO_FCALL_BY_NAME, php_memoize_fcall_by_name);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(memoize)
{
	if (MG(initialized)) {
		MG(initialized) = 0;

		apc_cache_destroy(
			php_memoize_cache);

		php_memoize_sma.cleanup();
	}

	zend_set_user_opcode_handler(ZEND_RETURN, zend_return_function);
	zend_set_user_opcode_handler(ZEND_DO_UCALL, zend_do_ucall_function);
	zend_set_user_opcode_handler(ZEND_DO_FCALL, zend_do_fcall_function);
	zend_set_user_opcode_handler(ZEND_DO_FCALL_BY_NAME, zend_do_fcall_by_name_function);

	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(memoize)
{
#if defined(COMPILE_DL_MEMOIZE) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(memoize)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(memoize)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "memoize support", MG(ini.enabled) ? "enabled" : "disabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}
/* }}} */

/* {{{ */
static const zend_module_dep php_memoize_deps[] = {
	ZEND_MOD_REQUIRED("APCu")
	ZEND_MOD_REQUIRED("SPL")
	ZEND_MOD_END
}; /* }}} */

/* {{{ */
ZEND_BEGIN_ARG_INFO_EX(php_memoize_info_arginfo, 0, 0, 0)
	ZEND_ARG_TYPE_INFO(0, limited, _IS_BOOL, 0)
ZEND_END_ARG_INFO()
/* }}} */

/* {{{ proto array php_memoize_info([bool limited = false]) */
PHP_FUNCTION(php_memoize_info) 
{
	zend_bool limited = 0;
	zval info;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b", &limited) != SUCCESS) {
		return;
	}

	info = apc_cache_info(php_memoize_cache, limited);

	RETVAL_ZVAL(&info, 0, 0);
} /* }}} */

/* {{{ */
static const zend_function_entry php_memoize_functions[] = {
	PHP_FE(php_memoize_info, php_memoize_info_arginfo)
	PHP_FE_END
}; /* }}} */

/* {{{ memoize_module_entry
 */
zend_module_entry memoize_module_entry = {
    STANDARD_MODULE_HEADER_EX,
    NULL,
    php_memoize_deps,
	PHP_MEMOIZE_EXTNAME,
	php_memoize_functions,
	PHP_MINIT(memoize),
	PHP_MSHUTDOWN(memoize),
	PHP_RINIT(memoize),
	PHP_RSHUTDOWN(memoize),
	PHP_MINFO(memoize),
	PHP_MEMOIZE_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_MEMOIZE
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(memoize)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
