LIBS = -L /usr/local/lib -l gaul-l gaul_util -ansi
LIBS2 = -L /usr/local/lib -l gaul -l gaul_util -std=c++11

all: gen

gen:
	g++ gen.cpp $(LIBS2) -o genetic

genetic_newcpp:
	g++ gen_input_proc.cpp gen_exec.cpp $(LIBS2) -o genetic_cpp

cpp: genetic_cpp gen_exec_cpp
	g++ -o genetic_cpp gen_input_proc.o gen_exec_cpp.o

genetic_cpp: gen_input_proc.c
	g++ -c gen_input_proc.c $(LIBS) -o gen_input_proc.o
gen_exec_cpp: gen_exec.cpp
	g++ -c gen_exec.cpp $(LIBS2) -o gen_exec_cpp.o

genetic: gen_input_proc.c gen_exec.c
		gcc gen_input_proc.c gen_exec.c $(LIBS) -o genetic

genetic_test: gen_algo_tut.c
	gcc gen_algo_tut.c $(LIBS) -o genetic

clean:
	rm genetic
