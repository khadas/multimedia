cmake_minimum_required(VERSION 3.8)

include(GNUInstallDirs)
#############################################
# mad-ahal
#############################################
aux_source_directory(. SRC_FILES)
#message(WARNING "PROJECT_SOURCE_DIR is ${PROJECT_SOURCE_DIR}")
#message(WARNING "SRC_FILES is ${SRC_FILES}")

add_library(mad-ahal SHARED
	${SRC_FILES}
)

target_compile_options(mad-ahal PRIVATE -Werror -Wmissing-field-initializers -Wunused-parameter -Wunused-variable -Wsign-compare)

add_definitions(-DUSE_DEFAULT_STDLIB  -DUSE_HELIX_AAC_DECODER)

#set_target_properties(mad-ahal PROPERTIES
#	VERSION ${PROJECT_VERSION}
#	SOVERSION 1)

target_include_directories(mad-ahal PRIVATE
	${PROJECT_SOURCE_DIR}/audio_codec/libmad
	${PROJECT_SOURCE_DIR}/amadec)

#target_link_libraries(mad-ahal
#	libutils libz libbinder libdl libcutils libc liblog)

install(TARGETS mad-ahal
	LIBRARY DESTINATION "lib")
