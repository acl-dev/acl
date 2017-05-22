package main

import (
	"fmt"
	"net/http"
	"os"

	"master"
)

func handler(w http.ResponseWriter, r *http.Request) {
	//fmt.Println("served", r.URL)
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.Header().Set("Server", "web")
	fmt.Fprintf(w, "Hello World!\r\n")
}

func main() {
	http.HandleFunc("/", handler)

	if len(os.Args) > 1 && os.Args[1] == "alone" {
		addrs := make([]string, 1)
		if len(os.Args) > 2 {
			addrs = append(addrs, os.Args[2])
		} else {
			addrs = append(addrs, "127.0.0.1:8880")
		}

		fmt.Printf("listen:")
		for _, addr := range addrs {
			fmt.Printf(" %s", addr)
		}
		fmt.Println()

		master.WebStart(addrs)
	} else {
		// daemon mode in master framework
		master.WebStart(nil)
	}
}
