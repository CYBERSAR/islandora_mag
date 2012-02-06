<?php

// $Id$

class MagBook {

	function __construct($pid = '') {
		module_load_include('inc', 'fedora_repository', 'api/fedora_item');
		if (!empty($pid)) {
			$this->pid = $pid;
			$this->item = new Fedora_Item($pid);
		}
	}

	public function showFieldSets() {
		module_load_include('inc', 'fedora_repository', 'plugins/tagging_form');
		module_load_include('inc', 'fedora_repository', 'plugins/DarwinCore');
		module_load_include('module', 'fedora_ilives');
		global $base_url;

		$tabset = array();

		$tabset['my_tabset'] = array(
      '#type' => 'tabset',
		);

		global $user;
		$qs = '';
		if ($user->uid != 0) {
			$qs = '?uid='. base64_encode($user->name . ':'. $user->pass);
		}

		$viewer_url = variable_get('fedora_base_url', '') . '/get/'. $this->pid . '/ilives:viewerSdef/getViewer'. $qs;
		$html = '<iframe src="'. $viewer_url . '" scrolling="0" frameborder="0" style="width: 100%; height: 800px;">Errors: unable to load viewer</iframe>';
		$tabset['my_tabset']['second_tab'] = array(
      '#type' => 'tabpage',
      '#title' => t('Full-size'),
      '#content' => $html
		);
		$tabset['my_tabset']['first_tab'] = array(
		// #type and #title are the minimum requirements.
      '#type' => 'tabpage',
      '#title' => t('Description'),
		// This will be the content of the tab.
      '#content' => fedora_ilives_create_book_view($this->pid),
		);

		// Hide "Add pages" tab if user don't have the right permissions
		if (user_access('purge objects and datastreams')) {		
			$tabset['my_tabset']['add_pages'] = array(
	      '#type' => 'tabpage',
	      '#title' => t('Add pages'),
	      '#content' => drupal_get_form('MagBook_add_pages_form', $this->pid),
			);
		}
		
		return tabs_render($tabset);
	}
}


function MagBook_add_pages_form(&$form_state, $pid) {
	$form['pid'] = array(
    '#type' => 'hidden',
    '#value' => $pid,
	);

	$form['tiff_dir'] = array(
    '#type' => 'textfield',
    '#title' => t('TIFF folder'),
    '#required' => TRUE,
    '#description' => t('Path to a web-accessible folder contining the book\'s page images in TIFF format on this server.'),
	);

	$form['submit'] = array(
    '#type' => 'submit',
    '#weight' => 10,
    '#value' => 'Aggiungi Pagine'
    );
    return $form;
}

function MagBook_add_pages_form_validate($form, &$form_state) {
	if ($form_state['values']['tiff_dir'] == '') {
		form_set_error('', t('You must select a name for this group of settings.'));
	}
}

function MagBook_add_pages_form_submit($form, &$form_state) {
	$pid = $form_state['values']['pid'];
	if (!empty($form_state['values']['tiff_dir'])) {
		$file_list = array();
		if ($tiff_dir = opendir($form_state['values']['tiff_dir'])) {
			while (FALSE !== ($file_name = readdir($tiff_dir))) {
				$ext = strrchr($file_name, '.');
				if ($ext == '.tif' || $ext == '.tiff') {
					array_push($file_list, $form_state['values']['tiff_dir'] .'/'. $file_name);
				}
			}
			closedir($tiff_dir);
			sort($file_list);
			MagBook_create_page_objects($pid, $file_list);
		}
		else {
			drupal_set_message(t('Invalid Directory! Remember to use an FTP directory!'), 'error');
			watchdog(
    		'islandora_mag',
    		"Invalid directory: '%dir'.",
			array('%dir' => $form_state['values']['tiff_dir'])
			);
		}
	}
}

/**
 * Create a page object for each element in $image_list as a child object of $pid
 */
function MagBook_create_page_objects($pid, $image_list = array()) {
	module_load_include('inc', 'fedora_repository', 'api/fedora_item');
	$book = new Fedora_Item($pid);
	if (!$book->exists()) {
		return FALSE;
	}
	$page_pids = array();
	$batch = array(
    'title' => 'Creating page objects',
    'operations' => array(),
    'file' => drupal_get_path('module', 'islandora_mag') .'/plugins/MagBook.inc',
	);

	//how many pages was ingested?
	$old_pages = $book->find_objects_by_pattern($pid . "-*");
	$page_num = count($old_pages) + 1;

	/*
	 * Controlla la presenza del datastream MAG nell'oggetto BOOK
	 * se è presente l'elemento <img_group> prendi l'ID da cui NEW_ID=ID+1 e crea un nuovo <img_group>
	 * con attributo "ID"=NEW_ID. Passa questo valore anche alla funzione _add_single_page_object
	 * che si occuperà di creare il MAG per la pagina, con la sezione <img> avente l'attributo "imggroupID"=NEW_ID
	 * Nella pagina, il <sequence_number> sarà $page_num; nel libro bisogna modificare, se presente, l'elemento <stop>
	 * della sezione <stru> che sarà: stop_new_getXML = stop_old_getXML + $old_pages.
	 */

	$mag_ds = $book->get_datastream_dissemination("MAG");
	$mag_ds = trim($mag_ds);

	$dom = new DOMDocument();
	if (!$dom->loadXML($mag_ds)) {
		echo "error loading xml";
	}
	else {
		$metadigit = $dom->getElementsByTagName('metadigit')->item(0);
		$metadigit->setAttribute("xmlns:niso","http://www.niso.org/pdfs/DataDict.pdf");
		$gen = $dom->getElementsByTagName('gen')->item(0);
		 
		//search for a previous "img_group" and set a new image group ID
		$img_group = $metadigit->getElementsByTagName('img_group');
		$img_group_id = 0;
		$img_group_length = $img_group->length;
		if ($img_group_length > 0) {
			$img_group_id = $img_group->item($img_group_length - 1)->getAttribute("ID"); //old id
			$img_group_id = $img_group_id + 1; //new id
		}
		 
		$new_img_group = $dom->createElement("img_group");
		$new_img_group->setAttribute("ID", $img_group_id);
		$gen->appendChild($new_img_group);

		//creates image-group properties: image_metrics
		$ingested_file = $image_list[0]; //use first image as template for this group
		$ingested_file_escaped = str_replace(" ", "\ ", $ingested_file);
		$exiftool_opt_printOnlyValue = "-s -s -s";
		 
		$image_metrics = $dom->createElement("image_metrics");
		$new_img_group->appendChild($image_metrics);

		$samplingfrequencyunit = "2"; //can be: 1) not specified, 2) inch, 3) centimeters
		$xsamplingfrequency = exec('exiftool -XResolution ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
		$ysamplingfrequency = exec('exiftool -YResolution ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
		$bitsPerSample = exec('exiftool -BitsPerSample ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
		$photometricInterpretation = exec('exiftool -PhotometricInterpretation ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);

		$imggroup_samplingfrequencyunit = $dom->createElement("niso:samplingfrequencyunit", $samplingfrequencyunit);
		$imggroup_xsamplingfrequency = $dom->createElement("niso:xsamplingfrequency", $xsamplingfrequency);
		$imggroup_ysamplingfrequency = $dom->createElement("niso:ysamplingfrequency", $ysamplingfrequency);
		$imggroup_bitpersample = $dom->createElement("niso:bitpersample", $bitsPerSample);
		$imggroup_photometricInterpretation = $dom->createElement("niso:photometricinterpretation", $photometricInterpretation);

		$image_metrics->appendChild($imggroup_samplingfrequencyunit);
		$image_metrics->appendChild($imggroup_xsamplingfrequency);
		$image_metrics->appendChild($imggroup_ysamplingfrequency);
		$image_metrics->appendChild($imggroup_bitpersample);
		$image_metrics->appendChild($imggroup_photometricInterpretation);


		//creates image-group properties: format
		$format = $dom->createElement("format");
		$new_img_group->appendChild($format);
		 
		$name = exec('exiftool -FileType ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
		$mime = exec('exiftool -MIMEType ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
		$compression = exec('exiftool -Compression ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
		 
		$imggroup_name = $dom->createElement("niso:name", $name);
		$imggroup_mime = $dom->createElement("niso:mime", $mime);
		$imggroup_compression = $dom->createElement("niso:compression", $compression);
		 
		$format->appendChild($imggroup_name);
		$format->appendChild($imggroup_mime);
		$format->appendChild($imggroup_compression);

		 
		if ($book->modify_datastream_by_value($dom->saveXML(), 'MAG', "MAG Record", 'text/xml') !== NULL) {
			drupal_set_message(t('Il datastream MAG per l\'oggetto "%pid" è stato modificato con successo.', array('%pid'=>$pid)));
		}
		else {
			drupal_set_message(t('Errore nella modifica del datastream MAG: !e', array('!e' => $e->getMessage())), 'error');
		}
	}

	$book_title = $book->objectProfile->objLabel;
	foreach ($image_list as $image_path) {
		// TODO: oppure in base al filename
		//$page_name = basename($image_path);
		//+ espressioni regolari su $page_name per estrarre il numero...
		 
		$batch['operations'][] = array('MagBook_add_single_page_object', array($pid, $image_path, $page_num, "$book_title - Page ". sprintf("%03d", $page_num), $img_group_id));
		$page_num++;
	}
	batch_set($batch);
}

/**
 * Creates each single page and adds the MAG's img section with required elements
 *
 * @param string $book_pid
 * @param string $page_file (url)
 * @param int		 $page_num
 * @param string $page_title
 * @param int		 $img_group_id
 */
function MagBook_add_single_page_object($book_pid, $page_file, $page_num = 1, $page_title = NULL, $img_group_id = 1) {
	global $base_url;

	module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
	module_load_include('inc', 'fedora_repository', 'api/fedora_item');

	$page_pid = "$book_pid-". sprintf("%03d", $page_num);

	$page_item = Fedora_Item::ingest_new_item($page_pid, 'A', $page_title);
	$page_item->add_relationship('hasModel', 'epistemetec:mag_page', FEDORA_MODEL_URI);
	$page_item->add_relationship('isMemberOf', $book_pid);
	$page_item->add_datastream_from_file($page_file, 'TIFF', 'Archival TIFF', 'image/tiff', 'M');

	//create MAG: we need only a subset of elements, usefull to be merged with the book's MAG
	$dom = new DomDocument("1.0", "UTF-8");
	$dom->formatOutput = TRUE;

	$mag = $dom->createElement("metadigit");
	$mag->setAttribute("xmlns:dc","http://purl.org/dc/elements/1.1/");
	$mag->setAttribute("xmlns:niso","http://www.niso.org/pdfs/DataDict.pdf");

	$img = $dom->createElement("img");
	$img->setAttribute("imggroudID", $img_group_id);

	$mag->appendChild($img);

	//required image elements
	$img_elements['sequence_number'] = $dom->createElement("sequence_number", $page_num);
	$img_elements['nomenclature'] = $dom->createElement("nomenclature", $page_pid);
	$img_elements['file'] = $dom->createElement("file", $base_url . '/fedora/repository/' . $page_pid);
	$img_elements['md5'] = $dom->createElement("md5", md5_file($page_file));
	$img_elements['image_dimensions'] = $dom->createElement("image_dimensions", "");

	//"image dimensions" has subelements
	list($width, $length, $type, $attr) = getimagesize($page_file);
	$imagelength = $dom->createElement("niso:imagelength", $length);
	$imagewidth = $dom->createElement("niso:imagewidth", $width);

	$img_elements['image_dimensions']->appendChild($imagelength);
	$img_elements['image_dimensions']->appendChild($imagewidth);

	//add each required element to MAG
	foreach ($img_elements as $img_element) {
		$img->appendChild($img_element);
	}

	if ($page_item->add_datastream_from_string($dom->saveXML($mag), 'MAG') !== NULL) {
		drupal_set_message(t('Il datastream MAG per l\'oggetto "%pid" è stato modificato con successo.', array('%pid'=>$page_pid)));
	}
}
