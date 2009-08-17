shFSO: shFSO.c
	gcc -o shFSO -g -g3 -ggdb -ggdb3 shFSO.c

clean:
	-rm shFSO

.PHONY: clean