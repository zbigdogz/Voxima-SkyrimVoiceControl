# It's recommended to set a minimum CMake version.
# If you use CMake features from higher versions, update this to match.
cmake_minimum_required(VERSION 3.21)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}")

# Set your project name. This will be the name of your SKSE .dll file. Version Notation = major.minor.patch.tweak
project(Voxima-SkyrimVoiceControl VERSION 3.0.0.0 LANGUAGES CXX)

# Set name of project folder
set(FOLDER_NAME "Voxima - Skyrim Voice Control")

#Add flag to allow more "sections" in the code
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")

# Add dependency on websocketpp
find_package(websocketpp CONFIG REQUIRED)

# Note: boost-random only seems to be requied when using the C++ *client*
# You can remove this if you are only using C++ for a WebSocket *server*
find_package(Boost REQUIRED COMPONENTS random)

# Find Valve OpenVR
find_package(OpenVR REQUIRED)

#
# YOU DO NOT NEED TO EDIT ANYTHING BELOW HERE
# 
# Please see the "Project Setup" section of README.md
# 

# If you're not using a mod manager, you probably want the SKSE plugin to go
# inside of your Skyrim "Data" folder.
#
# To do this automatically, set the `SKYRIM_FOLDER` environment variable
# to the path of your Skyrim Special Edition folder
if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()

# If you're using Mod Organizer 2 or Vortex, you might want this to go inside
# of your "mods" folder, inside of a subfolder named "<your mod>".
#
# To do this automatically, set the `SKYRIM_MODS_FOLDER` environment variable
# to the path of your "mods" folder
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${FOLDER_NAME}")
endif()

# Otherwise, you can set OUTPUT_FOLDER to any place you'd like :)
# set(OUTPUT_FOLDER "")

# Setup your SKSE plugin as an SKSE plugin!
find_package(CommonLibSSE CONFIG REQUIRED)
#add_commonlibsse_plugin(${PROJECT_NAME} SOURCES source/plugin.cpp) # <--- specifies plugin.cpp
add_commonlibsse_plugin(
    # The plugin's name, defaults to target.
    ${PROJECT_NAME}

    # The plugin's author, empty by default.
    AUTHOR "zbigdogz & Exergist"

    # The support email address, empty by default.
    #EMAIL <string>

    # The plugin version number, defaults to ${PROJECT_VERSION}.
    VERSION 3.0.0.0

    # Indicates the plugin is compatible with all runtimes via address library. This is the default if no
    # other compatibilility mode is specified. Can be used with USE_SIGNATURE_SCANNING but not
    # COMPATIBLE_RUNTIMES.
    USE_ADDRESS_LIBRARY

    # Indicates the plugin is compatible with all runtimes via signature scanning. Can be used with
    # USE_ADDRESS_LIBRARY but not COMPATIBLE_RUNTIMES.
    #USE_SIGNATURE_SCANNING

    # List of up to 16 Skyrim versions the plugin is compatible with. Cannot be used with
    # USE_ADDRESS_LIBRARY or USE_SIGNATURE_SCANNING.
    #COMPATIBLE_RUNTIMES <version number> [<version number>...]

    # The minimum SKSE version to support; defaults to 0, and recommended by SKSE project to be left
    # 0.
    MINIMUM_SKSE_VERSION 0.0.0.0

    # Omit from all targets, same as used with add_library.
    #EXCLUDE_FROM_ALL

    # List of the sources to include in the target, as would be the parameters to add_library.
    SOURCES source/plugin.cpp
)

#set(COMMONLIBSSE_VERSION_MATCH TRUE PARENT_SCOPE)
#    set(COMMONLIBSSE_VERSION_MAJOR "$0" PARENT_SCOPE)
#	#set(COMMONLIBSSE_VERSION_MAJOR "${CMAKE_MATCH_1}" PARENT_SCOPE)
#    set(COMMONLIBSSE_VERSION_MINOR "0" PARENT_SCOPE)
#    set(COMMONLIBSSE_VERSION_PATCH "0" PARENT_SCOPE)
#    set(COMMONLIBSSE_VERSION_TWEAK "0" PARENT_SCOPE)

 # Handle minimum SKSE version constraints.
 #   if (NOT DEFINED ADD_COMMONLIBSSE_PLUGIN_MINIMUM_SKSE_VERSION)
 #       set(ADD_COMMONLIBSSE_PLUGIN_MINIMUM_SKSE_VERSION 0)
 #   endif ()
 #   commonlibsse_parse_version("${ADD_COMMONLIBSSE_PLUGIN_MINIMUM_SKSE_VERSION}")
 #   if (NOT COMMONLIBSSE_VERSION_MATCH)
 #       message(FATAL_ERROR "Unable to parse SKSE minimum SKSE version number "
 #               "${ADD_COMMONLIBSSE_PLUGIN_MINIMUM_SKSE_VERSION}.")
 #   endif ()
 #   set(commonlibsse_min_skse_version "REL::Version{ ${COMMONLIBSSE_VERSION_MAJOR}, ${COMMONLIBSSE_VERSION_MINOR}, ${COMMONLIBSSE_VERSION_PATCH}, ${COMMONLIBSSE_VERSION_TWEAK} }")

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23) # <--- use C++23 standard
target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h) # <--- PCH.h is required!
# Add websocketpp dependency to the SKSE project
target_link_libraries(${PROJECT_NAME} PRIVATE websocketpp::websocketpp)
# websocketpp client seems to also need boost's random library
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost Boost::random)
# Add OpenVR dependency to the SKSE project
target_link_libraries("${PROJECT_NAME}" PRIVATE PUBLIC OpenVR::OpenVR)

# When your SKSE .dll is compiled, this will automatically copy the .dll into your mods folder.
# Only works if you configure DEPLOY_ROOT above (or set the SKYRIM_MODS_FOLDER environment variable)
if(DEFINED OUTPUT_FOLDER)
    # If you specify an <OUTPUT_FOLDER> (including via environment variables)
    # then we'll copy your mod files into Skyrim or a mod manager for you!

    # Copy the SKSE plugin .dll files into the SKSE/Plugins/ folder
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        VERBATIM
    )

    # If you perform a "Debug" build, also copy .pdb file (for debug symbols)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()
