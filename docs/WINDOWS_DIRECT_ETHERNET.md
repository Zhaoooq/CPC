# Windows 与树莓派网线直连

## 需要的硬件

- 一根普通 RJ45 网线。树莓派 5 和现代 Windows 网卡通常支持自动翻转，不需要交叉线。
- 树莓派原有的独立电源。网线只负责通信，不能为树莓派供电。

## 1. 配置 Windows 有线网卡

1. 将网线连接到树莓派和 Windows 电脑。
2. 打开“设置 → 网络和 Internet → 高级网络设置 → 更多网络适配器选项”。
3. 右键正在使用的“以太网”，选择“属性”。
4. 双击“Internet 协议版本 4 (TCP/IPv4)”。
5. 选择“使用下面的 IP 地址”，填写：

```text
IP 地址：   192.168.50.1
子网掩码：  255.255.255.0
默认网关：  留空
DNS：       留空
```

Windows 显示“未识别的网络”或“无 Internet”属于正常现象，这条网线只建立本地数据链路。

## 2. 配置树莓派有线网卡

以下命令适用于使用 NetworkManager 的 Raspberry Pi OS。先查看连接名称：

```bash
nmcli -t -f NAME,DEVICE connection show --active
```

找到设备为 `eth0` 的连接名称。例如连接名称为 `Wired connection 1`，执行：

```bash
sudo nmcli connection modify "Wired connection 1" \
  ipv4.method manual \
  ipv4.addresses 192.168.50.2/24 \
  ipv4.gateway "" \
  ipv4.dns ""
sudo nmcli connection up "Wired connection 1"
```

连接名称必须以第一条命令的实际输出为准。配置后检查：

```bash
ip -4 address show dev eth0
```

应当能看到 `192.168.50.2/24`。

## 3. 编译并启动 CPC

工程新增了 Qt Network 依赖。在树莓派上编译：

```bash
cd /home/pi/Desktop/CPC
qmake CPC_1.pro
make -j2
./CPC_1
```

当前树莓派若尚未安装 Qt 5 开发工具，可先执行：

```bash
sudo apt-get install qtbase5-dev qt5-qmake
```

再重新运行 qmake。CPC 开机自启动配置仍会使用同一个 `CPC_1` 可执行文件。

## 4. 从 Windows 访问

先打开 PowerShell 检查链路：

```powershell
ping 192.168.50.2
Test-NetConnection 192.168.50.2 -Port 8080
```

然后使用 Edge 或 Chrome 打开：

```text
http://192.168.50.2:8080/
```

页面每秒获取一次最新快照，显示当前浓度、采集状态和最近 600 个有效数据点（约 10 分钟）。
点击页面右上角“保存数据”开始记录这一段看板数据，按钮变为“停止保存”后再次点击即可将本段 CSV 下载到 Windows 本地目录；如果浏览器没有弹出保存位置，请在 Edge/Chrome 的下载设置中开启“每次下载前询问保存位置”。
树莓派主程序内部最多保留 3600 个看板数据点，不会自动保存到磁盘。

## 常见问题

### 可以 ping 通，但端口 8080 不通

- 确认新版本 `CPC_1` 已经编译并正在运行。
- 在树莓派执行 `ss -ltn | grep ':8080'`，应看到监听地址 `0.0.0.0:8080`。
- 检查树莓派防火墙是否阻止 TCP 8080。
- 检查是否已有其他程序占用了 8080 端口。

### 页面打开，但没有曲线

只有 CPC 开始 OPC 采集、累计出第一组有效浓度后才会出现曲线。页面的“采集状态”可以区分
“连接正常但尚未采集”和“网络连接断开”。

### Windows 同时需要访问互联网

保留 Wi-Fi 用于互联网，把有线网卡专门用于 `192.168.50.0/24`。由于直连有线网卡没有填写
默认网关，一般不会抢占 Windows 的互联网默认路由。
