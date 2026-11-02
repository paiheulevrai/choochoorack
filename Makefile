RACK_DIR ?= .tmp/Rack-SDK-2.6.6

FLAGS += -I$(CURDIR)/src
SOURCES += $(wildcard src/*.cpp src/dsp/*.cpp)

DISTRIBUTABLES += res LICENSE

include $(RACK_DIR)/plugin.mk

CORE_TESTS := mme bogie sintered
# Needed only for the module integration test; point to your installed Rack dir.
RACK_RUNTIME_DIR ?= $(RACK_DIR)

.PHONY: test
test: $(addprefix build/tests/,$(addsuffix _core_test.exe,$(CORE_TESTS))) build/tests/polyphony_test.exe
	@set -e; for core in $(CORE_TESTS); do ./build/tests/$${core}_core_test.exe; done
	PATH="$(RACK_RUNTIME_DIR):$$PATH" ./build/tests/polyphony_test.exe

build/tests/%_core_test.exe: tests/%_core_test.cpp src/dsp/%_core.cpp src/dsp/%_core.h
	@mkdir -p $(@D)
	$(CXX) -std=c++11 -O2 -Wall -Wextra $< src/dsp/$*_core.cpp -o $@

build/tests/polyphony_test.exe: tests/polyphony_test.cpp $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) -std=c++11 -O2 -D_USE_MATH_DEFINES -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations -I$(RACK_DIR)/include -I$(RACK_DIR)/dep/include $^ -L$(RACK_DIR) -lRack -static-libstdc++ -o $@
