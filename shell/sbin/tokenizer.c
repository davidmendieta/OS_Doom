#define MAX_ARG_LEN 100
#include "libc.h"
#include "tokenizer.h"

int increase_index_and_copy_into_temp_array(char *temp_array, char *from_array, int index, int *strlen) {
    if (index == '\0') {
        return -1;
    }
    int temp_array_index = 0;
    char curr_char = from_array[index];
    while (curr_char != '\0' && curr_char != ' ') { // while it doesn't equal null or space
        temp_array[temp_array_index] = from_array[index];
        index++;
        temp_array_index++;
        curr_char = from_array[index];
    }
    *strlen = temp_array_index;
    if (curr_char == '\0') {
        return -2; // end of input
    }
    return index + 1; // go past the bad char
}

Tokenized *tokenize(char *input)
{
    if (input[0] == 0)
    {
        // empty line, return
        return 0;
    }
    Tokenized *token = (Tokenized *) malloc(sizeof(Tokenized));
    token->argv = (char **) malloc(sizeof(char *) * MAX_ARG_LEN);
    token->argc = 0;
    char *temp_array = (char *) malloc(MAX_ARG_LEN);
    int index = 0;
    int str_len = 0;
    index = increase_index_and_copy_into_temp_array(temp_array, input, index, &str_len);

    while (index != -2) {
        //temp_array contains the command
        token->argv[token->argc] = malloc(str_len + 1); // for null terminated
        token->argv[token->argc][str_len] = '\0';
        memcpy((void *) token->argv[token->argc], (void *)temp_array, str_len);
        token->argc++;
        index = increase_index_and_copy_into_temp_array(temp_array, input, index, &str_len);
    }

    return token;
}