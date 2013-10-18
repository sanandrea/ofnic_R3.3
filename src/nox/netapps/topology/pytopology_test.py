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
# ----------------------------------------------------------------------
#
# This app just drops to the python interpreter.  Useful for debugging
#
from nox.lib.core import *
from nox.lib.netinet.netinet import datapathid
from nox.netapps.topology.pytopology import pytopology 


#Added by Andrea Simeoni for testing
from nox.netapps.routing import pyrouting
from nox.lib.netinet.netinet import create_ipaddr
from nox.lib.packet.packet_utils import  ipstr_to_int

from nox.webapps.webservice      import webservice
from nox.webapps.webservice.webservice import json_parse_message_body
from nox.webapps.webservice.webservice import NOT_DONE_YET 
from nox.netapps.routing import pyrouting
from nox.lib.core import *

import simplejson
import time

from nox.netapps.topology.pytopology import pytopology 
from nox.webapps.webservice      import webservice
from nox.lib.netinet.netinet import datapathid
from nox.webapps.webservice.webservice import json_parse_message_body
from nox.webapps.webservice.webservice import NOT_DONE_YET 
from nox.netapps.routing import pyrouting
from nox.lib.core import *

import simplejson
import time



class pytopology_test(Component):

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def timer(self):
        val1 = datapathid.from_host(1)
	val2 = datapathid.from_host(3)
        
        self.routing.netic_install_route(1,1,val1,val2,"4.0.0.10","6.0.0.10")
        #oute = pyrouting.Route()
        #route.id.src=val1
        #route.id.dst=val2
        #if self.routing.get_route(route):
        #    log.err('Found route')
        #    sip=create_ipaddr("10.0.0.1")
        #    dip=create_ipaddr("10.0.0.4")
        #    print str(sip)
        #    print str(dip)#

        #    flow={core.NW_SRC:"10.0.0.1",core.NW_DST:"10.0.0.4"}

        #    print str(flow)
        #    #self.routing.setup_route(flow, route, 1, 2, 5, [], True) 

        #   self.install_datapath_flow(1, flow, 5, 0,
        #                     {}, [], 
        #                     openflow.OFP_DEFAULT_PRIORITY,0,0)
        #else:
        #    log.err('errore del cazzo!!!')
        
        #print self.pytop.is_internal(val1, 0)
        #self.pytop.synchronize_node(val1)
      
	#print self.pytop.get_dpinfo(val2).active
	#print self.pytop.get_dpinfo(val2).dpid
	#li=self.pytop.synchronize_node(val1)
	#pinfo=self.pytop.synchronize_port("s4-eth1")
        #print len(ll)
	  
        #item1 = li[0] 
        #item2 = li[1]
	#item3 = li[2]
	#for item in li:
        #    print "----------"
        #    print item.name 
        #    print item.active
	#   print item.duplexity
        #   print "----------"
            
        #print pinfo.name 
        #print pinfo.active
	#print pinfo.duplexity
	#print pinfo.speed
	#print pinfo.medium

        #print item1.name 
        #print item1.active
	#print item1.duplexity
	#print item1.speed
	#print item1.medium
        #print item2.name 
        #print item2.active
	#print item2.duplexity
	#print item2.speed
	#print item2.medium
	
       # print item3.name 
        #print item3.active

	#for node in ll:
	 #   print node.active
	  #  print node.dpid
	
	#self.pytop.get_whole_topology()
        self.post_callback(10, self.timer)

    def install(self):
        self.routing = self.resolve(pyrouting.PyRouting)
        self.pytop = self.resolve(pytopology)
        #self.post_callback(10, self.timer)

        #self.pyrt=self.resolve(pyrouting.PyRouting)
        
        ws  = self.resolve(str(webservice.webservice))
        print str(ws)
        # v1=ws.get_version("1")
        #reg = v1.register_request

        neticpath= ( webservice.WSPathStaticString("netic"), )
        # /ws.v1/netic/provision
        provisionpath=neticpath+\
                (webservice.WSPathStaticString("provision"),)	
        #reg(self._netic_provision, "POST", provisionpath,
        #    """Installs a path in the network.""")




    def _netic_provision(self,request,arg):

        indp=datapathid.from_host(int(arg['first_node']))
        outdp=datapathid.from_host(int(arg['last_node']))
        ret= self.pyrt.netic_install_route(int(arg['first_port']),int(arg['last_port']),indp,outdp,str(arg['src_ip']),str(arg['dst_ip']))
        return ret
	

    def getInterface(self):
        return str(pytopology_test)


def getFactory():
    class Factory:
        def instance(self, ctxt):
            return pytopology_test(ctxt)

    return Factory()
