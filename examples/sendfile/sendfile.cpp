#ifndef WIN32
   #include <arpa/inet.h>
   #include <netdb.h>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
#endif
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include "udt.h"

using namespace std;

int sendfile(const std::string& fileName,const std::string& server_ip,const std::string& server_port);

int main(int argc, char* argv[])
{
   if ((argc < 4) || (0 == atoi(argv[2])))
   {
      cout << "usage: sendfile server_ip server_port filename ..." << endl;
      return -1;
   }

   // use this function to initialize the UDT library
   // 初始化UDT库
   UDT::startup();

   //std::vector<thread> threads;

   for (int i = 3; i <  argc; ++i)
   {
      sendfile(argv[i],argv[1],argv[2]);
      //threads.push_back(std::thread(sendfile,argv[i],argv[1],argv[2]));
   }

  /* for (int i = 0; i < threads.size(); ++i)
   {
      threads[i].join();
   }*/
   // use this function to release the UDT library
   UDT::cleanup();

   return 0;
}


int sendfile(const std::string& fileName,const std::string& server_ip,const std::string& server_port)
{
    // 地址信息提示、目标的地址信息指针
   struct addrinfo hints, *peer;

   // 内存初始化，全部置为0，设置同sendfile
   memset(&hints, 0, sizeof(struct addrinfo));
   hints.ai_flags = AI_PASSIVE;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   // 创建一个本地Socket
   UDTSOCKET fhandle = UDT::socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);

   // 根据传入参数和地址信息提示创建目标地址信息
   if (0 != getaddrinfo(server_ip.c_str(), server_port.c_str(), &hints, &peer))
   {
      std::cout << "incorrect server/peer address. " << server_ip << ":" << server_port << std::endl;
      return -1;
   }

   // connect to the server, implict bind
   // 连接到服务器，第一个参数为Socket，第二个为连接地址，第三个为地址长度（底层用来判断是否符合IPv4、IPv6标准的长度）
   if (UDT::ERROR == UDT::connect(fhandle, peer->ai_addr, peer->ai_addrlen))
   {
      cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
      return -1;
   }

   // 释放通过getaddrinfo分配的地址信息
   freeaddrinfo(peer);


   // send name information of the requested file
   // 发送请求的文件名称
   int len = fileName.size();

   // 向目标发送文件名的长度
   // 第一个参数为Socket，第二个参数为数据地址，第三个参数为数据长度，第四个参数会被底层忽略，没有意义
   if (UDT::ERROR == UDT::send(fhandle, (char*)&len, sizeof(int), 0))
   {
      cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
      return -1;
   }

   // 向目标发送文件名
   // 第一个参数为Socket，第二个参数为数据地址，第三个参数为数据长度，第四个参数会被底层忽略，没有意义
   if (UDT::ERROR == UDT::send(fhandle, fileName.c_str(), len, 0))
   {
      cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
      return -1;
   }

   // open the file
   // 打开请求方指定的文件，以二进制方式读取打开
   fstream ifs(fileName, ios::in | ios::binary);

   // 让文件指针定位到文件末尾
   ifs.seekg(0, ios::end);
   // 获得文件大小
   int64_t size = ifs.tellg();
   // 让文件指针定位到文件开头
   ifs.seekg(0, ios::beg);

   // send file size information
   // 向请求方发送文件长度
   // 第一个参数为Socket，第二个参数为数据地址，第三个参数为数据长度，第四个参数会被底层忽略，没有意义
   if (UDT::ERROR == UDT::send(fhandle, (char*)&size, sizeof(int64_t), 0))
   {
      cout << "send: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   // 初始采样，内容见CPerfMon
   UDT::TRACEINFO trace;
   UDT::perfmon(fhandle, &trace);

   // send the file
   // 偏移量为0，开始发送文件
   // 第一个参数为Socket，第二个参数为文件流，第三个参数为偏移量，第四个参数为文件大小
   int64_t offset = 0;
   if (UDT::ERROR == UDT::sendfile(fhandle, ifs, offset, size))
   {
      cout << "sendfile: " << UDT::getlasterror().getErrorMessage() << endl;
      return 0;
   }

   // 获取采样信息，内容见CPerfMon
   // 得出传输效率
   UDT::perfmon(fhandle, &trace);
   cout << fileName << ":speed = " << trace.mbpsSendRate << "Mbits/sec" << endl;

   // 关闭连接Socket
   UDT::close(fhandle);

   // 关闭文件流
   ifs.close();

   return 1;
}