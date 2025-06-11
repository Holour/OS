<script setup lang="ts">
import { ref, onMounted } from 'vue';
import type { WindowState } from '@/stores/windows';
import { useWindowsStore } from '@/stores/windows';

const props = defineProps<{
  windowState: WindowState;
}>();

const windowStore = useWindowsStore();
const windowEl = ref<HTMLElement | null>(null);
const isResizing = ref(false);
const isDragging = ref(false);
const resizeDirection = ref<string>('');

// 调整大小的方向定义
type ResizeDirection = 'n' | 's' | 'e' | 'w' | 'ne' | 'nw' | 'se' | 'sw' | '';

const startDrag = (event: MouseEvent) => {
  if (!windowEl.value) return;

  event.preventDefault();
  windowStore.focusWindow(props.windowState.id);

  // 如果窗口是最大化状态，拖动时先恢复到正常大小
  if (props.windowState.isMaximized) {
    windowStore.maximizeWindow(props.windowState.id); // 这会切换回正常状态并居中

    // 不立即开始拖动，让用户看到窗口已经恢复到中央
    // 如果用户继续移动鼠标，会触发一个新的mousedown事件
    return;
  }

  startDragFromPosition(event);
};

const startDragFromPosition = (event: MouseEvent) => {
  isDragging.value = true;

  const startX = event.clientX;
  const startY = event.clientY;
  const initialX = props.windowState.position.x;
  const initialY = props.windowState.position.y;

  let animationId: number | null = null;
  let lastMoveEvent: MouseEvent | null = null;

  const doDrag = (moveEvent: MouseEvent) => {
    lastMoveEvent = moveEvent;

    // 使用requestAnimationFrame来优化性能
    if (animationId) {
      cancelAnimationFrame(animationId);
    }

    animationId = requestAnimationFrame(() => {
      if (lastMoveEvent) {
        const newX = initialX + lastMoveEvent.clientX - startX;
        const newY = initialY + lastMoveEvent.clientY - startY;

        // 边界限制
        const maxX = window.innerWidth - props.windowState.size.width;
        const maxY = window.innerHeight - props.windowState.size.height - 48; // 48px是任务栏高度

        const constrainedX = Math.max(0, Math.min(newX, maxX));
        const constrainedY = Math.max(0, Math.min(newY, maxY));

        windowStore.updateWindowPosition(props.windowState.id, {
          x: constrainedX,
          y: constrainedY
        });
      }
    });
  };

  const stopDrag = () => {
    isDragging.value = false;
    if (animationId) {
      cancelAnimationFrame(animationId);
    }
    document.removeEventListener('mousemove', doDrag);
    document.removeEventListener('mouseup', stopDrag);

    // 清理
    lastMoveEvent = null;
    animationId = null;
  };

  document.addEventListener('mousemove', doDrag, { passive: true });
  document.addEventListener('mouseup', stopDrag);
};

const startResize = (event: MouseEvent, direction: ResizeDirection) => {
  if (!windowEl.value || props.windowState.isMaximized) return;

  event.preventDefault();
  event.stopPropagation();

  isResizing.value = true;
  resizeDirection.value = direction;
  windowStore.focusWindow(props.windowState.id);

  const startX = event.clientX;
  const startY = event.clientY;
  const initialWidth = props.windowState.size.width;
  const initialHeight = props.windowState.size.height;
  const initialX = props.windowState.position.x;
  const initialY = props.windowState.position.y;

  let animationId: number | null = null;
  let lastMoveEvent: MouseEvent | null = null;

  const doResize = (moveEvent: MouseEvent) => {
    lastMoveEvent = moveEvent;

    if (animationId) {
      cancelAnimationFrame(animationId);
    }

    animationId = requestAnimationFrame(() => {
      if (!lastMoveEvent) return;

      const deltaX = lastMoveEvent.clientX - startX;
      const deltaY = lastMoveEvent.clientY - startY;

      let newWidth = initialWidth;
      let newHeight = initialHeight;
      let newX = initialX;
      let newY = initialY;

      // 最小和最大尺寸限制
      const minWidth = 300;
      const minHeight = 200;
      const maxWidth = window.innerWidth;
      const maxHeight = window.innerHeight - 48; // 48px是任务栏高度

      // 根据拖拽方向调整尺寸和位置
      if (direction.includes('e')) {
        newWidth = Math.max(minWidth, Math.min(maxWidth, initialWidth + deltaX));
      }
      if (direction.includes('w')) {
        const proposedWidth = initialWidth - deltaX;
        if (proposedWidth >= minWidth) {
          newWidth = Math.min(maxWidth, proposedWidth);
          newX = initialX + deltaX;
        }
      }
      if (direction.includes('s')) {
        newHeight = Math.max(minHeight, Math.min(maxHeight, initialHeight + deltaY));
      }
      if (direction.includes('n')) {
        const proposedHeight = initialHeight - deltaY;
        if (proposedHeight >= minHeight) {
          newHeight = Math.min(maxHeight, proposedHeight);
          newY = initialY + deltaY;
        }
      }

      // 确保窗口不会超出屏幕边界
      if (newX < 0) {
        newWidth += newX;
        newX = 0;
      }
      if (newY < 0) {
        newHeight += newY;
        newY = 0;
      }
      if (newX + newWidth > window.innerWidth) {
        newWidth = window.innerWidth - newX;
      }
      if (newY + newHeight > window.innerHeight - 48) {
        newHeight = window.innerHeight - 48 - newY;
      }

      windowStore.updateWindowBounds(props.windowState.id, {
        position: { x: newX, y: newY },
        size: { width: newWidth, height: newHeight }
      });
    });
  };

  const stopResize = () => {
    isResizing.value = false;
    resizeDirection.value = '';
    if (animationId) {
      cancelAnimationFrame(animationId);
    }
    document.removeEventListener('mousemove', doResize);
    document.removeEventListener('mouseup', stopResize);

    lastMoveEvent = null;
    animationId = null;
  };

  document.addEventListener('mousemove', doResize, { passive: true });
  document.addEventListener('mouseup', stopResize);
};

const close = () => {
  windowStore.closeWindow(props.windowState.id);
};

const minimize = () => {
  windowStore.minimizeWindow(props.windowState.id);
};

const maximize = () => {
  windowStore.maximizeWindow(props.windowState.id);
};

const focus = () => {
  windowStore.focusWindow(props.windowState.id);
};

onMounted(() => {
    if (windowEl.value) {
        windowStore.focusWindow(props.windowState.id)
    }
})
</script>

<template>
  <div
    v-show="!windowState.isMinimized"
    ref="windowEl"
    class="app-window"
    :style="{
        left: windowState.position.x + 'px',
        top: windowState.position.y + 'px',
        width: windowState.size.width + 'px',
        height: windowState.size.height + 'px',
        zIndex: windowState.zIndex
    }"
    @mousedown="focus"
    :class="{
      active: windowState.isActive,
      dragging: isDragging,
      resizing: isResizing,
      maximized: windowState.isMaximized
    }"
  >
    <div class="title-bar" @mousedown.prevent.stop="startDrag" @dblclick="maximize">
      <div class="title-container">
        <div class="window-icon">🪟</div>
      <span class="title">{{ windowState.title }}</span>
      </div>
      <div class="controls">
        <button class="control-btn minimize" @click="minimize" title="最小化">
          <svg width="12" height="12" viewBox="0 0 12 12">
            <rect x="2" y="6" width="8" height="1" fill="currentColor"/>
          </svg>
        </button>
        <button class="control-btn maximize" @click="maximize" :title="windowState.isMaximized ? '还原' : '最大化'">
          <svg v-if="!windowState.isMaximized" width="12" height="12" viewBox="0 0 12 12">
            <rect x="2" y="2" width="8" height="8" stroke="currentColor" stroke-width="1" fill="none"/>
          </svg>
          <svg v-else width="12" height="12" viewBox="0 0 12 12">
            <rect x="2" y="3" width="6" height="6" stroke="currentColor" stroke-width="1" fill="none"/>
            <rect x="4" y="1" width="6" height="6" stroke="currentColor" stroke-width="1" fill="none"/>
          </svg>
        </button>
        <button class="control-btn close" @click="close" title="关闭">
          <svg width="12" height="12" viewBox="0 0 12 12">
            <path d="M2.5 2.5l7 7M9.5 2.5l-7 7" stroke="currentColor" stroke-width="1.5" stroke-linecap="round"/>
          </svg>
        </button>
      </div>
    </div>
    <div class="content">
      <slot></slot>
    </div>

    <!-- 调整大小的控制点 -->
    <template v-if="!windowState.isMaximized">
      <!-- 边框调整点 -->
      <div class="resize-handle resize-n" @mousedown="startResize($event, 'n')"></div>
      <div class="resize-handle resize-s" @mousedown="startResize($event, 's')"></div>
      <div class="resize-handle resize-e" @mousedown="startResize($event, 'e')"></div>
      <div class="resize-handle resize-w" @mousedown="startResize($event, 'w')"></div>

      <!-- 角落调整点 -->
      <div class="resize-handle resize-ne" @mousedown="startResize($event, 'ne')"></div>
      <div class="resize-handle resize-nw" @mousedown="startResize($event, 'nw')"></div>
      <div class="resize-handle resize-se" @mousedown="startResize($event, 'se')"></div>
      <div class="resize-handle resize-sw" @mousedown="startResize($event, 'sw')"></div>
    </template>
  </div>
</template>

<style scoped>
.app-window {
  position: absolute;
  border: 1px solid rgba(255, 255, 255, 0.2);
  background: rgba(249, 249, 249, 0.85);
  backdrop-filter: blur(40px) saturate(180%);
  border-radius: 12px;
  box-shadow:
    0 8px 40px rgba(0, 0, 0, 0.15),
    0 4px 16px rgba(0, 0, 0, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.6);
  display: flex;
  flex-direction: column;
  transition: all 0.3s cubic-bezier(0.25, 0.46, 0.45, 0.94);
  overflow: hidden;
  min-width: 300px;
  min-height: 200px;
  will-change: transform; /* 优化GPU加速 */
}

/* 拖动时禁用transition来提高性能 */
.app-window.dragging {
  transition: none !important;
}

/* 拖动时禁用大部分元素的pointer-events，但保留控制按钮 */
.app-window.dragging .content {
  pointer-events: none;
}

.app-window.dragging .title-container {
  pointer-events: none;
}

/* 确保控制按钮在拖动时仍然可用 */
.app-window.dragging .controls,
.app-window.dragging .control-btn {
  pointer-events: auto;
}

/* 确保调整大小控制点始终可用 */
.app-window.dragging .resize-handle,
.app-window.resizing .resize-handle {
  pointer-events: auto;
}

/* 调整大小时禁用其他元素的交互 */
.app-window.resizing .title-bar,
.app-window.resizing .controls {
  pointer-events: none;
}

.app-window.active {
  box-shadow:
    0 16px 60px rgba(0, 0, 0, 0.25),
    0 8px 24px rgba(0, 0, 0, 0.15),
    inset 0 1px 0 rgba(255, 255, 255, 0.7),
    0 0 0 1px rgba(0, 120, 215, 0.3);
  border-color: rgba(0, 120, 215, 0.2);
  transform: translateY(-2px);
}

.title-bar {
  height: 44px;
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(20px);
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 20px 0 16px;
  cursor: grab;
  border-bottom: 1px solid rgba(0, 0, 0, 0.05);
  position: relative;
}

.title-bar:active {
  cursor: grabbing;
}

.app-window.active .title-bar {
  background: rgba(255, 255, 255, 0.85);
  border-bottom-color: rgba(0, 120, 215, 0.1);
}

.title-container {
  display: flex;
  align-items: center;
  gap: 10px;
  flex: 1;
}

.window-icon {
  font-size: 16px;
  opacity: 0.8;
  filter: drop-shadow(0 1px 2px rgba(0, 0, 0, 0.1));
}

.title {
  font-weight: 600;
  color: #323130;
  font-size: 14px;
  letter-spacing: -0.2px;
  text-shadow: 0 1px 2px rgba(255, 255, 255, 0.8);
}

.controls {
  display: flex;
  gap: 0;
}

.control-btn {
  width: 46px;
  height: 32px;
  border: none;
  background: transparent;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #605e5c;
  transition: all 0.2s ease;
  border-radius: 6px;
  position: relative;
}

.control-btn:hover {
  background: rgba(0, 0, 0, 0.06);
  color: #323130;
}

.control-btn:active {
  background: rgba(0, 0, 0, 0.1);
  transform: scale(0.96);
}

.control-btn.minimize:hover {
  background: rgba(0, 120, 215, 0.1);
  color: #0078d4;
}

.control-btn.maximize:hover {
  background: rgba(0, 120, 215, 0.1);
  color: #0078d4;
}

.control-btn.close:hover {
  background: rgba(232, 17, 35, 0.9);
    color: white;
}

.control-btn.close:active {
  background: rgba(196, 43, 28, 1);
}

.content {
  flex-grow: 1;
  padding: 20px;
  overflow: auto;
  color: #323130;
  background: rgba(255, 255, 255, 0.4);
  backdrop-filter: blur(20px);
  position: relative;
}

.content::-webkit-scrollbar {
  width: 8px;
}

.content::-webkit-scrollbar-track {
  background: rgba(0, 0, 0, 0.05);
  border-radius: 4px;
}

.content::-webkit-scrollbar-thumb {
  background: rgba(0, 0, 0, 0.2);
  border-radius: 4px;
  transition: background 0.2s ease;
}

.content::-webkit-scrollbar-thumb:hover {
  background: rgba(0, 0, 0, 0.3);
}

/* 窗口阴影效果 */
.app-window::before {
  content: '';
  position: absolute;
  top: -2px;
  left: -2px;
  right: -2px;
  bottom: -2px;
  background: linear-gradient(135deg,
    rgba(255, 255, 255, 0.3) 0%,
    rgba(255, 255, 255, 0.1) 50%,
    rgba(0, 0, 0, 0.05) 100%);
  border-radius: 14px;
  z-index: -1;
  opacity: 0;
  transition: opacity 0.3s ease;
}

.app-window.active::before {
  opacity: 1;
}

/* 最大化状态样式 */
.app-window.maximized {
  border-radius: 0 !important;
  box-shadow: none !important;
  border: none !important;
}

.app-window.maximized .title-bar {
  border-radius: 0;
}

/* 调整大小控制点样式 */
.resize-handle {
  position: absolute;
  background: transparent;
  z-index: 10;
}

/* 边框调整点 */
.resize-n {
  top: -3px;
  left: 8px;
  right: 8px;
  height: 6px;
  cursor: ns-resize;
}

.resize-s {
  bottom: -3px;
  left: 8px;
  right: 8px;
  height: 6px;
  cursor: ns-resize;
}

.resize-e {
  right: -3px;
  top: 8px;
  bottom: 8px;
  width: 6px;
  cursor: ew-resize;
}

.resize-w {
  left: -3px;
  top: 8px;
  bottom: 8px;
  width: 6px;
  cursor: ew-resize;
}

/* 角落调整点 */
.resize-ne {
  top: -3px;
  right: -3px;
  width: 12px;
  height: 12px;
  cursor: nesw-resize;
}

.resize-nw {
  top: -3px;
  left: -3px;
  width: 12px;
  height: 12px;
  cursor: nwse-resize;
}

.resize-se {
  bottom: -3px;
  right: -3px;
  width: 12px;
  height: 12px;
  cursor: nwse-resize;
}

.resize-sw {
  bottom: -3px;
  left: -3px;
  width: 12px;
  height: 12px;
  cursor: nesw-resize;
}

/* 调整大小时的样式 */
.app-window.resizing {
  transition: none !important;
  user-select: none;
  border: 2px solid rgba(0, 120, 215, 0.5);
  box-shadow:
    0 0 0 1px rgba(0, 120, 215, 0.3),
    0 8px 32px rgba(0, 120, 215, 0.2);
}

.app-window.resizing .content {
  pointer-events: none;
}

/* 悬停时显示调整指示器 */
.resize-handle:hover {
  background: rgba(0, 120, 215, 0.3);
  border-radius: 2px;
}

/* 角落指示器 */
.resize-ne:hover,
.resize-nw:hover,
.resize-se:hover,
.resize-sw:hover {
  background: rgba(0, 120, 215, 0.3);
  border-radius: 50%;
}
</style>
