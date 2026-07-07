// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
#ifndef EXPERIMENT_LOGGER_H
#define EXPERIMENT_LOGGER_H

#include <fstream>
#include <string>
#include <stdint.h>
#include "network.h"

struct ExperimentConfig {
    int num_planes;
    int sats_per_plane;
    int orbital_slots;
    bool adjacent_sats;
    int sat_offset;

    double duration_s;
    double route_update_ms;
    double queue_sample_ms;
    double ping_interval_ms;

    uint64_t uplink_mbps;
    uint64_t downlink_mbps;
    uint64_t isl_mbps;
    int queue_pkts;
    int packet_size_bytes;

    double src_lat;
    double src_lon;
    double dst_lat;
    double dst_lon;

    std::string out_prefix;
    bool routing_only;
    bool verbose;
};

struct RouteMetrics {
    bool found;
    double rtt_ms;
    double one_way_latency_ms;
    std::string route_hash;
    bool changed;
    double route_duration_s;
    int link_hops;
    int isl_hops;
    int ground_hops;
    double dijkstra_cpu_ms;
};

class ExperimentLogger {
public:
    explicit ExperimentLogger(const std::string& prefix);
    ~ExperimentLogger();

    void log_config(const ExperimentConfig& cfg, int num_sats);
    void log_route(simtime_picosec t,
                   const std::string& direction,
                   const RouteMetrics& metrics,
                   int active_uplinks_src,
                   int active_uplinks_dst,
                   int inactive_sats_src,
                   int inactive_sats_dst,
                   double nearest_src_km,
                   double nearest_dst_km,
                   const ExperimentConfig& cfg,
                   int num_sats);
    void log_visibility(simtime_picosec t,
                        const std::string& endpoint,
                        int active_uplinks,
                        int inactive_sats,
                        double nearest_sat_km);
    void log_summary(const std::string& key, const std::string& value);

private:
    std::string _prefix;
    std::ofstream _routes;
    std::ofstream _visibility;
    std::ofstream _summary;
};

#endif
