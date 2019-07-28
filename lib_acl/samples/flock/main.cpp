#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>

int main(int argc acl_unused, char *argv[] acl_unused)
{
	ACL_FILE_HANDLE handle;
	const char *filename = "test.lock";
	char  ebuf[256];

	handle = acl_file_open(filename, O_RDWR | O_CREAT, 0600);
	if (handle == ACL_FILE_INVALID) {
		acl_msg_fatal("open file %s error(%s)",
			filename, acl_last_strerror(ebuf, sizeof(ebuf)));
	}

	printf("open %s ok, begin to lock it\r\n", filename);

	if (acl_myflock(handle, ACL_FLOCK_STYLE_FCNTL, ACL_FLOCK_OP_EXCLUSIVE) == -1) {
		acl_msg_error("lock error(%s)", acl_last_strerror(ebuf, sizeof(ebuf)));
		getchar();
		return (-1);
	}

	printf("lock %s ok, enter any key to unlock\r\n", filename);
	getchar();

	if (acl_myflock(handle, ACL_FLOCK_STYLE_FCNTL, ACL_FLOCK_OP_NONE) == -1) {
		acl_msg_error("unlock error(%s)", acl_last_strerror(ebuf, sizeof(ebuf)));
		getchar();
		return (-1);
	}

	printf("unlock %s ok, enter any key to close file\r\n", filename);
	getchar();
	
	acl_file_close(handle);

	return (0);
}
