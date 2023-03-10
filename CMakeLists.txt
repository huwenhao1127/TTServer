cmake_minimum_required(VERSION 3.0)
project(TTSever VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 打开构建过程中的详细信息
set(CMAKE_VERBOSE_MAKEFILE ON)

# c/c++编译参数
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -fPIC -ggdb -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

# # 源文件列表
# set(LIB_SRC
#     src/logger/Logger.cpp
#     )

# # 生成动态链接库
# add_library(TTServer SHARED ${LIB_SRC})
# # 添加头文件搜索路径
# target_include_directories(TTServer PUBLIC "${PROJECT_SOURCE_DIR}/src/base")
# target_include_directories(TTServer PUBLIC "${PROJECT_SOURCE_DIR}/src/logger")
# target_include_directories(TTServer PUBLIC "${PROJECT_SOURCE_DIR}/src/logger/Appender")


set(LIB_SRC
    src/test/logger_test.cpp
    src/logger/Logger.cpp
    src/logger/Formatter.cpp
    src/logger/Appender/StdoutAppender.cpp
    src/logger/Appender/FileAppender.cpp
    src/logger/Appender/BaseAppender.cpp
    )

message(${LIB_SRC})


# 生成可执行文件
add_executable(logger_test ${LIB_SRC})
# 添加头文件搜索路径
target_include_directories(logger_test PUBLIC "${PROJECT_SOURCE_DIR}/src/base")
target_include_directories(logger_test PUBLIC "${PROJECT_SOURCE_DIR}/src/logger")
target_include_directories(logger_test PUBLIC "${PROJECT_SOURCE_DIR}/src/logger/Appender")

# # 设置依赖，编译logger_test前先生成动态链接库
# add_dependencies(logger_test TTServer)

# # 连接动态链接库
# target_link_libraries(logger_test TTServer)


# 设置输出路径
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)