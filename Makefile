CC=g++
CFLAGS=-I./HCNetSDKCom/incEn -Wall
LDFLAGS=-L./HCNetSDKCom/lib -lhcnetsdk -lpthread -ldl -lstdc++ -Wl,-rpath,'$$ORIGIN/HCNetSDKCom/lib'

SRC=ptz_controlv3.cpp
BIN=ptz_controlv3

all: $(BIN)

$(BIN): $(SRC)
        $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
        rm -f $(BIN)
