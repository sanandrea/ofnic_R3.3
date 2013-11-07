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

# @author Andi
# @date 24/04/2013

from nox.lib.core import *
from nox.lib.packet.packet_utils      import ip_to_str

from nox.coreapps.pyrt.pycomponent import CONTINUE
from nox.coreapps.messenger.pyjsonmsgevent import JSONMsg_event
from nox.netapps.discovery import discovery #the topology is described in adjacency_list populated from discovery

import simplejson as json
from array import array
import logging
               
lg = logging.getLogger('snmp-handler')
hdlr = logging.FileHandler('OFNIC.log')
formatter = logging.Formatter('%(asctime)s|%(name)s|%(levelname)s|%(message)s')
hdlr.setFormatter(formatter)
lg.addHandler(hdlr) 
lg.setLevel(logging.WARNING)
        
class snmpMega(Component):
    """
    snmp messages MUST contain the "type": "snmp"
    
    "command" field should be one of the following:
        topo_req
        new_stat
        ...    
    
    """
    dpToNip={}
    switchesIP = array('i')
    totalStats ={}
    ipToDatapathid={}
    portidToPortname={}
    GRAPH_BUILD_PERIOD = 3
    
    #SNMP_WALKER_PATH = "nox/netapps/netic/snmp-walker.py"

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def __str__(self):
        return "snmp_handler"

    def configure(self, configuration):
        JSONMsg_event.register_event_converter(self.ctxt)

    def install(self):
        #need to resolve discovery where is the described the topology
        self.discovery = self.resolve(discovery.discovery)

        # Register for json messages from the gui
        self.register_handler( JSONMsg_event.static_get_name(), \
                         lambda event: self.handle_jsonmsg_event(event))

        self.register_for_datapath_join ( lambda dp,stats : \
                         snmpMega.dp_join(self, dp, stats) )

        self.register_for_datapath_leave ( lambda dp : \
                         snmpMega.dp_leave(self, dp) )
        #os.system("python " + self.SNMP_WALKER_PATH + " &")
        self.buildNetGraph()
        
    def getInterface(self):
        return str(snmpMega)


    def dp_leave(self, dp):   
        lg.info("*DP LEAVING FROM SNMP_STATS* "+str(dp))
        #leavedIpHost=self.ctxt.get_switch_ip(dp)
        for allhost in self.switchesIP:
            lg.info("Switch: "+str(allhost))
            #print "io voglio rimuovere: "+str(leavedIpHost)
                        
        ipToRemove = self.dpToNip[dp]
        if ipToRemove in self.switchesIP:
            self.switchesIP.remove(ipToRemove)
            lg.info("Rimosso switch "+str(ipToRemove))
                
        #if ip_to_str_reversed(leavedIpHost) in snmp_stats.portidToPortname:
        #        snmp_stats.portidToPortname.remove(ip_to_str_reversed(leavedIpHost))
        
    def dp_join(self, dp, stats):
        lg.info("*New switch from SNMP Handler* "+str(dp))
                
        joinedIpHost=self.ctxt.get_switch_ip(dp)
        self.dpToNip[dp]=joinedIpHost #needed to remove if there is a leave
        self.ipToDatapathid[ip_to_str_reversed(joinedIpHost)]=dp;#add the datapath id related to the host ip
        prtidprtnameMapping={}
        for port in stats[PORTS]:
            prtidprtnameMapping[port[PORT_NO]]=port['name']        
            #onw of this two instructions is redundant and will be deleted 
            #lg.info("Speed: "+str(port))
            prtidprtnameMapping[port['name']]=port[PORT_NO] #when i figure out what is more efficient TODO
        self.portidToPortname[ip_to_str_reversed(joinedIpHost)]=prtidprtnameMapping 
        # in portidToPortname we have the mapping of each port id with his name and viceversa for each host ip
        lg.info("Joined Switch: "+str(joinedIpHost))
        self.switchesIP.append(joinedIpHost)



    def handle_stats_from_snmp_server(self, msg):
        ipStr = ip_to_str_reversed(msg['dpid'])

        newStat = msg['stat']
        #print msg['stat']
        if not ipStr in self.totalStats:
            self.totalStats[ipStr] = newStat
            for iface in self.totalStats[ipStr]:
                #print self.totalStats[ipStr][iface]['ifName']
                self.totalStats[ipStr][iface]['txavg'] = 0
                self.totalStats[ipStr][iface]['rxavg'] = 0
                #print self.totalStats[ipStr][iface]['ts_openflow']
                #print self.totalStats[ipStr][iface]['ifName']
        else:
            for iface in self.totalStats[ipStr]:
                ifaceStat = self.totalStats[ipStr][iface]
                oldTS = long(ifaceStat['ts_openflow'])
                newTS = long(newStat[iface]['ts_openflow'])
                
                if (((newTS - oldTS)/1000000) == 0):
                    print "Why just why?"
                    continue
                
                #print "ts ", newTS, oldTS
                #print "counters t", newStat[iface]['txbytes'], ifaceStat['txbytes']
                #print "counters r", newStat[iface]['txbytes'], ifaceStat['txbytes']

                ifaceStat['txavg'] = ((newStat[iface]['txbytes'] - ifaceStat['txbytes'])/
                                      ((newTS - oldTS)/1000000))
                ifaceStat['rxavg'] = ((newStat[iface]['rxbytes'] - ifaceStat['rxbytes'])/
                                      ((newTS - oldTS)/1000000))

                #print "avg ", ifaceStat['txavg'], ifaceStat['rxavg']
                ifaceStat['ts_openflow'] = newStat[iface]['ts_openflow']
                ifaceStat['txbytes'] = newStat[iface]['txbytes']
                ifaceStat['rxbytes'] = newStat[iface]['rxbytes']
            #print self.totalStats[ipStr]
        return


    def buildNetGraph(self):
        self.netGraph={} #reset the current graph

        for host in self.switchesIP: #for all the host that i monitor
            ipString= ip_to_str_reversed(host)
            datapathID= self.ipToDatapathid[ipString] #take the datapathID for that host
            adjacencyTemp ={} #create the adjacency host list
            #lg.info(ipString)

            #the link information about the host are in self.discovery.adjacency_list
            for dp1, port1, dp2, port2 in self.discovery.adjacency_list:
                if dp1==datapathID:#if the dp1 is the host where i'm currently on
                    #weight = totalStats[]
                    portName= self.portidToPortname[ipString][port1]

                    if ipString not in self.totalStats:
                        continue

                    statsForHost= self.totalStats[ipString]
                    keys = statsForHost.keys()                    

                    for stat in keys:
                        #print portName
                        #print statsForHost[stat]["ifName"]
                        if statsForHost[stat]["ifName"] == portName:
                            #lg.info("found Stat: " +
                                   #str(statsForHost[stat]["rxavg"]) +
                                   #" bytes/s for interface: " + portName)
                        #adjacencyTemp[dp2]=statsForHost[stat]["rxavg"] #set the weight for that link
                                                        
                            #set the weight for that link (max bandwidth - busy bandwidth) TODO
                            adjacencyTemp[dp2] = (10000000 - 8 * statsForHost[stat]["rxavg"] -
                                                  8 * statsForHost[stat]["txavg"])

            self.netGraph[datapathID] = adjacencyTemp #update the information for that datapathID

        #lg.info(self.netGraph)
        #lg.info("END generation of graph for Dijkstra's algorithm")
        self.post_callback(self.GRAPH_BUILD_PERIOD, lambda : snmpMega.buildNetGraph(self))

    def handle_jsonmsg_event(self, e):
        ''' Handle incoming json messenges '''
        msg = json.loads(e.jsonstring)
        
        if msg["type"] != "snmp" :
            return CONTINUE
            
        if not "command" in msg:
            lg.debug( "Received message with no command field" )
            return CONTINUE
        
        if msg['command'] == "topo_req":
            #lg.info( "Received message requesting topology" )
            e.reply(json.dumps(self.dpToNip))

        if msg['command'] == "new_stat":
            self.handle_stats_from_snmp_server(msg)

        return CONTINUE

    def get_link_load(self, dp, portID):
        #lg.info("Returning stats about port and node\n\n")
        dpid = dp.as_host()
        pyload = PyLoad()

        if dpid in self.dpToNip:
            switchIP = self.dpToNip[dpid]
            ipString= ip_to_str_reversed(switchIP)

            portName= self.portidToPortname[ipString][portID]
            if ipString not in self.totalStats:
                return (pyload, False)

            statsForHost= self.totalStats[ipString]
            keys = statsForHost.keys()                    

            for stat in keys:
                #print portName
                #print statsForHost[stat]["ifName"]
                if statsForHost[stat]["ifName"] == portName:
                    pyload.txLoad = statsForHost[stat]["txavg"]
                    pyload.rxLoad = statsForHost[stat]["rxavg"]
                    return (pyload,True)

        return (pyload,False)
        
    def get_flow_load(self, pathID):
        return True

    def create_new_flow_monitor(self, pathID, dpid, duration, frequency):
        return True
    
    def remove_monitor_flow(self, monitorID):
        return True

    def get_path_mon_ids(self):
        return True

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return snmpMega(ctxt)

    return Factory()

class PyLoad:
    pass

def ip_to_str_reversed( ipToConvert ):
    ipSplitted = ip_to_str(ipToConvert).split(".")
    ip =  ipSplitted[3]+"."+  ipSplitted[2]+"."+ipSplitted[1]+"."+ipSplitted[0]
    return ip        

