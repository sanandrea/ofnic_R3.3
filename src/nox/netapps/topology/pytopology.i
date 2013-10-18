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

%module "nox.netapps.topology"

%{
#include "pytopology.hh"
#include "pyrt/pycontext.hh"
using namespace vigil;
using namespace vigil::applications;
%}

%import "netinet/netinet.i"

%include "common-defs.i"
%include "std_list.i"

struct PyLinkPorts {
    uint16_t src;
    uint16_t dst;
};

//UoR
struct PyNodeInfo{
    uint64_t dpid;
    bool active;
};

//UoR
struct PyPortInfo{
    std::string name;
    std::string state;
    std::string medium;
    std::string duplexity;
    uint16_t    portID;
    int speed;	
};

%template(LLinkSet) std::list<PyLinkPorts>; 
%template(LNodeSet) std::list<PyNodeInfo>;
%template(LPortSet) std::list<PyPortInfo>;


class pytopology_proxy{
public:

    pytopology_proxy(PyObject* ctxt);

    void configure(PyObject*);
    void install(PyObject*);

    std::list<PyLinkPorts> get_outlinks(datapathid dpsrc, datapathid dpdst) const;
    bool is_internal(datapathid dp, uint16_t port) const;
	
    /** \added by Andrea Simeoni--Returns the whole topology data structure
    */
    hash_map<datapathid,Topology::DpInfo> get_whole_topology();
        /** \added by Andrea Simeoni--Returns the whole topology data structure
    */
    
    DatapathFeatures get_dpinfo( datapathid dp) const;

    /** UoR
   */
   
    std::list<PyNodeInfo> synchronize_network();


    /** UoR
   */
   
    std::list<PyPortInfo> synchronize_node(datapathid dp);
 
   /** UoR
   */
   
    PyPortInfo synchronize_port(datapathid id, std::string pinfo);

   /** UoR
   */
    
    bool is_valid_dp(datapathid dp);

	bool is_valid_port(datapathid dp, std::string portName);
  

protected:   

    topology* topo;
    routeinstaller* ri;
    container::Component* c;
}; // class pytopology_proxy


%pythoncode
%{
from nox.lib.core import Component

class pytopology(Component):
    """
    An adaptor over the C++ based Python bindings to
    simplify their implementation.
    """  
    def __init__(self, ctxt):
        self.pytop = pytopology_proxy(ctxt)


    def configure(self, configuration):
        self.pytop.configure(configuration)

    def install(self):
        pass

    def getInterface(self):
        return str(pytopology)

    def get_outlinks(self, dpsrc, dpdst):
        return self.pytop.get_outlinks(dpsrc, dpdst)

    def is_internal(self, dp, port):
        return self.pytop.is_internal(dp, port)

    def get_whole_topology(self):
        return self.pytop.get_whole_topology()

    def get_dpinfo(self,dp):
        return self.pytop.get_dpinfo(dp)

    def synchronize_network(self):
        return self.pytop.synchronize_network()

    def synchronize_node(self,dp):
        return self.pytop.synchronize_node(dp)

    def synchronize_port(self, dp, port):
        return self.pytop.synchronize_port(dp, port)

    def is_valid_dp(self,dp):
        return self.pytop.is_valid_dp(dp)

    def is_valid_port(self, dp, port):
        return self.pytop.is_valid_port(dp, port)

def getFactory():
    class Factory():
        def instance(self, context):
                    
            return pytopology(context)

    return Factory()
%}
