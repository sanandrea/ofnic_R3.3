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

#include "pytopology.hh"

#include "pyrt/pycontext.hh"
#include "swigpyrun.h"
#include "vlog.hh"

using namespace std;
using namespace vigil;
using namespace vigil::applications;

namespace {
Vlog_module lg("pytopology");
}

pytopology_proxy::pytopology_proxy(PyObject* ctxt)
{
    if (!SWIG_Python_GetSwigThis(ctxt) || !SWIG_Python_GetSwigThis(ctxt)->ptr) {
        throw runtime_error("Unable to access Python context.");
    }
    
    c = ((PyContext*)SWIG_Python_GetSwigThis(ctxt)->ptr)->c;
}

void
pytopology_proxy::configure(PyObject* configuration) 
{
    c->resolve(topo);    
}

void 
pytopology_proxy::install(PyObject*) 
{
}

bool 
pytopology_proxy::is_internal(datapathid dp, uint16_t port) const
{
    return topo->is_internal(dp, port);
}

std::list<PyLinkPorts> 
pytopology_proxy::get_outlinks(datapathid dpsrc, datapathid dpdst) const
{
    std::list<PyLinkPorts> returnme;
    std::list<Topology::LinkPorts>  list;

    list =  topo->get_outlinks(dpsrc, dpdst);
    for(std::list<Topology::LinkPorts>::iterator i = list.begin(); 
            i != list.end(); ++i){
        returnme.push_back(PyLinkPorts((*i).src,(*i).dst));
    }
    return returnme; 
}

/**
* Added by Andrea Simeoni
*/
hash_map<datapathid,Topology::DpInfo> pytopology_proxy::get_whole_topology(){

	return topo->get_whole_topology();
	
}

    /** \added by Andrea Simeoni--Returns the whole topology data structure
    */
    DatapathFeatures pytopology_proxy::get_dpinfo(datapathid dp) const{
		DatapathFeatures df;
		Topology::DpInfo dpinfo=topo->get_dpinfo(dp);
		//df.state=dpinfo.state;
		//df.dpid=dp;
		return df;
	}

    /**UoR
    */

    std::list<PyNodeInfo> pytopology_proxy::synchronize_network() {

    	std::list<Topology::NodeInfo> nodelist = topo->get_node_list();
    	std::list<Topology::NodeInfo>::iterator it=nodelist.begin();

    	std::list<PyNodeInfo> ret;
    
    	while(it!=nodelist.end()){
			ret.push_back(PyNodeInfo((*it).dpid.as_host(),(*it).active));
		   	it++;	
	  	}
       	return ret;
	}

    /** UoR
   */
   
    std::list<PyPortInfo> pytopology_proxy::synchronize_node(datapathid dp){
    	std::list<Topology::PortInfo> portlist=topo->get_port_list(dp);
		std::list<Topology::PortInfo>::iterator it=portlist.begin();
		std::list<PyPortInfo> ret;

		
		while(it!=portlist.end()){
	
			PyPortInfo ppi;
			ppi.name=(*it).name;
			ppi.state=(*it).state;
			ppi.speed=(*it).speed;
			ppi.medium=(*it).medium;
			ppi.duplexity=(*it).duplexity;
			ret.push_back(ppi);	
			it++;	
		}
		
	 	return ret;
	}

    /** UoR
   */
   
    PyPortInfo pytopology_proxy::synchronize_port(datapathid id, std::string pname){

		Topology::PortInfo pinfo=topo->get_port_info(id, pname);
		PyPortInfo ret;
		ret.name=pinfo.name;
		ret.state=pinfo.state;
		ret.speed=pinfo.speed;
		ret.medium=pinfo.medium;
		ret.duplexity=pinfo.duplexity;
		ret.portID = pinfo.portID;
		return ret;
	
	}

   /** UoR
   */
    
    bool pytopology_proxy::is_valid_dp(datapathid dp){
	
	return topo->is_valid_datapath(dp);

	}

    bool pytopology_proxy::is_valid_port(datapathid dp, std::string portName){
    	return topo->is_valid_port(dp, portName);
    }






