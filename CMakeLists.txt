cmake_minimum_required(VERSION 3.5)

project(TRANSPORT_MANAGER LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TRANSPORT_MANAGER main.cpp
                                 graph.h
                                 bus.h
                                 bus.cpp
                                 bus_station.cpp
                                 bus_station.h
                                 transport_manager.cpp
                                 transport_manager.h
                                 path_item.h
                                 json.h
                                 json.cpp
                                 json_serialize.hpp
                                 json_serialize.cpp
                                 requester.h
                                 svg.h)
