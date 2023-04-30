# COMP4621_Project

## Compile

```
mkdir build
cd ./build
cmake ..
make
```

## Run

*Notice: Please ensure you are under the build directory*

- Firstly run the server by typing `./server`
- Then you could go with the interaction between server in other terminal windows

## Tests

|                Task                | Status |                  Issues(if any)                  |
| :---------------------------------: | :----: | :----------------------------------------------: |
|            Registration            |  DONE  |                       NULL                       |
|                Login                |  DONE  |                       NULL                       |
| Welcome for new user and login back |  DONE  |                       NULL                       |
|        Online direct message        |  DONE  |                       NULL                       |
|       Offline direct message       |  DONE  |                       NULL                       |
|                EXIT                | ~~Buggy~~ DONE | The state was not changed to `OFFLINE` (Fixed) |
|                 WHO                 |  DONE  |                       NULL                       |
|      OFFLINE message retrieval      |  DONE  |                       NULL                       |
|        Broadcasting message        |  DONE  |                       NULL                       |
|        Password for register        |  DONE  |                       NULL                       |
|         Password for log in         | ~~Buggy~~ DONE |      Correct password is regarded as wrong (Fixed) |
