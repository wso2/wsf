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

// array to keep track of classes that are already written 
$written_classes = array ();

// array to hold information on operations to help generate demo code 
$operations = array ();

// hold the actions array
$actions = NULL;

// hold the derive types per a perticular classes;

$anonymous_count = 0;

/**
 * Function to write sub classes.  
 * $nodes array of nodes corresponding to the classes to be written
 * returns code segment corresponding to sub classes as a string
 */
function wsf_write_sub_classes($node) {
    global $written_classes;
    $code = "";
    if ($node) {
        if ($node->hasAttributes()) {
            // Wrapper element found
            $attrs = $node->attributes;
            $type_name = "";
            if($attrs->getNamedItem(WSF_TYPE)) {
                $type_name = $attrs->getNamedItem(WSF_TYPE)->value;
            }
            else if($attrs->getNamedItem(WSF_EXTENSION)) {
                $type_name = $attrs->getNamedItem(WSF_EXTENSION)->value;
            }
            else if($attrs->getNamedItem(WSF_XSI_TYPE)) {
                $type_name = $attrs->getNamedItem(WSF_XSI_TYPE)->value;
            }

            // array to hold child elements corresponding to sub classes
            $child_array = array ();

            // check if the class was already written 
            if(array_key_exists($type_name, $written_classes) && $written_classes[$type_name] == TRUE) {
                return;
            }
            
            if($type_name == WSF_XSD_BASE64 && !$node->hasChildNodes()) {
                //return;
            }

            // write the extension code..
            $extension_code = wsf_write_extension($node, $code);

            $code = $code . "class " . $type_name . $extension_code. " {\n";
            $written_classes[$type_name] = TRUE;

            $child_array = array();
            $derived_classes_code = "";
            $code .= wsf_write_content_model($node, $child_array, $derived_classes_code);

            $code = $code . "\n}\n\n";
            // done writing the current class, now go and write the sub classes
            foreach($child_array as $child) {
                $code = $code . wsf_write_sub_classes($child);
            }

            $code .= $derived_classes_code;
        }
        // TODO: What about arrays?, ok this is for array api, not class api
    }
    return $code;
}


function wsf_write_extension(DomNode $sig_node, &$code) {
    $first_sig_child = $sig_node->firstChild;
   
    $extension_code = "";
    if($first_sig_child != NULL && $first_sig_child->nodeName == WSF_INHERITED_CONTENT) {

        $attrs = $first_sig_child->attributes;
        if($attrs->getNamedItem(WSF_EXTENSION)) {
            $type_name = $attrs->getNamedItem(WSF_EXTENSION)->value;
        }
        $extension_code .= " extends ".$type_name;

        $code = $code . wsf_write_sub_classes($first_sig_child);
    }
    else {
        $extension_code .= "";
    }
    return $extension_code;
}


function wsf_write_derives(DomNode $sig_node, &$code) {
    $sig_childs = $sig_node->childNodes;
    $types_arr = array();
  
    foreach($sig_childs as $sig_child) {
        if($sig_child != NULL && $sig_child->nodeName == WSF_INHERITING_TYPE) {

            $attrs = $sig_child->attributes;
            if($attrs->getNamedItem(WSF_XSI_TYPE)) {
                $type_name = $attrs->getNamedItem(WSF_XSI_TYPE)->value;
                $types_arr[] = $type_name;
            }
            $code = $code . wsf_write_sub_classes($sig_child);
        }
    }
    return $types_arr;
}


function wsf_write_content_model($parent_node, &$child_array, &$derived_classes_code) {
    global $anonymous_count;
    $code = "";
    $param_child_list = $parent_node->childNodes;
    $content_model = "";
    if($parent_node->attributes && $parent_node->attributes->getNamedItem(WSF_CONTENT_MODEL)) {
        $content_model = $parent_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value;
    }
    if($content_model == "choice") {
        //just start a comment
        $code .= "\n";
        $code .= "    // You may set only one from the following set\n";
        $code .= "    // ---------------Start Choice----------------\n";
    }
    foreach ($param_child_list as $param_child) {
        if($param_child->nodeName == WSF_PARAM) {
            $param_attr = $param_child->attributes;
            $param_name = $param_type = "";
            $parm_type = NULL;
            if($param_attr->getNamedItem(WSF_NAME)) {
                $param_name = $param_attr->getNamedItem(WSF_NAME)->value;
            }
            if($param_attr->getNamedItem(WSF_TYPE)) {
                $param_type = $param_attr->getNamedItem(WSF_TYPE)->value;
            }
            if($param_type == NULL) {
                $param_type = "anonymous" . $anonymous_count;
                $anonymous_count ++;
                $param_child->setAttribute(WSF_TYPE, $param_type);
            }


            if($param_child->getAttribute("simple") == "yes"){
                $comments = wsf_comment_on_simple_type($param_child, $param_name, $param_type);
            } else {
                $comments = '';
            }

            $min_occurs = $max_occurs = 1;

            // check if the attribute is of array type
            if($param_attr->getNamedItem(WSF_MAX_OCCURS)) {
                $max_occurs = $param_attr->getNamedItem(WSF_MAX_OCCURS)->value;
            }
            if($param_attr->getNamedItem(WSF_MIN_OCCURS)) {
                $min_occurs = $param_attr->getNamedItem(WSF_MIN_OCCURS)->value;
            }

            //resolving lists
            $is_list = FALSE;
            if($param_attr->getNamedItem(WSF_LIST)) {
                $is_list = $param_attr->getNamedItem(WSF_LIST)->value;
            }

            //resolving unions
            $is_union = FALSE;
            if($param_attr->getNamedItem(WSF_UNION)) {
                $is_union = $param_attr->getNamedItem(WSF_UNION)->value;
            }
            if($is_union) {
                $union_type_array = array();
                $union_childs = $param_child->childNodes;

                foreach($union_childs as $union_child) {
                     if($union_child->nodeName == "union") {
                         $union_type = $union_child->attributes->getNamedItem(WSF_TYPE)->value;
                         $union_type_array[] = $union_type;
                     }
                }
                $param_type = implode("/", $union_type_array);
            }

            // is array type exist..
            $array_type = "";
            if (($max_occurs != null && $max_occurs != "1") || $is_list) {
                $array_type = "array[$min_occurs, $max_occurs] of ";
            }
            
            // is object of exist..
            $object_of = "";
            if (($param_attr && $param_attr->getNamedItem(WSF_WSDL_SIMPLE) && 
                  $param_attr->getNamedItem(WSF_WSDL_SIMPLE)->value == 'no') ||
                    ($param_attr->getNamedItem(WSF_CONTENT_MODEL) && 
                     $param_attr->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT)) {
                $object_of = "(object)";
            }
            // write public members of the class 
           
            // we generate the code for base 64 type differently
            if($object_of == "" && $param_type == WSF_XSD_BASE64) {
                
                $code .= "\n";
                $code .= "    // You need to set only one from the following two vars\n";

                $code = $code . "\n    /**\n     * @var " . $array_type . "Plain Binary\n$comments     */\n" . "    public $" . $param_name . ";\n";
                $code = $code . "\n    /**\n     * @var " . $array_type . $param_type . "\n$comments     */\n" . "    public $" . $param_name .WSF_POSTFIX_BASE64. ";\n";

                $code .= "\n";
            }
            else {
                $derived_info = "\n";
               
                $derives_arr = wsf_write_derives($param_child, $derived_classes_code);

                if(count($derives_arr) > 0) {
                    $derived_info .= "     *    Or one of following derived class(es)\n";
                    foreach($derives_arr as $derived_type) {
                        $derived_info .= "     *       $derived_type\n";
                    }
                }
                $code = $code . "\n    /**\n     * @var " . $array_type . $object_of .$param_type . $derived_info. "$comments     */\n" . "    public $" . $param_name . ";\n";
            }

            // if it is not s simple type, we have to keep track of it to write a corresponding class
            if (($param_attr && $param_attr->getNamedItem(WSF_WSDL_SIMPLE) && 
                  $param_attr->getNamedItem(WSF_WSDL_SIMPLE)->value == 'no') ||
                    ($param_attr->getNamedItem(WSF_CONTENT_MODEL) && 
                     $param_attr->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT)) {
                $child_array[] = $param_child;
            }
        }
        else if($param_child->nodeName == WSF_INNER_CONTENT) {
            // in place of inner content recursively call the wsf_write_content_model
            $code .= wsf_write_content_model($param_child, $child_array, $derived_classes_code);
        }
    }
    if($content_model == "choice") {
        //just start a comment
        $code .= "    // ----------------End Choice---------------\n\n";
    }
    
    if($content_model == WSF_SIMPLE_CONTENT) {
        $extension_type = $parent_node->attributes->getNamedItem(WSF_EXTENSION)->value;
        $param_name = $parent_node->attributes->getNamedItem(WSF_NAME)->value;
        $code .= "\n";
        $code .= "    // The \"value\" represents the element '$param_name' value..\n";
        $ele_name = WSF_SIMPLE_CONTENT_VALUE;
        $comments = wsf_comment_on_simple_type($parent_node, $ele_name, $extension_type);
        
        // we generate the code for base 64 type differently
        if($extension_type == WSF_XSD_BASE64) { // this should be anyway a simple type..
            $code .= "\n";
            $code .= "    // You need to set only one from the following two vars\n";
            $code = $code . "\n    /**\n     * @var " . "Plain Binary" . "\n$comments     */\n" . "    public $" . $ele_name . ";\n";
            $code = $code . "\n    /**\n     * @var " . $extension_type . "\n$comments     */\n" . "    public $" . $ele_name . WSF_POSTFIX_BASE64 . ";\n";
            $code .= "\n";
        }
        else {
            $code = $code . "\n    /**\n     * @var " . $extension_type . "\n$comments     */\n" . "    public $" . $ele_name . ";\n";
        }
    }

    return $code;
}

/**
 * Function to write classes. Does the WSDL to PHP conversion using a 'sig model'.  
 * $wsdl_location location of the WSDL
 * returns code segment corresponding to classes as a string
 */
function wsf_wsdl2php($wsdl_location) {

    require_once ('dynamic_invocation/wsf_wsdl_consts.php');
    require_once ('dynamic_invocation/wsf_wsdl_util.php');

    global $written_classes;
    global $operations;
    global $actions;

    $written_classes[WSF_XSD_ANYTYPE] = TRUE;

    $code = "";

    $wsdl_dom = new DomDocument();
    $sig_model_dom = new DOMDocument();

    $xslt_location = "./xslt/";

    $sig_model_dom->preserveWhiteSpace = false;
    $wsdl_dom->preserveWhiteSpace = false;

    if (!$wsdl_location) {
        echo "WSDL is not found";
        return NULL;
    }
    $is_multiple_interfaces = FALSE;

    // load WSDL as DOM
    if (!$wsdl_dom->load($wsdl_location)) {
        echo "WSDL could not be loaded.";
        return NULL;
    }


    $wsdl_dom->preserveWhiteSpace = false;

    // changing code for processing mutiple port types in wsdl 1.1 
    $is_multiple_interfaces = wsf_is_mutiple_port_types($wsdl_dom);

    $is_wsdl_11 = $wsdl_11_dom = NULL;
    if ($is_multiple_interfaces == FALSE) {
        //wsdl1.1 to wsdl2 conversion
	$ctx = NULL;
        $wsdl_dom = wsf_get_wsdl_dom($wsdl_dom, $wsdl_location, $is_wsdl_11, $wsdl_11_dom, $ctx);
        if (!$wsdl_dom) {
            echo "Error creating WSDL DOM document";
            return NULL;
        }
        $sig_model_dom = wsf_get_sig_model_dom($wsdl_dom);
    } else {
        //wsdl1.1 to wsdl2 conversion
        $wsdl_dom = wsf_get_wsdl_dom($wsdl_dom, $wsdl_location, $is_wsdl_11, $wsdl_11_dom, $ctx);
        $sig_model_dom = wsf_process_multiple_interfaces($wsdl_dom, $sig_model_dom);
    }


    if (!$sig_model_dom) {
        echo "Error creating intermediate service operations signature model";
        return NULL;
    }


    // get the list of operations
    $op_nodes = $sig_model_dom->getElementsByTagName(WSF_OPERATION);

    // process each operation
    foreach ($op_nodes as $op_node) {

        // get operation name 
        $op_attr = $op_node->attributes;

        if($op_attr == NULL || $op_attr->getNamedItem(WSF_NAME) == NULL) {
            continue;
        }
        $op_name = $op_attr->getNamedItem(WSF_NAME)->value;

        if (array_key_exists($op_name, $operations) == FALSE || $operations[$op_name] == NULL) { // it operation is already found in an earlier parse, should not re-set it
            $operations[$op_name] = array ();
            $operations[$op_name][WSF_CLIENT] = "";
            $operations[$op_name][WSF_SERVICE] = "";
        }
        else {
            continue; //no need to record same operation multiple times
        }

        // get the nodes describing operation characteristics 
        $op_child_list = $op_node->childNodes;

        //first we have to have a good idea about headers, so processing headers first
        $in_headers = 0;
        $out_headers = 0;

        $in_header_objects = array();
        $in_header_objects[WSF_CLIENT] = "";
        $in_header_objects[WSF_SERVICE] = "";

        $out_header_objects = array();
        $out_header_objects[WSF_CLIENT] = "";
        $out_header_objects[WSF_SERVICE] = "";

        $service_function_doc_comment = "";
        $service_function_doc_comment .= "/**\n";
        $service_function_doc_comment .= " * Service function $op_name\n";

        // doc comments for input types
        foreach ($op_child_list as $op_child) {
            // process the signature node
            if ($op_child->nodeName == WSF_SIGNATURE) {
                // get the nodes representing operation parameters and return types within signature
                $param_list = $op_child->childNodes;
                foreach ($param_list as $param_node) {
                    if ($param_node) {
                        // look for params and returns nodes 
                        if ($param_node->nodeName == WSF_PARAMS) {
                            if ($param_node->hasAttributes()) {
                                // Wrapper element 
                                $params_attr = $param_node->attributes;

                                // get wrapper element name, this is going to be the class name
                                $ele_name = $params_attr->getNamedItem(WSF_WRAPPER_ELEMENT)->value;
            
                                $service_function_doc_comment .= " * @param object of $ele_name \$input \n";
                            }
                            else {
                                // No wrapper element, there won't be any class generated for this 

                                $child_array = array ();
                                $param_child_list = $param_node->childNodes;

                                foreach ($param_child_list as $param_child) {
                                    $param_attr = $param_child->attributes;

                                    $ele_name = $param_attr->getNamedItem(WSF_NAME)->value;

                                    $content_model = "";
                                    if($param_attr->getNamedItem(WSF_CONTENT_MODEL)) {
                                        $content_model = $param_attr->getNamedItem(WSF_CONTENT_MODEL)->value;
                                    }
                                    $param_type = $param_attr->getNamedItem(WSF_TYPE)->value;
                                    
                                    if($content_model == WSF_SIMPLE_CONTENT) {
                                        $service_function_doc_comment .= " * @param object of $ele_name \$input \n";
                                    }
                                    else {
                                        $service_function_doc_comment .= " * @param $param_type \$input \n";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        foreach ($op_child_list as $op_child) {
            if($op_child->nodeName == WSF_W2P_BINDING_DETAILS) {
                $op_binding_childs = $op_child->childNodes;
                foreach($op_binding_childs as $op_binding_child) {
                    if($op_binding_child->nodeName == WSF_SOAPHEADER) {
                        if($op_binding_child->attributes->getNamedItem(WSF_HEADER_FOR_ATTRIBUTE)) {
                            if($op_binding_child->attributes->getNamedItem(WSF_HEADER_FOR_ATTRIBUTE)->value == WSF_WSDL_OUTPUT) {
                                $direction = WSF_WSDL_OUTPUT;
                            } else {
                                $direction = WSF_WSDL_INPUT;
                            }
                        } else {
                           $direction = WSF_WSDL_INPUT;
                        }
                    }
                    
                    if($op_binding_child->attributes->getNamedItem(WSF_TYPE)) {
			$param_node = $op_binding_child->firstChild;
		
			if($param_node && $param_node->attributes->getNamedItem(WSF_TYPE)){
		
			$class_name = $param_node->attributes->getNamedItem(WSF_TYPE)->value;
                        if($direction == WSF_WSDL_INPUT) {
                            if (($param_node->attributes && $param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) && 
                                 $param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == 'no') ||
                                    ($param_node->attributes->getNamedItem(WSF_CONTENT_MODEL) && 
                                     $param_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT)) {
                                // for the complex types and simple contnet
                                $in_header_objects[WSF_CLIENT] .= "    \$header_in{$in_headers} = new $class_name();\n    // TODO: fill in the class fields of \$header_in{$in_headers} object which is of type $class_name to match your business logic\n\n";
                                $in_header_objects[WSF_SERVICE] .= "    // NOTE: \$header_in{$in_headers} object is of type $class_name\n";

                                $service_function_doc_comment .= " * @param object of $class_name \$header_in{$in_headers} input header\n";
                            }
                            else {
                                // for the simple types
                                $in_header_objects[WSF_CLIENT] .= "    // TODO: fill in the \$header_in{$in_headers} which is of type $class_name to match your business logic\n\n";
                                $in_header_objects[WSF_SERVICE] .= "    // NOTE: \$header_in{$in_headers} header is of type $class_name\n";

                                $service_function_doc_comment .= " * @param $class_name \$header_in{$in_headers} input header\n";
                            }

                            $in_headers ++;
                        }
                        else {

                            if (($param_node->attributes && $param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) && 
                                 $param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == 'no') ||
                                    ($param_node->attributes->getNamedItem(WSF_CONTENT_MODEL) && 
                                     $param_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT)) {
                                // for the complex types and simple contnet

                                $out_header_objects[WSF_CLIENT] .= "\n    // TODO: Implement business logic to consume \$header_out{$out_headers} object, which is of type class $class_name\n";
                                $out_header_objects[WSF_SERVICE] .= "    // NOTE: you should assign an object of type $class_name to \$header_out{$out_headers}\n";
                                
                                $service_function_doc_comment .= " * @param reference object of $class_name \$header_out{$out_headers} object output header\n";
                            }
                            else {
                                // for the simple types
                                $out_header_objects[WSF_CLIENT] .= "\n    // TODO: Implement business logic to consume \$header_out{$out_headers}, which is of type $class_name\n";
                                $out_header_objects[WSF_SERVICE] .= "    // NOTE: you should assign an object of type $class_name to \$header_out{$out_headers}\n";
                                
                                $service_function_doc_comment .= " * @param reference $class_name \$header_out{$out_headers} output header\n";
                            }

                            $out_headers ++;
                        }

                    if (($param_node->attributes && $param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) && 
                         $param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == 'no') ||
                            ($param_node->attributes->getNamedItem(WSF_CONTENT_MODEL) && 
                             $param_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT)) {
                        $code_for_header = wsf_write_sub_classes($param_node);
                        $code .= $code_for_header;
                    }
		 }
	       }
            }
          }
        }


        // doc comments for the return type
        foreach ($op_child_list as $op_child) {
            // process the signature node
            if ($op_child->nodeName == WSF_SIGNATURE) {
                // get the nodes representing operation parameters and return types within signature
                $param_list = $op_child->childNodes;
                foreach ($param_list as $param_node) {
                    if ($param_node) {
                        // look for params and returns nodes 
                        if ($param_node->nodeName == WSF_RETURNS) {
                            if ($param_node->hasAttributes()) {
                                // Wrapper element 
                                $params_attr = $param_node->attributes;

                                // get wrapper element name, this is going to be the class name
                                $ele_name = $params_attr->getNamedItem(WSF_WRAPPER_ELEMENT)->value;
            
                                $service_function_doc_comment .= " * @return object of $ele_name \n";
                            }
                            else {

                                // No wrapper element, there won't be any class generated for this 

                                $child_array = array ();
                                $param_child_list = $param_node->childNodes;

                                foreach ($param_child_list as $param_child) {
                                    $param_attr = $param_child->attributes;
                                    
                                    $ele_name = $param_type = "";
                                    if($param_attr->getNamedItem(WSF_NAME)) {
                                        $ele_name = $param_attr->getNamedItem(WSF_NAME)->value;
                                    }
                                    if($param_attr->getNamedItem(WSF_TYPE)) {
                                        $param_type = $param_attr->getNamedItem(WSF_TYPE)->value;
                                    }

                                    $content_model = "";
                                    if($param_attr->getNamedItem(WSF_CONTENT_MODEL)) {
                                        $content_model = $param_attr->getNamedItem(WSF_CONTENT_MODEL)->value;
                                    }
                                    
                                    if($content_model == WSF_SIMPLE_CONTENT) {
                                        $service_function_doc_comment .= " * @return object of $ele_name \n";
                                    }
                                    else {
                                        $service_function_doc_comment .= " * @return $param_type \n";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        $service_function_doc_comment .= " */\n";
      
        /*
        if($out_headers > 0) {
            $in_header_objects[WSF_CLIENT] .= "    // Declration of variables for the output headers\n"; 
            $in_header_objects[WSF_CLIENT] .= "    // NOTE: don't set any value to these objects, rather the call to proxy will assign values for them\n"; 
            for($i = 0; $i < $out_headers; $i ++) {
                $in_header_objects[WSF_CLIENT] .= "    \$header_out{$i} = NULL;\n";
            }
            $in_header_objects[WSF_CLIENT] .= "\n"; 
            $out_header_objects[WSF_SERVICE] .= "\n";
        }
        */
        
        if($in_header_objects[WSF_CLIENT] != "") {
            $in_header_objects[WSF_CLIENT] = substr_replace($in_header_objects[WSF_CLIENT], "", -1);
        }
        if($in_header_objects[WSF_SERVICE] != "") {
            $in_header_objects[WSF_SERVICE] .= "\n";
        }
        
        /* function declration variables for headers */
        $header_function_decl = "";
        for($i = 0; $i < $in_headers; $i ++) {
            $header_function_decl .= ", \$header_in".$i;
        }
        for($i = 0; $i < $out_headers; $i ++) {
            $header_function_decl .= ", &\$header_out".$i;
        }

        /* function call variables for headers */
        $header_function_call = "";
        for($i = 0; $i < $in_headers; $i ++) {
            $header_function_call .= ", \$header_in".$i;
        }
        for($i = 0; $i < $out_headers; $i ++) {
            $header_function_call .= ", &\$header_out".$i;
        }

        //put the doc comment
        //declare the start of service function
        $operations[$op_name][WSF_SERVICE] = $service_function_doc_comment;
        $operations[$op_name][WSF_SERVICE] .= "function " . $op_name . "(\$input{$header_function_decl}) {\n";

    
        foreach ($op_child_list as $op_child) {
            // process the signature node
            if ($op_child->nodeName == WSF_SIGNATURE) {
                // get the nodes representing operation parameters and return types within signature
                $param_list = $op_child->childNodes;
                foreach ($param_list as $param_node) {
                    if ($param_node) {
                        // look for params and returns nodes 
                        if ($param_node->nodeName == WSF_PARAMS || $param_node->nodeName == WSF_RETURNS) {
                            if ($param_node->hasAttributes()) {
                                // Wrapper element 
                                $params_attr = $param_node->attributes;

                                // get wrapper element name, this is going to be the class name
                                $ele_name = $params_attr->getNamedItem(WSF_WRAPPER_ELEMENT)->value;
            
                                // array to hold child elements corresponding to sub classes
                                $child_array = array ();

                                // check if the class is already written
                                if (array_key_exists($ele_name, $written_classes) && $written_classes[$ele_name] == TRUE) {
                                    continue;
                                }

                                // write the extension code..
                                $extension_code = wsf_write_extension($param_node, $code);
                                $derives_arr = wsf_write_derives($param_node, $code);


                                // start writing class    
                                $code = $code . "class " . $ele_name . $extension_code. " {\n";
                                $written_classes[$ele_name] = TRUE;

                                // prepare the demo code that the user could use for testing client. 
                                // shows how to create the input and receive the response

                                if ($param_node->nodeName == WSF_PARAMS) {
                                    $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "    \$input = new $ele_name();\n";
                                    if(count($derives_arr) > 0) {
                                        $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "    /**\n     * Here you can replace $ele_name with a following class(es)\n";
                                        foreach($derives_arr as $derive) {
                                            $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "     * $derive\n";
                                        }
                                        $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "     */\n";
                                    } 
                                    $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "    //TODO: fill in the class fields of \$input to match your business logic\n";
                                    $operations[$op_name][WSF_CLIENT] .= $in_header_objects[WSF_CLIENT];

                                    $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "    // TODO: fill in the business logic\n    // NOTE: \$input is of type $ele_name\n";
                                    if(count($derives_arr) > 0) {
                                        $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "    /**\n     * You can expect \$input to be an instance of following class(es) in addition to $ele_name\n";
                                        foreach($derives_arr as $derive) {
                                            $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "     * $derive\n";
                                        }
                                        $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "     */\n";
                                    }
                                    $operations[$op_name][WSF_SERVICE] .= $in_header_objects[WSF_SERVICE];
                                }
                                if ($param_node->nodeName == WSF_RETURNS) {
                                    $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "\n    // call the operation\n    \$response = \$proxy->" . $op_node->attributes->getNamedItem('name')->value . "(\$input{$header_function_call});\n    //TODO: Implement business logic to consume \$response, which is of type $ele_name\n";
                                    if(count($derives_arr) > 0) {
                                        $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "    /**\n     * You can expect \$response to be an instance of following class(es) in addition to $ele_name\n";
                                        foreach($derives_arr as $derive) {
                                            $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "     * $derive\n";
                                        }
                                        $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "     */\n";
                                    }
                                    $operations[$op_name][WSF_CLIENT] .= $out_header_objects[WSF_CLIENT];
                                    $operations[$op_name][WSF_SERVICE] .= $out_header_objects[WSF_SERVICE];
                                    $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "    // NOTE: should return an object of type $ele_name\n";
                                    if(count($derives_arr) > 0) {
                                        $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "    /**\n     * Here you can replace $ele_name with a following class(es)\n";
                                        foreach($derives_arr as $derive) {
                                            $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "     * $derive\n";
                                        }
                                        $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "     */\n";
                                    } 
                                }

                                $child_array = array();
                                $derived_classes_code = "";
                                $code .= wsf_write_content_model($param_node, $child_array, $derived_classes_code);
                                
                                $code = $code . "\n}\n\n";
                                // done writing the current class, now go and write the sub classes
                                foreach($child_array as $child) {
                                    $code = $code . wsf_write_sub_classes($child);
                                }
                                $code .= $derived_classes_code;
                            } else {
                                // TODO: No wrapper element, there won't be any class generated for this 

                                $child_array = array ();
                                $param_child_list = $param_node->childNodes;

                                foreach ($param_child_list as $param_child) {
                                    $param_attr = $param_child->attributes;

                                    $ele_name = $param_attr->getNamedItem(WSF_NAME)->value;
                                    $param_type = $param_attr->getNamedItem(WSF_TYPE)->value;

                                    $content_model = "";
                                    if($param_attr->getNamedItem(WSF_CONTENT_MODEL)) {
                                        $content_model = $param_attr->getNamedItem(WSF_CONTENT_MODEL)->value;
                                    }
                                    //resolving lists
                                    $is_list = FALSE;
                                    if($param_attr->getNamedItem(WSF_LIST)) {
                                        $is_list = $param_attr->getNamedItem(WSF_LIST)->value;
                                    }
                                    $type_prefix = "";
                                    if($is_list) {
                                        $type_prefix = "array of ";
                                    }

                                    //resolving unions
                                    $is_union = FALSE;
                                    if($param_attr->getNamedItem(WSF_UNION)) {
                                        $is_union = $param_attr->getNamedItem(WSF_UNION)->value;
                                    }
                                    if($is_union) {
                                        $union_type_array = array();
                                        $union_childs = $param_child->childNodes;

                                        foreach($union_childs as $union_child) {
                                             if($union_child->nodeName == "union") {
                                                 $union_type = $union_child->attributes->getNamedItem(WSF_TYPE)->value;
                                                 $union_type_array[] = $union_type;
                                             }
                                        }
                                        $param_type = implode("/", $union_type_array);
                                    }

                                    if($content_model == WSF_SIMPLE_CONTENT) {
                                        // start writing class    
                                        if(!array_key_exists($ele_name, $written_classes) || !$written_classes[$ele_name]) {
                                            $written_classes[$ele_name] = TRUE;
                                            $code = $code . "class " . $ele_name.  " {\n";

                                            $derived_classes_code = "";
                                            $code .= wsf_write_content_model($param_child, $child_array, $derived_classes_code);
                                            $code = $code . "\n}\n\n";

                                            $code .= $derived_classes_code;
                                        }

                                        if ($param_node->nodeName == WSF_PARAMS) {
                                            $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "    \$input = new $ele_name();\n    //TODO: fill in the class fields of \$input to match your business logic\n";
                                            $operations[$op_name][WSF_CLIENT] .= $in_header_objects[WSF_CLIENT];
                                            $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "    // TODO: fill in the business logic\n    // NOTE: \$input is of type $ele_name\n";
                                            $operations[$op_name][WSF_SERVICE] .= $in_header_objects[WSF_SERVICE];
                                        }
                                        if ($param_node->nodeName == WSF_RETURNS) {
                                            $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "\n    // call the operation\n    \$response = \$proxy->" . $op_node->attributes->getNamedItem('name')->value . "(\$input{$header_function_call});\n    //TODO: Implement business logic to consume \$response, which is of type $ele_name\n";
                                            $operations[$op_name][WSF_CLIENT] .= $out_header_objects[WSF_CLIENT];
                                            $operations[$op_name][WSF_SERVICE] .= $out_header_objects[WSF_SERVICE];
                                            $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "    // NOTE: should return an object of type $ele_name\n";
                                        }

                                    }
                                    else {
                                        // prepare the demo code that the user could use for testing client. 
                                        // shows how to create the input and receive the response

                                        if ($param_node->nodeName == WSF_PARAMS) {
                                            $simple_type_comment = wsf_comment_on_simple_type_wrapper($param_child, "\$input", $param_type);
                                            $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "    //TODO: fill \$input with (data type: {$type_prefix}{$param_type}) data to match your business logic\n";
                                            if($param_child->getAttribute("simple") == "yes"){
                                                $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . $simple_type_comment."\n";
                                            }
                                            $operations[$op_name][WSF_CLIENT] .= $in_header_objects[WSF_CLIENT];

                                            $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "    // TODO: fill in the business logic\n    // NOTE: \$input is of type {$type_prefix}{$param_type}\n";
                                            if($param_child->getAttribute("simple") == "yes"){
                                                $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . $simple_type_comment."\n";
                                            }
                                            $operations[$op_name][WSF_SERVICE] .= $in_header_objects[WSF_SERVICE];

                                        }
                                        if ($param_node->nodeName == WSF_RETURNS) {
                                            $simple_type_comment = wsf_comment_on_simple_type_wrapper($param_child, "output ", $param_type);
                                            $operations[$op_name][WSF_CLIENT] = $operations[$op_name][WSF_CLIENT] . "\n    // call the operation\n    \$response = \$proxy->" . $op_node->attributes->getNamedItem('name')->value . "(\$input{$header_function_call});\n    //TODO: Implement business logic to consume \$response, which is of type {$type_prefix}{$param_type}\n";
                                            $operations[$op_name][WSF_CLIENT] .= $out_header_objects[WSF_CLIENT];
                                            $operations[$op_name][WSF_SERVICE] .= $out_header_objects[WSF_SERVICE];
                                            $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "    // NOTE: should return an object of (type: {$type_prefix}{$param_type})\n";
                                            $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . $simple_type_comment."\n";
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else if($op_child->nodeName == WSF_W2P_BINDING_DETAILS) {
                /* action is default to wsa waction */
                $action = $op_child->getAttribute('wsawaction'); 
                if($action == NULL)
                {
                    $action = $op_child->getAttribute('soapaction');
                }

                if($action != NULL)
                {
                    /* we feeding the actions to the global variable */
                    if($actions == NULL)
                    {
                        $actions = array();
                    }
                
                    $actions[$action] = $op_name;
                }
            }
        }
        $operations[$op_name][WSF_SERVICE] = $operations[$op_name][WSF_SERVICE] . "\n}\n\n";
    }
    return $code;
}

function wsf_comment_on_simple_type_wrapper($param_node, $param_name, $param_type, $from_union = FALSE) {
    $comment = wsf_comment_on_simple_type($param_node, $param_name, $param_type, $from_union);
    if(!empty($comment)) {
         $wrapped = "    /**\n";
         $wrapped .= $comment;
         $wrapped .= "     */\n";
         return $wrapped;
    }
    return $comment;
}

function wsf_comment_on_simple_type($param_node, $param_name, $param_type, $from_union = FALSE)
{
    $comment = "";
    if($param_node->hasChildNodes()){
    }
    else{
        return "";
    }

    $i = 0;
    $restrictions = $param_node->childNodes;

    if(!$from_union) {
        foreach($restrictions as $restriction){
            if($restriction->nodeName == "param") {
            }
            else{
                $comment = "     *     NOTE: ${param_name} should follow the following restrictions\n";
                break;
            }
        }
    }

    // handle union
    foreach($restrictions as $restriction) {
        if($restriction->nodeName == "union") {
            $restriction_type = $restriction->attributes->getNamedItem(WSF_TYPE)->value;
            if($restriction->hasChildNodes()) {
                $comment .= "     * type {$restriction_type} hold the following restrictions\n";
                $comment .= wsf_comment_on_simple_type($restriction, $param_name, $param_type, TRUE);
            }
        }
    }


    // first iterate through the enumerations
    foreach($restrictions as $restriction){
        if($restriction->nodeName == "enumeration"){
            if($i == 0){
                $comment .= "     *     You can have one of the following value\n";
                $i ++;
            }
            $value = $restriction->getAttribute("value");
            $comment .= "     *     {$value}\n";
        }
    }

    $min_value = -1;
    $min_inclusive = FALSE;
    $max_value = -1;
    $max_inclusive = FALSE;


    // first iterate through the max/minInclusives/Exclusives
    foreach($restrictions as $restriction){
        if($restriction->nodeName == "minInclusive"){
            $value =  $restriction->getAttribute("value");
            if($min_value == -1){
                $min_value = $value;
                $min_inclusive = TRUE;
            }
            else{
                $min_value = ($min_value -1> $value)?$value:$min_value;
                $min_inclusive = TRUE;
            }
        }
        else if($restriction->nodeName == "maxInclusive"){
            $value = $restriction->getAttribute("value");
            if($max_value == -1){
                $max_value = $value;
                $max_inclusive = TRUE;
            }
            else{
                $max_value = ($max_value +1< $value)?$value:$max_value;
                $max_inclusive = TRUE;
            }
        }
        if($restriction->nodeName == "minExclusive"){
            $value =  $restriction->getAttribute("value");
            if($min_value == -1){
                $min_value = $value;
                $min_inclusive = FALSE;
            }
            else{
                $min_value = ($min_value > $value)?$value:$min_value;
                $min_inclusive = FALSE;
            }
        }
        else if($restriction->nodeName == "maxExclusive"){
            $value = $restriction->getAttribute("value");
            if($max_value == -1){
                $max_value = $value;
                $max_inclusive = FALSE;
            }
            else{
                $max_value = ($max_value < $value)?$value:$max_value;
                $max_inclusive = FALSE;
            }
        }
    }

    if($min_value == -1 && $max_value == -1){
        // just skip it
    }
    else
    {
        //print out the max/min information
        $comment .= "     *     Your value should be \n";
        if($min_value != -1){
            $comment .= "     *     Greater than ";
            if($min_inclusive){
                $comment .= "or equal to ";
            }
            $comment .= $min_value."\n";
        }
        if($max_value != -1){
            $comment .= "     *     Less than ";
            if($max_inclusive){
                $comment .= "or equal to ";
            }
            $comment .= $max_value."\n";
        }
    }
  
    $min_len = -1;
    $max_len = -1;

    // first iterate through the max/minInclusives/Exclusives
    foreach($restrictions as $restriction){
        if($restriction->nodeName == "minLength"){
            $len =  $restriction->getAttribute("value");
            if($min_len == -1){
                $min_len = $len;
            }
            else{
                $min_len = ($min_len > $len)?$len:$min_len;
            }
        }
        else if($restriction->nodeName == "maxLength"){
            $len = $restriction->getAttribute("value");
            if($max_len == -1){
                $max_len = $len;
            }
            else{
                $max_len = ($max_len < $len)?$len:$max_len;
            }
        }
    }

    if($min_len == -1 && $max_len == -1){
        // just skip it
    }
    else
    {
        //print out the max/min information
        $comment .= "     *     Your length of the value should be \n";
        if($min_len != -1){
            $comment .= "     *     Greater than ";
            $comment .= $min_len."\n";
        }
        if($max_len != -1){
            $comment .= "     *     Less than ";
            $comment .= $max_len."\n";
        }
    }

    //Iterate throught the total digits and length facets
    foreach($restrictions as $facet) {
        if($facet->nodeName == "pattern") {
            $pattern =  $facet->getAttribute("value");
            $comment .= "     *     The pattern should be \n     * {$pattern} \n";
        }
        else if($facet->nodeName == "length") {
            $length =  $facet->getAttribute("value");
            $comment .= "     *     The value length should be \n     * {$length} \n";
        }
        else if($facet->nodeName == "totalDigits") {
            $digits =  $facet->getAttribute("value");
            $comment .= "     *     The number of digits should be \n     * {$digits} \n";
        }
        else if($facet->nodeName == "fractionDigits") {
            $digits =  $facet->getAttribute("value");
            $comment .= "     *     The number of fraction digits should be \n     * {$digits} \n";
        }
    }
  
    return $comment;
}
?>
