import axios from 'axios';

const apiClient = axios.create({
  baseURL: 'http://localhost:8080/api/v1',
  headers: {
    'Content-Type': 'application/json',
  },
});

// 进程管理 API
export const processAPI = {
  // 获取所有进程
  getProcesses: () => apiClient.get('/processes'),

  // 创建新进程
  createProcess: (...args: any[]) => {
    if (typeof args[0] === 'string') {
      // 旧签名 (name: string, size: number)
      const [, size] = args as [string, number];
      return apiClient.post('/processes', { memory_size: size });
    }

    // 新签名 (size: number, cpu_time?: number, priority?: number)
    const [size, cpu_time = 10, priority = 5] = args as [number, number?, number?];
    return apiClient.post('/processes', { memory_size: size, cpu_time, priority });
  },

  // 终止进程
  terminateProcess: (pid: number) =>
    apiClient.delete(`/processes/${pid}`),

  // 注意：后端没有实现获取单个进程详情的API
  // getProcess: (pid: number) => apiClient.get(`/processes/${pid}`),
};

// 调度器 API
export const schedulerAPI = {
  // 执行调度tick
  tick: () => apiClient.post('/scheduler/tick'),

  // 获取就绪队列（修正路径为ready_queue）
  getReadyQueue: () => apiClient.get('/scheduler/ready_queue'),

  // 获取当前调度器配置（算法与时间片）
  getStatus: () => apiClient.get('/scheduler/config'),

  // 更新调度器配置
  setConfig: (algorithm: string, time_slice?: number) =>
    apiClient.put('/scheduler/config', { algorithm, time_slice }),

  // 获取甘特图数据
  getGanttChart: () => apiClient.get('/scheduler/gantt_chart'),
};

// 内存管理 API
export const memoryAPI = {
  // 获取内存状态
  getStatus: () => apiClient.get('/memory/status'),

  // 设置内存分配策略 - 新增功能
  setStrategy: (strategy: number) =>
    apiClient.put('/memory/strategy', { strategy }),

  // 注意：以下API在后端中不存在，仅保留注释
  // allocate: (pid: number, size: number) =>
  //   apiClient.post('/memory/allocate', { pid, size }),

  // deallocate: (pid: number) =>
  //   apiClient.delete(`/memory/deallocate/${pid}`),

  // getLayout: () => apiClient.get('/memory/layout'),
};

// 文件系统 API
export const filesystemAPI = {
  // 获取文件系统状态
  getStatus: () => apiClient.get('/filesystem/status'),

  // 列出目录内容
  // 后端路由形式为 /filesystem/directory/{path} ，当 path 为根目录时可以省略；
  // 其余情况需去掉开头的多个 '/' 并做 URL 编码，防止出现双斜杠以及特殊字符问题。
  listDirectory: (path: string = '/') => {
    if (path === '/' || path === '') {
      return apiClient.get('/filesystem/directory/');
    }

    const trimmed = path.replace(/^\/+/, '');
    const encoded = encodeURIComponent(trimmed);
    return apiClient.get(`/filesystem/directory/${encoded}`);
  },

  // 创建文件（支持模拟大小）
  createFile: (path: string, simulatedSize: number = 0, permissions: number = 0o644) =>
    apiClient.post('/filesystem/file', {
      path,
      simulated_size: simulatedSize,
      permissions
    }),

  // 创建目录
  createDirectory: (path: string, permissions: number = 0o755) =>
    apiClient.post('/filesystem/directory', {
      path,
      permissions
    }),

  // 读取文件
  readFile: (path: string) =>
    apiClient.get(`/filesystem/file/${encodeURIComponent(path)}`),

  // 删除文件/目录（支持递归删除）
  delete: (path: string, recursive: boolean = false) => {
    const url = `/filesystem/${encodeURIComponent(path)}`;
    return recursive
      ? apiClient.delete(`${url}?recursive=true`)
      : apiClient.delete(url);
  },

  // 获取文件系统配置并设置分配策略
  setAllocationStrategy: (strategy: 'INDEXED' | 'LINKED' | 'CONTIGUOUS') =>
    apiClient.put('/filesystem/config', { allocation_method: strategy }),

  // 获取文件地址信息
  getFileAddress: (path: string) =>
    apiClient.get('/filesystem/file-address', { params: { path } }),

  // 获取文件系统日志
  getLogs: (startTime?: string, endTime?: string, operationType?: string) => {
    const params: any = {};
    if (startTime) params.start_time = startTime;
    if (endTime) params.end_time = endTime;
    if (operationType) params.operation_type = operationType;
    return apiClient.get('/filesystem/logs', { params });
  },
};

// 设备管理 API
export const deviceAPI = {
  // 获取所有设备
  getDevices: () => apiClient.get('/devices'),

  // 注意：后端没有实现获取设备状态的API
  // getDeviceStatus: (deviceId: string) => apiClient.get(`/devices/${deviceId}/status`),

  // 设备请求（修正为后端实际端点）
  requestDevice: (deviceType: string) =>
    apiClient.post('/devices/request', { device_type: deviceType }),

  // 设备释放（修正为后端实际端点）
  releaseDevice: (deviceId: number) =>
    apiClient.post(`/devices/${deviceId}/release`),

  // 设备操作（修正为后端实际端点）
  deviceOperation: (deviceId: number, operation: string = 'TEST', params: any = {}) =>
    apiClient.post(`/devices/${deviceId}/operation`, {
      operation,
      params
    }),
};

// 中断管理 API
export const interruptAPI = {
  // 获取中断向量表（修正为后端实际端点）
  getInterruptTable: () => apiClient.get('/interrupts/vector_table'),

  // 触发中断
  triggerInterrupt: (interruptId: number, data?: any) =>
    apiClient.post('/interrupts/trigger', { vector: interruptId, data }),

  // 设置中断处理程序
  setHandler: (interruptId: number, handler: string) =>
    apiClient.post('/interrupts/handler', { vector: interruptId, handler_type: handler }),

  // 注意：后端没有实现中断历史API
  // getHistory: () => apiClient.get('/interrupts/history'),
};

// 时钟管理 API
export const clockAPI = {
  // 获取系统时间
  getTime: () => apiClient.get('/clock/time'),

  // 设置时钟间隔（修正为POST方法）
  setInterval: (interval: number) =>
    apiClient.post('/clock/interval', { interval }),

  // 设置定时器
  setTimer: (delay: number, repeat: boolean = false, interval?: number) =>
    apiClient.post('/clock/timer', { delay, repeat, interval }),
};

// 系统控制 API
export const systemAPI = {
  // 获取系统状态
  getStatus: () => apiClient.get('/system/status'),

  // 系统关机
  shutdown: () => apiClient.post('/system/shutdown'),

  // 系统重启
  reboot: () => apiClient.post('/system/reboot'),
};

export default apiClient;
