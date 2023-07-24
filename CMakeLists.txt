cmake_minimum_required(VERSION 3.0)
project(TTSever VERSION 1.0)

# 打开构建过程中的详细信息
set(CMAKE_VERBOSE_MAKEFILE OFF)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# c/c++编译参数
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -lpthread -O3 -fPIC -ggdb -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# 生成版本信息
# 添加自定义cmake文件目录
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# 使用Utility.cmake
include(Utility)

# 获取当前的GIT_HASH
set(GIT_HASH "")
get_git_hash(GIT_HASH)

# 获取当前的分支
set(GIT_BRANCH "")
get_git_branch(GIT_BRANCH)

# 获取最后提交时间
set(GIT_TIME "")
get_git_time(GIT_TIME)

# 获取最后提交描述
set(GIT_DES "")
get_git_des(GIT_DES)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in          # 输入
  ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h             # 输出
  )

message(STATUS "GIT_BRANCH: ${GIT_BRANCH}")
message(STATUS "GIT_HASH: ${GIT_HASH}")
message(STATUS "GIT_TIME: ${GIT_TIME}")
message(STATUS "GIT_DES: ${GIT_DES}")

# 编译动态连接库
# -----------------------------------------
# 源文件列表
set(TTSEVER_SRC
    src/logger/Logger.cpp
    src/logger/Formatter.cpp
    src/logger/Appender/StdoutAppender.cpp
    src/logger/Appender/FileAppender.cpp
    src/logger/Appender/BaseAppender.cpp
    src/logger/Appender/RotateAppender.cpp
    )
# 生成动态链接库
add_library(TTServer SHARED ${TTSEVER_SRC})
# 连接fmt库
add_subdirectory(run/lib/fmt EXCLUDE_FROM_ALL)
target_link_libraries(TTServer fmt-header-only)
# 添加头文件搜索路径
target_include_directories(TTServer PUBLIC "${PROJECT_SOURCE_DIR}/src/base")
target_include_directories(TTServer PUBLIC "${PROJECT_SOURCE_DIR}/src/logger")
target_include_directories(TTServer PUBLIC "${PROJECT_SOURCE_DIR}/src/logger/Appender")
target_include_directories(TTServer PUBLIC "${PROJECT_SOURCE_DIR}/run/lib/fmt/include/fmt")


# 生成可执行文件
# ------------------------------------------
# 时间轮测试
# set(TIME_TEST_BIN_SRC  
#     src/test/time_test.cpp
#     src/time/Now.cpp
#     src/time/TimeWheel.cpp
#     src/base/BaseThread.cpp
#     )
# add_executable(time_test ${TIME_TEST_BIN_SRC})
# # 添加头文件搜索路径
# target_include_directories(time_test PUBLIC "${PROJECT_SOURCE_DIR}/src/time")
# target_include_directories(time_test PUBLIC "${PROJECT_SOURCE_DIR}/src/base")
# target_include_directories(time_test PUBLIC  "${PROJECT_SOURCE_DIR}/src/logger")
# target_include_directories(time_test PUBLIC  "${PROJECT_SOURCE_DIR}/src/logger/Appender")
# # 设置依赖，编译logger_test前先生成动态链接库
# add_dependencies(time_test TTServer)
# # 连接动态链接库
# target_link_libraries(time_test TTServer)

# -------------------------------------
# logger功能测试
set(LOGGER_TEST_BIN_SRC  
    src/test/logger_test.cpp
    )
add_executable(logger_test ${LOGGER_TEST_BIN_SRC})
# 添加头文件搜索路径
target_include_directories(logger_test PUBLIC "${PROJECT_SOURCE_DIR}/src/base")
target_include_directories(logger_test PUBLIC "${PROJECT_SOURCE_DIR}/src/logger")
target_include_directories(logger_test PUBLIC "${PROJECT_SOURCE_DIR}/src/logger/Appender")
# 设置依赖，编译logger_test前先生成动态链接库
add_dependencies(logger_test TTServer)
# 连接动态链接库
target_link_libraries(logger_test TTServer)

# 设置输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/run/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/run/src)

# -------------------------------------
# # 线程功能测试
# set(THREAD_TEST_BIN_SRC  
#     src/test/thread_test.cpp
#     src/base/BaseThread.cpp
#     )
# add_executable(thread_test ${LOGGER_TEST_BIN_SRC})
# # 添加头文件搜索路径
# target_include_directories(thread_test PUBLIC "${PROJECT_SOURCE_DIR}/src/base")
# target_include_directories(thread_test PUBLIC "${PROJECT_SOURCE_DIR}/src/logger")
# target_include_directories(thread_test PUBLIC "${PROJECT_SOURCE_DIR}/src/logger/Appender")

# # 设置输出路径
# SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/run/bin)
# SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/run/src)
