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

	function showQt() {
		module_load_include('inc', 'fedora_repository', 'plugins/tagging_form');
		module_load_include('inc', 'fedora_repository', 'plugins/ShowStreamsInFieldSets');
		module_load_include('inc', 'fedora_repository', 'api/fedora_item');

		$info = $this->technicalMetadata(array('width' => 640, 'height' => 480));
		$width = $info['width'];
		$height = $info['height'];

		$pframe = $this->getPosterFrameDatastreamInfo();
		$media = $this->getMediaDatastreamInfo('PROXY', array('OBJ'));
		if($media === false ) {
			return '';
		}
		global $base_url;
		$path = drupal_get_path('module', 'Fedora_Repository');
		$fullPath=base_path().$path;
		$content= '';
		$pathTojs = drupal_get_path('module', 'Fedora_Repository').'/js/AC_Quicktime.js';
		drupal_add_js($pathTojs);

		$divid = 'player'.md5($this->pid).'MOV';
		$content .= '<div  class="player"  id="' . $divid .'">';
		if($pframe !== false) {
			$content .= '<div class="poster" style="cursor: pointer; position: relative; width: ' . $width .'px; min-height: ' . ($height) . 'px;">';
			$content .= '<img src="' . base_path().'fedora/repository/'.$this->pid.'/'. $pframe->ID . '/poster.jpg' . '" />';
			$content .= '<div class="play" style="font-size: 128px; color: white; position: absolute; top: 50%; left: 50%; margin-top: -0.085em; margin-left: -0.33em; opacity: 0.9; ">▶</div>';
			$content .= '</div>';
		}
		$content .= '</div>';
		if($this->enableDownload()) {
			$url = base_path().'fedora/repository/'.$this->pid.'/OBJ/MOV.mov';
			$content .= '<a class="download" href="' . $url . '">Download Media File</a>';
		}
		$src = base_path().'fedora/repository/'.$this->pid.'/' . $media->ID. '/MOV.mov';
		$qtparams = '';
		$qtparams .= "'autostart', '" . ($pframe !== false ? 'true' : 'false') . "', ";
		$init = <<<EOD
    $(function() { 
       src = "$src";
       if(src.substring(0,4) != 'http') {
       src = 'http://' + location.host + src;
       }
     	str = QT_GenerateOBJECTText_XHTML(src, "$width", ($height+15), '', 
     	$qtparams
       		'postdomevents', 'true',
      		'EnableJavaScript', 'true',
       		'bgcolor', 'black',
       		'controller', 'true',
       		'SCALE', 'aspect',
       		'LOOP', 'false'
       	);		
	if($('.poster', '#$divid').length ==0) {
      	$('#$divid').append(str);
        } else {
         $('#$divid .poster').one('click', function() { $(this).hide(); $('#$divid').append(str); });
        }
    });
EOD;

     	drupal_add_js($init, 'inline', 'footer');

     	$collection_fieldset = array(
       	 '#title' => t('Quicktime'),
       	 '#collapsible' => TRUE,
       	 '#collapsed' => FALSE,
       	 '#value' => $content);
     	return theme('fieldset',$collection_fieldset);
	}
}
