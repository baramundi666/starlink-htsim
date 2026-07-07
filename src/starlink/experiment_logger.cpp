// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
#include "experiment_logger.h"

#include <iomanip>
#include <iostream>

using std::endl;

ExperimentLogger::ExperimentLogger(const std::string& prefix)
    : _prefix(prefix),
      _routes((prefix + ".routes.csv").c_str()),
      _visibility((prefix + ".visibility.csv").c_str()),
      _summary((prefix + ".summary.csv").c_str())
{
    _routes << "time_s,direction,route_found,rtt_ms,one_way_latency_ms,route_hash,"
            << "route_changed,route_duration_s,link_hops,isl_hops,ground_hops,"
            << "active_uplinks_src,active_uplinks_dst,inactive_sats_src,inactive_sats_dst,"
            << "nearest_src_km,nearest_dst_km,dijkstra_cpu_ms,"
            << "num_planes,sats_per_plane,orbital_slots,adjacent_sats,num_sats"
            << endl;

    _visibility << "time_s,endpoint,active_uplinks,inactive_sats,nearest_sat_km" << endl;
    _summary << "key,value" << endl;
}

ExperimentLogger::~ExperimentLogger() {
    _routes.close();
    _visibility.close();
    _summary.close();
}

void ExperimentLogger::log_config(const ExperimentConfig& cfg, int num_sats) {
    std::ofstream json((_prefix + ".config.json").c_str());
    json << "{\n";
    json << "  \"num_planes\": " << cfg.num_planes << ",\n";
    json << "  \"sats_per_plane\": " << cfg.sats_per_plane << ",\n";
    json << "  \"orbital_slots\": " << cfg.orbital_slots << ",\n";
    json << "  \"adjacent_sats\": " << (cfg.adjacent_sats ? "true" : "false") << ",\n";
    json << "  \"sat_offset\": " << cfg.sat_offset << ",\n";
    json << "  \"num_sats_actual\": " << num_sats << ",\n";
    json << "  \"duration_s\": " << cfg.duration_s << ",\n";
    json << "  \"route_update_ms\": " << cfg.route_update_ms << ",\n";
    json << "  \"queue_sample_ms\": " << cfg.queue_sample_ms << ",\n";
    json << "  \"ping_interval_ms\": " << cfg.ping_interval_ms << ",\n";
    json << "  \"uplink_mbps\": " << cfg.uplink_mbps << ",\n";
    json << "  \"downlink_mbps\": " << cfg.downlink_mbps << ",\n";
    json << "  \"isl_mbps\": " << cfg.isl_mbps << ",\n";
    json << "  \"queue_pkts\": " << cfg.queue_pkts << ",\n";
    json << "  \"packet_size_bytes\": " << cfg.packet_size_bytes << ",\n";
    json << "  \"src_lat\": " << cfg.src_lat << ",\n";
    json << "  \"src_lon\": " << cfg.src_lon << ",\n";
    json << "  \"dst_lat\": " << cfg.dst_lat << ",\n";
    json << "  \"dst_lon\": " << cfg.dst_lon << ",\n";
    json << "  \"routing_only\": " << (cfg.routing_only ? "true" : "false") << "\n";
    json << "}\n";
}

void ExperimentLogger::log_route(simtime_picosec t,
                                 const std::string& direction,
                                 const RouteMetrics& m,
                                 int active_uplinks_src,
                                 int active_uplinks_dst,
                                 int inactive_sats_src,
                                 int inactive_sats_dst,
                                 double nearest_src_km,
                                 double nearest_dst_km,
                                 const ExperimentConfig& cfg,
                                 int num_sats) {
    _routes << std::fixed << std::setprecision(9) << timeAsSec(t) << ","
            << direction << ","
            << (m.found ? 1 : 0) << ","
            << std::setprecision(6) << m.rtt_ms << ","
            << m.one_way_latency_ms << ","
            << m.route_hash << ","
            << (m.changed ? 1 : 0) << ","
            << m.route_duration_s << ","
            << m.link_hops << ","
            << m.isl_hops << ","
            << m.ground_hops << ","
            << active_uplinks_src << ","
            << active_uplinks_dst << ","
            << inactive_sats_src << ","
            << inactive_sats_dst << ","
            << nearest_src_km << ","
            << nearest_dst_km << ","
            << m.dijkstra_cpu_ms << ","
            << cfg.num_planes << ","
            << cfg.sats_per_plane << ","
            << cfg.orbital_slots << ","
            << (cfg.adjacent_sats ? 1 : 0) << ","
            << num_sats << endl;
}

void ExperimentLogger::log_visibility(simtime_picosec t,
                                      const std::string& endpoint,
                                      int active_uplinks,
                                      int inactive_sats,
                                      double nearest_sat_km) {
    _visibility << std::fixed << std::setprecision(9) << timeAsSec(t) << ","
                << endpoint << ","
                << active_uplinks << ","
                << inactive_sats << ","
                << std::setprecision(6) << nearest_sat_km << endl;
}

void ExperimentLogger::log_summary(const std::string& key, const std::string& value) {
    _summary << key << "," << value << endl;
}
