#include "libc.h"



void one(int fd) {
    printf("*** fd = %d\n",fd);
    printf("*** len = %d\n",len(fd));

    cp(fd,2);
}

int main(int argc, char** argv) {

    // iamateapot();
    const char* secret = "gheith";
    int len_secret= 6;
    char display[8];

    for (int i = 0; i < len_secret; i++) {
        display[i] = '_';
    }
    display[len_secret] = '\0';

    int remaining = len_secret;
    int wrong = 0;
    const int max_wrong = 6;
    uint8_t game_done = 1;

    printf("Welcome to Hangman!\n");

    while (game_done) {
        // Game loop: runs until win or loss
        while (remaining > 0 && wrong < max_wrong) {
            printf("Word: %s\n", display);
            printf("Guess a letter: ");

            char c;
            if (read(0, &c, 1) != 1) { continue; }
            if (c == '\n' || c == '\r') { continue; }

            int found = 0;
            for (int i = 0; i < len_secret; i++) {
                if (secret[i] == c && display[i] == '_') {
                    display[i] = c;
                    remaining--;
                    found = 1;
                }
            }
            if (!found) {
                wrong++;
                printf("Wrong guess (%d/%d)\n", wrong, max_wrong);
            }
        }

        // Print result
        if (remaining == 0) {
            printf("Congratulations! You guessed '%s'\n", secret);
        } else {
            printf("Game over. The word was '%s'\n", secret);
        }

        // Exit the outer loop
        game_done = 0;
    }

    // while(true);
    
    // while (true) {
    //     //     PS2::read_ps2_data();
    //     //     //Debug::printf("0X%x\n", PS2::read_ps2_data());
    //     // }
    

    // printf("*** %d\n",argc);
    // for (int i=0; i<argc; i++) {
    //     printf("*** %s\n",argv[i]);
    // }
    // int fd = open("/etc/data.txt",0);
    // one(fd);

    // printf("*** close = %d\n",close(fd));

    // one(fd);
    // one(100);


    // printf("*** open again %d\n",open("/etc/data.txt",0));
    // printf("*** seek %ld\n",seek(3,17));
    
    // int id = fork();

    // if (id < 0) {
    //     printf("fork failed");
    // } else if (id == 0) {
    //     /* child */
    //     printf("*** in child\n");
    //     int rc = execl("/sbin/shell","shell","a","b","c",0);
    //     printf("*** execl failed, rc = %d\n",rc);
    // } else {
    //     /* parent */
    //     uint32_t status = 42;
    //     wait(id,&status);
    //     printf("*** back from wait %ld\n",status);

    //     int fd = open("/etc/panic.txt",0);
    //     cp(fd,1);
    // }

    shutdown();
    return 0;
}
