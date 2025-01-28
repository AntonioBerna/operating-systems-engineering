#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <stdexcept>

static size_t iterations;

struct ThreadData {
    std::thread thread;
    void (*start_routine)(size_t);
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
};

constexpr size_t NUM_THREADS = 2;
static ThreadData threads[NUM_THREADS];

static void ping(size_t id) {
    size_t next = (id + 1) % NUM_THREADS;

    for (size_t i = 0; i < iterations;) {
        std::unique_lock<std::mutex> lock(threads[id].mtx);
        threads[id].cv.wait(lock, [&] { return threads[id].ready; });
        threads[id].ready = false;
        std::cout << "(" << ++i << ") ping\n";
        {
            std::lock_guard<std::mutex> lock_next(threads[next].mtx);
            threads[next].ready = true;
        }
        threads[next].cv.notify_one();
    }
}

static void pong(size_t id) {
    size_t next = (id + 1) % NUM_THREADS;

    for (size_t i = 0; i < iterations;) {
        std::unique_lock<std::mutex> lock(threads[id].mtx);
        threads[id].cv.wait(lock, [&] { return threads[id].ready; });
        threads[id].ready = false;
        std::cout << "(" << ++i << ") pong\n";
        {
            std::lock_guard<std::mutex> lock_next(threads[next].mtx);
            threads[next].ready = true;
        }
        threads[next].cv.notify_one();
    }
}

static bool to_size_t(const char *buffer, size_t *value) {
    char *endptr;
    errno = 0;
    *value = static_cast<size_t>(strtol(buffer, &endptr, 0));
    return !(errno == ERANGE || endptr == buffer || (*endptr && *endptr != '\n') || *value == SIZE_MAX);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " [iterations]\n";
        return 1;
    }

    if (!to_size_t(argv[1], &iterations)) {
        std::cerr << "Invalid number of iterations: " << argv[1] << "\n";
        return 1;
    }

    try {
        for (size_t i = 0; i < NUM_THREADS; ++i) {
            threads[i].start_routine = (i & 1) == 0 ? ping : pong;
        }

        {
            std::lock_guard<std::mutex> lock(threads[0].mtx);
            threads[0].ready = true;
        }

        for (size_t i = 0; i < NUM_THREADS; ++i) {
            threads[i].thread = std::thread(threads[i].start_routine, i);
        }

        for (size_t i = 0; i < NUM_THREADS; ++i) {
            threads[i].thread.join();
        }
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
