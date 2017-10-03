/*
 * Copyright 2005,2010 WSO2, Inc. http://wso2.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif  

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "wsf.h"
#include "wsf_util.h"
#include "ext/standard/info.h"
#include <Zend/zend.h>
#include "zend_exceptions.h"
#include "zend_globals.h"
#include "zend_objects.h"
#include "wsf_common.h"
#include <axutil_env.h>
#include <axis2_svc_client.h>
#include <axiom_soap.h>
#include <axis2_http_transport.h>
#include <axis2_addr.h>
#include <axis2_const.h>
#include <axiom_util.h>
#include <sandesha2_client.h>
#include "wsf_client.h"
#include "wsf_policy.h"
#include "wsf_xml_msg_recv.h"
#include "wsf_version.h"
#include "php_version.h"
#include <php_main.h>
#include "wsf_wsdl.h"


ZEND_DECLARE_MODULE_GLOBALS(wsf)

zend_class_entry * ws_client_class_entry;
zend_class_entry * ws_service_class_entry;
zend_class_entry * ws_header_class_entry;
zend_class_entry * ws_fault_class_entry;
zend_class_entry * ws_message_class_entry;
zend_class_entry * ws_var_class_entry;
zend_class_entry * ws_client_proxy_class_entry;
zend_class_entry * ws_security_token_class_entry;
zend_class_entry * ws_policy_class_entry;
zend_class_entry * ws_param_class_entry;
zend_class_entry * ws_data_class_entry;

/* True global values, worker is thread safe,
 *  message receiver does not have state*/
static axutil_env_t *env;
static axutil_env_t *ws_env_svr;
static int ws_is_svr;
axis2_msg_recv_t * wsf_msg_recv;
wsf_worker_t * worker;

#define WSF_RESET_GLOBALS() \
		WSF_GLOBAL(soap_version) = AXIOM_SOAP12; \
		WSF_GLOBAL(soap_uri) = AXIOM_SOAP12_SOAP_ENVELOPE_NAMESPACE_URI; \
		WSF_GLOBAL(curr_ns_index) = 0; 


PHP_FUNCTION(is_ws_fault);

/** WSMessage functions */
PHP_METHOD(ws_message, __construct);
PHP_METHOD(ws_message, __destruct);
PHP_METHOD(ws_message, __get);
ZEND_BEGIN_ARG_INFO(__ws_message_get_args, 0)
ZEND_ARG_PASS_INFO(0) ZEND_END_ARG_INFO()
/** Client functions */

PHP_METHOD(ws_client, __construct);
PHP_METHOD(ws_client, __destruct);
PHP_METHOD(ws_client, request);
PHP_METHOD(ws_client, send);
PHP_METHOD(ws_client, get_last_response);
PHP_METHOD(ws_client, get_last_request);
PHP_METHOD(ws_client, get_last_response_headers);
PHP_METHOD(ws_client, get_proxy);
PHP_METHOD(ws_client, terminate_outgoing_rm);
PHP_METHOD(ws_client, wait);


/** WSService */
PHP_METHOD(ws_service, reply);
PHP_METHOD(ws_service, __construct);
PHP_METHOD(ws_service, __destruct);

/** WSHeader class functions */
PHP_METHOD(ws_header, __construct);

/* WSSecurityToken class functions */
PHP_METHOD(ws_security_token, __construct);
PHP_FUNCTION(ws_get_key_from_file);
PHP_FUNCTION(ws_get_cert_from_file);

/** WSFault */
PHP_METHOD(ws_fault, __construct);
PHP_METHOD(ws_fault, __destruct);
PHP_METHOD(ws_fault, __toString);

/** WSData class entry*/
PHP_METHOD(ws_data, __construct);


/** WSPolicy class functions */
PHP_METHOD(ws_policy, __construct);

/** WSClientProxy class functions */
PHP_METHOD(ws_client_proxy, __construct);
PHP_METHOD(ws_client_proxy, __call);
PHP_METHOD(ws_client_proxy, __destruct);
PHP_METHOD(ws_client_proxy, get_functions);
PHP_METHOD(ws_client_proxy, get_types);
PHP_METHOD(ws_client_proxy, get_location);

/** Logging errors */
PHP_FUNCTION(ws_log_write);

ZEND_BEGIN_ARG_INFO(ws_client_proxy_call_args, 0)
ZEND_ARG_PASS_INFO(0)
ZEND_ARG_PASS_INFO(0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(__ws_fault_get_args, 0)
ZEND_ARG_PASS_INFO(0)
ZEND_END_ARG_INFO()

zend_function_entry php_ws_message_class_functions[] = {
    PHP_ME(ws_message, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_message, __destruct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_message, __get, __ws_message_get_args, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};

/** client function entry */
zend_function_entry php_ws_client_class_functions[] = {
    PHP_MALIAS(ws_client, request, request, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client, send, send, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client, wait, wait, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client, getLastResponse, get_last_response, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client, getLastRequest, get_last_request, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client, getProxy, get_proxy, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client, terminateOutgoingRM, terminate_outgoing_rm, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client, getLastResponseHeaders, get_last_response_headers, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_client, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_client, __destruct, NULL, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};

zend_function_entry php_ws_client_proxy_class_functions[] = {
    PHP_ME(ws_client_proxy, __call, ws_client_proxy_call_args, ZEND_ACC_PUBLIC)
    PHP_ME(ws_client_proxy, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_client_proxy, __destruct, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client_proxy, getFunctions, get_functions, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client_proxy, getTypes, get_types, NULL, ZEND_ACC_PUBLIC)
    PHP_MALIAS(ws_client_proxy, getLocation, get_location, NULL, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};

/** service function entry */
zend_function_entry php_ws_service_class_functions[] = {
    PHP_MALIAS(ws_service, reply, reply, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_service, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_service, __destruct, NULL, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};

/** WSFault class entry */
zend_function_entry php_ws_fault_class_functions[] = {
    PHP_ME(ws_fault, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_fault, __destruct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(ws_fault, __toString, NULL, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};

/* {{{ WSHeader class functions */
zend_function_entry php_ws_header_class_functions[] = {
    PHP_ME(ws_header, __construct, NULL, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};

zend_function_entry php_ws_security_token_class_functions[] = {
    PHP_ME(ws_security_token, __construct, NULL, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};

zend_function_entry php_ws_policy_class_functions[] = {
    PHP_ME(ws_policy, __construct, NULL, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};

zend_function_entry php_ws_data_class_functions[] = {
    PHP_ME(ws_data, __construct, NULL, ZEND_ACC_PUBLIC) {
        NULL, NULL, NULL}
};


/* {{{ wsf_functions[] */
zend_function_entry wsf_functions[] = {
    PHP_FE(is_ws_fault, NULL)
    PHP_FE(ws_get_key_from_file, NULL)
    PHP_FE(ws_get_cert_from_file, NULL)
    PHP_FE(ws_log_write, NULL) {
        NULL, NULL, NULL}
};
/* }}} */

static zend_object_handlers ws_object_handlers;

/** object creation and destruction functions */
static zend_object_value
php_ws_object_new(zend_class_entry * class_type TSRMLS_DC);

static zend_object_value
php_ws_object_new_ex(zend_class_entry * class_type, ws_object ** obj TSRMLS_DC);

static void ws_objects_free_storage(void *object TSRMLS_DC);

/* {{{ proto wsf_log_version_string 
 */
void wsf_log_version_string(axutil_env_t *env) {
    if (!env || !env->log)
        return;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "wsf version - %s ", WSF_VERSION_STRING);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "php version - %s ", PHP_VERSION);

}
/* }}} */

/* {{{ wsf_module_entry */
zend_module_entry wsf_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901 
    STANDARD_MODULE_HEADER,
#endif  /*  */
    "wsf", wsf_functions,
    PHP_MINIT(wsf),
    PHP_MSHUTDOWN(wsf),
    PHP_RINIT(wsf),
    PHP_RSHUTDOWN(wsf),
    PHP_MINFO(wsf),
#if ZEND_MODULE_API_NO >= 20010901 
    "1.0",
#endif  
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_WSF

ZEND_GET_MODULE(wsf)
#endif  /*  */

/* {{{ PHP_INI */
PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("wsf.home", NULL, PHP_INI_ALL,
        OnUpdateString, home, zend_wsf_globals, wsf_globals)
STD_PHP_INI_ENTRY("wsf.log_path", WSF_TMP, PHP_INI_ALL,
        OnUpdateString, log_path, zend_wsf_globals, wsf_globals)
STD_PHP_INI_ENTRY("wsf.log_level", "4", PHP_INI_ALL,
        OnUpdateLong, log_level, zend_wsf_globals, wsf_globals)
STD_PHP_INI_ENTRY("wsf.rm_db_dir", WSF_TMP, PHP_INI_ALL,
        OnUpdateString, rm_db_dir, zend_wsf_globals, wsf_globals)
STD_PHP_INI_ENTRY("wsf.attachment_cache_dir", WSF_TMP, PHP_INI_ALL,
        OnUpdateString, attachment_cache_dir, zend_wsf_globals, wsf_globals)
STD_PHP_INI_ENTRY("wsf.enable_attachment_caching", "0", PHP_INI_ALL,
        OnUpdateLong, enable_attachment_caching, zend_wsf_globals, wsf_globals)

PHP_INI_END()
/* }}} */

/* {{{ ws_init_globals
 */
static void ws_init_globals(zend_wsf_globals * wsf_globals) {
    wsf_globals->home = NULL;
    wsf_globals->log_path = NULL;
    wsf_globals->log_level = 1;
    wsf_globals->soap_version = AXIOM_SOAP12;
    wsf_globals->soap_uri = AXIOM_SOAP12_SOAP_ENVELOPE_NAMESPACE_URI;
    wsf_globals->rm_db_dir = NULL;
    wsf_globals->curr_ns_index = 0;
    wsf_globals->attachment_cache_dir = NULL;
    wsf_globals->enable_attachment_caching = 0;
}

/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(wsf) {
    zend_class_entry ce;

    char *home_folder = NULL;

    ZEND_INIT_MODULE_GLOBALS(wsf, ws_init_globals, NULL);

    REGISTER_INI_ENTRIES();
    if (INI_STR("extension_dir")) {
        char *home_dir = pemalloc(strlen(INI_STR("extension_dir")) + strlen("/wsf_c") + 1, 1);
        strcpy(home_dir, INI_STR("extension_dir"));
        strcat(home_dir, "/wsf_c");
        home_folder = home_dir;
    }

    memcpy(&ws_object_handlers, zend_get_std_object_handlers(), sizeof (zend_object_handlers));

    ws_object_handlers.clone_obj = NULL;

    REGISTER_WSF_CLASS(ce, "WSClient", NULL,
            php_ws_client_class_functions, ws_client_class_entry);

    INIT_CLASS_ENTRY(ce, "WSClientProxy", php_ws_client_proxy_class_functions);
    ws_client_proxy_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

    REGISTER_WSF_CLASS(ce, "WSService", NULL, php_ws_service_class_functions, ws_service_class_entry);

    INIT_CLASS_ENTRY(ce, "WSMessage", php_ws_message_class_functions);
    ws_message_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

    INIT_CLASS_ENTRY(ce, "WSHeader", php_ws_header_class_functions);
    ws_header_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

    INIT_CLASS_ENTRY(ce, "WSFault", php_ws_fault_class_functions);
#ifdef ZEND_ENGINE_2    
    ws_fault_class_entry = zend_register_internal_class_ex(&ce,
            zend_exception_get_default(TSRMLS_C), NULL TSRMLS_CC);
#else    
    ws_fault_class_entry = zend_register_internal_class(&ce TSRMLS_CC);
#endif

    INIT_CLASS_ENTRY(ce, "WSSecurityToken", php_ws_security_token_class_functions);
    ws_security_token_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

    INIT_CLASS_ENTRY(ce, "WSPolicy", php_ws_policy_class_functions);
    ws_policy_class_entry = zend_register_internal_class(&ce TSRMLS_CC);

	INIT_CLASS_ENTRY(ce, "WSData", php_ws_data_class_functions);
    ws_data_class_entry = zend_register_internal_class(&ce TSRMLS_CC);


    REGISTER_LONG_CONSTANT("WS_SOAP_ROLE_NEXT", WSF_SOAP_ROLE_NEXT, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("WS_SOAP_ROLE_NONE", WSF_SOAP_ROLE_NONE, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("WS_SOAP_ROLE_ULTIMATE_RECEIVER", WSF_SOAP_ROLE_ULTIMATE_RECEIVER, CONST_CS | CONST_PERSISTENT);

    REGISTER_LONG_CONSTANT("WS_SCT_ID_GLOBAL", WS_SCT_ID_GLOBAL, CONST_CS | CONST_PERSISTENT);
    REGISTER_LONG_CONSTANT("WS_SCT_ID_LOCAL", WS_SCT_ID_LOCAL, CONST_CS | CONST_PERSISTENT);

    if (WSF_GLOBAL(home)) {
        home_folder = WSF_GLOBAL(home);
    } else {
        WSF_GLOBAL(home) = home_folder;
    }

    env = wsf_env_create(WSF_GLOBAL(log_path));
    env->log->level = WSF_GLOBAL(log_level);

    ws_env_svr = wsf_env_create_svr(WSF_GLOBAL(log_path));
    ws_env_svr->log->level = WSF_GLOBAL(log_level);

    wsf_log_version_string(ws_env_svr);
    wsf_log_version_string(env);

    wsf_msg_recv = wsf_xml_msg_recv_create(ws_env_svr);

    axiom_xml_reader_init();
    worker = wsf_worker_create(ws_env_svr, home_folder,
            axutil_strdup(ws_env_svr, WSF_GLOBAL(rm_db_dir)));

    return SUCCESS;
}


/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(wsf) {
    wsf_worker_free(worker, ws_env_svr);
    axis2_msg_recv_free(wsf_msg_recv, ws_env_svr);
    axutil_env_free(env);
    axutil_env_free(ws_env_svr);
    axiom_xml_reader_cleanup();
    UNREGISTER_INI_ENTRIES();
    return SUCCESS;
}


/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(wsf) {
    WSF_GLOBAL(soap_version) = AXIOM_SOAP12;
    return SUCCESS;
}


/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(wsf) {
    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(wsf) {
    php_info_print_table_start();
    php_info_print_table_row(2, "wsf support", "enabled");
    php_info_print_table_row(2, "wsf version", WSF_VERSION_STRING);
    php_info_print_table_end();
    DISPLAY_INI_ENTRIES();
}


/* }}} */

/* {{{ ws_object_new */
static zend_object_value
php_ws_object_new(zend_class_entry * class_type TSRMLS_DC) {
    ws_object * tmp;
    return php_ws_object_new_ex(class_type, &tmp TSRMLS_CC);
}
/* }}} */

/* {{{ ws_object_new_ex */
static zend_object_value
php_ws_object_new_ex(
        zend_class_entry * class_type,
        ws_object ** obj TSRMLS_DC) {
    zend_object_value retval;
    ws_object * intern;
    zval * tmp;

    intern = emalloc(sizeof (ws_object));
    memset(intern, 0, sizeof (ws_object));
    intern->std.ce = class_type;
    intern->obj_type = WSF_NONE;
    *obj = intern;

    ALLOC_HASHTABLE(intern->std.properties);
    zend_hash_init(intern->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0);
#if PHP_VERSION_ID < 50399
    zend_hash_copy(intern->std.properties, &class_type->default_properties,
            (copy_ctor_func_t) zval_add_ref, (void *) & tmp, sizeof (void *));
#else
        object_properties_init(&(intern->std), class_type); 
#endif

    retval.handle = zend_objects_store_put(intern,
            (zend_objects_store_dtor_t) zend_objects_destroy_object,
            (zend_objects_free_object_storage_t) ws_objects_free_storage, NULL TSRMLS_CC);

    retval.handlers = &ws_object_handlers;
    return retval;
}

/* }}} */

static void
ws_objects_free_storage(void *object TSRMLS_DC) {
    ws_object * intern = NULL;
    intern = (ws_object *) object;
    if (!intern) return;

    if (intern->std.properties) {
        zend_hash_destroy(intern->std.properties);
        FREE_HASHTABLE(intern->std.properties);
    }
    if (intern->obj_type == WSF_SVC_CLIENT) {
        axis2_svc_client_t * svc_client = NULL;
        svc_client = (axis2_svc_client_t *) intern->ptr;

        if (svc_client) {
            if (!wsf_util_is_module_engaged_to_svc_client(svc_client, env, WSF_MODULE_RM)) {
                axis2_svc_client_free(svc_client, env);
            }
        }
    } else if (intern->obj_type == WSF_SVC) {
        wsf_svc_info_t * svc_info = NULL;
        svc_info = (wsf_svc_info_t *) intern->ptr;
        if (svc_info) {
            wsf_svc_info_free(svc_info, ws_env_svr);
        }
    }
    intern->ptr = NULL;
    efree(object);
}

/*** {{{ is_ws_fault(Object obj) */
PHP_FUNCTION(is_ws_fault) {
    zval * object = NULL;
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "z", &object)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        return;
    }
    if (Z_TYPE_P(object) == IS_OBJECT && instanceof_function(Z_OBJCE_P(object),
            ws_fault_class_entry TSRMLS_CC)) {
        RETURN_TRUE;

    } else {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto WSMessage::__construct(mixed payload[, array properties]) */
PHP_METHOD(ws_message, __construct) {
    zval * object = NULL;
    zval * payload = NULL;
    zval * properties = NULL;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "z|a", &payload, &properties)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        return;
    }

    WSF_GET_THIS(object);
    WSF_OBJ_CHECK(env);

    if (Z_TYPE_P(payload) == IS_STRING) {
        add_property_stringl(object, WSF_MSG_PAYLOAD_STR, Z_STRVAL_P(payload), Z_STRLEN_P(payload), 1);
    } else {
        return;
    }
    if (NULL != properties) {
        zval ** tmp;
        HashTable * ht = Z_ARRVAL_P(properties);
        if (!ht) {
            return;
        }

        if (zend_hash_find(ht, WSF_ATTACHMENTS, sizeof (WSF_ATTACHMENTS),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {
            add_property_zval(object, WSF_ATTACHMENTS, *tmp);
        }
        if (zend_hash_find(ht, WSF_DEFAULT_ATTACHEMENT_CONTENT_TYPE, sizeof (WSF_DEFAULT_ATTACHEMENT_CONTENT_TYPE),
                (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_DEFAULT_ATTACHEMENT_CONTENT_TYPE,
                    Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
        }

        if (zend_hash_find(ht, WSF_LAST_MESSAGE, sizeof (WSF_LAST_MESSAGE),
                (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL) {
            add_property_bool(object, WSF_LAST_MESSAGE, Z_BVAL_PP(tmp));
        }

        if (zend_hash_find(ht, WSF_TO, sizeof (WSF_TO),
                (void**) & tmp) == SUCCESS) {
            if (Z_TYPE_PP(tmp) == IS_STRING) {
                add_property_stringl(object, WSF_TO, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
            } else if (Z_TYPE_PP(tmp) == IS_ARRAY) {
                add_property_zval(object, WSF_TO, *tmp);
            }
        }
        if (zend_hash_find(ht, WSF_ACTION, sizeof (WSF_ACTION),
                (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_ACTION, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
        }
		if (zend_hash_find(ht, WSF_ADDR_MUST_UNDERSTAND, sizeof (WSF_ADDR_MUST_UNDERSTAND),
			(void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL) {
				add_property_bool(object, WSF_ADDR_MUST_UNDERSTAND, Z_BVAL_PP(tmp));
        }
        if (zend_hash_find(ht, WSF_REPLY_TO, sizeof (WSF_REPLY_TO),
                (void**) & tmp) == SUCCESS) {
            if (Z_TYPE_PP(tmp) == IS_STRING) {
                add_property_stringl(object, WSF_REPLY_TO, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
            } else if (Z_TYPE_PP(tmp) == IS_ARRAY) {
                add_property_zval(object, WSF_REPLY_TO, *tmp);
            }
        }
        if (zend_hash_find(ht, WSF_FAULT_TO, sizeof (WSF_FAULT_TO),
                (void**) & tmp) == SUCCESS) {
            if (Z_TYPE_PP(tmp) == IS_STRING) {
                add_property_stringl(object, WSF_FAULT_TO, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
            } else if (Z_TYPE_PP(tmp) == IS_ARRAY) {
                add_property_zval(object, WSF_FAULT_TO, *tmp);
            }
        }
        if (zend_hash_find(ht, WSF_FROM, sizeof (WSF_FROM),
                (void**) & tmp) == SUCCESS) {
            if (Z_TYPE_PP(tmp) == IS_STRING) {
                add_property_stringl(object, WSF_FROM, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
            } else if (Z_TYPE_PP(tmp) == IS_ARRAY) {
                add_property_zval(object, WSF_FROM, *tmp);
            }

        }
        if (zend_hash_find(ht, WSF_REST_CONTENT_TYPE, sizeof (WSF_REST_CONTENT_TYPE),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object,
                    WSF_REST_CONTENT_TYPE, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);

        }
        if (zend_hash_find(ht, WSF_INPUT_HEADERS, sizeof (WSF_INPUT_HEADERS),
                (void **) & tmp) == SUCCESS) {
            if (Z_TYPE_PP(tmp) == IS_ARRAY) {
                add_property_zval(object, WSF_INPUT_HEADERS, *tmp);
            }
        }
    }
}


/* }}} */

/** desctructor */
PHP_METHOD(ws_message, __destruct) {
}

/* {{{ WSMessage::__get() */
PHP_METHOD(ws_message, __get) {
}
/* }}} */

/* {{{ proto void WSClient::__construct(string uri[, array options]) */
PHP_METHOD(ws_client, __construct) {
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    char *home_folder = NULL;
    axis2_svc_client_t * svc_client = NULL;
    zval * options = NULL;
    ws_is_svr = 0;
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "|a", &options)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        return;
    }

    WSF_OBJ_CHECK(env);

    WSF_GET_THIS(obj);

    intern = (ws_object *) zend_object_store_get_object(obj TSRMLS_CC);

    if (INI_STR("extension_dir")) {
        char *home_dir = pemalloc(strlen(INI_STR("extension_dir")) + strlen("/wsf_c") + 1, 1);
        strcpy(home_dir, INI_STR("extension_dir"));
        strcat(home_dir, "/wsf_c");
        home_folder = home_dir;
    }
    if (WSF_GLOBAL(home))
        home_folder = WSF_GLOBAL(home);

    svc_client = axis2_svc_client_create(env, home_folder);

    if (!svc_client) {
        zend_throw_exception_ex(zend_exception_get_default(TSRMLS_C), 1 TSRMLS_CC, "ws client create failed");
    }


    intern->ptr = svc_client;
    intern->obj_type = WSF_SVC_CLIENT;

    if (NULL != options) {
        HashTable * ht = Z_ARRVAL_P(options);
        if (!ht) return;
        /* add properties defined in API doc */
        wsf_client_add_properties(obj, ht TSRMLS_CC);
    }
    if (WSF_GLOBAL(enable_attachment_caching)) {
        axis2_svc_ctx_t *svc_ctx = NULL;
        axis2_conf_ctx_t *conf_ctx = NULL;
        axutil_param_t *cache_dir = NULL;
        axis2_conf_t *conf = NULL;
        svc_ctx = axis2_svc_client_get_svc_ctx(svc_client, env);
        conf_ctx = axis2_svc_ctx_get_conf_ctx(svc_ctx, env);
        conf = axis2_conf_ctx_get_conf(conf_ctx, env);

        cache_dir = axutil_param_create(env, AXIS2_ATTACHMENT_DIR,
                axutil_strdup(env, WSF_GLOBAL(attachment_cache_dir)));
        axis2_conf_add_param(conf, env, cache_dir);
    }
}
/* }}} */

/* {{{ WSClient::__destruct() */
PHP_METHOD(ws_client, __destruct) {

}
/* }}} */

/* {{{ proto string request(mixed payload)
        payload can be a string, dom element.
        client sends and receiveds the payload
 */
PHP_METHOD(ws_client, request) {
    zval * param = NULL;
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    axis2_svc_client_t * svc_client = NULL;
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "z", &param)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        return;
    }

    WSF_OBJ_CHECK(env);
    WSF_GET_THIS(obj);
    WSF_GET_OBJ(svc_client, obj, axis2_svc_client_t, intern);
    wsf_client_do_request(obj, param, return_value, env, svc_client, AXIS2_FALSE TSRMLS_CC);
    WSF_RESET_GLOBALS();
}


/* }}} end request */

/* {{{ proto send(string xmlpaylod)
    Sends the given payload in a robust manner.
    Means that SOAP faults are captured and reported.
 */
PHP_METHOD(ws_client, send) {
    zval * param = NULL;
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    axis2_svc_client_t * svc_client = NULL;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "z", &param)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        return;
    }
    WSF_OBJ_CHECK(env);
    WSF_GET_THIS(obj);
    WSF_GET_OBJ(svc_client, obj, axis2_svc_client_t, intern);
    wsf_client_do_request(obj, param, return_value, env, svc_client,
            AXIS2_TRUE TSRMLS_CC);

    WSF_RESET_GLOBALS();
}


/* }}} end send*/

/* {{{ proto getLastResponse() */
PHP_METHOD(ws_client, get_last_response) {
    axis2_svc_client_t * svc_client = NULL;
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    WSF_GET_THIS(obj);
    WSF_GET_OBJ(svc_client, obj, axis2_svc_client_t, intern);

    if (svc_client) {
        axis2_op_client_t * op_client = NULL;
        op_client = axis2_svc_client_get_op_client(svc_client, env);
        if (op_client) {
            axis2_char_t * msg =
                    wsf_util_get_soap_msg_from_op_client(op_client, env,
                    AXIS2_WSDL_MESSAGE_LABEL_IN);
            if (msg) {
                RETURN_STRING(msg, 1);
            }
        }
    }
}
/* }}} */

/* {{{ proto getLastRequest() */
PHP_METHOD(ws_client, get_last_request) {
    axis2_svc_client_t * svc_client = NULL;
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    WSF_GET_THIS(obj);
    WSF_GET_OBJ(svc_client, obj, axis2_svc_client_t, intern);
    if (svc_client) {
        axis2_op_client_t * op_client = NULL;
        op_client = axis2_svc_client_get_op_client(svc_client, env);
        if (op_client) {
            axis2_char_t * msg =
                    wsf_util_get_soap_msg_from_op_client(op_client, env,
                    AXIS2_WSDL_MESSAGE_LABEL_OUT);
            if (msg) {
                RETURN_STRING(msg, 1);
            }
        }
    }
}
/* }}} */

/* {{{ proto public WSClient::terminateOutgoingRM() */
PHP_METHOD(ws_client, terminate_outgoing_rm) {
    axis2_svc_client_t * svc_client = NULL;
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    HashTable *client_ht = NULL;
    int rm_spec_version = -1;
    WSF_GET_THIS(obj);

    WSF_GET_OBJ(svc_client, obj, axis2_svc_client_t, intern);
    client_ht = Z_OBJPROP_P(this_ptr);
    if (svc_client && rm_spec_version == WSF_RM_VERSION_1_1) {
        sandesha2_client_close_seq_with_svc_client(env, svc_client, NULL);
        sandesha2_client_terminate_seq_with_svc_client(env, svc_client, NULL);
    }
}
/* }}} */

/* {{{ proto getLastResponseHeaders() */
PHP_METHOD(ws_client, get_last_response_headers) {
    axis2_svc_client_t * svc_client = NULL;
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    WSF_GET_THIS(obj);
    WSF_GET_OBJ(svc_client, obj, axis2_svc_client_t, intern);
    if (svc_client) {
        axis2_op_client_t * op_client = NULL;
        op_client = axis2_svc_client_get_op_client(svc_client, env);
        if (op_client) {
            wsf_util_get_http_headers_from_op_client(op_client, env, return_value,
                    AXIS2_WSDL_MESSAGE_LABEL_OUT);
            return;
        }
    }
    RETURN_NULL();
}
/* }}} */

/* }}} end call */
PHP_METHOD(ws_client, get_proxy) {
    zval * client_proxy_zval = NULL;
    char *service = NULL;
    int service_len = 0;
    char *port = NULL;
    int port_len = 0;
    zval * obj = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "|ss", &service,
            &service_len, &port, &port_len) == FAILURE) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR,
                "Invalid Parameters,specify the service and port");
    }
    WSF_GET_THIS(obj);
    MAKE_STD_ZVAL(client_proxy_zval);
    object_init_ex(client_proxy_zval, ws_client_proxy_class_entry);
    if (service) {
        add_property_string(client_proxy_zval, WSF_SERVICE_NAME, service, 1);
    }
    if (port) {
        add_property_string(client_proxy_zval, WSF_PORT_NAME, port, 1);
    }

    add_property_zval(client_proxy_zval, "wsclient", this_ptr);

    /* load the wsdl information if available */
    wsf_wsdl_extract_wsdl_information_for_client(client_proxy_zval, env TSRMLS_CC);

    RETURN_ZVAL(client_proxy_zval, 0, 1);
}

/* {{{ proto void WSService::__construct([ array options])*/
PHP_METHOD(ws_service, __construct) {
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    wsf_svc_info_t * svc_info = NULL;
    zval * options = NULL;
    zval **server_vars = NULL;
    zval **data = NULL;
    HashTable * ht_options = NULL;
    HashTable * ht_actions = NULL;
    HashTable * ht_ops_to_funcs = NULL;
    HashTable * ht_ops_to_mep = NULL;
    HashTable * ht_opParams = NULL;
    HashTable * ht_classes = NULL;
    HashTable * ht_annotations = NULL;
    HashTable * ht_rest_map = NULL;
    char *service_name = NULL;
    char *port_name = NULL;
    char *request_uri = NULL;
    ws_is_svr = 1;

    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "|a",
            &options)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        return;
    }

    WSF_GET_THIS(obj);
    intern = (ws_object *) zend_object_store_get_object(obj TSRMLS_CC);

    svc_info = wsf_svc_info_create(ws_env_svr);

    svc_info->ops_to_functions = axutil_hash_make(ws_env_svr);

    svc_info->ops_to_actions = axutil_hash_make(ws_env_svr);

    intern->ptr = svc_info;

    intern->obj_type = WSF_SVC;

    svc_info->php_worker = worker;

    if (options) {
        zval ** tmp = NULL;
        ht_options = Z_ARRVAL_P(options);
        if (ht_options) {
            if (zend_hash_find(ht_options, WSF_ACTIONS, sizeof (WSF_ACTIONS),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {
                ht_actions = Z_ARRVAL_PP(tmp);
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
						      " setting actions ");

                svc_info->wsdl_gen_actions = *tmp;
                zval_add_ref(&(svc_info->wsdl_gen_actions));
            }
            if (zend_hash_find(ht_options, WSF_OPERATIONS, sizeof (WSF_OPERATIONS),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {

                ht_ops_to_funcs = Z_ARRVAL_PP(tmp);
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
						"setting operations");
            }

            if (zend_hash_find(ht_options, WSF_OP_MEP, sizeof (WSF_OP_MEP),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {

                ht_ops_to_mep = Z_ARRVAL_PP(tmp);
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
						" setting message exchange pattern");
            }
            if (zend_hash_find(ht_options, WSF_OP_PARAMS, sizeof (WSF_OP_PARAMS),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {

                ht_opParams = Z_ARRVAL_PP(tmp);
                zval_add_ref(tmp);
                svc_info->ht_op_params = ht_opParams;
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
						" setting message operation parameters");
            }
            if (zend_hash_find(ht_options, WSF_OP_TO_URL_MAP, sizeof (WSF_OP_TO_URL_MAP),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {
                ht_rest_map = Z_ARRVAL_PP(tmp);
            }
            if (zend_hash_find(ht_options, WSF_CACHE_WSDL, sizeof (WSF_CACHE_WSDL),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL) {
                svc_info->cache_wsdl = Z_BVAL_PP(tmp);
            }
            if (zend_hash_find(ht_options, WSF_OMIT_WSDL, sizeof (WSF_OMIT_WSDL),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL) {
                svc_info->omit_wsdl = Z_BVAL_PP(tmp);
            }
            if (zend_hash_find(ht_options, WSF_WSDL, sizeof (WSF_WSDL),
                    (void **) & tmp) == SUCCESS) {
                axis2_char_t *wsdl_path = NULL;
                int wsdl_path_length = 0;
                if (Z_TYPE_PP(tmp) == IS_STRING) {
                    wsdl_path = Z_STRVAL_PP(tmp);
                    wsdl_path_length = Z_STRLEN_PP(tmp);
                } else if (Z_TYPE_PP(tmp) == IS_ARRAY) {
                    zval **tmpval = NULL;
                    HashTable *ht = Z_ARRVAL_PP(tmp);

                    if (zend_hash_find(ht, WSF_WSA_ADDRESS, sizeof (WSF_WSA_ADDRESS),
                            (void **) & tmpval) == SUCCESS && Z_TYPE_PP(tmpval) == IS_STRING) {
                        wsdl_path = Z_STRVAL_PP(tmpval);
                        wsdl_path_length = Z_STRLEN_PP(tmpval);
                    }
                    if (zend_hash_find(ht, WSF_HTTP_AUTH_USERNAME, sizeof (WSF_HTTP_AUTH_USERNAME),
                            (void **) & tmpval) == SUCCESS && Z_TYPE_PP(tmpval) == IS_STRING) {
                        svc_info->auth_user = axutil_strdup(env, Z_STRVAL_PP(tmpval));
                    }
                    if (zend_hash_find(ht, WSF_HTTP_AUTH_PASSWORD, sizeof (WSF_HTTP_AUTH_PASSWORD),
                            (void **) & tmpval) == SUCCESS && Z_TYPE_PP(tmpval) == IS_STRING) {
                        svc_info->auth_password = axutil_strdup(env, Z_STRVAL_PP(tmpval));
                    }
                    if (zend_hash_find(ht, WSF_HTTP_AUTH_TYPE, sizeof (WSF_HTTP_AUTH_TYPE),
                            (void **) & tmpval) == SUCCESS && Z_TYPE_PP(tmpval) == IS_STRING) {
                        svc_info->auth_type = axutil_strdup(env, Z_STRVAL_PP(tmpval));
                    }
                }
                svc_info->wsdl = axutil_strdup(ws_env_svr, wsdl_path);
                add_property_stringl(this_ptr, WSF_WSDL, wsdl_path, wsdl_path_length, 1);
            }
            if (zend_hash_find(ht_options, WSF_CLASSMAP, sizeof (WSF_CLASSMAP),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {
                add_property_zval(this_ptr, WSF_CLASSMAP, *tmp);
                svc_info->class_map = *tmp;
                zval_add_ref(&(svc_info->class_map));
            }

            if (zend_hash_find(ht_options, WSF_USE_MTOM, sizeof (WSF_USE_MTOM),
                    (void **) & tmp) == SUCCESS) {
                if (Z_TYPE_PP(tmp) == IS_BOOL) {
                    svc_info->use_mtom = Z_BVAL_PP(tmp);
                }
                if (Z_TYPE_PP(tmp) == IS_STRING) {
                    char *value = NULL;
                    value = Z_STRVAL_PP(tmp);
                    if (value && (stricmp(value, WSF_SWA) == 0)) {

                        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                                " SwA enabled");
                        svc_info->use_mtom = 1; /* mtom */
                        svc_info->enable_swa = 1; /* mtom swa*/
                    }

                }
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                        " setting mtom property %d", svc_info->use_mtom);
            } else {
                svc_info->use_mtom = 0;
            }

            if (zend_hash_find(ht_options, WSF_USE_WSA, sizeof (WSF_USE_WSA),
                    (void **) & tmp) == SUCCESS) {
                if (Z_TYPE_PP(tmp) == IS_BOOL) {
                    svc_info->use_wsa = Z_BVAL_PP(tmp);
                }
            }

            if (zend_hash_find(ht_options, WSF_REQUEST_XOP, sizeof (WSF_REQUEST_XOP),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL) {

                svc_info->request_xop = Z_BVAL_PP(tmp);
            } else {
                svc_info->request_xop = 0;
            }

            AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                " request xop %d", svc_info->request_xop);

            if (zend_hash_find(ht_options, WSF_POLICY_NAME, sizeof (WSF_POLICY_NAME),
                    (void **) & tmp) == SUCCESS && Z_OBJCE_PP(tmp) == ws_policy_class_entry) {
                svc_info->policy = *tmp;
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
						" policy object present");
            }

            if (zend_hash_find(ht_options, WSF_OP_POLICIES, sizeof (WSF_OP_POLICIES),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {
                svc_info->ht_op_policies = Z_ARRVAL_PP(tmp);
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
						"operation policy object present");
            }
            if (zend_hash_find(ht_options, WSF_SECURITY_TOKEN, sizeof (WSF_SECURITY_TOKEN),
                    (void **) & tmp) == SUCCESS) {

                svc_info->security_token = *tmp;
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
						" security token object present ");
            }
            if (zend_hash_find(ht_options, WSF_RELIABLE, sizeof (WSF_RELIABLE),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL && Z_BVAL_PP(tmp) == 1) {

                if (!svc_info->modules_to_engage) {
                    svc_info->modules_to_engage = axutil_array_list_create(ws_env_svr, 3);
                }
                axutil_array_list_add(svc_info->modules_to_engage,
                        ws_env_svr, axutil_strdup(ws_env_svr, WSF_MODULE_RM));
            }
            if (zend_hash_find(ht_options, WSF_USE_OWN_STS, sizeof (WSF_USE_OWN_STS),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL && Z_BVAL_PP(tmp) == 1) {

                if (!svc_info->modules_to_engage) {
                    svc_info->modules_to_engage = axutil_array_list_create(ws_env_svr, 3);
                }
                axutil_array_list_add(svc_info->modules_to_engage,
                        ws_env_svr, axutil_strdup(ws_env_svr, WSF_MODULE_SC));
            }
            if (zend_hash_find(ht_options, WSF_BINDING_STYLE, sizeof (WSF_BINDING_STYLE),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
                add_property_stringl(obj, WSF_BINDING_STYLE,
                        Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
            }

            if (zend_hash_find(ht_options, WSF_CLASSES, sizeof (WSF_CLASSES),
                    (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {
                ht_classes = Z_ARRVAL_PP(tmp);

                svc_info->wsdl_gen_class_map = *tmp;
                zval_add_ref(&(svc_info->wsdl_gen_class_map));
            }
            if (zend_hash_find(ht_options, WSF_SERVICE_NAME, sizeof (WSF_SERVICE_NAME),
                    (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
                service_name = Z_STRVAL_PP(tmp);
            }
            if (zend_hash_find(ht_options, WSF_PORT_NAME, sizeof (WSF_PORT_NAME),
                    (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
                port_name = Z_STRVAL_PP(tmp);
            }
            if (zend_hash_find(ht_options, WSF_ANNOTATIONS, sizeof (WSF_ANNOTATIONS),
                    (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {
                ht_annotations = Z_ARRVAL_PP(tmp);

                svc_info->wsdl_gen_annotations = *tmp;
                zval_add_ref(&(svc_info->wsdl_gen_annotations));
            }

        }
    }

    zend_is_auto_global("_SERVER", sizeof ("_SERVER") - 1 TSRMLS_CC);

    if (zend_hash_find(&EG(symbol_table), "_SERVER", sizeof ("_SERVER"),
            (void **) & server_vars) == SUCCESS && (Z_TYPE_PP(server_vars) == IS_ARRAY)) {
        if ((zend_hash_find(Z_ARRVAL_PP(server_vars),
                "PHP_SELF", sizeof ("PHP_SELF"),
                (void **) & data) == SUCCESS) && Z_TYPE_PP(data) == IS_STRING) {
            request_uri = Z_STRVAL_PP(data);
        }
    }

    if (request_uri) { /** SG(request_info).request_uri */
        svc_info->svc_name =
                wsf_util_generate_svc_name_from_uri_and_set_loc_str(request_uri, svc_info, ws_env_svr);
        if (service_name) {
            svc_info->svc_name = strdup(service_name);
        } else {
            svc_info->generated_svc_name = 1;
        }
        svc_info->msg_recv = wsf_msg_recv;
        wsf_util_create_svc_from_svc_info(svc_info, ws_env_svr TSRMLS_CC);
    } else if (SG(request_info).path_translated) {
        svc_info->svc_name = wsf_util_generate_svc_name_from_uri_and_set_loc_str(
                SG(request_info).path_translated, svc_info, ws_env_svr);
        if (service_name) {
            svc_info->svc_name = strdup(service_name);
        } else {
            svc_info->generated_svc_name = 1;
        }
        svc_info->msg_recv = wsf_msg_recv;
        wsf_util_create_svc_from_svc_info(svc_info, ws_env_svr TSRMLS_CC);
    }

    if (port_name) {
        svc_info->port_name = strdup(port_name);
    }

    wsf_util_process_ws_service_operations(ht_ops_to_funcs, ht_ops_to_mep, svc_info, ws_env_svr TSRMLS_CC);

    if (ht_classes) {
        wsf_util_process_ws_service_classes(ht_classes, ht_ops_to_mep, svc_info, ws_env_svr TSRMLS_CC);
    }

    wsf_util_add_svc_to_conf(ws_env_svr, svc_info, wsf_worker_get_conf_ctx(worker, ws_env_svr));

    wsf_util_engage_modules_to_svc(ws_env_svr, wsf_worker_get_conf_ctx(worker, ws_env_svr), svc_info);

    wsf_util_process_ws_service_op_actions(ht_actions, svc_info, ws_env_svr TSRMLS_CC);

    wsf_util_process_rest_params(ws_env_svr, svc_info, ht_rest_map TSRMLS_CC);

    /*
    if(zend_hash_find(Z_OBJPROP_P(this_ptr), WSF_WSDL,
                      sizeof(WSF_WSDL), (void **)&wsdl_tmp) == SUCCESS){
        wsf_wsdl_process_service(svc_info, ws_env_svr TSRMLS_CC);
    }
     */

    if (SG(request_info).query_string &&
            ((stricmp(SG(request_info).query_string, WSF_WSDL) == 0)
            || (stricmp(SG(request_info).query_string, WSF_WSDL2) == 0))) {
        /** skip for WSDL Generation */

    } else {
        if (svc_info->security_token && (svc_info->policy || svc_info->ht_op_policies)) {
            axis2_conf_t * conf = NULL;
            conf = axis2_conf_ctx_get_conf(wsf_worker_get_conf_ctx(worker, ws_env_svr), env);
            wsf_policy_handle_server_security(svc_info, env, conf TSRMLS_CC);
        }
    }
}

PHP_METHOD(ws_client, wait) {
    long num;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &num) == FAILURE) {
        return;
    }
    if (num < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Number of seconds must be greater than or equal to 0");
        return;
    }
    php_sleep(num);
}


/* }}} */

/* {{{ proto void WSService::__destruct()
 */
PHP_METHOD(ws_service, __destruct) {
}

/* } }} end WSService::__destruct */

static void
serve_static_wsdl(char *wsdl_location TSRMLS_DC)
{
	int len = 0;
    char *val = NULL;
    int args_count = 0;
	zval function, retval, *param = NULL;
    int new_len = 0;
      
    INIT_ZVAL(retval);
    MAKE_STD_ZVAL(param);

	sapi_add_header("Content-Type:application/xml",
                sizeof ("Content-Type:application/xml") - 1, 1);
	
    ZVAL_STRING(param, wsdl_location, 1);
    ZVAL_STRING(&function, "file_get_contents", 1);

	if (call_user_function(EG(function_table), NULL, &function, 
			&retval, 1, &param TSRMLS_CC) == SUCCESS) {
            
			char *wsdl_string = NULL;
            char *wsdl_string1 = NULL;

            if (Z_TYPE_P(&retval) == IS_STRING) {

                wsdl_string = estrdup(Z_STRVAL(retval));
                len = Z_STRLEN(retval);
                /** remove xml version declaration */
				if (strstr(wsdl_string, "<?xml version=\"1.0\"")) {
                    new_len = strlen("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
                    wsdl_string = (wsdl_string + new_len);
                    wsdl_string1 = estrdup(strstr(wsdl_string, "<"));
                    len = strlen(wsdl_string1);
                    php_write(wsdl_string1, len TSRMLS_CC);
                } else {
                    php_write(wsdl_string, len TSRMLS_CC);
                }
                if (wsdl_string) {
                    efree(val);
                }
                if (wsdl_string1) {
                    efree(wsdl_string1);
                }
            } else {
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "WSDL Generation failed for the given WSDL");
            }
        }
        zval_ptr_dtor(&param);
        zval_dtor(&function);
        zval_dtor(&retval);
}



static void 
generate_wsdl_for_service(
		zval *svc_zval,
        wsf_svc_info_t *svc_info,
        wsf_request_info_t *req_info, 
		char *wsdl_ver_str,
        int in_cmd TSRMLS_DC) {

	zval **wsdl_location = NULL;

	/** WSDL is specified, so load and display from there */
    if ((zend_hash_find(Z_OBJPROP_P(svc_zval), WSF_WSDL, sizeof (WSF_WSDL),
            (void **) &wsdl_location) == SUCCESS && Z_TYPE_PP(wsdl_location) == IS_STRING)) {
				
				char *wsdl_file_location = Z_STRVAL_PP(wsdl_location);
				serve_static_wsdl(wsdl_file_location TSRMLS_CC);
    } else {
	
		char *service_name;
		zval func, retval, param0;
		zval * params[1];
		int len = 0;
		char *val = NULL;
		int args_count = 0;
	
		zval **class_map;
    
		axutil_hash_index_t * hi = NULL;
		zval *functions = NULL;

		/** For opening php script */
		zend_file_handle script;
		php_stream *stream;
		FILE *new_fp;

		zval **tmpval;
		char *binding_name;
		char *wsdl_version;
		smart_str full_path = {0};
		zval *op_val;
	
		zval *wsdata_obj;
		zend_class_entry **ce;


		service_name = svc_info->svc_name;

		if (!in_cmd) {
			smart_str_appends(&full_path, req_info->svr_name);
			if (req_info->svr_port != WSF_DEFAULT_PORT) {
				char svr_port[10];
				sprintf(svr_port, ":%ld", req_info->svr_port);
				smart_str_appends(&full_path, svr_port);
			}
			smart_str_appends(&full_path, req_info->request_uri);
			smart_str_0(&full_path);
		} else {
			smart_str_appends(&full_path, SG(request_info).path_translated);
			smart_str_0(&full_path);
		}

		/** for WSDL version. default is wsdl 1.1*/
		if ((stricmp(wsdl_ver_str, WSF_WSDL)) == 0) {
			wsdl_version = strdup(WSF_WSDL_1_1);
		} else {
			wsdl_version = strdup(WSF_WSDL_2_0);
		}
		/** getting the correct binding style */
		if ((zend_hash_find(Z_OBJPROP_P(svc_zval), WSF_BINDING_STYLE, sizeof (WSF_BINDING_STYLE),
				(void **) & tmpval)) == SUCCESS && Z_TYPE_PP(tmpval) == IS_STRING) {
			binding_name = Z_STRVAL_PP(tmpval);
		} else {
			binding_name = WSF_STYLE_DOCLIT;
		}

		/** Get the functions in the service.php file to an array */
		MAKE_STD_ZVAL(functions);
		array_init(functions);

		MAKE_STD_ZVAL(op_val);
		array_init(op_val);

		if (svc_info->ops_to_functions) {
			for (hi = axutil_hash_first(svc_info->ops_to_functions, ws_env_svr); hi;
					hi = axutil_hash_next(ws_env_svr, hi)) {
				void *value = NULL;
				const void *key = NULL;
				axutil_hash_this(hi, &key, NULL, &value);
				add_next_index_string(functions, (char *) value, 1);
				add_assoc_string(op_val, (char *) key, (char *) value, 1);
			}
		}

		INIT_ZVAL(func);
		ZVAL_STRING(&func, WSF_WSDL_GENERATION_FUNCTION, 1);
	    
		/** Create an object of type WSData */

		MAKE_STD_ZVAL(wsdata_obj);
		zend_lookup_class("WSData", strlen("WSData"), &ce TSRMLS_CC);
		object_init_ex(wsdata_obj, *ce);
		
		/** Add properties to WSData Object */
		add_property_string(wsdata_obj, "serviceName", service_name, 1);
		add_property_zval(wsdata_obj, "WSDLGEN_Functions", functions);
		
		if (svc_info->wsdl_gen_class_map) {
			add_property_zval(wsdata_obj, "WSDLGEN_Svcinfo_Classmap", svc_info->wsdl_gen_class_map);
		}
		else {
			add_property_null(wsdata_obj, "WSDLGEN_Svcinfo_Classmap");
		}
	       
		if (binding_name) {
			add_property_string(wsdata_obj, "WSDLGen_binding", binding_name,1);
		}else{
			add_property_null(wsdata_obj, "WSDLGen_binding");
		}
		if (wsdl_version) {
			add_property_string(wsdata_obj, "WSDLGen_wsdlversion",wsdl_version, 1); 
		}else{
			add_property_null(wsdata_obj, "WSDLGen_wsdlversion"); 
		}

		add_property_string(wsdata_obj, "WSDLGen_path", full_path.c, 1);

		add_property_zval(wsdata_obj, "WSDLGen_operations", op_val);
	       
		if (zend_hash_find(Z_OBJPROP_P(svc_zval), WSF_WSDL_CLASSMAP, sizeof (WSF_WSDL_CLASSMAP),
			(void **) & class_map) == SUCCESS) {
			add_property_zval(wsdata_obj, "WSDLGen_Classmap", *class_map);
		}else{
			add_property_null(wsdata_obj, "WSDLGen_Classmap");
		}

		if (svc_info->wsdl_gen_annotations) {
			add_property_zval(wsdata_obj, "WSDLGen_annotations", svc_info->wsdl_gen_annotations);
		} else {
			add_property_null(wsdata_obj, "WSDLGen_annotations");
		}

		if (svc_info->wsdl_gen_actions) {
			add_property_zval(wsdata_obj, "WSDLGen_actions", svc_info->wsdl_gen_actions);
		} else {
			add_property_null(wsdata_obj, "WSDLGen_actions");
		}
	    
		if (svc_info->use_wsa) {
			add_property_bool(wsdata_obj, "WSDLGen_usewsa", svc_info->use_wsa); 
		}else
		{
			add_property_null(wsdata_obj, "WSDLGen_usewsa");
		}

		params[0] = &param0; /** service name */
		ZVAL_ZVAL(params[0], wsdata_obj, NULL, NULL);
		INIT_PZVAL(params[0]);

		args_count = 1;

		script.type = ZEND_HANDLE_FP;

		script.filename = WSF_SCRIPT_FILENAME;

		script.opened_path = NULL;

		script.free_filename = 0;

		stream = php_stream_open_wrapper(WSF_SCRIPT_FILENAME, "rb",
				USE_PATH | REPORT_ERRORS | ENFORCE_SAFE_MODE, NULL);

		if (!stream) {
			return;
		}

		if (php_stream_cast(stream, PHP_STREAM_AS_STDIO | PHP_STREAM_CAST_RELEASE,
				(void*) & new_fp, REPORT_ERRORS) == FAILURE) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Unable to open script file or file not found:");
		}

		if (new_fp) {
			int status;
			script.handle.fp = new_fp;
			status = php_lint_script(&script TSRMLS_CC);
			if (call_user_function(EG(function_table), (zval **) NULL,
					&func, &retval, args_count, params TSRMLS_CC) == SUCCESS) {
					
					if (Z_TYPE(retval) == IS_STRING) {
						val = estrdup(Z_STRVAL(retval));
						len = Z_STRLEN(retval);
						sapi_add_header("Content-Type:application/xml",
								sizeof ("Content-Type:application/xml") - 1, 1);
						php_write(val, len TSRMLS_CC);
						if (val) {
							efree(val);
						}
					} else {
						php_error_docref(NULL TSRMLS_CC, E_ERROR, "WSDL Generation Failed");
					}
				}
			}
        
		smart_str_free(&full_path);
		zval_ptr_dtor(&op_val); 
        zval_ptr_dtor(&functions);
        /** end WSDL generation*/
    }
}

/* {{{ proto long reply([long style]) reply the SOAP request */
PHP_METHOD(ws_service, reply) {
    ws_object_ptr intern = NULL;
    zval * obj = NULL;
    axis2_conf_ctx_t * conf_ctx = NULL;
    wsf_svc_info_t *svc_info = NULL;
    wsf_request_info_t req_info;
    wsf_response_info_t res_info;
    zval ** server_vars, **data;
    wsf_worker_t * php_worker = NULL;
    zval ** raw_post = NULL;
    zval ** wsdl_tmp = NULL;

    char *arg_data = NULL;
    int arg_data_len = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|s", &arg_data, &arg_data_len) == FAILURE) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
    }

    WSF_GET_THIS(obj);
    intern = (ws_object *) zend_object_store_get_object(obj TSRMLS_CC);

    svc_info = (wsf_svc_info_t *) (intern->ptr);

    svc_info->attachment_cache_dir = WSF_GLOBAL(attachment_cache_dir);
    svc_info->enable_attachment_caching = WSF_GLOBAL(enable_attachment_caching);
    php_worker = svc_info->php_worker;

    conf_ctx = wsf_worker_get_conf_ctx(php_worker, ws_env_svr);

    if (!conf_ctx) {

        php_error_docref(NULL TSRMLS_CC, E_ERROR,
                "Error building the configuration, please check your php.ini entries");
        return;
    }

    zend_is_auto_global("_SERVER", sizeof ("_SERVER") - 1 TSRMLS_CC);

    wsf_request_info_init(&req_info);
    wsf_response_info_init(&res_info);

    if (SG(request_info).request_uri) {

        if (zend_hash_find(&EG(symbol_table), "_SERVER", sizeof ("_SERVER"),
                (void **) & server_vars) == SUCCESS && (Z_TYPE_PP(server_vars) == IS_ARRAY)) {

            if ((zend_hash_find(Z_ARRVAL_PP(server_vars),
                    "HTTP_TRANSFER_ENCODING", sizeof ("HTTP_TRANSFER_ENCODING"),
                    (void **) & data) == SUCCESS) && Z_TYPE_PP(data) == IS_STRING) {
                req_info.transfer_encoding = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "SERVER_NAME",
                    sizeof ("SERVER_NAME"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                req_info.svr_name = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "SERVER_PORT",
                    sizeof ("SERVER_PORT"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                char *p = NULL;
                p = Z_STRVAL_PP(data);
                req_info.svr_port = atoi(p);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "SERVER_PROTOCOL",
                    sizeof ("SERVER_PROTOCOL"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                req_info.http_protocol = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "HTTP_SOAPACTION",
                    sizeof ("HTTP_SOAPACTION"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                req_info.soap_action = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "REMOTE_ADDR",
                    sizeof ("REMOTE_ADDR"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                req_info.remote_address = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "HTTP_ACCEPT",
                    sizeof ("HTTP_ACCEPT"), (void **) & data) == SUCCESS) && Z_TYPE_PP(data) == IS_STRING) {
                req_info.accept = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "HTTP_ACCEPT_LANGUAGE",
                    sizeof ("HTTP_ACCEPT_LANGUAGE"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                req_info.accept_language = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "HTTP_ACCEPT_ENCODING",
                    sizeof ("HTTP_ACCEPT_ENCODING"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                req_info.accept_encoding = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "HTTP_ACCEPT_CHARSET",
                    sizeof ("HTTP_ACCEPT_CHARSET"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                req_info.accept_charset = Z_STRVAL_PP(data);
            }
            if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "PHP_SELF",
                    sizeof ("PHP_SELF"), (void **) & data) == SUCCESS) &&
                    Z_TYPE_PP(data) == IS_STRING) {
                req_info.request_uri = Z_STRVAL_PP(data);
            }
        }
        if (!req_info.request_uri) {
            req_info.request_uri = SG(request_info).request_uri;
        }
        req_info.content_length = SG(request_info).content_length;
        req_info.content_type = (char *) SG(request_info).content_type;
        req_info.request_method = (char *) SG(request_info).request_method;
        req_info.query_string = (char *) SG(request_info).query_string;

        /* HTTP_RAW_POST_DATA variable is not on by default. Using php://input instead
        if(req_info.request_method && strcmp(req_info.request_method,WSF_HTTP_POST) == 0){
                if (zend_hash_find (&EG(symbol_table), "HTTP_RAW_POST_DATA", sizeof ("HTTP_RAW_POST_DATA"),
                        (void **)&raw_post) != FAILURE  && ((*raw_post)->type ==  IS_STRING)){
                        char *value = NULL;
                        req_info.request_data = Z_STRVAL_PP (raw_post);
                        req_info.request_data_length = Z_STRLEN_PP (raw_post);
                        value = (char*)Z_STRVAL_PP(raw_post);
                }else
                {
                        zval function, retval , *param = NULL;
                        int new_len = 0;
                        INIT_ZVAL(retval);
                        MAKE_STD_ZVAL(param);

                        ZVAL_STRING(param, "php://input", 1);
                        ZVAL_STRING(&function, "file_get_contents", 1);
                        if (call_user_function(EG(function_table), NULL, &function,
                                &retval, 1, &param  TSRMLS_CC) == SUCCESS)
                        {
                                if (Z_TYPE (retval) == IS_STRING)
                                {
                                        char *data = NULL;
                                        data = Z_STRVAL(retval);
                                        req_info.request_data = Z_STRVAL(retval);
                                        req_info.request_data_length = Z_STRLEN(retval);
                                        AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                                                "php://input data found");
                                }else
                                {
                                        php_error_docref(NULL TSRMLS_CC, E_ERROR, "raw post data not found");
                                }
                        }

                }
        }else {
                if(req_info.request_method && strcmp(req_info.request_method,WSF_HTTP_PUT) == 0) */
        {
            zval function, retval, *param = NULL;
            int new_len = 0;
            INIT_ZVAL(retval);
            MAKE_STD_ZVAL(param);

            ZVAL_STRING(param, "php://input", 1);
            ZVAL_STRING(&function, "file_get_contents", 1);
            if (call_user_function(EG(function_table), NULL, &function,
                    &retval, 1, &param TSRMLS_CC) == SUCCESS) {
                if (Z_TYPE(retval) == IS_STRING) {
                    char *data = NULL;
                    data = Z_STRVAL(retval);
                    req_info.request_data = Z_STRVAL(retval);
                    req_info.request_data_length = Z_STRLEN(retval);
                    AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
							"php://input data found");
                } else {
                    php_error_docref(NULL TSRMLS_CC, E_ERROR, "raw post data not found");
                }
            }
            efree(param);
        }
        /*
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                        "raw post data not found");
                if(req_info.request_method && strcmp(req_info.request_method,WSF_HTTP_POST) == 0){
                        php_error_docref(NULL TSRMLS_CC, E_ERROR, "raw post data not found");
                }
        } */
    }
    else if (ZEND_NUM_ARGS() > 0 && arg_data_len > 0) {
        /* If we come here, it is not an HTTP post, 
           rather a command line script execution. 
           So set some defaults to facilitate standalone execution. */
        if ((arg_data_len == 4 || arg_data_len == 5) && (stricmp(arg_data, WSF_WSDL) ||
                stricmp(arg_data, WSF_WSDL2))) {
            req_info.request_uri = svc_info->svc_name;
            req_info.svr_name = estrdup(WSF_LOCALHOST);
            generate_wsdl_for_service(obj, svc_info, &req_info, arg_data, 1 TSRMLS_CC);
            efree(req_info.svr_name);
            return;
        }
        req_info.svr_name = strdup(WSF_LOCALHOST);
        req_info.svr_port = 9999;
        req_info.request_data = arg_data;
        req_info.request_data_length = arg_data_len;
        req_info.http_protocol = strdup(WSF_PROTOCOL_HTTP);
        req_info.request_uri = svc_info->svc_name;
        req_info.request_method = strdup(WSF_HTTP_POST);
        req_info.content_type = strdup("application/soap+xml;charset=UTF-8");
        req_info.content_length = arg_data_len;

    } else {
        php_printf("please provide reply function's argument SOAP Envelope XML string\n");
        return;
    }


	if (SG(request_info).query_string){

			if(((stricmp(SG(request_info).query_string, WSF_WSDL) == 0)
            || (stricmp(SG(request_info).query_string, WSF_WSDL2) == 0))) {
				/** begin WSDL Generation */
				generate_wsdl_for_service(obj, svc_info, &req_info, SG(request_info).query_string, 0 TSRMLS_CC);
			}
			if(axutil_strcasestr(SG(request_info).query_string, WSF_XSD) != NULL)
			{
				char *querystr = NULL, *filename = NULL;
				int length = 0;
				querystr = SG(request_info).query_string;
				length = strlen(querystr);
				if(length > 4){
					filename = axutil_string_substring_starting_at(querystr, 4);
					if(VCWD_ACCESS(filename, F_OK) != -1)
					serve_static_wsdl(filename TSRMLS_CC);
				}
			}

    } else {

        if (!svc_info->omit_wsdl && zend_hash_find(Z_OBJPROP_P(this_ptr), WSF_WSDL,
                sizeof (WSF_WSDL), (void **) & wsdl_tmp) == SUCCESS) {
            wsf_wsdl_process_service(svc_info, ws_env_svr TSRMLS_CC);
        }

        wsf_worker_process_request(php_worker, ws_env_svr, &req_info, &res_info, svc_info TSRMLS_CC);

        wsf_response_info_cleanup(&res_info, ws_env_svr);
    }
}
/* }}} end reply */

/* {{{ proto void WSFault::__construct(string faultcode, string faultreason [,string faultrole [, mixed detail[, string faultname]]]) */
PHP_METHOD(ws_fault, __construct) {
    char *sf_code = NULL, *sf_code_ns = NULL, *sf_reason = NULL, *sf_role = NULL, *value = NULL;
    int sf_code_len = 0, sf_reason_len = 0, sf_role_len = 0, value_len = 0;
    zval * code = NULL, *details = NULL;
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "zs|s!z!s",
            &code, &sf_reason, &sf_reason_len, &sf_role, &sf_role_len, &details, &value, &value_len)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        AXIS2_LOG_ERROR(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
					"Invalid Parameters, Fault Code and Fault Reason values are mandatory");
        return;
    }
    if (Z_TYPE_P(code) == IS_STRING) {
        sf_code = Z_STRVAL_P(code);
        sf_code_len = Z_STRLEN_P(code);
    } else if (Z_TYPE_P(code) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(code)) == 2) {
        zval ** tmp_ns, **tmp_code;
        zend_hash_internal_pointer_reset(Z_ARRVAL_P(code));
        zend_hash_get_current_data(Z_ARRVAL_P(code), (void **) & tmp_ns);
        zend_hash_move_forward(Z_ARRVAL_P(code));
        zend_hash_get_current_data(Z_ARRVAL_P(code), (void **) & tmp_code);
        if (Z_TYPE_PP(tmp_ns) == IS_STRING && Z_TYPE_PP(tmp_code) == IS_STRING) {
            sf_code_ns = Z_STRVAL_PP(tmp_ns);
            sf_code = Z_STRVAL_PP(tmp_code);
            sf_code_len = Z_STRLEN_PP(tmp_code);
        }
        else {
            php_error_docref(NULL TSRMLS_CC, E_ERROR,
                    "Fault Code and Fault Code namespace must be strings");

            AXIS2_LOG_ERROR(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
				"Fault Code and ns must be strings.");
            return;
        }
    } else {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Incorrect Fault Code");

        AXIS2_LOG_ERROR(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Invalid fault code.");
        return;
    }
    if (!(strcmp(sf_code, "VersionMismatch") == 0 ||
            strcmp(sf_code, "MustUnderstand") == 0 ||
            strcmp(sf_code, "DataEncodingUnknown") == 0 ||
            strcmp(sf_code, "Sender") == 0 ||
            strcmp(sf_code, "Client") == 0 ||
            strcmp(sf_code, "Server") == 0 ||
            strcmp(sf_code, "Receiver") == 0)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR,
                "Incorrect Fault Code, Code must be one of Client, Server, VersionMismatch,MustUnderstand,DataEncodingUnknown,Sender, Receiver");
    }


    if (!sf_code || !sf_reason) {
        if (WSF_GLOBAL(soap_version) == AXIOM_SOAP11) {
            AXIS2_LOG_ERROR(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
				"faultcode and faultstring are mandatory ");
            return;
        } else if (WSF_GLOBAL(soap_version) == AXIOM_SOAP12) {
            AXIS2_LOG_ERROR(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
				"Code and Reason are mandatory ");
        }
    }
    wsf_util_set_soap_fault(this_ptr, sf_code_ns, sf_code, sf_reason,
            sf_role, details, value TSRMLS_CC);
}
/* }}} */

/* {{{ proto */
PHP_METHOD(ws_fault, __destruct) {
}

/* }}} */
PHP_METHOD(ws_fault, __toString) {
    zval *code, *reason, *detail;
    char *fault_string;
    int length;

    if (ZEND_NUM_ARGS() > 0) {
        ZEND_WRONG_PARAM_COUNT();
    }

    code = zend_read_property(ws_fault_class_entry, this_ptr,
            WSF_FAULT_CODE, sizeof (WSF_FAULT_CODE) - 1, 1 TSRMLS_CC);
    reason = zend_read_property(ws_fault_class_entry, this_ptr,
            WSF_FAULT_REASON, sizeof (WSF_FAULT_REASON) - 1, 1 TSRMLS_CC);
    detail = zend_read_property(ws_fault_class_entry, this_ptr,
            WSF_FAULT_DETAIL, sizeof (WSF_FAULT_DETAIL) - 1, 1 TSRMLS_CC);

    if (Z_STRVAL_P(detail)) {
        length = spprintf(&fault_string, 0, "WS FAULT exception: [%s] %s %s",
                Z_STRVAL_P(code), Z_STRVAL_P(reason), Z_STRVAL_P(detail));
    } else {
        length = spprintf(&fault_string, 0, "WS FAULT exception: [%s] %s",
                Z_STRVAL_P(code), Z_STRVAL_P(reason));
    }
    RETURN_STRINGL(fault_string, length, 0);
}

PHP_METHOD(ws_header, __construct) {
    zval *arg = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "z", &arg) == FAILURE) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
    }

    if (Z_TYPE_P(arg) == IS_ARRAY) {
        zval ** tmp;
        HashTable *ht = Z_ARRVAL_P(arg);
        if (!ht) return;

        if (zend_hash_find(ht, WSF_HEADER_NS, sizeof (WSF_HEADER_NS), (void **) & tmp) == SUCCESS &&
                Z_TYPE_PP(tmp) == IS_STRING) {
            if (Z_STRLEN_PP(tmp) == 0) {
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "Namespace should not be empty");
            }
            add_property_stringl(this_ptr, WSF_HEADER_NS, Z_STRVAL_PP(tmp),
                    Z_STRLEN_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_HEADER_LOCALNAME, sizeof (WSF_HEADER_LOCALNAME), (void**) & tmp) == SUCCESS
                && Z_TYPE_PP(tmp) == IS_STRING) {
            if (Z_STRLEN_PP(tmp) == 0) {
                php_error_docref(NULL TSRMLS_CC, E_ERROR, "A Soap Header should have a non empty localname");
            }
            add_property_stringl(this_ptr, WSF_HEADER_LOCALNAME, Z_STRVAL_PP(tmp),
                    Z_STRLEN_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_HEADER_DATA, sizeof (WSF_HEADER_DATA), (void**) & tmp) == SUCCESS) {
#ifndef ZEND_ENGINE_2
            zval_add_ref(tmp);
#endif  /*  */
            add_property_zval(this_ptr, WSF_HEADER_DATA, *tmp);
        }
        if (zend_hash_find(ht, WSF_HEADER_MUST_UNDERSTAND, sizeof (WSF_HEADER_MUST_UNDERSTAND),
                (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL) {
            add_property_bool(this_ptr, WSF_HEADER_MUST_UNDERSTAND, Z_BVAL_PP(tmp));
        }
        if (zend_hash_find(ht, WSF_HEADER_PREFIX, sizeof (WSF_HEADER_PREFIX), (void **) & tmp) == SUCCESS
                && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_string(this_ptr, WSF_HEADER_PREFIX, Z_STRVAL_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_HEADER_ROLE, sizeof (WSF_HEADER_ROLE), (void**) & tmp) == SUCCESS) {
            if (Z_TYPE_PP(tmp) == IS_LONG &&
                    (Z_LVAL_PP(tmp) == WSF_SOAP_ROLE_NEXT
                    || Z_LVAL_PP(tmp) == WSF_SOAP_ROLE_NONE
                    || Z_LVAL_PP(tmp) == WSF_SOAP_ROLE_ULTIMATE_RECEIVER)) {
                add_property_long(this_ptr, WSF_HEADER_ROLE, Z_LVAL_PP(tmp));
            } else if (Z_TYPE_PP(tmp) == IS_STRING && Z_STRLEN_PP(tmp) > 0) {
                add_property_stringl(this_ptr, WSF_HEADER_ROLE,
                        Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
            } else {
                php_error_docref(NULL TSRMLS_CC, E_ERROR,
                        "Invalid parameters. Invalid role.");
            }
        }

    }

}

/* {{{ WSSecurityToken::__construct( */
PHP_METHOD(ws_security_token, __construct) {
    zval * object = NULL;
    zval * options = NULL;
    zval ** tmp = NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "a", &options) == FAILURE) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "An array should be passed here");
    }

    WSF_GET_THIS(object);
    if (options) {
        HashTable * ht = Z_ARRVAL_P(options);
        if (zend_hash_find(ht, WSF_CUSTOM_TOKENS, sizeof (WSF_CUSTOM_TOKENS),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY) {
            add_property_zval(object, WSF_CUSTOM_TOKENS, *tmp);
        }
        if (zend_hash_find(ht, WSF_USER, sizeof (WSF_USER),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_USER, Z_STRVAL_PP(tmp),
                    Z_STRLEN_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_PASSWORD_TYPE, sizeof (WSF_PASSWORD_TYPE),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_PASSWORD_TYPE,
                    Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_PASSWORD, sizeof (WSF_PASSWORD),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_string(object, WSF_PASSWORD, Z_STRVAL_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_PRIVATE_KEY, sizeof (WSF_PRIVATE_KEY),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_string(object, WSF_PRIVATE_KEY, Z_STRVAL_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_CERTIFICATE, sizeof (WSF_CERTIFICATE),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_CERTIFICATE, Z_STRVAL_PP(tmp),
                    Z_STRLEN_PP(tmp), 1);
        }

        if (zend_hash_find(ht, WSF_RECEIVER_CERTIFICATE,
                sizeof (WSF_RECEIVER_CERTIFICATE), (void **) & tmp) == SUCCESS
                && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_string(object, WSF_RECEIVER_CERTIFICATE,
                    Z_STRVAL_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_TTL, sizeof (WSF_TTL),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_LONG) {
            add_property_long(object, WSF_TTL, Z_LVAL_PP(tmp));
        }
		 if (zend_hash_find(ht, WSF_CLOCK_SKEW_BUFFER, sizeof (WSF_CLOCK_SKEW_BUFFER),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_LONG) {
					add_property_long(object, WSF_CLOCK_SKEW_BUFFER, Z_LVAL_PP(tmp));
        }

        if (zend_hash_find(ht, WSF_PKCS12_KEYSTORE, sizeof (WSF_PKCS12_KEYSTORE),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_PKCS12_KEYSTORE, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_PASSWORD_CALLBACK,
                sizeof (WSF_PASSWORD_CALLBACK), (void **) & tmp) == SUCCESS
                && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_string(object, WSF_PASSWORD_CALLBACK, Z_STRVAL_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_PASSWORD_CALLBACK_ARGS,
                sizeof (WSF_PASSWORD_CALLBACK_ARGS), (void **) & tmp) == SUCCESS) {
            add_property_zval(object, WSF_PASSWORD_CALLBACK_ARGS, *tmp);
        }
        if (zend_hash_find(ht, WSF_REPLAY_DETECT_CALLBACK, sizeof (WSF_REPLAY_DETECT_CALLBACK),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_REPLAY_DETECT_CALLBACK, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_REPLAY_DETECT_CALLBACK_ARGS, sizeof (WSF_REPLAY_DETECT_CALLBACK_ARGS),
                (void **) & tmp) == SUCCESS) {
            add_property_zval(object, WSF_REPLAY_DETECT_CALLBACK_ARGS, *tmp);
        }
        if (zend_hash_find(ht, WSF_ENABLE_REPLAY_DETECT, sizeof (WSF_ENABLE_REPLAY_DETECT),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_BOOL) {
            add_property_bool(object, WSF_ENABLE_REPLAY_DETECT, Z_BVAL_PP(tmp));
        }

        if (zend_hash_find(ht, WSF_STORE_SCT_CALLBACK, sizeof (WSF_STORE_SCT_CALLBACK),
                (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_STORE_SCT_CALLBACK, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_SCT_CALLBACK_ARGS, sizeof (WSF_SCT_CALLBACK_ARGS),
                (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_zval(object, WSF_SCT_CALLBACK_ARGS, *tmp);
        }
        if (zend_hash_find(ht, WSF_GET_SCT_CALLBACK, sizeof (WSF_GET_SCT_CALLBACK),
                (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_GET_SCT_CALLBACK, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
        }
        if (zend_hash_find(ht, WSF_DELETE_SCT_CALLBACK, sizeof (WSF_DELETE_SCT_CALLBACK),
                (void**) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING) {
            add_property_stringl(object, WSF_DELETE_SCT_CALLBACK, Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp), 1);
        }

    }
}

/* }}} */

/* {{{ WSPolicy::__construct( */
PHP_METHOD(ws_policy, __construct) {
    zval * object = NULL;
    zval * properties = NULL;
    if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "|z|a",
            &properties)) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        return;
    }
    WSF_GET_THIS(object);
    WSF_OBJ_CHECK(env);
    if (NULL != properties) {
        wsf_policy_set_policy_options(object, properties, env TSRMLS_CC);
    }
}
/* }}} */

/* {{{ WSPolicy::__construct) { */
PHP_METHOD(ws_data, __construct)
{

}
/* }}} */

/* {{{ proto string file_get_contents(string filename)
    read a pem file and return the key portion of the file as a string */
PHP_FUNCTION(ws_get_key_from_file) {
    char *filename;
    int filename_len;
    char *contents;
    php_stream * stream;
    int len;
    long maxlen = PHP_STREAM_COPY_ALL;
    zval * zcontext = NULL;
    php_stream_context * context = NULL;
    char *DELIMITER = "-----";

    /* Parse arguments */
    if (zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "s", &filename,
            &filename_len) == FAILURE) {
        return;
    }
    context = php_stream_context_from_zval(zcontext, 0);
    stream = php_stream_open_wrapper_ex(filename, "rb",
            (USE_PATH) | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL, context);
    if (!stream) {
        RETURN_FALSE;
    }
    if ((len = php_stream_copy_to_mem(stream, &contents, maxlen, 0)) > 0) {
        char *key = NULL;
        char *start_index = NULL;
        char *tmp_index = NULL;
        tmp_index = strstr(contents, DELIMITER);
        if (!tmp_index)
            return;
        if (len > (tmp_index - contents)) {
            tmp_index += 5;
            tmp_index = strstr(tmp_index, DELIMITER);
            if (!tmp_index)
                return;
            start_index = tmp_index + 6;
            tmp_index[contents - tmp_index] = '\0';
        }
        tmp_index = strstr(start_index, DELIMITER);
        if (!tmp_index)
            return;
        tmp_index = tmp_index + 1;
        start_index[tmp_index - start_index] = '\0';
        key = estrdup(start_index);
        efree(contents);
        RETVAL_STRINGL(key, strlen(key), 0);
    } else if (len == 0) {
        RETVAL_EMPTY_STRING();
    } else {
        RETVAL_FALSE;
    }
    php_stream_close(stream);
}
/* }}} */

/* {{{ proto string file_get_contents(string filename)
    read a pem file and return the key portion of the file as a string */
PHP_FUNCTION(ws_get_cert_from_file) {
    char *filename;
    int filename_len;
    char *contents;
    php_stream * stream;
    int len;
    long maxlen = PHP_STREAM_COPY_ALL;
    zval * zcontext = NULL;
    php_stream_context * context = NULL;
    char *DELIMITER = "-----";

    /* Parse arguments */
    if (zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "s", &filename,
            &filename_len) == FAILURE) {
        return;
    }
    context = php_stream_context_from_zval(zcontext, 0);
    stream = php_stream_open_wrapper_ex(filename, "rb",
            (USE_PATH) | ENFORCE_SAFE_MODE | REPORT_ERRORS, NULL, context);
    if (!stream) {
        RETURN_FALSE;
    }
    if ((len = php_stream_copy_to_mem(stream, &contents, maxlen, 0)) > 0) {
        char *key = NULL;
        char *start_index = NULL;
        char *tmp_index = NULL;
        tmp_index = strstr(contents, DELIMITER);
        if (!tmp_index)
            return;
        if (len > (tmp_index - contents)) {
            tmp_index += 5;
            tmp_index = strstr(tmp_index, DELIMITER);
            if (!tmp_index)
                return;
            start_index = tmp_index + 6;
            tmp_index[contents - tmp_index] = '\0';
        }
        tmp_index = strstr(start_index, DELIMITER);
        if (!tmp_index)
            return;
        tmp_index = tmp_index + 1;
        start_index[tmp_index - start_index] = '\0';
        key = estrdup(start_index);
        efree(contents);
        RETVAL_STRINGL(key, strlen(key), 0);
    } else if (len == 0) {
        RETVAL_EMPTY_STRING();
    } else {
        RETVAL_FALSE;
    }
    php_stream_close(stream);
}
/* }}} */

/* {{{  proto  WSClientProxy::__construct(mixed options) */
PHP_METHOD(ws_client_proxy, __construct) {
}


/* }}} end WSClient_proxt */

/* {{{ proto void WSClientProxy::__destruct() */
PHP_METHOD(ws_client_proxy, __destruct) {

}

/* {{{ proto WSClientProxy::__call() */
PHP_METHOD(ws_client_proxy, __call) {
    char *fn_name = NULL;
    long fn_name_len = 0;
    zval * args = NULL;
    int arg_count = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "sz", &fn_name,
            &fn_name_len, &args) == FAILURE) {
        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Invalid parameters");
        return;
    }
    arg_count = zend_hash_num_elements(Z_ARRVAL_P(args));


    wsf_wsdl_create_dynamic_client(this_ptr, fn_name, fn_name_len, arg_count,
            args, return_value, env TSRMLS_CC);

}


/* }}} end _call */

/* {{{ proto getFunctions() */
PHP_METHOD(ws_client_proxy, get_functions) {
    /* wsf_soap_get_functions (this_ptr, return_value, env TSRMLS_CC); */
}


/* }}} */

/* {{{ proto getFunctions() */
PHP_METHOD(ws_client_proxy, get_types) {
    /*    wsf_soap_get_types (this_ptr, return_value, env TSRMLS_CC); */
}


/* }}} */

/* {{{ proto getLocation() */
PHP_METHOD(ws_client_proxy, get_location) {
    /*    wsf_soap_get_location (this_ptr, return_value, env TSRMLS_CC); */
}


/* }}} end getLocation */

/* {{{ proto string ws_log_write(string file, string line, string level, string text) */
PHP_FUNCTION(ws_log_write) {
    char *text;
    int text_len;

    char *level_str;
    int level;
    int level_len;

    char *file;
    int file_len;

    char* line_str;
    int line;
    int line_len;

    axis2_char_t *buffer;

    axutil_log_t * log;
    axutil_env_t * current_env;

    if (ws_is_svr) {
        current_env = ws_env_svr;
    } else {
        current_env = env;
    }
    log = current_env->log;


    /* Parse arguments */
    if (zend_parse_parameters(ZEND_NUM_ARGS()TSRMLS_CC, "ssss",
            &file, &file_len, &line_str, &line_len, &level_str,
            &level_len, &text, &text_len) == FAILURE) {
        return;
    }

    buffer = axutil_stracat(env, WSF_PHP_LOG_PREFIX, text);

    line = axutil_atoi(line_str);
    level = axutil_atoi(level_str);

    switch (level) {
        case 0:
            axutil_log_impl_log_critical(log, file, line, buffer);
            break;
        case 1:
            axutil_log_impl_log_error(log, file, line, buffer);
            break;
        case 2:
            axutil_log_impl_log_warning(log, file, line, buffer);
            break;
        case 3:
            axutil_log_impl_log_info(log, file, line, buffer);
            break;
        case 4:
            axutil_log_impl_log_debug(log, file, line, buffer);
            break;
        case 5:
            axutil_log_impl_log_user(log, file, line, buffer);
            break;
        default:
            axutil_log_impl_log_trace(log, file, line, buffer);
            break;
    }

    AXIS2_FREE(env->allocator, buffer);

}
/* }}} */
