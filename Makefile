CXX=clang++
CC=clang
CFLAGS=-Wall -O2 -g -DBOOST_LOG_DYN_LINK=1
LDFLAGS=-lcgicc -lboost_json -lpqxx -lcrypt -lboost_log_setup -lboost_log -lboost_thread -lboost_system -lpthread
LIB_SOURCES=src/apihandler.cpp src/auth.cpp src/database.cpp src/enteties.cpp src/ecampus.cpp
TEST_SOURCES=tests/test_apihandler.cpp tests/test_auth.cpp
SOURCES=$(LIB_SOURCES) src/main.cpp
LIB_OBJECTS=$(LIB_SOURCES:.cpp=.o)
TEST_OBJECTS=$(TEST_SOURCES:.cpp=.o)
OBJECTS=${SOURCES:.cpp=.o}
EXECUTABLE=api.cgi

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
	rm -rf test

install:
	@if [ "$(id -u)" = "0" ]; then \
	    echo "Run as root"; \
		exit 1; \
	fi
	./scripts/deploy.sh

test: $(LIB_OBJECTS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(LIB_OBJECTS) $(TEST_SOURCES) -lboost_unit_test_framework -o test
	./test || echo "Done"

remove:
	@if [ "$(id -u)" = "0" ]; then \
	    echo "Run as root"; \
		exit 1; \
	fi
	./scripts/remove.sh

.PHONY: all clean install remove
