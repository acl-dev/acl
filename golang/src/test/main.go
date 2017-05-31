package main

import (
	"flag"
	"fmt"
	"master"
)

func main() {
	var n int
	var conf string
	flag.IntVar(&n, "d", 100, "")
	flag.StringVar(&conf, "f", "./test.cf", "test configure")
	flag.Parse()
	fmt.Printf("n=%d, conf=%s\r\n", n, conf)

	myConf := new(master.Config)
	myConf.InitConfig(conf)
	fmt.Printf("log=%s\r\n", myConf.Get("master_log"))
}
