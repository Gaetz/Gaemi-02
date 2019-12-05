#include "gamestate_main.h"
#include "../engine/resource_manager.h"

#include <cstdlib>
#include <ctime>


GameStateMain::GameStateMain()
{
}

GameStateMain::~GameStateMain()
{
	clean();
}

void GameStateMain::setGame(Game *_game)
{
	game = _game;
}

void GameStateMain::load()
{
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

void GameStateMain::clean()
{
	/*
	delete board;
	delete pieces;
	*/
}

void GameStateMain::pause()
{
}

void GameStateMain::resume()
{
}

void GameStateMain::handleEvent(const InputState &inputState)
{
}

void GameStateMain::update(unsigned int dt)
{

}

void GameStateMain::draw() {

}

int GameStateMain::getRand(int a, int b)
{
	return std::rand() % (b - a + 1) + a;
}
