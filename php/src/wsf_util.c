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

#include "php.h"
#include "zend_exceptions.h"
#include <axutil_dll_desc.h>
#include <axis2_msg_recv.h>
#include <axutil_class_loader.h>
#include "wsf_worker.h"
#include "wsf_xml_msg_recv.h"
#include <axis2_http_transport.h>
#include <axis2_addr.h>
#include "wsf.h"
#include "wsf_util.h"
#include <axutil_error_default.h>
#include <axutil_log_default.h>
#include <axutil_uuid_gen.h>
#include <axiom_util.h>
#include <axis2_http_client.h>

axiom_node_t *
wsf_util_construct_header_node (
    const axutil_env_t * env,
    axiom_node_t *parent,
	axis2_char_t *soap_uri,
	int soap_version,
    zval * header TSRMLS_DC)
{
    char *ns = NULL;
    char *localname = NULL;
    char prefix[6];
	char *nsprefix = NULL;
    axiom_namespace_t *header_ns = NULL;
    axiom_node_t *header_node = NULL;
    axiom_element_t *header_ele = NULL;
    axiom_namespace_t *soap_ns = NULL;
    int must_understand = 0;

    zval **tmp = NULL;


    AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI,
        " [wsf_log] construct header node ");

    if (zend_hash_find (Z_OBJPROP_P (header), WSF_HEADER_LOCALNAME,
            sizeof (WSF_HEADER_LOCALNAME), (void **) &tmp) == SUCCESS) {
        localname = Z_STRVAL_PP (tmp);
    } else {
        return NULL;
    }
    if (zend_hash_find (Z_OBJPROP_P (header), WSF_HEADER_NS,
            sizeof (WSF_HEADER_NS), (void **) &tmp) == SUCCESS
        && Z_TYPE_PP (tmp) == IS_STRING) {
        ns = Z_STRVAL_PP (tmp);
    }
	if (zend_hash_find (Z_OBJPROP_P (header), WSF_HEADER_PREFIX,
            sizeof (WSF_HEADER_PREFIX), (void **) &tmp) == SUCCESS
        && Z_TYPE_PP (tmp) == IS_STRING) {
        nsprefix = Z_STRVAL_PP (tmp);
    }

	if(ns && nsprefix){
		header_ns = axiom_namespace_create (env, ns, nsprefix);
	}else if(ns){
		sprintf (prefix, "ns%d", WSF_GLOBAL (curr_ns_index)++);
		header_ns = axiom_namespace_create (env, ns, prefix);
	}
    header_ele = axiom_element_create (env, parent , localname, header_ns, &header_node);

    if (zend_hash_find (Z_OBJPROP_P (header), WSF_HEADER_MUST_UNDERSTAND,
            sizeof (WSF_HEADER_MUST_UNDERSTAND), (void **) &tmp) == SUCCESS
        && Z_TYPE_PP (tmp) == IS_BOOL) {
        axiom_attribute_t *mu_attr = NULL;
        char must_val[2];

        AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI,
            "[wsf_client] must understand value is set");

        must_understand = Z_BVAL_PP (tmp);
        sprintf (must_val, "%d", must_understand);
        soap_ns = axiom_namespace_create (env, soap_uri, "soapenv");
        mu_attr = axiom_attribute_create (env, AXIOM_SOAP_ATTR_MUST_UNDERSTAND, must_val, soap_ns);
        axiom_element_add_attribute (header_ele, env, mu_attr, header_node);
    }
    if (zend_hash_find (Z_OBJPROP_P (header), WSF_HEADER_ROLE,
            sizeof (WSF_HEADER_ROLE), (void **) &tmp) == SUCCESS) {
        axiom_attribute_t *role_attr = NULL;
        char *role_val = NULL;
        if (Z_TYPE_PP (tmp) == IS_LONG) {
            if (Z_LVAL_PP (tmp) == WSF_SOAP_ROLE_NEXT) {
                role_val = WSF_SOAP_ROLE_NEXT_URI;
            } else if (Z_LVAL_PP (tmp) == WSF_SOAP_ROLE_NONE) {
                role_val = WSF_SOAP_ROLE_NONE_URI;
            } else if (Z_LVAL_PP (tmp) == WSF_SOAP_ROLE_ULTIMATE_RECEIVER) {
                role_val = WSF_SOAP_ROLE_ULTIMATE_RECEIVER_URI;
            }
        } else if (Z_TYPE_PP (tmp) == IS_STRING) {
            role_val = Z_STRVAL_PP (tmp);
        }
		if (!soap_ns){
            soap_ns = axiom_namespace_create (env, soap_uri, AXIOM_SOAP_DEFAULT_NAMESPACE_PREFIX);
		}
            /** role is only valid for soap12, for soap11 use actor, TODO */
        if (soap_version == AXIOM_SOAP12 && role_val) {
            role_attr = axiom_attribute_create (env, WSF_HEADER_ROLE, role_val, soap_ns);
            axiom_element_add_attribute (header_ele, env, role_attr, header_node);

        } else if (soap_version == AXIOM_SOAP11 && role_val) {
            role_attr = axiom_attribute_create (env, WSF_HEADER_ACTOR, role_val, soap_ns);
            axiom_element_add_attribute (header_ele, env, role_attr, header_node);
        }
    }
    if (zend_hash_find (Z_OBJPROP_P (header), WSF_HEADER_DATA,
            sizeof (WSF_HEADER_DATA), (void **) &tmp) == SUCCESS
			&& Z_TYPE_PP (tmp) == IS_STRING && Z_STRLEN_PP(tmp) > 0) {
				axiom_node_t *node = NULL;
				char *str = Z_STRVAL_PP(tmp);
				while(*str == ' ' && *str != '\0'){
					str++; 
				}
				if(str[0] == '<')
				{
					node = wsf_util_deserialize_buffer(env , Z_STRVAL_PP(tmp));
					if(node){
						axiom_node_add_child(header_node, env, node);
					}
				}
				if(!node){
					axiom_element_set_text (header_ele, env, Z_STRVAL_PP (tmp),
						header_node);
				}
    }else if (zend_hash_find (Z_OBJPROP_P (header), WSF_HEADER_DATA,
            sizeof (WSF_HEADER_DATA), (void **) &tmp) == SUCCESS
        && Z_TYPE_PP (tmp) == IS_ARRAY) {
        HashPosition pos;
        zval **param;
        HashTable * ht = Z_ARRVAL_PP(tmp);
        
        for (zend_hash_internal_pointer_reset_ex (ht, &pos);
             zend_hash_get_current_data_ex (ht, (void **) &param, &pos) == SUCCESS;
             zend_hash_move_forward_ex (ht, &pos)) {
            if(Z_TYPE_PP(param) == IS_OBJECT && 
               instanceof_function(Z_OBJCE_PP(param), ws_header_class_entry TSRMLS_CC)){  
				   wsf_util_construct_header_node(env, header_node, soap_uri, soap_version , *param TSRMLS_CC);  
            }
        }
   }
    return header_node;
}


/* {{{ efree wrapper */
static void WSF_CALL
wsf_free_wrapper_cli (
    axutil_allocator_t * allocator,
    void *ptr)
{
    if (ptr)
        pefree (ptr, 1);
}

/* }}} end efree wrapper */

/* {{{ malloc wrapper */
static void *WSF_CALL
wsf_malloc_wrapper_cli (
    axutil_allocator_t * allocator,
    size_t size)
{
    return pemalloc (size, 1);
}

/* }}} */
/* {{{ realloc wrapper */
static void *WSF_CALL
wsf_realloc_warpper_cli (
    axutil_allocator_t * allocator,
    void *ptr,
    size_t size)
{
    return perealloc (ptr, size, 1);
}

/* }}} */

/* {{{ malloc wrapper */
static void *WSF_CALL
wsf_malloc_wrapper (
    axutil_allocator_t * allocator,
    size_t size)
{
    return pemalloc (size, 1);
}

/* }}} */
/* {{{ realloc wrapper */
static void *WSF_CALL
wsf_realloc_warpper (
    axutil_allocator_t * allocator,
    void *ptr,
    size_t size)
{
    return perealloc (ptr, size, 1);
}

/* }}} */
/* {{{ free wrapper */
static void WSF_CALL
wsf_free_wrapper (
    axutil_allocator_t * allocator,
    void *ptr)
{

    if (ptr)
        pefree (ptr, 1);
}

/* }}} */

/* {{{ axis2_environment create function */
axutil_env_t *
wsf_env_create (
    axis2_char_t * path_tolog)
{
    axutil_allocator_t *allocator = NULL;
    axutil_error_t *error = NULL;
    axutil_log_t *log = NULL;
    axis2_char_t log_path[250];
    axutil_env_t *env = NULL;
    axutil_thread_pool_t *thread_pool = NULL;
    const axis2_char_t *LOG_NAME = "wsf_php_client.log";
    allocator = pemalloc (sizeof (axutil_allocator_t), 1);

    allocator->free_fn = wsf_free_wrapper_cli;
    allocator->malloc_fn = wsf_malloc_wrapper_cli;
    allocator->realloc = wsf_realloc_warpper_cli;
    allocator->local_pool = NULL;
    allocator->global_pool = NULL;
    allocator->global_pool_ref =0;
    error = axutil_error_create (allocator);
    if (path_tolog && (
            (0 == strcmp (path_tolog, "")) ||
            (0 == strcmp (path_tolog, ".")) ||
            (0 == strcmp (path_tolog, "./")))) {
        snprintf (log_path, 256, "%s", LOG_NAME);
    } else {
        snprintf (log_path, 256, "%s/%s", path_tolog, LOG_NAME);
    }

    thread_pool = axutil_thread_pool_init (allocator);
    log = axutil_log_create (allocator, NULL, log_path);
    env =
        axutil_env_create_with_error_log_thread_pool (allocator, error, log,
        thread_pool);
    return env;
}

/* }}} */
/* {{{ axis2_environment create function */
axutil_env_t *
wsf_env_create_svr (
    axis2_char_t * path_tolog)
{
    axutil_allocator_t *allocator = NULL;
    axutil_error_t *error = NULL;
    axutil_log_t *log = NULL;
    axis2_char_t log_path[250];
    axutil_env_t *env = NULL;
    axutil_thread_pool_t *thread_pool = NULL;
    const axis2_char_t *LOG_NAME = "wsf_php_server.log";
    allocator = pemalloc (sizeof (axutil_allocator_t), 1);

    allocator->free_fn = wsf_free_wrapper;
    allocator->malloc_fn = wsf_malloc_wrapper;
    allocator->realloc = wsf_realloc_warpper;
    allocator->local_pool = NULL;
    allocator->global_pool = NULL;
    allocator->global_pool_ref =0;     
    
    error = axutil_error_create (allocator);
    if (path_tolog && (
            (0 == strcmp (path_tolog, "")) ||
            (0 == strcmp (path_tolog, ".")) ||
            (0 == strcmp (path_tolog, "./")))) {
        snprintf (log_path, 256, "%s", LOG_NAME);
    } else {
        snprintf (log_path, 256, "%s/%s", path_tolog, LOG_NAME);
    }

    thread_pool = axutil_thread_pool_init (allocator);
    log = axutil_log_create (allocator, NULL, log_path);
    env =
        axutil_env_create_with_error_log_thread_pool (allocator, error, log,
        thread_pool);
    return env;
}

/* }}} */

void
wsf_env_free (
    axutil_env_t * env)
{

}

wsf_svc_info_t *
wsf_svc_info_create (
    axutil_env_t * env)
{
    wsf_svc_info_t *svc_info = NULL;
    svc_info = AXIS2_MALLOC (env->allocator, sizeof (wsf_svc_info_t));

    svc_info->svc = NULL;
    svc_info->svc_ctx = NULL;
    svc_info->svc_name = NULL;
    svc_info->generated_svc_name = 0;
    svc_info->port_name = NULL;
    svc_info->msg_recv = NULL;
    svc_info->php_worker = NULL;
    svc_info->use_mtom = 0;     /* default is false otherwise service side will send MIME
                                   headers which some servers can;t handle */
    svc_info->request_xop = 0;  /* default false */
    svc_info->enable_swa = 0;
    svc_info->use_wsa = 0;
    svc_info->policy = NULL;
    svc_info->security_token = NULL;
    /* caching wsdl is default to 1 */
    svc_info->cache_wsdl = 1;
    svc_info->omit_wsdl = 0;

    svc_info->ops_to_functions = NULL;
    svc_info->ops_to_actions = NULL;
	svc_info->ops_to_classes = NULL;
    svc_info->modules_to_engage = NULL;
    svc_info->ht_op_params = NULL;
    svc_info->class_args = NULL;
    svc_info->sig_model_string = NULL;
    svc_info->wsdl_string = NULL;
    svc_info->is_wsdl_11 = 0;
    svc_info->is_multi_interfaces = 0;
    svc_info->class_map = NULL;
    svc_info->wsdl_gen_class_map = NULL;
    svc_info->wsdl_gen_annotations = NULL;
    svc_info->wsdl_gen_actions = NULL;
    svc_info->wsdl = NULL;
	svc_info->loc_str = NULL;
	svc_info->ht_op_policies = NULL;
	svc_info->auth_type = WSF_PHP_AUTH_TYPE_BASIC;
	svc_info->auth_user = NULL;
	svc_info->auth_password = NULL;
	svc_info->attachment_cache_dir = NULL;
	svc_info->enable_attachment_caching =0;

    return svc_info;
}

/* free svc info struct */
void
wsf_svc_info_free (
    wsf_svc_info_t * svc_info,
    axutil_env_t * env)
{
    if (NULL != svc_info) {
        if (svc_info->svc_name) {
            AXIS2_FREE (env->allocator, svc_info->svc_name);
            svc_info->svc_name = NULL;
        }
        if (svc_info->port_name) {
            AXIS2_FREE (env->allocator, svc_info->port_name);
            svc_info->svc_name = NULL;
        }
        if (svc_info->ops_to_functions) {
            axutil_hash_free (svc_info->ops_to_functions, env);
        }
        if (svc_info->ops_to_actions) {
            axutil_hash_free (svc_info->ops_to_actions, env);
        }
        if (svc_info->modules_to_engage) {
            axutil_array_list_free (svc_info->modules_to_engage, env);
        }
        if( svc_info->wsdl) {
            AXIS2_FREE(env->allocator, svc_info->wsdl);
        }
		if(svc_info->auth_password)
		{
			AXIS2_FREE(env->allocator, svc_info->auth_password);
		}
		if(svc_info->auth_user)
		{
			AXIS2_FREE(env->allocator, svc_info->auth_user);
		}
		AXIS2_FREE (env->allocator, svc_info);
    }
}

/** create php request info struct */
void 
wsf_request_info_init (wsf_request_info_t *req_info)
{
    req_info->svr_name = NULL;
	req_info->remote_address = NULL;
	req_info->accept = NULL;
	req_info->accept_language = NULL;
	req_info->accept_encoding = NULL;
	req_info->accept_charset = NULL;
	/** default port is 80 */
    req_info->svr_port = 80;
    req_info->http_protocol = NULL;
    req_info->soap_action = NULL;
    req_info->request_uri = NULL;
    req_info->content_length = -1;
    req_info->content_type = NULL;
    req_info->request_method = NULL;
    req_info->request_data = NULL;
    req_info->request_data_length = 0;
	req_info->transfer_encoding = NULL;
    req_info->query_string = NULL;

	req_info->param_keys = NULL;
	req_info->param_values = NULL;

    return;
}

void wsf_response_info_init(wsf_response_info_t *res_info)
{
	res_info->response_data = NULL;
	res_info->response_length = 0;
	res_info->http_protocol = NULL;
	res_info->http_status_code = 0;
	res_info->http_status_code_name = NULL;
	res_info->content_type = NULL;
	return;
}

void
wsf_response_info_cleanup (
    wsf_response_info_t * response_info,
	axutil_env_t *env)
{
	/**note request info is not malloced, so should not be freed */
	if (response_info) {
		if(response_info->response_data){
			/** This is always allocated by axis2 */
			AXIS2_FREE(env->allocator, response_info->response_data);
		}
		if(response_info->content_type){
			AXIS2_FREE(env->allocator, response_info->content_type);
			response_info->content_type = NULL;
		}
    }
}

/* {{{ ws_read_payload */
axiom_node_t *
wsf_util_read_payload (
    axiom_xml_reader_t * reader,
    const axutil_env_t * env)
{
    axiom_stax_builder_t *builder = NULL;
    axiom_document_t *document = NULL;
    axiom_node_t *payload = NULL;

    builder = axiom_stax_builder_create (env, reader);

    if (!builder) {
        return NULL;
    }

    document = axiom_stax_builder_get_document (builder, env);

    if (!document) {
        return NULL;
    }

    payload = axiom_document_get_root_element (document, env);

    if (!payload) {
        return NULL;
    }
    axiom_document_build_all (document, env);

    /** free stax builder and associated document only */
    axiom_stax_builder_free_self (builder, env);

    return payload;
}

/* }}} ws_read_payload */

axis2_char_t *
wsf_util_get_soap_msg_from_op_client (
    axis2_op_client_t * op_client,
    axutil_env_t * env,
    axis2_wsdl_msg_labels_t msg_label)
{
    if (op_client) {
        axiom_soap_envelope_t *soap_env = NULL;
        const axis2_msg_ctx_t *msg_ctx = NULL;
        axiom_node_t *node = NULL;

        msg_ctx = axis2_op_client_get_msg_ctx (op_client, env, msg_label);
        if (!msg_ctx)
            return NULL;
        soap_env = axis2_msg_ctx_get_soap_envelope (msg_ctx, env);
        if (!soap_env)
            return NULL;
        node = axiom_soap_envelope_get_base_node (soap_env, env);
        return axiom_node_to_string (node, env);
    }
    return NULL;
}

axis2_status_t
wsf_util_get_http_headers_from_op_client (
    axis2_op_client_t * op_client,
    const axutil_env_t * env,
	zval* return_val,
    axis2_wsdl_msg_labels_t msg_label)
{
    if (op_client) {
        const axis2_msg_ctx_t *msg_ctx = NULL;
		axutil_array_list_t *list = NULL;
        axis2_http_header_t *header = NULL;
        int i = 0;
		msg_ctx = axis2_op_client_get_msg_ctx (op_client, env, msg_label);
        if (!msg_ctx)
            return AXIS2_FAILURE;
        list = axis2_msg_ctx_get_http_output_headers((axis2_msg_ctx_t *)msg_ctx, env);

        if (list) {
			array_init(return_val);
            if (!axutil_array_list_is_empty (list, env)) {
                for (i = 0; i < axutil_array_list_size (list, env); i++) {
					axis2_char_t *header_val = NULL;
                    header = (axis2_http_header_t *) axutil_array_list_get (list, env, i);
					if(header && (header_val  = axis2_http_header_to_external_form (header, env)))
					{
						axis2_char_t *name = NULL;
						axis2_char_t *value = NULL;
						name = axis2_http_header_get_name(header, env);
						value = axis2_http_header_get_value(header, env);
						if(name && value)
						{
							add_assoc_string_ex(return_val, name, strlen(name)+1, value, 1);
						}
					}
                }
            }
        }
    }
	return AXIS2_SUCCESS;
}

int
wsf_util_engage_module (
    axis2_conf_t * conf,
    axis2_char_t * module_name,
    const axutil_env_t * env,
    axis2_svc_t * svc)
{
    axis2_module_desc_t *module = NULL;
    axutil_qname_t *mod_qname = NULL;
    axis2_phase_resolver_t *phase_resolver = NULL;
    int status = AXIS2_FAILURE;

    mod_qname = axutil_qname_create (env, module_name, NULL, NULL);
    module = axis2_conf_get_module (conf, env, mod_qname);
    if (module) 
	{
        status = axis2_svc_engage_module (svc, env, module, conf);
		AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
			"Engaging module %s to service ", module_name);
        if (!status) 
		{
            phase_resolver =
                axis2_phase_resolver_create_with_config (env, conf);
            if (!phase_resolver) 
			{
                AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                    "Engaging module, PHASE RESLOVER NULL");
                return AXIS2_FAILURE;
            }
            status =
                axis2_phase_resolver_engage_module_to_svc (phase_resolver,
                env, svc, module);
        }
    }
    return status;
}

/* generate service name from uri */
char *
wsf_util_generate_svc_name_from_uri_and_set_loc_str (
    char *req_uri,
    wsf_svc_info_t * svc_info,
    axutil_env_t * env)
{
    char *svc_name = NULL;
    char *temp_string = NULL;
    char *index = NULL;
    char *uri = NULL;

    if (strncmp (req_uri, "/", 1) == 0) {
                /** start with a / */
        uri = req_uri + 1;
    } else {
        uri = req_uri;
    }

    temp_string = estrdup (uri);

    index = strstr (temp_string, ".php/");

    if (index) {
        char *loc_str = NULL;
        index = index + 4;
        loc_str = index + 1;
        temp_string[index - temp_string] = '\0';
        svc_info->loc_str = axutil_strdup (env, loc_str);
    }

    svc_name = axutil_replace (env, temp_string, '/', '_');

    efree (temp_string);

    return svc_name;
}

/* create service */
void
wsf_util_create_svc_from_svc_info (
    wsf_svc_info_t * svc_info,
    axutil_env_t * env TSRMLS_DC)
{
    axutil_qname_t *svc_qname = NULL;
    axis2_svc_t *svc = NULL;
    axis2_conf_t *conf = NULL;
    axis2_conf_ctx_t *conf_ctx = NULL;

    if (!svc_info->php_worker) {
        php_error_docref (NULL TSRMLS_CC, E_ERROR, "error creating service");
        return;
    }

    conf_ctx = wsf_worker_get_conf_ctx (svc_info->php_worker, env);

    conf = axis2_conf_ctx_get_conf (conf_ctx, env);
    if (!conf) {
        php_error_docref (NULL TSRMLS_CC, E_ERROR, "error creating qname");
        return;
    }

    svc = axis2_conf_get_svc (conf, env, svc_info->svc_name);

    if (NULL != svc) {

        svc_info->svc = svc;

    } else {
        svc_qname = axutil_qname_create (env, svc_info->svc_name, NULL, NULL);
        svc_info->svc = axis2_svc_create_with_qname (env, svc_qname);
        axutil_qname_free (svc_qname, env);
    }
    return;
}

/** create operation */
void
wsf_util_create_op_and_add_to_svc (
    wsf_svc_info_t * svc_info,
    axutil_env_t * env,
    char *op_name,
    HashTable * ht_mep TSRMLS_DC)
{
    axis2_svc_t *svc = NULL;
    axis2_op_t *op = NULL;
    axutil_qname_t *op_qname = NULL;
    zval **tmp;
    op_qname = axutil_qname_create (env, op_name, NULL, NULL);
    svc = svc_info->svc;

    if (NULL != svc && NULL != op_name) {
        op = axis2_svc_get_op_with_name (svc_info->svc, env, op_name);
        if (!op) {
            axis2_conf_t *conf = NULL;
            axis2_conf_ctx_t *conf_ctx = NULL;
            axis2_phases_info_t *info = NULL;

            op_qname = axutil_qname_create (env, op_name, NULL, NULL);

            op = axis2_op_create_with_qname (env, op_qname);

            axis2_op_set_msg_recv (op, env, svc_info->msg_recv);

            conf_ctx = wsf_worker_get_conf_ctx (svc_info->php_worker, env);

            conf = axis2_conf_ctx_get_conf (conf_ctx, env);

            info = axis2_conf_get_phases_info (conf, env);
            
			axis2_phases_info_set_op_phases (info, env, op);
            
			axis2_svc_add_op (svc_info->svc, env, op);

            if (ht_mep) {
                char operation[300];

                AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, "[wsf_service] ht mep not null, %s", op_name);
                sprintf (operation, "%s", op_name);
                if (zend_hash_find (ht_mep, operation, strlen (operation) + 1,
                        (void **) &tmp) == SUCCESS
                    && Z_TYPE_PP (tmp) == IS_STRING) {
                    char *mep = NULL;
                    mep = Z_STRVAL_PP (tmp);
                    if (mep) {
                        AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                            "op mep %s", mep);
                        if (strcmp (mep, WSF_IN_ONLY) == 0) {
                            axis2_op_set_msg_exchange_pattern (op, env,
                                AXIS2_MEP_URI_IN_ONLY);
                            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                                "AXIS2_MEP_URI_IN_ONLY");

                        } else if (strcmp (mep, WSF_IN_OUT) == 0) {
                            axis2_op_set_msg_exchange_pattern (op, env,
                                AXIS2_MEP_URI_IN_OUT);
                            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                                " AXIS2_MEP_URI_IN_OUT");
                        }
                    }
                } else {
                    AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                        " message exchange pattern for %s not found",
                        op_name);
                }
            }
        }
    }
	if(op_qname){
		axutil_qname_free(op_qname, env);
	}

    return;
}

void wsf_util_set_addr_action_to_op(
    wsf_svc_info_t * svc_info,
    char *action,
    axutil_env_t * env,
    char *op_name TSRMLS_DC)
{
    axis2_op_t *op = NULL;
    
    if (NULL != svc_info->svc && NULL != op_name) {
        op = axis2_svc_get_op_with_name (svc_info->svc, env, op_name);
        if (!op) {
                    AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                        " %s operation not found when trying to set the action ",
                        op_name);
       }
       if (action) {
                axis2_svc_add_mapping (svc_info->svc, env, action, op);
       }
    }
    return;
}

void
wsf_util_set_attachments_with_cids (
    const axutil_env_t * env,
    int enable_mtom,
    int enable_swa,
    axiom_node_t * payload_node,
    HashTable * attach_ht,
    char *default_cnt_type TSRMLS_DC)
{
    axiom_node_t *node = NULL;
    axiom_node_t *tmp_node = NULL;
    axiom_element_t *payload_element = NULL;
    axiom_element_t *ele = NULL;
    if (!payload_node || !attach_ht)
        return;

    if (axiom_node_get_node_type (payload_node, env) == AXIOM_ELEMENT) 
	{
        payload_element = (axiom_element_t *) axiom_node_get_data_element (payload_node, env);
        axiom_element_get_first_element (payload_element, env, payload_node, &node);

        if (node && axiom_node_get_node_type (node, env) == AXIOM_ELEMENT) 
		{

            ele = (axiom_element_t *) axiom_node_get_data_element (node, env);
            if (ele) 
			{
                axiom_namespace_t *ns = NULL;
                axis2_char_t *ns_uri = NULL;
                axis2_char_t *ele_localname = NULL;

                ele_localname = axiom_element_get_localname (ele, env);
                if (ele_localname && axutil_strcmp (ele_localname, AXIS2_XOP_INCLUDE) == 0) 
				{
                    ns = axiom_element_get_namespace (ele, env, node);
                    if (ns && (ns_uri = axiom_namespace_get_uri (ns, env)) &&
                        axutil_strcmp (ns_uri, AXIS2_XOP_NAMESPACE_URI) == 0) 
					{

                        axis2_char_t *cnt_type = NULL;
                        /* found a matching xop include element */
                        axis2_char_t *cid = NULL;
                        axis2_char_t *pos = NULL;
                        axis2_char_t *id = NULL;
                        zval **tmp = NULL;
                        axutil_hash_t *attribute_hash = NULL;
                        axiom_attribute_t *content_type_attri = NULL;

                        /** look for content type in parent */
                        attribute_hash = axiom_element_get_all_attributes(payload_element, env);

                        if(attribute_hash)
                        {
                             axutil_hash_index_t *hi;
                             void *val;
                             const void *key;
                        
                             for (hi = axutil_hash_first(attribute_hash, env); hi; hi = axutil_hash_next(env, hi))
                             {
                                 axutil_hash_this(hi, &key, NULL, &val);
                        
                                 if(strstr((axis2_char_t*)key, "contentType|http://www.w3.org/2004/06/xmlmime")
                                    ||strstr((axis2_char_t*)key, "contentType|http://www.w3.org/2005/05/xmlmime"))
                    
                                 {
                                     content_type_attri = (axiom_attribute_t*)val;
                                     break;
                                 }
                             }
                        }

                        if(content_type_attri) 
						{
                            cnt_type = axiom_attribute_get_value(content_type_attri, env);
                        }
                        else {
                            cnt_type = axiom_element_get_attribute_value_by_name
                                    (payload_element, env, "contentType");
                            
                        }
                        if (!cnt_type) {
                            cnt_type = default_cnt_type;
                        }

                        id = axiom_element_get_attribute_value_by_name (ele, env, "href");
                        if (!id)
                            return;
                        pos = axutil_strstr (id, "cid:");
                        if (pos) 
						{
                            cid = id + 4;
                            if (zend_hash_find (attach_ht, cid, strlen (cid) + 1, 
								(void **) &tmp) == SUCCESS && Z_TYPE_PP (tmp) == IS_STRING) 
							{
                                /** detach this node */
                                axiom_node_t *text_node = NULL;
                                axiom_text_t *text = NULL;
                                axiom_data_handler_t *data_handler = NULL;
                                axiom_node_detach (node, env);

                                tmp_node = axiom_node_get_first_child (payload_node, env);
                                while (tmp_node) 
								{
                                    axiom_node_t *next_tmp_node = NULL;
                                    next_tmp_node = axiom_node_get_next_sibling (tmp_node, env);

                                    axiom_node_free_tree(tmp_node, env);

                                    tmp_node = next_tmp_node;
                                }
								if(WSF_GLOBAL(enable_attachment_caching))
								{	/** If attachment caching is enabled, user will set the file name
									instead of the binary data */
									axis2_char_t *filename = Z_STRVAL_PP(tmp);
									filename = wsf_util_get_real_path(env,filename  TSRMLS_CC);
									AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
										WSF_PHP_LOG_PREFIX "Real path is %s", filename);

									if(filename && axutil_file_handler_access(filename, AXIS2_R_OK))
									{
											data_handler = axiom_data_handler_create(env, filename, cnt_type);
									}
								}else
								{
									void *binary_data = NULL;
									int binary_data_len = 0;

									binary_data_len = Z_STRLEN_PP (tmp);
									binary_data = AXIS2_MALLOC (env->allocator, sizeof (char) * binary_data_len);

									memcpy (binary_data, Z_STRVAL_PP (tmp), binary_data_len);
									if(binary_data)
									{
									/** attachment caching is not enabled, user provides binary data directly */
										data_handler = axiom_data_handler_create (env, NULL, cnt_type);

										axiom_data_handler_set_binary_data (data_handler, env, binary_data,
											binary_data_len);
									}
								}
                                text = axiom_text_create_with_data_handler
                                    (env, payload_node, data_handler, &text_node);

                                if (enable_swa) 
								{
                                    axiom_text_set_is_swa(text, env, AXIS2_TRUE);
                                }

                                if (enable_mtom == AXIS2_FALSE) 
								{
                                    axiom_text_set_optimize (text, env,
                                        AXIS2_FALSE);
                                }
                                return;
                            }
                        }
                    }
                }
            }
        }
    }
    tmp_node = axiom_node_get_first_child (payload_node, env);
    while (tmp_node) {
        wsf_util_set_attachments_with_cids (env, enable_mtom, enable_swa, tmp_node,
            attach_ht, default_cnt_type TSRMLS_CC);
        tmp_node = axiom_node_get_next_sibling (tmp_node, env);
    }
    return;
}

int wsf_util_get_attachments_from_soap_envelope (
     const axutil_env_t * env,
    axiom_soap_envelope_t *soap_envelope,
    zval * cid2str,
    zval * cid2contentType TSRMLS_DC)
{
    int attachments_found = 0;
    axiom_soap_builder_t *soap_builder = NULL;
    axutil_hash_t *mime_body_parts = NULL;

    soap_builder = axiom_soap_envelope_get_soap_builder(soap_envelope, env);

    if(soap_builder) {
        mime_body_parts = axiom_soap_builder_get_mime_body_parts(soap_builder, env);

        if(mime_body_parts) {
            axutil_hash_index_t *hi = NULL;
            axis2_char_t *cid = NULL;
            axiom_data_handler_t *data_handler = NULL;
            for (hi = axutil_hash_first(mime_body_parts, env);
                 hi; hi = axutil_hash_next(env, hi))    
            {                                                           
                axutil_hash_this(hi, (const void **)&cid, NULL, (void**)&data_handler); 
                if(cid && data_handler) 
                {
                    char *cnt_type = NULL;
                    char *data = NULL;
                    int data_len = 0;
					if(!axiom_data_handler_get_cached(data_handler, env))
					{
						data = axiom_data_handler_get_input_stream(data_handler, env);
						data_len = axiom_data_handler_get_input_stream_len(data_handler, env);
						/* axiom_data_handler_read_from (data_handler, env, &data, &data_len);*/
	                    			add_assoc_stringl (cid2str, cid, data, data_len, 1);
					}else
					{
						data = axiom_data_handler_get_file_name(data_handler, env);
						if(data)
						{
							data_len = strlen(data);
						}
						add_assoc_stringl(cid2str, cid, data, data_len, 1);
					}

					cnt_type = axiom_data_handler_get_content_type(data_handler, env);
					attachments_found = 1;
                    if (cnt_type) {
                        add_assoc_stringl (cid2contentType, cid,
                            cnt_type, strlen (cnt_type), 1);
                    }
                    
                }
            }   
        }
    }
    return attachments_found;
}

int
wsf_util_get_attachments (
    const axutil_env_t * env,
    axiom_node_t * om_node,
    zval * cid2str,
    zval * cid2contentType TSRMLS_DC)
{
    axiom_node_t *temp_node = NULL;
    axiom_node_t *nodes[256];
    int count = 0;
    int attachments_found = 0;

    if (!om_node)
    {
        return 0;
    }

    nodes[count++] = om_node;


    do {
		if (axiom_node_get_node_type(om_node, env) == AXIOM_TEXT) {
			axiom_text_t *text = NULL;
			axiom_data_handler_t *data_handler = NULL;
			
			
			text = (axiom_text_t*)axiom_node_get_data_element(om_node, env);
			if(text) {
				data_handler = axiom_text_get_data_handler (text, env);
                if (data_handler) {
                    char *cnt_type = NULL;
                    char *data = NULL;
                    int data_len = 0;
					char *cid = NULL;

                    axiom_data_handler_read_from (data_handler, env, &data, &data_len);
                    cnt_type = axiom_data_handler_get_content_type(data_handler, env);
					cid = axiom_text_get_content_id(text, env);
					add_assoc_stringl (cid2str, cid, data, data_len, 1);
                    attachments_found = 1;
					if (cnt_type) {
                        add_assoc_stringl (cid2contentType, cid, 
                            cnt_type, strlen (cnt_type), 1);

                    }
                }			
			}
        }
        temp_node = axiom_node_get_first_child(om_node, env);
        if (temp_node)
        {
            om_node = temp_node;
            nodes[count++] = om_node;
        }
        else
        {
            temp_node = axiom_node_get_next_sibling(om_node, env);
            if (temp_node)
            {
                om_node = temp_node;
                nodes[count -1] = om_node;
            }
            else
            {
                while (count > 1 && !temp_node)
                {
                    count--;
                    om_node = nodes[count -1];
                    temp_node = axiom_node_get_next_sibling(om_node, env);
                }
               
                if (temp_node && count > 1)
                {
                    om_node = temp_node;
                    nodes[count -1] = om_node;
                }
                else
                {
                    count--;
                }
            }
        }
    } while(count > 0);
    
    return attachments_found;
}

void
wsf_util_find_xop_content_and_convert_to_base64 (
    const axutil_env_t * env,
    axiom_node_t * om_node)
{
        axiom_node_t *temp_node = NULL;
    axiom_node_t *nodes[256];
    int count = 0;

    if (!om_node)
    {
        return;
    }

    nodes[count++] = om_node;

    do {
		if (axiom_node_get_node_type(om_node, env) == AXIOM_TEXT) 
		{
			axiom_text_t *text = NULL;
			axiom_data_handler_t *data_handler = NULL;
			
			
			text = (axiom_text_t*)axiom_node_get_data_element(om_node, env);
			if(text) 
			{
				data_handler = axiom_text_get_data_handler (text, env);
                if (data_handler) 
				{
                   axiom_text_set_optimize(text, env, AXIS2_FALSE);
                }			
			}
        }
        temp_node = axiom_node_get_first_child(om_node, env);
        if (temp_node)
        {
            om_node = temp_node;
            nodes[count++] = om_node;
        }
        else
        {
            temp_node = axiom_node_get_next_sibling(om_node, env);
            if (temp_node)
            {
                om_node = temp_node;
                nodes[count -1] = om_node;
            }
            else
            {
                while (count > 1 && !temp_node)
                {
                    count--;
                    om_node = nodes[count -1];
                    temp_node = axiom_node_get_next_sibling(om_node, env);
                }
               
                if (temp_node && count > 1)
                {
                    om_node = temp_node;
                    nodes[count -1] = om_node;
                }
                else
                {
                    count--;
                }
            }
        }
    } while(count > 0);
    
    return;
}

char *
wsf_util_serialize_om (
    const axutil_env_t * env,
    axiom_node_t * ret_node)
{
    axiom_xml_writer_t *writer = NULL;
    axiom_output_t *om_output = NULL;
    axis2_char_t *buffer = NULL, *new_buffer = NULL;
    unsigned int buffer_len = 0;

    writer = axiom_xml_writer_create_for_memory (env, NULL, AXIS2_TRUE, 0,
        AXIS2_XML_PARSER_TYPE_BUFFER);
    om_output = axiom_output_create (env, writer);

    /* We should serialize the sub tree to preserve namespaces that reside 
       outside ret_node but referenced form within the ret_node tree. 
       Hence use axiom_node_serialize_sub_tree and not axiom_node_serialize.
       axiom_node_serialize assumes the OM tree to be complete, hence, does 
       not double check if all referenced namespaces are serialized, but 
       axiom_node_serialize_sub_tree does. */
    axiom_node_serialize_sub_tree(ret_node, env, om_output); 

    buffer = (axis2_char_t *) axiom_xml_writer_get_xml (writer, env);
    buffer_len = axutil_strlen (buffer);

    new_buffer =
        AXIS2_MALLOC (env->allocator,
        sizeof (axis2_char_t) * (buffer_len + 1));
    memcpy (new_buffer, buffer, buffer_len);
    new_buffer[buffer_len] = '\0';

	axiom_output_free(om_output, env);

    return new_buffer;
}


axiom_node_t *
wsf_util_deserialize_buffer (
    const axutil_env_t * env,
    char *buffer)
{
    axiom_xml_reader_t *reader = NULL;
    axiom_stax_builder_t *builder = NULL;
    axiom_document_t *document = NULL;
    axiom_node_t *payload = NULL;

    reader = axiom_xml_reader_create_for_memory (env, buffer, axutil_strlen (buffer), 
		AXIS2_UTF_8, AXIS2_XML_PARSER_TYPE_BUFFER);

    if (!reader) 
	{
        return NULL;
    }

    builder = axiom_stax_builder_create (env, reader);

    if (!builder) 
	{
        return NULL;
    }
    document = axiom_stax_builder_get_document (builder, env);
    if (!document) 
	{
        AXIS2_LOG_ERROR (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Document is not found");
        return NULL;
    }

    payload = axiom_document_get_root_element (document, env);

    if (!payload) 
	{
        AXIS2_LOG_ERROR (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
            "Root element of the document is not found");
        return NULL;
    }
    axiom_document_build_all (document, env);
  
	axiom_stax_builder_free_self (builder, env);

    return payload;
}

void
wsf_util_set_soap_fault (
    zval * this_ptr,
    char *fault_code_ns,
    char *fault_code,
    char *fault_reason,
    char *fault_role,
    zval * fault_detail,
    char *name TSRMLS_DC)
{
    if (!this_ptr)
        return;

    if (fault_reason != NULL) 
	{
        add_property_string (this_ptr, WSF_FAULT_REASON, fault_reason, 1);
#ifdef ZEND_ENGINE_2
        zend_update_property_string (zend_exception_get_default (TSRMLS_C),
            this_ptr, "message", sizeof ("message") - 1,
            fault_reason TSRMLS_CC);
#endif
    }
    if (fault_code != NULL) 
	{
        int soap_version = WSF_GLOBAL (soap_version);

        if (fault_code_ns) 
		{
            add_property_string (this_ptr, WSF_FAULT_CODE, fault_code, 1);
            add_property_string (this_ptr, WSF_FAULT_CODE_NS, fault_code_ns,
                1);
        } else 
		{
            if (soap_version == AXIOM_SOAP11) 
			{
                add_property_string (this_ptr, WSF_FAULT_CODE, fault_code, 1);
                if (strcmp (fault_code, AXIOM_SOAP11_FAULT_CODE_SENDER) == 0 ||
                    strcmp (fault_code, AXIOM_SOAP12_SOAP_FAULT_VALUE_RECEIVER) == 0 ||
                    strcmp (fault_code, AXIOM_SOAP12_SOAP_FAULT_VALUE_VERSION_MISMATCH) == 0 ||
                    strcmp (fault_code, AXIOM_SOAP_ATTR_MUST_UNDERSTAND) == 0) 
				{
                    add_property_string (this_ptr, WSF_FAULT_CODE_NS,
                        WSF_SOAP_1_1_NAMESPACE_URI, 1);
                }
            } 
			else if (soap_version == AXIOM_SOAP12) 
			{
                if (strcmp (fault_code, AXIOM_SOAP12_SOAP_FAULT_VALUE_SENDER) == 0) 
				{
                    add_property_string (this_ptr, WSF_FAULT_CODE,  
						AXIOM_SOAP12_SOAP_FAULT_VALUE_SENDER, 1);
                    add_property_string (this_ptr, WSF_FAULT_CODE_NS, WSF_SOAP_1_2_NAMESPACE_URI, 1);
                } else if (strcmp (fault_code, AXIOM_SOAP11_FAULT_CODE_RECEIVER) == 0) 
				{
                    add_property_string (this_ptr, WSF_FAULT_CODE, 
						AXIOM_SOAP12_SOAP_FAULT_VALUE_RECEIVER, 1);
                    add_property_string (this_ptr, WSF_FAULT_CODE_NS, WSF_SOAP_1_1_NAMESPACE_URI, 1);

                } else if (strcmp (fault_code, AXIOM_SOAP12_SOAP_FAULT_VALUE_VERSION_MISMATCH) == 0
                    || strcmp (fault_code, AXIOM_SOAP_ATTR_MUST_UNDERSTAND) == 0
                    || strcmp (fault_code, AXIOM_SOAP12_SOAP_FAULT_VALUE_DATA_ENCODING_UKNOWN) == 0) 
				{
                    add_property_string (this_ptr, WSF_FAULT_CODE, fault_code, 1);
                    add_property_string (this_ptr, WSF_FAULT_CODE_NS, WSF_SOAP_1_2_NAMESPACE_URI, 1);
                } else 
				{
                    add_property_string (this_ptr, WSF_FAULT_CODE, fault_code, 1);
                }
            }
        }
    }
    if (fault_role != NULL)
	{
        add_property_string (this_ptr, WSF_FAULT_ROLE, fault_role, 1);
    }
    if (fault_detail != NULL) 
	{
        add_property_zval (this_ptr, WSF_FAULT_DETAIL, fault_detail);
    }
    if (name != NULL) 
	{
        add_property_string (this_ptr, WSF_FAULT_NAME, name, 1);
    }
}

static void 
wsf_util_handle_fault_code(
   zval *fault_obj,
   axiom_node_t *code_node,
   axiom_element_t *code_element,
   axutil_env_t *env, 
   int soap_version TSRMLS_DC)
{
    axiom_node_t *code_value_node = NULL;
    axiom_element_t *code_value_ele = NULL;
    char *code = NULL;
	if(soap_version == AXIOM_SOAP12)
	{
        code_value_ele = axiom_element_get_first_element(code_element, env, code_node, &code_value_node);
		code = axiom_element_get_text(code_value_ele, env, code_value_node);
        if(code)
		{
            add_property_string(fault_obj, WSF_FAULT_CODE, code, 1);
        }                  
	}else if(soap_version == AXIOM_SOAP11)
	{
		code = axiom_element_get_text(code_element, env, code_node);
		if(code)
		{
			add_property_string(fault_obj, WSF_FAULT_CODE, code, 1);
		}
	}
}

static void 
wsf_util_handle_fault_reason(
    zval *fault_obj,
    axiom_node_t *reason_node,
    axiom_element_t *reason_element,
    axutil_env_t *env,
	int soap_version TSRMLS_DC)
{
	axis2_char_t *text_value = NULL;

    if(soap_version == AXIOM_SOAP12)
	{
        axiom_node_t *text_node = NULL;
        axiom_element_t *text_element = NULL;
        text_element = axiom_element_get_first_element(reason_element, env, reason_node,
                &text_node);
        if(text_element)
		{
            text_value = axiom_element_get_text(text_element, env, text_node);
            if(text_value)
			{
				add_property_string(fault_obj, WSF_FAULT_REASON , text_value, 1);

				zend_update_property_string(zend_exception_get_default(TSRMLS_C), fault_obj,
					"message", sizeof("message")-1, text_value TSRMLS_CC);
            }
        }
	}else if(soap_version == AXIOM_SOAP11)
	{
		text_value = axiom_element_get_text(reason_element, env, reason_node);
		if(text_value)
		{
			add_property_string(fault_obj, WSF_FAULT_REASON, text_value, 1);
			zend_update_property_string(zend_exception_get_default(TSRMLS_C), fault_obj,
					"message", sizeof("message")-1, text_value TSRMLS_CC);
		}
	}
}


static void
wsf_util_handle_fault_detail(
    zval *fault_obj,
    axiom_node_t *detail_node,
    axiom_element_t *detail_element,
    axutil_env_t *env TSRMLS_DC)
{
	axis2_char_t *detail_string = NULL;
	axiom_node_t *detail_entry_node = NULL;
	if(detail_node)
	{
		detail_entry_node = axiom_node_get_first_element(detail_node, env);
		if(detail_entry_node)
		{
			detail_string = axiom_node_to_string(detail_entry_node, env);
			add_property_string(fault_obj, WSF_FAULT_DETAIL, detail_string, 1);
			if(detail_string)
			{
				AXIS2_FREE(env->allocator, detail_string);
			}
		}
	}
}

static void
wsf_util_handle_fault_role(
	zval *fault_obj,
	axiom_node_t *role_node,
	axiom_element_t *role_element,
	axutil_env_t *env TSRMLS_DC)
{
	axis2_char_t *role_uri = NULL;
	if(role_node && role_element)
	{
		role_uri = axiom_element_get_text(role_element, env, role_node);
		if(role_uri)
		{
             add_property_string(fault_obj, WSF_FAULT_ROLE, role_uri, 1);
		}
	}
}

void
wsf_util_handle_soap_fault(
	zval *fault_obj,
	axutil_env_t *env,
	axiom_node_t *fault_node,
	int soap_version TSRMLS_DC)
{
    axiom_element_t *fault_element = NULL;
    axiom_node_t *node = NULL;
    axiom_element_t *element = NULL;
    axiom_child_element_iterator_t *ele_iterator = NULL;

    if(!fault_node) return;
    
    fault_element = axiom_node_get_data_element(fault_node, env);

    ele_iterator = axiom_element_get_child_elements(fault_element, env, fault_node);

    if(!ele_iterator)
        return;
    while(AXIOM_CHILD_ELEMENT_ITERATOR_HAS_NEXT(ele_iterator, env))
	{
        node = AXIOM_CHILD_ELEMENT_ITERATOR_NEXT(ele_iterator, env);
        if(node)
		{
            element = axiom_node_get_data_element(node, env);
            if(element)
			{
                char *localname = NULL;
                localname = axiom_element_get_localname(element, env);
                if(localname)
				{
					if(strcmp(localname,AXIOM_SOAP12_SOAP_FAULT_CODE_LOCAL_NAME) == 0)
					{
						wsf_util_handle_fault_code(fault_obj, node, element, env, AXIOM_SOAP12 TSRMLS_CC);
					}else if(strcmp(localname, AXIOM_SOAP11_SOAP_FAULT_CODE_LOCAL_NAME) == 0)
					{
						wsf_util_handle_fault_code(fault_obj, node, element, env, AXIOM_SOAP11 TSRMLS_CC);
					}else if(strcmp(localname, AXIOM_SOAP12_SOAP_FAULT_REASON_LOCAL_NAME) == 0)
					{
                        wsf_util_handle_fault_reason(fault_obj, node, element, env, AXIOM_SOAP12 TSRMLS_CC);
					}else if(strcmp(localname,AXIOM_SOAP11_SOAP_FAULT_STRING_LOCAL_NAME) ==0)
					{
						wsf_util_handle_fault_reason(fault_obj, node, element, env, AXIOM_SOAP11 TSRMLS_CC);
					}else if((strcmp(localname, AXIOM_SOAP12_SOAP_FAULT_DETAIL_LOCAL_NAME)  == 0) ||
						(strcmp(localname, AXIOM_SOAP11_SOAP_FAULT_DETAIL_LOCAL_NAME)  == 0))
					{
                        wsf_util_handle_fault_detail(fault_obj, node, element, env TSRMLS_CC); 
                    }else if((strcmp(localname, AXIOM_SOAP12_SOAP_FAULT_ROLE_LOCAL_NAME) == 0) ||
						(strcmp(localname, AXIOM_SOAP11_SOAP_FAULT_ACTOR_LOCAL_NAME) == 0))
					{
						wsf_util_handle_fault_role(fault_obj, node, element, env TSRMLS_CC);                        
                    }
                }
            }
        }                
    }
}


int wsf_util_is_module_engaged_to_svc_client(
	const axis2_svc_client_t *svc_client,
	axutil_env_t *env,
	char *module_name)
{
	axutil_qname_t *mod_qname = NULL;
	axis2_svc_t *svc = NULL;
	int status = 0;
	if(!svc_client || !env || !module_name)
		return 0;
	mod_qname = axutil_qname_create(env, module_name, NULL, NULL);
	svc = axis2_svc_client_get_svc(svc_client, env);
	if(svc && mod_qname)
	{
		status = axis2_svc_is_module_engaged(svc, env, mod_qname);
		axutil_qname_free(mod_qname, env);
	}
	return status;	
}

int
wsf_util_add_svc_to_conf(
	axutil_env_t *env,
	wsf_svc_info_t *svc_info,
	axis2_conf_ctx_t *conf_ctx)
{
	axis2_conf_t *conf = NULL;
	conf = axis2_conf_ctx_get_conf(conf_ctx, env);
	if(conf && !axis2_conf_get_svc(conf, env, svc_info->svc_name))
	{
		axis2_conf_add_svc (conf, env, svc_info->svc);
		return AXIS2_SUCCESS;
	}
	return AXIS2_FAILURE;
}

int wsf_util_find_and_set_svc_ctx(
	const axutil_env_t *env,
	wsf_svc_info_t *svc_info,
	axis2_conf_ctx_t *conf_ctx) {


    axis2_svc_ctx_t *svc_ctx = NULL;
    axis2_svc_grp_t *svc_grp = NULL;
    axis2_svc_grp_ctx_t *svc_grp_ctx = NULL;
    const axis2_char_t *svc_grp_id = NULL;
    svc_grp = axis2_svc_get_parent(svc_info->svc, env);
	if(svc_grp)
    svc_grp_id = axis2_svc_grp_get_name(svc_grp, env);
    if(svc_grp_id) {
        svc_grp_ctx = axis2_conf_ctx_get_svc_grp_ctx(conf_ctx, env, svc_grp_id);
    }
    if(svc_grp_ctx) {
        svc_ctx = axis2_svc_grp_ctx_get_svc_ctx(svc_grp_ctx, env, svc_info->svc_name);
    }

    if(svc_ctx) {
        svc_info->svc_ctx = svc_ctx;
        return AXIS2_SUCCESS;
    }
    return AXIS2_FAILURE;

}

void wsf_util_engage_modules_to_svc(
    axutil_env_t *env,
    axis2_conf_ctx_t *conf_ctx,
    wsf_svc_info_t *svc_info)
{
    axis2_conf_t *conf = NULL;
    conf = axis2_conf_ctx_get_conf (conf_ctx, env);
    if (conf && svc_info->svc)
	{
		if(!axis2_conf_get_svc (conf, env, svc_info->svc_name))
		{
			axis2_conf_add_svc (conf, env, svc_info->svc);
		}
		if (NULL != svc_info->modules_to_engage)
		{
            int i = 0;
            int size = axutil_array_list_size (svc_info->modules_to_engage, env);
            for (i = 0; i < size; i++)
			{
                axis2_char_t * mod_name = (axis2_char_t *) axutil_array_list_get (
                    svc_info->modules_to_engage, env, i);
                    wsf_util_engage_module (conf, mod_name, env,
                        svc_info->svc);
            }
        }
    }
}

void wsf_util_process_ws_service_operations(
        HashTable *ht_ops_to_funcs,
        HashTable *ht_ops_to_mep,
        wsf_svc_info_t *svc_info,
        axutil_env_t *ws_env_svr TSRMLS_DC)
{
    HashPosition pos;
    zval ** tmp = NULL;
    int i = 0;
   
    if (!ht_ops_to_funcs) 
	{
        return;
    }
    
    zend_hash_internal_pointer_reset_ex (ht_ops_to_funcs, &pos);
    while (zend_hash_get_current_data_ex (ht_ops_to_funcs, (void **) &tmp, &pos) != FAILURE) 
	{

        char *op_name_to_store = NULL;
        char *op_name = NULL;
        unsigned int op_name_len = 0;
        unsigned long index = i;
        char *func_name = NULL;
	    zend_function * f;
        char *function_name = NULL;
        int key_len = 0;
            
        zend_hash_get_current_key_ex (ht_ops_to_funcs, &op_name, &op_name_len, &index, 0, &pos);
        
        func_name = Z_STRVAL_PP (tmp);
        if (op_name)
		{
            op_name_to_store = op_name;
        } 
		else 
		{
            op_name_to_store = func_name;
        }

   	    key_len = Z_STRLEN_PP(tmp);
        function_name = emalloc (key_len + 1);
        zend_str_tolower_copy (function_name, func_name, key_len);

		if (zend_hash_find (EG (function_table), function_name, key_len + 1, (void **) &f) == FAILURE) 
		{
            efree(function_name);
            php_error_docref (NULL TSRMLS_CC, E_ERROR, "Named function not in function table");
            AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
				" %s function not defined", func_name);
        }
		else
		{
    	    axutil_hash_set (svc_info->ops_to_functions, axutil_strdup (ws_env_svr, op_name_to_store),
                AXIS2_HASH_KEY_STRING, axutil_strdup (ws_env_svr, func_name));

     	    wsf_util_create_op_and_add_to_svc (svc_info, ws_env_svr,
                op_name_to_store, ht_ops_to_mep TSRMLS_CC);
     	    efree(function_name);
		 } 
        zend_hash_move_forward_ex (ht_ops_to_funcs, &pos);
        i++;
    }
}

void wsf_util_process_ws_service_op_actions(
        HashTable *ht_actions,
        wsf_svc_info_t *svc_info,
        axutil_env_t *ws_env_svr TSRMLS_DC)
{
    HashPosition pos;
    zval ** tmp = NULL;
 
    if (!ht_actions)
	{
        return;
    }
    
    zend_hash_internal_pointer_reset_ex (ht_actions, &pos);

    while (zend_hash_get_current_data_ex (ht_actions, (void **) &tmp, &pos) != FAILURE)
	{

        char *func_name = NULL;
	    char *wsa_action = NULL;
        uint str_length = 0;
        ulong num_index = 0;
        char *operation_name = NULL;
        operation_name = Z_STRVAL_PP (tmp);

        if (zend_hash_get_current_key_ex (ht_actions, &wsa_action, &str_length, 
			&num_index, AXIS2_TRUE, &pos) == FAILURE)
		{
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Key Restting function failed");
        }
        if (Z_TYPE_PP (tmp) != IS_STRING)
		{
            php_error_docref (NULL TSRMLS_CC, E_ERROR, "Tried to add a function that isn't a string");
        }

        func_name = axutil_hash_get (svc_info->ops_to_functions, 
			Z_STRVAL_PP (tmp), AXIS2_HASH_KEY_STRING);

        if (!func_name)
		{
            /** function corresponding to operation does not exist in the 
             *  ops to function table, this operation is assumed to be a function and 
             *  not to be a class method and check whether it exists. 
             *  If so it will be added as an operation
             * */
            char *key = NULL;
            int key_len = 0;
            zend_function *f;

            key_len = Z_STRLEN_PP(tmp);
            key = emalloc (key_len + 1);

            zend_str_tolower_copy (key, operation_name , key_len);
			
            if (zend_hash_find (EG (function_table), key, key_len + 1, (void **) &f) == FAILURE)
			{
                AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
					"%s The function does not exist in function table ", operation_name);
            }
			else
			{
				axutil_hash_set (svc_info->ops_to_functions, axutil_strdup (ws_env_svr, Z_STRVAL_PP (tmp)),
                    AXIS2_HASH_KEY_STRING, axutil_strdup (ws_env_svr, Z_STRVAL_PP (tmp)));
				/*
				wsf_util_create_op_and_add_to_svc (svc_info, ws_env_svr, 
					Z_STRVAL_PP(tmp), NULL TSRMLS_CC);
				*/
			 } 
            efree(key);
		}
        if (wsa_action) 
		{
            wsf_util_set_addr_action_to_op(svc_info, wsa_action, ws_env_svr, operation_name TSRMLS_CC);
            /* keep track of operations with actions */
            axutil_hash_set (svc_info->ops_to_actions, axutil_strdup (ws_env_svr, operation_name),
            AXIS2_HASH_KEY_STRING, axutil_strdup (ws_env_svr, wsa_action));
        }
        zend_hash_move_forward_ex (ht_actions, &pos);
	}
}


void wsf_util_process_ws_service_operations_for_classes(
        HashTable *ht_ops_to_funcs,
        HashTable *ht_ops_to_mep,
        wsf_svc_info_t *svc_info,
    	char *classname,
        axutil_env_t *ws_env_svr TSRMLS_DC)
{
    HashPosition pos;
    zval ** tmp = NULL;
    int i = 0;
      
    if (!ht_ops_to_funcs || !classname) 
	{
        return;
    }        
    zend_hash_internal_pointer_reset_ex (ht_ops_to_funcs, &pos);
    while (zend_hash_get_current_data_ex (ht_ops_to_funcs, (void **) &tmp, &pos) != FAILURE) 
	{

        char *op_name_to_store = NULL;
        char *op_name = NULL;
        unsigned int op_name_len = 0;
        unsigned long index = i;
        char *func_name = NULL;
        zend_hash_get_current_key_ex (ht_ops_to_funcs, &op_name, &op_name_len, &index, 0, &pos);
        zend_hash_move_forward_ex (ht_ops_to_funcs, &pos);
        i++;
        func_name = Z_STRVAL_PP (tmp);
        if (op_name)
		{
            op_name_to_store = axutil_strdup (ws_env_svr, op_name);
        } 
		else 
		{
            op_name_to_store = axutil_strdup (ws_env_svr,func_name);
        }

        axutil_hash_set (svc_info->ops_to_functions,
             op_name_to_store, AXIS2_HASH_KEY_STRING, axutil_strdup (ws_env_svr, func_name));

		axutil_hash_set(svc_info->ops_to_classes , op_name_to_store , AXIS2_HASH_KEY_STRING, classname);

        wsf_util_create_op_and_add_to_svc (svc_info, ws_env_svr, op_name_to_store, ht_ops_to_mep TSRMLS_CC);
    }
}        

void wsf_util_process_ws_service_classes(
	HashTable *ht_classes,
	HashTable *ht_ops_to_mep,
	wsf_svc_info_t *svc_info,
	axutil_env_t *ws_env_svr TSRMLS_DC)
{
	zval **tmp;
	HashPosition pos;
	zend_class_entry **ce;

    zend_hash_internal_pointer_reset_ex (ht_classes, &pos);
	while(zend_hash_get_current_data_ex(ht_classes, (void**)&tmp, &pos) != FAILURE)
	{
		HashTable *ht_ops_to_functions = NULL;
		uint str_length = 0;
		zval **tmpval = NULL;
		
		
        if(Z_TYPE_PP(tmp) == IS_ARRAY)
		{
			/** data value is an array 	"classes"=>array("foo"=>array($ops, $actions,..)" */
			HashTable *values = NULL;
            char *classname = NULL;
            ulong num_index = 0;
            int class_exists = 0;

			values = Z_ARRVAL_PP(tmp);
            if (zend_hash_get_current_key_ex (values, &classname, &str_length, 
				&num_index, AXIS2_TRUE, &pos) != FAILURE)
			{
				AXIS2_LOG_DEBUG(ws_env_svr->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
					"classname -> %s", classname);
            }
			if(values)
			{
				if(zend_hash_find(values, WSF_OPERATIONS, sizeof(WSF_OPERATIONS), (void**)&tmpval) == SUCCESS
					&& Z_TYPE_PP(tmpval) == IS_ARRAY){
						ht_ops_to_functions = Z_ARRVAL_PP(tmpval);
				}
				if(zend_hash_find(values, WSF_CONS_ARGS, sizeof(WSF_CONS_ARGS), (void**)&tmpval) == SUCCESS
					&& Z_TYPE_PP(tmpval) == IS_ARRAY){
						if(!svc_info->class_args){
							MAKE_STD_ZVAL(svc_info->class_args);
							array_init(svc_info->class_args);
						}
						zval_add_ref(tmpval);
						add_assoc_zval(svc_info->class_args, classname, *tmpval);
				}
			}
			class_exists = zend_lookup_class(classname, str_length, &ce TSRMLS_CC);
			if(class_exists)
			{
				if(!svc_info->ops_to_classes)
				{
					svc_info->ops_to_classes = axutil_hash_make(ws_env_svr);
				}
				wsf_util_process_ws_service_operations_for_classes(ht_ops_to_functions,
					ht_ops_to_mep, svc_info, classname, ws_env_svr TSRMLS_CC);
			}
		}
		zend_hash_move_forward_ex (ht_classes, &pos);
	}
}

void wsf_util_process_rest_params(axutil_env_t *env, 
	wsf_svc_info_t *svc_info,
	HashTable *ht_rest_map TSRMLS_DC)
{
	zval **tmp;
	HashPosition pos;
	if(!ht_rest_map)
		return;

    zend_hash_internal_pointer_reset_ex (ht_rest_map, &pos);
	while(zend_hash_get_current_data_ex(ht_rest_map, (void**)&tmp, &pos) != FAILURE){
		zval **tmpval;		
        if(Z_TYPE_PP(tmp) == IS_ARRAY)
		{
			/** data value is an array 	"<Operation>"=>array("HTTPMethod"=>"<OP>","restLocation"=>"<loc>" */
			HashTable *values = NULL;
            char *operation_name = NULL;
			uint str_length = 0;
            ulong num_index = 0;
			char *http_method = NULL;
			char *rest_location = NULL;

			values = Z_ARRVAL_PP(tmp);
			if (zend_hash_get_current_key_ex (values, &operation_name, &str_length, 
				&num_index, AXIS2_TRUE, &pos) != FAILURE)
			{
				AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
					"Processing Rest Params opname-> %s", operation_name);
            }
			if(values)
			{
				if(zend_hash_find(values, WSF_HTTP_METHOD, sizeof(WSF_HTTP_METHOD), (void**)&tmpval) == SUCCESS
					&& Z_TYPE_PP(tmpval) == IS_STRING)
				{
						http_method = Z_STRVAL_PP(tmpval);
				}
				if(zend_hash_find(values, WSF_REST_LOCATION, sizeof(WSF_REST_LOCATION), (void**)&tmpval) == SUCCESS
					&& Z_TYPE_PP(tmpval) == IS_STRING)
				{
						rest_location = Z_STRVAL_PP(tmpval);
				}
			}
			if(operation_name && http_method && rest_location)
			{
				axis2_op_t *op = NULL;
				op = axis2_svc_get_op_with_name(svc_info->svc, env, operation_name);
				if(op)
				{
					axis2_op_set_rest_http_method(op, env, http_method);
					axis2_op_set_rest_http_location(op, env, rest_location);
					axis2_svc_add_rest_mapping(svc_info->svc, env, http_method, rest_location, op);
				}
			}
		}
		zend_hash_move_forward_ex (ht_rest_map, &pos);
	}
}

axis2_endpoint_ref_t *
wsf_util_set_values_to_endpoint_ref(
	const axutil_env_t *env,
	HashTable *ht TSRMLS_DC)
{
	axis2_endpoint_ref_t *endpoint_ref = NULL;
	zval **tmp = NULL;
	
	if(!ht)
	{
		return NULL;
	}
	if(zend_hash_find(ht, WSF_WSA_ADDRESS, sizeof(WSF_WSA_ADDRESS), (void**)&tmp) == SUCCESS
		&& Z_TYPE_PP(tmp) == IS_STRING)
	{
		endpoint_ref = axis2_endpoint_ref_create(env, Z_STRVAL_PP(tmp));
	}else
	{
		return NULL;
	}
	if(zend_hash_find(ht, WSF_WSA_REFERENCE_PARAMETERS, sizeof(WSF_WSA_REFERENCE_PARAMETERS), (void **)&tmp)
		== SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY)
	{
		HashTable *ref_params = Z_ARRVAL_PP(tmp);
		HashPosition pos;
		zval **data = NULL;
		zend_hash_internal_pointer_reset_ex (ref_params, &pos);
		while(zend_hash_get_current_data_ex(ref_params, (void**)&data, &pos) != FAILURE)
		{
			if(Z_TYPE_PP(data) == IS_STRING)
			{
				axiom_node_t *ref_param_node = NULL;
				axis2_char_t *xml_str = Z_STRVAL_PP(data);
				ref_param_node = wsf_util_deserialize_buffer(env, xml_str);
				axis2_endpoint_ref_add_ref_param(endpoint_ref, env, ref_param_node);
			}
			zend_hash_move_forward_ex (ref_params, &pos);
		}
	}
	if(zend_hash_find(ht, WSF_WSA_METADATA, sizeof(WSF_WSA_METADATA), (void **)&tmp)
		== SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY)
	{
		HashTable *meta_params = Z_ARRVAL_PP(tmp);
		HashPosition pos;
		zval **data = NULL;
		zend_hash_internal_pointer_reset_ex (meta_params, &pos);
		while(zend_hash_get_current_data_ex(meta_params, (void**)&data, &pos) != FAILURE)
		{
			if(Z_TYPE_PP(data) == IS_STRING)
			{
				axiom_node_t *metadata_node = NULL;
				axis2_char_t *xml_str = Z_STRVAL_PP(data);
				metadata_node = wsf_util_deserialize_buffer(env, xml_str);
				axis2_endpoint_ref_add_metadata(endpoint_ref, env, metadata_node);
			}
			zend_hash_move_forward_ex (meta_params, &pos);
		}
	}
	return endpoint_ref;
}

/**
* obtains the real path from a relative path
*/
char *
wsf_util_get_real_path(
   const axutil_env_t *env,
   char *path TSRMLS_DC)
{
	char resolved_path_buff[MAXPATHLEN];

	if (VCWD_REALPATH(path, resolved_path_buff)) 
	{
#if PHP_VERSION_ID < 50399
		if (PG(safe_mode) && (!php_checkuid(resolved_path_buff, NULL, CHECKUID_CHECK_FILE_AND_DIR))) 
		{
			return NULL;
		}
#endif

		if (php_check_open_basedir(resolved_path_buff TSRMLS_CC)) 
		{
			return NULL;
		}

#ifdef ZTS
	if (VCWD_ACCESS(resolved_path_buff, F_OK))
	{
		return NULL;
	}
#endif
		return axutil_strdup(env, resolved_path_buff);
	} else 
	{
		return NULL;
	}
}

