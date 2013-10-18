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

#ifndef ROUTING_HH
#define ROUTING_HH 1

#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <list>
#include <queue>
#include <sstream>
#include <vector>

#include "component.hh"
#include "event.hh"
#include "flow.hh"
#include "flow-removed.hh"
#include "hash_map.hh"
#include "hash_set.hh"
#include "discovery/link-event.hh"
#include "nat_enforcer.hh"
#include "netinet++/datapathid.hh"
#include "netinet++/ethernetaddr.hh"
#include "network_graph.hh"
#include "openflow/openflow.h"
#include "topology/topology.hh"


namespace vigil {
namespace applications {

/** \ingroup noxcomponents
 *
 * Routing_module is a utility component that can be called to retrieve a
 * shortest path route and/or to set up flow entries in the network with
 * Openflow actions to route a flow.
 *
 * NOX instances will usually have an additional routing component listening
 * for Flow_in_events and making calls to this API to route the flows through
 * the network.  Such components should list 'routing_module' in their meta.xml
 * dependencies.  See 'sprouting.hh/.cc' for an example.
 *
 * Uses algorithm described in "A New Approach to Dynamic All Pairs Shortest
 * Paths" by C. Demetrescu to perform incremental updates when links are
 * added/removed from the network instead of recomputing all shortest paths.
 *
 * All integer values are stored in host byte order and should be passed in as
 * such as well.
 *
 */
struct Netic_path_info {
    std::string     nw_src;	/* Source Ip address of flow */
    std::string     nw_dst;	/* Destination Ip address of flow */
    uint64_t 	    duration;	/* Hard duration of the flow in seconds */
    uint64_t 	    bandwidth;	/* Reserved bandwidth of the flow (future releases */
    bool     	    set_arp;	/* set also arp path in the openflow network */
    bool	    bidirect;	/* Set also the inverse path, recommended to true */

    datapathid 	    dp_src;	/* the entry point switch in the network */
    datapathid 	    dp_dst;	/* entry point port of the switch */
    uint16_t	    first_port;	/* exit point switch in the network of the flow */
    uint16_t	    last_port;	/* exit port of the switch */

    ethernetaddr    dl_src;	/* source mac address of the tx machine */
    ethernetaddr    dl_dst;	/* destination mac address of the rx machine */

    uint8_t        ip_proto;   /* What protocol is IP carrying, look in http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xml */
    uint16_t	    tp_src;	/* source tp port of the flow */
    uint16_t	    tp_dst;	/* destination tp port of the flow */
    uint16_t	    vlan_tag;	/* vlan tag or id */
    };
class Routing_module
    : public container::Component {

public:

    struct Link {
        datapathid dst;    // destination dp of link starting at current dp
        uint16_t outport;  // outport of current dp link connected to
        uint16_t inport;   // inport of destination dp link connected to
    };

    struct RouteId {
        datapathid src;    // Source dp of a route
        datapathid dst;    // Destination dp of a route
    };

    struct Route {
        RouteId id;            // Start/End datapath
        std::list<Link> path;  // links connecting datapaths
    };

    struct PathResult{
	std::string	directPath;
	std::string	reversPath;
    };
    

    typedef boost::shared_ptr<Route> RoutePtr;
    typedef std::list<Nonowning_buffer> ActionList;

    //UoR
    struct Path{
        uint16_t 	     id;
        RoutePtr 	     rte; /* A pointer to the route */
	Netic_path_info      npi; /* the info regarding the path */
	timeval		     start_time; /* Time submitted */
	std::vector<uint64_t>  cookies; /*Cookies for all dpids*/
    };

    Routing_module(const container::Context*,
                   const json_object*);
    // for python
    Routing_module();
    ~Routing_module() { }


    static void getInstance(const container::Context*, Routing_module*&);

    void configure(const container::Configuration*);
    void install();

    // Given a RouteId 'id', sets 'route' to the route between the two
    // datapaths.  Returns 'true' if a route between the two exists, else
    // 'false'.  In most cases, the route should later be passed on to
    // check_route() with the inport and outport of the start and end dps to
    // verify that a packet will not get route out the same port it came in
    // on, (which should only happen in very specific cases, e.g. wireless APs).

    // Should avoid calling this method unnecessarily when id.src == id.dst -
    // as module will allocate a new empty Route() to populate 'route'.  Caller
    // will ideally check for this case.

    // Routes retrieved through this method SHOULD NOT BE MODIFIED.

    bool get_route(const RouteId& id, RoutePtr& route) const;

    // Given a route and an access point inport and outport, verifies that a
    // flow will not get route out the same port it came in on at the
    // endpoints.  Returns 'true' if this will not happen, else 'false'.

    bool check_route(const Route& route, uint16_t inport, uint16_t outport) const;

    bool is_on_path_location(const RouteId& id, uint16_t src_port,
                             uint16_t dst_port);

    // Sets up the switch entries needed to route Flow 'flow' according to
    // 'route' and the source access point 'inport' and destination access
    // point 'outport'.  Entries will time out after 'flow_timeout' seconds of
    // inactivity.  If 'actions' is non-empty, it should specify the set of
    // Openflow actions to input at each datapath en route, in addition to the
    // default OFPAT_OUTPUT action used to route the flow.  If non-empty,
    // 'actions' should be of length 'route.path.size() + 1'.  'actions[i]'
    // should be the set of actions to place in the ith dp en route to the
    // destination, packed in a buffer of size equal to the action data's len.
    // If a flow's headers will be overwritten by the actions at a particular
    // datapath, module takes care of putting the correct modified version of
    // 'flow' in later switch flow entries en route.  Buffers in 'actions'
    // should be Openflow message-ready - actions should be packed together
    // with correct padding and fields should be in network byte order.

    bool setup_route(const Flow& flow, const Route& route, uint16_t inport,
                     uint16_t outport, uint16_t flow_timeout,
                     const ActionList& actions, bool check_nat,
                     const GroupList *sdladdr_groups,
                     const GroupList *snwaddr_groups,
                     const GroupList *ddladdr_groups,
                     const GroupList *dnwaddr_groups);

    bool setup_flow(const Flow& flow, const datapathid& dp,
                    uint16_t outport, uint32_t bid, const Buffer& buf,
                    uint16_t flow_timeout, const Buffer& actions,
                    bool check_nat,
                    const GroupList *sdladdr_groups,
                    const GroupList *snwaddr_groups,
                    const GroupList *ddladdr_groups,
                    const GroupList *dnwaddr_groups);

    bool send_packet(const datapathid& dp, uint16_t inport, uint16_t outport,
                     uint32_t bid, const Buffer& buf,
                     const Buffer& actions, bool check_nat,
                     const Flow& flow,
                     const GroupList *sdladdr_groups,
                     const GroupList *snwaddr_groups,
                     const GroupList *ddladdr_groups,
                     const GroupList *dnwaddr_groups);


   /**
    * UoR
    * Installs on the route along sdpid and ddpid, a flow identified only by source and dest ip
    */
    PathResult netic_create_route(Netic_path_info& npi, int& result);


   /**
    * UoR:
    * Get list of ids of installed paths in the network. The ids are printed in a string separated by ':'.
    *
    */
    std::string netic_get_path_list(void);


   /**
    * UoR:
    * Delete a path identified by its path_id
    *
    */
    int netic_remove_path(uint16_t);
    
    
   /**
    * UoR:
    * Get path description
    *
    */
    Path* netic_get_path_desc(uint16_t, bool& found);
    
private:
    struct ridhash {
        std::size_t operator()(const RouteId& rid) const;
    };

    struct rideq {
        bool operator()(const RouteId& a, const RouteId& b) const;
    };

    struct routehash {
        std::size_t operator()(const RoutePtr& rte) const;
    };

    struct routeq {
        bool operator()(const RoutePtr& a, const RoutePtr& b) const;
    };

    struct ruleptrcmp {
        bool operator()(const RoutePtr& a, const RoutePtr& b) const;
    };



    
    typedef hash_map<uint16_t, Path> OngoingPaths;
    typedef hash_map<uint64_t, uint16_t> CookiePathIdBindings;

    typedef std::list<RoutePtr> RouteList;
    typedef hash_set<RoutePtr, routehash, routeq> RouteSet;
    typedef hash_map<RouteId, RoutePtr, ridhash, rideq> RouteMap;
    typedef hash_map<RouteId, RouteList, ridhash, rideq> RoutesMap;
    typedef hash_map<RoutePtr, RouteList, routehash, routeq> ExtensionMap;
    typedef std::priority_queue<RoutePtr, std::vector<RoutePtr>, ruleptrcmp> RouteQueue;

    // Data structures needed by All-Pairs Shortest Path Algorithm

    Topology *topology;
    NAT_enforcer *nat;
    RouteMap shortest;
    RoutesMap local_routes;
    ExtensionMap left_local;
    ExtensionMap left_shortest;
    ExtensionMap right_local;
    ExtensionMap right_shortest; 

    /* UoR */
    OngoingPaths activePaths;
    CookiePathIdBindings cookieBindings;

    std::vector<const std::vector<uint64_t>*> nat_flow;

    uint16_t max_output_action_len;
    uint16_t len_flow_actions;
    uint32_t num_actions;
    boost::shared_array<uint8_t> raw_of;
    ofp_flow_mod *ofm;

    std::ostringstream os;

    Disposition handle_link_change(const Event&);

    // All-pairs shortest path fns

    void cleanup(RoutePtr, bool);
    void clean_subpath(RoutePtr&, const RouteList&, RouteSet&, bool);
    void clean_route(const RoutePtr&, RoutePtr&, bool);

    void fixup(RouteQueue&, bool);
    void add_local_routes(const RoutePtr&, const RoutePtr&,
                          const RoutePtr&, RouteQueue&);

    void set_subpath(RoutePtr&, bool);
    void get_cached_path(RoutePtr&);

    bool remove(RouteMap&, const RoutePtr&);
    bool remove(RoutesMap&, const RoutePtr&);
    bool remove(ExtensionMap&, const RoutePtr&, const RoutePtr&);
    void add(RouteMap&, const RoutePtr&);
    void add(RoutesMap&, const RoutePtr&);
    void add(ExtensionMap&, const RoutePtr&, const RoutePtr&);

   /** UoR: 
    *  Added functions
    *
    */

    /*UoR*/

    void route2tree(network::termination dst, Routing_module::RoutePtr& sroute,
		   network::route& route);
    /*UoR*/
    bool get_shortest_path(network::termination dst, network::route& route,RoutePtr&);
   /**
    * UoR
    * Installs or Removes recursively the flow described with a Netic_path_info structure
    * Side effect: It binds the cookies returned by netic_setup_openflow_packet with the path_id in the
    * cookieBindings hash_map when adding flows.
    */
    void netic_exec_route(network::route* route,Path* path, uint16_t pathID, 
        ofp_flow_mod_command comm);

   /**
    * UoR: Setup the packet to be sent to the specified datapath. 
    * 
    *
    */
    uint64_t netic_setup_openflow_packet(datapathid dpid, uint16_t inport, uint16_t outport,
		std::string sip, std::string dip, uint8_t ip_proto, uint16_t tcp_src, uint16_t tcp_dst,
		uint64_t duration, ofp_flow_mod_command comm);

   /**
    * UoR: 
    * For every flow installed there is the need to install also the arp-packets flow route.
    * Up to now only the match in dl_type is used. Returns the cookie of the create packet.
    */
    uint64_t netic_set_arp_packet_command(datapathid dpid, uint16_t inport, ethernetaddr dl_src, uint64_t duration,
                ofp_flow_mod_command comm);
      
      
    ofp_match* get_match(Path* p);
    
    uint64_t get_cookie_from_match(ofp_match* match);
            
   /**
    * UoR:
    * Get path match, it is private. 
    * 
    */
    uint64_t post_path_event(Path* p, bool& b);

    Disposition manage_flow_expired(const Event&);

    uint16_t calculate_path_id(uint16_t first_port, uint16_t last_port,
    		uint64_t sdpid, uint64_t ddpid, std::string sip, std::string dip, 
    		uint8_t ip_proto, uint16_t tcp_src, uint16_t tcp_dst);

    void add_on_going_path(OngoingPaths&, uint16_t, const Path&); 
    void remove_on_going_path(OngoingPaths&, uint16_t);
    /* End of UoR functions */


    // Flow-in handler that sets up path

    void init_openflow(uint16_t);
    void check_openflow(uint16_t);
    void set_openflow(const Flow&, uint32_t, uint16_t);
    bool set_openflow_actions(const Buffer&, const datapathid&, uint16_t, bool, bool);
    void modify_match(const Buffer&);
    bool set_action(uint8_t*, const datapathid&, uint16_t, uint16_t&, bool check_nat,
                    bool allow_overwrite, uint64_t overwrite_mac, bool& nat_overwritten);
};

}
}

#endif
