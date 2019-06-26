# Imaging

<img src="https://media.giphy.com/media/QWpzbiFb53vySnBG0J/giphy.gif"/>

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
Imaging/src/include
```

4. Copy the directory:
```Bash
/usr/local/Cellar/sdl2/2.0.5/lib/
```
into the directory:
```Bash
Imaging/src/lib
```

5. Build the entire project by running:
```Bash
make all
```
in the `Imaging/src` directory


[Setting up SDL2 on mac without Xcode](https://medium.com/@edkins.sarah/set-up-sdl2-on-your-mac-without-xcode-6b0c33b723f7)


#### Testing

Instructions coming soon
