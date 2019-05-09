#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PWM_IOCTL_SET_FREQ      1
#define PWM_IOCTL_STOP          0

int main(void)
{
    int fd = open("/dev/buzzer", O_RDWR);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    char freq[10];
    while (1) {
        printf("输入频率 | close(关闭) | exit(退出):\r\n");
        scanf("%s", freq);
        if (!strcmp(freq, "exit")) {
            break;
        } else if (!strcmp(freq, "stop")) {
            ioctl(fd, PWM_IOCTL_STOP);
        } else {
            ioctl(fd, PWM_IOCTL_SET_FREQ, atoi(freq));
        }
    }
    ioctl(fd, PWM_IOCTL_STOP);
    close(fd);
    return 0;
}

