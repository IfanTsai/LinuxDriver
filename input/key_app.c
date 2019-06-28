#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/input.h>

#define KEYBOARD_PATH "/dev/input/event5"
#define MOUSE_PATH "/dev/input/event7"
#define X210_KEY_PATH "/dev/input/event2"

int main(void)
{
    int fd = open(X210_KEY_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    struct input_event event = { 0 };
    while (1) {
        if (read(fd, &event, sizeof(struct input_event)) < 0) {
            perror("read");
            exit(1);
        }
        printf("\n----------------start---------------------\n");
        printf("type  = %u\n", event.type);
        printf("code  = %u\n", event.code);
        printf("value = %u\n", event.value);
        printf("----------------end---------------------\n");
    }

    close(fd);
    return 0;
}
