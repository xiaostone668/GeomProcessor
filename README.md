# GeomProcessor - 几何数据处理器

GeomProcessor是一个基于OpenCASCADE的几何数据处理工具，与SimulationTool配合使用，提供高级的几何编辑和修复功能。

## 功能特性

### 核心功能
- **STEP文件加载/保存**：支持STEP格式的3D模型文件
- **Shell缝合**：将多个Shell合并为实体，支持自定义公差
- **形状修复**：自动修复几何体中的缺陷和不一致性
- **面删除**：删除选定面并自动愈合缺口
- **偏移操作**：对几何体进行偏移操作

### 界面功能
- **3D视图**：基于OpenCASCADE 3D渲染
- **面列表**：显示所有面的列表，支持多选
- **实时统计**：左侧显示面、Shell、实体数量
- **操作面板**：右侧提供所有操作的参数配置

### IPC通信
- 支持与SimulationTool通过共享内存进行通信
- 接收来自SimulationTool的几何数据
- 处理完成后自动将结果返回

## 系统要求

- **操作系统**：Windows 10/11
- **编译器**：MSVC 2015/2017/2019/2022
- **Qt**：5.6.3 或更高版本
- **OpenCASCADE**：7.7.0
- **CMake**：3.16 或更高版本

## 编译指南

### 1. 环境准备

确保已安装以下软件：
- Visual Studio（包含MSVC编译器）
- Qt 5.6.3
- OpenCASCADE 7.7.0

### 2. 编译步骤

```bash
# 克隆仓库
git clone https://github.com/xiaostone668/GeomProcessor.git
cd GeomProcessor

# 创建build目录
mkdir build
cd build

# 配置CMake（根据你的Qt和OCCT路径调整）
cmake .. -DQt5_DIR="D:/Qt/Qt5.6.3/5.6.3/msvc2015_64/lib/cmake/Qt5"

# 编译（Release模式）
cmake --build . --config Release
```

### 3. CMake配置

如果需要修改第三方库路径，编辑`CMakeLists.txt`中的以下变量：

```cmake
# Qt 5路径
set(Qt5_DIR "D:/Qt/Qt5.6.3/5.6.3/msvc2015_64/lib/cmake/Qt5")

# OpenCASCADE 7.7.0路径
set(OCC_ROOT "D:/OpenCASCADE-7.7.0")
```

## 使用方法

### 启动应用

```bash
# 直接运行编译后的可执行文件
cd build/Release
GeomProcessor.exe
```

### 主要操作

#### 1. 加载STEP文件
- **菜单**：文件 → 打开STEP文件
- **工具栏**：📂 打开按钮

#### 2. Shell缝合
- **菜单**：操作 → 缝合Shell
- **工具栏**：🔗 缝合按钮
- **操作面板**：设置公差参数，点击"执行缝合"
- **默认公差**：0.01

缝合完成后会显示统计信息：
```
缝合完成 | 89个Shell已合并为1个Shell | 352个面 | 12个实体 | 已自动保存并发送
```

#### 3. 形状修复
- **菜单**：操作 → 修复形状
- **工具栏**：🔧 修复按钮
- **操作面板**：设置精度，点击"执行修复"

#### 4. 删除面
- 在左侧面列表中选择要删除的面
- **菜单**：操作 → 删除选中面
- **工具栏**：✂ 删除面按钮

#### 5. 偏移操作
- **菜单**：操作 → 偏移
- **工具栏**：📐 偏移按钮
- **操作面板**：设置偏移量，点击"执行偏移"

#### 6. 发送结果到SimulationTool
- **菜单**：操作 → 发送结果到SimulationTool
- **工具栏**：📤 发回按钮
- **自动发送**：缝合成功后会自动发送

## IPC通信协议

GeomProcessor与SimulationTool通过共享内存通信，使用以下协议：

### 共享内存配置
- **Key**：`"GeomIPC_v1"`
- **大小**：2048字节

### 命令类型
- `CMD_SEND_GEOM(1)`：SimulationTool发送新几何
- `CMD_PROCESSING(2)`：正在处理
- `CMD_RESULT_READY(3)`：结果已准备好
- `CMD_ERROR(4)`：发生错误

### 数据交换流程
1. SimulationTool将几何写入STEP临时文件
2. 将文件路径写入共享内存，设置cmd为CMD_SEND_GEOM
3. GeomProcessor读取文件，进行处理
4. GeomProcessor将结果写入新的STEP文件
5. 将结果路径写入共享内存，设置cmd为CMD_RESULT_READY
6. SimulationTool读取结果并更新视图

## 项目结构

```
GeomProcessor/
├── include/              # 头文件
│   ├── OccViewWidget.h   # 3D视图组件
│   ├── GeomIPC.h         # IPC协议定义
│   ├── GeomReceiver.h    # IPC接收器
│   ├── GeomProcessor.h   # 几何处理核心
│   └── GeomProcessorWindow.h # 主窗口
├── src/                  # 源文件
│   ├── main.cpp          # 程序入口
│   ├── OccViewWidget.cpp
│   ├── GeomReceiver.cpp
│   ├── GeomProcessor.cpp
│   └── GeomProcessorWindow.cpp
├── example/              # 示例文件目录
│   └── returned/         # 处理结果存储
├── CMakeLists.txt        # CMake配置
└── README.md            # 本文件
```

## 技术细节

### 使用的OpenCASCADE库
- **核心库**：TKernel, TKMath, TKBRep, TKGeomBase, TKGeomAlgo
- **几何库**：TKG3d, TKG2d, TKTopAlgo, TKPrim
- **视图库**：TKV3d, TKService, TKOpenGl
- **STEP库**：TKSTEP, TKSTEPAttr, TKSTEPBase, TKSTEP209
- **修复库**：TKShHealing
- **布尔库**：TKBO, TKBool
- **偏移库**：TKOffset, TKFeat

### STEP保存模式
使用`STEPControl_ManifoldSolidBrep`模式保存STEP文件，确保：
- 所有几何数据完整保存
- 面的拓扑关系正确
- 实体的封闭性保持

### Shell统计功能
- `numShells()`：统计当前几何体中的Shell总数
- `numSewnShells()`：获取缝合操作中合并的Shell数量
- 实时显示在状态栏和左下角悬浮信息中

## 编译选项

### UTF-8编码支持
项目已启用`/utf-8`编译选项，确保中文字符正确显示。

### MOC/UIC/RCC
自动处理Qt的元对象编译、UI编译和资源编译。

## 故障排除

### 常见问题

1. **找不到Qt**
   - 确保`Qt5_DIR`变量指向正确的路径
   - 路径包含`lib/cmake/Qt5`子目录

2. **找不到OpenCASCADE库**
   - 确保`OCC_ROOT`变量指向OpenCASCADE 7.7.0根目录
   - 检查库文件是否存在于`opencascade-7.7.0/win64/vc14/lib`

3. **运行时缺少DLL**
   - 确保OpenCASCADE的bin目录在PATH中
   - 检查Qt DLL是否在可执行文件目录中

4. **中文乱码**
   - 确保编译时使用了`/utf-8`选项
   - 检查源文件编码是否为UTF-8

5. **IPC通信失败**
   - 确保SimulationTool正在运行
   - 检查共享内存key是否匹配

## 参考链接

- [OpenCASCADE](https://dev.opencascade.org/)
- [Qt](https://www.qt.io/)
- [STEP文件格式](https://en.wikipedia.org/wiki/ISO_10303)

## 许可证

本项目采用MIT许可证。

## 贡献

欢迎提交Issue和Pull Request！

## 作者

xiaostone668

## 更新日志

### v1.0.0 (2026-02-26)
- ✅ 添加Shell统计功能
- ✅ 优化缝合信息显示
- ✅ 缝合默认参数改为0.01
- ✅ 修复数据传输完整性（使用ManifoldSolidBrep模式）
- ✅ 添加自动保存和发送功能
- ✅ 支持Qt 5.6.3和OpenCASCADE 7.7.0
- ✅ 启用UTF-8编码支持中文
- ✅ 修复与SimulationTool的IPC通信