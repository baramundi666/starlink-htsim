// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-                                                                                                               
#ifndef CONSTELLATION_H
#define CONSTELLATION_H

class Constellation;
#include "isl.h"
#include "satellite.h"
#include "city.h"
#include "binary_heap.h"

#define MAXNODES 1600

enum LinkType {UPLINK=0, DOWNLINK=1, ISL=2};

class Constellation {
public:
    // num_planes: how many orbital planes to populate. Must divide 24
    // evenly (1,2,3,4,6,8,12,24) so the populated planes stay uniformly
    // spread around the globe at their real raan spacing, reproducing the
    // paper's partial-deployment scenarios (Sec 2.1, Figs 2/3/6). Planes
    // that aren't populated simply contain no satellites, so ISL
    // connectivity across the gap is lost -- this codebase has no
    // ground-relay chaining, so partial deployment here only tells you
    // "does an ISL path exist at all", not ground-relay latency.
    //
    // sats_per_plane: satellites per populated plane (default 66, the
    // real Starlink phase-1 value). Use a tiny value (e.g. 2) together
    // with num_planes=1 to build a minimal sanity-test topology (see
    // README scenario 0): a single ring of a handful of satellites with
    // no cross-plane ISLs, real orbital mechanics still apply.
    //
    // NOTE on accuracy: the cross-plane (east/west) ISL wraparound offset
    // math below was tuned by the original authors specifically for the
    // 24-plane/66-satellite case. For num_planes>1 and !=24 we approximate
    // it by treating the populated planes as a coarser ring of the same
    // shape; this is good enough to explore connectivity/latency trends
    // but is NOT a validated reproduction of the paper's partial-
    // deployment numbers -- treat it as approximate.
    // Keep the original constructor signature for the rest of htsim
    // (notably XcpNetworkTopology).  Do not add the new experiment-only
    // defaults to this signature, otherwise existing 7/9-argument calls are
    // compiled as calls to the newer 12-argument symbol and old build targets
    // fail at link time.
    Constellation(EventList& eventlist,
		  linkspeed_bps uplinkbitrate, mem_b uplinkqueuesize,
		  linkspeed_bps dowlinkbitrate, mem_b downlinkqueuesize,
		  linkspeed_bps islbitrate, mem_b islqueuesize,
		  int num_planes = 24, int sats_per_plane = 66);

    // Extended constructor used only by starlink_exp benchmark runs.
    Constellation(EventList& eventlist,
		  linkspeed_bps uplinkbitrate, mem_b uplinkqueuesize,
		  linkspeed_bps dowlinkbitrate, mem_b downlinkqueuesize,
		  linkspeed_bps islbitrate, mem_b islqueuesize,
		  int num_planes, int sats_per_plane,
		  int orbital_slots, bool adjacent_sats,
		  int sat_offset);
    Satellite** sats() {return _sats;}
    int num_sats() const {return _num_sats;}
    inline Link& activate_link(Node& src, Node& dst, LinkType linktype) {
	return _link_factory.activate_link(src, dst,
					   _linkbitrate[linktype],
					   _linkqueuesize[linktype]);
    }
    inline void drop_link(Link& link) {
	_link_factory.drop_link(link);
    }
    void dijkstra_up_all_links();
    void dijkstra_down_links_in_route(Route* route);
    simtime_picosec get_rtt(Route* route);
    void dijkstra(City& src, City& dst, simtime_picosec time);
    Route* find_route(City& dst);
    inline mem_b uplinkqueuesize() const {return _linkqueuesize[::UPLINK];}
    inline mem_b downlinkqueuesize() const {return _linkqueuesize[::DOWNLINK];}
    inline mem_b isllinkqueuesize() const {return _linkqueuesize[::ISL];}
    inline linkspeed_bps uplinkbitrate() const {return _linkbitrate[::UPLINK];}
    inline linkspeed_bps downlinkbitrate() const {return _linkbitrate[::DOWNLINK];}
    inline linkspeed_bps isllinkbitrate() const {return _linkbitrate[::ISL];}
    
private:
    Link_factory _link_factory;
    Satellite *_sats[MAXNODES];
    linkspeed_bps _linkbitrate[3];
    mem_b _linkqueuesize[3];
    int _num_sats;
    int _num_planes;
    int _sats_per_plane;
    int _orbital_slots;
    bool _adjacent_sats;
    int _sat_offset;
    BinaryHeap heap;
    Node* _route_src;
};

#endif