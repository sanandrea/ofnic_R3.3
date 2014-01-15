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

# @Author Andi Palo 
# @Date created 24/07/2013


import logging
from nox.lib.core     import *
from nox.webapps.webservice      import webservice
from nox.netapps.discovery.discoveryws import *
from nox.webapps.webservice.neticws      import *
from nox.coreapps.db_manager.mongodb_manager import *
import simplejson as json

lg = logging.getLogger('virtnet')

class VirtualNetworkWS(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def install(self):
        lg.debug("Hi there virtual is loaded")
        self.discoveryws = self.resolve(discoveryws)
        self.storage = self.resolve(NoSQLManager)
        ws  = self.resolve(str(webservice.webservice))
        v1  = ws.get_version("1")
        reg = v1.register_request
        
        rootpath = self.discoveryws.rootpath
        pushpath = rootpath + (webservice.WSPathStaticString("virtualnetwork"),)
        reg(self._flow_entry_root, "GET", 
            pushpath,"""Get info""",
            """Virtual netwrok Sub-Module""")

    def getDatabase(self):
        return self.db

    def _flow_entry_root(self, request, arg):
        a = {}
        a['Info'] = "Openflow Network Flow Configuration"
        neticResponse(request,NTC_OK,a)

    def getInterface(self):
        return str(VirtualNetworkWS)
        
        

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return VirtualNetworkWS(ctxt)

    return Factory()
