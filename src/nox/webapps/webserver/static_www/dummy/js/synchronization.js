var portSelectBefore = null;
var nodeSelectBefore = null;

// gestione dei tasti delle interfacce
function PortSelect(intName,index)
{
   if (intName != portSelectBefore) {
      sys1.prune();  //CANCELLA IL GRAFO 	
      portSelectBefore = intName;
      generateGraphPort(intName,nodeSelectBefore,index);
   }
	
}
	
	// abilita le interfacce presenti sul nodo selezionato e disabilita le altre
	function setPorts(result,node)
	{
	      sys1.prune();  //CANCELLA IL GRAFO 
	      //CANCELLA le info della porta
    	      $('#port_info').html("");
	      $('#graph_info').text("");			
	      nodeSelectBefore = node;	
	      portSelectBefore = null;
		 
		$('#left').html( "<table><tr><td colspan='2'>Information about node "+node+"</td></tr><tr><td>Num Buffers: </td><td>"+result.Num_Buffers+"</td></tr><tr><td>Num Tables: </td><td>"+result.Num_Tables+"</td></tr><tr><td>Actions: </td><td>"+result.Actions+"</td></tr><tr height='25'></tr><tr><td colspan='2'>Ports of node "+node+"</td></tr><tr height='10'></tr></table><div id='portLeft' class='btn-group' data-toggle='buttons-radio'></div>");
		
	      $.each(result.Port_Names, function(i,port) {
		$.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/node/"+node+"/port/"+result.Port_Index[i], function(data1) {

		 $('#portLeft').append("<button id='"+port+"' class='btn btn-blue' onClick=PortSelect('"+port+"','"+result.Port_Index[i]+"');>"+port+"</button>");

		if (data1.result.links == 'None'){
			$("#"+port).attr('disabled','disabled');       
		}	
	      });});

	
           
	}	


function displayPortInfo(result, port){
	$('#port_info').html( "<table><tr><td colspan='2' width='400'>Information about port "+port+"</td></tr><tr><td>Active: </td><td>"+result.Active+"</td></tr><tr><td>Config: </td><td>"+result.Config+"</td></tr><tr><td>State: </td><td>"+result.State+"</td></tr><tr><td>Speed: </td><td>"+result.Speed+"</td></tr><tr height='25'></tr></table>");

       $('#graph_info').text("link of port " + port);	
}
