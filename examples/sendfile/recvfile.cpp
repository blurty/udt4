#ifndef WIN32
   #include <cstdlib>
   #include <netdb.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
#endif
#include <fstream>
#include <iostream>
#include <cstring>
#include "udt.h"
#include "timecheck.hpp"

using namespace std;

void* recvfile(void*);


int main(int argc, char* argv[])
{
   //usage: sendfile [server_port]
   if ((2 < argc) || ((2 == argc) && (0 == atoi(argv[1]))))
   {
      cout << "usage: recvfile [server_port]" << endl;
      return 0;
   }

   // use this function to initialize the UDT library
   UDT::startup();

   // 地址信息提示
   addrinfo hints;

   // 实际使用的地址信息指针
   addrinfo* res;

   // 内存初始化，全部置为0
   memset(&hints, 0, sizeof(struct addrinfo));

   // 如上表所示，ai_flagsde值范围为0~7，取决于程序如何设置3个标志位，
   // 比如设置ai_flags为 “AI_PASSIVE|AI_CANONNAME”，ai_flags值就为3。三个参数的含义分别为：
   // (1)AI_PASSIVE当此标志置位时，表示调用者将在bind()函数调用中使用返回的地址结构。当此标志不置位时，表示将在connect()函数调用中使用。
   //    当节点名位NULL，且此标志置位，则返回的地址将是通配地址。如果节点名NULL，且此标志不置位，则返回的地址将是回环地址。
   // (2)AI_CANNONAME当此标志置位时，在函数所返回的第一个addrinfo结构中的ai_cannoname成员中，
   //    应该包含一个以空字符结尾的字符串，字符串的内容是节点名的正规名。
   // (3)AI_NUMERICHOST当此标志置位时，此标志表示调用中的节点名必须是一个数字地址字符串
   hints.ai_flags = AI_PASSIVE;

   // AF_UNIX（本机通信）
   // AF_INET（TCP/IP – IPv4）
   // AF_INET6（TCP/IP – IPv6）
   hints.ai_family = AF_INET;

   // SOCK_STREAM： 提供面向连接的稳定数据传输，即TCP协议。
   // OOB： 在所有数据传送前必须使用connect()来建立连接状态。
   // SOCK_DGRAM： 使用不连续不可靠的数据包连接。
   // SOCK_SEQPACKET： 提供连续可靠的数据包连接。
   // SOCK_RAW： 提供原始网络协议存取。
   // SOCK_RDM： 提供可靠的数据包连接。
   // SOCK_PACKET： 与网络驱动程序直接通信。
   //////////// 注：此处的SOCK_STREAM并不是表示UDT将会使用TCP类型的Socket，在底层将会转化为UDT_STREAM
   //////////// 并且在UDT中仅支持SOCK_STREAM和SOCK_DGRAM，分别对应UDT_STREAM和UDT_DGRAM
   hints.ai_socktype = SOCK_STREAM;

   // 默认服务端口
   string service("9000");
   if (2 == argc)
      service = argv[1];

   // 根据地址信息提示分配实际可用的地址信息
   if (0 != getaddrinfo(NULL, service.c_str(), &hints, &res))
   {
      cout << "illegal port number or port is busy.\n" << endl;
      return 0;
   }

   // 根据实际使用的地址信息创建一个UDT的socket
   // 此处实际上最终调用了CUDTUnited的newSocket，第一个参数会被直接设置到CUDT的m_iIPversion，第二个参数会被映射为UDT的连接类型，第三个参数被忽略，没有实际意义
   // Gu说了函数原型尽量保持与BSD的Socket一致，因此一些对系统没意义的参数也被照搬了进来……
   UDTSOCKET serv = UDT::socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   // Windows UDP issue
   // For better performance, modify HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
#ifdef WIN32
   // Windows下的UDP的问题
   // 如果想要提高效率，可以修改注册表HKLM\System\CurrentControlSet\Services\Afd\Parameters\FastSendDatagramThreshold
   int mss = 1052;

   // 第一个参数为要设置的UDTSocket
   // 第二个参数0是会被忽略的，没有实际意义
   // 第三个参数为要设置的参数，有以下几种选项、来自enum UDTOpt：
   //     UDT_MSS 最大传输单位
   //     UDT_SNDSYN 是否阻塞发送
   //     UDT_RCVSYN 是否阻塞接收
   //     UDT_CC 自定义拥塞控制算法
   //     UDT_FC 窗口大小
   //     UDT_SNDBUF 发送队列缓冲最大值
   //     UDT_RCVBUF UDT接收缓冲大小
   //     UDT_LINGER 关闭时等待数据发送完成
   //     UDP_SNDBUF UDP发送缓冲大小
   //     UDP_RCVBUF UDP接收缓冲大小
   //     UDT_RENDEZVOUS 会合连接模式
   //     UDT_SNDTIMEO send()超时
   //     UDT_RCVTIMEO recv()超时
   //     UDT_REUSEADDR 复用一个已存在的端口或者创建一个新的端口
   //     UDT_MAXBW 当前连接可以使用的最大带宽(bytes per second)
   // 第四个参数是参数值
   // 第五个参数为参数值长度，在底层也会被忽略，没有意义
   UDT::setsockopt(serv, 0, UDT_MSS, &mss, sizeof(int));
#endif

   // 绑定端口，第一个参数为Socket，第二个为绑定地址，第三个为地址长度（底层用来判断是否符合IPv4、IPv6标准的长度）
   if (UDT::ERROR == UDT::bind(serv, res->ai_addr, res->ai_addrlen))
   {
      cout << "bind: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   // 释放通过getaddrinfo分配的地址信息
   freeaddrinfo(res);

   cout << "server is ready at port: " << service << endl;

   // 开始监听，第一个参数为Socket，第二个参数为队列中允许的最大连接数
   UDT::listen(serv, 10);

   // 客户端地址信息
   sockaddr_storage clientaddr;
   // 地址信息数据长度
   int addrlen = sizeof(clientaddr);

   // 捕获连接的Socket
   UDTSOCKET fhandle;

   while (true)
   {
      // 接受连接请求，第一个参数为Socket，第二个参数为请求方地址信息，第三个参数为地址信息长度，
      // 第二个和第三个参数必须分配好地址空间，才能返回信息
      if (UDT::INVALID_SOCK == (fhandle = UDT::accept(serv, (sockaddr*)&clientaddr, &addrlen)))
      {
         cout << "accept: " << UDT::getlasterror().getErrorMessage() << endl;
         return 0;
      }

      // 通过getnameinfo()将地址信息转换为节点地址、端口信息
      char clienthost[NI_MAXHOST];
      char clientservice[NI_MAXSERV];
      getnameinfo((sockaddr *)&clientaddr, addrlen, clienthost, sizeof(clienthost), clientservice, sizeof(clientservice), NI_NUMERICHOST|NI_NUMERICSERV);
      cout << "new connection: " << clienthost << ":" << clientservice << endl;

      // 连接成功后启动sendfile线程，传入捕获到的Socket，这里使用了拷贝构造创建一个新的对象，使得fhandle本身的使用不会受到线程的影响
      #ifndef WIN32
         pthread_t filethread;
         pthread_create(&filethread, NULL, recvfile, new UDTSOCKET(fhandle));
         pthread_detach(filethread);
      #else
         CreateThread(NULL, 0, recvfile, new UDTSOCKET(fhandle), 0, NULL);
      #endif
   }

   // 关闭监听Socket
   UDT::close(serv);

   // use this function to release the UDT library
   UDT::cleanup();

   return 0;
}


void* recvfile(void* usocket)
{
    // 获取Socket，并清除原传入的Socket
   UDTSOCKET fhandle = *(UDTSOCKET*)usocket;
   delete (UDTSOCKET*)usocket;

   // aquiring file name information from client
   char file[1024];
   int len;

   // 接收一个数据包，内容为一个int值，长度为int的长度，数值保存在len中
   // 第一个参数为Socket，第二个参数为数据保存地址，第三个参数为数据长度，第四个参数底层会忽略，没有意义
   if (UDT::ERROR == UDT::recv(fhandle, (char*)&len, sizeof(int), 0))
   {
      cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
      return nullptr;
   }

   // 开始接收信息，通过上面获得的长度来判断信息的大小
   if (UDT::ERROR == UDT::recv(fhandle, file, len, 0))
   {
      cout << "recv: " << UDT::getlasterror().getErrorMessage() << endl;
      return nullptr;
   }

   // 给接收到的字符串加结束符
   file[len] = '\0';

   TimeCheck timech(file);

   // 接收文件大小
   int64_t size;

    // 接收一个文件大小数据，内容为一个int64_t值，长度为int64_t的长度，数值保存在size中
   // 第一个参数为Socket，第二个参数为数据保存地址，第三个参数为数据长度，第四个参数底层会忽略，没有意义
   if (UDT::ERROR == UDT::recv(fhandle, (char*)&size, sizeof(int64_t), 0))
   {
      cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
      return nullptr;
   }

   // 负值表示没有该文件
   if (size < 0)
   {
      cout << "no such file " << file << " on the server\n";
      return nullptr;
   }

   // receive the file
   // 开始接收文件
   // 通过二进制方式写入文件，如果文件已存在则覆盖
   fstream ofs(file, ios::out | ios::binary | ios::trunc);

   // 接收到的数据大小以及偏移量
   int64_t recvsize; 
   int64_t offset = 0;

   // 从0位置开始接收文件数据
   // 第一个参数为Socket，第二个参数为文件流，第三个参数为偏移量，第四个参数为文件大小
   if (UDT::ERROR == (recvsize = UDT::recvfile(fhandle, ofs, offset, size)))
   {
      cout << "recvfile: " << UDT::getlasterror().getErrorMessage() << endl;
      return nullptr;
   }

   // 关闭连接Socket
   UDT::close(fhandle);

   // 关闭文件流
   ofs.close();

   return nullptr;


}
