project(game)
set(CMAKE_CXX_STANDARD 20)

## Game sources
file( GLOB GAME_SOURCES *.cpp )
add_library( game ${GAME_SOURCES} )
target_include_directories(game PUBLIC engine)
target_link_libraries(game engine)
