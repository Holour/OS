<script setup lang="ts">
import { ref, onMounted } from 'vue';
import type { WindowState } from '@/stores/windows';
import { useWindowsStore } from '@/stores/windows';

const props = defineProps<{
  windowState: WindowState;
}>();

const windowStore = useWindowsStore();
const windowEl = ref<HTMLElement | null>(null);

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
      <span class="title">{{ windowState.title }}</span>
      <div class="controls">
        <button class="control-btn minimize">-</button>
        <button class="control-btn maximize">[]</button>
        <button class="control-btn close" @click="close">X</button>
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
  border: 1px solid #ccc;
  background-color: white;
  box-shadow: 0 0 10px rgba(0, 0, 0, 0.5);
  display: flex;
  flex-direction: column;
  transition: box-shadow 0.2s;
}

.app-window.active {
    box-shadow: 0 0 20px rgba(0,0,0,0.8);
    border-color: #aaa;
}

.title-bar {
  height: 30px;
  background-color: #f0f0f0;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 10px;
  cursor: grab;
  border-bottom: 1px solid #ccc;
}

.app-window.active .title-bar {
    background-color: #e0e0e0;
}

.title {
  font-weight: bold;
  color: #333;
  font-size: 13px;
}

.controls {
  display: flex;
}

.control-btn {
  width: 25px;
  height: 25px;
  margin-left: 5px;
  border: 1px solid #ccc;
  background-color: #e0e0e0;
  cursor: pointer;
}
.close:hover {
    background-color: #e81123;
    color: white;
}

.content {
  flex-grow: 1;
  padding: 10px;
  overflow: auto;
  color: #333;
  background-color: white;
}
</style>
