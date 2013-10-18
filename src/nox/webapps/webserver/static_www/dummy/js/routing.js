var addVirtualPath = false;
var nodePathSel = null;
var nodePathSource =null;
var nodePathDest =null;
var portPathSource =null;
var portPathDest =null;
var hostIpSource =null;
var hostIpDest =null;
var nodesOfPath = null;
var objGraft=null;
var pathTimer=null;
var pathExisting = null;
var openedPath = null;

//init per l'aggiunta del virtual path
function activeMenuPath()
{
	$('#addPath').attr('disabled','disabled');
	addVirtualPath = true;
 	$('#virtualPath').html("<div id='leftPath' class='span6'>Select Source</div><div id='rightPath' class='span6'>Select Destination</div>");
	nodePathSource =null;
	nodePathDest =null;
	portPathSource =null;
	portPathDest =null;
	hostIpSource =null;
	hostIpDest =null;
	if(nodesOfPath!=null)eraseVirtualPathLine(openedPath);
	
}

//visualizza il menu sul click del nodo salvandone il nome
function showMenuPath(xPos,yPos,node){
	
	if ((nodePathSource == node)||(nodePathDest == node)){
		alertMessage("Select another Node");
	}else{
		$('#menuPath').hide();
		$('#menuPath').show(300);  
		$('#menuPath').offset({ top: yPos, left: xPos });
		nodePathSel = node;
	}
}

//gestione del click sul menu
function selectNodeOne(){
	$('#menuPath').hide();
	displayPortPath("leftPath");
	nodePathSource = nodePathSel;
}

//gestione del click sul menu
function selectNodeTwo(){
	$('#menuPath').hide();
	displayPortPath("rightPath");
	nodePathDest = nodePathSel;
}

//visualizza le porte attive sul nodo selezionato, target mi identifica se source (left) o dest (right)
function displayPortPath(target){

	$.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/node/"+nodePathSel, function(data) {   

          $("#"+target).html("Ports Active of node "+nodePathSel+"<div id ='"+target+"Port' class='btn-group' data-toggle='buttons-radio'></div>");
		
	      $.each(data.result.Port_Names, function(i,port) {

		//vedo i link di ogni interfaccia
	        $.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/node/"+nodePathSel+"/port/"+data.result.Port_Index[i], function(data1) {

		 if (data1.result.links != 'None'){
			
			 $.each(data1.result.links, function(link) {

		         $.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/node/"+nodePathSel+"/port/"+data.result.Port_Index[i]+"/link/"+link,function(data2){	
			    if (data2.result.node != null){
			    	// su ogni link un solo nodo non bisogna fare $each
			    	$("#"+target+"Port").append("<button id='button"+target+data.result.Port_Index[i]+"' class='btn btn-blue' onClick=portPathSelect("+data.result.Port_Index[i]+",'"+target+"','');>"+port+" --> node"+data2.result.node+"</button>");
			    }else{
				$("#"+target+"Port").append("<button id='button"+target+data.result.Port_Index[i]+"' class='btn btn-blue' onClick=portPathSelect("+data.result.Port_Index[i]+",'"+target+"','"+data2.result['IP Addr']+"');>"+port+" --> "+data2.result.Name+"</button>");
				}
			 });	
		      });
                   }
			
	     
		
	     });});});
	
}

// setta la porta sorgente e di destinazione e visualizza la finestra dei parametri quando entrambe le porta sono settate
function portPathSelect(idPort,target,hostIp){

	if (target === "leftPath"){
		portPathSource = idPort;
		hostIpSource = hostIp;
	}else{
		portPathDest = idPort;
		hostIpDest = hostIp;
	}

	if ((portPathSource != null)&&(portPathDest != null)){
		$('#dp_src').val(nodePathSource);
		$('#dp_dst').val(nodePathDest);
		$('#nw_src').val(hostIpSource);
		$('#nw_dst').val(hostIpDest);
		$('#first_port').val(portPathSource);
		$('#last_port').val(portPathDest);
		$('#myModal').modal({backdrop:"static"});
		$('#myModal').modal('show');
	}
}

//resetta le porte al click su close della finestra dei parametri
function closeModal(){

	$("#buttonleftPath"+portPathSource).removeClass("active");
	$("#buttonrightPath"+portPathDest).removeClass("active");
	portPathSource = null;
	portPathDest = null;
	hostIpSource = null;
	hostIpDest = null;
	$('#pathParameters')[0].reset();	
	$('#myModal').modal('hide');
	
	
}		

//invia i dati
function submitModal(){

	$.ajax({
      type: "POST",
      url: serverPath+"/netic.v1/OFNIC/virtualpath/create",
      data: $("#pathParameters").serialize(),
      error: function() {
        alertMessage("Creation failed. Try again.");
      },
      success: function() {
        alertMessage("Creation Successfull!!!");      
      },
      complete: function() {
        addVirtualPath = false;
	
	$('#addPath').removeAttr('disabled');
	closeModal();
	displayVirtualPath();
      }
    });	
	
}


//visualizza i virtual path esistenti
function displayVirtualPath(){

$('#virtualPath').html("<div id='displayPath' class='accordion'></div>");

$.getJSON(serverPath+"/netic.v1/OFNIC/virtualpaths", function(data) {
	
	pathExisting = data.result.Paths;

	$.each(data.result.Paths, function(i,path) {
		if (path != ""){            
		$('#displayPath').append("<div id='accordion-group"+path+"'class='accordion-group'><div class='accordion-heading'><a  class='accordion-toggle' data-toggle='collapse' data-parent='#displayPath' href='#collapse"+i+"'>Virtual Path " + path + "</a></div><div style='float:right;'><a class='btn btn-danger' href=javascript:removeVirtualPath('"+path+"'); ><i class='icon-trash icon-white'></i></a></div> <div id='collapse"+i+"' class='accordion-body collapse' style='clear:both;'><div id='inner-"+path+"' class='accordion-inner'></div></div></div>");
				    
		$('#collapse'+i).on('shown', function () {
		getInfoVirtualPath(path);}); 

		$('#collapse'+i).on('hidden', function () {
		eraseVirtualPathLine(path);});
}

	  });	
       });
}

//rimuove il virtual path selezionato
function removeVirtualPath(path){
$.ajax({

      type: "DELETE",
      url: serverPath+"/netic.v1/OFNIC/virtualpath/"+path,
      
      error: function() {
        alertMessage("Remotion failed. Try again.");
      },
      success: function() {
        alertMessage("Remotion Successfull!!!");      
      },
      complete: function() {
	if(openedPath==path)eraseVirtualPathLine(path);
	timerVirtualPath();
      }
    });	
}

//visualizza le info del virtual path selezionaton e aumenta la larghezza degli archi interessati
function getInfoVirtualPath(path){
	
	 $.getJSON(serverPath+"/netic.v1/OFNIC/virtualpath/"+path, function(data) {   
	
	$("#inner-"+path).html( "<table><tr><td>Destination Ip: </td><td>"+data.result['Dest IP']+"</td></tr><tr><td>Time Remaining: </td><td>"+data.result['Time Remaining']+"</td></tr><tr><td>Nodes: </td><td>"+data.result.Nodes+"</td></tr><tr><td>Source Ip: </td><td>"+data.result['Source IP']+"</td></tr><tr><td>Bandwidth: </td><td>"+data.result.Bandwidth+"</td></tr><tr height='25'></tr></table>");
       
	nodesOfPath = data.result.Nodes; 
	openedPath = path;
	for (var i=0;i<nodesOfPath.length-1;i++)
	{
		eval('objGraft='+"{'nodes':{},'edges':{'"+nodesOfPath[i]+"':{'"+nodesOfPath[i+1]+"':{'lineWidth':5}}}}");
		sys.graft(objGraft);
	} 
   });

	
}

//resetta la dimensione del link tra i nodi
function eraseVirtualPathLine(path){

	if (openedPath == path){
		for (var i=0;i<nodesOfPath.length-1;i++)
		{
			eval('objGraft='+"{'nodes':{},'edges':{'"+nodesOfPath[i]+"':{'"+nodesOfPath[i+1]+"':{'lineWidth':1}}}}");
			sys.graft(objGraft);
		}
		nodesOfPath=null;
		openedPath = null;
	}
}


//elimina la visualizzazione dei path scaduti o eliminati
function timerVirtualPath(){

$.getJSON(serverPath+"/netic.v1/OFNIC/virtualpaths", function(data) {
	var tempPathExisting = pathExisting;	
	if ((tempPathExisting.length > data.result.Paths.length)||((tempPathExisting!="")&&(data.result.Paths==""))){
			
		for (var i=0;i<tempPathExisting.length;i++)
		{
			 if ($.inArray(tempPathExisting[i],data.result.Paths)==-1){
					$('#accordion-group'+tempPathExisting[i]).remove();
					if(openedPath==tempPathExisting[i])eraseVirtualPathLine(openedPath);	
				}
			
		} 
		pathExisting = data.result.Paths;
	}
       });

}
