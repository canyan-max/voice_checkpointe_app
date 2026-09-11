# Platform 层规则

本目录提供板级能力与 MCU 无关接口，同时继承仓库根目录 `AGENTS.md`。

## BSP

- BSP 是板级粘合层，公共文件和函数按能力命名，例如 `bsp_voice_synthesis_*`，不得按当前芯片型号命名。
- BSP 公共头文件只能暴露板级通用类型；不得包含 ExternalChip 驱动头文件或向 Service 暴露芯片私有事件、状态和编码类型。
- 具体芯片对象、协议类型及其与 BSP 类型的映射只允许出现在 BSP `.c` 文件内部。
- BSP 负责绑定逻辑资源、平台接口和 ExternalChip 回调，不负责 RTOS 调度、业务排队或跨音源策略。
- BSP 中由上层周期调用、仅用于收取硬件数据并产生事件的入口使用 `*_poll()` 命名；`*_process()` 留给确实执行协议或业务处理的层。
- 当前板上只有一个固定器件时不引入无意义的 BSP context；ExternalChip 为保持可移植性可以保留 context。

## MCU Interface 与公共类型

- `mcu_interface` 只定义与 MCU 型号无关的 GPIO、UART、I2S、时钟和日志接口。
- 公共资源 ID 使用 `plat_resource.h` 中的统一类型，不在各外设头文件重复 typedef。
- 接口参数不得出现 AT32 外设结构体、DMA 通道枚举或寄存器类型。
- 所有硬件等待必须有超时，ISR 路径禁止阻塞和执行复杂业务。
