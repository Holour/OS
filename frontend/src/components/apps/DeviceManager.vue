<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { deviceAPI } from '@/services/api';

interface Device {
  device_id: string;
  type: string;
  status: string;
  current_user: number | null;
  name?: string;
}

const devices = ref<Device[]>([]);
const isLoading = ref(true);
const selectedDevice = ref<Device | null>(null);

const fetchDevices = async () => {
  isLoading.value = true;
  try {
    const response = await deviceAPI.getDevices();
    devices.value = response.data.data;
  } catch (error) {
    console.error("Failed to fetch devices:", error);
  } finally {
    isLoading.value = false;
  }
};

const handleSelectDevice = (device: Device) => {
  selectedDevice.value = device;
};

const handleDeviceRequest = async () => {
  if (!selectedDevice.value) return;
  try {
    const response = await deviceAPI.requestDevice(selectedDevice.value.type);
    alert(`设备请求已发送。\n响应: ${JSON.stringify(response.data.message || response.data)}`);
    fetchDevices(); // 重新获取设备列表
  } catch (error: any) {
    alert(`设备请求失败: ${error.response?.data?.message || error.message}`);
  }
};

const handleDeviceRelease = async () => {
  if (!selectedDevice.value) return;
  const deviceId = parseInt(selectedDevice.value.device_id);
  try {
    const response = await deviceAPI.releaseDevice(deviceId);
    alert(`设备释放已发送。\n响应: ${JSON.stringify(response.data.message || response.data)}`);
    fetchDevices(); // 重新获取设备列表
  } catch (error: any) {
    alert(`设备释放失败: ${error.response?.data?.message || error.message}`);
  }
};

const handleDeviceOperation = async () => {
  if (!selectedDevice.value) return;
  const deviceId = parseInt(selectedDevice.value.device_id);
  try {
    const response = await deviceAPI.deviceOperation(deviceId);
    alert(`设备操作已执行。\n响应: ${JSON.stringify(response.data.message || response.data)}`);
    fetchDevices(); // 重新获取设备列表
  } catch (error: any) {
    alert(`设备操作失败: ${error.response?.data?.message || error.message}`);
  }
};

onMounted(fetchDevices);
</script>

<template>
  <div class="device-manager">
    <div class="device-list-pane">
      <h4>设备列表</h4>
      <div v-if="isLoading">加载中...</div>
      <ul v-else>
        <li
          v-for="device in devices"
          :key="device.device_id"
          @click="handleSelectDevice(device)"
          :class="{ selected: selectedDevice?.device_id === device.device_id }"
        >
          <span>{{ device.name || device.device_id }} ({{ device.type }})</span>
          <span :class="`status-${device.status.toLowerCase()}`">{{ device.status }}</span>
        </li>
      </ul>
    </div>
    <div class="device-details-pane">
      <h4>设备详情</h4>
      <div v-if="!selectedDevice" class="placeholder">请从左侧选择一个设备</div>
      <div v-else class="details-content">
        <p><strong>ID:</strong> {{ selectedDevice.device_id }}</p>
        <p><strong>名称:</strong> {{ selectedDevice.name || selectedDevice.device_id }}</p>
        <p><strong>类型:</strong> {{ selectedDevice.type }}</p>
        <p><strong>状态:</strong> {{ selectedDevice.status }}</p>
        <p><strong>当前用户:</strong> {{ selectedDevice.current_user ?? '无' }}</p>
        <div class="device-actions">
          <h5>设备操作</h5>
          <button @click="handleDeviceRequest">请求设备</button>
          <button @click="handleDeviceRelease">释放设备</button>
          <button @click="handleDeviceOperation">执行操作</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.device-manager {
  display: flex;
  height: 100%;
  font-size: 14px;
}
.device-list-pane {
  width: 250px;
  border-right: 1px solid #ccc;
  padding: 10px;
  overflow-y: auto;
}
.device-list-pane ul {
  list-style: none;
  padding: 0;
  margin: 0;
}
.device-list-pane li {
  display: flex;
  justify-content: space-between;
  padding: 8px;
  cursor: pointer;
  border-radius: 4px;
  margin-bottom: 5px;
}
.device-list-pane li:hover {
  background-color: #f0f0f0;
}
.device-list-pane li.selected {
  background-color: #0078d4;
  color: white;
}
.status-available { color: green; }
.status-busy { color: red; }

.device-details-pane {
  flex-grow: 1;
  padding: 20px;
}
.placeholder {
  color: #888;
}
.details-content p {
  margin: 5px 0;
}
.device-actions {
  margin-top: 20px;
  border-top: 1px solid #eee;
  padding-top: 15px;
}
.device-actions button {
  margin-right: 10px;
  padding: 8px 12px;
  cursor: pointer;
}
</style>
