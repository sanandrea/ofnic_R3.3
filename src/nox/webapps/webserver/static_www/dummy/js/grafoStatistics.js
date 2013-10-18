var packetCount = [];
var byteCount = [];
var totalPoints = 50;
var monitorID = null;
var options = null;
var timerStat = null;

function getStatMonitor(monitor) {

    monitorID = monitor;
$("#inner-"+monitor).html("<div id='graficoStatistiche"+monitor+"' style='width:100%;height:300px;'></div>");
   packetCount = [];
    byteCount = [];
     while (packetCount.length < totalPoints) {
            packetCount.push(0);
        }

	while (byteCount.length < totalPoints) {
            byteCount.push(0);
        }

    // setup plot
    options = { 
        yaxis: { min: 0 },
        xaxis: { show: false }
    };
	getStat();
    timerStat = setInterval(getStat, 500);
}

function getStat() {
        
        packetCount = packetCount.slice(1);
	byteCount = byteCount.slice(1);

	$.getJSON(serverPath+"/netic.v1/OFNIC/statistics/path/"+monitorID, function(data1) {   
	
	packetCount.push(data1.result['Packet count']);
	byteCount.push(data1.result['Byte count']);

	
});

        var res = [], res1 = [];
        for (var i = 0; i < packetCount.length; ++i)
            res.push([i, packetCount[i]])

	for (var i = 0; i < byteCount.length; ++i)
            res1.push([i, byteCount[i]])
        
	$.plot($("#graficoStatistiche"+monitorID), [ {
            data: res,
            label:'Packet count',
	    lines: { show: true },
            points: { show: true }
            } , {
            data: res1,
            label:'Byte count',
	    lines: { show: true },
            points: { show: true }
            } ], options);
	
	
}


