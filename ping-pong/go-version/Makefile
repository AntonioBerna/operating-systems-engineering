SRCS=./src/main.go
TARGET=multithread-ping-pong

BINARY_DIR=./bin/

all:
	@mkdir -p $(BINARY_DIR)
	go build -ldflags="-s -w" -o $(BINARY_DIR)$(TARGET) $(SRCS)

race_detector:
	@mkdir -p $(BINARY_DIR)
	go build -race -ldflags="-s -w" -o $(BINARY_DIR)$(TARGET) $(SRCS)

clean:
	$(RM) -r $(BINARY_DIR)

.PHONY: race_detector clean