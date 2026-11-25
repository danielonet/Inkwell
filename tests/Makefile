# Platform independent tests (run on Linux or Haiku): make -C tests test
CXX ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra

all: markdown_test rtf_test spell_test

markdown_test: markdown_test.cpp ../src/MarkdownRender.cpp ../src/MarkdownRender.h
	$(CXX) $(CXXFLAGS) -o $@ markdown_test.cpp ../src/MarkdownRender.cpp

rtf_test: rtf_test.cpp ../src/Rtf.cpp ../src/Rtf.h
	$(CXX) $(CXXFLAGS) -o $@ rtf_test.cpp ../src/Rtf.cpp

spell_test: spell_test.cpp ../src/Spell.cpp ../src/Spell.h
	$(CXX) $(CXXFLAGS) -o $@ spell_test.cpp ../src/Spell.cpp

test: all
	./markdown_test
	./rtf_test
	./spell_test

clean:
	rm -f markdown_test rtf_test spell_test

.PHONY: all test clean
