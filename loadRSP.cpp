#include <stdio.h>

#define MAX_SIZE 20

int main() {
	int ret;
	int arr[MAX_SIZE] = {0};

	for (int i = 0; i < MAX_SIZE; i++) {
		arr[i] = 0x1;
	}

	__asm__ volatile (	"movq %0, %%rcx;" : : "r"(arr)	);
	__asm__ volatile (	"movq %rsp, %r10;"	);
	__asm__ volatile (	"movq %rcx, %rsp;"	);
	__asm__ volatile (	"movq $1, (%rsp);"	);
	__asm__ volatile (	"movq %r10, %rsp;"	);
	
	printf("The contents of Arr is : ");
	for (int i = 0; i < MAX_SIZE; i++) {
		printf("%d ", arr[i]);
	}
	printf("\n");
	return 0;
}
