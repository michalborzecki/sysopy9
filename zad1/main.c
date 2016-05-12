#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void cleanup();
void sigint_handler(int signum);
void *philosopher_thread(void *arg);

sem_t forks[5];
sem_t waiter;
pthread_t threads_ids[5];

int main(int argc, char *argv[]) {
    atexit(cleanup);
    struct sigaction act;
    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTSTP, &act, NULL);
    for (int i = 0; i < 5; i++) {
        if (sem_init(&(forks[i]), 0, 1) != 0) {
            printf("Error while creating semaphore occurred.\n");
            return 1;
        }
    }
    if (sem_init(&waiter, 0, 4) != 0) {
        printf("Error while creating semaphore occurred.\n");
        return 1;
    }

    // prepare mask for philosophers' threads (after that, only main thread will catch signals).
    sigset_t signal_mask;
    sigset_t old_signal_mask;
    sigfillset(&signal_mask);
    pthread_sigmask(SIG_SETMASK, &signal_mask, &old_signal_mask);
    for (int i = 0; i < 5; i++) {
        if (pthread_create(&(threads_ids[i]), NULL, philosopher_thread, NULL) != 0) {
            printf("Error while creating new thread occurred.\n");
            break;
        }
    }
    pthread_sigmask(SIG_SETMASK, &old_signal_mask, NULL);

    while (1)
        pause();
}

void *philosopher_thread(void *arg) {
    int philosopher_id = 0;
    for (int i = 0; i < 5; i++) {
        if (pthread_equal(threads_ids[i], pthread_self())) {
            philosopher_id = i;
            break;
        }
    }
    unsigned int thinking_utime = 0;
    unsigned int eating_time = 500000;
    while (1) {
        printf("Philosopher #%d is thinking.\n", philosopher_id);
        thinking_utime = ((unsigned)rand() % 500000) + 500000;
        usleep(thinking_utime);

        sem_wait(&waiter);
        sem_wait(&forks[philosopher_id]);
        sem_wait(&forks[(philosopher_id + 1) % 5]);
        printf("Philosopher #%d is eating.\n", philosopher_id);
        usleep(eating_time);
        sem_post(&forks[philosopher_id]);
        sem_post(&forks[(philosopher_id + 1) % 5]);
        sem_post(&waiter);
    }
    return NULL;
}

void cleanup() {
    for (int i = 0; i < 5; i++)
        pthread_cancel(threads_ids[i]);
    for (int i = 0; i < 5; i++)
        pthread_join(threads_ids[i], NULL);
    for (int i = 0; i < 5; i++)
            sem_close(&forks[i]);
    sem_close(&waiter);
}

void sigint_handler(int signum) {
    printf("Program closed.\n");
    exit(0);
}
