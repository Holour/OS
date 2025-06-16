<template>
  <div class="input-dialog-overlay" v-if="visible" @click="handleOverlayClick">
    <div class="input-dialog" @click.stop>
      <!-- 标题栏 -->
      <div class="dialog-header">
        <div class="dialog-icon">
          <span>✏️</span>
        </div>
        <div class="dialog-title">{{ title }}</div>
        <button class="dialog-close" @click="close">
          <span>✕</span>
        </button>
      </div>

      <!-- 内容区域 -->
      <div class="dialog-content">
        <div class="dialog-message" v-if="message">{{ message }}</div>

        <!-- 单个输入框 -->
        <div v-if="!multipleInputs" class="input-group">
          <label class="input-label" v-if="inputLabel">{{ inputLabel }}</label>
          <input
            ref="inputElement"
            v-model="inputValue"
            :type="inputType"
            :placeholder="placeholder"
            class="input-field"
            @keyup.enter="handleConfirm"
            @keyup.escape="close"
          />
          <div class="input-hint" v-if="hint">{{ hint }}</div>
        </div>

        <!-- 多个输入框 -->
        <div v-else class="multiple-inputs">
          <div v-for="(input, index) in inputs" :key="index" class="input-group">
            <label class="input-label" v-if="input.label">{{ input.label }}</label>
            <input
              :ref="index === 0 ? 'inputElement' : undefined"
              v-model="input.value"
              :type="input.type || 'text'"
              :placeholder="input.placeholder"
              class="input-field"
              @keyup.enter="handleConfirm"
              @keyup.escape="close"
            />
            <div class="input-hint" v-if="input.hint">{{ input.hint }}</div>
          </div>
        </div>
      </div>

      <!-- 按钮区域 -->
      <div class="dialog-buttons">
        <button
          class="dialog-button primary"
          @click="handleConfirm"
          :disabled="!isValid"
        >
          确定
        </button>
        <button
          class="dialog-button secondary"
          @click="close"
        >
          取消
        </button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, nextTick, reactive } from 'vue';

interface InputItem {
  label?: string;
  value: string;
  type?: string;
  placeholder?: string;
  hint?: string;
  required?: boolean;
}

const props = defineProps<{
  visible: boolean;
  title: string;
  message?: string;
  inputLabel?: string;
  placeholder?: string;
  defaultValue?: string;
  inputType?: string;
  hint?: string;
  multipleInputs?: boolean;
  inputsConfig?: InputItem[];
  validator?: (value: string | InputItem[]) => boolean;
}>();

const emit = defineEmits<{
  close: [];
  confirm: [value: string | InputItem[]];
}>();

const inputElement = ref<HTMLInputElement>();
const inputValue = ref(props.defaultValue || '');

// 多输入框模式
const inputs = reactive<InputItem[]>(
  props.inputsConfig ? [...props.inputsConfig] : []
);

const isValid = computed(() => {
  if (props.validator) {
    return props.validator(props.multipleInputs ? inputs : inputValue.value);
  }

  if (props.multipleInputs) {
    return inputs.every(input =>
      !input.required || (input.value && input.value.trim().length > 0)
    );
  }

  return inputValue.value.trim().length > 0;
});

const handleOverlayClick = () => {
  close();
};

const handleConfirm = () => {
  if (!isValid.value) return;

  const result = props.multipleInputs ? inputs : inputValue.value;
  emit('confirm', result);
  close();
};

const close = () => {
  emit('close');
};

onMounted(() => {
  nextTick(() => {
    if (inputElement.value) {
      inputElement.value.focus();
      inputElement.value.select();
    }
  });
});
</script>

<style scoped>
.input-dialog-overlay {
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

.input-dialog {
  background: #ffffff;
  border-radius: 8px;
  box-shadow:
    0 8px 32px rgba(0, 0, 0, 0.12),
    0 2px 8px rgba(0, 0, 0, 0.08);
  min-width: 450px;
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
  background: linear-gradient(135deg, #f0f8ff 0%, #ffffff 100%);
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
  margin-bottom: 16px;
}

.input-group {
  margin-bottom: 16px;
}

.input-group:last-child {
  margin-bottom: 0;
}

.input-label {
  display: block;
  font-size: 13px;
  font-weight: 500;
  color: #323130;
  margin-bottom: 6px;
}

.input-field {
  width: 100%;
  padding: 10px 12px;
  border: 1px solid #d1d1d1;
  border-radius: 4px;
  font-size: 14px;
  background: #ffffff;
  transition: all 0.2s ease;
  box-sizing: border-box;
}

.input-field:focus {
  outline: none;
  border-color: #0078d4;
  box-shadow: 0 0 0 2px rgba(0, 120, 212, 0.2);
}

.input-field:hover {
  border-color: #c8c6c4;
}

.input-hint {
  font-size: 12px;
  color: #605e5c;
  margin-top: 4px;
  line-height: 1.3;
}

.multiple-inputs {
  display: flex;
  flex-direction: column;
  gap: 16px;
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

.dialog-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.dialog-button.primary {
  background: #0078d4;
  color: white;
  border-color: #0078d4;
}

.dialog-button.primary:hover:not(:disabled) {
  background: #106ebe;
  border-color: #106ebe;
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(0, 120, 212, 0.3);
}

.dialog-button.primary:active:not(:disabled) {
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
</style>
