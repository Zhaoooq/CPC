# CPC凝结核粒子计数器通信协议 V1.0

## 1. 协议概述

CPC系统包含两类网络通信接口：

1. HTTP/JSON Web监控协议：用于Windows浏览器查看设备状态和实时测量数据。
2. CPC TCP工业通信协议：用于Windows工控机软件实时接收颗粒浓度及设备状态数据。

---

# 2. HTTP/JSON Web通信协议

## 2.1 通信参数

| 参数 | 内容 |
|---|---|
| 网络方式 | Ethernet TCP/IP |
| 应用协议 | HTTP |
| 数据格式 | JSON |
| 服务端 | Raspberry Pi CPC |
| 客户端 | Windows浏览器 |
| 默认端口 | 8080 |

## 2.2 数据接口

请求：

```
GET /api/status
```

返回完整JSON包含：设备状态、颗粒浓度、OPC原始计数速率、温度、压差、气泵状态和比例阀状态。

示例：

```json
{
 "device":"CPC",
 "status":"running",
 "particle":{"concentration":12345.6,"unit":"#/cm3"},
 "temperature":{"condensation":10.2,"saturation":40.1,"opc":39.8}
}
```

---

# 3. CPC TCP工业通信协议

## 3.1 通信参数

| 参数 | 内容 |
|---|---|
| 协议 | TCP/IP |
| 服务端 | CPC Raspberry Pi |
| 客户端 | Windows工控机 |
| 默认端口 | 5000 |
| 数据方向 | CPC主动发送 |
| 发送周期 | 1 s |

## 3.2 数据帧格式

ASCII格式：

```
$CPC,ID,TIME,CONC,RAW,TEMP1,TEMP2,TEMP3,DP1,DP2,DP3,PUMP,VALVE,STATUS,CRC\r\n
```

字段：

|字段|说明|
|-|-|
|CONC|颗粒数目浓度 (#/cm3)|
|RAW|OPC原始计数速率|
|TEMP1~TEMP3|三段温度|
|DP1~DP3|三路压差|
|PUMP|气泵功率|
|VALVE|比例阀电流|
|STATUS|运行状态|
|CRC|CRC16校验|

---

# 4. 通讯状态机流程

```
启动
 |
初始化网络接口
 |
开启TCP Server
 |
等待工控机连接
 |
连接成功
 |
周期采集数据
 |
数据封装
 |
CRC校验计算
 |
发送数据帧
 |
等待下一周期
```

异常状态：

```
发送失败
   |
重新连接
   |
恢复数据发送
```

---

# 5. TCP数据帧时序

```
CPC服务器                 Windows工控机

监听5000端口
       |
       |<------ TCP Connect
       |
       ------ Ready ------>
       |
       |---- DATA FRAME -->
       |
       |---- DATA FRAME -->  (1s周期)
       |
```

---

# 6. CRC16-MODBUS校验

CRC计算范围：除CRC字段外的完整数据内容。

计算流程：

1. CRC寄存器初始化为0xFFFF。
2. 对每个数据字节进行异或。
3. 循环右移8次并根据多项式0xA001计算。
4. 得到16位校验结果。

接收端重新计算CRC，与帧内CRC字段比较。

---

# 7. 上位机软件通信流程

Windows工控机程序流程：

1. 配置CPC IP地址和TCP端口5000。
2. 建立TCP连接。
3. 接收ASCII数据帧。
4. 根据\r\n分割完整数据包。
5. 解析字段。
6. CRC校验。
7. 保存颗粒浓度和状态数据。

---

# 8. 状态码

|状态|含义|
|-|-|
|INIT|初始化|
|WARMUP|热机|
|READY|准备完成|
|RUN|正常测量|
|WARNING|报警|
|ERROR|故障|

---

# 9. 数据可靠性设计

TCP通信采用CRC16校验机制。
断线后客户端重新建立TCP连接；超过3秒未完成数据发送时，通信状态进入异常状态。

---

# 10. 应用说明

HTTP协议用于设备维护和远程监控。
TCP协议作为CPC与工业控制软件之间的正式数据接口。