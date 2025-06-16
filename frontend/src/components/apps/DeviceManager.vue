<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import { deviceAPI, processAPI } from '@/services/api';
import { useDialogs } from '@/composables/useDialogs';

interface Device {
  device_id: number;
  name: string;
  type: string;
  status: string;
  current_user: number | null;
}

interface DeviceOperation {
  id: string;
  deviceId: number;
  operation: string;
  timestamp: string;
  processId?: number;
  result: 'success' | 'error';
  message: string;
}

interface DeviceStats {
  totalOperations: number;
  successRate: number;
  avgResponseTime: number;
  busyTime: number;
}

const devices = ref<Device[]>([]);
const processes = ref<any[]>([]);
const selectedDevice = ref<Device | null>(null);
const isLoading = ref(true);
const autoRefresh = ref(true);
const refreshInterval = ref<number | null>(null);
const selectedOperation = ref('');
const selectedDeviceType = ref('');
const selectedProcessId = ref<number | null>(null);

const { alert, success, error: showError, confirm } = useDialogs();

// 设备操作历史
const operationHistory = ref<DeviceOperation[]>([]);

// 设备统计信息
const deviceStats = ref<Map<number, DeviceStats>>(new Map());

// 设备类型分组
const devicesByType = computed(() => {
  const grouped: Record<string, Device[]> = {};
  devices.value.forEach(device => {
    if (!grouped[device.type]) {
      grouped[device.type] = [];
    }
    grouped[device.type].push(device);
  });
  return grouped;
});

// 设备状态统计
const statusStats = computed(() => {
  const stats = { IDLE: 0, BUSY: 0, ERROR: 0 };
  devices.value.forEach(device => {
    if (stats.hasOwnProperty(device.status)) {
      stats[device.status as keyof typeof stats]++;
    }
  });
  return stats;
});

// 繁忙设备列表
const busyDevices = computed(() =>
  devices.value.filter(device => device.status === 'BUSY')
);

// 空闲设备列表
const idleDevices = computed(() =>
  devices.value.filter(device => device.status === 'IDLE')
);

// 获取设备列表
const fetchDevices = async () => {
  try {
    const response = await deviceAPI.getDevices();
    console.log('Device API response:', response.data); // 调试日志

    if (response.data.status === 'success') {
      const deviceData = response.data.data || [];
      devices.value = deviceData.map((device: any) => ({
        device_id: device.device_id,
        name: device.name || `Device-${device.device_id}`,
        type: device.type,
        status: device.status,
        current_user: device.current_user
      }));
      console.log('Mapped devices:', devices.value); // 调试日志
    } else {
      console.warn('Device API returned non-success status:', response.data);
      // 如果API返回了非成功状态，使用模拟数据
      loadMockDevices();
    }
  } catch (error) {
    console.error("Failed to fetch devices:", error);
    addOperationHistory(0, 'FETCH_DEVICES', 'error', '获取设备列表失败，使用模拟数据');
    // 在API失败时，加载一些模拟设备数据
    loadMockDevices();
  }
};

// 加载模拟设备数据（用于测试和演示）
const loadMockDevices = () => {
  devices.value = [
    {
      device_id: 1,
      name: '激光打印机-01',
      type: 'PRINTER',
      status: 'IDLE',
      current_user: null
    },
    {
      device_id: 2,
      name: '高速磁盘-01',
      type: 'DISK',
      status: 'BUSY',
      current_user: 101
    },
    {
      device_id: 3,
      name: '网络适配器-01',
      type: 'NETWORK',
      status: 'IDLE',
      current_user: null
    },
    {
      device_id: 4,
      name: '扫描仪-01',
      type: 'SCANNER',
      status: 'BUSY',
      current_user: 102
    },
    {
      device_id: 5,
      name: 'USB控制器-01',
      type: 'USB',
      status: 'ERROR',
      current_user: null
    }
  ];

  // 添加一些模拟历史记录
  addOperationHistory(1, 'MOCK_DATA', 'success', '加载模拟设备数据');
  addOperationHistory(2, 'REQUEST', 'success', '进程101请求磁盘设备', 101);
  addOperationHistory(4, 'REQUEST', 'success', '进程102请求扫描仪设备', 102);
};

// 获取进程列表
const fetchProcesses = async () => {
  try {
    const response = await processAPI.getProcesses();
    if (response.data.status === 'success') {
      processes.value = response.data.data;
    }
  } catch (error) {
    console.error("Failed to fetch processes:", error);
  }
};

// 获取进程名称
const getProcessName = (pid: number | null): string => {
  if (!pid) return 'N/A';
  const process = processes.value.find(p => p.pid === pid);
  return process ? `PID-${pid}` : `Unknown-${pid}`;
};

// 设备操作：请求设备
const requestDevice = async (deviceId: number, processId: number) => {
  try {
    const response = await deviceAPI.requestDevice(deviceId, processId);
    if (response.data.status === 'success') {
      const deviceInfo = response.data.data;
      addOperationHistory(
        deviceInfo.device_id,
        'REQUEST',
        'success',
        `进程 ${processId} 成功请求设备 ${deviceInfo.name}`,
        processId
      );
      await fetchDevices();
      return deviceInfo;
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message;
    addOperationHistory(deviceId, 'REQUEST', 'error', `请求设备失败: ${errorMsg}`, processId);
    throw error;
  }
};

// 设备操作：释放设备
const releaseDevice = async (deviceId: number, processId: number) => {
  try {
    const response = await deviceAPI.releaseDevice(deviceId, processId);
    if (response.data.status === 'success') {
      addOperationHistory(deviceId, 'RELEASE', 'success', `进程 ${processId} 释放设备成功`, processId);
      await fetchDevices();
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message;
    addOperationHistory(deviceId, 'RELEASE', 'error', `释放设备失败: ${errorMsg}`);
    throw error;
  }
};

// 设备操作：删除设备
const deleteDevice = async (deviceId: number) => {
  try {
    const response = await deviceAPI.deleteDevice(deviceId);
    if (response.data.status === 'success') {
      addOperationHistory(
        deviceId,
        'DELETE',
        'success',
        '设备删除成功'
      );
      await fetchDevices();
      // 如果删除的是当前选中的设备，清空选择
      if (selectedDevice.value?.device_id === deviceId) {
        selectedDevice.value = null;
      }
      return true;
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message;
    addOperationHistory(
      deviceId,
      'DELETE',
      'error',
      `删除设备失败: ${errorMsg}`
    );
    throw error;
  }
};

// 带进程ID的设备申请
const requestDeviceWithProcess = async (deviceId: number) => {
  if (!selectedProcessId.value) {
    showError('请先选择一个进程');
    return;
  }

  try {
    await requestDevice(deviceId, selectedProcessId.value);
    success('设备申请成功');
  } catch (error) {
    console.error('申请设备失败:', error);
  }
};

// 带进程ID的设备释放
const releaseDeviceWithProcess = async (deviceId: number) => {
  if (!selectedProcessId.value) {
    showError('请先选择一个进程');
    return;
  }

  try {
    await releaseDevice(deviceId, selectedProcessId.value);
    success('设备释放成功');
  } catch (error) {
    console.error('释放设备失败:', error);
  }
};

// 确认删除设备
const confirmDeleteDevice = async (deviceId: number) => {
  const device = devices.value.find(d => d.device_id === deviceId);
  if (!device) return;

  let confirmMessage = `确定要删除设备 "${device.name}" 吗？\n\n此操作不可恢复。`;

  // 如果设备是BUSY状态，需要特殊处理
  if (device.status === 'BUSY') {
    confirmMessage = `设备 "${device.name}" 正在被进程 ${device.current_user} 使用。\n\n确定要强制删除吗？这将：\n1. 先释放设备\n2. 然后删除设备\n\n此操作不可恢复。`;
  }

  try {
    const confirmed = await confirm(confirmMessage, '删除设备');
    if (confirmed) {
      await deleteDeviceWithForce(deviceId);
      success('设备删除成功');
    }
  } catch (error) {
    console.error('删除设备失败:', error);
  }
};

// 强制删除设备（处理BUSY状态）
const deleteDeviceWithForce = async (deviceId: number) => {
  const device = devices.value.find(d => d.device_id === deviceId);
  if (!device) return;

  try {
    // 如果设备是BUSY状态，先尝试释放
    if (device.status === 'BUSY' && device.current_user) {
      addOperationHistory(
        deviceId,
        'FORCE_RELEASE',
        'success',
        `开始强制释放设备，当前用户: 进程 ${device.current_user}`
      );

      try {
        // 尝试释放设备
        await deviceAPI.releaseDevice(deviceId, device.current_user);
        addOperationHistory(
          deviceId,
          'FORCE_RELEASE',
          'success',
          `设备已从进程 ${device.current_user} 强制释放`
        );

        // 等待一下让状态更新
        await new Promise(resolve => setTimeout(resolve, 500));
        await fetchDevices();

      } catch (releaseError: any) {
        console.warn('释放设备失败，继续尝试删除:', releaseError);
        addOperationHistory(
          deviceId,
          'FORCE_RELEASE',
          'error',
          `释放失败: ${releaseError.response?.data?.message || releaseError.message}，继续删除操作`
        );
      }
    }

    // 执行删除操作
    await deleteDevice(deviceId);

  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message;
    addOperationHistory(
      deviceId,
      'FORCE_DELETE',
      'error',
      `强制删除失败: ${errorMsg}`
    );
    throw error;
  }
};

// 设备操作：执行操作
const executeDeviceOperation = async (deviceId: number, operation: string, params: any = {}) => {
  try {
    const response = await deviceAPI.deviceOperation(deviceId, operation, params);
    if (response.data.status === 'success') {
      addOperationHistory(
        deviceId,
        operation.toUpperCase(),
        'success',
        `操作执行成功: ${operation}`
      );
      await fetchDevices();
      return response.data.data;
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message;
    addOperationHistory(
      deviceId,
      operation.toUpperCase(),
      'error',
      `操作执行失败: ${errorMsg}`
    );
    throw error;
  }
};

// 添加操作历史记录
const addOperationHistory = (
  deviceId: number,
  operation: string,
  result: 'success' | 'error',
  message: string,
  processId?: number
) => {
  const historyItem: DeviceOperation = {
    id: `${Date.now()}-${Math.random()}`,
    deviceId,
    operation,
    timestamp: new Date().toLocaleString('zh-CN'),
    processId,
    result,
    message
  };

  operationHistory.value.unshift(historyItem);

  // 保持历史记录在合理数量内
  if (operationHistory.value.length > 100) {
    operationHistory.value = operationHistory.value.slice(0, 100);
  }

  // 更新设备统计
  updateDeviceStats(deviceId, result);
};

// 更新设备统计
const updateDeviceStats = (deviceId: number, result: 'success' | 'error') => {
  const current = deviceStats.value.get(deviceId) || {
    totalOperations: 0,
    successRate: 100,
    avgResponseTime: 0,
    busyTime: 0
  };

  current.totalOperations++;
  const successCount = operationHistory.value
    .filter(op => op.deviceId === deviceId && op.result === 'success')
    .length;
  current.successRate = (successCount / current.totalOperations) * 100;

  deviceStats.value.set(deviceId, current);
};

// 设备选择处理
const selectDevice = (device: Device) => {
  selectedDevice.value = device;
};

// 获取设备图标
const getDeviceIcon = (type: string): string => {
  const icons: Record<string, string> = {
    'PRINTER': '🖨️',
    'DISK': '💽',
    'NETWORK': '🌐',
    'KEYBOARD': '⌨️',
    'MOUSE': '🖱️',
    'MONITOR': '🖥️',
    'SPEAKER': '🔊',
    'CAMERA': '📷',
    'USB': '🔌',
    'SCANNER': '📸',
    'MICROPHONE': '🎤',
    'HEADPHONE': '🎧',
    'TOUCHPAD': '📱',
    'GRAPHICS': '🎮',
    'BLUETOOTH': '📶',
    'WIFI': '📡',
    'ETHERNET': '🔗',
    'MODEM': '📞',
    'BATTERY': '🔋',
    'POWER': '⚡',
    'FAN': '🌪️',
    'TEMPERATURE': '🌡️',
    'SENSOR': '📊'
  };
  return icons[type] || '🔧';
};

// 获取状态颜色
const getStatusColor = (status: string): string => {
  const colors: Record<string, string> = {
    'IDLE': '#28a745',
    'BUSY': '#ffc107',
    'ERROR': '#dc3545'
  };
  return colors[status] || '#6c757d';
};

// 格式化设备利用率
const getDeviceUtilization = (device: Device): number => {
  const stats = deviceStats.value.get(device.device_id);
  if (!stats) return 0;
  return Math.min(100, (stats.busyTime / (Date.now() - 3600000)) * 100); // 简化计算
};

// 刷新数据
const refreshData = async () => {
  isLoading.value = true;
  try {
    await Promise.all([fetchDevices(), fetchProcesses()]);
  } finally {
    isLoading.value = false;
  }
};

// 切换自动刷新
const toggleAutoRefresh = () => {
  autoRefresh.value = !autoRefresh.value;
  if (autoRefresh.value) {
    startAutoRefresh();
  } else {
    stopAutoRefresh();
  }
};

// 开始自动刷新
const startAutoRefresh = () => {
  refreshInterval.value = window.setInterval(refreshData, 3000);
};

// 停止自动刷新
const stopAutoRefresh = () => {
  if (refreshInterval.value) {
    clearInterval(refreshInterval.value);
    refreshInterval.value = null;
  }
};

// 清除操作历史
const clearHistory = () => {
  operationHistory.value = [];
  deviceStats.value.clear();
};

// 执行高级操作
const executeAdvancedOperation = async () => {
  if (!selectedDevice.value || !selectedOperation.value) return;

  const operation = selectedOperation.value;
  const deviceId = selectedDevice.value.device_id;

  // 根据操作类型设置不同的参数
  const operationParams: Record<string, any> = {
    'RESET': { force: true, save_state: false },
    'CALIBRATE': { precision: 'high', auto_adjust: true },
    'DIAGNOSTIC': { full_scan: true, report_level: 'detailed' },
    'MAINTENANCE': { schedule: Date.now() + 3600000, duration: 1800 }, // 1小时后，30分钟维护
    'SUSPEND': { save_context: true },
    'RESUME': { restore_context: true }
  };

  try {
    await executeDeviceOperation(deviceId, operation, operationParams[operation] || {});
    selectedOperation.value = ''; // 清空选择
  } catch (error) {
    console.error('Advanced operation failed:', error);
  }
};

// 模拟设备队列管理
const simulateDeviceQueue = async (deviceType: string) => {
  // 模拟多个进程请求同一类型设备的场景
  const queueSize = Math.floor(Math.random() * 5) + 1;

  for (let i = 0; i < queueSize; i++) {
    setTimeout(async () => {
      try {
        await requestDevice(deviceType);
        addOperationHistory(
          0,
          'QUEUE_REQUEST',
          'success',
          `队列请求 ${i + 1}/${queueSize} 完成`
        );
      } catch (error) {
        addOperationHistory(
          0,
          'QUEUE_REQUEST',
          'error',
          `队列请求 ${i + 1}/${queueSize} 失败`
        );
      }
    }, i * 1000); // 每秒发起一个请求
  }

  addOperationHistory(
    0,
    'QUEUE_SIMULATION',
    'success',
    `开始模拟设备队列，${queueSize} 个请求已加入队列`
  );
};

// 导出设备报告
const exportReport = () => {
  const report = {
    timestamp: new Date().toISOString(),
    devices: devices.value,
    statistics: Object.fromEntries(deviceStats.value),
    operations: operationHistory.value.slice(0, 50),
    summary: {
      totalDevices: devices.value.length,
      activeDevices: busyDevices.value.length,
      idleDevices: idleDevices.value.length,
      errorDevices: devices.value.filter(d => d.status === 'ERROR').length,
      totalOperations: operationHistory.value.length,
      successRate: operationHistory.value.length > 0
        ? (operationHistory.value.filter(op => op.result === 'success').length / operationHistory.value.length * 100).toFixed(2) + '%'
        : '0%'
    }
  };

  const blob = new Blob([JSON.stringify(report, null, 2)], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `device-report-${Date.now()}.json`;
  a.click();
  URL.revokeObjectURL(url);
};

onMounted(() => {
  refreshData();
  startAutoRefresh();
});

onUnmounted(() => {
  stopAutoRefresh();
});
</script>

<template>
  <div class="device-manager">
    <!-- 顶部控制栏 -->
    <div class="control-bar">
      <div class="status-overview">
        <div class="status-item idle">
          <span class="icon">🟢</span>
          <span>空闲: {{ statusStats.IDLE }}</span>
        </div>
        <div class="status-item busy">
          <span class="icon">🟡</span>
          <span>繁忙: {{ statusStats.BUSY }}</span>
        </div>
        <div class="status-item error">
          <span class="icon">🔴</span>
          <span>错误: {{ statusStats.ERROR }}</span>
        </div>
        <div class="total-devices">
          <span class="icon">📊</span>
          <span>总计: {{ devices.length }}</span>
        </div>
      </div>

      <div class="control-buttons">
        <button @click="refreshData" :disabled="isLoading" class="btn-primary">
          {{ isLoading ? '刷新中...' : '刷新' }}
        </button>
        <button @click="toggleAutoRefresh" :class="autoRefresh ? 'btn-success' : 'btn-secondary'">
          {{ autoRefresh ? '自动刷新: 开' : '自动刷新: 关' }}
        </button>
        <button @click="loadMockDevices" class="btn-info" title="加载演示数据">
          演示数据
        </button>
        <button @click="exportReport" class="btn-info">导出报告</button>
        <button @click="clearHistory" class="btn-warning">清除历史</button>

        <!-- 设备类型快速操作 -->
        <div class="device-type-actions">
          <select v-model="selectedDeviceType" class="device-type-select">
            <option value="">选择设备类型...</option>
            <option v-for="(deviceList, type) in devicesByType" :key="type" :value="type">
              {{ type }} ({{ deviceList.length }})
            </option>
          </select>
          <button
            @click="simulateDeviceQueue(selectedDeviceType)"
            :disabled="!selectedDeviceType"
            class="btn-secondary"
          >
            模拟队列
          </button>
        </div>
      </div>
    </div>

    <!-- 主要内容区域 -->
    <div class="main-content">
      <!-- 设备列表面板 -->
      <div class="devices-panel">
        <div class="panel-header">
          <h3>设备列表</h3>
          <span class="device-count">{{ devices.length }} 台设备</span>
        </div>

        <div v-if="isLoading" class="loading-state">
          <div class="spinner"></div>
          <p>正在加载设备...</p>
        </div>

        <div v-else-if="devices.length === 0" class="empty-state">
          <div class="empty-icon">📱</div>
          <h3>暂无设备</h3>
          <p>系统中还没有注册任何设备</p>
          <button @click="refreshData" class="btn-primary">重新加载</button>
        </div>

        <div v-else class="devices-grid">
          <div
          v-for="device in devices"
          :key="device.device_id"
            @click="selectDevice(device)"
            :class="['device-card', {
              'selected': selectedDevice?.device_id === device.device_id,
              'busy': device.status === 'BUSY',
              'error': device.status === 'ERROR'
            }]"
          >
            <div class="device-header">
              <span class="device-icon">{{ getDeviceIcon(device.type) }}</span>
              <div class="device-info">
                <div class="device-name">{{ device.name }}</div>
                <div class="device-type">{{ device.type }}</div>
              </div>
              <div class="device-status" :style="{ color: getStatusColor(device.status) }">
                {{ device.status }}
              </div>
            </div>

            <div class="device-details">
              <div class="detail-item">
                <span>ID: {{ device.device_id }}</span>
              </div>
              <div class="detail-item">
                <span>用户: {{ getProcessName(device.current_user) }}</span>
              </div>
              <div class="detail-item">
                <span>利用率: {{ getDeviceUtilization(device).toFixed(1) }}%</span>
              </div>
            </div>

            <!-- 设备利用率进度条 -->
            <div class="utilization-bar">
              <div
                class="utilization-fill"
                :style="{
                  width: `${getDeviceUtilization(device)}%`,
                  backgroundColor: getStatusColor(device.status)
                }"
              ></div>
            </div>
          </div>
        </div>
      </div>

      <!-- 设备详情和操作面板 -->
      <div class="details-panel">
        <div v-if="!selectedDevice" class="no-selection">
          <div class="placeholder-icon">🔧</div>
          <h3>选择设备</h3>
          <p>从左侧选择一个设备查看详细信息和执行操作</p>
        </div>

        <div v-else class="device-details-content">
          <!-- 设备详情 -->
          <div class="detail-section">
            <h3>{{ selectedDevice.name }}</h3>
            <div class="detail-grid">
              <div class="detail-item">
                <label>设备ID:</label>
                <span>{{ selectedDevice.device_id }}</span>
              </div>
              <div class="detail-item">
                <label>类型:</label>
                <span>{{ selectedDevice.type }}</span>
              </div>
              <div class="detail-item">
                <label>状态:</label>
                <span :style="{ color: getStatusColor(selectedDevice.status) }">
                  {{ selectedDevice.status }}
                </span>
              </div>
              <div class="detail-item">
                <label>当前用户:</label>
                <span>{{ getProcessName(selectedDevice.current_user) }}</span>
              </div>
            </div>
          </div>

          <!-- 设备操作 -->
          <div class="operations-section">
            <h4>设备操作</h4>

            <!-- 进程选择器 -->
            <div class="process-selector" v-if="processes.length > 0">
              <label>选择进程:</label>
              <select v-model="selectedProcessId" class="process-select">
                <option value="">选择进程...</option>
                <option v-for="process in processes" :key="process.pid" :value="process.pid">
                  PID {{ process.pid }} ({{ process.state }})
                </option>
              </select>
            </div>

            <div class="operation-buttons">
              <button
                @click="requestDeviceWithProcess(selectedDevice.device_id)"
                :disabled="selectedDevice.status === 'BUSY' || !selectedProcessId"
                class="btn-primary"
                :title="!selectedProcessId ? '请先选择进程' : ''"
              >
                申请设备
              </button>
              <button
                @click="releaseDeviceWithProcess(selectedDevice.device_id)"
                :disabled="selectedDevice.status !== 'BUSY' || !selectedProcessId"
                class="btn-secondary"
                :title="!selectedProcessId ? '请先选择进程' : ''"
              >
                释放设备
              </button>
              <button
                @click="executeDeviceOperation(selectedDevice.device_id, 'TEST')"
                :disabled="selectedDevice.status === 'ERROR'"
                class="btn-info"
              >
                测试设备
              </button>
              <button
                @click="confirmDeleteDevice(selectedDevice.device_id)"
                :disabled="selectedDevice.status === 'ERROR'"
                :class="[
                  selectedDevice.status === 'BUSY' ? 'btn-danger-force' : 'btn-danger'
                ]"
                :title="selectedDevice.status === 'BUSY' ? '将先释放设备再删除' : '删除设备'"
              >
                <span v-if="selectedDevice.status === 'BUSY'">⚡ 强制删除</span>
                <span v-else>🗑️ 删除设备</span>
              </button>

              <!-- 高级操作下拉菜单 -->
              <div class="advanced-operations">
                <label for="operation-select">高级操作:</label>
                <select id="operation-select" v-model="selectedOperation" class="operation-select">
                  <option value="">选择操作...</option>
                  <option value="RESET">重置设备</option>
                  <option value="CALIBRATE">校准设备</option>
                  <option value="DIAGNOSTIC">诊断测试</option>
                  <option value="MAINTENANCE">维护模式</option>
                  <option value="SUSPEND">暂停设备</option>
                  <option value="RESUME">恢复设备</option>
                </select>
                <button
                  @click="executeAdvancedOperation"
                  :disabled="!selectedOperation || selectedDevice.status === 'ERROR'"
                  class="btn-secondary"
                >
                  执行
                </button>
              </div>
            </div>
          </div>

          <!-- 设备统计 -->
          <div class="stats-section" v-if="deviceStats.get(selectedDevice.device_id)">
            <h4>统计信息</h4>
            <div class="stats-grid">
              <div class="stat-item">
                <div class="stat-value">{{ deviceStats.get(selectedDevice.device_id)?.totalOperations || 0 }}</div>
                <div class="stat-label">总操作数</div>
              </div>
              <div class="stat-item">
                <div class="stat-value">{{ (deviceStats.get(selectedDevice.device_id)?.successRate || 0).toFixed(1) }}%</div>
                <div class="stat-label">成功率</div>
              </div>
              <div class="stat-item">
                <div class="stat-value">{{ getDeviceUtilization(selectedDevice).toFixed(1) }}%</div>
                <div class="stat-label">利用率</div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- 操作历史面板 -->
      <div class="history-panel">
        <div class="panel-header">
          <h3>操作历史</h3>
          <span class="history-count">{{ operationHistory.length }} 条记录</span>
        </div>

        <div class="history-list">
          <div
            v-for="operation in operationHistory.slice(0, 20)"
            :key="operation.id"
            :class="['history-item', operation.result]"
          >
            <div class="history-time">{{ operation.timestamp }}</div>
            <div class="history-info">
              <span class="operation-type">{{ operation.operation }}</span>
              <span class="device-id">设备 {{ operation.deviceId }}</span>
              <span v-if="operation.processId" class="process-id">进程 {{ operation.processId }}</span>
            </div>
            <div class="history-message">{{ operation.message }}</div>
            <div class="history-status" :class="operation.result">
              {{ operation.result === 'success' ? '✓' : '✗' }}
            </div>
    </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.device-manager {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #f8f9fa;
}

.control-bar {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 20px;
  background: white;
  border-bottom: 1px solid #e9ecef;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
  flex-wrap: wrap;
  gap: 15px;
}

@media (max-width: 768px) {
  .control-bar {
    flex-direction: column;
    align-items: stretch;
  }

  .status-overview {
    justify-content: center;
    margin-bottom: 10px;
  }

  .control-buttons {
    flex-direction: column;
    gap: 10px;
  }

  .device-type-actions {
    margin-left: 0;
    padding-left: 0;
    border-left: none;
    border-top: 1px solid #e9ecef;
    padding-top: 10px;
  }
}

.status-overview {
  display: flex;
  gap: 20px;
}

.status-item, .total-devices {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-radius: 6px;
  background: #f8f9fa;
  font-weight: 500;
}

.control-buttons {
  display: flex;
  gap: 10px;
  align-items: center;
}

.device-type-actions {
  display: flex;
  gap: 8px;
  align-items: center;
  margin-left: 20px;
  padding-left: 20px;
  border-left: 1px solid #e9ecef;
}

.device-type-select, .operation-select {
  padding: 6px 10px;
  border: 1px solid #ced4da;
  border-radius: 4px;
  background: white;
  font-size: 13px;
}

.btn-primary, .btn-secondary, .btn-success, .btn-info, .btn-warning, .btn-danger {
  padding: 8px 16px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-weight: 500;
  transition: all 0.2s;
  font-size: 14px;
}

.btn-primary { background: #007bff; color: white; }
.btn-secondary { background: #6c757d; color: white; }
.btn-success { background: #28a745; color: white; }
.btn-info { background: #17a2b8; color: white; }
.btn-warning { background: #ffc107; color: #212529; }
.btn-danger { background: #dc3545; color: white; }

.btn-primary:hover { background: #0056b3; }
.btn-secondary:hover { background: #545b62; }
.btn-success:hover { background: #218838; }
.btn-info:hover { background: #138496; }
.btn-warning:hover { background: #e0a800; }
.btn-danger:hover {
  background: #c82333;
  transform: scale(1.02);
  box-shadow: 0 2px 8px rgba(220, 53, 69, 0.3);
}

.btn-danger-force {
  background: linear-gradient(45deg, #dc3545, #fd7e14);
  color: white;
  border: none;
  border-radius: 4px;
  padding: 8px 16px;
  cursor: pointer;
  font-weight: 500;
  transition: all 0.2s;
  font-size: 14px;
  position: relative;
  overflow: hidden;
}

.btn-danger-force::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
  transition: left 0.5s;
}

.btn-danger-force:hover::before {
  left: 100%;
}

.btn-danger-force:hover {
  background: linear-gradient(45deg, #c82333, #e68900);
  transform: scale(1.02);
  box-shadow: 0 4px 12px rgba(220, 53, 69, 0.4);
}

.process-selector {
  margin-bottom: 15px;
  padding: 12px;
  background: #f8f9fa;
  border-radius: 6px;
  border: 1px solid #e9ecef;
}

.process-selector label {
  display: block;
  margin-bottom: 8px;
  font-weight: 500;
  color: #495057;
  font-size: 13px;
}

.process-select {
  width: 100%;
  padding: 8px 12px;
  border: 1px solid #ced4da;
  border-radius: 4px;
  background: white;
  font-size: 14px;
  transition: border-color 0.2s ease;
}

.process-select:focus {
  outline: none;
  border-color: #007bff;
  box-shadow: 0 0 0 2px rgba(0, 123, 255, 0.25);
}

.main-content {
  flex: 1;
  display: grid;
  grid-template-columns: minmax(400px, 2fr) minmax(300px, 1fr) minmax(250px, 1fr);
  gap: 20px;
  padding: 20px;
  overflow: hidden;
}

@media (max-width: 1200px) {
  .main-content {
    grid-template-columns: 1fr;
    grid-template-rows: auto auto auto;
  }

  .devices-panel {
    min-height: 400px;
  }

  .details-panel {
    min-height: 300px;
  }

  .history-panel {
    min-height: 250px;
  }
}

@media (max-width: 600px) {
  .devices-grid {
    grid-template-columns: 1fr;
    padding: 10px;
  }

  .device-card {
    padding: 12px;
  }

  .panel-header {
    padding: 10px 15px;
  }

  .main-content {
    padding: 10px;
    gap: 10px;
  }
}

.devices-panel, .details-panel, .history-panel {
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0,0,0,0.1);
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 20px;
  border-bottom: 1px solid #e9ecef;
  background: #f8f9fa;
}

.panel-header h3 {
  margin: 0;
  color: #333;
}

.device-count, .history-count {
  color: #6c757d;
  font-size: 12px;
}

.devices-grid {
  flex: 1;
  padding: 20px;
  overflow-y: auto;
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 15px;
  min-height: 0; /* 修复flex容器中的grid高度问题 */
}

.device-card {
  border: 2px solid #e9ecef;
  border-radius: 8px;
  padding: 15px;
  cursor: pointer;
  transition: all 0.2s;
  background: white;
}

.device-card:hover {
  border-color: #007bff;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0,0,0,0.1);
}

.device-card.selected {
  border-color: #007bff;
  background: #f8f9ff;
}

.device-card.busy {
  border-left: 4px solid #ffc107;
}

.device-card.error {
  border-left: 4px solid #dc3545;
}

.device-header {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 12px;
}

.device-icon {
  font-size: 24px;
}

.device-info {
  flex: 1;
}

.device-name {
  font-weight: 600;
  color: #333;
}

.device-type {
  font-size: 12px;
  color: #6c757d;
}

.device-status {
  font-weight: 600;
  font-size: 12px;
}

.device-details {
  margin-bottom: 10px;
}

.detail-item {
  font-size: 12px;
  color: #6c757d;
  margin-bottom: 4px;
}

.utilization-bar {
  height: 4px;
  background: #e9ecef;
  border-radius: 2px;
  overflow: hidden;
}

.utilization-fill {
  height: 100%;
  transition: width 0.3s;
}

.details-panel {
  padding: 20px;
  overflow-y: auto;
}

.no-selection {
  text-align: center;
  padding: 40px 20px;
  color: #6c757d;
}

.placeholder-icon {
  font-size: 48px;
  margin-bottom: 20px;
}

.device-details-content {
  height: 100%;
  overflow-y: auto;
}

.detail-section, .operations-section, .stats-section {
  margin-bottom: 25px;
  padding-bottom: 20px;
  border-bottom: 1px solid #e9ecef;
}

.detail-section:last-child,
.operations-section:last-child,
.stats-section:last-child {
  border-bottom: none;
  margin-bottom: 0;
}

.detail-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
  margin-top: 15px;
}

.detail-grid .detail-item {
  display: flex;
  justify-content: space-between;
}

.detail-grid label {
  font-weight: 500;
  color: #495057;
}

.operation-buttons {
  display: flex;
  flex-direction: column;
  gap: 10px;
  margin-top: 15px;
}

.advanced-operations {
  margin-top: 15px;
  padding-top: 15px;
  border-top: 1px solid #e9ecef;
}

.advanced-operations label {
  display: block;
  margin-bottom: 8px;
  font-weight: 500;
  color: #495057;
  font-size: 13px;
}

.advanced-operations .operation-select {
  width: 100%;
  margin-bottom: 10px;
}

.advanced-operations button {
  width: 100%;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 15px;
  margin-top: 15px;
}

.stat-item {
  text-align: center;
  padding: 15px;
  background: #f8f9fa;
  border-radius: 6px;
}

.stat-value {
  font-size: 20px;
  font-weight: 600;
  color: #007bff;
}

.stat-label {
  font-size: 12px;
  color: #6c757d;
  margin-top: 5px;
}

.history-panel {
  padding: 0;
}

.history-list {
  flex: 1;
  overflow-y: auto;
  padding: 10px;
}

.history-item {
  display: grid;
  grid-template-columns: 100px 1fr 20px;
  gap: 10px;
  padding: 10px;
  border-radius: 4px;
  margin-bottom: 8px;
  font-size: 12px;
  border-left: 3px solid #e9ecef;
}

.history-item.success {
  border-left-color: #28a745;
  background: #f8fff9;
}

.history-item.error {
  border-left-color: #dc3545;
  background: #fff8f8;
}

.history-time {
  color: #6c757d;
  font-weight: 500;
}

.history-info {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.operation-type {
  font-weight: 600;
  color: #333;
}

.device-id, .process-id {
  color: #6c757d;
}

.history-message {
  grid-column: 1 / -1;
  color: #495057;
  margin-top: 5px;
}

.history-status {
  text-align: center;
  font-weight: 600;
}

.history-status.success { color: #28a745; }
.history-status.error { color: #dc3545; }

/* 加载状态 */
.loading-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 20px;
  color: #6c757d;
}

.spinner {
  width: 40px;
  height: 40px;
  border: 4px solid #f3f3f3;
  border-top: 4px solid #007bff;
  border-radius: 50%;
  animation: spin 1s linear infinite;
  margin-bottom: 20px;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 20px;
  text-align: center;
  color: #6c757d;
}

.empty-icon {
  font-size: 64px;
  margin-bottom: 20px;
}

.empty-state h3 {
  margin: 0 0 10px 0;
  color: #495057;
}

.empty-state p {
  margin: 0 0 20px 0;
  font-size: 14px;
}
</style>
