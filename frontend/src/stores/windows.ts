import { defineStore } from 'pinia';

export interface WindowState {
  id: string;
  title: string;
  component: string;
  props?: Record<string, any>;
  isVisible: boolean;
  isActive: boolean;
  isFocused: boolean;
  isMinimized: boolean;
  isMaximized: boolean;
  position: { x: number; y: number };
  size: { width: number; height: number };
  originalPosition?: { x: number; y: number };
  originalSize?: { width: number; height: number };
  zIndex: number;
}

export const useWindowsStore = defineStore('windows', {
  state: () => ({
    windows: [] as WindowState[],
    nextZIndex: 100,
  }),

  getters: {
    // 组件默认大小配置
    componentDefaults: () => ({
      'DeviceManager': { width: 1200, height: 800 },
      'FileManager': { width: 900, height: 600 },
      'ProcessManager': { width: 1200, height: 750 },
      'Terminal': { width: 800, height: 500 },
      'SystemControl': { width: 700, height: 500 },
      'MemoryManager': { width: 900, height: 600 }
    }),

    defaultSize: () => ({ width: 640, height: 480 })
  },

  actions: {
    // 计算居中位置的辅助方法
    getCenterPosition(width: number, height: number) {
      return {
        x: Math.max(0, (window.innerWidth - width) / 2),
        y: Math.max(0, (window.innerHeight - height - 48) / 2) // 48px是任务栏高度
      };
    },

    openWindow(id: string, title: string, component: string, props = {}, options?: {
      size?: { width: number; height: number };
      position?: { x: number; y: number };
      center?: boolean;
    }) {
        const existingWindow = this.windows.find(w => w.id === id);
        if (existingWindow) {
            this.focusWindow(id);
            return;
        }

        // 默认窗口大小
        const defaultSize = this.defaultSize;

        // 组件特定的默认大小
        const componentDefaults: Record<string, { width: number; height: number }> = this.componentDefaults;

        const windowSize = options?.size || componentDefaults[component] || defaultSize;

        // 计算默认位置
        let defaultPosition = { x: 100, y: 100 };

        if (options?.center) {
          defaultPosition = this.getCenterPosition(windowSize.width, windowSize.height);
        } else if (options?.position) {
          defaultPosition = options.position;
        } else {
          // 为每个新窗口添加一些偏移，避免完全重叠
          const windowCount = this.windows.length;
          defaultPosition = {
            x: 100 + (windowCount * 30),
            y: 100 + (windowCount * 30)
          };
        }

        const newWindow: WindowState = {
            id,
            title,
            component,
            props,
            isVisible: true,
            isActive: true,
            isFocused: true,
            isMinimized: false,
            isMaximized: false,
            position: defaultPosition,
            size: windowSize,
            originalPosition: undefined,
            originalSize: undefined,
            zIndex: this.nextZIndex++,
        };

        this.windows.forEach(w => {
            w.isActive = false;
            w.isFocused = false;
        });
        this.windows.push(newWindow);
    },

    closeWindow(id: string) {
        this.windows = this.windows.filter(w => w.id !== id);
    },

    focusWindow(id: string) {
        this.windows.forEach(w => {
            if (w.id === id) {
                w.isActive = true;
                w.isFocused = true;
                w.zIndex = this.nextZIndex++;
            } else {
                w.isActive = false;
                w.isFocused = false;
            }
        });
    },

    updateWindowPosition(id: string, position: { x: number; y: number }) {
        const win = this.windows.find(w => w.id === id);
        if (win) {
            win.position = position;
        }
    },

    updateWindowSize(id: string, size: { width: number; height: number }) {
        const win = this.windows.find(w => w.id === id);
        if (win) {
            win.size = size;
        }
    },

    updateWindowBounds(id: string, bounds: {
        position?: { x: number; y: number };
        size?: { width: number; height: number };
    }) {
        const win = this.windows.find(w => w.id === id);
        if (win) {
            if (bounds.position) {
                win.position = bounds.position;
            }
            if (bounds.size) {
                win.size = bounds.size;
            }
        }
    },

    minimizeWindow(id: string) {
        const win = this.windows.find(w => w.id === id);
        if (win && !win.isMinimized) {
            win.isMinimized = true;
            win.isActive = false;
            win.isFocused = false;

            // 如果当前没有其他活动窗口，激活最近的一个
            const activeWindows = this.windows.filter(w => !w.isMinimized && w.id !== id);
            if (activeWindows.length > 0) {
                const lastWindow = activeWindows.reduce((prev, current) =>
                    (prev.zIndex > current.zIndex) ? prev : current
                );
                this.focusWindow(lastWindow.id);
            }
        }
    },

    maximizeWindow(id: string) {
        const win = this.windows.find(w => w.id === id);
        if (win) {
            if (win.isMaximized) {
                // 恢复窗口 - 使用默认大小并居中显示
                const componentDefaults: Record<string, { width: number; height: number }> = this.componentDefaults;

                const defaultSize = componentDefaults[win.component] || this.defaultSize;

                // 计算居中位置
                const centerPosition = this.getCenterPosition(defaultSize.width, defaultSize.height);

                console.log('恢复窗口:', {
                    component: win.component,
                    defaultSize,
                    centerPosition,
                    screenSize: { width: window.innerWidth, height: window.innerHeight }
                });

                win.position = centerPosition;
                win.size = defaultSize;
                win.isMaximized = false;
                win.originalPosition = undefined;
                win.originalSize = undefined;
            } else {
                // 最大化窗口
                win.originalPosition = { ...win.position };
                win.originalSize = { ...win.size };
                win.position = { x: 0, y: 0 };
                win.size = {
                    width: window.innerWidth,
                    height: window.innerHeight - 48 // 48px是任务栏高度
                };
                win.isMaximized = true;
            }
            this.focusWindow(id);
        }
    },

    restoreWindow(id: string) {
        const win = this.windows.find(w => w.id === id);
        if (win && win.isMinimized) {
            win.isMinimized = false;
            this.focusWindow(id);
        }
    },
  },
});
