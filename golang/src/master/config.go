package master

import (
	"bufio"
	"io"
	"os"
	"strings"
)

type Config struct {
	Entries map[string]string
}

func (c *Config) InitConfig(path string) {
	c.Entries = make(map[string]string)

	if len(path) == 0 {
		return
	}

	f, err := os.Open(path)
	if err != nil {
		panic(err)
	}

	defer f.Close()

	r := bufio.NewReader(f)
	for {
		line, _, err := r.ReadLine()
		if err != nil {
			if err == io.EOF {
				break
			}
			panic(err)
		}

		s := strings.TrimSpace(string(line))
		eq := strings.Index(s, "=")
		if eq < 0 {
			continue
		}

		name := strings.TrimSpace(s[:eq])
		if len(name) == 0 {
			continue
		}

		value := strings.TrimSpace(s[eq+1:])

		pos := strings.Index(value, "\t#")
		if pos > -1 {
			value = value[0:pos]
		}

		pos = strings.Index(value, " #")
		if pos > -1 {
			value = value[0:pos]
		}

		if len(value) == 0 {
			continue
		}

		c.Entries[name] = strings.TrimSpace(value)
	}
}

func (c Config) Get(name string) string {
	val, found := c.Entries[name]
	if !found {
		return ""
	}
	return val
}
