<template>
  <div class="boot-loader">
    <div class="boot-container">
      <!-- 启动Logo -->
      <div class="boot-logo">
        <div class="logo-circle">
          <div class="logo-inner">
            <span class="logo-text">OS</span>
          </div>
        </div>
      </div>

      <!-- 系统信息 -->
      <div class="system-info">
        <h1 class="system-title">操作系统模拟器</h1>
        <p class="system-version">Version 2.1.0</p>
      </div>

      <!-- 启动状态 -->
      <div class="boot-status">
        <div v-if="currentStage === 'initializing'" class="status-stage">
          <div class="spinner"></div>
          <p>正在初始化系统...</p>
        </div>

        <div v-else-if="currentStage === 'checking-backend'" class="status-stage">
          <div class="spinner"></div>
          <p>正在连接后端服务...</p>
          <small>{{ backendUrl }}</small>
        </div>

                          <div v-else-if="currentStage === 'backend-error'" class="status-stage error">
           <div class="error-icon pulse">⚠️</div>
           <p>{{ errorMessage.includes('秒后') ? '正在自动重试连接...' : '无法连接到后端服务' }}</p>
           <small class="error-message">{{ errorMessage }}</small>

           <!-- 自动重试提示 -->
           <div v-if="errorMessage.includes('秒后')" class="auto-retry-indicator">
             <div class="retry-spinner"></div>
             <span class="retry-text">系统将自动重试连接</span>
           </div>

           <!-- 手动操作区域 -->
           <div v-else class="error-actions">
             <button @click="retryConnection" class="retry-btn" :disabled="isRetrying" :class="{ 'retrying': isRetrying }">
               <span class="btn-icon" :class="{ 'spinning': isRetrying }">🔄</span>
               {{ isRetrying ? '正在重试...' : '立即重试' }}
             </button>
             <button @click="showDetails = !showDetails" class="details-btn">
               <span class="btn-icon">{{ showDetails ? '📄' : 'ℹ️' }}</span>
               {{ showDetails ? '隐藏详情' : '查看详情' }}
             </button>
           </div>

           <div v-if="showDetails" class="error-details">
             <h4>🔍 诊断信息</h4>
             <div class="diagnostic-grid">
               <div class="diagnostic-item">
                 <span class="label">后端地址:</span>
                 <span class="value">{{ backendUrl }}</span>
               </div>
               <div class="diagnostic-item">
                 <span class="label">连接状态:</span>
                 <span class="value" :class="connectionStatus.toLowerCase()">{{ connectionStatus }}</span>
               </div>
               <div class="diagnostic-item">
                 <span class="label">错误信息:</span>
                 <span class="value error-text">{{ errorMessage }}</span>
               </div>
             </div>

             <div class="troubleshooting">
               <h5>💡 故障排除建议</h5>
               <ul>
                 <li>确认后端服务已启动</li>
                 <li>检查端口 8080 是否被占用</li>
                 <li>验证防火墙设置</li>
                 <li>检查网络连接状态</li>
               </ul>
             </div>
           </div>
         </div>

        <div v-else-if="currentStage === 'starting'" class="status-stage">
          <div class="success-icon">✅</div>
          <p>后端连接成功，正在启动桌面...</p>
        </div>
      </div>

      <!-- 进度条 -->
      <div class="progress-container">
        <div class="progress-bar">
          <div class="progress-fill" :style="{ width: progress + '%' }"></div>
        </div>
        <span class="progress-text">{{ Math.round(progress) }}%</span>
      </div>
    </div>

    <!-- 背景粒子效果 -->
    <div class="particles">
      <div v-for="i in 60" :key="i" class="particle"
           :style="{
             left: Math.random() * 100 + '%',
             top: Math.random() * 100 + '%',
             animationDelay: Math.random() * 5 + 's',
             animationDuration: (Math.random() * 8 + 6) + 's'
           }"></div>
    </div>

    <!-- 环形装饰元素 -->
    <div class="decorative-rings">
      <div class="ring ring-1"></div>
      <div class="ring ring-2"></div>
      <div class="ring ring-3"></div>
    </div>

    <!-- 光晕效果 -->
    <div class="ambient-glow"></div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, withDefaults } from 'vue';

interface Props {
  currentStage: 'initializing' | 'checking-backend' | 'backend-error' | 'starting';
  progress: number;
  backendUrl: string;
  errorMessage: string;
  connectionStatus: string;
  isRetrying?: boolean;
}

const props = withDefaults(defineProps<Props>(), {
  isRetrying: false
});

const emit = defineEmits<{
  retry: []
}>();

const showDetails = ref(false);

const retryConnection = () => {
  if (props.isRetrying) return;  // 防止重复点击
  showDetails.value = false;
  emit('retry');
};

onMounted(() => {
  // 启动时的淡入动画
  const container = document.querySelector('.boot-container') as HTMLElement;
  if (container) {
    container.style.opacity = '0';
    container.style.transform = 'translateY(30px) scale(0.95)';

    setTimeout(() => {
      container.style.transition = 'all 1s cubic-bezier(0.4, 0, 0.2, 1)';
      container.style.opacity = '1';
      container.style.transform = 'translateY(0) scale(1)';
    }, 150);
  }

  // 为状态阶段添加动态入场动画
  setTimeout(() => {
    const statusStage = document.querySelector('.status-stage') as HTMLElement;
    if (statusStage) {
      statusStage.style.opacity = '1';
    }
  }, 800);
});
</script>

<style scoped>
.boot-loader {
  position: fixed;
  top: 0;
  left: 0;
  width: 100vw;
  height: 100vh;
  background:
    radial-gradient(circle at 20% 80%, rgba(120, 119, 198, 0.3) 0%, transparent 50%),
    radial-gradient(circle at 80% 20%, rgba(255, 119, 198, 0.3) 0%, transparent 50%),
    radial-gradient(circle at 40% 40%, rgba(120, 219, 255, 0.2) 0%, transparent 50%),
    linear-gradient(135deg, #0a0e1a 0%, #1a1f2e 25%, #2d1b3d 50%, #1e2a3a 75%, #0f1419 100%);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 9999;
  overflow: hidden;
  position: relative;
}

.boot-loader::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background:
    repeating-linear-gradient(
      0deg,
      transparent,
      transparent 2px,
      rgba(66, 153, 225, 0.03) 2px,
      rgba(66, 153, 225, 0.03) 4px
    ),
    repeating-linear-gradient(
      90deg,
      transparent,
      transparent 2px,
      rgba(66, 153, 225, 0.03) 2px,
      rgba(66, 153, 225, 0.03) 4px
    );
  animation: gridMove 20s linear infinite;
}

@keyframes gridMove {
  0% { transform: translate(0, 0); }
  100% { transform: translate(4px, 4px); }
}

.boot-container {
  text-align: center;
  color: white;
  max-width: 520px;
  padding: 50px 45px;
  position: relative;
  z-index: 2;
  background: rgba(255, 255, 255, 0.05);
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 24px;
  box-shadow:
    0 20px 40px rgba(0, 0, 0, 0.3),
    0 8px 24px rgba(0, 0, 0, 0.2),
    inset 0 1px 0 rgba(255, 255, 255, 0.1);
  animation: containerFloat 8s ease-in-out infinite;
}

.boot-container::before {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: linear-gradient(135deg,
    rgba(255, 255, 255, 0.1) 0%,
    rgba(255, 255, 255, 0.05) 50%,
    rgba(255, 255, 255, 0.1) 100%);
  border-radius: 24px;
  opacity: 0;
  animation: glassShimmer 4s ease-in-out infinite;
}

@keyframes containerFloat {
  0%, 100% { transform: translateY(0px) rotate(0deg); }
  33% { transform: translateY(-4px) rotate(0.5deg); }
  66% { transform: translateY(2px) rotate(-0.3deg); }
}

@keyframes glassShimmer {
  0%, 100% { opacity: 0; }
  50% { opacity: 1; }
}

/* Logo样式 */
.boot-logo {
  margin-bottom: 35px;
  animation: slideInUp 0.8s ease 0.1s both;
}

.logo-circle {
  width: 140px;
  height: 140px;
  margin: 0 auto;
  border: none;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  background:
    conic-gradient(from 0deg,
      #4299e1 0deg,
      #63b3ed 60deg,
      #81c5f7 120deg,
      #9face6 180deg,
      #b794f6 240deg,
      #4299e1 360deg);
  animation: logoRotate 4s ease-in-out infinite, logoFloat 6s ease-in-out infinite;
  box-shadow:
    0 0 40px rgba(66, 153, 225, 0.4),
    0 0 80px rgba(66, 153, 225, 0.2),
    inset 0 0 20px rgba(255, 255, 255, 0.1);
}

.logo-circle::before {
  content: '';
  position: absolute;
  width: 150px;
  height: 150px;
  border: 2px solid transparent;
  border-radius: 50%;
  background:
    conic-gradient(from 90deg,
      transparent 0deg,
      rgba(66, 153, 225, 0.8) 45deg,
      transparent 90deg,
      rgba(99, 179, 237, 0.6) 180deg,
      transparent 270deg,
      rgba(129, 197, 247, 0.4) 315deg,
      transparent 360deg);
  animation: logoSpin 3s linear infinite;
  filter: blur(1px);
}

.logo-circle::after {
  content: '';
  position: absolute;
  width: 120px;
  height: 120px;
  border-radius: 50%;
  background:
    radial-gradient(circle at 30% 30%,
      rgba(255, 255, 255, 0.3) 0%,
      rgba(255, 255, 255, 0.1) 40%,
      transparent 70%);
  animation: logoGlow 5s ease-in-out infinite;
}

.logo-inner {
  width: 90px;
  height: 90px;
  background:
    radial-gradient(circle at 30% 30%, #ffffff 0%, #f0f8ff 20%, #4299e1 70%, #2d3748 100%);
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow:
    0 0 30px rgba(66, 153, 225, 0.6),
    inset 0 4px 8px rgba(255, 255, 255, 0.3),
    inset 0 -4px 8px rgba(0, 0, 0, 0.2);
  position: relative;
  overflow: hidden;
  z-index: 2;
}

.logo-inner::before {
  content: '';
  position: absolute;
  top: -50%;
  left: -50%;
  width: 200%;
  height: 200%;
  background: conic-gradient(from 0deg, transparent 0deg, rgba(255, 255, 255, 0.4) 45deg, transparent 90deg);
  animation: innerSpin 4s linear infinite;
}

.logo-text {
  font-size: 32px;
  font-weight: 700;
  color: #1a202c;
  text-shadow:
    0 0 15px rgba(255, 255, 255, 0.8),
    0 2px 4px rgba(0, 0, 0, 0.3);
  position: relative;
  z-index: 3;
  background: linear-gradient(45deg, #1a202c 0%, #2d3748 50%, #4a5568 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

@keyframes logoRotate {
  0%, 100% { transform: rotate(0deg) scale(1); }
  25% { transform: rotate(3deg) scale(1.02); }
  50% { transform: rotate(0deg) scale(1.05); }
  75% { transform: rotate(-3deg) scale(1.02); }
}

@keyframes logoFloat {
  0%, 100% { transform: translateY(0px); }
  33% { transform: translateY(-12px); }
  66% { transform: translateY(4px); }
}

@keyframes logoGlow {
  0%, 100% {
    opacity: 0.8;
    transform: scale(1) rotate(0deg);
  }
  50% {
    opacity: 1;
    transform: scale(1.1) rotate(180deg);
  }
}

@keyframes innerSpin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

@keyframes logoSpin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

/* 系统信息 */
.system-info {
  margin-bottom: 45px;
  animation: slideInUp 0.8s ease 0.2s both;
}

.system-title {
  font-size: 42px;
  font-weight: 100;
  margin: 0 0 15px 0;
  background: linear-gradient(135deg,
    #ffffff 0%,
    #f7fafc 20%,
    #63b3ed 40%,
    #4299e1 60%,
    #3182ce 80%,
    #ffffff 100%);
  background-size: 300% 300%;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  animation: titleGradient 4s ease-in-out infinite, titleFloat 6s ease-in-out infinite;
  filter: drop-shadow(0 0 20px rgba(66, 153, 225, 0.4));
  letter-spacing: 2px;
  position: relative;
}

.system-title::before {
  content: '操作系统模拟器';
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: linear-gradient(135deg,
    rgba(66, 153, 225, 0.3) 0%,
    rgba(129, 197, 247, 0.2) 50%,
    rgba(66, 153, 225, 0.3) 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  filter: blur(2px);
  animation: titleGlow 3s ease-in-out infinite;
  z-index: -1;
}

.system-version {
  font-size: 18px;
  opacity: 0.9;
  margin: 0;
  color: #e2e8f0;
  animation: fadeIn 1s ease 0.8s both, versionPulse 4s ease-in-out infinite 2s;
  font-weight: 300;
  letter-spacing: 1px;
  background: linear-gradient(90deg, #a0aec0 0%, #cbd5e0 50%, #a0aec0 100%);
  background-size: 200% 100%;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

@keyframes titleGradient {
  0%, 100% { background-position: 0% 0%; }
  25% { background-position: 100% 100%; }
  50% { background-position: 200% 0%; }
  75% { background-position: 100% 100%; }
}

@keyframes titleFloat {
  0%, 100% { transform: translateY(0px); }
  50% { transform: translateY(-3px); }
}

@keyframes titleGlow {
  0%, 100% { opacity: 0.3; }
  50% { opacity: 0.7; }
}

@keyframes versionPulse {
  0%, 100% {
    opacity: 0.9;
    background-position: 0% 50%;
  }
  50% {
    opacity: 1;
    background-position: 100% 50%;
  }
}

@keyframes fadeIn {
  from { opacity: 0; }
  to { opacity: 0.9; }
}

@keyframes textFlow {
  0%, 100% { background-position: 0% 50%; }
  50% { background-position: 100% 50%; }
}

@keyframes textPulse {
  0%, 100% { opacity: 0.8; }
  50% { opacity: 1; }
}

/* 启动状态 */
.status-stage {
  margin-bottom: 35px;
  min-height: 130px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  animation: slideInUp 0.6s ease 0.4s both;
  opacity: 0;
}

.status-stage p {
  font-size: 20px;
  margin: 15px 0 8px 0;
  font-weight: 300;
  letter-spacing: 0.5px;
  background: linear-gradient(90deg, #e2e8f0 0%, #ffffff 50%, #e2e8f0 100%);
  background-size: 200% 100%;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  animation: textFlow 3s ease-in-out infinite;
}

.status-stage small {
  font-size: 15px;
  opacity: 0.8;
  margin: 0;
  color: #a0aec0;
  font-weight: 300;
  animation: textPulse 2s ease-in-out infinite;
}

/* 加载动画 */
.spinner {
  width: 50px;
  height: 50px;
  border: 4px solid transparent;
  border-radius: 50%;
  background:
    conic-gradient(from 0deg,
      #4299e1 0deg,
      #63b3ed 90deg,
      #81c5f7 180deg,
      #9face6 270deg,
      #4299e1 360deg);
  animation: spin 1.5s linear infinite, spinGlow 3s ease-in-out infinite;
  margin-bottom: 20px;
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
}

.spinner::before {
  content: '';
  position: absolute;
  width: 36px;
  height: 36px;
  background:
    radial-gradient(circle,
      rgba(15, 20, 25, 0.9) 0%,
      rgba(15, 20, 25, 0.8) 70%,
      transparent 100%);
  border-radius: 50%;
}

.spinner::after {
  content: '';
  position: absolute;
  width: 60px;
  height: 60px;
  border: 2px solid rgba(66, 153, 225, 0.2);
  border-radius: 50%;
  animation: spin 2s linear infinite reverse;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

@keyframes spinGlow {
  0%, 100% {
    box-shadow: 0 0 20px rgba(66, 153, 225, 0.4);
    filter: brightness(1);
  }
  50% {
    box-shadow: 0 0 40px rgba(66, 153, 225, 0.8);
    filter: brightness(1.2);
  }
}

/* 成功和错误图标 */
.success-icon, .error-icon {
  font-size: 40px;
  margin-bottom: 15px;
  animation: bounceIn 0.6s ease;
}

.error-icon.pulse {
  animation: bounceIn 0.6s ease, pulse 2s ease-in-out infinite 0.6s;
}

@keyframes bounceIn {
  0% { transform: scale(0.3); opacity: 0; }
  50% { transform: scale(1.05); }
  70% { transform: scale(0.9); }
  100% { transform: scale(1); opacity: 1; }
}

@keyframes pulse {
  0%, 100% {
    transform: scale(1);
    filter: brightness(1);
  }
  50% {
    transform: scale(1.1);
    filter: brightness(1.2);
  }
}

/* 错误状态 */
.status-stage.error {
  color: #fc8181;
}

.error-actions {
  margin-top: 20px;
  display: flex;
  gap: 10px;
}

.retry-btn, .details-btn {
  padding: 12px 24px;
  border: 1px solid rgba(66, 153, 225, 0.3);
  background: rgba(66, 153, 225, 0.05);
  color: #4299e1;
  border-radius: 12px;
  cursor: pointer;
  font-size: 14px;
  font-weight: 500;
  transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
  backdrop-filter: blur(10px);
  -webkit-backdrop-filter: blur(10px);
  position: relative;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(66, 153, 225, 0.1);
}

.retry-btn::before, .details-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.5s ease;
}

.retry-btn:hover {
  background: rgba(66, 153, 225, 0.1);
  border-color: #4299e1;
  color: #63b3ed;
  transform: translateY(-2px);
  box-shadow: 0 8px 20px rgba(66, 153, 225, 0.2);
}

.retry-btn:hover::before {
  left: 100%;
}

.details-btn:hover {
  background: rgba(66, 153, 225, 0.08);
  border-color: rgba(66, 153, 225, 0.5);
  transform: translateY(-1px);
  box-shadow: 0 6px 16px rgba(66, 153, 225, 0.15);
}

.details-btn:hover::before {
  left: 100%;
}

.error-details {
  margin-top: 20px;
  text-align: left;
  background: rgba(252, 129, 129, 0.05);
  backdrop-filter: blur(10px);
  -webkit-backdrop-filter: blur(10px);
  padding: 20px;
  border-radius: 12px;
  border: 1px solid rgba(252, 129, 129, 0.2);
  box-shadow: 0 8px 20px rgba(252, 129, 129, 0.1);
  animation: slideInUp 0.4s ease;
}

.error-details h4 {
  margin: 0 0 15px 0;
  font-size: 16px;
  display: flex;
  align-items: center;
  gap: 8px;
}

.diagnostic-grid {
  display: grid;
  gap: 10px;
  margin-bottom: 20px;
}

.diagnostic-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 8px 12px;
  background: rgba(0, 0, 0, 0.2);
  border-radius: 4px;
  border-left: 3px solid #4299e1;
}

.diagnostic-item .label {
  font-weight: 500;
  color: #a0aec0;
  font-size: 13px;
}

.diagnostic-item .value {
  font-family: 'Courier New', monospace;
  font-size: 13px;
  color: #e2e8f0;
  word-break: break-all;
}

.diagnostic-item .value.connected {
  color: #68d391;
}

.diagnostic-item .value.disconnected {
  color: #fc8181;
}

.diagnostic-item .value.error-text {
  color: #feb2b2;
}

.troubleshooting {
  border-top: 1px solid rgba(252, 129, 129, 0.2);
  padding-top: 15px;
}

.troubleshooting h5 {
  margin: 0 0 10px 0;
  font-size: 14px;
  color: #a0aec0;
  display: flex;
  align-items: center;
  gap: 6px;
}

.troubleshooting ul {
  margin: 0;
  padding-left: 20px;
}

.troubleshooting li {
  margin: 8px 0;
  font-size: 13px;
  color: #cbd5e0;
}

.btn-icon {
  margin-right: 6px;
  transition: transform 0.2s ease;
}

.btn-icon.spinning {
  animation: spin 1s linear infinite;
}

.retry-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.retry-btn.retrying {
  pointer-events: none;
}

/* 自动重试指示器 */
.auto-retry-indicator {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  margin: 20px 0;
  padding: 15px;
  background: rgba(66, 153, 225, 0.1);
  border: 1px solid rgba(66, 153, 225, 0.3);
  border-radius: 8px;
  animation: slideInUp 0.5s ease;
}

.retry-spinner {
  width: 20px;
  height: 20px;
  border: 2px solid rgba(66, 153, 225, 0.3);
  border-top: 2px solid #4299e1;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

.retry-text {
  font-size: 14px;
  color: #4299e1;
  font-weight: 500;
}

@keyframes slideInUp {
  from {
    opacity: 0;
    transform: translateY(20px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}

/* 错误消息样式优化 */
.error-message {
  display: block;
  margin: 10px 0;
  padding: 8px 12px;
  background: rgba(252, 129, 129, 0.1);
  border-left: 3px solid #fc8181;
  border-radius: 4px;
  font-size: 14px;
  color: #feb2b2;
  animation: fadeInScale 0.4s ease;
}

@keyframes fadeInScale {
  from {
    opacity: 0;
    transform: scale(0.95);
  }
  to {
    opacity: 1;
    transform: scale(1);
  }
}

/* 进度条 */
.progress-container {
  margin-top: 25px;
  animation: slideInUp 0.6s ease 0.3s both;
}

.progress-bar {
  width: 100%;
  height: 8px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 4px;
  overflow: hidden;
  margin-bottom: 12px;
  position: relative;
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.1);
}

.progress-bar::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  animation: shimmer 2s infinite;
}

.progress-fill {
  height: 100%;
  background: linear-gradient(90deg, #4299e1, #63b3ed, #4299e1);
  background-size: 200% 100%;
  border-radius: 4px;
  transition: width 0.5s cubic-bezier(0.4, 0, 0.2, 1);
  animation: progressFlow 3s ease-in-out infinite, progressGlow 2s ease-in-out infinite alternate;
  position: relative;
  overflow: hidden;
}

.progress-fill::after {
  content: '';
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: linear-gradient(45deg, transparent 30%, rgba(255, 255, 255, 0.3) 50%, transparent 70%);
  animation: shine 2s infinite;
}

@keyframes progressFlow {
  0%, 100% { background-position: 0% 50%; }
  50% { background-position: 100% 50%; }
}

@keyframes progressGlow {
  0% { box-shadow: 0 0 8px rgba(66, 153, 225, 0.4), 0 0 16px rgba(66, 153, 225, 0.2); }
  100% { box-shadow: 0 0 16px rgba(66, 153, 225, 0.6), 0 0 24px rgba(66, 153, 225, 0.3); }
}

@keyframes shimmer {
  0% { left: -100%; }
  100% { left: 100%; }
}

@keyframes shine {
  0% { transform: translateX(-100%); }
  100% { transform: translateX(100%); }
}

.progress-text {
  font-size: 16px;
  opacity: 0.9;
  font-weight: 500;
  color: #e2e8f0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
  background: linear-gradient(90deg, #a0aec0 0%, #e2e8f0 50%, #a0aec0 100%);
  background-size: 200% 100%;
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  animation: progressTextFlow 3s ease-in-out infinite;
}

@keyframes progressTextFlow {
  0%, 100% { background-position: 0% 50%; }
  50% { background-position: 100% 50%; }
}

/* 背景粒子效果 */
.particles {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
  z-index: 1;
  overflow: hidden;
}

.particle {
  position: absolute;
  border-radius: 50%;
  background: radial-gradient(circle,
    rgba(66, 153, 225, 0.8) 0%,
    rgba(129, 197, 247, 0.4) 50%,
    transparent 100%);
  animation: float linear infinite, particleGlow 3s ease-in-out infinite;
  box-shadow: 0 0 10px rgba(66, 153, 225, 0.3);
}

.particle:nth-child(odd) {
  width: 3px;
  height: 3px;
  background: radial-gradient(circle,
    rgba(255, 119, 198, 0.8) 0%,
    rgba(255, 119, 198, 0.3) 50%,
    transparent 100%);
  box-shadow: 0 0 8px rgba(255, 119, 198, 0.4);
}

.particle:nth-child(3n) {
  width: 4px;
  height: 4px;
  background: radial-gradient(circle,
    rgba(120, 219, 255, 0.9) 0%,
    rgba(120, 219, 255, 0.4) 50%,
    transparent 100%);
  box-shadow: 0 0 12px rgba(120, 219, 255, 0.5);
}

@keyframes float {
  0% {
    opacity: 0;
    transform: translateY(100vh) translateX(0px) scale(0) rotate(0deg);
  }
  10% {
    opacity: 1;
  }
  90% {
    opacity: 1;
  }
  100% {
    opacity: 0;
    transform: translateY(-100vh) translateX(50px) scale(1.5) rotate(360deg);
  }
}

@keyframes particleGlow {
  0%, 100% {
    filter: brightness(1);
    transform: scale(1);
  }
  50% {
    filter: brightness(1.5);
    transform: scale(1.2);
  }
}

/* 装饰环形元素 */
.decorative-rings {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  pointer-events: none;
  z-index: 0;
}

.ring {
  position: absolute;
  border: 1px solid rgba(66, 153, 225, 0.1);
  border-radius: 50%;
  animation: ringRotate linear infinite;
}

.ring-1 {
  width: 800px;
  height: 800px;
  top: -400px;
  left: -400px;
  animation-duration: 60s;
  border-color: rgba(66, 153, 225, 0.05);
}

.ring-2 {
  width: 1200px;
  height: 1200px;
  top: -600px;
  left: -600px;
  animation-duration: 80s;
  animation-direction: reverse;
  border-color: rgba(255, 119, 198, 0.03);
}

.ring-3 {
  width: 1600px;
  height: 1600px;
  top: -800px;
  left: -800px;
  animation-duration: 120s;
  border-color: rgba(120, 219, 255, 0.02);
}

@keyframes ringRotate {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

/* 环境光晕 */
.ambient-glow {
  position: absolute;
  top: 50%;
  left: 50%;
  width: 400px;
  height: 400px;
  transform: translate(-50%, -50%);
  background:
    radial-gradient(circle,
      rgba(66, 153, 225, 0.08) 0%,
      rgba(66, 153, 225, 0.04) 40%,
      transparent 70%);
  border-radius: 50%;
  animation: ambientPulse 8s ease-in-out infinite;
  z-index: 0;
}

@keyframes ambientPulse {
  0%, 100% {
    transform: translate(-50%, -50%) scale(1);
    opacity: 0.6;
  }
  50% {
    transform: translate(-50%, -50%) scale(1.2);
    opacity: 1;
  }
}

/* 响应式设计 */
@media (max-width: 768px) {
  .boot-container {
    padding: 30px 25px;
    max-width: 90%;
    border-radius: 20px;
  }

  .system-title {
    font-size: 32px;
  }

  .logo-circle {
    width: 110px;
    height: 110px;
  }

  .logo-inner {
    width: 75px;
    height: 75px;
  }

  .logo-text {
    font-size: 26px;
  }

  .ring-1, .ring-2, .ring-3 {
    display: none;
  }

  .ambient-glow {
    width: 300px;
    height: 300px;
  }
}
</style>
