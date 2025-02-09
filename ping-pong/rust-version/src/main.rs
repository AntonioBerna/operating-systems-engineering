use std::sync::{Arc, Condvar, Mutex};
use std::thread;

pub const NUM_THREADS: usize = 2;

#[derive(Debug)]
pub struct ThreadData {
    ready: Mutex<bool>,
    condvar: Condvar,
    message: String,
}

pub fn worker(own_data: Arc<ThreadData>, next_data: Arc<ThreadData>, iterations: usize) {
    for i in 0..iterations {
        let mut ready = own_data.ready.lock().unwrap();
        while !*ready {
            ready = own_data.condvar.wait(ready).unwrap();
        }

        *ready = false;
        drop(ready);

        println!("({}) {}", i + 1, own_data.message);

        let mut next_ready = next_data.ready.lock().unwrap();
        *next_ready = true;
        next_data.condvar.notify_one();
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let iterations: usize = args
        .get(1)
        .unwrap_or_else(|| {
            eprintln!("Usage: {} <iterations>", args[0]);
            std::process::exit(1);
        })
        .parse()
        .unwrap_or_else(|_| {
            eprintln!("Invalid number of iterations: {}", args[1]);
            std::process::exit(1);
        });

    let threads_data: Vec<Arc<ThreadData>> = (0..NUM_THREADS)
        .map(|i| {
            Arc::new(ThreadData {
                ready: Mutex::new(false),
                condvar: Condvar::new(),
                message: if i == 0 { "ping" } else { "pong" }.to_string(),
            })
        })
        .collect();

    *threads_data[0].ready.lock().unwrap() = true;

    let handles: Vec<_> = threads_data
        .iter()
        .enumerate()
        .map(|(i, data)| {
            let next_data = threads_data[(i + 1) % NUM_THREADS].clone();
            let data_clone = data.clone();

            thread::spawn(move || {
                worker(data_clone, next_data, iterations);
            })
        })
        .collect();

    for handle in handles {
        handle.join().unwrap();
    }
}
