<template>
  <div class="properties-dialog-overlay" v-if="visible" @click="handleOverlayClick">
    <div class="properties-dialog" @click.stop>
      <!-- 标题栏 -->
      <div class="dialog-header">
        <div class="dialog-icon">
          <span v-if="fileType === 'directory'">📁</span>
          <span v-else-if="fileType === 'executable'">⚙️</span>
          <span v-else>📄</span>
        </div>
        <div class="dialog-title">{{ fileName }} - 属性</div>
        <button class="dialog-close" @click="close">
          <span>✕</span>
        </button>
      </div>

      <!-- 选项卡 -->
      <div class="dialog-tabs">
        <button
          v-for="tab in tabs"
          :key="tab.key"
          class="tab-button"
          :class="{ active: activeTab === tab.key }"
          @click="activeTab = tab.key"
        >
          {{ tab.label }}
        </button>
      </div>

      <!-- 内容区域 -->
      <div class="dialog-content">
        <!-- 常规选项卡 -->
        <div v-if="activeTab === 'general'" class="tab-content">
          <div class="property-section">
            <div class="file-icon-large">
              <span v-if="fileType === 'directory'">📁</span>
              <span v-else-if="fileType === 'executable'">⚙️</span>
              <span v-else>📄</span>
            </div>
            <div class="file-info">
              <div class="file-name">{{ fileName }}</div>
              <div class="file-type">{{ fileTypeText }}</div>
            </div>
          </div>

          <div class="property-list">
            <div v-for="prop in generalProperties" :key="prop.key" class="property-item">
              <span class="property-label">{{ prop.label }}:</span>
              <span class="property-value">{{ prop.value }}</span>
            </div>
          </div>
        </div>

        <!-- 详细信息选项卡 -->
        <div v-if="activeTab === 'details'" class="tab-content">
          <div class="property-list">
            <div v-for="prop in detailProperties" :key="prop.key" class="property-item">
              <span class="property-label">{{ prop.label }}:</span>
              <span class="property-value">{{ prop.value }}</span>
            </div>
          </div>
        </div>

        <!-- 权限选项卡 -->
        <div v-if="activeTab === 'permissions'" class="tab-content">
          <div class="permissions-section">
            <h4>文件权限</h4>
            <div class="permission-item">
              <input type="checkbox" id="readable" v-model="permissions.readable" disabled>
              <label for="readable">可读</label>
            </div>
            <div class="permission-item">
              <input type="checkbox" id="writable" v-model="permissions.writable" disabled>
              <label for="writable">可写</label>
            </div>
            <div class="permission-item">
              <input type="checkbox" id="executable" v-model="permissions.executable" disabled>
              <label for="executable">可执行</label>
            </div>
          </div>
        </div>
      </div>

      <!-- 按钮区域 -->
      <div class="dialog-buttons">
        <button
          class="dialog-button primary"
          @click="close"
        >
          确定
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, reactive } from 'vue';

interface FileProperties {
  name: string;
  type: 'file' | 'directory';
  size?: number;
  simulated_size?: number;
  path: string;
  permissions?: string;
  last_modified?: string;
  created?: string;
  [key: string]: any;
}

const props = defineProps<{
  visible: boolean;
  file: FileProperties;
}>();

const emit = defineEmits<{
  close: [];
}>();

const activeTab = ref('general');

const tabs = [
  { key: 'general', label: '常规' },
  { key: 'details', label: '详细信息' },
  { key: 'permissions', label: '权限' }
];

const fileName = computed(() => props.file?.name || '');
const fileType = computed(() => {
  if (props.file?.type === 'directory') return 'directory';
  if (props.file?.name?.endsWith('.pubt')) return 'executable';
  return 'file';
});

const fileTypeText = computed(() => {
  switch (fileType.value) {
    case 'directory':
      return '文件夹';
    case 'executable':
      return 'PUBT 可执行文件';
    default:
      const ext = props.file?.name?.split('.').pop()?.toUpperCase();
      return ext ? `${ext} 文件` : '文件';
  }
});

const formatBytes = (bytes: number) => {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
};

const formatDate = (dateString?: string) => {
  if (!dateString) return '未知';
  try {
    return new Date(dateString).toLocaleString('zh-CN');
  } catch {
    return dateString;
  }
};

const generalProperties = computed(() => [
  {
    key: 'type',
    label: '类型',
    value: fileTypeText.value
  },
  {
    key: 'location',
    label: '位置',
    value: props.file?.path?.replace(`/${props.file?.name}`, '') || '/'
  },
  {
    key: 'size',
    label: '大小',
    value: props.file?.size ? formatBytes(props.file.size) : '未知'
  },
  {
    key: 'simulated_size',
    label: '模拟大小',
    value: props.file?.simulated_size ? formatBytes(props.file.simulated_size) : '与实际大小相同'
  },
  {
    key: 'modified',
    label: '修改时间',
    value: formatDate(props.file?.last_modified)
  }
]);

const detailProperties = computed(() => [
  {
    key: 'full_path',
    label: '完整路径',
    value: props.file?.path || ''
  },
  {
    key: 'name',
    label: '文件名',
    value: props.file?.name || ''
  },
  {
    key: 'extension',
    label: '扩展名',
    value: props.file?.name?.includes('.') ? props.file.name.split('.').pop() : '无'
  },
  {
    key: 'permissions_raw',
    label: '权限字符串',
    value: props.file?.permissions || '未知'
  },
  {
    key: 'created',
    label: '创建时间',
    value: formatDate(props.file?.created)
  }
]);

const permissions = reactive({
  readable: props.file?.permissions?.includes('r') || false,
  writable: props.file?.permissions?.includes('w') || false,
  executable: props.file?.permissions?.includes('x') || false
});

const handleOverlayClick = () => {
  close();
};

const close = () => {
  emit('close');
};
</script>

<style scoped>
.properties-dialog-overlay {
  position: fixed;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(4px);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 10000;
  animation: fadeIn 0.2s ease-out;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

.properties-dialog {
  background: #ffffff;
  border-radius: 8px;
  box-shadow:
    0 8px 32px rgba(0, 0, 0, 0.12),
    0 2px 8px rgba(0, 0, 0, 0.08);
  width: 500px;
  max-height: 80vh;
  overflow: hidden;
  animation: slideIn 0.3s cubic-bezier(0.25, 0.46, 0.45, 0.94);
}

@keyframes slideIn {
  from {
    opacity: 0;
    transform: translateY(-20px) scale(0.95);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

.dialog-header {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 20px 24px 16px;
  border-bottom: 1px solid #e1e1e1;
  background: linear-gradient(135deg, #f0f8ff 0%, #ffffff 100%);
}

.dialog-icon {
  font-size: 24px;
  line-height: 1;
}

.dialog-title {
  flex: 1;
  font-size: 16px;
  font-weight: 600;
  color: #323130;
  margin: 0;
}

.dialog-close {
  background: none;
  border: none;
  padding: 6px;
  border-radius: 4px;
  cursor: pointer;
  color: #605e5c;
  font-size: 16px;
  line-height: 1;
  transition: all 0.2s ease;
}

.dialog-close:hover {
  background: rgba(0, 0, 0, 0.05);
  color: #323130;
}

.dialog-tabs {
  display: flex;
  background: #f8f9fa;
  border-bottom: 1px solid #e1e1e1;
}

.tab-button {
  background: none;
  border: none;
  padding: 12px 20px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 500;
  color: #605e5c;
  transition: all 0.2s ease;
  border-bottom: 2px solid transparent;
}

.tab-button:hover {
  background: rgba(0, 0, 0, 0.05);
  color: #323130;
}

.tab-button.active {
  color: #0078d4;
  background: #ffffff;
  border-bottom-color: #0078d4;
}

.dialog-content {
  max-height: 400px;
  overflow-y: auto;
}

.tab-content {
  padding: 20px 24px;
}

.property-section {
  display: flex;
  align-items: center;
  gap: 16px;
  margin-bottom: 24px;
  padding: 16px;
  background: #f8f9fa;
  border-radius: 6px;
}

.file-icon-large {
  font-size: 48px;
  line-height: 1;
}

.file-info {
  flex: 1;
}

.file-name {
  font-size: 16px;
  font-weight: 600;
  color: #323130;
  margin-bottom: 4px;
  word-break: break-all;
}

.file-type {
  font-size: 13px;
  color: #605e5c;
}

.property-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.property-item {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  padding: 8px 0;
  border-bottom: 1px solid #f3f2f1;
}

.property-item:last-child {
  border-bottom: none;
}

.property-label {
  font-size: 13px;
  font-weight: 500;
  color: #605e5c;
  min-width: 100px;
  text-align: right;
}

.property-value {
  font-size: 13px;
  color: #323130;
  flex: 1;
  word-break: break-all;
  font-family: 'Segoe UI', sans-serif;
}

.permissions-section {
  padding: 16px;
  background: #f8f9fa;
  border-radius: 6px;
}

.permissions-section h4 {
  margin: 0 0 16px 0;
  font-size: 14px;
  font-weight: 600;
  color: #323130;
}

.permission-item {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-bottom: 8px;
}

.permission-item input[type="checkbox"] {
  width: 16px;
  height: 16px;
}

.permission-item label {
  font-size: 13px;
  color: #323130;
  cursor: default;
}

.dialog-buttons {
  display: flex;
  gap: 12px;
  justify-content: flex-end;
  padding: 16px 24px 20px;
  background: #f8f9fa;
  border-top: 1px solid #e1e1e1;
}

.dialog-button {
  padding: 8px 20px;
  border-radius: 4px;
  font-size: 14px;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s ease;
  border: 1px solid transparent;
  min-width: 80px;
}

.dialog-button.primary {
  background: #0078d4;
  color: white;
  border-color: #0078d4;
}

.dialog-button.primary:hover {
  background: #106ebe;
  border-color: #106ebe;
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(0, 120, 212, 0.3);
}

.dialog-button.primary:active {
  background: #005a9e;
  transform: translateY(0);
}
</style>
