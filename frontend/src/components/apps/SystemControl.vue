<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import { systemAPI, clockAPI, schedulerAPI } from '@/services/api';

const systemTime = ref<string>('');
const clockStatus = ref<any>(null);
const systemStatus = ref<any>(null);
const schedulerStatus = ref<any>(null);
const clockInterval = ref(100);
const isLoading = ref(true);
const error = ref<string | null>(null);
let intervalId: number;

const fetchSystemData = async () => {
  try {
    const [timeRes, clockRes, sysRes, schedRes] = await Promise.all([
      clockAPI.getTime().catch(() => ({ data: { data: { current_time: new Date().toLocaleString() } } })),
      clockAPI.getStatus().catch(() => ({ data: { data: { interval: 100 } } })),
      systemAPI.getStatus().catch(() => ({ data: { data: { status: 'running' } } })),
      schedulerAPI.getStatus().catch(() => ({ data: { data: { algorithm: 'round_robin' } } }))
    ]);

    if (timeRes.data.status === 'success' || timeRes.data.data) {
      systemTime.value = timeRes.data.data.current_time || new Date().toLocaleString();
    }

    if (clockRes.data.status === 'success' || clockRes.data.data) {
      clockStatus.value = clockRes.data.data;
    }

    if (sysRes.data.status === 'success' || sysRes.data.data) {
      systemStatus.value = sysRes.data.data;
    }

    if (schedRes.data.status === 'success' || schedRes.data.data) {
      schedulerStatus.value = schedRes.data.data;
    }

    error.value = null;
  } catch (err: any) {
    error.value = err.message || 'Failed to fetch system data';
  } finally {
    isLoading.value = false;
  }
};

const updateClockInterval = async () => {
  try {
    await clockAPI.setInterval(clockInterval.value);
    fetchSystemData();
    alert('时钟间隔更新成功！');
  } catch (err: any) {
    error.value = err.message || 'Failed to update clock interval';
  }
};

const executeTick = async () => {
  try {
    await schedulerAPI.tick();
    alert('调度器 tick 执行成功！');
    fetchSystemData();
  } catch (err: any) {
    error.value = err.message || 'Failed to execute scheduler tick';
  }
};

const systemShutdown = async () => {
  if (!confirm('确定要关闭系统吗？')) return;

  try {
    await systemAPI.shutdown();
    alert('系统关闭命令已发送');
  } catch (err: any) {
    error.value = err.message || 'Failed to shutdown system';
  }
};

const systemReboot = async () => {
  if (!confirm('确定要重启系统吗？')) return;

  try {
    await systemAPI.reboot();
    alert('系统重启命令已发送');
  } catch (err: any) {
    error.value = err.message || 'Failed to reboot system';
  }
};

onMounted(() => {
  fetchSystemData();
  intervalId = window.setInterval(fetchSystemData, 2000);
});

onUnmounted(() => {
  clearInterval(intervalId);
});
</script>

<template>
  <div class="system-control">
    <div class="header">
      <h3>系统控制面板</h3>
      <button @click="fetchSystemData" :disabled="isLoading">刷新</button>
    </div>

    <div v-if="error" class="error">{{ error }}</div>
    <div v-if="isLoading" class="loading">加载中...</div>

    <div v-else class="content">
      <!-- 系统时钟 -->
      <div class="panel-section">
        <h4>系统时钟</h4>
        <div class="clock-display">
          <div class="current-time">{{ systemTime }}</div>
          <div class="clock-info" v-if="clockStatus">
            <span>时钟间隔: {{ clockStatus.interval }}ms</span>
          </div>
        </div>

        <div class="clock-controls">
          <label>设置时钟间隔:</label>
          <input v-model.number="clockInterval" type="number" min="10" max="10000" />
          <button @click="updateClockInterval">更新间隔</button>
        </div>
      </div>

      <!-- 系统状态 -->
      <div class="panel-section">
        <h4>系统状态</h4>
        <div class="status-grid" v-if="systemStatus">
          <div class="status-item">
            <span class="label">状态:</span>
            <span class="value status-running">{{ systemStatus.status || 'running' }}</span>
          </div>
          <div class="status-item" v-if="systemStatus.uptime">
            <span class="label">运行时间:</span>
            <span class="value">{{ systemStatus.uptime }}</span>
          </div>
        </div>
      </div>

      <!-- 调度器控制 -->
      <div class="panel-section">
        <h4>调度器</h4>
        <div class="scheduler-info" v-if="schedulerStatus">
          <div class="status-item">
            <span class="label">调度算法:</span>
            <span class="value">{{ schedulerStatus.algorithm || 'round_robin' }}</span>
          </div>
          <div class="status-item" v-if="schedulerStatus.current_process">
            <span class="label">当前进程:</span>
            <span class="value">PID {{ schedulerStatus.current_process }}</span>
          </div>
        </div>

        <div class="scheduler-controls">
          <button @click="executeTick" class="tick-btn">执行调度 Tick</button>
        </div>
      </div>

      <!-- 系统控制 -->
      <div class="panel-section">
        <h4>系统控制</h4>
        <div class="system-controls">
          <button @click="systemReboot" class="reboot-btn">重启系统</button>
          <button @click="systemShutdown" class="shutdown-btn">关闭系统</button>
        </div>
      </div>

      <!-- 快捷信息 -->
      <div class="panel-section">
        <h4>快捷信息</h4>
        <div class="quick-info">
          <div class="info-item">
            <span class="label">操作系统:</span>
            <span class="value">OS-Simulator v1.0</span>
          </div>
          <div class="info-item">
            <span class="label">架构:</span>
            <span class="value">x86_64</span>
          </div>
          <div class="info-item">
            <span class="label">内核版本:</span>
            <span class="value">1.0.0</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.system-control {
  display: flex;
  flex-direction: column;
  height: 100%;
  font-size: 12px;
  color: #333;
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px;
  border-bottom: 1px solid #ccc;
  background-color: #f5f5f5;
}

.header h3 {
  margin: 0;
}

.error {
  background-color: #ffe6e6;
  color: #d8000c;
  padding: 10px;
  margin: 10px;
}

.loading {
  text-align: center;
  padding: 20px;
}

.content {
  padding: 10px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.panel-section {
  border: 1px solid #ddd;
  border-radius: 5px;
  padding: 10px;
  background-color: #fafafa;
}

.panel-section h4 {
  margin: 0 0 10px 0;
  color: #333;
  border-bottom: 1px solid #eee;
  padding-bottom: 5px;
}

.clock-display {
  text-align: center;
  margin-bottom: 10px;
}

.current-time {
  font-size: 18px;
  font-weight: bold;
  color: #007acc;
  margin-bottom: 5px;
}

.clock-info {
  font-size: 11px;
  color: #666;
}

.clock-controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.clock-controls label {
  font-weight: bold;
}

.clock-controls input {
  padding: 3px;
  font-size: 11px;
  width: 80px;
}

.clock-controls button {
  background-color: #007acc;
  color: white;
  border: none;
  padding: 5px 10px;
  cursor: pointer;
  font-size: 11px;
}

.status-grid,
.scheduler-info,
.quick-info {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.status-item,
.info-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.label {
  font-weight: bold;
  color: #555;
}

.value {
  color: #333;
}

.status-running {
  color: #28a745;
  font-weight: bold;
}

.scheduler-controls {
  margin-top: 10px;
}

.tick-btn {
  background-color: #ffc107;
  color: #333;
  border: none;
  padding: 8px 15px;
  cursor: pointer;
  font-size: 12px;
  font-weight: bold;
}

.system-controls {
  display: flex;
  gap: 10px;
}

.reboot-btn {
  background-color: #fd7e14;
  color: white;
  border: none;
  padding: 8px 15px;
  cursor: pointer;
  font-size: 12px;
  font-weight: bold;
}

.shutdown-btn {
  background-color: #dc3545;
  color: white;
  border: none;
  padding: 8px 15px;
  cursor: pointer;
  font-size: 12px;
  font-weight: bold;
}

.tick-btn:hover {
  background-color: #e0a800;
}

.reboot-btn:hover {
  background-color: #e8590c;
}

.shutdown-btn:hover {
  background-color: #c82333;
}

.clock-controls button:hover {
  background-color: #0056b3;
}
</style>
