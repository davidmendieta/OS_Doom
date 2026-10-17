#include "libc.h"



void one(int fd) {
    printf("*** fd = %d\n",fd);
    printf("*** len = %d\n",len(fd));

    cp(fd,2);
}

int main(int argc, char** argv) {
    struct Pollfd {
        int fd;
        short events;
        short revents;
    };
    // iamateapot();
    printf("***Quick tests for poll!\n");


    //open the keyboard as a file descriptor
    int keyboard = open("/dev/input/keyboard", 0);

    printf("***Keyboard assigned to fd #%d\n", keyboard);

    struct Pollfd* key_pfd = (struct Pollfd*)malloc(sizeof(struct Pollfd));
    key_pfd->fd = keyboard;
    key_pfd->events = 0x1;
    key_pfd->revents = 0;

    //should loop infinitely until pressing a key if using gui or sending a key with no gui
    int poll_cnt = poll((uint32_t*)key_pfd, 1, -1);
    printf("***Checking return value\n***Expected : 1, Actual : %d\n\n", poll_cnt);


    //open mouse as file descriptor
    int mouse = open("/dev/input/mouse", 0);

    printf("***Mouse assigned to fd #%d\n", mouse);

    struct Pollfd* mouse_pfd = (struct Pollfd*)malloc(sizeof(struct Pollfd));
    mouse_pfd->fd = mouse;
    mouse_pfd->events = 0x1;
    mouse_pfd->revents = 0;

    poll_cnt += poll((uint32_t*)mouse_pfd, 1, -1);
    printf("***Checking return value\n***Expected : 2, Actual : %d\n\n", poll_cnt);


    printf("***Test 1: testing with several fds!!!\n");

    //by default stdin, out, and err should always be ready, right now no way to dequeue buffers so 
    //keyboard and mouse will always be ready since buffers still have content
    struct Pollfd* stdin_pfd = (struct Pollfd*)malloc(sizeof(struct Pollfd));
    stdin_pfd->fd = 0;
    stdin_pfd->events = 0x1;
    stdin_pfd->revents = 0; 

    struct Pollfd* stdout_pfd = (struct Pollfd*)malloc(sizeof(struct Pollfd));
    stdout_pfd->fd = 1;
    stdout_pfd->events = 0x4;
    stdout_pfd->revents = 0;

    struct Pollfd* stderr_pfd = (struct Pollfd*)malloc(sizeof(struct Pollfd));
    stderr_pfd->fd = 2;
    stderr_pfd->events = 0x4;
    stderr_pfd->revents = 0;

    //create an array of pollfdss to pass into poll
    struct Pollfd* fds = (struct Pollfd*) malloc(sizeof(struct Pollfd) * 5);
    fds[0] = *key_pfd;
    fds[1] = *mouse_pfd;
    fds[2] = *stdin_pfd;
    fds[3] = *stdout_pfd;
    fds[4] = *stderr_pfd;

    poll_cnt += poll((uint32_t*)fds, 5, -1);
    printf("***Checking return value\n***Expected : 7, Actual : %d\n\n", poll_cnt);


    printf("***Test 2: testing timeout!!!\n");
    stdin_pfd->events = 0x20;

    //timeouts should return 0, so value should stay the same
    poll_cnt += poll((uint32_t*)stdin_pfd, 1, 10);
    printf("***Checking return value\n***Expected : 7, Actual : %d\n\n", poll_cnt);


    printf("***Test 3: testing errors!!!\n");
    //change a previous pollfd's fd to be some invalid num
    mouse_pfd->fd = 99;
    //errors should consider fd as "ready" and return 1
    poll_cnt += poll((uint32_t*)mouse_pfd, 1, -1);
    printf("***Checking return value\n***Expected : 8, Actual : %d\n\n", poll_cnt);


    //final test tests an infinite loop where a pollfds waits for an event should never happen
    //therefore, test should "end" here and process shouldn't shut down unless forced by tester 
    printf("***Final Test: testing indefinite timeout!!!\n");

    mouse_pfd->fd = mouse;
    mouse_pfd->events = 0x00; //cant write into a mouse
    poll_cnt += poll((uint32_t*)mouse_pfd, 1, -1);

    printf("***FAILED! SHOULD NEVER REACH HERE!\n");
    shutdown();
    return 0;
}
