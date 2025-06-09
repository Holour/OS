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
/* 任务栏样式美化 - 统一协调设计 */
.taskbar {
  height: 48px;
  background: rgba(248, 248, 248, 0.9);
  backdrop-filter: blur(25px) saturate(150%);
  color: #323130;
  display: flex;
  align-items: center;
  padding: 0 16px;
  flex-shrink: 0;
  z-index: 10000;
  box-shadow:
    0 -1px 3px rgba(0, 0, 0, 0.08),
    0 -2px 12px rgba(0, 0, 0, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.8);
  border-top: 1px solid rgba(255, 255, 255, 0.5);
  position: relative;
}

/* 开始按钮 - 简洁现代设计 */
.start-button {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 48px;
  height: 36px;
  border-radius: 8px;
  border: none;
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(10px);
  color: #323130;
  cursor: pointer;
  font-size: 14px;
  font-weight: 600;
  transition: all 0.2s cubic-bezier(0.25, 0.46, 0.45, 0.94);
  margin-right: 16px;
  box-shadow:
    0 2px 8px rgba(0, 0, 0, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  position: relative;
  overflow: hidden;
}

.start-button::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: linear-gradient(135deg, rgba(0, 120, 215, 0.1) 0%, rgba(0, 150, 215, 0.15) 100%);
  border-radius: 8px;
  opacity: 0;
  transition: opacity 0.2s ease;
}

.start-button:hover::before {
  opacity: 1;
}

.start-button:hover {
  background: rgba(255, 255, 255, 0.85);
  transform: translateY(-1px);
  box-shadow:
    0 4px 16px rgba(0, 0, 0, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 1);
}

.start-button:active {
  transform: translateY(0);
  background: rgba(255, 255, 255, 0.95);
}

.start-icon {
  font-size: 18px;
  font-weight: bold;
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  -webkit-background-clip: text;
  background-clip: text;
  -webkit-text-fill-color: transparent;
  filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.1));
}

/* 开始菜单 - 保持现有样式 */
.start-menu-popup {
  position: absolute;
  bottom: 100%;
  left: 16px;
  width: 320px;
  background: rgba(249, 249, 249, 0.95);
  backdrop-filter: blur(40px) saturate(180%);
  border: 1px solid rgba(255, 255, 255, 0.3);
  border-radius: 12px;
  box-shadow:
    0 16px 40px rgba(0, 0, 0, 0.12),
    0 8px 16px rgba(0, 0, 0, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.7);
  z-index: 10001;
  color: #323130;
  overflow: hidden;
  margin-bottom: 12px;
}

.menu-header {
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  padding: 20px 24px;
  font-weight: 600;
  font-size: 15px;
  color: white;
  display: flex;
  align-items: center;
  gap: 12px;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.2);
}

.header-icon {
  font-size: 20px;
  filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.2));
}

.menu-section {
  padding: 12px 0;
}

.menu-item {
  display: flex;
  align-items: center;
  gap: 14px;
  padding: 14px 24px;
  cursor: pointer;
  transition: all 0.2s cubic-bezier(0.25, 0.46, 0.45, 0.94);
  font-size: 14px;
  color: #323130;
  font-weight: 500;
  position: relative;
}

.menu-item::before {
  content: '';
  position: absolute;
  left: 0;
  top: 0;
  bottom: 0;
  width: 3px;
  background: linear-gradient(to bottom, #0078d4, #106ebe);
  transform: scaleY(0);
  transition: transform 0.2s ease;
  border-radius: 0 2px 2px 0;
}

.menu-item:hover::before {
  transform: scaleY(1);
}

.menu-item:hover {
  background: rgba(0, 120, 215, 0.08);
  color: #0078d4;
  padding-left: 28px;
  transform: translateX(4px);
}

.menu-item:active {
  background: rgba(0, 120, 215, 0.12);
  transform: translateX(2px);
}

.menu-icon {
  width: 20px;
  text-align: center;
  font-size: 16px;
  filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.1));
}

.menu-divider {
  height: 1px;
  background: linear-gradient(90deg,
    transparent 0%,
    rgba(0, 0, 0, 0.08) 20%,
    rgba(0, 0, 0, 0.08) 80%,
    transparent 100%);
  margin: 8px 24px;
}

/* 任务栏窗口 - 保持现有样式 */
.running-apps {
  flex-grow: 1;
  margin: 0 16px;
  height: 100%;
  display: flex;
  align-items: center;
  gap: 4px;
}

.taskbar-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 16px;
  background: rgba(255, 255, 255, 0.6);
  backdrop-filter: blur(15px);
  border: 1px solid rgba(255, 255, 255, 0.3);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.25s cubic-bezier(0.25, 0.46, 0.45, 0.94);
  max-width: 200px;
  min-width: 150px;
  position: relative;
  overflow: hidden;
}

.taskbar-item::before {
  content: '';
  position: absolute;
  bottom: 0;
  left: 50%;
  transform: translateX(-50%);
  width: 0;
  height: 2px;
  background: linear-gradient(90deg, #0078d4, #106ebe);
  transition: width 0.3s ease;
  border-radius: 1px;
}

.taskbar-item:hover::before,
.taskbar-item.active::before {
  width: 80%;
}

.taskbar-item:hover {
  background: rgba(255, 255, 255, 0.8);
  transform: translateY(-2px);
  box-shadow:
    0 6px 20px rgba(0, 0, 0, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.taskbar-item.active {
  background: rgba(0, 120, 215, 0.12);
  border-color: rgba(0, 120, 215, 0.3);
  box-shadow:
    inset 0 2px 6px rgba(0, 120, 215, 0.15),
    0 2px 8px rgba(0, 0, 0, 0.08);
}

.window-title {
  flex-grow: 1;
  font-size: 13px;
  font-weight: 500;
  color: #323130;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.close-btn {
  background: none;
  border: none;
  color: #605e5c;
  font-size: 14px;
  font-weight: bold;
  cursor: pointer;
  padding: 4px 8px;
  border-radius: 4px;
  transition: all 0.2s ease;
  line-height: 1;
  opacity: 0.7;
}

.close-btn:hover {
  background: rgba(232, 17, 35, 0.9);
  color: white;
  opacity: 1;
  transform: scale(1.1);
}

.close-btn:active {
  transform: scale(0.95);
}

/* 系统托盘 - 重新设计协调风格 */
.system-tray {
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 12px;
  font-weight: 500;
  padding: 6px 16px;
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(15px);
  border-radius: 8px;
  border: 1px solid rgba(255, 255, 255, 0.4);
  box-shadow:
    0 2px 8px rgba(0, 0, 0, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.8);
  transition: all 0.2s ease;
}

.system-tray:hover {
  background: rgba(255, 255, 255, 0.85);
  transform: translateY(-1px);
  box-shadow:
    0 4px 12px rgba(0, 0, 0, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.system-monitors {
  display: flex;
  gap: 8px;
}

.monitor-item {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 8px;
  background: rgba(255, 255, 255, 0.6);
  border-radius: 6px;
  font-size: 10px;
  cursor: help;
  transition: all 0.2s ease;
  border: 1px solid rgba(255, 255, 255, 0.3);
}

.monitor-item:hover {
  background: rgba(255, 255, 255, 0.9);
  transform: translateY(-1px);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.monitor-icon {
  font-size: 11px;
  opacity: 0.8;
}

.monitor-value {
  font-weight: 600;
  color: #323130;
  font-size: 9px;
}

.time-display {
  display: flex;
  flex-direction: column;
  align-items: center;
  line-height: 1.1;
  color: #323130;
  cursor: pointer;
  padding: 4px 8px;
  border-radius: 6px;
  transition: all 0.2s ease;
}

.time-display:hover {
  background: rgba(255, 255, 255, 0.3);
}

.time {
  font-size: 12px;
  font-weight: 600;
  margin-bottom: 1px;
  font-family: 'Segoe UI', monospace;
}

.date {
  font-size: 9px;
  opacity: 0.8;
  font-weight: 500;
}
</style>
