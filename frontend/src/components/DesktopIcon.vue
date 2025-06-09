<script setup lang="ts">
import { ref, watch } from 'vue';

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
  updatePosition: [file: FileItem, position: { x: number; y: number }];
}>();

const isHovered = ref(false);
const isDragging = ref(false);
const dragOffset = ref({ x: 0, y: 0 });
const currentPosition = ref({ ...props.position });

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

const startDrag = (event: MouseEvent) => {
  event.preventDefault();

  // 防止双击时触发拖拽
  if (event.detail === 2) return;

  isDragging.value = true;

  const rect = (event.target as HTMLElement).closest('.desktop-icon')?.getBoundingClientRect();
  if (rect) {
    dragOffset.value = {
      x: event.clientX - rect.left,
      y: event.clientY - rect.top
    };
  }

  const handleMouseMove = (moveEvent: MouseEvent) => {
    if (!isDragging.value) return;

    currentPosition.value = {
      x: moveEvent.clientX - dragOffset.value.x,
      y: moveEvent.clientY - dragOffset.value.y
    };
  };

  const handleMouseUp = () => {
    if (isDragging.value) {
      isDragging.value = false;

      // 网格对齐逻辑
      const gridSize = 100; // 网格大小
      const margin = 20; // 边距

      // 计算对齐后的位置
      const alignedX = Math.round((currentPosition.value.x - margin) / gridSize) * gridSize + margin;
      const alignedY = Math.round((currentPosition.value.y - margin) / gridSize) * gridSize + margin;

      // 边界检查
      const maxX = window.innerWidth - 100;
      const maxY = window.innerHeight - 150; // 考虑任务栏高度

      const finalPosition = {
        x: Math.max(margin, Math.min(alignedX, maxX)),
        y: Math.max(margin, Math.min(alignedY, maxY))
      };

      // 更新最终位置
      currentPosition.value = finalPosition;
      emit('updatePosition', props.file, finalPosition);
    }

    document.removeEventListener('mousemove', handleMouseMove);
    document.removeEventListener('mouseup', handleMouseUp);
  };

  document.addEventListener('mousemove', handleMouseMove);
  document.addEventListener('mouseup', handleMouseUp);
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

// 监听props.position变化，更新当前位置
watch(() => props.position, (newPos) => {
  if (!isDragging.value) {
    currentPosition.value = { ...newPos };
  }
}, { immediate: true });
</script>

<template>
  <div
    class="desktop-icon"
    :style="{
      left: (isDragging ? currentPosition.x : position.x) + 'px',
      top: (isDragging ? currentPosition.y : position.y) + 'px'
    }"
    :class="{
      selected: isSelected,
      hovered: isHovered,
      dragging: isDragging
    }"
    @click="handleClick"
    @dblclick="handleDoubleClick"
    @contextmenu="handleContextMenu"
    @mouseenter="isHovered = true"
    @mouseleave="isHovered = false"
    @mousedown="startDrag"
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
  transition: transform 0.15s ease, box-shadow 0.2s ease;
  z-index: 1;
}

.desktop-icon.dragging {
  z-index: 1000;
  cursor: grabbing;
  transform: scale(1.1) rotate(2deg);
  box-shadow: 0 8px 25px rgba(0, 0, 0, 0.3);
  transition: none;
}

.desktop-icon.hovered:not(.dragging) {
  transform: scale(1.05);
}

.icon-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 8px;
  border-radius: 8px;
  transition: all 0.2s ease;
  border: 2px solid transparent;
  backdrop-filter: blur(5px);
}

.desktop-icon:hover .icon-container {
  background-color: rgba(255, 255, 255, 0.2);
  border-color: rgba(255, 255, 255, 0.4);
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
}

.desktop-icon.selected .icon-container {
  background-color: rgba(0, 120, 215, 0.7);
  border-color: rgba(0, 120, 215, 0.9);
  box-shadow: 0 0 15px rgba(0, 120, 215, 0.5);
}

.desktop-icon.dragging .icon-container {
  background-color: rgba(255, 255, 255, 0.9);
  border-color: rgba(0, 120, 215, 0.6);
  box-shadow: 0 8px 30px rgba(0, 0, 0, 0.25);
}

.desktop-icon:active:not(.dragging) .icon-container {
  background-color: rgba(255, 255, 255, 0.3);
  transform: scale(0.98);
}

.icon {
  font-size: 40px;
  margin-bottom: 6px;
  filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.6));
  transition: filter 0.2s ease, transform 0.2s ease;
}

.desktop-icon.selected .icon {
  filter: drop-shadow(0 2px 8px rgba(0, 0, 0, 0.8));
}

.desktop-icon.dragging .icon {
  filter: drop-shadow(0 4px 12px rgba(0, 0, 0, 0.4));
  transform: scale(1.1);
}

.label {
  color: white;
  font-size: 12px;
  font-weight: 600;
  text-align: center;
  text-shadow:
    0 1px 3px rgba(0, 0, 0, 0.9),
    0 0 6px rgba(0, 0, 0, 0.5);
  word-wrap: break-word;
  max-width: 75px;
  line-height: 1.3;
  background-color: rgba(0, 0, 0, 0.3);
  padding: 3px 6px;
  border-radius: 4px;
  backdrop-filter: blur(3px);
  transition: all 0.2s ease;
}

.desktop-icon.selected .label {
  background-color: rgba(0, 120, 215, 0.8);
  color: white;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.8);
}

.desktop-icon.dragging .label {
  background-color: rgba(0, 0, 0, 0.4);
  transform: scale(1.05);
}
</style>
