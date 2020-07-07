# Http Web Server
## 1. Description
A http web server written in C++ which provides static resources support. It could enable users deploying their own static web pages. Furthermore it could also provide reverse proxy service for the server side.
This web server was powered by epoll under Linux operating system. Leveraging dynamic thread pool to accelerate efficiency and take advantage of hardware(physical machines) resource effectively.
## 2. Step to run the server
- Start the server  
```
cd http-server/
sudo make
./bin/startup.sh
```
- Shutdown the server
```
cd http-server/
./bin/shutdown.sh
```
Note:  
1. Text based files(html, css, js, etc.) are placed in res/html directory. Binary based files(png, jpg, gif, etc.) are placed in res/pic directory.  
2. logs/ directory is required to save logs.
3. conf/server.xml file is required to initiate the server, and format of this xml file should be strictly correct.
## 3. Structure of the program
![Structure Diagram](https://raw.githubusercontent.com/zheyuyang0515/Pic/master/structure.jpg)
## 4. Test
### i. Environment
1. Operating System: Ubuntu 18.04.4 LTS (Kernel: 4.15.0-106-generic)
2. CPU Logic Core: 4
3. RAM: 6144 MB
4. Tools: Apache JMeter 5.3
5. Test Resources Size: 106.15 KB
6. Network Type: Local Area Network
### ii. Test Result 
- Maximal Number of Workers: 1  

 |Number of Threads(Users)| Loop Count  | Duration(seconds) |  Throughput(transactions/second) |  Error(%) | Received(KB/Sec) | Sent(KB/Sec)    |
 |  ----                  | ----        |  ----             |              ----                |  ----     |     ----         |            ---- |
 | 5000                   | Infinite    |     60            |      3921.5                      |   0       |   4901.00        |  471.04         |
 | 7000                   | Infinite    |     60            |      3794.5                      |   0       |   4743.07        |  455.78         |
 | 9000                   | Infinite    |     60            |      3863.5                      |   0       |   4829.41        |  464.08         |
 | 11000                  | Infinite    |     60            |      3690.8                      |   0       |   4613.45        |  443.32         |
 | 13000                  | Infinite    |     60            |      3257.4                      |   0       |   4071.72        |  391.27         |
 | 15000                  | Infinite    |     60            |      2847.0                      |   17.80   |   4148.62        |  281.11         |

- Maximal Number of Workers: 4  

 |Number of Threads(Users)| Loop Count  | Duration(seconds) |  Throughput(transactions/second) |  Error(%) | Received(KB/Sec) | Sent(KB/Sec)    |
 |  ----                  | ----        |  ----             |              ----                |  ----     |     ----         |            ---- |
 | 5000                   | Infinite    |     60            |    7972.9                        |   0       |   9966.11        |   957.68        |
 | 7000                   | Infinite    |     60            |    6788.5                        |   0       |   8485.58        |   815.41        |
 | 9000                   | Infinite    |     60            |    6967.8                        |   0       |   8709.69        |   836.95        |
 | 11000                  | Infinite    |     60            |    6461.8                        |   0       |   8077.27        |   776.18        |
 | 13000                  | Infinite    |     60            |    4542.3                        |   0       |   5677.85        |   545.61        |
 | 15000                  | Infinite    |     60            |    4550.9                        |   19.93   |   6744.39        |   437.69        |


