package master

import (
	//"flag"
	"log"
	"net"
	"os"
)

type AcceptFunc func(net.Conn)
type CloseFunc func(net.Conn)

var (
	acceptHandler AcceptFunc = nil
	closeHandler  CloseFunc  = nil
)

func handleConn(conn net.Conn) {
	if acceptHandler == nil {
		panic("acceptHandler nil")
	}

	connCountInc()

	acceptHandler(conn)

	if closeHandler != nil {
		closeHandler(conn)
	} else {
		log.Println("closeHandler nil")
	}

	conn.Close()

	connCountDec()
}

func loopAccept(ln net.Listener) {
	for {
		conn, err := ln.Accept()
		if err != nil {
			log.Println("Accept error", err)
			break
		}

		go handleConn(conn)
	}

	if stopping {
		log.Println("server stopping")
	} else {
		log.Println("server listen error")
		netStop(false)
	}
}

func OnAccept(handler AcceptFunc) {
	for _, arg := range os.Args {
		log.Println("arg=", arg)
	}

	acceptHandler = handler
}

func OnClose(handler CloseFunc) {
	closeHandler = handler
}

func NetStart(addrs []string) {
	prepare()

	if preJailHandler != nil {
		preJailHandler()
	}

	chroot()

	if initHandler != nil {
		initHandler()
	}

	var daemon bool
	var listeners []*net.Listener
	if addrs != nil && len(addrs) > 0 {
		listeners = getListenersByAddrs(addrs)
		daemon = false
	} else {
		listeners = getListeners()
		daemon = true
	}

	if len(listeners) == 0 {
		panic("no available listener!")
	}

	for _, ln := range listeners {
		go loopAccept(*ln)
	}

	if daemon {
		go monitorMaster(listeners, nil, netStop)
	}

	log.Println("service started!")
	res := <-doneChan

	close(doneChan)
	doneChan = nil

	if exitHandler != nil {
		exitHandler()
	}

	if res {
		log.Println("service stopped normal!")
	} else {
		log.Println("service stopped abnormal!")
	}
}

func netStop(n bool) {
	if doneChan != nil {
		doneChan <- n
	}
}
