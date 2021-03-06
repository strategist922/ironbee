
#
# -Wcase-align works with gcc, but not clang on
#  some of the modp_* files.  Remove for now.
#

CFLAGS=-g -O3 -Wall -Wextra -Wshadow -Wpointer-arith -Wcast-qual -Werror
# -DNDEBUG

help:
	@echo ""
	@echo "note: there is no library-- it's just one file"
	@echo "      see one of the unit tests or 'reader' to see how to"
	@echo "      compile"
	@echo ""
	@echo "make test_unit"
	@echo "make test_speed"
	@echo "make test_negative -- test false positives"
	@echo "make test_positive -- test sqli"
	@echo "make indent -- re-idents main c file"
	@echo "make unused -- print unused fingerprints"
	@echo "make sort -- resort fingerprints"
	@echo "make clean  -- cleanup"
	@echo "make lcov -- lcov"
	@echo "make reader -- line by line analyzer"
	@echo "make http -- runs mini-http server, handy for viewing lcov"
	@echo "make sqli -- a CLI tokenizier (need renaming)"
	@echo "make fptool -- a fingerprinting tool"
	@echo ""

allbin: reader sqli fptool

reader: Makefile reader.c libinjection_sqli_data.h libinjection.h libinjection_sqli.c
	${CC}  ${CFLAGS} -o reader libinjection_sqli.c reader.c

reader-gprof: Makefile reader.c libinjection_sqli_data.h libinjection.h libinjection_sqli.c
	make clean
	${CC} -g -O2 -pg -o reader libinjection_sqli.c reader.c

extractor: Makefile extractor.cpp libinjection_sqli.c libinjection.h libinjection_sqli_data.h libinjection_sqli_data.h
	g++ ${STATIC} ${CFLAGS} -o extractor extractor.cpp libinjection_sqli.c

is_sqli: Makefile is_sqli_cli.cpp libinjection_sqli.c libinjection.h libinjection_sqli_data.h libinjection_sqli_data.h
	${CXX}  ${CFLAGS} -O3 -o is_sqli is_sqli_cli.cpp libinjection_sqli.c

sqli: Makefile sqli_cli.c libinjection_sqli.c libinjection.h libinjection_sqli_data.h
	${CC} -g -O1 -o sqli sqli_cli.c

fptool: Makefile fptool.c libinjection_sqli.c libinjection.h libinjection_sqli_data.h
	${CC} ${CFLAGS} -o fptool fptool.c libinjection_sqli.c

test_speed: libinjection_sqli_data.h libinjection_sqli.c libinjection.h test_speed.c
	${CC} ${CFLAGS} -DNDEDBUG -o test_speed libinjection_sqli.c test_speed.c
	./test_speed

test_unit: test_driver.py sqli libinjection_sqli_data.h libinjection_sqli.c libinjection.h
	nosetests --with-xunit test_driver.py

valgrind: test_driver.py sqli libinjection_sqli_data.h libinjection_sqli.c libinjection.h
	rm -f valgrind-*.xml
	${MAKE} VALGRIND=1 test_unit

cppcheck:
	rm -f cppcheck-result.xml
	cppcheck --enable=all --inconclusive --std=posix  --xml . 2> cppcheck-result.xml

unused:
	-${MAKE} test_positive | grep True | awk '{print $$4}' | sort -u > /tmp/junk
	diff /tmp/junk fingerprints.txt  | grep '>'


# fail if any input is 'safe', e.g. not SQLi
test_positive: reader
	./reader ../data/sqli-*.txt

# invert... fail if any are sqli
test_negative: reader
	./reader -i ../data/false_*.txt

.PHONY: test_positive test_negative test test_unit test_speed

sqlparse_data.json: Makefile sqlparse_map.py fingerprints.txt
	./sqlparse_map.py > sqlparse_data.json

libinjection_sqli_data.h: Makefile sqlparse2c.py sqlparse_data.json
	./sqlparse2c.py < sqlparse_data.json > ./libinjection_sqli_data.h

# yeah we do 'make_parens.py twice
#
sort:
	wc -l fingerprints.txt
	cat fingerprints.txt | ./make_parens.py | ./make_parens.py | LC_ALL=C sort -u > /tmp/junk
	mv /tmp/junk fingerprints.txt
	wc -l fingerprints.txt
	${MAKE} libinjection_sqli_data.h

clean:
	rm -f *~ *.pyc
	rm -rf build
	rm -f fptool
	rm -f test_tokens test_speed
	rm -rf *.dSYM
	rm -rf *.gcno *.gcda *.info cppcheck-result.xml cobertura-coverage.xml
	rm -rf lcov-html
	rm -rf core* a.out
	rm -f gmon.out nosetests.xml valgrind-*.txt
	rm -f reader extractor is_sqli lcov-reader sqli

sqli-gcov:
	rm -f sqli
	gcc -g -fprofile-arcs -ftest-coverage -O0 -o sqli sqli_cli.c

# for jenkins
gcovr-unittest-xml: sqli-gcov
	nosetests ./test_driver.py
	gcovr -d -o libinjection-unittest-gcov.xml --gcov-exclude='.*modp_.*'  --gcov-exclude='reader*' --xml -r `pwd`

# for jenkins
gcovr-samples-xml:
	rm -f reader libinjection-samples-gcov.xml
	g++ -g -O0 -fprofile-arcs -ftest-coverage -Wall -Wextra -o reader libinjection_sqli.c reader.c
	./reader ../data/*.txt > /dev/null
	gcovr --version
	gcovr -d -o libinjection-samples-gcov.xml --gcov-exclude='.*modp_.*' --gcov-exclude='reader.*' --xml -r `pwd`

lcov: sqli-gcov
	lcov -b . --directory . --zerocounters
	./test_driver.py ../tests/test-*.txt
	lcov -b . --directory . --capture --output-file libinjection.info
	lcov -b . --directory . --remove libinjection.info '/usr/include*' -o libinjection.info
	lcov -b . --directory . --remove libinjection.info 'modp_*' -o libinjection.info
	lcov -b . --directory . --remove libinjection.info 'sqli' -o libinjection.info
	rm -rf lcov-html
	mkdir lcov-html
	genhtml -o lcov-html libinjection.info
.PHONY: lcov

lcov-reader: Makefile sqlparse_data.h libinjection.h libinjection_sqli.c reader.c
	g++ -g -O0 -fprofile-arcs -ftest-coverage -Wall -Wextra -o reader libinjection_sqli.c reader.c
	lcov -b . --directory . --zerocounters
	-./reader ../data/*.txt
	lcov -b . --directory . --capture --output-file libinjection.info
	lcov -b . --directory . --remove libinjection.info '/usr/include*' -o libinjection.info
	lcov -b . --directory . --remove libinjection.info 'modp_*' -o libinjection.info
	lcov -b . --directory . --remove libinjection.info 'reader' -o libinjection.info
	rm -rf lcov-html
	mkdir lcov-html
	genhtml -o lcov-html libinjection.info
.PHONY: lcov-reader

http:
	python -m  SimpleHTTPServer
.PHONY: http

python: libinjection_sqli.c libinjection.h libinjection_sqli_data.h python_libinjection_module.c
	python setup.py --verbose build --force
	sudo python setup.py --verbose install
