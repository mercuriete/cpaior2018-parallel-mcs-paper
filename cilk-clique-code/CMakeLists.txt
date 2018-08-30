# Distributed under the MIT License (See accompanying file /LICENSE )

# CMake build : main application

#configure variables
set (APP_NAME "${PROJECT_NAME}CilkCliqueCode")

#configure directories
set (APP_MODULE_PATH "${PROJECT_SOURCE_DIR}/cilk-clique-code")
set (APP_SRC_PATH  "${APP_MODULE_PATH}/src" )
set (APP_INCLUDE_PATH  "${APP_MODULE_PATH}/include")

#set includes
include_directories (${APP_INCLUDE_PATH})

#set sources
file (GLOB LIB_HEADER_FILES "${APP_INCLUDE_PATH}/*.hh")
file (GLOB APP_SOURCE_FILES "${APP_SRC_PATH}/*.cc")

#set target executable
add_executable (${APP_NAME} ${APP_SOURCE_FILES})

