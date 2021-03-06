BUILD_DIR := intermediate
TARGET_DIR := ./
SUBMAKEFILES := file.mk

boost_ldlibs := -lboost_regex -lboost_thread -lboost_system -lboost_program_options

override CXXFLAGS += -O3 -march=native -std=c++17 -I./ -W -Wall -g -ggdb3 -pthread -fcilkplus
override LDFLAGS += -pthread -fcilkplus

TARGET := libmax_clique.a

SOURCES := \
    clique.cc \
    bit_graph.cc

TGT_LDLIBS := $(boost_ldlibs)

