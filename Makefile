RACK_DIR ?= .tmp/Rack-SDK-2.6.6

FLAGS += -I$(CURDIR)/src
SOURCES += $(wildcard src/*.cpp src/dsp/*.cpp)

DISTRIBUTABLES += res LICENSE

include $(RACK_DIR)/plugin.mk

CORE_TESTS := mme bogie sintered

.PHONY: test
test: $(addprefix build/tests/,$(addsuffix _core_test.exe,$(CORE_TESTS)))
	@set -e; for core in $(CORE_TESTS); do ./build/tests/$${core}_core_test.exe; done

build/tests/%_core_test.exe: tests/%_core_test.cpp src/dsp/%_core.cpp src/dsp/%_core.h
	@mkdir -p $(@D)
	$(CXX) -std=c++11 -O2 -Wall -Wextra $< src/dsp/$*_core.cpp -o $@
