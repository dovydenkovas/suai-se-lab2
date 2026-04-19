CXX=clang++
CC=clang
CFLAGS=-c -Wall -O3
LDFLAGS=-lcgicc -lboost_json -lboost_system -lpqxx
SOURCES=src/main.cpp src/apihandler.cpp src/auth.cpp src/database.cpp src/enteties.cpp
OBJECTS=${SOURCES:.cpp=.o}
EXECUTABLE=api.cgi

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)

install:
	@if [ "$(id -u)" = "0" ]; then \
	    echo "Run as root"; \
		exit 1; \
	fi
	./scripts/deploy.sh

remove:
	@if [ "$(id -u)" = "0" ]; then \
	    echo "Run as root"; \
		exit 1; \
	fi
	./scripts/remove.sh

.PHONY: all clean install remove
