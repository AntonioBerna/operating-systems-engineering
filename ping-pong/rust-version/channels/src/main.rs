use std::sync::mpsc;
use std::thread;

pub const NUM_THREADS: usize = 2;

pub fn worker(
    rx: mpsc::Receiver<()>,
    tx: Option<mpsc::Sender<()>>,
    message: &str,
    iterations: usize,
) {
    for i in 0..iterations {
        let _ = rx.recv();
        println!("({}) {}", i + 1, message);
        if let Some(tx) = &tx {
            let _ = tx.send(());
        }
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

    let (tx_ping, rx_pong) = mpsc::channel();
    let (tx_pong, rx_ping) = mpsc::channel();

    let tx_pong_main = tx_pong.clone();

    let handle_ping = thread::spawn({
        let rx = rx_ping;
        move || {
            worker(rx, Some(tx_ping), "ping", iterations);
        }
    });

    let handle_pong = thread::spawn({
        let rx = rx_pong;
        move || {
            worker(rx, Some(tx_pong), "pong", iterations);
        }
    });

    tx_pong_main.send(()).unwrap();

    handle_ping.join().unwrap();
    handle_pong.join().unwrap();
}
