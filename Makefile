ALL=explain out sort struct8 struct16 struct32

all:	${ALL}

explain:	explain.c

out:	out.c

sort:	sort.c

struct8:	struct.c
	${CC} -fpack-struct=1 struct.c -o struct8

struct16:	struct.c
	${CC} -fpack-struct=2 struct.c -o struct16

struct32:	struct.c
	${CC} -fpack-struct=4 struct.c -o struct32

clean:
	rm -f ${ALL}
