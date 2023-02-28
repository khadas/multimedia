cmake_minimum_required(VERSION 3.8)

include(GNUInstallDirs)

#############################################
# faad-ahal
#############################################
aux_source_directory(. SRC_FILES)
message(WARNING "SRC_FILES is ${SRC_FILES}")

add_library(faad-ahal SHARED
	${SRC_FILES}
)

target_compile_options(faad-ahal PRIVATE -Werror -Wunused-function)

#add_definitions(-DUSE_DEFAULT_STDLIB  -DUSE_HELIX_AAC_DECODER)

#set_target_properties(faad-ahal PROPERTIES
#	VERSION ${PROJECT_VERSION}
#	SOVERSION 1)

target_include_directories(faad-ahal PRIVATE
	${PROJECT_SOURCE_DIR}/audio_codec/libfaad
	${PROJECT_SOURCE_DIR}/audio_codec/libfaad/codebook
	${PROJECT_SOURCE_DIR}/audio_codec/libfaad/helixaac
	${PROJECT_SOURCE_DIR}/amadec)

target_link_libraries(faad-ahal
	-lcutils)

install(TARGETS faad-ahal
	LIBRARY DESTINATION "lib")
