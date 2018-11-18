NAME=webproxy
DEPENDENCIES=driver.cpp webserver.cpp webproxy.cpp
#  Compiler to use (gcc / g++)
COMPILER=g++
#  MinGW
ifeq "$(OS)" "Windows_NT"
CFLG=-O3 -Wall
LIBS=
CLEAN=del $(NAME) *.o *.a
else
#  OSX
ifeq "$(shell uname)" "Darwin"
CFLG=-O3 -Wall
LIBS=
#  Linux/Unix/Solaris
else
CFLG=-O3 -Wall
LIBS=
endif
#  OSX/Linux/Unix/Solaris
CLEAN=rm -f $(NAME) *.o *.a
endif

#== Compile and link (customize name at top of makefile)
$(NAME) : $(DEPENDENCIES)
	$(COMPILER) $(CFLG) -o $(NAME) $^ $(LIBS)

#== Clean
clean :
	$(CLEAN)

#== Run: test compiled binary file with argument
test : $(NAME)
	./$(NAME) 8888 60 &

#== Recycle: remove made files, compile, then run test recipe
recycle : $(DEPENDENCIES)
	$(CLEAN)
	$(MAKE)
	$(MAKE) test # Run the compiled binary file
