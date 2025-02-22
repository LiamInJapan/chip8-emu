CC = clang
CFLAGS = -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lSDL2

chip8: chip8.c
	$(CC) $(CFLAGS) -o chip8 chip8.c $(LDFLAGS)