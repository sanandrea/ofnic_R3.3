/* Copyright 2008, 2009 (C) Nicira, Inc.
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

#include "pyrouting.hh"
#include "vlog.hh"
#include "swigpyrun.h"
#include "pyrt/pycontext.hh"


#include <boost/bind.hpp>
/*UoR*/
#include "netinet++/ipaddr.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ip.hh"

#include "openflow-action.hh"





namespace vigil {
namespace applications {

static Vlog_module lg("pyrouting");

PyRouting_module::PyRouting_module(PyObject* ctxt)
    : routing(0)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw std::runtime_error("Unable to access Python context.");
    }

    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
PyRouting_module::configure(PyObject* configuration)
{
    c->resolve(routing);
}

bool
PyRouting_module::get_route(Routing_module::Route& route) const
{
    Routing_module::RoutePtr rte;

    if (route.id.src == route.id.dst) {
        route.path.clear();
        return true;
    }

    if (!routing->get_route(route.id, rte)) {
        return false;
    }

    route.path = rte->path;
    return true;
}

bool
PyRouting_module::check_route(const Routing_module::Route& route,
                              uint16_t inport, uint16_t outport) const
{
    return routing->check_route(route, inport, outport);
}

bool
PyRouting_module::setup_route(const Flow& flow,
                              const Routing_module::Route& route,
                              uint16_t inport, uint16_t outport,
                              uint16_t flow_timeout,
                              const std::list<Nonowning_buffer>& bufs,
                              bool check_nat)
{
    return routing->setup_route(flow, route, inport, outport,
                                flow_timeout, bufs, check_nat,
                                NULL, NULL, NULL, NULL);
}

bool
PyRouting_module::setup_flow(const Flow& flow, const datapathid& dp,
                             uint16_t outport, uint32_t bid,
                             const Nonowning_buffer& buf,
                             uint16_t flow_timeout,
                             const Nonowning_buffer& actions, bool check_nat)
{
    return routing->setup_flow(flow, dp, outport, bid, buf, flow_timeout,
                               actions, check_nat, NULL, NULL, NULL, NULL);
}

bool
PyRouting_module::send_packet(const datapathid& dp, uint16_t inport,
                              uint16_t outport, uint32_t bid,
                              const Nonowning_buffer& buf,
                              const Nonowning_buffer& actions,
                              bool check_nat, const Flow& flow)
{
    return routing->send_packet(dp, inport, outport, bid, buf, actions,
                                check_nat, flow, NULL, NULL, NULL, NULL);
}

/**
 * UoR
 * Installs on the route along sdpid and ddpid, a flow identified only by source and dest ip
 */
PyPathResult 
PyRouting_module::netic_create_route(Netic_path_info npi,int& result){
    PyPathResult ppr;
    Routing_module::PathResult pr = routing->netic_create_route(npi,result);
    ppr.directPath = pr.directPath;
    ppr.reversPath = pr.reversPath;
    return ppr;
}

std::string
PyRouting_module::netic_get_path_list(){

    return routing->netic_get_path_list();
}

int
PyRouting_module::netic_remove_path(uint16_t path_id){
    return routing->netic_remove_path(path_id);
}   

Netic_path_desc
PyRouting_module::netic_get_path_desc(uint16_t pID,bool& found){
    Netic_path_desc npd;
    Routing_module::RoutePtr rte;
    timeval curtime = { 0, 0 };
    Routing_module::Path* p = routing->netic_get_path_desc(pID,found);
    std::stringstream ss;
    if(found == false){
        return npd;
    }
    rte = p->rte;
    
   /**
    * Andi:
    * This is a pain in the ... Python and Swig together are comploting against me!
    * When including more than one shared library Python seems to manage to pick always
    * the wrong wrapper for list iterators. So lists are marshalled in a string and then
    * re-inflated back in python. This is not elegant but is the best I can do.
    */
    std::list<Routing_module::Link>::iterator li;
    for (li = rte->path.begin();li != rte->path.end(); li++){
        if (li != rte->path.begin())
            ss << ":";
        ss << (li->dst).as_host();
        ss << ",";
        ss << li->inport;
        ss << ",";
        ss << li->outport;
    }
    npd.path = ss.str();
    
    gettimeofday(&curtime, NULL);
    npd.timeToDie = p->npi.duration - (curtime.tv_sec - p->start_time.tv_sec);
    npd.npi = p->npi;
    VLOG_DBG(lg,"s is %s",p->npi.nw_src.c_str());
    npd.rteId = rte->id;
    return npd;
}
  
} /* namespace*/
} /* namespace*/

