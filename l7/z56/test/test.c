#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>

#define CHUNK_SIZE (20 * 1024 * 1024)
#define CHUNK_NUM 100
#define CHUNKS_COMMIT 7
#define OFFSET_SML 0x100000
#define OFFSET_BIG 0x10000000

int main(int arg, char **argv) {
	void *chunks[CHUNK_NUM];

	unsigned long totaltime = 0;

	printf("Program start\n");

	for (size_t i = 0; i < CHUNK_NUM; i++) {
		if (!(chunks[i] = malloc(CHUNK_SIZE))) {
			printf("Allocation %zd failed\n", i);
		}
	}

	sleep(2);
	printf("=========================================%p\n", 0xdeadbeef);
	sleep(3);

	printf("expected pgfault at %p\n", (chunks[5]+OFFSET_SML+0x0));
	sleep(2);
	*((char*)chunks[5] + OFFSET_SML + 0x0) = 0;

	printf("expected pgfault at %p\n", (chunks[5]+OFFSET_SML+0x1000));
	sleep(2);
	*((char*)chunks[5] + OFFSET_SML + 0x1000) = 0;

	sleep(5);

	printf("not expected pgfault %p\n", (chunks[5]+OFFSET_SML+0x500));
	sleep(2);
	*((char*)chunks[5] + OFFSET_SML + 0x500) = 0;

	printf("not expected pgfault %p\n", (chunks[5]+OFFSET_SML+0x1500));
	sleep(2);
	*((char*)chunks[5] + OFFSET_SML + 0x1500) = 0;

	printf("expecting invalid address %p\n", (chunks[5]+OFFSET_BIG));
	sleep(2);
	*((char*)chunks[5] + OFFSET_BIG) = 0;

	sleep(5);
	return 0;
}
