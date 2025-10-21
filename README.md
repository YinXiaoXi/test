# Winlogon Manager Service v2.0

一个用于管理Windows winlogon进程的Windows服务，支持进程暂停/恢复、服务管理等功能。

## 功能特性

- **服务管理**: 安装、卸载、启动、停止、重启Windows服务
- **进程管理**: 暂停、恢复、查询winlogon进程状态
- **IPC通信**: 支持进程间通信，允许多个客户端连接
- **日志记录**: 完整的日志记录系统，支持不同日志级别
- **错误处理**: 完善的错误处理和状态报告
- **线程安全**: 多线程环境下的安全操作

## 系统要求

- Windows 10/11 或 Windows Server 2016+
- Visual Studio 2019+ 或 MinGW-w64
- 管理员权限（用于进程管理操作）

## 编译

### 使用Visual Studio

1. 打开Visual Studio
2. 选择"从现有代码创建项目"
3. 选择C++项目类型
4. 添加所有源文件
5. 配置项目属性：
   - 字符集：Unicode
   - 平台工具集：v143或更高版本
   - 运行时库：多线程(/MT)

### 使用CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 使用方法

### 基本命令

```bash
# 显示帮助信息
service.exe --help

# 安装服务
service.exe --install

# 卸载服务
service.exe --uninstall

# 启动服务
service.exe --start

# 停止服务
service.exe --stop

# 重启服务
service.exe --restart

# 查询服务状态
service.exe --status
```

### Winlogon进程管理

```bash
# 暂停winlogon进程（需要管理员权限）
service.exe --suspend

# 恢复winlogon进程（需要管理员权限）
service.exe --resume

# 查询winlogon进程状态
service.exe --winlogon-status
```

### 作为服务运行

服务安装后会自动以Windows服务形式运行，支持以下功能：

- 自动启动
- 服务控制（启动/停止/重启）
- IPC通信服务器
- 进程管理

## 项目结构

```
├── Common.h              # 公共定义和类型
├── Logger.h/.cpp         # 日志系统
├── Utils.h/.cpp          # 工具函数
├── ServiceManager.h/.cpp # 服务管理
├── ProcessManager.h/.cpp # 进程管理
├── IPCManager.h/.cpp     # IPC通信
├── WinlogonService.h/.cpp # 主服务类
├── main.cpp              # 程序入口
├── CMakeLists.txt        # CMake构建文件
└── README.md             # 说明文档
```

## 错误代码

| 代码 | 说明 |
|------|------|
| 0    | 成功 |
| 1    | 无效参数 |
| 2    | 访问被拒绝 |
| 3    | 服务未找到 |
| 4    | 进程未找到 |
| 5    | IPC连接失败 |
| 999  | 未知错误 |

## 注意事项

1. **管理员权限**: 暂停/恢复winlogon进程需要管理员权限
2. **系统稳定性**: 暂停winlogon进程可能导致系统不稳定，请谨慎使用
3. **服务依赖**: 确保服务在系统启动时正确安装和配置
4. **日志文件**: 默认日志输出到控制台，可通过代码配置输出到文件

## 开发说明

### 添加新功能

1. 在相应的管理器类中添加方法
2. 在`WinlogonService::HandleCommand`中添加命令处理
3. 更新帮助信息
4. 添加相应的测试

### 调试

- 使用`--help`查看所有可用命令
- 检查日志输出了解程序状态
- 使用Windows事件查看器查看服务日志

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 更新日志

### v2.0.0
- 完全重写，采用现代C++设计
- 改进的架构和错误处理
- 增强的日志系统
- 更好的线程安全性
- 支持CMake构建系统