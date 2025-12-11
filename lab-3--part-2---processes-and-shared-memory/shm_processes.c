#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

void ChildProcess(int *);

int main(int argc, char *argv[])
{
    int ShmID;
    int *ShmPTR;
    pid_t pid;
    int status;
    int i;

    // Seed random number generator
    srand(time(NULL));

    ShmID = shmget(IPC_PRIVATE, 2*sizeof(int), IPC_CREAT | 0666);
    if (ShmID < 0) {
        printf("*** shmget error (server) ***\n");
        exit(1);
    }
    printf("Parent has received a shared memory of two integers...\n");

    ShmPTR = (int *) shmat(ShmID, NULL, 0);
    if ((long)ShmPTR == -1) {
        printf("*** shmat error (server) ***\n");
        exit(1);
    }
    printf("Parent has attached the shared memory...\n");

    // Initialize shared memory
    ShmPTR[0] = 0;  // BankAccount
    ShmPTR[1] = 0;  // Turn
    printf("Parent has initialized BankAccount = %d and Turn = %d\n",
           ShmPTR[0], ShmPTR[1]);

    printf("Parent is about to fork a child process...\n");
    pid = fork();
    if (pid < 0) {
        printf("*** fork error (server) ***\n");
        exit(1);
    }
    else if (pid == 0) {
        // Child process
        srand(time(NULL) + getpid());  // Different seed for child
        ChildProcess(ShmPTR);
        exit(0);
    }

    // Parent process (Dear Old Dad)
    for (i = 0; i < 25; i++) {
        // Sleep random time 0-5 seconds
        sleep(rand() % 6);

        // Copy BankAccount to local variable
        int account = ShmPTR[0];

        // Wait for Turn to be 0
        while (ShmPTR[1] != 0);

        if (account <= 100) {
            // Try to deposit money
            int balance = rand() % 101;  // Random 0-100

            if (balance % 2 == 0) {
                // Even - deposit
                account += balance;
                printf("Dear old Dad: Deposits $%d / Balance = $%d\n", 
                       balance, account);
            } else {
                // Odd - no money
                printf("Dear old Dad: Doesn't have any money to give\n");
            }
        } else {
            printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", 
                   account);
        }

        // Copy back to shared memory
        ShmPTR[0] = account;

        // Set Turn to 1 (Child's turn)
        ShmPTR[1] = 1;
    }

    wait(&status);
    printf("Parent has detected the completion of its child...\n");
    shmdt((void *) ShmPTR);
    printf("Parent has detached its shared memory...\n");
    shmctl(ShmID, IPC_RMID, NULL);
    printf("Parent has removed its shared memory...\n");
    printf("Parent exits...\n");
    exit(0);
}

void ChildProcess(int *SharedMem)
{
    int i;

    printf("   Child process started\n");

    // Poor Student
    for (i = 0; i < 25; i++) {
        // Sleep random time 0-5 seconds
        sleep(rand() % 6);

        // Copy BankAccount to local variable
        int account = SharedMem[0];

        // Wait for Turn to be 1
        while (SharedMem[1] != 1);

        // Generate random balance needed (0-50)
        int balance = rand() % 51;
        printf("Poor Student needs $%d\n", balance);

        if (balance <= account) {
            // Withdraw
            account -= balance;
            printf("Poor Student: Withdraws $%d / Balance = $%d\n", 
                   balance, account);
        } else {
            printf("Poor Student: Not Enough Cash ($%d)\n", account);
        }

        // Copy back to shared memory
        SharedMem[0] = account;

        // Set Turn to 0 (Parent's turn)
        SharedMem[1] = 0;
    }

    printf("   Child process is about to exit\n");
}