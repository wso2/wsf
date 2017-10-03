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


#ifndef WSF_WSDL_H
#define WSF_WSDL_H

#include <axutil_env.h>
#include <axiom.h>
#include <axis2_svc_client.h>
#include <php.h>
#include "wsf_common.h"
#include "wsf_policy.h"
#include <axiom_soap_body.h>
#include <TSRM.h>


/**
* 
* @param this_ptr, 
* @param fn_name, 
* @param fn_name_len, 
* @param arg_count, 
* @param args, 
* @param return_value, 
* @param TSRMLS_DC, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
void wsf_wsdl_create_dynamic_client(
   zval *this_ptr, 
   char *fn_name, 
   int fn_name_len, 
   int arg_count,
   zval *args, 
   zval *return_value, 
   axutil_env_t * env TSRMLS_DC);

/**
* 
* @param wsdl
* @param svc_info, 
* @param TSRMLS_DC, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/

void wsf_wsdl_process_service(
                wsf_svc_info_t *svc_info,
                const axutil_env_t *env TSRMLS_DC);

/**
* 
* @param wsdl_path, 
* @param svc_info, 
* @param TSRMLS_DC, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
void wsf_wsdl_set_sig_model(char* wsdl_path,
                            wsf_svc_info_t *svc_info,
                            const axutil_env_t *env TSRMLS_DC);
/**
 * extract the wsdl information to the current pointer
 * @param this_zval can be ether WSClient or WSServie
 * @parm env axis2 environment
 */
void wsf_wsdl_extract_wsdl_information_for_client(
    zval *this_zval,
        const axutil_env_t * env TSRMLS_DC);

/**
 * extract the wsdl information to the service
 * @param svc_info the wsf_svc_info_t instance
 * @parm env axis2 environment
 */
void wsf_wsdl_extract_wsdl_information_for_service(
        wsf_svc_info_t *svc_info,
        const axutil_env_t * env TSRMLS_DC);

/**
 * caching the wsdl info
 * @param svc_info the wsf_svc_info_t instance
 * @parm env axis2 environment
 */
void wsf_wsdl_cache_wsdl_info(wsf_svc_info_t *svc_info,
            const axutil_env_t *env TSRMLS_DC);

#define WSF_WSDL_FILE_PATH       "/scripts/dynamic_invocation/wsf_wsdl.php"
#define WSF_WSDL_XSLT_PATH            "/scripts/dynamic_invocation/xslt"

#define WSF_WSDL_WSCLIENT        "wsclient"
#define WSF_WSDL_ENDPOINT        "endpoint"
#define WSF_WSDL_CLASSMAP        "classmap"
#define WSF_WSDL_XSLT_LOCATION   "xslt_location"
#define WSF_WSDL_ARG_COUNT       "arg_count"
#define WSF_WSDL_ARG_ARRAY       "arg_array"
#define WSF_WSDL_INVOKE_FUNCTION "invoke_function"

#define WSF_WSDL_REQ_FUNCTION    "wsf_process_wsdl"
#define WSF_WSDL_EXTRACT_WSDL    "wsf_extract_wsdl_info"
#define WSF_WSDL_RES_FUNCTION    "wsf_process_response"
#define WSF_WSDL_MULTI_INTERFACES "is_multiple_interfaces"

#define WSF_WSDL_SERVICE_REQ_FUNCTION "wsf_process_wsdl_for_service"

#define WSF_WSDL_ENDPOINT_URI    "endpoint_uri"
#define WSF_WSDL_BINDING_DETAILS "binding_details"
#define WSF_WSDL_WSA             "wsa"
#define WSF_WSDL_SOAP            "soap"
#define WSF_WSDL_SOAP_VERSION    "soap_version"
#define WSF_WSDL_REQ_PAYLOAD     "request_payload"
#define WSF_WSDL_INPUT_HEADERS   "input_headers"
#define WSF_WSDL_OUTPUT_HEADERS  "output_headers"
#define WSF_WSDL_POLICY_NODE     "policy_node"
#define WSF_WSDL_POLICIES        "policies"
#define WSF_WSDL_SIG_MODEL       "sig_model_string"
#define WSF_WSDL_RES_SIG_MODEL   "response_sig_model"
#define WSF_WSDL_IS_WSDL_11      "is_wsdl_11"
#define WSF_WSDL_ARGS            "args"
#define WSF_WSDL_ATTACHMENT_MAP  "attachment_map"
#define WSF_WSDL_RES_PAYLOAD     "response_payload"

#define WSF_WSDL_OP_POLICY       "operation_policy"
#define WSF_WSDL_IN_POLICY       "input_policy"
#define WSF_WSDL_OUT_POLICY      "output_policy"

#define WSF_WSDL_ENV_UTF         "UTF-8"
#define WSF_WSDL_ENV_1_0         "1.0"

#define WSF_WSDL_DOM          "wsdl_dom"

#define WSF_WSDL_RESPONSE_PAYLOAD_PARAM "response_payload_param"
#define WSF_WSDL_RESPONSE_HEADER_PARAM "response_header_param"

#define WSF_WSDL_CHECK_FUNCTION "wsf_wsdl_check"

#define WSF_WSDL_DYNAMIC_INVOC_SCRIPT "wsf_wsdl.php"

#endif /* WSF_WSDL_H */

