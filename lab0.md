### part1
第一部分是用已有的轮子，利用操作系统的tcp栈，和目标服务器的http服务进程建立tcp连接，发送http协议包
### part2
第二部分是in-memory可靠传输，单线程，读写不同时进行，相当于不需要并发控制的pipe，其实就是一个queue，用一个string来作为queue，读有一个指针，写有一个指针，利用读指针从queue中依次读若干个字节，利用写指针向queue中依次写若干字符