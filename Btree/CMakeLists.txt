cmake_minimum_required(VERSION 3.31)
PROJECT(Btree)

add_definitions(-g)
#add_definitions(-v)

INCLUDE_DIRECTORIES(include)

AUX_SOURCE_DIRECTORY(source SRCS)

ADD_EXECUTABLE(Btree ${SRCS})
