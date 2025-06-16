<script setup lang="ts">
import { ref, onMounted } from 'vue';
import TheDesktop from './components/TheDesktop.vue';
import TheTaskbar from './components/TheTaskbar.vue';
import BootLoader from './components/BootLoader.vue';
import MessageDialog from './components/dialogs/MessageDialog.vue';
import InputDialog from './components/dialogs/InputDialog.vue';
import PropertiesDialog from './components/dialogs/PropertiesDialog.vue';
import { useDialogs } from './composables/useDialogs';
import { HealthCheckService } from './services/healthCheck';

const {
  messageDialog,
  inputDialog,
  propertiesDialog,
  closeMessageDialog,
  confirmMessageDialog,
  closeInputDialog,
  confirmInputDialog,
  closePropertiesDialog
} = useDialogs();

// 启动状态管理
const isBooting = ref(true);
const currentStage = ref<'initializing' | 'checking-backend' | 'backend-error' | 'starting'>('initializing');
const progress = ref(0);
const backendUrl = ref('http://localhost:8080');
const errorMessage = ref('');
const connectionStatus = ref('');
const isRetrying = ref(false);
const retryCount = ref(0);
const maxRetries = ref(5);
const retryDelay = ref(3000); // 3秒后重试

// 健康检查服务实例
const healthCheck = HealthCheckService.getInstance(backendUrl.value);

// 后端检查函数
const checkBackend = async (): Promise<boolean> => {
  try {
    currentStage.value = 'checking-backend';
    progress.value = 30;

    // 使用健康检查服务
    const healthResult = await healthCheck.checkHealth(8000); // 8秒超时
    progress.value = 60;

    if (healthResult.status === 'ok') {
      connectionStatus.value = 'Connected';
      progress.value = 80;

      // 检查各个服务状态
      const allServicesUp = Object.values(healthResult.services).every(service => service);
      if (!allServicesUp) {
        errorMessage.value = '部分后端服务不可用';
        return false;
      }

      return true;
    } else {
      connectionStatus.value = 'Disconnected';
      errorMessage.value = '后端服务健康检查失败';
      return false;
    }
  } catch (error: any) {
    connectionStatus.value = 'Disconnected';
    errorMessage.value = error.message || '连接后端服务时发生未知错误';
    return false;
  }
};

// 启动桌面
const startDesktop = async () => {
  currentStage.value = 'starting';
  progress.value = 90;

  // 模拟启动过程
  await new Promise(resolve => setTimeout(resolve, 1000));

  progress.value = 100;

  // 延迟一下再隐藏启动画面，让用户看到100%
  setTimeout(() => {
    isBooting.value = false;
  }, 500);
};

// 自动重试连接
const autoRetryConnection = async () => {
  if (retryCount.value >= maxRetries.value) {
    currentStage.value = 'backend-error';
    errorMessage.value = `连接失败，已尝试 ${maxRetries.value} 次，请检查后端服务`;
    return;
  }

  retryCount.value++;
  isRetrying.value = true;

  // 显示重试倒计时
  currentStage.value = 'checking-backend';
  errorMessage.value = `正在进行第 ${retryCount.value} 次连接尝试...`;

  try {
    const backendAvailable = await checkBackend();

    if (backendAvailable) {
      // 连接成功，重置重试计数
      retryCount.value = 0;
      await startDesktop();
    } else {
      // 连接失败，等待后自动重试
      await waitForRetry();
    }
  } catch (error) {
    console.error('自动重试失败:', error);
    await waitForRetry();
  } finally {
    isRetrying.value = false;
  }
};

// 等待重试
const waitForRetry = async () => {
  currentStage.value = 'backend-error';

  if (retryCount.value < maxRetries.value) {
    // 倒计时重试
    for (let i = retryDelay.value / 1000; i > 0; i--) {
      errorMessage.value = `连接失败，${i} 秒后自动重试...`;
      await new Promise(resolve => setTimeout(resolve, 1000));
    }

    // 继续自动重试
    setTimeout(() => {
      autoRetryConnection();
    }, 100);
  }
};

// 手动重试连接（保留用于用户主动重试）
const retryConnection = async () => {
  retryCount.value = 0; // 重置重试计数
  progress.value = 0;
  errorMessage.value = '';
  connectionStatus.value = '';
  await autoRetryConnection();
};

// 系统初始化
const initializeSystem = async () => {
  try {
    // 初始化阶段
    currentStage.value = 'initializing';
    progress.value = 10;

    // 模拟初始化延迟
    await new Promise(resolve => setTimeout(resolve, 1200));
    progress.value = 20;

    // 开始自动连接尝试
    await autoRetryConnection();
  } catch (error: any) {
    console.error('系统初始化失败:', error);
    currentStage.value = 'backend-error';
    errorMessage.value = error.message || '系统初始化失败';
    progress.value = 0;
  }
};

// 组件挂载时启动系统
onMounted(() => {
  initializeSystem();
});
</script>

<template>
  <div class="app-container">
    <!-- 启动加载器 -->
    <BootLoader
      v-if="isBooting"
      :current-stage="currentStage"
      :progress="progress"
      :backend-url="backendUrl"
      :error-message="errorMessage"
      :connection-status="connectionStatus"
      :is-retrying="isRetrying"
      @retry="retryConnection"
    />

    <!-- 主桌面界面 -->
    <template v-else>
      <TheDesktop />
      <TheTaskbar />
    </template>

    <!-- 对话框组件 -->
    <MessageDialog
      :visible="messageDialog.visible"
      :type="messageDialog.type"
      :title="messageDialog.title"
      :message="messageDialog.message"
      :details="messageDialog.details"
      @close="closeMessageDialog"
      @confirm="confirmMessageDialog"
      @cancel="closeMessageDialog"
    />

    <InputDialog
      :visible="inputDialog.visible"
      :title="inputDialog.title"
      :message="inputDialog.message"
      :input-label="inputDialog.inputLabel"
      :placeholder="inputDialog.placeholder"
      :default-value="inputDialog.defaultValue"
      :input-type="inputDialog.inputType"
      :hint="inputDialog.hint"
      :multiple-inputs="inputDialog.multipleInputs"
      :inputs-config="inputDialog.inputsConfig"
      :validator="inputDialog.validator"
      @close="closeInputDialog"
      @confirm="confirmInputDialog"
    />

    <PropertiesDialog
      :visible="propertiesDialog.visible"
      :file="propertiesDialog.file!"
      @close="closePropertiesDialog"
    />
  </div>
</template>

<style scoped>
.app-container {
  width: 100vw;
  height: 100vh;
  background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
  overflow: hidden;
  display: flex;
  flex-direction: column;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  user-select: none;
}

/* 全局滚动条样式 */
:global(::-webkit-scrollbar) {
  width: 8px;
  height: 8px;
}

:global(::-webkit-scrollbar-track) {
  background: rgba(0, 0, 0, 0.05);
  border-radius: 4px;
}

:global(::-webkit-scrollbar-thumb) {
  background: rgba(0, 0, 0, 0.2);
  border-radius: 4px;
  transition: background 0.2s ease;
}

:global(::-webkit-scrollbar-thumb:hover) {
  background: rgba(0, 0, 0, 0.3);
}

/* 全局选择样式 */
:global(::selection) {
  background: rgba(0, 120, 215, 0.3);
  color: inherit;
}
</style>
