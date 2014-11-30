#include "lib_acl.h"

int main(void)
{
	char* s = strdup("aaa\r\n\r\nbbb\r\n\nccc\n\r\nddd\n\neee\r\nfff\nggg\r\n\r\r\nhhh\r\r\r\n\r\niii\r\n");
	char* next = s;
	int nleft = strlen(s);
	ACL_LINE_STATE state;

	printf("-------------------------------------------------------\r\n");
	printf("[%s]\r\n", s);
	printf("-------------------------------------------------------\r\n");

	acl_line_state_reset(&state, 0);

	while (*(s + state.offset) != 0) {
		int n = 1;
		int ret = acl_find_blank_line(s + state.offset, n, &state);
		if (state.finish) {
			char ch = s[state.offset];
			s[state.offset] = 0;
			printf("ok, find it, ret: %d, off: %d, header:\r\n[%s]\r\n",
				ret, state.offset, next);
			s[state.offset] = ch;
			next = s + state.offset;

			acl_line_state_reset(&state, state.offset);
		}
		nleft -= n - ret;
	}

	printf("offset: %d, len: %d\n", state.offset, (int) strlen(s));

	free(s);

	return 0;
}
