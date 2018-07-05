// Our first program will print the classic "hello world"
// message. Here's the full source code.

package main;

func hello() {
	var i int = 0;
	i += 1;
}

func main() {
	var i int = 0;
	i = i + 1;
	go func() {
		var j int = 0;
		j += 1;
	}();
}


