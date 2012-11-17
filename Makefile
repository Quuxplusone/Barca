barca: *.c *.cc *.h
	gcc -c ImageFmtc.c simple-png.c
	g++ barca.cc input.cc ai.cc ImageFmtc.o simple-png.o -lpng -o barca
