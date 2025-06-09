import './assets/main.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import router from './router'

const app = createApp(App)

app.use(createPinia())
app.use(router)

app.mount('#app')

// 添加全局CSS变量
const root = document.documentElement;
root.style.setProperty('--primary-color', '#0078d4');
root.style.setProperty('--primary-hover', '#106ebe');
root.style.setProperty('--accent-color', '#005a9e');
root.style.setProperty('--bg-primary', 'rgba(249, 249, 249, 0.85)');
root.style.setProperty('--bg-secondary', 'rgba(255, 255, 255, 0.6)');
root.style.setProperty('--text-primary', '#323130');
root.style.setProperty('--text-secondary', '#605e5c');
root.style.setProperty('--border-color', 'rgba(255, 255, 255, 0.3)');
root.style.setProperty('--shadow-light', '0 2px 8px rgba(0, 0, 0, 0.06)');
root.style.setProperty('--shadow-medium', '0 8px 24px rgba(0, 0, 0, 0.12)');
root.style.setProperty('--shadow-heavy', '0 16px 40px rgba(0, 0, 0, 0.2)');
root.style.setProperty('--blur-light', 'blur(15px)');
root.style.setProperty('--blur-heavy', 'blur(40px)');
root.style.setProperty('--transition-smooth', 'all 0.25s cubic-bezier(0.25, 0.46, 0.45, 0.94)');
