<script setup lang="ts">
import { ref, onMounted, watch } from 'vue';
import { filesystemAPI } from '@/services/api';

interface FileAddress {
  path: string;
  addresses: {
    contiguous?: number;
    linked?: number;
    indexed?: number;
  };
}

const props = defineProps<{
  filePath: string;
}>();

const fileAddressInfo = ref<FileAddress | null>(null);
const currentAllocationStrategy = ref<string>('');
const isLoading = ref(false);
const error = ref<string | null>(null);

// 获取当前分配策略
const getCurrentAllocationStrategy = async () => {
  try {
    const response = await filesystemAPI.getStatus();
    if (response.data.status === 'success') {
      currentAllocationStrategy.value = response.data.data.allocation_method;
    }
  } catch (err) {
    console.error('Failed to get allocation strategy:', err);
  }
};

// 加载文件地址信息
const loadFileAddress = async () => {
  if (!props.filePath) return;

  isLoading.value = true;
  error.value = null;

  try {
    const response = await filesystemAPI.getFileAddress(props.filePath);
    if (response.data.status === 'success') {
      fileAddressInfo.value = response.data.data;
      await getCurrentAllocationStrategy();
    }
  } catch (err: any) {
    error.value = `获取文件地址失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  } finally {
    isLoading.value = false;
  }
};

// 刷新文件地址信息
const refreshFileAddress = async () => {
  await loadFileAddress();
};

// 监听文件路径变化
watch(() => props.filePath, () => {
  loadFileAddress();
}, { immediate: true });

onMounted(() => {
  loadFileAddress();
});
</script>

<template>
  <div class="file-address-viewer">
    <div class="header">
      <div class="header-content">
        <h3>📍 文件存储地址信息</h3>
        <div class="header-subtitle">查看文件在不同分配策略下的存储位置</div>
      </div>
      <button @click="refreshFileAddress" :disabled="isLoading" class="refresh-btn">
        <span v-if="isLoading">⏳ 加载中...</span>
        <span v-else>🔄 刷新</span>
      </button>
    </div>

    <div v-if="error" class="error">❌ {{ error }}</div>

    <div v-if="isLoading && !fileAddressInfo" class="loading">
      <div class="loading-spinner">⏳</div>
      <div class="loading-text">正在加载文件地址信息...</div>
    </div>

    <div v-if="fileAddressInfo" class="content">
      <!-- 当前策略信息 -->
      <div class="strategy-info">
        <div class="current-strategy-badge">
          <span class="strategy-label">当前策略:</span>
          <span class="strategy-value" :class="currentAllocationStrategy.toLowerCase()">
            {{ currentAllocationStrategy }}
          </span>
        </div>
      </div>

      <!-- 文件路径 -->
      <div class="file-path">
        <div class="path-label">📁 文件路径:</div>
        <div class="path-value">{{ fileAddressInfo.path }}</div>
      </div>

      <!-- 地址信息 -->
      <div class="address-section">
        <h4>📦 各策略下的存储地址:</h4>
        <div class="address-grid">
          <div class="address-item" :class="{ active: currentAllocationStrategy === 'CONTIGUOUS' }">
            <div class="address-type">
              <span class="strategy-icon">🔗</span>
              <span>连续分配</span>
            </div>
            <div class="address-value">
              <span v-if="fileAddressInfo.addresses.contiguous !== null && fileAddressInfo.addresses.contiguous !== undefined">
                块 #{{ fileAddressInfo.addresses.contiguous }}
              </span>
              <span v-else class="no-address">未分配</span>
            </div>
          </div>
          <div class="address-item" :class="{ active: currentAllocationStrategy === 'LINKED' }">
            <div class="address-type">
              <span class="strategy-icon">🔗</span>
              <span>链接分配</span>
            </div>
            <div class="address-value">
              <span v-if="fileAddressInfo.addresses.linked !== null && fileAddressInfo.addresses.linked !== undefined">
                起始块 #{{ fileAddressInfo.addresses.linked }}
              </span>
              <span v-else class="no-address">未分配</span>
            </div>
          </div>
          <div class="address-item" :class="{ active: currentAllocationStrategy === 'INDEXED' }">
            <div class="address-type">
              <span class="strategy-icon">📇</span>
              <span>索引分配</span>
            </div>
            <div class="address-value">
              <span v-if="fileAddressInfo.addresses.indexed !== null && fileAddressInfo.addresses.indexed !== undefined">
                索引块 #{{ fileAddressInfo.addresses.indexed }}
              </span>
              <span v-else class="no-address">未分配</span>
            </div>
          </div>
        </div>
        <div class="address-note">
          💡 当前激活的分配策略以高亮显示。切换分配策略后，文件会重新分配到新的存储位置。
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.file-address-viewer {
  padding: 20px;
  font-size: 12px;
  color: #333;
  height: 100%;
  overflow-y: auto;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding-bottom: 15px;
  border-bottom: 2px solid #e9ecef;
}

.header-content {
  display: flex;
  flex-direction: column;
}

.header-content h3 {
  margin: 0;
  color: #2c3e50;
  font-weight: 600;
  font-size: 16px;
}

.header-content .header-subtitle {
  color: #6c757d;
  font-size: 12px;
  margin-top: 4px;
}

.refresh-btn {
  background: linear-gradient(135deg, #007bff 0%, #0056b3 100%);
  color: white;
  border: none;
  padding: 8px 16px;
  border-radius: 6px;
  cursor: pointer;
  font-size: 11px;
  font-weight: 600;
  transition: all 0.2s ease;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
}

.refresh-btn:hover {
  background: linear-gradient(135deg, #0056b3 0%, #004085 100%);
  transform: translateY(-1px);
  box-shadow: 0 4px 8px rgba(0,0,0,0.2);
}

.refresh-btn:disabled {
  background: #6c757d;
  cursor: not-allowed;
  transform: none;
}

.error {
  background: linear-gradient(135deg, #f8d7da 0%, #f5c6cb 100%);
  color: #721c24;
  padding: 12px 16px;
  border-radius: 8px;
  margin-bottom: 20px;
  border: 1px solid #f5c6cb;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
}

.loading {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 40px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 4px 12px rgba(0,0,0,0.1);
}

.loading-spinner {
  font-size: 32px;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

.loading-text {
  margin-top: 12px;
  color: #6c757d;
  font-weight: 500;
}

.content {
  background: white;
  border-radius: 12px;
  padding: 20px;
  box-shadow: 0 4px 12px rgba(0,0,0,0.1);
}

.strategy-info {
  display: flex;
  justify-content: center;
  margin-bottom: 25px;
  padding: 15px;
  background: linear-gradient(135deg, #e3f2fd 0%, #bbdefb 100%);
  border-radius: 10px;
  border: 1px solid #2196f3;
}

.current-strategy-badge {
  display: flex;
  align-items: center;
  gap: 12px;
}

.strategy-label {
  font-weight: 600;
  color: #1565c0;
  font-size: 13px;
}

.strategy-value {
  padding: 6px 16px;
  border-radius: 20px;
  font-weight: 700;
  font-size: 12px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.strategy-value.contiguous {
  background: linear-gradient(135deg, #4caf50 0%, #66bb6a 100%);
  color: white;
  box-shadow: 0 2px 4px rgba(76, 175, 80, 0.3);
}

.strategy-value.linked {
  background: linear-gradient(135deg, #ff9800 0%, #ffb74d 100%);
  color: white;
  box-shadow: 0 2px 4px rgba(255, 152, 0, 0.3);
}

.strategy-value.indexed {
  background: linear-gradient(135deg, #2196f3 0%, #42a5f5 100%);
  color: white;
  box-shadow: 0 2px 4px rgba(33, 150, 243, 0.3);
}

.file-path {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-bottom: 25px;
  padding: 15px;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
  border-radius: 8px;
  border: 1px solid #ced4da;
}

.path-label {
  font-weight: 600;
  color: #495057;
  font-size: 12px;
}

.path-value {
  font-family: 'Courier New', monospace;
  background: white;
  padding: 8px 12px;
  border-radius: 6px;
  border: 1px solid #dee2e6;
  color: #2c3e50;
  font-weight: 600;
  font-size: 13px;
}

.address-section h4 {
  margin: 0 0 20px 0;
  color: #495057;
  font-weight: 600;
  font-size: 14px;
}

.address-grid {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 20px;
}

.address-item {
  display: grid;
  grid-template-columns: 1fr 1fr;
  align-items: center;
  padding: 15px;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  border-radius: 8px;
  border: 2px solid #e9ecef;
  transition: all 0.3s ease;
}

.address-item.active {
  background: linear-gradient(135deg, #e8f5e8 0%, #c8e6c8 100%);
  border-color: #28a745;
  box-shadow: 0 4px 12px rgba(40, 167, 69, 0.2);
  transform: translateY(-2px);
}

.address-type {
  display: flex;
  align-items: center;
  gap: 10px;
  font-weight: 600;
  color: #495057;
  font-size: 13px;
}

.strategy-icon {
  font-size: 16px;
}

.address-value {
  text-align: right;
  font-family: 'Courier New', monospace;
  background: linear-gradient(135deg, #e9ecef 0%, #dee2e6 100%);
  padding: 8px 12px;
  border-radius: 6px;
  font-weight: 600;
  color: #2c3e50;
  font-size: 12px;
  border: 1px solid #ced4da;
}

.address-item.active .address-value {
  background: linear-gradient(135deg, #d4edda 0%, #c3e6cb 100%);
  border-color: #28a745;
  color: #155724;
}

.no-address {
  color: #6c757d;
  font-style: italic;
  font-weight: normal;
}

.address-note {
  background: linear-gradient(135deg, #fff3cd 0%, #ffeaa7 100%);
  border: 1px solid #ffc107;
  border-radius: 8px;
  padding: 12px;
  font-size: 11px;
  color: #856404;
  line-height: 1.5;
  font-weight: 500;
}
</style>
