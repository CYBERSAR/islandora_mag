// $Id: script.js,v1 $
(function($) {
	Drupal.behaviors.islandoraMag = function (context) {
		$("#main").before('<div id="relmask" style="display:none"></div>');
		$("#main").append('<div id="reldialog" class="relwindow" style="padding: 10px; display:none"><span><b>Oggetti in relazione</b><a href="#" class="close" style="float:right">Chiudi</a></span></div>');
		$("#reldialog").append('<div id="reldialog-content" style="padding: 25px"></div>');
		
		$("input.form-text[name='dc:relation']").click(function(e) {
				e.preventDefault();
				
				var maskHeight = $(document).height();
				var maskWidth = $(window).width();
				$('#relmask').css("position", "absolute");
				$('#relmask').css("z-index", "9999");
				$('#relmask').css({'width':maskWidth,'height':maskHeight});
				$('#relmask').css("background-color", "#000");
				$('#relmask').fadeIn(500);	
				$('#relmask').fadeTo("slow",0.8);	
				
				get_relations();
		});
		
		//if close button is clicked
		$('.relwindow .close').click(function (e) {
			//Cancel the link behavior
			e.preventDefault();
			$('#relmask, .relwindow').hide();
		});		
		
		//if mask is clicked
		$('#relmask').click(function () {
			$(this).hide();
			$('.relwindow').hide();
		});
		
	};
	
	function get_relations() {
		$(".relwindow").fadeIn(500);	
		$('.relwindow').fadeTo("slow");
		
		var url = window.location.toString().split("/");
		url = url[url.length-2];
		
		$.ajax({
	        url: "/islandora_mag/ajax_calls/get_objects_by/" + url, //menu callback defined in .module
	        global: false,
	        type: "GET",
	        dataType: "html",
	        async: true,
	        success : function (data,stato) {
	        	$("#reldialog-content").html(data);
	        },
	        error : function (richiesta, stato, errori) {
	          alert("E' avvenuto un errore: " + stato);
	        }
	    });
		
		$('#reldialog').dialogCenter();
	}

	$.fn.dialogCenter = function () {
	    this.css("position", "absolute");
	    this.css("z-index", "9999");
	    this.css("border", "2px solid black");
	    this.css("background-color", "#FFF");
	    this.css("top", $(window).scrollTop() + 50 +  "px");
	    this.css("left", $(window).width()/6 + "px");
	    this.css("right", $(window).width()/8 + "px");
	    return this;
	}

}(jQuery));