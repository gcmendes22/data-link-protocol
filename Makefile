CC = gcc
CFLAGS = -Wall

APPLICATION_DIR = application/
PROTOCOL_DIR = protocol/
CABLE_DIR = cable/

all: application_main cable_app

application_main: $(APPLICATION_DIR)/main.c $(PROTOCOL_DIR)/*.c
	$(CC) $(CFLAGS) -o $@ $^ -I$(PROTOCOL_DIR)

cable_app: $(CABLE_DIR)/cable.c
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm application_main
	rm cable_app