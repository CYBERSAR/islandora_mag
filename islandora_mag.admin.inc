<?php
// $Id$

/**
 * Form builder to define some useful variables
 */
function islandora_mag_admin_globals() {
  $form['islandora_mag_admin_globals'] = array(
    '#type' => 'fieldset',
    '#title' => t('Global settings'),
  );
  

  $form['islandora_mag_admin_globals']['islandora_mag_image_label_cc_str'] = array(
    '#type' => 'textfield',
    '#title' => t('Creative Commons Label on Watermark'),
    '#default_value' => variable_get('islandora_mag_img_cc_str', 'You can remove this label without breaking the Creative Commons license.'),
    '#description' => t('Default string to be shown on the label attached to the image.')
  );
  
  $taxonomies = array("---");
  
  $tax_vocs = taxonomy_get_vocabularies();
  
  if (!empty($tax_vocs)) {
	  foreach ($tax_vocs as $tax_voc) {
	  	$taxonomies[$tax_voc->vid] = $tax_voc->name;
	  }
  }
  else {
  	drupal_set_message(t('You have to create at least one taxonomy!'), 'warning');
  }
  
  $form['islandora_mag_admin_globals']['islandora_mag_dcsubject_taxonomy'] = array(
    '#type' => 'select',
    '#title' => t('MAG dc:subject Taxonomy'),
    '#options' => $taxonomies,
    '#default_value' => variable_get('islandora_mag_dcsubject_taxonomy', 0),
    '#description' => t('Taxonomy used to fill the dc:subject MAG\'s field.')
  );
  
  /*
  $options = array('1' => t('Enabled'), '0' => t('Disabled'));
  $form['islandora_mag_admin_globals']['islandora_mag_delete_fo_on_del'] = array(
   '#type' => 'radios',
    '#title' => t('Cancellation of the Fedora object upon removal of the node'),
    '#default_value' => variable_get('islandora_mag_delete_fo_on_del', '0'),
  	'#options' => $options,
    '#description' => t('WARNING: Set this to "Enabled" will remove the Fedora Object related to the deleted node.')
  );
  */

  $form['islandora_mag_admin_globals']['#submit'][] = 'islandora_mag_admin_globals_submit';
  
  return system_settings_form($form); 
}