# This file is designed to help isolate your specific project settings.
# The variable names are predetermined. DO NOT CHANGE the variable names.
# You may change the values of the variables.
# Please do not remove any comment automation markers or put anything in between. They are reserved for future use.

# Set the name of your project here. Avoid spaces and funny characters. (Change the part in quotes, not the variable name BLURRR_USER_PROJECT_NAME)
set(BLURRR_USER_PROJECT_NAME "FlappyBlurrrC")

# Set the name of your executable here. Avoid spaces and funny characters. (Change the part in quotes, not the variable name BLURRR_USER_PROJECT_NAME)
set(BLURRR_USER_EXECUTABLE_NAME "FlappyBlurrrC")

# Set the human readable name of program your  here. (Change the part in quotes, not the variable name BLURRR_USER_HUMAN_NAME)
set(BLURRR_USER_HUMAN_NAME "Flappy Blurrr (C)")

# Your company identifier
set(BLURRR_USER_COMPANY_IDENTIFIER "net.playcontrol")
# The name of your unique bundle identifer.
set(BLURRR_USER_BUNDLE_IDENTIFIER "${BLURRR_USER_COMPANY_IDENTIFIER}.${BLURRR_USER_EXECUTABLE_NAME}")

# You must create a unique GUID for your application for WinRT. Other platforms can ignore this.
set(BLURRR_USER_PACKAGE_GUID "01234567-89ab-cdef-0123-456789abcdef")

set(BLURRR_VERSION_MAJOR "1")
set(BLURRR_VERSION_MINOR "0")
set(BLURRR_VERSION_PATCH "0")

set(BLURRR_USER_APP_DESCRIPTION "Game made with Blurrr SDK")


# Code Signing: Maybe changing in the IDE directly is sufficent. If not, change it here.
macro(BLURRR_CONFIGURE_CODESIGNING _EXE_NAME)
	if(IOS)
		set_target_properties(${_EXE_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
	endif(IOS)
endmacro(BLURRR_CONFIGURE_CODESIGNING)


macro(BLURRR_CONFIGURE_APPLICATION_MANIFEST _EXE_NAME)
	if(APPLE)

		set(MACOSX_BUNDLE_GUI_IDENTIFIER "${BLURRR_USER_BUNDLE_IDENTIFIER}")
		set(MACOSX_BUNDLE_DISPLAY_NAME "${_EXE_NAME}")
		set(MACOSX_BUNDLE_EXECUTABLE_NAME "${_EXE_NAME}")
		set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${BLURRR_VERSION_MAJOR}.${BLURRR_VERSION_MINOR}")
		set(MACOSX_BUNDLE_BUNDLE_VERSION "${BLURRR_VERSION_MAJOR}.${BLURRR_VERSION_MINOR}")
		
	endif()
endmacro(BLURRR_CONFIGURE_APPLICATION_MANIFEST)


# IPA note:
# /usr/bin/xcrun -sdk iphoneos PackageApplication -v $Absolute_path_to.app -o $Absolute_path_to.ipa


