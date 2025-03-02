#include "thread_pool.hpp"
#include <iostream>
#include <fstream>
#include <unordered_set>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <math.h>

#define CHUNK_SIZE     64 * 1024 * 1024
#define THREDS         16
#define SUBCHUNK_COUNT THREDS

std::mutex set_mutex;
std::unordered_set<std::string> global_set; // global words set

void Parser(std::string chunk, std::string last_word)
{
    std::string word;
    if(last_word.size() > 0)
    {
        word = last_word;
    }
    for (size_t i = 0; i < chunk.size(); ++i)
    {
        char c = chunk[i];
        if (c >= 'a' && c <= 'z')
        {
            word += c;
        }
        else if (!word.empty())
        {
            std::lock_guard<std::mutex> lock(set_mutex);
            global_set.insert(word);
            word.clear();
        }
    }
}

void SplitChunks(const char* filename)
{
    std::string prev_word;
    std::string last_word;

    int fd = open(filename, O_RDONLY);

    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    {
        ThreadPool tpool(THREDS);

        // Get file size
        off_t file_size = lseek(fd, 0, SEEK_END);
        if (file_size == -1) {
            perror("Error getting file size");
            close(fd);
            return;
        }

        // Process the file in chunks
        for (size_t offset = 0; offset < (size_t)file_size; offset += CHUNK_SIZE)
        {
            // std::cout << offset << "/" << file_size << std::endl;
            size_t map_size = std::min((size_t)CHUNK_SIZE, (size_t)file_size - offset);

            char* chunk = static_cast<char*>(mmap(nullptr, map_size, PROT_READ, MAP_PRIVATE, fd, offset));
            size_t sub_chunk_size = map_size / SUBCHUNK_COUNT;

            for (size_t i = 0; i < SUBCHUNK_COUNT; ++i)
            {
                const char* start = chunk + i * sub_chunk_size;

                size_t size = (i == SUBCHUNK_COUNT - 1) ? (map_size - i * sub_chunk_size) : sub_chunk_size;
                const char* end = start + size-1;
                std::string sub_chunk(start, size);


                // wanna keep last word to pass into next calculation
                prev_word = "";
                while(end != start && *end != ' ')
                {
                    prev_word = *end + prev_word;
                    --end;
                }
                
                // pass function call into thread pool
                // don't use futures as it doesn't return anything
                tpool.enqueue(Parser, std::move(sub_chunk), std::move(last_word));
                last_word = std::move(prev_word);
                
            }

            // keep the last word between chunks
            last_word = "";
            const char* end = chunk + map_size - 1;
            const char* start = chunk;
            while(end != start && *end != ' ')
            {
                last_word = *end + last_word;
                --end;
            }

            tpool.WaitForCompletion();
            munmap(chunk, map_size);
        }
    }
    close(fd);

    global_set.insert(last_word); // very last word
}


int main(int argc, char* argv[]) {
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }
    SplitChunks(argv[1]);
    std::cout << global_set.size() << std::endl;

    return 0;
}