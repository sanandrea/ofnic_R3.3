/* Copyright 2010 (C) Stanford University.
 *
 * This file is part of NOX.
 *
 * NOX is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * NOX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with NOX.  If not, see <http://www.gnu.org/licenses/>.
 */

/* Copyright 2013 (C) Universita' di Roma La Sapienza
 *
 * This file is part of OFNIC Uniroma GE.
 *
 * OFNIC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OFNIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OFNIC.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "assert.hh"
#include "linkload.hh"
#include "port-status.hh"
#include "datapath-leave.hh"
#include "openflow-pack.hh"
#include <boost/bind.hpp>

#include "fnv_hash.h"
#include "serialize.h"

namespace vigil
{
  static Vlog_module lg("linkload");
  
  void linkload::configure(const Configuration* c)
  {
    //Get commandline arguments
    const hash_map<string, string> argmap = \
      c->get_arguments_list();
    hash_map<string, string>::const_iterator i;
    i = argmap.find("interval");
    if (i != argmap.end())
      load_interval = atoi(i->second.c_str());
    else
      load_interval = LINKLOAD_DEFAULT_INTERVAL;
      
    //TODO macro
    flow_interval = 1;

    resolve(dpmem);
    
    register_handler<Port_stats_in_event>
      (boost::bind(&linkload::handle_port_stats, this, _1));
    register_handler<Flow_stats_in_event>
      (boost::bind(&linkload::handle_flow_stats, this, _1));
    register_handler<Datapath_leave_event>
      (boost::bind(&linkload::handle_dp_leave, this, _1));
    register_handler<Port_status_event>
      (boost::bind(&linkload::handle_port_event, this, _1));
    register_handler<Path_event>
      (boost::bind(&linkload::handle_path_event, this, _1));
  }
  
  void linkload::dummyLoad(){
	  VLOG_DBG(lg,"Dummmmmyy!!!");
  }

  void linkload::install()
  {
    dpi = dpmem->dp_events.begin();

    post(boost::bind(&linkload::stat_probe, this), get_next_time());
    post(boost::bind(&linkload::flow_stat_probe, this), get_next_monitor_time());
  }

  void linkload::stat_probe()
  {
    if (dpmem->dp_events.size() != 0)
    {
      if (dpi == dpmem->dp_events.end())
	dpi = dpmem->dp_events.begin();
      
/*
      VLOG_DBG(lg, "Send probe to %"PRIx64"",
	       dpi->second.datapath_id.as_host());
*/

      send_stat_req(dpi->second.datapath_id);

      dpi++;
    }

    post(boost::bind(&linkload::stat_probe, this), get_next_time());
  }
  std::string 
  linkload::get_path_mon_ids(){
    hash_map<MonitorID_t,monitorInfo>::iterator mi;
    std::string ret = "";
    char       out[9];
    
    for (mi = monitoredFlows.begin(); mi!=monitoredFlows.end(); mi++){
        sprintf(out, "%x", mi->first);
        if (mi != monitoredFlows.begin())
            ret += ":";
        ret += out;
    }
    return ret;
  }
  
  void linkload::flow_stat_probe()
  {
    //VLOG_DBG(lg, "Probing: there are %d monitored flows ",monitoredFlows.size());
    hash_map<MonitorID_t,monitorInfo>::iterator mon_iter;
    
    for (mon_iter = monitoredFlows.begin(); mon_iter != monitoredFlows.end(); mon_iter++)
    {
      /* Check if it has expired */
      timeval curtime = { 0, 0 };
      gettimeofday(&curtime, NULL);
//      VLOG_DBG(lg,"%li %"PRIu64"",(curtime.tv_sec - mon_iter->second.start_time.tv_sec), mon_iter->second.duration);
      if ((curtime.tv_sec - mon_iter->second.start_time.tv_sec) > mon_iter->second.duration){
        VLOG_DBG(lg, "Monitor Expired erasing %"PRIx32"",mon_iter->first);
        remove_monitor_flow(mon_iter->first);
        continue;
      }
      VLOG_DBG(lg,"Probing dpid %"PRIx64" for path id %"PRIx16"", mon_iter->second.dpid.as_host(), mon_iter->second.pid);
      send_flow_stat_req(mon_iter->second.dpid, mon_iter->second.match);
    }
    post(boost::bind(&linkload::flow_stat_probe, this), get_next_monitor_time());
  }

  float linkload::get_link_load_ratio(datapathid dpid, uint16_t port, bool tx)
  {
    bool found = false;
    bool& fRef = found;
    uint32_t speed = dpmem->get_link_speed(dpid, port);
    linkload::load ll  = get_link_load(dpid,port,fRef);
    if (speed == 0 || ll.interval == 0)
      return -1;
 
    if (tx)
      return (float) ((double) ll.txLoad*8)/ \
	(((double) dpmem->get_link_speed(dpid, port))*1000*\
	 ((double) ll.interval));
    else
      return (float) ((double) ll.rxLoad*8)/ \
	(((double) dpmem->get_link_speed(dpid, port))*1000*\
	 ((double) ll.interval));
  }

  linkload::load linkload::get_link_load(datapathid dpid, uint16_t port, bool& found)
  {
    hash_map<switchport, load>::iterator i = \
      loadmap.find(switchport(dpid, port));
    if (i == loadmap.end()){
      VLOG_DBG(lg,"Port %d not found for load request",port);
      found = false;
      return load(0,0,0);
    }
    else
    	found = true;
    	return i->second;
  }
  
  linkload::flow_load linkload::get_flow_load(MonitorID_t mid, int& error){
    hash_map<MonitorID_t, flow_load>::iterator i = flowLoadMap.find(mid);
    hash_map<MonitorID_t, monitorInfo>::iterator existEntry = monitoredFlows.find(mid);
    
    if (existEntry == monitoredFlows.end()){
      /* Unknown monitor ID */
      error = 1;
      return flow_load(0,0,0);  
    }  
    if (i == flowLoadMap.end()){
      error = 2;
      /* No stats available */
      VLOG_DBG(lg,"Monitored resource %"PRIx32" not found for load request",mid);
      return flow_load(0,0,0);
    }
    else{
      error = 0;
      return i->second;
    }
  }

  linkload::ErrorsPerSecond linkload::get_port_errors(datapathid dpid, uint16_t port, bool& found)
  {
    hash_map<switchport, ErrorsPerSecond>::iterator i = \
      errorMap.find(switchport(dpid, port));
    if (i == errorMap.end()){
    	found = false;
      VLOG_DBG(lg,"Port %d not found for errors request",port);
      return ErrorsPerSecond(0,0,0,0,0,0,0,0,0);
    }
    else
      found = true;
      return i->second;
  }

  void linkload::send_stat_req(const datapathid& dpid, uint16_t port)
  {
//    VLOG_DBG(lg, "Sending request to port number: %d", port);
    size_t size = sizeof(ofp_stats_request)+sizeof(ofp_port_stats_request);
    of_raw.reset(new uint8_t[size]);
    /* UoR */
    of_stats_request osr(openflow_pack::header(OFPT_STATS_REQUEST, size),
			 OFPST_PORT, 1); 
    
    //of_stats_request osr(openflow_pack::header(OFPT_STATS_REQUEST, size),
    //			 OFPST_PORT, 0); 
    of_port_stats_request opsr;
    opsr.port_no = OFPP_NONE;

    osr.pack((ofp_stats_request*) openflow_pack::get_pointer(of_raw));
    opsr.pack((ofp_port_stats_request*) openflow_pack::get_pointer(of_raw, sizeof(ofp_stats_request)));

    send_openflow_command(dpi->second.datapath_id,of_raw, false);
  }
  void linkload::send_flow_stat_req(const datapathid& dpid, ofp_match* om){
    VLOG_DBG(lg,"Sending flow stat request");
    
    size_t size = sizeof(ofp_stats_request)+sizeof(ofp_flow_stats_request);
    of_raw.reset(new uint8_t[size]);
      
    of_stats_request osr(openflow_pack::header(OFPT_STATS_REQUEST, size),
		 OFPST_FLOW, 0);
			 
    of_flow_stats_request ofsr;
    ofsr.table_id = 0xff;
    ofsr.out_port = OFPP_NONE;
    
    if (om != NULL){
        ofsr.match.in_port = om->in_port;
        ofsr.match.dl_vlan = om->dl_vlan;
        
	ofsr.match.nw_src = ntohl(om->nw_src);
	ofsr.match.nw_dst = ntohl(om->nw_dst);
	
	ofsr.match.dl_type = ntohs(om->dl_type);

	ofsr.match.nw_proto = om->nw_proto;
	ofsr.match.tp_src = om->tp_src;
	ofsr.match.tp_dst = om->tp_dst;
	
	ofsr.match.wildcards = ntohl(om->wildcards);
    }
    
    osr.pack((ofp_stats_request*) openflow_pack::get_pointer(of_raw));
    ofsr.pack((ofp_flow_stats_request*) openflow_pack::get_pointer(of_raw, sizeof(ofp_stats_request)));

    send_openflow_command(dpid,of_raw, false);
    
  }

  timeval linkload::get_next_time()
  {
    timeval tv = {0,0};
    if (dpmem->dp_events.size() == 0)
      tv.tv_sec = load_interval;
    else
      tv.tv_sec = load_interval/dpmem->dp_events.size();

    if (tv.tv_sec == 0)
      tv.tv_sec = 1;

    return tv;
  }
  timeval linkload::get_next_monitor_time()
  {
    timeval tv = {0,0};
    tv.tv_sec = flow_interval;
    return tv;
  }

  Disposition linkload::handle_port_event(const Event& e)
  {
    const Port_status_event& pse = assert_cast<const Port_status_event&>(e);

    hash_map<switchport, Port_stats>::iterator swstat = \
      statmap.find(switchport(pse.datapath_id, pse.port.port_no));
    if (swstat != statmap.end() &&
	pse.reason == OFPPR_DELETE)
      statmap.erase(swstat);

    hash_map<switchport, load>::iterator swload = \
      loadmap.find(switchport(pse.datapath_id, pse.port.port_no));
    if (swload != loadmap.end() &&
	pse.reason == OFPPR_DELETE)
      loadmap.erase(swload);

    /**
     * Andi: erase even errors of port too, when ports changes status
     */
    hash_map<switchport, ErrorsPerSecond>::iterator swErrors = \
      errorMap.find(switchport(pse.datapath_id, pse.port.port_no));
    if (swErrors != errorMap.end() &&
	pse.reason == OFPPR_DELETE)
      errorMap.erase(swErrors);

    return CONTINUE;
  }
  
  Disposition linkload::handle_path_event(const Event& e){
    const Path_event& pe = assert_cast<const Path_event&>(e);

    //VLOG_DBG(lg,"Received path event with path id %"PRIx16"",pe.pathID);
    
    if (pe.action == Path_event::ADD){
        VLOG_DBG(lg,"Adding pathID %"PRIx16" to pathMatches structure",pe.pathID);
        pathCookies[pe.pathID] = pe.cookies;
        pathMatches[pe.pathID] = pe.matchesPtr;
    }
    else{
        //VLOG_DBG(lg,"Erasing entry of pathID %"PRIx16"",pe.pathID);
        uint64_t counter = 0;
        //deallocate matches...
        hash_map<PathID_t, MatchesPtr_t>::iterator paths = pathMatches.find(pe.pathID);
        if (paths != pathMatches.end()){
            hash_map<datapathid, ofp_match*>::iterator matches = paths->second->begin();
            //VLOG_DBG(lg,"Size is %d",paths->second->size());
            while(counter < paths->second->size()){
                VLOG_DBG(lg, "Provando di cancellare un match con %"PRIx32" in dpid: %"PRIx64"",matches->second->nw_dst, matches->first.as_host());
                delete matches->second;
                matches++;
                counter++;
            }
        }
        
        pathMatches.erase(pe.pathID);
		pathCookies.erase(pe.pathID);
        // erase all monitor ID s that refer to this path.
        hash_map<MonitorID_t, monitorInfo>::iterator mids = monitoredFlows.begin();
        while (mids != monitoredFlows.end()){
            if (mids->second.pid == pe.pathID)
                mids = monitoredFlows.erase(mids);
            else
                mids++;
        }
    }
    
    return CONTINUE;
  }

  Disposition linkload::handle_dp_leave(const Event& e)
  {
    const Datapath_leave_event& dle = assert_cast<const Datapath_leave_event&>(e);

    hash_map<switchport, Port_stats>::iterator swstat = statmap.begin();
    while (swstat != statmap.end())
    {
      if (swstat->first.dpid == dle.datapath_id)
	swstat = statmap.erase(swstat);
      else
	swstat++;
    }

    hash_map<switchport, load>::iterator swload = loadmap.begin();
    while (swload != loadmap.end())
    {
      if (swload->first.dpid == dle.datapath_id)
	swload = loadmap.erase(swload);
      else
	swload++;
    }

    /**
     * Andi: Erase all errors statistics of all ports of the datapath that just left.
     */
    hash_map<switchport, ErrorsPerSecond>::iterator swError = errorMap.begin();
    while (swError != errorMap.end())
    {
      if (swError->first.dpid == dle.datapath_id)
	swError = errorMap.erase(swError);
      else
	swError++;
    }
    //TODO remove also other monitor resources connected to this dp


    return CONTINUE;
  }


  Disposition linkload::handle_port_stats(const Event& e)
  {
    const Port_stats_in_event& psie = assert_cast<const Port_stats_in_event&>(e);

    vector<Port_stats>::const_iterator i = psie.ports.begin();
    while (i != psie.ports.end())
    {
      hash_map<switchport, Port_stats>::iterator swstat = \
	statmap.find(switchport(psie.datapath_id, i->port_no));
      if (swstat != statmap.end())
      {
	updateLoad(psie.datapath_id, i->port_no,
		   swstat->second, *i);
        updateErrors(psie.datapath_id, i->port_no,
                   swstat->second, *i);
	statmap.erase(swstat);
      }
      statmap.insert(make_pair(switchport(psie.datapath_id, i->port_no),
			       Port_stats(*i)));
      i++;
    }

    return CONTINUE;
  }
  Disposition linkload::handle_flow_stats(const Event& e)
  {
    const Flow_stats_in_event& fsie = assert_cast<const Flow_stats_in_event&>(e);
    vector<Flow_stats>::const_iterator i = fsie.flows.begin();
    uint32_t mid;
//    uint16_t pathID;
    
    while (i != fsie.flows.end())
    {
      VLOG_DBG(lg,"Received statistics from datapath id %"PRIu64" with cookie %"PRIx64"",fsie.datapath_id.as_host(), htonll(i->cookie));
      VLOG_DBG(lg,"TX packet %"PRIx64" and TX bytes are %"PRIx64"", i->packet_count, i->byte_count);
      
      
      /* here we need hash of dpid and cookie */
      mid = calculate_monitorID(fsie.datapath_id, htonll(i->cookie));
      VLOG_DBG(lg, "Searching for MonitorID %"PRIx32"",mid);

      hash_map<MonitorID_t, Flow_stats>::iterator flowStat = flowStatMap.find(mid);
      
      if (flowStat != flowStatMap.end())
      {
	updateFlowLoad(mid,flowStat->second,*i);
        
	flowStatMap.erase(mid);
      }
      flowStatMap.insert(make_pair(mid, Flow_stats(*i)));
      i++;
    }
    return CONTINUE;
  }
  uint32_t linkload::create_monitor_flow(PathID_t pathID, datapathid dpid, uint64_t duration, uint64_t frequency, int& error){
      hash_map<PathID_t, CookiesPtr_t>::iterator pathCookieIt = pathCookies.find(pathID);
      hash_map<PathID_t, MatchesPtr_t>::iterator pathMatchIt  = pathMatches.find(pathID);
      uint32_t mid;
      
      if (pathCookieIt != pathCookies.end() && pathMatchIt != pathMatches.end()){
          hash_map<datapathid, Cookie_t>::iterator dpIterator = pathCookieIt->second->find(dpid);
          hash_map<datapathid, ofp_match*>::iterator matchesIterator = pathMatchIt->second->find(dpid);
          
          if (dpIterator != pathCookieIt->second->end() && matchesIterator != pathMatchIt->second->end()){
              monitorInfo monInfo;
              monInfo.pid = pathID;
              monInfo.dpid = dpid;
              monInfo.match = matchesIterator->second;
//              monInfo.match = dpIterator->second;
              
              timeval curtime = { 0, 0 };
              gettimeofday(&curtime, NULL);
              
              monInfo.start_time = curtime;
              monInfo.duration = duration;
              monInfo.frequency = frequency;
              
//              cookie = get_cookie_for_match(dpIterator->second);
              monInfo.cookie = dpIterator->second;
              VLOG_DBG(lg,"Generated cookie %"PRIx64"",monInfo.cookie);
              
              mid = calculate_monitorID(dpid,dpIterator->second);
//              VLOG_DBG(lg,"Monitor id produced is %"PRIu32"",mid);
              monitoredFlows[mid] = monInfo;
              error = 0;
              
              return mid;
          }
          else{
              /* Wrong Datapath ID */
              error = 2;
          }          
      }
      else{
          /* wrong Path ID */
          error = 1;
      }
      return 0;      
  }
  int linkload::remove_monitor_flow(uint32_t mid){
      hash_map<MonitorID_t, monitorInfo>::iterator mids = monitoredFlows.find(mid);
      if (mids == monitoredFlows.end())
      {
          return 1;
      }
      else{
          monitoredFlows.erase(mids);
          
          hash_map<MonitorID_t, Flow_stats>::iterator i = flowStatMap.find(mid);
          hash_map<MonitorID_t, flow_load>::iterator j = flowLoadMap.find(mid);
          
          if (i != flowStatMap.end())
              flowStatMap.erase(i);
          
          if (j != flowLoadMap.end())
              flowLoadMap.erase(j);      
          return 0;
      }
  }
  linkload::Cookie_t
  linkload::get_cookie_for_match(ofp_match* match){
              /* Even cookie is needed */
      Flow* flow = new Flow();
      flow->in_port 	= match->in_port;
      flow->nw_src 	= match->nw_src;
      flow->nw_dst 	= match->nw_dst;
      flow->dl_type	= htons(0x0800);
      flow->nw_proto    = match->nw_proto;
      flow->tp_src      = htons(match->tp_src);
      flow->tp_dst      = htons(match->tp_dst);
      return htonll(flow->hash_code());
  } 
  uint32_t linkload::calculate_monitorID(datapathid dpid, Cookie_t cookie) {

	uint16_t length = 0;
	char* buffer;
	uint8_t counter = 0;
	unsigned int hash;

	length += sizeof(uint64_t);
	length += sizeof(uint64_t);

	buffer = new char[length];

	for(int i = 0;i < length;i++)
		buffer[i] = 0;

	counter += serialize_uint64(dpid.as_host(),&buffer[counter],sizeof(uint64_t));
	counter += serialize_uint64(cookie,&buffer[counter],sizeof(uint64_t));
	
	hash = fnv_32_buf((void*)buffer, (unsigned int)length, FNV1_32_INIT);
	//VLOG_DBG(lg,"Created hash %"PRIu32"",hash);
	return hash;
}

  void linkload::updateFlowLoad(MonitorID_t mid, const Flow_stats& oldstat, const Flow_stats& newstat){
  
      hash_map<MonitorID_t, flow_load>::iterator fi = flowLoadMap.find(mid);
      
      if (fi != flowLoadMap.end())
          flowLoadMap.erase(mid);
          
      //VLOG_DBG(lg,"Inserting entry with mid %"PRIx32"",mid);
      flowLoadMap.insert(make_pair(mid, 
                                   flow_load(ntohll(newstat.packet_count - oldstat.packet_count),
                                             ntohll(newstat.byte_count - oldstat.byte_count),
                                             flow_interval)));
  }
  
  void linkload::updateLoad(const datapathid& dpid, uint16_t port,
			    const Port_stats& oldstat,
			    const Port_stats& newstat)
  {
    hash_map<switchport, load>::iterator i = \
      loadmap.find(switchport(dpid, port));
    
    if (i != loadmap.end())
      loadmap.erase(i);

    loadmap.insert(make_pair(switchport(dpid, port),
			     load(newstat.tx_bytes-oldstat.tx_bytes,
				  newstat.rx_bytes-oldstat.rx_bytes,
				  load_interval)));
  }

  void linkload::updateErrors(const datapathid& dpid, uint16_t port,
			    const Port_stats& oldstat,
			    const Port_stats& newstat)
  {
    hash_map<switchport, ErrorsPerSecond>::iterator i = \
      errorMap.find(switchport(dpid, port));

    if (i != errorMap.end())
      errorMap.erase(i);

    errorMap.insert(make_pair(switchport(dpid, port),
			     ErrorsPerSecond(newstat.rx_dropped-oldstat.rx_dropped,
			    		 newstat.tx_dropped - oldstat.tx_dropped,
			    		 newstat.rx_errors - oldstat.rx_errors,
			    		 newstat.tx_errors - oldstat.tx_errors,
			    		 newstat.rx_frame_err - oldstat.rx_frame_err,
			    		 newstat.rx_over_err - oldstat.rx_over_err,
			    		 newstat.rx_crc_err - oldstat.rx_crc_err,
			    		 newstat.collisions - oldstat.collisions,
			    		 load_interval)));
  }

  void linkload::getInstance(const Context* c,
				  linkload*& component)
  {
    component = dynamic_cast<linkload*>
      (c->get_by_interface(container::Interface_description
			      (typeid(linkload).name())));
  }

  REGISTER_COMPONENT(Simple_component_factory<linkload>,
		     linkload);
} // vigil namespace
