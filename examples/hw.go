// Our first program will print the classic "hello world"
// message. Here's the full source code.

package main;

import (
	"fmt"
)

func hello() {
	var i int = 0;
	i += 1;
	fmt.Println("asd");
}

func main() {
	var i int = 0;
	i = i + 1;
	go hello();
}


