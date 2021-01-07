CC=gcc
CXX=g++
FLEX=flex
BISON=bison

.lex: lex.l
	$(FLEX) lex.l
.syntax: syntax.y
	$(BISON) -t -d syntax.y
splc: .lex .syntax
	@mkdir -p bin
	$(CXX) syntax.tab.c -g -lfl -ly -o bin/splc
	@chmod +x bin/splc
clean:
	@rm -rf bin/
	@rm -f lex.yy.c syntax.tab.*
.PHONY: splc
