var selectedContent = null;

function setContent(val){
	
   switch (val)
  {
	  case 0:
            $('#content').html("<div id='left' class='span5'> </div><div id='right' class='span7'> <div  class='span5' id='port_info' > </div><div id='graph_info'></div><canvas id='viewport1'></canvas></div>");
	
	    //grafo secondario
            sys1 = arbor.ParticleSystem(1000); // creo un sistema di particelle
	    sys1.parameters({gravity:true}); // includo la gravità
	    sys1.renderer = Renderer("#viewport1"); //inizio a disegnare nel viewport	      
	
	    if(nodesOfPath!=null)eraseVirtualPathLine(openedPath);
	    nodeSelectBefore = null;
	    selectedContent = 0;
	    statOption = 0;
	    addVirtualPath = false;
	    clearInterval(pathTimer);
	    clearInterval(timerStat);
	    $("li").removeClass("active");
	    $("li::eq(0)").addClass("active");
	    $('#menuPath').hide();
		addVirtualPath = false;
	    break;
	  case 1:
	    $('#content').html("<div class='span12' style='margin-bottom:25px;'><div class='btn-group' data-toggle='buttons-radio'><button id='portStat' class='btn btn-blue' onClick=activeGetPortStat()>Get Port Statistics</button><button id='pathStatDisplay' class='btn btn-blue' onClick=displayMonitorStat()>Get Monitor Statistic</button><button id='addPathMonitor' class='btn btn-blue' onClick=activeAddPathStat()>Add Monitor to a Path</button></div> </div><div id='statistics' class='span12'></div>");

	    if(nodesOfPath!=null)eraseVirtualPathLine(openedPath);
	    nodeSelectBefore = null;
	    clearInterval(pathTimer);
	    clearInterval(timerStat);
    	    selectedContent = 1;
	    statOption = 0;
	    addVirtualPath = false;
	    $("li").removeClass("active");
	    $("li::eq(1)").addClass("active");
	    displayMonitorStat();
	    $('#pathStatDisplay').addClass('active');
	    $('#menuPath').hide();
		addVirtualPath = false;
	    break;
	  case 2:
	    $('#content').html("<div class='span12' style='margin-bottom:25px;'><button id = 'addPath' value='addPath' class='btn btn-blue' onClick='activeMenuPath();'>Add new Virtual Path</button> </div><div id='virtualPath' class='span12'></div>");
	    
            displayVirtualPath();
	    if(nodesOfPath!=null)eraseVirtualPathLine(openedPath);
	    selectedContent = 2;
	    nodeSelectBefore = null;
	    statOption = 0;
	    pathTimer = setInterval(timerVirtualPath,10000);
	    clearInterval(timerStat);
	    $("li").removeClass("active");
	    $("li::eq(2)").addClass("active");
   	    $('#menuPath').hide();
		addVirtualPath = false;
	    break;
  }
}



