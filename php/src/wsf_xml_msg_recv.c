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

#include <axis2_msg_recv.h>
#include "wsf_xml_msg_recv.h"
#include <string.h>
#include <axiom.h>
#include <axiom_soap.h>
#include <axutil_error.h>
#include <axutil_env.h>
#include <php.h>
#include <string.h>
#include <axutil_string.h>
#include "wsf_util.h"
#include <php.h>
#include <string.h>
#include "wsf.h"
#include <php_main.h>
#include "wsf_wsdl.h"

axis2_status_t AXIS2_CALL wsf_xml_msg_recv_invoke_business_logic_sync(
        axis2_msg_recv_t * msg_recv,
        const axutil_env_t * env,
        axis2_msg_ctx_t * in_msg_ctx,
        axis2_msg_ctx_t * out_msg_ctx);

/************************* Private Functions **********************************/

static zval*
wsf_xml_msg_recv_process_incomming_headers(
        axiom_soap_envelope_t *envelope,
        const axutil_env_t *env TSRMLS_DC);

static axis2_char_t *wsf_xml_msg_recv_get_method_name(
        axis2_msg_ctx_t * msg_ctx,
        const axutil_env_t * env);

static axiom_node_t *wsf_xml_msg_recv_invoke_wsmsg(
        const axutil_env_t * env,
        axis2_char_t * op_name,
        axis2_msg_ctx_t * out_msg_ctx,
        axis2_msg_ctx_t * in_msg_ctx,
        wsf_svc_info_t *svc_info,
        axis2_char_t *classname,
        axis2_char_t *content_type TSRMLS_DC);


static axiom_node_t *wsf_xml_msg_recv_invoke_mixed(
        const axutil_env_t * env,
        wsf_svc_info_t * svc_info,
        axis2_msg_ctx_t * in_msg_ctx,
        axis2_msg_ctx_t * out_msg_ctx,
        axis2_char_t * op_name,
        axis2_char_t * class_name,
        zval *** p_output_headers TSRMLS_DC);

static void wsf_xml_msg_recv_set_soap_fault(
        const axutil_env_t * env,
        axis2_msg_ctx_t *in_msg_ctx,
        axis2_msg_ctx_t * out_msg_ctx,
        zval *zval_soap_fault TSRMLS_DC);

/************************** End of function prototypes ************************/

WSF_PHP_API axis2_msg_recv_t *WSF_CALL
wsf_xml_msg_recv_create(
        const axutil_env_t * env)
{

    axis2_msg_recv_t *msg_recv = NULL;
    axis2_status_t status = AXIS2_FAILURE;

    AXIS2_ENV_CHECK(env, NULL);
    msg_recv = axis2_msg_recv_create(env);

    if (NULL == msg_recv)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "xml msg recv null");
        return NULL;
    }
    status = axis2_msg_recv_set_scope(msg_recv, env, AXIS2_APPLICATION_SCOPE);

    if (AXIS2_TRUE != status)
    {
        axis2_msg_recv_free(msg_recv, env);
        return NULL;
    }

    axis2_msg_recv_set_invoke_business_logic(msg_recv, env,
            wsf_xml_msg_recv_invoke_business_logic_sync);

    return msg_recv;
}

/***************************Function implementation****************************/

axis2_status_t AXIS2_CALL
wsf_xml_msg_recv_invoke_business_logic_sync(
        axis2_msg_recv_t * msg_recv,
        const axutil_env_t * env,
        axis2_msg_ctx_t * in_msg_ctx,
        axis2_msg_ctx_t * out_msg_ctx)
{

    axis2_op_ctx_t *op_ctx = NULL;
    axis2_op_t *op_desc = NULL;

    axiom_namespace_t *env_ns = NULL;

    int soap_version = AXIOM_SOAP12;
    axis2_status_t status = AXIS2_SUCCESS;
    axis2_bool_t skel_invoked = AXIS2_FALSE;

    const axis2_char_t *style = NULL;
    axis2_char_t *local_name = NULL;
    axis2_char_t *soap_ns = AXIOM_SOAP12_SOAP_ENVELOPE_NAMESPACE_URI;
    axis2_char_t *operation_name = NULL;
    char *classname = NULL;

    axutil_property_t *svc_info_prop = NULL;
    axutil_property_t *req_info_prop = NULL;
    wsf_svc_info_t *svc_info = NULL;
    wsf_request_info_t *req_info = NULL;

    /** store in_msg_ctx envelope */
    axiom_soap_envelope_t *envelope = NULL;
    axiom_soap_body_t *body = NULL;
    axiom_node_t *in_body_node = NULL;

    /* store out_msg_ctx envelope */
    axiom_soap_envelope_t *default_envelope = NULL;
    axiom_soap_body_t *out_body = NULL;
    axiom_soap_header_t *out_header = NULL;
    axiom_soap_fault_t *out_soap_fault = NULL;

    axiom_node_t *result_node = NULL;

    axiom_node_t *out_body_content_node = NULL;
    axiom_element_t *out_body_content_element = NULL;
    axiom_node_t *out_node = NULL;

    zval **output_headers_zval = NULL;

    TSRMLS_FETCH();

    AXIS2_PARAM_CHECK(env->error, in_msg_ctx, AXIS2_FAILURE);
    AXIS2_PARAM_CHECK(env->error, out_msg_ctx, AXIS2_FAILURE);

    op_ctx = axis2_msg_ctx_get_op_ctx(in_msg_ctx, env);
    op_desc = axis2_op_ctx_get_op(op_ctx, env);
    style = axis2_op_get_style(op_desc, env);

    envelope = axis2_msg_ctx_get_soap_envelope(in_msg_ctx, env);

    body = axiom_soap_envelope_get_body(envelope, env);

    in_body_node = axiom_soap_body_get_base_node(body, env);


    if (0 == axutil_strcmp(AXIS2_STYLE_DOC, style))
    {
        local_name = wsf_xml_msg_recv_get_method_name(in_msg_ctx, env);
        if (!local_name)
        {
            return AXIS2_FAILURE;
        }
    } else if (0 == axutil_strcmp(AXIS2_STYLE_RPC, style))
    {

        axiom_node_t *op_node = NULL;
        axiom_element_t *op_element = NULL;
        op_node = axiom_node_get_first_child(in_body_node, env);
        if (!op_node)
        {
            return AXIS2_FAILURE;
        }
        op_element = axiom_node_get_data_element(op_node, env);

        if (!op_element)
        {
            return AXIS2_FAILURE;
        }
        local_name = axiom_element_get_localname(op_element, env);
        if (!local_name)
        {
            return AXIS2_FAILURE;
        }
    }

    /** set soap version and soap namespace to local variables */
    if (in_msg_ctx && axis2_msg_ctx_get_is_soap_11(in_msg_ctx, env))
    {
        soap_ns = AXIOM_SOAP11_SOAP_ENVELOPE_NAMESPACE_URI; /* default is 1.2 */
        soap_version = AXIOM_SOAP11;
    }

    svc_info_prop = axis2_msg_ctx_get_property(in_msg_ctx, env, WSF_SVC_INFO);
    if (svc_info_prop)
    {
        svc_info = (wsf_svc_info_t *) axutil_property_get_value(svc_info_prop, env);
        if (svc_info)
        {
            operation_name = axutil_hash_get(svc_info->ops_to_functions, local_name,
                    AXIS2_HASH_KEY_STRING);
            if (!operation_name)
            {
                return AXIS2_FAILURE;
            }
        } else
        {
            return AXIS2_FAILURE;
        }
        if (svc_info->ops_to_classes)
        {
            classname = axutil_hash_get(svc_info->ops_to_classes, local_name, AXIS2_HASH_KEY_STRING);
        }
    }

    req_info_prop = axis2_msg_ctx_get_property(in_msg_ctx, env, WSF_REQ_INFO);
    if (req_info_prop)
    {
        req_info = (wsf_request_info_t *) axutil_property_get_value(req_info_prop, env);

        if (axis2_msg_ctx_get_doing_rest(in_msg_ctx, env))
        {
            axis2_op_t *op = NULL;
            axiom_node_t *body_child_node = NULL;
            axiom_element_t *body_child = NULL;
            int i = 0;

            body_child_node = axiom_node_get_first_child(in_body_node, env);

            if (!body_child_node)
            {
                op = axis2_msg_ctx_get_op(in_msg_ctx, env);
                if (op)
                {
                    body_child = axiom_element_create_with_qname(env, NULL,
                            axis2_op_get_qname(op, env), &body_child_node);
                    axiom_soap_body_add_child(body, env, body_child_node);
                }
            }
            if (req_info->param_keys && req_info->param_values)
            {
                int i = 0;
                for (i = 0; i < axutil_array_list_size(req_info->param_keys, env); i++)
                {
                    axiom_node_t *node = NULL;
                    axiom_element_t *element = NULL;

                    axis2_char_t *param_key = NULL;
                    axis2_char_t *param_value = NULL;

                    param_key = axutil_array_list_get(req_info->param_keys, env, i);
                    param_value = axutil_array_list_get(req_info->param_values, env, i);

                    element = axiom_element_create(env, NULL, param_key,
                            NULL, &node);
                    axiom_element_set_text(element, env, param_value, node);
                    axiom_node_add_child(body_child_node, env, node);

                    AXIS2_FREE(env->allocator, param_key);
                    AXIS2_FREE(env->allocator, param_value);
                }
                axutil_array_list_free(req_info->param_keys, env);
                axutil_array_list_free(req_info->param_values, env);
            }
        }
    }

    if (svc_info->ht_op_params)
    {
        zval **tmp;
        char *function_type = NULL;
        if (zend_hash_find(svc_info->ht_op_params,
                operation_name, strlen(operation_name) + 1,
                (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING)
        {
            function_type = Z_STRVAL_PP(tmp);

            if (strcmp(function_type, WSF_MIXED) == 0)
            {
                result_node = wsf_xml_msg_recv_invoke_mixed(env, svc_info,
                        in_msg_ctx, out_msg_ctx, operation_name,
                        classname, &output_headers_zval TSRMLS_CC);

            } else if (strcmp(function_type, WSF_WSMESSAGE) == 0)
            {
                result_node = wsf_xml_msg_recv_invoke_wsmsg(env, operation_name,
                        in_msg_ctx, out_msg_ctx, svc_info, classname, req_info->content_type TSRMLS_CC);
            }
        }
    } else
    {
        /* this is where the default value for opParam is set,
           If the wsdl option is set go for the MIXED mode by default */
		if (svc_info->wsdl == NULL || svc_info->omit_wsdl)
        {
            result_node = wsf_xml_msg_recv_invoke_wsmsg(env, operation_name,
                    in_msg_ctx, out_msg_ctx, svc_info, classname, req_info->content_type TSRMLS_CC);
        } else
        {
            result_node = wsf_xml_msg_recv_invoke_mixed(env, svc_info,
                    in_msg_ctx, out_msg_ctx, operation_name,
                    classname, &output_headers_zval TSRMLS_CC);
        }
    }
    if (!result_node)
    {
        status = AXIS2_ERROR_GET_STATUS_CODE(env->error);
    } else
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Response node is not null");
    }

    if (result_node)
    {
        if (0 == axutil_strcmp(style, AXIS2_STYLE_RPC))
        {
            axiom_namespace_t *ns = NULL;
            axis2_char_t *response_name = NULL;

            response_name = axutil_stracat(env, local_name, "Response");
            ns = axiom_namespace_create(env, "http://soapenc/", "res");
            if (!ns)
            {
                return AXIS2_FAILURE;
            }

            out_body_content_element = axiom_element_create(env, NULL, response_name,
                    ns, &out_body_content_node);
            axiom_node_add_child(out_body_content_node, env, result_node);

        } else
        {
            out_body_content_node = result_node;
        }
    }

    if (axis2_msg_ctx_get_soap_envelope(out_msg_ctx, env))
    {
        /* service implementation has set the envelope, useful when setting a SOAP fault.
           No need to further process */
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "soap fault is set");
        return AXIS2_SUCCESS;
    }

    /* create the soap envelope here */
    env_ns = axiom_namespace_create(env, soap_ns, AXIOM_SOAP_DEFAULT_NAMESPACE_PREFIX);
    if (!env_ns)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsfphp] error seting the namespces for the "
                AXIOM_SOAP_DEFAULT_NAMESPACE_PREFIX);
        return AXIS2_FAILURE;
    }

    default_envelope = axiom_soap_envelope_create(env, env_ns);

    if (!default_envelope)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "failed in creating the response soap envelope");
        return AXIS2_FAILURE;
    }


    out_body = axiom_soap_body_create_with_parent(env, default_envelope);
    if (!out_body)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsfphp] failed in creating the response soap body");
        return AXIS2_FAILURE;
    }

    out_header = axiom_soap_header_create_with_parent(env, default_envelope);
    if (!out_header)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "failed in creating the response soap headers");
        return AXIS2_FAILURE;
    }

    if (output_headers_zval)
    {
        axiom_node_t *header_base_node = NULL;

        HashPosition pos;
        zval **param;
        char *header_str;
        axiom_node_t *header_node;

        for (zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(output_headers_zval), &pos);
                zend_hash_get_current_data_ex(Z_ARRVAL_PP(output_headers_zval),
                (void **) & param, &pos) == SUCCESS;
                zend_hash_move_forward_ex(Z_ARRVAL_PP(output_headers_zval), &pos))
        {

            if (Z_TYPE_PP(param) == IS_STRING)
            {

                header_base_node = axiom_soap_header_get_base_node(out_header, env);

                if (header_base_node)
                {
                    header_str = Z_STRVAL_PP(param);
                    header_node = wsf_util_deserialize_buffer(env, header_str);
                    axiom_node_add_child(header_base_node, env, header_node);
                } else
                {
                    AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsfphp] failed in retrieving the response soap headers node");
                    return AXIS2_FAILURE;
                }
            }
        }
    }

    out_node = axiom_soap_body_get_base_node(out_body, env);
    if (!out_node)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsfphp] failed in retrieving the response soap body node");
        return AXIS2_FAILURE;
    }

    if (status != AXIS2_SUCCESS)
    {
        /* something went wrong, set a SOAP Fault */
        axis2_char_t *fault_value_str =
                AXIOM_SOAP_DEFAULT_NAMESPACE_PREFIX ":" AXIOM_SOAP12_SOAP_FAULT_VALUE_SENDER;

        axis2_char_t *fault_reason_str = NULL;
        axis2_char_t *err_msg = NULL;

        if (!skel_invoked)
            fault_value_str = AXIOM_SOAP_DEFAULT_NAMESPACE_PREFIX ":" AXIOM_SOAP12_SOAP_FAULT_VALUE_RECEIVER;
        ;

        err_msg = (char *) AXIS2_ERROR_GET_MESSAGE(env->error);

        if (err_msg)
        {
            fault_reason_str = err_msg;
        } else
        {
            fault_reason_str = "Error occurred while processing SOAP message";
        }

        out_soap_fault = axiom_soap_fault_create_default_fault(env, out_body,
                fault_value_str, fault_reason_str, soap_version);
    }

    if (out_body_content_node)
    {
        axiom_node_add_child(out_node, env, out_body_content_node);
        status = axis2_msg_ctx_set_soap_envelope(out_msg_ctx, env,
                default_envelope);
    } else if (out_soap_fault)
    {
        axis2_msg_ctx_set_soap_envelope(out_msg_ctx, env, default_envelope);
        status = AXIS2_FAILURE; /* if there is a failure we have to return a failure code */
    } else
    {
        /* we should free the memory as the envelope is not used, one way case */
        axiom_soap_envelope_free(default_envelope, env);
        default_envelope = NULL;
    }
    return AXIS2_SUCCESS;
}

/**
 * Private function implementations
 */
static axis2_char_t *
wsf_xml_msg_recv_get_method_name(
        axis2_msg_ctx_t * msg_ctx,
        const axutil_env_t * env)
{
    axis2_op_ctx_t *op_ctx = NULL;
    axis2_op_t *op = NULL;
    const axutil_qname_t *qname = NULL;
    axis2_char_t *name = NULL;

    op_ctx = axis2_msg_ctx_get_op_ctx(msg_ctx, env);

    if (!op_ctx)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsfphp] Operation Context is not found");
        return NULL;
    }

    op = axis2_op_ctx_get_op(op_ctx, env);

    if (!op)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsfphp] Operation is not found");
        return NULL;
    }

    qname = axis2_op_get_qname(op, env);

    if (!qname)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsfphp] Operation QName is not found");
        return NULL;
    }

    name = axutil_qname_get_localpart(qname, env);

    return name;
}

static axiom_node_t *
wsf_xml_msg_recv_invoke_mixed(
        const axutil_env_t * env,
        wsf_svc_info_t * svc_info,
        axis2_msg_ctx_t * in_msg_ctx,
        axis2_msg_ctx_t * out_msg_ctx,
        axis2_char_t * function_name,
        axis2_char_t * class_name,
        zval ***p_output_headers TSRMLS_DC)
{
    axiom_soap_envelope_t *soap_envelope = NULL;
    axiom_soap_header_t *req_soap_header = NULL;
    axiom_soap_body_t *soap_body = NULL;
    axiom_node_t *req_soap_header_node = NULL;
    axiom_node_t *soap_body_node = NULL;
    axiom_node_t *payload_node = NULL;
    char *in_msg_body_string = NULL;
    char *header_string = NULL;
    char *operation_name = NULL;
    axutil_hash_index_t * hi = NULL;

    zval * params[1];
    zval request_function, retval, param1;
    zval *param_array;
    char *res_payload_str = NULL;
    axiom_node_t *res_om_node = NULL;

    zval *class_args;
    zval class_args_val;

    zval *cid2str = NULL;
    zval *cid2contentType = NULL;

    int use_mtom = AXIS2_TRUE;
    int enable_swa = AXIS2_FALSE;

    zval **output_headers = NULL;

	zval *wsdata_obj;
	zend_class_entry **ce;

    if (!in_msg_ctx || !function_name)
    {
        return NULL;
    }

    /* load the sig model form cache if exist */
    if (svc_info->wsdl)
    {
        wsf_wsdl_cache_wsdl_info(svc_info, env TSRMLS_CC);
    }

    use_mtom = svc_info->use_mtom;
    enable_swa = svc_info->enable_swa;

    soap_envelope = axis2_msg_ctx_get_soap_envelope(in_msg_ctx, env);
    if (!soap_envelope)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsf_wsdl] soap envelope not found");
        return NULL;
    }

    soap_body = axiom_soap_envelope_get_body(soap_envelope, env);
    if (!soap_body)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsf_wsdl] soap body not found");
        return NULL;
    }
    soap_body_node = axiom_soap_body_get_base_node(soap_body, env);
    if (!soap_body_node)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsf_wsdl] soap body base node not found");
        return NULL;
    }
    payload_node = axiom_node_get_first_child(soap_body_node, env);
    while (payload_node && axiom_node_get_node_type(payload_node, env) != AXIOM_ELEMENT)
    {
        payload_node = axiom_node_get_next_sibling(payload_node, env);
    }
    if (!payload_node)
    {
        /* return NULL;*/
    }

    req_soap_header = axiom_soap_envelope_get_header(soap_envelope, env);
    /* this is not an mandatory */
    if (req_soap_header)
    {
        axiom_node_t * envelope_node = NULL;
        axis2_char_t *envelope_string = NULL;
        axiom_node_t *envelope_dup = NULL;

        envelope_node = axiom_soap_envelope_get_base_node(soap_envelope, env);
        envelope_string = axiom_node_to_string(envelope_node, env);
        envelope_dup = wsf_util_deserialize_buffer(env, envelope_string);
        req_soap_header_node = axiom_node_get_first_child(envelope_node, env);

        while (req_soap_header_node && axiom_node_get_node_type(req_soap_header_node, env) != AXIOM_ELEMENT)
        {
            req_soap_header_node = axiom_node_get_next_sibling(req_soap_header_node, env);
        }

        /* req_soap_header_node = axiom_soap_header_get_base_node(req_soap_header, env); */

        if (req_soap_header_node && axiom_node_get_first_child(req_soap_header_node, env) != NULL)
        {
            header_string = axiom_node_sub_tree_to_string(req_soap_header_node, env);
        }

    }
    if (!header_string)
    {
        header_string = axutil_strdup(env, "");
    }


    MAKE_STD_ZVAL(cid2str);
    MAKE_STD_ZVAL(cid2contentType);

    array_init(cid2str);
    array_init(cid2contentType);

    /*
     * call wsf_util_get_attachments_form_soap_envelope(env, soap_envelope, cid2str, cid2contentType TSRMLS_CC);
     */
    wsf_util_get_attachments_from_soap_envelope(env, soap_envelope, cid2str, cid2contentType TSRMLS_CC);

    in_msg_body_string = wsf_util_serialize_om(env, payload_node);
    if (in_msg_body_string)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[wsf_wsdl]Input soap body is \t %s \n", in_msg_body_string);
    }

    if (class_name && svc_info->class_args)
    {
        zval **tmp;
        if (zend_hash_find(Z_ARRVAL_P(svc_info->class_args), class_name,
                strlen(class_name) + 1, (void**) & tmp) == SUCCESS)
        {
            class_args = *tmp;
            /* zval_add_ref(class_arg); */
        } else
        {
            class_args = &class_args_val;
            ZVAL_NULL(class_args);
            INIT_PZVAL(class_args);
        }
    } else
    {
        class_args = &class_args_val;
        ZVAL_NULL(class_args);
        INIT_PZVAL(class_args);
    }
    if (svc_info->sig_model_string)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
                "[wsf_wsdl]sig model string found is \t \n %s \n", svc_info->sig_model_string);
    } else
    {
        zval ** server_vars, **data;
        char *server_name = NULL;
        char *port = NULL;
        smart_str req_uri = {0};

        if (SG(request_info).request_uri)
        {
            if (zend_hash_find(&EG(symbol_table), "_SERVER", sizeof ("_SERVER"),
                    (void **) & server_vars) == SUCCESS && (Z_TYPE_PP(server_vars) == IS_ARRAY))
            {
                if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "SERVER_NAME",
                        sizeof ("SERVER_NAME"), (void **) & data) == SUCCESS) &&
                        Z_TYPE_PP(data) == IS_STRING)
                {
                    server_name = Z_STRVAL_PP(data);
                }
                if ((zend_hash_find(Z_ARRVAL_PP(server_vars), "SERVER_PORT",
                        sizeof ("SERVER_PORT"), (void **) & data) == SUCCESS) &&
                        Z_TYPE_PP(data) == IS_STRING)
                {
                    port = Z_STRVAL_PP(data);
                }
            }

            smart_str_appends(&req_uri, "http://");
            smart_str_appends(&req_uri, server_name);
            smart_str_appends(&req_uri, ":");
            if (port != NULL)
            {
                smart_str_appends(&req_uri, port);
            }
            smart_str_appends(&req_uri, SG(request_info).request_uri);
            smart_str_appends(&req_uri, "?wsdl");
            smart_str_0(&req_uri);
            wsf_wsdl_set_sig_model(axutil_strdup(env, req_uri.c), svc_info, env TSRMLS_CC);
            smart_str_free(&req_uri);
        }
    }

    /* find the operation name */
    if (svc_info->ops_to_functions)
    {
        for (hi = axutil_hash_first(svc_info->ops_to_functions, env); hi;
                hi = axutil_hash_next(env, hi))
        {
            void *v = NULL;
            const void *k = NULL;
            axutil_hash_this(hi, &k, NULL, &v);
            if (strcmp(function_name, (axis2_char_t *) v) == 0)
            {
                operation_name = (axis2_char_t *) k;
                break;
            }
        }
    }
	

    MAKE_STD_ZVAL(param_array);
    array_init(param_array);

    add_assoc_string(param_array, "sig_model_string", svc_info->sig_model_string, 1);
    add_assoc_string(param_array, "payload_string", in_msg_body_string, 1);
    add_assoc_string(param_array, "operation_name", operation_name, 1);
    add_assoc_string(param_array, "function_name", function_name, 1);
    add_assoc_bool(param_array, "useMTOM", use_mtom);
    add_assoc_string(param_array, "header_string", header_string, 1);


    if (class_name)
    {
        add_assoc_string(param_array, "class_name", class_name, 1);
    }

    if (svc_info->class_map)
    {
        add_assoc_zval(param_array, WSF_WSDL_CLASSMAP, svc_info->class_map);
    }

    if (!svc_info->generated_svc_name)
    {
        add_assoc_string(param_array, WSF_SERVICE_NAME, svc_info->svc_name, 1);
    }
    if (svc_info->port_name)
    {
        add_assoc_string(param_array, WSF_PORT_NAME, svc_info->port_name, 1);
    }

    add_assoc_zval(param_array, WSF_ATTACHMENTS, cid2str);
    add_assoc_zval(param_array, WSF_CID2CONTENT_TYPE, cid2contentType);

    add_assoc_zval(param_array, "class_args", class_args);

    ZVAL_STRING(&request_function, WSF_WSDL_PROCESS_IN_MSG, 0);
    
	MAKE_STD_ZVAL(wsdata_obj);
	zend_lookup_class("WSData", strlen("WSData"), &ce TSRMLS_CC);
	object_init_ex(wsdata_obj, *ce);
    
	add_property_zval(wsdata_obj, "WSDL_ParamArray", param_array);
	
	params[0] = &param1;

	ZVAL_ZVAL(params[0], wsdata_obj, NULL, NULL);
    INIT_PZVAL(params[0]);


    if (call_user_function(EG(function_table), (zval **) NULL,
            &request_function, &retval, 1, params TSRMLS_CC) == SUCCESS)
    {
        if (Z_TYPE_P(&retval) == IS_ARRAY)
        {

            HashTable *ht_return = NULL;
            zval **tmp = NULL;

            ht_return = Z_ARRVAL_P(&retval);

            if (zend_hash_find(ht_return, WSF_WSDL_RES_PAYLOAD,
                    sizeof (WSF_WSDL_RES_PAYLOAD),
                    (void **) & tmp) == SUCCESS &&
                    Z_TYPE_PP(tmp) == IS_STRING)
            {
                res_payload_str = Z_STRVAL_PP(tmp);
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
                        "[wsf_wsdl]return payload string is\n %s", res_payload_str);

                if (!res_payload_str)
                {
                    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
                            "[wsf_wsdl] response payload string not found");
                } else
                {

                    res_om_node = wsf_util_deserialize_buffer(env, res_payload_str);

                    if (!res_om_node)
                    {
                        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
                                "[wsf_wsdl] error deserializing the res payload");
                    }

                    if (zend_hash_find(ht_return, WSF_WSDL_ATTACHMENT_MAP,
                            sizeof (WSF_WSDL_ATTACHMENT_MAP),
                            (void**) & tmp) == SUCCESS &&
                            Z_TYPE_PP(tmp) == IS_ARRAY)
                    {
                        int enable_mtom = AXIS2_TRUE;
                        int enable_swa = AXIS2_FALSE;
                        HashTable *ht_attachments = Z_ARRVAL_PP(tmp);

                        wsf_util_set_attachments_with_cids(env, enable_mtom, enable_swa,
                                res_om_node, ht_attachments,
                                AXIOM_MIME_TYPE_OCTET_STREAM TSRMLS_CC);
                    }
                }
            }
            if (zend_hash_find(ht_return, WSF_WSDL_OUTPUT_HEADERS, sizeof (WSF_WSDL_OUTPUT_HEADERS),
                    (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_ARRAY)
            {
                output_headers = tmp;
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[wsf_wsdl] response headers found");
            }
        }

        if (EG(exception) && Z_TYPE_P(EG(exception)) == IS_OBJECT &&
                instanceof_function(Z_OBJCE_P(EG(exception)), ws_fault_class_entry TSRMLS_CC))
        {
            wsf_xml_msg_recv_set_soap_fault(env, in_msg_ctx, out_msg_ctx,
                    EG(exception) TSRMLS_CC);
            zend_clear_exception(TSRMLS_C);

        }
    }

    *p_output_headers = output_headers;

    return res_om_node;
}

static axiom_node_t *
wsf_xml_msg_recv_invoke_wsmsg(
        const axutil_env_t * env,
        axis2_char_t * op_name,
        axis2_msg_ctx_t *in_msg_ctx,
        axis2_msg_ctx_t * out_msg_ctx,
        wsf_svc_info_t *svc_info,
        axis2_char_t *class_name,
        axis2_char_t *content_type TSRMLS_DC)
{

    char *req_payload = NULL, *res_payload = NULL, *req_envelope;
    axiom_node_t *res_om_node = NULL;
    axiom_node_t *om_node = NULL;
    axiom_soap_envelope_t *soap_envelope = NULL;
	axiom_node_t *envelope_node = NULL;
    axiom_soap_body_t *soap_body = NULL;
    axiom_node_t *soap_body_node = NULL;

    int use_mtom = AXIS2_TRUE;
    int enable_swa = AXIS2_FALSE;
    int request_xop = AXIS2_FALSE;
    int _bailout = 0;

    zval *output_headers = NULL;
    zval func, retval, param;
    zval * params[1];
    HashTable *ft = NULL;
    zend_class_entry **ce = NULL;
    zval *msg = NULL;


    if (!in_msg_ctx)
        return NULL;

    soap_envelope = axis2_msg_ctx_get_soap_envelope(in_msg_ctx, env);
    if (!soap_envelope)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "soap envelope not found");
        return NULL;
    }
	envelope_node = axiom_soap_envelope_get_base_node(soap_envelope, env);

    soap_body = axiom_soap_envelope_get_body(soap_envelope, env);
    if (!soap_body)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "soap body not found");
        return NULL;
    }
    soap_body_node = axiom_soap_body_get_base_node(soap_body, env);
    if (!soap_body_node)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "soap body base node not found");
        return NULL;
    }

    om_node = axiom_node_get_first_child(soap_body_node, env);
    if (!om_node)
        return NULL;

    use_mtom = svc_info->use_mtom;
    enable_swa = svc_info->enable_swa;
    request_xop = svc_info->request_xop;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX " Calling php service");


    output_headers = wsf_xml_msg_recv_process_incomming_headers(soap_envelope, env TSRMLS_CC);
    zend_try
    {
        MAKE_STD_ZVAL(msg);
        object_init_ex(msg, ws_message_class_entry);

        if (output_headers)
        {
            add_property_zval(msg, WSF_OUTPUT_HEADERS, output_headers);
        }

        if (request_xop == AXIS2_TRUE)
        {

            zval *cid2str = NULL;
            zval *cid2contentType = NULL;

            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                "requestXOP is TRUE, setting attachments");

            MAKE_STD_ZVAL(cid2str);
            MAKE_STD_ZVAL(cid2contentType);

            array_init(cid2str);
            array_init(cid2contentType);

            /*
             * call wsf_util_get_attachments_form_soap_envelope(env, soap_envelope, cid2str, cid2contentType TSRMLS_CC);
             */
            wsf_util_get_attachments_from_soap_envelope(env, soap_envelope, cid2str, cid2contentType TSRMLS_CC);
            add_property_zval(msg, WSF_ATTACHMENTS, cid2str);
            add_property_zval(msg, WSF_CID2CONTENT_TYPE, cid2contentType);

        } else if (request_xop == AXIS2_FALSE)
        {
            wsf_util_find_xop_content_and_convert_to_base64(env, om_node);
        }

        /** this should be after mtom processing */
        req_payload = wsf_util_serialize_om(env, om_node);
        add_property_stringl(msg, WSF_MESSAGE_STR, req_payload, req_payload ? strlen(req_payload) : 0, 1);
		req_envelope = wsf_util_serialize_om(env, envelope_node);
		add_property_stringl(msg, WSF_MESSAGE_REQUEST_SOAP_ENVELOPE_STR, req_envelope, req_envelope ? strlen(req_envelope) : 0, 1);
       
        if (content_type)
        {
            add_property_string(msg, WSF_REST_CONTENT_TYPE, content_type, 1);
        }
        ZVAL_STRING(&func, op_name, 0);
        params[0] = &param;
        ZVAL_ZVAL(params[0], msg, NULL, NULL);
        INIT_PZVAL(params[0]);

        if (class_name)
        {
            zval **argv = NULL;
            zval **tmp = NULL;
            int argc = 0;

            zend_lookup_class(class_name, strlen(class_name), &ce TSRMLS_CC);
            if (class_name && svc_info->class_args)
            {
                if (zend_hash_find(Z_ARRVAL_P(svc_info->class_args), class_name,
                        strlen(class_name) + 1, (void**) & tmp) == SUCCESS)
                {
                    if (Z_TYPE_PP(tmp) == IS_ARRAY)
                    {
                        zval **tmp1 = NULL;
                        HashPosition pos;
                        int i = 0;
                        argc = zend_hash_num_elements(Z_ARRVAL_PP(tmp));
                        argv = safe_emalloc(sizeof (zval), argc, 0);

                        zend_hash_internal_pointer_reset_ex(Z_ARRVAL_PP(tmp), &pos);
                        while (zend_hash_get_current_data_ex(Z_ARRVAL_PP(tmp), (void**) & tmp1, &pos) != FAILURE)
                        {
                            argv[i] = *tmp1;
                            i++;
                            zend_hash_move_forward_ex(Z_ARRVAL_PP(tmp), &pos);
                        }
                    }
                }
            }
            if (*ce)
            {
                /* call constructor function */
                zval *tmp_cls_obj;
                MAKE_STD_ZVAL(tmp_cls_obj);
                object_init_ex(tmp_cls_obj, *ce);


                if (zend_hash_exists(&Z_OBJCE_P(tmp_cls_obj)->function_table,
                        ZEND_CONSTRUCTOR_FUNC_NAME, sizeof (ZEND_CONSTRUCTOR_FUNC_NAME)))
                {
                    zval c_ret, constructor;
                    INIT_ZVAL(c_ret);
                    INIT_ZVAL(constructor);

                    ZVAL_STRING(&constructor, ZEND_CONSTRUCTOR_FUNC_NAME, 1);
                    if (call_user_function(NULL, &tmp_cls_obj, &constructor,
                            &c_ret, argc, argv TSRMLS_CC) == FAILURE)
                    {
                        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Error Calling the constructor ");
                    }
                    zval_dtor(&constructor);
                    zval_dtor(&c_ret);
                } else if (zend_hash_exists(&Z_OBJCE_P(tmp_cls_obj)->function_table,
                        class_name, strlen(class_name)) == SUCCESS)
                {
                    zval c_ret, constructor;
                    INIT_ZVAL(c_ret);
                    INIT_ZVAL(constructor);

                    ZVAL_STRING(&constructor, class_name, 1);
                    if (call_user_function(NULL, &tmp_cls_obj, &constructor,
                            &c_ret, argc, argv TSRMLS_CC) == FAILURE)
                    {
                        php_error_docref(NULL TSRMLS_CC, E_ERROR, "Error Calling the constructor ");
                    }

                    zval_dtor(&constructor);
                    zval_dtor(&c_ret);
                }

                ft = &(*ce)->function_table;
                if (zend_hash_exists(ft, op_name, strlen(op_name)) == SUCCESS ||
                        zend_hash_exists(ft, WSF_MAGIC_FUNC_CALL, strlen(WSF_MAGIC_FUNC_CALL)) == SUCCESS)
                {
                    if (call_user_function(NULL, &tmp_cls_obj, &func, &retval, 1, params TSRMLS_CC) != SUCCESS)
                    {
                        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
							"call_user_function failed ");
                        return NULL;
                    }
                }
            }
        } else if (call_user_function(CG(function_table), (zval **) NULL, &func,
                &retval, 1, params TSRMLS_CC) != SUCCESS)
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "call_user_function failed ");
            return NULL;
        }

        if (EG(exception) && Z_TYPE_P(EG(exception)) == IS_OBJECT &&
                instanceof_function(Z_OBJCE_P(EG(exception)), ws_fault_class_entry TSRMLS_CC))
        {
            wsf_xml_msg_recv_set_soap_fault(env, in_msg_ctx, out_msg_ctx, EG(exception) TSRMLS_CC);

        }else if(Z_TYPE(retval) == IS_OBJECT && instanceof_function(Z_OBJCE(retval),
                ws_fault_class_entry TSRMLS_CC)){
        	wsf_xml_msg_recv_set_soap_fault(env, in_msg_ctx, out_msg_ctx, &retval TSRMLS_CC);

        }else if (Z_TYPE(retval) == IS_OBJECT && instanceof_function(Z_OBJCE(retval),
                ws_message_class_entry TSRMLS_CC))
        {
            zval **msg_tmp = NULL;
            axis2_char_t *default_cnt_type = NULL;

            if (zend_hash_find(Z_OBJPROP(retval), WSF_DEFAULT_ATTACHEMENT_CONTENT_TYPE,
                    sizeof (WSF_DEFAULT_ATTACHEMENT_CONTENT_TYPE), (void **) & msg_tmp) == SUCCESS
                    && Z_TYPE_PP(msg_tmp) == IS_STRING)
            {
                default_cnt_type = Z_STRVAL_PP(msg_tmp);
            } else
            {
                default_cnt_type = AXIOM_MIME_TYPE_OCTET_STREAM;
            }

            if (zend_hash_find(Z_OBJPROP(retval), WSF_ACTION, sizeof (WSF_ACTION),
                    (void **) & msg_tmp) == SUCCESS && Z_TYPE_PP(msg_tmp) == IS_STRING)
            {
                axis2_char_t *action = NULL;
                action = Z_STRVAL_PP(msg_tmp);
                axis2_msg_ctx_set_wsa_action(out_msg_ctx, env, action);
            }

            if (zend_hash_find(Z_OBJPROP(retval), WSF_MESSAGE_STR, sizeof (WSF_MESSAGE_STR),
                    (void **) & msg_tmp) == SUCCESS && Z_TYPE_PP(msg_tmp) == IS_STRING)
            {
                res_payload = Z_STRVAL_PP(msg_tmp);
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
						"response payload %s", res_payload);
                if (res_payload)
                {
                    res_om_node = wsf_util_deserialize_buffer(env, res_payload);
                }
            }
            if (zend_hash_find(Z_OBJPROP(retval), WSF_REST_CONTENT_TYPE,
                    sizeof (WSF_REST_CONTENT_TYPE), (void **) & msg_tmp) == SUCCESS &&
                    Z_TYPE_PP(msg_tmp) == IS_STRING)
            {
                axutil_property_t *cnt_property = NULL;
                cnt_property = axutil_property_create_with_args(env, AXIS2_SCOPE_REQUEST,
                        AXIS2_TRUE, NULL, axutil_strdup(env, Z_STRVAL_PP(msg_tmp)));
                axis2_msg_ctx_set_property(out_msg_ctx, env, WSF_REST_CONTENT_TYPE, cnt_property);
            }
            if (zend_hash_find(Z_OBJPROP(retval), WSF_HTTP_STATUS_CODE,
                    sizeof (WSF_HTTP_STATUS_CODE), (void **) & msg_tmp) == SUCCESS &&
                    Z_TYPE_PP(msg_tmp) == IS_LONG)
            {
                axis2_msg_ctx_set_status_code(out_msg_ctx, env, Z_LVAL_PP(msg_tmp));
            }

            if (zend_hash_find(Z_OBJPROP(retval), WSF_ATTACHMENTS, sizeof (WSF_ATTACHMENTS),
                    (void **) & msg_tmp) == SUCCESS && Z_TYPE_PP(msg_tmp) == IS_ARRAY)
            {
                HashTable *ht = NULL;
                ht = Z_ARRVAL_PP(msg_tmp);

                if (ht && res_om_node)
                {
                    wsf_util_set_attachments_with_cids(env, use_mtom, enable_swa,
                            res_om_node, ht, default_cnt_type TSRMLS_CC);
                    if (use_mtom == 1)
                    {
                        axis2_msg_ctx_set_doing_mtom(out_msg_ctx, env, AXIS2_TRUE);
                    }
                }
            }
        } else if (Z_TYPE(retval) == IS_STRING)
        {
            res_payload = Z_STRVAL(retval);
            if (res_payload)
            {
                res_om_node = wsf_util_deserialize_buffer(env, res_payload);
            }
        }
        zval_ptr_dtor(&msg);
    }

    zend_catch
    {

        if (EG(exception) && Z_TYPE_P(EG(exception)) == IS_OBJECT &&
                instanceof_function(Z_OBJCE_P(EG(exception)), ws_fault_class_entry TSRMLS_CC))
        {
        	wsf_xml_msg_recv_set_soap_fault(env, in_msg_ctx, out_msg_ctx, EG(exception) TSRMLS_CC);
            zend_clear_exception(TSRMLS_C);
        } else
        {
            _bailout = 1;
        }

    }
    zend_end_try();

    if (_bailout)
    {
        zend_bailout();
    }
    if (req_payload)
    {
        AXIS2_FREE(env->allocator, req_payload);
    }

    if (!res_payload)
    {
        if (axis2_msg_ctx_get_doing_rest(in_msg_ctx, env))
        {
            axis2_msg_ctx_set_no_content(in_msg_ctx, env, AXIS2_TRUE);
        }

        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Response Payload is NULL");
        return NULL;
    }
    return res_om_node;
}

static void
wsf_xml_msg_recv_set_soap_fault(
        const axutil_env_t * env,
        axis2_msg_ctx_t * in_msg_ctx,
        axis2_msg_ctx_t * out_msg_ctx,
        zval *zval_soap_fault TSRMLS_DC)
{

    axiom_soap_envelope_t *out_envelope = NULL;
    axiom_soap_header_t *out_header = NULL;
    axiom_soap_body_t *out_body = NULL;
    axiom_soap_fault_t *out_fault = NULL;
    axiom_soap_fault_detail_t *fault_detail = NULL;
    axiom_soap_fault_role_t *fault_role = NULL;
    axiom_node_t *detail_node = NULL;
    axiom_namespace_t *env_ns = NULL;

    axis2_char_t *code = NULL;
    axis2_char_t *reason = NULL;
    axis2_char_t *detail = NULL;
    axis2_char_t *role = NULL;

    axis2_char_t *soap_ns = AXIOM_SOAP12_SOAP_ENVELOPE_NAMESPACE_URI;
    int soap_version = AXIOM_SOAP12;

    smart_str fcode = {0};
    zval **tmp;

    if (!in_msg_ctx || !out_msg_ctx)
        return;

    if (axis2_msg_ctx_get_is_soap_11(in_msg_ctx, env))
    {
        soap_ns = AXIOM_SOAP11_SOAP_ENVELOPE_NAMESPACE_URI; /* default is 1.2 */
        soap_version = AXIOM_SOAP11;
    }

    if (strcmp(soap_ns, AXIOM_SOAP12_SOAP_ENVELOPE_NAMESPACE_URI) == 0)
    {
        soap_version = AXIOM_SOAP12;
    } else if (strcmp(soap_ns, AXIOM_SOAP11_SOAP_ENVELOPE_NAMESPACE_URI) == 0)
    {
        soap_version = AXIOM_SOAP11;
    } else
        return;

    env_ns = axiom_namespace_create(env, soap_ns, AXIOM_SOAP_DEFAULT_NAMESPACE_PREFIX);
    if (!env_ns)
    {
        return;
    }

    if (zend_hash_find(Z_OBJPROP_P(zval_soap_fault), WSF_FAULT_REASON,
            sizeof (WSF_FAULT_REASON), (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING)
    {
        reason = Z_STRVAL_PP(tmp);
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[wsf_service] setting fault reason %s", reason);
    } else
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[wsf_service]invalid soap fault (reason|string) missing");
        reason = "Error Reason unspecified";
    }

    if (zend_hash_find(Z_OBJPROP_P(zval_soap_fault), WSF_FAULT_CODE, sizeof (WSF_FAULT_CODE),
            (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING)
    {
        smart_str_appends(&fcode, AXIOM_SOAP_DEFAULT_NAMESPACE_PREFIX);
        smart_str_appends(&fcode, ":");
        smart_str_appends(&fcode, Z_STRVAL_PP(tmp));
        smart_str_0(&fcode);
        code = fcode.c;
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Setting fault code %s", code);
    } else
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
			"Invalid soap fault (reason|string) missing");
        code = AXIOM_SOAP12_FAULT_CODE_RECEIVER;
    }

    if (zend_hash_find(Z_OBJPROP_P(zval_soap_fault), WSF_FAULT_ROLE, sizeof (WSF_FAULT_ROLE),
            (void **) & tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING)
    {
        role = Z_STRVAL_PP(tmp);
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "setting fault role %s", role);
    }
    if (zend_hash_find(Z_OBJPROP_P(zval_soap_fault), WSF_FAULT_DETAIL,
            sizeof (WSF_FAULT_DETAIL), (void **) & tmp) == SUCCESS
            && Z_TYPE_PP(tmp) == IS_STRING)
    {
        axiom_node_t *text_node = NULL;
        detail = Z_STRVAL_PP(tmp);
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Setting fault detail %s", detail);
        text_node = axiom_node_create_from_buffer(env, detail);
		if(text_node && axiom_node_get_node_type(text_node,env) == AXIOM_ELEMENT)
		{
			detail_node = text_node;
		}else
		{
			axiom_element_create(env, NULL, "error", NULL, &detail_node);
			if(text_node)
			{
				axiom_node_add_child(detail_node, env, text_node);
			}else{
				axiom_text_create(env, detail_node, detail, &text_node);
			}
		}
	}

    out_envelope = axiom_soap_envelope_create(env, env_ns);
    out_header = axiom_soap_header_create_with_parent(env, out_envelope);
    out_body = axiom_soap_body_create_with_parent(env, out_envelope);
    out_fault = axiom_soap_fault_create_default_fault(env, out_body, code, reason, soap_version);
    if (role)
    {
        fault_role = axiom_soap_fault_role_create_with_parent(env, out_fault);
        axiom_soap_fault_role_set_role_value(fault_role, env, role);
    }

    if (detail)
    {
        fault_detail = axiom_soap_fault_detail_create_with_parent(env, out_fault);
        axiom_soap_fault_detail_add_detail_entry(fault_detail, env, detail_node);
    }

    axis2_msg_ctx_set_soap_envelope(out_msg_ctx, env, out_envelope);
    smart_str_free(&fcode);
}

static zval*
wsf_xml_msg_recv_process_incomming_headers(
        axiom_soap_envelope_t *envelope,
        const axutil_env_t *env TSRMLS_DC)
{
    axiom_soap_header_t *header = NULL;
    zval *output_headers = NULL;
    axiom_node_t *header_node = NULL;
    axiom_node_t *child_node = NULL;
    axiom_element_t *child_ele = NULL;
    axiom_node_t *sibling_node = NULL;
    axiom_element_t *sibling_ele = NULL;

    if (!envelope)
        return NULL;

    header = axiom_soap_envelope_get_header(envelope, env);
    if (header)
    {
        header_node = axiom_soap_header_get_base_node(header, env);
        if (header_node)
        {
            child_node = axiom_node_get_first_element(header_node, env);
            if (child_node)
            {
                child_ele = (axiom_element_t*) axiom_node_get_data_element(child_node, env);
                if (child_ele)
                {
                    char *localname = NULL;
                    zval *ws_header = NULL;
                    char *xml_string = NULL;
                    localname = axiom_element_get_localname(child_ele, env);
                    xml_string = axiom_node_sub_tree_to_string(child_node, env);

                    MAKE_STD_ZVAL(output_headers);
                    array_init(output_headers);

                    MAKE_STD_ZVAL(ws_header);
                    object_init_ex(ws_header, ws_header_class_entry);
                    add_property_string(ws_header, WSF_HEADER_LOCALNAME, localname, 1);
                    add_property_string(ws_header, WSF_HEADER_STR, xml_string, 1);
                    add_next_index_zval(output_headers, ws_header);

                    sibling_node = axiom_node_get_next_sibling(child_node, env);
                    do
                    {
                        if (sibling_node && axiom_node_get_node_type(sibling_node, env) == AXIOM_ELEMENT)
                        {
                            sibling_ele = (axiom_element_t*) axiom_node_get_data_element(sibling_node, env);
                            if (sibling_ele)
                            {
                                char *localname = NULL;
                                char *xml_string = NULL;
                                zval *ws_header = NULL;

                                localname = axiom_element_get_localname(sibling_ele, env);
                                xml_string = axiom_node_sub_tree_to_string(sibling_node, env);

                                MAKE_STD_ZVAL(ws_header);
                                object_init_ex(ws_header, ws_header_class_entry);
                                add_property_string(ws_header, WSF_HEADER_LOCALNAME, localname, 1);
                                add_property_string(ws_header, WSF_HEADER_STR, xml_string, 1);
                                add_next_index_zval(output_headers, ws_header);
                            }
                        }
                        if (sibling_node)
                        {
                            sibling_node = axiom_node_get_next_sibling(sibling_node, env);
                        }

                    } while (sibling_node != NULL);
                }
            }
        }
    }
    return output_headers;
}
