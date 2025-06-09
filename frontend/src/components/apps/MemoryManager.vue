<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import { memoryAPI, processAPI } from '@/services/api';

interface MemoryBlock {
  base_address: number;
  size: number;
  owner_pid?: number;
  is_free: boolean;
}

interface MemoryStatus {
  total_size: number;
  used_size: number;
  free_size: number;
  fragmentation: number;
}

const memoryStatus = ref<MemoryStatus | null>(null);
const memoryLayout = ref<MemoryBlock[]>([]);
const processes = ref<any[]>([]);
const isLoading = ref(true);
const error = ref<string | null>(null);
const selectedPid = ref<number | null>(null);
const allocationSize = ref(1024);
let intervalId: number;

const fetchMemoryData = async () => {
  try {
    const [statusRes, layoutRes, processRes] = await Promise.all([
      memoryAPI.getStatus(),
      memoryAPI.getLayout(),
      processAPI.getProcesses()
    ]);

    if (statusRes.data.status === 'success') {
      memoryStatus.value = statusRes.data.data;
    }

    if (layoutRes.data.status === 'success') {
      memoryLayout.value = layoutRes.data.data || [];
    }

    if (processRes.data.status === 'success') {
      processes.value = processRes.data.data || [];
    }

    error.value = null;
  } catch (err: any) {
    error.value = err.message || 'Failed to fetch memory data';
  } finally {
    isLoading.value = false;
  }
};

const allocateMemory = async () => {
  if (!selectedPid.value || allocationSize.value <= 0) {
    alert('请选择进程并输入有效的内存大小');
    return;
  }

  try {
    await memoryAPI.allocate(selectedPid.value, allocationSize.value);
    fetchMemoryData();
    alert('内存分配成功！');
  } catch (err: any) {
    error.value = err.message || 'Failed to allocate memory';
  }
};

const deallocateMemory = async (pid: number) => {
  if (!confirm(`确定要释放进程 ${pid} 的所有内存吗？`)) return;

  try {
    await memoryAPI.deallocate(pid);
    fetchMemoryData();
    alert('内存释放成功！');
  } catch (err: any) {
    error.value = err.message || 'Failed to deallocate memory';
  }
};

const getBlockColor = (block: MemoryBlock) => {
  if (block.is_free) return '#e8f5e8';
  return `hsl(${(block.owner_pid || 0) * 50 % 360}, 70%, 80%)`;
};

const formatSize = (bytes: number) => {
  if (bytes >= 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  if (bytes >= 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${bytes} B`;
};

onMounted(() => {
  fetchMemoryData();
  intervalId = window.setInterval(fetchMemoryData, 3000);
});

onUnmounted(() => {
  clearInterval(intervalId);
});
</script>

<template>
  <div class="memory-manager">
    <div class="header">
      <h3>内存管理器</h3>
      <button @click="fetchMemoryData" :disabled="isLoading">刷新</button>
    </div>

    <div v-if="error" class="error">{{ error }}</div>

    <div v-if="isLoading" class="loading">加载中...</div>

    <div v-else class="content">
      <!-- 内存状态概览 -->
      <div class="memory-status" v-if="memoryStatus">
        <h4>内存状态</h4>
        <div class="status-grid">
          <div class="status-item">
            <span class="label">总内存:</span>
            <span class="value">{{ formatSize(memoryStatus.total_size) }}</span>
          </div>
          <div class="status-item">
            <span class="label">已使用:</span>
            <span class="value">{{ formatSize(memoryStatus.used_size) }}</span>
          </div>
          <div class="status-item">
            <span class="label">空闲:</span>
            <span class="value">{{ formatSize(memoryStatus.free_size) }}</span>
          </div>
          <div class="status-item">
            <span class="label">碎片化:</span>
            <span class="value">{{ (memoryStatus.fragmentation * 100).toFixed(1) }}%</span>
          </div>
        </div>

        <!-- 内存使用率进度条 -->
        <div class="memory-bar">
          <div
            class="memory-used"
            :style="{ width: (memoryStatus.used_size / memoryStatus.total_size * 100) + '%' }"
          ></div>
        </div>
      </div>

      <!-- 内存分配控制 -->
      <div class="allocation-control">
        <h4>内存分配</h4>
        <div class="control-row">
          <select v-model="selectedPid">
            <option :value="null">选择进程</option>
            <option v-for="proc in processes" :key="proc.pid" :value="proc.pid">
              PID: {{ proc.pid }} ({{ proc.state }})
            </option>
          </select>
          <input
            v-model.number="allocationSize"
            type="number"
            placeholder="内存大小 (bytes)"
            min="1"
          />
          <button @click="allocateMemory">分配内存</button>
        </div>
      </div>

      <!-- 内存布局可视化 -->
      <div class="memory-layout">
        <h4>内存布局</h4>
        <div class="layout-container">
          <div
            v-for="(block, index) in memoryLayout"
            :key="index"
            class="memory-block"
            :style="{
              backgroundColor: getBlockColor(block),
              height: Math.max(20, (block.size / 1024)) + 'px',
              minHeight: '20px'
            }"
            :title="`地址: 0x${block.base_address.toString(16)}\n大小: ${formatSize(block.size)}\n${block.is_free ? '空闲' : `进程 ${block.owner_pid}`}`"
          >
            <div class="block-info">
              <span class="address">0x{{ block.base_address.toString(16) }}</span>
              <span class="size">{{ formatSize(block.size) }}</span>
              <span v-if="!block.is_free" class="owner">PID: {{ block.owner_pid }}</span>
              <span v-if="block.is_free" class="free">空闲</span>
              <button
                v-if="!block.is_free && block.owner_pid"
                @click="deallocateMemory(block.owner_pid)"
                class="deallocate-btn"
              >
                释放
              </button>
            </div>
          </div>
        </div>
      </div>

      <!-- 进程内存使用情况 -->
      <div class="process-memory">
        <h4>进程内存使用</h4>
        <div class="process-list">
          <div v-for="proc in processes" :key="proc.pid" class="process-item">
            <span class="pid">PID: {{ proc.pid }}</span>
            <span class="memory-usage">
              {{ formatSize(proc.memory_info?.reduce((sum: number, block: any) => sum + block.size, 0) || 0) }}
            </span>
            <button @click="deallocateMemory(proc.pid)" class="release-btn">释放内存</button>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.memory-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  font-size: 12px;
  overflow-y: auto;
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
  display: flex;
  flex-direction: column;
  gap: 15px;
}

.memory-status {
  border: 1px solid #ddd;
  border-radius: 5px;
  padding: 10px;
}

.status-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-bottom: 10px;
}

.status-item {
  display: flex;
  justify-content: space-between;
}

.label {
  font-weight: bold;
}

.value {
  color: #666;
}

.memory-bar {
  height: 20px;
  background-color: #f0f0f0;
  border: 1px solid #ccc;
  border-radius: 3px;
  overflow: hidden;
}

.memory-used {
  height: 100%;
  background-color: #007acc;
  transition: width 0.3s;
}

.allocation-control {
  border: 1px solid #ddd;
  border-radius: 5px;
  padding: 10px;
}

.control-row {
  display: flex;
  gap: 10px;
  align-items: center;
}

.control-row select,
.control-row input {
  padding: 5px;
  font-size: 12px;
}

.control-row button {
  background-color: #28a745;
  color: white;
  border: none;
  padding: 5px 10px;
  cursor: pointer;
  font-size: 12px;
}

.memory-layout {
  border: 1px solid #ddd;
  border-radius: 5px;
  padding: 10px;
}

.layout-container {
  display: flex;
  flex-wrap: wrap;
  gap: 2px;
  max-height: 200px;
  overflow-y: auto;
}

.memory-block {
  border: 1px solid #999;
  border-radius: 3px;
  padding: 5px;
  margin: 1px;
  min-width: 150px;
  position: relative;
  cursor: pointer;
}

.block-info {
  display: flex;
  flex-direction: column;
  gap: 2px;
  font-size: 10px;
}

.address {
  font-family: monospace;
  font-weight: bold;
}

.size {
  color: #666;
}

.owner {
  color: #007acc;
  font-weight: bold;
}

.free {
  color: #28a745;
  font-weight: bold;
}

.deallocate-btn {
  background-color: #dc3545;
  color: white;
  border: none;
  padding: 2px 4px;
  font-size: 9px;
  cursor: pointer;
  margin-top: 2px;
}

.process-memory {
  border: 1px solid #ddd;
  border-radius: 5px;
  padding: 10px;
}

.process-list {
  display: flex;
  flex-direction: column;
  gap: 5px;
}

.process-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 5px;
  background-color: #f9f9f9;
  border-radius: 3px;
}

.pid {
  font-weight: bold;
}

.memory-usage {
  color: #666;
}

.release-btn {
  background-color: #dc3545;
  color: white;
  border: none;
  padding: 3px 6px;
  font-size: 10px;
  cursor: pointer;
}
</style>
