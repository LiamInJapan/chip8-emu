# Collect the Dots - CHIP-8 Game (Octo Format)

: main
    cls              
    V0 := 5          
    V1 := 5          
    rnd V2, 32       
    rnd V3, 32       
    V4 := 0          
    jump gameLoop    

: gameLoop
    skp V0          
    jump gameLoop   

: moveUp
    add V1, 1       
    jump gameLoop

: moveDown
    sub V1, 1       
    jump gameLoop

: moveLeft
    sub V0, 1       
    jump gameLoop

: moveRight
    add V0, 1       
    jump gameLoop

: checkCollision
    se V0, V2       
    sne V1, V3      
    add V4, 1       
    rnd V2, 32      
    rnd V3, 32      
    jump gameLoop