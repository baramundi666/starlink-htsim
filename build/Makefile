CXX ?= g++
PROJECT_ROOT ?= ..
SRC_ROOT ?= $(PROJECT_ROOT)/src
STARLINK_DIR ?= $(SRC_ROOT)/starlink
HTSIM_LIB ?= $(SRC_ROOT)/libhtsim.a
PARSE_OUTPUT_BIN ?= $(SRC_ROOT)/parse_output

CXXFLAGS += -I$(SRC_ROOT) -I$(STARLINK_DIR) -Wall -std=c++11 -g -O2

STARLINK_OBJS = \
	isl.o \
	node.o \
	satellite.o \
	city.o \
	constellation.o \
	binary_heap.o \
	experiment_logger.o \
	main.o

.PHONY: all clean deep-clean htsim_lib parse_output_link

all: starlink_exp parse_output_link

htsim_lib:
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

parse_output_link: htsim_lib
	@if [ -x "$(PARSE_OUTPUT_BIN)" ]; then \
		ln -sf "$(PARSE_OUTPUT_BIN)" parse_output; \
	else \
		echo "Warning: $(PARSE_OUTPUT_BIN) not found; run ../src make if you need log parsing" >&2; \
	fi

clean:
	rm -f $(STARLINK_OBJS) starlink_exp parse_output starlink2
