# Some Blurrr components are optional.
# If you don't need them and want to override their inclusion in the build process,
# use this file to override which ones you want.
# Any values you set here will override the default ones,
# and any values you omit, will continue to be set to the defaults.

# You must set these values before the initial CMake generation, otherwise you must clear the CMake cache.
# (Simply deleting your build folder and regenerating will suffice if you don't know how to clear the CMake cache.)

# Note: The default templates may be using this code as a starting point, so make sure to remove any code in use.


# Core libraries
# BLURRR_FORCE_OPTION(BLURRR_USE_OPENAL "Use OpenAL audio library" ON)
# BLURRR_FORCE_OPTION(BLURRR_USE_ALMIXER "Use ALmixer audio library (requires OpenAL)" ON)
# BLURRR_FORCE_OPTION(BLURRR_USE_CHIPMUNK "Use Chipmunk physics library" ON)
# BLURRR_FORCE_OPTION(BLURRR_USE_SDLTTF "Use SDL_ttf library" ON)
# BLURRR_FORCE_OPTION(BLURRR_USE_SDLIMAGE "Use SDL_image library" ON)
# BLURRR_FORCE_OPTION(BLURRR_USE_SDLGPU "Use SDL_gpu library" OFF)

# Lua Modules/PlugIns
IF(BLURRR_USE_LUA)
	# BLURRR_FORCE_OPTION(BLURRR_USE_LUAAL "Use LuaAL module" ON)
	# BLURRR_FORCE_OPTION(BLURRR_USE_LPEG "Use LPeg module" ON)
	# BLURRR_FORCE_OPTION(BLURRR_USE_LUASOCKET "Use LuaSocket module" ON)
ENDIF()


