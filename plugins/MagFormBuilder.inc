<?php
// $Id$

/*
 * Created on 01-Feb-11
 *
 *
 * implements methods from content model ingest form xml
 * builds MAG metadata form
 */


$image_audio_path = drupal_get_path('module', 'islandora_mag') . "/images/cover-audio.png";
variable_set('default_image_audio', $image_audio_path);

$image_book_path = drupal_get_path('module', 'islandora_mag') . "/images/cover-book.png";
variable_set('default_image_book', $image_book_path);

$collection_policy_path = drupal_get_path('module', 'islandora_mag') . "/foxml/COLLECTION_POLICY.foxml";
variable_set('default_collection_policy', $collection_policy_path);

module_load_include('inc', 'fedora_repository', 'plugins/FormBuilder');
class MagFormBuilder extends FormBuilder {
	protected $cm;
	protected $item;
	protected $pid;
	var $mag_fields = array("gen", "bib", "img", "doc", "audio", "video", "bigimg");

	function __construct($pid=null) {
		parent::__construct();
		if (isset($pid)) {
			module_load_include('inc', 'fedora_repository', 'ContentModel');
			module_load_include('inc', 'fedora_repository', 'api/fedora_item');

			$this->pid = $pid;
			$this->cm = ContentModel::loadFromObject($pid);
			$this->item = new fedora_item($pid);
		}
	}

	/**
	 * Returns the content model type of the actual object in both cases of insert and delete.
	 *
	 * @param string $action
	 * @return string img|doc|audio|video
	 */
	function getContentModelType($action) {
		/*
		 * Pick a name template to be used for the various Content Models and then do (if any) the needed string transformations.
		 * Was chosen the following form:
		 *
		 * epistemetec:mag_$cmtype
		 *
		 * Where $cmtype can take the values: doc, img, audio, video, etc. depending on the name of MAG section that must be implemented.
		 * The dash-low is used to separate this part.
		 *
		 * When igesting, the information is contained in $_POST['models'] followed by "/ISLANDORACM" or similar: we must remove this part.
		 * When editing, this information is contained in $this->cm->pid without the end "/ISLANDORACM".
		 */

		if ((isset($_POST['models']) and !empty($_POST['models'])) or !empty($this->cm->pid)) {
			if ($action == "insert") {
				$content_model = explode("/", $_POST['models']);
				$content_model = $content_model[0];
			}
			elseif ($action == "edit" || $action == "clone") {
				$content_model = $this->cm->pid;
			}
			else {
				drupal_set_message(t('Error: no action specified!'), 'error');
				return;
			}

			$content_model = explode("_", $content_model);
			if ($content_model[sizeof($content_model)-2] == "big") {
				$content_model = "bigimg";
			}
			else {
				$content_model = $content_model[sizeof($content_model)-1]; // img|audio|video|doc|book|collection
			}

			return $content_model;
		}
		else {
			drupal_set_message(t('Error retrieving the Content Model in !err stage', array('!err' => $action)), 'error');
			return;
		}
	}

	/**
	 * Creates the MAG's metadata stream
	 *
	 * @param array $form_values
	 * @param dom $dom
	 * @param dom $rootElement
	 */
	function createMAGStream(&$form_values, &$dom, &$rootElement) {
		$datastream = $dom->createElement("foxml:datastream");
		$datastream->setAttribute("ID", "MAG");
		$datastream->setAttribute("STATE", "A");
		$datastream->setAttribute("CONTROL_GROUP", "X");

		$version = $dom->createElement("foxml:datastreamVersion");
		$version->setAttribute("ID", "MAG.0");
		$version->setAttribute("MIMETYPE", "text/xml");
		$version->setAttribute("LABEL", "Mag Record");
		$datastream->appendChild($version);

		$content = $dom->createElement("foxml:xmlContent");
		$version->appendChild($content);

		$mag = $this->createMetadigitStream($form_values, $dom);
		$content->appendChild($mag);

		$rootElement->appendChild($datastream);
	}


	/**
	 * Creates the MAG's Metadigit section
	 *
	 * @param array $form_values
	 * @param dom $dom
	 * @param string $action
	 */
	function createMetadigitStream(&$form_values, &$dom, $action="insert") {
		global $base_url;
		global $language;
			
		$mag = $dom->createElement("metadigit");
		$mag->setAttribute("xmlns:dc","http://purl.org/dc/elements/1.1/");
		$mag->setAttribute('xml:lang', $language->language);

		/*
		 * TODO
		 *
		 * http://www.iccu.sbn.it/upload/documenti/manuale.html#magDoc
		 * -----------------------------------------------------------
		 *
		 * L'elemento root dello schema MAG è <metadigit> (o <mag:metadigit> a seconda che si decida o meno di usare elementi qualificati).
		 * Nello start tag debbono essere richiamati tutti gli attributi necessari ad abilitare i numerosi namespace usati dallo schema MAG,
		 * come nell'esempio:

		 <mag:metadigit xmlns:dc="http://purl.org/dc/elements/1.1/"
		 xmlns:niso="http://www.niso.org/pdfs/DataDict.pdf"
		 xmlns:xlink="http://www.w3.org/TR/xlink"
		 xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
		 xmlns:mag="http://www.iccu.sbn.it/metaAG1.pdf"
		 xsi:schemaLocation="http://www.iccu.sbn.it/metaAG1.pdf metadigit.xsd"
		 version="2.0.1">

		 <!-- qui il documento -->

		 </mag:metadigit>

		 Per l'elemento è definito un unico attributo opzionale:

		 * version : indica la versione dello schema MAG. Il valore di default è 2.0.1

		 Si noti che l'esempio presenta un documento MAG qualificato, che normalmente si usa solo per la distribuzione del documento all'esterno dell'istiuzione; in ogni caso lo schema distribuito prevede che i documenti presenti elementi qualififcati e attributi non qualificati (file metadigit.xsd):

		 <xsd:schema xmlns="http://www.iccu.sbn.it/metaAG1.pdf"
		 xmlns:dc="http://purl.org/dc/elements/1.1/"
		 xmlns:xsd="http://www.w3.org/2001/XMLSchema"
		 targetNamespace="http://www.iccu.sbn.it/metaAG1.pdf"
		 elementFormDefault="qualified" attributeFormDefault="unqualified">

		 <!-- qui lo schema -->

		 </xsd:schema>


		 */

		$previousElement = NULL;
		foreach ($form_values as $key => $value) {
			$index = strrpos($key, '-');
			if ($index > 01) {
				$key = substr($key, 0, $index);
			}
			$test = substr($key, 0, 2);

			if ($test == 'ma' || $test == 'dc') {
				$tags = explode(":", $key);
				$sw_value = $test == 'dc' ? $tags[0] : $tags[1];

				switch ($sw_value) {
					case "gen":
						if (!isset($gen)) {
							$gen = $dom->createElement("gen");
							$mag->appendChild($gen);
						}
						$previousElement = $dom->createElement($tags[2], $value);
						$gen->appendChild($previousElement);
						break;
					case "dc":
						if (!isset($bib)) {
							$bib = $dom->createElement("bib");
							$mag->appendChild($bib);
						}
						$ele = $tags[0] . ":" . $tags[1];
						$previousElement = $dom->createElement($ele, $value);
						$bib->appendChild($previousElement);

						if ($ele == "dc:subject") {
							$term = taxonomy_get_term_by_name($value);
							$tid = $term[0]->tid;
							$parents = taxonomy_get_parents($tid);
							foreach ($parents as $parent) {
								$previousElement = $dom->createElement($ele, $parent->name);
								$bib->appendChild($previousElement);
							}
						}

						break;
					case "bib":
						if (!isset($bib)) {
							$bib = $dom->createElement("bib");
							$mag->appendChild($bib);
						}
						if (isset($tags[3])) {
							if (isset($holdings))	{
								$previousElement = $dom->createElement($tags[3], $value);
								$holdings->appendChild($previousElement);
							}
						}
						else {
							$holdings = $dom->createElement($tags[2]);
							$bib->appendChild($holdings);
						}
						break;
				}
			}
		}

		//Gets CM type and creates the right MAG section
		$mag_section_name = $this->getContentModelType($action);
		if (isset($mag_section_name) && in_array($mag_section_name, $this->mag_fields)) {
			if ($mag_section_name == "bigimg") {
				$mag_section_name = "img";
			}
			if ($mag_section_name == "img") {
				//mag's image content model uses the "niso" namespace for some fields, so add it
				$mag->setAttribute("xmlns:niso","http://www.niso.org/pdfs/DataDict.pdf");
			}
			$stuff = $this->createMetadigitSection($form_values, $dom, $action, $mag_section_name);

			if (isset($stuff)) {
				$mag->appendChild($stuff);
			}
		}

		return $mag;
	}


	/**
	 * Creates the section in the Metadigit datastream based on her name.
	 * TODO: Create a class that allows to abstract the bash.
	 *
	 * @param array $form_values
	 * @param dom object $dom
	 * @param string $action
	 * @param string $section_name
	 */
	function createMetadigitSection(&$form_values, &$dom, $action, $section_name) {
		$section = $dom->createElement($section_name);
			
		$tags3_sublevels = array(
	  		"audio_dimentions",
	  		"audio_metrics",
	  		"video_dimentions",
	  		"video_metrics",
	  		"digitisation",
	  		"format",
	  		"transcription"
	  		);
	  		$tags2_sublevels = array(
	  		"image-dimentions",
	  		"image-metrics",
	  		"format",
	  		"proxies"
	  		);

	  		$depth = 0; //used to specify section levels

	  		$previousElements[$depth] = $section;
	  		foreach ($form_values as $key => $value) {
	  			$test = substr($key, 0, 2);

	  			if ($test == 'ma') {
	  				$tags = explode(":", $key);
	  				switch ($tags[1]) {
	  					case $section_name:

	  						if (isset($tags[4])) {
	  							$issub = false;
	  							if (strcmp($tags[3], "niso") == 0) {
	  								$new_key = $tags[3] . ":" . $tags[4];
	  								$depth = 1;
	  							}
	  							else {
	  								$new_key = $tags[4];
	  								$depth = 2;
	  							}
	  						}
	  						elseif (isset($tags[3])) {
	  							if (in_array($tags[3], $tags3_sublevels)) {
	  								$issub = true;
	  							}
	  							else {
	  								$issub = false;
	  							}
	  							$new_key = $tags[3];
	  							$depth = 1;
	  						}
	  						elseif (isset($tags[2])) {
	  							if (in_array($tags[2], $tags2_sublevels)) {
	  								$issub = true;
	  							}
	  							else {
	  								$issub = false;
	  							}
	  							$new_key = $tags[2];
	  							$depth = 0;
	  						}

	  						if ($action == "insert") {
	  							$ingested_file = $form_values["ingest-file-location"];
	  							$ingested_file_escaped = str_replace(" ", "\ ", $ingested_file);

	  							$exiftool_opt_printOnlyValue = "-s -s -s";
	  							$exiftool_filetype = exec('exiftool -FileType ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);

	  							switch ($new_key) {
	  								// Shared by all sections
	  								case "sequence_number":
	  									//this field is required: for convenience default value will be 1
	  									if (empty($value)) {
	  										$value = 1;
	  									}
	  									break;
	  								case "file":
	  									$value = $base_url . '/fedora/repository/' . $form_values['pid'];
	  									break;
	  								case "md5":
	  									$value = md5_file($ingested_file);
	  									break;
	  								case "filesize":
	  									$value = filesize($ingested_file);
	  									break;
	  								case "datetimecreated":
	  									//ingection date
	  									$value = date("Y-m-d H:i:s", filemtime($ingested_file));
	  									break;
	  								case "name":
	  									//takes file extension
	  									$value = strtolower(array_pop(explode('.', $ingested_file)));
	  									break;
	  								case "mime":
	  									module_load_include('inc', 'Fedora_Repository', 'MimeClass');
	  									$mime = new MimeClass();
	  									$value = $mime->get_mimetype($ingested_file);
	  									break;
	  								case "compression":
	  									if ($exiftool_filetype == "WAV") {
	  										$value = exec('exiftool -Encoding ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									}
	  									elseif($exiftool_filetype == "MP3") {
	  										$value = exec('exiftool -AudioLayer ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  										if (!empty($value)) {
	  											$value = "MPEG-1 layer " . $value;
	  										}
	  									}
	  									else {
	  										$value = "Uncompressed";
	  									}

	  									if (empty($value)) {
	  										$value = "non definito";
	  									}

	  									break;

	  									// Specific for the img section
	  								case "niso:imagelength":
	  									$value = exec('exiftool -ImageHeight ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									break;
	  								case "niso:imagewidth":
	  									$value = exec('exiftool -ImageWidth ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									break;
	  								case "ppi":
	  								case "dpi":
	  									if (exec('exiftool -XResolution ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped) == exec('exiftool -YResolution ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped)) {
	  										$value = exec('exiftool -XResolution ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									}
	  									else {
	  										$value = "custom";
	  									}
	  								case "niso:xsamplingfrequency":
	  									$value = exec('exiftool -XResolution ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									break;
	  								case "niso:ysamplingfrequency":
	  									$value = exec('exiftool -YResolution ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									break;

	  									// Specific for the video section
	  								case "duration":
	  									$value = exec('exiftool -Duration ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									break;
	  								case "videosize":
	  									$value = exec('exiftool -ImageSize ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									break;
	  								case "aspectratio":
	  									$video_w = exec('exiftool -SourceImageWidth ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									$video_h = exec('exiftool -SourceImageHeight ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);

	  									if ($video_h != 0) {
	  										$ratio = $video_w/$video_h;

	  										if($ratio == 1) {
	  											$value = "1:1";
	  										}
	  										elseif ($ratio == 4/3) {
	  											$value = "4:3";
	  										}
	  										elseif($ratio == 16/9) {
	  											$value = "16:9";
	  										}
	  										elseif($ratio == (2.11/1)) {
	  											$value = "2.11:1";
	  										}
	  										else {
	  											$value = "proporzioni di: " . $ratio . " non riconosciute dallo standard MAG.";
	  										}
	  									}
	  									else {
	  										$value = "proporzioni sconosciute.";
	  									}

	  									break;
	  								case "framerate":
	  									$value = exec('exiftool -VideoFrameRate ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									break;
	  								case "codec":
	  									$value = exec('exiftool -CompressorID ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									break;
	  									// Specific for the audio section
	  								case "samplingfrequency":
	  									$value = exec('exiftool -SampleRate ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									if ($value) {
	  										$value = $value / 1000;
	  									}
	  									break;
	  								case "bitrate":
	  									if ($exiftool_filetype == "WAV") {
	  										$value = exec('exiftool -AvgBytesPerSec ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									}
	  									elseif($exiftool_filetype == "MP3") {
	  										$value = exec('exiftool -AudioBitrate ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									}

	  									$value = $value ? intval($value / 1000) : "non definito";

	  									break;
	  								case "channel_configuration":
	  									if ($exiftool_filetype == "WAV") {
	  										$value = exec('exiftool -NumChannels ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									}
	  									elseif($exiftool_filetype == "MP3") {
	  										$value = exec('exiftool -ChannelMode ' . $exiftool_opt_printOnlyValue . " " . $ingested_file_escaped);
	  									}

	  									if (empty($value)) {
	  										$value = "non definito";
	  									}

	  									break;
	  							}
	  						}

	  						//creates subsections to put the following elements or put the element in the correct section
	  						if ($issub) {
	  							$sub_section = $dom->createElement($new_key, $value);
	  							$previousElements[$depth+1] = $previousElements[$depth]->appendChild($sub_section);
	  						}
	  						else {
	  							$element = $dom->createElement($new_key, $value);
	  							$previousElements[$depth]->appendChild($element);
	  						}

	  						//this is necessary to update $form_values because it will be used outside this module
	  						$form_values[$key] = $value;

	  						break;
	  				}
	  			}
	  		}
	  		return $section;
	}

	/**
	 * Creates the Language datastream
	 *
	 * @param array $form_values
	 * @param dom $dom
	 * @param dom $rootElement
	 *
	 function createLangStream(&$form_values, &$dom, &$rootElement) {
	 global $language;
	  
	 $datastream = $dom->createElement("foxml:datastream");
	 $datastream->setAttribute("ID", "LANG");
	 $datastream->setAttribute("STATE", "A");
	 $datastream->setAttribute("CONTROL_GROUP", "X");

	 $version = $dom->createElement("foxml:datastreamVersion");
	 $version->setAttribute("ID", "LANG.0");
	 $version->setAttribute("MIMETYPE", "text/xml");
	 $version->setAttribute("LABEL", "Language Record");
	 $datastream->appendChild($version);

	 $content = $dom->createElement("foxml:xmlContent");
	 $version->appendChild($content);

	 $lang = $dom->createElement("languages");
	 $content->appendChild($lang);

	 $lang_def = $dom->createElement("language", $language->language);
	 $lang_def->setAttribute("MAG_DS_ID", "MAG");
	 $lang_def->setAttribute("DEFAULT", "true");
	 $lang->appendChild($lang_def);

	 $rootElement->appendChild($datastream);
	 }
	 */



	function handleXMLForm($form_values) {
		module_load_include('inc', 'fedora_repository', 'api/fedora_item');
		module_load_include('inc', 'fedora_repository', 'CollectionPolicy');

		$dom = new DomDocument("1.0", "UTF-8");
		$dom->formatOutput = TRUE;
		$pid = $form_values['pid'];
		$rootElement = $dom->createElement("foxml:digitalObject");
		$rootElement->setAttribute('VERSION', '1.1');
		$rootElement->setAttribute('PID', "$pid");
		$rootElement->setAttribute('xmlns:foxml', "info:fedora/fedora-system:def/foxml#");
		$rootElement->setAttribute('xmlns:xsi', "http://www.w3.org/2001/XMLSchema-instance");
		$rootElement->setAttribute('xsi:schemaLocation', "info:fedora/fedora-system:def/foxml# http://www.fedora.info/definitions/1/0/foxml1-1.xsd");

		$dom->appendChild($rootElement);


		$this->magWatermark($form_values);

		// Create standard fedora stuff
		$this->createStandardFedoraStuff($form_values, $dom, $rootElement);

		// Create relationships
		$this->createRelationShips($form_values, $dom, $rootElement);
		$collectionPid = $form_values['collection_pid'];

		if (($cp = CollectionPolicy::LoadFromCollection($collectionPid)) !== FALSE) {
			$collectionName = trim($cp->getName());
			if (trim($collectionName) != '') {
				$form_values['dc:relation'] = $collectionName;
			}
		}
		// Create dublin core
		//$this->createQDCStream($form_values, $dom, $rootElement);

		// Create MAG datastream
		$form_values['dc:identifier'] = $pid;
		$this->createMAGStream($form_values, $dom, $rootElement);

		global $language;
		$form_values['metadigit_lang'] = $language->language;

		//$this->createLangStream($form_values, $dom, $rootElement);

		if (!empty($form_values['ingest-file-location'])) {
			$this->createFedoraDataStreams($form_values, $dom, $rootElement);
		}

		$this->createPolicy($collectionPid, &$dom, &$rootElement);

		// definition of fedora_alter_ingest hooks
		$datastreams = Array();
		$hook = 'fedora_alter_ingest';
		foreach (module_implements($hook) as $module) {
			$function = $module . '_' . $hook;
			/*
			$datastream = $function($form_values, $dom, $rootElement);
			if (isset($datastream) && is_array($datastream)) {
				$datastreams = array_merge($datastreams, $datastream);
			}
			elseif (isset($datastream)) {
				$datastreams[] = $datastream;
			}
			*/
			$function($form_values, $dom, $rootElement);
		}

		// we can see the full xml only saving it. Eclipse debugger crop it at 1024 char
		// file_save_data($dom->saveXML(), 'full_xml_debug.txt', FILE_EXISTS_REPLACE);

		try {
			$object = Fedora_Item::ingest_from_FOXML($dom);
			$datastreams = $object->get_datastreams_list_as_array();

			//we need TN: use a default one if not present
			if(!isset($datastreams["TN"])) {
				switch ($this->getContentModelType("insert")) {
					case "audio":
						$image_audio_path = drupal_get_path('module', 'islandora_mag') . "/images/cover-audio.png";
						$file_tn = variable_get('default_image_audio', $image_audio_path);
						break;
					case "book":
						$image_book_path = drupal_get_path('module', 'islandora_mag') . "/images/cover-book.png";
						$file_tn = variable_get('default_image_book', $image_book_path);
						break;
					case "collection":
						$image_coll_path = drupal_get_path('module', 'islandora_mag') . "/images/cover-collection.jpg";
						$file_tn = variable_get('default_image_collection', $image_coll_path);
						$object->add_datastream_from_file($file_tn, "MEDIUM_SIZE", "Default Collection");
						break;
				}
				$object->add_datastream_from_file($file_tn, "TN", "Default Thumbnail");
			}
			
			/*
			 * When managing with user defined sub-collection, e.g. epistemetec:mag_collection,
			 * we need to add collection policies to show the ingestion button and thereby allow
			 * the selection of the content models specified in "COLLECTION_POLICY.foxml"
			 */
			if ($this->getContentModelType("insert") == "collection" && !isset($datastreams["COLLECTION_POLICY"])) {
				$collection_policy_path = drupal_get_path('module', 'islandora_mag') . "/foxml/COLLECTION_POLICY.foxml";
				$file_cp = variable_get('default_collection_policy', $collection_policy_path);
				$object->add_datastream_from_file($file_cp, "COLLECTION_POLICY", "Collection Policy");
				
				
				$collection_father = new fedora_item($collectionPid);
				$father_child_security = $collection_father->get_datastream_dissemination('CHILD_SECURITY');

				if ( !empty($father_child_security) ) {
					$object->add_datastream_from_string($father_child_security, "POLICY");
					$object->add_datastream_from_string($father_child_security, "CHILD_SECURITY");
				 	drupal_set_message(t("Successfully added the POLICY datastream to this collection"));
				}
				else {
					drupal_set_message(t("Errors adding the POLICY datastream to this collection"), 'warning');
				}
				
				
			}

			if (!empty($object->pid)) {
				drupal_set_message(t("Item !pid created successfully.", array('!pid' => l($object->pid, 'fedora/repository/'. $object->pid))), "status");

				//invoke all fedora_after_ingest hooks
				$hook = 'fedora_after_ingest';
				foreach (module_implements($hook) as $module) {
					$function = $module . '_' . $hook;
					$status = $function($form_values); //TRUE on success, FALSE otherwise
				}

			}

			if (!empty($_SESSION['fedora_ingest_files'])) {
				foreach ($_SESSION['fedora_ingest_files'] as $dsid => $createdFile) {
					file_delete($createdFile);
				}
			}
			file_delete($form_values['ingest-file-location']);
		}
		catch (exception $e) {
			drupal_set_message(t('Error ingesting object from FOXML: !e', array('!e' => $e->getMessage())), 'error');
			watchdog(t("Fedora_Repository"), t("Error ingesting object from FOXML: !e", array('!e' => $e->getMessage())), NULL, WATCHDOG_ERROR);
			return;
		}
	}


	function createBaseForm(&$form, $action) {
		$form['indicator2']['metadigit'] = array(
	      '#type' => 'fieldset',
	      '#title' => t('Datastream MAG'),
		);

		$form['indicator2']['metadigit']['bib'] = array(
	      '#type' => 'fieldset',
	      '#title' => t('Sezione BIB'),
		);

		$mag_section_name = $this->getContentModelType($action);

		if ($mag_section_name != "collection") {
			$form['indicator2']['metadigit']['gen'] = array(
		      '#type' => 'fieldset',
		      '#title' => t('Sezione GEN'),
			);
		}

		if (isset($mag_section_name) && in_array($mag_section_name, $this->mag_fields)) {
			//use the img section also for high-resolution images
			$section = $mag_section_name == "bigimg" ? "img" : $mag_section_name;
			$form['indicator2']['metadigit'][$section] = array(
				'#type' => 'fieldset',
			  	'#title' => t('Sezione ' . strtoupper($section) ),
			);
		}

		if ($mag_section_name == "img" && $action != "edit") {
			//insert the watermark field only for simple images (jpg/png)
			$form['indicator2']['metadigit'][$mag_section_name]['watermarks'] = array(
			      '#type' => 'fieldset',
			      '#title' => t('Watermark'),
		     		'#description' => t('Inserisci il testo che comparirà nell\'immagine come Watermark.
		     				Il campo è opzionale; se lasciato vuoto non verrà aggiunto alcun Watermark.
		     		')
			);
			$form['indicator2']['metadigit'][$mag_section_name]['watermarks']['watermark'] = array(
			      '#type' => 'textfield',
						'#description' => t('NB: Questo campo non fa parte dello standard MAG e non farà parte dell\'XML; è posizionato qui per praticità.'),
			);
		}
	}


	function magBuildForm(&$form, $elements, &$form_values) {
		global $language;
			
		$action = "insert";
		
		if ( isset($form_values['storage']['source_pid'] )) {

			$action = "clone";

			$form['step']['#value'] = 2;

			module_load_include('inc', 'fedora_repository', 'ObjectHelper');
			$objectHelper = new ObjectHelper();

			$ds_info = $objectHelper->getStream($form_values['storage']['source_pid'], "MAG");
			if (!isset($ds_info) || empty($ds_info)) {
				watchdog("islandora_sync", "Can't get datastream info from pid: !pid", array('pid' => $pid), WATCHDOG_ERROR);
				drupal_set_message("Can't get datastream info from pid {$pid}", $type = 'error');
				return -1;
			}

			module_load_include('php', 'islandora_mag', 'mag_utils');

			$xml_array_values = mag_xml_to_array($ds_info);

			$this->pid = $form_values['storage']['source_pid'];
			$this->cm = ContentModel::loadFromObject($form_values['storage']['source_pid']);
			$this->item = new fedora_item($form_values['storage']['source_pid']);
		}
		
		$form['#multistep'] = TRUE; // used so that it triggers a form rebuild every time.
		
		$this->createBaseForm($form, $action);
			
		foreach ($elements as $element) {
			$name = explode('][', $element['name']);

			$excluded_values = array ( "dc:identifier", "mag:img:file", "mag:img:md5", "mag:img:filesize",
				"mag:img:image-dimentions:niso:imagelength", "mag:img:image-dimentions:niso:imagewidth",
				"mag:img:datetimecreated"
				);
				$the_name = $name[sizeof($name)-1];
				$the_value = $xml_array_values[$the_name];
					
				if ( isset($the_value) &&  !in_array( $the_name, $excluded_values ) ) {
					$el_value = $the_value;
				}
				else {
					$el_value = "";
				}
					
					
				if ($element['type'] == 'markup') {
					$el = array('#value'=> $element['description']);
				}
				else {
					$el = array(
		          '#title' => t($element['label'], array(), $element['lang']),
		          '#required' => ($element['required'] ? 1 : 0),
		          '#description' => t($element['description'] , array(), $element['lang']),
		          '#type' => $element['type'],
					'#default_value' => $el_value
					);
				}

					
				$elLocation = &$form['indicator2'];
				while (isset($elLocation[$name[0]]) && ($partial = array_shift($name)) != NULL) {
					$elLocation = &$elLocation[$partial];
				}

				$autocomplete_path = FALSE;
				$autocomplete_omit_collection = FALSE;
				foreach ($element['parameters'] as $key => $val) {
					if ($key == '#autocomplete_path') {
						$autocomplete_path = $val;
					}
					elseif ($key == '#autocomplete_omit_collection') {
						$autocomplete_omit_collection = TRUE;
					}
					else {
						$el[$key] = $val;
					}
				}

				if ($autocomplete_path !== FALSE) {
					$el['#autocomplete_path'] = $autocomplete_path . (!$autocomplete_omit_collection?'/'.$form_values['storage']['collection_pid']:'/');
				}

				if ($element['type'] == 'select' || $element['type'] == 'other_select') {
					$el['#options'] = isset($element['authoritative_list']) ? $element['authoritative_list'] : array();
				}

				$elLocation[join('][', $name)] = $el;
		}

		return $form;
	}


	/**
	 * Creates MAG's editing form
	 *
	 * @return $form
	 */
	function magBuildEditMetadataForm($action = "edit") {
		$form['#multistep'] = TRUE; // used so that it triggers a form rebuild every time.

		$this->createBaseForm($form, $action);

		//ID del datastream
		$dsId = "MAG";

		if ($this->cm !== FALSE && $this->item != NULL) {
			$form['pid'] = array('#type'=>'hidden','#value'=>$this->pid);
			$pid = $form['pid']['#value'];

			//elementi del form (metadigit][gen][mag:gen:campo ecc, ma anche dc:campo)
			$elements = $this->cm->getIngestFormElements();

			//XML metadigit (verifica la versione del contenuto ... possono esserci più xml content dopo ogni modifica...)
			$content = $this->item->get_datastream_dissemination($dsId);

			$dom = new DOMDocument();
			if (!$dom->loadXML($content)) {
				echo "error loading xml";
			}

			$xpath = new DOMXPath($dom);

			// Register the php: namespace (required)
			$xpath->registerNamespace("php", "http://php.net/xpath");

			// Register PHP functions (no restrictions)
			$xpath->registerPHPFunctions();

			foreach ($elements as $element) { // per ogni elemento del form (per come è nel CM)
				$name = explode('][', $element['name']);
				$elLocation = &$form['indicator2'];
					
				while (isset($elLocation[$name[0]]) && ($partial = array_shift($name)) != NULL) {
					$elLocation = &$elLocation[$partial];
				}

				$tags = explode(":", $name[0]);

				if ($tags[0] == "mag") {
					if ($tags[1] == "gen") { //mag -> gen fields
						$path = $tags[2];
					}
					else { //mag -> doc|img|video|audio
						if (isset($tags[4]) and strcmp($tags[3],"niso") == 0) { //ex. mag:img:image-metrics:niso:bitpersample
							$path = $tags[3] . ":" . $tags[4];
						}
						elseif (isset($tags[4])) {
							$path = $tags[4];
						}
						elseif (isset($tags[3])) {
							$path = $tags[3];
						}
						else {
							$path = $tags[2];
						}
					}
				}
				elseif ($tags[0] == "dc") {
					$path = $name[0];
				}

				$nodeList = $xpath->query('//' . $path);

				$el = array(
				  '#title' => $element['label'],
				  '#required' => ($element['required'] ? 1 : 0),
				  '#description' => $element['description'],
				  '#type' => $element['type'],
				);

				if ($el['#type'] == 'list') {
					$values = array();
					for ($i = 0; $i < $nodeList->length; $i++) {
						$values[] = $nodeList->item($i)->nodeValue;
					}

					$el['#default_value'] = join('; ', $values);
				}
				else {
					if (($tags[2] == "format" OR $tags[2] == "image-dimentions" OR $tags[2] == "image-metrics") AND !isset($tags[3])) {
						//l'elemento in questione è format ma non un suo sottoelemento
						$el['#default_value'] = "";
					}
					else {
						$el['#default_value'] = $nodeList->item(0)->nodeValue;
					}
				}

				if ($element['type'] == 'select' || $element['type'] == 'other_select') {
					$el['#options'] = isset($element['authoritative_list']) ? $element['authoritative_list'] : array();
				}

				$elLocation[join('][', $name)] = $el;
			}
		}

		$form['submit'] = array(
		  '#type' => 'submit',
		  '#submit' => array('fedora_repository_edit_qdc_form_submit'),
		  '#value' => 'Save Metadata'
		 );

		return $form;
	}

	/**
	 * Gestisce i metadata modificati e aggiorna il datastream.
	 * I parametri in ingresso provengono da "ContentModel.inc" (riga 1153)
	 * nel caso dell'invocazione del metodo di gestione del metadata custom.
	 */
	function magHandleEditMetadataForm(&$form_id, &$form_values, &$soap_client) {
		foreach ($form_values as $key => $value) {
			if (isset($_POST[$key]) && $key != "form_build_id") {
				$form_values[$key] = $_POST[$key];
			}
		}
			
		//retrieve collection pid...
		$uris = __getCollectionPid($this->pid);
		$form_values['collection_pid'] = $uris[0];
			
		$dummy = $this->magUpdateDCMetadata($form_id, $form_values, $soap_client);
			
		$dom = new DomDocument("1.0", "UTF-8");
		$dom->formatOutput = TRUE;

		$mag = $this->createMetadigitStream($form_values, $dom, $action="edit");
		$dom->appendChild($mag);

		if ($this->item->modify_datastream_by_value($dom->saveXML(), 'MAG', "MAG Record", 'text/xml') !== NULL) {
			drupal_set_message(t('Success changing MAG for the object "%pid"', array('%pid' => $this->pid)));

			//invoke all fedora_after_edit hooks
			$hook = 'fedora_after_edit';
			foreach (module_implements($hook) as $module) {
				$function = $module . '_' . $hook;
				$function($form_values, $this->pid);
			}
		}
		else {
			drupal_set_message(t('Errori nella modifica del datastream per l\'oggetto "%pid".',	array('%pid'=>$this->pid)));
		}

		drupal_goto('/fedora/repository/'. $this->pid);
	}



	function __getCollectionPid($pid) {
		$uris = array();

		module_load_include('inc', 'fedora_repository', 'ObjectHelper');
		$object_helper = new ObjectHelper();
		$collection_objs = $object_helper->get_parent_objects($pid);
			
		try {
			$parent_collections = new SimpleXMLElement($collection_objs);
		}
		catch (exception $e) {
			drupal_set_message(t('Error getting parent objects !e', array('!e' => $e->getMessage())));
			return;
		}

		foreach ($parent_collections->results->result as $result) {
			foreach ($result->object->attributes() as $a => $b) {
				if ($a == 'uri') {
					$uri = (string) $b;
					$uri = substr($uri, strpos($uri, '/')+1);
				}
			}

			$uris[] = $uri;
		}

		return $uris;
	}


	/**
	 * Per modificare il datastream DC (di base) è necessario passare il titolo modificato
	 * e l'id dell'oggetto e invocare la funzione "updateMetaData" di formClass.
	 */
	function magUpdateDCMetadata($form_id, $form_value, $soap_client) {
		module_load_include('inc', 'fedora_repository', 'formClass');

		$metaDataForm = new formClass();

		$form_dc = array('dsid' => 'DC'); //va imposto

		//i campi del DC devono essere chiamati dc:campo-
		//NB il trattino finale è fondamentale
		foreach ($form_value as $key => $value) {
			if ($key == "dc:title") {
				$form_dc["dc:title-"] = $value;
			}
			if ($key == "dc:identifier") {
				$form_dc["dc:identifier-"] = $value;
				$form_dc['pid'] = $value;
			}
		}

		$ret = $metaDataForm->updateMetaData($form_id, $form_dc, $soap_client);

		return;
	}

	/**
	 * When ingesting a new Digital Image Object, the operator would choose
	 * to insert a label on the bottom of the image.
	 *
	 * @param array $form_values
	 */
	function magWatermark($form_values) {
		// Create Watermark if exists
		if (isset($form_values['watermark']) and !empty($form_values['watermark']) ) {
			$wmk_cc = variable_get('islandora_mag_img_cc_str', 'You can remove this label without breaking the Creative Commons license.');
			$wmk_str = $form_values['watermark'];

			$img_in = str_replace(" ", "\ ", $form_values["ingest-file-location"]);
			$wmk_w = exec('exiftool -ImageWidth -s -s -s ' . $img_in);
			$wmk_h = 50; //fixed height px

			exec("/usr/bin/convert -size " . $wmk_w . "x" . $wmk_h . " xc:white -stroke black -strokewidth 0 -draw \"line 0,0 " . $wmk_w . ",0\" label_0.png");
			exec("/usr/bin/convert label_0.png -gravity northeast -pointsize 12 -draw \" fill black  text 0,5 '" . $wmk_cc  . "'\"  label_1.png");
			exec("/usr/bin/convert label_1.png -gravity south     -pointsize 16 -draw \" fill black  text 0,5 '" . $wmk_str . "'\"  label_2.png");
			exec("/usr/bin/convert " . $img_in . " label_2.png -append " . $img_in);

			watchdog(
  			'[Islandora MAG] Watermark',
  			'Added watermark: "' . $form_values['watermark'] . '" to "' . $form_values["ingest-file-location"] . '"'
  			);
		}
		else {
			watchdog(
  			'[Islandora MAG] Watermark',
  			'Watermark empty for "' . $form_values["ingest-file-location"] . '"'
  			);
		}
	}

}
