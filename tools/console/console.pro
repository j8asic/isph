TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
TARGET = isph

QMAKE_CC = clang
QMAKE_CXX = clang++

LIBS = -stdlib=libc++ -lc++abi -lpthread -lOpenCL -L../../isphlib -lisph
QMAKE_CXXFLAGS = -stdlib=libc++ -std=c++11
QMAKE_CXXFLAGS_RELEASE = -O3
QMAKE_CXXFLAGS_DEBUG = -O0 -g

INCLUDEPATH += /usr/include/c++/v1 ../../isphlib

SOURCES += main.cpp
