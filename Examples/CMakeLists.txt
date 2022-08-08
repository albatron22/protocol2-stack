cmake_minimum_required(VERSION 3.0.0)

project(example VERSION 0.1.0)

#set(CMAKE_MAKE_PROGRAM make)

set(SRC_LIST main.c Protocol2_call_back.c ../Protocol2.c)

add_executable(example ${SRC_LIST})

######################
# cd build
#cmd: 
#    cmake .. -G "Unix Makefiles"
######################