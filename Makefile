# NAME: Ruth Johnson
# EMAIL: ruthjohnson@ucla.edu
# ID: 704275412

CC=gcc

default: lab3a.o
	$(CC) -o lab3a lab3a.c
check:
	bash test_script.sh
clean:
	rm lab4b-704275412.tar.gz
	rm lab4b.c
	rm README
	rm Makefile
	rm test_script.sh
	rm lab4b.o
	rm lab4b

dist:
	tar -czvf lab4b-704275412.tar.gz lab4b.c README Makefile test_script.sh
