# 医疗滤芯气密性测试系统 - 规格说明书

## 1. 项目概述

**项目名称**: MedicalFilterTester - 医疗滤芯气密性测试系统

**项目类型**: Qt5.14 Windows 桌面应用程序

**核心功能概述**: 用于测试医疗滤芯气密性的专业测试系统，支持扩散流测试、泡点测试等多种测试方法，系统自动控制测试流程、计算测试结果并生成Excel报表。

**目标用户**: 医疗滤芯生产厂商的质量检测人员、工厂操作员

**目标平台**: Windows 触摸屏工控机

---

## 2. UI/UX 规格

### 2.1 窗口模型

| 窗口 | 类型 | 描述 |
|------|------|------|
| 登录窗口 | 主窗口 | 启动时显示，包含用户名/密码输入和登录按钮 |
| 主窗口 | 主窗口 | 登录成功后显示，左侧导航栏+右侧内容区 |
| 弹出键盘 | 子窗口 | 点击输入框时显示的虚拟键盘 |
| 测试进行窗口 | 对话框 | 显示测试进度和实时数据 |

### 2.2 视觉设计

**颜色方案**:
- 主色调: `#2196F3` (医疗蓝色)
- 次要色: `#1976D2` (深蓝色)
- 强调色: `#4CAF50` (合格绿色), `#F44336` (不合格红色)
- 背景色: `#F5F5F5` (浅灰色)
- 卡片背景: `#FFFFFF` (白色)
- 文字主色: `#212121` (深灰)
- 文字次色: `#757575` (中灰)

**字体**:
- 主字体: "Microsoft YaHei UI", 10pt
- 标题字体: "Microsoft YaHei UI", 14pt, QFont::Bold
- 按钮字体: "Microsoft YaHei UI", 11pt

**间距系统**:
- 基础间距: 10px
- 内边距: 15px
- 卡片间距: 20px
- 按钮高度: 45px (触摸屏友好)

**视觉效果**:
- 按钮: 圆角 8px, hover时加深10%背景色
- 卡片: 圆角 10px, 带轻微阴影 `QGraphicsDropShadowEffect`
- 输入框: 圆角 5px, 边框 2px solid #E0E0E0
- 进度条: 圆角 10px, 渐变填充

### 2.3 组件规格

#### 登录窗口布局
```
+----------------------------------+
|        [系统Logo/标题]            |
|                                  |
|    用户名: [_______________]     |
|    密码:   [_______________]     |
|                                  |
|         [登  录  按  钮]         |
|                                  |
+----------------------------------+
```
- 窗口尺寸: 480x640 像素
- 居中显示，带背景图片/渐变

#### 主窗口布局
```
+----------------+----------------------------------------+
|   [系统标题]   |                                        |
+----------------+                                        |
|                |                                        |
| [滤芯规格参数]  |         [右侧内容区域]                  |
|                |                                        |
| [进行测试]     |    根据左侧选择显示不同界面:              |
|                |    - 规格参数输入界面                    |
| [导出报表]     |    - 测试选择界面                        |
|                |    - 测试进行界面                        |
| [历史记录]     |    - 历史记录界面                        |
|                |                                        |
| [系统设置]     |                                        |
|                |                                        |
+----------------+----------------------------------------+
|            [状态栏: 当前用户/时间]                       |
+----------------+----------------------------------------+
```
- 左侧导航栏宽度: 200px
- 导航按钮高度: 60px
- 使用QStackedWidget管理右侧页面切换

#### 虚拟键盘
```
+----------------------------------------------------------+
|  [Q] [W] [E] [R] [T] [Y] [U] [I] [O] [P]                  |
|   [A] [S] [D] [F] [G] [H] [J] [K] [L]                    |
|  [SHIFT] [Z] [X] [C] [V] [B] [N] [M] [BACKSPACE]         |
|  [123] [SPACE_______________] [ENTER]                    |
+----------------------------------------------------------+
```
- 键盘按键尺寸: 50x50px
- 触摸屏友好间距

---

## 3. 功能规格

### 3.1 登录模块

**功能**:
- 用户名输入 (支持键盘弹窗)
- 密码输入 (支持键盘弹窗, 显示为***)
- 登录验证 (预设用户: admin/admin123)
- 登录成功跳转主界面
- 登录失败显示错误提示

### 3.2 滤芯规格参数输入模块

**输入字段**:
| 字段名 | 类型 | 单位 | 说明 |
|--------|------|------|------|
| 滤芯型号 | 字符串 | - | 如: HF-2540 |
| 滤芯面积 | 浮点数 | cm² | 有效过滤面积 |
| 测试压力(Pc) | 浮点数 | mbar | 标准测试压力 |
| 扩散流阈值(COA) | 浮点数 | ml/min | 厂家给定合格阈值 |
| 测试时间(T) | 整数 | 秒 | 保压测试持续时间 |
| 罐体积(Vg) | 浮点数 | ml | 500ml气罐 |

**功能**:
- 参数持久化存储到SQLite
- 支持修改已有参数
- 参数校验 (数值范围检查)

### 3.3 测试方法选择模块

**可用测试**:
| 测试名称 | 说明 |
|----------|------|
| 扩散流测试 | 气体扩散流量检测 |
| 泡点测试 | 气泡点压力检测 |
| 保压测试 | 密闭性保压检测 |
| 水侵入测试 | (预留) |

**界面**:
- 显示测试方法按钮列表
- 点击后进入对应测试界面

### 3.4 扩散流测试模块 (核心功能)

**测试流程** (根据提供的markdown算法):

#### 阶段1: 气源检测
- 检测P1压力是否 > 测试压力
- 开启V1阀，保持15秒后关闭

#### 阶段2: 气路密闭检测
- 关闭V1后开始
- 检测P1压力在时间段内是否保持不变
- 开启V2，检测P2压力是否在时间段内保持不变

#### 阶段3: 计算体积Vs
- 公式: `Vs = Vg * (P11 - P10) / (P21 - P20)`
- 步骤:
  1. 开V1给气罐充气，P1=测试压力Pc-200后关V1
  2. 等待稳定，记录P10、P20
  3. 开V2，等待稳定，记录P11、P21
  4. 计算Vs，关V2

#### 阶段4: 保压测试
- 开V3，开V1充气，P1=Pc+2000后关V1，记录P12
- 开V2，经过时间 `t = -Vs/C * ln((P12-Pc)/(P12-P23))`
- 几秒后判断P2是否满足Pc±10，不足则开V2充气，多则开V5排气
- 保持测试时间T

#### 阶段5: 计算扩散流
- 公式: `Q = (P24 - P23) * Vs / (T * 1013.25)`
- 每隔一定时间段计算一次Q

#### 阶段6: 合格判定
- Q < COA 则合格

**UI显示**:
- 进度条 (0-100%)
- 当前阶段文字显示
- 实时压力数据 (P1, P2)
- 实时计算的Q值
- 测试结果 (合格/不合格)

### 3.5 导出Excel报表模块

**报表内容**:
- 滤芯规格参数
- 测试日期时间
- 测试方法
- 测试数据 (P1, P2, Q值等)
- 测试结果
- 操作员

**功能**:
- 选择历史记录导出
- 生成.xlsx文件
- 自动打开文件对话框保存

### 3.6 历史记录模块

**功能**:
- 显示历史测试记录列表
- 支持按日期、滤芯型号筛选
- 支持查看详细测试数据
- 支持删除记录

---

## 4. 数据模型

### 4.1 SQLite数据库表

**users表**:
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**filter_params表**:
```sql
CREATE TABLE filter_params (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    model TEXT NOT NULL,
    area REAL NOT NULL,
    test_pressure REAL NOT NULL,
    coa_threshold REAL NOT NULL,
    test_duration INTEGER NOT NULL,
    tank_volume REAL NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**test_results表**:
```sql
CREATE TABLE test_results (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    filter_param_id INTEGER NOT NULL,
    test_method TEXT NOT NULL,
    test_data TEXT NOT NULL,
    result TEXT NOT NULL,
    operator TEXT NOT NULL,
    test_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (filter_param_id) REFERENCES filter_params(id)
);
```

---

## 5. 技术架构

### 5.1 目录结构
```
MedicalFilterTester/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── MainWindow.h/cpp
│   ├── LoginWindow.h/cpp
│   ├── VirtualKeyboard.h/cpp
│   ├── Pages/
│   │   ├── ParamInputPage.h/cpp
│   │   ├── TestSelectionPage.h/cpp
│   │   ├── DiffusionFlowTestPage.h/cpp
│   │   ├── HistoryPage.h/cpp
│   │   └── ExportPage.h/cpp
│   ├── Database/
│   │   └── DatabaseManager.h/cpp
│   ├── TestEngine/
│   │   ├── TestEngineBase.h/cpp
│   │   └── DiffusionFlowTest.h/cpp
│   └── Export/
│       └── ExcelExporter.h/cpp
├── include/
└── resources/
    └── qss/
```

### 5.2 依赖库
- Qt 5.14.x
- Qt SQL (SQLite)
- Qt Xlsx (用于Excel导出, 或使用QAxObject)

---

## 6. 验收标准

### 6.1 功能验收
- [ ] 登录界面正确显示，可通过键盘输入用户名密码
- [ ] 登录验证正确，admin/admin123可登录
- [ ] 主界面左侧导航栏显示5个按钮
- [ ] 规格参数输入界面可正确输入和保存参数
- [ ] 测试选择界面显示所有测试方法
- [ ] 扩散流测试按流程执行，进度条正确更新
- [ ] 当前阶段文字正确显示
- [ ] 测试完成后结果正确判定
- [ ] Excel报表导出成功
- [ ] 历史记录正确显示和筛选

### 6.2 UI验收
- [ ] 触摸屏按钮大小≥45px
- [ ] 键盘弹出位置合适，不遮挡输入框
- [ ] 进度条动画流畅
- [ ] 颜色方案符合医疗专业风格

### 6.3 数据验收
- [ ] 参数保存后重启程序仍存在
- [ ] 测试结果正确存入数据库
- [ ] 报表数据与测试数据一致
