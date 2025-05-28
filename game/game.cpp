#include "game.hpp"

void Game::run() {
    Engine engine;

    engine.init();
    engine.run();
    engine.cleanup();
}

