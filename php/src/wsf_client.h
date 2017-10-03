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

#ifndef WSF_CLIENT_H
#define WSF_CLIENT_H

#include <axutil_env.h>
#include <axiom.h>
#include <axis2_op_client.h>
#include <axis2_svc_client.h>
#include <axis2_options.h>
#include <php.h>
#include "wsf_common.h"
#include <axiom_soap_body.h>
#include <TSRM.h>

#ifdef __cplusplus
extern "C"
	{
#endif

/**
* Set SOAP Security and HTTP Options 
* @param client_ht, WSClient options array
* @param msg_ht,  WSMessage options array
* @param env,	  axis2 environment
* @param options, axis2 options
* @param svc_client, axis2 service client
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
int wsf_client_set_options (
    HashTable * client_ht,
    HashTable * msg_ht,
    axutil_env_t * env,
    axis2_options_t * options,
    axis2_svc_client_t * svc_client,
    int *rest_enabled TSRMLS_DC);

/**
* Process WSMessage inputHeaders array and set to svc_client
* as axiom nodes
* @param env, 
* @param svc_client, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
int wsf_client_set_headers (
    const axutil_env_t * env,
    axis2_svc_client_t * svc_client,
    zval * msg TSRMLS_DC);


/**
* Process WSMessage and WSClient options and uses axis2_svc_client
* instance to handle the request. Both WSClient->request() and 
* WSClient->send() methods call this function
* @param this_ptr, WSClient instance
* @param param,    Argument of request(), send() methods, can be either
*                  WSMessage or a string
* @param return_value,
* @param env, 
* @param svc_client, 
*/
void wsf_client_do_request (
    zval * this_ptr,
    zval * param,
    zval * return_value,
    axutil_env_t * env,
    axis2_svc_client_t * svc_client,
    int is_oneway TSRMLS_DC);

/**
* Sets SSL options to axis2 options and enable SSL
* @param ht, WSClient options array 
* @param env, 
* @param options, axis2 options
*/
void wsf_client_enable_ssl (
    HashTable * ht,
    axutil_env_t * env,
    axis2_options_t *options TSRMLS_DC);

/**
 * Set the HTTP authentication headers
 * @param ht, WSClient options array
 * @param env, 
 * @param options, axis2 options
 */
void
wsf_client_set_http_auth_info(
        HashTable * client_ht,
        axutil_env_t * env,
        axis2_options_t *options TSRMLS_DC);

/**
 * Set the proxy authentication headers
 * @param ht, WSClient options array
 * @param env, 
 * @param options, axis2 options
 */
void
wsf_client_set_proxy_auth_info(
        HashTable * client_ht,
        axutil_env_t * env,
        axis2_options_t *options TSRMLS_DC);

/**
* Sets proxy params to svc_client
* @param ht, WSClient options array Hashtable
* @param env, 
*/
void wsf_client_enable_proxy (
    HashTable * ht,
    axutil_env_t * env, 
    axis2_svc_client_t * svc_client TSRMLS_DC);

/**
* Add WSClient options array values as properties to WSClient instance
* @param this_ptr, WSClient instance
*/
void wsf_client_add_properties (
    zval * this_ptr,
    HashTable * ht TSRMLS_DC);

/**
* Set "to" and soap action to client options
* @param client_ht, Hash table of WSClient options array
* @param msg_ht, HashTable of WSMessage options array
* @param env,    axis2 environment struct 
* @param client_options axis2_options struct
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
int wsf_client_set_endpoint_and_soap_action (
    HashTable * client_ht,
    HashTable * msg_ht,
    axutil_env_t * env,
    axis2_options_t * options TSRMLS_DC);

/**
* Set Terminate sequence message property
* @param env,  environment
* @param will_continue_sequence, will_continute_sequence property value
* @param rm_spec_version, RM Specification value
* @param sequence_key, sequence key value
* @param svc_client, service client object
*/
void wsf_client_send_terminate_sequence (
    axutil_env_t * env,
    int will_continue_sequence,
    int rm_spec_version,
    char *sequence_key,
    axis2_svc_client_t * svc_client);


#ifdef __cplusplus
}
#endif

#endif /* WSF_CLIENT_H */
