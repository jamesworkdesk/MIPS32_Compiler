CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET   = assembler
SRCS     = main.cpp assembler.cpp lexer.cpp encoder.cpp error.cpp
OBJS     = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Header dependencies
main.o: main.cpp assembler.h
assembler.o: assembler.cpp assembler.h lexer.h encoder.h error.h
lexer.o: lexer.cpp lexer.h error.h
encoder.o: encoder.cpp encoder.h lexer.h error.h
error.o: error.cpp error.h

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe

test: $(TARGET)
	./$(TARGET) Input.txt
	@echo "Comparing output (ignoring comments)..."
	@sed 's/;.*/;/' Input.mif > .test_new.tmp
	@sed 's/;.*/;/' Output.mif > .test_ref.tmp
	@diff --strip-trailing-cr .test_new.tmp .test_ref.tmp && echo "PASS: Output matches" || echo "FAIL: Output differs"
	@rm -f .test_new.tmp .test_ref.tmp

.PHONY: all clean test
