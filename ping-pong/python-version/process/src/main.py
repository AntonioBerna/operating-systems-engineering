from multiprocessing import Process, Semaphore
import sys

def worker(sem_acquire, sem_release, text, iterations):
    for i in range(1, iterations + 1):
        sem_acquire.acquire()
        print(f"({i}) {text}")
        sem_release.release()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"Usage: python {sys.argv[0]} <iterations>")
        sys.exit(1)
    
    try:
        iterations = int(sys.argv[1])
        if iterations <= 0:
            raise ValueError
    except ValueError:
        print("The number of iterations must be a positive integer.")
        sys.exit(1)
    
    ping_sem = Semaphore(1)
    pong_sem = Semaphore(0)
    
    ping_process = Process(target=worker, args=(ping_sem, pong_sem, "ping", iterations))
    pong_process = Process(target=worker, args=(pong_sem, ping_sem, "pong", iterations))
    
    ping_process.start()
    pong_process.start()
    
    ping_process.join()
    pong_process.join()