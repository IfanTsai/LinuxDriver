#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int fd = open("/dev/x210-button", O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    int key_val;
    while (1) {
        if (read(fd, &key_val, sizeof(key_val)) < 0) {
            perror("read");
            exit(1);
        }
        printf("-----------------> the button val is %d <------------------\n",key_val);
    }
    close(fd);
    return 0;
}
