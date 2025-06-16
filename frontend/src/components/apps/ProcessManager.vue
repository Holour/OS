<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed, watch } from 'vue';
import { processAPI, schedulerAPI } from '@/services/api';
import { useDialogs } from '@/composables/useDialogs';

interface MemoryBlock {
  base_address: number;
  size: number;
}

interface Process {
  pid: number;
  name?: string;
  parent_pid?: number;
  state: string;
  program_counter: number;
  cpu_time?: number;
  priority?: number;
  creation_time?: number;
  memory_info: MemoryBlock[];
}

interface SchedulerConfig {
  algorithm: string;
  time_slice?: number;
}

interface GanttItem {
  pid: number;
  start: number;
  end: number;
}

const processes = ref<Process[]>([]);
const readyQueue = ref<Process[]>([]);
const processRelationships = ref<any[]>([]);
const isLoading = ref(true);
const error = ref<string | null>(null);

const { alert, success, error: showError, confirm } = useDialogs();
const newProcessName = ref('');
const newProcessSize = ref(1024);
const newProcessCPUTime = ref(10);
const newProcessPriority = ref(5);

// 新增功能相关变量
const selectedProcessForState = ref<number | null>(null);
const newProcessState = ref('READY');
const selectedParentProcess = ref<number | null>(null);
const childProcessName = ref('');
const childProcessSize = ref(1024);
const childProcessCPUTime = ref(10);
const childProcessPriority = ref(5);
const process1ForRelation = ref<number | null>(null);
const process2ForRelation = ref<number | null>(null);
const relationshipType = ref<'SYNC' | 'MUTEX'>('SYNC');

const processStates = ['NEW', 'READY', 'RUNNING', 'BLOCKED', 'TERMINATED'];

// 对话框显示状态
const showCreateProcessDialog = ref(false);
const showUpdateStateDialog = ref(false);
const showCreateChildDialog = ref(false);
const showCreateRelationDialog = ref(false);
const showReadyQueueDialog = ref(false);
const showRelationshipListDialog = ref(false);

let intervalId: number;

const schedulerConfig = ref<SchedulerConfig>({ algorithm: 'FCFS', time_slice: 1 });
const isEditingScheduler = ref(false);
const isSchedulerLoading = ref(false);
const isExecutingSchedule = ref(false);

const algorithmOptions = ['FCFS', 'SJF', 'PRIORITY', 'RR'];
const selectedAlgorithm = ref('FCFS');
const timeSlice = ref(1);

const ganttData = ref<GanttItem[]>([]);
const minStart = computed(() => (ganttData.value.length ? Math.min(...ganttData.value.map(g => g.start)) : 0));
const maxEnd = computed(() => (ganttData.value.length ? Math.max(...ganttData.value.map(g => g.end)) : 0));
const totalDuration = computed(() => maxEnd.value - minStart.value || 1);

// 时间格式化
const formatTime = (ms: number | undefined) => {
  if (!ms) return 'N/A';
  const date = new Date(ms);
  return date.toLocaleString();
};

// 获取进程名称
const getProcessName = (pid: number) => {
  const process = processes.value.find(p => p.pid === pid);
  return process ? (process.name || '未命名') : '未找到';
};

// 用于嵌套显示的扁平化进程列表，每个对象包含缩进级别
interface DisplayProcess extends Process { indent: number }
const nestedProcesses = computed<DisplayProcess[]>(() => {
  // 先按照 PID 建立 Map
  const map = new Map<number, Process[]>();
  processes.value.forEach(proc => {
    if (proc.parent_pid !== undefined && proc.parent_pid !== null && proc.parent_pid !== -1) {
      const list = map.get(proc.parent_pid) || [];
      list.push(proc);
      map.set(proc.parent_pid, list);
    }
  });

  // 递归构建结果
  const result: DisplayProcess[] = [];
  const visited = new Set<number>();
  const build = (proc: Process, indent: number) => {
    result.push({ ...proc, indent });
    visited.add(proc.pid);
    const children = map.get(proc.pid) || [];
    children.forEach(child => build(child, indent + 1));
  };

  // 先找所有顶级（无父或 parent_pid==-1）
  processes.value
    .filter(p => p.parent_pid === undefined || p.parent_pid === null || p.parent_pid === -1)
    .sort((a, b) => a.pid - b.pid)
    .forEach(parent => build(parent, 0));

  // 如果有孤儿进程（父进程已终止或数据缺失），也追加
  processes.value
    .filter(p => !visited.has(p.pid))
    .forEach(orphan => result.push({ ...orphan, indent: 0 }));

  return result;
});

const fetchProcesses = async () => {
  try {
    const response = await processAPI.getProcesses();
    if (response.data.status === 'success') {
      processes.value = response.data.data;
    } else {
      throw new Error(response.data.message || 'Failed to fetch processes');
    }
  } catch (err: any) {
    error.value = err.message || 'An unknown error occurred';
  } finally {
    isLoading.value = false;
  }
};

const fetchReadyQueue = async () => {
  try {
    const response = await schedulerAPI.getReadyQueue();
    if (response.data.status === 'success') {
      readyQueue.value = response.data.data;
    }
  } catch (err) {
    console.error('Failed to fetch ready queue:', err);
  }
};

const fetchSchedulerConfig = async () => {
  try {
    const res = await schedulerAPI.getStatus();
    if (res.data.status === 'success') {
      schedulerConfig.value = res.data.data;
      // 仅当用户未在编辑时才同步 UI 选择，避免被轮询覆盖
      if (!isEditingScheduler.value) {
        selectedAlgorithm.value = schedulerConfig.value.algorithm;
        if (schedulerConfig.value.time_slice !== undefined) {
          timeSlice.value = schedulerConfig.value.time_slice;
        }
      }
    }
  } catch (err) {
    console.error('Failed to fetch scheduler config:', err);
  }
};

const updateSchedulerConfig = async () => {
  try {
    isSchedulerLoading.value = true;
    // 提交前先重置编辑状态
    isEditingScheduler.value = false;
    await schedulerAPI.setConfig(selectedAlgorithm.value, selectedAlgorithm.value === 'RR' ? timeSlice.value : undefined);
    await fetchSchedulerConfig();

    // 清空当前甘特图数据，为新的调度算法准备
    ganttData.value = [];

    // 立即刷新数据以获取新的甘特图
    await Promise.all([
      fetchProcesses(),
      fetchReadyQueue(),
      fetchGanttData()
    ]);

    success('调度器配置已更新');
  } catch (err: any) {
    error.value = err.message || 'Failed to update scheduler config';
  } finally {
    isSchedulerLoading.value = false;
  }
};

const fetchGanttData = async () => {
  try {
    const res = await schedulerAPI.getGanttChart();
    if (res.data.status === 'success' && Array.isArray(res.data.data)) {
      // 总是更新甘特图数据，不管是否为空
      ganttData.value = res.data.data;
      console.log('甘特图数据已更新:', res.data.data);
    }
  } catch (err) {
    console.error('Failed to fetch gantt data:', err);
  }
};

const fetchProcessRelationships = async () => {
  try {
    const response = await processAPI.getProcessRelationships();
    if (response.data.status === 'success') {
      processRelationships.value = response.data.data || [];
    }
  } catch (err) {
    console.error('Failed to fetch process relationships:', err);
  }
};

onMounted(() => {
  // 初始化随机优先级
  newProcessPriority.value = Math.floor(Math.random() * 5) + 1;
  childProcessPriority.value = Math.floor(Math.random() * 5) + 1;

  // 初始化CPU时间
  newProcessCPUTime.value = Math.max(200, Math.min(1000, Math.floor(newProcessSize.value / 32)));
  childProcessCPUTime.value = Math.max(200, Math.min(1000, Math.floor(childProcessSize.value / 32)));

  fetchProcesses();
  fetchReadyQueue();
  fetchSchedulerConfig();
  fetchGanttData();
  fetchProcessRelationships();
  intervalId = window.setInterval(() => {
    fetchProcesses();
    fetchReadyQueue();
    fetchSchedulerConfig();
    fetchGanttData();
    fetchProcessRelationships();
  }, 3000); // Refresh every 3 seconds
});

const createProcess = async () => {
  if (newProcessSize.value <= 0) {
    showError('请输入有效的内存大小');
    return;
  }

  try {
    // 生成随机优先级（1-5）
    const randomPriority = Math.floor(Math.random() * 5) + 1;

    // 根据内存大小计算CPU时间（内存大小除以32）
    const calculatedCPUTime = Math.max(200, Math.min(1000, Math.floor(newProcessSize.value / 32)));

    // 生成进程名（如果用户没有输入）
    const processName = newProcessName.value.trim() || `Process_${Date.now().toString().slice(-6)}`;

    await processAPI.createProcess(
      newProcessSize.value,
      calculatedCPUTime,
      randomPriority,
      processName
    );

    // 重置表单，优先级使用随机值
    newProcessName.value = '';
    newProcessSize.value = 1024;
    newProcessCPUTime.value = Math.max(1, Math.floor(1024 / 32)); // 默认根据1024计算
    newProcessPriority.value = Math.floor(Math.random() * 5) + 1; // 随机1-5

    fetchProcesses();
    success(`进程创建成功！\n进程名: ${processName}\n优先级: ${randomPriority}\nCPU时间: ${calculatedCPUTime}ms`);
    showCreateProcessDialog.value = false;
  } catch (err: any) {
    error.value = err.message || 'Failed to create process';
  }
};

const terminateProcess = async (pid: number) => {
  try {
    const confirmed = await confirm(`确定要终止进程 ${pid} 吗？`, '终止进程');
    if (!confirmed) return;

    await processAPI.terminateProcess(pid);
    fetchProcesses();
    success('进程终止成功！');
  } catch (err: any) {
    showError(err.message || 'Failed to terminate process');
  }
};

const executeTick = async () => {
  try {
    isExecutingSchedule.value = true;
    const response = await schedulerAPI.tick();
    if (response.data.status === 'success') {
      // 执行调度后立即刷新数据
      await Promise.all([
        fetchProcesses(),
        fetchReadyQueue(),
        fetchGanttData()
      ]);

      if (response.data.data) {
        console.log('调度执行成功，当前运行进程:', response.data.data);
      } else {
        console.log('调度执行成功，但就绪队列为空');
      }
    }
  } catch (err: any) {
    console.error('调度执行失败:', err);
    error.value = err.message || 'Failed to execute scheduler tick';
  } finally {
    isExecutingSchedule.value = false;
  }
};

// 更新进程状态
const updateProcessState = async () => {
  if (!selectedProcessForState.value) {
    showError('请选择要更新状态的进程');
    return;
  }

  try {
    await processAPI.updateProcessState(selectedProcessForState.value, newProcessState.value);
    await fetchProcesses();
    success(`进程 ${selectedProcessForState.value} 状态已更新为 ${newProcessState.value}`);
    selectedProcessForState.value = null;
    showUpdateStateDialog.value = false;
  } catch (err: any) {
    error.value = err.message || 'Failed to update process state';
  }
};

// 创建子进程
const createChildProcess = async () => {
  if (!selectedParentProcess.value) {
    showError('请选择父进程');
    return;
  }

  if (childProcessSize.value <= 0) {
    showError('请输入有效的内存大小');
    return;
  }

  try {
    // 生成随机优先级（1-5）
    const randomPriority = Math.floor(Math.random() * 5) + 1;

    // 根据内存大小计算CPU时间（内存大小除以32）
    const calculatedCPUTime = Math.max(200, Math.min(1000, Math.floor(childProcessSize.value / 32)));

    // 生成子进程名（如果用户没有输入）
    const childName = childProcessName.value.trim() || `Child_${Date.now().toString().slice(-6)}`;

    await processAPI.createChildProcess(
      selectedParentProcess.value,
      childProcessSize.value,
      calculatedCPUTime,
      randomPriority,
      childName
    );

    // 重置表单
    selectedParentProcess.value = null;
    childProcessName.value = '';
    childProcessSize.value = 1024;
    childProcessCPUTime.value = Math.max(1, Math.floor(1024 / 32)); // 根据默认大小计算
    childProcessPriority.value = Math.floor(Math.random() * 5) + 1; // 随机1-5

    await fetchProcesses();
    success(`子进程创建成功！\n进程名: ${childName}\n优先级: ${randomPriority}\nCPU时间: ${calculatedCPUTime}ms`);
    showCreateChildDialog.value = false;
  } catch (err: any) {
    error.value = err.message || 'Failed to create child process';
  }
};

// 创建进程关系
const createProcessRelationship = async () => {
  if (!process1ForRelation.value || !process2ForRelation.value) {
    showError('请选择两个进程来建立关系');
    return;
  }

  if (process1ForRelation.value === process2ForRelation.value) {
    showError('不能为同一个进程建立关系');
    return;
  }

  try {
    await processAPI.createProcessRelationship(
      process1ForRelation.value,
      process2ForRelation.value,
      relationshipType.value
    );

    // 重置表单
    process1ForRelation.value = null;
    process2ForRelation.value = null;

    // 刷新进程关系列表
    await fetchProcessRelationships();

    success(`已建立 ${relationshipType.value} 关系！`);
    showCreateRelationDialog.value = false;
  } catch (err: any) {
    error.value = err.message || 'Failed to create process relationship';
  }
};

onUnmounted(() => {
  clearInterval(intervalId);
});

watch([selectedAlgorithm, timeSlice], () => {
  // 仅当与后端当前配置不一致时，认为用户正在编辑
  const algoChanged = selectedAlgorithm.value !== schedulerConfig.value.algorithm;
  const sliceChanged = schedulerConfig.value.algorithm === 'RR' && selectedAlgorithm.value === 'RR'
    ? timeSlice.value !== schedulerConfig.value.time_slice
    : false;
  if (algoChanged || sliceChanged) {
    isEditingScheduler.value = true;
  }
});

// 监听内存大小变化，自动更新CPU时间
watch(newProcessSize, (newSize) => {
      newProcessCPUTime.value = Math.max(200, Math.min(1000, Math.floor(newSize / 32)));
});

watch(childProcessSize, (newSize) => {
      childProcessCPUTime.value = Math.max(200, Math.min(1000, Math.floor(newSize / 32)));
});
</script>

<template>
  <div class="process-manager">
    <!-- 调度器配置置顶 -->
    <div class="scheduler-config top-config">
      <div class="config-container">
        <div class="config-main-row">
          <div class="config-title">🚀 调度器配置</div>

          <div class="config-controls">
            <div class="control-group">
        <label>算法:</label>
              <select v-model="selectedAlgorithm" :disabled="isSchedulerLoading">
          <option v-for="alg in algorithmOptions" :key="alg" :value="alg">{{ alg }}</option>
        </select>
              <span class="algorithm-hint">
                <span v-if="selectedAlgorithm === 'FCFS'">先来先服务</span>
                <span v-else-if="selectedAlgorithm === 'SJF'">短作业优先</span>
                <span v-else-if="selectedAlgorithm === 'PRIORITY'">优先级调度</span>
                <span v-else-if="selectedAlgorithm === 'RR'">时间片轮转</span>
              </span>
            </div>

            <div v-show="selectedAlgorithm === 'RR'" class="control-group">
              <label>时间片:</label>
        <input
          v-model.number="timeSlice"
          type="number"
          min="1"
          class="timeslice-input"
          placeholder="ms"
                :disabled="isSchedulerLoading"
              />
              <span class="unit-label">ms</span>
            </div>

            <div class="control-group actions">
              <button @click="updateSchedulerConfig" :disabled="isSchedulerLoading" class="update-btn compact">
                <div v-if="isSchedulerLoading" class="loading-spinner"></div>
                <span v-else>⚙️</span>
                {{ isSchedulerLoading ? '更新中' : '更新' }}
              </button>

              <button @click="executeTick" :disabled="isExecutingSchedule" class="schedule-btn compact">
                <div v-if="isExecutingSchedule" class="loading-spinner"></div>
                <span v-else>⚡</span>
                {{ isExecutingSchedule ? '执行中' : '执行调度' }}
              </button>
            </div>
          </div>
        </div>

        <div class="config-status-row">
          <div class="current-status">
            <span class="status-label">当前:</span>
            <span class="status-value">{{ schedulerConfig.algorithm }}</span>
            <span v-if="schedulerConfig.algorithm === 'RR'" class="time-slice-value">
              / {{ schedulerConfig.time_slice }}ms
        </span>
          </div>
          <div class="config-indicator" :class="{ active: isEditingScheduler }">
            <div class="indicator-dot"></div>
            <span class="indicator-text">{{ isEditingScheduler ? '有待保存的更改' : '配置已同步' }}</span>
          </div>
        </div>
      </div>
    </div>

    <!-- 功能按钮区域 -->
    <div class="feature-buttons">
      <button @click="showCreateProcessDialog = true" class="feature-btn create-btn">
        ➕ 创建新进程
      </button>
      <button @click="showUpdateStateDialog = true" class="feature-btn state-btn">
        🔄 更新进程状态
      </button>
      <button @click="showCreateChildDialog = true" class="feature-btn child-btn">
        👥 创建子进程
      </button>
      <button @click="showCreateRelationDialog = true" class="feature-btn relation-btn">
        🔗 创建进程关系
      </button>
      <button @click="showReadyQueueDialog = true" class="feature-btn queue-btn">
        📋 查看就绪队列
      </button>
      <button @click="showRelationshipListDialog = true" class="feature-btn relation-list-btn">
        🔗 查看进程关系
      </button>
    </div>

    <div v-if="isLoading" class="loading">加载进程中...</div>
    <div v-else-if="error" class="error-message">错误: {{ error }}</div>

    <div v-else class="process-layout">
      <div class="process-list-container">
        <table class="process-table">
          <thead>
            <tr>
              <th>PID</th>
              <th>进程名</th>
              <th>父PID</th>
              <th>状态</th>
              <th>程序计数器</th>
              <th>CPU 时间 (ms)</th>
              <th>优先级</th>
              <th>创建时间</th>
              <th>内存首地址</th>
              <th>内存大小 (KB)</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            <tr v-for="proc in nestedProcesses" :key="proc.pid"
                :class="['process-row', `indent-${proc.indent}`, `state-row-${proc.state.toLowerCase()}`]">
              <td class="pid-cell">
                <div class="pid-container" :style="{ paddingLeft: (proc.indent * 24) + 'px' }">
                  <div v-if="proc.indent > 0" class="process-tree">
                    <div class="tree-line"></div>
                    <div class="tree-connector">└─</div>
                  </div>
                  <div class="pid-badge" :class="`priority-${proc.priority || 5}`">
                    <span class="pid-number">{{ proc.pid }}</span>
                    <span class="priority-indicator">P{{ proc.priority || 5 }}</span>
                  </div>
                </div>
              </td>
              <td class="process-name-cell">
                <div class="process-name">
                  <span class="name-text">{{ proc.name || 'N/A' }}</span>
                  <span v-if="proc.indent > 0" class="child-indicator">子进程</span>
                </div>
              </td>
              <td class="parent-cell">
                <span v-if="proc.parent_pid === -1 || !proc.parent_pid" class="root-process">根进程</span>
                <span v-else class="parent-pid">{{ proc.parent_pid }}</span>
              </td>
              <td class="state-cell">
                <div :class="['state-badge', 'state-' + proc.state.toLowerCase()]">
                  <div class="state-indicator"></div>
                  <span class="state-text">{{ proc.state }}</span>
      </div>
              </td>
              <td class="counter-cell">{{ proc.program_counter }}</td>
              <td class="cpu-cell">
                <span class="cpu-time">{{ proc.cpu_time || 'N/A' }}</span>
                <span v-if="proc.cpu_time" class="time-unit">ms</span>
              </td>
              <td class="priority-cell">
                <div class="priority-display" :class="`priority-level-${proc.priority || 5}`">
                  {{ proc.priority ?? 'N/A' }}
          </div>
              </td>
              <td class="time-cell">{{ formatTime(proc.creation_time) }}</td>
              <td class="memory-cell">
                <div v-if="proc.memory_info.length > 0" class="memory-info">
                  <span class="memory-address">0x{{ proc.memory_info[0].base_address.toString(16).toUpperCase() }}</span>
        </div>
                <div v-else class="no-memory">
                  <span class="no-memory-text">无内存</span>
      </div>
              </td>
              <td class="size-cell">
                <div class="memory-size">
                  <span class="size-number">{{ (proc.memory_info.reduce((sum: number, block: MemoryBlock) => sum + block.size, 0) / 1024).toFixed(2) }}</span>
                  <span class="size-unit">KB</span>
                </div>
              </td>
              <td class="action-cell">
                <button @click="terminateProcess(proc.pid)" class="terminate-btn">
                  <span class="btn-icon">🗑️</span>
                  <span class="btn-text">终止</span>
                </button>
              </td>
            </tr>
          </tbody>
        </table>
      </div>

    </div>

    <div class="gantt-section">
      <div class="gantt-header">
        <h4>调度甘特图</h4>
        <button @click="fetchGanttData" class="refresh-btn" title="手动刷新甘特图">
          🔄 刷新
        </button>
      </div>
      <div v-if="ganttData.length" class="gantt-chart">
        <div class="gantt-scroll-container">
          <div class="gantt-bar" :style="{ minWidth: Math.max(600, ganttData.length * 80) + 'px' }">
            <div
              v-for="(item, index) in ganttData"
              :key="index"
              class="gantt-block"
              :style="{
                left: ((item.start - minStart) / totalDuration * 100) + '%',
                width: ((item.end - item.start) / totalDuration * 100) + '%',
                backgroundColor: `hsl(${item.pid * 40 % 360},70%,70%)`
              }"
            >
              PID {{ item.pid }}
            </div>
          </div>
          <div v-if="ganttData.length > 5" class="scroll-hint">
            ← → 横向滚动查看更多
          </div>
        </div>
        <div class="gantt-time-axis">
          <div class="time-marker" style="left: 0%">{{ minStart }}ms</div>
          <div class="time-marker" style="right: 0%">{{ maxEnd }}ms</div>
        </div>
      </div>
      <div v-else class="empty-queue">
        <div class="empty-content">
          <div class="empty-icon">📊</div>
          <div>暂无甘特图数据</div>
          <div class="empty-tip">请先创建进程并运行调度器</div>
        </div>
      </div>
    </div>

    <!-- 创建新进程对话框 -->
    <div v-if="showCreateProcessDialog" class="dialog-overlay" @click="showCreateProcessDialog = false">
      <div class="dialog" @click.stop>
        <div class="dialog-header">
          <h3>创建新进程</h3>
          <button @click="showCreateProcessDialog = false" class="close-btn">×</button>
        </div>
        <div class="dialog-body">
          <div class="form-group">
            <label for="process-name">进程名称：</label>
            <input id="process-name" v-model="newProcessName" type="text" placeholder="可选，如：Worker" />
          </div>
          <div class="form-group">
            <label for="process-memory">内存大小（字节）：</label>
            <input id="process-memory" v-model.number="newProcessSize" type="number" min="1" placeholder="例如：1024" />
          </div>
          <div class="form-group">
            <label for="process-cpu">CPU时间（毫秒）：</label>
            <input id="process-cpu" v-model.number="newProcessCPUTime" type="number" min="1" placeholder="自动计算（内存大小÷32）" readonly />
            <div class="form-note">自动根据内存大小计算：{{ Math.max(200, Math.min(1000, Math.floor(newProcessSize / 32))) }}ms (范围: 200-1000ms)</div>
          </div>
          <div class="form-group">
            <label for="process-priority">优先级：</label>
            <input id="process-priority" v-model.number="newProcessPriority" type="number" min="1" max="5" placeholder="随机生成（1-5）" readonly />
            <div class="form-note">随机生成优先级：{{ newProcessPriority }}（数字越小优先级越高）</div>
          </div>
          <div class="form-help">
            <p>🎯 智能进程创建说明：</p>
            <ul>
              <li><strong>进程名</strong>：如果不填写，系统会自动生成唯一名称</li>
              <li><strong>内存大小</strong>：进程占用的内存空间，单位为字节</li>
              <li><strong>CPU时间</strong>：<span class="highlight">自动计算</span> - 内存大小 ÷ 32，模拟实际系统中内存与CPU时间的关系</li>
              <li><strong>优先级</strong>：<span class="highlight">随机分配</span> - 系统随机分配1-5的优先级，数字越小优先级越高</li>
            </ul>
            <p>✨ 创建的进程将自动分配PID并进入NEW状态，等待调度器调度。</p>
          </div>
        </div>
        <div class="dialog-footer">
          <button @click="showCreateProcessDialog = false" class="cancel-btn">取消</button>
          <button @click="createProcess" class="confirm-btn">创建进程</button>
        </div>
      </div>
    </div>

    <!-- 更新进程状态对话框 -->
    <div v-if="showUpdateStateDialog" class="dialog-overlay" @click="showUpdateStateDialog = false">
      <div class="dialog" @click.stop>
        <div class="dialog-header">
          <h3>更新进程状态</h3>
          <button @click="showUpdateStateDialog = false" class="close-btn">×</button>
        </div>
        <div class="dialog-body">
          <div class="form-group">
            <label for="process-select">选择进程：</label>
            <select id="process-select" v-model="selectedProcessForState">
              <option value="">请选择要更新状态的进程</option>
              <option v-for="proc in processes" :key="proc.pid" :value="proc.pid">
                PID {{ proc.pid }} - {{ proc.name || '未命名' }} (当前状态: {{ proc.state }})
              </option>
            </select>
          </div>
          <div class="form-group">
            <label for="new-state">新状态：</label>
            <select id="new-state" v-model="newProcessState">
              <option v-for="state in processStates" :key="state" :value="state">{{ state }}</option>
            </select>
          </div>
          <div class="form-help">
            <p>状态说明：</p>
            <ul>
              <li><strong>NEW</strong>：新建状态，进程刚创建</li>
              <li><strong>READY</strong>：就绪状态，等待CPU分配</li>
              <li><strong>RUNNING</strong>：运行状态，正在占用CPU</li>
              <li><strong>BLOCKED</strong>：阻塞状态，等待I/O或资源</li>
              <li><strong>TERMINATED</strong>：终止状态，进程结束</li>
            </ul>
          </div>
        </div>
        <div class="dialog-footer">
          <button @click="showUpdateStateDialog = false" class="cancel-btn">取消</button>
          <button @click="updateProcessState" class="confirm-btn">更新状态</button>
        </div>
      </div>
    </div>

    <!-- 创建子进程对话框 -->
    <div v-if="showCreateChildDialog" class="dialog-overlay" @click="showCreateChildDialog = false">
      <div class="dialog" @click.stop>
        <div class="dialog-header">
          <h3>创建子进程</h3>
          <button @click="showCreateChildDialog = false" class="close-btn">×</button>
        </div>
        <div class="dialog-body">
          <div class="form-group">
            <label for="parent-process">选择父进程：</label>
            <select id="parent-process" v-model="selectedParentProcess">
              <option value="">请选择作为父进程的进程</option>
              <option v-for="proc in processes" :key="proc.pid" :value="proc.pid">
                PID {{ proc.pid }} - {{ proc.name || '未命名' }}
              </option>
            </select>
          </div>
          <div class="form-group">
            <label for="child-name">子进程名称：</label>
            <input id="child-name" v-model="childProcessName" type="text" placeholder="输入子进程名称（可选）" />
          </div>
          <div class="form-group">
            <label for="child-memory">内存大小（字节）：</label>
            <input id="child-memory" v-model.number="childProcessSize" type="number" min="1" placeholder="例如：4096" />
          </div>
          <div class="form-group">
            <label for="child-cpu">CPU时间（毫秒）：</label>
            <input id="child-cpu" v-model.number="childProcessCPUTime" type="number" min="1" placeholder="自动计算（内存大小÷32）" readonly />
            <div class="form-note">自动根据内存大小计算：{{ Math.max(200, Math.min(1000, Math.floor(childProcessSize / 32))) }}ms (范围: 200-1000ms)</div>
          </div>
          <div class="form-group">
            <label for="child-priority">优先级：</label>
            <input id="child-priority" v-model.number="childProcessPriority" type="number" min="1" max="5" placeholder="随机生成（1-5）" readonly />
            <div class="form-note">随机生成优先级：{{ childProcessPriority }}（数字越小优先级越高）</div>
          </div>
          <div class="form-help">
            <p>🎯 智能子进程创建说明：</p>
            <ul>
              <li><strong>继承关系</strong>：子进程将继承父进程的某些属性，并在父进程终止时自动终止</li>
              <li><strong>进程名</strong>：如果不填写，系统会自动生成带有"Child_"前缀的唯一名称</li>
              <li><strong>CPU时间</strong>：<span class="highlight">自动计算</span> - 内存大小 ÷ 32</li>
              <li><strong>优先级</strong>：<span class="highlight">随机分配</span> - 系统随机分配1-5的优先级</li>
            </ul>
          </div>
        </div>
        <div class="dialog-footer">
          <button @click="showCreateChildDialog = false" class="cancel-btn">取消</button>
          <button @click="createChildProcess" class="confirm-btn">创建子进程</button>
        </div>
      </div>
    </div>

    <!-- 创建进程关系对话框 -->
    <div v-if="showCreateRelationDialog" class="dialog-overlay" @click="showCreateRelationDialog = false">
      <div class="dialog" @click.stop>
        <div class="dialog-header">
          <h3>创建进程关系</h3>
          <button @click="showCreateRelationDialog = false" class="close-btn">×</button>
        </div>
        <div class="dialog-body">
          <div class="form-group">
            <label for="process1">选择进程1：</label>
            <select id="process1" v-model="process1ForRelation">
              <option value="">请选择第一个进程</option>
              <option v-for="proc in processes" :key="proc.pid" :value="proc.pid">
                PID {{ proc.pid }} - {{ proc.name || '未命名' }}
              </option>
            </select>
          </div>
          <div class="form-group">
            <label for="process2">选择进程2：</label>
            <select id="process2" v-model="process2ForRelation">
              <option value="">请选择第二个进程</option>
              <option v-for="proc in processes" :key="proc.pid" :value="proc.pid">
                PID {{ proc.pid }} - {{ proc.name || '未命名' }}
              </option>
            </select>
          </div>
          <div class="form-group">
            <label for="relation-type">关系类型：</label>
            <select id="relation-type" v-model="relationshipType">
              <option value="SYNC">同步关系 (SYNC)</option>
              <option value="MUTEX">互斥关系 (MUTEX)</option>
            </select>
          </div>
          <div class="form-help">
            <p>关系类型说明：</p>
            <ul>
              <li><strong>同步关系 (SYNC)</strong>：一个进程状态改变时，另一个进程状态也会同步改变</li>
              <li><strong>互斥关系 (MUTEX)</strong>：两个进程不能同时访问共享资源</li>
            </ul>
          </div>
        </div>
        <div class="dialog-footer">
          <button @click="showCreateRelationDialog = false" class="cancel-btn">取消</button>
          <button @click="createProcessRelationship" class="confirm-btn">建立关系</button>
        </div>
      </div>
    </div>

    <!-- 就绪队列对话框 -->
    <div v-if="showReadyQueueDialog" class="dialog-overlay" @click="showReadyQueueDialog = false">
      <div class="dialog ready-queue-dialog" @click.stop>
        <div class="dialog-header">
          <h3>📋 就绪队列</h3>
          <button @click="showReadyQueueDialog = false" class="close-btn">×</button>
        </div>
        <div class="dialog-body">
          <div class="queue-info">
            <div class="queue-stats">
              <span class="queue-count">队列长度: {{ readyQueue.length }}</span>
              <button @click="fetchReadyQueue" class="refresh-queue-btn">🔄 刷新</button>
            </div>
          </div>

          <div v-if="readyQueue.length === 0" class="empty-queue-dialog">
            <div class="empty-content">
              <div class="empty-icon">🔄</div>
              <div class="empty-text">就绪队列为空</div>
              <div class="empty-tip">当前没有进程等待调度</div>
            </div>
          </div>

          <div v-else class="ready-queue-list">
            <div v-for="(proc, index) in readyQueue" :key="proc.pid" class="ready-queue-item-dialog">
              <div class="queue-position">
                <span class="position-number">#{{ index + 1 }}</span>
              </div>
              <div class="queue-process-info">
                <div class="process-header">
                  <span class="process-pid">PID {{ proc.pid }}</span>
                  <span class="process-name">{{ proc.name || '未命名' }}</span>
                </div>
                <div class="process-details">
                  <span class="detail-item">优先级: <strong>{{ proc.priority || 'N/A' }}</strong></span>
                  <span class="detail-item">CPU时间: <strong>{{ proc.cpu_time || 'N/A' }}ms</strong></span>
                  <span class="detail-item">内存: <strong>{{ (proc.memory_info?.reduce((sum: number, block: MemoryBlock) => sum + block.size, 0) / 1024).toFixed(2) }}KB</strong></span>
                </div>
              </div>
              <div class="queue-status">
                <div class="status-indicator ready-indicator">
                  <div class="pulse-dot"></div>
                  <span>等待中</span>
                </div>
              </div>
            </div>
          </div>

          <div class="queue-help">
            <p>💡 <strong>就绪队列说明</strong></p>
            <ul>
              <li>就绪队列包含所有处于READY状态的进程</li>
              <li>调度器会按照当前算法从队列中选择进程执行</li>
              <li>队列顺序可能根据调度算法而变化（如优先级调度）</li>
              <li>点击"执行调度"按钮可以手动触发调度器运行</li>
            </ul>
          </div>
        </div>
        <div class="dialog-footer">
          <button @click="executeTick" class="schedule-btn">⚡ 执行调度</button>
          <button @click="showReadyQueueDialog = false" class="cancel-btn">关闭</button>
        </div>
      </div>
    </div>

    <!-- 进程关系列表对话框 -->
    <div v-if="showRelationshipListDialog" class="dialog-overlay" @click="showRelationshipListDialog = false">
      <div class="dialog relationships-dialog" @click.stop>
        <div class="dialog-header">
          <h3>🔗 进程关系列表</h3>
          <button @click="showRelationshipListDialog = false" class="close-btn">×</button>
        </div>
        <div class="dialog-body">
          <div class="relationships-info">
            <div class="relationships-stats">
              <span class="relationships-count">关系总数: {{ processRelationships.length }}</span>
              <button @click="fetchProcessRelationships" class="refresh-relationships-btn">🔄 刷新</button>
            </div>
          </div>

          <div v-if="processRelationships.length === 0" class="empty-relationships-dialog">
            <div class="empty-content">
              <div class="empty-icon">🔗</div>
              <div class="empty-text">暂无进程关系</div>
              <div class="empty-tip">进程之间尚未建立任何同步或互斥关系</div>
            </div>
          </div>

          <div v-else class="relationships-list">
            <div v-for="(rel, index) in processRelationships" :key="index" class="relationship-item-dialog">
              <div class="relationship-index">
                <span class="index-number">#{{ index + 1 }}</span>
              </div>
              <div class="relationship-details">
                <div class="relationship-header">
                  <div class="process-pair">
                    <div class="process-info">
                      <span class="process-label">进程A:</span>
                      <span class="process-pid">PID {{ rel.pid1 }}</span>
                      <span class="process-name">{{ getProcessName(rel.pid1) }}</span>
                    </div>
                    <div class="relationship-arrow">
                      <span v-if="rel.relation_type === 'SYNC'" class="sync-arrow">🔄</span>
                      <span v-else class="mutex-arrow">🚫</span>
                    </div>
                    <div class="process-info">
                      <span class="process-label">进程B:</span>
                      <span class="process-pid">PID {{ rel.pid2 }}</span>
                      <span class="process-name">{{ getProcessName(rel.pid2) }}</span>
                    </div>
                  </div>
                </div>
                <div class="relationship-type-info">
                  <div :class="['relationship-type-badge', 'type-' + rel.relation_type.toLowerCase()]">
                    <span class="type-icon">
                      {{ rel.relation_type === 'SYNC' ? '🔄' : '🚫' }}
                    </span>
                    <span class="type-text">
                      {{ rel.relation_type === 'SYNC' ? '同步关系' : '互斥关系' }}
                    </span>
                    <span class="type-desc">
                      {{ rel.relation_type === 'SYNC' ? '(状态同步)' : '(资源互斥)' }}
                    </span>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <div class="relationships-help">
            <p>💡 <strong>进程关系说明</strong></p>
            <ul>
              <li><strong>同步关系 (SYNC)</strong>：两个进程状态会同步变化，一个阻塞时另一个也会阻塞</li>
              <li><strong>互斥关系 (MUTEX)</strong>：两个进程不能同时访问共享资源，避免竞争条件</li>
              <li>关系是双向的：进程A与进程B的关系等同于进程B与进程A的关系</li>
            </ul>
          </div>
        </div>
        <div class="dialog-footer">
          <button @click="showCreateRelationDialog = true; showRelationshipListDialog = false" class="create-relation-btn">
            ➕ 创建新关系
          </button>
          <button @click="showRelationshipListDialog = false" class="cancel-btn">关闭</button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.process-manager {
  display: flex;
  flex-direction: column;
  height: 100%;
  font-size: 13px;
  color: #323130;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: linear-gradient(135deg, #f8f9fa 0%, #f1f3f4 100%);
}

.header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px 20px;
  border-bottom: 1px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
}

.header h3 {
  margin: 0;
  color: #323130;
  font-weight: 600;
  font-size: 16px;
}

.create-process {
  padding: 16px 20px;
  border-bottom: 1px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
}

.create-process h3 {
  margin: 0 0 12px 0;
  color: #323130;
  font-weight: 600;
  font-size: 15px;
}

.create-form {
  display: flex;
  gap: 12px;
  align-items: center;
  flex-wrap: wrap;
}

.create-form input {
  padding: 8px 12px;
  font-size: 13px;
  color: #323130;
  border: 2px solid #e1e5e9;
  border-radius: 6px;
  background: #ffffff;
  transition: all 0.2s ease;
  min-width: 140px;
}

.create-form input:focus {
  outline: none;
  border-color: #0078d4;
  box-shadow: 0 0 0 2px rgba(0, 120, 212, 0.2);
}

.create-form input:hover {
  border-color: #c7c6c4;
}

.create-form button {
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  color: white;
  border: none;
  padding: 8px 16px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  border-radius: 6px;
  transition: all 0.2s ease;
  box-shadow: 0 2px 4px rgba(0, 120, 212, 0.2);
}

.create-form button:hover {
  background: linear-gradient(135deg, #106ebe 0%, #005a9e 100%);
  box-shadow: 0 4px 8px rgba(0, 120, 212, 0.3);
  transform: translateY(-1px);
}

.create-form button:active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(0, 120, 212, 0.2);
}

.process-layout {
  display: flex;
  flex-grow: 1;
  overflow: hidden;
  gap: 1px;
}

.process-list-container {
  flex-grow: 1;
  overflow-y: auto;
  padding: 20px;
  background: #ffffff;
}

.ready-queue-container {
  width: 240px;
  border-left: 2px solid #e1e5e9;
  padding: 20px;
  overflow-y: auto;
  background: linear-gradient(135deg, #f8f9fa 0%, #f1f3f4 100%);
}

.ready-queue-container h4 {
  margin-top: 0;
  margin-bottom: 16px;
  border-bottom: 2px solid #0078d4;
  padding-bottom: 8px;
  color: #323130;
  font-weight: 600;
  font-size: 14px;
}

.ready-queue-item {
  padding: 12px;
  margin-bottom: 8px;
  background: #ffffff;
  border: 1px solid #e1e5e9;
  border-radius: 8px;
  font-size: 12px;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.05);
  transition: all 0.2s ease;
}

.ready-queue-item:hover {
  background: #f8f9fa;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
  transform: translateY(-1px);
}

.empty-queue {
  text-align: center;
  color: #8a8886;
  font-style: italic;
  padding: 20px;
  background: #ffffff;
  border-radius: 8px;
  border: 2px dashed #e1e5e9;
}

.loading, .error-message {
  padding: 40px 20px;
  text-align: center;
  font-size: 14px;
}

.error-message {
  background: linear-gradient(135deg, #fdf2f2 0%, #fef5f5 100%);
  color: #d13438;
  border-radius: 8px;
  margin: 20px;
  border-left: 4px solid #d13438;
}

.process-table {
  width: 100%;
  border-collapse: separate;
  border-spacing: 0;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  background: #ffffff;
}

th, td {
  border: none;
  border-bottom: 1px solid #f3f2f1;
  padding: 12px 16px;
  text-align: left;
  font-size: 12px;
  color: #323130;
}

th {
  color: #323130;
  font-weight: 600;
  font-size: 13px;
  background: linear-gradient(135deg, #f8f9fa 0%, #f1f3f4 100%);
  position: sticky;
  top: 0;
  z-index: 10;
}

.process-table tr:hover {
  background: linear-gradient(135deg, #f8f9fa 0%, #f1f3f4 100%);
}

.process-table tr:nth-child(even) {
  background: #fafafa;
}

.process-table tr:nth-child(even):hover {
  background: linear-gradient(135deg, #f1f3f4 0%, #e8eaed 100%);
}

.terminate-btn {
  background: linear-gradient(135deg, #d13438 0%, #a4262c 100%);
  color: white;
  border: none;
  padding: 6px 12px;
  cursor: pointer;
  font-size: 11px;
  font-weight: 600;
  border-radius: 6px;
  transition: all 0.2s ease;
  box-shadow: 0 2px 4px rgba(209, 52, 56, 0.2);
}

.terminate-btn:hover {
  background: linear-gradient(135deg, #a4262c 0%, #8b1f23 100%);
  box-shadow: 0 4px 8px rgba(209, 52, 56, 0.3);
  transform: translateY(-1px);
}

.terminate-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(209, 52, 56, 0.2);
}

.memory-address {
  font-family: 'Cascadia Code', 'Courier New', monospace;
  font-weight: 600;
  color: #0078d4;
  background: linear-gradient(135deg, #f3f9fd 0%, #e1f5fe 100%);
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 11px;
  border: 1px solid #bee1f4;
}

.no-memory {
  color: #8a8886;
  font-style: italic;
}

.scheduler-config {
  padding: 16px 20px;
  border-bottom: 2px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.05);
}

.scheduler-config h3 {
  margin: 0 0 12px 0;
  color: #323130;
  font-weight: 600;
  font-size: 15px;
}

/* 旧的config-form样式已被新样式替换 */

.gantt-section {
  padding: 16px 20px;
  border-top: 2px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.05);
}

.gantt-section h4 {
  margin: 0 0 12px 0;
  color: #323130;
  font-weight: 600;
  font-size: 14px;
}

.gantt-chart {
  position: relative;
  margin-top: 12px;
  height: 50px;
  border: 2px solid #e1e5e9;
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  border-radius: 8px;
  overflow: hidden;
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.05);
}

.gantt-scroll-container {
  position: relative;
  overflow-x: auto;
  overflow-y: hidden;
  height: 100%;
  width: 100%;
  scrollbar-width: thin;
  scrollbar-color: #c7c6c4 #f1f1f1;
}

.gantt-scroll-container::-webkit-scrollbar {
  height: 6px;
}

.gantt-scroll-container::-webkit-scrollbar-track {
  background: #f1f1f1;
  border-radius: 3px;
}

.gantt-scroll-container::-webkit-scrollbar-thumb {
  background: linear-gradient(135deg, #c7c6c4 0%, #a19f9d 100%);
  border-radius: 3px;
  transition: background 0.2s ease;
}

.gantt-scroll-container::-webkit-scrollbar-thumb:hover {
  background: linear-gradient(135deg, #a19f9d 0%, #8a8886 100%);
}

.gantt-bar {
  position: relative;
  height: 100%;
}

.gantt-block {
  position: absolute;
  top: 0;
  height: 100%;
  color: #ffffff;
  font-size: 12px;
  font-weight: 600;
  text-align: center;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  border-right: 2px solid #ffffff;
  display: flex;
  align-items: center;
  justify-content: center;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.3);
  transition: all 0.2s ease;
}

.gantt-block:hover {
  transform: scaleY(1.1);
  z-index: 5;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
}

/* 顶部调度器区域样式优化 */
.top-config {
  border-bottom: 2px solid #0078d4;
  background: linear-gradient(135deg, #f3f9fd 0%, #e1f5fe 100%);
  box-shadow: 0 2px 6px rgba(0, 120, 212, 0.1);
  padding: 12px 20px;
}

.gantt-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.refresh-btn {
  background: linear-gradient(135deg, #f3f2f1 0%, #e1dfdd 100%);
  border: 1px solid #d2d0ce;
  border-radius: 6px;
  padding: 6px 12px;
  cursor: pointer;
  color: #323130;
  font-size: 12px;
  font-weight: 500;
  transition: all 0.2s ease;
  display: flex;
  align-items: center;
  gap: 4px;
}

.refresh-btn:hover {
  background: linear-gradient(135deg, #e1dfdd 0%, #d2d0ce 100%);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
  transform: translateY(-1px);
}

.refresh-btn:active {
  transform: translateY(0);
  box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.gantt-time-axis {
  display: flex;
  justify-content: space-between;
  margin-top: 8px;
}

.time-marker {
  font-size: 12px;
  color: #605e5c;
}

.empty-content {
  text-align: center;
  padding: 20px;
  background: #ffffff;
  border-radius: 8px;
  border: 2px dashed #e1e5e9;
}

.empty-icon {
  font-size: 40px;
  margin-bottom: 12px;
}

.empty-tip {
  font-size: 12px;
  color: #8a8886;
  font-style: italic;
}

/* 通用滚动条美化 */
::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}

::-webkit-scrollbar-track {
  background: #f1f1f1;
  border-radius: 4px;
}

::-webkit-scrollbar-thumb {
  background: linear-gradient(135deg, #c7c6c4 0%, #a19f9d 100%);
  border-radius: 4px;
  transition: background 0.2s ease;
}

::-webkit-scrollbar-thumb:hover {
  background: linear-gradient(135deg, #a19f9d 0%, #8a8886 100%);
}

.scroll-hint {
  position: absolute;
  top: 50%;
  right: 10px;
  transform: translateY(-50%);
  background: rgba(0, 0, 0, 0.7);
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 10px;
  white-space: nowrap;
  pointer-events: none;
  opacity: 0.8;
  animation: fadeInOut 3s ease-in-out infinite;
}

@keyframes fadeInOut {
  0%, 100% { opacity: 0.3; }
  50% { opacity: 0.8; }
}

/* 功能按钮区域样式 */
.feature-buttons {
  background: linear-gradient(135deg, #ffffff 0%, #f8f9fa 100%);
  border-bottom: 1px solid #e1e5e9;
  padding: 16px 20px;
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  justify-content: center;
}

.feature-btn {
  padding: 10px 20px;
  font-size: 14px;
  color: #ffffff;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s ease;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 6px;
  min-width: 140px;
  justify-content: center;
}

.feature-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 12px rgba(0, 0, 0, 0.2);
}

.state-btn {
  background: linear-gradient(135deg, #6c5ce7 0%, #a29bfe 100%);
}

.state-btn:hover {
  background: linear-gradient(135deg, #5f3dc4 0%, #6c5ce7 100%);
}

.child-btn {
  background: linear-gradient(135deg, #fd79a8 0%, #fdcb6e 100%);
}

.child-btn:hover {
  background: linear-gradient(135deg, #e84393 0%, #fd79a8 100%);
}

.relation-btn {
  background: linear-gradient(135deg, #00b894 0%, #00cec9 100%);
}

.relation-btn:hover {
  background: linear-gradient(135deg, #00a085 0%, #00b894 100%);
}

.create-btn {
  background: linear-gradient(135deg, #0078d4 0%, #005a9e 100%);
}

.create-btn:hover {
  background: linear-gradient(135deg, #005a9e 0%, #004578 100%);
}

.queue-btn {
  background: linear-gradient(135deg, #9b59b6 0%, #8e44ad 100%);
}

.queue-btn:hover {
  background: linear-gradient(135deg, #8e44ad 0%, #7d3c98 100%);
}

/* 对话框样式 */
.dialog-overlay {
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: rgba(0, 0, 0, 0.5);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 1000;
  backdrop-filter: blur(4px);
}

.dialog {
  background: #ffffff;
  border-radius: 12px;
  box-shadow: 0 10px 30px rgba(0, 0, 0, 0.3);
  width: 90%;
  max-width: 500px;
  max-height: 80vh;
  overflow-y: auto;
  animation: dialogSlideIn 0.3s ease;
}

@keyframes dialogSlideIn {
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
  justify-content: space-between;
  align-items: center;
  padding: 20px;
  border-bottom: 1px solid #e1e5e9;
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
}

.dialog-header h3 {
  margin: 0;
  color: #323130;
  font-weight: 600;
  font-size: 18px;
}

.close-btn {
  background: none;
  border: none;
  font-size: 24px;
  color: #605e5c;
  cursor: pointer;
  padding: 0;
  width: 30px;
  height: 30px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  transition: all 0.2s ease;
}

.close-btn:hover {
  background: rgba(0, 0, 0, 0.1);
  color: #323130;
}

.dialog-body {
  padding: 20px;
}

.form-group {
  margin-bottom: 16px;
}

.form-group label {
  display: block;
  margin-bottom: 6px;
  font-weight: 600;
  color: #323130;
  font-size: 14px;
}

.form-group select,
.form-group input {
  width: 100%;
  padding: 10px 12px;
  font-size: 14px;
  color: #323130;
  border: 2px solid #e1e5e9;
  border-radius: 6px;
  background: #ffffff;
  transition: all 0.2s ease;
  box-sizing: border-box;
}

.form-group select:focus,
.form-group input:focus {
  outline: none;
  border-color: #0078d4;
  box-shadow: 0 0 0 2px rgba(0, 120, 212, 0.2);
}

.form-help {
  margin-top: 16px;
  padding: 12px;
  background: #f8f9fa;
  border-radius: 6px;
  border-left: 4px solid #0078d4;
}

.form-help p {
  margin: 0 0 8px 0;
  font-weight: 600;
  color: #323130;
}

.form-help ul {
  margin: 0;
  padding-left: 20px;
}

.form-help li {
  margin-bottom: 4px;
  font-size: 13px;
  color: #605e5c;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
  padding: 20px;
  border-top: 1px solid #e1e5e9;
  background: #f8f9fa;
}

.cancel-btn,
.confirm-btn {
  padding: 10px 20px;
  font-size: 14px;
  font-weight: 600;
  border: none;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.2s ease;
  min-width: 80px;
}

.cancel-btn {
  background: #f3f2f1;
  color: #323130;
  border: 1px solid #e1dfdd;
}

.cancel-btn:hover {
  background: #e1dfdd;
}

.confirm-btn {
  background: linear-gradient(135deg, #0078d4 0%, #106abe 100%);
  color: #ffffff;
}

.confirm-btn:hover {
  background: linear-gradient(135deg, #106abe 0%, #005a9e 100%);
  transform: translateY(-1px);
  box-shadow: 0 4px 8px rgba(0, 120, 212, 0.3);
}

/* 进程表格美化样式 */
.process-row {
  transition: all 0.2s ease;
  position: relative;
}

.process-row:hover {
  background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
  transform: translateY(-1px);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

/* 父子进程层次展示 */
.pid-cell {
  position: relative;
  min-width: 120px;
}

.pid-container {
  display: flex;
  align-items: center;
  position: relative;
}

.process-tree {
  position: absolute;
  left: -12px;
  top: 50%;
  transform: translateY(-50%);
  display: flex;
  align-items: center;
}

.tree-line {
  width: 2px;
  height: 20px;
  background: linear-gradient(180deg, #d2d0ce 0%, #8a8886 50%, #d2d0ce 100%);
  margin-right: 4px;
  border-radius: 1px;
}

.tree-connector {
  color: #8a8886;
  font-weight: 600;
  font-size: 12px;
  margin-right: 8px;
}

.pid-badge {
  display: flex;
  flex-direction: column;
  align-items: center;
  background: linear-gradient(135deg, #ffffff 0%, #f3f2f1 100%);
  border: 2px solid #e1dfdd;
  border-radius: 8px;
  padding: 6px 8px;
  min-width: 60px;
  transition: all 0.2s ease;
  position: relative;
}

.pid-badge:hover {
  border-color: #0078d4;
  box-shadow: 0 2px 8px rgba(0, 120, 212, 0.2);
}

.pid-number {
  font-weight: 700;
  font-size: 14px;
  color: #323130;
  line-height: 1;
}

.priority-indicator {
  font-size: 10px;
  font-weight: 600;
  color: #605e5c;
  margin-top: 2px;
  line-height: 1;
}

/* 优先级颜色 */
.priority-1 { border-color: #d83b01; }
.priority-1 .priority-indicator { color: #d83b01; }
.priority-2 { border-color: #ff8c00; }
.priority-2 .priority-indicator { color: #ff8c00; }
.priority-3 { border-color: #107c10; }
.priority-3 .priority-indicator { color: #107c10; }
.priority-4 { border-color: #0078d4; }
.priority-4 .priority-indicator { color: #0078d4; }
.priority-5 { border-color: #8a8886; }
.priority-5 .priority-indicator { color: #8a8886; }

/* 进程名称单元格 */
.process-name-cell {
  min-width: 150px;
}

.process-name {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.name-text {
  font-weight: 600;
  color: #323130;
  font-size: 14px;
}

.child-indicator {
  font-size: 11px;
  color: #8a8886;
  background: #f3f2f1;
  padding: 2px 6px;
  border-radius: 10px;
  width: fit-content;
  font-weight: 500;
}

/* 父进程单元格 */
.root-process {
  color: #107c10;
  font-weight: 600;
  font-size: 12px;
  background: linear-gradient(135deg, #f0f9ff 0%, #e6f7ff 100%);
  padding: 4px 8px;
  border-radius: 12px;
  border: 1px solid #b3e5fc;
}

.parent-pid {
  color: #0078d4;
  font-weight: 600;
  background: #f8f9fa;
  padding: 4px 8px;
  border-radius: 6px;
  border: 1px solid #e1e5e9;
}

/* 状态单元格美化 */
.state-cell {
  min-width: 100px;
}

.state-badge {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 12px;
  border-radius: 16px;
  font-weight: 600;
  font-size: 12px;
  width: fit-content;
  transition: all 0.2s ease;
}

.state-indicator {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.6; }
}

.state-new {
  background: linear-gradient(135deg, #f3f2f1 0%, #e1dfdd 100%);
  color: #605e5c;
  border: 1px solid #d2d0ce;
}

.state-new .state-indicator {
  background: #8a8886;
}

.state-ready {
  background: linear-gradient(135deg, #e1f5fe 0%, #b3e5fc 100%);
  color: #0078d4;
  border: 1px solid #81d4fa;
}

.state-ready .state-indicator {
  background: #0078d4;
}

.state-running {
  background: linear-gradient(135deg, #e8f5e8 0%, #c8e6c9 100%);
  color: #107c10;
  border: 1px solid #a5d6a7;
}

.state-running .state-indicator {
  background: #107c10;
}

.state-blocked {
  background: linear-gradient(135deg, #fff3e0 0%, #ffcc02 100%);
  color: #d83b01;
  border: 1px solid #ffb74d;
}

.state-blocked .state-indicator {
  background: #d83b01;
}

.state-terminated {
  background: linear-gradient(135deg, #fafafa 0%, #e0e0e0 100%);
  color: #605e5c;
  border: 1px solid #bdbdbd;
}

.state-terminated .state-indicator {
  background: #605e5c;
}

/* CPU时间单元格 */
.cpu-cell {
  min-width: 80px;
}

.cpu-time {
  font-weight: 600;
  color: #323130;
}

.time-unit {
  font-size: 11px;
  color: #8a8886;
  margin-left: 2px;
}

/* 优先级显示 */
.priority-cell {
  min-width: 60px;
}

.priority-display {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border-radius: 50%;
  font-weight: 700;
  font-size: 14px;
  margin: 0 auto;
  transition: all 0.2s ease;
}

.priority-level-1 {
  background: linear-gradient(135deg, #ffebee 0%, #ffcdd2 100%);
  color: #d83b01;
  border: 2px solid #ef5350;
}

.priority-level-2 {
  background: linear-gradient(135deg, #fff3e0 0%, #ffe0b2 100%);
  color: #ff8c00;
  border: 2px solid #ff9800;
}

.priority-level-3 {
  background: linear-gradient(135deg, #e8f5e8 0%, #c8e6c9 100%);
  color: #107c10;
  border: 2px solid #4caf50;
}

.priority-level-4 {
  background: linear-gradient(135deg, #e3f2fd 0%, #bbdefb 100%);
  color: #0078d4;
  border: 2px solid #2196f3;
}

.priority-level-5 {
  background: linear-gradient(135deg, #f5f5f5 0%, #e0e0e0 100%);
  color: #8a8886;
  border: 2px solid #9e9e9e;
}

/* 内存信息单元格 */
.memory-cell {
  min-width: 120px;
}

.memory-info .memory-address {
  font-family: 'Courier New', monospace;
  font-size: 12px;
  font-weight: 600;
  color: #0078d4;
  background: #f8f9fa;
  padding: 4px 8px;
  border-radius: 4px;
  border: 1px solid #e1e5e9;
}

.no-memory .no-memory-text {
  color: #8a8886;
  font-style: italic;
  font-size: 12px;
}

/* 内存大小单元格 */
.size-cell {
  min-width: 80px;
}

.memory-size {
  display: flex;
  align-items: baseline;
  gap: 2px;
  justify-content: center;
}

.size-number {
  font-weight: 600;
  color: #323130;
  font-size: 14px;
}

.size-unit {
  font-size: 11px;
  color: #8a8886;
}

/* 操作按钮美化 */
.action-cell {
  min-width: 80px;
}

.terminate-btn {
  display: flex;
  align-items: center;
  gap: 4px;
  background: linear-gradient(135deg, #ffebee 0%, #ffcdd2 100%);
  color: #d83b01;
  border: 1px solid #ef5350;
  border-radius: 6px;
  padding: 6px 12px;
  cursor: pointer;
  font-size: 12px;
  font-weight: 600;
  transition: all 0.2s ease;
}

.terminate-btn:hover {
  background: linear-gradient(135deg, #ffcdd2 0%, #e57373 100%);
  transform: translateY(-1px);
  box-shadow: 0 4px 8px rgba(211, 59, 1, 0.2);
}

.terminate-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(211, 59, 1, 0.2);
}

.btn-icon {
  font-size: 12px;
}

.btn-text {
  font-size: 11px;
}

/* 行状态样式 */
.state-row-running {
  background: linear-gradient(135deg, #f0fff4 0%, #e8f5e8 100%);
  border-left: 4px solid #107c10;
}

.state-row-blocked {
  background: linear-gradient(135deg, #fffaf0 0%, #fff3e0 100%);
  border-left: 4px solid #d83b01;
}

.state-row-terminated {
  background: linear-gradient(135deg, #fafafa 0%, #f5f5f5 100%);
  border-left: 4px solid #8a8886;
  opacity: 0.7;
}

/* 对话框表单美化 */
.form-note {
  font-size: 12px;
  color: #107c10;
  margin-top: 4px;
  font-weight: 500;
  background: #f0f9ff;
  padding: 4px 8px;
  border-radius: 4px;
  border-left: 3px solid #0078d4;
}

.form-group input[readonly] {
  background: #f8f9fa;
  color: #605e5c;
  cursor: not-allowed;
  border-style: dashed;
}

.highlight {
  background: linear-gradient(135deg, #fff3cd 0%, #ffeaa7 100%);
  padding: 2px 4px;
  border-radius: 3px;
  font-weight: 600;
  color: #856404;
}

/* 紧凑调度器配置样式 */
.config-container {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.config-main-row {
  display: flex;
  align-items: center;
  gap: 20px;
  flex-wrap: wrap;
}

.config-title {
  font-size: 16px;
  font-weight: 600;
  color: #323130;
  margin: 0;
  flex-shrink: 0;
}

.config-controls {
  display: flex;
  align-items: center;
  gap: 16px;
  flex-wrap: wrap;
  flex: 1;
}

.control-group {
  display: flex;
  align-items: center;
  gap: 8px;
}

.control-group label {
  font-weight: 600;
  color: #323130;
  font-size: 13px;
  white-space: nowrap;
}

.control-group select {
  padding: 6px 10px;
  border: 2px solid #e1e5e9;
  border-radius: 4px;
  background: #ffffff;
  font-size: 13px;
  color: #323130;
  transition: all 0.2s ease;
  min-width: 100px;
}

.control-group select:focus {
  outline: none;
  border-color: #0078d4;
  box-shadow: 0 0 0 2px rgba(0, 120, 212, 0.2);
}

.control-group select:disabled {
  background: #f3f2f1;
  color: #8a8886;
  cursor: not-allowed;
}

.algorithm-hint {
  font-size: 11px;
  color: #605e5c;
  font-style: italic;
  background: #f0f9ff;
  padding: 2px 6px;
  border-radius: 3px;
  border-left: 2px solid #0078d4;
  white-space: nowrap;
}

.timeslice-input {
  width: 60px;
  padding: 6px 8px;
  border: 2px solid #e1e5e9;
  border-radius: 4px;
  font-size: 13px;
  text-align: center;
  transition: all 0.2s ease;
}

.timeslice-input:focus {
  outline: none;
  border-color: #0078d4;
  box-shadow: 0 0 0 2px rgba(0, 120, 212, 0.2);
}

.timeslice-input:disabled {
  background: #f3f2f1;
  color: #8a8886;
  cursor: not-allowed;
}

.unit-label {
  font-size: 11px;
  color: #8a8886;
  font-weight: 500;
}

.control-group.actions {
  gap: 8px;
}

.update-btn, .schedule-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 12px;
  border: none;
  border-radius: 4px;
  font-size: 12px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
  justify-content: center;
}

.update-btn.compact, .schedule-btn.compact {
  min-width: 80px;
  padding: 6px 10px;
}

.update-btn {
  background: linear-gradient(135deg, #107c10 0%, #0e6e0e 100%);
  color: #ffffff;
  box-shadow: 0 1px 3px rgba(16, 124, 16, 0.2);
}

.update-btn:hover:not(:disabled) {
  background: linear-gradient(135deg, #0e6e0e 0%, #0c5c0c 100%);
  box-shadow: 0 2px 6px rgba(16, 124, 16, 0.3);
  transform: translateY(-1px);
}

.update-btn:disabled {
  background: #f3f2f1;
  color: #8a8886;
  cursor: not-allowed;
  transform: none;
  box-shadow: none;
}

.schedule-btn {
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  color: #ffffff;
  box-shadow: 0 1px 3px rgba(0, 120, 212, 0.2);
}

.schedule-btn:hover:not(:disabled) {
  background: linear-gradient(135deg, #106ebe 0%, #005a9e 100%);
  box-shadow: 0 2px 6px rgba(0, 120, 212, 0.3);
  transform: translateY(-1px);
}

.schedule-btn:disabled {
  background: #f3f2f1;
  color: #8a8886;
  cursor: not-allowed;
  transform: none;
  box-shadow: none;
}

.config-status-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: #f8f9fa;
  padding: 6px 12px;
  border-radius: 4px;
  border: 1px solid #e1e5e9;
  font-size: 12px;
}

.current-status {
  display: flex;
  align-items: center;
  gap: 6px;
}

.status-label {
  color: #8a8886;
  font-weight: 500;
}

.status-value {
  color: #323130;
  font-weight: 600;
  background: #ffffff;
  padding: 2px 6px;
  border-radius: 3px;
  border: 1px solid #e1e5e9;
}

.time-slice-value {
  color: #605e5c;
  font-weight: 500;
}

.config-indicator {
  display: flex;
  align-items: center;
  gap: 4px;
  color: #8a8886;
  transition: all 0.2s ease;
}

.config-indicator.active {
  color: #d83b01;
}

.indicator-dot {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  background: #8a8886;
  transition: all 0.2s ease;
}

.config-indicator.active .indicator-dot {
  background: #d83b01;
  animation: pulse 2s infinite;
}

.indicator-text {
  font-size: 11px;
}

/* 加载动画 */
.loading-spinner {
  width: 16px;
  height: 16px;
  border: 2px solid rgba(255, 255, 255, 0.3);
  border-top: 2px solid #ffffff;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

/* 就绪队列对话框样式 */
.ready-queue-dialog {
  max-width: 700px;
  width: 95%;
}

.queue-info {
  margin-bottom: 16px;
}

.queue-stats {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: #f8f9fa;
  padding: 12px;
  border-radius: 6px;
  border: 1px solid #e1e5e9;
}

.queue-count {
  font-weight: 600;
  color: #323130;
  font-size: 14px;
}

.refresh-queue-btn {
  background: linear-gradient(135deg, #f3f2f1 0%, #e1dfdd 100%);
  border: 1px solid #d2d0ce;
  border-radius: 4px;
  padding: 6px 12px;
  cursor: pointer;
  color: #323130;
  font-size: 12px;
  font-weight: 500;
  transition: all 0.2s ease;
}

.refresh-queue-btn:hover {
  background: linear-gradient(135deg, #e1dfdd 0%, #d2d0ce 100%);
  transform: translateY(-1px);
}

.empty-queue-dialog {
  text-align: center;
  padding: 40px 20px;
  background: #f8f9fa;
  border-radius: 8px;
  border: 2px dashed #d2d0ce;
  margin: 20px 0;
}

.empty-queue-dialog .empty-icon {
  font-size: 48px;
  margin-bottom: 16px;
  opacity: 0.6;
}

.empty-queue-dialog .empty-text {
  font-size: 18px;
  color: #605e5c;
  font-weight: 600;
  margin-bottom: 8px;
}

.empty-queue-dialog .empty-tip {
  font-size: 14px;
  color: #8a8886;
  font-style: italic;
}

.ready-queue-list {
  max-height: 400px;
  overflow-y: auto;
  padding: 8px 0;
}

.ready-queue-item-dialog {
  display: flex;
  align-items: center;
  gap: 16px;
  background: #ffffff;
  border: 2px solid #e1e5e9;
  border-radius: 8px;
  padding: 16px;
  margin-bottom: 12px;
  transition: all 0.2s ease;
}

.ready-queue-item-dialog:hover {
  border-color: #0078d4;
  box-shadow: 0 4px 12px rgba(0, 120, 212, 0.1);
  transform: translateY(-2px);
}

.queue-position {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  border-radius: 50%;
  color: #ffffff;
  font-weight: 700;
  font-size: 14px;
  flex-shrink: 0;
}

.queue-process-info {
  flex-grow: 1;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.process-header {
  display: flex;
  align-items: center;
  gap: 12px;
}

.process-pid {
  font-weight: 700;
  color: #323130;
  font-size: 16px;
}

.process-name {
  background: #f8f9fa;
  color: #605e5c;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 12px;
  font-weight: 500;
}

.process-details {
  display: flex;
  gap: 16px;
  flex-wrap: wrap;
}

.detail-item {
  font-size: 12px;
  color: #8a8886;
}

.detail-item strong {
  color: #323130;
  font-weight: 600;
}

.queue-status {
  display: flex;
  align-items: center;
  flex-shrink: 0;
}

.ready-indicator {
  display: flex;
  align-items: center;
  gap: 6px;
  background: linear-gradient(135deg, #e1f5fe 0%, #b3e5fc 100%);
  color: #0078d4;
  padding: 6px 12px;
  border-radius: 16px;
  font-size: 12px;
  font-weight: 600;
  border: 1px solid #81d4fa;
}

.pulse-dot {
  width: 8px;
  height: 8px;
  background: #0078d4;
  border-radius: 50%;
  animation: pulse 2s infinite;
}

.queue-help {
  background: #f0f9ff;
  border-left: 4px solid #0078d4;
  padding: 16px;
  border-radius: 6px;
  margin-top: 20px;
}

.queue-help p {
  margin: 0 0 12px 0;
  font-weight: 600;
  color: #323130;
  font-size: 14px;
}

.queue-help ul {
  margin: 0;
  padding-left: 20px;
}

.queue-help li {
  margin-bottom: 6px;
  font-size: 13px;
  color: #605e5c;
  line-height: 1.4;
}

/* 进程关系对话框样式 */
.relationships-dialog {
  min-width: 600px;
  max-width: 800px;
  width: 90vw;
}

.relationships-info {
  margin-bottom: 20px;
}

.relationships-stats {
  display: flex;
  justify-content: space-between;
  align-items: center;
  background: #f8f9fa;
  padding: 12px;
  border-radius: 6px;
  border: 1px solid #e1e5e9;
}

.relationships-count {
  font-weight: 600;
  color: #323130;
  font-size: 14px;
}

.refresh-relationships-btn {
  background: linear-gradient(135deg, #f3f2f1 0%, #e1dfdd 100%);
  border: 1px solid #d2d0ce;
  border-radius: 4px;
  padding: 6px 12px;
  cursor: pointer;
  color: #323130;
  font-size: 12px;
  font-weight: 500;
  transition: all 0.2s ease;
}

.refresh-relationships-btn:hover {
  background: linear-gradient(135deg, #e1dfdd 0%, #d2d0ce 100%);
  transform: translateY(-1px);
}

.empty-relationships-dialog {
  text-align: center;
  padding: 40px 20px;
  background: #f8f9fa;
  border-radius: 8px;
  border: 2px dashed #d2d0ce;
  margin: 20px 0;
}

.empty-relationships-dialog .empty-icon {
  font-size: 48px;
  margin-bottom: 16px;
  opacity: 0.6;
}

.empty-relationships-dialog .empty-text {
  font-size: 18px;
  color: #605e5c;
  font-weight: 600;
  margin-bottom: 8px;
}

.empty-relationships-dialog .empty-tip {
  font-size: 14px;
  color: #8a8886;
  font-style: italic;
}

.relationships-list {
  max-height: 400px;
  overflow-y: auto;
  padding: 8px 0;
}

.relationship-item-dialog {
  display: flex;
  align-items: flex-start;
  gap: 16px;
  background: #ffffff;
  border: 2px solid #e1e5e9;
  border-radius: 8px;
  padding: 16px;
  margin-bottom: 12px;
  transition: all 0.2s ease;
}

.relationship-item-dialog:hover {
  border-color: #0078d4;
  box-shadow: 0 4px 12px rgba(0, 120, 212, 0.1);
  transform: translateY(-2px);
}

.relationship-index {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 40px;
  height: 40px;
  background: linear-gradient(135deg, #0078d4 0%, #106ebe 100%);
  border-radius: 50%;
  color: #ffffff;
  font-weight: 700;
  font-size: 14px;
  flex-shrink: 0;
}

.relationship-details {
  flex-grow: 1;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.relationship-header {
  width: 100%;
}

.process-pair {
  display: flex;
  align-items: center;
  gap: 16px;
  justify-content: space-between;
}

.process-info {
  display: flex;
  flex-direction: column;
  gap: 4px;
  flex: 1;
}

.process-label {
  font-size: 12px;
  color: #8a8886;
  font-weight: 500;
}

.process-pid {
  font-weight: 700;
  color: #323130;
  font-size: 14px;
}

.process-name {
  background: #f8f9fa;
  color: #605e5c;
  padding: 2px 6px;
  border-radius: 4px;
  font-size: 11px;
  font-weight: 500;
  align-self: flex-start;
}

.relationship-arrow {
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 24px;
  flex-shrink: 0;
}

.sync-arrow {
  color: #107c10;
  animation: rotate 2s linear infinite;
}

.mutex-arrow {
  color: #d83b01;
}

.relationship-type-info {
  width: 100%;
}

.relationship-type-badge {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-radius: 6px;
  font-size: 13px;
  font-weight: 600;
}

.relationship-type-badge.type-sync {
  background: linear-gradient(135deg, #f0fff4 0%, #e8f5e8 100%);
  color: #107c10;
  border: 1px solid #a4de6c;
}

.relationship-type-badge.type-mutex {
  background: linear-gradient(135deg, #ffebee 0%, #ffcdd2 100%);
  color: #d83b01;
  border: 1px solid #ef5350;
}

.type-icon {
  font-size: 16px;
}

.type-text {
  font-weight: 700;
}

.type-desc {
  font-size: 11px;
  opacity: 0.8;
  font-weight: 400;
}

.relationships-help {
  background: #f0f9ff;
  border-left: 4px solid #0078d4;
  padding: 16px;
  border-radius: 6px;
  margin-top: 20px;
}

.relationships-help p {
  margin: 0 0 12px 0;
  font-weight: 600;
  color: #323130;
  font-size: 14px;
}

.relationships-help ul {
  margin: 0;
  padding-left: 20px;
}

.relationships-help li {
  margin-bottom: 6px;
  font-size: 13px;
  color: #605e5c;
  line-height: 1.4;
}

.create-relation-btn {
  background: linear-gradient(135deg, #107c10 0%, #0e6e0e 100%);
  color: white;
  border: none;
  padding: 8px 16px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  border-radius: 6px;
  transition: all 0.2s ease;
  box-shadow: 0 2px 4px rgba(16, 124, 16, 0.2);
}

.create-relation-btn:hover {
  background: linear-gradient(135deg, #0e6e0e 0%, #0c5c0c 100%);
  box-shadow: 0 4px 8px rgba(16, 124, 16, 0.3);
  transform: translateY(-1px);
}

.create-relation-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(16, 124, 16, 0.2);
}

.relation-list-btn {
  background: linear-gradient(135deg, #673ab7 0%, #5e35b1 100%);
  color: white;
  border: none;
  padding: 8px 16px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  border-radius: 6px;
  transition: all 0.2s ease;
  box-shadow: 0 2px 4px rgba(103, 58, 183, 0.2);
}

.relation-list-btn:hover {
  background: linear-gradient(135deg, #5e35b1 0%, #512da8 100%);
  box-shadow: 0 4px 8px rgba(103, 58, 183, 0.3);
  transform: translateY(-1px);
}

.relation-list-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 4px rgba(103, 58, 183, 0.2);
}

@keyframes rotate {
  from {
    transform: rotate(0deg);
  }
  to {
    transform: rotate(360deg);
  }
}
</style>
