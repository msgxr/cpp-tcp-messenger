CXX := g++
CXXFLAGS := -Wall -Wextra -Wpedantic -std=c++17 -pthread
SRC_DIR := ../01_Kaynak_Kod_Dosyalari
BIN_DIR := Derlenmis_Uygulama_Dosyalari
COMMON_SRC := $(SRC_DIR)/protocol.cpp
COMMON_HDR := $(SRC_DIR)/protocol.h $(SRC_DIR)/chat_common.h $(SRC_DIR)/ui.h
SERVER := $(BIN_DIR)/server
CLIENT := $(BIN_DIR)/client

.PHONY: all clean run-server run-client

all: $(SERVER) $(CLIENT)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(SERVER): $(SRC_DIR)/server.cpp $(COMMON_SRC) $(COMMON_HDR) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $(SRC_DIR)/server.cpp $(COMMON_SRC) -o $(SERVER)

$(CLIENT): $(SRC_DIR)/client.cpp $(COMMON_SRC) $(COMMON_HDR) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $(SRC_DIR)/client.cpp $(COMMON_SRC) -o $(CLIENT)

run-server: $(SERVER)
	./$(SERVER)

run-client: $(CLIENT)
	./$(CLIENT)

clean:
	rm -rf $(BIN_DIR)
