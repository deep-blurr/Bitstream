CXX := g++
CXXFLAGS := -std=c++20 -Wall -Iinclude
LDFLAGS := -lssl -lcrypto

SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

PARSER_SRC := $(SRC_DIR)/TorrentParser.cpp
PARSER_OBJ := $(BUILD_DIR)/TorrentParser.o
PARSER_LIB := $(BUILD_DIR)/libtorrentparser.a

TEST_SRC := $(TEST_DIR)/test_parser.cpp
TEST_BIN := $(BUILD_DIR)/test_parser

all: $(TEST_BIN)

$(PARSER_LIB): $(PARSER_OBJ)
	ar rcs $@ $^

$(PARSER_OBJ): $(PARSER_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to build the test executable
$(TEST_BIN): $(TEST_SRC) $(PARSER_LIB)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Run the test
run: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean run
