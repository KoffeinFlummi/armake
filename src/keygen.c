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
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>

#include "args.h"
#include "filesystem.h"
#include "utils.h"
#include "keygen.h"


#if OPENSSL_VERSION_NUMBER < 0x10100000L || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x2070000fL)
void RSA_get0_key(const RSA *r,
        const BIGNUM **n, const BIGNUM **e, const BIGNUM **d) {
    if (n != NULL)
        *n = r->n;
    if (e != NULL)
        *e = r->e;
    if (d != NULL)
        *d = r->d;
}

void RSA_get0_factors(const RSA *r, const BIGNUM **p, const BIGNUM **q) {
    if (p != NULL)
        *p = r->p;
    if (q != NULL)
        *q = r->q;
}

void RSA_get0_crt_params(const RSA *r,
        const BIGNUM **dmp1, const BIGNUM **dmq1, const BIGNUM **iqmp) {
    if (dmp1 != NULL)
        *dmp1 = r->dmp1;
    if (dmq1 != NULL)
        *dmq1 = r->dmq1;
    if (iqmp != NULL)
        *iqmp = r->iqmp;
}
#endif

int custom_bn2lebinpad(const BIGNUM *a, unsigned char *to, int tolen) {
    int len;
    int result;
    char *hexstring;

    hexstring = BN_bn2hex(a);

    len = strlen(hexstring) / 2;

    if (len < tolen)
        memset(to, 0, tolen - len);

    result = BN_bn2bin(a, to + (tolen - len));
    reverse_endianness(to, tolen);

    free(hexstring);

    return result;
}


int generate_keypair(char *name, char *path_private, char *path_public) {
    /*
     * Generates a BI key pair (.bikey and .biprivatekey) for the given paths
     * with the given name. Existing files are overwritten. Returns 0 on
     * success and a positive integer on failure.
     */

    RSA *rsa;
    BIGNUM *exponent;
    const BIGNUM *n, *p, *q, *dmp1, *dmq1, *iqmp, *d;
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
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
    do {
        RAND_poll();
    } while (RAND_status() != 1);
#else
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

    RSA_get0_key(rsa, &n, NULL, &d);
    RSA_get0_factors(rsa, &p, &q);
    RSA_get0_crt_params(rsa, &dmp1, &dmq1, &iqmp);

    custom_bn2lebinpad(n, buffer, length / 8);
    fwrite(buffer, length / 8, 1, f_private);

    custom_bn2lebinpad(p, buffer, length / 8);
    fwrite(buffer, length / 16, 1, f_private);

    custom_bn2lebinpad(q, buffer, length / 8);
    fwrite(buffer, length / 16, 1, f_private);

    custom_bn2lebinpad(dmp1, buffer, length / 8);
    fwrite(buffer, length / 16, 1, f_private);

    custom_bn2lebinpad(dmq1, buffer, length / 8);
    fwrite(buffer, length / 16, 1, f_private);

    custom_bn2lebinpad(iqmp, buffer, length / 8);
    fwrite(buffer, length / 16, 1, f_private);

    custom_bn2lebinpad(d, buffer, length / 8);
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

    custom_bn2lebinpad(n, buffer, length / 8);
    fwrite(buffer, length / 8, 1, f_public);

    fclose(f_public);
    
    // clean up
    BN_free(exponent);
    RSA_free(rsa);

    CRYPTO_cleanup_all_ex_data();

    return 0;
}


int cmd_keygen() {
    extern struct arguments args;
    char name[512];
    char path_private[2048];
    char path_public[2048];
    int success;

    if (args.num_positionals != 2)
        return 128;

    if (strrchr(args.positionals[1], PATHSEP) == NULL)
        strcpy(name, args.positionals[1]);
    else
        strcpy(name, strrchr(args.positionals[1], PATHSEP) + 1);

    strcpy(path_private, args.positionals[1]);
    strcat(path_private, ".biprivatekey");

    strcpy(path_public, args.positionals[1]);
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
