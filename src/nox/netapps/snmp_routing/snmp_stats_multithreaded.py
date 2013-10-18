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

from nox.lib.core import *
from nox.lib.packet.packet_utils      import ip_to_str
from array import array

#snmp stuff
from pysnmp.entity.rfc3413.oneliner import cmdgen
from pysnmp.smi import builder

from nox.netapps.discovery import discovery #the topology is described in adjacency_list populated from discovery

from copy import deepcopy #needed to deep copy dictonary of dictionary

import socket, struct

#tentativi
from multiprocessing import Process
import time, threading


#100 mibt = 10000000
lg = logging.getLogger('snmp-stats')
def hello(name):
    print "ole"
class get_stats(threading.Thread):
    def run(self):
        print "inside thread"
    
class snmp_stats(Component):
    dpToNip={}
    switchesIP = array('i')
    totalStats ={}
    ipToDatapathid={}
    portidToPortname={}
    adjacency_hosts={}
    netGraph={}
    SNMP_CHECK_PERIOD = 10
        
    
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
        
        #self.adjacency_list = discovery.adjacency_list
    
    def install(self):
        # register for event dp_join
        self.discovery = self.resolve(discovery.discovery) #need to resolve discovery where is the described the topology
        self.register_for_datapath_join ( lambda dp,stats : snmp_stats.dp_join(self, dp, stats) )
        self.register_for_datapath_leave ( lambda dp : snmp_stats.dp_leave(self, dp) )
        self.start_poll_snmp_thread()
        pass
    
    def getInterface(self):
        return str(snmp_stats)
        
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
        lg.info("*NEW SWITCH FROM SNMP_STATS* "+str(dp))
                
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
                                                
        #print ip_to_str(self.ctxt.get_switch_ip(dp))
        #for port in stats[PORTS]:
        #        print port
                    
        
    # ---------------------------------------------------------------------- 
    # Start snmp timer which gather statistics from hosts every
    # SNMP_CHECK_PERIOD. self._target(*self._args, **self._kwargs)
    # ---------------------------------------------------------------------- 
    def start_poll_snmp_thread(self):
        lg.info("Starting gathering statistics... ")
        if len(threading.enumerate()) >= len(self.switchesIP):
            self.post_callback(self.SNMP_CHECK_PERIOD, lambda : snmp_stats.start_poll_snmp_thread(self))
            return

        for host in self.switchesIP:
            t = threading.Thread(target=self.poll_switch, args = (host,))
            t.start()
            print "Ended gathering for node ", ip_to_str(host)
        lg.info("Ended gathering statistics...")

        self.buildNetGraph()
        #reschedule the statistics gatheringTypeError: poll_switch() argument after * must be a sequence, not int  
        self.post_callback(self.SNMP_CHECK_PERIOD, lambda : snmp_stats.start_poll_snmp_thread(self))  


    def poll_switch(self,host):
        lg.info("IP: "+ip_to_str(host))
                   
        cmdGen = cmdgen.CommandGenerator()
        lg.info("IP 2: "+ip_to_str(host))       
        mibBuilder = cmdGen.snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder
        #print str(mibBuilder.getMibSources())

        mibSources = mibBuilder.getMibSources() + (
                builder.DirMibSource('/hosthome/OF/controller_utils/pymib'),
                )

        mibBuilder.setMibSources(*mibSources)
        mibBuilder.loadModules('ETHTOOL-MIB',)
        #print builder.MibBuilder().getMibPath()

        lg.info("Probing IP: "+ip_to_str_reversed(host))

        errorIndication, errorStatus, errorIndex, varBindTable = cmdGen.nextCmd(
                    cmdgen.CommunityData('public'),
                    cmdgen.UdpTransportTarget((ip_to_str_reversed(host), 161)),
                    #cmdgen.MibVariable('SNMPv2-MIB', 'sysDescr', 0),
                    (1,3,6,1,4,1,39178,100,1,1),
                    #(1,3,6,1,2,1,31,1,1,1,1),
                    lookupNames=True, lookupValues=True,
                )

        # Check for errors and print out results
        if errorIndication:
            print(errorIndication)
            return
        if errorStatus:
            print(errorStatus)
            return

        oldId="-1"
        switchStats ={}
        oldStats ={}
        if self.totalStats.has_key(ip_to_str_reversed(host)) :
            oldStats = deepcopy(self.totalStats[ip_to_str_reversed(host)])

        self.totalStats[ip_to_str_reversed(host)]=switchStats
        for varBindTableRow in varBindTable:
            for name, val in varBindTableRow:
                print('%s = %s' % (name.prettyPrint(), val.prettyPrint()))        
                statPrettified = name.prettyPrint().replace("ETHTOOL-MIB::ethtoolStat.\"","")
                #this is a progressive number starting from 1 given by the snmpwalk on the oid
                idStat = statPrettified[:statPrettified.find(".")].replace("\"","")
                statPrettified = name.prettyPrint().replace("ETHTOOL-MIB::ethtoolStat.\"" + idStat + 
                                                            "\".\"","").replace("\"","")
                #here there will be the name of the stat
                print (statPrettified)
                                        
                if oldId != idStat:         
                    #if i received stat for a new interface i have to initialize the dictionary
                    switchStats[idStat]= {} 
                    oldId = idStat                                                     

                if (statPrettified != "rxbytes" and statPrettified != "ts_milliseconds" and
                    statPrettified != "ts_openflow" and statPrettified != "txbytes" and
                    statPrettified != "ts_seconds"): #if is the ifName

                    switchStats[idStat]["ifName"]= statPrettified #store the name of the iface
                else:
                    #store the stat if it's not the ifname
                    switchStats[idStat][statPrettified]= long(val.prettyPrint()) 

                if statPrettified == "txbytes":
                    if (switchStats.has_key(idStat) and 
                        oldStats.has_key(idStat) and
                        oldStats[idStat].has_key("ts_openflow") and 
                        ((switchStats[idStat]["ts_openflow"] -
                          oldStats[idStat]["ts_openflow"])/1000000)>0):

                        print switchStats[idStat]["ifName"]
                        #print "Elapsed Time on: "+ip_to_str_reversed(host)+" : "+str((switchStats[idStat]["ts_openflow"]-oldStats[idStat]["ts_openflow"]))
                        switchStats[idStat]["txavg"] = ((switchStats[idStat]["txbytes"] -
                                                         oldStats[idStat]["txbytes"]) /
                                                         ((switchStats[idStat]["ts_openflow"] -
                                                         oldStats[idStat]["ts_openflow"])/1000000))
                        #print switchStats[idStat]["txavg"]

                        switchStats[idStat]["rxavg"] = ((switchStats[idStat]["rxbytes"] -
                                                         oldStats[idStat]["rxbytes"]) /
                                                         ((switchStats[idStat]["ts_openflow"] -
                                                         oldStats[idStat]["ts_openflow"])/1000000))
                        #print switchStats[idStat]["rxavg"]
                    else:
                        switchStats[idStat]["txavg"] = 0
                        switchStats[idStat]["rxavg"] = 0

    def buildNetGraph(self):
        self.netGraph={} #reset the current graph

        for host in self.switchesIP: #for all the host that i monitor
            ipString= ip_to_str_reversed(host)
            datapathID= self.ipToDatapathid[ipString] #take the datapathID for that host
            adjacencyTemp ={} #create the adjacency host list
            lg.info(ipString)

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
                            lg.info("found Stat: " +
                                   str(statsForHost[stat]["rxavg"]) +
                                   " bytes/s for interface: " + portName)
                        #adjacencyTemp[dp2]=statsForHost[stat]["rxavg"] #set the weight for that link
                                                        
                            #set the weight for that link (max bandwidth - busy bandwidth)
                            adjacencyTemp[dp2] = (10000000-statsForHost[stat]["rxavg"] -
                                                  statsForHost[stat]["txavg"])

            self.netGraph[datapathID] = adjacencyTemp #update the information for that datapathID

        lg.info(self.netGraph)
        lg.info("END generation of graph for Dijkstra's algorithm")

    def dLoad(self):
        return True
        
    def get_link_load(self, dp, portID):
        #lg.info("Returning stats about port and node\n\n")
        dpid = dp.as_host()
        a = {}

        if dpid in self.dpToNip:
            switchIP = self.dpToNip[dpid]
            ipString= ip_to_str_reversed(switchIP)

            portName= self.portidToPortname[ipString][portID]
            if ipString not in self.totalStats:
                return (a, False)

            statsForHost= self.totalStats[ipString]
            keys = statsForHost.keys()                    

            for stat in keys:
                #print portName
                #print statsForHost[stat]["ifName"]
                if statsForHost[stat]["ifName"] == portName:
                    a['Rx_bytes'] = statsForHost[stat]["rxavg"]
                    a['Tx_bytes'] = statsForHost[stat]["txavg"]
                    
                    return (a,True)

        return (a,False)
        
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
            return snmp_stats(ctxt)

    return Factory()


def ip_to_str_reversed( ipToConvert ):
    ipSplitted = ip_to_str(ipToConvert).split(".")
    ip =  ipSplitted[3]+"."+  ipSplitted[2]+"."+ipSplitted[1]+"."+ipSplitted[0]
    return ip        

def str_ip_to_str_reversed( ipToConvert ):
    ipSplitted=str(ipToConvert).split(".")
    ip =  ipSplitted[3]+"."+  ipSplitted[2]+"."+ipSplitted[1]+"."+ipSplitted[0]
    return ip                
        
def ip2long(ip):
    packedIP = socket.inet_aton(ip)
    return struct.unpack("!L", packedIP)[0]
