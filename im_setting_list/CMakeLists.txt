CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(SRC_LIST
    input_method_setting_list_ui.cpp
    input_method_setting_list.cpp
)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
    SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")

#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
                  capi-appfw-application appcore-efl isf elementary vconf
                  vconf-internal-keys efl-extension pkgmgr-info dlog
)

FOREACH(flag ${pkgs_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
    SET(EXTRA_CXXFLAGS "${EXTRA_CXXFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -Wall -fPIE")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXXFLAGS} -Wall -fPIE")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -Wall -fPIE")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -Wall -fPIE")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fPIE")
SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -g -fPIE")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

SET(TARGET_NAME "${PROJECT_NAME}-list")
ADD_EXECUTABLE(${TARGET_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES(${TARGET_NAME} ${pkgs_LDFLAGS} "-lm -pie")
INSTALL(TARGETS ${TARGET_NAME} DESTINATION ${BINDIR})
