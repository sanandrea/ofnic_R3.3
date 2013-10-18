/* Copyright 2008 (C) Nicira, Inc.
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

#include "topology.hh"

#include <boost/bind.hpp>
#include <inttypes.h>

#include "assert.hh"
#include "datapath-join.hh"
#include "datapath-leave.hh"
#include "port-status.hh"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;
using namespace vigil::container;

namespace vigil {
namespace applications {

static Vlog_module lg("topology");

Topology::Topology(const Context* c,
                   const json_object*)
    : Component(c)
{
    empty_dp.active = false;

        // For bebugging
        // Link_event le;
        // le.action = Link_event::ADD;
        // le.dpsrc = datapathid::from_host(0);
        // le.dpdst = datapathid::from_host(1);
        // le.sport = 0;
        // le.dport = 0;
        // add_link(le);
}

void
Topology::getInstance(const container::Context* ctxt, Topology*& t)
{
    t = dynamic_cast<Topology*>
        (ctxt->get_by_interface(container::Interface_description
                                (typeid(Topology).name())));
}

void
Topology::configure(const Configuration*)
{
    register_handler<Link_event>
        (boost::bind(&Topology::handle_link_event, this, _1));
    register_handler<Datapath_join_event>
        (boost::bind(&Topology::handle_datapath_join, this, _1));
    register_handler<Datapath_leave_event>
        (boost::bind(&Topology::handle_datapath_leave, this, _1));
    register_handler<Port_status_event>
        (boost::bind(&Topology::handle_port_status, this, _1));
}

void
Topology::install()
{}

const Topology::DpInfo&
Topology::get_dpinfo(const datapathid& dp) const
{
    NetworkLinkMap::const_iterator nlm_iter = topology.find(dp);

    if (nlm_iter == topology.end()) {
        return empty_dp;
    }

    return nlm_iter->second;
}

const Topology::DatapathLinkMap&
Topology::get_outlinks(const datapathid& dpsrc) const
{
    NetworkLinkMap::const_iterator nlm_iter = topology.find(dpsrc);

    if (nlm_iter == topology.end()) {
        return empty_dp.outlinks;
    }

    return nlm_iter->second.outlinks;
}


const Topology::LinkSet&
Topology::get_outlinks(const datapathid& dpsrc, const datapathid& dpdst) const
{
    NetworkLinkMap::const_iterator nlm_iter = topology.find(dpsrc);

    if (nlm_iter == topology.end()) {
        return empty_link_set;
    }

    DatapathLinkMap::const_iterator dlm_iter = nlm_iter->second.outlinks.find(dpdst);
    if (dlm_iter == nlm_iter->second.outlinks.end()) {
        return empty_link_set;
    }

    return dlm_iter->second;
}


bool
Topology::is_internal(const datapathid& dp, uint16_t port) const
{
    NetworkLinkMap::const_iterator nlm_iter = topology.find(dp);

    if (nlm_iter == topology.end()) {
        return false;
    }

    PortMap::const_iterator pm_iter = nlm_iter->second.internal.find(port);
    return (pm_iter != nlm_iter->second.internal.end());
}
/**
* Added by Andrea Simeoni
*/
hash_map<datapathid,Topology::DpInfo> Topology::get_whole_topology(){

	hash_map<datapathid,DpInfo> topo=Topology::topology;
	return topo;
}

Disposition
Topology::handle_datapath_join(const Event& e)
{
    const Datapath_join_event& dj = assert_cast<const Datapath_join_event&>(e);
    NetworkLinkMap::iterator nlm_iter = topology.find(dj.datapath_id);

    if (nlm_iter == topology.end()) {
        nlm_iter = topology.insert(std::make_pair(dj.datapath_id,
                                                  DpInfo())).first;
    }

    nlm_iter->second.active = true;
    nlm_iter->second.ports = dj.ports;
    return CONTINUE;
}

Disposition
Topology::handle_datapath_leave(const Event& e)
{
    const Datapath_leave_event& dl = assert_cast<const Datapath_leave_event&>(e);
    NetworkLinkMap::iterator nlm_iter = topology.find(dl.datapath_id);

    if (nlm_iter != topology.end()) {
        if (!(nlm_iter->second.internal.empty()
              && nlm_iter->second.outlinks.empty()))
        {
            nlm_iter->second.active = false;
            nlm_iter->second.ports.clear();
        } else {
            topology.erase(nlm_iter);
        }
    } else {
        VLOG_ERR(lg, "Received datapath_leave for non-existing dp %"PRIx64".",
                 dl.datapath_id.as_host());
    }
    return CONTINUE;
}

Disposition
Topology::handle_port_status(const Event& e)
{
    const Port_status_event& ps = assert_cast<const Port_status_event&>(e);

    if (ps.reason == OFPPR_DELETE) {
        delete_port(ps.datapath_id, ps.port);
    } else {
        add_port(ps.datapath_id, ps.port, ps.reason != OFPPR_ADD);
        
}

    return CONTINUE;
}

void
Topology::add_port(const datapathid& dp, const Port& port, bool mod)
{
    NetworkLinkMap::iterator nlm_iter = topology.find(dp);
    if (nlm_iter == topology.end()) {
        VLOG_WARN(lg, "Add/mod port %"PRIu16" to unknown datapath %"PRIx64" - adding default entry.",
                  port.port_no, dp.as_host());
        nlm_iter = topology.insert(std::make_pair(dp,
                                                  DpInfo())).first;
        nlm_iter->second.active = false;
        nlm_iter->second.ports.push_back(port);
        return;
    }

    for (std::vector<Port>::iterator p_iter = nlm_iter->second.ports.begin();
         p_iter != nlm_iter->second.ports.end(); ++p_iter)
    {
        if (p_iter->port_no == port.port_no) {
            if (!mod) {
                VLOG_DBG(lg, "Add known port %"PRIu16" on datapath %"PRIx64" - modifying port.",
                         port.port_no, dp.as_host());
            }
            *p_iter = port;
            return;
        }
    }

    if (mod) {
        VLOG_DBG(lg, "Mod unknown port %"PRIu16" on datapath %"PRIx64" - adding port.",
                 port.port_no, dp.as_host());
    }
    nlm_iter->second.ports.push_back(port);
}

void
Topology::delete_port(const datapathid& dp, const Port& port)
{
    NetworkLinkMap::iterator nlm_iter = topology.find(dp);
    if (nlm_iter == topology.end()) {
        VLOG_ERR(lg, "Delete port from unknown datapath %"PRIx64".",
                 dp.as_host());
        return;
    }

    for (std::vector<Port>::iterator p_iter = nlm_iter->second.ports.begin();
         p_iter != nlm_iter->second.ports.end(); ++p_iter)
    {
        if (p_iter->port_no == port.port_no) {
            nlm_iter->second.ports.erase(p_iter);
            return;
        }
    }

    VLOG_ERR(lg, "Delete unknown port %"PRIu16" from datapath %"PRIx64".",
             port.port_no, dp.as_host());
}

Disposition
Topology::handle_link_event(const Event& e)
{

    const Link_event& le = assert_cast<const Link_event&>(e);
    if (le.action == Link_event::ADD) {
        add_link(le);
    } else if (le.action == Link_event::REMOVE) {
        remove_link(le);
    } else {
        lg.err("unknown link action %u", le.action);
    }

    return CONTINUE;
}


void
Topology::add_link(const Link_event& le)
{
    NetworkLinkMap::iterator nlm_iter = topology.find(le.dpsrc);
    DatapathLinkMap::iterator dlm_iter;
    if (nlm_iter == topology.end()) {
        VLOG_WARN(lg, "Add link to unknown datapath %"PRIx64" - adding default entry.",
                  le.dpsrc.as_host());
        nlm_iter = topology.insert(std::make_pair(le.dpsrc,
                                                  DpInfo())).first;
        nlm_iter->second.active = false;
        dlm_iter = nlm_iter->second.outlinks.insert(std::make_pair(le.dpdst,
                                                                   LinkSet())).first;
    } else {
        dlm_iter = nlm_iter->second.outlinks.find(le.dpdst);
        if (dlm_iter == nlm_iter->second.outlinks.end()) {
            dlm_iter = nlm_iter->second.outlinks.insert(std::make_pair(le.dpdst,
                                                                       LinkSet())).first;
        }
    }

    LinkPorts lp = { le.sport, le.dport };
    dlm_iter->second.push_back(lp);
    add_internal(le.dpdst, le.dport);
}


void
Topology::remove_link(const Link_event& le)
{
    NetworkLinkMap::iterator nlm_iter = topology.find(le.dpsrc);
    if (nlm_iter == topology.end()) {
        lg.err("Remove link event for non-existing link %"PRIx64":%hu --> %"PRIx64":%hu (src dp)",
               le.dpsrc.as_host(), le.sport, le.dpdst.as_host(), le.dport);
        return;
    }

    DatapathLinkMap::iterator dlm_iter = nlm_iter->second.outlinks.find(le.dpdst);
    if (dlm_iter == nlm_iter->second.outlinks.end()) {
        lg.err("Remove link event for non-existing link %"PRIx64":%hu --> %"PRIx64":%hu (dst dp)",
               le.dpsrc.as_host(), le.sport, le.dpdst.as_host(), le.dport);
        return;
    }

    for (LinkSet::iterator ls_iter = dlm_iter->second.begin();
         ls_iter != dlm_iter->second.end(); ++ls_iter)
    {
        if (ls_iter->src == le.sport && ls_iter->dst == le.dport) {
            dlm_iter->second.erase(ls_iter);
            remove_internal(le.dpdst, le.dport);
            if (dlm_iter->second.empty()) {
                nlm_iter->second.outlinks.erase(dlm_iter);
                if (!nlm_iter->second.active && nlm_iter->second.ports.empty()
                    && nlm_iter->second.internal.empty()
                    && nlm_iter->second.outlinks.empty())
                {
                    topology.erase(nlm_iter);
                }
            }
            return;
        }
    }

    lg.err("Remove link event for non-existing link %"PRIx64":%hu --> %"PRIx64":%hu",
           le.dpsrc.as_host(), le.sport, le.dpdst.as_host(), le.dport);
}


void
Topology::add_internal(const datapathid& dp, uint16_t port)
{
    NetworkLinkMap::iterator nlm_iter = topology.find(dp);
    if (nlm_iter == topology.end()) {
        VLOG_WARN(lg, "Add internal to unknown datapath %"PRIx64" - adding default entry.",
                  dp.as_host());
        DpInfo& di = topology[dp] = DpInfo();
        di.active = false;
        di.internal.insert(std::make_pair(port, std::make_pair(port, 1)));
        return;
    }

    PortMap::iterator pm_iter = nlm_iter->second.internal.find(port);
    if (pm_iter == nlm_iter->second.internal.end()) {
        nlm_iter->second.internal.insert(
            std::make_pair(port, std::make_pair(port, 1)));
    } else {
        ++(pm_iter->second.second);
    }
}


    /**UoR
    */
std::list<Topology::NodeInfo> 
Topology::get_node_list() 
{
   	Topology::NetworkLinkMap::iterator it= topology.begin(); 
   	std::list<Topology::NodeInfo> output;

   	while(it!=topology.end()){
  
       	Topology::NodeInfo ni;
 	  	ni.active=it->second.active;
  	  	ni.dpid=it->first;
  		it++;
  		output.push_back(ni);
  		VLOG_DBG(lg, "datapathid %X",ni.dpid.as_host());
	}
   return output;
}

    /**UoR
    */
std::list<Topology::PortInfo> 
Topology::get_port_list(datapathid dp)
{
	Topology::NetworkLinkMap::iterator nodeiter= topology.find(dp);
	Topology::PortVector::iterator portiter= nodeiter->second.ports.begin();   
	std::list<PortInfo> ret;
	portiter++;

	while(portiter!=nodeiter->second.ports.end()){
		
	Topology::PortInfo pi;
	if (portiter->state==OFPPS_LINK_DOWN){pi.state="OFPPS_LINK_DOWN";}		
        else if (portiter->state==OFPPS_STP_LEARN ){pi.state="OFPPS_STP_LEARN ";}
	else if (portiter->state == OFPPS_STP_FORWARD){pi.state="OFPPS_STP_FORWARD";}
	else if (portiter->state ==OFPPS_STP_BLOCK  ){pi.state="OFPPS_STP_BLOCK ";}
	else if (portiter->state == OFPPS_STP_MASK ){pi.state="OFPPS_STP_MASK";}
        else if (portiter->state==OFPPS_STP_LISTEN){pi.state="OFPPS_STP_LISTEN";}
	else pi.state="UNKNOWN_STATE";

	pi.name=portiter->name;
	pi.speed=portiter->speed;
	pi.portID = portiter->port_no;
		
	string dup;
	string med;

    	VLOG_DBG(lg, "PORTA:%s",pi.name.c_str());
	if (portiter->curr & (OFPPF_10MB_FD|OFPPF_100MB_FD|OFPPF_1GB_FD|OFPPF_10GB_FD))
		{dup="FULL_DUPLEX";} 
        else if(portiter->curr & (OFPPF_10MB_HD|OFPPF_100MB_HD|OFPPF_1GB_HD)) {dup ="HALF_DUPLEX";} 
	else {dup="UNKNOWN";}
	if (portiter->curr & (OFPPF_COPPER))
		{med="COPPER";} 
                else if(portiter->curr & (OFPPF_FIBER)){med="FIBER";} 
		else{med="UNKNOWN";}
		
		pi.duplexity=dup;
		pi.medium=med;
		ret.push_back(pi);
		portiter++;	
	 }
        return ret;
	}

    /**UoR
    */
Topology::PortInfo
Topology::get_port_info(datapathid id, std::string pname)
{
	Topology::PortInfo result;
	result.name="INVALID_PORT";
	//std::list<Topology::NodeInfo> nodelist=get_node_list();
	//std::list<Topology::NodeInfo>::iterator nit=nodelist.begin();
	std::list<Topology::PortInfo> portlist = get_port_list(id);
	std::list<Topology::PortInfo>::iterator pit=portlist.begin();

	/**
	 *  Andi Old implementation which iterated through all nodes.
	 *  Now the dpid is passed as a parameter.
	 */
//	while(nit!=nodelist.end()){
//
//	    std::list<Topology::PortInfo> portlist=get_port_list(nit->dpid);
//	    pit=portlist.begin();
//	    while(pit!=portlist.end()){
//
//	        VLOG_DBG(lg,"compared port: %s",pit->name.c_str());
//	        if(pname.compare(pit->name)==0){return *pit;} //port found
//
//	        pit++;
//
//	    }
//         nit++;
//	}
	/**
	 * Andi: New Implementation
	 */

	while(pit!=portlist.end()){

		VLOG_DBG(lg,"compared port: %s %"PRIu16"",pit->name.c_str(),pit->portID);
		if(pname.compare(pit->name)==0){
			VLOG_DBG(lg,"PortID is :%d",pit->portID);
			return *pit;} //port found

		pit++;

	}
	return result;
	
	}

   /** UoR
   */
    
    bool Topology::is_valid_datapath(datapathid dp){

        return (topology.find(dp)!=topology.end());

	}

    bool Topology::is_valid_port(datapathid dpid, std::string portName){
    	std::list<Topology::PortInfo> portlist = get_port_list(dpid);
    	std::list<Topology::PortInfo>::iterator pit=portlist.begin();

    	while(pit!=portlist.end()){
    			if(portName.compare(pit->name)==0){
    				VLOG_DBG(lg,"Port found: %s",pit->name.c_str());
    				return true;
    			} //port found
    			pit++;
    		}
    	return false;
    }

void
Topology::remove_internal(const datapathid& dp, uint16_t port)
{
    NetworkLinkMap::iterator nlm_iter = topology.find(dp);
    if (nlm_iter == topology.end()) {
        lg.err("Remove internal for non-existing dp %"PRIx64":%hu",
               dp.as_host(), port);
        return;
    }

    PortMap::iterator pm_iter = nlm_iter->second.internal.find(port);
    if (pm_iter == nlm_iter->second.internal.end()) {
        lg.err("Remove internal for non-existing ap %"PRIx64":%hu.",
               dp.as_host(), port);
    } else {
        if (--(pm_iter->second.second) == 0) {
            nlm_iter->second.internal.erase(pm_iter);
            if (!nlm_iter->second.active && nlm_iter->second.ports.empty()
                && nlm_iter->second.internal.empty()
                && nlm_iter->second.outlinks.empty())
            {
                topology.erase(nlm_iter);
            }
        }
    }
}

}
}

REGISTER_COMPONENT(container::Simple_component_factory<Topology>, Topology);
