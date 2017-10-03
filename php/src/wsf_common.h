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

#ifndef WSF_COMMON_H
#define WSF_COMMON_H

#ifdef PHP_WIN32
#define WSF_PHP_API __declspec(dllexport)
#else
#define WSF_PHP_API
#endif

#ifdef PHP_WIN32
#define WSF_CALL	__stdcall
#else
#define WSF_CALL
#endif
#include <axis2_svc.h>
#include <axis2_msg_recv.h>
#include <axiom.h>
#include <sandesha2_constants.h>
#include <php.h>


/* {{{ axis2 object types */
typedef enum ws_object_types_t
{
    WSF_NONE,
    WSF_SVC_CLIENT,
    WSF_SVC,
    WSF_FAULT,
    WSF_PARAM,
    WSF_OBJECT_LAST
} ws_object_types_t;
/* }}} */

/** using input */
typedef enum ws_input_types
{
    WSF_USING_STRING,
    WSF_USING_MSG,
    WSF_USING_INCORRECT_INPUT
} ws_input_types_t;

/** options for services array */

#define WSF_PHP_LOG_PREFIX      "[WSF/PHP] "

#define WSF_MIXED "MIXED"
#define WSF_WSMESSAGE "WSMESSAGE"

#define WSF_MESSAGE_STR "str"
#define WSF_MESSAGE_REQUEST_SOAP_ENVELOPE_STR "soapEnvelopeStr"

#define WSF_ACTIONS      "actions"
#define WSF_OPERATIONS   "operations"
#define WSF_OP_MEP       "opMEP"
#define WSF_OP_PARAMS    "opParams"
#define WSF_WSDL         "wsdl"
#define WSF_XSD			 "xsd"
#define WSF_OMIT_WSDL    "omitProcesswsdl"
#define WSF_CLASSMAP     "classmap"
#define WSF_CLASSES      "classes"
#define WSF_CONS_ARGS    "args"
#define WSF_SERVICE_NAME "serviceName"
#define WSF_PORT_NAME    "portName"

#define WSF_ANNOTATIONS "annotations"
#define WSF_WSDL_PROCESS_IN_MSG "wsf_wsdl_process_in_msg"

/** REST */
#define WSF_OP_TO_URL_MAP "RESTMapping"
#define WSF_REST_LOCATION "RESTLocation"
#define WSF_REST_CONTENT_TYPE "contentType"

/** Options array options */

#define WSF_IN_ONLY "IN_ONLY"
#define WSF_IN_OUT "IN_OUT"

#define WSF_TO				"to"    /** endpoint uri */
#define WSF_TRANSPORT_URL   "transportURL"  /** Transport URL */

#define WSF_ADDR_MUST_UNDERSTAND "WSAMustUnderstand" /** must understand property */

#define WSF_SWA     		"swa"
#define WSF_USE_WSA         "useWSA"
#define WSF_ACTION			"action"        /** addressing options */
#define WSF_FROM		    "from"
#define WSF_REPLY_TO	    "replyTo"
#define WSF_FAULT_TO	    "faultTo"
/** additional endpoint reference properties */
#define WSF_WSA_ADDRESS "address"
#define WSF_WSA_REFERENCE_PARAMETERS "referenceParameters"
#define WSF_WSA_METADATA "metadata"

#define WSF_SOAP_ACTION     "SOAPAction"

#define WSF_USES_XOP			"usesXOP"      /** MTOM XOP */
#define WSF_DEFAULT_ATTACHEMENT_CONTENT_TYPE "defaultAttachmentContentType"
#define WSF_ATTACHMENTS 			"attachments"
#define WSF_CID2CONTENT_TYPE 	"cid2contentType"
#define WSF_RESPONSE_XOP 		"responseXOP"
#define WSF_REQUEST_XOP  		"requestXOP"
#define WSF_USE_MTOM     		"useMTOM"
#define WSF_TIMEOUT              "timeout"

/** Reliable Messaging */
#define WSF_WILL_CONTINUE_SEQUENCE 	"willContinueSequence"
#define WSF_LAST_MESSAGE 		    "lastMessage"
#define WSF_RELIABLE			        "reliable"
#define WSF_SEQUENCE_EXPIRY_TIME 	"sequenceExpiryTime"
#define WSF_SEQUENCE_KEY      		"sequenceKey"

/** protocol */
#define WSF_USE_SOAP			    "useSOAP"
#define WSF_HTTP_METHOD 			"HTTPMethod"


/** SSL certificate */
#define WSF_SERVER_CERT 			"CACert"
#define WSF_CLIENT_CERT 			"clientCert"
#define WSF_PASSPHRASE  			"passphrase"

/** wsdl mode */
#define WSF_BINDING_STYLE 		"bindingStyle"
#define WSF_WSDL          		"wsdl"
#define WSF_CACHE_WSDL           "cacheWSDL"
#define WSF_CLASSMAP             "classmap"

/** http authentication */
#define WSF_HTTP_AUTH_USERNAME      "httpAuthUsername"
#define WSF_HTTP_AUTH_PASSWORD      "httpAuthPassword"
#define WSF_HTTP_AUTH_TYPE          "httpAuthType"
#define WSF_PHP_AUTH_TYPE_BASIC			"Basic"

#define WSF_WSDL_CA_CERT			"CACert"
#define WSF_WSDL_CLIENT_CERT		"clientCert"
#define WSF_WSDL_PASSPHRASE			"passphrase"

/** proxy authentication */
#define WSF_PROXY_AUTH_USERNAME      "proxyAuthUsername"
#define WSF_PROXY_AUTH_PASSWORD      "proxyAuthPassword"
#define WSF_PROXY_AUTH_TYPE          "proxyAuthType"

/** soap fault */
#define WSF_FAULT_REASON 		"Reason"
#define WSF_FAULT_ROLE   		"Role"
#define WSF_FAULT_TEXT   		"text"
#define WSF_FAULT_CODE   		"Code"
#define WSF_FAULT_CODE_NS        "codens"
#define WSF_FAULT_SUBCODE 		"Subcode"
#define WSF_FAULT_DETAIL 		"Detail"
#define WSF_FAULT_STR "str"
#define WSF_FAULT_NAME "_name"

/** http status code, required when handling rest style messages */
#define WSF_HTTP_STATUS_CODE "httpStatusCode"


/** header options */
#define WSF_INPUT_HEADERS        "inputHeaders"
#define WSF_OUTPUT_HEADERS       "outputHeaders"
#define WSF_HEADER_PREFIX        "prefix"
#define WSF_HEADER_NS    		"ns"
#define WSF_HEADER_LOCALNAME 	"name"
#define WSF_HEADER_DATA     		"data"
#define WSF_HEADER_STR           "str"
#define WSF_HEADER_ROLE     		"role"
#define WSF_HEADER_ACTOR         "actor"
#define WSF_HEADER_MUST_UNDERSTAND 	"mustUnderstand"
#define WSF_HEADER_PREFIX        "prefix"

/** http Headers */

#define WSF_HTTP_HEADERS "httpHeaders"

/** security policy */
#define WSF_POLICY_NAME			"policy"
#define WSF_SECURITY_TOKEN 		"securityToken"
#define WSF_OP_POLICIES         "opPolicies"

#define WSF_PROXY_HOST           "proxyHost"
#define WSF_PROXY_PORT           "proxyPort"
/** end options */

#define WSF_MSG_PAYLOAD_STR 		"str"

/** soap header attributes */

#define WSF_SOAP_ROLE_NONE			        1
#define WSF_SOAP_ROLE_NEXT			        2
#define WSF_SOAP_ROLE_ULTIMATE_RECEIVER		3

#define WSF_SOAP_ROLE_NONE_URI "http://www.w3.org/2003/05/soap-envelope/role/none"
#define WSF_SOAP_ROLE_NEXT_URI "http://www.w3.org/2003/05/soap-envelope/role/next"
#define WSF_SOAP_ROLE_ULTIMATE_RECEIVER_URI "http://www.w3.org/2003/05/soap-envelope/role/next"

#define WSF_SOAP_1_1_NAMESPACE_URI "http://schemas.xmlsoap.org/soap/envelope/"
#define WSF_SOAP_1_2_NAMESPACE_URI "http://www.w3.org/2003/05/soap-envelope"

/** sandesha2 */

#define WSF_SANDESHA2_CLIENT_ACKS_TO "Sandesha2AcksTo"
#define WSF_SANDESHA2_CLIENT_LAST_MESSAGE "Sandesha2LastMessage"
#define WSF_SANDESHA2_CLIENT_OFFERED_SEQ_ID "Sandesha2OfferedSequenceId"
#define WSF_SANDESHA2_CLIENT_DEBUG_MODE "Sandesha2DebugMode"
#define WSF_SANDESHA2_CLIENT_SEQ_KEY "Sandesha2SequenceKey"
#define WSF_SANDESHA2_CLIENT_MESSAGE_NUMBER "Sandesha2MessageNumber"
#define WSF_SANDESHA2_CLIENT_RM_SPEC_VERSION "Sandesha2RMSpecVersion"
#define WSF_SANDESHA2_CLIENT_DUMMY_MESSAGE "Sandesha2DummyMessage"
#define WSF_SANDESHA2_INACTIVITY_TIMEOUT "InactivityTimeout"
#define WSF_SANDHSHA2_LAST_MESSAGE "Sandesha2LastMessage"
#define WSF_SANDHSHA2_SANDESHA2_OFFERED_SEQUENCE_ID "Sandesha2OfferedSequenceId"

#define WSF_RM_VERSION_1_0_STR SANDESHA2_SPEC_VERSION_1_0
#define WSF_RM_VERSION_1_0  1
#define WSF_RM_VERSION_1_1_STR SANDESHA2_SPEC_VERSION_1_1
#define WSF_RM_VERSION_1_1  2

#define WSF_RM_RESPONSE_TIMEOUT "responseTimeout"
#define WSF_RM_DEFAULT_RESPONSE_TIMEOUT "12"

#define WSF_SANDESHA2_DB "sandesha2_db"

/** Addressing options */

#define WSF_ADDRESSING_VERSION_1_0 "1.0"
#define WSF_ADDRESSING_SUBMISSION "submission"

/** SOAP Constants */

#define WSF_SOAP_VERSION_1_1 "1.1"
#define WSF_SOAP_VERSION_1_2 "1.2"

/** module names */
#define WSF_MODULE_ADDRESSING "addressing"
#define WSF_MODULE_SECURITY   "rampart"
#define WSF_MODULE_RM         "sandesha2"
#define WSF_MODULE_SC         "rahas"

#define WSF_SVC_INFO "WSFPHPSvcInfo"

#define WSF_REQ_INFO "WSFPHPRequestInfo"

#define WSF_MAGIC_FUNC_CALL "__call"

/** HTTP Methods */
#define WSF_HTTP_POST "POST"
#define WSF_HTTP_GET  "GET"
#define WSF_HTTP_PUT  "PUT"
#define WSF_WSDL "wsdl"
#define WSF_WSDL2 "wsdl2"
#define WSF_LOCALHOST "localhost"
#define WSF_PROTOCOL_HTTP "HTTP"

#define WSF_WSDL_1_1 "wsdl1.1"
#define WSF_WSDL_2_0 "wsdl2.0"

#define WSF_STYLE_DOCLIT "doclit"
#define WSF_WSDL_GENERATION_FUNCTION "ws_generate_wsdl"
#define WSF_SCRIPT_FILENAME "wsf.php"
#define WSF_DEFAULT_PORT 80
#define WSF_SWA "swa"

/** service context properties */
#define WSF_SVC_PROP_SIG_MODEL_STR "sig_model_string"
#define WSF_SVC_PROP_WSDL_STR "wsf_wsdl_string"
#define WSF_SVC_PROP_IS_WSDL_11 "wsf_is_wsdl_11"
#define WSF_SVC_PROP_IS_MULTI_INTERFACES "wsf_is_multi_interfaces"

/** this option is read from the WSService constructor to engage rahas*/
#define WSF_USE_OWN_STS "useOwnSTS"
/** security policy */

typedef struct wsf_svc_info
{
	/** corresponding axis2 service*/
  axis2_svc_t *svc;
  /** service context */
  axis2_svc_ctx_t *svc_ctx;
  /** message receiver object state less and common to all */
  axis2_msg_recv_t *msg_recv;
  /** php worker object */
  struct wsf_worker_t *php_worker;
  /** operations to actions mapping */
  axutil_hash_t *ops_to_actions;
  /** operations to functions mapping */
  axutil_hash_t *ops_to_functions;
  /** modules to be engaged */
  axutil_array_list_t *modules_to_engage;
  /** class operations */
  axutil_hash_t *ops_to_classes;
  /** class arguments */
  zval *class_args;
  /** op params */
  HashTable *ht_op_params;
  /** request xop value */
  int request_xop;
  /** use mtom value */
  int use_mtom;
  /** use wsa - for wsdl generation*/
  int use_wsa;
  /** enable swa value */
  int enable_swa;
  /** ws policy object for service level */
  zval *policy;
  /** ws operations to policy array */
  HashTable *ht_op_policies;
  /** ws security token object */
  zval *security_token;
  /** service name ( this can be a user generated one or a provided one */
  char *svc_name;
  /** port name */
  char *port_name;
  /** flag to indicate whether the svc name is a generates one or not */ 
  int generated_svc_name;
  /** sig model string */
  char *sig_model_string;
  /** wsdl string */
  char *wsdl_string;
  /** is wsdl version 11 */
  int is_wsdl_11;
  /** is multi interfaces */
  int is_multi_interfaces;
  /** class map for wsdl generation */	
  zval *class_map;
  /** class map for wsdl generation */
  zval *wsdl_gen_class_map;
  /** wsdl annotations */
  zval *wsdl_gen_annotations;
  /** actions to wsdl generation*/
  zval *wsdl_gen_actions;
  /** wsdl endpoint */
  char *wsdl;
  /**http://<domain>/<..*.php>/<loc_str>/ */
  char *loc_str;	
  /** cache the wsdl */
  int cache_wsdl;
  /** omit the wsdl */
  int omit_wsdl;
  /** http_auth_username */
  char *auth_user;
  /** http_auth_password */
  char *auth_password;
  /** http_auth_type */
  char* auth_type;
 /** location where the cache attachments will be stored */
  char *attachment_cache_dir;
  /** whether to enable attachment caching or not */
  int enable_attachment_caching;
}
wsf_svc_info_t;


typedef struct wsf_request_info
{
    long svr_port;
    char *svr_name;
    char *http_protocol;
	char *accept;
	char *accept_language;
	char *accept_encoding;
	char *accept_charset;

	char *remote_address;
    char *soap_action;
    char *request_uri;
    char *query_string;
    long content_length;
    char *content_type;
    char *request_method;
    void *request_data;
    int request_data_length;
    
	char *transfer_encoding;
	
	/** REST parameters */
	axutil_array_list_t *param_keys;
	axutil_array_list_t *param_values;

} wsf_request_info_t;

typedef struct wsf_response_info
{
	void *response_data;
	int  response_length;
	char *http_protocol;
	int  http_status_code;
	char *http_status_code_name;
	char *content_type;
}wsf_response_info_t;

/** functions */

#endif /* WS_COMMON_H */
