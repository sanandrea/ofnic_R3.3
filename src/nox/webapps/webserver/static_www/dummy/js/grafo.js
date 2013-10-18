
function generateGraph(){
//SCRIPT PER LA GENERAZIONE DEL GRAFO DI RETE

    // vedo i nodi della rete	  
    $.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network", function(data) {
	//grafo principale
	sys = arbor.ParticleSystem(1000); // creo un sistema di particelle
	sys.parameters({gravity:true}); // includo la gravità
	sys.renderer = Renderer("#viewport"); //inizio a disegnare nel viewport
	

	
      if (data.result.Nodes.length != 0){
	 //aggiungo i nodi
	$.each(data.result.Nodes, function() {
	   sys.addNode(this,{color:'#b01700', shape:'dot', label:this});
	});
	
        	
	$.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/all", function(data) {
	$.each(data.result.pairs, function(i,nodes) {
             sys.addEdge(sys.getNode(nodes[0]),sys.getNode(nodes[1]),{lineWidth:1});
	  });

	$.each(data.result.hosts, function(i,hosts) {
	     if (sys1.getNode(hosts[1])==null) sys.addNode(hosts[1],{color:'#0000cd', label:hosts[1]});
             sys.addEdge(sys.getNode(hosts[0]),sys.getNode(hosts[1]),{lineWidth:1});
	  });	
       });
          
      }
      else{alertMessage("None Nodes detected");}
    });
}


function findNode(nameNode){

   if (nameNode != nodeSelectBefore){
   //RESTITUISCE LE INTERFACCE DEL NODO
   $.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/node/"+nameNode, function(data) {   
	if (statOption == 1){   
		setPortsStat(data.result,nameNode);
	}else{
		setPorts(data.result,nameNode);
	}	
   });
   }
}


function generateGraphPort(nameInterface, selectedNode, index){
        
				
		
		//vedo i link di ogni interfaccia
	        $.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/node/"+selectedNode+"/port/"+index, function(data1) {
		   if (data1.result.links != 'None'){
			// genera tabella delle info della porta
			displayPortInfo(data1.result, nameInterface);
		     
			 $.each(data1.result.links, function(link) {

		         $.getJSON(serverPath+"/netic.v1/OFNIC/synchronize/network/node/"+selectedNode+"/port/"+index+"/link/"+link,function(data2){	
			    if (data2.result.node != null){
			    	// su ogni link un solo nodo non bisogna fare $each
			    	sys1.addNode(data2.result.node,{color:'#b01700', shape:'dot', label:data2.result.node});
			    	if (sys1.getNode(nameInterface)==null) sys1.addNode(nameInterface,{color:'#32CD32', label:nameInterface});
                            	sys1.addEdge(sys1.getNode(nameInterface),sys1.getNode(data2.result.node));
			    }else{
				sys1.addNode(data2.result.Name,{color:'#0000cd', label:data2.result.Name});
			    	if (sys1.getNode(nameInterface)==null) sys1.addNode(nameInterface,{color:'#32CD32', label:nameInterface});
                            	sys1.addEdge(sys1.getNode(nameInterface),sys1.getNode(data2.result.Name));
				}
			 });	
		      });
                   }
	   
	});   
}

