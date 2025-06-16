<template>
  <div class="message-dialog-overlay" v-if="visible" @click="handleOverlayClick">
    <div class="message-dialog" :class="dialogClass" @click.stop>
      <!-- 标题栏 -->
      <div class="dialog-header">
        <div class="dialog-icon">
          <span v-if="type === 'info'">ℹ️</span>
          <span v-else-if="type === 'success'">✅</span>
          <span v-else-if="type === 'warning'">⚠️</span>
          <span v-else-if="type === 'error'">❌</span>
          <span v-else-if="type === 'question'">❓</span>
        </div>
        <div class="dialog-title">{{ title }}</div>
        <button class="dialog-close" @click="close" v-if="!isAlert">
          <span>✕</span>
        </button>
      </div>

      <!-- 内容区域 -->
      <div class="dialog-content">
        <div class="dialog-message">{{ message }}</div>
        <div class="dialog-details" v-if="details">{{ details }}</div>
      </div>

      <!-- 按钮区域 -->
      <div class="dialog-buttons">
        <button
          v-if="type === 'question'"
          class="dialog-button primary"
          @click="handleConfirm"
          ref="confirmButton"
        >
          确定
        </button>
        <button
          v-if="type === 'question'"
          class="dialog-button secondary"
          @click="handleCancel"
        >
          取消
        </button>
        <button
          v-if="type !== 'question'"
          class="dialog-button primary"
          @click="close"
          ref="okButton"
        >
          确定
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, nextTick } from 'vue';

const props = defineProps<{
  visible: boolean;
  type: 'info' | 'success' | 'warning' | 'error' | 'question';
  title: string;
  message: string;
  details?: string;
  isAlert?: boolean;
}>();

const emit = defineEmits<{
  close: [];
  confirm: [];
  cancel: [];
}>();

const confirmButton = ref<HTMLButtonElement>();
const okButton = ref<HTMLButtonElement>();

const dialogClass = computed(() => ({
  [`dialog-${props.type}`]: true
}));

const handleOverlayClick = () => {
  if (!props.isAlert) {
    close();
  }
};

const handleConfirm = () => {
  emit('confirm');
  emit('close');
};

const handleCancel = () => {
  emit('cancel');
  emit('close');
};

const close = () => {
  emit('close');
};

onMounted(() => {
  nextTick(() => {
    // 自动聚焦到主要按钮
    if (props.type === 'question' && confirmButton.value) {
      confirmButton.value.focus();
    } else if (okButton.value) {
      okButton.value.focus();
    }
  });
});
</script>

<style scoped>
.message-dialog-overlay {
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

.message-dialog {
  background: #ffffff;
  border-radius: 8px;
  box-shadow:
    0 8px 32px rgba(0, 0, 0, 0.12),
    0 2px 8px rgba(0, 0, 0, 0.08);
  min-width: 400px;
  max-width: 600px;
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
  background: linear-gradient(135deg, #f8f9fa 0%, #ffffff 100%);
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

.dialog-content {
  padding: 20px 24px;
}

.dialog-message {
  font-size: 14px;
  line-height: 1.5;
  color: #323130;
  margin-bottom: 8px;
}

.dialog-details {
  font-size: 12px;
  line-height: 1.4;
  color: #605e5c;
  background: #f8f9fa;
  padding: 12px;
  border-radius: 4px;
  border-left: 3px solid #e1e1e1;
  white-space: pre-wrap;
  font-family: 'Consolas', 'Monaco', monospace;
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

.dialog-button.secondary {
  background: #ffffff;
  color: #323130;
  border-color: #d1d1d1;
}

.dialog-button.secondary:hover {
  background: #f3f2f1;
  border-color: #c8c6c4;
  transform: translateY(-1px);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.dialog-button.secondary:active {
  background: #edebe9;
  transform: translateY(0);
}

/* 不同类型的对话框样式 */
.dialog-info .dialog-header {
  background: linear-gradient(135deg, #e6f3ff 0%, #ffffff 100%);
}

.dialog-success .dialog-header {
  background: linear-gradient(135deg, #e6f7e6 0%, #ffffff 100%);
}

.dialog-warning .dialog-header {
  background: linear-gradient(135deg, #fff8e1 0%, #ffffff 100%);
}

.dialog-error .dialog-header {
  background: linear-gradient(135deg, #ffebee 0%, #ffffff 100%);
}

.dialog-question .dialog-header {
  background: linear-gradient(135deg, #f3e5f5 0%, #ffffff 100%);
}
</style>

