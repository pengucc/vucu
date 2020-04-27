CFLAGS=$(shell pkg-config --cflags gtk+-2.0) -g
LDLIBS=$(shell pkg-config --libs gtk+-2.0)
CXXFLAGS=${CFLAGS}

vucu: vu.cc vu.tab.o lex.vu.o cu.o cu.tab.o lex.cu.o
	g++ -o vucu $+ ${CXXFLAGS} ${LDLIBS}
vu.tab.o: vu.tab.c
	g++ -c vu.tab.c
lex.vu.o: lex.vu.c vu.tab.c
	g++ -c lex.vu.c

cu.o: cu.cc cu.h cu.tab.c hsv.h
	g++ -c -g cu.cc
cu.tab.o: cu.tab.c
	g++ -c -g cu.tab.c
lex.cu.o: lex.cu.c cu.tab.c
	g++ -c -g lex.cu.c

clean:
	rm -f *.o
	rm -f vucu
