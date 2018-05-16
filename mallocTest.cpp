#include <stdlib.h>

int main() {
	int len = 10;
	char *a = (char *)malloc(len);
	free(a);
	a = NULL;
	a = (char *)malloc(len + 100);
	free(a);
	a = NULL;
	return 0;
}
