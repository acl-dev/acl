package main

import (
	"fmt"
	"log"
	"master"
	"net"
	"os"
)

func onAccept(conn net.Conn) {
	buf := make([]byte, 8192)
	for {
		n, err := conn.Read(buf)
		if err != nil {
			fmt.Println("read over", err)
			break
		}

		conn.Write(buf[0:n])
	}
}

func onClose(conn net.Conn) {
	log.Println("---client onClose---")
}

func main() {
	var logFile = "/home/zsx/tmp/acl/libexec/log.txt"

	f, err := os.OpenFile(logFile, os.O_RDWR|os.O_CREATE|os.O_APPEND, 0644)
	if err != nil {
		fmt.Println("open file error", err)
		return
	}

	log.SetOutput(f)
	//log.SetOutput(io.MultiWriter(os.Stderr, f))

	master.OnClose(onClose)
	master.OnAccept(onAccept)
	master.Start()
}
