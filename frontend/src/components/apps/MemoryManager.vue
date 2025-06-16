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

interface PagingInfo {
  total_pages: number;
  used_pages: number;
  free_pages: number;
}

interface MemoryStatus {
  total_memory: number;
  used_memory: number;
  allocation_strategy: number;
  free_blocks?: MemoryBlock[];
  partitions?: PartitionInfo[];
  paging?: PagingInfo;
}

const memoryStatus = ref<MemoryStatus | null>(null);
const processes = ref<any[]>([]);
const isLoading = ref(true);
const isChangingStrategy = ref(false);
const error = ref<string | null>(null);
const successMessage = ref<string | null>(null);
const selectedStrategy = ref(0);  // 当前选择的内存分配策略
let intervalId: number;

// 内存分配策略映射
const strategyNames = {
  0: '连续分配',
  1: '分区分配',
  2: '分页分配'
};

// 显示成功消息
const showSuccess = (message: string) => {
  successMessage.value = message;
  setTimeout(() => {
    successMessage.value = null;
  }, 3000);
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
    isChangingStrategy.value = true;
    const oldStrategy = memoryStatus.value?.allocation_strategy;
    const response = await memoryAPI.setStrategy(selectedStrategy.value);
    if (response.data.status === 'success') {
      showSuccess(`内存分配策略已从 ${strategyNames[oldStrategy as keyof typeof strategyNames]} 更改为 ${strategyNames[selectedStrategy.value as keyof typeof strategyNames]}`);
      await fetchMemoryData(); // 重新获取数据
    }
  } catch (err: any) {
    error.value = err.message || 'Failed to set memory strategy';
  } finally {
    isChangingStrategy.value = false;
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
      <div class="header-content">
        <h3>🧠 内存管理器</h3>
        <div class="header-subtitle">内存分配策略管理</div>
      </div>
      <button @click="fetchMemoryData" :disabled="isLoading">刷新</button>
    </div>

    <div v-if="successMessage" class="success">✅ {{ successMessage }}</div>
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

        <!-- 策略切换等待动画 -->
        <div v-if="isChangingStrategy" class="strategy-loading">
          <div class="loading-animation">
            <div class="loading-spinner">🧠</div>
            <div class="loading-text">正在切换内存分配策略...</div>
            <div class="loading-progress">
              <div class="progress-bar-loading">
                <div class="progress-fill-loading"></div>
              </div>
            </div>
          </div>
        </div>

        <div class="control-row">
          <select v-model="selectedStrategy" :disabled="isChangingStrategy">
            <option :value="0">连续分配</option>
            <option :value="1">分区分配</option>
            <option :value="2">分页分配</option>
          </select>
          <button @click="setMemoryStrategy" :disabled="isChangingStrategy || selectedStrategy === memoryStatus?.allocation_strategy">
            <span v-if="isChangingStrategy">⏳ 切换中...</span>
            <span v-else>🚀 更改策略</span>
          </button>
        </div>
      </div>

      <!-- 内存布局可视化 -->
      <div class="memory-layout">
        <h4>内存布局 ({{ strategyNames[memoryStatus?.allocation_strategy as keyof typeof strategyNames] }})</h4>

        <!-- 连续分配：显示空闲块 -->
        <div v-if="memoryStatus?.free_blocks && memoryStatus.allocation_strategy === 0" class="free-blocks">
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

        <!-- 分页分配：显示分页信息 -->
        <div v-if="memoryStatus?.paging && memoryStatus.allocation_strategy === 2" class="paging-info">
          <h5>分页内存信息</h5>
          <div class="paging-container">
            <div class="paging-stats">
              <div class="paging-stat-item">
                <span class="stat-label">总页框数:</span>
                <span class="stat-value">{{ memoryStatus.paging.total_pages }}</span>
              </div>
              <div class="paging-stat-item">
                <span class="stat-label">已使用页框:</span>
                <span class="stat-value used">{{ memoryStatus.paging.used_pages }}</span>
              </div>
              <div class="paging-stat-item">
                <span class="stat-label">空闲页框:</span>
                <span class="stat-value free">{{ memoryStatus.paging.free_pages }}</span>
              </div>
            </div>

            <!-- 分页使用率可视化 -->
            <div class="paging-visual">
              <div class="paging-bar">
                <div
                  class="paging-used-bar"
                  :style="{ width: (memoryStatus.paging.used_pages / memoryStatus.paging.total_pages * 100) + '%' }"
                  :title="`已使用: ${memoryStatus.paging.used_pages}页 / ${memoryStatus.paging.total_pages}页`"
                ></div>
              </div>
              <div class="paging-legend">
                <span class="legend-item">
                  <span class="legend-color used-color"></span>
                  已使用 ({{ memoryStatus.paging.used_pages }}页)
                </span>
                <span class="legend-item">
                  <span class="legend-color free-color"></span>
                  空闲 ({{ memoryStatus.paging.free_pages }}页)
                </span>
              </div>
            </div>

            <!-- 分页详细信息 -->
            <div class="paging-details">
              <div class="page-utilization">
                <span class="utilization-label">页框利用率:</span>
                <span class="utilization-value">
                  {{ ((memoryStatus.paging.used_pages / memoryStatus.paging.total_pages) * 100).toFixed(1) }}%
                </span>
              </div>
              <div class="page-size-info">
                <span class="info-label">平均页大小:</span>
                <span class="info-value">
                  {{ formatSize(memoryStatus.total_memory / memoryStatus.paging.total_pages) }}
                </span>
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

.header-content {
  display: flex;
  flex-direction: column;
}

.header-content h3 {
  margin: 0;
}

.header-subtitle {
  color: #666;
  font-size: 0.8em;
}

.success {
  background: linear-gradient(135deg, #d4edda 0%, #c3e6cb 100%);
  color: #155724;
  padding: 12px 16px;
  border-radius: 8px;
  margin-bottom: 20px;
  border: 1px solid #c3e6cb;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
  font-weight: 500;
}

.error {
  background: #f8d7da;
  color: #721c24;
  padding: 10px;
  border-radius: 4px;
  margin-bottom: 10px;
  border: 1px solid #f5c6cb;
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

.strategy-loading {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 20px;
  margin-bottom: 20px;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
  border-radius: 12px;
  border: 1px solid #dee2e6;
  box-shadow: 0 4px 12px rgba(0,0,0,0.1);
}

.loading-animation {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 15px;
}

.loading-spinner {
  font-size: 24px;
  animation: spin 2s linear infinite;
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

.loading-text {
  font-weight: 600;
  color: #495057;
  font-size: 14px;
  text-align: center;
}

.loading-progress {
  width: 200px;
  height: 8px;
  background: #e9ecef;
  border-radius: 4px;
  overflow: hidden;
  border: 1px solid #ccc;
  box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
}

.progress-bar-loading {
  background: linear-gradient(90deg, #007bff 0%, #28a745 50%, #ffc107 100%);
  width: 0%;
  animation: progressFill 2s ease-in-out infinite;
}

@keyframes progressFill {
  0% { width: 0%; }
  50% { width: 70%; }
  100% { width: 100%; }
}

/* 分页显示样式 */
.paging-info {
  border: 1px solid #ddd;
  border-radius: 8px;
  padding: 15px;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
  margin-top: 10px;
}

.paging-info h5 {
  margin: 0 0 15px 0;
  color: #495057;
  font-size: 14px;
  font-weight: 600;
  border-bottom: 2px solid #0078d4;
  padding-bottom: 5px;
}

.paging-container {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.paging-stats {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 15px;
}

.paging-stat-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
  background: #ffffff;
  padding: 12px;
  border-radius: 6px;
  border: 1px solid #dee2e6;
  box-shadow: 0 2px 4px rgba(0,0,0,0.05);
}

.stat-label {
  font-size: 11px;
  color: #6c757d;
  font-weight: 500;
  text-transform: uppercase;
}

.stat-value {
  font-size: 18px;
  font-weight: 700;
  color: #495057;
}

.stat-value.used {
  color: #dc3545;
}

.stat-value.free {
  color: #28a745;
}

.paging-visual {
  display: flex;
  flex-direction: column;
  gap: 10px;
  background: #ffffff;
  padding: 15px;
  border-radius: 6px;
  border: 1px solid #dee2e6;
}

.paging-bar {
  height: 20px;
  background: linear-gradient(135deg, #e9ecef 0%, #f8f9fa 100%);
  border: 1px solid #ced4da;
  border-radius: 10px;
  overflow: hidden;
  position: relative;
}

.paging-used-bar {
  height: 100%;
  background: linear-gradient(135deg, #dc3545 0%, #c82333 100%);
  border-radius: 10px;
  transition: width 0.5s ease-in-out;
  position: relative;
}

.paging-used-bar::after {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: linear-gradient(90deg, transparent 0%, rgba(255,255,255,0.3) 50%, transparent 100%);
  animation: shimmer 2s ease-in-out infinite;
}

@keyframes shimmer {
  0% { transform: translateX(-100%); }
  100% { transform: translateX(100%); }
}

.paging-legend {
  display: flex;
  justify-content: center;
  gap: 20px;
  flex-wrap: wrap;
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 12px;
  color: #495057;
  font-weight: 500;
}

.legend-color {
  width: 12px;
  height: 12px;
  border-radius: 2px;
  border: 1px solid rgba(0,0,0,0.1);
}

.used-color {
  background: linear-gradient(135deg, #dc3545 0%, #c82333 100%);
}

.free-color {
  background: linear-gradient(135deg, #28a745 0%, #20c997 100%);
}

.paging-details {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
}

.page-utilization,
.page-size-info {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: #ffffff;
  padding: 10px 15px;
  border-radius: 6px;
  border: 1px solid #dee2e6;
  box-shadow: 0 1px 3px rgba(0,0,0,0.05);
}

.utilization-label,
.info-label {
  font-size: 12px;
  color: #6c757d;
  font-weight: 500;
}

.utilization-value,
.info-value {
  font-size: 14px;
  font-weight: 700;
  color: #0078d4;
}

.utilization-value {
  background: linear-gradient(135deg, #e3f2fd 0%, #bbdefb 100%);
  padding: 4px 8px;
  border-radius: 4px;
  border: 1px solid #90caf9;
}
</style>
