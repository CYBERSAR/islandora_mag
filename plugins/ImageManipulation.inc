<?php

// $Id$

/*
 *
 *
 * This Class implements the methods defined in the STANDARD_IMAGE content model
 */

class ImageManipulation {

  function ImageManipulation() {
    module_load_include('inc', 'fedora_repository', 'ObjectHelper');
  }

  function createPreview($parameterArray, $dsid, $file, $file_ext) {
    $system = getenv('System');
    $file_suffix = '_' . $dsid . '.' . $file_ext;
    $width = $parameterArray['width']; 

    if (!isset($parameterArray['height'])) {
	$height = $width;
    } else { 
	$height = $parameterArray['height'];
    }

    $returnValue = TRUE;
    $output = array();
    
    $destDir  = dirname($file).'/work';
    $destFile = $destDir .'/'. basename($file) . $file_suffix;
    if (!is_dir($destDir)) { 
      @mkdir($destDir);
    }

    if (!file_exists($destFile)) {
      exec('convert -resize ' . $width . 'x'.$height.' -quality 85  "' . $file . '"[0] -strip "' .$destFile . '" 2>&1 &', $output, $returnValue);
    }
    else
      $returnValue = '0';

    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files']["$dsid"] = $destFile;
      return TRUE;
    }
    else {
      return $returnValue;
    }
  }

  function createPNG($parameterArray = NULL, $dsid, $file, $file_ext) {
    $file_suffix = '_' . $dsid . '.' . $file_ext;
    $returnValue = TRUE;

    if (!file_exists($file . $file_suffix)) {
      system("convert  \"$file\"  \"$file$file_suffix\" 2>&1 &", $returnValue);
    }
    else {
      $returnValue = '0';
    }

    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files']["$dsid"] = $file . $file_suffix;
      return TRUE;
    }
    else {
      return $returnValue;
    }
  }

  function createJP2($parameterArray = NULL, $dsid, $file, $file_ext) {
  	$file_ext = "jpx";
    $file_suffix = "_$dsid.$file_ext";
    $return_value = TRUE;

    $output = array();
    
    //try to see if tiff is compressed: if so, we need to decompress it because kdu_compress can't manage compressed tiff in it's free version
    $cmd = "exiftool -Compression -s -s -s " . $file;
    $compression = exec($cmd);
    
    $filein = $file;
    if ($compression != "Uncompressed") {
    	$filenew = $file . ".new.tiff";
    	$cmd = "convert -compress None -colorspace sRGB " . $file . " " .  $filenew;
    	exec($cmd);
    	$filein = $filenew;
    }
    
    $opts = ' -rate 0.5 Clayers=1 Clevels=7 "Cprecincts={256,256},{256,256},{256,256},{128,128},{128,128},{64,64},{64,64},{32,32},{16,16}" "Corder=RPCL" "ORGgen_plt=yes" "ORGtparts=R" "Cblk={32,32}" Cuse_sop=yes';
    exec('kdu_compress -i "' . $filein . '" -o "' . $file . $file_suffix . '" '. $opts, $output, $returnValue);

    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files'][$dsid] = $file . $file_suffix;
    }
    else {
      ObjectHelper::warnIfMisconfigured('Kakadu');
    }

    $opts = ' -rate -,0.5 Clayers=2 Creversible=yes Clevels=8 "Cprecincts={256,256},{256,256},{128,128}" Corder="RPCL" ORGgen_plt="yes" ORGtparts="R" Cblk="{32,32}"';
    exec('kdu_compress -i "' . $filein . '" -o "' . $file . "_lossless." . $file_ext . '" ' . $opts, $output, $returnValue);
    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files']["LOSSLESS_JP2"] = $file . '_lossless.' . $file_ext;
    }
    else {
      ObjectHelper::warnIfMisconfigured('Kakadu');
    }
    
	/*		
	exec('convert "' . $filein . '" -define jp2:tilewidth=256 -define jp2:tileheight=256 "' . $file . $file_suffix . '"', $output, $returnValue);
    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files'][$dsid] = $file . $file_suffix;
    }
    else {
      ObjectHelper::warnIfMisconfigured('Kakadu');
    }
    */
	
	//exec('convert "' . $filein . '" -define jp2:tilewidth=256 -define jp2:tileheight=256 "' . $file . "_lossless.jp2" . '"');
	//$_SESSION['fedora_ingest_files'][$dsid] = $file . "_lossless.jp2";
    
    exec("convert  -resize 800 \"{$filein}\"[0]  \"$file-med.jpg\"", $output, $returnValue);
    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files']["JPG"] = $file . '-med.jpg';
    }
    else {
      ObjectHelper::warnIfMisconfigured('ImageMagick');
    }
    exec("convert {$filein}[0] -thumbnail x2000 -thumbnail x450  -resize '450x<' -resize 50% -fuzz 1% -trim +repage -gravity center -crop 200x200+0+0 +repage -format jpg -quality 100  $file-tn.jpg", $output, $returnValue);
    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files']["TN"] = $file . '-tn.jpg';
    }
    else {
      ObjectHelper::warnIfMisconfigured('ImageMagick');
    }
    
    if ($compression != "Uncompressed") {
    	unlink($filenew);
    }
    

    if ($returnValue == '0') {
      return TRUE;
    }
    else {

      return $returnValue;
    }
  }

  //use imagemapi to manipulate images instead of going directly to imagemagick or whatever
  function manipulateImage($parameterArray = NULL, $dsid, $file, $file_ext) {
    $height = $parameterArray['height'];
    $width = $parameterArray['width'];

    $file_suffix = '_' . $dsid . '.' . $file_ext;
    $returnValue = TRUE;


    $image = imageapi_image_open($file);

    if (!$image) {
      drupal_set_message(t("Error opening image."));
      return FALSE;
    }

    if (!empty($height) || !empty($width)) {
      $returnValue = imageapi_image_scale($image, $height, $width);
    }

    if (!$returnValue) {
      drupal_set_message(t("Error scaling image."));
      return $returnValue;
    }
    $filename = substr(strrchr($file, '/'), 1);
    $output_path = $_SERVER['DOCUMENT_ROOT'] . base_path() . file_directory_path() . '/' . $filename . $file_suffix;
    $returnValue = imageapi_image_close($image, $output_path);
    if ($returnValue) {
      $_SESSION['fedora_ingest_files']["$dsid"] = $file . $file_suffix;
      return TRUE;
    }
    else {
      return $returnValue;
    }
  }

  function createThumbnailFromPDF($parameterArray, $dsid, $file, $file_ext) {
    $height = $parameterArray['height'];
    $width = $parameterArray['width'];
    $file_suffix = '_' . $dsid . '.' . $file_ext;
    $returnValue = TRUE;
    //system("convert $file\[0\] -thumbnail 128x128 $uploaddir$thumb");
    // Use this for Linux.
    if (stristr($_SERVER['SERVER_SOFTWARE'], 'microsoft')) {
      
    }
    elseif (stristr($_SERVER['SERVER_SOFTWARE'], 'linux')) {
      $cmdline = "/usr/local/bin/convert \"$file\"\[0\] -colorspace RGB -thumbnail $width" . "x$height \"$file$file_suffix\"";
    }
    elseif (stristr($_SERVER['SERVER_SOFTWARE'], 'unix')) {
      // Use this for Mac OS X (MAMP)
      $cmdline = "sips -s format jpeg \"$file\" -z $height $height --out \"$file$file_suffix\" >/dev/null";
    }
    else {
      $cmdline = "convert \"$file\"\[0\] -colorspace RGB -thumbnail " . $width . "x" . $height . " \"$file$file_suffix\"";
    }

    system($cmdline, $returnValue);
    //system("convert $file\[0\] -thumbnail 128x128 $uploaddir$thumb");
    $var = $file . $file_suffix . ' returnvalue= ' . $returnValue;

    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files']["$dsid"] = $file . $file_suffix;
      return TRUE;
    }
    else {
      return $returnValue;
    }
  }

  function createThumbnail($parameterArray, $dsid, $file, $file_ext) {
    // var_dump($parameterArray);exit(0);
    $file_suffix = '_' . $dsid . '.' . $file_ext;
    $height = $parameterArray['height'];
    $width = $parameterArray['width'];
    $returnValue = TRUE;
    system("convert -resize $width -quality 85  \"$file\"\[0\] -strip \"$file$file_suffix\" 2>&1 &", $returnValue);
    //system("convert -resize $width -quality 85  \"$file\" -strip \"$file$file_suffix\"",$returnValue);

    if ($returnValue == '0') {
      $_SESSION['fedora_ingest_files']["$dsid"] = $file . $file_suffix;
      return TRUE;
    }
    else {
      return $returnValue;
    }
  }

}
