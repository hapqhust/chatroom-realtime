#ifndef CAESAR
#define CAESAR

/* Function to shift a letter by a given key */
char shift_letter(char letter, int key);

/* Function to encrypt a message using the Caesar Cipher */
void encrypt_caesar(char message[], int key);

/* Function to decrypt a message using the Caesar Cipher */
void decrypt_caesar(char message[], int key);

#endif // CAESAR