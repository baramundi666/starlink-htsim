#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_PARSE_OUTPUT="${SCRIPT_DIR}/../src/parse_output"

PREFIX="${1:-results/run}"
PARSE_OUTPUT="${2:-${PARSE_OUTPUT:-${DEFAULT_PARSE_OUTPUT}}}"
BINLOG="${PREFIX}.binlog"

if [[ ! -f "${BINLOG}" ]]; then
    echo "Missing binary log: ${BINLOG}" >&2
    exit 1
fi

if [[ ! -x "${PARSE_OUTPUT}" ]]; then
    echo "Missing executable parse_output: ${PARSE_OUTPUT}" >&2
    echo "Run ./build.sh first, or pass the parser path as the second argument." >&2
    exit 1
fi

"${PARSE_OUTPUT}" "${BINLOG}" -ascii > "${PREFIX}.ascii.txt"
"${PARSE_OUTPUT}" "${BINLOG}" -csv > "${PREFIX}.events.csv"
"${PARSE_OUTPUT}" "${BINLOG}" -queue-csv > "${PREFIX}.queue.csv"

grep 'XCP_SINK' "${PREFIX}.ascii.txt" > "${PREFIX}.xcp_sink.txt" || true
grep 'TCP_SINK' "${PREFIX}.ascii.txt" > "${PREFIX}.tcp_sink.txt" || true
grep 'QUEUE' "${PREFIX}.ascii.txt" > "${PREFIX}.queue_ascii.txt" || true

"${PARSE_OUTPUT}" "${BINLOG}" -tcp > "${PREFIX}.tcp_sink_summary.txt" || true
"${PARSE_OUTPUT}" "${BINLOG}" -queue > "${PREFIX}.queue_summary.txt" || true

echo "Parsed ${BINLOG}"
echo "  ${PREFIX}.routes.csv and ${PREFIX}.visibility.csv are produced by starlink_exp"
echo "  ${PREFIX}.events.csv and ${PREFIX}.queue.csv are produced by parse_output"
