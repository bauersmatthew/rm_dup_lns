/*
 * File: rm_dup_lns.cpp
 * Edited: 15 Apr 2016
 * Author: Matthew Bauer
 */

#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <cstdio>

const uint16_t F_BUFF_SZ = 16*1024;

void print_usage(std::ostream& out)
{
    out << "Usage: rm_dup_lns <-h/--help OR file> [> dest]" << std::endl;
}

uint64_t hash_str(char *str)
{
    uint64_t hash = 5381;
    int32_t c;
    while(c = *str++)
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

int main(int argc, char **argv)
{
    std::set<uint64_t> ln_hashes;

    // check args
    if(argc != 2)
    {
        std::cerr << "E: bad arg count" << std::endl;
        print_usage(std::cerr);
        return -1;
    }
    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))
    {
        print_usage(std::cerr);
        return 0;
    }
    
    // open file
    int fd = open(argv[1], O_RDONLY);
    if(fd == -1)
    {
        std::cerr << "E: could not open file" << std::endl;
        return -2;
    }
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL); // hopefully this speeds things up
    char buf[F_BUFF_SZ + 1];
    size_t leftover_sz = 0;
    
    // read through all bytes in the file, in buffered chunks
    while(uint16_t bytes_read = read(fd, buf + leftover_sz, F_BUFF_SZ - leftover_sz))
    {
        if(bytes_read == -1) // ==> error
        {
            std::cerr << "E: could not read file" << std::endl;
            close(fd);
            return -3;
        }
        if(!bytes_read) // ==> eof
        {
            break;
        }
        
        bytes_read += leftover_sz; // this works for all intents and purposes

        // go through buffer, processing newlines
        char *p_f, *p_s;
        for(p_f = p_s = buf; (p_s = (char*)memchr(p_s, '\n', (buf + bytes_read) - p_s)); )
        {
            *p_s = 0; // put null terminator over \n
            uint64_t hash = hash_str(p_f);
            if(ln_hashes.find(hash) == ln_hashes.end()) // sig. faster than std::find!
            {
                // not found (good)
                // spit line back out, save the hash as 'seen'
                puts(p_f); // puts writes a \n automatically
                ln_hashes.insert(hash);
            }
            p_f = ++p_s; // p_f, p_s -> the first char on next line
        }
        // save leftovers
        leftover_sz = buf + bytes_read - p_f;
        memcpy(buf, p_f, leftover_sz);
    }
    close(fd);
    if(leftover_sz != 0)
    {
        // file didn't end with \n
        buf[leftover_sz] = 0; // place null terminator
        if(ln_hashes.find(hash_str(buf)) == ln_hashes.end())
        {
            fputs(buf, stdout); // no new line, preserve the user choice of no \n
        }
    }
    
    return 0;
}
