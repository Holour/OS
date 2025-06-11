<script setup lang="ts">
import { ref, nextTick } from 'vue';
import { interruptAPI, filesystemAPI, processAPI, memoryAPI, deviceAPI, clockAPI } from '@/services/api';

interface HistoryItem {
  id: number;
  type: 'command' | 'response' | 'error' | 'info';
  content: string | object;
}

const history = ref<HistoryItem[]>([
    { id: 0, type: 'info', content: 'Welcome to OS-Simulator Terminal.' },
    { id: 1, type: 'info', content: 'Type "help" to see available commands.' }
]);
const command = ref('');
const historyEl = ref<HTMLElement | null>(null);
const currentDirectory = ref('/');
let historyId = 2;

const executeCommand = async () => {
  const [cmd, ...args] = command.value.trim().split(/\s+/);
  if (!cmd) return;

  addToHistory('command', `[${currentDirectory.value}]> ${command.value}`);
  command.value = '';

  try {
    switch (cmd.toLowerCase()) {
      // 基本命令
      case 'help':
        showHelp();
        break;
      case 'clear':
        history.value = [];
        break;

      // 文件系统命令
      case 'ls':
        await handleLs(args);
        break;
      case 'cd':
        await handleCd(args);
        break;
      case 'pwd':
        handlePwd();
        break;
      case 'cat':
        await handleCat(args);
        break;
      case 'mkdir':
        await handleMkdir(args);
        break;
      case 'touch':
        await handleTouch(args);
        break;
      case 'rm':
        await handleRm(args);
        break;
      case 'echo':
        handleEcho(args);
        break;

      // 进程管理命令
      case 'ps':
        await handlePs();
        break;
      case 'kill':
        await handleKill(args);
        break;
      case 'run':
        await handleRun(args);
        break;

      // 内存管理命令
      case 'free':
        await handleFree();
        break;

      // 设备管理命令
      case 'lsdev':
        await handleLsdev();
        break;

      // 中断管理命令
      case 'register':
        await handleRegister(args);
        break;
      case 'trigger':
        await handleTrigger(args);
        break;
      case 'inttable':
        await handleIntTable();
        break;
      case 'int':
        await handleInt(args);
        break;

      // 时钟管理命令
      case 'time':
        await handleTime();
        break;
      case 'timer':
        await handleTimer(args);
        break;

      // 其他系统命令
      case 'uname':
        handleUname();
        break;
      case 'uptime':
        await handleUptime();
        break;
      case 'whoami':
        handleWhoami();
        break;
      case 'date':
        await handleDate();
        break;
      case 'schedule':
        await handleSchedule(args);
        break;
      case 'top':
        await handleTop();
        break;

      default:
        addToHistory('error', `Command not found: ${cmd}. Type 'help' for available commands.`);
    }
  } catch (err: any) {
    addToHistory('error', err.response?.data?.message || err.message || 'An unknown error occurred');
  }
};

const handleRegister = async (args: string[]) => {
  if (args.length < 2) throw new Error('Usage: register <vector> <type> [priority]');
  const vector = parseInt(args[0]);
  const type = args[1];
  const priority = args[2] ? parseInt(args[2]) : 0;
  if (isNaN(vector)) throw new Error('Invalid vector number');

  const res = await interruptAPI.setHandler(vector, type);
  addToHistory('response', res.data);
};

const handleTrigger = async (args: string[]) => {
  if (args.length < 1) throw new Error('Usage: trigger <vector> [json_data]');
  const vector = parseInt(args[0]);
  if (isNaN(vector)) throw new Error('Invalid vector number');
  let data = {};
  if (args.length > 1) {
      try {
          data = JSON.parse(args.slice(1).join(' '));
      } catch {
          throw new Error('Invalid JSON data');
      }
  }
  const res = await interruptAPI.triggerInterrupt(vector, data);
  addToHistory('response', res.data);
};

const handleIntTable = async () => {
  try {
  const res = await interruptAPI.getInterruptTable();
    if (res.data.status === 'success') {
      const vectors = res.data.data.vectors;
      if (vectors && vectors.length > 0) {
        let output = 'Vector  Type       Priority  Status\n';
        output += '------  ---------  --------  --------\n';
        vectors.forEach((vector: any) => {
          const vectorNum = vector.vector.toString().padEnd(6);
          const type = vector.handler_type.padEnd(9);
          const priority = vector.priority.toString().padEnd(8);
          const status = vector.enabled ? 'ENABLED' : 'DISABLED';
          output += `${vectorNum}  ${type}  ${priority}  ${status}\n`;
        });
        addToHistory('response', output.trim());
      } else {
        addToHistory('info', 'No interrupt handlers registered');
      }
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `Failed to get interrupt table: ${errorMsg}`);
  }
};

// 简化的中断命令
const handleInt = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('info', 'Usage: int <command> [args...]\nCommands: list, register, trigger, simulate');
    return;
  }

  const subCmd = args[0].toLowerCase();
  const subArgs = args.slice(1);

  switch (subCmd) {
    case 'list':
      await handleIntTable();
      break;
    case 'register':
      await handleRegister(subArgs);
      break;
    case 'trigger':
      await handleTrigger(subArgs);
      break;
    case 'simulate':
      await handleIntSimulate(subArgs);
      break;
    default:
      addToHistory('error', `int: unknown command '${subCmd}'`);
  }
};

// 模拟中断场景
const handleIntSimulate = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('info', 'Available simulations: timer, keyboard, disk, network, syscall');
    return;
  }

  const scenario = args[0].toLowerCase();
  try {
    switch (scenario) {
      case 'timer':
        // 模拟时钟中断
        await interruptAPI.setHandler(32, 'TIMER');
        await interruptAPI.triggerInterrupt(32, { tick_count: Date.now() });
        addToHistory('info', 'Simulated timer interrupt (vector 32)');
        break;
      case 'keyboard':
        // 模拟键盘中断
        await interruptAPI.setHandler(33, 'IO');
        await interruptAPI.triggerInterrupt(33, { device: 'keyboard', key_code: 65 });
        addToHistory('info', 'Simulated keyboard interrupt (vector 33)');
        break;
      case 'disk':
        // 模拟磁盘中断
        await interruptAPI.setHandler(46, 'IO');
        await interruptAPI.triggerInterrupt(46, { device: 'disk', operation: 'read_complete' });
        addToHistory('info', 'Simulated disk I/O interrupt (vector 46)');
        break;
      case 'network':
        // 模拟网络中断
        await interruptAPI.setHandler(47, 'IO');
        await interruptAPI.triggerInterrupt(47, { device: 'network', packet_received: true });
        addToHistory('info', 'Simulated network interrupt (vector 47)');
        break;
      case 'syscall':
        // 模拟系统调用中断
        await interruptAPI.setHandler(0x80, 'SYSCALL');
        await interruptAPI.triggerInterrupt(0x80, { syscall_number: 1, process_id: 123 });
        addToHistory('info', 'Simulated system call interrupt (vector 128)');
        break;
      default:
        addToHistory('error', `int simulate: unknown scenario '${scenario}'`);
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `Failed to simulate interrupt: ${errorMsg}`);
  }
};

// 显示帮助信息
const showHelp = () => {
  const helpText = `Available commands:

File System:
  ls [path]           - List directory contents
  cd <path>          - Change directory
  pwd                - Print working directory
  cat <file>         - Display file contents
  mkdir <dir>        - Create directory
  touch <file>       - Create empty file
  rm <path>          - Delete file or directory
  echo <text>        - Print text

Process Management:
  ps                 - List running processes
  kill <pid>         - Terminate process
  run <program> <memory> - Create new process
  top                - Show system resources and processes

System Information:
  free               - Show memory usage
  lsdev              - List devices
  time               - Show current time
  timer <delay>      - Set timer
  date               - Show current date and time
  uname              - Show system info
  uptime             - Show system uptime
  whoami             - Show current user

Scheduler Management:
  schedule tick      - Execute one scheduling tick
  schedule queue     - Show ready queue
  schedule status    - Show process status

Interrupt Management:
  register <vector> <type> [priority] - Register interrupt handler
  trigger <vector> [data]             - Trigger interrupt
  inttable                           - Show interrupt table
  int <cmd> [args]                   - Interrupt management commands
    int list                         - List interrupt vectors
    int register <vector> <type>     - Register handler
    int trigger <vector> [data]      - Trigger interrupt
    int simulate <scenario>          - Simulate interrupt scenarios
      Available scenarios: timer, keyboard, disk, network, syscall

General:
  help               - Show this message
  clear              - Clear terminal`;

  addToHistory('info', helpText);
};

// 文件系统命令实现
const handleLs = async (args: string[]) => {
  const path = args[0] || currentDirectory.value;
  try {
    const res = await filesystemAPI.listDirectory(path);
    if (res.data.status === 'success') {
      const items = res.data.data;
      let output = '';

      if (items && items.length > 0) {
        // 按类型排序：目录在前，文件在后
        const sortedItems = items.sort((a: any, b: any) => {
          if (a.type === 'directory' && b.type === 'file') return -1;
          if (a.type === 'file' && b.type === 'directory') return 1;
          return a.name.localeCompare(b.name);
        });

        sortedItems.forEach((item: any) => {
          const icon = item.type === 'directory' ? '📁' : '📄';
          const size = item.type === 'directory' ? '<DIR>' : formatFileSize(item.size || 0);
          const permissions = formatPermissions(item.permissions || 0);
          const modified = formatDateTime(item.modified_at || item.created_at);

          output += `${icon} ${item.name.padEnd(20)} ${size.padStart(8)} ${permissions} ${modified}\n`;
        });

        addToHistory('response', output.trim());
      } else {
        addToHistory('info', 'Directory is empty');
      }
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `ls: cannot access '${path}': ${errorMsg}`);
  }
};

const handleCd = async (args: string[]) => {
  if (args.length === 0) {
    currentDirectory.value = '/';
    addToHistory('info', 'Changed to root directory');
    return;
  }

  let newPath = args[0];

  // 处理相对路径
  if (!newPath.startsWith('/')) {
    if (currentDirectory.value === '/') {
      newPath = '/' + newPath;
    } else {
      newPath = currentDirectory.value + '/' + newPath;
    }
  }

  // 处理 .. (上级目录)
  if (newPath.includes('..')) {
    const parts = newPath.split('/').filter(p => p !== '');
    const newParts: string[] = [];
    for (const part of parts) {
      if (part === '..') {
        newParts.pop();
      } else {
        newParts.push(part);
      }
    }
    newPath = '/' + newParts.join('/');
  }

  // 规范化路径
  if (newPath !== '/' && newPath.endsWith('/')) {
    newPath = newPath.slice(0, -1);
  }

  try {
    // 验证目录是否存在
    await filesystemAPI.listDirectory(newPath);
    currentDirectory.value = newPath;
    addToHistory('info', `Changed directory to ${newPath}`);
  } catch (error) {
    addToHistory('error', `cd: ${newPath}: No such file or directory`);
  }
};

const handlePwd = () => {
  addToHistory('response', currentDirectory.value);
};

const handleCat = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'cat: missing file operand');
    return;
  }

  let filePath = args[0];
  if (!filePath.startsWith('/')) {
    filePath = currentDirectory.value === '/' ? '/' + filePath : currentDirectory.value + '/' + filePath;
  }

  try {
    const res = await filesystemAPI.readFile(filePath);
    if (res.data.status === 'success') {
      addToHistory('response', res.data.data.content || '(empty file)');
    }
  } catch (error) {
    addToHistory('error', `cat: ${filePath}: No such file or directory`);
  }
};

const handleMkdir = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'mkdir: missing operand');
    return;
  }

  let dirPath = args[0];
  if (!dirPath.startsWith('/')) {
    dirPath = currentDirectory.value === '/' ? '/' + dirPath : currentDirectory.value + '/' + dirPath;
  }

  try {
    await filesystemAPI.createDirectory(dirPath);
    addToHistory('info', `Directory created: ${dirPath}`);
  } catch (error) {
    addToHistory('error', `mkdir: cannot create directory '${dirPath}'`);
  }
};

const handleTouch = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'touch: missing operand');
    return;
  }

  let filePath = args[0];
  if (!filePath.startsWith('/')) {
    filePath = currentDirectory.value === '/' ? '/' + filePath : currentDirectory.value + '/' + filePath;
  }

  try {
    // 根据API文档，createFile需要模拟大小参数
    await filesystemAPI.createFile(filePath, 0, 0o644);
    addToHistory('info', `File created: ${filePath}`);
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `touch: cannot create file '${filePath}': ${errorMsg}`);
  }
};

const handleRm = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'rm: missing operand');
    return;
  }

  let targetPath = args[0];
  if (!targetPath.startsWith('/')) {
    targetPath = currentDirectory.value === '/' ? '/' + targetPath : currentDirectory.value + '/' + targetPath;
  }

  try {
    await filesystemAPI.delete(targetPath);
    addToHistory('info', `Removed: ${targetPath}`);
  } catch (error) {
    addToHistory('error', `rm: cannot remove '${targetPath}': No such file or directory`);
  }
};

const handleEcho = (args: string[]) => {
  const text = args.join(' ');
  addToHistory('response', text);
};

// 进程管理命令
const handlePs = async () => {
  try {
    const res = await processAPI.getProcesses();
    if (res.data.status === 'success') {
      const processes = res.data.data;
      if (processes && processes.length > 0) {
        let output = 'PID    STATE      MEMORY    PC\n';
        output += '----   --------   --------  --------\n';
      processes.forEach((proc: any) => {
        const pid = proc.pid.toString().padEnd(6);
          const state = proc.state.padEnd(10);
          const memory = proc.memory_info?.length > 0 ?
            formatFileSize(proc.memory_info[0].size) : 'N/A';
          const pc = proc.program_counter?.toString() || 'N/A';
          output += `${pid} ${state} ${memory.padEnd(8)} ${pc}\n`;
      });
      addToHistory('response', output.trim());
      } else {
        addToHistory('info', 'No processes running');
      }
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `Failed to list processes: ${errorMsg}`);
  }
};

const handleKill = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'kill: missing process ID');
    return;
  }

  const pid = parseInt(args[0]);
  if (isNaN(pid)) {
    addToHistory('error', 'kill: invalid process ID');
    return;
  }

  try {
    await processAPI.terminateProcess(pid);
    addToHistory('info', `Process ${pid} terminated`);
  } catch (error) {
    addToHistory('error', `kill: failed to terminate process ${pid}`);
  }
};

const handleRun = async (args: string[]) => {
  if (args.length < 2) {
    addToHistory('error', 'run: usage: run <program> <memory_size>');
    return;
  }

  const program = args[0];
  const memorySize = parseInt(args[1]);

  if (isNaN(memorySize)) {
    addToHistory('error', 'run: invalid memory size');
    return;
  }

  try {
          const res = await processAPI.createProcess(program, memorySize);
    if (res.data.status === 'success') {
      addToHistory('info', `Process created: ${program} (PID: ${res.data.data.pid})`);
    }
  } catch (error) {
    addToHistory('error', `run: failed to create process ${program}`);
  }
};

// 内存管理命令
const handleFree = async () => {
  try {
    const res = await memoryAPI.getStatus();
    if (res.data.status === 'success') {
      const memory = res.data.data;
      let output = 'Memory Status:\n';
      output += `Total:     ${memory.total_memory} KB\n`;
      output += `Used:      ${memory.used_memory} KB\n`;
      output += `Free:      ${memory.free_memory} KB\n`;
      output += `Usage:     ${memory.usage_percentage}%\n`;

      if (memory.allocations && memory.allocations.length > 0) {
        output += '\nAllocations:\n';
        memory.allocations.forEach((alloc: any) => {
          output += `  PID ${alloc.process_id}: ${alloc.size} KB\n`;
        });
      }

      addToHistory('response', output.trim());
    }
  } catch (error) {
    addToHistory('error', 'Failed to get memory status');
  }
};

// 设备管理命令
const handleLsdev = async () => {
  try {
    const res = await deviceAPI.getDevices();
    if (res.data.status === 'success') {
      const devices = res.data.data;
      let output = 'DEVICE    TYPE           STATUS    OWNER\n';
      output += '--------- -------------- --------- --------\n';
      devices.forEach((dev: any) => {
        const device = dev.device_id.toString().padEnd(9);
        const type = dev.type.padEnd(14);
        const status = dev.status.padEnd(9);
        const owner = dev.owner_process_id ? dev.owner_process_id.toString() : 'none';
        output += `${device} ${type} ${status} ${owner}\n`;
      });
      addToHistory('response', output.trim());
    }
  } catch (error) {
    addToHistory('error', 'Failed to list devices');
  }
};

// 时钟管理命令
const handleTime = async () => {
  try {
    const res = await clockAPI.getTime();
    if (res.data.status === 'success') {
      const time = res.data.data;
      addToHistory('response', `Current time: ${time.ticks} ticks (${time.uptime}ms uptime)\nReal time: ${time.real_time}`);
    }
  } catch (error) {
    addToHistory('error', 'Failed to get current time');
  }
};

const handleTimer = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'timer: missing delay argument');
    addToHistory('info', 'Usage: timer <delay> [repeat] [interval]');
    return;
  }

  const delay = parseInt(args[0]);
  if (isNaN(delay)) {
    addToHistory('error', 'timer: invalid delay value');
    return;
  }

  const repeat = args[1] === 'true' || args[1] === '1';
  const interval = args[2] ? parseInt(args[2]) : undefined;

  try {
    if (args.length === 1 && args[0] === 'interval') {
      // 特殊情况：设置时钟间隔
      const newInterval = parseInt(args[1]) || 100;
      await clockAPI.setInterval(newInterval);
      addToHistory('info', `Clock interval set to ${newInterval} ms`);
    } else {
      // 设置定时器
      const res = await clockAPI.setTimer(delay, repeat, interval);
      if (res.data.status === 'success') {
        const timerId = res.data.data.timer_id;
        addToHistory('info', `Timer ${timerId} set for ${delay}ms${repeat ? ' (repeating)' : ''}`);
      }
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `Failed to set timer: ${errorMsg}`);
  }
};

// 系统信息命令
const handleUname = () => {
  addToHistory('response', 'OS-Simulator v1.0.0 (Virtual Operating System)');
};

const handleUptime = async () => {
  try {
    const res = await clockAPI.getTime();
    if (res.data.status === 'success') {
      const uptime = res.data.data.uptime;
      const seconds = Math.floor(uptime / 1000);
      const minutes = Math.floor(seconds / 60);
      const hours = Math.floor(minutes / 60);

      let uptimeStr = '';
      if (hours > 0) uptimeStr += `${hours}h `;
      if (minutes % 60 > 0) uptimeStr += `${minutes % 60}m `;
      uptimeStr += `${seconds % 60}s`;

      addToHistory('response', `System uptime: ${uptimeStr} (${uptime}ms)`);
    }
  } catch (error) {
    addToHistory('error', 'Failed to get system uptime');
  }
};

const handleWhoami = () => {
  addToHistory('response', 'root');
};

const handleDate = async () => {
  try {
    const res = await clockAPI.getTime();
    if (res.data.status === 'success') {
      const realTime = res.data.data.real_time;
      const date = new Date(realTime);
      addToHistory('response', date.toLocaleString('zh-CN'));
    }
  } catch (error) {
    addToHistory('response', new Date().toLocaleString('zh-CN'));
  }
};

const handleSchedule = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('info', 'Usage: schedule <command>\nCommands: tick, queue, status');
    return;
  }

  const subCmd = args[0].toLowerCase();
  try {
    switch (subCmd) {
      case 'tick':
        const res = await schedulerAPI.tick();
        if (res.data.status === 'success') {
          if (res.data.data) {
            addToHistory('info', `Scheduled process ${res.data.data.pid} to run`);
          } else {
            addToHistory('info', 'No process to schedule (ready queue is empty)');
          }
        }
        break;
      case 'queue':
        const queueRes = await schedulerAPI.getReadyQueue();
        if (queueRes.data.status === 'success') {
          const queue = queueRes.data.data;
          if (queue && queue.length > 0) {
            let output = 'Ready Queue:\n';
            queue.forEach((proc: any, index: number) => {
              output += `${index + 1}. PID ${proc.pid} (${proc.state})\n`;
            });
            addToHistory('response', output.trim());
          } else {
            addToHistory('info', 'Ready queue is empty');
          }
        }
        break;
      case 'status':
        await handlePs();
        break;
      default:
        addToHistory('error', `schedule: unknown command '${subCmd}'`);
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `schedule: ${errorMsg}`);
  }
};

const handleTop = async () => {
  try {
    const [processRes, memoryRes] = await Promise.all([
      processAPI.getProcesses(),
      memoryAPI.getStatus()
    ]);

    let output = 'System Resources:\n';

    // 内存信息
    if (memoryRes.data.status === 'success') {
      const memory = memoryRes.data.data;
      const totalMB = Math.round(memory.total_memory / 1024 / 1024);
      const usedMB = Math.round(memory.used_memory / 1024 / 1024);
      const freeMB = totalMB - usedMB;
      const usage = Math.round((memory.used_memory / memory.total_memory) * 100);

      output += `Memory: ${usedMB}MB used, ${freeMB}MB free, ${totalMB}MB total (${usage}%)\n\n`;
    }

    // 进程信息
    if (processRes.data.status === 'success') {
      const processes = processRes.data.data;
      output += 'PID    STATE      MEMORY    PC\n';
      output += '----   --------   --------  --------\n';
      processes.forEach((proc: any) => {
        const pid = proc.pid.toString().padEnd(6);
        const state = proc.state.padEnd(10);
        const memory = proc.memory_info?.length > 0 ?
          formatFileSize(proc.memory_info[0].size) : 'N/A';
        const pc = proc.program_counter.toString();
        output += `${pid} ${state} ${memory.padEnd(8)} ${pc}\n`;
      });
    }

    addToHistory('response', output.trim());
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `top: ${errorMsg}`);
  }
};

const addToHistory = (type: HistoryItem['type'], content: string | object) => {
  history.value.push({ id: historyId++, type, content });
  scrollToBottom();
};

const scrollToBottom = () => {
  nextTick(() => {
    if (historyEl.value) {
      historyEl.value.scrollTop = historyEl.value.scrollHeight;
    }
  });
};

// 格式化文件大小
const formatFileSize = (bytes: number): string => {
  if (bytes >= 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024 * 1024)).toFixed(1)}G`;
  if (bytes >= 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(1)}M`;
  if (bytes >= 1024) return `${(bytes / 1024).toFixed(1)}K`;
  return `${bytes}B`;
};

// 格式化权限
const formatPermissions = (permissions: number): string => {
  if (!permissions) return '---';
  const octal = permissions.toString(8);
  return octal.length > 3 ? octal.slice(-3) : octal.padStart(3, '0');
};

// 格式化日期时间
const formatDateTime = (dateStr: string): string => {
  if (!dateStr) return '--------';
  try {
    const date = new Date(dateStr);
    return date.toLocaleDateString('zh-CN') + ' ' + date.toLocaleTimeString('zh-CN', { hour12: false }).slice(0, 5);
  } catch {
    return '--------';
  }
};
</script>

<template>
  <div class="terminal" @click="$refs.inputRef.focus()">
    <div ref="historyEl" class="history">
      <div v-for="item in history" :key="item.id" :class="`history-item ${item.type}`">
        <pre v-if="typeof item.content === 'object'">{{ JSON.stringify(item.content, null, 2) }}</pre>
        <span v-else>{{ item.content }}</span>
      </div>
    </div>
    <div class="input-line">
      <span class="prompt">[{{ currentDirectory }}]></span>
      <input
        ref="inputRef"
        type="text"
        v-model="command"
        @keydown.enter="executeCommand"
        autofocus
      />
    </div>
  </div>
</template>

<style scoped>
.terminal {
  width: 100%;
  height: 100%;
  background-color: black;
  color: #eee;
  font-family: 'Consolas', 'Courier New', Courier, monospace;
  font-size: 14px;
  padding: 10px;
  display: flex;
  flex-direction: column;
  box-sizing: border-box;
}
.history {
  flex-grow: 1;
  overflow-y: auto;
  margin-bottom: 10px;
}
.history-item {
  white-space: pre-wrap;
  word-break: break-all;
}
.history-item.command {
  color: #87d7ff;
}
.history-item.response {
  color: #a6e22e;
}
.history-item.error {
  color: #f92672;
}
.history-item.info {
  color: #999;
}
.input-line {
  display: flex;
  align-items: center;
}
.input-line > span.prompt {
  margin-right: 5px;
  color: #87d7ff;
  font-weight: bold;
}
input {
  flex-grow: 1;
  background: transparent;
  border: none;
  color: #eee;
  font-family: inherit;
  font-size: inherit;
}
input:focus {
  outline: none;
}
</style>
