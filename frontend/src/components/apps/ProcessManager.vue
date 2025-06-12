<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import { processAPI, schedulerAPI } from '@/services/api';

interface MemoryBlock {
  base_address: number;
  size: number;
}

interface Process {
  pid: number;
  name: string;
  state: string;
  program_counter: number;
  memory_info: MemoryBlock[];
}

const processes = ref<Process[]>([]);
const readyQueue = ref<Process[]>([]);
const isLoading = ref(true);
const error = ref<string | null>(null);
const newProcessName = ref('');
const newProcessSize = ref(1024);
let intervalId: number;

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

onMounted(() => {
  fetchProcesses();
  fetchReadyQueue();
  intervalId = window.setInterval(() => {
    fetchProcesses();
    fetchReadyQueue();
  }, 2000); // Refresh every 2 seconds
});

const createProcess = async () => {
  if (!newProcessName.value.trim() || newProcessSize.value <= 0) {
    alert('请输入有效的进程名和内存大小');
    return;
  }

  try {
    await processAPI.createProcess(newProcessName.value, newProcessSize.value);
    newProcessName.value = '';
    newProcessSize.value = 1024;
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

onUnmounted(() => {
  clearInterval(intervalId);
});
</script>

<template>
  <div class="process-manager">
    <div class="header">
      <h3>进程管理器</h3>
      <button @click="fetchProcesses" :disabled="isLoading">刷新</button>
    </div>

    <div class="create-process">
      <h4>创建新进程</h4>
      <div class="create-form">
        <input v-model="newProcessName" placeholder="进程名" />
        <input v-model.number="newProcessSize" type="number" placeholder="内存大小 (bytes)" min="1" />
        <button @click="createProcess">创建进程</button>
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
              <th>进程名</th>
              <th>状态</th>
              <th>程序计数器</th>
              <th>内存首地址</th>
              <th>内存大小 (KB)</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="proc in processes" :key="proc.pid">
              <td>{{ proc.pid }}</td>
              <td>{{ proc.name || 'N/A' }}</td>
              <td>{{ proc.state }}</td>
              <td>{{ proc.program_counter }}</td>
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
            <span>PID: {{ proc.pid }} ({{ proc.name || 'Unknown' }})</span>
          </div>
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

.create-process {
  padding: 10px;
  border-bottom: 1px solid #eee;
  background-color: #fafafa;
}

.create-process h4 {
  margin: 0 0 10px 0;
  color: #222;
  font-weight: 600;
}

.create-form {
  display: flex;
  gap: 10px;
  align-items: center;
}

.create-form input {
  padding: 5px;
  font-size: 12px;
  color: #333;
  border: 1px solid #ccc;
  border-radius: 3px;
}

.create-form button {
  background-color: #28a745;
  color: white;
  border: none;
  padding: 5px 10px;
  cursor: pointer;
  font-size: 12px;
}

.process-layout {
  display: flex;
  flex-grow: 1;
  overflow: hidden;
}

.process-list-container {
  flex-grow: 1;
  overflow-y: auto;
  padding: 10px;
}

.ready-queue-container {
  width: 200px;
  border-left: 1px solid #ccc;
  padding: 10px;
  overflow-y: auto;
  background-color: #f9f9f9;
}

.ready-queue-container h4 {
  margin-top: 0;
  border-bottom: 1px solid #ddd;
  padding-bottom: 5px;
}

.ready-queue-item {
  padding: 5px;
  margin-bottom: 5px;
  background-color: #fff;
  border: 1px solid #eee;
  border-radius: 3px;
  font-size: 11px;
}

.loading, .error-message {
  padding: 20px;
  text-align: center;
}

.error-message {
  background-color: #ffe6e6;
  color: #d8000c;
}

.process-table {
  width: 100%;
  border-collapse: collapse;
}

th, td {
  border: 1px solid #ddd;
  padding: 8px;
  text-align: left;
  font-size: 11px;
  color: #333;
}

th {
  color: #222;
  font-weight: 600;
}

thead {
  background-color: #f2f2f2;
}

.terminate-btn {
  background-color: #dc3545;
  color: white;
  border: none;
  padding: 3px 8px;
  cursor: pointer;
  font-size: 10px;
}

.terminate-btn:hover {
  background-color: #c82333;
}

.create-form button:hover {
  background-color: #218838;
}

.memory-address {
  font-family: 'Courier New', monospace;
  font-weight: bold;
  color: #007bff;
  background-color: #f8f9fa;
  padding: 2px 4px;
  border-radius: 3px;
  font-size: 10px;
}

.no-memory {
  color: #6c757d;
  font-style: italic;
}
</style>
