#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <stdexcept>
#include <string>

static size_t iterations;
std::mutex cout_mtx;

struct ThreadData {
    std::thread thread;
    void (*start_routine)(size_t);
    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
};

constexpr size_t NUM_THREADS = 2;
static ThreadData threads[NUM_THREADS];
constexpr const char *messages[NUM_THREADS] = { "ping", "pong" };

static void worker(size_t id) {
    try {
        const std::string msg = messages[id];
        size_t next = (id + 1) % NUM_THREADS;

        for (size_t i = 0; i < iterations;) {
            std::unique_lock<std::mutex> lock(threads[id].mtx);
            threads[id].cv.wait(lock, [&] { return threads[id].ready; });
            threads[id].ready = false;

            {
                std::lock_guard<std::mutex> cout_lock(cout_mtx);
                std::cout << "(" << ++i << ") " << msg << "\n";
            }

            lock.unlock();

            {
                std::lock_guard<std::mutex> lock_next(threads[next].mtx);
                threads[next].ready = true;
            }
            threads[next].cv.notify_one();
        }
    } catch (const std::exception &e) {
        std::lock_guard<std::mutex> cout_lock(cout_mtx);
        std::cerr << "Exception in " << messages[id] << ": " << e.what() << "\n";
    } catch (...) {
        std::lock_guard<std::mutex> cout_lock(cout_mtx);
        std::cerr << "Unknown exception in " << messages[id] << "\n";
    }
}

static bool to_size_t(const char *buffer, size_t *value) {
    char *endptr;
    if (*buffer == '-') return false;
    errno = 0;
    size_t val = strtoul(buffer, &endptr, 0);
    if (errno == ERANGE || endptr == buffer || *endptr != '\0' || val > SIZE_MAX) {
        return false;
    }
    *value = static_cast<size_t>(val);
    return true;
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
            threads[i].start_routine = (i & 1) == 0 ? worker : worker;
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