LIBS = -L /usr/local/lib -l gaul -L /usr/local/lib -l gaul_util -ansi
LIBS2 = -L /usr/local/lib -l gaul -l gaul_util -std=c++11

all: gen

gen:
	g++ gen.cpp run_voltspot.cpp $(LIBS) -o genetic


genetic_newcpp: gen_library gen_me
	g++ -o genetic gen_library.o gen_me.o

gen_me:
	g++ gen.cpp $(LIBS2) -o gen_me.o

gen_library:
	g++ -c gen_library.cpp $(LIBS) -o gen_library.o

cpp: genetic_cpp gen_exec_cpp
	g++ -o genetic_cpp gen_input_proc.o gen_exec_cpp.o

genetic_cpp: gen_input_proc.c
	g++ -c gen_input_proc.c $(LIBS) -o gen_input_proc.o
gen_exec_cpp: gen_exec.cpp
	g++ -c gen_exec.cpp $(LIBS2) -o gen_exec_cpp.o

clean:
	rm genetic
