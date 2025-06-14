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
  simulated_size?: number;
}

const desktopFiles = ref<FileItem[]>([]);
const isLoading = ref(true);
const showContextMenu = ref(false);
const contextMenuPosition = ref({ x: 0, y: 0 });
const selectedIcon = ref<FileItem | null>(null);
const showFileContextMenu = ref(false);
const fileContextMenuPosition = ref({ x: 0, y: 0 });
const contextMenuFile = ref<FileItem | null>(null);

// 图标位置管理
const iconPositions = ref<Record<string, { x: number; y: number }>>({});

const componentMap: Record<string, any> = {
  ProcessManager: defineAsyncComponent(() => import('./apps/ProcessManager.vue')),
  Terminal: defineAsyncComponent(() => import('./apps/Terminal.vue')),
  FileManager: defineAsyncComponent(() => import('./apps/FileManager.vue')),
  FileSystemConfig: defineAsyncComponent(() => import('./apps/FileSystemConfig.vue')),
  MemoryManager: defineAsyncComponent(() => import('./apps/MemoryManager.vue')),
  DeviceManager: defineAsyncComponent(() => import('./apps/DeviceManager.vue')),
  SystemControl: defineAsyncComponent(() => import('./apps/SystemControl.vue')),
  FileAddressViewer: defineAsyncComponent(() => import('./apps/FileAddressViewer.vue')),
  MusicPlayer: defineAsyncComponent(() => import('./apps/MusicPlayer.vue')),
};

// 加载桌面文件（根目录）
const loadDesktopFiles = async () => {
  try {
    const response = await filesystemAPI.listDirectory('/');
    if (response.data.status === 'success' && response.data.data) {
      const data = response.data.data;
      // 根据新的API结构更新数据映射
      const allFiles = data.map((item: any) => ({
        name: item.name,
        type: item.type,
        size: item.size,
        simulated_size: item.simulated_size,
        path: item.type === 'directory' ? `/${item.name}` : `/${item.name}`,
        permissions: item.permissions,
        last_modified: item.last_modified
      }));
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
      const processSize = Math.max(1024, file.simulated_size || file.size || 1024); // 至少1KB，优先使用模拟大小

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
    // 读取.pubt文件信息获取模拟大小
    const response = await filesystemAPI.readFile(file.path);
    if (response.data.status === 'success') {
      // 使用文件的模拟大小作为内存大小，如果没有则使用默认值
      const memorySize = response.data.data.simulated_size || file.simulated_size || 1024; // 默认1KB

      // 创建进程，使用文件名（去掉.pubt扩展名）作为进程名
      const processName = file.name.replace(/\.pubt$/, '') || 'unnamed';

      // 根据程序名称启动对应的应用
      if (processName.toLowerCase().includes('音乐') || processName.toLowerCase().includes('music')) {
        // 启动音乐播放器
        windowsStore.openWindow('music-player', '在线音乐', 'MusicPlayer', {}, { center: true });
        console.log(`音乐播放器 "${processName}" 已启动`);
      } else {
        // 其他应用只创建进程
        const processResult = await processAPI.createProcess(memorySize, 1000, 5);
        console.log(`程序 "${processName}" 已启动，进程ID: ${processResult.data.data.pid}，分配内存: ${formatBytes(memorySize)}`);
      }
    } else {
      throw new Error('无法读取.pubt文件信息');
    }
  } catch (err: any) {
    throw err; // 重新抛出错误，让上层处理
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
const getIconPosition = (file: FileItem, index: number) => {
  // 如果有保存的位置，使用保存的位置
  if (iconPositions.value[file.name]) {
    return iconPositions.value[file.name];
  }

  // 否则使用默认网格位置
  const iconsPerColumn = Math.floor((window.innerHeight - 120) / 100); // 考虑任务栏高度
  const column = Math.floor(index / iconsPerColumn);
  const row = index % iconsPerColumn;

  const position = {
    x: 20 + column * 100,
    y: 20 + row * 100
  };

  // 保存默认位置
  iconPositions.value[file.name] = position;
  return position;
};

// 更新图标位置
const updateIconPosition = (file: FileItem, position: { x: number; y: number }) => {
  // 边界检查
  const maxX = window.innerWidth - 100;
  const maxY = window.innerHeight - 150; // 考虑任务栏高度

  const clampedPosition = {
    x: Math.max(0, Math.min(position.x, maxX)),
    y: Math.max(0, Math.min(position.y, maxY))
  };

  iconPositions.value[file.name] = clampedPosition;

  // 可以在这里添加保存到localStorage的逻辑
  saveIconPositions();
};

// 保存图标位置到localStorage
const saveIconPositions = () => {
  try {
    localStorage.setItem('desktop-icon-positions', JSON.stringify(iconPositions.value));
  } catch (error) {
    console.warn('Failed to save icon positions:', error);
  }
};

// 从localStorage加载图标位置
const loadIconPositions = () => {
  try {
    const saved = localStorage.getItem('desktop-icon-positions');
    if (saved) {
      iconPositions.value = JSON.parse(saved);
    }
  } catch (error) {
    console.warn('Failed to load icon positions:', error);
    iconPositions.value = {};
  }
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
    const sizeStr = prompt('请输入模拟大小(例如: 1024, 1KB, 1MB):', '1024');
    let simulatedSize = 1024; // 默认1KB

    if (sizeStr && sizeStr.trim()) {
      // 简单解析大小字符串
      const size = parseFloat(sizeStr);
      if (!isNaN(size)) {
        if (sizeStr.toLowerCase().includes('kb')) {
          simulatedSize = size * 1024;
        } else if (sizeStr.toLowerCase().includes('mb')) {
          simulatedSize = size * 1024 * 1024;
        } else if (sizeStr.toLowerCase().includes('gb')) {
          simulatedSize = size * 1024 * 1024 * 1024;
        } else {
          simulatedSize = size;
        }
      }
    }

    filesystemAPI.createFile(fileName.trim(), simulatedSize)
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

// 处理文件地址查看请求
const handleOpenFileAddress = (filePath: string) => {
  const fileName = filePath.split('/').pop() || 'unknown';
  const windowId = `file-address-${filePath.replace(/\//g, '-')}`;
  windowsStore.openWindow(
    windowId,
    `文件地址 - ${fileName}`,
    'FileAddressViewer',
    { filePath }
  );
};

// 创建音乐播放器桌面快捷方式
const createMusicPlayerShortcut = async () => {
  try {
    // 检查是否已经存在音乐播放器快捷方式
    const existingFile = desktopFiles.value.find(file =>
      file.name.includes('音乐') || file.name.toLowerCase().includes('music')
    );

    if (!existingFile) {
      // 创建音乐播放器快捷方式
      await filesystemAPI.createFile('在线音乐.pubt', 4096, 0o755); // 4KB, 可执行权限
      console.log('音乐播放器桌面快捷方式已创建');
      await loadDesktopFiles(); // 重新加载桌面文件
    }
  } catch (err) {
    console.error('创建音乐播放器快捷方式失败:', err);
  }
};

onMounted(() => {
  loadDesktopFiles();
  loadIconPositions();
  createMusicPlayerShortcut(); // 创建音乐播放器快捷方式
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
      :position="getIconPosition(file, index)"
      :isSelected="selectedIcon?.name === file.name"
      @openFolder="handleOpenFolder"
              @openFile="handleOpenFile"
        @selectIcon="handleSelectIcon"
        @contextMenu="handleFileContextMenu"
        @updatePosition="updateIconPosition"
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
      <component
        :is="componentMap[win.component]"
        v-bind="win.props"
        @openFileAddress="handleOpenFileAddress"
      />
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
  background: rgba(255, 255, 255, 0.9);
  backdrop-filter: blur(20px) saturate(180%);
  border: 1px solid rgba(255, 255, 255, 0.3);
  border-radius: 12px;
  box-shadow:
    0 12px 40px rgba(0, 0, 0, 0.15),
    0 6px 16px rgba(0, 0, 0, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.7);
  z-index: 10001;
  min-width: 180px;
  font-size: 14px;
  color: #323130;
  padding: 8px 0;
  overflow: hidden;
}

.file-context-menu {
  min-width: 160px;
}

.menu-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 20px;
  cursor: pointer;
  transition: all 0.2s cubic-bezier(0.25, 0.46, 0.45, 0.94);
  color: #323130;
  font-weight: 500;
  position: relative;
}

.menu-item::before {
  content: '';
  position: absolute;
  left: 0;
  top: 0;
  bottom: 0;
  width: 3px;
  background: linear-gradient(to bottom, #0078d4, #106ebe);
  transform: scaleY(0);
  transition: transform 0.2s ease;
  border-radius: 0 2px 2px 0;
}

.menu-item:hover::before {
  transform: scaleY(1);
}

.menu-item:hover {
  background: rgba(0, 120, 215, 0.08);
  color: #0078d4;
  padding-left: 24px;
  transform: translateX(4px);
}

.menu-item:active {
  background: rgba(0, 120, 215, 0.12);
  transform: translateX(2px);
}

.menu-icon {
  width: 18px;
  text-align: center;
  font-size: 16px;
  filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.1));
}

.menu-divider {
  height: 1px;
  background: linear-gradient(90deg,
    transparent 0%,
    rgba(0, 0, 0, 0.08) 20%,
    rgba(0, 0, 0, 0.08) 80%,
    transparent 100%);
  margin: 8px 0;
}
</style>
