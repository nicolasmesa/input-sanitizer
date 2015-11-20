# Makeifle for input_sanitizer

# List the object files in one place
OBJ=main.o

build:	input_sanitizer

input_sanitizer: $(OBJ)
	cc -o $@ $(OBJ)

test:	build
	./input_sanitizer < test1.txt

exec: build
	./input_sanitizer $(ARG)

clean:
	rm -f input_sanitizer *.o

