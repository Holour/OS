<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import { useWindowsStore } from '@/stores/windows';
import { memoryAPI, systemAPI, schedulerAPI } from '@/services/api';

const windowsStore = useWindowsStore();
const showStartMenu = ref(false);
const currentTime = ref('');
const cpuUsage = ref(0);
const memoryUsage = ref(0);
const currentDate = ref('');

const toggleStartMenu = () => {
  showStartMenu.value = !showStartMenu.value;
};

const openProcessManager = () => {
  windowsStore.openWindow('proc-manager', '进程管理器', 'ProcessManager');
  showStartMenu.value = false;
};

const openTerminal = () => {
  windowsStore.openWindow('terminal', '终端', 'Terminal');
  showStartMenu.value = false;
};

const openFileManager = () => {
  windowsStore.openWindow('file-manager', '文件管理器', 'FileManager');
  showStartMenu.value = false;
};

const openMemoryManager = () => {
  windowsStore.openWindow('memory-manager', '内存管理器', 'MemoryManager');
  showStartMenu.value = false;
};

const openSystemControl = () => {
  windowsStore.openWindow('system-control', '系统控制', 'SystemControl');
  showStartMenu.value = false;
};

const openDeviceManager = () => {
  windowsStore.openWindow('device-manager', '设备管理器', 'DeviceManager');
  showStartMenu.value = false;
};

const openClockManager = () => {
  windowsStore.openWindow('clock-manager', '时钟管理', 'ClockManager');
  showStartMenu.value = false;
};

const runSchedulerTick = async () => {
  try {
    await schedulerAPI.tick();
    console.log('调度器执行成功');
  } catch (error) {
    console.error('调度器执行失败:', error);
  }
};

const updateTime = () => {
  const now = new Date();
  currentTime.value = now.toLocaleTimeString('zh-CN', {
    hour12: false,
    hour: '2-digit',
    minute: '2-digit'
  });
  currentDate.value = now.toLocaleDateString('zh-CN', {
    month: '2-digit',
    day: '2-digit'
  });
};

// 更新系统状态
const updateSystemStatus = async () => {
  try {
    // 获取内存使用情况
    const memoryResponse = await memoryAPI.getStatus();
    if (memoryResponse.data.status === 'success') {
      const { total_memory, used_memory } = memoryResponse.data.data;
      memoryUsage.value = Math.round((used_memory / total_memory) * 100);
    }

    // 模拟CPU使用率（可以从调度器或其他API获取）
    cpuUsage.value = Math.floor(Math.random() * 30) + 10; // 10-40%的随机值
  } catch (error) {
    console.error('Failed to update system status:', error);
  }
};

const focusWindow = (windowId: string) => {
  windowsStore.focusWindow(windowId);
};

const closeWindow = (windowId: string, event: Event) => {
  event.stopPropagation();
  windowsStore.closeWindow(windowId);
};

let timeInterval: number;

onMounted(() => {
  updateTime();
  updateSystemStatus();

  timeInterval = setInterval(() => {
    updateTime();
    updateSystemStatus();
  }, 5000); // 每5秒更新一次

  // 点击其他地方关闭开始菜单
  document.addEventListener('click', (e) => {
    const target = e.target as HTMLElement;
    if (!target.closest('.start-button') && !target.closest('.start-menu-popup')) {
      showStartMenu.value = false;
    }
  });
});

onUnmounted(() => {
  clearInterval(timeInterval);
});
</script>

<template>
  <div class="taskbar">
    <!-- 开始按钮 -->
    <div class="start-button" @click="toggleStartMenu">
      <div class="start-icon">⊞</div>
      <span>开始</span>
    </div>

    <!-- 开始菜单 -->
    <div v-if="showStartMenu" class="start-menu-popup">
      <div class="menu-header">
        <span class="header-icon">🖥️</span>
        <span>系统程序</span>
      </div>

      <!-- 核心管理工具 -->
      <div class="menu-section">
        <div class="menu-item" @click="openProcessManager">
          <span class="menu-icon">📊</span>
          <span>进程管理器</span>
        </div>
        <div class="menu-item" @click="openMemoryManager">
          <span class="menu-icon">💾</span>
          <span>内存管理器</span>
        </div>
        <div class="menu-item" @click="openFileManager">
          <span class="menu-icon">📁</span>
          <span>文件管理器</span>
        </div>
      </div>

      <div class="menu-divider"></div>

      <!-- 系统工具 -->
      <div class="menu-section">
        <div class="menu-item" @click="openTerminal">
          <span class="menu-icon">⌨️</span>
          <span>终端</span>
        </div>
        <div class="menu-item" @click="openDeviceManager">
          <span class="menu-icon">🔌</span>
          <span>设备管理器</span>
        </div>
        <div class="menu-item" @click="openClockManager">
          <span class="menu-icon">⏰</span>
          <span>时钟管理</span>
        </div>
      </div>

      <div class="menu-divider"></div>

      <!-- 系统控制 -->
      <div class="menu-section">
        <div class="menu-item" @click="openSystemControl">
          <span class="menu-icon">⚙️</span>
          <span>系统控制</span>
        </div>
        <div class="menu-item" @click="runSchedulerTick">
          <span class="menu-icon">🔄</span>
          <span>执行调度</span>
        </div>
      </div>
    </div>

    <!-- 任务栏窗口 -->
    <div class="running-apps">
      <div
        v-for="window in windowsStore.windows"
        :key="window.id"
        class="taskbar-item"
        :class="{ active: window.isFocused }"
        @click="focusWindow(window.id)"
      >
        <span class="window-title">{{ window.title }}</span>
        <button class="close-btn" @click="closeWindow(window.id, $event)">×</button>
      </div>
    </div>

    <!-- 系统托盘 -->
    <div class="system-tray">
      <div class="system-monitors">
        <div class="monitor-item" :title="`CPU使用率: ${cpuUsage}%`">
          <span class="monitor-icon">🔥</span>
          <span class="monitor-value">{{ cpuUsage }}%</span>
        </div>
        <div class="monitor-item" :title="`内存使用率: ${memoryUsage}%`">
          <span class="monitor-icon">💾</span>
          <span class="monitor-value">{{ memoryUsage }}%</span>
        </div>
      </div>
      <div class="time-display">
        <div class="time">{{ currentTime }}</div>
        <div class="date">{{ currentDate }}</div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.taskbar {
  height: 48px;
  background: linear-gradient(to bottom,
    rgba(255, 255, 255, 0.95) 0%,
    rgba(245, 245, 245, 0.95) 50%,
    rgba(225, 225, 225, 0.95) 51%,
    rgba(235, 235, 235, 0.95) 100%);
  backdrop-filter: blur(20px);
  color: #333;
  display: flex;
  align-items: center;
  padding: 0 8px;
  flex-shrink: 0;
  z-index: 10000;
  box-shadow:
    0 -2px 8px rgba(0, 0, 0, 0.1),
    0 -1px 3px rgba(0, 0, 0, 0.05);
  border-top: 1px solid rgba(255, 255, 255, 0.3);
  position: relative;
}

/* 开始按钮 */
.start-button {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 16px;
  border-radius: 6px;
  border: 1px solid rgba(0, 0, 0, 0.15);
  background: linear-gradient(to bottom,
    rgba(255, 255, 255, 0.9) 0%,
    rgba(240, 240, 240, 0.9) 100%);
  color: #333;
  cursor: pointer;
  font-size: 12px;
  font-weight: 600;
  text-shadow: 0 1px 0 rgba(255, 255, 255, 0.8);
  transition: all 0.2s ease;
  margin-right: 8px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
}

.start-button:hover {
  background: linear-gradient(to bottom,
    rgba(255, 255, 255, 1) 0%,
    rgba(245, 245, 245, 1) 100%);
  border-color: rgba(0, 120, 215, 0.3);
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.15);
  transform: translateY(-1px);
}

.start-icon {
  font-size: 14px;
  font-weight: bold;
}

/* 开始菜单 */
.start-menu-popup {
  position: absolute;
  bottom: 100%;
  left: 8px;
  width: 280px;
  background: rgba(255, 255, 255, 0.98);
  backdrop-filter: blur(20px);
  border: 1px solid rgba(0, 0, 0, 0.1);
  border-radius: 12px;
  box-shadow:
    0 8px 32px rgba(0, 0, 0, 0.15),
    0 4px 16px rgba(0, 0, 0, 0.1);
  z-index: 10001;
  color: #333;
  overflow: hidden;
  margin-bottom: 8px;
}

.menu-header {
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  padding: 16px 20px;
  font-weight: bold;
  font-size: 14px;
  color: white;
  display: flex;
  align-items: center;
  gap: 10px;
}

.header-icon {
  font-size: 18px;
}

.menu-section {
  padding: 8px 0;
}

.menu-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 20px;
  cursor: pointer;
  transition: all 0.2s ease;
  font-size: 13px;
  color: #333;
  font-weight: 500;
}

.menu-item:hover {
  background-color: rgba(0, 120, 215, 0.1);
  color: #0078d4;
  padding-left: 24px;
}

.menu-icon {
  width: 18px;
  text-align: center;
  font-size: 16px;
}

.menu-divider {
  height: 1px;
  background: linear-gradient(90deg,
    transparent 0%,
    rgba(0, 0, 0, 0.1) 20%,
    rgba(0, 0, 0, 0.1) 80%,
    transparent 100%);
  margin: 8px 20px;
}

/* 任务栏窗口 */
.running-apps {
  flex-grow: 1;
  margin: 0 10px;
  height: 100%;
  display: flex;
  align-items: center;
  gap: 2px;
}

.taskbar-item {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  background: rgba(255, 255, 255, 0.7);
  border: 1px solid rgba(0, 0, 0, 0.1);
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s ease;
  max-width: 180px;
  min-width: 140px;
  backdrop-filter: blur(10px);
}

.taskbar-item:hover {
  background: rgba(255, 255, 255, 0.9);
  border-color: rgba(0, 120, 215, 0.3);
  transform: translateY(-1px);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.taskbar-item.active {
  background: rgba(0, 120, 215, 0.1);
  border-color: rgba(0, 120, 215, 0.4);
  box-shadow: inset 0 2px 4px rgba(0, 120, 215, 0.2);
}

.window-title {
  flex-grow: 1;
  font-size: 12px;
  font-weight: 500;
  color: #333;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.close-btn {
  background: none;
  border: none;
  color: #666;
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
  padding: 2px 6px;
  border-radius: 4px;
  transition: all 0.2s ease;
  line-height: 1;
}

.close-btn:hover {
  background: rgba(255, 0, 0, 0.8);
  color: white;
  transform: scale(1.1);
}

/* 系统托盘 */
.system-tray {
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 11px;
  font-weight: 500;
  padding: 6px 12px;
  background: rgba(255, 255, 255, 0.6);
  border-radius: 8px;
  border: 1px solid rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(10px);
}

.system-monitors {
  display: flex;
  gap: 8px;
}

.monitor-item {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 6px;
  background: rgba(255, 255, 255, 0.8);
  border-radius: 4px;
  font-size: 10px;
  cursor: help;
  transition: all 0.2s ease;
}

.monitor-item:hover {
  background: rgba(255, 255, 255, 1);
  transform: scale(1.05);
}

.monitor-icon {
  font-size: 12px;
}

.monitor-value {
  font-family: 'Consolas', 'Monaco', monospace;
  font-weight: bold;
  color: #0078d4;
}

.time-display {
  text-align: right;
}

.time {
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 14px;
  font-weight: bold;
  color: #333;
}

.date {
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 10px;
  color: #666;
  margin-top: 1px;
}
</style>
