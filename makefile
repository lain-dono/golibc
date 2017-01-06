utf8.test: utf8.c utf8_test.c
	gcc -g -Wall -Wextra -Werror -std=c11 $^ -o $@

clean:
	rm -f *.test
