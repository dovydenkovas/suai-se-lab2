CXX=clang++
CC=clang
CFLAGS=-Wall -O3 -g -DBOOST_LOG_DYN_LINK=1
LDFLAGS=-lcgicc -lboost_json -lpqxx -lcrypt -lboost_log_setup -lboost_log -lboost_thread -lboost_system -lpthread
SOURCES=src/apihandler.cpp src/auth.cpp src/database.cpp src/enteties.cpp src/main.cpp
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

test:
	$(CXX) $(CFLAGS) src/apihandler.cpp tests/tests.cpp -lboost_unit_test_framework $(LDFLAGS) -o test
	./test

remove:
	@if [ "$(id -u)" = "0" ]; then \
	    echo "Run as root"; \
		exit 1; \
	fi
	./scripts/remove.sh

.PHONY: all clean install remove
