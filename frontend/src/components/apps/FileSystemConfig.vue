<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { filesystemAPI } from '@/services/api';

interface FileSystemStatus {
  total_space: number;
  used_space: number;
  free_space: number;
  allocation_method: 'INDEXED' | 'LINKED' | 'CONTIGUOUS';
  block_size: number;
  total_blocks: number;
  used_blocks: number;
  total_inodes: number;
  used_inodes: number;
}

interface FileSystemLog {
  timestamp: string;
  operation: string;
  path: string;
  status: string;
  details?: string;
}

const fsStatus = ref<FileSystemStatus | null>(null);
const logs = ref<FileSystemLog[]>([]);
const isLoading = ref(false);
const error = ref<string | null>(null);
const selectedStrategy = ref<'INDEXED' | 'LINKED' | 'CONTIGUOUS'>('INDEXED');

const loadStatus = async () => {
  isLoading.value = true;
  error.value = null;
  try {
    const response = await filesystemAPI.getStatus();
    if (response.data.status === 'success') {
      fsStatus.value = response.data.data;
      selectedStrategy.value = response.data.data.allocation_method;
    }
  } catch (err: any) {
    error.value = `加载文件系统状态失败: ${err.response?.data?.message || err.message}`;
  } finally {
    isLoading.value = false;
  }
};

const loadLogs = async () => {
  try {
    const response = await filesystemAPI.getLogs();
    if (response.data.status === 'success') {
      logs.value = response.data.data || [];
    }
  } catch (err: any) {
    console.error('Failed to load logs:', err);
  }
};

const changeAllocationStrategy = async () => {
  try {
    await filesystemAPI.setAllocationStrategy(selectedStrategy.value);
    await loadStatus();
    alert(`分配策略已更改为: ${selectedStrategy.value}`);
  } catch (err: any) {
    error.value = `更改分配策略失败: ${err.response?.data?.message || err.message}`;
  }
};

const formatBytes = (bytes: number): string => {
  if (bytes >= 1024 * 1024 * 1024) {
    return `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
  }
  if (bytes >= 1024 * 1024) {
    return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
  }
  if (bytes >= 1024) {
    return `${(bytes / 1024).toFixed(2)} KB`;
  }
  return `${bytes} B`;
};

const getUsagePercentage = (used: number, total: number): number => {
  return total > 0 ? (used / total) * 100 : 0;
};

const getStrategyDescription = (strategy: string): string => {
  switch (strategy) {
    case 'INDEXED':
      return '索引分配：使用索引块存储文件块地址，支持随机访问，适合大文件';
    case 'LINKED':
      return '链接分配：每个块指向下一个块，节省空间但顺序访问';
    case 'CONTIGUOUS':
      return '连续分配：文件块连续存储，访问速度快但可能产生碎片';
    default:
      return '未知分配策略';
  }
};

onMounted(() => {
  loadStatus();
  loadLogs();
});
</script>

<template>
  <div class="filesystem-config">
    <div class="header">
      <h3>文件系统配置</h3>
      <button @click="loadStatus" :disabled="isLoading">
        {{ isLoading ? '加载中...' : '刷新状态' }}
      </button>
    </div>

    <div v-if="error" class="error">{{ error }}</div>

    <!-- 文件系统状态 -->
    <div v-if="fsStatus" class="status-section">
      <h4>📊 系统状态</h4>
      <div class="status-grid">
        <div class="status-item">
          <span class="label">总空间:</span>
          <span class="value">{{ formatBytes(fsStatus.total_space) }}</span>
        </div>
        <div class="status-item">
          <span class="label">已用空间:</span>
          <span class="value">{{ formatBytes(fsStatus.used_space) }}</span>
        </div>
        <div class="status-item">
          <span class="label">可用空间:</span>
          <span class="value">{{ formatBytes(fsStatus.free_space) }}</span>
        </div>
        <div class="status-item">
          <span class="label">使用率:</span>
          <span class="value">{{ getUsagePercentage(fsStatus.used_space, fsStatus.total_space).toFixed(1) }}%</span>
        </div>
      </div>

      <!-- 使用率进度条 -->
      <div class="progress-bar">
        <div class="progress-fill"
             :style="{ width: getUsagePercentage(fsStatus.used_space, fsStatus.total_space) + '%' }">
        </div>
      </div>

      <div class="status-grid">
        <div class="status-item">
          <span class="label">块大小:</span>
          <span class="value">{{ formatBytes(fsStatus.block_size) }}</span>
        </div>
        <div class="status-item">
          <span class="label">总块数:</span>
          <span class="value">{{ fsStatus.total_blocks.toLocaleString() }}</span>
        </div>
        <div class="status-item">
          <span class="label">已用块数:</span>
          <span class="value">{{ fsStatus.used_blocks.toLocaleString() }}</span>
        </div>
        <div class="status-item">
          <span class="label">Inode使用:</span>
          <span class="value">{{ fsStatus.used_inodes }}/{{ fsStatus.total_inodes }}</span>
        </div>
      </div>
    </div>

    <!-- 分配策略配置 -->
    <div class="strategy-section">
      <h4>⚙️ 分配策略</h4>
      <div class="current-strategy">
        <span class="label">当前策略:</span>
        <span class="strategy-name">{{ fsStatus?.allocation_method || '未知' }}</span>
      </div>
      <p class="strategy-description">
        {{ getStrategyDescription(fsStatus?.allocation_method || '') }}
      </p>

      <div class="strategy-selector">
        <label>选择新策略:</label>
        <select v-model="selectedStrategy">
          <option value="INDEXED">索引分配 (INDEXED)</option>
          <option value="LINKED">链接分配 (LINKED)</option>
          <option value="CONTIGUOUS">连续分配 (CONTIGUOUS)</option>
        </select>
        <button @click="changeAllocationStrategy"
                :disabled="selectedStrategy === fsStatus?.allocation_method">
          应用策略
        </button>
      </div>
    </div>

    <!-- 操作日志 -->
    <div class="logs-section">
      <div class="logs-header">
        <h4>📋 操作日志</h4>
        <button @click="loadLogs">刷新日志</button>
      </div>

      <div class="logs-container">
        <div v-if="logs.length === 0" class="no-logs">暂无操作日志</div>
        <div v-else class="logs-list">
          <div v-for="log in logs.slice(0, 10)" :key="log.timestamp" class="log-item">
            <div class="log-time">{{ new Date(log.timestamp).toLocaleString() }}</div>
            <div class="log-operation" :class="log.status.toLowerCase()">{{ log.operation }}</div>
            <div class="log-path">{{ log.path }}</div>
            <div class="log-status" :class="log.status.toLowerCase()">{{ log.status }}</div>
            <div v-if="log.details" class="log-details">{{ log.details }}</div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.filesystem-config {
  padding: 15px;
  font-size: 12px;
  color: #333;
  height: 100%;
  overflow-y: auto;
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding-bottom: 10px;
  border-bottom: 2px solid #e9ecef;
}

.header h3 {
  margin: 0;
  color: #2c3e50;
  font-weight: 600;
}

.header button {
  background: #007bff;
  color: white;
  border: none;
  padding: 6px 12px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 11px;
}

.header button:hover {
  background: #0056b3;
}

.header button:disabled {
  background: #6c757d;
  cursor: not-allowed;
}

.error {
  background: #f8d7da;
  color: #721c24;
  padding: 8px 12px;
  border-radius: 4px;
  margin-bottom: 15px;
  border: 1px solid #f5c6cb;
}

.status-section, .strategy-section, .logs-section {
  background: #f8f9fa;
  border-radius: 8px;
  padding: 15px;
  margin-bottom: 20px;
  border: 1px solid #e9ecef;
}

.status-section h4, .strategy-section h4, .logs-section h4 {
  margin: 0 0 15px 0;
  color: #495057;
  font-weight: 600;
}

.status-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 10px;
  margin-bottom: 15px;
}

.status-item {
  display: flex;
  justify-content: space-between;
  padding: 8px;
  background: white;
  border-radius: 4px;
  border: 1px solid #dee2e6;
}

.label {
  font-weight: 500;
  color: #6c757d;
}

.value {
  font-weight: 600;
  color: #2c3e50;
}

.progress-bar {
  width: 100%;
  height: 20px;
  background: #e9ecef;
  border-radius: 10px;
  overflow: hidden;
  margin-bottom: 15px;
}

.progress-fill {
  height: 100%;
  background: linear-gradient(90deg, #28a745 0%, #ffc107 70%, #dc3545 90%);
  transition: width 0.3s ease;
}

.current-strategy {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 10px;
}

.strategy-name {
  background: #007bff;
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  font-weight: 600;
  font-size: 11px;
}

.strategy-description {
  background: #e3f2fd;
  padding: 10px;
  border-radius: 4px;
  margin: 10px 0;
  font-style: italic;
  color: #1565c0;
  border-left: 4px solid #2196f3;
}

.strategy-selector {
  display: flex;
  align-items: center;
  gap: 10px;
  flex-wrap: wrap;
}

.strategy-selector label {
  font-weight: 500;
  color: #495057;
}

.strategy-selector select {
  padding: 6px 10px;
  border: 1px solid #ced4da;
  border-radius: 4px;
  background: white;
  font-size: 11px;
}

.strategy-selector button {
  background: #28a745;
  color: white;
  border: none;
  padding: 6px 12px;
  border-radius: 4px;
  cursor: pointer;
  font-size: 11px;
}

.strategy-selector button:hover {
  background: #218838;
}

.strategy-selector button:disabled {
  background: #6c757d;
  cursor: not-allowed;
}

.logs-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
}

.logs-header button {
  background: #6c757d;
  color: white;
  border: none;
  padding: 4px 8px;
  border-radius: 3px;
  cursor: pointer;
  font-size: 10px;
}

.logs-container {
  max-height: 300px;
  overflow-y: auto;
}

.no-logs {
  text-align: center;
  color: #6c757d;
  padding: 20px;
  font-style: italic;
}

.logs-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.log-item {
  background: white;
  border: 1px solid #dee2e6;
  border-radius: 4px;
  padding: 8px;
  font-size: 10px;
}

.log-time {
  color: #6c757d;
  font-size: 9px;
  margin-bottom: 4px;
}

.log-operation {
  font-weight: 600;
  margin-bottom: 2px;
}

.log-operation.success { color: #28a745; }
.log-operation.fail { color: #dc3545; }

.log-path {
  color: #495057;
  font-family: monospace;
  margin-bottom: 2px;
}

.log-status {
  font-weight: 500;
  font-size: 9px;
  padding: 2px 6px;
  border-radius: 2px;
  display: inline-block;
}

.log-status.success {
  background: #d4edda;
  color: #155724;
}

.log-status.fail {
  background: #f8d7da;
  color: #721c24;
}

.log-details {
  color: #6c757d;
  font-size: 9px;
  margin-top: 4px;
  padding-top: 4px;
  border-top: 1px solid #f8f9fa;
}
</style>
