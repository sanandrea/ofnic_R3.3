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

import sys, dl
sys.setdlopenflags(sys.getdlopenflags() | dl.RTLD_GLOBAL)
from nox.netapps.routing import pyrouting
from nox.webapps.webservice      import webservice
from nox.webapps.webservice.neticws      import *
from nox.lib.core import Component
from nox.lib.netinet.netinet import datapathid
from nox.lib.netinet.netinet import create_eaddr
from nox.netapps.routing.pyrouting import Netic_path_info
from nox.netapps.discovery.discoveryws import *
from nox.lib.packet.packet_utils import octstr_to_array
import simplejson as json
from twisted.web.http import Request


class routingws(Component):
    """Web service for routing commands"""
    
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)      

    def install(self):

        self.pyrt=self.resolve(pyrouting.PyRouting)
        self.discoveryws = self.resolve(discoveryws)
        self.discovery = self.discoveryws.discovery
        
        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request


        rootpath = self.discoveryws.rootpath
        virtualpath = rootpath + (webservice.WSPathStaticString("virtualpath"),)
        
        # /ws.v1/netic/OF_UoR/virtualpaths
        reg(self._netic_show_paths, "GET", virtualpath,"""Get a list of currently installed paths""","""Routing Sub-Module""")

        # /ws.v1/netic/OF_UoR/virtualpath/create
        provisionpath=virtualpath + (webservice.WSPathStaticString("create"),)
        
        reg(self._netic_create_path, "POST", provisionpath,
            """Installs a path in the network.""")
        
        # /ws.v1/netic/OF_UoR/virtualpath/{id}
        pathIDpath = virtualpath + (WSPathExistingPathName(self.discoveryws),)
        reg(self._netic_remove_path,"DELETE",pathIDpath,
            """Remove a previously created path""")
        
        reg(self._netic_get_path_info,"GET",pathIDpath,
            """Retrieve path information""")
        
    def _netic_get_path_info(self,request,arg):
        a = {}
        nodesList = []
        pathID = int(str(arg['{path_ID}']),16)
        npd,found = self.pyrt.netic_get_path_info(pathID);
        
        if found == False:
            neticResponse(request,NTC_VP_ERR_PATH_NOT_FOUND)
            return

        nodesList.append(npd.rteId.src.as_host())
        linkList = npd.path.split(':')
        for item in linkList:
            link = item.split(',')
            nodesList.append(int(link[0]))
            
        a['Nodes'] = nodesList
        a['Time Remaining'] = npd.timeToDie;
        
        a['Source IP'] = npd.npi.nw_src;
        a['Dest IP'] = npd.npi.nw_dst;
        a['Bandwidth'] = npd.npi.bandwidth;
        neticResponse(request,NTC_OK,a)
    
    def _netic_show_paths(self,request,arg):
        a = {}
        a['Paths'] = []
        outString = self.pyrt.netic_get_path_list()
        hexList = outString.split(':')
        for item in hexList:
            a['Paths'].append(item)
        
        neticResponse(request,NTC_OK,a)
    
    def get_current_path_list(self):
        a = {}
        a['Paths'] = []
        outString = self.pyrt.netic_get_path_list()
        hexList = outString.split(':')
        for item in hexList:
            a['Paths'].append(item)
        return a
        
    def netic_is_valid_path(self, path):
        outString = self.pyrt.netic_get_path_list()
        hexList = outString.split(':')
        for item in hexList:
            if (path == item):
                return True
        return False
        
    def _netic_remove_path(self,request, arg):
        path = int(str(arg['{path_ID}']),16)
        res = self.pyrt.netic_remove_path(path)
        if res == 0:
            self.discoveryws.remove_existing_path(str(arg['{path_ID}']))
            neticResponse(request,NTC_OK,responseCode=204)
        else:
            errorCode = {'errorCode':NTC_VP_ERR_PATH_NOT_FOUND}
            webservice.badRequest(request,errcode2str[NTC_VP_ERR_PATH_NOT_FOUND],errorCode)
            return NOT_DONE_YET
        return 

    def _netic_create_path(self,request,arg):
  
        #content = json_parse_message_body(arg)

        content = request.content.read()

        errorCode = {}
        
        try:
            nw_src = str(request.args['nw_src'][0])
            nw_dst = str(request.args['nw_dst'][0])
        
            duration = int(request.args['duration'][0])
            bandwidth = int(request.args['bandwidth'][0])
            set_arp = int(request.args['set_arp'][0])
            bidirectional = int (request.args['bidirectional'][0])
        except:
            errorCode['errorCode'] = NTC_VP_ERR_MISSING_M_FIELD
            webservice.badRequest(request,errcode2str[NTC_VP_ERR_MISSING_M_FIELD],errorCode)
            return NOT_DONE_YET
        #bidirectional and set_arp has to be either 0 or 1:
        if (((set_arp != 0) and (set_arp != 1)) or ((bidirectional != 0) and (bidirectional != 1))):
            errorCode['errorCode'] = NTC_VP_ERR_MISSING_M_FIELD
            webservice.badRequest(request,errcode2str[NTC_VP_ERR_MISSING_M_FIELD],errorCode)
            return NOT_DONE_YET
        
        paramsMissing = 0;
        try:
            dp_src = datapathid.from_host(int(request.args['dp_src'][0]))                
            dp_dst = datapathid.from_host(int(request.args['dp_dst'][0]))
            first_port=int(request.args['first_port'][0])
            last_port=int(request.args['last_port'][0])
        except:
            dp_src = -1
            dp_dst = -1
            first_port = -1
            last_port = -1
            paramsMissing = 1;
        
        info_src = self.discovery.find_host_by_ipstr(nw_src)
        info_dst = self.discovery.find_host_by_ipstr(nw_dst)
        
        if paramsMissing == 1:
            #Try to find in which port and switch the path will begin and
            #terminate
            if (info_src == None or info_dst == None):
                errorCode['errorCode'] = NTC_VP_ERR_MISSING_LOC_INFO
                webservice.badRequest(request,errcode2str[NTC_VP_ERR_MISSING_LOC_INFO],errorCode)
                return NOT_DONE_YET
            else:
                dp_src = datapathid.from_host(info_src['dp'])
                first_port = info_src['port']
                dp_dst = datapathid.from_host(info_dst['dp'])
                last_port = info_dst['port']
          
        else:
        # the dp source and dest and also first and last ports 
        # are those specified by the request, lets check them
            if (not self.discoveryws.discovery.is_valid_dpid(dp_src.as_host()) or 
                   not self.discoveryws.discovery.is_valid_dpid(dp_dst.as_host())):
                errorCode['errorCode'] = NTC_VP_ERR_UNKNOWN_SWITCH
                webservice.badRequest(request,errcode2str[NTC_VP_ERR_UNKNOWN_SWITCH],errorCode)
                return NOT_DONE_YET

            if (not self.discoveryws.discovery.is_valid_port_in_dpid(dp_src.as_host(),first_port) or 
                   not self.discoveryws.discovery.is_valid_port_in_dpid(dp_dst.as_host(),last_port)):
                errorCode['errorCode'] = NTC_VP_ERR_UNKNOWN_PORT
                webservice.badRequest(request,errcode2str[NTC_VP_ERR_UNKNOWN_PORT],errorCode)
                return NOT_DONE_YET

        # At this point we have the mandatory params of the flow and 
        # location of the source and destination point of the path 

        paramsMissing = 0
        keyError = 0
        with_arp = False
        if set_arp == 1:
            with_arp = True
            try:
                dl_src = create_eaddr(str(request.args['dl_src'][0]))
                dl_dst = create_eaddr(str(request.args['dl_dst'][0]))
            except KeyError:
                dl_src = 0
                dl_dst = 0
                keyError = 1
                paramsMissing = 1
            except:
                print "other error"
            
            if ((dl_src == None) | (dl_dst == None)):
                paramsMissing = 1
                            
            if paramsMissing == 1:
                if info_src != None:
                    dl_src = info_src['dl_addr']
                if info_dst != None:
                    dl_dst = info_dst['dl_addr']
        
            if ((dl_src == None) | (dl_dst == None)):
                if keyError == 1:
                    errorCode['errorCode'] = NTC_VP_ERR_MISSING_MAC_ADDRESS
                    webservice.badRequest(request,errcode2str[NTC_VP_ERR_MISSING_MAC_ADDRESS],errorCode)
                else:
                    errorCode['errorCode'] = NTC_VP_ERR_BAD_MAC_ADDRESS
                    webservice.badRequest(request,errcode2str[NTC_VP_ERR_MISSING_MAC_ADDRESS],errorCode)                    
                return NOT_DONE_YET
                    
        #At this point even arp info is ready
        
        tp_src = 0
        tp_dst = 0
        ip_proto = 255
        granularity = True
        
        try:
            ip_proto = int(request.args['ip_proto'][0])
        except:
            granularity = False
            
        if (granularity == True):
            try:
                tp_src = int(request.args['tp_src'][0])
            except:
                tp_src = 0
            try:
                tp_dst = int(request.args['tp_dst'][0])
            except:
                tp_dst = 0
        else:
            ip_proto = 255
                
        npi = Netic_path_info()
        npi.nw_src = nw_src;
        npi.nw_dst = nw_dst;
        npi.duration = duration;
        npi.bandwidth = bandwidth;
        if set_arp == True:
            npi.set_arp = True
        else:
            npi.set_arp = False
        
        if bidirectional == True:
            npi.bidirect = True
        else:
            npi.bidirect = False
            
        npi.dp_src = dp_src
        npi.dp_dst = dp_dst
        npi.first_port = first_port
        npi.last_port = last_port
        
        if (set_arp == True):
            npi.dl_src = dl_src
            npi.dl_dst = dl_dst
        
        npi.ip_proto = ip_proto        
        npi.tp_src = tp_src
        npi.tp_dst = tp_dst

        res,error = self.pyrt.netic_create_route(npi)
                
        if (error == 0):
            a = {}
            a['directPath'] = res.directPath
            self.discoveryws.add_created_path(res.directPath)
            
            if (res.reversPath != None):
                self.discoveryws.add_created_path(res.reversPath)
                a['reversPath'] = res.reversPath
            
            neticResponse(request,NTC_OK,a)
        else:
            neticResponse(request,NTC_VP_INFO_PATH_NOT_FOUND)
    

    def getInterface(self):
        return str(routingws)


def getFactory():
    class Factory:
        def instance(self, ctxt):
            return routingws(ctxt)

    return Factory()




