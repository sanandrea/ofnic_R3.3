var statOption = 0;

//setta le variabili e il template per ottenere le statistiche sulla porta
function activeGetPortStat(){
	statOption = 1;
	nodeSelectBefore = null;	
        portSelectBefore = null;
        clearInterval(timerStat);
	$('#statistics').html("<div id='left' class='span5'>Select a node </div><div id='right' class='span7'>");

}

//setta le variabili e il template per ottenere le statistiche sulla porta
function activeAddPathStat(){
	statOption = 2;
	nodeSelectBefore = null;	
        portSelectBefore = null;
	clearInterval(timerStat);
	displayVirtualPathStat();
}

//visualizza le porte del nodo selezionato
function setPortsStat(result,node)
{
	      $('#right').html("");
    	      nodeSelectBefore = node;	
	      portSelectBefore = null;
		 
		$('#left').html( "<table><tr><td colspan='2'>Information about node "+node+"</td></tr><tr><td>Num Buffers: </td><td>"+result.Num_Buffers+"</td></tr><tr><td>Num Tables: </td><td>"+result.Num_Tables+"</td></tr><tr><td>Actions: </td><td>"+result.Actions+"</td></tr><tr height='25'></tr><tr><td colspan='2'>Ports of node "+node+"</td></tr><tr height='10'></tr></table><div id='portLeft' class='btn-group' data-toggle='buttons-radio'></div>");
		
	      $.each(result.Port_Names, function(i,port) {
		$.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/node/"+node+"/port/"+result.Port_Index[i], function(data1) {

		 $('#portLeft').append("<button class='btn btn-blue' onClick=PortSelectStat('"+port+"','"+result.Port_Index[i]+"');>"+port+"</button>");

	      });});
       
}

// gestione dei tasti delle interfacce
function PortSelectStat(intName,index)
{
   	if (intName != portSelectBefore) {
      		portSelectBefore = intName;
      	// get port stat
      	  	$.getJSON(serverPath+"/netic.v1/OFNIC/statistics/node/"+nodeSelectBefore+"/port/"+index, function(data) {
			displayPortStat(data.result,intName);	        
		});   
	}
   
}	

//visualizza le statistuche della porta
function displayPortStat(result, port){

	$('#right').html( "<table><tr><td colspan='2' width='400'>Statistics about port "+port+"</td></tr><tr><td>Tx_bytes: </td><td>"+result.Tx_bytes+"</td></tr><tr><td>Tx_errors: </td><td>"+result.Tx_errors+"</td></tr><tr><td>Rx_bytes: </td><td>"+result.Rx_bytes+"</td></tr><tr><td>Rx_errors: </td><td>"+result.Rx_errors+"</td></tr><tr height='25'></tr></table>");

      
}


//visualizza i virtual path esistenti per aggiungere un monitor
function displayVirtualPathStat(){

$('#statistics').html("<div id='displayPathStat' class='accordion'></div>");

$.getJSON(serverPath+"/netic.v1/OFNIC/virtualpaths", function(data) {
	
	pathExisting = data.result.Paths;

	$.each(data.result.Paths, function(i,path) {
		if (path != ""){            
		$('#displayPathStat').append("<div id='accordion-group"+path+"'class='accordion-group'><div class='accordion-heading'><a  class='accordion-toggle' data-toggle='collapse' data-parent='#displayPathStat' href='#collapse"+i+"'>Virtual Path " + path + "</a></div><div id='buttonPathStat"+path+"' class='btn-group' data-toggle='buttons-radio' style='float:right;'></div> <div id='collapse"+i+"' class='accordion-body collapse' style='clear:both;'><div id='inner-"+path+"' class='accordion-inner'></div></div></div>");
		
		$.getJSON(serverPath+"/netic.v1/OFNIC/virtualpath/"+path, function(data) {   
	
	for (var j=0;j<data.result.Nodes.length;j++)
	{
		$("#buttonPathStat"+path).append("<button id='btn"+path+"node"+data.result.Nodes[j]+"'class='btn btn-blue' onClick=portPathStatSelect('"+data.result.Nodes[j]+"','"+path+"')>"+data.result.Nodes[j]+"</button>");	
	}		    
		$('#collapse'+i).on('shown', function () {
		getInfoVirtualPath(path);}); 

		$('#collapse'+i).on('hidden', function () {
		eraseVirtualPathLine(path);});
	  });	
	} 
      });
});
}


// click per aggiungere il monitor sulla porta selezionata
function portPathStatSelect(node,path){

	$('#dpid').val(node);
	$('#PathID').val(path);
	$('#myModalStat').modal({backdrop:"static"});
	$('#myModalStat').modal('show');
	
}


//resetta le porte al click su close della finestra dei parametri
function closeModalStat(){

	$("#btn"+$('#PathID').val()+"node"+$('#dpid').val()).removeClass('active');
	$('#pathParameters')[0].reset();	
	$('#myModalStat').modal('hide');
	
	
}		

//invia i dati
function submitModalStat(){

	$.ajax({
      type: "POST",
      url: serverPath+"/netic.v1/OFNIC/statistics/path/create",
      data: $("#pathStatParameters").serialize(),
      error: function() {
        alertMessage("Creation failed. Try again.");
      },
      success: function() {
        alertMessage("Creation Successfull!!!");      
      },
      complete: function() {
        
	$('#addPathMonitor').removeClass('active');
	$('#pathStatDisplay').addClass('active');
	displayMonitorStat();
	closeModalStat();
      }
    });	
	
}

//visualizza i monitor attivi
function displayMonitorStat(){
clearInterval(timerStat);
$('#statistics').html("<div id='displayMonitor' class='accordion'></div>");

$.getJSON(serverPath+"/netic.v1/OFNIC/statistics/path/MonitorIDs", function(data) {
	
	
	$.each(data.result['MonitorIDs'], function(i,monitor) {

		if (monitor != ""){            
		$('#displayMonitor').append("<div id='accordion-group"+monitor+"'class='accordion-group'><div class='accordion-heading'><a  class='accordion-toggle' data-toggle='collapse' data-parent='#displayMonitor' href='#collapse"+i+"'>Monitor " + monitor + "</a></div><div style='float:right;'><a class='btn btn-danger' href=javascript:removeMonitorPath('"+monitor+"'); ><i class='icon-trash icon-white'></i></a></div> <div id='collapse"+i+"' class='accordion-body collapse' style='clear:both;'><div id='inner-"+monitor+"' class='accordion-inner'></div></div></div>");
				    
		$('#collapse'+i).on('shown', function () {
		getStatMonitor(monitor);}); 

		$('#collapse'+i).on('hidden', function () {
		clearInterval(timerStat);}); 
}

	  });	
       });

}
/*
//visualizza le statistiche del monitor
function getStatMonitor(monitor){
	
	 $.getJSON(serverPath+"/netic.v1/OFNIC/statistics/path/"+monitor, function(data) {   
	
	$("#inner-"+monitor).html( "<table><tr><td>Packet count: </td><td>"+data.result['Packet_per_s']+"</td></tr><tr><td>Byte count: </td><td>"+data.result['Byte_per_s']+"</td></tr><tr height='25'></tr></table>");
});
}*/


//rimuove il monitor
function removeMonitorPath(monitor){

$.ajax({

      type: "DELETE",
      url: serverPath+"/netic.v1/OFNIC/statistics/path/"+monitor,
      
      error: function() {
        alertMessage("Remotion failed. Try again.");
      },
      success: function() {
        alertMessage("Remotion Successfull!!!");      
      },
      complete: function() {
	$('#accordion-group'+monitor).remove();
      }
    });	


}
