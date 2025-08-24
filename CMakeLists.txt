cmake_minimum_required(VERSION 3.5.0)
project(kanji-anki-card-builder VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 23)

add_executable(kanji-anki-card-builder source.cpp pugixml/pugixml.cpp)


