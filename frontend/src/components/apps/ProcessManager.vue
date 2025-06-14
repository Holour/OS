<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import { processAPI, schedulerAPI } from '@/services/api';

interface MemoryBlock {
  base_address: number;
  size: number;
}

interface Process {
  pid: number;
  name?: string;
  state: string;
  program_counter: number;
  cpu_time?: number;
  priority?: number;
  creation_time?: number;
  memory_info: MemoryBlock[];
}

interface SchedulerConfig {
  algorithm: string;
  time_slice?: number;
}

interface GanttItem {
  pid: number;
  start: number;
  end: number;
}

const processes = ref<Process[]>([]);
const readyQueue = ref<Process[]>([]);
const isLoading = ref(true);
const error = ref<string | null>(null);
const newProcessSize = ref(1024);
const newProcessCPUTime = ref(10);
const newProcessPriority = ref(5);
let intervalId: number;

const schedulerConfig = ref<SchedulerConfig>({ algorithm: 'FCFS', time_slice: 1 });

const algorithmOptions = ['FCFS', 'SJF', 'PRIORITY', 'RR'];
const selectedAlgorithm = ref('FCFS');
const timeSlice = ref(1);

const ganttData = ref<GanttItem[]>([]);
const minStart = computed(() => (ganttData.value.length ? Math.min(...ganttData.value.map(g => g.start)) : 0));
const maxEnd = computed(() => (ganttData.value.length ? Math.max(...ganttData.value.map(g => g.end)) : 0));
const totalDuration = computed(() => maxEnd.value - minStart.value || 1);

// 时间格式化
const formatTime = (ms: number | undefined) => {
  if (!ms) return 'N/A';
  const date = new Date(ms);
  return date.toLocaleString();
};

const fetchProcesses = async () => {
  try {
    const response = await processAPI.getProcesses();
    if (response.data.status === 'success') {
      processes.value = response.data.data;
    } else {
      throw new Error(response.data.message || 'Failed to fetch processes');
    }
  } catch (err: any) {
    error.value = err.message || 'An unknown error occurred';
  } finally {
    isLoading.value = false;
  }
};

const fetchReadyQueue = async () => {
  try {
    const response = await schedulerAPI.getReadyQueue();
    if (response.data.status === 'success') {
      readyQueue.value = response.data.data;
    }
  } catch (err) {
    console.error('Failed to fetch ready queue:', err);
  }
};

const fetchSchedulerConfig = async () => {
  try {
    const res = await schedulerAPI.getStatus();
    if (res.data.status === 'success') {
      schedulerConfig.value = res.data.data;
      selectedAlgorithm.value = schedulerConfig.value.algorithm;
      if (schedulerConfig.value.time_slice) {
        timeSlice.value = schedulerConfig.value.time_slice;
      }
    }
  } catch (err) {
    console.error('Failed to fetch scheduler config:', err);
  }
};

const updateSchedulerConfig = async () => {
  try {
    await schedulerAPI.setConfig(selectedAlgorithm.value, selectedAlgorithm.value === 'RR' ? timeSlice.value : undefined);
    await fetchSchedulerConfig();

    // 清空当前甘特图数据，为新的调度算法准备
    ganttData.value = [];

    // 立即刷新数据以获取新的甘特图
    await Promise.all([
      fetchProcesses(),
      fetchReadyQueue(),
      fetchGanttData()
    ]);

    alert('调度器配置已更新');
  } catch (err: any) {
    error.value = err.message || 'Failed to update scheduler config';
  }
};

const fetchGanttData = async () => {
  try {
    const res = await schedulerAPI.getGanttChart();
    if (res.data.status === 'success' && Array.isArray(res.data.data)) {
      // 总是更新甘特图数据，不管是否为空
      ganttData.value = res.data.data;
      console.log('甘特图数据已更新:', res.data.data);
    }
  } catch (err) {
    console.error('Failed to fetch gantt data:', err);
  }
};

onMounted(() => {
  fetchProcesses();
  fetchReadyQueue();
  fetchSchedulerConfig();
  fetchGanttData();
  intervalId = window.setInterval(() => {
    fetchProcesses();
    fetchReadyQueue();
    fetchSchedulerConfig();
    fetchGanttData();
  }, 3000); // Refresh every 3 seconds
});

const createProcess = async () => {
  if (newProcessSize.value <= 0) {
    alert('请输入有效的内存大小');
    return;
  }

  try {
    await processAPI.createProcess(newProcessSize.value, newProcessCPUTime.value, newProcessPriority.value);
    newProcessSize.value = 1024;
    newProcessCPUTime.value = 10;
    newProcessPriority.value = 5;
    fetchProcesses();
    alert('进程创建成功！');
  } catch (err: any) {
    error.value = err.message || 'Failed to create process';
  }
};

const terminateProcess = async (pid: number) => {
  if (!confirm(`确定要终止进程 ${pid} 吗？`)) return;

  try {
    await processAPI.terminateProcess(pid);
    fetchProcesses();
    alert('进程终止成功！');
  } catch (err: any) {
    error.value = err.message || 'Failed to terminate process';
  }
};

const executeTick = async () => {
  try {
    const response = await schedulerAPI.tick();
    if (response.data.status === 'success') {
      // 执行调度后立即刷新数据
      await Promise.all([
        fetchProcesses(),
        fetchReadyQueue(),
        fetchGanttData()
      ]);

      if (response.data.data) {
        console.log('调度执行成功，当前运行进程:', response.data.data);
      } else {
        console.log('调度执行成功，但就绪队列为空');
      }
    }
  } catch (err: any) {
    console.error('调度执行失败:', err);
    error.value = err.message || 'Failed to execute scheduler tick';
  }
};

onUnmounted(() => {
  clearInterval(intervalId);
});
</script>

<template>
  <div class="process-manager">
    <!-- 调度器配置置顶 -->
    <div class="scheduler-config top-config">
      <h3>调度器配置</h3>
      <div class="config-form">
        <label>算法:</label>
        <select v-model="selectedAlgorithm">
          <option v-for="alg in algorithmOptions" :key="alg" :value="alg">{{ alg }}</option>
        </select>
        <label v-show="selectedAlgorithm === 'RR'">时间片:</label>
        <input
          v-show="selectedAlgorithm === 'RR'"
          v-model.number="timeSlice"
          type="number"
          min="1"
          class="timeslice-input"
          placeholder="ms"
        />
        <button @click="updateSchedulerConfig">更新</button>
        <button @click="executeTick">执行调度</button>
        <span class="current-config">
          当前: {{ schedulerConfig.algorithm }}<span v-if="schedulerConfig.algorithm === 'RR'"> / {{ schedulerConfig.time_slice }}ms</span>
        </span>
      </div>
    </div>

    <div class="create-process">
      <h3>创建新进程</h3>
      <div class="create-form">
        <input v-model.number="newProcessSize" type="number" placeholder="内存大小 (bytes)" min="1" />
        <input v-model.number="newProcessCPUTime" type="number" placeholder="CPU 时间 (ms)" min="1" />
        <input v-model.number="newProcessPriority" type="number" placeholder="优先级 (数字越小越高)" min="1" />
        <button @click="createProcess">创建</button>
      </div>
    </div>

    <div v-if="isLoading" class="loading">加载进程中...</div>
    <div v-else-if="error" class="error-message">错误: {{ error }}</div>

    <div v-else class="process-layout">
      <div class="process-list-container">
        <table class="process-table">
          <thead>
            <tr>
              <th>PID</th>
              <th>状态</th>
              <th>程序计数器</th>
              <th>CPU 时间 (ms)</th>
              <th>优先级</th>
              <th>创建时间</th>
              <th>内存首地址</th>
              <th>内存大小 (KB)</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="proc in processes" :key="proc.pid">
              <td>{{ proc.pid }}</td>
              <td>{{ proc.state }}</td>
              <td>{{ proc.program_counter }}</td>
              <td>{{ proc.cpu_time || 'N/A' }}</td>
              <td>{{ proc.priority ?? 'N/A' }}</td>
              <td>{{ formatTime(proc.creation_time) }}</td>
              <td>
                <span v-if="proc.memory_info.length > 0" class="memory-address">
                  0x{{ proc.memory_info[0].base_address.toString(16).toUpperCase() }}
                </span>
                <span v-else class="no-memory">无内存</span>
              </td>
              <td>{{ (proc.memory_info.reduce((sum: number, block: MemoryBlock) => sum + block.size, 0) / 1024).toFixed(2) }}</td>
              <td>
                <button @click="terminateProcess(proc.pid)" class="terminate-btn">终止</button>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
      <div class="ready-queue-container">
        <h4>就绪队列</h4>
        <div v-if="readyQueue.length === 0" class="empty-queue">队列为空</div>
        <div v-else>
          <div v-for="proc in readyQueue" :key="proc.pid" class="ready-queue-item">
            <span>PID: {{ proc.pid }}</span>
          </div>
        </div>
      </div>
    </div>

    <div class="gantt-section">
      <div class="gantt-header">
        <h4>调度甘特图</h4>
        <button @click="fetchGanttData" class="refresh-btn" title="手动刷新甘特图">
          🔄 刷新
        </button>
      </div>
      <div v-if="ganttData.length" class="gantt-chart">
        <div class="gantt-scroll-container">
          <div class="gantt-bar" :style="{ minWidth: Math.max(600, ganttData.length * 80) + 'px' }">
            <div
              v-for="(item, index) in ganttData"
              :key="index"
              class="gantt-block"
              :style="{
                left: ((item.start - minStart) / totalDuration * 100) + '%',
                width: ((item.end - item.start) / totalDuration * 100) + '%',
                backgroundColor: `hsl(${item.pid * 40 % 360},70%,70%)`
              }"
            >
              PID {{ item.pid }}
            </div>
          </div>
          <div v-if="ganttData.length > 5" class="scroll-hint">
            ← → 横向滚动查看更多
          </div>
        </div>
        <div class="gantt-time-axis">
          <div class="time-marker" style="left: 0%">{{ minStart }}ms</div>
          <div class="time-marker" style="right: 0%">{{ maxEnd }}ms</div>
        </div>
      </div>
      <div v-else class="empty-queue">
        <div class="empty-content">
          <div class="empty-icon">📊</div>
          <div>暂无甘特图数据</div>
          <div class="empty-tip">请先创建进程并运行调度器</div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.process-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  font-size: 13px;
  color: #323130;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: linear-gradient(135deg, #f8f9fa 0%, #f1f3f4 100%);
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 20px;
  border-bottom: 1px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
}

.header h3 {
  margin: 0;
  color: #323130;
  font-weight: 600;
  font-size: 16px;
}

.create-process {
  padding: 16px 20px;
  border-bottom: 1px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
}

.create-process h3 {
  margin: 0 0 12px 0;
  color: #323130;
  font-weight: 600;
  font-size: 15px;
}

.create-form {
  display: flex;
  gap: 12px;
  align-items: center;
  flex-wrap: wrap;
}

.create-form input {
  padding: 8px 12px;
  font-size: 13px;
  color: #323130;
  border: 2px solid #e1e5e9;
  border-radius: 6px;
  background: #ffffff;
  transition: all 0.2s ease;
  min-width: 140px;
}

.create-form input:focus {
  outline: none;
  border-color: #0078d4;
  box-shadow: 0 0 0 2px rgba(0, 120, 212, 0.2);
}

.create-form input:hover {
  border-color: #c7c6c4;
}

.create-form button {
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  color: white;
  border: none;
  padding: 8px 16px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  border-radius: 6px;
  transition: all 0.2s ease;
  box-shadow: 0 2px 4px rgba(0, 120, 212, 0.2);
}

.create-form button:hover {
  background: linear-gradient(135deg, #106ebe 0%, #005a9e 100%);
  box-shadow: 0 4px 8px rgba(0, 120, 212, 0.3);
  transform: translateY(-1px);
}

.create-form button:active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(0, 120, 212, 0.2);
}

.process-layout {
  display: flex;
  flex-grow: 1;
  overflow: hidden;
  gap: 1px;
}

.process-list-container {
  flex-grow: 1;
  overflow-y: auto;
  padding: 20px;
  background: #ffffff;
}

.ready-queue-container {
  width: 240px;
  border-left: 2px solid #e1e5e9;
  padding: 20px;
  overflow-y: auto;
  background: linear-gradient(135deg, #f8f9fa 0%, #f1f3f4 100%);
}

.ready-queue-container h4 {
  margin-top: 0;
  margin-bottom: 16px;
  border-bottom: 2px solid #0078d4;
  padding-bottom: 8px;
  color: #323130;
  font-weight: 600;
  font-size: 14px;
}

.ready-queue-item {
  padding: 12px;
  margin-bottom: 8px;
  background: #ffffff;
  border: 1px solid #e1e5e9;
  border-radius: 8px;
  font-size: 12px;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
  transition: all 0.2s ease;
}

.ready-queue-item:hover {
  background: #f8f9fa;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
  transform: translateY(-1px);
}

.empty-queue {
  text-align: center;
  color: #8a8886;
  font-style: italic;
  padding: 20px;
  background: #ffffff;
  border-radius: 8px;
  border: 2px dashed #e1e5e9;
}

.loading, .error-message {
  padding: 40px 20px;
  text-align: center;
  font-size: 14px;
}

.error-message {
  background: linear-gradient(135deg, #fdf2f2 0%, #fef5f5 100%);
  color: #d13438;
  border-radius: 8px;
  margin: 20px;
  border-left: 4px solid #d13438;
}

.process-table {
  width: 100%;
  border-collapse: separate;
  border-spacing: 0;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  background: #ffffff;
}

th, td {
  border: none;
  border-bottom: 1px solid #f3f2f1;
  padding: 12px 16px;
  text-align: left;
  font-size: 12px;
  color: #323130;
}

th {
  color: #323130;
  font-weight: 600;
  font-size: 13px;
  background: linear-gradient(135deg, #f8f9fa 0%, #f1f3f4 100%);
  position: sticky;
  top: 0;
  z-index: 10;
}

.process-table tr:hover {
  background: linear-gradient(135deg, #f8f9fa 0%, #f1f3f4 100%);
}

.process-table tr:nth-child(even) {
  background: #fafafa;
}

.process-table tr:nth-child(even):hover {
  background: linear-gradient(135deg, #f1f3f4 0%, #e8eaed 100%);
}

.terminate-btn {
  background: linear-gradient(135deg, #d13438 0%, #a4262c 100%);
  color: white;
  border: none;
  padding: 6px 12px;
  cursor: pointer;
  font-size: 11px;
  font-weight: 600;
  border-radius: 6px;
  transition: all 0.2s ease;
  box-shadow: 0 2px 4px rgba(209, 52, 56, 0.2);
}

.terminate-btn:hover {
  background: linear-gradient(135deg, #a4262c 0%, #8b1f23 100%);
  box-shadow: 0 4px 8px rgba(209, 52, 56, 0.3);
  transform: translateY(-1px);
}

.terminate-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(209, 52, 56, 0.2);
}

.memory-address {
  font-family: 'Cascadia Code', 'Courier New', monospace;
  font-weight: 600;
  color: #0078d4;
  background: linear-gradient(135deg, #f3f9fd 0%, #e1f5fe 100%);
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 11px;
  border: 1px solid #bee1f4;
}

.no-memory {
  color: #8a8886;
  font-style: italic;
}

.scheduler-config {
  padding: 16px 20px;
  border-bottom: 2px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.05);
}

.scheduler-config h3 {
  margin: 0 0 12px 0;
  color: #323130;
  font-weight: 600;
  font-size: 15px;
}

.config-form {
  display: flex;
  gap: 12px;
  align-items: center;
  flex-wrap: wrap;
  margin-top: 8px;
}

.config-form label {
  font-weight: 600;
  color: #323130;
  font-size: 13px;
}

.config-form select {
  padding: 8px 12px;
  border: 2px solid #e1e5e9;
  border-radius: 6px;
  background: #ffffff;
  color: #323130;
  font-size: 13px;
  transition: all 0.2s ease;
  min-width: 120px;
}

.config-form select:focus {
  outline: none;
  border-color: #0078d4;
  box-shadow: 0 0 0 2px rgba(0, 120, 212, 0.2);
}

.config-form select:hover {
  border-color: #c7c6c4;
}

.config-form button {
  background: linear-gradient(135deg, #107c10 0%, #0e6e0e 100%);
  color: white;
  border: none;
  padding: 8px 16px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  border-radius: 6px;
  transition: all 0.2s ease;
  box-shadow: 0 2px 4px rgba(16, 124, 16, 0.2);
}

.config-form button:hover {
  background: linear-gradient(135deg, #0e6e0e 0%, #0c5c0c 100%);
  box-shadow: 0 4px 8px rgba(16, 124, 16, 0.3);
  transform: translateY(-1px);
}

.config-form button:active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(16, 124, 16, 0.2);
}

/* 执行调度按钮特殊样式 */
.config-form button:nth-of-type(2) {
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  box-shadow: 0 2px 4px rgba(0, 120, 212, 0.2);
}

.config-form button:nth-of-type(2):hover {
  background: linear-gradient(135deg, #106ebe 0%, #005a9e 100%);
  box-shadow: 0 4px 8px rgba(0, 120, 212, 0.3);
  transform: translateY(-1px);
}

.config-form button:nth-of-type(2):active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(0, 120, 212, 0.2);
}

.current-config {
  font-size: 12px;
  color: #605e5c;
  background: linear-gradient(135deg, #f3f2f1 0%, #edebe9 100%);
  padding: 6px 12px;
  border-radius: 4px;
  border: 1px solid #e1dfdd;
}

.gantt-section {
  padding: 16px 20px;
  border-top: 2px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.05);
}

.gantt-section h4 {
  margin: 0 0 12px 0;
  color: #323130;
  font-weight: 600;
  font-size: 14px;
}

.gantt-chart {
  position: relative;
  margin-top: 12px;
  height: 50px;
  border: 2px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  border-radius: 8px;
  overflow: hidden;
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.05);
}

.gantt-scroll-container {
  position: relative;
  overflow-x: auto;
  overflow-y: hidden;
  height: 100%;
  width: 100%;
  scrollbar-width: thin;
  scrollbar-color: #c7c6c4 #f1f1f1;
}

.gantt-scroll-container::-webkit-scrollbar {
  height: 6px;
}

.gantt-scroll-container::-webkit-scrollbar-track {
  background: #f1f1f1;
  border-radius: 3px;
}

.gantt-scroll-container::-webkit-scrollbar-thumb {
  background: linear-gradient(135deg, #c7c6c4 0%, #a19f9d 100%);
  border-radius: 3px;
  transition: background 0.2s ease;
}

.gantt-scroll-container::-webkit-scrollbar-thumb:hover {
  background: linear-gradient(135deg, #a19f9d 0%, #8a8886 100%);
}

.gantt-bar {
  position: relative;
  height: 100%;
}

.gantt-block {
  position: absolute;
  top: 0;
  height: 100%;
  color: #ffffff;
  font-size: 12px;
  font-weight: 600;
  text-align: center;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  border-right: 2px solid #ffffff;
  display: flex;
  align-items: center;
  justify-content: center;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
  transition: all 0.2s ease;
}

.gantt-block:hover {
  transform: scaleY(1.1);
  z-index: 5;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
}

/* 顶部调度器区域样式优化 */
.top-config {
  border-bottom: 2px solid #0078d4;
  background: linear-gradient(135deg, #f3f9fd 0%, #e1f5fe 100%);
  box-shadow: 0 2px 6px rgba(0, 120, 212, 0.1);
}

.top-config .config-form {
  flex-wrap: wrap;
}

.timeslice-input {
  width: 80px;
}

.gantt-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.refresh-btn {
  background: linear-gradient(135deg, #f3f2f1 0%, #e1dfdd 100%);
  border: 1px solid #d2d0ce;
  border-radius: 6px;
  padding: 6px 12px;
  cursor: pointer;
  color: #323130;
  font-size: 12px;
  font-weight: 500;
  transition: all 0.2s ease;
  display: flex;
  align-items: center;
  gap: 4px;
}

.refresh-btn:hover {
  background: linear-gradient(135deg, #e1dfdd 0%, #d2d0ce 100%);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  transform: translateY(-1px);
}

.refresh-btn:active {
  transform: translateY(0);
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.gantt-time-axis {
  display: flex;
  justify-content: space-between;
  margin-top: 8px;
}

.time-marker {
  font-size: 12px;
  color: #605e5c;
}

.empty-content {
  text-align: center;
  padding: 20px;
  background: #ffffff;
  border-radius: 8px;
  border: 2px dashed #e1e5e9;
}

.empty-icon {
  font-size: 40px;
  margin-bottom: 12px;
}

.empty-tip {
  font-size: 12px;
  color: #8a8886;
  font-style: italic;
}

/* 通用滚动条美化 */
::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}

::-webkit-scrollbar-track {
  background: #f1f1f1;
  border-radius: 4px;
}

::-webkit-scrollbar-thumb {
  background: linear-gradient(135deg, #c7c6c4 0%, #a19f9d 100%);
  border-radius: 4px;
  transition: background 0.2s ease;
}

::-webkit-scrollbar-thumb:hover {
  background: linear-gradient(135deg, #a19f9d 0%, #8a8886 100%);
}

.scroll-hint {
  position: absolute;
  top: 50%;
  right: 10px;
  transform: translateY(-50%);
  background: rgba(0, 0, 0, 0.7);
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 10px;
  white-space: nowrap;
  pointer-events: none;
  opacity: 0.8;
  animation: fadeInOut 3s ease-in-out infinite;
}

@keyframes fadeInOut {
  0%, 100% { opacity: 0.3; }
  50% { opacity: 0.8; }
}
</style>
