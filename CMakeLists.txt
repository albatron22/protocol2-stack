cmake_minimum_required(VERSION 3.0.0)

project(protocol2 VERSION 0.0.1 LANGUAGES CXX)

add_library(protocol2 STATIC Protocol2.c)

set_target_properties(protocol2 PROPERTIES LINKER_LANGUAGE CXX)