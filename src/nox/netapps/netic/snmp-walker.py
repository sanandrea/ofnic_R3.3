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
# @date 23/04/2013

from copy import deepcopy #needed to deep copy dictonary of dictionary
from datetime import datetime

#snmp stuff
from pysnmp.entity.rfc3413.oneliner import cmdgen
from pysnmp.smi import builder

import errno
import os
import socket
import simplejson as json
import struct
import time
import signal


HOST = '127.0.0.1'
PORT = 2703
SLEEPTIME = 3

def send_stat_string_to_controller(dpid, stat):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST,PORT))
    except socket.error as e:
        if e.errno == errno.ECONNREFUSED:
            print ("Error, connection to nox socket was refused")
        print "Could not send stat to controller."
        print "Is it dead, Jim?"
        return
    msg = {}
    msg['type'] = "snmp"
    msg['command'] = "new_stat"
    msg['dpid'] = dpid
    msg['stat'] = stat

    s.sendall(json.dumps(msg))
    
    #response = json.loads(s.recv(4096))
    #print json.dumps(response, indent=4)
    s.send("{\"type\":\"disconnect\"}")
    s.shutdown(1)
    s.close()

def send_stat_to_controller(dpid, stat_id, if_name, stat_name, stat_value, timestamp):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST,PORT))
    except socket.error as e:
        if e.errno == errno.ECONNREFUSED:
            print ("Error, connection to nox socket was refused")
        print "Could not send stat to controller."
        print "Is it dead, Jim?"
        return
    msg = {}
    msg['type'] = "snmp"
    msg['command'] = "new_stat"
    msg['dpid'] = dpid
    msg['stat_id'] = stat_id
    msg['if_name'] = if_name
    msg['s_name'] = stat_name
    msg['s_value'] = stat_value
    msg['ts'] = timestamp
    #print msg
    s.sendall(json.dumps(msg))
    
    #response = json.loads(s.recv(4096))
    #print json.dumps(response, indent=4)
    s.send("{\"type\":\"disconnect\"}")
    s.shutdown(1)
    s.close()

class SnmpWalker:
    totalStats ={}
    nox_ip = None
    nox_port = None

    def __init__(self, nox_ip_, nox_port_):
        self.nox_ip = nox_ip_
        self.nox_port = nox_port_

    def send_stat_nox(self, stat):
        pass

    def pollSwitch(self, host):
        #lg.info("IP: "+ip_to_str(host))
                   
        cmdGen = cmdgen.CommandGenerator()
                   
        mibBuilder = cmdGen.snmpEngine.msgAndPduDsp.mibInstrumController.mibBuilder
        #print str(mibBuilder.getMibSources())

        mibSources = mibBuilder.getMibSources() + (
                #builder.DirMibSource('/hosthome/OF/controller_utils/pymib'),
                #builder.DirMibSource('../../pymib'),
                builder.DirMibSource('../nox/pymib'),
                )
        mibBuilder.setMibSources(*mibSources)
        mibBuilder.loadModules('ETHTOOL-MIB',)
        #print builder.MibBuilder().getMibPath()

        #print "Probing IP: ", ip_to_str_reversed(host)

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

        oldId = -1
        switchStats = {}

        for varBindTableRow in varBindTable:
            for name, val in varBindTableRow:
                #print('%s = %s' % (name.prettyPrint(), val.prettyPrint()))        
                statPrettified = name.prettyPrint().replace("ETHTOOL-MIB::ethtoolStat.\"","")
                #this is a progressive number starting from 1 given by the snmpwalk on the oid
                idStat = statPrettified[:statPrettified.find(".")].replace("\"","")
                statPrettified = name.prettyPrint().replace("ETHTOOL-MIB::ethtoolStat.\"" + idStat + 
                                                            "\".\"","").replace("\"","")
                #here there will be the name of the stat
                #print (statPrettified), " and ID stat is: ", idStat
                
                if oldId != idStat:         
                    #if i received stat for a new interface i have to initialize the dictionary
                    switchStats[idStat]= {} 
                    oldId = idStat
                
                if (statPrettified != "rxbytes" and statPrettified != "ts_milliseconds" and
                    statPrettified != "ts_openflow" and statPrettified != "txbytes" and
                    statPrettified != "ts_seconds"): #if is the ifName

                    switchStats[idStat]["ifName"]= statPrettified #store the name of the iface
                elif statPrettified == "ts_openflow":
                    switchStats[idStat][statPrettified] = str(val.prettyPrint())
                else:
                    #store the stat if it's not the ifname
                    switchStats[idStat][statPrettified] = long(val.prettyPrint())

        ripStat = {}
        for stat in switchStats:
            iface_dict = switchStats[stat]
            #don't send loopback or teql
            if (iface_dict['ifName'] == "lo" or
                iface_dict['ifName'] == "teql0" or
                iface_dict['ifName'] == "br0"):
                continue
            ripStat[stat] = iface_dict

        send_stat_string_to_controller(host, ripStat)

        os._exit(0)

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

def ip_to_str(a):
    #return "%d.%d.%d.%d" % ((a >> 24) & 0xff, (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff)
    return socket.inet_ntoa(struct.pack('!L', a))


def get_switches_ips():
    msg = {}
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST,PORT))
    except socket.error as e:
        if e.errno == errno.ECONNREFUSED:
            print ("Error, connection to nox socket was refused")
        print "Could not retrieve switches IP"
        return {}

    msg['type'] = "snmp"
    msg['command'] = "topo_req"
    #print msg
    s.sendall(json.dumps(msg))

    response = json.loads(s.recv(4096))
    #print json.dumps(response, indent=4)
    s.send("{\"type\":\"disconnect\"}")
    s.shutdown(1)
    s.close()
    return response

if __name__ == "__main__":
    print "Snmp walker started..."
    #ignore child death, kernel will automatically reap them
    signal.signal(signal.SIGCHLD, signal.SIG_IGN)
    get_switch_frequency = 3
    i = 0
    
    switchesIp = get_switches_ips()
    snmpWalker = SnmpWalker(HOST, PORT)
    
    while True:
        i = i + 1
        if (i % get_switch_frequency == 0):
            #every 3 switchpolls renew ip info.
            switchesIp = {}
            switchesIp = get_switches_ips()

        for ip in switchesIp:
            newpid = os.fork()
            if (newpid == 0):
                #give this child process the task to poll a switch
                snmpWalker.pollSwitch(switchesIp[ip])
            else:
                #do nothing in father
                pass
        #print "before sleep"
        time.sleep(SLEEPTIME)
        #print "after sleep"





