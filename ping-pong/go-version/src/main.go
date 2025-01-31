package main

import (
	"fmt"
	"os"
	"strconv"
	"sync"
)

func worker(message string, in <-chan struct{}, out chan<- struct{}, iterations int, wg *sync.WaitGroup) {
	defer wg.Done()

	for i := 0; i < iterations; i++ {
		<-in
		fmt.Printf("(%d) %s\n", i+1, message)
		out <- struct{}{}
	}
}

func main() {
	if len(os.Args) != 2 {
		fmt.Printf("Usage: %s <iterations>\n", os.Args[0])
		os.Exit(1)
	}

	iterations, err := strconv.Atoi(os.Args[1])
	if err != nil {
		fmt.Println("Invalid number of iterations.")
		os.Exit(1)
	}

	if iterations < 1 {
		fmt.Println("Number of iterations must be greater than 0.")
		os.Exit(1)
	}

	pingChan := make(chan struct{}, 1)
	pongChan := make(chan struct{}, 1)

	var wg sync.WaitGroup
	wg.Add(2)

	pingChan <- struct{}{}

	go worker("ping", pingChan, pongChan, iterations, &wg)
	go worker("pong", pongChan, pingChan, iterations, &wg)

	wg.Wait()
}
