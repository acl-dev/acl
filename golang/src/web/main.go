package main

import (
	"flag"
	"fmt"
	"net/http"

	"master"
)

func handler(w http.ResponseWriter, r *http.Request) {
	//fmt.Println("served", r.URL)
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.Header().Set("Server", "web")
	fmt.Fprintf(w, "Hello World!\r\n")
}

var (
	filePath   string
	listenAddr string
)

func main() {
	flag.StringVar(&filePath, "c", "dummy.cf", "configure filePath")
	flag.StringVar(&listenAddr, "listen", "127.0.0.1:8880", "listen addr in alone running")
	flag.Parse()

	master.Prepare()

	http.HandleFunc("/", handler)

	if master.Alone {
		addrs := make([]string, 1)
		if len(listenAddr) == 0 {
			panic("listenAddr null")
		}

		addrs = append(addrs, listenAddr)

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
