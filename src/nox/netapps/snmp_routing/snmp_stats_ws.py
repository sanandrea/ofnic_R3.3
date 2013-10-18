# Copyright 2013 (C) Universita' di Roma La Sapienza
# 
# This file is part of OFNIC Uniroma GE.
# 
# OFNIC is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# OFNIC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with OFNIC.  If not, see <http://www.gnu.org/licenses/>.

from nox.webapps.webservice      import webservice
from nox.lib.core import *
from nox.webapps.webservice.neticws      import *
from nox.netapps.discovery.discoveryws import *
from nox.netapps.netic.snmp_handler import snmpMega
from nox.netapps.snmp_routing.open_stats import open_stats

lg = logging.getLogger('statistics-ws')
hdlr = logging.FileHandler('OFNIC.log')
formatter = logging.Formatter('%(asctime)s|%(name)s|%(levelname)s|%(message)s')
hdlr.setFormatter(formatter)
lg.addHandler(hdlr) 
lg.setLevel(logging.WARNING)

#
# Verifies that node is a valid datapath name but based on discovery.py component
#
class WSPathExistingPathMonitorID(webservice.WSPathComponent):
    def __init__(self, statisticsws):
        webservice.WSPathComponent.__init__(self)
        self._statws = statisticsws
        
    def __str__(self):
        return "{Mon_ID}"

    def extract(self, node, data):
        if (node == None):
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            mid=int(str(node),16)
            if  mid not in (self._statws.monitorIDs): 
                e = "MonitorID '%s' is unknown" % node
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid node name %s" % str(node)
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(node)

class snmp_stats_ws(Component):
    """Web service for accessing statistics information"""
    
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)      

    def install(self):
        self.ll = self.resolve(snmpMega)
        self.openStats = self.resolve(open_stats)
        self.discoveryws = self.resolve(discoveryws)
        
        self.monitorIDs = []
        
        
        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request

        rootpath = self.discoveryws.rootpath
        statpath = rootpath + (webservice.WSPathStaticString("statistics"),)
        
        # /ws.v1/netic/OF_UoR/statistics/
        reg(self._statistics_root, "GET", statpath,"""Get network statistics info""","""Network Statistics Sub-Module""")

        nodePath = statpath + (webservice.WSPathStaticString("node"),)
        nodeIdPath = nodePath + (WSPathExistingNodeName_V2(self.discoveryws.discovery),)
        portPath = nodeIdPath + (webservice.WSPathStaticString("port"),)
        portIdPath = portPath + (WSPathExistingPortName_V2(self.discoveryws.discovery),)
        
        taskPath = statpath + (webservice.WSPathStaticString("task"),)
        # /ws.v1/netic/OF_UoR/statistics/task
        reg(self._get_all_mon_ids,"GET",taskPath, """Get a list of all path Monitor IDs""")

        createMonitorpath = taskPath + (webservice.WSPathStaticString("create"),)

        pathMonitorIDpath = taskPath + (WSPathExistingPathMonitorID(self),)

        # /ws.v1/netic/OF_UoR/statistics/node/<node id>/port/<port id>
        reg(self._get_port_stats, "GET", portIdPath, """Get Port Statistics""")

        # /ws.v1/netic/OF_UoR/statistics/path/create
        reg(self._create_flow_monitor,"POST",createMonitorpath,"""Begin monitoring a flow in a network node""")

        # /ws.v1/netic/OF_UoR/statistics/path/{Mon_ID}        
        reg(self._get_path_stats,"GET",pathMonitorIDpath, """Get statistics for the Monitor ID""")
        reg(self._remove_flow_monitor,"DELETE",pathMonitorIDpath,""" Delete the specified Monitor ID """)

    def _statistics_root(self, request, arg):
        a = {}
        a['Info'] = "Openflow Network Statistics about port and paths"
        neticResponse(request,NTC_OK,a)
    
    
    def _get_port_stats(self, request, arg):
        port = int(arg['{port_ID}'])
        dp = datapathid.from_host(int(arg['{node_ID}']))
        #pinfo=self.discoveryws.pytop.synchronize_port(dp, port)
        #print pinfo.portID
        a = {}
        pyload,found = self.ll.get_link_load(dp, port)        
        if (found == False):
            neticResponse(request,NTC_ST_ERR_PORT_NOT_FOUND)
            return
        #The following code is commented because errors are not
        # registered in the new snmp version
        
        #pyporterrors,found = self.ll.get_port_errors(dp, port)
        #if (found == False):
            #neticResponse(request,NTC_ST_ERR_PORT_NOT_FOUND)
            #return
        #print( "Tx Bytes: " + str(pyload.txLoad) + " Rx Bytes: " + str(pyload.rxLoad))
        a['Tx_bytes'] = pyload.txLoad
        a['Rx_bytes'] = pyload.rxLoad
        #a['Tx_errors'] = pyporterrors.txErrors
        #a['Rx_errors'] = pyporterrors.rxErrors
        
        neticResponse(request,NTC_OK,a)
        
    def _create_flow_monitor(self, request, arg):
        a = {}
        content = request.content.read()

        errorCode = {}
        try:
            pathID = int(str(request.args['PathID'][0]),16)
            dpid = datapathid.from_host(int(request.args['dpid'][0]))
            
        except KeyError:
            errorCode['errorCode'] = NTC_ST_ERR_MISSING_M_FIELD
            webservice.badRequest(request,errcode2str[NTC_ST_ERR_MISSING_M_FIELD],errorCode)
            return NOT_DONE_YET
        except TypeError:
            errorCode['errorCode'] = NTC_ST_ERR_BAD_ARGUMENT
            webservice.badRequest(request,errcode2str[NTC_ST_ERR_BAD_ARGUMENT],errorCode)
            return NOT_DONE_YET
        
        try:
            duration = int(request.args['duration'][0])
            frequency = int(request.args['frequency'][0])
        except:
            duration = 100
            frequency = 1;
        
        monitorID,error = self.openStats.create_new_flow_monitor(pathID,dpid,duration,frequency)
        
        if (error == 0):
            hexRes = "%x" %monitorID
            a['MonitorID'] = hexRes
            self.monitorIDs.append(monitorID)
            neticResponse(request,NTC_OK,a)
        if(error == 1):
            errorCode['errorCode'] = NTC_ST_ERR_PATH_NOT_FOUND
            webservice.badRequest(request,errcode2str[NTC_ST_ERR_PATH_NOT_FOUND],errorCode)
            return NOT_DONE_YET
        elif (error == 2):
            errorCode['errorCode'] = NTC_ST_ERR_DPID_NOT_FOUND
            webservice.badRequest(request,errcode2str[NTC_ST_ERR_DPID_NOT_FOUND],errorCode)
            return NOT_DONE_YET
                    
        
    def _get_path_stats(self,request, arg):
        a = {}
        monID = int(str(arg['{Mon_ID}']),16)
        flowLoad,error = self.openStats.get_flow_load(monID)
        if (error == 0):
            a['Byte_per_s'] = flowLoad.b_count
            a['Packet_per_s'] = flowLoad.p_count
            neticResponse(request,NTC_OK,a)
        elif (error == 1):
            neticResponse(request,NTC_ST_ERR_PMID_NOT_FOUND)
        elif (error == 2):
            a['Byte_per_s'] = flowLoad.b_count
            a['Packet_per_s'] = flowLoad.p_count
            neticResponse(request,NTC_ST_INFO_NO_STATS,a)
        else:
            neticResponse(request,NTC_ST_INFO_NO_STATS,a)

    def _get_all_mon_ids(self,request,arg):
        a = {}
        a['MonitorIDs'] = []
        outString = self.openStats.get_path_mon_ids()
        hexList = outString.split(':')
        for item in hexList:
            a['MonitorIDs'].append(item)
        neticResponse(request,NTC_OK,a)
        
    def _remove_flow_monitor(self, request, arg):
        a = {}
        monID = int(str(arg['{Mon_ID}']),16)
        ret = self.openStats.remove_monitor_flow(monID)
        
        if (ret == 0):
            self.monitorIDs.remove(monID)
            neticResponse(request,NTC_OK,responseCode = 204)
        else:
            neticResponse(request,NTC_OK,responseCode = 204)

    def getInterface(self):
        return str(snmp_stats_ws)


def getFactory():
    class Factory:
        def instance(self, ctxt):
            return snmp_stats_ws(ctxt)

    return Factory()




