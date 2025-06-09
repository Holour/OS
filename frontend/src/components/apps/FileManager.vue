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

const loadDirectory = async (path: string = currentPath.value) => {
  isLoading.value = true;
  error.value = null;
  try {
    const response = await filesystemAPI.listDirectory(path);
    if (response.data.status === 'success') {
      const data = response.data.data;
      // 根据API文档，数据结构包含files和directories数组
      const allFiles = [
        ...(data.directories || []).map((dir: any) => ({
          name: dir.name,
          type: 'directory' as const,
          size: undefined,
          permissions: dir.permissions,
          last_modified: dir.last_modified
        })),
        ...(data.files || []).map((file: any) => ({
          name: file.name,
          type: 'file' as const,
          size: file.size,
          permissions: file.permissions,
          last_modified: file.last_modified
        }))
      ];
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

const createFile = async () => {
  if (!newFileName.value.trim()) return;
  try {
    const filePath = currentPath.value === '/' ? newFileName.value : `${currentPath.value}/${newFileName.value}`;
    await filesystemAPI.createFile(filePath, fileContent.value);
    newFileName.value = '';
    fileContent.value = '';
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

const deleteItem = async (itemName: string) => {
  if (!confirm(`确定要删除 ${itemName} 吗？`)) return;
  try {
    const itemPath = currentPath.value === '/' ? itemName : `${currentPath.value}/${itemName}`;
    await filesystemAPI.delete(itemPath);
    loadDirectory();
  } catch (err: any) {
    error.value = `删除失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

const readFile = async (fileName: string) => {
  try {
    const filePath = currentPath.value === '/' ? fileName : `${currentPath.value}/${fileName}`;
    const response = await filesystemAPI.readFile(filePath);
    if (response.data.status === 'success') {
      selectedFile.value = fileName;
      fileContent.value = response.data.data.content || '';
    }
  } catch (err: any) {
    error.value = `读取文件失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

const saveFile = async () => {
  if (!selectedFile.value) return;
  try {
    const filePath = currentPath.value === '/' ? selectedFile.value : `${currentPath.value}/${selectedFile.value}`;
    await filesystemAPI.writeFile(filePath, fileContent.value);
    alert('文件保存成功！');
  } catch (err: any) {
    error.value = `保存文件失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

// 处理.pubt文件执行
const handlePubtFile = async (fileName: string) => {
  try {
    const filePath = currentPath.value === '/' ? fileName : `${currentPath.value}/${fileName}`;

    // 读取.pubt文件内容获取内存大小
    const response = await filesystemAPI.readFile(filePath);
    if (response.data.status === 'success') {
      const content = response.data.data.content.trim();

      // 解析内存大小，支持如"10MB", "24MB", "5MB"等格式
      const memorySize = parseMemorySize(content);
      if (memorySize === null) {
        throw new Error(`无效的内存大小格式: ${content}`);
      }

      // 创建进程，使用文件名（去掉.pubt扩展名）作为进程名
      const processName = fileName.replace(/\.pubt$/, '') || 'unnamed';

      const processResult = await processAPI.createProcess(processName, memorySize);
      console.log(`程序 "${processName}" 已启动，进程ID: ${processResult.data.pid}，分配内存: ${formatBytes(memorySize)}`);

      // 可以选择显示成功消息
      alert(`程序 "${processName}" 已启动！分配内存: ${formatBytes(memorySize)}`);
    } else {
      throw new Error('无法读取.pubt文件内容');
    }
  } catch (err: any) {
    error.value = `执行程序失败: ${err.response?.data?.message || err.message || '未知错误'}`;
  }
};

// 解析内存大小字符串，返回字节数
const parseMemorySize = (sizeStr: string): number | null => {
  const match = sizeStr.match(/^(\d+(?:\.\d+)?)\s*(MB|KB|GB|B)$/i);
  if (!match) return null;

  const value = parseFloat(match[1]);
  const unit = match[2].toUpperCase();

  switch (unit) {
    case 'B': return value;
    case 'KB': return value * 1024;
    case 'MB': return value * 1024 * 1024;
    case 'GB': return value * 1024 * 1024 * 1024;
    default: return null;
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
    // 普通文件，进入编辑模式
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

const showFileProperties = () => {
  if (!contextMenuFile.value) return;

  const file = contextMenuFile.value;
  const props = [
    `名称: ${file.name}`,
    `类型: ${file.type === 'directory' ? '文件夹' : '文件'}`,
    `路径: ${currentPath.value}/${file.name}`,
    file.size ? `大小: ${file.size} 字节` : '',
    `权限: ${(file as any).permissions || '未知'}`
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
      <button @click="goBack" :disabled="currentPath === '/'">上一级</button>
      <span class="path">{{ currentPath }}</span>
      <button @click="loadDirectory()">刷新</button>
    </div>

    <div v-if="error" class="error">{{ error }}</div>

    <div class="main-content">
      <div class="file-list">
        <div v-if="isLoading" class="loading">加载中...</div>
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
            <span class="name">{{ file.name }}</span>
            <span class="size" v-if="file.type === 'file'">{{ file.size || 0 }} bytes</span>
            <button @click="deleteItem(file.name)" class="delete-btn">删除</button>
          </div>
        </div>
      </div>

      <div class="editor-panel" v-if="selectedFile">
        <h4>编辑文件: {{ selectedFile }}</h4>
        <textarea v-model="fileContent" class="file-editor"></textarea>
        <button @click="saveFile" class="save-btn">保存</button>
      </div>
    </div>

    <div class="create-panel">
      <div class="create-section">
        <h4>创建文件</h4>
        <input v-model="newFileName" placeholder="文件名" />
        <textarea v-model="fileContent" placeholder="文件内容" rows="3"></textarea>
        <button @click="createFile">创建文件</button>
      </div>

      <div class="create-section">
        <h4>创建目录</h4>
        <input v-model="newDirName" placeholder="目录名" />
        <button @click="createDirectory">创建目录</button>
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
      <div class="menu-divider"></div>
      <div class="menu-item" @click="deleteFileFromMenu">
        <span class="menu-icon">🗑️</span>
        <span>删除</span>
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
  padding: 5px;
  border-bottom: 1px solid #ccc;
  background-color: #f5f5f5;
  color: #333;
}

.toolbar button {
  background-color: #e9ecef;
  border: 1px solid #ced4da;
  color: #333;
  padding: 4px 8px;
  font-size: 11px;
  cursor: pointer;
  border-radius: 3px;
}

.toolbar button:hover {
  background-color: #dee2e6;
}

.toolbar button:disabled {
  background-color: #f8f9fa;
  color: #6c757d;
  cursor: not-allowed;
}

.path {
  flex-grow: 1;
  font-weight: bold;
  color: #222;
}

.error {
  background-color: #ffe6e6;
  color: #d8000c;
  padding: 5px;
  margin: 5px 0;
}

.main-content {
  display: flex;
  flex-grow: 1;
  min-height: 0;
}

.file-list {
  flex: 1;
  overflow-y: auto;
  border-right: 1px solid #ccc;
}

.file-item {
  display: flex;
  align-items: center;
  padding: 5px;
  border-bottom: 1px solid #eee;
  cursor: pointer;
}

.file-item:hover {
  background-color: #f0f0f0;
}

.file-item.directory {
  font-weight: bold;
}

.icon {
  margin-right: 8px;
}

.name {
  flex-grow: 1;
  color: #222;
  font-weight: 500;
}

.size {
  margin-right: 10px;
  color: #555;
  font-size: 10px;
  font-weight: 500;
}

.delete-btn {
  background-color: #ff4444;
  color: white;
  border: none;
  padding: 2px 6px;
  font-size: 10px;
  cursor: pointer;
}

.editor-panel {
  flex: 1;
  padding: 10px;
  display: flex;
  flex-direction: column;
}

.file-editor {
  flex-grow: 1;
  font-family: 'Consolas', monospace;
  font-size: 12px;
}

.save-btn {
  margin-top: 5px;
  background-color: #007acc;
  color: white;
  border: none;
  padding: 5px 10px;
  cursor: pointer;
}

.create-panel {
  display: flex;
  border-top: 1px solid #ccc;
  background-color: #f9f9f9;
}

.create-section {
  flex: 1;
  padding: 10px;
  border-right: 1px solid #ccc;
}

.create-section:last-child {
  border-right: none;
}

.create-section h4 {
  margin: 0 0 5px 0;
  color: #222;
  font-weight: 600;
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
  min-width: 140px;
  font-size: 12px;
  color: #333;
  padding: 6px 0;
  overflow: hidden;
}

.menu-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  cursor: pointer;
  transition: all 0.2s ease;
  color: #333;
  font-weight: 500;
}

.menu-item:hover {
  background-color: rgba(0, 120, 215, 0.1);
  color: #0078d4;
}

.menu-icon {
  width: 14px;
  text-align: center;
  font-size: 12px;
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

.create-section input,
.create-section textarea {
  width: 100%;
  margin-bottom: 5px;
  padding: 3px;
  font-size: 11px;
  color: #333;
  border: 1px solid #ccc;
  border-radius: 3px;
}

.create-section button {
  background-color: #28a745;
  color: white;
  border: none;
  padding: 5px 10px;
  cursor: pointer;
  font-size: 11px;
}

.loading {
  text-align: center;
  padding: 20px;
}
</style>
