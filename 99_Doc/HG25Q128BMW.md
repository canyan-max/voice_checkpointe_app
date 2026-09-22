# QSPI Flash：HG25Q128BMW

## 1. 器件确认

- 板上实际丝印：`HG25Q128BMW`
- 厂商：HGSEMI（华冠）
- 系列：HG25Q128B
- 容量：128 Mbit，即 16 MiB
- 工作电压：2.7 V～3.6 V，板上使用 3.3 V
- 支持 SPI Mode 0 和 Mode 3；本工程使用 Mode 0
- 支持 Single、Dual、Quad I/O，以及 QPI（4-4-4）模式
- 数据手册标称最高时钟：133 MHz；本工程稳定配置使用 48 MHz（144 MHz / 3）

板上实测识别结果：

```text
RDID  0x9F -> C2 20 18
REMS  0x90 -> C2 17
RDSR  0x05 -> 00
RDCR  0x15 -> 07
```

上述 ID 与 HG25Q128B 数据手册一致。

> 注意：该器件虽然使用 `C2 20 18`，但应以 HG25Q128B 数据手册为准，不能直接套用 GD25Q128E 的状态寄存器和命令定义。

## 2. 板级连接与当前配置

### 2.1 MCU 外设

- MCU：AT32F435VGT7
- 外设：QSPI1
- 片选：QSPI1 硬件片选
- 时钟模式：Mode 0，SCK 空闲为低电平
- 位序：MSB first
- 当前 QSPI 时钟：144 MHz / 3 = 48 MHz
- 当前传输方式：命令端口轮询阻塞，不使用 DMA/EDMA

### 2.2 信号连接

| Flash 信号 | QSPI 含义 | 板上说明 |
| --- | --- | --- |
| CS# | 硬件片选 | 10 kΩ 上拉到 3.3 V |
| SCLK | 串行时钟 | Mode 0，空闲低电平 |
| SI/SIO0 | IO0 | Single 模式下为输入，Quad 模式下为双向 |
| SO/SIO1 | IO1 | Single 模式下为输出，Quad 模式下为双向 |
| WP#/SIO2 | IO2 | 10 kΩ 上拉到 3.3 V；Quad 模式下为双向 |
| RESET#/SIO3 | IO3 | 10 kΩ 上拉到 3.3 V；Quad 模式下为双向 |
| VCC | 电源 | 3.3 V |

## 3. 存储组织

| 项目 | 大小 | 数量 |
| --- | ---: | ---: |
| 总容量 | 16 MiB（0x01000000） | 1 |
| Page | 256 Bytes | 65,536 |
| Sector | 4 KiB | 4,096 |
| 32 KiB Block | 32 KiB | 512 |
| 64 KiB Block | 64 KiB | 256 |

- 有效地址范围：`0x000000`～`0xFFFFFF`
- 地址长度：24 bit（3 Bytes）
- Page Program 一次最多写入 256 Bytes，且不得跨越 256 Bytes 页边界。
- Sector 0 地址范围：`0x000000`～`0x000FFF`。
- Sector 4095 地址范围：`0xFFF000`～`0xFFFFFF`。

## 4. 关键命令

### 4.1 识别和寄存器

| 功能 | 命令 | 地址/数据 | 说明 |
| --- | ---: | --- | --- |
| Read Identification | `0x9F` | 读 3 Bytes | 返回 `C2 20 18` |
| Read Electronic Manufacturer/Device ID | `0x90` | 3-Byte 地址后读 2 Bytes | 地址为 0 时返回 `C2 17` |
| Read SFDP | `0x5A` | 3-Byte 地址 + Dummy | 用于读取可发现参数 |
| Read Status Register | `0x05` | 读 1 Byte | WIP 位为 bit0，WEL 位为 bit1，QE 位为 bit6 |
| Read Configuration Register | `0x15` | 读 1 Byte | 当前板上读回 `0x07` |
| Write Status/Configuration Register | `0x01` | 写 1～2 Bytes | 执行前必须先发送 WREN |
| Write Enable | `0x06` | 无数据 | 置位 WEL |
| Write Disable | `0x04` | 无数据 | 清除 WEL |

### 4.2 数据读取

| 功能 | 命令 | 传输模式 |
| --- | ---: | --- |
| Normal Read | `0x03` | 1-1-1 |
| Fast Read | `0x0B` | 1-1-1，带 Dummy |
| Dual Output Read | `0x3B` | 1-1-2 |
| Dual I/O Read | `0xBB` | 1-2-2 |
| Quad Output Read | `0x6B` | 1-1-4 |
| Quad I/O Read | `0xEB` | 1-4-4 |

Quad 读取使用的 Dummy Cycle 数量受配置寄存器 DC1/DC0 影响，实现驱动时必须按照当前配置选择，不能固定套用其他型号参数。

### 4.3 编程与擦除

| 功能 | 命令 | 范围/说明 |
| --- | ---: | --- |
| Page Program | `0x02` | Single 输入，1～256 Bytes |
| Quad Page Program | `0x38` | Quad 输入，1～256 Bytes |
| Sector Erase | `0x20` | 4 KiB |
| Block Erase | `0x52` | 32 KiB |
| Block Erase | `0xD8` | 64 KiB |
| Chip Erase | `0x60` 或 `0xC7` | 整片擦除，禁止用于普通板测 |

编程和擦除的基本流程：

1. 检查目标地址、长度和页/扇区边界。
2. 发送 `WREN (0x06)`。
3. 读取状态寄存器，确认 WEL 已置位。
4. 发送编程或擦除命令。
5. 轮询 `RDSR (0x05)` 的 WIP bit0，必须带总超时。
6. 操作结束后读回校验。

## 5. 状态、配置与 Quad 模式注意事项

- 状态寄存器：
  - bit0：WIP，`1` 表示编程/擦除/写寄存器进行中。
  - bit1：WEL，`1` 表示已写使能。
  - bit6：QE，使用 Quad Read/Quad Program 前需要置位。
- 配置寄存器：
  - bit7～bit6：DC1/DC0，控制部分高速读取命令的 Dummy Cycle。
  - bit4：PBE。
  - bit3：TB，为非易失 OTP 配置，修改前必须确认保护策略。
  - bit1～bit0：ODS1/ODS0，输出驱动强度。
- 修改状态/配置寄存器可能影响写保护、Quad 模式和时序，初期调试阶段不主动修改。

### 5.1 `0x35` 的高风险差异

`HG25Q128B` 的 `0x35` 是 **Enable QPI（EQIO）** 命令，不是读取 Status Register 2。

- 发送 `0x35` 后，器件进入 4-4-4 QPI 模式。
- 退出 QPI 使用 `RSTQIO (0xF5)`，该命令需要在 QPI 模式下发送。
- 完全断电重新上电也会恢复标准 SPI 状态。
- 驱动中禁止把 `0x35` 当作 GD25Q128E 的“读 SR2”使用。

## 6. 当前工程实现边界

- `plat_qspi` 只提供 MCU 无关的命令、读和写事务接口。
- AT32 实现使用 QSPI 命令端口轮询，并对整个事务设置总超时。
- `HG25Q128B` ExternalChip 驱动负责器件识别、QE、WREN/WEL、WIP 总超时、页拆分和 4 KiB 扇区擦除。
- 正式读取路径使用 `0x6B + 8 Dummy Cycle + 1-1-4`；编程和擦除仍使用 1-1-1，不进入 4-4-4 QPI。
- `bsp_storage_flash` 只向上提供通用存储 Flash 能力，不暴露 HG25Q128B 私有类型或命令。
- 当前接口为轮询阻塞方式且只有一个板载实例；调用者需要串行化并发访问。DMA 留到后续连续大块读取存在明确性能需求时再评估。
- 当前 `start_or_test_f` 通过 `bsp_storage_flash` 完成初始化和写入/读取验证，不再直接拼装芯片命令。
- 临时写入测试使用末尾扇区 `0xFFF000`：只有确认整个 4 KiB 扇区均为 `0xFF` 后才允许写入；测试完成后擦除该扇区并再次确认全为 `0xFF`。
- 如果末尾扇区存在任何数据，测试必须中止，禁止擦除未知内容。
- 如果检测到的非空内容逐字节符合本测试的确定性数据模式（未写入部分允许保持 `0xFF`），可判定为中断遗留的测试数据，自动擦除恢复后继续；任何不匹配的数据仍按未知内容保护。
- 四线读取测试会写满该测试扇区的 16 个 Page，然后使用 `0x6B + 8 Dummy Cycle + 1-1-4` 重复读取 1,024 遍（累计 4 MiB）逐字节校验，最后擦除恢复。
- 测试初始化只在 QE bit6 未置位时，通过 `WRSR (0x01)` 保留其他状态位并置位 QE；不发送 `0x35`，不进入 4-4-4 QPI 模式。

## 7. 资料来源

- HG25Q128B 数据手册：<https://atta.szlcsc.com/upload/public/pdf/source/20240606/CC57811BFF4A30B2CF2185F84B7C008B.pdf>
- HG25Q128BMW/TR 产品页：<https://item.szlcsc.com/24141302.html>
