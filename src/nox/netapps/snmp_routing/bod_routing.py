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

from __future__ import generators #needed for Dijkstra's algorithm
from nox.lib.core import *
from nox.lib.openflow import *
from nox.lib.packet.packet_utils      import ip_to_str
from array import array
from datetime import datetime

from nox.netapps.discovery import discovery #the topology is described in adjacency_list populated from discovery
import socket, struct
import hashlib

from nox.netapps.netic.snmp_handler import snmpMega
from nox.netapps.snmp_routing.open_stats import open_stats

lg = logging.getLogger('bod-routing')
hdlr = logging.FileHandler('OFNIC.log')
formatter = logging.Formatter('%(asctime)s|%(name)s|%(levelname)s|%(message)s')
hdlr.setFormatter(formatter)
lg.addHandler(hdlr) 
lg.setLevel(logging.WARNING)

class bod_routing(Component):
    netGraph = {}
    installTimes = 0
    installedFlows = {}
    statsManager = None
    pathMonitor = None
    GRAPH_CHECK_PERIOD = 1 #seconds
    

    def __init__(self, ctxt):
        Component.__init__(self, ctxt)
    
    def install(self):
        #need to resolve discovery where is the described the topology
        self.discovery = self.resolve(discovery.discovery)
        try:
            self.statsManager = self.resolve(snmpMega)
            self.pathMonitor = self.resolve(open_stats)
            self.updateGraph()
        except:
            lg.warn("No network statistics manager was found")
        self.register_for_flow_removed ( lambda dp, flow : bod_routing.manage_flow_expired(self, dp, flow) )
        

    def updateGraph(self):
        self.netGraph = self.statsManager.netGraph
        self.post_callback(self.GRAPH_CHECK_PERIOD, lambda : bod_routing.updateGraph(self))


    def netic_create_route(self, npi):
        return self.install_path(npi)

    def calculate_path_md5(self, path, startPort, endPort):
        m = hashlib.md5()
        stringToMd5 = str(startPort) #a path start with the port where the host is connected
        for switchOnPath in path:
            stringToMd5 = stringToMd5 + str(switchOnPath)
        stringToMd5 = stringToMd5 + str(endPort) #a path ends with the port where the destination host is connected
        m.update(stringToMd5)
        return m.hexdigest()

    def calculate_path_id(self, npi):
        m = hashlib.md5()
        stringToMd5 = (str(npi.nw_src) + str(npi.nw_dst) +
                       str(npi.ip_proto) + str(npi.tp_src) +
                       str(npi.tp_dst) + str(npi.dp_src) +
                       str(npi.dp_dst))
        m.update(stringToMd5)
        #return the truncation to the first 16 hex chars. TODO
        return m.hexdigest()[:4]

    def calculate_cookie(self, dp, pathID):
        m = hashlib.md5()
        stringToMd5 = str(dp) + str(pathID)
        m.update(stringToMd5)
        outLarge = m.hexdigest()
        #return the truncation to the first 16 hex chars. TODO
        return outLarge[:16]

    def revert_netic_path_info(self, npi):
        nnpi = Netic_path_info()
        nnpi.nw_src = npi.nw_dst
        nnpi.nw_dst = npi.nw_src
        nnpi.duration = npi.duration
        nnpi.bandwidth = npi.bandwidth
        nnpi.set_arp = npi.set_arp
        nnpi.dp_src = npi.dp_dst
        nnpi.dp_dst = npi.dp_src
        nnpi.first_port = npi.last_port
        nnpi.last_port = npi.first_port
        if (npi.set_arp):
            nnpi.dl_src = npi.dl_dst
            nnpi.dl_dst = npi.dl_src

        nnpi.ip_proto = npi.ip_proto
        nnpi.tp_src = npi.tp_dst
        nnpi.tp_dst = npi.tp_src
        return nnpi

    def install_path(self, npi, setInverse = True): #install all needed flow between two dpid
        #G = {'s':{'u':10, 'x':5}, 'u':{'v':1, 'x':2}, 'v':{'y':4}, 'x':{'u':3, 'v':9, 'y':2}, 'y':{'s':7, 'v':6}}
        #sP = shortestPath(G,'s','v',0)
        #print sP
        sP=[]
        
        print "inside function"
    
        print self.netGraph
        error = 0;
        startNode = npi.dp_src.as_host()
        endNode = npi.dp_dst.as_host()
        startPort = npi.first_port #port id where the source node is connected
        endPort = npi.last_port #port id where the destination node is connected
        requiredBW = npi.bandwidth * 1000 #from interface it is in kbps
        
        print requiredBW

        if (self.netGraph.has_key(startNode) and
            self.netGraph.has_key(endNode)):

            sP = shortestPath(self.netGraph,startNode,endNode,requiredBW) #bandwidth requisite in Mb
            print sP
        else:
            #Should not go here some very bad thing happened
            lg.error("Start node and/or end node not found in Network Graph")

        #start analysing the path to retrieve the port id for each link returned from the shortest path method
        i = 0
        enhancedPath=[]

        while i < len(sP) - 1:
            for dp1, port1, dp2, port2 in self.discovery.adjacency_list:
                if dp1 == sP[i] and dp2 == sP[i + 1]:#if i found a link that connect the two nodes on the path
                    enhancedPath.append([port1, port2])
            i += 1

        pathMd5 = self.calculate_path_md5(sP, startPort, endPort)
        print str(type(pathMd5))
        print "md5 of calculated path ", pathMd5

        #needed to identify if i have alrealdy calculated this path
        pathID = self.calculate_path_id(npi)
        print "pathID: "+str(pathID)
        
        if self.installedFlows.has_key(pathID):
            if self.installedFlows[pathID]["md5"] == pathMd5: #if the path is already working return
                print "already installed path" #TODO
                return Result(),1
            else: #if the calculated path is different i need to remove it
                print "remove old path"
                for involvedDpId in self.installedFlows[pathID]["path"]:
                    for attr in self.installedFlows[pathID][involvedDpId]:
                        self.delete_datapath_flow(involvedDpId,attr)#remove the old flow in the path
                        print "removed attr from: "+str(involvedDpId)
                
        
        #if i'm here i removed any old flows for this path if any 
        self.installedFlows[pathID] = {}    
        self.installedFlows[pathID]["initTime"] = datetime.now()
        self.installedFlows[pathID]["duration"] = npi.duration
        self.installedFlows[pathID]["md5"] = pathMd5 #store the md5 of the calculated path
        self.installedFlows[pathID]["bandwidth"] = requiredBW

        print "*** Path with source and destination port information***"
        self.installedFlows[pathID]["path"]=sP
        
        self.netic_exec_route(pathID, sP , enhancedPath, npi, not setInverse)
        
        self.add_to_path_monitor(self.installedFlows[pathID], pathID)
        
        result = Result()
        result.directPath = -1
        result.reversPath = -1
        error = 0
        #install inverse path if required
        if (setInverse and npi.bidirect):
            result,error = self.install_path(self.revert_netic_path_info(npi),False)
        #append the PathID to the result
        if setInverse:
            result.directPath = pathID
        else:
            result.reversPath = pathID

        return result,error

    def netic_exec_route(self, pathID, sP, enhancedPath, npi, isReverse):
        i = 0
        self.installedFlows[pathID]["cookies"] = []
        while i < len(sP)-1 and len(enhancedPath) > 0:
            print "Node: " + str(sP[i])

            cookie = int(self.calculate_cookie(sP[i], pathID),16)
            #add cookie to list of cookies of this path
            self.installedFlows[pathID]["cookies"].append((sP[i],cookie))
            self.installedFlows[pathID][sP[i]] = []
            if i == 0:
                #first node
                attrsInvolved = self.netic_setup_openflow_packet(sP[i],npi.first_port,
                                                                 enhancedPath[i][0],
                                                                 npi, cookie)

                self.installedFlows[pathID][sP[i]].append(attrsInvolved) #store the attrs
                
                if npi.set_arp:
                    self.netic_setup_arp_packet(sP[i],npi.first_port,
                                                enhancedPath[i][0],
                                                npi, isReverse)
            else:
                #middle nodes
                attrsInvolved = self.netic_setup_openflow_packet(sP[i],enhancedPath[i-1][1],
                                                                 enhancedPath[i][0],
                                                                 npi, cookie)
                self.installedFlows[pathID][sP[i]].append(attrsInvolved) #store the attrs
                if npi.set_arp:
                    self.netic_setup_arp_packet(sP[i],enhancedPath[i-1][1],
                                                enhancedPath[i][0],
                                                npi, isReverse)
            i += 1

        #last node
        if len( enhancedPath)> 0:
            cookie = int(self.calculate_cookie(sP[len(sP)-1], pathID),16)
            #add cookie to list of cookies of this path
            self.installedFlows[pathID]["cookies"].append((sP[len(sP)-1], 
                                                           cookie))
            self.installedFlows[pathID][sP[len(sP)-1]] = []

            print "Node: "+str(sP[len(sP)-1])

            attrsInvolved = self.netic_setup_openflow_packet(sP[len(sP)-1],
                                                             enhancedPath[len(sP)-2][1],
                                                             npi.last_port, npi, cookie) 

            self.installedFlows[pathID][sP[len(sP)-1]].append(attrsInvolved)
            if npi.set_arp:
                self.netic_setup_arp_packet(sP[len(sP)-1],
                                            enhancedPath[len(sP)-2][1],
                                            npi.last_port, npi, isReverse)

    def netic_setup_openflow_packet(self, dp_id, inport, outport, npi, cookie):
        attrs = {core.IN_PORT : inport,
                 core.DL_TYPE : ethernet.ethernet.IP_TYPE,
                 core.NW_SRC : npi.nw_src,
                 core.NW_DST : npi.nw_dst}
                 
        if not (npi.ip_proto == 255):
            attrs[core.NW_PROTO] = npi.ip_proto
            attrs[core.TP_SRC] = npi.tp_src
            attrs[core.TP_DST] = npi.tp_dst
    
        idle_timeout = openflow.OFP_FLOW_PERMANENT
        hard_timeout = npi.duration
        
        actions = [[openflow.OFPAT_OUTPUT, [0, outport]]]
        
        print ("***try to install flow with dp_id: "+ 
               str(dp_id) +" inport: "+ str(inport) +" outport: " +
               str(outport) +" src_ip "+ str(npi.nw_src) +" dst_ip: " + 
               str(npi.nw_dst) +" ***")
        self.install_datapath_flow(dp_id, attrs,
                                   idle_timeout,
                                   hard_timeout,actions,cookie)
        return attrs                        

    def netic_setup_arp_packet(self, dp_id, inport, outport, npi, isReverse):

        #is needed to remove the flow when is needed
        #and keep track of the attrs related to the installed flow
        
        attrs = {core.IN_PORT : inport,core.DL_TYPE : ethernet.ethernet.ARP_TYPE,
                 #core.NW_PROTO:arp.arp.REQUEST,
                 core.NW_SRC : npi.nw_src,
                 core.NW_DST : npi.nw_dst}
        if isReverse:
            attrs[core.NW_PROTO] = arp.arp.REPLY
        else:
            attrs[core.NW_PROTO] = arp.arp.REQUEST
        
        idle_timeout = openflow.OFP_FLOW_PERMANENT
        hard_timeout = npi.duration
        actions = [[openflow.OFPAT_OUTPUT, [0, outport]]]

        print ("***try to install arp flow with dp_id: "+ 
               str(dp_id) +" inport: "+ str(inport) +" outport: " +
               str(outport) +" src_ip "+ str(npi.nw_src) +" dst_ip: " + 
               str(npi.nw_dst) +" ***")
        self.install_datapath_flow(dp_id,attrs,
                                   idle_timeout,
                                   hard_timeout,actions)
        return attrs

    def add_to_path_monitor(self, path, pathID):
        self.pathMonitor.add_path(path, pathID)
        return
    def manage_flow_expired(self, dp, flow):
        pathToDel = None
        for pathID in self.installedFlows:
            path = self.installedFlows[pathID]
            for (d,cookie) in path['cookies']:
                if (cookie == flow['cookie']):
                    #lg.info("found cookie to remove..." + str(pathID))
                    pathToDel = pathID

        if not (pathToDel == None):
            self.path_expire_cb(pathToDel)
            del self.installedFlows[pathToDel]
            self.pathMonitor.remove_path(pathToDel)
        

    def getInterface(self):
        return str(bod_routing)

    def netic_get_path_list(self):
        strOut = ""
        count = 0
        for path in self.installedFlows:
            if count > 0:
                strOut += ":"
            strOut += str(path)
            count += 1;
        return strOut

    def register_path_expire_cb(self, cb):
        self.path_expire_cb = cb

    def netic_remove_path(self, pID):
        if self.installedFlows.has_key(pID):
            pathEntry = self.installedFlows[pID]
            
            for dpID in pathEntry["path"]:
                for attrs in pathEntry[dpID]:
                    self.delete_strict_datapath_flow(dpID, attrs)
            return 0
        return 1
        
    def netic_get_path_info(self, pID):
        if self.installedFlows.has_key(pID):
            pathEntry = self.installedFlows[pID]
            #print pathEntry
            npi = Netic_path_info()
            td = (datetime.now() - pathEntry["initTime"])
            npi.timeToDie = pathEntry["duration"] - (td.seconds + td.days * 3600 * 24) 
            if npi.timeToDie < 0:
                npi.timeToDie = 0
            npi.path = pathEntry["path"]
            #pick first node to know the flow
            dp1 = pathEntry["path"][0]
            npi.nw_src = pathEntry[dp1][0]["nw_src"]
            npi.nw_dst = pathEntry[dp1][0]["nw_dst"]
            npi.bandwidth = pathEntry["bandwidth"]
            return npi
        return None
            
            
        
def getFactory():
    class Factory:
        def instance(self, ctxt):
            return bod_routing(ctxt)
    
    return Factory()

class Netic_path_info:
    pass
class Result:
    pass

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
    

# Dijkstra's algorithm for shortest paths

def Dijkstra(G,start,bandwidth,end=None):
    """
    Find shortest paths from the start vertex to all
    vertices nearer than or equal to the end.

    The input graph G is assumed to have the following
    representation: A vertex can be any object that can
    be used as an index into a dictionary.  G is a
    dictionary, indexed by vertices.  For any vertex v,
    G[v] is itself a dictionary, indexed by the neighbors
    of v.  For any edge v->w, G[v][w] is the length of
    the edge.  This is related to the representation in
    <http://www.python.org/doc/essays/graphs.html>
    where Guido van Rossum suggests representing graphs
    as dictionaries mapping vertices to lists of neighbors,
    however dictionaries of edges have many advantages
    over lists: they can store extra information (here,
    the lengths), they support fast existence tests,
    and they allow easy modification of the graph by edge
    insertion and removal.  Such modifications are not
    needed here but are important in other graph algorithms.
    Since dictionaries obey iterator protocol, a graph
    represented as described here could be handed without
    modification to an algorithm using Guido's representation.

    Of course, G and G[v] need not be Python dict objects;
    they can be any other object that obeys dict protocol,
    for instance a wrapper in which vertices are URLs
    and a call to G[v] loads the web page and finds its links.
    
    The output is a pair (D,P) where D[v] is the distance
    from start to v and P[v] is the predecessor of v along
    the shortest path from s to v.
    
    Dijkstra's algorithm is only guaranteed to work correctly
    when all edge lengths are positive. This code does not
    verify this property for all edges (only the edges seen
     before the end vertex is reached), but will correctly
    compute shortest paths even for some graphs with negative
    edges, and will raise an exception if it discovers that
    a negative edge has caused it to make a mistake.
    """

    D = {}    # dictionary of final distances
    P = {}    # dictionary of predecessors
    Q = priorityDictionary()   # est.dist. of non-final vert.
    Q[start] = 0
    
    for v in Q:
        D[v] = Q[v]
        if v == end: break
        
        for w in G[v]:
            vwLength = D[v] + G[v][w]
            if w in D:
                if vwLength < D[w]:
                    raise ValueError, \
  "Dijkstra: found better path to already-final vertex"
            elif w not in Q or vwLength < Q[w]:
                if vwLength >= bandwidth: #bandwidth requisite
                    Q[w] = vwLength
                    P[w] = v
    
    return (D,P)
            
def shortestPath(G,start,end,bandwidth):
    """
    Find a single shortest path from the given start vertex
    to the given end vertex.
    The input has the same conventions as Dijkstra().
    The output is a list of the vertices in order along
    the shortest path.
    """
    if bandwidth <0: bandwidth=0
    D,P = Dijkstra(G,start,bandwidth,end)
    Path = []
    while 1:
        Path.append(end)
        if end == start: break

        if P.has_key(end):
            end = P[end]
        else: return {}
    Path.reverse()
    return Path

#support class for Dijkstra's algorithm 
    
class priorityDictionary(dict):
    def __init__(self):
        '''Initialize priorityDictionary by creating binary heap
of pairs (value,key).  Note that changing or removing a dict entry will
not remove the old pair from the heap until it is found by smallest() or
until the heap is rebuilt.'''
        self.__heap = []
        dict.__init__(self)

    def smallest(self):
        '''Find smallest item after removing deleted items from heap.'''
        if len(self) == 0:
            raise IndexError, "smallest of empty priorityDictionary"
        heap = self.__heap
        while heap[0][1] not in self or self[heap[0][1]] != heap[0][0]:
            lastItem = heap.pop()
            insertionPoint = 0
            while 1:
                smallChild = 2*insertionPoint+1
                if smallChild+1 < len(heap) and \
                        heap[smallChild] > heap[smallChild+1]:
                    smallChild += 1
                if smallChild >= len(heap) or lastItem <= heap[smallChild]:
                    heap[insertionPoint] = lastItem
                    break
                heap[insertionPoint] = heap[smallChild]
                insertionPoint = smallChild
        return heap[0][1]
    
    def __iter__(self):
        '''Create destructive sorted iterator of priorityDictionary.'''
        def iterfn():
            while len(self) > 0:
                x = self.smallest()
                yield x
                del self[x]
        return iterfn()
    
    def __setitem__(self,key,val):
        '''Change value stored in dictionary and add corresponding
pair to heap.  Rebuilds the heap if the number of deleted items grows
too large, to avoid memory leakage.'''
        dict.__setitem__(self,key,val)
        heap = self.__heap
        if len(heap) > 2 * len(self):
            self.__heap = [(v,k) for k,v in self.iteritems()]
            self.__heap.sort()  # builtin sort likely faster than O(n) heapify
        else:
            newPair = (val,key)
            insertionPoint = len(heap)
            heap.append(None)
            while insertionPoint > 0 and \
                    newPair < heap[(insertionPoint-1)//2]:
                heap[insertionPoint] = heap[(insertionPoint-1)//2]
                insertionPoint = (insertionPoint-1)//2
            heap[insertionPoint] = newPair
    
    def setdefault(self,key,val):
        '''Reimplement setdefault to call our customized __setitem__.'''
        if key not in self:
            self[key] = val
        return self[key]
