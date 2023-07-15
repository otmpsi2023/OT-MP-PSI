CXX  :=  g++
CXX_FLAGS := -std=c++20 -Wall

BIN := bin
SRC := src
THIRD_PARTY = third_party
INCLUDE := include
LIB := lib
BENCHMARK = tools/benchmark
GENPRIME = tools/gen_prime
LIBRARIES := -lntl -lgmp -lm -lpthread
EXECUTABLE1 := main
EXECUTABLE2 := benchmark
EXECUTABLE3 := gen_prime

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    INCLUDE +=  /opt/homebrew/Cellar/ntl/11.5.1/include /opt/homebrew/Cellar/gmp/6.2.1_1/include /opt/homebrew/Cellar/boost/1.81.0_1/include
    LIB +=  /opt/homebrew/Cellar/ntl/11.5.1/lib /opt/homebrew/Cellar/gmp/6.2.1_1/lib /opt/homebrew/Cellar/boost/1.81.0_1/lib
endif

UNAME := $(shell uname)
ifeq ($(UNAME),Darwin)
    LIBRARIES += -lboost_thread-mt
else
	LIBRARIES +=  -lboost_thread
endif

all: $(BIN)/$(EXECUTABLE1) $(BIN)/$(EXECUTABLE2) $(BIN)/$(EXECUTABLE3)

run: clean all
	@echo "Executing..."
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE1): $(SRC)/*.cpp $(SRC)/*/*.cpp $(THIRD_PARTY)/*/*.cpp
	@echo "Building..."
	$(CXX) $(CXX_FLAGS) $(addprefix -I,$(INCLUDE)) $(addprefix -L,$(LIB)) $^ -o $@ $(LIBRARIES)

$(BIN)/$(EXECUTABLE2): $(BENCHMARK)/*.cpp $(SRC)/*/*.cpp $(THIRD_PARTY)/*/*.cpp
	@echo "Building..."
	$(CXX) $(CXX_FLAGS) $(addprefix -I,$(INCLUDE)) $(addprefix -L,$(LIB)) $^ -o $@ $(LIBRARIES)

$(BIN)/$(EXECUTABLE3): $(GENPRIME)/*.cpp
	@echo "Building..."
	$(CXX) $(CXX_FLAGS) $(addprefix -I,$(INCLUDE)) $(addprefix -L,$(LIB)) $^ -o $@ $(LIBRARIES)

clean:
	@echo "Clearing..."
	-rm -f $(BIN)/*
	-rm -f config/*



