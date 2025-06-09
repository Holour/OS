<script setup lang="ts">
import { defineAsyncComponent, ref, onMounted, onUnmounted } from 'vue';
import { useWindowsStore } from '@/stores/windows';
import AppWindow from './AppWindow.vue';
import DesktopIcon from './DesktopIcon.vue';
import { filesystemAPI, processAPI } from '@/services/api';

const windowsStore = useWindowsStore();

interface FileItem {
  name: string;
  type: 'file' | 'directory';
  size?: number;
  path: string;
}

const desktopFiles = ref<FileItem[]>([]);
const isLoading = ref(true);
const showContextMenu = ref(false);
const contextMenuPosition = ref({ x: 0, y: 0 });
const selectedIcon = ref<FileItem | null>(null);
const showFileContextMenu = ref(false);
const fileContextMenuPosition = ref({ x: 0, y: 0 });
const contextMenuFile = ref<FileItem | null>(null);

const componentMap: Record<string, any> = {
  ProcessManager: defineAsyncComponent(() => import('./apps/ProcessManager.vue')),
  Terminal: defineAsyncComponent(() => import('./apps/Terminal.vue')),
  FileManager: defineAsyncComponent(() => import('./apps/FileManager.vue')),
  MemoryManager: defineAsyncComponent(() => import('./apps/MemoryManager.vue')),
  DeviceManager: defineAsyncComponent(() => import('./apps/DeviceManager.vue')),
  SystemControl: defineAsyncComponent(() => import('./apps/SystemControl.vue')),
};

// 加载桌面文件（根目录）
const loadDesktopFiles = async () => {
  try {
    const response = await filesystemAPI.listDirectory('/');
    if (response.data.status === 'success' && response.data.data) {
      const data = response.data.data;
      const allFiles = [
        ...(data.directories || []).map((dir: any) => ({
          name: dir.name,
          type: 'directory' as const,
          size: undefined,
          path: `/${dir.name}`,
        })),
        ...(data.files || []).map((file: any) => ({
          name: file.name,
          type: 'file' as const,
          size: file.size,
          path: `/${file.name}`,
        }))
      ];
      desktopFiles.value = allFiles;
    }
  } catch (err) {
    console.error('Failed to load desktop files:', err);
  } finally {
    isLoading.value = false;
  }
};

// 处理文件夹双击 - 打开文件管理器窗口
const handleOpenFolder = (folderPath: string) => {
  const folderId = `file-manager-${folderPath.replace(/\//g, '-')}`;
  windowsStore.openWindow(
    folderId,
    `文件管理器 - ${folderPath}`,
    'FileManager',
    { initialPath: folderPath }
  );
};

// 处理文件双击 - 创建进程
const handleOpenFile = async (file: FileItem) => {
  try {
    const ext = file.name.split('.').pop()?.toLowerCase();

    if (ext === 'pubt') {
      // 处理.pubt可执行文件
      await handlePubtFile(file);
    } else {
      // 处理普通文件
      const processName = file.name.replace(/\.[^/.]+$/, '') || 'unnamed'; // 去掉扩展名
      const processSize = Math.max(1024, file.size || 1024); // 至少1KB

      const processResult = await processAPI.createProcess(processName, processSize);
      console.log(`文件 "${file.name}" 已创建进程 "${processName}"，进程ID: ${processResult.data.pid}`);
    }
  } catch (err: any) {
    console.error(`创建进程失败: ${err.message || '未知错误'}`);
  }
};

// 处理.pubt文件执行
const handlePubtFile = async (file: FileItem) => {
  try {
    // 读取.pubt文件内容获取内存大小
    const response = await filesystemAPI.readFile(file.path);
    if (response.data.status === 'success') {
      const content = response.data.data.content.trim();

      // 解析内存大小，支持如"10MB", "24MB", "5MB"等格式
      const memorySize = parseMemorySize(content);
      if (memorySize === null) {
        throw new Error(`无效的内存大小格式: ${content}`);
      }

      // 创建进程，使用文件名（去掉.pubt扩展名）作为进程名
      const processName = file.name.replace(/\.pubt$/, '') || 'unnamed';

      const processResult = await processAPI.createProcess(processName, memorySize);
      console.log(`程序 "${processName}" 已启动，进程ID: ${processResult.data.pid}，分配内存: ${formatBytes(memorySize)}`);
    } else {
      throw new Error('无法读取.pubt文件内容');
    }
  } catch (err: any) {
    throw err; // 重新抛出错误，让上层处理
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

// 计算图标位置（网格布局）
const getIconPosition = (index: number) => {
  const iconsPerColumn = Math.floor((window.innerHeight - 120) / 100); // 考虑任务栏高度
  const column = Math.floor(index / iconsPerColumn);
  const row = index % iconsPerColumn;

  return {
    x: 20 + column * 100,
    y: 20 + row * 100
  };
};

// 图标选中处理
const handleSelectIcon = (file: FileItem) => {
  selectedIcon.value = file;
};

// 点击空白区域取消选中
const handleDesktopClick = () => {
  selectedIcon.value = null;
  hideAllContextMenus();
};

const handleRefresh = async () => {
  isLoading.value = true;
  await loadDesktopFiles();
  hideAllContextMenus();
};

// 文件/文件夹右键菜单
const handleFileContextMenu = (event: MouseEvent, file: FileItem) => {
  event.preventDefault();
  event.stopPropagation();
  fileContextMenuPosition.value = { x: event.clientX, y: event.clientY };
  contextMenuFile.value = file;
  showFileContextMenu.value = true;
  showContextMenu.value = false; // 隐藏桌面右键菜单
};

// 桌面右键菜单处理
const handleContextMenu = (event: MouseEvent) => {
  event.preventDefault();
  contextMenuPosition.value = { x: event.clientX, y: event.clientY };
  showContextMenu.value = true;
  showFileContextMenu.value = false; // 隐藏文件右键菜单
};

const hideAllContextMenus = () => {
  showContextMenu.value = false;
  showFileContextMenu.value = false;
};

const hideContextMenu = () => {
  showContextMenu.value = false;
};

const createNewFolder = () => {
  const folderName = prompt('请输入文件夹名称:');
  if (folderName && folderName.trim()) {
    filesystemAPI.createDirectory(folderName.trim())
      .then(() => {
        loadDesktopFiles();
        alert('文件夹创建成功!');
      })
      .catch((err) => {
        alert(`创建文件夹失败: ${err.response?.data?.message || err.message}`);
      });
  }
  hideContextMenu();
};

const createNewFile = () => {
  const fileName = prompt('请输入文件名称:');
  if (fileName && fileName.trim()) {
    filesystemAPI.createFile(fileName.trim(), '')
      .then(() => {
        loadDesktopFiles();
        console.log('文件创建成功!');
      })
      .catch((err) => {
        console.error(`创建文件失败: ${err.response?.data?.message || err.message}`);
      });
  }
  hideContextMenu();
};

// 文件操作函数
const openFile = () => {
  if (contextMenuFile.value) {
    if (contextMenuFile.value.type === 'directory') {
      handleOpenFolder(contextMenuFile.value.path);
    } else {
      handleOpenFile(contextMenuFile.value);
    }
  }
  hideAllContextMenus();
};

const deleteFile = () => {
  if (!contextMenuFile.value) return;

  const confirmMsg = `确定要删除 "${contextMenuFile.value.name}" 吗？`;
  if (confirm(confirmMsg)) {
    filesystemAPI.delete(contextMenuFile.value.path)
      .then(() => {
        loadDesktopFiles();
        selectedIcon.value = null;
        console.log('删除成功!');
      })
      .catch((err) => {
        console.error(`删除失败: ${err.response?.data?.message || err.message}`);
      });
  }
  hideAllContextMenus();
};

const renameFile = () => {
  if (!contextMenuFile.value) return;

  const newName = prompt('请输入新名称:', contextMenuFile.value.name);
  if (newName && newName.trim() && newName !== contextMenuFile.value.name) {
    // 这里需要实现重命名API，暂时显示提示
    console.log(`重命名功能待实现: ${contextMenuFile.value.name} -> ${newName}`);
    alert('重命名功能暂未实现');
  }
  hideAllContextMenus();
};

const showProperties = () => {
  if (!contextMenuFile.value) return;

  const file = contextMenuFile.value;
  const props = [
    `名称: ${file.name}`,
    `类型: ${file.type === 'directory' ? '文件夹' : '文件'}`,
    `路径: ${file.path}`,
    file.size ? `大小: ${file.size} 字节` : ''
  ].filter(Boolean).join('\n');

  alert(`文件属性:\n\n${props}`);
  hideAllContextMenus();
};

onMounted(() => {
  loadDesktopFiles();
  // 点击其他地方隐藏右键菜单
  document.addEventListener('click', hideAllContextMenus);
});

onUnmounted(() => {
  document.removeEventListener('click', hideAllContextMenus);
});
</script>

<template>
  <div class="desktop" @contextmenu="handleContextMenu" @click="handleDesktopClick">
    <!-- 桌面文件图标 -->
    <DesktopIcon
      v-for="(file, index) in desktopFiles"
      :key="file.name"
      :file="file"
      :position="getIconPosition(index)"
      :isSelected="selectedIcon?.name === file.name"
      @openFolder="handleOpenFolder"
      @openFile="handleOpenFile"
      @selectIcon="handleSelectIcon"
      @contextMenu="handleFileContextMenu"
    />

    <!-- 桌面右键菜单 -->
    <div
      v-if="showContextMenu"
      class="context-menu"
      :style="{ left: contextMenuPosition.x + 'px', top: contextMenuPosition.y + 'px' }"
      @click.stop
    >
      <div class="menu-item" @click="createNewFolder">
        <span class="menu-icon">📁</span>
        <span>新建文件夹</span>
      </div>
      <div class="menu-item" @click="createNewFile">
        <span class="menu-icon">📄</span>
        <span>新建文件</span>
      </div>
      <div class="menu-divider"></div>
      <div class="menu-item" @click="handleRefresh">
        <span class="menu-icon">🔄</span>
        <span>刷新</span>
      </div>
    </div>

    <!-- 文件/文件夹右键菜单 -->
    <div
      v-if="showFileContextMenu"
      class="context-menu file-context-menu"
      :style="{ left: fileContextMenuPosition.x + 'px', top: fileContextMenuPosition.y + 'px' }"
      @click.stop
    >
      <div class="menu-item" @click="openFile">
        <span class="menu-icon">📂</span>
        <span>打开</span>
      </div>
      <div class="menu-divider"></div>
      <div class="menu-item" @click="deleteFile">
        <span class="menu-icon">🗑️</span>
        <span>删除</span>
      </div>
      <div class="menu-item" @click="renameFile">
        <span class="menu-icon">✏️</span>
        <span>重命名</span>
      </div>
      <div class="menu-divider"></div>
      <div class="menu-item" @click="showProperties">
        <span class="menu-icon">ℹ️</span>
        <span>属性</span>
      </div>
    </div>

    <!-- 应用窗口 -->
    <AppWindow
      v-for="win in windowsStore.windows"
      :key="win.id"
      :windowState="win"
      v-show="win.isVisible"
    >
      <component :is="componentMap[win.component]" v-bind="win.props" />
    </AppWindow>
  </div>
</template>

<style scoped>
.desktop {
  flex-grow: 1;
  position: relative;
  background: url('@/assets/wallpaper.jpg') center center / cover no-repeat;
  overflow: hidden;
  min-height: 0;
}

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
  font-size: 13px;
  color: #333;
  padding: 6px 0;
  overflow: hidden;
}

.file-context-menu {
  min-width: 140px;
}

.menu-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px 16px;
  cursor: pointer;
  transition: all 0.2s ease;
  color: #333;
  font-weight: 500;
  position: relative;
}

.menu-item:hover {
  background-color: rgba(0, 120, 215, 0.1);
  color: #0078d4;
}

.menu-item:active {
  background-color: rgba(0, 120, 215, 0.2);
}

.menu-icon {
  width: 16px;
  text-align: center;
  font-size: 14px;
}

.menu-divider {
  height: 1px;
  background: linear-gradient(90deg,
    transparent 0%,
    rgba(0, 0, 0, 0.1) 20%,
    rgba(0, 0, 0, 0.1) 80%,
    transparent 100%);
  margin: 6px 0;
}

.menu-divider {
  height: 1px;
  background-color: #eee;
  margin: 4px 0;
}
</style>
