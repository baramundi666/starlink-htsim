# Build this from starlink-htsim/build.
# Repository layout assumed by this Makefile:
#   starlink-htsim/
#     build/        <- this Makefile, starlink_exp, benchmark scripts, results
#     src/          <- libhtsim.a, parse_output, common simulator sources
#       starlink/   <- Starlink experiment sources

CXX ?= g++
PROJECT_ROOT ?= ..
SRC_ROOT ?= $(PROJECT_ROOT)/src
STARLINK_DIR ?= $(SRC_ROOT)/starlink
HTSIM_LIB ?= $(SRC_ROOT)/libhtsim.a
PARSE_OUTPUT_BIN ?= $(SRC_ROOT)/parse_output

CXXFLAGS ?= -I$(SRC_ROOT) -I$(STARLINK_DIR) -Wall -std=c++11 -g -O2

# Important: do NOT add -DXCP_STATIC_NETWORK here. The dynamic constellation
# constructor is required for variable satellite-count experiments.
STARLINK_OBJS = \
	isl.o \
	node.o \
	satellite.o \
	city.o \
	constellation.o \
	binary_heap.o \
	experiment_logger.o \
	main.o

.PHONY: all clean deep-clean check-layout htsim_lib parse_output_link

all: check-layout starlink_exp parse_output_link

check-layout:
	@test -d "$(SRC_ROOT)" || (echo "Missing $(SRC_ROOT). Run make from starlink-htsim/build" >&2; exit 1)
	@test -d "$(STARLINK_DIR)" || (echo "Missing $(STARLINK_DIR). Run make from starlink-htsim/build" >&2; exit 1)

# Build the simulator support library from src only if it is missing.
# Normally build.sh runs 'make' in ../src first, so this target is just a fallback.
htsim_lib: check-layout
	@if [ ! -f "$(HTSIM_LIB)" ]; then \
		echo "$(HTSIM_LIB) not found; building simulator library in $(SRC_ROOT)"; \
		$(MAKE) -C $(SRC_ROOT); \
	fi

starlink_exp: htsim_lib $(STARLINK_OBJS) $(HTSIM_LIB)
	$(CXX) $(STARLINK_OBJS) $(HTSIM_LIB) -o $@

isl.o: $(STARLINK_DIR)/isl.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

node.o: $(STARLINK_DIR)/node.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

satellite.o: $(STARLINK_DIR)/satellite.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

city.o: $(STARLINK_DIR)/city.cpp $(STARLINK_DIR)/city.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

constellation.o: $(STARLINK_DIR)/constellation.cpp $(STARLINK_DIR)/constellation.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

binary_heap.o: $(STARLINK_DIR)/binary_heap.cpp $(STARLINK_DIR)/binary_heap.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

experiment_logger.o: $(STARLINK_DIR)/experiment_logger.cpp $(STARLINK_DIR)/experiment_logger.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

main.o: $(STARLINK_DIR)/main.cpp $(STARLINK_DIR)/experiment_logger.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Convenience symlink only. The real parser is built by ../src/Makefile.
parse_output_link: htsim_lib
	@if [ -x "$(PARSE_OUTPUT_BIN)" ]; then \
		ln -sf "$(PARSE_OUTPUT_BIN)" parse_output; \
	else \
		echo "Warning: $(PARSE_OUTPUT_BIN) not found; run ../src make if you need log parsing" >&2; \
	fi

clean:
	rm -f $(STARLINK_OBJS) starlink_exp parse_output starlink2

# Use this after changing headers in src/starlink, especially constellation.h.
# It removes old objects in both src and build, preventing stale ABI/linker issues.
deep-clean: clean
	$(MAKE) -C $(SRC_ROOT) clean || true
	find $(SRC_ROOT) -name '*.o' -delete
	rm -f $(SRC_ROOT)/libhtsim.a $(SRC_ROOT)/parse_output
