package master

import (
	"flag"
	"fmt"
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
	prepareCalled  bool = false
)

// from configure file of the app
var (
	MasterService  string
	MasterLogPath  string
	MasterOwner    string
	MasterArgs     string
	AppRootDir     string
	AppUseLimit    int    = 0
	AppIdleLimit   int    = 0
	AppQuickAbort  bool   = false
	AppWaitLimit   int    = 10
	AppAccessAllow string = "all"
)

// from command args
var (
	MasterConfigure    string
	MasterServiceName  string
	MasterServiceType  string
	MasterVerbose      bool
	MasterUnprivileged bool
	//	MasterChroot       bool
	MasterSocketCount int = 1
	Alone             bool
)

// set the max opened file handles for current process which let
// the process can handle more connections.
func setOpenMax() {
	var rlim syscall.Rlimit
	err := syscall.Getrlimit(syscall.RLIMIT_NOFILE, &rlim)
	if err != nil {
		fmt.Println("get rlimit error: " + err.Error())
		return
	}
	if rlim.Max <= 0 {
		rlim.Max = 100000
	}
	rlim.Cur = rlim.Max

	err = syscall.Setrlimit(syscall.RLIMIT_NOFILE, &rlim)
	if err != nil {
		fmt.Println("set rlimit error: " + err.Error())
	}
}

// init the command args come from acl_master; the application should call
// flag.Parse() in its main function!
func initFlags() {
	flag.StringVar(&MasterConfigure, "f", "", "app configure file")
	flag.StringVar(&MasterServiceName, "n", "", "app service name")
	flag.StringVar(&MasterServiceType, "t", "sock", "app service type")
	flag.BoolVar(&Alone, "alone", false, "stand alone running")
	flag.BoolVar(&MasterVerbose, "v", false, "app verbose")
	flag.BoolVar(&MasterUnprivileged, "u", false, "app unprivileged")
	//	flag.BoolVar(&MasterChroot, "c", false, "app chroot")
	flag.IntVar(&MasterSocketCount, "s", 1, "listen fd count")
}

func init() {
	initFlags()
	setOpenMax()
}

func parseArgs() {
	var n = len(os.Args)
	for i := 0; i < n; i++ {
		switch os.Args[i] {
		case "-s":
			i++
			if i < n {
				listenFdCount, _ = strconv.Atoi(os.Args[i])
				if listenFdCount <= 0 {
					listenFdCount = 1
				}
			}
		case "-f":
			i++
			if i < n {
				confPath = os.Args[i]
			}
		case "-t":
			i++
			if i < n {
				sockType = os.Args[i]
			}
		case "-n":
			i++
			if i < n {
				services = os.Args[i]
			}
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

// this function can be called automatically in net_service.go or
// web_service.go to load configure, and it can also be canned in appliction's
// main function
func Prepare() {
	if prepareCalled {
		return
	} else {
		prepareCalled = true
	}

	parseArgs()

	conf := new(Config)
	conf.InitConfig(confPath)

	MasterLogPath = conf.GetString("master_log")
	if len(MasterLogPath) > 0 {
		f, err := os.OpenFile(MasterLogPath, os.O_RDWR|os.O_CREATE|os.O_APPEND, 0644)
		if err != nil {
			fmt.Printf("OpenFile %s error %s", MasterLogPath, err)
		} else {
			log.SetOutput(f)
			//log.SetOutput(io.MultiWriter(os.Stderr, f))
		}
	}

	MasterService = conf.GetString("master_service")
	MasterOwner = conf.GetString("master_owner")
	MasterArgs = conf.GetString("master_args")

	AppRootDir = conf.GetString("app_queue_dir")
	AppUseLimit = conf.GetInt("app_use_limit")
	AppIdleLimit = conf.GetInt("app_idle_limit")
	AppQuickAbort = conf.GetBool("app_quick_abort")
	AppWaitLimit = conf.GetInt("app_wait_limit")
	AppAccessAllow = conf.GetString("app_access_allow")

	log.Printf("Args: %s, AppAccessAllow: %s\r\n", MasterArgs, AppAccessAllow)
}

func chroot() {
	if len(MasterArgs) == 0 || !privilege || len(MasterOwner) == 0 {
		return
	}

	user, err := user.Lookup(MasterOwner)
	if err != nil {
		log.Printf("Lookup %s error %s", MasterOwner, err)
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

	if chrootOn && len(AppRootDir) > 0 {
		err := syscall.Chroot(AppRootDir)
		if err != nil {
			log.Printf("Chroot error %s, path %s", err, AppRootDir)
		} else {
			log.Printf("Chroot ok, path %s", AppRootDir)
			err := syscall.Chdir("/")
			if err != nil {
				log.Printf("Chdir error %s", err)
			} else {
				log.Printf("Chdir ok")
			}
		}
	}
}

// In run alone mode, the application should give the listening addrs and call
// this function to listen the given addrs
func getListenersByAddrs(addrs []string) []*net.Listener {
	listeners := []*net.Listener(nil)
	for _, addr := range addrs {
		ln, err := net.Listen("tcp", addr)
		if err != nil {
			panic(fmt.Sprintf("listen error=\"%s\", addr=%s", err, addr))
		}
		listeners = append(listeners, &ln)
	}
	return listeners
}

// In acl_master daemon running mode, this function will be called for init
// the listener handles.
func getListeners() []*net.Listener {
	listeners := []*net.Listener(nil)
	for fd := listenFdStart; fd < listenFdStart+listenFdCount; fd++ {
		file := os.NewFile(uintptr(fd), "open one listenfd")
		ln, err := net.FileListener(file)
		if err == nil {
			listeners = append(listeners, &ln)
			log.Printf("add fd: %d", fd)
			continue
		}
		file.Close()
		log.Println(fmt.Sprintf("create FileListener error=\"%s\", fd=%d", err, fd))
	}
	return listeners
}

// monitor the PIPE IPC between the current process and acl_master,
// when acl_master close thePIPE, the current process should exit after
// which has handled all its tasks
func monitorMaster(listeners []*net.Listener,
	onStopHandler func(), stopHandler func(bool)) {

	file := os.NewFile(uintptr(stateFd), "")
	conn, err := net.FileConn(file)
	if err != nil {
		log.Println("FileConn error", err)
	}

	log.Println("waiting master exiting ...")

	buf := make([]byte, 1024)
	_, err = conn.Read(buf)
	if err != nil {
		log.Println("disconnected from master", err)
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
		if AppWaitLimit > 0 && i >= AppWaitLimit {
			log.Printf("waiting too long >= %d", AppWaitLimit)
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
