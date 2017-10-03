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
#include <axis2_addr.h>
#include "wsf.h"
#include "wsf_util.h"
#include <axutil_error_default.h>
#include <axutil_log_default.h>
#include <axutil_uuid_gen.h>
#include <axiom_util.h>
#include "wsf_policy.h"
#include <axiom.h>
#include <rampart_context.h>
#include <rampart_sec_processed_result.h>
#include <axis2_key_type.h>
#include <neethi_options.h>
#include <axis2_policy_include.h>
#include <neethi_engine.h>
#include <secconv_security_context_token.h>
#include <axis2_msg.h>
#include <rp_defines.h>


int 
wsf_set_security_policy_options (
    zval * policy_obj,
    zval ** tmp,
    const axutil_env_t * env TSRMLS_DC);

axiom_node_t*
wsf_create_policy_node (
    const axutil_env_t * env,
    axiom_node_t * node);

/** Callback function prototypes */

axis2_char_t* AXIS2_CALL
wsf_password_callback_function (
    const axutil_env_t * env,
    const axis2_char_t * username,
    void *ctx);

axis2_status_t AXIS2_CALL
wsf_replay_detection_callback_function(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    rampart_context_t *rampart_context,
    void *user_params);

axis2_status_t AXIS2_CALL
wsf_store_sct_callback_function(
	const axutil_env_t *env,
	axis2_msg_ctx_t* msg_ctx, 
	axis2_char_t *sct_global_id, 
	axis2_char_t *sct_local_id, 
	void *sct, 
	void *user_params);

void* AXIS2_CALL
wsf_get_sct_callback_function(
	const axutil_env_t *env, 
	axis2_bool_t is_encryption, 
	axis2_msg_ctx_t* msg_ctx, 
	axis2_char_t *sct_id, 
	int sct_id_type,
	void* user_params);
	 
axis2_status_t AXIS2_CALL
wsf_delete_sct_callback_function(
	const axutil_env_t *env,
	axis2_msg_ctx_t* msg_ctx, 
	axis2_char_t *sct_id, 
	int sct_id_type,
	void* user_params);

axis2_status_t AXIS2_CALL
wsf_validate_sct_callback_function(
    const axutil_env_t *env,
    axiom_node_t *sct_node,
    axis2_msg_ctx_t *msg_ctx,
    void *user_params);

void
wsf_set_rampart_sct_options (
	rampart_context_t *rampart_context,
	HashTable * ht_token,
	const axutil_env_t * env TSRMLS_DC);


char*
wsf_get_rampart_token_value(
	char *token_ref);

int
wsf_policy_handle_client_security (
    zval * sec_token,
    zval * policy,
    axutil_env_t * env,
    axis2_svc_client_t * svc_client TSRMLS_DC)
{
	axiom_node_t *policy_node = NULL;

    HashTable *ht = NULL;
    zval **tmp = NULL;

    rampart_context_t *rampart_ctx = NULL;

    axis2_svc_ctx_t *svc_ctx = NULL;
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;

    axutil_param_t *security_param = NULL;

    axiom_element_t *root_ele = NULL;
    axis2_desc_t *desc = NULL;
    axis2_policy_include_t *policy_include = NULL;
    axis2_svc_t *svc = NULL;

    neethi_policy_t *neethi_policy = NULL;

    char *policy_xml = NULL;

    if (!sec_token) {
        return AXIS2_FAILURE;
    }

    if (policy && Z_TYPE_P (policy) == IS_OBJECT) {
        ht = Z_OBJPROP_P (policy);
        if (zend_hash_find (ht, WSF_POLICY_XML, sizeof (WSF_POLICY_XML),
                (void **) & tmp) == SUCCESS && (Z_TYPE_PP (tmp) == IS_STRING)) 
		{
            policy_xml = Z_STRVAL_PP (tmp);
            policy_node = wsf_util_deserialize_buffer (env, policy_xml);
        }
		else
		{
			policy_node = wsf_do_create_policy (policy, AXIS2_FAILURE, env TSRMLS_CC);
		}
    }
    rampart_ctx = rampart_context_create (env);

    wsf_set_rampart_options (rampart_ctx, sec_token, policy, env TSRMLS_CC);

	if(rampart_context_get_custom_tokens(rampart_ctx, env) && policy == NULL) {
        return AXIS2_FAILURE;
    }

	if(policy == NULL && rampart_context_get_custom_tokens(rampart_ctx, env) != NULL) {

        neethi_options_t *neethi_options = NULL;

        neethi_options = neethi_options_create (env); 
        policy_node = neethi_options_get_root_node (neethi_options, env);
    }

    if (policy_node) {
        if (axiom_node_get_node_type (policy_node, env) == AXIOM_ELEMENT) 
		{
            root_ele = (axiom_element_t *)axiom_node_get_data_element (policy_node, env);

            if (root_ele) 
			{
                neethi_policy = neethi_engine_get_policy (env, policy_node, root_ele);
                if (!neethi_policy) 
				{
                    return AXIS2_FAILURE;
                }
                svc = axis2_svc_client_get_svc (svc_client, env);
                if (!svc) 
				{
                    return AXIS2_FAILURE;
                }
                desc = axis2_svc_get_base (svc, env);
                if (!desc) 
				{
                    return AXIS2_FAILURE;
                }
                policy_include = axis2_desc_get_policy_include (desc, env);
                if (!policy_include) 
				{
                    return AXIS2_FAILURE;
                }
                axis2_policy_include_add_policy_element (policy_include, env,
                    AXIS2_SERVICE_POLICY, neethi_policy);

                svc_ctx = axis2_svc_client_get_svc_ctx (svc_client, env);
                conf_ctx = axis2_svc_ctx_get_conf_ctx (svc_ctx, env);
                conf = axis2_conf_ctx_get_conf (conf_ctx, env);

                security_param = axutil_param_create (env, WSF_RAMPART_CONFIGURATION, (void *) rampart_ctx);
                axis2_conf_add_param (conf, env, security_param);
            }
        }
    }

    axis2_svc_client_engage_module (svc_client, env, WSF_MODULE_SECURITY);

    /** Used for Debugging */
	if(env->log->level == AXIS2_LOG_LEVEL_DEBUG)
	{
		if (policy_node) 
		{
			axis2_char_t *om_str_out = NULL;
			
			om_str_out = axiom_node_to_string (policy_node, env);

			AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, 
			WSF_PHP_LOG_PREFIX "creating rampart client outgoing policy node \n\t %s \n", om_str_out);

			AXIS2_FREE(env->allocator, om_str_out);
			om_str_out = NULL;

		}
	}
    return AXIS2_SUCCESS;
}

int
wsf_policy_handle_server_security (
	wsf_svc_info_t *svc_info,
    axutil_env_t * env,
    axis2_conf_t * conf TSRMLS_DC)
{
	axiom_node_t *policy_node = NULL;
	HashTable *policy_ht = NULL;
    rampart_context_t *rampart_ctx = NULL;
    axutil_param_t *security_param = NULL;
    char *policy_xml = NULL;
    axiom_element_t *root_ele = NULL;
    axis2_desc_t *desc = NULL;
    axis2_policy_include_t *policy_include = NULL;

    if (!svc_info->security_token && !svc_info->policy && !svc_info->svc && !conf)
        return AXIS2_FAILURE;

    if (svc_info->policy && Z_TYPE_P (svc_info->policy) == IS_OBJECT) 
	{
		zval **tmp = NULL;
		policy_ht = Z_OBJPROP_P (svc_info->policy);
		
        if (zend_hash_find (policy_ht, WSF_POLICY_XML, sizeof (WSF_POLICY_XML), 
			(void **) &tmp) == SUCCESS && (Z_TYPE_PP (tmp) == IS_STRING)) {
            policy_xml = Z_STRVAL_PP (tmp);
            policy_node = wsf_util_deserialize_buffer(env, policy_xml);
        }
		else
		{
			policy_node  = wsf_do_create_policy (svc_info->policy, AXIS2_TRUE, env TSRMLS_CC);
		}
    }
   if (policy_node) 
	{
		
        if (axiom_node_get_node_type (policy_node, env) == AXIOM_ELEMENT) 
		{
            root_ele = (axiom_element_t *)axiom_node_get_data_element (policy_node, env);
            if (root_ele) 
			{
                neethi_policy_t *neethi_policy = NULL;
                neethi_policy = neethi_engine_get_policy (env, policy_node, root_ele);
                if (!neethi_policy) 
				{
                    return AXIS2_FAILURE;
                }
                
                desc = axis2_svc_get_base (svc_info->svc, env);
                if (!desc) 
				{
                    return AXIS2_FAILURE;
                }
                policy_include = axis2_desc_get_policy_include (desc, env);
                if (!policy_include) 
				{
                    return AXIS2_FAILURE;
                }
                axis2_policy_include_add_policy_element (policy_include, env,
					AXIS2_SERVICE_POLICY, neethi_policy);
            }
        }
    }

	if(svc_info->ht_op_policies)
	{
		if(!wsf_policy_handle_op_policies(svc_info->ht_op_policies, env, svc_info->svc TSRMLS_CC))
			return AXIS2_FAILURE;
	}

	rampart_ctx = rampart_context_create (env);
	wsf_set_rampart_options (rampart_ctx, svc_info->security_token, svc_info->policy, env TSRMLS_CC);

	security_param = axutil_param_create (env, WSF_RAMPART_CONFIGURATION, (void *) rampart_ctx);
	axis2_svc_add_param (svc_info->svc, env, security_param);
    /** engage module rampart */
    wsf_util_engage_module (conf, WSF_MODULE_SECURITY, env, svc_info->svc);

	if(env->log->level == AXIS2_LOG_LEVEL_DEBUG)
	{
		if (policy_node) 
		{
			axis2_char_t *policy_string = NULL;

			policy_string = axiom_node_to_string (policy_node, env);

			AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
				"creating rampart service policy node \n\t %s \n", policy_string);
			AXIS2_FREE(env->allocator, policy_string);
		}
	}
    return AXIS2_SUCCESS;
}
/** 
* Set policy include for each of the policy operations
*/
void
wsf_policy_handle_op_policy(
	zval *policy,
	axutil_env_t *env,
	axis2_svc_t *svc,
	axis2_char_t *op_name TSRMLS_DC)
{
	axiom_node_t *policy_node = NULL;
	axiom_node_t *in_msg_policy_node = NULL;
	axiom_node_t *out_msg_policy_node = NULL;
	HashTable *policy_ht = NULL;

	
	axis2_op_t *op = NULL;

	if (Z_OBJCE_P(policy) == ws_policy_class_entry) 
	{
		zval **tmp = NULL;
		policy_ht = Z_OBJPROP_P (policy);
		if(zend_hash_find(policy_ht, WSF_IN_POLICY, sizeof(WSF_IN_POLICY), (void **)&tmp) == SUCCESS)
		{
			if(Z_TYPE_PP(tmp) == IS_ARRAY)
			{
				in_msg_policy_node = wsf_do_create_policy(*tmp, AXIS2_TRUE, env TSRMLS_CC);
			}
			else if(Z_TYPE_PP(tmp) == IS_STRING)
			{
				in_msg_policy_node = wsf_util_deserialize_buffer(env, Z_STRVAL_PP(tmp));
			}
		}
		if(zend_hash_find(policy_ht, WSF_OUT_POLICY, sizeof(WSF_OUT_POLICY), (void **)&tmp) == SUCCESS)
		{
			if(Z_TYPE_PP(tmp) == IS_ARRAY)
			{
				out_msg_policy_node = wsf_do_create_policy(*tmp, AXIS2_TRUE, env TSRMLS_CC);
			}
			else if(Z_TYPE_PP(tmp) == IS_STRING)
			{
				out_msg_policy_node = wsf_util_deserialize_buffer(env, Z_STRVAL_PP(tmp));
			}
		}
		if (zend_hash_find (policy_ht, WSF_POLICY_XML, sizeof (WSF_POLICY_XML), 
			(void **) &tmp) == SUCCESS && (Z_TYPE_PP (tmp) == IS_STRING))
		{
				
				policy_node = wsf_util_deserialize_buffer(env, Z_STRVAL_PP (tmp));
		}
		else
		{
				policy_node  = wsf_do_create_policy (policy, AXIS2_TRUE, env TSRMLS_CC);
		}
	}
	if(in_msg_policy_node || out_msg_policy_node || policy_node)
	{
		axis2_desc_t *desc = NULL;
		axis2_policy_include_t *policy_include = NULL;
		op = axis2_svc_get_op_with_name(svc, env, op_name);
		if (policy_node) 
		{
			axiom_element_t *policy_element = NULL;	
			if (axiom_node_get_node_type (policy_node, env) == AXIOM_ELEMENT) 
			{
				policy_element = (axiom_element_t *)axiom_node_get_data_element (policy_node, env);
				if (policy_element) 
				{
					neethi_policy_t *op_policy = NULL;
					op_policy = neethi_engine_get_policy (env, policy_node, policy_element);
					if (!op_policy) 
					{
						return;
					}
					
					desc = axis2_op_get_base (op, env);
					if (!desc) 
					{
						return;
					}
					policy_include = axis2_desc_get_policy_include (desc, env);
					if (!policy_include) 
					{
						return ;
					}
					axis2_policy_include_add_policy_element (policy_include, env,
						AXIS2_SERVICE_POLICY, op_policy);
				}
			}
		}	
		if(in_msg_policy_node)
		{
			axis2_msg_t *msg = NULL;
			
			msg = axis2_op_get_msg(op, env, WSF_POLICY_MSG_LABEL_IN);
			if(msg)
			{
				desc = axis2_msg_get_base(msg, env);
				if(desc)
				{
					policy_include = axis2_desc_get_policy_include(desc, env);
					if(axiom_node_get_node_type(in_msg_policy_node, env) == AXIOM_ELEMENT)
					{
						axiom_element_t *policy_element = NULL;
						neethi_policy_t *op_in_msg_policy = NULL;
						policy_element = axiom_node_get_data_element(in_msg_policy_node, env);
						op_in_msg_policy = neethi_engine_get_policy(env, 
							in_msg_policy_node, policy_element);
						axis2_policy_include_add_policy_element(policy_include, env, 
							AXIS2_SERVICE_POLICY, op_in_msg_policy);
					}
				}
				
			}
		}
		if(out_msg_policy_node)
		{
			axis2_msg_t *msg = NULL;

			msg = axis2_op_get_msg(op, env, WSF_POLICY_MSG_LABEL_IN);
			if(msg)
			{
				desc = axis2_msg_get_base(msg, env);
				if(desc)
				{
					policy_include = axis2_desc_get_policy_include(desc, env);
					if(axiom_node_get_node_type(out_msg_policy_node, env) == AXIOM_ELEMENT)
					{
						axiom_element_t *policy_element = NULL;
						neethi_policy_t *op_out_msg_policy = NULL;
						policy_element = axiom_node_get_data_element(out_msg_policy_node, env);
						op_out_msg_policy = neethi_engine_get_policy(env, 
							in_msg_policy_node, policy_element);
						axis2_policy_include_add_policy_element(policy_include, env, 
						AXIS2_SERVICE_POLICY, op_out_msg_policy);
					}
				}
			}
		}
	}
}


/**
* 
* @param ht_policies, 
* @param env, 
* @param TSRMLS_DC, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
axis2_status_t 
wsf_policy_handle_op_policies(
  HashTable *ht_policies,
  axutil_env_t *env,
  axis2_svc_t *svc TSRMLS_DC)
{
			
	HashPosition pos;
	zval ** tmp = NULL;
	int i = 0;

	if (!ht_policies) 
	{
		return AXIS2_FAILURE;
	}

	zend_hash_internal_pointer_reset_ex (ht_policies, &pos);
	while (zend_hash_get_current_data_ex (ht_policies, (void **) &tmp, &pos) != FAILURE) 
	{
		char *op_name = NULL;
		unsigned int op_name_len = 0;
		unsigned long index = i;

		zend_hash_get_current_key_ex (ht_policies, &op_name, &op_name_len, &index, 0, &pos);
		zend_hash_get_current_data_ex(ht_policies,(void**)&tmp, &pos);
		if(Z_OBJCE_PP(tmp) == ws_policy_class_entry)
		{
			wsf_policy_handle_op_policy(*tmp, env, svc, op_name TSRMLS_CC);

		}
		zend_hash_move_forward_ex (ht_policies, &pos);
		i++;
	}
	return AXIS2_SUCCESS;
}



/**
* 
* @param rampart_context, 
* @param sec_token, 
* @param policy, 
* @param TSRMLS_DC, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
int
wsf_set_rampart_options (
    rampart_context_t *rampart_context,
    zval * sec_token,
    zval * policy,
    const axutil_env_t * env TSRMLS_DC)
{
    HashTable *ht_policy = NULL;
    HashTable *ht_token = NULL;
    zval **token_val = NULL;

    if(policy)
	{
        if (Z_TYPE_P (policy) == IS_OBJECT)
		{
            ht_policy = Z_OBJPROP_P (policy);
		}else 
		{
            ht_policy = Z_ARRVAL_P (policy);
		}
    }
	if(!rampart_context)
	{
		AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX " rampart context null");
		return -1;
	}
	if(sec_token)
	{
		ht_token = Z_OBJPROP_P (sec_token);
	}else
	{
		return AXIS2_FAILURE;
	}
    
    AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI,WSF_PHP_LOG_PREFIX "Setting values to rampart context ");
    
    
    if (zend_hash_find (ht_token, WSF_TTL, sizeof (WSF_TTL),
            (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_LONG) 
	{
		rampart_context_set_ttl(rampart_context, env, Z_LVAL_PP(token_val));
    }

	if (zend_hash_find (ht_token, WSF_CLOCK_SKEW_BUFFER, sizeof (WSF_CLOCK_SKEW_BUFFER),
            (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_LONG) 
	{
		rampart_context_set_clock_skew_buffer(rampart_context, env, Z_LVAL_PP(token_val));
    }

    if (zend_hash_find (ht_token, WSF_USER, sizeof (WSF_USER),
            (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_STRING) 
	{
        rampart_context_set_user(rampart_context, env, Z_STRVAL_PP(token_val));
    }
    if (zend_hash_find (ht_token, WSF_CERTIFICATE, sizeof (WSF_CERTIFICATE),
            (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_STRING)
    {
        rampart_context_set_certificate(rampart_context, env, Z_STRVAL_PP (token_val)); 
    }
    if (rampart_context_set_certificate_type (rampart_context, env, AXIS2_KEY_TYPE_PEM) == AXIS2_SUCCESS)
    {
    AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Setting certificate type");
    }

    if (zend_hash_find (ht_token, WSF_RECEIVER_CERTIFICATE,
            sizeof (WSF_RECEIVER_CERTIFICATE), (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_STRING) 
    {
        rampart_context_set_receiver_certificate(rampart_context, env, Z_STRVAL_PP(token_val));
    }
    if (rampart_context_set_receiver_certificate_type (rampart_context, env, AXIS2_KEY_TYPE_PEM) == AXIS2_SUCCESS)
    {
        AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Setting receiver certificate type");
    }
    if (zend_hash_find (ht_token, WSF_PRIVATE_KEY, sizeof (WSF_PRIVATE_KEY),
            (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_STRING)
    {
		rampart_context_set_prv_key(rampart_context, env, Z_STRVAL_PP(token_val));
		AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI,WSF_PHP_LOG_PREFIX "Setting pvt key ");	
    }
	if (rampart_context_set_prv_key_type (rampart_context, env, AXIS2_KEY_TYPE_PEM) == AXIS2_SUCCESS)
	{
        AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Setting pvt key format ");
	}
    if (zend_hash_find (ht_token, WSF_PASSWORD_TYPE, sizeof (WSF_PASSWORD_TYPE),
            (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_STRING) 
    {
        rampart_context_set_password_type(rampart_context, env, Z_STRVAL_PP(token_val));
    }
    if (zend_hash_find (ht_token, WSF_PASSWORD, sizeof (WSF_PASSWORD),
            (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_STRING) 
    {
        rampart_context_set_password(rampart_context, env, Z_STRVAL_PP(token_val));
    }
    if (zend_hash_find (ht_token, WSF_PASSWORD_CALLBACK,
            sizeof (WSF_PASSWORD_CALLBACK), (void **) &token_val) == SUCCESS
        && Z_TYPE_PP (token_val) == IS_STRING) 
    {
		zval **tmp = NULL;
		wsf_callback_args_t *callback_args = NULL;
		callback_args = wsf_callback_args_create(env);
		callback_args->callback_function = axutil_strdup(env, Z_STRVAL_PP(token_val));
		if (zend_hash_find(ht_token, WSF_PASSWORD_CALLBACK_ARGS, sizeof(WSF_PASSWORD_CALLBACK_ARGS),
			(void **)&tmp) == SUCCESS)
		{
			callback_args->callback_data = *tmp;
		}	 
		rampart_context_set_pwcb_function(rampart_context, env, wsf_password_callback_function, 
			callback_args);
    }
    if (zend_hash_find (ht_token, WSF_PKCS12_KEYSTORE, sizeof (WSF_PKCS12_KEYSTORE), 
            (void **) &token_val) == SUCCESS && Z_TYPE_PP (token_val) == IS_STRING) 
    {
        rampart_context_set_key_store_buff(rampart_context, env, Z_STRVAL_PP(token_val), Z_STRLEN_PP(token_val));
    }
    if (zend_hash_find(ht_token, WSF_REPLAY_DETECT_CALLBACK, sizeof(WSF_REPLAY_DETECT_CALLBACK),
            (void **)&token_val) == SUCCESS && Z_TYPE_PP(token_val) == IS_STRING)
    {
		zval **tmp = NULL;
		wsf_callback_args_t *args = wsf_callback_args_create(env);
		args->callback_function = Z_STRVAL_PP(token_val);
		if (zend_hash_find(ht_token, WSF_REPLAY_DETECT_CALLBACK_ARGS, sizeof(WSF_REPLAY_DETECT_CALLBACK_ARGS),
			(void **)&tmp) == SUCCESS)
		{
			args->callback_data = *tmp;
		}	 
        
		rampart_context_set_replay_detect_function(rampart_context, env, 
			wsf_replay_detection_callback_function, args);
    }
    if (zend_hash_find(ht_token, WSF_ENABLE_REPLAY_DETECT, sizeof(WSF_ENABLE_REPLAY_DETECT),
            (void **)&token_val) == SUCCESS && Z_TYPE_PP(token_val) == IS_BOOL)
    {
        rampart_context_set_rd_val(rampart_context, env, axutil_strdup(env, WSF_REPLAY_DETECT_NUMBER));
    }
	
    if (zend_hash_find (ht_token, WSF_CUSTOM_TOKENS, sizeof (WSF_CUSTOM_TOKENS), 
                (void **) &token_val) == SUCCESS  && Z_TYPE_PP (token_val) == IS_ARRAY) 
    {
       
        zval **tmp = NULL;
        HashPosition pos;
        axutil_array_list_t *custom_tokens = NULL;
        HashTable *ht_custom_tokens = NULL;

        /* custom token hash */
        ht_custom_tokens = Z_ARRVAL_PP(token_val);
        /* initialize the tokens array */
        custom_tokens = axutil_array_list_create(env, 10);
        /* reset the hash pointer to start */
        zend_hash_internal_pointer_reset_ex (ht_custom_tokens, &pos);
        /* loop through each element */
        while(zend_hash_get_current_data_ex(ht_custom_tokens, (void**)&tmp, &pos) != FAILURE)
		{
            if(Z_TYPE_PP (tmp) == IS_STRING) 
			{
                axis2_char_t *custom_token = NULL;
                axiom_node_t *custom_token_node = NULL;

                custom_token = Z_STRVAL_PP(tmp);

                AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
                    "Custom token: %s ", custom_token);
                
                custom_token_node = wsf_util_deserialize_buffer(env, custom_token);
                if(custom_token_node) 
				{
                    axutil_array_list_add(custom_tokens, env, (void*)custom_token_node);
                }
            }
            zend_hash_move_forward_ex (ht_custom_tokens, &pos);
        }
        
		rampart_context_set_custom_tokens(rampart_context, env, custom_tokens);
    }

	wsf_set_rampart_sct_options(rampart_context, ht_token, env TSRMLS_CC);
    return 0;
}

axiom_node_t *
wsf_do_create_policy (
    zval *policy,
	axis2_bool_t is_server_side,
    axutil_env_t * env TSRMLS_DC)
{
	HashTable *ht_policy = NULL;
    neethi_options_t *neethi_options = NULL;
    axiom_node_t *return_node = NULL;
    zval **tmp = NULL;
    char *algo_suite = NULL;
    char *token_ref = NULL;
    char *protection_order = NULL;

	if(!policy)
		return NULL;

	if(Z_TYPE_P(policy) == IS_OBJECT)
	{
		ht_policy = Z_OBJPROP_P(policy);
	}
	else if(Z_TYPE_P(policy) == IS_ARRAY)
	{
		ht_policy = Z_ARRVAL_P(policy);
	}

    AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Creating the policy node");

    neethi_options = neethi_options_create (env);
    if (neethi_options)
	{
        if (zend_hash_find (ht_policy, WSF_TIMESTAMP, sizeof (WSF_TIMESTAMP),
                (void **) &tmp) == SUCCESS)
		{
            if (neethi_options_set_include_timestamp (neethi_options, env, AXIS2_TRUE))
                AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Timestamp_enabled ");
		}
		if (zend_hash_find (ht_policy, WSF_UT, sizeof (WSF_UT),
				(void **) &tmp) == SUCCESS) 
		{
			if (neethi_options_set_is_username_token (neethi_options, env, AXIS2_TRUE))
				AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Token reference_enabled ");
		}
		if (zend_hash_find (ht_policy, WSF_ENCRYPT, sizeof (WSF_ENCRYPT),
				(void **) &tmp) == SUCCESS) 
		{
			if (neethi_options_set_encrypt_body (neethi_options, env, AXIS2_TRUE))
				AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Encrypt body option enabled ");
		}
		if (zend_hash_find (ht_policy, WSF_ALGORITHM, sizeof (WSF_ALGORITHM),
				(void **) &tmp) == SUCCESS && tmp != NULL && Z_TYPE_PP (tmp) == IS_STRING)
		{
			algo_suite = Z_STRVAL_PP (tmp);
			if (neethi_options_set_algorithmsuite (neethi_options, env, algo_suite))
				AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "AlgorithmSuite enabled ");
		}
		if (zend_hash_find (ht_policy, WSF_SIGN, sizeof (WSF_SIGN),
				(void **) &tmp) == SUCCESS)
		{
			if (neethi_options_set_sign_body (neethi_options, env, AXIS2_TRUE))
				AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Sign_enabled ");
		}
		if (zend_hash_find (ht_policy, WSF_TOKEN_REFERENCE, sizeof (WSF_TOKEN_REFERENCE),
							(void **) &tmp) == SUCCESS && tmp != NULL
			&& Z_TYPE_PP (tmp) == IS_STRING)
		{
			token_ref = wsf_get_rampart_token_value(Z_STRVAL_PP (tmp));
			if (neethi_options_set_keyidentifier (neethi_options, env, token_ref))
			{
				AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Token_ref_enabled ");
			}
			if(is_server_side == AXIS2_SUCCESS)
			{
				neethi_options_set_server_side(neethi_options, env, is_server_side);
			}
		}
	    
		if (zend_hash_find (ht_policy, WSF_ENCRYPT_SIGNATURE,
							sizeof (WSF_ENCRYPT_SIGNATURE), (void **) &tmp) == SUCCESS) 
		{
			if (neethi_options_set_signature_protection(neethi_options, env, AXIS2_TRUE))
				AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Encrypt_sign_enabled ");
		}

		if (zend_hash_find (ht_policy, WSF_PROTECTION_ORDER, 
			sizeof (WSF_PROTECTION_ORDER), (void **) &tmp) == SUCCESS && tmp != NULL 
			&& Z_TYPE_PP (tmp) == IS_STRING)
		{
			protection_order = Z_STRVAL_PP (tmp);
			if(strcmp(protection_order, WSF_ENCRYPT_BEFORE) == 0)
			{
				 if (neethi_options_set_encrypt_before_sign (neethi_options, env, AXIS2_TRUE))
				 {
					AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Encrypt_before_sign_enabled");
				 }
			}
			else if(strcmp(protection_order, WSF_SIGN_BEFORE) == 0)
			{
				if (neethi_options_set_encrypt_before_sign (neethi_options, env, AXIS2_FALSE))
					AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI,WSF_PHP_LOG_PREFIX "Sign_before_encrption_enabled");
			}
			else 
			{
				AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Wrong Protection Order ");
				php_error_docref (NULL TSRMLS_CC, E_ERROR, "Wrong option given for protection order");
			}
		}

		if(zend_hash_find(ht_policy, WSF_SECURITY_BINDING, sizeof(WSF_SECURITY_BINDING), 
			(void**)&tmp) == SUCCESS && Z_TYPE_PP(tmp) == IS_STRING)
		{
			if(strcmp(Z_STRVAL_PP(tmp),WSF_SECURITY_BINDING_ASYMETRIC) == 0)
			{
				neethi_options_set_binding(neethi_options, env, RP_ASYMMETRIC_BINDING);
			}else
			{
				neethi_options_set_binding(neethi_options, env, RP_SYMMETRIC_BINDING);
			}
		}


	}
		
    if (neethi_options)
	{
        return_node = neethi_options_get_root_node (neethi_options, env);
    }
    return return_node;
}

int
wsf_policy_set_policy_options (
    zval * this_ptr,
    zval * property,
    const axutil_env_t * env TSRMLS_DC)
{

    zval **tmp = NULL;

    if (!property)
        return AXIS2_FAILURE;

    if (Z_TYPE_P (property) == IS_ARRAY) 
	{
        HashTable *ht = Z_ARRVAL_P (property);

        if (!ht)
            return AXIS2_FAILURE;
        /* for security policy related things  */
        if (zend_hash_find (ht, WSF_POLICY_SECURITY, sizeof (WSF_POLICY_SECURITY),
                (void **) & tmp) == SUCCESS && Z_TYPE_PP (tmp) == IS_ARRAY) 
		{
            wsf_set_security_policy_options (this_ptr, tmp, env TSRMLS_CC);
            return AXIS2_SUCCESS;
        }
    }
    if (Z_TYPE_P (property) == IS_STRING) 
	{
        add_property_zval (this_ptr, WSF_POLICY_XML, property);
        AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Policy node creating from a file ");
        return AXIS2_SUCCESS;
    }
    return AXIS2_FAILURE;
}

int
wsf_set_security_policy_options (
    zval * policy_obj,
    zval ** sec_options,
    const axutil_env_t * env TSRMLS_DC)
{
    HashTable *ht_sec = NULL;
    zval **sec_prop = NULL;

    if (sec_options == NULL)
        return AXIS2_FAILURE;

    if (Z_TYPE_PP (sec_options) == IS_ARRAY)
	{
        ht_sec = Z_ARRVAL_PP (sec_options);
        if (!ht_sec)
            return AXIS2_FAILURE;
		if(zend_hash_find(ht_sec, WSF_SECURITY_BINDING, sizeof(WSF_SECURITY_BINDING), 
			(void**)&sec_prop) == SUCCESS && Z_TYPE_PP(sec_prop) == IS_STRING)
		{
			add_property_stringl(policy_obj, WSF_SECURITY_BINDING, Z_STRVAL_PP(sec_prop), Z_STRLEN_PP(sec_prop),1);
		}
        if (zend_hash_find (ht_sec, WSF_SIGN, sizeof (WSF_SIGN),
                (void **) &sec_prop) == SUCCESS && (Z_TYPE_PP (sec_prop) == IS_STRING
                || Z_TYPE_PP (sec_prop) == IS_BOOL)) 
		{
            add_property_zval (policy_obj, WSF_SIGN, *sec_prop);
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX " Signing is enabled ");
        }

        if (zend_hash_find (ht_sec, WSF_ENCRYPT, sizeof (WSF_ENCRYPT),
                (void **) &sec_prop) == SUCCESS && (Z_TYPE_PP (sec_prop) == IS_STRING
                || Z_TYPE_PP (sec_prop) == IS_BOOL)) 
		{
            add_property_zval (policy_obj, WSF_ENCRYPT, *sec_prop);
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX " Encryption is enabled ");
        }

        if (zend_hash_find (ht_sec, WSF_ALGORITHM, sizeof (WSF_ALGORITHM),
                (void **) &sec_prop) == SUCCESS && (Z_TYPE_PP (sec_prop) == IS_STRING)) {
			add_property_stringl (policy_obj, WSF_ALGORITHM, 
				Z_STRVAL_PP (sec_prop), Z_STRLEN_PP (sec_prop), 1);
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "AlgorithmSuite is enabled ");
        }

        if (zend_hash_find (ht_sec, WSF_LAYOUT, sizeof (WSF_LAYOUT),
                (void **) &sec_prop) == SUCCESS
            && (Z_TYPE_PP (sec_prop) == IS_STRING)) 
		{
            add_property_stringl (policy_obj, WSF_LAYOUT, Z_STRVAL_PP (sec_prop), Z_STRLEN_PP (sec_prop), 1);
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Layout is enabled ");
        }

        if (zend_hash_find (ht_sec, WSF_TIMESTAMP, sizeof (WSF_TIMESTAMP),
                (void **) &sec_prop) == SUCCESS
            && (Z_TYPE_PP (sec_prop) == IS_BOOL)) 
		{
            add_property_bool (policy_obj, WSF_TIMESTAMP, Z_BVAL_PP (sec_prop));
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Timestamp is enabled ");

        }

        if (zend_hash_find (ht_sec, WSF_PROTECTION_ORDER,
                sizeof (WSF_PROTECTION_ORDER), (void **) &sec_prop) == SUCCESS
            && (Z_TYPE_PP (sec_prop) == IS_STRING)) 
		{
            add_property_stringl (policy_obj, WSF_PROTECTION_ORDER,
                Z_STRVAL_PP (sec_prop), Z_STRLEN_PP (sec_prop), 1);
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
				"Protection order is enabled ");

        }

        if (zend_hash_find (ht_sec, WSF_UT, sizeof (WSF_UT),
                (void **) &sec_prop) == SUCCESS
            && (Z_TYPE_PP (sec_prop) == IS_BOOL)) 
		{
            add_property_bool (policy_obj, WSF_UT, Z_BVAL_PP (sec_prop));
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX " UsernameToken is enabled ");

        }

        if (zend_hash_find (ht_sec, WSF_TOKEN_REFERENCE, sizeof (WSF_TOKEN_REFERENCE), 
			(void **) &sec_prop) == SUCCESS && (Z_TYPE_PP (sec_prop) == IS_STRING)) 
		{
            add_property_stringl (policy_obj, WSF_TOKEN_REFERENCE,
                Z_STRVAL_PP (sec_prop), Z_STRLEN_PP (sec_prop), 1);
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Token reference  is enabled ");

        }

        if (zend_hash_find (ht_sec, WSF_PASSWORD_CALLBACK, sizeof (WSF_PASSWORD_CALLBACK),
                (void **) &sec_prop) == SUCCESS && Z_TYPE_PP (sec_prop) == IS_STRING) 
		{
            add_property_stringl (policy_obj, WSF_PASSWORD_CALLBACK,
                Z_STRVAL_PP (sec_prop), Z_STRLEN_PP (sec_prop), 1);
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "Callback is enabled");
        }

        if (zend_hash_find (ht_sec, WSF_ENCRYPT_SIGNATURE, sizeof (WSF_ENCRYPT_SIGNATURE),
			(void **) &sec_prop) == SUCCESS && (Z_TYPE_PP (sec_prop) == IS_STRING
                || Z_TYPE_PP (sec_prop) == IS_BOOL)) 
		{
            add_property_zval (policy_obj, WSF_ENCRYPT_SIGNATURE, *sec_prop);
            AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI,WSF_PHP_LOG_PREFIX \
				"Signature encryption is enabled ");
        }

        /* This is only useful for operation level policies In policy maps to inMsgPolicy */
        if (zend_hash_find (ht_sec, WSF_IN_POLICY, sizeof (WSF_IN_POLICY),
                (void **) &sec_prop) == SUCCESS) 
		{
            if (Z_TYPE_PP (sec_prop) == IS_ARRAY) 
			{
                add_property_zval (policy_obj, WSF_IN_POLICY, *sec_prop);
                AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
					" In policy array is enabled");
            }
            if (Z_TYPE_PP (sec_prop) == IS_STRING) 
			{
                add_property_stringl (policy_obj, WSF_IN_POLICY,
                    Z_STRVAL_PP (sec_prop), Z_STRLEN_PP (sec_prop), 1);
                AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
					"In policy XML file is enabled");
            }
        }
        if (zend_hash_find (ht_sec, WSF_OUT_POLICY, sizeof (WSF_OUT_POLICY),
                (void **) &sec_prop) == SUCCESS) 
		{
            if (Z_TYPE_PP (sec_prop) == IS_ARRAY) 
			{
                add_property_zval (policy_obj, WSF_OUT_POLICY, *sec_prop);
                AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
					"Out policy array is enabled");
			}
            if (Z_TYPE_PP (sec_prop) == IS_STRING) 
			{
                add_property_stringl (policy_obj, WSF_OUT_POLICY,
                    Z_STRVAL_PP (sec_prop), Z_STRLEN_PP (sec_prop), 1);
                AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
					"Out policy XML file is enabled");
            }
        }
    }
    return AXIS2_SUCCESS;
}

/**
* 
* @param env, 
* @param username, 
* @param ctx, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
axis2_char_t *AXIS2_CALL
wsf_password_callback_function (
    const axutil_env_t * env,
    const axis2_char_t * username,
    void *ctx)
{
    zval func, param1, param2, retval;
    zval *params[2];
    char *val = NULL;
	wsf_callback_args_t *callback_args = NULL;

    TSRMLS_FETCH ();
    params[0] = &param1;
	params[1] = &param2;

	callback_args = (wsf_callback_args_t*)ctx;

    ZVAL_STRING (&func, (char *) callback_args->callback_function, 1);

    ZVAL_STRING (params[0], (char *) username, 1);
	INIT_PZVAL(params[0]);
	if(callback_args->callback_data)
	{
		ZVAL_ZVAL(params[1],callback_args->callback_data, 0,1);
	}
	else
	{
		ZVAL_NULL(params[1]);
	}
	INIT_PZVAL(params[1]);

    if (call_user_function (EG (function_table), (zval **) NULL,
            &func, &retval, 2, params TSRMLS_CC) == SUCCESS) 
	{
        if (Z_TYPE_P (&retval) == IS_STRING) 
		{
            val = strdup (Z_STRVAL (retval));
        }
    }
	wsf_callback_args_free(callback_args, env);
	
    return val;
}

/** This is the replay detection callback function that will be set to the rampart context
Rampart replay detection will call this function. If true is returned, the message is not replayed,
if false is returned, it is a replayed function */

axis2_status_t AXIS2_CALL
wsf_replay_detection_callback_function(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    rampart_context_t *rampart_context,
    void *user_params)
{
    zval func, param1, param2, param3, retval;
    zval *params[3];
    int status = AXIS2_FAILURE;

    axis2_char_t *msg_id = NULL;
    axis2_char_t *time_stamp = NULL;
    axutil_hash_t *hash = NULL;
    zend_bool ret_bool;
    wsf_callback_args_t *callback_args = NULL;
    TSRMLS_FETCH();
    params[0] = &param1;
    params[1] = &param2;
    params[2] = &param3;
	
	callback_args = (wsf_callback_args_t*)user_params;
    
    msg_id = (axis2_char_t*)axis2_msg_ctx_get_wsa_message_id(msg_ctx, env);
    
    /*Get timestamp from security processed results*/
    hash = (axutil_hash_t*)rampart_get_all_security_processed_results(env, msg_ctx);
    time_stamp = axutil_hash_get(hash, RAMPART_SPR_TS_CREATED, AXIS2_HASH_KEY_STRING);
    
    AXIS2_LOG_DEBUG (env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX \
		"replay callback received time Stamp: %s", time_stamp);
    

    ZVAL_STRING (&func, callback_args->callback_function, 1);
    
	ZVAL_STRING (params[0], (char*)msg_id, 1);
	INIT_PZVAL(params[0]);
	
	if(time_stamp)
	{
		ZVAL_STRING (params[1], (char*)time_stamp, 1);
	}
	else
	{
		ZVAL_NULL(params[1]);
	}
	INIT_PZVAL(params[1]);
    
	if(callback_args->callback_data)
	{
		ZVAL_ZVAL(params[2], callback_args->callback_data, 0, 1);
	}
	else
	{
		ZVAL_NULL(params[2]);
	}
	INIT_PZVAL(params[2]);
/** Parameter order is (msg id, timestamp, callback data) */
	if (call_user_function (EG (function_table), (zval **) NULL,
            &func, &retval, 3, params TSRMLS_CC) == SUCCESS) 
    {
        if (Z_TYPE (retval) == IS_BOOL) 
		{
            ret_bool = (Z_BVAL (retval));
            if(ret_bool)
            {
				status = AXIS2_SUCCESS;
            }
        }
    }
	wsf_callback_args_free(callback_args, env);
    return status;    

}
/**
* 
* @param env, 
* @param msg_ctx, 
* @param sct_global_id, 
* @param sct_local_id, 
* @param sct, 
* @param user_params, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
axis2_status_t AXIS2_CALL
wsf_store_sct_callback_function(
	const axutil_env_t *env,
	axis2_msg_ctx_t* msg_ctx, 
	axis2_char_t *sct_global_id, 
	axis2_char_t *sct_local_id, 
	void *sct, 
	void *user_params)
{
	zval func, param1, param2, param3, param4, retval;
	zval *params[4];
	int status = AXIS2_FAILURE;
	axis2_char_t *sct_str = NULL;
	zend_bool ret_bool;
	wsf_callback_args_t *callback_args = NULL;
	TSRMLS_FETCH();
	params[0] = &param1;
	params[1] = &param2;
	params[2] = &param3;
	params[3] = &param4;

	callback_args = (wsf_callback_args_t*)user_params;

	ZVAL_STRING (&func, callback_args->store_sct_callback_fn, 1);

	if(sct)
	{
		sct_str = security_context_token_serialize(sct, env);
		if(!sct_str)
			return AXIS2_FAILURE;
	}
	else
	{
		return AXIS2_FAILURE;
	}
	ZVAL_STRING (params[0], (char*)sct_str, 1);
	INIT_PZVAL(params[0]);

	if(sct_global_id)
	{
		ZVAL_STRING (params[1], (char*)sct_global_id, 1);
	}
	else
	{
		ZVAL_NULL(params[1]);
	}
	INIT_PZVAL(params[1]);

	if(sct_global_id)
	{
		ZVAL_STRING (params[2], (char*)sct_local_id, 1);
	}
	else
	{
		ZVAL_NULL(params[2]);
	}
	INIT_PZVAL(params[2]);

	if(callback_args->callback_data)
	{
		ZVAL_ZVAL(params[3], callback_args->callback_data, 0, 1);
	}
	else
	{
		ZVAL_NULL(params[3]);
	}
	INIT_PZVAL(params[3]);

/** Store callback function arguments order (sct str,global id, local id, user args ) */

	if (call_user_function (EG (function_table), (zval **) NULL,
		&func, &retval, 4, params TSRMLS_CC) == SUCCESS) 
		{
			if (Z_TYPE (retval) == IS_BOOL) 
			{
				ret_bool = (Z_BVAL (retval));
				if(ret_bool)
				{
					status = AXIS2_SUCCESS;
				}
			}
		}
	AXIS2_FREE(env->allocator, sct_str);
	return status;    
}

/**
* This is the extension level callback function registered in rampart for getting security token 
* @param env, 
* @param is_encryption, 
* @param msg_ctx, 
* @param sct_id, 
* @param sct_id_type, 
* @param user_params, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
void* AXIS2_CALL
wsf_get_sct_callback_function(
	const axutil_env_t *env, 
	axis2_bool_t is_encryption, 
	axis2_msg_ctx_t* msg_ctx, 
	axis2_char_t *sct_id, 
	int sct_id_type,
	void* user_params)
{
	zval func, param1, param2, param3, param4, retval;
	zval *params[4];
	wsf_callback_args_t *callback_args = NULL;
	security_context_token_t *sct = NULL;

	TSRMLS_FETCH();
	params[0] = &param1;
	params[1] = &param2;
	params[2] = &param3;
	params[3] = &param4;

	callback_args = (wsf_callback_args_t*)user_params;
	/** return if the callback values are null */
	if(!callback_args || !callback_args->get_sct_callback_fn)
		return NULL;

	ZVAL_STRING (&func, callback_args->get_sct_callback_fn, 1);
	if(sct_id)
	{
		ZVAL_STRING (params[0], (char*)sct_id, 1);
		INIT_PZVAL(params[0]);
	}
	else
	{
		return NULL;
	}
	
	if(sct_id_type)
	{
		ZVAL_BOOL (params[1], sct_id_type);
	}
	else
	{
		ZVAL_NULL(params[1]);
	}
	INIT_PZVAL(params[1]);

	if(is_encryption)
	{
		ZVAL_BOOL (params[2], is_encryption);
	}
	else
	{
		ZVAL_NULL(params[2]);
	}
	INIT_PZVAL(params[2]);

	if(callback_args->callback_data)
	{
		ZVAL_ZVAL(params[3], callback_args->callback_data, 0, 1);
	}
	else
	{
		ZVAL_NULL(params[3]);
	}
	INIT_PZVAL(params[3]);
/** Parameter order (sct_id,sct_id_type,is_encryption, user_args) */
	if (call_user_function (EG (function_table), (zval **) NULL,
		&func, &retval, 4, params TSRMLS_CC) == SUCCESS) 
	{
		if (Z_TYPE(retval) == IS_STRING) 
		{
			sct = security_context_token_create(env);
			security_context_token_deserialize(sct, env, Z_STRVAL(retval));		
		}
	}
	return sct;
}

/**
* 
* @param env, 
* @param msg_ctx, 
* @param sct_id, 
* @param sct_id_type, 
* @param user_params, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
axis2_status_t AXIS2_CALL
wsf_delete_sct_callback_function(
	const axutil_env_t *env,
	axis2_msg_ctx_t* msg_ctx, 
	axis2_char_t *sct_id, 
	int sct_id_type,
	void* user_params)
{

	zval func, param1, param2, param3, retval;
	zval *params[3];
	wsf_callback_args_t *callback_args = NULL;
	axis2_status_t status = AXIS2_FAILURE;
	
	TSRMLS_FETCH();
	params[0] = &param1;
	params[1] = &param2;
	params[2] = &param3;

	callback_args = (wsf_callback_args_t*)user_params;
	
	ZVAL_STRING (&func, callback_args->delete_sct_callback_fn, 1);
	if(sct_id)
	{
		ZVAL_STRING (params[0], (char*)sct_id, 1);
		INIT_PZVAL(params[0]);
	}
	else
	{
		return AXIS2_FAILURE;
	}

	if(sct_id_type)
	{
		ZVAL_BOOL (params[1], sct_id_type);
	}
	else
	{
		ZVAL_NULL(params[1]);
	}
	INIT_PZVAL(params[1]);


	if(callback_args->callback_data)
	{
		ZVAL_ZVAL(params[2], callback_args->callback_data, 0, 1);
	}
	else
	{
		ZVAL_NULL(params[2]);
	}
	INIT_PZVAL(params[2]);
/** Parameter order (sct_id,sct_id_type, userargs) */
	if (call_user_function (EG (function_table), (zval **) NULL,
		&func, &retval, 3, params TSRMLS_CC) == SUCCESS) 
	{
		if (Z_TYPE (retval) == IS_BOOL) 
		{
			status = Z_BVAL(retval);
		}
	}
	return status;
}

axis2_status_t AXIS2_CALL
wsf_validate_sct_callback_function(
    const axutil_env_t *env,
    axiom_node_t *sct_node,
    axis2_msg_ctx_t *msg_ctx,
    void *user_params)
{
    return AXIS2_SUCCESS;
}


/** compares and returns the token reference */
char *
wsf_get_rampart_token_value(char *token_ref)
{
    if(strcmp(token_ref, WSF_ISSUER_SERIAL) == 0)
        return RP_REQUIRE_ISSUER_SERIAL_REFERENCE;
    if(strcmp(token_ref, WSF_KEYIDENTIFIER) == 0)
        return RP_REQUIRE_KEY_IDENTIFIRE_REFERENCE;
    if(strcmp(token_ref, WSF_EMBEDDEDTOKEN) == 0)
        return RP_REQUIRE_EMBEDDED_TOKEN_REFERENCE;
    if(strcmp(token_ref, WSF_THUMBPRINT) == 0)
        return RP_REQUIRE_THUMBPRINT_REFERENCE;
    else
        return NULL;
}
/**
* This function handles security context token related callback function setting to rampart.
* @param rampart_context, rampart context
* @param sec_token, WSSecurityToken instance
* @param policy,  WSPolicy instance
* @param env, axutil_environment
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
void
wsf_set_rampart_sct_options (
	rampart_context_t *rampart_context,
	HashTable *ht_token,
	const axutil_env_t * env TSRMLS_DC)
{
	wsf_callback_args_t *callback_args = NULL;
	zval **token_val = NULL;
	char *store_cb_fn = NULL;
	char *get_cb_fn = NULL;
	char *delete_cb_fn = NULL;

	if (zend_hash_find(ht_token, WSF_STORE_SCT_CALLBACK, sizeof(WSF_STORE_SCT_CALLBACK),
		(void **)&token_val) == SUCCESS && Z_TYPE_PP(token_val) == IS_STRING)
	{
		store_cb_fn = Z_STRVAL_PP(token_val);
		rampart_context_set_store_security_context_token_fn(rampart_context, env, 
			wsf_store_sct_callback_function);
		AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "store callback function name %s"
			, store_cb_fn);
	}
	if (zend_hash_find(ht_token, WSF_GET_SCT_CALLBACK, sizeof(WSF_GET_SCT_CALLBACK),
		(void **)&token_val) == SUCCESS && Z_TYPE_PP(token_val) == IS_STRING)
	{
		get_cb_fn = Z_STRVAL_PP(token_val);
		rampart_context_set_obtain_security_context_token_fn(rampart_context, env, 
			wsf_get_sct_callback_function);
		AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "get callback function name %s"
			, get_cb_fn);
	}
	if (zend_hash_find(ht_token, WSF_DELETE_SCT_CALLBACK, sizeof(WSF_DELETE_SCT_CALLBACK),
		(void **)&token_val) == SUCCESS && Z_TYPE_PP(token_val) == IS_STRING)
	{
		delete_cb_fn = Z_STRVAL_PP(token_val);
		rampart_context_set_delete_security_context_token_fn(rampart_context,env,
			wsf_delete_sct_callback_function);
		AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, WSF_PHP_LOG_PREFIX "delete callback function name %s"
			, delete_cb_fn);
	}
    rampart_context_set_validate_security_context_token_fn(rampart_context, env, 
            wsf_validate_sct_callback_function);

	if(store_cb_fn || get_cb_fn || delete_cb_fn)
	{
		callback_args = wsf_callback_args_create(env);
		callback_args->delete_sct_callback_fn = delete_cb_fn;
		callback_args->get_sct_callback_fn = get_cb_fn;
		callback_args->store_sct_callback_fn = store_cb_fn;
		if(zend_hash_find(ht_token, WSF_SCT_CALLBACK_ARGS, sizeof(WSF_SCT_CALLBACK_ARGS),
			(void**)&token_val) == SUCCESS)
		{
			callback_args->callback_data = *token_val;
		}
		rampart_context_set_security_context_token_user_params(rampart_context, env,(void *)callback_args);
	}
}

/** returns a pointer to an allocated callback args struct */
wsf_callback_args_t* 
wsf_callback_args_create(const axutil_env_t *env)
{
	wsf_callback_args_t *args = NULL;
	args = AXIS2_MALLOC(env->allocator, sizeof(wsf_callback_args_t));
	args->callback_data = NULL;
	args->get_sct_callback_fn = NULL;
	args->store_sct_callback_fn = NULL;
	args->delete_sct_callback_fn = NULL;
	args->callback_function = NULL;
	return args;
}
/** free callback args struct instance */
void 
wsf_callback_args_free(
	wsf_callback_args_t *callback_args,
	const axutil_env_t *env)
{
	if(callback_args)
	{
		AXIS2_FREE(env->allocator, callback_args);
	}
}
