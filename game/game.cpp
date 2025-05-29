#include "game.hpp"

void Game::run() {
    Engine engine;

    engine.Init();
    engine.Run();
    engine.Clean();
}

