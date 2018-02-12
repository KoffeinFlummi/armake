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

#include "sha1.h"
#include "args.h"
#include "filesystem.h"
#include "utils.h"
#include "keygen.h"
#include "sign.h"


void pad_hash(unsigned char *hash, char *buffer, size_t buffsize) {
    int i;

    buffer[0] = 0;
    buffer[1] = 1;

    for (i = 0; i < (buffsize - 38); i++)
        buffer[2 + i] = 255;

    memcpy(buffer + buffsize - 36,
        "\x00\x30\x21\x30\x09\x06\x05\x2b\x0e\x03\x02\x1a\x05\x00\x04\x14", 16);
    memcpy(buffer + buffsize - 20, hash, 20);
}


int sign_pbo(char *path_pbo, char *path_privatekey, char *path_signature) {
    SHA1Context sha;
    BN_CTX *bignum_context;
    BIGNUM *hash1_padded;
    BIGNUM *hash2_padded;
    BIGNUM *hash3_padded;
    BIGNUM *sig1;
    BIGNUM *sig2;
    BIGNUM *sig3;
    BIGNUM *exp;
    BIGNUM *modulus;
    bool nothing;
    long i;
    long fp_header;
    long fp_body;
    long fp_tmp;
    uint32_t temp;
    uint32_t keylength;
    uint32_t exponent_le;
    char buffer[4096];
    char prefix[512];
    char keyname[512];
    unsigned char hash1[20];
    unsigned char hash2[20];
    unsigned char hash3[20];
    unsigned char filehash[20];
    unsigned char namehash[20];
    FILE *f_pbo;
    FILE *f_privatekey;
    FILE *f_signature;

    f_pbo = fopen(path_pbo, "rb");
    if (!f_pbo)
        return 1;

    // get prefix
    if (fgetc(f_pbo) == 0) {
        fseek(f_pbo, 20, SEEK_CUR);
        do {
            fp_tmp = ftell(f_pbo);
            fread(buffer, sizeof(buffer), 1, f_pbo);
            if (strcmp(buffer, "prefix") == 0)
                strncpy(prefix, buffer + strlen(buffer) + 1, sizeof(prefix));
            fseek(f_pbo, fp_tmp + strlen(buffer) + 1, SEEK_SET);
        } while (strlen(buffer) > 0);
    } else {
        fseek(f_pbo, 0, SEEK_SET); // no header extensions
    }

    if (prefix[strlen(prefix) - 1] != '\\')
        strcat(prefix, "\\");

    fp_header = ftell(f_pbo);

    // calculate name hash
    SHA1Reset(&sha);
    do {
        fp_tmp = ftell(f_pbo);
        fread(buffer, sizeof(buffer), 1, f_pbo);
        lower_case(buffer);
        fseek(f_pbo, fp_tmp + strlen(buffer) + 17, SEEK_SET);
        fread(&temp, sizeof(temp), 1, f_pbo);
        if (temp > 0)
            SHA1Input(&sha, (unsigned char *)buffer, strlen(buffer));
    } while (strlen(buffer) > 0);

    if (!SHA1Result(&sha)) {
        fclose(f_pbo);
        return 1;
    }

    for (i = 0; i < 5; i++)
        reverse_endianness(&sha.Message_Digest[i], sizeof(sha.Message_Digest[i]));

    memcpy(namehash, &sha.Message_Digest[0], 20);

    fp_body = ftell(f_pbo);

    // calculate file hash
    SHA1Reset(&sha);
    nothing = true;
    while (true) {
        fseek(f_pbo, fp_header, SEEK_SET);
        fread(buffer, sizeof(buffer), 1, f_pbo);
        lower_case(buffer);
        fseek(f_pbo, fp_header + strlen(buffer) + 17, SEEK_SET);
        fread(&temp, sizeof(temp), 1, f_pbo);

        fp_header = ftell(f_pbo);

        if (strlen(buffer) == 0)
            break;

        if (temp == 0)
            continue;

        if (strchr(buffer, '.') != NULL && (
                strcmp(strrchr(buffer, '.'), ".paa") == 0 ||
                strcmp(strrchr(buffer, '.'), ".jpg") == 0 ||
                strcmp(strrchr(buffer, '.'), ".p3d") == 0 ||
                strcmp(strrchr(buffer, '.'), ".tga") == 0 ||
                strcmp(strrchr(buffer, '.'), ".rvmat") == 0 ||
                strcmp(strrchr(buffer, '.'), ".lip") == 0 ||
                strcmp(strrchr(buffer, '.'), ".ogg") == 0 ||
                strcmp(strrchr(buffer, '.'), ".wss") == 0 ||
                strcmp(strrchr(buffer, '.'), ".png") == 0 ||
                strcmp(strrchr(buffer, '.'), ".rtm") == 0 ||
                strcmp(strrchr(buffer, '.'), ".pac") == 0 ||
                strcmp(strrchr(buffer, '.'), ".fxy") == 0 ||
                strcmp(strrchr(buffer, '.'), ".wrp") == 0)) {
            fp_body += temp;
            continue;
        }

        nothing = false;

        fseek(f_pbo, fp_body, SEEK_SET);

        for (i = 0; temp - i >= sizeof(buffer); i += sizeof(buffer)) {
            fread(buffer, sizeof(buffer), 1, f_pbo);
            SHA1Input(&sha, (unsigned char *)buffer, sizeof(buffer));
        }
        fread(buffer, temp - i, 1, f_pbo);
        SHA1Input(&sha, (unsigned char *)buffer, temp - i);

        fp_body += temp;
    }

    if (nothing)
        SHA1Input(&sha, (unsigned char *)"nothing", strlen("nothing"));

    fseek(f_pbo, 0, SEEK_END);

    if (!SHA1Result(&sha)) {
        fclose(f_pbo);
        return 1;
    }


    for (i = 0; i < 5; i++)
        reverse_endianness(&sha.Message_Digest[i], sizeof(sha.Message_Digest[i]));

    memcpy(filehash, &sha.Message_Digest[0], 20);

    // get hash 1
    fseek(f_pbo, -20, SEEK_END);
    fread(hash1, 20, 1, f_pbo);

    // calculate hash 2
    SHA1Reset(&sha);
    SHA1Input(&sha, hash1, 20);
    SHA1Input(&sha, namehash, 20);
    if (strlen(prefix) > 1)
        SHA1Input(&sha, (unsigned char *)prefix, strlen(prefix));

    if (!SHA1Result(&sha)) {
        fclose(f_pbo);
        return 1;
    }

    for (i = 0; i < 5; i++)
        reverse_endianness(&sha.Message_Digest[i], sizeof(sha.Message_Digest[i]));

    memcpy(hash2, &sha.Message_Digest[0], 20);

    // calculate hash 3
    SHA1Reset(&sha);
    SHA1Input(&sha, filehash, 20);
    SHA1Input(&sha, namehash, 20);
    if (strlen(prefix) > 1)
        SHA1Input(&sha, (unsigned char *)prefix, strlen(prefix));

    if (!SHA1Result(&sha)) {
        fclose(f_pbo);
        return 1;
    }

    for (i = 0; i < 5; i++)
        reverse_endianness(&sha.Message_Digest[i], sizeof(sha.Message_Digest[i]));

    memcpy(hash3, &sha.Message_Digest[0], 20);

    // read private key data
    f_privatekey = fopen(path_privatekey, "rb");
    if (!f_privatekey) {
        fclose(f_pbo);
        return 1;
    }

    fread(keyname, sizeof(keyname), 1, f_privatekey);
    fseek(f_privatekey, strlen(keyname) + 1, SEEK_SET);
    fseek(f_privatekey, 16, SEEK_CUR);
    fread(&keylength, sizeof(keylength), 1, f_privatekey);
    fread(&exponent_le, sizeof(exponent_le), 1, f_privatekey);

    fread(buffer, keylength / 8, 1, f_privatekey);
    reverse_endianness(buffer, keylength / 8);
    modulus = BN_new();
    BN_bin2bn((unsigned char *)buffer, keylength / 8, modulus);

    fseek(f_privatekey, (keylength / 16) * 5, SEEK_CUR);

    fread(buffer, keylength / 8, 1, f_privatekey);
    reverse_endianness(buffer, keylength / 8);
    exp = BN_new();
    BN_bin2bn((unsigned char *)buffer, keylength / 8, exp);

    // generate signature values
    pad_hash(hash1, buffer, keylength / 8);
    hash1_padded = BN_new();
    BN_bin2bn((unsigned char *)buffer, keylength / 8, hash1_padded);

    pad_hash(hash2, buffer, keylength / 8);
    hash2_padded = BN_new();
    BN_bin2bn((unsigned char *)buffer, keylength / 8, hash2_padded);

    pad_hash(hash3, buffer, keylength / 8);
    hash3_padded = BN_new();
    BN_bin2bn((unsigned char *)buffer, keylength / 8, hash3_padded);

    bignum_context = BN_CTX_new();

    sig1 = BN_new();
    BN_mod_exp(sig1, hash1_padded, exp, modulus, bignum_context);

    sig2 = BN_new();
    BN_mod_exp(sig2, hash2_padded, exp, modulus, bignum_context);

    sig3 = BN_new();
    BN_mod_exp(sig3, hash3_padded, exp, modulus, bignum_context);

    // write to file
    f_signature = fopen(path_signature, "wb");
    if (!f_signature) {
        fclose(f_privatekey);
        fclose(f_pbo);
        return 1;
    }

    fwrite(keyname, strlen(keyname) + 1, 1, f_signature);
    temp = keylength / 8 + 20;
    fwrite(&temp, sizeof(temp), 1, f_signature);
    fwrite("\x06\x02\x00\x00\x00\x24\x00\x00", 8, 1, f_signature);
    fwrite("RSA1", 4, 1, f_signature);
    fwrite(&keylength, sizeof(keylength), 1, f_signature);
    fwrite(&exponent_le, sizeof(exponent_le), 1, f_signature);

    custom_bn2lebinpad(modulus, (unsigned char *)buffer, keylength / 8);
    fwrite(buffer, keylength / 8, 1, f_signature);

    temp = keylength / 8;
    fwrite(&temp, sizeof(temp), 1, f_signature);

    custom_bn2lebinpad(sig1, (unsigned char *)buffer, keylength / 8);
    fwrite(buffer, keylength / 8, 1, f_signature);

    temp = 2;
    fwrite(&temp, sizeof(temp), 1, f_signature);

    temp = keylength / 8;
    fwrite(&temp, sizeof(temp), 1, f_signature);

    custom_bn2lebinpad(sig2, (unsigned char *)buffer, keylength / 8);
    fwrite(buffer, keylength / 8, 1, f_signature);

    temp = keylength / 8;
    fwrite(&temp, sizeof(temp), 1, f_signature);

    custom_bn2lebinpad(sig3, (unsigned char *)buffer, keylength / 8);
    fwrite(buffer, keylength / 8, 1, f_signature);

    // clean up
    BN_CTX_free(bignum_context);
    BN_free(exp);
    BN_free(modulus);
    BN_free(hash1_padded);
    BN_free(hash2_padded);
    BN_free(hash3_padded);
    BN_free(sig1);
    BN_free(sig2);
    BN_free(sig3);
    fclose(f_pbo);
    fclose(f_privatekey);
    fclose(f_signature);

    return 0;
}


int cmd_sign() {
    extern struct arguments args;
    char keyname[512];
    char path_signature[2048];
    int success;

    if (args.num_positionals != 3)
        return 128;

    if (strcmp(strrchr(args.positionals[1], '.'), ".biprivatekey") != 0) {
        errorf("File %s doesn't seem to be a valid private key.\n", args.positionals[1]);
        return 1;
    }

    if (strchr(args.positionals[1], PATHSEP) == NULL)
        strcpy(keyname, args.positionals[1]);
    else
        strcpy(keyname, strrchr(args.positionals[1], PATHSEP) + 1);
    *strrchr(keyname, '.') = 0;

    if (args.signature) {
        strcpy(path_signature, args.signature);
        if (strlen(path_signature) < 7 || strcmp(&path_signature[strlen(path_signature) - 7], ".bisign") != 0)
            strcat(path_signature, ".bisign");
    } else {
        strcpy(path_signature, args.positionals[2]);
        strcat(path_signature, ".");
        strcat(path_signature, keyname);
        strcat(path_signature, ".bisign");
    }

    // check if target already exists
    if (access(path_signature, F_OK) != -1 && !args.force) {
        errorf("File %s already exists and --force was not set.\n", path_signature);
        return 1;
    }

    success = sign_pbo(args.positionals[2], args.positionals[1], path_signature);

    if (success)
        errorf("Failed to sign file.\n");

    return success;
}
