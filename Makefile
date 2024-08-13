RM = rm -f

SRC_DIR=src
BIN_DIR=bin

CXXFLAGS := -Wall -I $(SRC_DIR)

srcfiles := $(shell find ./$(SRC_DIR) -name "*.cpp") $(CAPN_SRCS)
objects  := $(patsubst ./$(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(srcfiles))

run_driver_objects := $(filter-out $(BIN_DIR)/sim.o,$(objects))
sim_objects := $(filter-out $(BIN_DIR)/run_driver.o,$(objects))

dir_guard=@mkdir -p $(@D)

all: driver sim

driver: $(BIN_DIR)/run_driver
$(BIN_DIR)/run_driver: $(run_driver_objects)
	$(dir_guard)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(BIN_DIR)/run_driver $(run_driver_objects) $(LDLIBS)

sim: $(BIN_DIR)/sim
$(BIN_DIR)/sim: $(sim_objects)
	$(dir_guard)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(BIN_DIR)/sim $(sim_objects) $(LDLIBS)

depend: .depend

.depend: $(srcfiles)
	$(RM) ./$@
	$(CXX) $(CXXFLAGS) -MM $^>>./$@;
	sed -i 's/^.\+\.o\:/$(BIN_DIR)\/\0/' ./$@

clean:
	$(RM) $(objects) $(BIN_DIR)/run_driver $(BIN_DIR)/sim

distclean: clean
	$(RM) *~ .depend

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(dir_guard)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

test:
	kitty ./bin/sim
	kitty ./bin/run_driver

include .depend
