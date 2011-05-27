<?php

// $Id$

/*
 * Created on Feb 2011
 *
 * To change the template for this generated file go to
 * Window - Preferences - PHPeclipse - PHP - Code Templates
 */
module_load_include('inc', 'fedora_repository', 'ObjectHelper');

class MagObjectHelper extends ObjectHelper {

  function MagObjectHelper() {
    drupal_bootstrap(DRUPAL_BOOTSTRAP_FULL);
    module_load_include('inc', 'fedora_repository', 'ConnectionHelper');
    $connectionHelper = new ConnectionHelper();
  }

  /**
   * Queries fedora for what we call the MAG.
   * Transforms the returned xml to html
   * This is the another metadata view.
   * @param $pid String
   * @return String
   */
  function getMAG($pid) {
    global $base_url;
    $path = drupal_get_path('module', 'fedora_repository');
    $thispath = drupal_get_path('module', 'islandora_mag');
    
    module_load_include('inc', 'fedora_repository', 'ConnectionHelper');

    $soapHelper = new ConnectionHelper();
    $client = $soapHelper->getSoapClient(variable_get('fedora_soap_url', 'http://localhost:8080/fedora/services/access?wsdl'));

    $dsId = 'MAG';
    $params = array(
    	'pid' => "$pid",
      'dsID' => "$dsId",
      'asOfDateTime' => ""
    );
    try {
      $object = $client->__soapCAll('getDatastreamDissemination', array('parameters' => $params));
    }
    catch (Exception $e) {
    	drupal_set_message($e->getMessage(), 'error');
      return;
    }
    
    $xmlstr = $object->dissemination->stream;
    try {
      $proc = new XsltProcessor();
    }
    catch (Exception $e) {
      drupal_set_message($e->getMessage(), 'error');
      return;
    }

    /*
     * Nuovo codice
     */

    


    /*
     * Questo è il mio commento a questo blocco di codice.
     * Non devi cambiare nulla in questo commendo mi raccomando.
     */
    $proc->setParameter('', 'baseUrl', $base_url);
    $proc->setParameter('', 'path', $base_url . '/' . $path);
    $input = NULL;
    $xsl = new DomDocument();
    try {
      $xsl->load($thispath . '/xsl/convertMAG.xsl');
      $input = new DomDocument();
      $input->loadXML(trim($xmlstr));
    }
    catch (exception $e) {
      watchdog(t("Fedora_Repository"), t("Problem loading XSL file: !e", array('!e' => $e)), NULL, WATCHDOG_ERROR);
    }
    
    $xsl = $proc->importStylesheet($xsl);
    $newdom = $proc->transformToDoc($input);
    $output = $newdom->saveXML();
    $baseUrl = base_path();
    
    if (user_access(ObjectHelper :: $EDIT_FEDORA_METADATA)) {
      $allow=TRUE;
      if (module_exists('fedora_fesl')) { 
				$allow = fedora_fesl_check_roles($pid,'write');
      }
      if ($allow) {
	  		$output .= '<br /><a title = "' . t('Edit Meta Data') . '" href="' . $base_url . '/fedora/repository/editmetadata/' . $pid . '/' .
	    	$dsId . '"><img src="' . $base_url . '/' . $path . '/images/edit.gif" alt="' . t('Edit Meta Data') . '" /></a>';
      }
    }
    return $output;
  }
}
