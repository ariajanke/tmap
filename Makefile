CXX = g++
LD = g++
CXXFLAGS = -std=c++17 -O3 -I./inc -Ilib/cul/inc -Wall -pedantic -Werror -DMACRO_PLATFORM_LINUX
SOURCES  = $(shell find src | grep '[.]cpp$$')
OBJECTS_DIR = .release-build
OBJECTS = $(addprefix $(OBJECTS_DIR)/,$(SOURCES:%.cpp=%.o))
OUTPUT = libtmap.a
$(OBJECTS_DIR)/%.o: | $(OBJECTS_DIR)/src
	$(CXX) $(CXXFLAGS) -c $*.cpp -o $@

.PHONY: default
default: $(OBJECTS)
	@echo $(SOURCES)
	@echo $(OBJECTS)
	ar rvs $(OUTPUT) $(OBJECTS)

$(OBJECTS_DIR)/src:
	mkdir -p $(OBJECTS_DIR)/src
.PHONY: clean
clean:
	rm -rf $(OBJECTS_DIR)

#DEMO_OPTIONS = -g -L/usr/lib/ -L./demos -lsfml-system -lsfml-graphics -lsfml-window -lcommon-d -lksg-d
#.PHONY: demos
#demos:
#	$(CXX) $(CXXFLAGS) demos/demo.cpp $(DEMO_OPTIONS) -o demos/.demo
#	$(CXX) $(CXXFLAGS) demos/spacer_tests.cpp $(DEMO_OPTIONS) -o demos/.spacer_tests
DEMO_OPTIONS = -L/usr/lib/ -L./. -lsfml-system -lsfml-graphics -lsfml-window -ltmap -ltinyxml2 -lcommon -lz

demo: $(OUTPUT)
	$(CXX) $(CXXFLAGS) demo/map-demo.cpp $(DEMO_OPTIONS) -o demo/.demo
