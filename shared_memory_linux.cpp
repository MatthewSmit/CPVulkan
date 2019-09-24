#include <iostream>

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cassert>

int main() {
    auto fd = shm_open("/cpvulkan", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    assert(fd != -1);

    if (ftruncate(fd, 1024) == -1)
        assert(false);

//    auto pid = getpid();
//    char path[1024];
//    sprintf(path, "/proc/%d/mem", pid);
//
//    std::cout << path << std::endl;

    auto original = mmap(nullptr, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    std::cout << errno << std::endl;
    assert(original != (void *)-1);
    auto mappedBacking = mmap(nullptr, 1024, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    assert(mappedBacking != (void *)-1);
    std::cout << errno << std::endl;

//    auto fd = open(path, O_DSYNC | O_RDWR);
//    assert(fd);

    auto tmp = mmap(mappedBacking, 1024, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);
    std::cout << errno << std::endl;
    assert(tmp != (void *)-1);

    *(uint64_t*)original = 0xABCDEF;
    std::cout << std::hex << *(uint64_t*)original << std::endl;
    std::cout << std::hex << *(uint64_t*)mappedBacking << std::endl;

    std::cout << std::hex << (uint64_t)original << std::endl;
    std::cout << std::hex << (uint64_t)mappedBacking << std::endl;

    close(fd);
    shm_unlink("/cpvulkan");
    munmap(mappedBacking, 1024);
    munmap(original, 1024);

    return 0;
}