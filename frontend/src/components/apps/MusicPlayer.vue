<script setup lang="ts">
import { ref, reactive, onMounted, onUnmounted, computed, watch } from 'vue';
import { processAPI, deviceAPI } from '@/services/api';

interface Song {
  id: string;
  title: string;
  artist: string;
  duration: number;
  file: string;
  format: string;
}

interface AudioDevice {
  device_id: number;
  name: string;
  type: string;
  status: string;
  current_user: number | null;
}

// 音乐播放状态
const playerState = reactive({
  isPlaying: false,
  currentSong: null as Song | null,
  currentTime: 0,
  volume: 0.7,
  isMuted: false,
  repeatMode: 'off' as 'off' | 'one' | 'all',
  shuffleMode: false,
  currentDevice: null as AudioDevice | null
});

// 音乐列表和音频相关
const songList = ref<Song[]>([]);
const audio = ref<HTMLAudioElement | null>(null);
const isLoading = ref(true);
const error = ref<string | null>(null);
const devices = ref<AudioDevice[]>([]);
const processId = ref<number | null>(null);
const showDeviceList = ref(false);

// 创建进程
const createMusicProcess = async () => {
  try {
    const response = await processAPI.createProcess(4096, 1000, 3); // 4KB内存，1000ms CPU时间，优先级3
    if (response.data.status === 'success') {
      processId.value = response.data.data.pid;
      console.log('音乐播放器进程已创建，PID:', processId.value);
    }
  } catch (err: any) {
    console.error('创建音乐播放器进程失败:', err);
    console.warn('进程创建失败，将使用默认音频播放模式');
    // 不设置错误，允许音乐播放器继续工作
  }
};

// 加载音乐列表
const loadMusicList = async () => {
  try {
    // 模拟从resources/music目录加载音乐文件
    const musicFiles = [
      'Idina Menzel - Let It Go.ogg',
      '周深 - 蜃楼.mp3',
      '孙楠和周深 - 相见不如怀念.ogg',
      'Faouzia - RIP, Love.ogg',
      '陈奕迅 - 这样很好.ogg',
      '林俊杰 - 愿与愁.mp3',
      '周杰伦 - 最伟大的作品.mp3',
      '周深 - 少管我.mp3'
    ];

    songList.value = musicFiles.map((file, index) => {
      const [artist, titleWithExt] = file.split(' - ');
      const title = titleWithExt.replace(/\.(mp3|ogg|wav|m4a)$/, '');
      const format = titleWithExt.split('.').pop() || 'mp3';

      return {
        id: `song-${index}`,
        title,
        artist,
        duration: Math.floor(Math.random() * 300) + 180, // 随机3-8分钟
        file: `/resources/music/${file}`,
        format
      };
    });

    console.log('音乐列表加载完成:', songList.value.length, '首歌曲');
  } catch (err: any) {
    console.error('加载音乐列表失败:', err);
    // 即使加载失败，也不设置全局错误，只记录日志
    console.warn('音乐列表加载失败，但播放器仍可使用');
  } finally {
    isLoading.value = false;
  }
};

// 加载音频设备
const loadAudioDevices = async () => {
  try {
    const response = await deviceAPI.getDevices();
    if (response.data.status === 'success') {
      // 筛选出音频相关设备（名称包含耳机、音响等关键词或类型为AUDIO）
      const audioKeywords = ['耳机', '音响', '音箱', '喇叭', 'speaker', 'headphone', 'audio', 'sound'];
      devices.value = response.data.data.filter((device: AudioDevice) =>
        device.type === 'AUDIO' ||
        audioKeywords.some(keyword =>
          device.name.toLowerCase().includes(keyword.toLowerCase())
        )
      );

      console.log('找到音频设备:', devices.value.length, '个');

      // 注意：不在这里设置错误，允许音乐播放器在没有设备管理的情况下工作
      if (devices.value.length === 0) {
        console.warn('未找到可用的音频设备，将使用默认音频播放');
      }
    }
  } catch (err: any) {
    console.error('加载音频设备失败:', err);
    // 不设置错误，允许音乐播放器继续工作
    console.warn('设备管理不可用，将使用默认音频播放');
  }
};

// 处理设备切换
const handleDeviceSwitch = async (device: AudioDevice) => {
  if (device.status !== 'IDLE' && playerState.currentDevice?.device_id !== device.device_id) {
    error.value = `设备 ${device.name} 正在被其他进程使用`;
    return;
  }

  if (playerState.currentDevice?.device_id === device.device_id) {
    return; // 已经在使用这个设备
  }

  try {
    const success = await switchAudioDevice(device.device_id);
    if (success) {
      showDeviceList.value = false;
      error.value = null;
    }
  } catch (err: any) {
    console.error('切换设备失败:', err);
  }
};

// 申请指定的音频设备
const requestAudioDevice = async (deviceId?: number) => {
  // 如果没有进程ID或没有设备，仍然允许播放（使用默认音频）
  if (!processId.value) {
    console.warn('音乐播放器进程未创建，使用默认音频播放');
    return true; // 返回true允许播放继续
  }

  if (devices.value.length === 0) {
    console.warn('未找到可用的音频设备，使用默认音频播放');
    return true; // 返回true允许播放继续
  }

  try {
    let targetDevice: AudioDevice | undefined;

    if (deviceId) {
      // 申请指定设备
      targetDevice = devices.value.find(d => d.device_id === deviceId);
      if (!targetDevice) {
        error.value = '指定的音频设备不存在';
        return false;
      }
      if (targetDevice.status !== 'IDLE') {
        error.value = `设备 ${targetDevice.name} 当前不可用 (${targetDevice.status})`;
        return false;
      }
    } else {
      // 自动找到空闲的音频设备
      targetDevice = devices.value.find(d => d.status === 'IDLE');
      if (!targetDevice) {
        console.warn('所有音频设备都在使用中，使用默认音频播放');
        return true; // 返回true允许播放继续
      }
    }

    // 申请设备
    const response = await deviceAPI.requestDevice(targetDevice.device_id, processId.value);
    if (response.data.status === 'success') {
      playerState.currentDevice = response.data.data;
      await loadAudioDevices(); // 刷新设备状态
      console.log('成功申请音频设备:', targetDevice.name);
      error.value = null;
      return true;
    }
  } catch (err: any) {
    console.error('申请音频设备失败:', err);
    console.warn('设备申请失败，使用默认音频播放');
    // 不设置错误，允许默认播放
    return true;
  }
  return false;
};

// 释放音频设备
const releaseAudioDevice = async () => {
  if (!playerState.currentDevice || !processId.value) {
    // 如果没有设备或进程ID，直接清空当前设备
    playerState.currentDevice = null;
    return;
  }

  try {
    await deviceAPI.releaseDevice(playerState.currentDevice.device_id, processId.value);
    console.log('已释放音频设备:', playerState.currentDevice.name);
    playerState.currentDevice = null;
    await loadAudioDevices(); // 刷新设备状态
  } catch (err: any) {
    console.error('释放音频设备失败:', err);
    // 即使释放失败，也清空当前设备引用
    playerState.currentDevice = null;
  }
};

// 切换音频设备
const switchAudioDevice = async (newDeviceId: number) => {
  const wasPlaying = playerState.isPlaying;
  const currentTime = audio.value?.currentTime || 0;

  try {
    // 暂停播放并释放当前设备
    if (playerState.isPlaying) {
      await pausePlay();
    }

    if (playerState.currentDevice) {
      await releaseAudioDevice();
    }

    // 申请新设备
    const success = await requestAudioDevice(newDeviceId);
    if (success && wasPlaying && playerState.currentSong) {
      // 恢复播放
      if (audio.value) {
        audio.value.currentTime = currentTime;
        await resumePlay();
      }
    }

    return success;
  } catch (err: any) {
    console.error('切换音频设备失败:', err);
    error.value = '切换音频设备失败';
    return false;
  }
};

// 播放歌曲
const playSong = async (song: Song) => {
  // 如果没有设备，尝试申请设备（但不阻止播放）
  if (!playerState.currentDevice) {
    await requestAudioDevice();
    // 注意：不检查返回值，即使设备申请失败也允许播放
  }

  try {
    if (audio.value) {
      audio.value.src = song.file;
      audio.value.volume = playerState.isMuted ? 0 : playerState.volume;

      playerState.currentSong = song;
      await audio.value.play();
      playerState.isPlaying = true;

      console.log('开始播放:', song.artist, '-', song.title);
    }
  } catch (err: any) {
    console.error('播放失败:', err);
    error.value = '播放失败，请检查音频文件';
  }
};

// 暂停播放
const pausePlay = async () => {
  if (audio.value) {
    audio.value.pause();
    playerState.isPlaying = false;
    console.log('播放已暂停');
  }
};

// 停止播放并释放设备
const stopPlay = async () => {
  if (audio.value) {
    audio.value.pause();
    audio.value.currentTime = 0;
    playerState.isPlaying = false;
    playerState.currentTime = 0;
    await releaseAudioDevice();
    console.log('播放已停止');
  }
};

// 继续播放
const resumePlay = async () => {
  // 如果没有设备，尝试申请一个设备（但允许失败）
  if (!playerState.currentDevice) {
    await requestAudioDevice();
    // 注意：不检查返回值，即使设备申请失败也允许播放
  }

  if (audio.value && playerState.currentSong) {
    try {
      await audio.value.play();
      playerState.isPlaying = true;
      console.log('播放已继续');
    } catch (err: any) {
      console.error('继续播放失败:', err);
      error.value = '继续播放失败';
    }
  }
};

// 切换播放/暂停
const togglePlay = async () => {
  if (playerState.isPlaying) {
    await pausePlay();
  } else if (playerState.currentSong) {
    await resumePlay();
  }
};

// 上一首
const playPrevious = async () => {
  if (!playerState.currentSong) return;

  const currentIndex = songList.value.findIndex(s => s.id === playerState.currentSong!.id);
  const prevIndex = currentIndex > 0 ? currentIndex - 1 : songList.value.length - 1;
  await playSong(songList.value[prevIndex]);
};

// 下一首
const playNext = async () => {
  if (!playerState.currentSong) return;

  const currentIndex = songList.value.findIndex(s => s.id === playerState.currentSong!.id);
  const nextIndex = currentIndex < songList.value.length - 1 ? currentIndex + 1 : 0;
  await playSong(songList.value[nextIndex]);
};

// 设置音量
const setVolume = (volume: number) => {
  playerState.volume = volume;
  if (audio.value) {
    audio.value.volume = playerState.isMuted ? 0 : volume;
  }
};

// 静音切换
const toggleMute = () => {
  playerState.isMuted = !playerState.isMuted;
  if (audio.value) {
    audio.value.volume = playerState.isMuted ? 0 : playerState.volume;
  }
};

// 设置播放进度
const setProgress = (progress: number) => {
  if (audio.value && playerState.currentSong) {
    const duration = audio.value.duration || playerState.currentSong.duration;
    audio.value.currentTime = (progress / 100) * duration;
  }
};

// 格式化时间
const formatTime = (seconds: number) => {
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60);
  return `${mins}:${secs.toString().padStart(2, '0')}`;
};

// 计算属性
const currentProgress = computed(() => {
  if (!playerState.currentSong || !audio.value) return 0;
  const duration = audio.value.duration || playerState.currentSong.duration;
  return duration > 0 ? (playerState.currentTime / duration) * 100 : 0;
});

const currentTimeText = computed(() => formatTime(playerState.currentTime));
const totalTimeText = computed(() => {
  if (!playerState.currentSong) return '0:00';
  const duration = audio.value?.duration || playerState.currentSong.duration;
  return formatTime(duration);
});

// 音频事件处理
const setupAudioEvents = () => {
  if (!audio.value) return;

  audio.value.addEventListener('timeupdate', () => {
    if (audio.value) {
      playerState.currentTime = audio.value.currentTime;
    }
  });

  audio.value.addEventListener('ended', async () => {
    if (playerState.repeatMode === 'one') {
      if (audio.value) {
        audio.value.currentTime = 0;
        await audio.value.play();
      }
    } else if (playerState.repeatMode === 'all' || playerState.shuffleMode) {
      await playNext();
    } else {
      await stopPlay();
    }
  });

  audio.value.addEventListener('error', (e) => {
    console.error('音频播放错误:', e);
    error.value = '音频播放错误';
    playerState.isPlaying = false;
  });
};

// 设备状态检查
const checkDeviceStatus = async () => {
  if (playerState.currentDevice) {
    try {
      await loadAudioDevices();
      const currentDevice = devices.value.find(d => d.device_id === playerState.currentDevice!.device_id);

      if (!currentDevice) {
        // 设备被移除或删除
        await stopPlay();
        error.value = '⚠️ 音频设备已被删除，播放已停止';
        playerState.currentDevice = null;
        console.log('当前使用的音频设备已被删除');

        // 尝试自动切换到其他可用设备
        const availableDevice = devices.value.find(d => d.status === 'IDLE');
        if (availableDevice && playerState.currentSong && !playerState.isPlaying) {
          console.log('尝试切换到其他可用设备:', availableDevice.name);
          setTimeout(() => {
            error.value = `📡 已切换到设备: ${availableDevice.name}`;
          }, 2000);
        }

      } else if (currentDevice.status === 'ERROR') {
        // 设备出现错误
        await stopPlay();
        error.value = '⚠️ 音频设备出现错误，播放已停止';
        console.log('当前音频设备状态为ERROR');
      } else if (currentDevice.current_user !== processId.value && currentDevice.status === 'BUSY') {
        // 设备被其他进程占用（可能被强制释放了）
        await stopPlay();
        error.value = '⚠️ 音频设备被其他进程占用，播放已停止';
        playerState.currentDevice = null;
        console.log('当前音频设备被其他进程占用');
      }
    } catch (err) {
      console.error('检查设备状态失败:', err);
    }
  }
};

// 监听组件卸载
onMounted(async () => {
  audio.value = new Audio();
  setupAudioEvents();

  // 并行加载，但不让任何一个失败阻止整个初始化
  try {
    await Promise.allSettled([
      createMusicProcess(),
      loadMusicList(),
      loadAudioDevices()
    ]);
  } catch (err) {
    console.error('初始化过程中发生错误:', err);
  }

  // 定期检查设备状态（仅当有设备时）
  setInterval(() => {
    if (playerState.currentDevice) {
      checkDeviceStatus();
    }
  }, 5000);
});

onUnmounted(async () => {
  await stopPlay();
  if (processId.value) {
    try {
      await processAPI.terminateProcess(processId.value);
      console.log('音乐播放器进程已终止');
    } catch (err) {
      console.error('终止进程失败:', err);
    }
  }
});
</script>

<template>
  <div class="music-player">
    <!-- 标题栏 -->
    <div class="player-header">
      <div class="header-title">
        <span class="music-icon">🎵</span>
        <h2>在线音乐播放器</h2>
        <div class="process-info" v-if="processId">
          <span class="process-badge">PID: {{ processId }}</span>
        </div>
      </div>

      <!-- 设备选择区域 -->
      <div class="device-selector">
        <div class="device-info">
          <span class="device-label">输出设备:</span>
          <div class="current-device" v-if="playerState.currentDevice">
            <span class="device-icon">🎧</span>
            <span class="device-name">{{ playerState.currentDevice.name }}</span>
            <span class="device-status" :class="playerState.currentDevice.status.toLowerCase()">
              {{ playerState.currentDevice.status }}
            </span>
          </div>
          <span v-else class="no-device">未选择设备</span>
        </div>

        <!-- 设备切换按钮 -->
        <div class="device-controls">
          <button
            @click="showDeviceList = !showDeviceList"
            class="device-switch-btn"
            :class="{ active: showDeviceList }"
            title="切换音频设备"
          >
            🔧 设备
          </button>

          <!-- 设备下拉列表 -->
          <div v-if="showDeviceList" class="device-dropdown">
            <div class="device-dropdown-header">
              <span>可用音频设备</span>
              <button @click="loadAudioDevices" class="refresh-btn" title="刷新设备列表">🔄</button>
            </div>

            <div class="device-list">
              <div v-if="devices.length === 0" class="no-devices">
                暂无可用的音频设备<br>
                <small>将使用默认音频播放</small>
              </div>

              <div
                v-for="device in devices"
                :key="device.device_id"
                class="device-option"
                :class="{
                  current: playerState.currentDevice?.device_id === device.device_id,
                  disabled: device.status !== 'IDLE' && playerState.currentDevice?.device_id !== device.device_id
                }"
                @click="handleDeviceSwitch(device)"
              >
                <div class="device-option-icon">
                  {{ device.type === 'AUDIO' ? '🎧' : '🔊' }}
                </div>
                <div class="device-option-info">
                  <div class="device-option-name">{{ device.name }}</div>
                  <div class="device-option-status">{{ device.status }}</div>
                </div>
                <div class="device-option-actions" v-if="playerState.currentDevice?.device_id === device.device_id">
                  <span class="current-indicator">当前设备</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div v-if="isLoading" class="loading">
      <div class="loading-spinner"></div>
      <span>加载中...</span>
    </div>

    <div v-else-if="error" class="error-message">
      <span class="error-icon">⚠️</span>
      <span>{{ error }}</span>
      <button @click="error = null" class="dismiss-btn">关闭</button>
    </div>

    <div v-else class="player-content">
      <!-- 当前播放信息 -->
      <div class="now-playing" v-if="playerState.currentSong">
        <div class="song-info">
          <div class="song-cover">🎵</div>
          <div class="song-details">
            <h3 class="song-title">{{ playerState.currentSong.title }}</h3>
            <p class="song-artist">{{ playerState.currentSong.artist }}</p>
          </div>
        </div>

        <!-- 播放控制 -->
        <div class="player-controls">
          <button @click="playPrevious" class="control-btn" title="上一首">⏮️</button>
          <button @click="togglePlay" class="control-btn play-btn" :title="playerState.isPlaying ? '暂停' : '播放'">
            {{ playerState.isPlaying ? '⏸️' : '▶️' }}
          </button>
          <button @click="playNext" class="control-btn" title="下一首">⏭️</button>
          <button @click="stopPlay" class="control-btn" title="停止">⏹️</button>
        </div>

        <!-- 进度条 -->
        <div class="progress-section">
          <span class="time-text">{{ currentTimeText }}</span>
          <div class="progress-bar">
            <input
              type="range"
              min="0"
              max="100"
              :value="currentProgress"
              @input="setProgress(($event.target as HTMLInputElement).valueAsNumber)"
              class="progress-slider"
            />
          </div>
          <span class="time-text">{{ totalTimeText }}</span>
        </div>

        <!-- 音量控制 -->
        <div class="volume-section">
          <button @click="toggleMute" class="control-btn" :title="playerState.isMuted ? '取消静音' : '静音'">
            {{ playerState.isMuted ? '🔇' : '🔊' }}
          </button>
          <div class="volume-bar">
            <input
              type="range"
              min="0"
              max="1"
              step="0.01"
              :value="playerState.volume"
              @input="setVolume(($event.target as HTMLInputElement).valueAsNumber)"
              class="volume-slider"
            />
          </div>
        </div>
      </div>

      <!-- 歌曲列表 -->
      <div class="song-list">
        <h3 class="list-header">
          <span>歌曲列表</span>
          <span class="song-count">{{ songList.length }} 首歌曲</span>
        </h3>
        <div class="list-container">
          <div
            v-for="(song, index) in songList"
            :key="song.id"
            class="song-item"
            :class="{ active: playerState.currentSong?.id === song.id }"
            @click="playSong(song)"
          >
            <div class="song-index">{{ index + 1 }}</div>
            <div class="song-info-item">
              <div class="song-title-item">{{ song.title }}</div>
              <div class="song-artist-item">{{ song.artist }}</div>
            </div>
            <div class="song-duration">{{ formatTime(song.duration) }}</div>
            <div class="song-format">{{ song.format.toUpperCase() }}</div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.music-player {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
  color: white;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  overflow: hidden;
}

.player-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 20px 24px;
  background: rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(20px);
  border-bottom: 1px solid rgba(255, 255, 255, 0.2);
}

.header-title {
  display: flex;
  align-items: center;
  gap: 12px;
}

.music-icon {
  font-size: 24px;
}

.header-title h2 {
  margin: 0;
  font-size: 20px;
  font-weight: 600;
}

.process-info {
  margin-top: 4px;
}

.process-badge {
  background: rgba(255, 255, 255, 0.2);
  padding: 2px 8px;
  border-radius: 12px;
  font-size: 0.75em;
  font-weight: 500;
}

.device-selector {
  display: flex;
  flex-direction: column;
  align-items: flex-end;
  position: relative;
}

.device-info {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.device-label {
  font-size: 0.85em;
  opacity: 0.8;
}

.current-device {
  display: flex;
  align-items: center;
  gap: 8px;
  background: rgba(255, 255, 255, 0.1);
  padding: 4px 12px;
  border-radius: 16px;
  font-size: 0.9em;
}

.device-name {
  font-weight: 500;
}

.device-status {
  font-size: 0.75em;
  padding: 2px 6px;
  border-radius: 8px;
  font-weight: 600;
  text-transform: uppercase;
}

.device-status.idle {
  background: rgba(40, 167, 69, 0.8);
  color: white;
}

.device-status.busy {
  background: rgba(255, 193, 7, 0.8);
  color: #212529;
}

.device-status.error {
  background: rgba(220, 53, 69, 0.8);
  color: white;
}

.no-device {
  color: rgba(255, 255, 255, 0.6);
  font-style: italic;
  font-size: 0.9em;
}

.device-controls {
  display: flex;
  align-items: center;
  gap: 8px;
}

.device-switch-btn {
  background: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.2);
  color: white;
  padding: 6px 12px;
  border-radius: 20px;
  cursor: pointer;
  font-size: 0.85em;
  transition: all 0.2s ease;
  backdrop-filter: blur(10px);
}

.device-switch-btn:hover {
  background: rgba(255, 255, 255, 0.2);
  transform: translateY(-1px);
}

.device-switch-btn.active {
  background: rgba(255, 255, 255, 0.25);
  box-shadow: 0 0 0 2px rgba(255, 255, 255, 0.3);
}

.device-dropdown {
  position: absolute;
  top: 100%;
  right: 0;
  width: 320px;
  background: rgba(255, 255, 255, 0.95);
  color: #333;
  border-radius: 12px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
  backdrop-filter: blur(20px);
  border: 1px solid rgba(255, 255, 255, 0.3);
  z-index: 1000;
  overflow: hidden;
  margin-top: 8px;
}

.device-dropdown-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: rgba(0, 0, 0, 0.05);
  border-bottom: 1px solid rgba(0, 0, 0, 0.1);
  font-weight: 600;
  font-size: 0.9em;
}

.refresh-btn {
  background: none;
  border: none;
  color: #666;
  cursor: pointer;
  padding: 4px;
  border-radius: 4px;
  transition: all 0.2s ease;
}

.refresh-btn:hover {
  background: rgba(0, 0, 0, 0.1);
  transform: rotate(180deg);
}

.device-list {
  max-height: 240px;
  overflow-y: auto;
}

.no-devices {
  padding: 16px;
  text-align: center;
  color: #666;
  font-style: italic;
}

.device-option {
  display: flex;
  align-items: center;
  padding: 12px 16px;
  cursor: pointer;
  transition: all 0.2s ease;
  border-bottom: 1px solid rgba(0, 0, 0, 0.05);
}

.device-option:hover {
  background: rgba(0, 0, 0, 0.05);
}

.device-option.current {
  background: rgba(30, 60, 114, 0.1);
  border-left: 3px solid #1e3c72;
}

.device-option.disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.device-option-icon {
  font-size: 1.2em;
  margin-right: 12px;
}

.device-option-info {
  flex: 1;
}

.device-option-name {
  font-weight: 500;
  margin-bottom: 2px;
}

.device-option-status {
  font-size: 0.8em;
  color: #666;
}

.current-indicator {
  background: #1e3c72;
  color: white;
  padding: 2px 8px;
  border-radius: 10px;
  font-size: 0.7em;
  font-weight: 600;
}

.loading, .error-message {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  padding: 40px;
  text-align: center;
}

.loading-spinner {
  width: 20px;
  height: 20px;
  border: 2px solid rgba(255, 255, 255, 0.3);
  border-top: 2px solid white;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.error-message {
  background: rgba(255, 0, 0, 0.1);
  border: 1px solid rgba(255, 0, 0, 0.3);
  border-radius: 8px;
  margin: 20px;
}

.dismiss-btn {
  background: rgba(255, 255, 255, 0.2);
  border: none;
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 12px;
}

.player-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.now-playing {
  padding: 24px;
  background: rgba(255, 255, 255, 0.05);
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.song-info {
  display: flex;
  align-items: center;
  gap: 16px;
  margin-bottom: 20px;
}

.song-cover {
  width: 60px;
  height: 60px;
  background: rgba(255, 255, 255, 0.15);
  border-radius: 8px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 24px;
}

.song-title {
  margin: 0 0 4px 0;
  font-size: 18px;
  font-weight: 600;
}

.song-artist {
  margin: 0;
  font-size: 14px;
  opacity: 0.8;
}

.player-controls {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  margin-bottom: 20px;
}

.control-btn {
  background: rgba(255, 255, 255, 0.15);
  border: none;
  color: white;
  width: 44px;
  height: 44px;
  border-radius: 50%;
  cursor: pointer;
  font-size: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.2s ease;
}

.control-btn:hover {
  background: rgba(255, 255, 255, 0.25);
  transform: scale(1.05);
}

.play-btn {
  width: 52px;
  height: 52px;
  background: rgba(255, 255, 255, 0.2);
  font-size: 18px;
}

.progress-section, .volume-section {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
}

.time-text {
  font-size: 12px;
  font-family: 'Courier New', monospace;
  min-width: 40px;
}

.progress-bar, .volume-bar {
  flex: 1;
}

.volume-bar {
  max-width: 100px;
}

.progress-slider, .volume-slider {
  width: 100%;
  height: 4px;
  background: rgba(255, 255, 255, 0.2);
  border-radius: 2px;
  outline: none;
  cursor: pointer;
  -webkit-appearance: none;
}

.progress-slider::-webkit-slider-thumb, .volume-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 16px;
  height: 16px;
  background: white;
  border-radius: 50%;
  cursor: pointer;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
}

.song-list {
  flex: 1;
  padding: 24px;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}

.list-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin: 0 0 16px 0;
  font-size: 16px;
  font-weight: 600;
}

.song-count {
  font-size: 14px;
  opacity: 0.7;
  font-weight: normal;
}

.list-container {
  flex: 1;
  overflow-y: auto;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 8px;
}

.song-item {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 12px 16px;
  cursor: pointer;
  transition: all 0.2s ease;
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.song-item:hover {
  background: rgba(255, 255, 255, 0.1);
}

.song-item.active {
  background: rgba(255, 255, 255, 0.15);
  border-left: 4px solid white;
}

.song-index {
  width: 24px;
  text-align: center;
  font-size: 14px;
  opacity: 0.7;
}

.song-info-item {
  flex: 1;
}

.song-title-item {
  font-size: 14px;
  font-weight: 500;
  margin-bottom: 2px;
}

.song-artist-item {
  font-size: 12px;
  opacity: 0.7;
}

.song-duration, .song-format {
  font-size: 12px;
  opacity: 0.7;
  min-width: 50px;
  text-align: right;
}

.song-format {
  min-width: 40px;
  font-weight: 600;
}

/* 滚动条美化 */
.list-container::-webkit-scrollbar {
  width: 6px;
}

.list-container::-webkit-scrollbar-track {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 3px;
}

.list-container::-webkit-scrollbar-thumb {
  background: rgba(255, 255, 255, 0.3);
  border-radius: 3px;
}

.list-container::-webkit-scrollbar-thumb:hover {
  background: rgba(255, 255, 255, 0.5);
}
</style>
