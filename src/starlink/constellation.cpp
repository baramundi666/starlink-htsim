// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-                                             

#include "binary_heap.h"
#include "constellation.h"

#include <cstdlib>
#include <iostream>
#include <set>
#include <sstream>

#ifdef STARLINK_VERBOSE_DEBUG
#define STARLINK_DEBUG_LOG(x) do { x; } while (0)
#else
#define STARLINK_DEBUG_LOG(x) do { } while (0)
#endif

using std::cerr;
using std::cout;
using std::endl;

// Must set Link::_logfile,
// Link::_queue_logger_sampling_interval,
// MultipathXcpSink::_logfile,
// MultipathXcpSink::_eventlist,
// MultipathXcpSrc::network_topology,
// MultipathXcpSrc::_logfile before use
// Original ABI-compatible constructor used by existing htsim components.
// It delegates to the experiment constructor with the canonical 66 orbital
// slots and spread satellite placement.
Constellation::Constellation(EventList& eventlist,
                  linkspeed_bps uplinkbitrate, mem_b uplinkqueuesize,
                  linkspeed_bps downlinkbitrate, mem_b downlinkqueuesize,
                  linkspeed_bps islbitrate, mem_b islqueuesize,
                  int num_planes, int sats_per_plane)
    : Constellation(eventlist,
                    uplinkbitrate, uplinkqueuesize,
                    downlinkbitrate, downlinkqueuesize,
                    islbitrate, islqueuesize,
                    num_planes, sats_per_plane,
                    66, false, 0)
{
}

Constellation::Constellation(EventList& eventlist,
                  linkspeed_bps uplinkbitrate, mem_b uplinkqueuesize,
                  linkspeed_bps downlinkbitrate, mem_b downlinkqueuesize,
                  linkspeed_bps islbitrate, mem_b islqueuesize,
                  int num_planes, int sats_per_plane,
                  int orbital_slots, bool adjacent_sats, int sat_offset) :
    _link_factory(eventlist), heap(MAXNODES), _route_src(0)
{
#ifdef XCP_STATIC_NETWORK
    _linkbitrate[::UPLINK] = uplinkbitrate;
    _linkbitrate[::DOWNLINK] = downlinkbitrate;
    _linkbitrate[::ISL] = islbitrate;
    
    _linkqueuesize[::UPLINK] = uplinkqueuesize;
    _linkqueuesize[::DOWNLINK] = downlinkqueuesize;
    _linkqueuesize[::ISL] = islqueuesize;
    
    Node::link_factory = &_link_factory;

	_sats[0] = new Satellite(0, 0, 0, 53.0, -5, 10, 0, 0);
	_sats[1] = new Satellite(0, 1, 1, 53.0, 0, 10, 0, 0);
	_sats[2] = new Satellite(0, 2, 2, 53.0, 5, 10, 0, 0);
	_sats[3] = new Satellite(0, 3, 3, 53.0, -5, 0, 0, 0);
	_sats[4] = new Satellite(0, 4, 4, 53.0, 5, 0, 0, 0);
	_sats[5] = new Satellite(0, 5, 5, 53.0, -5, -10, 0, 0);
	_sats[6] = new Satellite(0, 6, 6, 53.0, 0, -10, 0, 0);
	_sats[7] = new Satellite(0, 7, 7, 53.0, 5, -10, 0, 0);

	_num_sats = 8;

	_sats[0]->add_link_to_dst(*(_sats[1]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[1]->add_link_to_dst(*(_sats[0]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[1]->add_link_to_dst(*(_sats[2]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[2]->add_link_to_dst(*(_sats[1]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[3]->add_link_to_dst(*(_sats[4]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[4]->add_link_to_dst(*(_sats[3]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[5]->add_link_to_dst(*(_sats[6]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[6]->add_link_to_dst(*(_sats[5]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[6]->add_link_to_dst(*(_sats[7]), _linkbitrate[ISL], _linkqueuesize[ISL]);
	_sats[7]->add_link_to_dst(*(_sats[6]), _linkbitrate[ISL], _linkqueuesize[ISL]);

#else
    _linkbitrate[::UPLINK] = uplinkbitrate;
    _linkbitrate[::DOWNLINK] = downlinkbitrate;
    _linkbitrate[::ISL] = islbitrate;
    
    _linkqueuesize[::UPLINK] = uplinkqueuesize;
    _linkqueuesize[::DOWNLINK] = downlinkqueuesize;
    _linkqueuesize[::ISL] = islqueuesize;
    
    Node::link_factory = &_link_factory;

    // Sanitize experiment parameters.  MAXNODES is fixed in the original
    // code, so keep generated topologies inside that bound.
    if (num_planes < 1) num_planes = 1;
    if (num_planes > 24) num_planes = 24;
    if (sats_per_plane < 1) sats_per_plane = 1;
    if (orbital_slots < sats_per_plane) orbital_slots = sats_per_plane;

    _num_planes = num_planes;
    _sats_per_plane = sats_per_plane;
    _orbital_slots = orbital_slots;
    _adjacent_sats = adjacent_sats;
    _sat_offset = sat_offset;

    if (_num_planes * _sats_per_plane > MAXNODES) {
	cerr << "Requested topology has " << (_num_planes * _sats_per_plane)
	     << " satellites, but MAXNODES is " << MAXNODES << endl;
	abort();
    }

    // real Starlink FCC-filing geometry is fixed at 24 planes; we
    // "populate" only every (24/num_planes)-th plane so the ones we do
    // use stay at their real, globally-uniform raan spacing (this is what
    // the paper's Fig 2/3 partial-deployment scenarios do).
    int plane_step = 24 / _num_planes;      // real orbital-plane index stride
    if (plane_step < 1) plane_step = 1;
    int sats_per_orbit = _sats_per_plane;

    double raan = 0;
    int satnum = -1;
    for (int slot = 0; slot < _num_planes; slot++) {
	int plane = slot * plane_step;     // real plane index, 0..23
	double mean_anomaly = 0;
	for (int sat = 0; sat < sats_per_orbit; sat++) {
	    // Two useful modes:
	    //  - spread: populated satellites are evenly spread over the plane;
	    //  - adjacent: populate consecutive FCC slots from a 66-slot plane.
	    //    This is essential for the 2-satellite sanity benchmark, because
	    //    "2 satellites in a plane" would otherwise place them opposite
	    //    each other and make the test unrealistic.
	    int slot_id = _adjacent_sats ? (_sat_offset + sat) : sat;
	    int anomaly_denominator = _adjacent_sats ? _orbital_slots : sats_per_orbit;
	    mean_anomaly = (plane * 13.0/24.0 + slot_id) * 360.0 / anomaly_denominator;
	    raan = plane * 360.0 / 24;
	    satnum = sat + slot*sats_per_orbit;
	    _sats[satnum] = new Satellite(plane, sat, satnum, 53.0, raan,
					 mean_anomaly, 5739.0, 550.0 /* alt in km */);
	}
    }
    _num_sats = satnum + 1;

    int isl_plane_shift = -1;
    int isl_plane_step = 1;
    for (int slot = 0; slot < _num_planes; slot++) {
	for (int sat = 0; sat < sats_per_orbit; sat++) {
	    int satnum = sat + slot*sats_per_orbit;
	    Satellite *s = _sats[satnum];

	    // In-plane ISLs.  With only two satellites, next and prev are the
	    // same node; do not add the duplicate directed link twice.
	    int nextidx = (sat + 1) % sats_per_orbit;
	    int previdx = (sat - 1 + sats_per_orbit) % sats_per_orbit;
	    Satellite *nextsat = _sats[slot*sats_per_orbit + nextidx];
	    Satellite *prevsat = _sats[slot*sats_per_orbit + previdx];
	    if (nextsat != s) {
		s->add_link_to_dst(*nextsat, _linkbitrate[ISL], _linkqueuesize[ISL]);
	    }
	    if (prevsat != s && prevsat != nextsat) {
		s->add_link_to_dst(*prevsat, _linkbitrate[ISL], _linkqueuesize[ISL]);
	    }

	    // Cross-plane (east/west) ISLs only make sense once we have more
	    // than one populated plane. With exactly the original 24 planes
	    // we keep the authors' exact tuned wraparound math; for any
	    // other num_planes we use a simplified wrap around the populated
	    // slots (approximate -- see header comment).
	    if (_num_planes == 1) {
		continue; // single ring: no cross-plane ISLs at all
	    }

	    if (_num_planes == 24) {
		int leftoffset = isl_plane_step*sats_per_orbit + isl_plane_shift;
		while ((sat + leftoffset) / sats_per_orbit < isl_plane_step) {
		    leftoffset += sats_per_orbit;
		}
		while ((sat + leftoffset) / sats_per_orbit > isl_plane_step) {
		    leftoffset -= sats_per_orbit;
		}
		int leftsatnum = satnum + leftoffset;
		if (leftsatnum >= _num_sats) {
		    leftsatnum = leftsatnum - _num_sats + 13;
		    if (leftsatnum >= sats_per_orbit) {
			leftsatnum -= sats_per_orbit;
		    }
		}
		Satellite *leftsat = _sats[leftsatnum];
		s->add_link_to_dst(*leftsat, _linkbitrate[ISL], _linkqueuesize[ISL]);

		int rightoffset = -isl_plane_step*sats_per_orbit - isl_plane_shift;
		while (floor((sat + rightoffset) / (double)sats_per_orbit) < -isl_plane_step) {
		    rightoffset += sats_per_orbit;
		}
		while (floor((sat + rightoffset) / (double) sats_per_orbit) > -isl_plane_step) {
		    rightoffset -= sats_per_orbit;
		}
		int rightsatnum = satnum + rightoffset;
		if (rightsatnum < 0) {
		    rightsatnum = rightsatnum + _num_sats - 13;
		    if (rightsatnum < _num_sats - sats_per_orbit) {
			rightsatnum += sats_per_orbit;
		    }
		}
		Satellite *rightsat = _sats[rightsatnum];
		s->add_link_to_dst(*rightsat, _linkbitrate[ISL], _linkqueuesize[ISL]);
	    } else {
		// Simplified approximate wrap: connect same sat-in-plane
		// index in the neighbouring populated slot, ring-wrapped.
		int leftslot = (slot + 1) % _num_planes;
		int rightslot = (slot - 1 + _num_planes) % _num_planes;
		Satellite *leftsat = _sats[sat + leftslot*sats_per_orbit];
		Satellite *rightsat = _sats[sat + rightslot*sats_per_orbit];
		if (leftsat != s) {
		    s->add_link_to_dst(*leftsat, _linkbitrate[ISL], _linkqueuesize[ISL]);
		}
		if (rightsat != s && rightsat != leftsat) {
		    s->add_link_to_dst(*rightsat, _linkbitrate[ISL], _linkqueuesize[ISL]);
		}
	    }
	}
    }
    /*
    for (int sat=0; sat < _num_sats; sat++) {
	_sats[sat]->print_links();
    }
    
    for (int sat=_num_sats-66; sat < _num_sats; sat++) {
	_sats[sat]->print_links();
    }
    */
#endif
}

void
Constellation::dijkstra(City& src, City& dst, simtime_picosec time) {
    assert(heap.size() == 0);
	src.update_uplinks(time);
    dst.update_uplinks(time);
    _route_src = &src;
    heap.insert(src);
    for (int sat=0; sat < _num_sats; sat++) {
		_sats[sat]->clear_routing();
		heap.insert_at_infinity(*_sats[sat]);
    }
	dst.clear_routing();
    heap.insert_at_infinity(dst);
    src.set_dist(0);
    while (heap.size() > 0) {
	Node& u = heap.extract_min();
	//for (int i = 0; i < u.link_count(); i++) {
	unordered_set<Link*>::iterator i;;
	for (i = u._links.begin(); i != u._links.end(); i++) {
		if ((*i)->is_dijkstra_up()) {
			Link& l = *(*i);
			Node& n = l.get_neighbour(u);
			simtime_picosec dist = n.dist();
			simtime_picosec linkdelay = l.delay(time);
			//cout << "ps2:" << linkdelay << endl;
			simtime_picosec newdist = u.dist() + linkdelay;
			//cout << "newdist:" << newdist << endl;
			if (newdist < dist) {
			//cout << "newdist2:" << newdist << " was " << dist << endl;
			n.set_dist(newdist);
			n.set_parent(u, l);
			heap.decrease_distance(n, newdist);
			}
		}
	}
    }
}

void Constellation::dijkstra_down_links_in_route(Route* route) {
	for (auto it = route->begin() ; it != route->end() ; ++it) {
		Link* link = dynamic_cast<Link*>(*it);
		if (link != NULL) {
			link->dijkstra_down();
		}
	}
}

simtime_picosec Constellation::get_rtt(Route* route) {
	simtime_picosec rtt = 0;
	for (auto it = route->begin() ; it != route->end() ; ++it) {
		Link* link = dynamic_cast<Link*>(*it);
		if (link != NULL) {
			rtt += link->retrieve_delay();
		}
	}

	return rtt * 2;
}


Route*
Constellation::find_route(City& dst) {
    //cout << "find_route" << endl;
    Route* route = new Route();
    Node* n = &dst;
    while (n && n != _route_src) {
	Link* l = n->parentlink();
	if (l) {
	    //cout << l->nodename() << " " << this << endl;
	    route->push_front(static_cast<PacketSink*>(l));
	    //cout << l->queue().nodename() << " " << this << endl;
	    route->push_front(static_cast<PacketSink*>(l->queue()));
	}
	n = n->parent();
	if (n == NULL) {
	    STARLINK_DEBUG_LOG(cout << "no next hop" << endl);
	    break;
	}
    }
    //cout << _route_src->position() << endl;
    //cout << "Latency: " << timeAsMs(dst.dist()) << endl;

    /*
    cout << "test" << endl;
    for (int sat=0; sat < _num_sats; sat++) {
        cout << "sat: " << sat << " dist: " << _sats[sat]->dist() << endl;
    }
    */

    if (std::distance(route->begin(),route->end()) < 2 || &dynamic_cast<Link*>(*(++(route->begin())))->src() != _route_src) {
		delete route;
		return NULL;
	}

    return route;
}

void Constellation::dijkstra_up_all_links() {
	_link_factory.dijkstra_up_all_links();
}