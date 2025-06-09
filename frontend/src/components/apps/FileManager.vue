<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import { filesystemAPI, processAPI } from '@/services/api';

// 接收props参数
const props = defineProps<{
  initialPath?: string;
}>();

interface FileItem {
  name: string;
  type: 'file' | 'directory';
  size?: number;
  simulated_size?: number;
  permissions?: number;
  last_modified?: string;
}

interface FileAddress {
  path: string;
  addresses: {
    contiguous?: number;
    linked?: number;
    indexed?: number;
  };
}

const currentPath = ref('/');
const files = ref<FileItem[]>([]);
const isLoading = ref(false);
const error = ref<string | null>(null);
const newFileName = ref('');
const newDirName = ref('');
const fileContent = ref('');
const selectedFile = ref<string | null>(null);
const showFileContextMenu = ref(false);
const fileContextMenuPosition = ref({ x: 0, y: 0 });
const contextMenuFile = ref<FileItem | null>(null);

// 新增：模拟大小支持
const showCreateFileDialog = ref(false);
const newFileSimulatedSize = ref(0);
const fileSizeUnit = ref('B');

// 新增：文件地址查看
const showFileAddressDialog = ref(false);
const fileAddressInfo = ref<FileAddress | null>(null);

const loadDirectory = async (path: string = currentPath.value) => {
  isLoading.value = true;
  error.value = null;
  try {
    const response = await filesystemAPI.listDirectory(path);
    if (response.data.status === 'success') {
      const data = response.data.data;
      // 根据新的API结构更新文件列表
      const allFiles = data.map((item: any) => ({
        name: item.name,
        type: item.type,
        size: item.size,
        simulated_size: item.simulated_size,
        permissions: item.permissions,
        last_modified: item.last_modified
      }));
      files.value = allFiles;
      currentPath.value = path;
    } else {
      throw new Error(response.data.message || 'Failed to load directory');
    }
  } catch (err: any) {
    console.error('Directory load error:', err);
    error.value = `加载目录失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  } finally {
    isLoading.value = false;
  }
};

const navigateToDirectory = (dirName: string) => {
  const newPath = currentPath.value === '/' ? `/${dirName}` : `${currentPath.value}/${dirName}`;
  loadDirectory(newPath);
};

const goBack = () => {
  if (currentPath.value === '/') return;
  const parentPath = currentPath.value.substring(0, currentPath.value.lastIndexOf('/')) || '/';
  loadDirectory(parentPath);
};

const openCreateFileDialog = () => {
  showCreateFileDialog.value = true;
  newFileName.value = '';
  newFileSimulatedSize.value = 0;
  fileSizeUnit.value = 'B';
};

const createFile = async () => {
  if (!newFileName.value.trim()) return;
  try {
    const filePath = currentPath.value === '/' ? newFileName.value : `${currentPath.value}/${newFileName.value}`;

    // 计算实际字节数
    let sizeInBytes = newFileSimulatedSize.value;
    switch (fileSizeUnit.value) {
      case 'KB': sizeInBytes *= 1024; break;
      case 'MB': sizeInBytes *= 1024 * 1024; break;
      case 'GB': sizeInBytes *= 1024 * 1024 * 1024; break;
    }

    await filesystemAPI.createFile(filePath, sizeInBytes);
    showCreateFileDialog.value = false;
    loadDirectory();
  } catch (err: any) {
    error.value = `创建文件失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

const createDirectory = async () => {
  if (!newDirName.value.trim()) return;
  try {
    const dirPath = currentPath.value === '/' ? newDirName.value : `${currentPath.value}/${newDirName.value}`;
    await filesystemAPI.createDirectory(dirPath);
    newDirName.value = '';
    loadDirectory();
  } catch (err: any) {
    error.value = `创建目录失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

const deleteItem = async (itemName: string, recursive: boolean = false) => {
  const action = recursive ? '递归删除' : '删除';
  if (!confirm(`确定要${action} ${itemName} 吗？`)) return;

  try {
    const itemPath = currentPath.value === '/' ? itemName : `${currentPath.value}/${itemName}`;
    await filesystemAPI.delete(itemPath, recursive);
    loadDirectory();
  } catch (err: any) {
    error.value = `${action}失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

const readFile = async (fileName: string) => {
  try {
    const filePath = currentPath.value === '/' ? fileName : `${currentPath.value}/${fileName}`;
    const response = await filesystemAPI.readFile(filePath);
    if (response.data.status === 'success') {
      selectedFile.value = fileName;
      // 由于新API不再返回文件内容，这里只是选中文件
      fileContent.value = `文件: ${fileName}\n模拟大小: ${response.data.data.simulated_size || 0} 字节\n\n注意：此文件系统主要用于演示存储分配策略，不存储实际文件内容。`;
    }
  } catch (err: any) {
    error.value = `读取文件失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

const showFileAddress = async (fileName: string) => {
  try {
    const filePath = currentPath.value === '/' ? fileName : `${currentPath.value}/${fileName}`;
    const response = await filesystemAPI.getFileAddress(filePath);
    if (response.data.status === 'success') {
      fileAddressInfo.value = response.data.data;
      showFileAddressDialog.value = true;
    }
  } catch (err: any) {
    error.value = `获取文件地址失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

// 处理.pubt文件执行
const handlePubtFile = async (fileName: string) => {
  try {
    const filePath = currentPath.value === '/' ? fileName : `${currentPath.value}/${fileName}`;

    // 读取.pubt文件内容获取内存大小
    const response = await filesystemAPI.readFile(filePath);
    if (response.data.status === 'success') {
      // 使用文件的模拟大小作为内存大小
      const memorySize = response.data.data.simulated_size || 1024; // 默认1KB

      // 创建进程，使用文件名（去掉.pubt扩展名）作为进程名
      const processName = fileName.replace(/\.pubt$/, '') || 'unnamed';

      const processResult = await processAPI.createProcess(processName, memorySize);
      console.log(`程序 "${processName}" 已启动，进程ID: ${processResult.data.pid}，分配内存: ${formatBytes(memorySize)}`);

      alert(`程序 "${processName}" 已启动！分配内存: ${formatBytes(memorySize)}`);
    } else {
      throw new Error('无法读取.pubt文件信息');
    }
  } catch (err: any) {
    error.value = `执行程序失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

// 格式化字节数为可读格式
const formatBytes = (bytes: number): string => {
  if (bytes >= 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024 * 1024)).toFixed(1)} GB`;
  if (bytes >= 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
  if (bytes >= 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${bytes} B`;
};

// 处理文件双击事件
const handleFileDoubleClick = async (fileName: string) => {
  const ext = fileName.split('.').pop()?.toLowerCase();
  if (ext === 'pubt') {
    // 执行.pubt文件
    await handlePubtFile(fileName);
  } else {
    // 普通文件，进入查看模式
    await readFile(fileName);
  }
};

// 文件右键菜单处理
const handleFileContextMenu = (event: MouseEvent, file: FileItem) => {
  event.preventDefault();
  event.stopPropagation();
  fileContextMenuPosition.value = { x: event.clientX, y: event.clientY };
  contextMenuFile.value = file;
  showFileContextMenu.value = true;
};

const hideFileContextMenu = () => {
  showFileContextMenu.value = false;
};

// 文件操作函数
const openFileFromMenu = () => {
  if (contextMenuFile.value) {
    if (contextMenuFile.value.type === 'directory') {
      navigateToDirectory(contextMenuFile.value.name);
    } else {
      handleFileDoubleClick(contextMenuFile.value.name);
    }
  }
  hideFileContextMenu();
};

const deleteFileFromMenu = () => {
  if (contextMenuFile.value) {
    deleteItem(contextMenuFile.value.name);
  }
  hideFileContextMenu();
};

const recursiveDeleteFromMenu = () => {
  if (contextMenuFile.value) {
    deleteItem(contextMenuFile.value.name, true);
  }
  hideFileContextMenu();
};

const showFileAddressFromMenu = () => {
  if (contextMenuFile.value && contextMenuFile.value.type === 'file') {
    showFileAddress(contextMenuFile.value.name);
  }
  hideFileContextMenu();
};

const showFileProperties = () => {
  if (!contextMenuFile.value) return;

  const file = contextMenuFile.value;
  const props = [
    `名称: ${file.name}`,
    `类型: ${file.type === 'directory' ? '文件夹' : '文件'}`,
    `路径: ${currentPath.value === '/' ? '/' + file.name : currentPath.value + '/' + file.name}`,
    file.simulated_size ? `模拟大小: ${formatBytes(file.simulated_size)}` : '',
    `权限: ${file.permissions ? file.permissions.toString(8) : '未知'}`,
    file.last_modified ? `修改时间: ${new Date(file.last_modified).toLocaleString()}` : ''
  ].filter(Boolean).join('\n');

  alert(`文件属性:\n\n${props}`);
  hideFileContextMenu();
};

const getFileIcon = (file: FileItem) => {
  if (file.type === 'directory') {
    return '📁';
  }

  const ext = file.name.split('.').pop()?.toLowerCase();
  switch (ext) {
    case 'txt': return '📄';
    case 'log': return '📋';
    case 'conf':
    case 'config': return '⚙️';
    case 'exe': return '⚡';
    case 'pubt': return '🚀'; // .pubt文件显示火箭图标
    default: return '📄';
  }
};

onMounted(() => {
  // 如果有初始路径参数，使用它，否则使用根目录
  const startPath = props.initialPath || '/';
  loadDirectory(startPath);

  // 点击其他地方隐藏右键菜单
  document.addEventListener('click', hideFileContextMenu);
});

onUnmounted(() => {
  document.removeEventListener('click', hideFileContextMenu);
});
</script>

<template>
  <div class="file-manager">
    <div class="toolbar">
      <button @click="goBack" :disabled="currentPath === '/'">⬅️ 返回</button>
      <span class="path">📁 {{ currentPath }}</span>
      <button @click="loadDirectory()">🔄 刷新</button>
      <button @click="openCreateFileDialog" class="create-btn">📄 新建文件</button>
    </div>

    <div v-if="error" class="error">❌ {{ error }}</div>

    <div class="main-content">
      <div class="file-list">
        <div v-if="isLoading" class="loading">⏳ 加载中...</div>
        <div v-else>
          <div
            v-for="file in files"
            :key="file.name"
            class="file-item"
            :class="{ directory: file.type === 'directory' }"
            @dblclick="file.type === 'directory' ? navigateToDirectory(file.name) : handleFileDoubleClick(file.name)"
            @contextmenu="handleFileContextMenu($event, file)"
          >
            <span class="icon">{{ getFileIcon(file) }}</span>
            <div class="file-info">
              <span class="name">{{ file.name }}</span>
              <div class="details">
                <span v-if="file.type === 'file' && file.simulated_size" class="size">
                  📏 {{ formatBytes(file.simulated_size) }}
                </span>
                <span v-if="file.permissions" class="permissions">
                  🔒 {{ file.permissions.toString(8) }}
                </span>
              </div>
            </div>
            <div class="actions">
              <button v-if="file.type === 'file'" @click.stop="showFileAddress(file.name)"
                      class="address-btn" title="查看文件地址">📍</button>
              <button @click.stop="deleteItem(file.name)" class="delete-btn" title="删除">🗑️</button>
            </div>
          </div>
        </div>
      </div>

      <div class="info-panel" v-if="selectedFile">
        <h4>📄 文件信息: {{ selectedFile }}</h4>
        <textarea v-model="fileContent" class="file-info-display" readonly></textarea>
      </div>
    </div>

    <div class="create-panel">
      <div class="create-section">
        <h4>📁 创建目录</h4>
        <input v-model="newDirName" placeholder="目录名称" />
        <button @click="createDirectory" :disabled="!newDirName.trim()">创建目录</button>
      </div>
    </div>

    <!-- 创建文件对话框 -->
    <div v-if="showCreateFileDialog" class="modal-backdrop" @click="showCreateFileDialog = false">
      <div class="modal-content" @click.stop>
        <div class="modal-header">
          <h3>📄 创建新文件</h3>
          <button @click="showCreateFileDialog = false" class="close-btn">❌</button>
        </div>
        <div class="modal-body">
          <div class="form-group">
            <label>文件名:</label>
            <input v-model="newFileName" placeholder="请输入文件名" class="form-input" />
          </div>
          <div class="form-group">
            <label>模拟大小:</label>
            <div class="size-input-group">
              <input v-model.number="newFileSimulatedSize" type="number" min="0" class="size-input" />
              <select v-model="fileSizeUnit" class="unit-select">
                <option value="B">B</option>
                <option value="KB">KB</option>
                <option value="MB">MB</option>
                <option value="GB">GB</option>
              </select>
            </div>
          </div>
          <div class="form-hint">
            💡 模拟大小用于演示不同的存储分配策略，实际不存储文件内容
          </div>
        </div>
        <div class="modal-footer">
          <button @click="showCreateFileDialog = false" class="cancel-btn">取消</button>
          <button @click="createFile" :disabled="!newFileName.trim()" class="confirm-btn">创建</button>
        </div>
      </div>
    </div>

    <!-- 文件地址信息对话框 -->
    <div v-if="showFileAddressDialog" class="modal-backdrop" @click="showFileAddressDialog = false">
      <div class="modal-content" @click.stop>
        <div class="modal-header">
          <h3>📍 文件存储地址</h3>
          <button @click="showFileAddressDialog = false" class="close-btn">❌</button>
        </div>
        <div class="modal-body" v-if="fileAddressInfo">
          <div class="address-info">
            <div class="address-path">
              <strong>文件路径:</strong> {{ fileAddressInfo.path }}
            </div>
            <div class="address-details">
              <h4>📦 存储地址信息:</h4>
              <div v-if="fileAddressInfo.addresses.contiguous !== undefined" class="address-item">
                <span class="address-type">连续分配:</span>
                <span class="address-value">块 #{{ fileAddressInfo.addresses.contiguous }}</span>
              </div>
              <div v-if="fileAddressInfo.addresses.linked !== undefined" class="address-item">
                <span class="address-type">链接分配:</span>
                <span class="address-value">起始块 #{{ fileAddressInfo.addresses.linked }}</span>
              </div>
              <div v-if="fileAddressInfo.addresses.indexed !== undefined" class="address-item">
                <span class="address-type">索引分配:</span>
                <span class="address-value">索引块 #{{ fileAddressInfo.addresses.indexed }}</span>
              </div>
            </div>
          </div>
        </div>
        <div class="modal-footer">
          <button @click="showFileAddressDialog = false" class="confirm-btn">关闭</button>
        </div>
      </div>
    </div>

    <!-- 文件右键菜单 -->
    <div
      v-if="showFileContextMenu"
      class="context-menu"
      :style="{ left: fileContextMenuPosition.x + 'px', top: fileContextMenuPosition.y + 'px' }"
      @click.stop
    >
      <div class="menu-item" @click="openFileFromMenu">
        <span class="menu-icon">📂</span>
        <span>打开</span>
      </div>
      <div v-if="contextMenuFile?.type === 'file'" class="menu-item" @click="showFileAddressFromMenu">
        <span class="menu-icon">📍</span>
        <span>查看地址</span>
      </div>
      <div class="menu-divider"></div>
      <div class="menu-item" @click="deleteFileFromMenu">
        <span class="menu-icon">🗑️</span>
        <span>删除</span>
      </div>
      <div v-if="contextMenuFile?.type === 'directory'" class="menu-item" @click="recursiveDeleteFromMenu">
        <span class="menu-icon">💥</span>
        <span>递归删除</span>
      </div>
      <div class="menu-divider"></div>
      <div class="menu-item" @click="showFileProperties">
        <span class="menu-icon">ℹ️</span>
        <span>属性</span>
      </div>
    </div>
  </div>
</template>

<style scoped>
.file-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  font-size: 12px;
  color: #333;
}

.toolbar {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px;
  border-bottom: 1px solid #ccc;
  background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%);
  color: #333;
}

.toolbar button {
  background: #007bff;
  border: 1px solid #0056b3;
  color: white;
  padding: 6px 12px;
  font-size: 11px;
  cursor: pointer;
  border-radius: 4px;
  display: flex;
  align-items: center;
  gap: 4px;
  transition: all 0.2s ease;
}

.toolbar button:hover {
  background: #0056b3;
  transform: translateY(-1px);
}

.toolbar button:disabled {
  background: #6c757d;
  border-color: #5a6268;
  color: #e9ecef;
  cursor: not-allowed;
  transform: none;
}

.toolbar .create-btn {
  background: #28a745;
  border-color: #1e7e34;
}

.toolbar .create-btn:hover {
  background: #1e7e34;
}

.path {
  flex-grow: 1;
  font-weight: bold;
  color: #2c3e50;
  background: rgba(255, 255, 255, 0.8);
  padding: 6px 12px;
  border-radius: 4px;
  border: 1px solid #dee2e6;
}

.error {
  background: linear-gradient(135deg, #f8d7da 0%, #f5c6cb 100%);
  color: #721c24;
  padding: 8px 12px;
  margin: 5px;
  border-radius: 6px;
  border: 1px solid #f5c6cb;
  box-shadow: 0 2px 4px rgba(0,0,0,0.1);
}

.main-content {
  display: flex;
  flex-grow: 1;
  min-height: 0;
  gap: 1px;
}

.file-list {
  flex: 1;
  overflow-y: auto;
  background: #f8f9fa;
  border-right: 1px solid #dee2e6;
}

.file-item {
  display: flex;
  align-items: center;
  padding: 8px 12px;
  border-bottom: 1px solid #e9ecef;
  cursor: pointer;
  transition: all 0.2s ease;
  background: white;
  margin: 2px;
  border-radius: 4px;
}

.file-item:hover {
  background: linear-gradient(135deg, #e3f2fd 0%, #bbdefb 100%);
  transform: translateX(2px);
  box-shadow: 0 2px 8px rgba(0,0,0,0.1);
}

.file-item.directory {
  font-weight: bold;
  background: linear-gradient(135deg, #f1f8e9 0%, #dcedc8 100%);
}

.file-item.directory:hover {
  background: linear-gradient(135deg, #e8f5e8 0%, #c8e6c8 100%);
}

.icon {
  margin-right: 12px;
  font-size: 16px;
  width: 20px;
  text-align: center;
}

.file-info {
  flex-grow: 1;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.name {
  color: #2c3e50;
  font-weight: 500;
  font-size: 13px;
}

.details {
  display: flex;
  gap: 12px;
  font-size: 10px;
}

.size {
  color: #6c757d;
  background: #e9ecef;
  padding: 2px 6px;
  border-radius: 3px;
}

.permissions {
  color: #495057;
  background: #fff3cd;
  padding: 2px 6px;
  border-radius: 3px;
}

.actions {
  display: flex;
  gap: 4px;
}

.address-btn, .delete-btn {
  background: transparent;
  border: 1px solid #dee2e6;
  padding: 4px 6px;
  font-size: 12px;
  cursor: pointer;
  border-radius: 3px;
  transition: all 0.2s ease;
}

.address-btn:hover {
  background: #17a2b8;
  color: white;
  border-color: #138496;
}

.delete-btn:hover {
  background: #dc3545;
  color: white;
  border-color: #c82333;
}

.info-panel {
  flex: 1;
  padding: 15px;
  display: flex;
  flex-direction: column;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
}

.info-panel h4 {
  margin: 0 0 10px 0;
  color: #495057;
  font-weight: 600;
}

.file-info-display {
  flex-grow: 1;
  font-family: 'Consolas', monospace;
  font-size: 11px;
  padding: 10px;
  border: 1px solid #ced4da;
  border-radius: 4px;
  background: white;
  resize: none;
}

.create-panel {
  border-top: 1px solid #dee2e6;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
  padding: 10px;
}

.create-section {
  padding: 10px;
}

.create-section h4 {
  margin: 0 0 8px 0;
  color: #495057;
  font-weight: 600;
}

.create-section input {
  width: 100%;
  margin-bottom: 8px;
  padding: 6px 10px;
  font-size: 11px;
  color: #495057;
  border: 1px solid #ced4da;
  border-radius: 4px;
  background: white;
}

.create-section button {
  background: #28a745;
  color: white;
  border: none;
  padding: 6px 12px;
  cursor: pointer;
  font-size: 11px;
  border-radius: 4px;
  transition: all 0.2s ease;
}

.create-section button:hover {
  background: #218838;
  transform: translateY(-1px);
}

.create-section button:disabled {
  background: #6c757d;
  cursor: not-allowed;
  transform: none;
}

/* 模态对话框样式 */
.modal-backdrop {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  justify-content: center;
  align-items: center;
  z-index: 10000;
}

.modal-content {
  background: white;
  border-radius: 8px;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
  min-width: 400px;
  max-width: 600px;
  max-height: 80vh;
  overflow-y: auto;
}

.modal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 15px 20px;
  border-bottom: 1px solid #dee2e6;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
}

.modal-header h3 {
  margin: 0;
  color: #495057;
  font-size: 16px;
  font-weight: 600;
}

.close-btn {
  background: transparent;
  border: none;
  font-size: 16px;
  cursor: pointer;
  padding: 4px;
  border-radius: 4px;
  transition: background 0.2s ease;
}

.close-btn:hover {
  background: rgba(220, 53, 69, 0.1);
}

.modal-body {
  padding: 20px;
}

.form-group {
  margin-bottom: 15px;
}

.form-group label {
  display: block;
  margin-bottom: 5px;
  font-weight: 500;
  color: #495057;
}

.form-input {
  width: 100%;
  padding: 8px 12px;
  border: 1px solid #ced4da;
  border-radius: 4px;
  font-size: 13px;
  transition: border-color 0.2s ease;
}

.form-input:focus {
  outline: none;
  border-color: #007bff;
  box-shadow: 0 0 0 2px rgba(0, 123, 255, 0.25);
}

.size-input-group {
  display: flex;
  gap: 8px;
}

.size-input {
  flex: 1;
  padding: 8px 12px;
  border: 1px solid #ced4da;
  border-radius: 4px;
  font-size: 13px;
}

.unit-select {
  padding: 8px 12px;
  border: 1px solid #ced4da;
  border-radius: 4px;
  background: white;
  font-size: 13px;
  cursor: pointer;
}

.form-hint {
  background: #e3f2fd;
  padding: 10px;
  border-radius: 4px;
  font-size: 11px;
  color: #1565c0;
  border-left: 4px solid #2196f3;
}

.modal-footer {
  display: flex;
  justify-content: flex-end;
  gap: 10px;
  padding: 15px 20px;
  border-top: 1px solid #dee2e6;
  background: #f8f9fa;
}

.cancel-btn, .confirm-btn {
  padding: 8px 16px;
  border: none;
  border-radius: 4px;
  font-size: 12px;
  cursor: pointer;
  transition: all 0.2s ease;
}

.cancel-btn {
  background: #6c757d;
  color: white;
}

.cancel-btn:hover {
  background: #5a6268;
}

.confirm-btn {
  background: #007bff;
  color: white;
}

.confirm-btn:hover {
  background: #0056b3;
}

.confirm-btn:disabled {
  background: #6c757d;
  cursor: not-allowed;
}

/* 文件地址信息样式 */
.address-info {
  font-size: 13px;
}

.address-path {
  background: #f8f9fa;
  padding: 10px;
  border-radius: 4px;
  margin-bottom: 15px;
  border: 1px solid #dee2e6;
}

.address-details h4 {
  color: #495057;
  margin: 0 0 10px 0;
  font-size: 14px;
}

.address-item {
  display: flex;
  justify-content: space-between;
  padding: 8px 12px;
  background: white;
  border: 1px solid #dee2e6;
  border-radius: 4px;
  margin-bottom: 8px;
}

.address-type {
  font-weight: 500;
  color: #495057;
}

.address-value {
  font-family: monospace;
  background: #e9ecef;
  padding: 2px 6px;
  border-radius: 3px;
  color: #495057;
}

/* 右键菜单样式 */
.context-menu {
  position: fixed;
  background: rgba(255, 255, 255, 0.95);
  backdrop-filter: blur(10px);
  border: 1px solid rgba(0, 0, 0, 0.15);
  border-radius: 8px;
  box-shadow:
    0 8px 32px rgba(0, 0, 0, 0.15),
    0 2px 8px rgba(0, 0, 0, 0.1);
  z-index: 10001;
  min-width: 160px;
  font-size: 12px;
  color: #333;
  padding: 6px 0;
  overflow: hidden;
}

.menu-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 8px 12px;
  cursor: pointer;
  transition: all 0.2s ease;
  color: #495057;
  font-weight: 500;
}

.menu-item:hover {
  background: linear-gradient(135deg, rgba(0, 123, 255, 0.1) 0%, rgba(0, 123, 255, 0.05) 100%);
  color: #007bff;
}

.menu-icon {
  width: 16px;
  text-align: center;
  font-size: 13px;
}

.menu-divider {
  height: 1px;
  background: linear-gradient(90deg,
    transparent 0%,
    rgba(0, 0, 0, 0.1) 20%,
    rgba(0, 0, 0, 0.1) 80%,
    transparent 100%);
  margin: 4px 0;
}

.loading {
  text-align: center;
  padding: 40px 20px;
  color: #6c757d;
  font-size: 14px;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
  margin: 10px;
  border-radius: 8px;
}
</style>
