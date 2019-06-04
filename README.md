# Computer Science Team

All of the code and tests implemented by the computer science team from Summer
2019 and onwards.


### Prerequisites

* `clang` or `gcc`
* `Python 3.6`


### Installing

```Bash
git clone https://github.com/ColumbiaOpenSourceUltrasound/CS_Team.git
cd CS_Team
make all
```


### Testing

Run the server, which will listen on port 10000:

```Bash
./server 10000
```

Run the client (in a different terminal):

```Bash
python test/main.py test/bunny.obj
```
