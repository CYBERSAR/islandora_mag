<?php
/**
 * Take a xml string and retrieve an array like $form_values for MAG datasteam
 * @param string $xml
 */
function mag_xml_to_array($xml_str) {
	$xml_multiarray_values = xml2array($xml_str);
		
	$xml_array_values = array();

	//this is needed because all "dc:" field must haven't a prefix
	$array_minus_bib = array_key_remove($xml_multiarray_values, "bib");
	$array_flatten_key_dc = array_flatten($xml_multiarray_values['metadigit']["bib"]);

	$array_flatten_key_prefix = array_flatten_sep($array_minus_bib['metadigit'], ":", "mag:");

	$xml_array_values = array_merge($array_flatten_key_dc, $array_flatten_key_prefix);

	$default_lang = variable_get("islandora_mag_default_metadigit_lang", "it");

	$xml_array_values["metadigit_lang"] = isset($xml_multiarray_values["metadigit_attr"]["xml:lang"]) ? $xml_multiarray_values["metadigit_attr"]["xml:lang"] : $default_lang;


	return $xml_array_values;
}

/**
 * Walks through a multidimensional array and takes only leafs with right keys
 *
 * @param array $array
 * 		multidimensional array
 * @param string $sep
 * 		key separator
 * @param string $pre
 * 		optional key prefix
 * @return array $return
 * 		monodimensional array
 */
function array_flatten_sep($array, $sep, $pre = "") {
	$result = array();
	$stack = array();
	array_push($stack, array("", $array));

	while (count($stack) > 0) {
		list($prefix, $array) = array_pop($stack);

		foreach ($array as $key => $value) {
			$new_key = $prefix . strval($key);

			if (is_array($value))
			array_push($stack, array($new_key . $sep, $value));
			else
			$result[$pre . $new_key] = $value;
		}
	}

	return $result;
}

/**
 * Flattens an array, or returns FALSE on fail.
 *
 * @param array $array
 */
function array_flatten($array) {
	if (!is_array($array)) {
		return FALSE;
	}
	$result = array();
	foreach ($array as $key => $value) {
		//necessary because dc:subject can be an array
		if (is_array($value) and ($key != "dc:subject")) {
			$result = array_merge($result, array_flatten($value));
		}
		else {
			$result[$key] = $value;
		}
	}
	return $result;
}

/**
 * Remove a portion of a n-dimentional array based on the value of the key
 * @param unknown_type $array
 * @param unknown_type $key
 */
function array_key_remove($array, $key) {
	$holding = array();

	foreach($array as $k => $v){
		if (is_array($v) and $key != $k) {
			$holding [$k] = array_key_remove($v, $key);
		}
		elseif ($key != $k){
			$holding[$k] = $v; // removes an item by combing through the array in order and saving the good stuff
		}
	}
	return $holding; // only pass back the holding array if we didn't find the value
}


/**
 * xml2array() will convert the given XML text to an array in the XML structure.
 * Link: http://www.bin-co.com/php/scripts/xml2array/
 * Arguments : $contents - The XML text
 *                $get_attributes - 1 or 0. If this is 1 the function will get the attributes as well as the tag values - this results in a different array structure in the return value.
 *                $priority - Can be 'tag' or 'attribute'. This will change the way the resulting array sturcture. For 'tag', the tags are given more importance.
 * Return: The parsed XML in an array form. Use print_r() to see the resulting array structure.
 */
function xml2array($contents, $get_attributes=1, $priority = 'tag') {
	if(!$contents) return array();

	if(!function_exists('xml_parser_create')) {
		//print "'xml_parser_create()' function not found!";
		return array();
	}

	//Get the XML parser of PHP - PHP must have this module for the parser to work
	$parser = xml_parser_create('');
	xml_parser_set_option($parser, XML_OPTION_TARGET_ENCODING, "UTF-8"); # http://minutillo.com/steve/weblog/2004/6/17/php-xml-and-character-encodings-a-tale-of-sadness-rage-and-data-loss
	xml_parser_set_option($parser, XML_OPTION_CASE_FOLDING, 0);
	xml_parser_set_option($parser, XML_OPTION_SKIP_WHITE, 1);
	xml_parse_into_struct($parser, trim($contents), $xml_values);
	xml_parser_free($parser);

	if(!$xml_values) return;//Hmm...

	//Initializations
	$xml_array = array();
	$parents = array();
	$opened_tags = array();
	$arr = array();

	$current = &$xml_array; //Refference

	//Go through the tags.
	$repeated_tag_index = array();//Multiple tags with same name will be turned into an array
	foreach($xml_values as $data) {
		unset($attributes, $value);//Remove existing values, or there will be trouble

		//This command will extract these variables into the foreach scope
		// tag(string), type(string), level(int), attributes(array).
		extract($data);//We could use the array by itself, but this cooler.

		$result = array();
		$attributes_data = array();

		if(isset($value)) {
			if($priority == 'tag')
			$result = $value;
			else
			$result['value'] = $value; //Put the value in a assoc array if we are in the 'Attribute' mode
		}

		//Set the attributes too.
		if(isset($attributes) and $get_attributes) {
			foreach($attributes as $attr => $val) {
				if($priority == 'tag')
				$attributes_data[$attr] = $val;
				else
				$result['attr'][$attr] = $val; //Set all the attributes in a array called 'attr'
			}
		}

		//See tag status and do the needed.
		if($type == "open") { //The starting of the tag '<tag>'
			$parent[$level-1] = &$current;
			if(!is_array($current) or (!in_array($tag, array_keys($current)))) { //Insert New tag
				$current[$tag] = $result;
				if($attributes_data) $current[$tag. '_attr'] = $attributes_data;
				$repeated_tag_index[$tag.'_'.$level] = 1;

				$current = &$current[$tag];

			}
			else { //There was another element with the same tag name
				if(isset($current[$tag][0])) { //If there is a 0th element it is already an array
					$current[$tag][$repeated_tag_index[$tag.'_'.$level]] = $result;
					$repeated_tag_index[$tag.'_'.$level]++;
				}
				else { //This section will make the value an array if multiple tags with the same name appear together
					$current[$tag] = array($current[$tag],$result);//This will combine the existing item and the new item together to make an array
					$repeated_tag_index[$tag.'_'.$level] = 2;

					if(isset($current[$tag.'_attr'])) { //The attribute of the last(0th) tag must be moved as well
						$current[$tag]['0_attr'] = $current[$tag.'_attr'];
						unset($current[$tag.'_attr']);
					}

				}
				$last_item_index = $repeated_tag_index[$tag.'_'.$level]-1;
				$current = &$current[$tag][$last_item_index];
			}
		}
		elseif($type == "complete") { //Tags that ends in 1 line '<tag />'
			//See if the key is already taken.
			if(!isset($current[$tag])) { //New Key
				$current[$tag] = $result;
				$repeated_tag_index[$tag.'_'.$level] = 1;
				if($priority == 'tag' and $attributes_data) $current[$tag. '_attr'] = $attributes_data;

			}
			else { //If taken, put all things inside a list(array)
				if(isset($current[$tag][0]) and is_array($current[$tag])) {//If it is already an array...

					// ...push the new element into that array.
					$current[$tag][$repeated_tag_index[$tag.'_'.$level]] = $result;

					if($priority == 'tag' and $get_attributes and $attributes_data) {
						$current[$tag][$repeated_tag_index[$tag.'_'.$level] . '_attr'] = $attributes_data;
					}
					$repeated_tag_index[$tag.'_'.$level]++;

				}
				else { //If it is not an array...
					$current[$tag] = array($current[$tag],$result); //...Make it an array using using the existing value and the new value
					$repeated_tag_index[$tag.'_'.$level] = 1;
						
					if($priority == 'tag' and $get_attributes) {
						if(isset($current[$tag.'_attr'])) { //The attribute of the last(0th) tag must be moved as well

							try {
								$current[$tag]['0_attr'] = $current[$tag.'_attr'];
								unset($current[$tag.'_attr']);
							}
							catch (exception $e) {
								drupal_set_message(t('Error ') . $e->getMessage(), 'error');
							}
								
								
						}

						if($attributes_data) {
							$current[$tag][$repeated_tag_index[$tag.'_'.$level] . '_attr'] = $attributes_data;
						}
					}
					$repeated_tag_index[$tag.'_'.$level]++; //0 and 1 index is already taken
				}
			}

		}
		elseif($type == 'close') { //End of tag '</tag>'
			$current = &$parent[$level-1];
		}
	}

	return($xml_array);
}