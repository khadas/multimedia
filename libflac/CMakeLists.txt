cmake_minimum_required(VERSION 3.8)

include(GNUInstallDirs)

#############################################
# flac-aml
#############################################
aux_source_directory(. SRC_FILES)
message(WARNING "PROJECT_SOURCE_DIR is ${PROJECT_SOURCE_DIR}")
message(WARNING "SRC_FILES is ${SRC_FILES}")
message(WARNING "CMAKE_SYSTEM_PROCESSOR is ${CMAKE_SYSTEM_PROCESSOR}")

add_library(flac-aml SHARED
    ${SRC_FILES}
)

target_compile_options(flac-aml PRIVATE -Werror -Wmissing-field-initializers)

add_definitions(-DUSE_DEFAULT_STDLIB  -DUSE_FLAC_DECODER)

#set_target_properties(flac-aml PROPERTIES
#    VERSION ${PROJECT_VERSION}
#    SOVERSION 1)

target_include_directories(flac-aml PRIVATE
    ${PROJECT_SOURCE_DIR}/audio_codec/libflac
    ${PROJECT_SOURCE_DIR}/amadec)

#target_link_libraries(flac-aml
#    libutils libz libbinder libdl libcutils libc liblog)

install(TARGETS flac-aml
    LIBRARY DESTINATION "lib")
