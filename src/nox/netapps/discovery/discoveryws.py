# Copyright 2008 (C) Nicira, Inc.
# 
# This file is part of NOX.
# 
# NOX is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# NOX is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with NOX.  If not, see <http://www.gnu.org/licenses/>.

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
# =====================================================================


from nox.netapps.topology.pytopology import pytopology 
from nox.webapps.webservice      import webservice
from nox.webapps.webservice.neticws import *
from nox.lib.netinet.netinet import datapathid
from nox.webapps.webservice.webservice import json_parse_message_body
from nox.webapps.webservice.webservice import NOT_DONE_YET 
from nox.netapps.discovery import discovery
from nox.lib.core import *
from nox.lib.packet.packet_utils      import ip_to_str

import simplejson as json
import time
from platform import _node

#
# Verifies that node is a valid datapath name 
#
class WSPathExistingNodeName(webservice.WSPathComponent):
    def __init__(self, pytopology):
        webservice.WSPathComponent.__init__(self)
        self._pytopology = pytopology

    def __str__(self):
        return "<node_ID>"

    def extract(self, node, data):
        if node == None:
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            dpid=datapathid.from_host(int(node))
            if  not (self._pytopology.is_valid_dp(dpid)): 
                e = "node '%s' is unknown" % node
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid node name %s" % str(node)
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(node)

#
# Verifies that node is a valid datapath name but based on discovery.py component
#
class WSPathExistingNodeName_V2(webservice.WSPathComponent):
    def __init__(self, discovery):
        webservice.WSPathComponent.__init__(self)
        self._discovery = discovery
        
    def __str__(self):
        return "{node_ID}"

    def extract(self, node, data):
        if (node == None):
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            dpid=int(node)
            if  not (self._discovery.is_valid_dpid(dpid)): 
                e = "node '%s' is unknown" % node
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid node name %s" % str(node)
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(node)


# 
# Verifies that port is a valid port name 
#
class WSPathExistingPortName(webservice.WSPathComponent):
    def __init__(self, pytopology):
        webservice.WSPathComponent.__init__(self)
        self._pytopology = pytopology

    def __str__(self):
        return "<port_ID>"

    def extract(self, port, data):
        if port == None:
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            dpid=datapathid.from_host(int(data['<node_ID>']))
            pinfo=self._pytopology.is_valid_port(dpid, port)
            if (pinfo == False): 
                e = "port '%s' is unknown" % port
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid port name %s" % port
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(port)

#
# Verifies that port is a valid port name, but based on discovery.py component
#
class WSPathExistingPortName_V2(webservice.WSPathComponent):
    def __init__(self, discovery):
        webservice.WSPathComponent.__init__(self)
        self._discovery = discovery
    
    def __str__(self):
        return "{port_ID}"
    
    def extract(self, port, data):
        if port == None:
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            dpid = int(data['{node_ID}'])
            pinfo=self._discovery.is_valid_port_in_dpid(dpid, port)
            if (pinfo == False): 
                e = "port '%s' is unknown" % port
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid port name %s" % port
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(port)

#
# Attention verifies that this port of this dpid has any connected switch not the ID of the link
#
class WSPathExistingLinkName(webservice.WSPathComponent):
    def __init__(self, discovery):
        webservice.WSPathComponent.__init__(self)
        self._discovery = discovery
    
    def __str__(self):
        return "{link_ID}"
    
    def extract(self, link, data):
        if link == None:
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            dpid = int(data['{node_ID}'])
            portID = int(data['{port_ID}'])

            linkInfo=self._discovery.is_switch_only_port(dpid, portID)
            peerInfo=self._discovery.is_host_only_port(dpid,portID)
            if linkInfo == False and peerInfo == False: 
                e = 'port {0} in node {1} is not connected to any other switch or host !!!  '.format(portID, dpid)
                return webservice.WSPathExtractResult(error=e)
            if not self._discovery.is_valid_link_in_port(dpid,portID,link):
                e = 'Wrong link index in port {0} of node {1}'.format(portID,dpid)
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = e
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(link)
    
class WSPathExistingPathName(webservice.WSPathComponent):
    def __init__(self, discoveryws):
        webservice.WSPathComponent.__init__(self)
        self._discoveryws = discoveryws

    def __str__(self):
        return "{path_ID}"

    def extract(self, node, data):
        if node == None:
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            path = str(node)
            if  not (self._discoveryws.netic_is_valid_path(path)): 
                e = "Path '%s' is unknown" % str(path)
                return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid path_ID format %s" % str(node)
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(node)

class discoveryws(Component):
    """Web service for accessing topology information"""
    networkIdString = "OFNIC"
    pathlist = []
    
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)     

    def install(self):

        self.pytop = self.resolve(pytopology)
        
        self.discovery = self.resolve(discovery.discovery)

        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request


        #neticpath = ( webservice.WSPathStaticString("netic"), )
        #self.rootpath = neticpath + (webservice.WSPathStaticString(self.networkIdString),)
        #rootpath = https://localhost/netic.v1/OFNIC
        self.rootpath = (webservice.WSPathStaticString(self.networkIdString),)
        
        reg(self._root_information,"GET",self.rootpath,"""Get info about this network and 
            its capabilities""","""Openflow Network Module""")
        
        
        
        synchpath = self.rootpath + (webservice.WSPathStaticString("synchronize"),)
        #rootpath = https://localhost/netic.v1/OFNIC/synchronize

        # /ws.v1/netic/synchronize
        networkId = synchpath
        reg(self._synchronize_root, "GET", networkId,"""Get network ID ""","""Synchronization Sub-Module""")

        # /ws.v1/netic/synchronize/network
        networkpath=synchpath + (webservice.WSPathStaticString("network"),)
        reg(self._synchronize_network_V2, "GET", networkpath, """Get list of nodes and paths.""")
        

        # /ws.v1/netic/synchronize/network/all
        fullpath=networkpath + (webservice.WSPathStaticString("all"),)
        reg(self._synchronize_all, "GET", fullpath, """Get list all network connected node pairs.""")

        # /ws.v1/netic/synchronize/network/node/<node name>
        nodepath=networkpath + (webservice.WSPathStaticString("node"),)
        
        nodenamepath=nodepath + (WSPathExistingNodeName_V2(self.discovery),)

        reg(self._synchronize_node_V2, "GET", nodenamepath,
            """Get list of ports for a node.""")


        # /ws.v1/netic/synchronize/network/node/<node name>/port/<port name>
        portpath=nodenamepath + (webservice.WSPathStaticString("port"),)

        portnamepath=portpath + (WSPathExistingPortName_V2(self.discovery),)

        reg(self._synchronize_port_V2, "GET", portnamepath,
            """Get port info""")

        # /ws.v1/netic/synchronize/network/node/<node name>/port/<port name>/link/<link name>
        # where link name name is in the form node/<node name>/port/<port name>
        linkPath = portnamepath + (webservice.WSPathStaticString("link"),)
        linkNamePath = linkPath + (WSPathExistingLinkName(self.discovery),)
        reg(self._synchronize_link, "GET", linkNamePath, """Get Link Peer""")
    


    #web services added by UoR
    def add_created_path(self, path):
        self.pathlist.append(path)

    def remove_existing_path(self, path):
        self.pathlist.remove(str(path))
    
    def netic_is_valid_path(self, path):
        if path in self.pathlist:
            return True
        else:
            return False
    
    def _root_information(self,request,arg):
        a = {}
        commandList = ["synchronize","statistics","virtualpaths"]
        a['modules'] = commandList
        neticResponse(request,NTC_OK,a)


    def _synchronize_all(self, request, arg):
        a = {}
        l = self.discovery.get_network_pairs()
        k = self.discovery.get_connected_hosts()
        a['pairs'] = l
        a['hosts'] = k
        neticResponse(request, NTC_OK, a)

 

    def _synchronize_network(self, request, arg):
        
        nodelist=self.pytop.synchronize_network()
        result=""
        for item in nodelist:
            if item.active:
                stat="active"
            else:
                stat="inactive"
            result=result+"OFNODE_#"+str(item.dpid)+", status: "+stat+'\n'

        return result
    
    def _synchronize_network_V2(self, request, arg):
        a = {}
        nodelist = []
        for index,node in enumerate(self.discovery.dps):
            nodelist.append(node)
        a['Nodes'] = nodelist
        a['Paths'] = self.pathlist
        neticResponse(request,NTC_OK,a)   


    def _synchronize_root(self, request, arg):
        a = {}
        a['Network Name'] = self.networkIdString
        neticResponse(request,NTC_OK,a)

    def _synchronize_node(self, request, arg):
        
        dp=datapathid.from_host(int(arg['<node_ID>']))
        portlist=self.pytop.synchronize_node(dp)
        result=""
        for item in portlist:
            
            #result=result+"PORT_#"+str(item.name)+", status: "+stat+", speed: "+str(item.speed)+"Mbps, medium: "+str(item.medium)+", duplexity: "+str(item.duplexity)+'\n'
            result=result+"port: "+str(item.name)+", status: "+str(item.state)+'\n'
        print result
        return result
    
    def _synchronize_node_V2(self, request, arg):
        dpid = int(arg['{node_ID}'])
        ret = self.discovery.synchronize_node(dpid)
        a = {'Num_Tables': ret['n_tables']}
        a['Num_Buffers'] = ret['n_bufs']
        a['Actions'] = ret['actions']
        
        portNameList = []
        portIndexList = []
    
        for index, pp in enumerate(ret['ports']):
            portNameList.append(pp['name'])
            portIndexList.append(pp['port_no'])
            #print str(index) + pp['name']
        a['Port_Names'] = portNameList
        a['Port_Index'] = portIndexList
        neticResponse(request,NTC_OK,a)


    def _synchronize_port(self, request, arg):
        
        port=arg['{port_ID}']
        dp=datapathid.from_host(int(arg['{node_ID}']))
        pinfo=self.pytop.synchronize_port(dp, port)
        result=""
        result=result+"port#"+str(pinfo.name)+", status: "+str(pinfo.state)+", speed: "+str(pinfo.speed)+"Mbps, medium: "+str(pinfo.medium)+", duplexity: "+str(pinfo.duplexity)+'\n'
        
        return result
        
    def _synchronize_port_V2(self, request, arg):
        
        port=int(arg['{port_ID}'])
        dp=int(arg['{node_ID}'])
        ret = self.discovery.synchronize_port_of_node(dp, port)

        a = {'Active': ret['enabled']}
        a['Speed'] = ret['speed']
        a['State'] = ret['state']
        a['Config']= ret['config']
        
        if self.discovery.is_switch_only_port(dp,port):
            ret = self.discovery.connected_peers(dp, port)
            a['links'] = range(len(ret))
        elif self.discovery.is_host_only_port(dp,port):
            ret = self.discovery.connected_hosts(dp, port)
            a['links'] = range(len(ret))
        else:
            a['links'] = 'None'
        
        neticResponse(request,NTC_OK,a)

    def _synchronize_link(self, request, arg):
        port=int(arg['{port_ID}'])
        dp=int(arg['{node_ID}'])
        link=int(arg['{link_ID}'])
        a= {}
        if self.discovery.is_switch_only_port(dp,port):
            ret = self.discovery.connected_peers(dp,port)
            peer = ret[link]
            a['node'] = peer[0]
            a['port'] = peer[1]

        elif self.discovery.is_host_only_port(dp,port):
            ret = self.discovery.connected_hosts(dp,port)
            peer = ret[link]
            a['Name'] = peer[2]
            a['IP Addr'] = ip_to_str(peer[1])
            #a['Mac Addr'] = peer[0]
            #dladdr is not json serializable, it has to be converted is some other type

        neticResponse(request,NTC_OK,a)



    def getInterface(self):
        return str(discoveryws)



def getFactory():
    class Factory:
        def instance(self, ctxt):
            return discoveryws(ctxt)

    return Factory()
