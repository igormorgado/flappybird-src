# This file is designed to help isolate your specific project settings.
# The variable names are predetermined. DO NOT CHANGE the variable names.
# You may change the values of the variables.
# Please do not remove any comment automation markers or put anything in between. They are reserved for future use.


# You have a choice. 
# When you add resource files, you may manually add it here.
# Or you may disable manual mode and have the system automatically create the list.
# The downside to automatic mode is that you must remember to regenerate when you add a new file.
# The upside to manual mode is you can get fancy and case for things like Windows/Android/Mac/iOS/Linux.
# CMake officially recommends manual mode.

set(BLURRR_USE_MANUAL_FILE_LISTING_FOR_RESOURCE_FILES TRUE)
set(BLURRR_USE_MANUAL_FILE_LISTING_FOR_SCRIPT_FILES TRUE)

# Resource files
if(BLURRR_USE_MANUAL_FILE_LISTING_FOR_RESOURCE_FILES)

	# Add your user resource files here
	set(BLURRR_USER_RESOURCE_FILES
		${PROJECT_SOURCE_DIR}/resources/VeraMono.ttf
		${PROJECT_SOURCE_DIR}/resources/background.png
		${PROJECT_SOURCE_DIR}/resources/bush.png
		${PROJECT_SOURCE_DIR}/resources/clouds.png
		${PROJECT_SOURCE_DIR}/resources/fly0.png
		${PROJECT_SOURCE_DIR}/resources/fly1.png
		${PROJECT_SOURCE_DIR}/resources/fly2.png
		${PROJECT_SOURCE_DIR}/resources/ground.png
		${PROJECT_SOURCE_DIR}/resources/pipe_bottom.png
		${PROJECT_SOURCE_DIR}/resources/pipe_top.png
		${PROJECT_SOURCE_DIR}/resources/coin_ding.wav
		${PROJECT_SOURCE_DIR}/resources/davedes_fastsimplechop5b.wav
		${PROJECT_SOURCE_DIR}/resources/LeftHook_SoundBible_com-516660386.wav
		${PROJECT_SOURCE_DIR}/resources/SlideWhistle.wav
		${PROJECT_SOURCE_DIR}/resources/14609__man__swosh.wav

		${PROJECT_SOURCE_DIR}/resources/acknowtt.ttf

		${PROJECT_SOURCE_DIR}/resources/MedalBackground.png
		

		${PROJECT_SOURCE_DIR}/resources/momoko_Bronze_Medallion.png
		${PROJECT_SOURCE_DIR}/resources/momoko_Silver_Medallion.png
		${PROJECT_SOURCE_DIR}/resources/momoko_Gold_Medallion.png
		${PROJECT_SOURCE_DIR}/resources/momoko_Platinum_Medallion.png
		${PROJECT_SOURCE_DIR}/resources/Placeholder_Medallion.png
		
		${PROJECT_SOURCE_DIR}/resources/playbutton.png
		${PROJECT_SOURCE_DIR}/resources/quitbutton.png
		
		${PROJECT_SOURCE_DIR}/resources/LICENSE.txt
		${PROJECT_SOURCE_DIR}/resources/acknowledge.txt


		${PROJECT_SOURCE_DIR}/resources/04B_19__.TTF
#		${PROJECT_SOURCE_DIR}/resources/PressStart2P.ttf


		

		# AUTOMATION_BEGIN:BLURRR_USER_RESOURCE_FILES
		# AUTOMATION_END:BLURRR_USER_RESOURCE_FILES
	)
else()
	# We will automatically slurp everything.
	# But you must remember to regenerate every time you add a file.
	set(BLURRR_AUTOMATIC_USER_RESOURCE_FILES_SHOULD_FOLLOW_SYMLINKS TRUE)
	set(BLURRR_AUTOMATIC_USER_RESOURCE_FILES_GLOB_PATTERN "${PROJECT_SOURCE_DIR}/resources/*")
endif()

# Script files
if(BLURRR_USE_MANUAL_FILE_LISTING_FOR_SCRIPT_FILES)

	# This is an example of how to be clever (but still maintainable).
	# We just want the variable BLURRR_USER_SCRIPT_FILES defined.
	# If you have incompatible scripts like using new language features in Lua 5.3 vs. Lua 5.1 (LuaJIT), 
	# then you can conditionalize what gets put into BLURRR_USER_SCRIPT_FILES.

	set(BLURRR_USER_SCRIPT_FILES_LUA53
		# ${PROJECT_SOURCE_DIR}/scripts/native_bitops.lua
		
		# AUTOMATION_BEGIN:BLURRR_USER_SCRIPT_FILES_LUA53
		# AUTOMATION_END:BLURRR_USER_SCRIPT_FILES_LUA53
	)
	set(BLURRR_USER_SCRIPT_FILES_LUAJIT
		# ${PROJECT_SOURCE_DIR}/scripts/jit_control.lua

		# AUTOMATION_BEGIN:BLURRR_USER_SCRIPT_FILES_LUAJIT
		# AUTOMATION_END:BLURRR_USER_SCRIPT_FILES_LUAJIT
	)
	set(BLURRR_USER_SCRIPT_FILES_LUACOMMON
		# ${PROJECT_SOURCE_DIR}/scripts/render_stuff.lua

		# AUTOMATION_BEGIN:BLURRR_USER_SCRIPT_FILES_LUACOMMON
		# AUTOMATION_END:BLURRR_USER_SCRIPT_FILES_LUACOMMON
	)
	set(BLURRR_USER_SCRIPT_FILES_JAVASCRIPT
		# ${PROJECT_SOURCE_DIR}/scripts/render_stuff.js

		# AUTOMATION_BEGIN:BLURRR_USER_SCRIPT_FILES_JAVASCRIPT
		# AUTOMATION_END:BLURRR_USER_SCRIPT_FILES_JAVASCRIPT
	)

	# Initialize variable 
	set(BLURRR_USER_SCRIPT_FILES "")

	if(BLURRR_USE_LUA)
		list(APPEND BLURRR_USER_SCRIPT_FILES ${BLURRR_USER_SCRIPT_FILES_LUACOMMON})
		if(BLURRR_USE_LUA53)
			list(APPEND BLURRR_USER_SCRIPT_FILES ${BLURRR_USER_SCRIPT_FILES_LUA53})
		elseif(BLURRR_USE_LUAJIT)
			list(APPEND BLURRR_USER_SCRIPT_FILES ${BLURRR_USER_SCRIPT_FILES_LUAJIT})
		endif()
	elseif(BLURRR_USE_JAVASCRIPT)
		list(APPEND BLURRR_USER_SCRIPT_FILES ${BLURRR_USER_SCRIPT_FILES_JAVASCRIPT})
	endif()


else()
	# We will automatically slurp everything.
	# But you must remember to regenerate every time you add a file.
	set(BLURRR_AUTOMATIC_USER_SCRIPT_FILES_SHOULD_FOLLOW_SYMLINKS TRUE)
	set(BLURRR_AUTOMATIC_USER_SCRIPT_FILES_GLOB_PATTERN "${PROJECT_SOURCE_DIR}/scripts/*")
endif()





add_definitions(-DCIRCULAR_QUEUE_NAMESPACE_PREFIX=Flappy)

# Add your C/C++/Obj-C files here
set(BLURRR_USER_COMPILED_FILES
	# This is main_c.c or main_lua.c or main_JavaScriptCore.c
	${BLURRR_CORE_SOURCE_MAIN}
	${PROJECT_SOURCE_DIR}/source/CircularQueue.c	
	${PROJECT_SOURCE_DIR}/source/CircularQueue.h	
	${PROJECT_SOURCE_DIR}/source/TimeTicker.h	
	${PROJECT_SOURCE_DIR}/source/TimeTicker.c

#	${PROJECT_SOURCE_DIR}/source/main_c.c	
	# AUTOMATION_BEGIN:BLURRR_USER_COMPILED_FILES
	# AUTOMATION_END:BLURRR_USER_COMPILED_FILES
)


# Add your C/C++/Obj-C include paths here
set(BLURRR_USER_INCLUDE_PATHS
	${PROJECT_SOURCE_DIR}/source
	# AUTOMATION_BEGIN:BLURRR_USER_INCLUDE_PATHS
	# AUTOMATION_END:BLURRR_USER_INCLUDE_PATHS
)


# Add your find_package() stuff here as needed, 
# then set BLURRR_USER_LIBRARIES and BLURRR_USER_THIRD_PARTY_LIBRARIES_TO_EMBED
set(BLURRR_USER_LIBRARIES

	# AUTOMATION_BEGIN:BLURRR_USER_LIBRARIES
	# AUTOMATION_END:BLURRR_USER_LIBRARIES
)

# Remember that system libraries and static libraries don't need to be embedded.
set(BLURRR_USER_THIRD_PARTY_LIBRARIES_TO_EMBED

	# AUTOMATION_BEGIN:BLURRR_USER_THIRD_PARTY_LIBRARIES_TO_EMBED
	# AUTOMATION_END:BLURRR_USER_THIRD_PARTY_LIBRARIES_TO_EMBED
)


