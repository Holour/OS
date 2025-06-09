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
  createProcess: (name: string, size: number) =>
    apiClient.post('/processes', { memory_size: size }),

  // 终止进程
  terminateProcess: (pid: number) =>
    apiClient.delete(`/processes/${pid}`),

  // 注意：后端没有实现获取单个进程详情的API
  // getProcess: (pid: number) => apiClient.get(`/processes/${pid}`),
};

// 调度器 API
export const schedulerAPI = {
  // 获取调度器状态
  getStatus: () => apiClient.get('/scheduler/status'),

  // 执行调度tick
  tick: () => apiClient.post('/scheduler/tick'),

  // 注意：后端没有实现设置调度算法的API
  // setAlgorithm: (algorithm: string) => apiClient.put('/scheduler/algorithm', { algorithm }),

  // 获取就绪队列（修正路径为ready_queue）
  getReadyQueue: () => apiClient.get('/scheduler/ready_queue'),
};

// 内存管理 API
export const memoryAPI = {
  // 获取内存状态
  getStatus: () => apiClient.get('/memory/status'),

  // 分配内存
  allocate: (pid: number, size: number) =>
    apiClient.post('/memory/allocate', { pid, size }),

  // 释放内存
  deallocate: (pid: number) =>
    apiClient.delete(`/memory/deallocate/${pid}`),

  // 获取内存布局
  getLayout: () => apiClient.get('/memory/layout'),
};

// 文件系统 API
export const filesystemAPI = {
  // 获取文件系统状态
  getStatus: () => apiClient.get('/filesystem/status'),

  // 列出目录内容
  listDirectory: (path: string = '/') => {
    const cleanPath = path === '/' ? '' : path;
    return apiClient.get(`/filesystem/directory/${cleanPath}`);
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
  deviceOperation: (deviceId: number) =>
    apiClient.post(`/devices/${deviceId}/operation`),
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

  // 获取时钟状态
  getStatus: () => apiClient.get('/clock/status'),
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
