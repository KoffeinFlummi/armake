/*
 * Copyright (C)  2016  Felix "KoffeinFlummi" Wiegand
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"
#include "keygen.h"


int generate_keypair(char *name, char *path_private, char *path_public) {
    /*
     * Generates a BI key pair (.bikey and .biprivatekey) for the given paths
     * with the given name. Existing files are overwritten. Returns 0 on
     * success and a positive integer on failure.
     */

    RSA *rsa;
    BIGNUM *exponent;
    uint32_t exponent_le;
    uint32_t exponent_be;
    uint32_t length;
    uint32_t temp;
    unsigned char buffer[4096];
    FILE *f_private;
    FILE *f_public;

    length = KEY_LENGTH;

    // convert exponent to bignum
    exponent_le = KEY_EXPONENT;
    exponent_be = exponent_le;
    reverse_endianness(&exponent_be, sizeof(exponent_be));
    exponent = BN_bin2bn((unsigned char *)&exponent_be, sizeof(exponent_be), NULL);

    // seed PRNG
#ifdef _WIN32
    do {
        RAND_screen();
    } while (RAND_status() != 1);
#else
    char rand_buffer[256];
    FILE *f_random;
    f_random = fopen("/dev/urandom", "rb");
    do {
        fread(rand_buffer, sizeof(rand_buffer), 1, f_random);
        RAND_seed(rand_buffer, sizeof(rand_buffer));
    } while (RAND_status() != 1);
    fclose(f_random);
#endif

    // generate keypair
    rsa = RSA_new();
    if (!RSA_generate_key_ex(rsa, length, exponent, NULL))
        return 1;

    // write privatekey
    f_private = fopen(path_private, "wb");
    if (!f_private)
        return 2;

    fwrite(name, strlen(name) + 1, 1, f_private);
    temp = length / 16 * 9 + 20;
    fwrite(&temp, sizeof(temp), 1, f_private);
    fwrite("\x07\x02\x00\x00\x00\x24\x00\x00", 8, 1, f_private);
    fwrite("RSA2", 4, 1, f_private);
    fwrite(&length, sizeof(length), 1, f_private);
    fwrite(&exponent_le, sizeof(exponent_le), 1, f_private);

    BN_bn2bin(rsa->n, buffer);
    reverse_endianness(buffer, length / 8);
    fwrite(buffer, length / 8, 1, f_private);

    BN_bn2bin(rsa->p, buffer);
    reverse_endianness(buffer, length / 16);
    fwrite(buffer, length / 16, 1, f_private);

    BN_bn2bin(rsa->q, buffer);
    reverse_endianness(buffer, length / 16);
    fwrite(buffer, length / 16, 1, f_private);

    BN_bn2bin(rsa->dmp1, buffer);
    reverse_endianness(buffer, length / 16);
    fwrite(buffer, length / 16, 1, f_private);

    BN_bn2bin(rsa->dmq1, buffer);
    reverse_endianness(buffer, length / 16);
    fwrite(buffer, length / 16, 1, f_private);

    BN_bn2bin(rsa->iqmp, buffer);
    reverse_endianness(buffer, length / 16);
    fwrite(buffer, length / 16, 1, f_private);

    BN_bn2bin(rsa->d, buffer);
    reverse_endianness(buffer, length / 8);
    fwrite(buffer, length / 8, 1, f_private);

    fclose(f_private);

    // write publickey
    f_public = fopen(path_public, "wb");
    if (!f_public)
        return 2;

    fwrite(name, strlen(name) + 1, 1, f_public);
    temp = length / 8 + 20;
    fwrite(&temp, sizeof(temp), 1, f_public);
    fwrite("\x06\x02\x00\x00\x00\x24\x00\x00", 8, 1, f_public);
    fwrite("RSA1", 4, 1, f_public);
    fwrite(&length, sizeof(length), 1, f_public);
    fwrite(&exponent_le, sizeof(exponent_le), 1, f_public);

    BN_bn2bin(rsa->n, buffer);
    reverse_endianness(buffer, length / 8);
    fwrite(buffer, length / 8, 1, f_public);

    fclose(f_public);
    
    // clean up
    BN_free(exponent);
    RSA_free(rsa);

    return 0;
}


int keygen() {
    extern DocoptArgs args;
    char name[512];
    char path_private[2048];
    char path_public[2048];
    int success;

    if (strchr(args.target, PATHSEP) == NULL)
        strcpy(name, args.target);
    else
        strcpy(name, strchr(args.target, PATHSEP) + 1);

    strcpy(path_private, args.target);
    strcat(path_private, ".biprivatekey");

    strcpy(path_public, args.target);
    strcat(path_public, ".bikey");

    // check if target already exists
    if (access(path_private, F_OK) != -1 && !args.force) {
        errorf("File %s already exists and --force was not set.\n", path_private);
        return 1;
    }
    if (access(path_public, F_OK) != -1 && !args.force) {
        errorf("File %s already exists and --force was not set.\n", path_public);
        return 1;
    }

    success = generate_keypair(name, path_private, path_public);

    if (success)
        errorf("Failed to generate key pair.\n");

    return success;
}
