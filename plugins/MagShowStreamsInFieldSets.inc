<?php
// $Id$
module_load_include('inc', 'fedora_repository', 'plugins/ShowStreamsInFieldSets');

/*
 * Created on 17-Apr-08
 */
class MagShowStreamsInFieldSets {
  private $pid = NULL;
  function MagShowStreamsInFieldSets($pid) {
    $this->pid = $pid;
  }

  function showMag() {
    module_load_include('inc', 'islandora_mag', 'MagObjectHelper');
    $objectHelper = new MagObjectHelper();
    $content = $objectHelper->getMAG($this->pid);
    $collection_fieldset = array(
      '#title' => t('MAG DATA Description'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
      '#value' => $content,
    );
    return theme('fieldset', $collection_fieldset);
  }
  
  function showPDFPreview() {
    global $base_url;

    $tabset = array();

    $tabset['my_tabset'] = array(
      '#type' => 'tabset',
    );
    $tabset['my_tabset']['first_tab'] = array(
      '#type' => 'tabpage',
      '#title' => t('DATASTREAMS Description'),
      '#content' => "<div></div>",
    );

    $tabset['my_tabset']['second_tab'] = array(
      '#type' => 'tabpage',
      '#title' => t('Read Online'),
      '#content' => "<iframe src=\"http://docs.google.com/viewer?url=". $base_url . '/fedora/repository/'. 
        $this->pid . '/OBJ/preview.pdf'. "&embedded=TRUE\" style=\"width:600px; height:500px;\" frameborder=\"0\"></iframe>"
    );

    // Render the tabset.
    return tabs_render($tabset);

    return theme('fieldset', $collection_fieldset);
//      . (user_access('add fedora datastreams') ? drupal_get_form('fedora_ilives_image_tagging_form', $this->pid) : '');
  }
}
