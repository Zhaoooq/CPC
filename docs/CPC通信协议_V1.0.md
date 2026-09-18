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

返回示例：

```json
{
 "device":"CPC",
 "status":"running",
 "particle":{
  "concentration":12345.6,
  "unit":"#/cm3"
 },
 "temperature":{
  "condensation":10.2,
  "saturation":40.1,
  "opc":39.8
 }
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

采用ASCII数据帧：

```
$CPC,ID,TIME,CONC,RAW,TEMP1,TEMP2,TEMP3,DP1,DP2,DP3,PUMP,VALVE,STATUS,CRC\r\n
```

## 3.3 字段说明

|字段|说明|
|-|-|
|CONC|颗粒数目浓度 (#/cm3)|
|RAW|OPC原始计数速率|
|TEMP1~TEMP3|冷凝段、饱和段、OPC段温度|
|DP1~DP3|三路压差|
|PUMP|气泵功率|
|VALVE|比例阀控制电流|
|STATUS|设备运行状态|
|CRC|CRC16校验|

---

# 4. 状态码

|状态|含义|
|-|-|
|INIT|初始化|
|WARMUP|热机|
|READY|准备完成|
|RUN|正常测量|
|WARNING|报警|
|ERROR|故障|

---

# 5. 数据可靠性设计

TCP通信采用CRC16校验机制。

断线后客户端重新建立TCP连接；超过3秒未完成数据发送时，通信状态进入异常状态。

---

# 6. 应用说明

HTTP协议用于设备维护和远程监控。
TCP协议作为CPC与工业控制软件之间的正式数据接口。