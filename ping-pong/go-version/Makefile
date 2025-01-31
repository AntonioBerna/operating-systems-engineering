SRCS=./src/main.go

BINARY_DIR=./bin/

all:
	@mkdir -p $(BINARY_DIR)
	go build -ldflags="-s -w" -o $(BINARY_DIR)ping-pong $(SRCS)

race_detector:
	@mkdir -p $(BINARY_DIR)
	go build -race -ldflags="-s -w" -o $(BINARY_DIR)ping-pong $(SRCS)

clean:
	$(RM) -r $(BINARY_DIR)

.PHONY: race_detector clean