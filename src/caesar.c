#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include"caesar.h"

#define ALPHABET_SIZE 26

/* Function to shift a letter by a given key */
char shift_letter(char letter, int key)
{
    /* Check letter is upper? 
     * Then shift letter by key, wrapping around the alphabet
     */
    if (isupper(letter)) {
        letter = ((letter - 'A' + key) % ALPHABET_SIZE) + 'A';
    } else {
        letter = ((letter - 'a' + key) % ALPHABET_SIZE) + 'a';
    }
    
    return letter;
}

/* Function to encrypt a message using the Caesar Cipher */
void encrypt_caesar(char message[], int key)
{
    int length = strlen(message);
    int i;
    for (i = 0; i < length; i++)
    {
        if (isalpha(message[i]))
        {
            message[i] = shift_letter(message[i], key);
        }
    }
}

/* Function to decrypt a message using the Caesar Cipher */
void decrypt_caesar(char message[], int key)
{
    int length = strlen(message);
    int i;
    for (i = 0; i < length; i++)
    {
        if (isalpha(message[i]))
        {
            message[i] = shift_letter(message[i], ALPHABET_SIZE - key);
        }
    }
}
