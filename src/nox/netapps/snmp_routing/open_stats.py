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
from datetime import datetime
import hashlib
lg = logging.getLogger('open-stats')

hdlr = logging.FileHandler('OFNIC.log')
formatter = logging.Formatter('%(asctime)s|%(name)s|%(levelname)s|%(message)s')
hdlr.setFormatter(formatter)
lg.addHandler(hdlr) 
lg.setLevel(logging.WARNING)


class open_stats(Component):
    portStatMap = {}
    activeFlows = {}
    monitoredFlows = {}
    pathCookies = {}

    portLoadMap = {}
    #Carries the last stat packet(all values)
    flowStatMap = {}
    #carries the values
    flowLoadMap = {}
    
    pathList = {}
    FLOW_CHECK_INTERVAL = 2
    
    def __init__(self, ctxt):
        Component.__init__(self, ctxt)

    def getInterface(self):
        return str(open_stats)    

    def install(self):
        self.register_for_datapath_leave (lambda dp : open_stats.handle_dp_leave(self, dp) )

        self.register_for_datapath_join (lambda dp,stats : open_stats.handle_dp_join(self, dp, stats) )

        self.register_for_port_status( lambda dp, of_port_reason, attrs : 
                                       self.handle_port_event(dp, 
                                                             of_port_reason,
                                                             attrs))
        #Flow_stats_in_event.register_event_converter(self.ctxt)
        self.register_for_flow_stats_in(lambda dp, stats : open_stats.handle_flow_stats(self, dp, stats))
        self.flow_stat_probe()

    def configure(self, config):
        pass
        #print config

    def handle_dp_join(self, dp, stats):
        pass

    def flow_stat_probe(self):
        #TODO delete expired entries
        toBeDel = []
        for mid in self.monitoredFlows:
            monInfo = self.monitoredFlows[mid]
            
            tNow = datetime.now()
            td = tNow - monInfo['stime']
            tdSec = td.days * 24 * 3600 + td.seconds
            if (tdSec > monInfo['duration']):
                lg.info("Entry expired: " + str(mid))
                toBeDel.append(mid)
                continue
            
            #lg.info("Probing " + str (mid))
            self.send_flow_stat_request(self.monitoredFlows[mid]['dpid'], 
                                        self.monitoredFlows[mid]['match'])
        
        
        #delete entries
        if (len(toBeDel) > 0):
            for entry in toBeDel:
                del self.monitoredFlows[entry]
        
        #post a callback for itself
        self.post_callback(self.FLOW_CHECK_INTERVAL, 
                           lambda : open_stats.flow_stat_probe(self))


    def calculate_mon_info_md5(self, monInfo):
        m = hashlib.md5()
        #stringToMd5 = (str(monInfo.pathID) + str(monInfo.dpid) +
                       #str(monInfo.cookie) + str(monInfo.start_time))
        
        stringToMd5 = (str(monInfo['pathID']) + str(monInfo['dpid']) +
                       str(monInfo['cookie']))
        m.update(stringToMd5)
        #return the truncation to the first 16 hex chars. TODO
        return m.hexdigest()[:8]

    def create_new_flow_monitor(self, pid,
                                datapathid,duration,
                                frequency):
        pathID = hex(pid)[2:]
        dpid = datapathid.as_host()
        
        if not pathID in self.activeFlows:
            #wrong pathID
            return 0,1
        pathInfo = self.activeFlows[pathID]
        
        if not dpid in pathInfo:
            #wrong dpid
            return 0,2

        #implemented with empty class
        #monInfo = MonitorInfo()
        #monInfo.pathID = pathID
        #monInfo.dpid = dpid
        #monInfo.match = self.activeFlows[pathID][dpid]
        #for dp,cookie in self.activeFlows[pathID]["cookies"]:
            #if dp == dpid:
                #monInfo.cookie = cookie
        #monInfo.start_time = datetime.now()

        #implemented with dict
        monInfo = {}
        monInfo['pathID'] = pathID
        monInfo['dpid'] = dpid
        monInfo['match'] = self.activeFlows[pathID][dpid][0]
        for dp,cookie in self.activeFlows[pathID]["cookies"]:
            if dp == dpid:
                monInfo['cookie'] = cookie
        monInfo['stime'] = datetime.now()
        monInfo['duration'] = duration

        #print monInfo
        mid = self.calculate_mon_info_md5(monInfo)
        
        self.monitoredFlows[mid] = monInfo
        
        
        return int(mid,16),0

    def send_flow_stat_request(self, dpid, attrs):
        flow = set_match(attrs)
        self.ctxt.send_flow_stats_request(dpid, flow)

    def handle_dp_leave(self,dp):
        pass
    def handle_port_event(self, dp, of_port_reason, attrs):
        pass

    def handle_flow_stats(self, dp, flows):

        for fsie in flows:
            for monID in self.monitoredFlows:
                monInfo = self.monitoredFlows[monID]
                if monInfo['cookie'] == fsie['cookie']:
                    if monID in self.flowStatMap:
                        #update statistics
                        self.updateFlowLoad(monID, fsie,
                                            self.flowStatMap[monID])
                    #put this in memory
                    self.flowStatMap[monID] = fsie
                        


    def updateFlowLoad(self, mon, flowStatNew, flowStatOld):
        #print flowStatNew['packet_count'], flowStatOld['packet_count']
        #print flowStatNew['byte_count'], flowStatOld['byte_count']

        self.flowLoadMap[mon] = {}

        self.flowLoadMap[mon]['packet_per_s'] = ((flowStatNew['packet_count'] - 
                                                  flowStatOld['packet_count']) /
                                                 self.FLOW_CHECK_INTERVAL)

        self.flowLoadMap[mon]['byte_per_s'] = ((flowStatNew['byte_count'] - 
                                                  flowStatOld['byte_count']) /
                                                 self.FLOW_CHECK_INTERVAL)

    def add_path(self, path, pathID):
        self.activeFlows[pathID] = path
        return

    def remove_path(self, pathID):
        if pathID in self.activeFlows:
            del self.activeFlows[pathID]

        toBeDel = None
        for monID in self.monitoredFlows:
            if self.monitoredFlows[monID]['pathID'] == pathID:
                toBeDel = monID

        if not (toBeDel == None):
            lg.info("Deleting the pathID---")
            del self.monitoredFlows[toBeDel]
            


    def get_flow_load(self, mid):
        #convert from int to hex string without the preceding '0x'
        monID = hex(mid)[2:]
        load = Load()
        if (monID in self.monitoredFlows):
            if (monID in self.flowLoadMap):
                load.p_count = self.flowLoadMap[monID]['packet_per_s']
                load.b_count = self.flowLoadMap[monID]['byte_per_s']
                return load,0
            else:
                load.p_count = 0
                load.b_count = 0
                return load,2
        else:
            return None,1

    def get_path_mon_ids(self):
        strOut = ""
        count = 0
        for monID in self.monitoredFlows:
            if count > 0:
                strOut += ":"
            strOut += str(monID)
            count += 1
        #print strOut#7e6b26a5
        return strOut

    def remove_monitor_flow(self,mid):
        monID = hex(mid)[2:]
        if monID in self.monitoredFlows:
            del self.monitoredFlows[monID]
        if monID in self.flowStatMap:
            del self.flowStatMap[monID]
        if monID in self.flowLoadMap:
            del self.flowLoadMap[monID]
        return 0

def getFactory():
    class Factory:
        def instance(self, ctxt):
            return open_stats(ctxt)

    return Factory()

#Some needed C-like structures
class SwitchPort:
    pass
class Load:
    pass
class MonitorInfo:
    pass



