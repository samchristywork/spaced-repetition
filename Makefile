build/spaced-repetition: main.c
	mkdir -p build
	gcc -o build/spaced-repetition main.c -lm

clean:
	rm -rf build
