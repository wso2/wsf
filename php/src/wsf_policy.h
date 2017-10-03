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


#ifndef WSF_POLICY_H
#define WSF_POLICY_H

#include <axutil_env.h>
#include <axiom.h>
#include <axis2_svc_client.h>
#include <php.h>
#include "wsf_common.h"
#include <axiom_soap_body.h>
#include <TSRM.h>
#include <rampart_context.h>

/** Identifier for a policy object */
#define WSF_POLICY "Policy"

/** Input policy and output policy in case where the in policy and out policy are different */
#define WSF_IN_POLICY "inMessagePolicy"
#define WSF_OUT_POLICY "outMessagePolicy"

#define WSF_POLICY_MSG_LABEL_IN "in"
#define WSF_POLICY_MSG_LABEL_OUT "out"

#define WSF_POLICY_XML "policyXML"
#define WSF_POLICY_SECURITY "security"



#define WSF_SIGN "sign"
#define WSF_ENCRYPT "encrypt"
#define WSF_ALGORITHM "algorithmSuite"
#define WSF_LAYOUT "layOut"
#define WSF_TIMESTAMP "includeTimeStamp"
#define WSF_PROTECTION_ORDER "protectionOrder"
#define WSF_UT "useUsernameToken"
#define WSF_TOKEN_REFERENCE "securityTokenReference"
#define WSF_ENCRYPT_SIGNATURE "encryptSignature"
#define WSF_CUSTOM_TOKENS "customTokens"

#define WSF_DEFAULT_TOKEN_VAL "X509"
#define WSF_X509_TOKEN_VAL "X509Token"
#define WSF_INITIATOR_VAL "InitiatorToken"
#define WSF_INCLUDE_TOKEN_URI "http://schemas.xmlsoap.org/ws/2005/07/securitypolicy/IncludeToken/Always"
#define WSF_INCLUDE_TOKEN "IncludeToken"
#define WSF_RECIPIENT_TOKEN "RecipientToken"
#define WSF_USERNAMETOKEN "UsernameToken"
#define WSF_SIGNSUPPORTING_TOKEN "SignedSupportingTokens"
#define WSF_ALGO_SUITE "AlgorithmSuite"
#define WSF_ONLY_SIGN_HEADER_AND_BODY "OnlySignEntireHeadersAndBody"
#define WSF_ENCRYPTPARTS "EncryptedParts"
#define WSF_BODY "Body"
#define WSF_LAYOUT_VAL "Layout"
#define WSF_STRICT "Strict"
#define WSF_WSS10 "Wss10"


/** ws_security_token options */
#define WSF_USER "user"
#define WSF_CERTIFICATE "certificate"
#define WSF_PASSWORD_TYPE "passwordType"
#define WSF_PASSWORD "password"
#define WSF_PRIVATE_KEY "privateKey"
#define WSF_TTL "ttl"
#define WSF_CLOCK_SKEW_BUFFER "clockSkewBuffer"
#define WSF_RECEIVER_CERTIFICATE "receiverCertificate"
#define WSF_RAMPART_CONFIGURATION "RampartConfiguration"
#define WSF_DIGEST "Digest"
#define WSF_PLAINTEXT "plainText"

#define WSF_BASIC256 "Basic256"
#define WSF_BASIC192 "Basic192"
#define WSF_BASIC128 "Basic128"
#define WSF_TRIPLEDES "TripleDes"
#define WSF_BASIC256_RSA15 "Basic256Rsa15"
#define WSF_BASIC192_RSA15 "Basic192Rsa15"
#define WSF_BASIC128_RSA15 "Basic128Rsa15"
#define WSF_TRIPLEDES_RSA15 "TripleDesRsa15"

#define WSF_ISSUER_SERIAL "IssuerSerial"
#define WSF_KEYIDENTIFIER "KeyIdentifier"
#define WSF_EMBEDDEDTOKEN "EmbeddedToken"
#define WSF_THUMBPRINT "Thumbprint"
#define WSF_DIRECT "Direct"
#define WSF_ISSUER_SERIAL_VAL "MustSupportRefIssuerSerial"
#define WSF_KEYIDENTIFIER_VAL "MustSupportRefKeyIdentifier"
#define WSF_EMBEDDEDTOKEN_VAL "MustSupportRefEmbeddedToken"
#define WSF_THUMBPRINT_VAL "MustSupportRefThumbprint"


#define WSF_SECURITY_BINDING "binding"
#define WSF_SECURITY_BINDING_ASYMETRIC "Asymmetric"
#define WSF_SECURITY_BINDING_SYMETRIC "Symmetric"

#define WSF_SIGN_BEFORE "SignBeforeEncrypt"
#define WSF_ENCRYPT_BEFORE "EncryptBeforeSigning"

#define WSF_PKCS12_KEYSTORE "PKCS12KeyStore"

/** Callback functions */

/** Password callback function */
#define WSF_PASSWORD_CALLBACK "passwordCallback"
/** Password callback context data */
#define WSF_PASSWORD_CALLBACK_ARGS "passwordCallbackData"

/** Replay detection functionality */
#define WSF_REPLAY_DETECT_CALLBACK "replayDetectionCallback"
/**  Enable Replay detection option */
#define WSF_ENABLE_REPLAY_DETECT "enableReplayDetect"
/** Replay detect rampart parameter */
#define WSF_REPLAY_DETECT_NUMBER "5"
/** Replay detect context data */
#define WSF_REPLAY_DETECT_CALLBACK_ARGS "replayDetectionCallbackData"
/** This is the callback function for storing the security context data at PHP level */
#define WSF_STORE_SCT_CALLBACK		"storeSCTCallback"
/** user specific data to be passed to the callback */
#define WSF_GET_SCT_CALLBACK		"getSCTCallback"
/** user specific data to be passed to the callback */
#define WSF_DELETE_SCT_CALLBACK		"deleteSCTCallback"
/** user specific callback data to be passed to the callback */
#define WSF_SCT_CALLBACK_ARGS "SCTCallbackData"

#define WS_SCT_ID_GLOBAL 2
#define WS_SCT_ID_LOCAL  1


/** This struct will be used to pass data to the callbacks */
typedef struct wsf_callback_args
{
/** callback function name */
	char *callback_function;
/** callback function name for store sct */
	char *store_sct_callback_fn;
/** callback function name for get sct */
	char *get_sct_callback_fn;
/** callback function name for delete sct */
	char *delete_sct_callback_fn;
/** callback function args zval */
	zval *callback_data;
}wsf_callback_args_t;

/** returns a pointer to an allocated callback args struct */
wsf_callback_args_t* wsf_callback_args_create(const axutil_env_t *env);
/** free callback args struct instance */
void wsf_callback_args_free(wsf_callback_args_t *callback_args,const axutil_env_t *env);


/**
* @param this_ptr, This ptr is the WSPolicy instance
* @param property, The options array extracted from the constructor function
* @param env, axutil_env_t instance
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
int 
wsf_policy_set_policy_options (
    zval * this_ptr,
    zval * property,
    const axutil_env_t * env TSRMLS_DC);

/**
* 
* @param sec_token, An instance of WSSecurityToken
* @param policy, An instance of WSPolicy 
* @param env, environment 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
int 
wsf_policy_handle_client_security (
    zval * sec_token,
    zval * policy,
    axutil_env_t * env,
    axis2_svc_client_t * svc_client TSRMLS_DC);

/**
* Handle service side security.. Basically sets the options
* to the rampart context and engage to rampart module
* @param svc_info, wsf_svc_info_t instance
* @param env, axutil_env_t environment
* @param conf, The configuration context
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
int 
wsf_policy_handle_server_security (
    wsf_svc_info_t *svc_info,
    axutil_env_t * env,
    axis2_conf_t * conf TSRMLS_DC);

/**
* Set the processed options to the rampart context
* @param rampart_ctx, Rampart context
* @param sec_token, an instance WSSecurityToken instance
* @param policy, an instance of WSPolicy instance
* @param env, axutil_env_t environment
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
int
wsf_set_rampart_options (
    rampart_context_t* rampart_ctx,
    zval * sec_token,
    zval * policy,
    const axutil_env_t * env TSRMLS_DC);


/**
* 
* @param sec_token, An instance of WSSecurityToken object
* @param policy, An instance of WSPolicy object
* @param is_server_side, Here
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
axiom_node_t *
wsf_do_create_policy (
    zval *policy,
	axis2_bool_t is_server_side,
    axutil_env_t * env TSRMLS_DC);

/**
* This method handle operation level policies 
* @param ht_policies, 
* @param env, 
* @param svc, 
* @returns AXIS2_SUCCESS on success, AXIS2_FAILURE Otherwise
*/
axis2_status_t 
wsf_policy_handle_op_policies(
	HashTable *ht_policies,
	axutil_env_t *env,
	axis2_svc_t *svc TSRMLS_DC);



#endif /* WSF_POLICY_H */
