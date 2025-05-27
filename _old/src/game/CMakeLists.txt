file( GLOB game_SOURCES *.cpp )
add_library( game ${game_SOURCES} )
target_include_directories(game PUBLIC .)
