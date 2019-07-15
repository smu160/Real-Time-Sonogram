# Real-Time-Image_Rend

<img src="https://media.giphy.com/media/KAwax2fd4wegVYKEgT/giphy.gif"/>

### Prerequisites

[SDL2](https://www.libsdl.org/index.php)


### Installing

#### macOS

1. Install SDL2 using brew:
```Bash
brew install sdl2
```

2. Navigate to the directory that sdl2 was installed into. For example:
```Bash
/usr/local/Cellar/sdl2/2.0.5
```

3. Copy the contents of:
```Bash
/usr/local/Cellar/sdl2/2.0.5/include/
```
into:
```Bash
Real-Time-Image_Rend/src/include
```

4. Copy the directory:
```Bash
/usr/local/Cellar/sdl2/2.0.5/lib/
```
into the directory:
```Bash
Real-Time-Image_Rend/src/lib
```


### Try it out!
Change into the `Real-Time-Image_Rend/src` directory and build the sonogram
renderer and its corresponding server:

```sh
make all
```

Run the renderer & server, which will listen on port 11112:

```sh
./main
```

Run a test client (in a different terminal):

```sh
cd Real-Time-Image_Rend/test
while true; do cat new_protoc_test.csv; done | nc localhost 11112
```

If everything went smoothly, you should see an image akin to the one at the top
of this page.


The SDL2 installation instructions are based on:
[Setting up SDL2 on mac without Xcode](https://medium.com/@edkins.sarah/set-up-sdl2-on-your-mac-without-xcode-6b0c33b723f7)

