all: man
	gcc -Wall -g -DVERSION=\"1.0\" -DGIT_COMMIT=\"$$(git show-ref -s |head -n1)\" -lX11 *.c -o bin/clio
man:
	-rm -rf bin
	-mkdir bin
	pandoc -s -t man man.md | gzip > bin/clio.1.gz
install:
	install -m755 bin/clio /usr/bin
	install -m755 bin/clio.1.gz /usr/share/man/man1
