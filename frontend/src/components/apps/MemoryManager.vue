<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import { memoryAPI, processAPI } from '@/services/api';

interface MemoryBlock {
  base_address: number;
  size: number;
}

interface PartitionInfo {
  base_address: number;
  size: number;
  is_free: boolean;
  owner_pid: number;
}

interface MemoryStatus {
  total_memory: number;
  used_memory: number;
  allocation_strategy: number;
  free_blocks?: MemoryBlock[];
  partitions?: PartitionInfo[];
}

const memoryStatus = ref<MemoryStatus | null>(null);
const processes = ref<any[]>([]);
const isLoading = ref(true);
const error = ref<string | null>(null);
const selectedStrategy = ref(0);  // 当前选择的内存分配策略
let intervalId: number;

// 内存分配策略映射
const strategyNames = {
  0: '连续分配',
  1: '分区分配',
  2: '分页分配'
};

const fetchMemoryData = async () => {
  try {
    const [statusRes, processRes] = await Promise.all([
      memoryAPI.getStatus(),
      processAPI.getProcesses()
    ]);

    if (statusRes.data.status === 'success') {
      memoryStatus.value = statusRes.data.data;
      selectedStrategy.value = statusRes.data.data.allocation_strategy;
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

// 设置内存分配策略
const setMemoryStrategy = async () => {
  try {
    const response = await memoryAPI.setStrategy(selectedStrategy.value);
    if (response.data.status === 'success') {
      alert(`内存分配策略已更改为: ${strategyNames[selectedStrategy.value as keyof typeof strategyNames]}`);
      await fetchMemoryData(); // 重新获取数据
    }
  } catch (err: any) {
    error.value = err.message || 'Failed to set memory strategy';
    alert('设置内存分配策略失败: ' + error.value);
  }
};

const getBlockColor = (block: MemoryBlock | PartitionInfo, isPid: boolean = false) => {
  if ('is_free' in block && block.is_free) return '#e8f5e8';
  if (isPid && 'owner_pid' in block) {
    return `hsl(${(block.owner_pid || 0) * 50 % 360}, 70%, 80%)`;
  }
  return '#b8e6b8'; // 默认占用颜色
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
            <span class="value">{{ formatSize(memoryStatus.total_memory) }}</span>
          </div>
          <div class="status-item">
            <span class="label">已使用:</span>
            <span class="value">{{ formatSize(memoryStatus.used_memory) }}</span>
          </div>
          <div class="status-item">
            <span class="label">空闲:</span>
            <span class="value">{{ formatSize(memoryStatus.total_memory - memoryStatus.used_memory) }}</span>
          </div>
          <div class="status-item">
            <span class="label">当前策略:</span>
            <span class="value">{{ strategyNames[memoryStatus.allocation_strategy as keyof typeof strategyNames] }}</span>
          </div>
        </div>

        <!-- 内存使用率进度条 -->
        <div class="memory-bar">
          <div
            class="memory-used"
            :style="{ width: (memoryStatus.used_memory / memoryStatus.total_memory * 100) + '%' }"
          ></div>
        </div>
      </div>

      <!-- 内存分配策略控制 -->
      <div class="strategy-control">
        <h4>内存分配策略</h4>
        <div class="control-row">
          <select v-model="selectedStrategy">
            <option :value="0">连续分配</option>
            <option :value="1">分区分配</option>
            <option :value="2">分页分配</option>
          </select>
          <button @click="setMemoryStrategy">更改策略</button>
        </div>
      </div>

      <!-- 内存布局可视化 -->
      <div class="memory-layout">
        <h4>内存布局 ({{ strategyNames[memoryStatus?.allocation_strategy as keyof typeof strategyNames] }})</h4>

        <!-- 连续分配和分页分配：显示空闲块 -->
        <div v-if="memoryStatus?.free_blocks && (memoryStatus.allocation_strategy === 0 || memoryStatus.allocation_strategy === 2)" class="free-blocks">
          <h5>空闲内存块</h5>
          <div class="blocks-container">
            <div
              v-for="(block, index) in memoryStatus.free_blocks"
              :key="index"
              class="memory-block free-block"
              :title="`地址: 0x${block.base_address.toString(16)}\n大小: ${formatSize(block.size)}\n状态: 空闲`"
            >
              <div class="block-info">
                <span class="address">0x{{ block.base_address.toString(16) }}</span>
                <span class="size">{{ formatSize(block.size) }}</span>
                <span class="status">空闲</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 分区分配：显示分区信息 -->
        <div v-if="memoryStatus?.partitions && memoryStatus.allocation_strategy === 1" class="partitions">
          <h5>内存分区</h5>
          <div class="blocks-container">
            <div
              v-for="(partition, index) in memoryStatus.partitions"
              :key="index"
              class="memory-block"
              :class="{ 'free-block': partition.is_free, 'used-block': !partition.is_free }"
              :style="{
                backgroundColor: getBlockColor(partition, true)
              }"
              :title="`地址: 0x${partition.base_address.toString(16)}\n大小: ${formatSize(partition.size)}\n状态: ${partition.is_free ? '空闲' : '占用'}\n${!partition.is_free ? `进程PID: ${partition.owner_pid}` : ''}`"
            >
              <div class="block-info">
                <span class="address">0x{{ partition.base_address.toString(16) }}</span>
                <span class="size">{{ formatSize(partition.size) }}</span>
                <span v-if="partition.is_free" class="status">空闲</span>
                <span v-else class="owner">PID: {{ partition.owner_pid }}</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- 进程内存使用情况 -->
      <div class="process-memory">
        <h4>进程内存占用</h4>
        <div v-if="processes.length === 0" class="no-processes">
          暂无进程
        </div>
        <div v-else class="process-list">
          <div v-for="proc in processes" :key="proc.pid" class="process-item">
            <div class="process-info">
              <span class="pid">PID: {{ proc.pid }}</span>
              <span class="state">状态: {{ proc.state }}</span>
            </div>
            <div class="memory-blocks">
              <div v-for="(block, index) in proc.memory_info" :key="index" class="memory-block-item">
                <span class="address">0x{{ block.base_address.toString(16).toUpperCase() }}</span>
                <span class="size">{{ formatSize(block.size) }}</span>
              </div>
            </div>
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

.strategy-control {
  border: 1px solid #ddd;
  border-radius: 5px;
  padding: 10px;
  background-color: #f8f9fa;
}

.strategy-control h4 {
  margin: 0 0 10px 0;
  color: #495057;
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
  background-color: #007bff;
  color: white;
  border: none;
  padding: 5px 10px;
  cursor: pointer;
  font-size: 12px;
  border-radius: 3px;
}

.control-row button:hover {
  background-color: #0056b3;
}

.memory-layout {
  border: 1px solid #ddd;
  border-radius: 5px;
  padding: 10px;
}

.blocks-container {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  max-height: 300px;
  overflow-y: auto;
  margin-top: 10px;
}

.free-blocks h5, .partitions h5 {
  margin: 10px 0 5px 0;
  color: #495057;
  font-size: 14px;
}

.free-block {
  background-color: #d4edda !important;
  border-color: #c3e6cb !important;
}

.used-block {
  border-color: #f5c6cb !important;
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

.no-processes {
  text-align: center;
  color: #6c757d;
  padding: 20px;
  font-style: italic;
}

.process-info {
  display: flex;
  gap: 15px;
  align-items: center;
  margin-bottom: 8px;
}

.memory-blocks {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.memory-block-item {
  background-color: #e9ecef;
  border: 1px solid #dee2e6;
  border-radius: 3px;
  padding: 4px 8px;
  font-size: 10px;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.memory-block-item .address {
  font-family: 'Courier New', monospace;
  font-weight: bold;
  color: #007bff;
}

.memory-block-item .size {
  color: #6c757d;
}
</style>
