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

#include "routing.hh"
#include "openflow-default.hh"

#include <boost/bind.hpp>
#include <inttypes.h>

#include "assert.hh"
#include "openflow/nicira-ext.h"
#include "vlog.hh"
#include "openflow-pack.hh"
#include "openflow-action.hh"
#include "path-event.hh"

#include "fnv_hash.h"
#include "serialize.h"

namespace vigil {
namespace applications {

static Vlog_module lg("routing");

/*UoR*/
Netic_path_info
reverse_path_info(Netic_path_info npi)
{
    Netic_path_info nnpi;
    nnpi.nw_src = npi.nw_dst;
    nnpi.nw_dst = npi.nw_src;
    nnpi.duration = npi.duration;
    nnpi.bandwidth = npi.bandwidth;
    nnpi.set_arp = npi.set_arp;
    nnpi.dp_src = npi.dp_dst;
    nnpi.dp_dst = npi.dp_src;
    nnpi.first_port = npi.last_port;
    nnpi.last_port = npi.first_port;
    nnpi.dl_src = npi.dl_dst;
    nnpi.dl_dst = npi.dl_src;
    nnpi.ip_proto = npi.ip_proto;
    nnpi.tp_src = npi.tp_dst;
    nnpi.tp_dst = npi.tp_src;
    nnpi.vlan_tag = npi.vlan_tag;
    return nnpi;
}

bool Routing_module::get_shortest_path(network::termination dst,
		network::route& route, RoutePtr& sroute) {
	Routing_module::RouteId id;
	id.src = route.in_switch_port.dpid;
	id.dst = dst.dpid;

	if (!get_route(id, sroute))
		return false;
	route2tree(dst, sroute, route);
	return true;
}

void Routing_module::route2tree(network::termination dst,
		Routing_module::RoutePtr& sroute, network::route& route) {
	route.next_hops.clear();
	network::hop* currHop = &route;
	std::list<Routing_module::Link>::iterator i = sroute->path.begin();
	while (i != sroute->path.end()) {
		network::hop* nhop = new network::hop(i->dst, i->inport);
		currHop->next_hops.push_front(std::make_pair(i->outport, nhop));
		currHop = nhop;
		i++;
	}
	network::hop* nhop = new network::hop(datapathid(), 0);
	currHop->next_hops.push_front(std::make_pair(dst.port, nhop));

}

uint16_t Routing_module::calculate_path_id(uint16_t first_port, uint16_t last_port,
		uint64_t sdpid, uint64_t ddpid, std::string sip, std::string dip, uint8_t ip_proto, uint16_t tcp_src, uint16_t tcp_dst) {

	uint16_t length = 0;
	char* buffer;
	const char* temp;
	uint8_t counter = 0;
	unsigned int hash;

//	VLOG_DBG(lg,"Source_port: %u; destination port: %u,\n",first_port, last_port);
//	VLOG_DBG(lg,"source_id: %Li; Destination id: %li\n", sdpid, ddpid);
//	VLOG_DBG(lg,"%s %s",sip.c_str(), dip.c_str());

	length += 2 * sizeof(uint16_t);
	length += 2 * sizeof(uint64_t);
	length += sip.length();
	length += dip.length();
	length += 1 * sizeof(uint8_t);
	length += 2 * sizeof(uint16_t);

	buffer = new char[length];

	for(int i = 0;i < length;i++)
		buffer[i] = 0;

	counter += serialize_uint16(first_port,&buffer[counter],sizeof(uint16_t));
	counter += serialize_uint16(last_port,&buffer[counter],sizeof(uint16_t));

	counter += serialize_uint64(sdpid,&buffer[counter],sizeof(uint64_t));
	counter += serialize_uint64(ddpid,&buffer[counter],sizeof(uint64_t));
	
	temp = sip.c_str();
	for (int i = 0; i < sip.length(); i++) {
		buffer[counter] = temp[i];
		counter++;
	}
	temp = dip.c_str();
	for (int i = 0; i < dip.length(); i++) {
		buffer[counter] = temp[i];
		counter++;
	}
        
        buffer[counter] = (char) ip_proto;
        counter += sizeof(uint8_t);
        
	counter += serialize_uint16(tcp_src,&buffer[counter],sizeof(uint16_t));
	counter += serialize_uint16(tcp_dst,&buffer[counter],sizeof(uint16_t));

	hash = fnv_32_buf((void*)buffer, (unsigned int)length, FNV1_32_INIT);
//	VLOG_DBG(lg,"Created hash %d",hash);
	return (hash>>16) ^ (hash & MASK_16);
}

Routing_module::PathResult Routing_module::netic_create_route(Netic_path_info& npi, int& result) {

	std::string hash_string;
	PathResult res;
	uint16_t hash;
	bool error;
	char out[5];//path_id four hexa-decimal digits
	RoutePtr saveRoute;
	Path currentPath;
	Path reversePath;

	network::route rte(npi.dp_src, npi.first_port);
	network::termination endpt(npi.dp_dst, npi.last_port);

	network::route inverse_rte(npi.dp_dst, npi.first_port);
	network::termination inverse_endpt(npi.dp_src, npi.last_port);

	currentPath.npi = npi;

	result = 0;
	if (get_shortest_path(endpt, rte, saveRoute)) {
        	currentPath.rte = saveRoute;
		/* calculate hash here */
		hash = calculate_path_id(npi.first_port, npi.last_port,
				npi.dp_src.as_host(), npi.dp_dst.as_host(),
				npi.nw_src, npi.nw_dst, npi.ip_proto, npi.tp_src, npi.tp_dst);
                currentPath.id = hash;
                
                timeval curtime = { 0, 0 };
                gettimeofday(&curtime, NULL);
                currentPath.start_time = curtime;

		netic_exec_route(&rte, &currentPath, hash, OFPFC_ADD); /*install first direction*/

		//	VLOG_DBG(lg,"Created hash %x",hash);
                VLOG_DBG(lg,"lunghezza cookies %d",currentPath.cookies.size());
                VLOG_DBG(lg,"cookie 0 is %"PRIx64"",currentPath.cookies[0]);
                
		sprintf(out, "%x", hash);
		//	VLOG_DBG(lg,"Created hash %s",s.c_str());
		hash_string = out;
		res.directPath = hash_string;
		add_on_going_path(activePaths, hash, currentPath);
		
		/* Post path event. This is catched from linkload at the moment*/
		post_path_event(&currentPath, error);
		//VLOG_DBG(lg,"Added path event with cookie: %"PRIx64"",cookie);
	}
	else{
		result = 1;
		return res;
	}
	if (npi.bidirect) {
	    if (get_shortest_path(inverse_endpt, inverse_rte, saveRoute)){
                Netic_path_info rnpi = reverse_path_info(npi);
	        reversePath.rte = saveRoute;
		/* calculate hash here */
		hash = calculate_path_id(rnpi.first_port, rnpi.last_port,
				rnpi.dp_src.as_host(), rnpi.dp_dst.as_host(),
				rnpi.nw_src, rnpi.nw_dst, rnpi.ip_proto, rnpi.tp_src, rnpi.tp_dst);
                reversePath.id = hash;
                
                timeval curtime = { 0, 0 };
                gettimeofday(&curtime, NULL);
                reversePath.start_time = curtime;

                reversePath.npi = rnpi;
		netic_exec_route(&inverse_rte, &reversePath, hash, OFPFC_ADD); /*install reverse direction*/

		//	VLOG_DBG(lg,"Created hash %x",hash);

		sprintf(out, "%x", hash);
		//	VLOG_DBG(lg,"Created hash %s",s.c_str());
		hash_string = out;
		add_on_going_path(activePaths, hash, reversePath);
		res.reversPath = hash_string;
		
		/* Post path event. This is catched from linkload at the moment*/
		post_path_event(&reversePath, error); 
	    }
	    else{ /* Revers Path not found */
		result = 1;
		return res;
	    }
	}
	return res;
}

int
Routing_module::netic_remove_path(uint16_t path_id){
    /* must get route first */
// XXX
    VLOG_DBG(lg,"Deleting path with id: %"PRIx16"",path_id);
    Path        path;
    OngoingPaths::iterator path_iter;
    RoutePtr   installedRoute;

    path_iter = activePaths.find(path_id);
    if (path_iter != activePaths.end()){
        VLOG_DBG(lg,"found path, proceeding...");
        path = path_iter->second;
        
        installedRoute = path.rte;
        if (installedRoute == NULL)
            {
            VLOG_DBG(lg,"Route was NULL, error!!!");
            return 1;
            }
        network::route rte(path.npi.dp_src, path.npi.first_port);
        network::termination endpt(path.npi.dp_dst, path.npi.last_port);
        
        route2tree(endpt,installedRoute,rte);
        
        netic_exec_route(&rte,&path, path_id, OFPFC_DELETE);
        return 0;
    }
    return 1;
}

Routing_module::Path*
Routing_module::netic_get_path_desc(uint16_t path_id, bool& found){

    Path* path;
    OngoingPaths::iterator path_iter;
    RoutePtr   installedRoute;
    
    found = false;

    path_iter = activePaths.find(path_id);
    if (path_iter != activePaths.end()){
        VLOG_DBG(lg,"found path, returning description");
        path = &(path_iter->second);
        found = true;
        return path;
    }
    return NULL;
}
/**
 * UoR
 * 
 */
void Routing_module::netic_exec_route(network::route* route,Path* path, uint16_t pathID, ofp_flow_mod_command comm) {
    uint64_t cookie;
    if (route->next_hops.begin()->second->in_switch_port.dpid.empty()) { //Last node of the path
        VLOG_DBG(lg,"fine");
    
        cookie = netic_setup_openflow_packet(route->in_switch_port.dpid,
		        route->in_switch_port.port, route->next_hops.begin()->first,
			path->npi.nw_src, path->npi.nw_dst, path->npi.ip_proto, path->npi.tp_src, 
			path->npi.tp_dst, path->npi.duration, comm);
	path->cookies.push_back(cookie);
	VLOG_DBG(lg,"path has %d cookies",path->cookies.size());
	
	if (comm == OFPFC_ADD){
	    /* Bind the cookie returned with the path id */
	    VLOG_DBG(lg,"Binding cookie %"PRIx64" with pathID %"PRIx16"",cookie, pathID);
	    cookieBindings[cookie] = pathID;
	}
	else if (comm == OFPFC_DELETE){
            ofm->out_port = htons(OFPP_NONE);
	}
        send_openflow_command(route->in_switch_port.dpid, &ofm->header, false);
        
        /* Check also if path had also the arp rules */
        if (path->npi.set_arp == true){
            cookie = netic_set_arp_packet_command(route->in_switch_port.dpid,
				route->in_switch_port.port,path->npi.dl_src, path->npi.duration, comm);
	    if (comm == OFPFC_ADD){
	        /* Bind the cookie returned with the path id */
       	        VLOG_DBG(lg,"Binding cookie %"PRIx64" with pathID %"PRIx16"",cookie, pathID);
		cookieBindings[cookie] = pathID;
	    }
	    else if (comm == OFPFC_DELETE){
                ofm->out_port = htons(OFPP_NONE);
	    }
            send_openflow_command(route->in_switch_port.dpid, &ofm->header, false);
        }
        return;
    }   
    else { //Intermediate node on the path
        VLOG_DBG(lg,"intermediate");        
        cookie = netic_setup_openflow_packet(route->in_switch_port.dpid,
			route->in_switch_port.port, route->next_hops.begin()->first,
			path->npi.nw_src, path->npi.nw_dst, path->npi.ip_proto, path->npi.tp_src, 
			path->npi.tp_dst, path->npi.duration, comm);
	path->cookies.push_back(cookie);
	VLOG_DBG(lg,"path has %d cookies",path->cookies.size());
	
	if (comm == OFPFC_ADD){
	    VLOG_DBG(lg,"Binding cookie %"PRIx64" with pathID %"PRIx16"",cookie, pathID);
	    /* Bind the cookie returned with the path id */
	    cookieBindings[cookie] = pathID;
	}
	else if (comm == OFPFC_DELETE){
                ofm->out_port = htons(OFPP_NONE);
	    }
        send_openflow_command(route->in_switch_port.dpid, &ofm->header, false);
        
        /* Check also if path had also the arp rules */
        if (path->npi.set_arp == true){
            cookie = netic_set_arp_packet_command(route->in_switch_port.dpid,
				route->in_switch_port.port,path->npi.dl_src, path->npi.duration, comm);
	    if (comm == OFPFC_ADD){
	        /* Bind the cookie returned with the path id */
	        VLOG_DBG(lg,"Binding cookie %"PRIx64" with pathID %"PRIx16"",cookie, pathID);
		cookieBindings[cookie] = pathID;
	    }
	    else if (comm == OFPFC_DELETE){
                ofm->out_port = htons(OFPP_NONE);
	    }
            send_openflow_command(route->in_switch_port.dpid, &ofm->header, false);
        }
        Routing_module::netic_exec_route(route->next_hops.begin()->second, path, pathID, comm);
        return;
    }
}

/**
  * Creates the common part of the match of a path by allocating a new match.  
  * Only in_port parameter of match remains to be specified
  */
ofp_match* 
Routing_module::get_match(Path* p){
    ofp_match* match = new ofp_match();
    match->dl_vlan = p->npi.vlan_tag;

    ipaddr src_ip(p->npi.nw_src);
    ipaddr dst_ip(p->npi.nw_dst);
    match->nw_src = src_ip;
    match->nw_dst = dst_ip;
    match->tp_src = p->npi.tp_src;
    match->tp_dst = p->npi.tp_dst;

    match->dl_type = htons(0x0800);//IP

    match->wildcards = htonl(OFPFW_DL_SRC | OFPFW_DL_VLAN  | 
	                     OFPFW_DL_DST | OFPFW_NW_PROTO |
                             OFPFW_TP_SRC | OFPFW_TP_DST | 
                             OFPFW_NW_TOS | OFPFW_DL_VLAN_PCP);

    match->nw_proto = p->npi.ip_proto;
    if (p->npi.ip_proto != 255){
        match->wildcards = match->wildcards & ~(htonl(OFPFW_NW_PROTO));
        if (p->npi.tp_src != 0)
            match->wildcards = match->wildcards & ~(htonl(OFPFW_TP_SRC));
        if (p->npi.tp_dst != 0)
            match->wildcards = match->wildcards & ~(htonl(OFPFW_TP_DST));
    }
    return match;
}

uint64_t
Routing_module::post_path_event(Path* path, bool& error){
    
    if (path == NULL){
        error = true;
        return 0;
    }
    ofp_match* m;
    boost::shared_ptr< hash_map<datapathid, uint64_t> > cookies;
    cookies.reset(new hash_map<datapathid, uint64_t>());
    
    boost::shared_ptr< hash_map<datapathid, ofp_match*> > matchesPtr;
    matchesPtr.reset(new hash_map<datapathid, ofp_match*>());
    
    /* Insert the first match */
    m = get_match(path);
    m->in_port = path->npi.first_port;

    matchesPtr->insert(std::make_pair(path->rte->id.src,m));
    cookies->insert(std::make_pair(path->rte->id.src,path->cookies[0]));    
//    cookies->insert(std::make_pair(path->rte->id.src,get_cookie_from_match(m)));

    /* insert the other matches */
    std::list<Link>::iterator li;
    int counter  = 1;
    for (li = path->rte->path.begin(); li != path->rte->path.end(); li++) {
        m = get_match(path);
        m->in_port = li->inport;
        matchesPtr->insert(std::make_pair(li->dst,m));
        cookies->insert(std::make_pair(li->dst,path->cookies[counter]));
        counter++;
//        cookies->insert(std::make_pair(li->dst,get_cookie_from_match(m)));
    }
        

              
    Path_event* pe = new Path_event(path->id,cookies, matchesPtr, Path_event::ADD);
    post(pe);
    return 0;
}

uint64_t Routing_module::get_cookie_from_match(ofp_match* match){
    Flow* flow = new Flow();
    flow->in_port 	= match->in_port;
    flow->nw_src 	= match->nw_src;
    flow->nw_dst 	= match->nw_dst;
    flow->dl_type	= htons(0x0800);
    flow->nw_proto    = match->nw_proto;
    flow->tp_src      = htons(match->tp_src);
    flow->tp_dst      = htons(match->tp_dst);
    uint64_t res = flow->hash_code(32);
    delete[] flow;
    return res;
}

uint64_t Routing_module::netic_setup_openflow_packet(datapathid dpid, uint16_t inport, uint16_t outport,
		std::string sip, std::string dip, uint8_t ip_proto, uint16_t tp_src, uint16_t tp_dst,
		uint64_t duration, ofp_flow_mod_command comm) {

	ipaddr src_ip(sip);
	ipaddr dst_ip(dip);

	Flow* flow = new Flow();
	flow->in_port 	= inport;
	flow->nw_src 	= src_ip;
	flow->nw_dst 	= dst_ip;
	flow->dl_type	= htons(0x0800);
	
	flow->nw_proto = ip_proto;
	flow->tp_src = htons(tp_src);
	flow->tp_dst = htons(tp_dst);

	set_openflow(*flow, UINT32_MAX, OFP_FLOW_PERMANENT);
	
        ofm->command = htons(comm);
	ofm->hard_timeout = htons(duration);
        ofm->match.wildcards = htonl(OFPFW_DL_SRC | OFPFW_DL_VLAN  | 
			             OFPFW_DL_DST | OFPFW_NW_PROTO |
                                     OFPFW_TP_SRC | OFPFW_TP_DST | 
                                     OFPFW_NW_TOS | OFPFW_DL_VLAN_PCP);
                                     
        if (ip_proto != 255){
            ofm->match.wildcards = ofm->match.wildcards & ~(htonl(OFPFW_NW_PROTO));

            if (tp_src != 0)
                ofm->match.wildcards = ofm->match.wildcards & ~(htonl(OFPFW_TP_SRC));
            if (tp_dst != 0)
                ofm->match.wildcards = ofm->match.wildcards & ~(htonl(OFPFW_TP_DST));
        }
        
	uint16_t packet_len = sizeof(*ofm);
	bool nat_overwritten;
	set_action((uint8_t*) ofm->actions, dpid, outport,
			packet_len, false, false, 0, nat_overwritten);
	ofm->header.length = htons(packet_len);
	
	ofm->match.in_port = htons(inport);

	delete[] flow;
        return ntohll(ofm->cookie);
}

uint64_t Routing_module::netic_set_arp_packet_command(datapathid dpid, uint16_t inport, ethernetaddr dl_src, 
uint64_t duration, ofp_flow_mod_command comm) {
	
    VLOG_DBG(lg,"Creating arp packet");
    Flow* flow = new Flow();
    flow->in_port 	= inport;

    flow->dl_src 	= dl_src;
    flow->dl_type	= htons(0x0806);
    flow->nw_proto	= 1;
    
    set_openflow(*flow, UINT32_MAX, OFP_FLOW_PERMANENT);
    ofm->command = htons(comm);
	
    ofm->hard_timeout = htons(duration);
    ofm->match.wildcards = htonl(OFPFW_NW_PROTO | OFPFW_DL_DST | OFPFW_DL_VLAN  | OFPFW_NW_TOS | OFPFW_TP_SRC | OFPFW_TP_DST | OFPFW_DL_VLAN_PCP | OFPFW_NW_SRC_MASK | OFPFW_NW_DST_MASK );

    uint16_t packet_len = sizeof(*ofm);
    bool nat_overwritten;
    set_action((uint8_t*) ofm->actions, dpid, OFPP_ALL,
		packet_len, false, false, 0, nat_overwritten);
    ofm->header.length = htons(packet_len);
	
    ofm->match.in_port = htons(inport);
    return ntohll(ofm->cookie);
}



void 
Routing_module::add_on_going_path(OngoingPaths& o, uint16_t h, const Path& p){
    VLOG_DBG(lg,"Adding path in on going paths");
    o[h] = p;
}

void 
Routing_module::remove_on_going_path(OngoingPaths& o, uint16_t h){
    OngoingPaths::iterator pi = o.find(h);
    if (pi != o.end()){
        VLOG_DBG(lg,"Removing ongoing path with pathID: %"PRIx16"",h);
        o.erase(pi);
    }
    else{
        VLOG_DBG(lg,"Path id not found perhaps it was cancelled before");
    }
}


std::string
Routing_module::netic_get_path_list(void){
    OngoingPaths::iterator pi;

    std::string ret = "";
    char       out[5];
    for (pi = activePaths.begin(); pi!=activePaths.end(); pi++){
        sprintf(out, "%x", pi->first);
        if (pi != activePaths.begin())
            ret += ":";
        ret += out;
    }
    return ret;
}


std::size_t Routing_module::ridhash::operator()(const RouteId& rid) const {
	HASH_NAMESPACE::hash<datapathid> dphash;
	return (dphash(rid.src) ^ dphash(rid.dst));
}

bool Routing_module::rideq::operator()(const RouteId& a,
		const RouteId& b) const {
	return (a.src == b.src && a.dst == b.dst);
}

std::size_t Routing_module::routehash::operator()(const RoutePtr& rte) const {
	return (ridhash()(rte->id) ^ HASH_NAMESPACE::hash<uint32_t>()(rte->path.size()));
}

bool Routing_module::routeq::operator()(const RoutePtr& a,
		const RoutePtr& b) const {
	if (a->id.src != b->id.src || a->id.dst != b->id.dst
			|| a->path.size() != b->path.size()) {
		return false;
	}
	std::list<Link>::const_iterator aiter, biter;
	for (aiter = a->path.begin(), biter = b->path.begin();
			aiter != a->path.end(); ++aiter, ++biter) {
		if (aiter->dst != biter->dst || aiter->outport != biter->outport
				|| aiter->inport != biter->inport) {
			return false;
		}
	}
	return true;
}

bool Routing_module::ruleptrcmp::operator()(const RoutePtr& a,
		const RoutePtr& b) const {
	return (a->path.size() > b->path.size());
}

static inline uint32_t get_max_action_len() {
	if (sizeof(ofp_action_output) >= sizeof(nx_action_snat)) {
		return sizeof(ofp_action_output) + sizeof(ofp_action_dl_addr);
	}
	return sizeof(nx_action_snat) + sizeof(ofp_action_dl_addr);
}

// Constructor - initializes openflow packet memory used to setup route

Routing_module::Routing_module(const container::Context* c,
		const json_object* d) :
		container::Component(c), topology(0), nat(0), len_flow_actions(0), num_actions(
				0), ofm(0) {
	max_output_action_len = get_max_action_len();
}

void Routing_module::getInstance(const container::Context* ctxt,
		Routing_module*& r) {
	r = dynamic_cast<Routing_module*>(ctxt->get_by_interface(
			container::Interface_description(typeid(Routing_module).name())));
}

void Routing_module::configure(const container::Configuration*) {
	resolve(topology);
	resolve(nat);
	register_handler<Link_event>(
			boost::bind(&Routing_module::handle_link_change, this, _1));
	register_handler<Flow_removed_event>(
			boost::bind(&Routing_module::manage_flow_expired, this, _1));
}

void Routing_module::install() {
}

bool Routing_module::get_route(const RouteId& id, RoutePtr& route) const {
	RouteMap::const_iterator rte = shortest.find(id);
	if (rte == shortest.end()) {
		if (id.src == id.dst) {
			route.reset(new Route());
			route->id = id;
			return true;
		}
		return false;
	}

	route = rte->second;
	return true;
}

bool Routing_module::check_route(const Route& route, uint16_t inport,
		uint16_t outport) const {
	if (route.path.empty()) {
		if (inport == outport) {
			return false;
		}
	} else if (inport == route.path.front().outport) {
		return false;
	} else if (outport == route.path.back().inport) {
		return false;
	}
	return true;
}

bool Routing_module::is_on_path_location(const RouteId& id, uint16_t src_port,
		uint16_t dst_port) {
	Routing_module::RoutePtr rte;
	return (id.src != id.dst && get_route(id, rte)
			&& rte->path.front().outport != src_port
			&& rte->path.back().inport == dst_port);
}

// Updates shortests paths on link change based on algorithm in "A New Approach
// to Dynamic All Pairs Shortest Paths" - C. Demetrescu

Disposition Routing_module::handle_link_change(const Event& e) {
	const Link_event& le = assert_cast<const Link_event&>(e);

	RouteQueue new_candidates;
	RoutePtr route(new Route());
	Link tmp = { le.dpdst, le.sport, le.dport };
	route->id.src = le.dpsrc;
	route->id.dst = le.dpdst;
	route->path.push_back(tmp);
	if (le.action == Link_event::REMOVE) {
		cleanup(route, true);
		fixup(new_candidates, true);
	} else if (le.action == Link_event::ADD) {
		RoutePtr left_subpath(new Route());
		RoutePtr right_subpath(new Route());
		left_subpath->id.src = left_subpath->id.dst = le.dpsrc;
		right_subpath->id.src = right_subpath->id.dst = le.dpdst;
		add(local_routes, route);
		add(left_local, route, right_subpath);
		add(right_local, route, left_subpath);
		new_candidates.push(route);
		fixup(new_candidates, false);
	} else {
		VLOG_ERR(lg, "Unknown link event action %u", le.action);
	}

	return CONTINUE;
}

Disposition Routing_module::manage_flow_expired(const Event& e){
    const Flow_removed_event& fre = assert_cast<const Flow_removed_event&>(e);
    //VLOG_DBG(lg,"Flow expired event");

    uint64_t key = fre.cookie;
    uint16_t path_id;
    CookiePathIdBindings::iterator ci = cookieBindings.find(key);
    
    VLOG_DBG(lg,"Searching for cookie: %"PRIx64"", key);

    if (ci != cookieBindings.end()){
        path_id = ci->second;
        cookieBindings.erase(ci);
        VLOG_DBG(lg,"Cookie found removing path id %"PRIx16"",path_id);
	remove_on_going_path(activePaths, path_id);
        Path_event* pe = new Path_event(path_id, Path_event::REMOVE);
	post(pe);
    }
    else{
        VLOG_DBG(lg,"Cookie not found, doing nothing...");
    }
    return CONTINUE;
}

void Routing_module::cleanup(RoutePtr route, bool delete_route) {
	bool is_short = remove(shortest, route);
	if (delete_route) {
		remove(local_routes, route);
	}

	RoutePtr subpath(new Route());
	*subpath = *route;
	set_subpath(subpath, true);
	if (delete_route) {
		remove(right_local, route, subpath);
	}
	if (is_short) {
		remove(right_shortest, route, subpath);
	}

	*subpath = *route;
	set_subpath(subpath, false);
	if (delete_route) {
		remove(left_local, route, subpath);
	}
	if (is_short) {
		remove(left_shortest, route, subpath);
	}

	RouteSet to_clean;
	subpath = route;
	while (true) {
		RouteList left, right;
		ExtensionMap::iterator rtes = left_local.find(subpath);
		if (rtes != left_local.end()) {
			left_shortest.erase(subpath);
			left.swap(rtes->second);
			left_local.erase(rtes);
		}
		rtes = right_local.find(subpath);
		if (rtes != right_local.end()) {
			right_shortest.erase(subpath);
			right.swap(rtes->second);
			right_local.erase(rtes);
		}

		clean_subpath(subpath, left, to_clean, true);
		clean_subpath(subpath, right, to_clean, false);

		if (to_clean.empty())
			break;
		subpath = *(to_clean.begin());
		to_clean.erase(to_clean.begin());
	}
}

void Routing_module::clean_subpath(RoutePtr& subpath,
		const RouteList& extensions, RouteSet& to_clean,
		bool is_left_extension) {
	datapathid tmpdp;
	Link tmplink;

	if (is_left_extension) {
		tmplink = subpath->path.back();
		subpath->path.pop_back();
		if (subpath->path.empty()) {
			subpath->id.dst = subpath->id.src;
		} else {
			subpath->id.dst = subpath->path.back().dst;
		}
	} else {
		tmplink = subpath->path.front();
		subpath->path.pop_front();
		tmpdp = subpath->id.src;
		subpath->id.src = tmplink.dst;
	}

	for (RouteList::const_iterator rte = extensions.begin();
			rte != extensions.end(); ++rte) {
		clean_route(*rte, subpath, is_left_extension);
		to_clean.insert(*rte);
	}

	if (is_left_extension) {
		subpath->path.push_back(tmplink);
		subpath->id.dst = tmplink.dst;
	} else {
		subpath->path.push_front(tmplink);
		subpath->id.src = tmpdp;
	}
}

void Routing_module::clean_route(const RoutePtr& route, RoutePtr& subpath,
		bool cleaned_left) {
	datapathid tmpdp;

	if (remove(local_routes, route)) {
		bool is_short = remove(shortest, route);
		if (cleaned_left) {
			tmpdp = subpath->id.src;
			subpath->id.src = route->id.src;
			subpath->path.push_front(route->path.front());
			remove(right_local, route, subpath);
			if (is_short)
				remove(right_shortest, route, subpath);
			subpath->id.src = tmpdp;
			subpath->path.pop_front();
		} else {
			const Link& newlink = route->path.back();
			tmpdp = subpath->id.dst;
			subpath->id.dst = newlink.dst;
			subpath->path.push_back(newlink);
			remove(left_local, route, subpath);
			if (is_short)
				remove(left_shortest, route, subpath);
			subpath->id.dst = tmpdp;
			subpath->path.pop_back();
		}
	}
}

void Routing_module::fixup(RouteQueue& new_candidates, bool add_least) {
	if (add_least) {
		for (RoutesMap::iterator rtes = local_routes.begin();
				rtes != local_routes.end(); ++rtes) {
			new_candidates.push(*(rtes->second.begin()));
		}
	}

	while (!new_candidates.empty()) {
		RoutePtr route = new_candidates.top();
		new_candidates.pop();
		RouteMap::iterator old = shortest.find(route->id);
		if (old != shortest.end()) {
			if (old->second->path.size() <= route->path.size()) {
				continue;
			}
			cleanup(old->second, false);
		} else if (route->id.src == route->id.dst) {
			continue;
		}

		RoutePtr left_subpath(new Route());
		RoutePtr right_subpath(new Route());
		*left_subpath = *right_subpath = *route;
		set_subpath(left_subpath, true);
		set_subpath(right_subpath, false);
		get_cached_path(left_subpath);
		get_cached_path(right_subpath);
		add(shortest, route);
		add(left_shortest, route, right_subpath);
		add(right_shortest, route, left_subpath);
		add_local_routes(route, left_subpath, right_subpath, new_candidates);
	}
}

void Routing_module::add_local_routes(const RoutePtr& route,
		const RoutePtr& left_subpath, const RoutePtr& right_subpath,
		RouteQueue& new_candidates) {
	ExtensionMap::iterator rtes = left_shortest.find(left_subpath);
	if (rtes != left_shortest.end()) {
		for (RouteList::const_iterator rte = rtes->second.begin();
				rte != rtes->second.end(); ++rte) {
			RoutePtr new_local(new Route());
			*new_local = *route;
			new_local->id.src = (*rte)->id.src;
			new_local->path.push_front((*rte)->path.front());
			add(local_routes, new_local);
			add(left_local, new_local, route);
			add(right_local, new_local, *rte);
			new_candidates.push(new_local);
		}
	}
	rtes = right_shortest.find(right_subpath);
	if (rtes != right_shortest.end()) {
		for (RouteList::const_iterator rte = rtes->second.begin();
				rte != rtes->second.end(); ++rte) {
			RoutePtr new_local(new Route());
			*new_local = *route;
			new_local->id.dst = (*rte)->id.dst;
			new_local->path.push_back((*rte)->path.back());
			add(local_routes, new_local);
			add(left_local, new_local, *rte);
			add(right_local, new_local, route);
			new_candidates.push(new_local);
		}
	}
}

void Routing_module::set_subpath(RoutePtr& subpath, bool left) {
	if (left) {
		subpath->path.pop_back();
		if (subpath->path.empty()) {
			subpath->id.dst = subpath->id.src;
			return;
		}
		subpath->id.dst = subpath->path.back().dst;
	} else {
		subpath->id.src = subpath->path.front().dst;
		subpath->path.pop_front();
	}
}

void Routing_module::get_cached_path(RoutePtr& route) {
	RoutesMap::iterator rtes = local_routes.find(route->id);
	if (rtes == local_routes.end()) {
		// for (dp, dp) paths
		ExtensionMap::iterator check = left_local.find(route);
		if (check != left_local.end()) {
			route = check->first;
		}
		return;
	}

	for (RouteList::const_iterator rte = rtes->second.begin();
			rte != rtes->second.end(); ++rte) {
		if (routeq()(route, *rte)) {
			route = *rte;
			return;
		}
	}
}

bool Routing_module::remove(RouteMap& routes, const RoutePtr& route) {
	RouteMap::iterator rtes = routes.find(route->id);
	if (rtes != routes.end()) {
		if (routeq()(route, rtes->second)) {
			routes.erase(rtes);
			return true;
		}
	}
	return false;
}

bool Routing_module::remove(RoutesMap& routes, const RoutePtr& route) {
	RoutesMap::iterator rtes = routes.find(route->id);
	if (rtes != routes.end()) {
		for (RouteList::iterator rte = rtes->second.begin();
				rte != rtes->second.end(); ++rte) {
			if (routeq()(route, *rte)) {
				rtes->second.erase(rte);
				if (rtes->second.empty())
					routes.erase(rtes);
				return true;
			}
		}
	}
	return false;
}

bool Routing_module::remove(ExtensionMap& routes, const RoutePtr& route,
		const RoutePtr& subpath) {
	ExtensionMap::iterator rtes = routes.find(subpath);
	if (rtes != routes.end()) {
		for (RouteList::iterator rte = rtes->second.begin();
				rte != rtes->second.end(); ++rte) {
			if (routeq()(route, *rte)) {
				rtes->second.erase(rte);
				if (rtes->second.empty()) {
					routes.erase(rtes);
				}
				return true;
			}
		}
	}
	return false;
}

void Routing_module::add(RouteMap& routes, const RoutePtr& route) {
	routes[route->id] = route;
}

void Routing_module::add(RoutesMap& routes, const RoutePtr& route) {
	RoutesMap::iterator rtes = routes.find(route->id);
	if (rtes == routes.end()) {
		routes[route->id] = RouteList(1, route);
	} else {
		uint32_t len = route->path.size();
		for (RouteList::iterator rte = rtes->second.begin();
				rte != rtes->second.end(); ++rte) {
			if ((*rte)->path.size() > len) {
				rtes->second.insert(rte, route);
				return;
			}
		}
		rtes->second.push_back(route);
	}
}

void Routing_module::add(ExtensionMap& routes, const RoutePtr& route,
		const RoutePtr& subpath) {
	ExtensionMap::iterator rtes = routes.find(subpath);
	if (rtes == routes.end()) {
		routes[subpath] = RouteList(1, route);
	} else {
		rtes->second.push_back(route);
	}
}

// Methods handling a Flow_in_event, setting up the route to permit the flow
// all the way to its destination without having to go up to the controller for
// a permission check.

#define CHECK_OF_ERR(error, dp)                                         \
    if (error) {                                                        \
        if (error == EAGAIN) {                                          \
            VLOG_DBG(lg, "Add flow entry to dp:%"PRIx64" failed with EAGAIN.", \
                     dp.as_host());                                     \
        } else {                                                        \
            VLOG_ERR(lg, "Add flow entry to dp:%"PRIx64" failed with %d:%s.", \
                     dp.as_host(), error, strerror(error));             \
        }                                                               \
        os.str("");                                                     \
        return false;                                                   \
    }

void Routing_module::init_openflow(uint16_t actions_len) {
	len_flow_actions = actions_len;
	raw_of.reset(new uint8_t[sizeof(*ofm) + len_flow_actions]);
	ofm = (ofp_flow_mod*) raw_of.get();

	ofm->header.version = OFP_VERSION;
	ofm->header.type = OFPT_FLOW_MOD;
	ofm->header.xid = openflow_pack::get_xid();

	ofm->match.wildcards = 0;
	memset(&ofm->match.pad1, 0, sizeof(ofm->match.pad1));
	memset(&ofm->match.pad2, 0, sizeof(ofm->match.pad2));
	ofm->cookie = 0;
	ofm->command = htons(OFPFC_ADD);
	ofm->priority = htons(OFP_DEFAULT_PRIORITY);
	ofm->flags = htons(ofd_flow_mod_flags());
}

void Routing_module::check_openflow(uint16_t actions_len) {
	if (actions_len <= len_flow_actions) {
		return;
	} else if (raw_of == NULL) {
		init_openflow(actions_len);
		return;
	}

	actions_len *= 2;

	boost::shared_array<uint8_t> old_of = raw_of;
	raw_of.reset(new uint8_t[sizeof(*ofm) + actions_len]);
	ofm = (ofp_flow_mod*) raw_of.get();
	memcpy(ofm, old_of.get(), sizeof(*ofm));
	len_flow_actions = actions_len;
}

void Routing_module::set_openflow(const Flow& flow, uint32_t buffer_id,
		uint16_t timeout) {
	if (len_flow_actions < max_output_action_len) {
		init_openflow(8 * max_output_action_len);
	}

	ofm->buffer_id = htonl(buffer_id);
	ofm->idle_timeout = htons(timeout);
	ofm->hard_timeout = htons(OFP_FLOW_PERMANENT);
	
	timeval curtime = { 0, 0 };
        gettimeofday(&curtime, NULL);
        
        long currentMu = curtime.tv_sec * 1000000 + curtime.tv_usec;
	
	ofm->cookie = htonll(flow.hash_code(currentMu));

	ofp_match& match = ofm->match;
	match.in_port = flow.in_port;
	memcpy(match.dl_src, flow.dl_src.octet, ethernetaddr::LEN);
	memcpy(match.dl_dst, flow.dl_dst.octet, ethernetaddr::LEN);
	match.dl_vlan = flow.dl_vlan;
	match.dl_vlan_pcp = flow.dl_vlan_pcp;
	match.dl_type = flow.dl_type;
	match.nw_src = flow.nw_src;
	match.nw_dst = flow.nw_dst;
	match.nw_tos = flow.nw_tos;
	match.nw_proto = flow.nw_proto;
	match.tp_src = flow.tp_src;
	match.tp_dst = flow.tp_dst;
}

bool Routing_module::set_openflow_actions(const Buffer& actions,
		const datapathid& dp, uint16_t outport, bool check_nat,
		bool allow_mac_overwrite) {
	check_openflow(actions.size() + max_output_action_len);
	memcpy(ofm->actions, actions.data(), actions.size());

	uint16_t packet_len = sizeof(*ofm) + actions.size();
	bool nat_overwritten;
	bool natted = set_action(((uint8_t*) ofm->actions) + actions.size(), dp,
			outport, packet_len, check_nat, allow_mac_overwrite, 0,
			nat_overwritten);
	ofm->header.length = htons(packet_len);
	return natted;
}

void Routing_module::modify_match(const Buffer& actions) {
	const uint8_t *pos = actions.data();
	uint16_t ofs = 0;

	ofp_match& match = ofm->match;

	while (ofs < actions.size()) {
		const ofp_action_header *action = (ofp_action_header*) (pos + ofs);

		switch (ntohs(action->type)) {
		case OFPAT_OUTPUT:
		case OFPAT_VENDOR:
			break;
		case OFPAT_SET_VLAN_PCP:
			match.dl_vlan_pcp = ((ofp_action_vlan_pcp*) action)->vlan_pcp;
			break;
		case OFPAT_SET_VLAN_VID:
			match.dl_vlan = ((ofp_action_vlan_vid*) action)->vlan_vid;
			break;
		case OFPAT_STRIP_VLAN:
			match.dl_vlan = htons(OFP_VLAN_NONE);
			break;
		case OFPAT_SET_DL_SRC:
			memcpy(match.dl_src, ((ofp_action_dl_addr*) action)->dl_addr,
					ethernetaddr::LEN);
			break;
		case OFPAT_SET_DL_DST:
			memcpy(match.dl_dst, ((ofp_action_dl_addr*) action)->dl_addr,
					ethernetaddr::LEN);
			break;
		case OFPAT_SET_NW_SRC:
			match.nw_src = ((ofp_action_nw_addr*) action)->nw_addr;
			break;
		case OFPAT_SET_NW_DST:
			match.nw_dst = ((ofp_action_nw_addr*) action)->nw_addr;
			break;
		case OFPAT_SET_TP_SRC:
			match.tp_src = ((ofp_action_tp_port*) action)->tp_port;
			break;
		case OFPAT_SET_TP_DST:
			match.tp_dst = ((ofp_action_tp_port*) action)->tp_port;
			break;
		default:
			VLOG_WARN(
					lg,
					"Routing_module doesn't recognize Openflow action %"PRIu16".", ntohs(action->type));
		}
		ofs += ntohs(action->len);
	}
}

bool Routing_module::setup_flow(const Flow& flow, const datapathid& dp,
		uint16_t outport, uint32_t bid, const Buffer& buf,
		uint16_t flow_timeout, const Buffer& actions, bool check_nat,
		const GroupList *sdladdr_groups, const GroupList *snwaddr_groups,
		const GroupList *ddladdr_groups, const GroupList *dnwaddr_groups) {
	uint32_t inport = ntohs(flow.in_port);
	uint16_t actions_len = actions.size();
	uint64_t overwrite = 0;
	bool nat_overwritten = false;

	set_openflow(flow, bid, flow_timeout);
	check_openflow(actions_len + max_output_action_len);
	memcpy(ofm->actions, actions.data(), actions.size());

	if (!check_nat) {
		set_action((uint8_t*) ofm->actions + actions_len, dp, outport,
				actions_len, false, false, 0, nat_overwritten);
	} else {
		nat->get_nat_locations(&flow, sdladdr_groups, snwaddr_groups,
				ddladdr_groups, dnwaddr_groups, nat_flow);
		bool allow_overwrite = !flow.dl_dst.is_multicast();
		if (outport == OFPP_FLOOD) {
			uint64_t orig_dl = flow.dl_dst.hb_long();
			const Topology::DpInfo& dpinfo = topology->get_dpinfo(dp);
			check_openflow(
					(dpinfo.ports.size() * max_output_action_len)
							+ actions_len);
			for (Topology::PortVector::const_iterator p_iter =
					dpinfo.ports.begin(); p_iter != dpinfo.ports.end();
					++p_iter) {
				if (p_iter->port_no != inport && p_iter->port_no < OFPP_MAX) {
					set_action(((uint8_t*) ofm->actions) + actions_len, dp,
							p_iter->port_no, actions_len, true, allow_overwrite,
							overwrite, nat_overwritten);
					overwrite = nat_overwritten ? orig_dl : 0;
				}
			}
			if (actions_len == actions.size()) {
				VLOG_ERR(lg, "No outports to set flow actions with.");
				return false;
			}
		} else {
			set_action((uint8_t*) ofm->actions + actions_len, dp, outport,
					actions_len, true, allow_overwrite, 0, nat_overwritten);
		}
	}
	ofm->header.length = htons(sizeof(*ofm) + actions_len);
	int ret = send_openflow_command(dp, &ofm->header, false);
	bool buf_sent = false;
	if (!ret && bid == UINT32_MAX) {
		buf_sent = true;
		ret = send_openflow_packet(dp, buf, ofm->actions, actions_len, inport,
				false);
	}

	if (ret) {
		if (ret == EAGAIN) {
			VLOG_DBG(
					lg,
					"Send openflow %s to dp:%"PRIx64" failed with EAGAIN.", buf_sent ? "packet" : "command", dp.as_host());
		} else {
			VLOG_ERR(
					lg,
					"Send openflow %s to dp:%"PRIx64" failed with %d:%s.", buf_sent ? "packet" : "command", dp.as_host(), ret, strerror(ret));
		}
		return false;
	}

	return true;
}

bool Routing_module::setup_route(const Flow& flow, const Route& route,
		uint16_t ap_inport, uint16_t ap_outport, uint16_t flow_timeout,
		const ActionList& actions, bool check_nat,
		const GroupList *sdladdr_groups, const GroupList *snwaddr_groups,
		const GroupList *ddladdr_groups, const GroupList *dnwaddr_groups) {
	if (lg.is_dbg_enabled()) {
		os << flow;
		VLOG_DBG(lg, "Route: %s", os.str().c_str());
		os.str("");
		os << std::hex;
	}
	bool actions_def = !actions.empty();
	
	if (actions_def && actions.size() != route.path.size() + 1) {
		VLOG_ERR(lg, "Invalid length of ActionList, not enforcing.");
		actions_def = false;
	}

	set_openflow(flow, UINT32_MAX, flow_timeout);

	std::list<Link>::const_iterator link = route.path.begin();
	ActionList::const_iterator action = actions.begin();

	datapathid dp = route.id.src;
	uint16_t outport, inport = ap_inport;
	bool nat_match = false;
	if (check_nat) {
		nat->get_nat_locations(&flow, sdladdr_groups, snwaddr_groups,
				ddladdr_groups, dnwaddr_groups, nat_flow);
	}
	while (true) {
		if (link == route.path.end()) {
			outport = ap_outport;
		} else {
			outport = link->outport;
		}

		if (inport == outport) {
			VLOG_WARN(
					lg,
					"Entry on dp:%"PRIx64" routes out inport:%"PRIu16".", dp.as_host(), inport);
		}

		bool allow_overwrite = !flow.dl_dst.is_multicast();
		if (actions_def) {
			nat_match = set_openflow_actions(*action, dp, outport, check_nat,
					allow_overwrite);
		} else {
			uint16_t packet_len = sizeof(*ofm);
			bool nat_overwritten;
			nat_match = set_action((uint8_t*) ofm->actions, dp, outport,
					packet_len, check_nat, allow_overwrite, 0, nat_overwritten);
			ofm->header.length = htons(packet_len);
		}
		ofm->match.in_port = htons(inport);

		int err = send_openflow_command(dp, &ofm->header, false);
		CHECK_OF_ERR(err, dp);

		if (lg.is_dbg_enabled()) {
			os << inport << ":" << dp.as_host() << ':' << outport;
		}

		if (link == route.path.end() || nat_match) {
			break;
		}

		if (actions_def) {
			modify_match(*action);
			++action;
		}

		dp = link->dst;
		inport = link->inport;
		++link;
		if (lg.is_dbg_enabled()) {
			os << " --> ";
		}
	}

	if (lg.is_dbg_enabled()) {
		if (nat_match) {
			os << " NAT";
		}VLOG_DBG(lg, "%s", os.str().c_str());
		os.str("");
	}
	return true;
}

// return true if nat-ed, else false
bool Routing_module::set_action(uint8_t *action, const datapathid& dp,
		uint16_t port, uint16_t& actions_len, bool check_nat,
		bool allow_mac_overwrite, uint64_t overwrite_mac,
		bool& nat_overwritten) {
	uint64_t nat_dladdr = 0;
	bool natted = check_nat
			&& nat->nat_location(dp.as_host() + (((uint64_t) port) << 48),
					nat_dladdr, nat_flow);
	nat_overwritten = false;
	if (natted && allow_mac_overwrite && nat_dladdr != 0) {
		nat_overwritten = true;
		overwrite_mac = nat_dladdr;
	}
	if (overwrite_mac != 0) {
		ofp_action_dl_addr& dlset = *((ofp_action_dl_addr*) action);
		memset(&dlset, 0, sizeof(ofp_action_dl_addr));
		dlset.type = htons(OFPAT_SET_DL_DST);
		dlset.len = htons(sizeof(ofp_action_dl_addr));
		memcpy(dlset.dl_addr, ethernetaddr(overwrite_mac).octet,
				ethernetaddr::LEN);
		actions_len += sizeof(ofp_action_dl_addr);
		action += sizeof(ofp_action_dl_addr);
	}
	if (natted) {
		nx_action_snat& snat = *((nx_action_snat*) action);
		memset(&snat, 0, sizeof(nx_action_snat));
		snat.type = htons(OFPAT_VENDOR);
		snat.len = htons(sizeof(snat));
		snat.vendor = htonl(NX_VENDOR_ID);
		snat.subtype = htons(NXAST_SNAT);
		snat.port = htons(port);
		actions_len += sizeof(nx_action_snat);
		return true;
	}
	ofp_action_output& output = *((ofp_action_output*) action);
	memset(&output, 0, sizeof(ofp_action_output));
	output.type = htons(OFPAT_OUTPUT);
	output.len = htons(sizeof(output));
	output.max_len = 0;
	output.port = htons(port);
	actions_len += sizeof(ofp_action_output);
	return false;
}

bool Routing_module::send_packet(const datapathid& dp, uint16_t inport,
		uint16_t outport, uint32_t bid, const Buffer& buf,
		const Buffer& actions, bool check_nat, const Flow& flow,
		const GroupList *sdladdr_groups, const GroupList *snwaddr_groups,
		const GroupList *ddladdr_groups, const GroupList *dnwaddr_groups) {
	uint16_t actions_len = actions.size();
	uint64_t overwrite = 0;
	bool nat_overwritten = false;

	check_openflow(actions_len + max_output_action_len);
	memcpy(ofm->actions, actions.data(), actions.size());

	if (!check_nat) {
		set_action(((uint8_t*) ofm->actions) + actions_len, dp, outport,
				actions_len, false, false, 0, nat_overwritten);
	} else {
		nat->get_nat_locations(&flow, sdladdr_groups, snwaddr_groups,
				ddladdr_groups, dnwaddr_groups, nat_flow);
		bool allow_overwrite = !flow.dl_dst.is_multicast();
		if (outport == OFPP_FLOOD) {
			const Topology::DpInfo& dpinfo = topology->get_dpinfo(dp);
			check_openflow(
					actions_len
							+ (dpinfo.ports.size() * max_output_action_len));
			uint64_t orig_dl = flow.dl_dst.hb_long();
			for (Topology::PortVector::const_iterator p_iter =
					dpinfo.ports.begin(); p_iter != dpinfo.ports.end();
					++p_iter) {
				if (p_iter->port_no != inport && p_iter->port_no < OFPP_MAX) {
					set_action(((uint8_t*) ofm->actions) + actions_len, dp,
							p_iter->port_no, actions_len, true, allow_overwrite,
							overwrite, nat_overwritten);
					overwrite = nat_overwritten ? orig_dl : 0;
				}
			}
			if (actions_len == actions.size()) {
				VLOG_ERR(lg, "No outports to set packet actions with.");
				return false;
			}
		} else {
			set_action(((uint8_t*) ofm->actions) + actions_len, dp, outport,
					actions_len, true, allow_overwrite, 0, nat_overwritten);
		}
	}

	int ret;
	if (bid == UINT32_MAX) {
		ret = send_openflow_packet(dp, buf, ofm->actions, actions_len, inport,
				false);
	} else {
		ret = send_openflow_packet(dp, bid, ofm->actions, actions_len, inport,
				false);
	}

	if (ret) {
		if (ret == EAGAIN) {
			VLOG_DBG(
					lg,
					"Send packet to dp:%"PRIx64" failed with EAGAIN.", dp.as_host());
		} else {
			VLOG_ERR(
					lg,
					"Send packet to dp:%"PRIx64" failed with %d:%s.", dp.as_host(), ret, strerror(ret));
		}
		return false;
	}

	return true;
}

}
}

REGISTER_COMPONENT(
		vigil::container::Simple_component_factory <vigil::applications::Routing_module>,
		vigil::applications::Routing_module);
