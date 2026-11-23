all:
    g++ game.cpp -o game -I include/SDL2 -I include -L lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf
    .\game