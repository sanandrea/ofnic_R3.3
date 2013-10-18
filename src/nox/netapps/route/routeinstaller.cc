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
#include "routeinstaller.hh"
#include "openflow-default.hh"
#include "vlog.hh"
#include "assert.hh"
#include <boost/bind.hpp>
/*UoR*/
#include "netinet++/ipaddr.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ip.hh"
#include "openflow/openflow/openflow.h"

using namespace std;

namespace vigil 
{
  using namespace vigil::container;
  static Vlog_module lg("routeinstaller");
  
  void routeinstaller::configure(const Configuration*) 
  {
    resolve(routing);
  }
  
  void routeinstaller::install() 
  {
  }

  bool routeinstaller::get_shortest_path(std::list<network::termination> dst,
					 network::route& route)
  {
    std::list<network::termination>::iterator i = dst.begin();
    if (i == dst.end())
      return false;
 
    if (!get_shortest_path(*i, route))
    {
      return false;
    }

    i++;
    while (i != dst.end())
    {
      network::route r2(route);
      if (!get_shortest_path(*i, r2))
	return false;

      merge_route(&route, &r2);
      i++;
    }

    return true;
  }


  bool routeinstaller::get_shortest_path(network::termination dst,
					 network::route& route)
  {
    Routing_module::RoutePtr sroute;
    Routing_module::RouteId id;
    id.src = route.in_switch_port.dpid;
    id.dst = dst.dpid;

    if (!routing->get_route(id, sroute))
      return false;

    route2tree(dst, sroute, route);
    return true;
  }

  void routeinstaller::route2tree(network::termination dst,
				  Routing_module::RoutePtr sroute,
				  network::route& route)
  {
    route.next_hops.clear();
    network::hop* currHop = &route;
    std::list<Routing_module::Link>::iterator i = sroute->path.begin();
    while (i != sroute->path.end())
    {
      network::hop* nhop = new network::hop(i->dst, i->inport);
      currHop->next_hops.push_front(std::make_pair(i->outport, nhop));
      VLOG_DBG(lg, "Pushing hop from %"PRIx64" from in port %"PRIx16" to out port %"PRIx16"",
      	       i->dst.as_host(), i->inport, i->outport);
      currHop = nhop;
      i++;
    }
    network::hop* nhop = new network::hop(datapathid(), 0);
    currHop->next_hops.push_front(std::make_pair(dst.port, nhop));
    VLOG_DBG(lg, "Pushing last hop to out port %"PRIx16"", dst.port);
    
  }

  void routeinstaller::install_route(const Flow& flow, network::route route, 
				     uint32_t buffer_id, uint32_t wildcards,
				     uint16_t idletime, uint16_t hardtime)
  {
    hash_map<datapathid,ofp_action_list> act;
    list<datapathid> dplist;
    install_route(flow, route, buffer_id, act, dplist, wildcards,
		  idletime, hardtime);
  }

  void routeinstaller::install_route(const Flow& flow, network::route route, 
				     uint32_t buffer_id,
				     hash_map<datapathid,ofp_action_list>& actions,
				     list<datapathid>& skipoutput,
				     uint32_t wildcards,
				     uint16_t idletime, uint16_t hardtime)
  {
    real_install_route(flow, route, buffer_id, actions, skipoutput, wildcards, 
		       idletime, hardtime);  
  }

  void routeinstaller::real_install_route(const Flow& flow, network::route route, 
					  uint32_t buffer_id,
					  hash_map<datapathid,ofp_action_list>& actions,
					  list<datapathid>& skipoutput,
					  uint32_t wildcards,
					  uint16_t idletime, uint16_t hardtime)
  {
    if (route.in_switch_port.dpid.empty())
      return;
    
    //Recursively install 
    network::nextHops::iterator i = route.next_hops.begin();
    while (i != route.next_hops.end())
    {
      if (!(i->second->in_switch_port.dpid.empty()))
	real_install_route(flow, *(i->second), buffer_id, actions, skipoutput,
			   wildcards, idletime, hardtime);
      i++;
    }

    //Check for auxiliary actions
    hash_map<datapathid,ofp_action_list>::iterator j = \
      actions.find(route.in_switch_port.dpid);
    ofp_action_list oalobj;
    ofp_action_list* oal;
    if (j == actions.end())
      oal = &oalobj;
    else
      oal = &(j->second);

    //Add forwarding actions
    list<datapathid>::iterator k = skipoutput.begin();
    while (k != skipoutput.end())
    {
      if (*k == route.in_switch_port.dpid)
	break;
      k++;
    }
    if (k == skipoutput.end())
    {
      i = route.next_hops.begin();
      while (i != route.next_hops.end())
      {
	ofp_action* ofpa = new ofp_action();
	ofpa->set_action_output(i->first, 0);
	oal->action_list.push_back(*ofpa);
	i++;
      }
    }

    //Install flow entry
    install_flow_entry(route.in_switch_port.dpid, flow, buffer_id, 
		       route.in_switch_port.port, *oal,
		       wildcards, idletime, hardtime);
  }
  
  void routeinstaller::install_flow_entry(const datapathid& dpid,
					  const Flow& flow, uint32_t buffer_id, uint16_t in_port,
					  ofp_action_list act_list, uint32_t wildcards_,
					  uint16_t idletime, uint16_t hardtime, uint64_t cookie)
  {
    ssize_t size = sizeof(ofp_flow_mod)+act_list.mem_size();
    of_raw.reset(new uint8_t[size]);
    of_flow_mod ofm;
    ofm.header = openflow_pack::header(OFPT_FLOW_MOD, size);
    ofm.match = flow.get_exact_match();
    ofm.match.in_port = in_port;
    ofm.match.wildcards = wildcards_;
    ofm.cookie = cookie;
    ofm.command = OFPFC_ADD;
    ofm.flags = ofd_flow_mod_flags();
    ofm.idle_timeout = idletime;
    ofm.hard_timeout = hardtime;
    ofm.buffer_id = buffer_id;
    ofm.out_port = OFPP_NONE;
    ofm.pack((ofp_flow_mod*) openflow_pack::get_pointer(of_raw));
    act_list.pack(openflow_pack::get_pointer(of_raw,sizeof(ofp_flow_mod)));
    VLOG_DBG(lg,"Install flow entry %s with %zu actions", 
	     flow.to_string().c_str(), act_list.action_list.size());
    send_openflow_command(dpid, of_raw, false);

    ofm.command = OFPFC_MODIFY_STRICT;
    ofm.pack((ofp_flow_mod*) openflow_pack::get_pointer(of_raw));
    send_openflow_command(dpid, of_raw, false);
  }

  void routeinstaller::merge_route(network::route* tree, network::route* route)
  {
    network::nextHops::iterator routeOut = route->next_hops.begin();
    network::nextHops::iterator i = tree->next_hops.begin();
    while (i != tree->next_hops.end())
    {
      if ((i->first == routeOut->first) &&
	  (i->second->in_switch_port.dpid == 
	   routeOut->second->in_switch_port.dpid))
      {
	merge_route(i->second, routeOut->second);
	return;
      }
      i++;
    }
    
    if (routeOut != route->next_hops.end())
      tree->next_hops.push_front(*routeOut);
  }

  void routeinstaller::getInstance(const container::Context* ctxt, 
				   vigil::routeinstaller*& scpa)
  {
    scpa = dynamic_cast<routeinstaller*>
      (ctxt->get_by_interface(container::Interface_description
			      (typeid(routeinstaller).name())));
  }


    /**
     * UoR
     * Installs on the route along sdpid and ddpid, a flow identified only by source and dest ip
    */
     std::string routeinstaller::netic_install_route(uint16_t first_port,uint16_t last_port,datapathid sdpid,datapathid ddpid,std::string sip,std::string dip){


    		network::route rte(sdpid,first_port);
		network::termination endpt(ddpid, last_port);

    		network::route inverse_rte(ddpid,first_port);
		network::termination inverse_endpt(sdpid, last_port);

		if (get_shortest_path(endpt, rte) && get_shortest_path(inverse_endpt,inverse_rte)){

                     routeinstaller::netic_install_on_route(&rte,sip,dip); //install first diresction
                     routeinstaller::netic_install_on_route(&inverse_rte,dip,sip); //install reverse direction
                     return "Path installato";
                   }
  
    return "Path inesistente";

      }

    /**
     * UoR
     * Returns a buffer for a OpenFlow message, with all fields wildcarded except for source and destination IP addresses
    */
     boost::shared_array<uint8_t> routeinstaller::netic_get_of_message(uint16_t inport,uint16_t outport,std::string sip,std::string dip){

	        boost::shared_array<uint8_t> msg_buffer;
		ipaddr sipa(sip);	

                ofp_action_list act_list;
    		ofp_action* normal_action = new ofp_action();
		normal_action->set_action_output(outport,0);
		act_list.action_list.push_back(*normal_action);
			     
		ofp_match tmpmatch;
  tmpmatch.wildcards=OFPFW_DL_VLAN|OFPFW_DL_SRC|OFPFW_DL_DST|OFPFW_NW_PROTO|OFPFW_TP_SRC|OFPFW_TP_DST|OFPFW_NW_DST_ALL| OFPFW_DL_VLAN_PCP|OFPFW_NW_TOS;
                tmpmatch.nw_src=sipa.addr;
                tmpmatch.dl_type=0x800;
                tmpmatch.in_port=inport;
        
        ssize_t size = sizeof(ofp_flow_mod)+act_list.mem_size();
        msg_buffer.reset(new uint8_t[size]);
        of_flow_mod ofm;
        ofm.header = openflow_pack::header(OFPT_FLOW_MOD, size);
        ofm.cookie = 0;
        ofm.command = OFPFC_ADD;
        ofm.flags = ofd_flow_mod_flags();
        memcpy(&ofm.match, &tmpmatch, sizeof(ofp_match));
        ofm.idle_timeout = 50;
        ofm.hard_timeout = 0;
        ofm.buffer_id = 0;
        ofm.out_port = OFPP_NONE;
    

        ofm.pack((ofp_flow_mod*) openflow_pack::get_pointer(msg_buffer));
        act_list.pack(openflow_pack::get_pointer(msg_buffer,sizeof(ofp_flow_mod)));
        return msg_buffer;
        }


    /**
     * UoR
     * Installs recursively the flow specified by sip and dip in a route identified by
    */
     void routeinstaller::netic_install_on_route(network::route* route,std::string sip,std::string dip){
     
     if(route->next_hops.begin()->second->in_switch_port.dpid.empty()){ //Last node of the path
           
        boost::shared_array<uint8_t> buf=routeinstaller::netic_get_of_message(route->in_switch_port.port,route->next_hops.begin()->first,sip,dip);
        send_openflow_command(route->in_switch_port.dpid, buf, false); 
       
//           VLOG_DBG(lg,"Action installed in node: %d in port: %d out port: %d",route->in_switch_port.dpid.as_host(),
//                    route->in_switch_port.port,route->next_hops.begin()->first);
        return;
     } 

     else{ //Intermediate node on the path
       routeinstaller::netic_install_on_route(route->next_hops.begin()->second, sip, dip);

        boost::shared_array<uint8_t> buf2=routeinstaller::netic_get_of_message(route->in_switch_port.port,route->next_hops.begin()->first,sip,dip);
        send_openflow_command(route->in_switch_port.dpid, buf2, false); 

//         VLOG_DBG(lg,"Action installed in node: %d in port: %d out port: %d",route->in_switch_port.dpid.as_host(),
//                   route->in_switch_port.port, route->next_hops.begin()->first);
        return;
     }


     }
  
} // namespace vigil

namespace {
  REGISTER_COMPONENT(vigil::container::Simple_component_factory<vigil::routeinstaller>, 
		     vigil::routeinstaller);
} // unnamed namespace
