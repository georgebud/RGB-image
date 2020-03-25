build: quadtree.o
	gcc quadtree.o -o quadtree -lm

quadtree.o: quadtree.c
	gcc -Wall -c quadtree.c -o quadtree.o

clean:
	rm quadtree quadtree.o
