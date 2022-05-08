# BabyWebServer
Study project for 《Linux高性能服务器编程》

## 整体逻辑

## 演示

![image-20220508114448910](images/image-20220508114448910.png)

<center style="font-size:14px;color:#C0C0C0;text-decoration:underline">200 OK</center>

![image-20220508114626773](images/image-20220508114626773.png)

<center style="font-size:14px;color:#C0C0C0;text-decoration:underline">404 Not Found</center>



![image-20220508114714844](images/image-20220508114714844.png)

<center style="font-size:14px;color:#C0C0C0;text-decoration:underline">403 Forbidden</center>

当然上面都是次要的🤪，随便摸了几个页面而已，只支持GET也没啥好演示的。

### 压力测试

![image-20220508120910212](images/image-20220508120910212.png)

client上到3w时：

![image-20220508115007515](images/image-20220508115007515.png)





## 细节拆分

- [lock](https://github.com/yuuuuu422/BabyWebServer/tree/main/lock)：线程同步的包装类，包含互斥量、信号量和条件变量

  - 互斥量主要用作保护线程池中的请求队列，确保其独占式访问。
  - 信号量和条件变量都可以用作收到请求时，对线程的通知，代码使用了信号量。
- [threadpool](https://github.com/yuuuuu422/BabyWebServer/tree/main/threadpool)：线程池，包含线程的创建和销毁，内部维护一个请求队列，当主进程收到`EPOLLIN`信息时，将连接请求加入队列，分发给子线程解析。

- [http](https://github.com/yuuuuu422/BabyWebServer/tree/main/http)：核心为一个内部驱动的状态转移有限自动机，首先逐行读取和解析HTTP请求行、请求头和请求体，再根据请求的资源`munmap`映射到内存中，最后读取资源做出Response。
  - 需要注意的一点是不同的请求资源回应的`content-type`也应该不同，这里简单维护了一个map对资源后缀进行判断。（刚开始发现css都加载不出来...）
- main.cpp：同步方式的Proactor。

## 拓展

- 定时器
- 日志
- POST请求

## 参考

- [Linux高性能服务器编程](https://book.douban.com/subject/24722611/) 及其 [15章代码](https://github.com/raichen/LinuxServerCodes/tree/master/15)

- 社长：[TinyWebServer(Raw_Version)](https://github.com/qinguoyi/TinyWebServer/tree/raw_version)

- 小林coding：[I/O 多路复用：select/poll/epoll](https://xiaolincoding.com/os/8_network_system/selete_poll_epoll.html) And [高性能网络模式：Reactor 和 Proactor](https://xiaolincoding.com/os/8_network_system/reactor.html)
