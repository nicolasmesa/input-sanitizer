# Makeifle for input_sanitizer

# List the object files in one place
OBJ=main.o

build:	input_sanitizer

input_sanitizer: $(OBJ)
	cc -o $@ $(OBJ)

test:	build
	@echo Test 1
	./input_sanitizer < test_data/test1.txt
	
	@echo Test 2 
	./input_sanitizer < test_data/test2.txt

	@echo Test 3 
	./input_sanitizer < test_data/test3.txt
	
	@echo Test 4 
	./input_sanitizer < test_data/test4.txt

exec: build
	./input_sanitizer $(ARG)

clean:
	rm -f input_sanitizer *.o
	rm -f /tmp/*.nm2805
	rm -f /tmp/.*.nm2805
	rm -f /home/nm2805/*.nm2805
	rm -f /homw/nm2805/.*.nm2805

