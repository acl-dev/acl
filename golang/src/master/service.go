package master

import (
	"log"
	"net"
	"os"
	"sync"
	"time"
)

const (
	stateFd       = 5
	listenFdStart = 6
	listenFdCount = 1
)

type PreJailFunc func()
type InitFunc func()
type ExitFunc func()

var (
	preJailHandler PreJailFunc = nil
	initHandler    InitFunc    = nil
	exitHandler    ExitFunc    = nil
	doneChan       chan bool   = make(chan bool)
	connCount      int         = 0
	connMutex      sync.RWMutex
	stopping       bool = false
	waitExit       int  = 10
)

func getListenersByAddrs(addrs []string) []*net.Listener {
	listeners := []*net.Listener(nil)
	for _, addr := range addrs {
		ln, err := net.Listen("tcp", addr)
		if err != nil {
			panic("listen error")
		}
		listeners = append(listeners, &ln)
	}
	return listeners
}

func getListeners() []*net.Listener {
	listeners := []*net.Listener(nil)
	for fd := listenFdStart; fd < listenFdStart+listenFdCount; fd++ {
		file := os.NewFile(uintptr(fd), "one listenfd")
		ln, err := net.FileListener(file)
		if err != nil {
			log.Println("create FileListener error", err)
			continue
		}
		listeners = append(listeners, &ln)
		log.Printf("add fd: %d", fd)
	}
	return listeners
}

func monitorMaster(listeners []*net.Listener, onStopHandler func(), stopHandler func(bool)) {

	file := os.NewFile(uintptr(stateFd), "")
	conn, err1 := net.FileConn(file)
	if err1 != nil {
		log.Println("FileConn error", err1)
	}

	log.Println("waiting master exiting ...")

	buf := make([]byte, 1024)
	_, err2 := conn.Read(buf)
	if err2 != nil {
		log.Println("disconnected from master", err2)
	}

	var n, i int
	n = 0
	i = 0

	stopping = true

	if onStopHandler != nil {
		onStopHandler()
	} else {
		// XXX: force stopping listen again
		for _, ln := range listeners {
			log.Println("Closing one listener")
			(*ln).Close()
		}
	}

	for {
		connMutex.RLock()
		if connCount <= 0 {
			connMutex.RUnlock()
			break
		}
		n = connCount
		connMutex.RUnlock()
		time.Sleep(time.Second) // sleep 1 second
		i++
		log.Printf("exiting, clients=%d, sleep=%d seconds", n, i)
		if waitExit > 0 && i >= waitExit {
			log.Printf("waiting too long >= %d", waitExit)
			break
		}
	}

	log.Println("master disconnected, exiting now")

	stopHandler(true)
}

func connCountInc() {
	connMutex.Lock()
	connCount++
	connMutex.Unlock()
}

func connCountDec() {
	connMutex.Lock()
	connCount--
	connMutex.Unlock()
}

func connCountCur() int {
	connMutex.RLock()
	n := connCount
	connMutex.RUnlock()
	return n
}

func OnPreJail(handler PreJailFunc) {
	preJailHandler = handler
}

func OnInit(handler InitFunc) {
	initHandler = handler
}

func OnExit(handler ExitFunc) {
	exitHandler = handler
}
