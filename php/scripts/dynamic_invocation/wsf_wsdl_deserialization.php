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
 * parse payload and return an array of paramerters
 * @param $payload 
 * @param $sig_node
 *      example of sig..
 *
 *      <params wrapper-element="myDemo"
 *             wrapper-element-ns="http://wso2.org/dyn/codegen/demo"
 *             simple="no" contentModel="all">
 *          <param token="#in"
 *               targetNamespace="http://wso2.org/dyn/codegen/demo"
 *               minOccurs="1" maxOccurs="1" name="demo1" type="int"
 *               type-namespace="http://www.w3.org/2001/XMLSchema" 
 *               type-prefix="xs" simple="yes"/>
 *          <param token="#in" 
 *               targetNamespace="http://wso2.org/dyn/codegen/demo"
 *               minOccurs="1" maxOccurs="1" name="demo2" type="string"
 *               type-namespace="http://www.w3.org/2001/XMLSchema" 
 *               type-prx="xs" simple="yes"/>
 *      </params>
 *
 *
 * @return the parse tree
 */
function wsf_parse_payload_for_array(DomNode $payload, DomNode $sig_node,
                    $cid2cont_type, $cid2attachments) {


    while($payload != NULL && $payload->nodeType != XML_ELEMENT_NODE) {
        $payload = $payload->nextSibling;
    }

    $is_simple_header = FALSE;
    if($sig_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) &&
            $sig_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == "yes") {
        $is_simple_header = TRUE;
    }

    $parse_tree = array();

    if($payload == NULL) {
        return $parse_tree; //the empty array
    }

    if($sig_node->hasAttributes() && !$is_simple_header) {
        //wrapped situations..    

        //again simple content should parsed differently
        if($sig_node && $sig_node->attributes->getNamedItem(WSF_CONTENT_MODEL) &&
                $sig_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT) {
            $parse_tree = wsf_infer_content_model($payload, $sig_node, NULL,
                    $cid2cont_type, $cid2attachments);
            $attr_parse_tree = wsf_infer_attributes($payload, $sig_node);
            $parse_tree = array_merge($parse_tree, $attr_parse_tree);
        }
        else {
            /* check the attributes */
            $parse_tree = wsf_infer_attributes($payload, $sig_node);

            /* go for the childs */
            $current_child = $payload->firstChild;
            while($current_child != NULL && $current_child->nodeType != XML_ELEMENT_NODE) {
                $current_child = $current_child->nextSibling;
            }
            if($current_child == NULL) {
                return $parse_tree; //just the empty array
            }

            /* check the other content model */
            $content_parse_tree = wsf_infer_content_model($current_child, $sig_node, NULL,
                        $cid2cont_type, $cid2attachments);

            $parse_tree = array_merge($content_parse_tree, $parse_tree);
        } 
        return $parse_tree;
    }
    else {
        if($is_simple_header) {
            $the_only_node = $sig_node;
        }
        else {
            // this situation meets only for non-wrapped mode as doclit-bare wsdls
            $the_only_node = $sig_node->firstChild;
        }

        //  handle simple content extension seperatly
        if($the_only_node->attributes->getNamedItem(WSF_CONTENT_MODEL) &&
                $the_only_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT) {
            $parse_tree = wsf_infer_content_model($payload, $the_only_node, NULL,
                            $cid2cont_type, $cid2attachments);
            $attr_parse_tree = wsf_infer_attributes($payload, $the_only_node);
            $parse_tree = array_merge($parse_tree, $attr_parse_tree);
        }
        else { 
            $is_simple = FALSE;
            if($the_only_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) &&
                    $the_only_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == "yes") {
                $is_simple = TRUE;
            }
            $param_type = NULL;
            if($the_only_node->attributes->getNamedItem(WSF_TYPE)) {
                $param_type = $the_only_node->attributes->getNamedItem(WSF_TYPE)->value;
            }
            if($is_simple) {
                if ($payload !== NULL) {
                    if($payload->firstChild) {
                        $original_value = $payload->firstChild->nodeValue;
                    }
                    else {
                        $original_value = "";
                    }

                    $is_list = FALSE;
                    if($the_only_node->attributes->getNamedItem(WSF_LIST) &&
                        $the_only_node->attributes->getNamedItem(WSF_LIST)->value == "yes") {
                         $is_list = TRUE;
                    }
                    $converted_value = wsf_wsdl_deserialize_string_value($param_type, $original_value, $the_only_node, $payload);

                    return $converted_value;
                }
            }
        }
    }
    

    return $parse_tree;

}


/**
 * parse payload and return an object hierarchy
 * @param $payload 
 * @param $sig_node
 *      example of sig..
 *
 *      <params wrapper-element="myDemo"
 *             wrapper-element-ns="http://wso2.org/dyn/codegen/demo"
 *             simple="no" contentModel="all">
 *          <param token="#in"
 *               targetNamespace="http://wso2.org/dyn/codegen/demo"
 *               minOccurs="1" maxOccurs="1" name="demo1" type="int"
 *               type-namespace="http://www.w3.org/2001/XMLSchema" 
 *               type-prefix="xs" simple="yes"/>
 *          <param token="#in" 
 *               targetNamespace="http://wso2.org/dyn/codegen/demo"
 *               minOccurs="1" maxOccurs="1" name="demo2" type="string"
 *               type-namespace="http://www.w3.org/2001/XMLSchema" 
 *               type-prx="xs" simple="yes"/>
 *      </params>
 *
 *
 * @param $element_type element name under which the parsing happen
 * @return the parsed result objects
 */
function wsf_parse_payload_for_class_map(DomNode $payload, DomNode $sig_node, $element_type, $classmap,
                    $cid2cont_type, $cid2attachments) {

    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "Loading in to parsing classmap");

    while($payload != NULL && $payload->nodeType != XML_ELEMENT_NODE) {
        $payload = $payload->nextSibling;
    }

    // valid for headers
    $is_simple_header = FALSE;
    if($sig_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) &&
            $sig_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == "yes") {
        $is_simple_header = TRUE;
    }

    // this situation meets only for non-wrapped mode as doclit-bare wsdls
    if($is_simple_header) {
        $the_only_node = $sig_node;
    }
    else {
        $the_only_node = $sig_node->firstChild;
    }

    if($sig_node->hasAttributes() || 
        ($the_only_node && $the_only_node->attributes->getNamedItem(WSF_CONTENT_MODEL) &&
         $the_only_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT)) {


        //this will be assigned a value inside wsf_infer_sig_node_from_xml passing by references..
        $object = NULL;

        $changed_sig_node = wsf_infer_sig_node_from_xml($sig_node, $payload, $object, $element_type, $classmap);

        // here on we consider our sig node is the changed one 
        $sig_node = $changed_sig_node;

        if($object == NULL) {
            return NULL;
        }

        // for a NULL payload return the empty object
        if($payload == NULL) {
            return $object;
        }
    }

    if($sig_node->hasAttributes() && !$is_simple_header) {
        //wrapped situations..    

        //initializing parse tree handling the nil situation
        $is_nil = $payload->getAttributeNS(WSF_XSI_NAMESPACE, "nil");
        ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "xsi:nil = ".$is_nil);
        if($is_nil && ($is_nil == "1" || $is_nil == "true" || $is_nil == "TRUE")) {
             $parse_tree = NULL;
        }
        else {
            $parse_tree = "";
        }
        // this situation gotta use the object..


        //again simple content should parsed differently
        if($sig_node && $sig_node->attributes->getNamedItem(WSF_CONTENT_MODEL) &&
                $sig_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT) {
            $parse_tree = wsf_infer_content_model($payload, $sig_node, $classmap,
                            $cid2cont_type, $cid2attachments);
            $attr_parse_tree = wsf_infer_attributes($payload, $sig_node);
            $parse_tree = array_merge($parse_tree, $attr_parse_tree);
        }
        else {
            // first parse the attributes
            ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "payload: ".wsf_test_serialize_node($payload));
            $parse_tree = wsf_infer_attributes($payload, $sig_node);
            

            $current_child = $payload->firstChild;
            while($current_child != NULL && $current_child->nodeType != XML_ELEMENT_NODE) {
                $current_child = $current_child->nextSibling;
            }
            if($current_child != NULL) {
                $content_parse_tree = wsf_infer_content_model($current_child, $sig_node, $classmap,
                                $cid2cont_type, $cid2attachments);
                $parse_tree = array_merge($content_parse_tree, $parse_tree);
            }
        }
        ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "parsed tree: ".print_r($parse_tree, TRUE));
        ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "parsed tree: "."\$parse_tree === NULL".($parse_tree === NULL));

        // all the above part of the story we were handling non-wrap types like in doclit-bare

        if($parse_tree === NULL) {
            return NULL;
        }
        foreach($parse_tree as $parsed_key => $parsed_value) {
            $object->$parsed_key = $parsed_value;
        }
        ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "parsed object: ".print_r($object, TRUE));

        return $object;

    }
    else {
        // this situation meets only for non-wrapped mode as doclit-bare wsdls

        //  handle simple content extension seperatly
        if($the_only_node && $the_only_node->attributes->getNamedItem(WSF_CONTENT_MODEL) &&
                $the_only_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value == WSF_SIMPLE_CONTENT) {
            $parse_tree = wsf_infer_content_model($payload, $the_only_node, $classmap,
                            $cid2cont_type, $cid2attachments);
            $attr_parse_tree = wsf_infer_attributes($payload, $the_only_node);
            $parse_tree = array_merge($parse_tree, $attr_parse_tree);

            foreach($parse_tree as $parsed_key => $parsed_value) {
                $object->$parsed_key = $parsed_value;
            }
            return $object;
        }

        else {
            $is_simple = FALSE;
            if($the_only_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) &&
                    $the_only_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == "yes") {
                $is_simple = TRUE;
            }
            $param_type = NULL;
            if($the_only_node->attributes->getNamedItem(WSF_TYPE)) {
                $param_type = $the_only_node->attributes->getNamedItem(WSF_TYPE)->value;
            }
            if($is_simple) {
                if ($payload !== NULL) {
                    if($payload->firstChild) {
                        $original_value = $payload->firstChild->nodeValue;
                    }
                    else {
                        $original_value = "";
                    }
                    $is_list = FALSE;
                    if($the_only_node->attributes->getNamedItem(WSF_LIST) &&
                        $the_only_node->attributes->getNamedItem(WSF_LIST)->value == "yes") {
                         $is_list = TRUE;
                    }
                    $converted_value = wsf_wsdl_deserialize_string_value($param_type, $original_value, $the_only_node, $payload);

                    return $converted_value;
                }
            }
        }
    }

    return NULL;
}


/**
 * wsf_parse_payload_for_unknown_array
 * @param $current_node node to parse
 * @returns array of parsed data
 */
function wsf_parse_payload_for_unknown_array(DomNode $current_node) {
    $param_child = array();

    while($current_node != NULL && $current_node->nodeType != XML_ELEMENT_NODE) {
        $current_node = $current_node->nextSibling;
    }

    foreach($current_node->childNodes as $child) {
        if($child->nodeType == XML_TEXT_NODE) {
            $node_value = $child->nodeValue;
            return $node_value;
        }


        $name = $child->tagName;
        $needle = strpos($name, ':');

        if($needle !== FALSE) {
            $name = substr($name, $needle + 1);
        }

        if($child->nodeType ==  XML_ELEMENT_NODE) {
            $node_value = wsf_parse_payload_for_unknown_array($child);
        }
        else
        {
            $node_value = NULL;
        }

        if($node_value !== NULL) {
            if($param_child[$name] === NULL) {
                $param_child[$name] = $node_value;
            }
            else
            {
                if(is_array($param_child[$name]) && !wsf_is_map($param_child[$name])) {
                    $i = count($param_child[$name]);
                    $param_child[$name][$i] = $node_value;
                }
                else
                {
                    $tmp = $param_child[$name];
                    $param_child[$name] = array($tmp, $node_value);
                }
            }
        }
    }

    return $param_child;
}

/**
 * wsf_parse_payload_for_unknown_class_map
 * @param $current_node node to parse
 * @returns array of parsed data
 */
function wsf_parse_payload_for_unknown_class_map(DomNode $current_node, $element_type, $classmap) {
    $class_name = NULL;
    if(is_array($classmap) && array_key_exists($element_type, $classmap)) {
        $class_name = $classmap[$element_type];
    }
    if(!isset($class_name) || $class_name == NULL) {
        $class_name = $element_type;
    }
   
    try
    {
        $ref_class = new ReflectionClass($class_name);
        if ($ref_class->isInstantiable()) {
            $object = $ref_class->newInstance();

        }
    } catch(Exception $e) {
        $object = new WSFUnknownSchemaConstruct();
    }

    if($object == NULL) {
        return NULL;
    }

    while($current_node != NULL && $current_node->nodeType != XML_ELEMENT_NODE) {
        $current_node = $current_node->nextSibling;
    }

    foreach($current_node->childNodes as $child) {
        if($child->nodeType == XML_TEXT_NODE) {
            $node_value = $child->nodeValue;
            return $node_value;
        }


        $name = $child->tagName;
        $needle = strpos($name, ':');

        if($needle !== FALSE) {
            $name = substr($name, $needle + 1);
        }

        if($child->nodeType ==  XML_ELEMENT_NODE) {
            $node_value = wsf_parse_payload_for_unknown_class_map($child, $name, $classmap);
        }
        else
        {
            $node_value = NULL;
        }

        if($node_value !== NULL) {
            if($object->$name === NULL) {
                $object->$name = $node_value;
            }
            else
            {
                if(is_array($object->$name) && !wsf_is_map($object->$name)) {
                    $i = count($object->$name);
                    $tmp = $object->$name;
                    $tmp[$i] = $node_value;
                    $object->$name = $tmp;
                }
                else
                {
                    $tmp = $object->$name;
                    $object->$name = array($tmp, $node_value);
                }
            }
        }
    }

    return $object;
}

/** 
 * Do deserialization over simple schema types and move to the next element
 * @param $current_child is the relevent xml node to parse, and 
 *                   move the current_child to the next child
 * @param $sig_param_node the sig model for the param
 * @return the result simple type value.
 */

function wsf_deserialize_simple_types(&$current_child, DomNode $sig_param_node, $classmap,
                    $cid2cont_type, $cid2attachments) {

    $ret_val = NULL;
    $target_namespace = NULL;
    $min_occurs = 1;
    $max_occurs = 1;
    $sig_param_attris = $sig_param_node->attributes;
    $param_type = NULL;
    $param_name = NULL;
    $is_list = FALSE;
    $content_model = NULL;

    if($sig_param_attris->getNamedItem(WSF_NAME)) {
        $param_name =
            $sig_param_attris->getNamedItem(WSF_NAME)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_TARGETNAMESPACE)) {
         $target_namespace =
            $sig_param_attris->getNamedItem(WSF_TARGETNAMESPACE)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_MIN_OCCURS)) {
         $min_occurs =
            $sig_param_attris->getNamedItem(WSF_MIN_OCCURS)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_MAX_OCCURS)) {
         $max_occurs =
            $sig_param_attris->getNamedItem(WSF_MAX_OCCURS)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_TYPE)) {
         $param_type =
            $sig_param_attris->getNamedItem(WSF_TYPE)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_LIST) &&
        $sig_param_attris->getNamedItem(WSF_LIST)->value == "yes") {
         $is_list = TRUE;
    }


    if($sig_param_attris->getNamedItem(WSF_CONTENT_MODEL)) {
        $content_model = $sig_param_attris->getNamedItem(WSF_CONTENT_MODEL)->value;
    }

    if($max_occurs > 1 || $max_occurs == "unbounded") {
        $i = 0;
        $tmp_array = array();
        while($current_child !== NULL && $current_child->localName == $param_name) {

			if ($content_model == WSF_SIMPLE_CONTENT) {
				$param_type = $sig_param_attris->getNamedItem(WSF_EXTENSION)->value;
			}
            //modify the mtom node to keep only the cid
            if($param_type == WSF_XSD_BASE64) {
                $include_node = $current_child->firstChild;
                while($include_node && $include_node->nodeType != XML_ELEMENT_NODE) {
                    $include_node = $include_node->nextSibling;
                }
                if($include_node) {
                    if($include_node->localName == "Include") {
                        $href = $include_node->getAttribute("href");
                        $cid = substr($href, 4);
                        $current_child->removeChild($include_node);

                        $current_child->nodeValue = $cid;
                    }
                }
            }
            if($content_model == WSF_SIMPLE_CONTENT) {
                if($classmap) {
                    $converted_value = wsf_parse_payload_for_class_map($current_child, $sig_param_node,
                                    $param_type, $classmap, $cid2cont_type, $cid2attachments);
                }
                else { 
                    $converted_value = wsf_parse_payload_for_array($current_child, $sig_param_node, 
                                    $cid2cont_type, $cid2attachments);
                }
            }
            else if($current_child->firstChild) {
                $converted_value =  wsf_wsdl_deserialize_string_value($param_type,
                        $current_child->firstChild->wholeText, $sig_param_node, $current_child);
            }
            else{
                // handling nil element 
                $converted_value = wsf_wsdl_deserialize_string_value($param_type, "", $sig_param_node, $current_child);
            }
            $tmp_array[$i++] = $converted_value;
            $current_child = $current_child->nextSibling;
            while($current_child != NULL && $current_child->nodeType != XML_ELEMENT_NODE) {
                $current_child = $current_child->nextSibling;
            }
        }
        $ret_val = $tmp_array;
        
    }
    else {

		if ($content_model == WSF_SIMPLE_CONTENT) {
			$param_type = $sig_param_attris->getNamedItem(WSF_EXTENSION)->value;
		}
        //modify the mtom node to keep only the cid
        if($param_type == WSF_XSD_BASE64) {
            $include_node = $current_child->firstChild;
            while($include_node && $include_node->nodeType != XML_ELEMENT_NODE) {
                $include_node = $include_node->nextSibling;
            }
            if($include_node) {
                if($include_node->localName == "Include") {
                    $href = $include_node->getAttribute("href");
                    $cid = substr($href, 4);
                    $current_child->removeChild($include_node);

                    $current_child->nodeValue = $cid;
                }
            }
        }

        if($content_model == WSF_SIMPLE_CONTENT) {
            if($classmap) {
                $converted_value = wsf_parse_payload_for_class_map($current_child, $sig_param_node,
                                $param_type, $classmap, $cid2cont_type, $cid2attachments);
            }
            else { 
                $converted_value = wsf_parse_payload_for_array($current_child, $sig_param_node,
                                    $cid2cont_type, $cid2attachments);
            }
        }
        else if($current_child->firstChild) {
            $converted_value =  wsf_wsdl_deserialize_string_value($param_type,
                    $current_child->firstChild->wholeText, $sig_param_node, $current_child);
        }
        else
        {
            // handling nil element 
            $converted_value = wsf_wsdl_deserialize_string_value($param_type, "", $sig_param_node, $current_child);
        }
        $ret_val = $converted_value;
        $current_child = $current_child->nextSibling;
        while($current_child != NULL && $current_child->nodeType != XML_ELEMENT_NODE) {
            $current_child = $current_child->nextSibling;
        }
    }
    return $ret_val;
}

/** 
 * Do deserialization over complex schema types and move to the next element
 * @param $current_child is the relevent xml node to parse, and 
 *                   move the current_child to the next child
 * @param $sig_param_node the sig model for the param
 *
 * @param $classmap the classmap passed by the user
 * @return the result complex type value.
 */
function wsf_deserialize_complex_types(&$current_child, DomNode $sig_param_node, $classmap,
                    $cid2cont_type, $cid2attachments) {

    $ret_val = NULL;
    $target_namespace = NULL;
    $min_occurs = 1;
    $max_occurs = 1;
    $sig_param_attris = $sig_param_node->attributes;
    $param_type = NULL;
    $param_name = NULL;
    $is_attribute = FALSE;
    $nillable = FALSE;

    if($sig_param_attris->getNamedItem(WSF_NAME)) {
        $param_name =
            $sig_param_attris->getNamedItem(WSF_NAME)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_TARGETNAMESPACE)) {
         $target_namespace =
            $sig_param_attris->getNamedItem(WSF_TARGETNAMESPACE)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_MIN_OCCURS)) {
         $min_occurs =
            $sig_param_attris->getNamedItem(WSF_MIN_OCCURS)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_MAX_OCCURS)) {
         $max_occurs =
            $sig_param_attris->getNamedItem(WSF_MAX_OCCURS)->value;
    }

    if($sig_param_attris->getNamedItem(WSF_TYPE)) {
         $param_type =
            $sig_param_attris->getNamedItem(WSF_TYPE)->value;
    }
    if($sig_param_attris->getNamedItem(WSF_XSD_NILLABLE) &&
            $sig_param_attris->getNamedItem(WSF_XSD_NILLABLE)->value = "true") {
        $nillable = TRUE;
    }

    if($sig_param_attris->getNamedItem(WSF_ATTRIBUTE) &&
        $sig_param_attris->getNamedItem(WSF_ATTRIBUTE)->value == "yes") {
        $is_attribute = TRUE;
    }

    if($max_occurs > 1 || $max_occurs == "unbounded") {
        $i = 0;
        $tmp_array = array();
        while($current_child !== NULL && $current_child->localName == $param_name) {
            if($sig_param_node->hasChildNodes()) {
                if($classmap) {
                    $converted_value = wsf_parse_payload_for_class_map($current_child, $sig_param_node,
                                        $param_type, $classmap, $cid2cont_type, $cid2attachments);
                }
                else { 
                    $converted_value = wsf_parse_payload_for_array($current_child, $sig_param_node,
                                        $cid2cont_type, $cid2attachments);
                }

            }
            else {
                if($classmap) {
                    $converted_value = wsf_parse_payload_for_unknown_class_map($current_child, $param_type, $classmap);
                }
                else {
                    $converted_value = wsf_parse_payload_for_unknown_array($current_child);
                }
            }
            if(!$current_child->firstChild && !$current_child->hasAttributes()) {
                
                // handling nil element 
                $converted_value = wsf_wsdl_deserialize_string_value($param_type, "", $sig_param_node, $current_child);
            }
            $tmp_array[$i++] = $converted_value;
            $current_child = $current_child->nextSibling;
            while($current_child != NULL && $current_child->nodeType != XML_ELEMENT_NODE) {
                $current_child = $current_child->nextSibling;
            }
        }
        $ret_val = $tmp_array;
    }
    else {
        if($sig_param_node->hasChildNodes()) {
            if($classmap) {
                $converted_value = wsf_parse_payload_for_class_map($current_child, $sig_param_node,
                                    $param_type, $classmap, $cid2cont_type, $cid2attachments);
            }
            else { 
                $converted_value = wsf_parse_payload_for_array($current_child, $sig_param_node,
                                $cid2cont_type, $cid2attachments);
            }
        }
        else {
            if($classmap) {
                $converted_value = wsf_parse_payload_for_unknown_class_map($current_child, $param_type, $classmap);
            }
            else {
                $converted_value = wsf_parse_payload_for_unknown_array($current_child);
            }
        }
        if(!$current_child->firstChild && !$current_child->hasAttributes()) {
            // handling nil element 
            $converted_value = wsf_wsdl_deserialize_string_value($param_type, "", $sig_param_node, $current_child);
        }
        $ret_val = $converted_value;
        $current_child = $current_child->nextSibling;
        while($current_child != NULL && $current_child->nodeType != XML_ELEMENT_NODE) {
            $current_child = $current_child->nextSibling;
        }
    }
    return $ret_val;
}


/**
 * handle content model in parsing, attributes are too hanldled in this level
 * @param $current_child, starting child element to handle...
 * @param $sig_node the sig model for the content model 
 *
 * @param $classmap the user passed classmap
 */
function wsf_infer_content_model(DomNode &$current_child, DomNode $sig_node=NULL, $classmap,
                    $cid2cont_type, $cid2attachments) {

    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "parsing content model");

    $parse_tree = array();
    if($sig_node == NULL) {
        return $parse_tree; //just the empty array
    }

    $content_model = WSF_WSDL_SEQUENCE;

    if($sig_node->attributes->getNamedItem(WSF_CONTENT_MODEL)) {
        $content_model = $sig_node->attributes->getNamedItem(WSF_CONTENT_MODEL)->value;
    }

    // simple content extension should be treated differently
    if($content_model == WSF_SIMPLE_CONTENT) {
            
        $param_type = $sig_node->attributes->getNamedItem(WSF_EXTENSION)->value;
        $text_node = $current_child->firstChild;

        if($text_node && $text_node->nodeType == XML_TEXT_NODE) {
            $text_value = $text_node->nodeValue;
            /* type conversion is needed */

            $is_list = FALSE;
            if($sig_node->attributes->getNamedItem(WSF_LIST) &&
                $sig_node->attributes->getNamedItem(WSF_LIST)->value == "yes") {
                 $is_list = TRUE;
            }

            $converted_value =  wsf_wsdl_deserialize_string_value($param_type, $text_value, $sig_node, $current_child);
            
            // we let the deserialize_type_info to extract out the mtom information
            wsf_deserialize_type_info($param_type, $parse_tree, WSF_SIMPLE_CONTENT_VALUE, $converted_value,
                            $cid2cont_type, $cid2attachments);
            //$parse_tree[WSF_SIMPLE_CONTENT_VALUE] = $converted_value;
        }
        //special for the simple content extension, to be more tested..
    }

    $first_child = $current_child;

    if($sig_node->hasChildNodes()) {
        foreach($sig_node->childNodes as $sig_param_node) {

            if($sig_param_node->nodeName == WSF_PARAM) {
                $is_simple = FALSE;
                $is_attribute = FALSE;
                $param_name = NULL;
                $param_type = NULL;
                $min_occurs = 1;
                $max_occurs = 1;

                if($sig_param_node->attributes->getNamedItem(WSF_NAME)) {
                    $param_name = $sig_param_node->attributes->getNamedItem(WSF_NAME)->value;
                }
                if($sig_param_node->attributes->getNamedItem(WSF_TYPE)) {
                    $param_type = $sig_param_node->attributes->getNamedItem(WSF_TYPE)->value;
                }
                if($sig_param_node->attributes->getNamedItem(WSF_MIN_OCCURS)) {
                    $min_occurs = $sig_param_node->attributes->getNamedItem(WSF_MIN_OCCURS)->value;
                }
                if($sig_param_node->attributes->getNamedItem(WSF_MAX_OCCURS)) {
                    $max_occurs = $sig_param_node->attributes->getNamedItem(WSF_MAX_OCCURS)->value;
                }
                if($sig_param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) &&
                        $sig_param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == "yes") {
                    $is_simple = TRUE;
                }

                if($sig_param_node->attributes->getNamedItem(WSF_ATTRIBUTE) &&
                    $sig_param_node->attributes->getNamedItem(WSF_ATTRIBUTE)->value == "yes") {
                     $is_attribute = TRUE;
                }
                // for any types no content model can be specified..
                if($param_type == "anyType" && $current_child !== NULL) {
                    if($max_occurs > 1 || $max_occurs == "unbounded") {
                        $tag_name = $current_child->localName;
                        $tmp_array = array();
                        $i = 0;
                        while($current_child !== NULL && $current_child->localName == $param_name) {
                            $converted_value = wsf_parse_payload_for_unknown_array($current_child);
                            $tmp_array[$i++] = $converted_value;
                            $current_child = $current_child->nextSibling;
                            while($current_child != NULL && $current_child->nodeType != XML_ELEMENT_NODE) {
                                $current_child = $current_child->nextSibling;
                            }
                        }
                        $parse_tree[$param_name] = $tmp_array;
                        continue;
                    }
                    else
                    {
                        $tag_name = $current_child->localName;
                        if($param_name == $tag_name) {
                            $converted_value = wsf_parse_payload_for_unknown_array($current_child);
                            $parse_tree[$param_name] = $converted_value;
                            continue;
                        }
                    }
                }

                if($content_model == WSF_WSDL_SEQUENCE && $current_child !== NULL) {
                    if($param_name == $current_child->localName) {

                        if($is_simple) {
                            // this moves the current_child pointer to the next child..
                            $param_value = wsf_deserialize_simple_types($current_child, $sig_param_node, $classmap, 
                                                        $cid2cont_type, $cid2attachments);
                            // we let the deserialize_type_info to extract out the mtom information
                            wsf_deserialize_type_info($param_type, $parse_tree, $param_name, $param_value,
                                            $cid2cont_type, $cid2attachments);
                        }
                        else {
                            $parse_tree[$param_name] = wsf_deserialize_complex_types($current_child, $sig_param_node, $classmap,
                                                        $cid2cont_type, $cid2attachments);
                        }
                    }
                    else
                    {
                        if($min_occurs != 0) {
                            ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "minOccurs != 0 element ". $param_name ." doesn't exist in the sequence.");
                            if($current_child->localName != NULL){
                                ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, $current_child->localName. " is found in place of ". $param_name);
                            }
                        }
                    }
                }
                else if($content_model == WSF_WSDL_ALL) {
                    $found = FALSE;
                    $current_child = $first_child;

                    while($current_child !== NULL) {
                        if($current_child->nodeType == XML_TEXT_NODE) {
                            continue;
                        }
                        if($param_name == $current_child->localName) {
                            if($is_simple) {
                                // this moves the current_child pointer to the next child..
                                $param_value = wsf_deserialize_simple_types($current_child, $sig_param_node, $classmap,
                                                            $cid2cont_type, $cid2attachments);

                                wsf_deserialize_type_info($param_type, $parse_tree, $param_name, $param_value,
                                            $cid2cont_type, $cid2attachments);
                            }
                            else {
                                $parse_tree[$param_name] = wsf_deserialize_complex_types($current_child, $sig_param_node, $classmap,
                                                            $cid2cont_type, $cid2attachments);
                            }
                            $found = TRUE;
                            break; //found and no need to investigate more..
                        }

                        $current_child = $current_child->nextSibling;
                    }
                    if(!$found) {
                        if($min_occurs == 0) {
                            // if array_key doesn't exist that mean minOccurs = 1;
                            ws_log_write(__FILE__, __LINE__, WSF_LOG_ERROR, "minOccurs != 0 element ". $param_name ." doesn't exist.");
                        }  
                    }
                }
                else if($content_model == "choice" && $current_child != NULL) {
                    if($param_name == $current_child->localName) {
                        if($is_simple) {
                            // this moves the current_child pointer to the next child..
                            $param_value = wsf_deserialize_simple_types($current_child, $sig_param_node, $classmap,
                                                                $cid2cont_type, $cid2attachments);
                            wsf_deserialize_type_info($param_type, $parse_tree, $param_name, $param_value,
                                            $cid2cont_type, $cid2attachments);
                        }
                        else {
                            $parse_tree[$param_name] = wsf_deserialize_complex_types($current_child, $sig_param_node, $classmap,
                                                                $cid2cont_type, $cid2attachments);
                        }

                        while($current_child != NULL && $current_child->nodeType != XML_ELEMENT_NODE) {
                            $current_child = $current_child->nextSibling;
                        }
                        //finally found what they were asking for, no need to continue any further..
                        break;
                    }
                }
            }
            else if($sig_param_node->nodeName == WSF_INNER_CONTENT && $current_child) {
                $tmp_tree = wsf_infer_content_model($current_child, $sig_param_node, $classmap,
                                $cid2cont_type, $cid2attachments);
                $parse_tree = array_merge($parse_tree, $tmp_tree);
            }
            else if($sig_param_node->nodeName == WSF_INHERITED_CONTENT && $current_child) {
                $tmp_tree = wsf_infer_content_model($current_child, $sig_param_node, $classmap,
                                $cid2cont_type, $cid2attachments);
                $parse_tree = array_merge($parse_tree, $tmp_tree);
            }
        }
    }
 
    return $parse_tree;
}

/**
 * infer attributes
 * @param $parent_node parent node containing the attributes
 * @param $sig_node sig node containing the attribute signature
 * @return $parset_tree the parsed tree of attributes
 */
function wsf_infer_attributes(DomNode $parent_node, DomNode $sig_node) {
    $parse_tree = array();
    if($sig_node->hasChildNodes()) {
        foreach($sig_node->childNodes as $sig_param_node) {

            if($sig_param_node->nodeName == WSF_PARAM) {
                $is_simple = FALSE;
                $is_attribute = FALSE;
                $param_name = NULL;
                $param_type = NULL;

                if($sig_param_node->attributes->getNamedItem(WSF_NAME)) {
                    $param_name = $sig_param_node->attributes->getNamedItem(WSF_NAME)->value;
                }
                if($sig_param_node->attributes->getNamedItem(WSF_TYPE)) {
                    $param_type = $sig_param_node->attributes->getNamedItem(WSF_TYPE)->value;
                }
                if($sig_param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE) &&
                        $sig_param_node->attributes->getNamedItem(WSF_WSDL_SIMPLE)->value == "yes") {
                    $is_simple = TRUE;
                }

                if($sig_param_node->attributes->getNamedItem(WSF_ATTRIBUTE) &&
                    $sig_param_node->attributes->getNamedItem(WSF_ATTRIBUTE)->value == "yes") {
                     $is_attribute = TRUE;
                }
                if($is_attribute) {
                    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, $param_name.":".wsf_test_serialize_node($parent_node));
                    if($parent_node && $parent_node->attributes->getNamedItem($param_name)) {
                        $original_value = $parent_node->attributes->getNamedItem($param_name)->value;

                        $is_list = FALSE;
                        if($sig_param_node->attributes->getNamedItem(WSF_LIST) &&
                            $sig_param_node->attributes->getNamedItem(WSF_LIST)->value == "yes") {
                             $is_list = TRUE;
                        }
                        $converted_value =  wsf_wsdl_deserialize_string_value($param_type, $original_value, $sig_param_node, NULL);
                        $parse_tree[$param_name] = $converted_value;
                    }
                }
            }else if($sig_param_node->nodeName == WSF_INHERITED_CONTENT) {
		 $parse_tree = array_merge($parse_tree, wsf_infer_attributes($parent_node, $sig_param_node));
	    } 
        }
    }
    return $parse_tree;
}

/**
 * deserialize the php value from the string value to the given xsd type value
 * @param $xsd_type, xsd type the value hold
 * @param $data_value, the data_value with the string type
 * @param $sig_param_node, whether the type is a list or not..
 * @param $value_parent_node, used to check whether the value is nil, if available
 * @return deserialized to given php type value
 */
function wsf_wsdl_deserialize_string_value($xsd_type, $data_value, $sig_param_node, $value_parent_node) {

    if($value_parent_node && $value_parent_node->nodeType == XML_ELEMENT_NODE) {
        $is_nil = $value_parent_node->getAttributeNS(WSF_XSI_NAMESPACE, "nil");
        ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "xsi:nil = ".$is_nil);
        if($is_nil && ($is_nil == "1" || $is_nil == "true" || $is_nil == "TRUE")) {
             return NULL;
        }
    }

    $is_list = FALSE;
    if($sig_param_node->attributes->getNamedItem(WSF_LIST) &&
        $sig_param_node->attributes->getNamedItem(WSF_LIST)->value == "yes") {
         $is_list = TRUE;
    }

    //currently union handling is not done correctly
    $is_union = FALSE;
    if($sig_param_node->attributes->getNamedItem(WSF_UNION) &&
        $sig_param_node->attributes->getNamedItem(WSF_UNION)->value == "yes") {
         $is_union = TRUE;
    }

    if($is_list) {
        $tmp_value = explode(" ", $data_value);
        $data_values = $tmp_value;

        $converted_value = array();
        foreach($data_values as $this_data_value) {
            $converted_value[] = wsf_convert_string_to_php_type($xsd_type, $this_data_value);
        }
    }
    else {
        $converted_value = wsf_convert_string_to_php_type($xsd_type, $data_value);
    }

    
    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "deserialized to ".$converted_value);

    return $converted_value;
}

/**
 * deserialize the php value from the string value to the given xsd type value
 * @param $xsd_type, xsd type the value hold
 * @param $data_value, the data_value with the string type
 * @return cast type
 */
function wsf_convert_string_to_php_type($xsd_type, $data_value) {
	// we treat dateTime value differently
	if($xsd_type == "dateTime" || $xsd_type == "date" || $xsd_type == "time") {
		$date_value = str_replace("T", " ", $data_value);
		$converted_value = strtotime($date_value);
	}
	else {
		$xsd_php_mapping_table = wsf_wsdl_util_xsd_to_php_type_map();
		
		$converted_value = $data_value;
		if(array_key_exists($xsd_type, $xsd_php_mapping_table)) {
			$type = $xsd_php_mapping_table[$xsd_type];
			if($type == 'integer') {
				$converted_value = (int)($data_value);
			}
			else if ($type == 'float') {
				$converted_value = (float)($data_value);
			}
			else if ($type == 'boolean') {
				$converted_value = ($data_value === "true");
			}
			else if ($type == 'string') {
					$converted_value = $data_value;
				}
				else {
					$converted_value = $data_value;
				}
		}
	}
    return $converted_value;
}

/**
 * this handles the mtom attachement + other type info deserialization
 */
function wsf_deserialize_type_info($param_type, &$parse_tree, $param_name, $param_value,
                $cid2cont_type, $cid2attachments)
{
    if($param_type == WSF_XSD_BASE64 && !is_array($param_value) && !is_object($param_value)) {
        if(!is_array($cid2attachments) || count($cid2attachments) == 0) {
            // this mean no attachments, values are sent as MTOM attachments..
            $parse_tree[$param_name. WSF_POSTFIX_BASE64] = $param_value;
            $parse_tree[$param_name] = base64_decode($param_value);
        }
        else {
            // if attachment array exists, $param_value contains the cid
          
            $attachemnt_value = NULL;
            $content_type = NULL;
            if(array_key_exists($param_value, $cid2attachments)) {
                $attachment_value = $cid2attachments[$param_value];
            }
            if(array_key_exists($param_value, $cid2cont_type)) {
                $content_type = $cid2cont_type[$param_value];
            }

            if($attachment_value !== NULL && $content_type !== NULL) {
                $parse_tree[$param_name] = $attachment_value;
                $parse_tree[$param_name.WSF_POSTFIX_BASE64] = base64_encode($attachment_value);

                //filling the content type
                $parse_tree[WSF_CONTENT_TYPE] = $content_type;
            }
        }
    }
    else {
        $parse_tree[$param_name] = $param_value;
    }
}

/**
 * Extract the sig node looking at the incomming xml type
 * @param array $sig_model as a DomNode
 * @param DomNode $parent_node - The parent node to check for the content 
 * @param $object mixed relevent object instance(passed by reference)
 * @param $original_type the type in the first place
 * @param $classmap - the type name to class map
 */
function wsf_infer_sig_node_from_xml($sig_node, $parent_node, &$object, $original_type, $classmap) {

    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "Calling infer sig mode from xml");

    // first it is to be safte to keep the element type as the original type and the sig node too
    $the_element_type = $original_type;
    $the_sig_node = $sig_node;


    // second we have to find whether there is xsi:type in the xml.
    $xml_type = NULL;
    $xsi_type_attri_node =  $parent_node->getAttributeNodeNS(WSF_XSI_NAMESPACE, "type");
    if($xsi_type_attri_node) {
        $xml_type = $xsi_type_attri_node->value;
    }
    
    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "xml type $xml_type");

    if($xml_type) {
        //looks like there is a type declared in the xml, so find possible types..
      
        $inheriting_type_found = FALSE;
        
        $xml_type_local_value = $xml_type;
        $pos = strpos($xml_type, ":");
        if($pos !== FALSE) {
            $xml_type_local_value = substr($xml_type, $pos + 1);
        }
        $sig_child_nodes = $sig_node->childNodes;
        foreach($sig_child_nodes as $sig_child_node) {
            
            if($sig_child_node->localName == WSF_INHERITING_TYPE) {

                $sig_child_attris = $sig_child_node->attributes;
                $type_name = $type_ns = "";
                if($sig_child_attris->getNamedItem(WSF_XSI_TYPE)) {
                     $type_name = $sig_child_attris->getNamedItem(WSF_XSI_TYPE)->value;
                }
                
                if($xml_type_local_value == $type_name) {
                    ws_log_write(__FILE__, __LINE__, WSF_LOG_DEBUG, "type name $type_name found ");
                    $the_element_type = $type_name;
                    $the_sig_node = $sig_child_node;
                    $inheriting_type_found = TRUE;
                    break;
                }
            }
        }
    }


    if(is_array($classmap) && array_key_exists($the_element_type, $classmap)) {
        $class_name = $classmap[$the_element_type];
    }
    if(!isset($class_name) || $class_name == NULL) {
        $class_name = $the_element_type;
    }

    try {
        $ref_class = new ReflectionClass($class_name);
        if ($ref_class->isInstantiable()) {
            $object = $ref_class->newInstance();

        }
    } catch(Exception $e) {
        $object = new WSFUnknownSchemaConstruct();
    }
    
    return $the_sig_node;
}
//-----------------------------------------------------------------------------


?>
