<?php
/*
 * Copyright (c) 2005-2010 WSO2, Inc. http://wso2.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * This function is used to load a wsdl from a url requiring HTTP Digest authentication.
 * @param wsdl_url  WSDL url
 * @param username username for the digest authentication
 * @param password password for the digest authentication
 * @returns On success returns the obtained wsdl string. otherwise NULL 
 */

/**
 * This function is used to load a wsdl from a url requiring HTTP digest authentication.
 * @param wsdl_url  WSDL url
 * @param username username for the digest authentication
 * @param password password for the digest authentication
 * @returns On success returns the obtained wsdl string. otherwise NULL 
 */
function wsf_get_wsdl_with_http_auth_digest($wsdl_url, $username, $password)
{
	preg_match('@^(?:http://)?([^/]+)@i', $wsdl_url, $matches);
    $host = $matches[1];
    $file = strstr($wsdl_url, $host);

    if (!$fp=fsockopen($host,80, $errno, $errstr, 15))
        return false;
        
    //first do the non-authenticated header so that the server
    //sends back a 401 error containing its nonce and opaque
    $out = "GET /$file HTTP/1.1\r\n";
       $out .= "Host: $host\r\n";
       $out .= "Connection: Close\r\n\r\n";

     fwrite($fp, $out);

    //read the reply and look for the WWW-Authenticate element
    while (!feof($fp))
    {
        $line=fgets($fp, 512);
        
        if (strpos($line,"WWW-Authenticate:")!==false)
            $authline=trim(substr($line,18));
    }
    
    fclose($fp);
       
    //split up the WWW-Authenticate string to find digest-realm,nonce and opaque values
    //if qop value is presented as a comma-seperated list (e.g auth,auth-int) then it won't be retrieved correctly
    //but that doesn't matter because going to use 'auth' anyway
    $authlinearr=explode(",",$authline);
    $autharr=array();
    
    foreach ($authlinearr as $el)
    {
        $elarr=explode("=",$el);
        //the substr here is used to remove the double quotes from the values
        $autharr[trim($elarr[0])]=substr($elarr[1],1,strlen($elarr[1])-2);
    }
    
    foreach ($autharr as $k=>$v)
        echo("$k ==> $v\r\n");
    
    //these are all the vals required from the server
    $nonce=$autharr['nonce'];
    $opaque=$autharr['opaque'];
    $drealm=$autharr['Digest realm'];
    
    //client nonce can be anything since this authentication session is not going to be persistent
    $cnonce="wso2wsfphp123456789";
    
    //calculate the hashes of A1 and A2 as described in RFC 2617
    $a1="$username:$drealm:$password";$a2="GET:/$file";
    $ha1=md5($a1);$ha2=md5($a2);
    
    //calculate the response hash as described in RFC 2617
    $concat = $ha1.':'.$nonce.':00000001:'.$cnonce.':auth:'.$ha2;
    $response=md5($concat);
    
    //put together the Authorization Request Header
    $out = "GET /$file HTTP/1.1\r\n";
    $out .= "Host: $host\r\n";
    $out .= "Connection: Close\r\n";
    $out .= "Authorization: Digest username=\"$username\", realm=\"$drealm\", qop=\"auth\", algorithm=\"MD5\", uri=\"/$file\", nonce=\"$nonce\", nc=00000001, cnonce=\"$cnonce\", opaque=\"$opaque\", response=\"$response\"\r\n\r\n";
    
    if (!$fp=fsockopen($host,80, $errno, $errstr, 15))
        return false;
    
    fwrite($fp, $out);
    
    while (!feof($fp))
    {
        $str.=fgets($fp, 512);
    }
    
    fclose($fp);
    return $str;
}

/**
 * This function is used to load a wsdl from a url requiring HTTP Basic authentication.
 * @param wsdl_url  WSDL url
 * @param username username for the digest authentication
 * @param password password for the digest authentication
 * @returns On success returns the obtained wsdl string. otherwise NULL 
 */
function wsf_get_wsdl_with_http_auth_basic($wsdl_url, $username, $password)
{
	$cred = sprintf('Authorization: Basic %s', base64_encode($username.':'.$password));
	$options = array(
		'http'=>array(
		'method'=>'GET',
		'header'=>$cred));
	$ctx = stream_context_create($options);
	$str = file_get_contents($wsdl_url, false, $ctx);
	if(empty($str))
	{
		throw new WSFault("Receiver","Could not load WSDL from WSDL url $wsdl_url");
		return NULL;
	}
	return $str;
 }

/**
 * This function is used to load a wsdl from a url, It handles the http authentication scenarios 
 * if necessary.
 * @param wsdl_url  WSDL url
 * @param username username for the digest authentication
 * @param password password for the digest authentication
 * @returns On success returns the obtained wsdl string. otherwise NULL 
 */
function wsf_get_wsdl_str_from_url($wsdl_url,$user_parameters)
{
	require_once('dynamic_invocation/wsf_wsdl_consts.php');
	$username = NULL;
	$password  = NULL;
	$password_type = NULL;
	/** https client auth */
	$cacert = NULL;
	$clientcert = NULL;
	$passphrase = NULL;
	$options = NULL;
	if(array_key_exists(WSF_HTTP_AUTH_USERNAME,$user_parameters))
	{
		 $username= $user_parameters[WSF_HTTP_AUTH_USERNAME];
		
	}
	if(array_key_exists(WSF_HTTP_AUTH_PASSWORD, $user_parameters))
	{
		$password = $user_parameters[WSF_HTTP_AUTH_PASSWORD];
			
	}
	if(array_key_exists(WSF_HTTP_AUTH_TYPE, $user_parameters))
	{
		$password_type = $user_parameters[WSF_HTTP_AUTH_TYPE];	
	}
	if(array_key_exists(WSF_WSDL_CA_CERT, $user_parameters)){
		$cacert = $user_parameters[WSF_WSDL_CA_CERT];
	}
	if(array_key_exists(WSF_WSDL_CLIENT_CERT, $user_parameters)){
		$clientcert = $user_parameters[WSF_WSDL_CLIENT_CERT];
	}
	if(array_key_exists(WSF_WSDL_PASSPHRASE, $user_parameters)){
		$passphrase = $user_parameters[WSF_WSDL_PASSPHRASE];
	}
	
	if(!is_null($username) && !is_null($password) && !is_null($password_type))
	{
		
		if(strcmp($password_type,"Basic") == 0)
		{
			return wsf_get_wsdl_with_http_auth_basic($wsdl_url, $username, $password);
		}
		else if(strcmp($password_type,"Digest") == 0)
		{
			return wsf_get_wsdl_with_http_auth_digest($wsdl_url, $username, $password);
		}
	}else
	{
		$stream_ctx = NULL;
		if(!is_null($cacert) && !is_null($clientcert)){
			$stream_ctx = stream_context_create(
							array("ssl"=>array(
								'cafile' => $cacert,
								'local_cert'=>$clientcert,
								'verify_peer'=>false,
							)));
		}
		$result = file_get_contents($wsdl_url, 0 ,$stream_ctx);	
		if(empty($result))
		{
			throw new WSFault("Receiver","Could not Load WSDL from $wsdl_url");
			return NULL;
		}
		return $result;
	}    	
	return NULL;	
}

/**
 * This function is called from call_user_function in C level.
 * Once this function is called, it will fill in the WSDL information, 
 * such as sig model, and the wsdl string
 * into an array and return that array.
 * @param array $user_parameters the details of WSDL endpoint, service address
 * and class map
 * @param array $function_parameters details of the invoked function
 * @return array $return_value array of details to be passed to C level
 */
function wsf_extract_wsdl_info($wsdata) {
    require_once('dynamic_invocation/wsf_wsdl_consts.php');
    require_once('dynamic_invocation/wsf_wsdl_util.php');
    
    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "calling for the wsdl info");

    $user_parameters = $wsdata->WSDL_UserParameters;
    
    $is_wsdl_11 = TRUE;
    $wsdl_11_dom = NULL;
    
    /** https client auth */
	$cacert = NULL;
	$clientcert = NULL;
	$passphrase = NULL;
	$stream_ctx = NULL;

	if(array_key_exists(WSF_WSDL_CA_CERT, $user_parameters)){
		        $cacert = $user_parameters[WSF_WSDL_CA_CERT];
	}
	if(array_key_exists(WSF_WSDL_CLIENT_CERT, $user_parameters)){
		        $clientcert = $user_parameters[WSF_WSDL_CLIENT_CERT];
	}
	if(array_key_exists(WSF_WSDL_PASSPHRASE, $user_parameters)){
		        $passphrase = $user_parameters[WSF_WSDL_PASSPHRASE];
	}
	/** wsf_get_wsdl_function using an stream context object */
	
	if(!is_null($cacert) && !is_null($clientcert)){
			$stream_ctx = stream_context_create(
							array("ssl"=>array(
								'cafile' => $cacert,
								'local_cert'=>$clientcert,
								'verify_peer'=>false,
							)));
	}
	
    $return_value = array();

    /* retrieving the user parameters */
    $service = NULL;
    $port = NULL;
    if(array_key_exists(WSF_SERVICE_NAME, $user_parameters)) {
        $service = $user_parameters[WSF_SERVICE_NAME];
    }
    if(array_key_exists(WSF_PORT_NAME, $user_parameters)) {
        $port = $user_parameters[WSF_PORT_NAME];
    }
    $wsdl_location = $user_parameters[WSF_WSDL];


    $wsdl_dom = new DomDocument();
    $sig_model_dom  = new DOMDocument();

    $sig_model_dom->preserveWhiteSpace = FALSE;
    $wsdl_dom->preserveWhiteSpace = FALSE;
       
    if(!$wsdl_location) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "WSDL location uri is not found");
        return "WSDL location uri is not found";
    }
    
    $is_multiple_interfaces = FALSE;
    
    
    // Load WSDL as DOM
    $wsdl_str = wsf_get_wsdl_str_from_url($wsdl_location ,$user_parameters);
    
    if(is_null($wsdl_str))
    {
    	ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "Reading WSDL from {$wsdl_location} failed ");
        return "Reading WSDL from {$wsdl_location} failed.";
    }
    if(!$wsdl_dom->loadXML($wsdl_str)) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "WSDL {$wsdl_location} could not be loaded");
        return "WSDL {$wsdl_location} could not be loaded.";
    }
   

    $wsdl_dom->preserveWhiteSpace = FALSE;
    /* changing code for processing mutiple port types in wsdl 1.1 */
    $is_multiple_interfaces = wsf_is_mutiple_port_types($wsdl_dom);

    if ($is_multiple_interfaces == FALSE) {
        // this will return the wsdl2.0 dom and the information 
        // about is_wsdl_11 and wsdl_11_dom + bundles the imports, includes
        $wsdl_dom = wsf_get_wsdl_dom($wsdl_dom, $wsdl_location, $is_wsdl_11, $wsdl_11_dom, $stream_ctx);
        
        if(!$wsdl_dom) {
            ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "Error creating WSDL Dom Document,".
                    "Please check whether the wsdl is an valid XML");
            return "Error creating WSDL Dom Document".
                    "Please check whether the wsdl is an valid XML";
        }
        
        $sig_model_dom = wsf_get_sig_model_dom($wsdl_dom);
    }
    else {
        // this will return the wsdl2.0 dom and the information
        // about is_wsdl_11 and wsdl_11_dom + bundles the imports, includes
        $wsdl_dom = wsf_get_wsdl_dom($wsdl_dom, $wsdl_location, $is_wsdl_11, $wsdl_11_dom, $stream_ctx);
        $sig_model_dom = wsf_process_multiple_interfaces($wsdl_dom);
    }
    
    if(!$sig_model_dom) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "Error creating intermediate sig model");
        return "Error creating intermediate model";
    }

    $sig_model_string = $sig_model_dom->saveXML();

    if($is_wsdl_11 == TRUE && $wsdl_11_dom) {
        $wsdl_dom = $wsdl_11_dom;
    }

    $wsdl_dom_string = $wsdl_dom->saveXML();
    
    $return_value = array(WSF_IS_WSDL_11 => $is_wsdl_11,
                          WSF_SIG_MODEL_STRING => $sig_model_string,
                          WSF_WSDL_DOM => $wsdl_dom_string,
                          WSF_IS_MULTI_INTERFACES => $is_multiple_interfaces
                          );
    return $return_value;

}

/**
 * This function is called from call_user_function in C level.
 * It will provide the sig model information and the policies
 * and return the array of response payload + attachments.
 * @param array $user_parameters the details of WSDL endpoint, service address
 * and class map
 * @param array $function_parameters the arguments passed with the functions..
 * @param string $wsdl_dom_string the original wsdl as a strng
 * @param boolean $is_wsdl_11 whether this is wsdl1.1. or not
 * @param string $sig_model_string the sig model as a string
 * @return array $return_value array of details to be passed to C level
 */
function wsf_process_wsdl($wsdata)
{
    require_once('dynamic_invocation/wsf_wsdl_consts.php');
    require_once('dynamic_invocation/wsf_wsdl_util.php');
    require_once('dynamic_invocation/wsf_wsdl_client.php');
    
    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "wsf_process_wsdl called");

    $user_parameters = $wsdata->WSDL_UserParameters; 
    $function_parameters = $wsdata->WSDL_FunctionParameters;
    $wsdl_dom_string = $wsdata->WSDL_SigModelDom;
    $is_wsdl_11 = $wsdata->WSDL_IsWSDL11;
    $sig_model_string = $wsdata->WSDL_SigModelString;
    $is_multiple_interfaces = $wsdata->WSDL_IsMultipleInterfaces;
    
    
    $return_value = array();
    $policy_array = array();
    $binding_array = array();

    $is_doc_lit = FALSE;
    $is_rpc_enc = FALSE;
    $use_mtom = TRUE; //default to on

    

    /* retrieving the user parameters */
    $service = NULL;
    $port = NULL;
    if(array_key_exists(WSF_SERVICE_NAME, $user_parameters)) {
        $service = $user_parameters[WSF_SERVICE_NAME];
    }
    if(array_key_exists(WSF_PORT_NAME, $user_parameters)) {
        $port = $user_parameters[WSF_PORT_NAME];
    }
    if(array_key_exists(WSF_USE_MTOM, $user_parameters)) {
        $use_mtom = $user_parameters[WSF_USE_MTOM];
    }

    /* class map is an optional parameter */
    if (isset($user_parameters[WSF_CLASSMAP])) {
        $class_map = $user_parameters[WSF_CLASSMAP];
    }
    else {
        $class_map = NULL;
    }

    /* wsf endpoint is an optional parameter */
    if(array_key_exists(WSF_ENDPOINT, $user_parameters) &&
            isset($user_parameters[WSF_ENDPOINT])) {
        $endpoint_address = $user_parameters[WSF_ENDPOINT];
    }
    else {
        $endpoint_address = NULL;
    }

    /* extracting the information on function parameters */
    $operation_name = $function_parameters[WSF_INVOKE_FUNCTION];
    $arg_count = $function_parameters[WSF_ARG_COUNT];
    $arguments = $function_parameters[WSF_ARG_ARRAY];

    // we load the wsdl dom and the sig model dom from the stored strings
    $wsdl_dom = new DomDocument();
    $sig_model_dom  = new DOMDocument();

    $sig_model_dom->preserveWhiteSpace = FALSE;
    $wsdl_dom->preserveWhiteSpace = FALSE;
    
    $sig_model_dom->loadXML($sig_model_string);
    if(!$sig_model_dom) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR,
                "Error retrieving the intermediate sig model");
        return "Error retrieving the intermediate model";
    }
    $wsdl_dom->loadXML($wsdl_dom_string);
    if(!$wsdl_dom) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, 
            "Error retrieving the wsdl string to process the request message");
        return "Error retrieving the wsdl string to process the request message";
    }

    $is_multiple_interfaces = wsf_is_mutiple_port_types($wsdl_dom);

    if($is_wsdl_11) {
        $wsdl_11_dom = $wsdl_dom;
    }

    /* endpoint_address is used for just to parse back to the c code: */
    if(!$endpoint_address) {
        $endpoint_address = wsf_get_endpoint_address($sig_model_dom);
    }
    else{
        $multiple_ep = TRUE;
        $multiple_ep = wsf_is_multiple_endpoints($sig_model_dom);
        if(!$multiple_ep) {
            $endpoint_address = wsf_get_endpoint_address($sig_model_dom);
        }
    }
    
    /* for retrieve binding we have to go to the old WSDL */
    if ($is_wsdl_11 &&  $wsdl_11_dom != NULL) {
        $binding_node = wsf_get_binding($wsdl_11_dom, $service, $port, TRUE);
        if(!$binding_node) {
            ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "binding node not found");
            return  NULL;
        }
        $policy_array = wsf_get_all_policies($wsdl_11_dom, $binding_node, $operation_name, $is_wsdl_11);
        $is_rpc_enc =  wsf_is_rpc_enc_wsdl($binding_node, $operation_name);
        /* rpc literal not supported */
    }
    else{
        $binding_node = wsf_get_binding($wsdl_dom, $service, $port, FALSE);
        if(!$binding_node) {
            ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "binding node not found");
            return  NULL;
        }
        $policy_array = wsf_get_all_policies($wsdl_dom, $binding_node, $operation_name, FALSE);
    }

    $operation = wsf_find_operation($sig_model_dom, $operation_name, $service, $endpoint_address, $port, $is_multiple_interfaces);

    if(!$operation) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "operation node not found");
        return;
    }

    $binding_array = wsf_get_binding_details($operation);

    //extracting out the sig_method from the operation sig node
    if($operation) {
        foreach($operation->childNodes as $style) {
            if($style->tagName == WSF_SIGNATURE) {
                $sig_method = $style;
                if($style->attributes->getNamedItem(WSF_METHOD)->value == WSF_INFERENCE)
                    $is_doc_lit = TRUE;
                else if ($style->attributes->getNamedItem(WSF_METHOD)->value == WSF_RPC)
                    $is_doc_lit = FALSE;
            }
        }
    }

    $attachment_map = array();
    $return_info = NULL;
    if($sig_method && !$is_rpc_enc) {
        $return_info = wsf_create_payload($sig_method, $is_doc_lit, $operation_name,
                    $arg_count, $arguments, $class_map, $use_mtom, $attachment_map);
    }
    else {
        $return_info = wsf_create_payload($sig_method, FALSE, $operation_name,
                $arg_count, $arguments, $class_map, $use_mtom, $attachment_map);
    }

    $payload = $return_info[WSF_REQUEST_PAYLOAD];
    $headers = $return_info[WSF_INPUT_HEADERS];


    if(is_array($attachment_map) && count($attachment_map) == 0) {
        $attachment_map = NULL;
    }

    $response_sig_model_string = $sig_model_dom->saveXML($operation);

    $return_value = array(
                          WSF_RESPONSE_SIG_MODEL => $response_sig_model_string,
                          WSF_WSDL_DOM => $wsdl_dom_string,
                          WSF_ENDPOINT_URI=> $endpoint_address,
                          WSF_BINDING_DETAILS=> $binding_array,
                          WSF_REQUEST_PAYLOAD=> $payload,
                          WSF_INPUT_HEADERS=> $headers,
                          WSF_POLICY_NODE=> $policy_array,
                          WSF_ATTACHMENT_MAP => $attachment_map);

    return $return_value;
}


/**
 * Processes and validate response message and assign values to class map.
 * @param string $response_payload_string response envelope
 * @param string $response_sig_model_string response parameter string
 * @param array $response_parameters array of response parameters
 * @return mixed an object, an array or a simple type in line with the 
 * expected format of the response
 */
function wsf_process_response($wsdata)
{
    require_once('dynamic_invocation/wsf_wsdl_consts.php');
    require_once('dynamic_invocation/wsf_wsdl_util.php');
    require_once('dynamic_invocation/wsf_wsdl_client.php');
    
    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "wsf_process_response is called");

    $response_payload_string = $wsdata->WSDL_ResponseBuffer;
    $response_header_string = $wsdata->WSDL_ResponseHeaderBuffer;
    $response_sig_model_string = $wsdata->WSDL_ResponseSigModelString;
    $response_parameters = $wsdata->WSDL_ResponseParameters;
    $wsdldom_string = $wsdata->WSDL_DomString;
    
    $payload_dom = new DomDocument(); 
    $sig_model_dom = new DomDocument();
    $wsdl_dom = new DomDocument();

    $header_dom = NULL;
    
    $payload_dom->preserveWhiteSpace = FALSE;
    $sig_model_dom->preserveWhiteSpace = FALSE;
    $wsdl_dom->preserveWhiteSpace = FALSE;

    $payload_dom->loadXML($response_payload_string);
    $sig_model_dom->loadXML($response_sig_model_string);
    $wsdl_dom->loadXML($wsdldom_string);

    if($response_header_string && !empty($response_header_string)) {
        $header_dom = new DomDocument();
        $header_dom->preserveWhiteSpace = FALSE;
        $header_dom->loadXML($response_header_string);
    }
    
    $response_class = wsf_client_response_and_validate($payload_dom, $header_dom,
                                    $sig_model_dom, $response_parameters);
    
    return $response_class;
    
}

function wsf_process_wsdl_for_service($wsdata)
{
    require_once('dynamic_invocation/wsf_wsdl_consts.php');
    require_once('dynamic_invocation/wsf_wsdl_util.php');
    require_once('dynamic_invocation/wsf_wsdl_service.php');

    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "wsf_process_wsdl_for_service called");
	
    $parameters = $wsdata->WSDL_ParamArray; 
    $operation_array = $wsdata->WSDL_Operations;
    $wsdl_dom_string = $wsdata->WSDL_string;
    $is_wsdl_11 = $wsdata->WSDL_IsWSDL11;
    $sig_model_string = $wsdata->WSDL_SigModelString;
    $is_multiple_interfaces = $wsdata->WSDL_IsMultipleInterfaces;
    
    
    $wsdl_dom = new DomDocument();
    $sig_model_dom  = new DOMDocument();
    $sig_model_dom->preserveWhiteSpace = FALSE;
    $wsdl_dom->preserveWhiteSpace = FALSE;
    
    $service_name = NULL;
    $port_name = NULL;

    if(array_key_exists(WSF_SERVICE_NAME, $parameters)) {
        $service_name = $parameters[WSF_SERVICE_NAME];
    }
    if(array_key_exists(WSF_PORT_NAME, $parameters)) {
        $port_name = $parameters[WSF_PORT_NAME];
    }
    
    $sig_model_dom->preserveWhiteSpace = FALSE;
    $wsdl_dom->preserveWhiteSpace = FALSE;

    $sig_model_dom->loadXML($sig_model_string);
    if(!$sig_model_dom) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR,
                "Error retrieving the intermediate sig model");
        return "Error retrieving the intermediate model";
    }
    $wsdl_dom->loadXML($wsdl_dom_string);
    if(!$wsdl_dom) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, 
            "Error retrieving the wsdl string to process the request message");
        return "Error retrieving the wsdl string to process the request message";
    }
    
    if($is_wsdl_11) {
        $wsdl_11_dom = $wsdl_dom;
    }
	
    /* creating policy array */
    $policy_array = array();
    /* this will no longer need for the service */
    $endpoint_address = wsf_get_endpoint_address($sig_model_dom);
    
    if ($is_wsdl_11 == TRUE && $wsdl_11_dom != NULL) {
        $binding_node = wsf_get_binding($wsdl_11_dom, $service_name, $port_name, TRUE);
        if(!$binding_node)
            return  NULL;
        foreach($operation_array as $value) {
            $policy_array[$value] = wsf_get_all_policies($wsdl_11_dom, $binding_node, $value, TRUE);
         } 
                
    }
    else{
        $binding_node = wsf_get_binding($wsdl_dom, $service_name, $port_name, FALSE);
        if(!$binding_node)
            return  NULL;
        foreach($operation_array as $value) {
            $policy_array[$value] = wsf_get_all_policies($wsdl_dom, $binding_node, $value, FALSE);
        }
    }
    
    $return_array = array();
    $return_array[WSF_SIG_MODEL_STRING] = $sig_model_string;
    $return_array[WSF_POLICIES] = $policy_array;

    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "policy: ".print_r($policy_array, TRUE));

    return $return_array;
}

function wsf_wsdl_process_in_msg($wsdata)
{
    require_once('dynamic_invocation/wsf_wsdl_consts.php');
    require_once('dynamic_invocation/wsf_wsdl_util.php');
    require_once('dynamic_invocation/wsf_wsdl_service.php');

    $parameters = $wsdata->WSDL_ParamArray;
    
    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "wsf_wsdl_process_in_msg is called");
    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, print_r($parameters, TRUE));

    $payload_dom = new DomDocument();
    $sig_model_dom = new DomDocument();

    $payload_dom->preserveWhiteSpace = FALSE;
    $sig_model_dom->preserveWhiteSpace = FALSE;

    $service_name = NULL;
    $port_name = NULL;
    $is_multiple_interfaces = FALSE;
    
    $sig_model_string = NULL;
    $payload_string = NULL;
    $operation_name = NULL;
    $function_name = NULL;
    $class_name = NULL;
    $class_map = NULL;
    $class_args = NULL;
    $header_string = NULL;

    // this is about the respnse message
    $mtom_on = TRUE;

    $cid2cont_type = array();
    $cid2attachments = array();

    if(array_key_exists(WSF_SERVICE_NAME, $parameters)) {
        $service_name = $parameters[WSF_SERVICE_NAME];
    }
    if(array_key_exists(WSF_PORT_NAME, $parameters)) {
        $port_name = $parameters[WSF_PORT_NAME];
    }
  
    if(array_key_exists(WSF_SIG_MODEL_STRING, $parameters)) {
        $sig_model_string = $parameters[WSF_SIG_MODEL_STRING];
    }
    if(array_key_exists(WSF_PAYLOAD_STRING, $parameters)) {
        $payload_string = $parameters[WSF_PAYLOAD_STRING];
    }
    if(array_key_exists(WSF_OPERATION_NAME, $parameters)) {
        $operation_name = $parameters[WSF_OPERATION_NAME];
    }
    if(array_key_exists(WSF_FUNCTION_NAME, $parameters)) {
        $function_name = $parameters[WSF_FUNCTION_NAME];
    }
    if(array_key_exists(WSF_CLASS_NAME, $parameters)) {
        $class_name = $parameters[WSF_CLASS_NAME];
    }
    if(array_key_exists(WSF_CLASSMAP, $parameters)) {
        $class_map = $parameters[WSF_CLASSMAP];
    }
    if(array_key_exists(WSF_CLASS_ARGS, $parameters)) {
        $class_args = $parameters[WSF_CLASS_ARGS];
    }

    if(array_key_exists(WSF_ATTACHMENTS, $parameters)) {
        $cid2attachments = $parameters[WSF_ATTACHMENTS];
    }
    if(array_key_exists(WSF_CID2CONTENT_TYPE, $parameters)) {
        $cid2cont_type = $parameters[WSF_CID2CONTENT_TYPE];
    }
    if(array_key_exists(WSF_USE_MTOM, $parameters)) {
        $mtom_on = $parameters[WSF_USE_MTOM];
    }

    $header_element = NULL;
    if(array_key_exists(WSF_HEADER_STRING, $parameters)) {
        $header_string = $parameters[WSF_HEADER_STRING];
    
        if($header_string && !empty($header_string)) {
            $header_dom = new DomDocument(); 
            $header_dom->preserveWhiteSpace = FALSE;

            $header_dom->loadXML($header_string);
            $header_element = $header_dom->documentElement;
        }
    }

    // payload can be NULL
    if(!empty($payload_string)) {
        $payload_dom->loadXML($payload_string);
        $payload_element = $payload_dom->documentElement;
    } else {
        $payload_dom = NULL;
        $payload_element = NULL;
    }
    $sig_model_dom->loadXML($sig_model_string);

    $endpoint_address = wsf_get_endpoint_address($sig_model_dom);

    $operation_node = wsf_find_operation($sig_model_dom, $operation_name,
            $service_name, $endpoint_address, $port_name, $is_multiple_interfaces);

    if(!$operation_node) {
        ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "$operation_name operaiton not found");
        throw new WSFault("Sender","$operation_name operation not found");
    }

    $return_payload_info = wsf_serivce_invoke_function($operation_node, $function_name,
                $class_name, $class_args, $payload_element, $header_element, $class_map,
                $mtom_on, $cid2cont_type, $cid2attachments);

    return $return_payload_info;
}

function wsf_wsdl_check() {
    return 1;
}

?>
