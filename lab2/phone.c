#include <stdio.h>
#include <stdlib.h>

/*
Write a small program to use scanf to read two values from standard input. The first is a
is a 10 character string and the second is an integer. The program takes no command-line arguments.

If the integer is -1, the program prints the full string to stdout. If the integer is between 0 and 9 inclusive
the program prints only the corresponding digit from the string to stdout. In both of these cases the
program returns 0. If the integer is less than -1 or greater than 9, the program prints the message "ERROR"
to stdout and returns 1.

 */




int main() {
    char my_str[11];
    int my_int;
    scanf("%10s", my_str);
    scanf("%d", &my_int);

    if (my_int == -1) {
       printf("%s\n", my_str);
    } else if (my_int >= 0 && my_int <= 9) {
       printf("%c\n", my_str[my_int]);
    } else {
       printf("%s\n", "ERROR");
       return 1;
    }
    return 0;
}


