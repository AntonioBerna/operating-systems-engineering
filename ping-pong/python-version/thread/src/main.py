import sys
import threading

def worker(lock_acquire, lock_release, text, iterations):
    for i in range(1, iterations + 1):
        lock_acquire.acquire()
        print(f"({i}) {text}")
        lock_release.release()

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <iterations>")
        sys.exit(1)
    
    try:
        iterations = int(sys.argv[1])
        if iterations <= 0:
            raise ValueError
    except ValueError:
        print("Il numero di iterazioni deve essere un intero positivo.")
        sys.exit(1)

    ping_lock = threading.Lock()
    pong_lock = threading.Lock()
    
    pong_lock.acquire()
    
    ping_thread = threading.Thread(
        target=worker,
        args=(ping_lock, pong_lock, "ping", iterations)
    )
    pong_thread = threading.Thread(
        target=worker,
        args=(pong_lock, ping_lock, "pong", iterations)
    )
    
    ping_thread.start()
    pong_thread.start()
    
    ping_thread.join()
    pong_thread.join()