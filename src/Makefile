raycaster: raycaster.o api_interface.o
	g++ -ggdb3 -Wall -o raycaster raycaster.o api_interface.o -lSDL

raycaster.o: raycaster.cc
	g++ -ggdb3 -Wall -c -o raycaster.o raycaster.cc

api_interface.o: api_interface.cc api_interface.h
	g++ -ggdb3 -Wall -c -o api_interface.o api_interface.cc

clean:
	rm raycaster *.o

