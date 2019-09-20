/**
 * Copyright (C) 2019 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
  * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include <malloc.h>
#include "snappy_boot_v1.h"

static uint32_t crc32(uint32_t crc, unsigned char *buf, size_t len)
{
    int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
    }
    return ~crc;
}

/**
 * Program to operare snap bootsel environment
 */
void print_usage() {
    printf("usage:\n"
           "\t-r, --read==<file name>\n"
           "\t\tread content of passed boot env\n"
           "\t\tadditional passed parameters are ignored\n\n"
           "\t-w, --write=<file name>\n"
           "\t\twrite new clean boot env to the file (file is created if it does not exists\n"
           "\t-u, --update=<file name>\n\n"
           "\t\tupdate existing boot env, file has to exists\n\n"
           "\tOptional parameters:\n"
           "\t\t[-a, --snap-mode=<snap mode>]\n"
           "\t\t[-b, --snap-kernel=<kernel-snap revision>]\n"
           "\t\t[-c, --snap-try-kernel=<kernel-snap revision>]\n"
           "\t\t[-d, --snap-core=<core-snap revision>]\n"
           "\t\t[-e, --snap-try-core=<core-snap revision>]\n"
           "\t\t[-f, --reboot-reason=<reboot reason>\n"
           "\t\t[-g, --boot-0-part=<boot image 0 part name>]\n"
           "\t\t[-i, --boot-1-part=<boot image 1 part name>]\n"
           "\t\t[-j, --boot-0-snap=<installed kernel snap revision>]\n"
           "\t\t[-k, --boot-1-snap=<installed kernel snap revision>]\n"
           "\t\t[-l, --bootimg-file=<name of bootimg in kernel snap>]\n"
           "\t\t[-m, --gadget-0-part=<gadget image 0 part name>]\n"
           "\t\t[-n, --gadget-1-part=<gadget image 1 part name>]\n"
           "\t\t[-o, --gadget-0-snap=<installed gadget snap revision>]\n"
           "\t\t[-p, --gadget-1-snap=<installed gadget snap revision>]\n"
           "\t\t[-q, --gadget-mode=<gadget mode>]\n"
           "\t\t[-s, --snap-gadget=<gadget snap revision>]\n"
           "\t\t[-t, --snap-try-gadget=<gadget snap revision>]\n"
           "\n");
}

// Open env file, exit if it fails
FILE* open_file(const char *filename, const char *mode, const char *arg) {
    FILE *fp = NULL;
    if(!optarg) {
        printf( "Missing file name argument for %s option\n", arg );
        print_usage();
        exit(EXIT_FAILURE);
    }
    fp = fopen(filename, mode);
    if (!fp) {  /* validate file open for reading */
        fprintf (stderr, "error: file open failed '%s'.\n", filename);
        exit(EXIT_FAILURE);
    }
    return fp;
}

// read and validate environment from passed file
SNAP_BOOT_SELECTION_t* read_validate_environment(FILE *fp) {
    SNAP_BOOT_SELECTION_t *bootSel;
    bootSel = (SNAP_BOOT_SELECTION_t *)malloc(sizeof(SNAP_BOOT_SELECTION_t));
    if (!bootSel) {
        fprintf (stderr, "error: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    fread(bootSel, 1, sizeof(SNAP_BOOT_SELECTION_t), fp);
    if (bootSel->crc32 != crc32(0x0, (void *)bootSel, sizeof(SNAP_BOOT_SELECTION_t)-sizeof(uint32_t))) {
        printf("BROKEN crc32!!!!! [0x%X] vs [0x%X]\n", bootSel->crc32, crc32(0x0, (void *)bootSel, sizeof(SNAP_BOOT_SELECTION_t)-sizeof(uint32_t)));
        free(bootSel);
        exit(EXIT_FAILURE);
    }
    // check compulsory values of boot env structure
    if (bootSel->signature != SNAP_BOOTSELECT_SIGNATURE){
        fprintf (stderr, "error: Missing or wrong SIGNATURE value in environment\n");
        free(bootSel);
        exit(EXIT_FAILURE);
    }
    if (bootSel->version != SNAP_BOOTSELECT_VERSION) {
        fprintf (stderr, "error: Missing or wrong VERSION value in environment\n");
        free(bootSel);
        exit(EXIT_FAILURE);
    }
    printf("crc32 validated [0x%X]\n", bootSel->crc32);
    return bootSel;
}

// cleane clean boot select environment
SNAP_BOOT_SELECTION_t* create_clean_boot_environment() {
    SNAP_BOOT_SELECTION_t *bootSel;
    bootSel = (SNAP_BOOT_SELECTION_t *)malloc(sizeof(SNAP_BOOT_SELECTION_t));
    if (!bootSel) {
        fprintf (stderr, "error: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    memset(bootSel, 0, sizeof(SNAP_BOOT_SELECTION_t));
    // fill compulsory values of boot env structure
    bootSel->signature=SNAP_BOOTSELECT_SIGNATURE;
    bootSel->version=SNAP_BOOTSELECT_VERSION;
    return bootSel;
}

void print_keys_value(const char *key, const char *value) {
    if (strlen(value)) {
      printf("%s [%s]\n", key, value);
    }
}

// print environment values
void print_environment(const SNAP_BOOT_SELECTION_t *bootSel) {
    printf("signature: [0x%X]\n", bootSel->signature);
    printf("version:   [0x%X]\n", bootSel->version);
    // only show populated values
    print_keys_value("snap_mode", bootSel->snap_mode);
    print_keys_value("snap_kernel", bootSel->snap_kernel);
    print_keys_value("snap_core", bootSel->snap_core);
    print_keys_value("snap_try_kernel", bootSel->snap_try_kernel);
    print_keys_value("snap_try_core", bootSel->snap_try_core);
    print_keys_value("reboot_reason", bootSel->reboot_reason);
    printf("bootimg_matrix [%s][%s]\n", bootSel->bootimg_matrix[0][0], bootSel->bootimg_matrix[0][1]);
    printf("bootimg_matrix [%s][%s]\n", bootSel->bootimg_matrix[1][0], bootSel->bootimg_matrix[1][1]);
    print_keys_value("bootimg_file_name", bootSel->bootimg_file_name);
    print_keys_value("gadget_mode", bootSel->gadget_mode);
    print_keys_value("snap_gadget", bootSel->snap_gadget);
    print_keys_value("snap_try_gadget", bootSel->snap_try_gadget);
    printf("gadget_asset_matrix [%s][%s]\n", bootSel->gadget_asset_matrix[0][0], bootSel->gadget_asset_matrix[0][1]);
    printf("gadget_asset_matrix [%s][%s]\n", bootSel->gadget_asset_matrix[1][0], bootSel->gadget_asset_matrix[1][1]);
}

int main (int argc, char **argv) {
    SNAP_BOOT_SELECTION_t *bootSel;
    FILE *fp = NULL;
    int opt= 0;
    size_t bwriten =0;
    static struct option long_options[] = {
        // long name      | has argument  | flag | short value
        {"write",           required_argument, 0, 'w'},
        {"read",            required_argument, 0, 'r'},
        {"update",          required_argument, 0, 'u'},
        {"help",            no_argument,       0, 'h'},
        {"snap-mode",       required_argument, 0, 'a'},
        {"snap-kernel",     required_argument, 0, 'b'},
        {"snap-try-kernel", required_argument, 0, 'c'},
        {"snap-core",       required_argument, 0, 'd'},
        {"snap-try-core",   required_argument, 0, 'e'},
        {"reboot-reason",   required_argument, 0, 'f'},
        {"boot-0-part",     required_argument, 0, 'g'},
        {"boot-1-part",     required_argument, 0, 'i'},
        {"boot-0-snap",     required_argument, 0, 'j'},
        {"boot-1-snap",     required_argument, 0, 'k'},
        {"bootimg-file",    required_argument, 0, 'l'},
        {"gadget-0-part",   required_argument, 0, 'm'},
        {"gadget-1-part",   required_argument, 0, 'n'},
        {"gadget-0-snap",   required_argument, 0, 'o'},
        {"gadget-1-snap",   required_argument, 0, 'p'},
        {"gadget-mode",     required_argument, 0, 'q'},
        {"snap-gadget",     required_argument, 0, 's'},
        {"snap-try-gadget", required_argument, 0, 't'},
        {0, 0, 0, 0 }
    };

    int long_index =0;
    if (argc <= 1) {
        printf( "Missing compulsory arguments\n");
        print_usage();
        exit(EXIT_FAILURE);
    }
    while ((opt = getopt_long(argc, argv, "w:r:u:ha:b:c:d:e:f:g:i:j:k:l:m:n:o:p:q:s:t:",
                   long_options, &long_index )) != -1) {
        switch (opt) {
          case 'w' :
              printf( "running in write mode\n");
              fp = open_file( optarg, "w", "write");
              // prepare empty boot_env structure
              bootSel = create_clean_boot_environment();
              break;
          case 'r' :
              printf( "running in read mode\n");
              fp = open_file( optarg, "r", "read");
              bootSel = read_validate_environment(fp);
              print_environment(bootSel);
              free(bootSel);
              fclose(fp);
              exit(EXIT_SUCCESS);
              break;
          case 'u' :
              printf( "running in updated mode\n");
              fp = open_file( optarg, "r", "update");
              bootSel = read_validate_environment(fp);
              fclose(fp);
              fp = open_file( optarg, "w", "update");
              break;
          case 'a' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->snap_mode, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'b' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->snap_kernel, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'c' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->snap_try_kernel, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'd' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->snap_core, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'e' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->snap_try_core, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'f' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->reboot_reason, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'g' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->bootimg_matrix[0][0], optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'i' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->bootimg_matrix[1][0], optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'j' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->bootimg_matrix[0][1], optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'k' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->bootimg_matrix[1][1], optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'l' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->bootimg_file_name, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'm' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->gadget_asset_matrix[0][0], optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'n' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->gadget_asset_matrix[1][0], optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'o' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->gadget_asset_matrix[1][0], optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'p' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->gadget_asset_matrix[1][1], optarg, SNAP_NAME_MAX_LEN);
              break;
          case 'q' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->gadget_mode, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 's' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->snap_gadget, optarg, SNAP_NAME_MAX_LEN);
              break;
          case 't' :
              if(!optarg) {
                  printf( "Missing argument for %c option\n", opt);
                  print_usage();
                  exit(EXIT_FAILURE);
              }
              strncpy(bootSel->snap_try_gadget, optarg, SNAP_NAME_MAX_LEN);
              break;        
          break;
          case 'h' :
          default:
              print_usage();
              exit(EXIT_FAILURE);
        }
    }
    bootSel->crc32 = crc32(0x0, (void *)bootSel, sizeof(SNAP_BOOT_SELECTION_t)-sizeof(uint32_t));
    printf( "\nNew calculated crc32 is [0x%X]\n", bootSel->crc32);
    printf( "SIGNATURE [0x%X]\n", bootSel->signature);
    printf( "VERSION [0x%X]\n", bootSel->version);
    // save structure
    // bwriten = fwrite(bootSel, sizeof(SNAP_BOOT_SELECTION_t), 1, fp);
    bwriten = fwrite(bootSel, 1, sizeof(SNAP_BOOT_SELECTION_t), fp);
    printf( "Written %ld bytes\n", bwriten);
    fclose(fp);
    free(bootSel);
    return 0;
}

