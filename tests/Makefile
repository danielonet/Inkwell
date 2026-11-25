# Platform independent tests (run on Linux or Haiku): make -C tests test
CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra

all: markdown_test rtf_test

markdown_test: markdown_test.cpp ../src/MarkdownRender.cpp ../src/MarkdownRender.h
	$(CXX) $(CXXFLAGS) -o $@ markdown_test.cpp ../src/MarkdownRender.cpp

rtf_test: rtf_test.cpp ../src/Rtf.cpp ../src/Rtf.h
	$(CXX) $(CXXFLAGS) -o $@ rtf_test.cpp ../src/Rtf.cpp

test: all
	./markdown_test
	./rtf_test

clean:
	rm -f markdown_test rtf_test

.PHONY: all test clean
