#include <stdio.h>
int main(int argc,char** argv){
    int n, t1 = 0, t2 = 1, nextTerm;

    printf("Enter the number of terms: ");
    n = argv[1];

    if (n <= 0) {
        printf("Please enter a positive integer.\n");
    } else if (n == 1) {
        printf("Fibonacci Series: 0\n");
    } else {
        printf("Fibonacci Series: %d, %d", t1, t2);

        for (int i = 3; i <= n; i++) {
            nextTerm = t1 + t2;
            printf(", %d", nextTerm);
            t1 = t2;
            t2 = nextTerm;
        }
        printf("\n");
    }

    return 0;
}
