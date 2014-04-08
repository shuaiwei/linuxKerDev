#include<stdio.h>
#include<fcntl.h>
int main()
{
	int fd;

	/* Will map to the character device routine kyouko2_open
	 */
	fd = open("/dev/kyouko2",O_RDWR);

	if(fd >= 0)
		printf("Open in user space.\n");

	/* Will map to the kyouko2_release
	 */
	close(fd);
	
	return 0;
}
