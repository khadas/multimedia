cmake_minimum_required(VERSION 3.8)

include(GNUInstallDirs)

#############################################
# adpcm-aml
#############################################
aux_source_directory(. SRC_FILES)
message(WARNING "PROJECT_SOURCE_DIR is ${PROJECT_SOURCE_DIR}")
message(WARNING "SRC_FILES is ${SRC_FILES}")
message(WARNING "CMAKE_SYSTEM_PROCESSOR is ${CMAKE_SYSTEM_PROCESSOR}")

add_library(adpcm-aml SHARED
    ${SRC_FILES}
)

target_compile_options(adpcm-aml PRIVATE -Werror -Wmissing-field-initializers -g)

add_definitions(-DUSE_DEFAULT_STDLIB  -DUSE_ADPCM_DECODER)

#set_target_properties(adpcm-aml PROPERTIES
#    VERSION ${PROJECT_VERSION}
#    SOVERSION 1)

#target_link_libraries(adpcm-aml
#    libutils libz libbinder libdl libcutils libc liblog)

install(TARGETS adpcm-aml
    LIBRARY DESTINATION "lib")
