: main
    clear                # Clear screen
    v0 := 32             # Initial X position
    v1 := 16             # Initial Y position

: game-loop
    v2 := key            # Check keys
    if v2 == 5 then v1 += 1  # W moves down
    if v2 == 8 then v1 += -1 # S moves up
    if v2 == 7 then v0 += -1 # A moves left
    if v2 == 9 then v0 += 1  # D moves right

    clear
    i := square
    sprite v0 v1 5
    jump game-loop

: square
    0xF0 0x90 0x90 0x90 0xF0