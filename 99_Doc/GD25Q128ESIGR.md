# QSPI芯片-GD25Q128ESIGR

## 1.本机硬件原理配置

## 1.1 通讯方式

* QSPI
* 硬件片选（若不行换软件CS），好像无法更换软件片选QSPI
* QSPI模式0，暂未显示配置DMA或EDMA，调试用的阻塞方式,先看硬件情况如何
* MCU QSPI时钟频率12MHZ-12分频
* MSB最高有效位
* Sector Erase Time min:45 max:300 ms
* Block Erase Time (32K Bytes) min:0.15 max:1.2 s
* Page Programming Time min:0.5 max:2.4 ms
* Write Status Register Cycle Time min:5 max:30 ms
* 

## 1.2 硬件接线方式和芯片通讯格式

### 1.2.1 板上链接方式和芯片特性

* MCU<-(QSPI1)->GD25Q128ESIGR
* SCLK-初始化上电后低电平-正确-模式0
* VCC-3.3v-正确
* CS#-接MCU->10K电阻-上拉3.3v-电平信号3.3v-正确
* WP#-接MCU->10K电阻-上拉3.3v-电平信号3.3v-正确
* MISO(IO1)->低电平-空闲下
* MOSI(IO0)->低电平-空闲下
* HOLD#-接MCU->10K电阻-上拉3.3v-电平信号3.3v-正确

### 1.2.2 板上链接方式和芯片特性

* 容量：128M-bit，16M-Byte

* 总page:256,bolock:0~255,sector:0~4095

* block  :0 :sector 0~15

* sector :0 address range:000000H~000FFFH address 24bit:0b000000000000000000000000~0b000000000000111111111111

* block  :255 :sector 4080~4095

* sector :4080 address range:FF0000H~FF0FFFH address 24bit

* 24bit:111111110000000000000000~111111110000111111111111

* GD25Q128E 在使用“Quad Output Fast Read”和“Quad I/O Fast Read”（6BH，EBH）命令时，支持 Quad SPI 操作。这些命令允许以标准 SPI 四倍的速度向设备传输数据或从设备读取数据。使用 Quad SPI 命令时，SI 和 SO 引脚变为双向 I/O 引脚：IO0 和 IO1；WP# 和 HOLD#/reset# 引脚也变为双向 I/O 引脚：IO2 和 IO3。使用 Quad SPI 命令时，状态寄存器中的非易失性 Quad Enable 位（QE）需设置为 1

* 支持模式标准SPI，DUAL，QSPI

* 最大工作频率：133MHZ(3.0v~3.6v),104MHZ(2.7v~3.0v)

* DUAL-266Mbits/s

* QUAD-532Mbits/s

* read status register adr : 05H(s7~s0) 35H(s15~s8) 15H(s23~s16)

* QE:register in 35H (s9 bit)-支持掉电保存

* DC:register in 15H(s16 bit)-支持掉电保存

  Command    DC bit       Numbers of Dummy Cycles       Freq.(MHz)

  BBH		0 (default)  4 							104

  ​		   1 		   8 							133R

  EBH		0 (default)  6 							104

  ​		   1 		   10 						   133R

* write status register adr : 01H(s7~s0) 31H(s15~s8) 11H(s23~s16)

* write enable register adr :06h

* write disable register adr :04h

* Quad output fast read :6BH

* read data bytes :03H

* Quad page program :32H

* sector erase :20H

* read device id: 90H

