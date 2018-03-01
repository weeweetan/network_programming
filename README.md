## network_programming
使用C语言实现的一些网络编程demo

# tips
TCPEchoClient4.c与TCPEchoServer4.c是ipv4的客户端与服务端<br>
TCPEchoClient.c与TCPEchoServer.c是通用的客户端与服务端<br>
# 编译事项
客户端编译时，需要TCPEchoClient.c TCPClientUtility.c DieWithMessage.c一起编译<br>
服务端编译时，需要TCPEchoServer.c TCPServerUtility.c DieWithMessage.c一起编译<br>

UDP服务端程序需要UDPEchoServer.c DieWithMessage.c TCPServerUtility.c一起编译<br>
UDP客户端程序需要UDPEchoClient.c DieWithMessage.c一起编译<br>