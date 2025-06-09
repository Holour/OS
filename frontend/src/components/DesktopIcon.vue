<script setup lang="ts">
import { ref } from 'vue';

interface FileItem {
  name: string;
  type: 'file' | 'directory';
  size?: number;
  path: string;
}

const props = defineProps<{
  file: FileItem;
  position: { x: number; y: number };
  isSelected?: boolean;
}>();

const emit = defineEmits<{
  openFolder: [path: string];
  openFile: [file: FileItem];
  selectIcon: [file: FileItem];
  contextMenu: [event: MouseEvent, file: FileItem];
}>();

const isHovered = ref(false);

const handleDoubleClick = () => {
  if (props.file.type === 'directory') {
    emit('openFolder', props.file.path);
  } else {
    emit('openFile', props.file);
  }
};

const handleClick = () => {
  emit('selectIcon', props.file);
};

const handleContextMenu = (event: MouseEvent) => {
  event.preventDefault();
  event.stopPropagation();
  emit('contextMenu', event, props.file);
};

const getIcon = () => {
  if (props.file.type === 'directory') {
    return '📁';
  }

  const ext = props.file.name.split('.').pop()?.toLowerCase();
  switch (ext) {
    case 'txt': return '📄';
    case 'log': return '📋';
    case 'conf':
    case 'config': return '⚙️';
    case 'exe': return '⚡';
    case 'pubt': return '🚀'; // .pubt文件显示火箭图标，表示可执行程序
    default: return '📄';
  }
};
</script>

<template>
  <div
    class="desktop-icon"
    :style="{ left: position.x + 'px', top: position.y + 'px' }"
    :class="{ selected: isSelected, hovered: isHovered }"
    @click="handleClick"
    @dblclick="handleDoubleClick"
    @contextmenu="handleContextMenu"
    @mouseenter="isHovered = true"
    @mouseleave="isHovered = false"
  >
    <div class="icon-container">
      <div class="icon">{{ getIcon() }}</div>
      <div class="label">{{ file.name }}</div>
    </div>
  </div>
</template>

<style scoped>
.desktop-icon {
  position: absolute;
  width: 80px;
  cursor: pointer;
  user-select: none;
  transition: transform 0.1s ease;
}

.desktop-icon.hovered {
  transform: scale(1.05);
}

.icon-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 6px;
  border-radius: 6px;
  transition: all 0.2s ease;
  border: 2px solid transparent;
}

.desktop-icon:hover .icon-container {
  background-color: rgba(255, 255, 255, 0.15);
  backdrop-filter: blur(5px);
  border-color: rgba(255, 255, 255, 0.3);
}

.desktop-icon.selected .icon-container {
  background-color: rgba(0, 120, 215, 0.6);
  border-color: rgba(0, 120, 215, 0.8);
  box-shadow: 0 0 10px rgba(0, 120, 215, 0.4);
}

.desktop-icon:active .icon-container {
  background-color: rgba(255, 255, 255, 0.25);
  transform: scale(0.98);
}

.icon {
  font-size: 36px;
  margin-bottom: 6px;
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.6));
  transition: filter 0.2s ease;
}

.desktop-icon.selected .icon {
  filter: drop-shadow(0 2px 6px rgba(0, 0, 0, 0.8));
}

.label {
  color: white;
  font-size: 12px;
  font-weight: 500;
  text-align: center;
  text-shadow:
    0 1px 2px rgba(0, 0, 0, 0.9),
    0 0 4px rgba(0, 0, 0, 0.5);
  word-wrap: break-word;
  max-width: 72px;
  line-height: 1.3;
  background-color: rgba(0, 0, 0, 0.2);
  padding: 2px 4px;
  border-radius: 3px;
  backdrop-filter: blur(2px);
}

.desktop-icon.selected .label {
  background-color: rgba(0, 120, 215, 0.7);
  color: white;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.8);
}
</style>
