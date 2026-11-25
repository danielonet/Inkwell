# Platform independent tests (run on Linux or Haiku): make -C tests
CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra

markdown_test: markdown_test.cpp ../src/MarkdownRender.cpp ../src/MarkdownRender.h
	$(CXX) $(CXXFLAGS) -o $@ markdown_test.cpp ../src/MarkdownRender.cpp

test: markdown_test
	./markdown_test

clean:
	rm -f markdown_test

.PHONY: test clean
