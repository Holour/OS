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

const startDrag = (event: MouseEvent) => {
  if (!windowEl.value) return;
  windowStore.focusWindow(props.windowState.id);

  const startX = event.clientX;
  const startY = event.clientY;
  const initialX = props.windowState.position.x;
  const initialY = props.windowState.position.y;

  const doDrag = (moveEvent: MouseEvent) => {
    const newX = initialX + moveEvent.clientX - startX;
    const newY = initialY + moveEvent.clientY - startY;
    windowStore.updateWindowPosition(props.windowState.id, { x: newX, y: newY });
  };

  const stopDrag = () => {
    document.removeEventListener('mousemove', doDrag);
    document.removeEventListener('mouseup', stopDrag);
  };

  document.addEventListener('mousemove', doDrag);
  document.addEventListener('mouseup', stopDrag);
};

const close = () => {
  windowStore.closeWindow(props.windowState.id);
};

const minimize = () => {
  // 最小化功能待实现
  console.log('Minimize window:', props.windowState.id);
};

const maximize = () => {
  // 最大化功能待实现
  console.log('Maximize window:', props.windowState.id);
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
    :class="{ active: windowState.isActive }"
  >
    <div class="title-bar" @mousedown.prevent.stop="startDrag">
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
        <button class="control-btn maximize" @click="maximize" title="最大化">
          <svg width="12" height="12" viewBox="0 0 12 12">
            <rect x="2" y="2" width="8" height="8" stroke="currentColor" stroke-width="1" fill="none"/>
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
</style>
