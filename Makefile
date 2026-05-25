CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -pedantic
COVERAGE_FLAGS := -ftest-coverage -fprofile-arcs
LDFLAGS :=
LDLIBS := -lCatch2Main -lCatch2

TARGET := testa_monitora_logs
SOURCES := monitora_logs.cpp testa_monitora_logs.cpp
HEADERS := monitora_logs.hpp

.PHONY: all test coverage lint static valgrind gdb docs clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $(TARGET)

test: $(TARGET)
	./$(TARGET)

coverage:
	$(CXX) $(CXXFLAGS) $(COVERAGE_FLAGS) $(SOURCES) $(LDFLAGS) $(LDLIBS) -o $(TARGET)
	./$(TARGET)
	gcov $(TARGET)-monitora_logs.gcno

lint:
	cpplint --filter=-legal/copyright,-build/include_subdir,-build/c++11 $(SOURCES) $(HEADERS)

static:
	cppcheck --enable=warning --std=c++17 --language=c++ $(SOURCES) $(HEADERS)

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$(TARGET)

gdb: $(TARGET)
	gdb ./$(TARGET)

docs:
	doxygen Doxyfile

clean:
	rm -f $(TARGET) *.o *.gcda *.gcno *.gcov
	rm -rf html latex
