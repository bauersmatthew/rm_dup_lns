CXX = g++
CFLAGS = -std=c++1y -Wall

all: rm_dup_lns.cpp
	$(CXX) $(CFLAGS) rm_dup_lns.cpp -o rm_dup_lns
