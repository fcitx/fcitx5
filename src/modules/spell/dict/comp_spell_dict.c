/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#if defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#else
#include <sys/endian.h>
#endif
#include <string.h>

#define DICT_BIN_MAGIC "FSCD0000"
const char null_byte = '\0';

static int
compile_dict(int ifd, int ofd)
{
    struct stat istat_buf;
    uint32_t wcount = 0;
    char *p;
    char *ifend;
    if (fstat(ifd, &istat_buf) == -1)
        return 1;
    p = mmap(NULL, istat_buf.st_size + 1, PROT_READ, MAP_PRIVATE, ifd, 0);
    ifend = istat_buf.st_size + p;
    close(ifd);
    write(ofd, DICT_BIN_MAGIC, strlen(DICT_BIN_MAGIC));
    lseek(ofd, sizeof(uint32_t), SEEK_CUR);
    while (p < ifend) {
        char *start;
        long int ceff;
        uint16_t ceff_buff;
        ceff = strtol(p, &p, 10);
        if (*p != ' ')
            return 1;
        ceff_buff = htole16(ceff > UINT16_MAX ? UINT16_MAX : ceff);
        write(ofd, &ceff_buff, sizeof(uint16_t));
        start = ++p;
        p += strcspn(p, "\n");
        write(ofd, start, p - start);
        write(ofd, &null_byte, 1);
        wcount++;
        p++;
    }
    lseek(ofd, strlen(DICT_BIN_MAGIC), SEEK_SET);
    wcount = htole32(wcount);
    write(ofd, &wcount, sizeof(uint32_t));
    close(ofd);
    return 0;
}


int
main(int argc, char *argv[])
{
    int ifd;
    int ofd;
    const char *action = argv[1];
    if (strcmp(action, "--comp-dict") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Wrong number of arguments.\n");
            exit(1);
        }
        ifd = open(argv[2], O_RDONLY);
        ofd = open(argv[3], O_WRONLY | O_TRUNC | O_CREAT, 0644);
        if (ifd < 0 || ofd < 0)
            return 1;
        return compile_dict(ifd, ofd);
    }
    return 1;
}
