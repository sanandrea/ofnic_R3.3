var alphabet = "abcdefghijklmnopqrstuvwxyz";

function checkThis(el){
document.myForm.elements['letters[]'][el].checked = 1;
}

function buildTable(){
var pre='<table cellpadding="0" cellspacing="4"><tr>';
var post='</tr></table>';
var cont='';
for (var i = 0; i < alphabet.length; i++) {
	var ch=alphabet[i];
	cont+='<td><input type="checkbox" name="letters[]" class="check-me" value="'+ch+'" /><span>'+ch.toUpperCase()+'</span><select onchange="checkThis('+i+');" name="'+ch+'"><option value="0">*</option><option value="1">1</option><option value="2">2</option><option value="3">3</option><option value="4">4</option><option value="5">5</option></select></td>';
}

$("letters_table").set('html',pre+cont+post);
}

window.addEvent('domready', function() {

buildTable();

var fx = {
	'loading': new Fx.Morph( 'loading', 'opacity',{ duration: 200 } ),
	'success': new Fx.Morph( 'success', 'opacity',{ duration: 200 } ),
	'fail': new Fx.Morph( 'fail', 'opacity',{ duration: 200 } )
};

// Hides the loading div, and shows the el div for
// a period of four seconds.
var showHide = function( el ){
	fx.loading.set(0);
	(fx[ el ]).start(0,1);
	(function(){ (fx[ el ]).start(1,0); }).delay( 4000 );
}


$('myForm').addEvent('submit', function(e) {

        e.stop();
         this.set('send',{
            url: this.get("action"),
            data: this,
         onRequest: function(){
			// Show loading div.
			fx.loading.start( 1,0 );
		},
		onSuccess: function(){
			// Hide loading and show success for 3 seconds.
			showHide( 'success' );

		},

		onComplete: function(response){
			jsonObj = JSON.decode(response);
			
			var html="<h2>"+jsonObj.length+" words found!</h2><br/><ul>";
			jsonObj.forEach(function(item) { 

				html+="<li>"+item+"</li>";

});
			html+="</ul>";

			$("result").set('html',html);
		},

		onFailure: function(){
			// Hide loading and show fail for 3 seconds.
			showHide( 'fail' );
		}
        }).send();
    }); 

});
