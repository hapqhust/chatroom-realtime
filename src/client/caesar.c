#include <stdio.h>
#include <string.h>

#define ALPHABET_SIZE 26

/* Function to shift a letter by a given key */
char shift_letter(char letter, int key)
{
    /* Ensure letter is lowercase */
    letter = tolower(letter);
    /* Shift letter by key, wrapping around the alphabet */
    letter = ((letter - 'a' + key) % ALPHABET_SIZE) + 'a';
    return letter;
}

/* Function to encrypt a message using the Caesar Cipher */
void encrypt_caesar(char message[], int key)
{
    int length = strlen(message);
    for (int i = 0; i < length; i++)
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
    for (int i = 0; i < length; i++)
    {
        if (isalpha(message[i]))
        {
            message[i] = shift_letter(message[i], ALPHABET_SIZE - key);
        }
    }
}
