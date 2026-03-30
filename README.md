# CAN-DBC

基于 Qt 和 WebSocket 的 CAN 消息通信系统，支持 DBC 文件解析、信号打包和 Protobuf 序列化。

## 功能特性

- **DBC 文件解析**：支持标准 DBC 文件格式解析
- **信号打包**：支持 Intel（小端）和 Motorola（大端）两种字节序
- **WebSocket 通信**：客户端与服务端通过 WebSocket 进行实时通信
- **Protobuf 序列化**：使用 Protocol Buffers 进行高效的数据序列化
- **信号位重叠检测**：自动检测并警告信号位重叠问题
- **数值范围验证**：对物理值和原始值进行范围检查



## 环境要求

- **操作系统**：Windows 10/11
- **Qt 版本**：Qt 6.x 
- **编译器**：MinGW 
- **Protobuf**：Protocol Buffers 3.21.12

## 安装步骤

### 1. 安装 Qt

1. 访问 [Qt 官网](https://www.qt.io/download)
2. 下载并安装 Qt（推荐 Qt 6.11.0 或更高版本）
3. 安装时选择 MinGW 或 MSVC 编译器
4. 配置环境变量（可选）

### 2. 安装 Protocol Buffers

#### Windows 安装方式：

**方法一：使用预编译版本**

1. 访问 [Protocol Buffers GitHub Releases](https://github.com/protocolbuffers/protobuf/releases)
2. 下载 `protoc-21.12-win64.zip`
3. 解压到 `C:\protobuf-cpp-3.21.12\` 目录
4. 将 `bin` 目录添加到系统 PATH 环境变量

**方法二：从源码编译**

1. 安装 CMake 和 Visual Studio
2. 下载 [Protobuf 源码](https://github.com/protocolbuffers/protobuf/releases/download/v21.12.12/protobuf-cpp-3.21.12.tar.gz)
3. 解压并编译：
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 16 2019" -A x64
   cmake --build . --config Release
   ```

#### 验证安装：

打开命令行，运行：
```bash
protoc --version
```

应该显示：`libprotoc 21.12`

### 3.  DBC 来源
https://github.com/rundekugel/DBC-files

### 4. 编译 Protobuf 文件

在项目目录下运行：

```bash
protoc --cpp_out=. can_message.proto
```

这将生成：
- `can_message.pb.h`
- `can_message.pb.cc`

## 项目构建与运行

### 构建服务端

1. **打开 Qt Creator**
   - 文件 → 打开文件或项目
   - 选择 `server/can_server.pro`

2. **配置项目**
   - 选择构建套件（MinGW 或 MSVC）
   - 点击 "配置项目"

3. **修改 Protobuf 路径**（如果需要）
   
   编辑 `server/can_server.pro`，修改以下路径为你的 Protobuf 安装路径：
   ```qmake
   INCLUDEPATH += C:/protobuf-cpp-3.21.12/protobuf-3.21.12/src
   LIBS += -LC:/protobuf-cpp-3.21.12/protobuf-3.21.12/build -lprotobuf
   ```

4. **构建项目**
   - 点击左下角的 "构建" 按钮（或按 Ctrl+B）
   - 等待编译完成

5. **运行服务端**
   - 点击 "运行" 按钮（或按 Ctrl+R）
   - 服务端窗口将显示启动信息

### 服务端启动方式

服务端通过 `main.cpp` 中的以下代码启动：

```cpp
Server server(8080);
```

**WebSocket 默认地址与端口**：
- **监听地址**：`0.0.0.0`（所有网络接口）
- **默认端口**：`8080`
- **协议**：WebSocket（ws://）

**启动日志示例**：
```
WebSocket server listening on port 8080
```

```

## 使用说明

### 连接服务端

客户端可以通过 WebSocket 连接到服务端：

```
ws://localhost:8080
```


### Protobuf 消息格式

服务端接收的 Protobuf 消息格式定义在 `can_message.proto`：

```protobuf
syntax = "proto3";

package canproto;

message Signal {
  string name = 1;
  double physical_value = 2;
  string unit = 3;
  int64 raw_value = 4;
}

message CanFrame {
  uint32 message_id = 1;
  string message_name = 2;
  repeated Signal signal_list = 3;
}
```

### 示例消息

发送二进制 Protobuf 消息到服务端，服务端将解析并显示：

```
=== CanFrame Parsed ===
Message ID: 0x123 (291)
Message Name: EngineStatus
Signals Count: 3

  Signal 1: RPM
    Physical Value: 2500.0 rpm
    Raw Value: 2500

  Signal 2: Temperature
    Physical Value: 85.5 °C
    Raw Value: 855

  Signal 3: Pressure
    Physical Value: 2.5 bar
    Raw Value: 25
==================================
```

