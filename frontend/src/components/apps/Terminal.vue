<script setup lang="ts">
import { ref, nextTick } from 'vue';
import { interruptAPI, filesystemAPI, processAPI, memoryAPI, deviceAPI, clockAPI, schedulerAPI } from '@/services/api';
import { useWindowsStore } from '@/stores/windows';

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

// 窗口管理器
const windowsStore = useWindowsStore();

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
      case 'request':
        await handleRequest(args);
        break;
      case 'release':
        await handleRelease(args);
        break;
      case 'deldev':
        await handleDeldev(args);
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
      case 'gantt':
        await handleGantt();
        break;
      case 'config':
        await handleConfig(args);
        break;
      case 'fork':
        await handleFork(args);
        break;
      case 'relation':
        await handleRelation(args);
        break;
      case 'memstrategy':
        await handleMemStrategy(args);
        break;
      case 'fsconfig':
        await handleFsConfig(args);
        break;
      case 'fileaddr':
        await handleFileAddr(args);
        break;
      case 'exec':
        await handleExec(args);
        break;
      case 'start':
        await handleStart(args);
        break;

      default:
        // 尝试将命令作为.pubt文件执行
        if (await tryExecutePubtFile(cmd)) {
          // 成功执行.pubt文件，无需其他操作
        } else {
          addToHistory('error', `Command not found: ${cmd}. Type 'help' for available commands.`);
        }
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
  ls [path]             - List directory contents
  cd <path>            - Change directory
  pwd                  - Print working directory
  cat <file>           - Display file contents
  mkdir <dir>          - Create directory
  touch <file>         - Create empty file
  rm <path>            - Delete file or directory
  echo <text>          - Print text
  fileaddr <file>      - Show file allocation addresses
  fsconfig [method]    - Show/set file system allocation method

Process Management:
  ps                   - List running processes
  kill <pid>           - Terminate process
  run <memory> [cpu] [priority] - Create new process
  fork <parent> <memory> [cpu] [priority] [name] - Create child process
  exec <pubt_file>     - Execute .pubt program file
  start <pubt_file>    - Start .pubt program file (alias for exec)
  top                  - Show system resources and processes
  relation <cmd>       - Process relationship management
    relation list      - List process relationships
    relation create <pid1> <pid2> <type> - Create SYNC/MUTEX relation

Memory Management:
  free                 - Show detailed memory usage
  memstrategy <num>    - Set memory allocation strategy (0/1/2)

Device Management:
  lsdev                - List all devices
  request <dev> <pid>  - Request device for process
  release <dev> <pid>  - Release device from process
  deldev <device_id>   - Delete idle device

Scheduler Management:
  schedule tick        - Execute one scheduling tick
  schedule queue       - Show ready queue
  schedule status      - Show process status
  gantt                - Show scheduling gantt chart
  config scheduler [alg] [slice] - Show/set scheduler config

System Information:
  time                 - Show current time
  timer <delay> [repeat] [interval] - Set timer
  date                 - Show current date and time
  uname                - Show system info
  uptime               - Show system uptime
  whoami               - Show current user

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
  help                 - Show this message
  clear                - Clear terminal

Notes:
  - Memory strategies: 0=Continuous, 1=Partitioned, 2=Paged
  - File system methods: CONTIGUOUS, LINKED, INDEXED
  - Scheduler algorithms: FCFS, SJF, PRIORITY, RR
  - Process relation types: SYNC, MUTEX
  - You can directly run .pubt programs by typing their name (e.g., "在线音乐")
  - .pubt files will auto-launch corresponding GUI applications or create processes`;

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
  if (args.length < 1) {
    addToHistory('error', 'run: usage: run <memory_size> [cpu_time] [priority]');
    return;
  }

  const memorySize = parseInt(args[0]);
  const cpuTime = args[1] ? parseInt(args[1]) : 1000;
  const priority = args[2] ? parseInt(args[2]) : 5;

  if (isNaN(memorySize)) {
    addToHistory('error', 'run: invalid memory size');
    return;
  }

  try {
    const res = await processAPI.createProcess(memorySize, cpuTime, priority);
    if (res.data.status === 'success') {
      const process = res.data.data;
      addToHistory('info', `Process created: PID ${process.pid} (Memory: ${formatFileSize(memorySize)}, CPU: ${cpuTime}ms, Priority: ${priority})`);
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `run: ${errorMsg}`);
  }
};

// 内存管理命令
const handleFree = async () => {
  try {
    const res = await memoryAPI.getStatus();
    if (res.data.status === 'success') {
      const memory = res.data.data;
      let output = 'Memory Status:\n';
      output += `Total:     ${formatFileSize(memory.total_memory)}\n`;
      output += `Used:      ${formatFileSize(memory.used_memory)}\n`;
      output += `Free:      ${formatFileSize(memory.total_memory - memory.used_memory)}\n`;
      output += `Strategy:  ${getStrategyName(memory.allocation_strategy)}\n\n`;

      // 显示不同策略的详细信息
      if (memory.allocation_strategy === 0 && memory.free_blocks) {
        output += 'Free Blocks:\n';
        memory.free_blocks.forEach((block: any, index: number) => {
          output += `  Block ${index + 1}: ${formatFileSize(block.size)} at 0x${block.base_address.toString(16)}\n`;
        });
      }

      if (memory.allocation_strategy === 1 && memory.partitions) {
        output += 'Partitions:\n';
        memory.partitions.forEach((partition: any, index: number) => {
          const status = partition.is_free ? 'FREE' : `PID ${partition.owner_pid}`;
          output += `  Part ${index + 1}: ${formatFileSize(partition.size)} at 0x${partition.base_address.toString(16)} (${status})\n`;
        });
      }

      if (memory.allocation_strategy === 2 && memory.paging) {
        output += 'Paging Information:\n';
        output += `  Total Pages: ${memory.paging.total_pages}\n`;
        output += `  Used Pages:  ${memory.paging.used_pages}\n`;
        output += `  Free Pages:  ${memory.paging.free_pages}\n`;
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
      let output = 'ID       NAME            TYPE           STATUS    OWNER\n';
      output += '-------- --------------- -------------- --------- --------\n';
      devices.forEach((dev: any) => {
        const id = dev.device_id.toString().padEnd(8);
        const name = dev.name.padEnd(15);
        const type = dev.type.padEnd(14);
        const status = dev.status.padEnd(9);
        const owner = dev.current_user ? dev.current_user.toString() : 'none';
        output += `${id} ${name} ${type} ${status} ${owner}\n`;
      });
      addToHistory('response', output.trim());
    }
  } catch (error) {
    addToHistory('error', 'Failed to list devices');
  }
};

// 申请设备
const handleRequest = async (args: string[]) => {
  if (args.length < 2) {
    addToHistory('error', 'request: usage: request <device_id> <process_id>');
    return;
  }

  const deviceId = parseInt(args[0]);
  const processId = parseInt(args[1]);

  if (isNaN(deviceId) || isNaN(processId)) {
    addToHistory('error', 'request: invalid device ID or process ID');
    return;
  }

  try {
    const res = await deviceAPI.requestDevice(deviceId, processId);
    if (res.data.status === 'success') {
      const device = res.data.data;
      addToHistory('info', `Device ${device.name} (ID: ${deviceId}) allocated to process ${processId}`);
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `request: ${errorMsg}`);
  }
};

// 释放设备
const handleRelease = async (args: string[]) => {
  if (args.length < 2) {
    addToHistory('error', 'release: usage: release <device_id> <process_id>');
    return;
  }

  const deviceId = parseInt(args[0]);
  const processId = parseInt(args[1]);

  if (isNaN(deviceId) || isNaN(processId)) {
    addToHistory('error', 'release: invalid device ID or process ID');
    return;
  }

  try {
    await deviceAPI.releaseDevice(deviceId, processId);
    addToHistory('info', `Device ${deviceId} released from process ${processId}`);
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `release: ${errorMsg}`);
  }
};

// 删除设备
const handleDeldev = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'deldev: missing device ID');
    return;
  }

  const deviceId = parseInt(args[0]);
  if (isNaN(deviceId)) {
    addToHistory('error', 'deldev: invalid device ID');
    return;
  }

  try {
    await deviceAPI.deleteDevice(deviceId);
    addToHistory('info', `Device ${deviceId} deleted successfully`);
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `deldev: ${errorMsg}`);
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

// 甘特图命令
const handleGantt = async () => {
  try {
    const res = await schedulerAPI.getGanttChart();
    if (res.data.status === 'success') {
      const chart = res.data.data;
      if (chart && chart.length > 0) {
        let output = 'Gantt Chart:\n';
        output += 'PID    START    END      DURATION\n';
        output += '----   -----    -----    --------\n';
        chart.forEach((entry: any) => {
          const pid = entry.pid.toString().padEnd(6);
          const start = entry.start.toString().padEnd(8);
          const end = entry.end.toString().padEnd(8);
          const duration = (entry.end - entry.start).toString();
          output += `${pid} ${start} ${end} ${duration}ms\n`;
        });
        addToHistory('response', output.trim());
      } else {
        addToHistory('info', 'No scheduling data available');
      }
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `gantt: ${errorMsg}`);
  }
};

// 配置命令
const handleConfig = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('info', 'config: usage: config <type> [options]\nTypes: scheduler');
    return;
  }

  const configType = args[0].toLowerCase();
  const subArgs = args.slice(1);

  try {
    switch (configType) {
      case 'scheduler':
        if (subArgs.length === 0) {
          // 查询调度器配置
          const res = await schedulerAPI.getConfig();
          if (res.data.status === 'success') {
            const config = res.data.data;
            addToHistory('response', `Scheduler Algorithm: ${config.algorithm}\nTime Slice: ${config.time_slice}ms`);
          }
        } else {
          // 设置调度器配置
          const algorithm = subArgs[0].toUpperCase();
          const timeSlice = subArgs[1] ? parseInt(subArgs[1]) : undefined;

          if (!['FCFS', 'SJF', 'PRIORITY', 'RR'].includes(algorithm)) {
            addToHistory('error', 'config scheduler: invalid algorithm. Must be FCFS, SJF, PRIORITY, or RR');
            return;
          }

          const res = await schedulerAPI.setConfig(algorithm, timeSlice);
          if (res.data.status === 'success') {
            addToHistory('info', `Scheduler configured: ${algorithm}${timeSlice ? ` (time slice: ${timeSlice}ms)` : ''}`);
          }
        }
        break;
      default:
        addToHistory('error', `config: unknown type '${configType}'`);
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `config: ${errorMsg}`);
  }
};

// fork命令 - 创建子进程
const handleFork = async (args: string[]) => {
  if (args.length < 2) {
    addToHistory('error', 'fork: usage: fork <parent_pid> <memory_size> [cpu_time] [priority] [name]');
    return;
  }

  const parentPid = parseInt(args[0]);
  const memorySize = parseInt(args[1]);
  const cpuTime = args[2] ? parseInt(args[2]) : 1000;
  const priority = args[3] ? parseInt(args[3]) : 5;
  const name = args[4] || undefined;

  if (isNaN(parentPid) || isNaN(memorySize)) {
    addToHistory('error', 'fork: invalid parent PID or memory size');
    return;
  }

  try {
    const res = await processAPI.createChildProcess(parentPid, memorySize, cpuTime, priority, name);
    if (res.data.status === 'success') {
      const child = res.data.data;
      addToHistory('info', `Child process created: PID ${child.pid} (parent: ${parentPid})`);
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `fork: ${errorMsg}`);
  }
};

// 进程关系命令
const handleRelation = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('info', 'relation: usage: relation <command> [args]\nCommands: list, create');
    return;
  }

  const subCmd = args[0].toLowerCase();
  const subArgs = args.slice(1);

  try {
    switch (subCmd) {
      case 'list':
        const res = await processAPI.getProcessRelationships();
        if (res.data.status === 'success') {
          const relations = res.data.data;
          if (relations && relations.length > 0) {
            let output = 'Process Relations:\n';
            output += 'PID1   PID2   TYPE\n';
            output += '----   ----   -------\n';
            relations.forEach((rel: any) => {
              const pid1 = rel.pid1.toString().padEnd(6);
              const pid2 = rel.pid2.toString().padEnd(6);
              output += `${pid1} ${pid2} ${rel.relation_type}\n`;
            });
            addToHistory('response', output.trim());
          } else {
            addToHistory('info', 'No process relations found');
          }
        }
        break;
      case 'create':
        if (subArgs.length < 3) {
          addToHistory('error', 'relation create: usage: create <pid1> <pid2> <type>');
          return;
        }
        const pid1 = parseInt(subArgs[0]);
        const pid2 = parseInt(subArgs[1]);
        const relationType = subArgs[2].toUpperCase();

        if (isNaN(pid1) || isNaN(pid2)) {
          addToHistory('error', 'relation create: invalid process IDs');
          return;
        }

        if (!['SYNC', 'MUTEX'].includes(relationType)) {
          addToHistory('error', 'relation create: type must be SYNC or MUTEX');
          return;
        }

        const createRes = await processAPI.createProcessRelationship(pid1, pid2, relationType);
        if (createRes.data.status === 'success') {
          addToHistory('info', `${relationType} relation created between PID ${pid1} and PID ${pid2}`);
        }
        break;
      default:
        addToHistory('error', `relation: unknown command '${subCmd}'`);
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `relation: ${errorMsg}`);
  }
};

// 内存分配策略命令
const handleMemStrategy = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('info', 'memstrategy: usage: memstrategy <strategy>\nStrategies: 0(continuous), 1(partitioned), 2(paged)');
    return;
  }

  const strategy = parseInt(args[0]);
  if (isNaN(strategy) || strategy < 0 || strategy > 2) {
    addToHistory('error', 'memstrategy: invalid strategy. Must be 0, 1, or 2');
    return;
  }

  try {
    const res = await memoryAPI.setStrategy(strategy);
    if (res.data.status === 'success') {
      const strategyName = getStrategyName(strategy);
      addToHistory('info', `Memory allocation strategy changed to: ${strategyName}`);
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `memstrategy: ${errorMsg}`);
  }
};

// 文件系统配置命令
const handleFsConfig = async (args: string[]) => {
  if (args.length === 0) {
    // 显示当前配置
    try {
      const res = await filesystemAPI.getStatus();
      if (res.data.status === 'success') {
        const status = res.data.data;
        let output = 'File System Status:\n';
        output += `Total Space:    ${formatFileSize(status.total_space)}\n`;
        output += `Used Space:     ${formatFileSize(status.used_space)}\n`;
        output += `Free Space:     ${formatFileSize(status.free_space)}\n`;
        output += `Files:          ${status.total_files}\n`;
        output += `Directories:    ${status.total_dirs}\n`;
        output += `Allocation:     ${status.allocation_method}\n`;
        addToHistory('response', output.trim());
      }
    } catch (error: any) {
      const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
      addToHistory('error', `fsconfig: ${errorMsg}`);
    }
    return;
  }

  const method = args[0].toUpperCase();
  if (!['CONTIGUOUS', 'LINKED', 'INDEXED'].includes(method)) {
    addToHistory('error', 'fsconfig: invalid method. Must be CONTIGUOUS, LINKED, or INDEXED');
    return;
  }

  try {
    await filesystemAPI.setConfig(method);
    addToHistory('info', `File system allocation method changed to: ${method}`);
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `fsconfig: ${errorMsg}`);
  }
};

// 文件地址查询命令
const handleFileAddr = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'fileaddr: missing file path');
    return;
  }

  let filePath = args[0];
  if (!filePath.startsWith('/')) {
    filePath = currentDirectory.value === '/' ? '/' + filePath : currentDirectory.value + '/' + filePath;
  }

  try {
    const res = await filesystemAPI.getFileAddress(filePath);
    if (res.data.status === 'success') {
      const data = res.data.data;
      let output = `File Address Information for: ${data.path}\n`;
      output += 'Allocation Strategy Addresses:\n';

      if (data.addresses.contiguous !== null) {
        output += `  Contiguous: Block ${data.addresses.contiguous}\n`;
      } else {
        output += `  Contiguous: Not allocated\n`;
      }

      if (data.addresses.linked !== null) {
        output += `  Linked:     Block ${data.addresses.linked}\n`;
      } else {
        output += `  Linked:     Not allocated\n`;
      }

      if (data.addresses.indexed !== null) {
        output += `  Indexed:    Index Block ${data.addresses.indexed}\n`;
      } else {
        output += `  Indexed:    Not allocated\n`;
      }

      addToHistory('response', output.trim());
    }
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `fileaddr: ${errorMsg}`);
  }
};

// .pubt程序启动命令
const handleExec = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'exec: missing .pubt file path');
    addToHistory('info', 'Usage: exec <pubt_file>');
    return;
  }

  let filePath = args[0];
  if (!filePath.startsWith('/')) {
    filePath = currentDirectory.value === '/' ? '/' + filePath : currentDirectory.value + '/' + filePath;
  }

  // 检查文件扩展名
  if (!filePath.toLowerCase().endsWith('.pubt')) {
    addToHistory('error', 'exec: file must have .pubt extension');
    return;
  }

  try {
    await handlePubtFile(filePath);
  } catch (error: any) {
    const errorMsg = error.response?.data?.message || error.message || 'Unknown error';
    addToHistory('error', `exec: ${errorMsg}`);
  }
};

// start命令 - exec的别名
const handleStart = async (args: string[]) => {
  if (args.length === 0) {
    addToHistory('error', 'start: missing .pubt file path');
    addToHistory('info', 'Usage: start <pubt_file>');
    return;
  }

  await handleExec(args);
};

// 处理.pubt文件执行（从桌面组件移植）
const handlePubtFile = async (filePath: string) => {
  try {
    // 读取.pubt文件信息获取模拟大小
    const response = await filesystemAPI.readFile(filePath);
    if (response.data.status === 'success') {
      // 使用文件的模拟大小作为内存大小，如果没有则使用默认值
      const memorySize = response.data.data.simulated_size || 1024; // 默认1KB

      // 创建进程，使用文件名（去掉.pubt扩展名）作为进程名
      const fileName = filePath.split('/').pop() || 'unnamed';
      const processName = fileName.replace(/\.pubt$/, '') || 'unnamed';

      // 根据程序名称启动对应的应用
      if (processName.toLowerCase().includes('音乐') || processName.toLowerCase().includes('music')) {
        // 启动音乐播放器
        windowsStore.openWindow('music-player', '在线音乐', 'MusicPlayer', {}, { center: true });
        addToHistory('info', `音乐播放器 "${processName}" 已启动`);
      } else if (processName.toLowerCase().includes('进程') || processName.toLowerCase().includes('process')) {
        // 启动进程管理器
        windowsStore.openWindow('process-manager', '进程管理器', 'ProcessManager', {}, { center: true });
        addToHistory('info', `进程管理器 "${processName}" 已启动`);
      } else if (processName.toLowerCase().includes('内存') || processName.toLowerCase().includes('memory')) {
        // 启动内存管理器
        windowsStore.openWindow('memory-manager', '内存管理器', 'MemoryManager', {}, { center: true });
        addToHistory('info', `内存管理器 "${processName}" 已启动`);
      } else if (processName.toLowerCase().includes('设备') || processName.toLowerCase().includes('device')) {
        // 启动设备管理器
        windowsStore.openWindow('device-manager', '设备管理器', 'DeviceManager', {}, { center: true });
        addToHistory('info', `设备管理器 "${processName}" 已启动`);
      } else if (processName.toLowerCase().includes('文件') || processName.toLowerCase().includes('file')) {
        // 启动文件管理器
        windowsStore.openWindow('file-manager', '文件管理器', 'FileManager', {}, { center: true });
        addToHistory('info', `文件管理器 "${processName}" 已启动`);
      } else if (processName.toLowerCase().includes('系统') || processName.toLowerCase().includes('system')) {
        // 启动系统控制
        windowsStore.openWindow('system-control', '系统控制', 'SystemControl', {}, { center: true });
        addToHistory('info', `系统控制 "${processName}" 已启动`);
      } else {
        // 其他应用创建进程，使用智能算法
        const randomPriority = Math.floor(Math.random() * 5) + 1;
        const calculatedCPUTime = Math.max(200, Math.min(1000, Math.floor(memorySize / 32)));

        const processResult = await processAPI.createProcess(
          memorySize,
          calculatedCPUTime,
          randomPriority
        );

        if (processResult.data.status === 'success') {
          const pid = processResult.data.data.pid;
          addToHistory('info', `程序 "${processName}" 已启动`);
          addToHistory('info', `进程ID: ${pid}, 内存: ${formatFileSize(memorySize)}, 优先级: ${randomPriority}, CPU时间: ${calculatedCPUTime}ms`);
        }
      }
    } else {
      throw new Error('无法读取.pubt文件信息');
    }
  } catch (err: any) {
    throw err; // 重新抛出错误，让上层处理
  }
};

// 尝试将命令作为.pubt文件执行
const tryExecutePubtFile = async (cmd: string): Promise<boolean> => {
  try {
    // 尝试不同的路径查找.pubt文件
    const possiblePaths = [
      // 当前目录
      currentDirectory.value === '/' ? `/${cmd}.pubt` : `${currentDirectory.value}/${cmd}.pubt`,
      // 根目录（桌面）
      `/${cmd}.pubt`,
      // 直接使用命令作为完整路径（如果用户输入了完整路径）
      cmd.endsWith('.pubt') ? (cmd.startsWith('/') ? cmd : currentDirectory.value === '/' ? `/${cmd}` : `${currentDirectory.value}/${cmd}`) : null
    ].filter(Boolean);

    for (const filePath of possiblePaths) {
      try {
        // 尝试读取文件以验证其存在
        const testResponse = await filesystemAPI.readFile(filePath as string);
        if (testResponse.data.status === 'success') {
          // 文件存在，执行它
          await handlePubtFile(filePath as string);
          return true;
        }
      } catch (err) {
        // 文件不存在，继续尝试下一个路径
        continue;
      }
    }

    return false; // 所有路径都尝试失败
  } catch (err) {
    return false;
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

// 获取策略名称
const getStrategyName = (strategy: number): string => {
  switch (strategy) {
    case 0: return 'Continuous';
    case 1: return 'Partitioned';
    case 2: return 'Paged';
    default: return 'Unknown';
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
