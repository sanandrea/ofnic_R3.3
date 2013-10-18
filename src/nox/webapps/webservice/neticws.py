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

# Created 02/10/2012
# Author Andi
# Company University of Rome

from nox.webapps.webservice      import webservice
from nox.webapps.webservice.webservice import json_parse_message_body
from nox.webapps.webservice.webservice import NOT_DONE_YET
from nox.lib.packet.packet_utils      import ip_to_str
from nox.lib.core import *

import simplejson as json
import time
from platform import _node

NTC_OK					= 0

NTC_VP_ERR_HOST_NOT_FOUND 		= 1
NTC_VP_ERR_UNKNOWN_SWITCH 		= 2
NTC_VP_ERR_UNKNOWN_PORT 		= 3
NTC_VP_ERR_MISSING_M_FIELD 		= 4
NTC_VP_ERR_MISSING_LOC_INFO 		= 5
NTC_VP_ERR_MISSING_MAC_ADDRESS 		= 6
NTC_VP_ERR_BAD_MAC_ADDRESS 		= 7
NTC_VP_INFO_PATH_NOT_FOUND		= 8
NTC_VP_ERR_PATH_NOT_FOUND		= 9
NTC_VP_ERR_MISSING_PORT_INFO		= 10

NTC_ST_ERR_PORT_NOT_FOUND       	= 11
NTC_ST_ERR_PATH_NOT_FOUND       	= 12
NTC_ST_ERR_MISSING_M_FIELD              = 13
NTC_ST_ERR_BAD_ARGUMENT                 = 14
NTC_ST_ERR_PMID_NOT_FOUND               = 15
NTC_ST_INFO_NO_STATS                    = 16
NTC_ST_ERR_DPID_NOT_FOUND               = 17

errcode2str = {}
errcode2str[NTC_OK]			= "No error"
errcode2str[NTC_VP_ERR_HOST_NOT_FOUND] 	= "Host not found"
errcode2str[NTC_VP_ERR_UNKNOWN_SWITCH] 	= "Unknown switch"
errcode2str[NTC_VP_ERR_UNKNOWN_PORT]   	= "Unknown port"
errcode2str[NTC_VP_ERR_MISSING_M_FIELD]	= "Missing or bad mandatory field"
errcode2str[NTC_VP_ERR_MISSING_LOC_INFO]	= "Host IP is unknown in the network, \
provide complete Datapath and port info"
errcode2str[NTC_VP_ERR_MISSING_MAC_ADDRESS]     = "Missing MAC address in order \
to set up the ARP path"	
errcode2str[NTC_VP_ERR_BAD_MAC_ADDRESS]	="Bad MAC address"
errcode2str[NTC_VP_INFO_PATH_NOT_FOUND]   = "Could not setup the required path"
errcode2str[NTC_VP_ERR_PATH_NOT_FOUND]   = "Path doesn't exist"
errcode2str[NTC_VP_ERR_MISSING_PORT_INFO] = "Ip proto specified but port info is missing"

errcode2str[NTC_ST_ERR_PORT_NOT_FOUND]   = "Port is not under monitoring or no stats available"
errcode2str[NTC_ST_ERR_PATH_NOT_FOUND]   = "Path doesn't exist"
errcode2str[NTC_ST_ERR_MISSING_M_FIELD] = "Missing mandatory info of post"
errcode2str[NTC_ST_ERR_BAD_ARGUMENT] = "Bad argument type"
errcode2str[NTC_ST_ERR_PMID_NOT_FOUND] = "Bad path monitor ID"
errcode2str[NTC_ST_INFO_NO_STATS] = "Statistics not yet available"
errcode2str[NTC_ST_ERR_DPID_NOT_FOUND] = "Wrong Datapath ID, maybe this path does not pass through it"

def neticResponse(request, resultCode, otherInfo={},responseCode=200):
    """Return a response for a valid REST Request"""
    request.setResponseCode(responseCode, "OK")
    request.setHeader('Access-Control-Allow-Origin',"*")
    request.setHeader("Content-Type", "application/json")
    d = {'result':otherInfo}
    d["displayError"] = errcode2str[resultCode]
    d["resultCode"] = resultCode
    request.write(json.dumps(d, indent=4))
    request.finish()
    return

#
# Verifies that node is a valid extension alias is provided
#
class WSPathExistingAliasID(webservice.WSPathComponent):
    def __init__(self, apimgr):
        webservice.WSPathComponent.__init__(self)
        self._apimgr = apimgr
        
    def __str__(self):
        return "{alias}"

    def extract(self, node, data):
        if (node == None):
            return webservice.WSPathExtractResult(error="Missing end of requested URI")
        try:
            alias = str(node)
            for item in self._apimgr.aliases:
                if alias == item:
                    return webservice.WSPathExtractResult(node)

            e = "Alias '%s' is unknown" % node
            return webservice.WSPathExtractResult(error=e)
        except ValueError, e:        
            err = "invalid node name %s" % str(node)
            return webservice.WSPathExtractResult(error=err)
        return webservice.WSPathExtractResult(node)

class NeticApiMgr(Component):
    extensionUri = None
    aliases = ['Uniroma', 'alfa']
    extensions = {}
    extensions[aliases[0]] = {}
    extensions[aliases[0]]['name'] = "FIWARE Extension"
    extensions[aliases[0]]['namespace'] = """https://forge.fi-ware.eu/plugins/mediawiki/wiki/fiware/index.php/NetIC_RESTful_API_Specification"""
    extensions[aliases[0]]['alias'] = "Uniroma",
    extensions[aliases[0]]['updated'] = "2013-03-04T10:45:03+01:00",
    extensions[aliases[0]]['description'] = "Adds the capability to create Monitoring Tasks in the network"
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def __str__(self):
        return "netic_api_mgr"    

    def install(self):

        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request

        #self.rootpath = neticpath + (webservice.WSPathStaticString(self.networkIdString),)
        self.extensionUri = (webservice.WSPathStaticString("extension"),)
        
        reg(self._ext_info,"GET",self.extensionUri,"""Get all extensions list""")
        
        self.aliasUri = self.extensionUri + (WSPathExistingAliasID(self),)
        reg(self._alias_info,"GET",self.aliasUri,"""Get info about extension with alias""")

    def _ext_info(self, request, arg):
        request.write(json.dumps(self.extensions, indent=4))
        request.finish()

    def _alias_info(self,request, arg):
        alias = arg['{alias}']
        request.write(json.dumps(self.extensions[alias], indent=4))
        request.finish()

    def getInterface(self):
        return str(NeticApiMgr)

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return NeticApiMgr(ctxt)

    return Factory() 




