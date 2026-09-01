# AGENTS.md

本文件只保留代理在本仓库工作的入口规则。详细设计分别放在 `.agents/` 文档中，避免重复和规则漂移。

## 1. 工作原则

- 实现前先核对工程事实；重大歧义必须说明并询问，不凭经验猜测硬件配置。
- 使用最简单、可验证的方案，只修改需求直接涉及的文件，不顺便重构。
- 保留用户已有改动；发现无关问题只报告，不擅自清理。
- 代码改动必须定义成功标准并验证；优先自动化测试，其次编译和可重复的板上验证。
- 数组边界、参数检查、硬件等待超时等嵌入式防御措施不得以“简化”为由省略。

## 2. 项目事实

| 项目 | 当前配置 |
| --- | --- |
| MCU | AT32F435VGT7，Cortex-M4，144 MHz，FPU |
| Device Pack | `ArteryTek.AT32F435_437_DFP.2.2.7` |
| 工具链 | Keil MDK 5，Arm Compiler 6.22 |
| RTOS | FreeRTOS Kernel V10.4.3，`heap_4.c` |
| 调试器 | J-Link |
| MDK 工程 | `project/MDK_V5/voice_checkpointe_app.uvprojx` |
| 输出文件 | `project/MDK_V5/objects/voice_checkpointe_app.hex` |

关键约束：

- Flash 1024 KiB；SRAM 384 KiB；主栈 2 KiB；FreeRTOS 堆 128KB。实际值以工程文件为准。
- 禁止使用 `malloc`/`free`；动态内存只能使用 FreeRTOS 分配接口，并说明所有权、释放时机和失败处理。
- 任务栈必须通过栈高水位验证。UART 中断响应目标小于 10 μs，控制环路目标为 1 kHz。
- `project/` 中厂商生成文件优先只改 user code 区；不得擅自改写生成结构或迁移现有代码。
- libraries和middlewares是厂商文件，基本不用修改，修改前需询问，最好是不修改。

## 3. 构建与烧录

在 `01_Prg` 根目录使用 Git Bash：

```bash
./keil_build_flash.bash -b  # 编译
./keil_build_flash.bash -f  # 烧录
./keil_build_flash.bash -a  # 编译并烧录
```

- 代码或工程配置修改后默认只编译，要求 0 error、0 warning。
- 只有用户明确要求且开发板已连接时才烧录；纯文档修改无需编译。
- Keil 路径无效时报告环境问题，不擅自修改脚本为未经确认的路径。
