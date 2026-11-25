# TTS 重复播报接口计划

MP3 和 TTS 共用现有的一个 `audio_player_service_t`。播放器任务拥有该对象和播放期间的文本。三条语音是业务数据，后续由卡口业务保存，按条目选出文本和次数后提交给播放器。

## App 接口

```c
typedef struct AUDIO_PLAYBACK_VOICE_REQUEST_T {
    bsp_voice_synthesis_encoding_t encoding;
    const uint8_t *p_text;
    uint16_t text_size;
    uint8_t repeat_count;
} audio_playback_voice_request_t;

platform_err_t audio_playback_app_voice_request(
    const audio_playback_voice_request_t *p_request);
```

- 次数 `0` 和 `1` 均播一次，`2～16` 播指定次数，大于 `16` 返回参数错误。
- 保留 `audio_playback_app_voice_speak()` 作为单次播报入口。音量提示继续使用它。
- 请求提交时复制文本到已有的 512 字节播放缓冲。调用方可在返回后复用自己的缓冲。播放期间复用这一份文本，直到整次播报完成或取消。
- 返回 `OK` 表示请求已入队。实际开始、完成和失败由播放器任务处理并记录。停止继续使用 `audio_playback_app_stop()`。

## Service 状态与时间

在现有播放器对象中增加 TTS 播报状态，保存当前文本引用、编码、长度、剩余次数和轮次时间。扩展已有的语音合成 `start` 和 `process`，由 App 传入当前时间，Service 通过周期调用推进状态。

| 事件 | 处理 |
| --- | --- |
| 开始 TTS 请求 | 校验并归一化次数，发送第一轮，播放器进入 TTS 占用状态。 |
| 当前轮收到有效的完成回传 `4F`，且还有剩余次数 | 扣减次数，进入轮间等待。首版采用 35 ms 的非阻塞等待，然后复用原文发下一轮。 |
| 最后一轮完成 | 释放播放器。App 只在此时处理整次播报完成和现有字幕收尾。 |
| 停止、紧急打断、芯片拒绝、收发异常或超时 | 取消剩余轮次，沿用播放器的停止和清理路径。轮间等待也可取消。 |

开始新请求前清理已缓冲的旧回传。本轮收到 `41`（接收成功）或 `4E`（播报中）后才用 `4F` 计数。每轮暂设 180 秒超时，后续按最长文本和最慢语速板测确认。

## 实现顺序与验收

1. 扩展播放器 App 和 Service，用固定 GBK 短句验证 `0`、`1` 各播一次，`3` 播三次。
2. 测试播报中和轮间停止，然后再次播报。超过 16 次的请求应被拒绝。
3. 三轮 TTS 期间尝试 MP3，确认不会抢占。TTS 播完或停止后，再验证 MP3 能正常启动。
4. 基础链路通过后，再接卡口条目、独立字幕、PIR 和播报参数。

板测入口：SW5 短按播“注意安全”三次，双击提交次数 `0`（播一次），长按停止。SW2 仍用于测试 MP3。仅最后一轮打印 `Voice synthesis playback complete`，随后可查看播放器任务的栈高水位。

芯片的命令和轮间时序见 [VTX316 通讯格式](VTX316通讯格式.md)。
