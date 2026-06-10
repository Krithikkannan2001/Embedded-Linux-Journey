all: hello calculator

hello: hello.c
	gcc hello.c -o hello

calculator: calculator.c
	gcc calculator.c -o calculator

calculator_preproc: calculator.c
	gcc -E calculator.c -o calculator.i

calculator_comp: calculator.c
	gcc -S calculator.i -o calculator.s

calculator_ass: calculator.c
	gcc -c calculator.s -o calculator.o

clean:
	rm -f calculator calculator.i calculator.s calculator.o hello
