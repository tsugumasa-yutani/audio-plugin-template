cmake_minimum_required(VERSION 3.22)

# Version is needed by JUCE.
project(AudioPlugin VERSION 0.1.0)

# Adding a directory with the library/application name as a subfolder of the
# include folder is a good practice. It helps avoid name clashes later on.
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/YourPluginName")

# Adds a plugin target (that's basically what the Projucer does).
juce_add_plugin(
  ${PROJECT_NAME}
  COMPANY_NAME
  "TsugumasaYutani" # change this
  IS_SYNTH
  FALSE # may change this
  NEEDS_MIDI_INPUT
  FALSE # may change this
  NEEDS_MIDI_OUTPUT
  FALSE # may change this
  PLUGIN_MANUFACTURER_CODE
  Mcmp # change this (for max compatibility, first letter must be uppercase, remaining lowercase)
  PLUGIN_CODE
  Expl # change this (for max compatibility, first letter must be uppercase, remaining lowercase)
  FORMATS
  Standalone
  VST3 # may change this
  PRODUCT_NAME
  "YourPluginName" # change this
)

# Sets the source files of the plugin project.
set(SOURCE_FILES source/PluginEditor.cpp source/PluginProcessor.cpp)

# Optional; includes header files in the project file tree in Visual Studio
set(HEADER_FILES ${INCLUDE_DIR}/PluginEditor.h ${INCLUDE_DIR}/PluginProcessor.h)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES} ${HEADER_FILES})

# Sets the include directories of the plugin project.
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Links to all necessary dependencies. The present ones are recommended by JUCE.
# If you use one of the additional modules, like the DSP module, you need to specify it here.
target_link_libraries_system(${PROJECT_NAME} PUBLIC juce::juce_audio_utils)
target_link_libraries(
  ${PROJECT_NAME} PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
  juce::juce_recommended_warning_flags
)

# These definitions are recommended by JUCE.
target_compile_definitions(${PROJECT_NAME} PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

# Enables strict C++ warnings and treats warnings as errors.
# This needs to be set up only for your projects, not 3rd party
set_source_files_properties(${SOURCE_FILES} PROPERTIES COMPILE_OPTIONS "${PROJECT_WARNINGS_CXX}")

# In Visual Studio this command provides a nice grouping of source files in "filters".
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# =============================
# VST3 bundle auto-copy feature (for macOS)
if(APPLE)
  set(VST3_BUNDLE_NAME "YourPluginName.vst3")
  set(VST3_PATH "${CMAKE_CURRENT_BINARY_DIR}/AudioPlugin_artefacts/VST3/${VST3_BUNDLE_NAME}")
  set(VST3_USER_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")

  add_custom_target(copy_vst3 ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${VST3_USER_DIR}"
    COMMAND /bin/sh -c "if [ -d '${VST3_PATH}' ]; then \
            if [ -d '${VST3_USER_DIR}/${VST3_BUNDLE_NAME}' ]; then \
                rm -rf '${VST3_USER_DIR}/${VST3_BUNDLE_NAME}'; \
            fi; \
            ${CMAKE_COMMAND} -E copy_directory '${VST3_PATH}' '${VST3_USER_DIR}/${VST3_BUNDLE_NAME}'; \
            codesign --force --deep --sign - '${VST3_USER_DIR}/${VST3_BUNDLE_NAME}'; \
        else \
            echo 'VST3 bundle not found.'; \
        fi"
    DEPENDS ${PROJECT_NAME} "${VST3_PATH}" ${SOURCE_FILES} ${HEADER_FILES}
    COMMENT "Automatically copy and codesign VST3 bundle to user plugin folder after build"
    VERBATIM
  )
endif()