#include "SceneMain.h"
#include "../engine/ResourceManager.h"

#include <cstdlib>
#include <ctime>
#include <algorithm>

SceneMain::SceneMain() {
}

SceneMain::~SceneMain() {
    clean();
}

void SceneMain::setGame(Game *_game) {
    game = _game;
}

void SceneMain::load() {
    std::srand((int) std::time(nullptr));
    ResourceManager::loadTexture("./assets/textures/wall.png", "wall");

    /*
    screenHeight = game->windowHeight;
    moveLeftKey = SDL_SCANCODE_LEFT;
    moveRightKey = SDL_SCANCODE_RIGHT;
    rotateKey = SDL_SCANCODE_UP;
    fallKey = SDL_SCANCODE_DOWN;

    ResourceManager::loadTexture("./assets/textures/tile.png", "tile");
    ResourceManager::loadTexture("./assets/textures/tile_fall.png", "tile_fall");
    pieces = new Pieces();
    board = new Board(pieces, game->windowHeight);
    board->initBoard();
    counter = 0;

    // First piece
    currentPiece.kind = getRand(0, 6);
    currentPiece.rotation = getRand(0, 3);
    currentPiece.x = BOARD_WIDTH / 2 + pieces->getXInitialPosition(currentPiece.kind, currentPiece.rotation);
    currentPiece.y = pieces->getYInitialPosition(currentPiece.kind, currentPiece.rotation);
    //  Next piece
    nextPiece.kind = getRand(0, 6);
    nextPiece.rotation = getRand(0, 3);
    nextPiece.x = BOARD_WIDTH / 2 + pieces->getXInitialPosition(nextPiece.kind, nextPiece.rotation);
    nextPiece.y = 20;
    */
}

void SceneMain::clean() {
    /*
    delete board;
    delete pieces;
    */
}

void SceneMain::pause() {
}

void SceneMain::resume() {
}

void SceneMain::handleEvent(const InputState &inputState) {
}

void SceneMain::update(unsigned int dt) {
    // Update
    isUpdatingActors = true;
    for(auto actor : actors) {
        actor->update(dt);
    }
    isUpdatingActors = false;

    // Move pending actors into actors
    for(auto pendingActor : pendingActors) {
        actors.emplace_back(pendingActor);
    }
    pendingActors.clear();

    removeExpiredActors();
}

void SceneMain::removeExpiredActors() {
    vector<Actor*> expiredActors;
    for(auto actor : actors) {
        if(actor->getState() == Actor::Expired) {
            expiredActors.emplace_back(actor);
        }
    }
    for (auto actor : expiredActors) {
        delete actor;
    }
}

void SceneMain::draw() {

}

int SceneMain::getRand(i32 a, i32 b) {
    return std::rand() % (b - a + 1) + a;
}

void SceneMain::addActor(Actor *actor) {
    if (isUpdatingActors) {
        pendingActors.emplace_back(actor);
    } else {
        actors.emplace_back(actor);
    }
}

void SceneMain::removeActor(Actor *actor) {
    // Is in pending actors ?
    auto iter = std::find(pendingActors.begin(), pendingActors.end(), actor);
    if (iter != pendingActors.end()) {
        std::iter_swap(iter, pendingActors.end() - 1);
        pendingActors.pop_back();
    }

    // Is in actors ?
    iter = std::find(actors.begin(), actors.end(), actor);
    if (iter != actors.end()) {
        std::iter_swap(iter, actors.end() - 1);
        actors.pop_back();
    }
}