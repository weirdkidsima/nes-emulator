# NES Emulator

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -Isrc
LDFLAGS =

# Определяем ОС
ifeq ($(OS),Windows_NT)
    OUT = nes-emulator.exe
    SDL_FLAGS = -lmingw32 -lSDL2main -lSDL2
    RM = del /Q
else
    OUT = nes-emulator
    SDL_FLAGS = -lSDL2
    RM = rm -f
endif

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(SDL_FLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ) $(OUT)

.PHONY: all clean