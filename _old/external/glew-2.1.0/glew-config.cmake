set(GLEW_INCLUDE_DIRS "include")

# Support both 32 and 64 bit builds
if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
  set(GLEW_LIBRARIES "lib/Release/x64/glew32.lib")
else ()
  set(GLEW_LIBRARIES "lib/Release/Win32/glew32.lib")
endif ()

string(STRIP "${GLEW_LIBRARIES}" GLEW_LIBRARIES)