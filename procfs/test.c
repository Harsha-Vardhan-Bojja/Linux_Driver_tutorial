#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    int fd = open("/proc/bojja_proc_dir/bojja_proc", O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Cannot open the file\n");
        return fd;
    }

    int choice;
    char userInput[1024];
    char read_buf[1024];

    while (1) {
        printf("**** Please Enter the Option ******\n");
        printf("        1. Write                \n");
        printf("        2. Read                 \n");
        printf("        3. Exit                 \n");
        printf("*********************************\n");
        scanf(" %d", &choice);
        getchar();  
        printf("Your Option = %d\n", choice);

        switch (choice) {
            case 1:
                printf("Enter the string to write into driver:\n");
                fgets(userInput, sizeof(userInput), stdin);
               	userInput[strcspn(userInput, "\n")] = 0;
                printf("Data Writing ... ");
                write(fd, userInput, strlen(userInput) + 1);
                
                break;

            case 2:
                printf("Data Reading ... ");
                read(fd, read_buf, sizeof(read_buf) - 1);
                printf("Done!\n\nData = %s\n\n", read_buf);
                break;

            case 3:
                close(fd);
                return 0;  

            default:
                printf("Enter a valid option: %d\n", choice);
                break;
        }
    }

    close(fd);
    return 0;
}

