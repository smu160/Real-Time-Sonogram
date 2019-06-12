# Imaging

All of the code and tests implemented by the computer science team from Summer
2019 and onwards.


### Prerequisites

* SDL2


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
Imaging/recon/include
```

4. Copy the directory:
```Bash
/usr/local/Cellar/sdl2/2.0.5/lib/
```
into the directory:
```Bash
Imaging/recon/lib
```

5. Build the entire project by running:
```Bash
make all
```
in the `Imaging/recon` directory

6. Finally, run it with the provided test data:
```Bash
./main test_data.csv
```
