package master

import (
	"log"
	"net"
	"os"
	"os/user"
	"strconv"
	"sync"
	"syscall"
	"time"
)

const (
	stateFd       = 5
	listenFdStart = 6
)

var (
	listenFdCount int = 1
	confPath      string
	sockType      string
	services      string
	privilege     bool = false
	verbose       bool = false
	chrootOn      bool = false
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

var (
	logPath    string
	username   string
	masterArgs string
	rootDir    string
)

func parseArgs() {
	/*
		flag.IntVar(&listenFdCount, "s", 1, "listen fd count")
		flag.StringVar(&confPath, "f", "", "configure path")
		flag.StringVar(&sockType, "t", "sock", "socket type")
		flag.StringVar(&services, "n", "", "master services")
		flag.BoolVar(&privilege, "u", false, "running privilege")
		flag.BoolVar(&verbose, "v", false, "debug verbose")
		flag.BoolVar(&chrootOn, "c", false, "chroot dir")

		flag.Parse()
	*/

	var n = len(os.Args)
	for i := 0; i < n; i++ {
		switch os.Args[i] {
		case "-s":
			i++
			if i >= n {
				break
			}
			listenFdCount, _ = strconv.Atoi(os.Args[i])
			if listenFdCount <= 0 {
				listenFdCount = 1
			}
		case "-f":
			i++
			if i >= n {
				break
			}
			confPath = os.Args[i]
		case "-t":
			i++
			if i >= n {
				break
			}
			sockType = os.Args[i]
		case "-n":
			i++
			if i >= n {
				break
			}
			services = os.Args[i]
		case "-u":
			privilege = true
		case "-v":
			verbose = true
		case "-c":
			chrootOn = true
		}
	}

	log.Printf("listenFdCount=%d, sockType=%s, services=%s",
		listenFdCount, sockType, services)
}

func prepare() {
	parseArgs()

	conf := new(Config)
	conf.InitConfig(confPath)

	logPath = conf.Get("master_log")
	if len(logPath) > 0 {
		f, err := os.OpenFile(logPath,
			os.O_RDWR|os.O_CREATE|os.O_APPEND, 0644)
		if err != nil {
			log.Println("OpenFile error", err)
		} else {
			log.SetOutput(f)
			//log.SetOutput(io.MultiWriter(os.Stderr, f))
		}
	}

	masterArgs = conf.Get("master_args")
	username = conf.Get("fiber_owner")
	rootDir = conf.Get("fiber_queue_dir")

	log.Printf("Args: %s\r\n", masterArgs)
}

func chroot() {
	if len(masterArgs) == 0 || !privilege || len(username) == 0 {
		return
	}

	user, err := user.Lookup(username)
	if err != nil {
		log.Printf("Lookup %s error %s", username, err)
	} else {
		gid, err := strconv.Atoi(user.Gid)
		if err != nil {
			log.Printf("invalid gid=%s, %s", user.Gid, err)
		} else if err := syscall.Setgid(gid); err != nil {
			log.Printf("Setgid error %s", err)
		} else {
			log.Printf("Setgid ok")
		}

		uid, err := strconv.Atoi(user.Uid)
		if err != nil {
			log.Printf("invalid uid=%s, %s", user.Uid, err)
		} else if err := syscall.Setuid(uid); err != nil {
			log.Printf("Setuid error %s", err)
		} else {
			log.Printf("Setuid ok")
		}
	}

	if chrootOn && len(rootDir) > 0 {
		err := syscall.Chroot(rootDir)
		if err != nil {
			log.Printf("Chroot error %s, path %s", err, rootDir)
		} else {
			log.Printf("Chroot ok, path %s", rootDir)
			err := syscall.Chdir("/")
			if err != nil {
				log.Printf("Chdir error %s", err)
			} else {
				log.Printf("Chdir ok")
			}
		}
	}
}

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

func monitorMaster(listeners []*net.Listener,
	onStopHandler func(), stopHandler func(bool)) {

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
