## Game sources
file( GLOB game_SOURCES *.cpp )
add_library( game ${game_SOURCES} )
target_link_libraries(game engine)
target_include_directories(game PUBLIC engine)

## Executable
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} game engine)